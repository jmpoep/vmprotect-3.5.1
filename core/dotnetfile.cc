#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "pefile.h"
#include "processors.h"
#include "il.h"
#include "dotnet.h"
#include "dotnetfile.h"
#include "core.h"
#include "lang.h"
#include "script.h"

static AssemblyResolver resolver;

std::string ILName(const std::string &ret, const std::string &type, const std::string &method, const std::string &signature)
{
	std::string res;
	if (!ret.empty())
		res += ret + ' ';
	if (!type.empty())
		res += type + "::";
	return res + method + signature;
}

/**
 * ILData
 */

ILData::ILData()
	: std::vector<uint8_t>()
{

}

ILData::ILData(const uint8_t *data, size_t size)
	: std::vector<uint8_t>()
{
	insert(begin(), data, data + size);
}

uint32_t ILData::ReadEncoded(uint32_t &pos) const
{
	if (pos >= size())
		throw std::runtime_error("Invalid index for stream");

	uint32_t res;
	uint8_t b = at(pos);
	if ((b & 0x80) == 0) {
		res = at(pos) & 0x7f;
		pos++;
	} else if ((b & 0x40) == 0) {
		res = ((at(pos) & 0x3f) << 8) + at(pos + 1);
		pos += 2;
	} else {
		res = ((at(pos) & 0x1f) << 24) + (at(pos + 1) << 16) + (at(pos + 2) << 8) + at(pos + 3);
		pos += 4;
	}
	return res;
}

void ILData::WriteEncoded(uint32_t value)
{
	if (value < 0x80) {
		WriteByte(static_cast<uint8_t>(value));
	} else if (value < 0x4000) {
		WriteByte(static_cast<uint8_t>(value >> 8) | 0x80);
		WriteByte(static_cast<uint8_t>(value));
	} else if (value < 0x02000000) {
		WriteByte(static_cast<uint8_t>(value >> 24) | 0xc0);
		WriteByte(static_cast<uint8_t>(value >> 16));
		WriteByte(static_cast<uint8_t>(value >> 8));
		WriteByte(static_cast<uint8_t>(value));
	} else {
		throw std::runtime_error("Value can not be encoded");
	}
}

void ILData::Read(uint32_t &pos, void *buffer, uint32_t size) const
{
	if (pos + size > this->size())
		throw std::runtime_error("Invalid index for stream");
	memcpy(buffer, data() + pos, size);
	pos += size;
}

uint8_t ILData::ReadByte(uint32_t &pos) const
{
	uint8_t res;
	Read(pos, &res, sizeof(res));
	return res;
}

uint16_t ILData::ReadWord(uint32_t &pos) const
{
	uint16_t res;
	Read(pos, &res, sizeof(res));
	return res;
}

uint32_t ILData::ReadDWord(uint32_t &pos) const
{
	uint32_t res;
	Read(pos, &res, sizeof(res));
	return res;
}

uint64_t ILData::ReadQWord(uint32_t &pos) const
{
	uint64_t res;
	Read(pos, &res, sizeof(res));
	return res;
}

std::string ILData::ReadString(uint32_t &pos) const
{
	std::string res;
	uint32_t size = ReadEncoded(pos);
	for (size_t i = 0; i < size; i++) {
		res.push_back(ReadByte(pos));
	}
	return res;
}

void ILData::WriteByte(uint8_t value)
{
	push_back(value);
}

void ILData::WriteWord(uint16_t value)
{
	Write(&value, sizeof(value));
}

void ILData::WriteDWord(uint32_t value)
{
	Write(&value, sizeof(value));
}

void ILData::WriteQWord(uint64_t value)
{
	Write(&value, sizeof(value));
}

void ILData::Write(const void *buffer, size_t size)
{
	if (size) {
		const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buffer);
		insert(end(), ptr, ptr + size);
	}
}

void ILData::WriteString(const std::string &value)
{
	WriteEncoded(static_cast<uint32_t>(value.size()));
	if (!value.empty())
		Write(value.data(), value.size());
}

/**
 * ILStream
 */

ILStream::ILStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: BaseLoadCommand(owner), address_(address), size_(size), name_(name)
{

}

ILStream::ILStream(ILMetaData *owner, const ILStream &src)
	: BaseLoadCommand(owner)
{
	address_ = src.address_;
	size_ = src.size_;
	name_ = src.name_;
}

ILStream *ILStream::Clone(ILoadCommandList *owner) const
{
	ILStream *stream = new ILStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

void ILStream::WriteToFile(NETArchitecture &file)
{
	address_ = file.AddressTell();
}

void ILStream::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

void ILStream::FreeByManager(MemoryManager &manager)
{
	manager.Add(address(), size());
}

/**
 * ILStringsStream
 */

ILStringsStream::ILStringsStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: ILStream(owner, address, size, name)
{

}

ILStringsStream::ILStringsStream(ILMetaData *owner, const ILStringsStream &src)
	: ILStream(owner, src)
{
	data_ = src.data_;
}

ILStream *ILStringsStream::Clone(ILoadCommandList *owner) const
{
	ILStringsStream *stream = new ILStringsStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

void ILStringsStream::ReadFromFile(NETArchitecture &file)
{
	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid stream address");

	data_.resize(size());
	file.Read(data_.data(), data_.size());
}

void ILStringsStream::WriteToFile(NETArchitecture &file)
{
	ILStream::WriteToFile(file);
	data_.resize(AlignValue(data_.size(), sizeof(uint32_t)), 0);
	set_size(static_cast<uint32_t>(file.Write(data_.data(), data_.size())));
}

void ILStringsStream::Prepare()
{
	data_.clear();
	map_.clear();
	AddString("");
}

std::string ILStringsStream::GetString(uint32_t pos) const
{
	if (pos >= data_.size())
		throw std::runtime_error("Invalid index for " + name());

	size_t len = data_.size() - pos;
	for (size_t i = 0; i < len; i++) {
		if (data_[pos + i] == 0) {
			len = i;
			break;
		}
	}
	if (len == data_.size() - pos)
		throw std::runtime_error("Invalid index for " + name());

	return std::string(reinterpret_cast<const char *>(data_.data() + pos), len);
}

uint32_t ILStringsStream::AddString(const std::string &str)
{
	std::map<std::string, uint32_t>::const_iterator it = map_.find(str);
	if (it != map_.end())
		return it->second;

	uint32_t res = static_cast<uint32_t>(data_.size());
	map_[str] = res;

	data_.insert(data_.end(), str.data(), str.data() + str.size() + 1);

	return res;
}

/**
 * ILUserStringsStream
 */

ILUserStringsStream::ILUserStringsStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: ILStream(owner, address, size, name)
{

}

ILUserStringsStream::ILUserStringsStream(ILMetaData *owner, const ILUserStringsStream &src)
	: ILStream(owner, src)
{
	data_ = src.data_;
}

ILUserStringsStream *ILUserStringsStream::Clone(ILoadCommandList *owner) const
{
	ILUserStringsStream *stream = new ILUserStringsStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

void ILUserStringsStream::ReadFromFile(NETArchitecture &file)
{
	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid stream address");

	data_.resize(size());
	file.Read(data_.data(), data_.size());
}

void ILUserStringsStream::WriteToFile(NETArchitecture &file)
{
	ILStream::WriteToFile(file);
	data_.resize(AlignValue(data_.size(), sizeof(uint32_t)), 0);
	set_size(static_cast<uint32_t>(file.Write(data_.data(), data_.size())));
}

void ILUserStringsStream::Prepare()
{
	data_.clear();
	data_.push_back(0);
	map_.clear();
}

std::string ILUserStringsStream::GetString(uint32_t pos) const
{
	uint32_t len = data_.ReadEncoded(pos);
	if (pos + len > data_.size())
		throw std::runtime_error("Invalid index for " + name());

	len >>= 1;
	if (!len)
		return std::string();

	os::unicode_string str;
	str.resize(len);
	memcpy(&str[0], &data_[pos], str.size() * sizeof(os::unicode_char));

	return os::ToUTF8(str);
}

ILData ILUserStringsStream::GetData(uint32_t pos, uint64_t *address) const
{
	uint32_t len = data_.ReadEncoded(pos);
	if (pos + len > data_.size())
		throw std::runtime_error("Invalid index for " + name());

	if (len & 1)
		len--;
	if (address)
		*address = this->address() + pos;
	return ILData(&data_[pos], len);
}

uint32_t ILUserStringsStream::AddString(const std::string &str)
{
	std::map<std::string, uint32_t>::const_iterator it = map_.find(str);
	if (it != map_.end())
		return it->second;

	uint32_t res = static_cast<uint32_t>(data_.size());
	map_[str] = res;

	os::unicode_string tmp = os::FromUTF8(str);
	size_t size = tmp.size() * sizeof(os::unicode_char) + 1;
	data_.WriteEncoded(static_cast<uint32_t>(size));
	data_.insert(data_.end(), reinterpret_cast<const uint8_t *>(tmp.data()), reinterpret_cast<const uint8_t *>(tmp.data()) + size);

	return res;
}

/**
 * ILBlobStream
 */

ILBlobStream::ILBlobStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: ILStream(owner, address, size, name)
{

}

ILBlobStream::ILBlobStream(ILMetaData *owner, const ILBlobStream &src)
	: ILStream(owner, src)
{
	data_ = src.data_;
}

ILStream *ILBlobStream::Clone(ILoadCommandList *owner) const
{
	ILBlobStream *stream = new ILBlobStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

void ILBlobStream::ReadFromFile(NETArchitecture &file)
{
	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid stream address");

	data_.resize(size());
	file.Read(data_.data(), data_.size());
}

void ILBlobStream::WriteToFile(NETArchitecture &file)
{
	ILStream::WriteToFile(file);
	data_.resize(AlignValue(data_.size(), sizeof(uint32_t)), 0);
	set_size(static_cast<uint32_t>(file.Write(data_.data(), data_.size())));
}

void ILBlobStream::Prepare()
{
	data_.clear();
	map_.clear();
	AddData(ILData());
}

ILData ILBlobStream::GetData(uint32_t pos) const
{
	uint32_t len = data_.ReadEncoded(pos);
	if (pos + len > data_.size())
		throw std::runtime_error("Invalid index for " + name());

	return ILData(&data_[pos], len);
}

uint32_t ILBlobStream::AddData(const ILData &value)
{
	std::map<ILData, uint32_t>::const_iterator it = map_.find(value);
	if (it != map_.end())
		return it->second;

	uint32_t res = static_cast<uint32_t>(data_.size());
	map_[value] = res;

	data_.WriteEncoded(static_cast<uint32_t>(value.size()));
	data_.insert(data_.end(), value.data(), value.data() + value.size());

	return res;
}

/**
 * ILGuidStream
 */

ILGuidStream::ILGuidStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: ILStream(owner, address, size, name)
{

}

ILGuidStream::ILGuidStream(ILMetaData *owner, const ILGuidStream &src)
	: ILStream(owner, src)
{
	data_ = src.data_;
}

ILStream *ILGuidStream::Clone(ILoadCommandList *owner) const
{
	ILGuidStream *stream = new ILGuidStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

void ILGuidStream::Prepare()
{
	data_.clear();
	map_.clear();
}

void ILGuidStream::ReadFromFile(NETArchitecture &file)
{
	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid stream address");

	data_.resize(size());
	file.Read(data_.data(), data_.size());
}

void ILGuidStream::WriteToFile(NETArchitecture &file)
{
	ILStream::WriteToFile(file);
	data_.resize(AlignValue(data_.size(), sizeof(uint32_t)), 0);
	set_size(static_cast<uint32_t>(file.Write(data_.data(), data_.size())));
}

ILData ILGuidStream::GetData(uint32_t pos) const
{
	uint32_t len = 16;
	if (pos + len > data_.size())
		throw std::runtime_error("Invalid index for " + name());

	return ILData(&data_[pos], len);
}

uint32_t ILGuidStream::AddData(const ILData &value)
{
	std::map<ILData, uint32_t>::const_iterator it = map_.find(value);
	if (it != map_.end())
		return it->second;

	uint32_t res = static_cast<uint32_t>(data_.size());
	map_[value] = res;

	data_.insert(data_.end(), value.data(), value.data() + value.size());

	return res;
}

/**
 * ILHeapStream
 */

ILHeapStream::ILHeapStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name)
	: ILStream(owner, address, size, name)
{

}

ILHeapStream::~ILHeapStream()
{
	for (size_t i = 0; i < table_list_.size(); i++) {
		delete table_list_[i];
	}
}

ILHeapStream::ILHeapStream(ILMetaData *owner, const ILHeapStream &src)
	: ILStream(owner, src)
{
	reserved_ = src.reserved_;
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	heap_offset_sizes_ = src.heap_offset_sizes_;
	reserved2_ = src.reserved2_;
	mask_valid_ = src.mask_valid_;
	mask_sorted_ = src.mask_sorted_;
	extra_data_ = src.extra_data_;

	for (size_t i = 0; i < src.table_list_.size(); i++) {
		table_list_.push_back(src.table_list_[i]->Clone(owner));
	}
}

ILStream *ILHeapStream::Clone(ILoadCommandList *owner) const
{
	ILHeapStream *stream = new ILHeapStream(reinterpret_cast<ILMetaData *>(owner), *this);
	return stream;
}

ILTable *ILHeapStream::Add(ILTokenType type, uint32_t row_count)
{
	ILTable *table = new ILTable(reinterpret_cast<ILMetaData *>(owner()), type, row_count);
	table_list_.push_back(table);
	return table;
}

void ILHeapStream::ReadFromFile(NETArchitecture &file)
{
	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid heap address");

	reserved_ = file.ReadDWord();
	major_version_ = file.ReadByte();
	minor_version_ = file.ReadByte();
	heap_offset_sizes_ = file.ReadByte();
	reserved2_ = file.ReadByte();
	mask_valid_ = file.ReadQWord();
	mask_sorted_ = file.ReadQWord();

	size_t i;
	uint64_t mask = mask_valid_;
	for (i = 0; i < 64; mask >>= 1, i++) {
		uint32_t token_count = (mask & 1) ? file.ReadDWord() : 0;
		Add(static_cast<ILTokenType>(i << 24), token_count);
	}

	if (heap_offset_sizes_ & 0x40) {
		extra_data_.resize(sizeof(uint32_t));
		file.Read(extra_data_.data(), extra_data_.size());
	}

	for (i = 0; i < count(); i++) {
		table(i)->ReadFromFile(file);
	}
}

void ILHeapStream::WriteToFile(NETArchitecture &file)
{
	ILStream::WriteToFile(file);

	size_t i;

	for (i = 0; i < count(); i++) {
		table(i)->WriteToStreams(*reinterpret_cast<ILMetaData*>(owner()));
	}

	heap_offset_sizes_ = 0;
	if (file.command_list()->string_data_size() > UINT16_MAX)
		heap_offset_sizes_ |= 1;
	if (file.command_list()->guid_data_size() > UINT16_MAX)
		heap_offset_sizes_ |= 2;
	if (file.command_list()->blob_data_size() > UINT16_MAX)
		heap_offset_sizes_ |= 4;
	if (!extra_data_.empty())
		heap_offset_sizes_ |= 0x40;

	uint64_t pos = file.Tell();

	file.WriteDWord(reserved_);
	file.WriteByte(major_version_);
	file.WriteByte(minor_version_);
	file.WriteByte(heap_offset_sizes_);
	file.WriteByte(reserved2_);

	mask_valid_ = 0;
	uint64_t mask = 1;
	for (i = 0; i < count(); mask <<= 1, i++) {
		ILTable *table = this->table(i);
		if (table->count())
			mask_valid_ |= mask;
	}

	file.WriteQWord(mask_valid_);
	file.WriteQWord(mask_sorted_);

	for (i = 0; i < count(); i++) {
		ILTable *table = this->table(i);
		if (table->count())
			file.WriteDWord(static_cast<uint32_t>(table->count()));
	}

	if (!extra_data_.empty())
		file.Write(extra_data_.data(), extra_data_.size());

	for (i = 0; i < count(); i++) {
		table(i)->WriteToFile(file);
	}

	size_t size, pad_size;
	for (size = static_cast<size_t>(file.Tell() - pos), pad_size = AlignValue(size, sizeof(uint32_t)); size < pad_size; size++) {
		file.WriteByte(0);
	}
	set_size(static_cast<uint32_t>(size));
}

void ILHeapStream::Rebase(uint64_t delta_base)
{
	ILStream::Rebase(delta_base);

	for (size_t i = 0; i < count(); i++) {
		table(i)->Rebase(delta_base);
	}
}

void ILHeapStream::UpdateTokens()
{
	size_t i, j;
	std::map<ILToken *, ILToken *> token_map;
	ILTable *table;
	ILToken *token;

	for (i = 0; i < count(); i++) {
		table = this->table(i);
		for (j = 0; j < table->count(); j++) {
			token = table->item(j);
			if (token->is_deleted())
				token_map[token] = NULL;
		}
	}
	if (!token_map.empty()) {
		for (i = 0; i < count(); i++) {
			table = this->table(i);
			for (j = 0; j < table->count(); j++) {
				token = table->item(j);
				if (token->is_deleted())
					continue;

				token->RemapTokens(token_map);
				if (token->is_deleted())
					token_map[token] = NULL;
			}
		}
	}

	std::vector<ILTable *> need_sort_list;
	for (i = 0; i < count(); i++) {
		table = this->table(i);
		switch (table->type()) {
		case ttConstant:
		case ttCustomAttribute:
		case ttGenericParam:
		case ttGenericParamConstraint:
		case ttImplMap:
		case ttInterfaceImpl:
		case ttMethodImpl:
		case ttMethodSemantics:
			need_sort_list.push_back(table);
			break;
		default:
			table->UpdateTokens();
			break;
		}
	}
	for (i = 0; i < need_sort_list.size(); i++) {
		table = need_sort_list[i];
		table->Sort();
		table->UpdateTokens();
	}
}

void ILHeapStream::Pack()
{
	for (size_t i = 0; i < count(); i++) {
		table(i)->Pack();
	}
}

/**
 * ILMetaData
 */

ILMetaData::ILMetaData(NETArchitecture *owner)
	: BaseCommandList(owner), strings_(NULL), user_strings_(NULL), blob_(NULL), guid_(NULL), heap_(NULL),
	address_(0), signature_(0), major_version_(0), minor_version_(0), reserved_(0), flags_(0), size_(0)
{
	us_table_ = new ILUserStringsTable(this);
}

ILMetaData::ILMetaData(NETArchitecture *owner, const ILMetaData &src)
	: BaseCommandList(owner, src), strings_(NULL), user_strings_(NULL), blob_(NULL), guid_(NULL), heap_(NULL), size_(0)
{
	address_ = src.address_;
	signature_ = src.signature_;
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	reserved_ = src.reserved_;
	flags_ = src.flags_;
	version_ = src.version_;
	framework_ = src.framework_;
	us_table_ = src.us_table_->Clone(this);

	if (src.strings_)
		strings_ = reinterpret_cast<ILStringsStream *>(item(src.IndexOf(src.strings_)));
	if (src.user_strings_)
		user_strings_ = reinterpret_cast<ILUserStringsStream *>(item(src.IndexOf(src.user_strings_)));
	if (src.blob_)
		blob_ = reinterpret_cast<ILBlobStream *>(item(src.IndexOf(src.blob_)));
	if (src.guid_)
		guid_ = reinterpret_cast<ILGuidStream *>(item(src.IndexOf(src.guid_)));
	if (src.heap_) {
		heap_ = reinterpret_cast<ILHeapStream *>(item(src.IndexOf(src.heap_)));
		for (size_t i = 0; i < heap_->count(); i++) {
			ILTable *table = heap_->table(i);
			for (size_t j = 0; j < table->count(); j++) {
				table->item(j)->UpdateTokens();
			}
		}
	}
}

ILMetaData::~ILMetaData()
{
	delete us_table_;
}

ILMetaData *ILMetaData::Clone(NETArchitecture *owner) const
{
	ILMetaData *meta_data = new ILMetaData(owner, *this);
	return meta_data;
}

ILStream *ILMetaData::Add(uint64_t address, uint32_t size, const std::string &name)
{
	ILStream *stream = NULL;
	if (name == "#Strings") {
		if (!strings_) {
			strings_ = new ILStringsStream(this, address, size, name);
			stream = strings_;
		}
	} else if (name == "#US") {
		if (!user_strings_) {
			user_strings_ = new ILUserStringsStream(this, address, size, name);
			stream = user_strings_;
		}
	} else if (name == "#Blob") {
		if (!blob_) {
			blob_ = new ILBlobStream(this, address, size, name);
			stream = blob_;
		}
	} else if (name == "#GUID") {
		if (!guid_) {
			guid_ = new ILGuidStream(this, address, size, name);
			stream = guid_;
		}
	} else if (name == "#~") {
		if (!heap_) {
			heap_ = new ILHeapStream(this, address, size, name);
			stream = heap_;
		}
	}

	if (!stream)
		stream = new ILStream(this, address, size, name);
	AddObject(stream);
	return stream;
}

void ILMetaData::ReadFromFile(NETArchitecture &file, uint64_t address)
{
	if (!file.AddressSeek(address))
		throw std::runtime_error("Invalid meta data address");

	size_t i;
	address_ = address;
	signature_ = file.ReadDWord();
	if (signature_ != 'BJSB')
		throw std::runtime_error("Invalid meta data signature");

	major_version_ = file.ReadWord();
	minor_version_ = file.ReadWord();
	reserved_ = file.ReadDWord();
	size_t version_len = file.ReadDWord();
	if (version_len) {
		version_.resize(version_len);
		file.Read(&version_[0], version_len);
	}
	flags_ = file.ReadWord();
	size_t streams = file.ReadWord();
	for (i = 0; i < streams; i++) {
		uint64_t stream_address = file.ReadDWord() + address_;
		uint32_t stream_size = file.ReadDWord();
		std::string stream_name = file.ReadString();
		for (size_t str_size = stream_name.size() + 1, pad_size = AlignValue(str_size, sizeof(uint32_t)); str_size < pad_size; str_size++) {
			file.ReadByte();
		}
		Add(stream_address, stream_size, stream_name);
	}
	if (!heap_)
		throw std::runtime_error("Invalid .NET format");

	for (i = 0; i < count(); i++) {
		ILStream *stream = item(i);
		if (stream == heap_)
			continue;
		stream->ReadFromFile(file);
	}
	heap_->ReadFromFile(file);

	ILTable *table = this->table(ttNestedClass);
	for (i = 0; i < table->count(); i++) {
		ILNestedClass *nested = reinterpret_cast<ILNestedClass *>(table->item(i));
		ILTypeDef *nested_type = nested->nested_type();
		ILTypeDef *declaring_type = nested->declaring_type();
		if (!nested_type || !declaring_type)
			throw std::runtime_error("Invalid nested class");
		nested_type->set_declaring_type(declaring_type);
	}

	table = this->table(ttTypeDef);
	for (i = 0; i < table->count(); i++) {
		ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(table->item(i));
		ILTypeDef *next = type_def->next();
		ILMethodDef *method_end = next ? next->method_list() : NULL;
		if (type_def->method_list() == method_end)
			type_def->set_method_list(NULL);
		else for (ILMethodDef *method = type_def->method_list(); method && method != method_end; method = method->next())
			method->set_declaring_type(type_def);

		ILField *field_end = next ? next->field_list() : NULL;
		if (type_def->field_list() == field_end)
			type_def->set_field_list(NULL);
		else for (ILField *field = type_def->field_list(); field && field != field_end; field = field->next())
			field->set_declaring_type(type_def);
	}

	table = this->table(ttMethodDef);
	for (i = 0; i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
		ILMethodDef *next = method->next();
		ILParam *param_end = next ? next->param_list() : NULL;
		if (method->param_list() == param_end)
			method->set_param_list(NULL);
		else for (ILParam *param = method->param_list(); param && param != param_end; param = param->next())
			param->set_parent(method);
	}

	table = this->table(ttPropertyMap);
	for (i = 0; i < table->count(); i++) {
		ILPropertyMap *property_map = reinterpret_cast<ILPropertyMap *>(table->item(i));
		ILPropertyMap *next = property_map->next();
		ILProperty *property_end = next ? next->property_list() : NULL;
		if (property_map->property_list() == property_end)
			property_map->set_property_list(NULL);
		else for (ILProperty *property = property_map->property_list(); property && property != property_end; property = property->next())
			property->set_declaring_type(property_map->parent());
	}

	table = this->table(ttEventMap);
	for (i = 0; i < table->count(); i++) {
		ILEventMap *event_map = reinterpret_cast<ILEventMap *>(table->item(i));
		ILEventMap *next = event_map->next();
		ILEvent *event_end = next ? next->event_list() : NULL;
		if (event_map->event_list() == event_end)
			event_map->set_event_list(NULL);
		else for (ILEvent *event = event_map->event_list(); event && event != event_end; event = event->next())
			event->set_declaring_type(event_map->parent());
	}

	table = this->table(ttCustomAttribute);
	for (i = 0; i < table->count(); i++) {
		ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(table->item(i));
		if (attribute->type()->type() == ttMemberRef) {
			ILMemberRef *member_ref = reinterpret_cast<ILMemberRef *>(attribute->type());
			if (member_ref->name() == ".ctor") {
				if (ILToken *declaring_type = member_ref->declaring_type()) {
					std::string full_name;
					switch (declaring_type->type()) {
					case ttTypeRef:
						full_name = reinterpret_cast<ILTypeRef *>(declaring_type)->full_name();
						break;
					}
					if (full_name == "System.Runtime.Versioning.TargetFrameworkAttribute") {
						CustomAttributeValue *value = attribute->ParseValue();
						if (value->fixed_list()->count() == 1) {
							std::string str = value->fixed_list()->item(0)->ToString();
							str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == ' '; }), str.end());
							i = str.find(",");
							if (i != std::string::npos) {
								std::string name = str.substr(0, i);
								if (name == ".NETFramework")
									framework_.type = fwFramework;
								else if (name == ".NETCoreApp")
									framework_.type = fwCore;
								else if (name == ".NETStandard")
									framework_.type = fwStandard;
							}
							if (framework_.type != fwUnknown) {
								i = str.find("Version=v");
								if (i != std::string::npos)
									framework_.version.Parse(str.substr(i + 9));
							}
							break;
						}
					}
				}
			}
		}
	}

	if (framework_.type == fwUnknown) {
		if (ILAssemblyRef *core_lib = GetCoreLib()) {
			if (core_lib->name() == "mscorlib") {
				framework_.type = fwFramework;
				framework_.version.major = core_lib->major_version();
			}
			else if (core_lib->name() == "System.Runtime")
				framework_.type = fwCore;
			else if (core_lib->name() == "netstandard")
				framework_.type = fwStandard;
		}
	}
}

void ILMetaData::FreeByManager(MemoryManager &manager)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->FreeByManager(manager);
	}
}

void ILMetaData::Prepare()
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Prepare();
	}
}

void ILMetaData::WriteToFile(NETArchitecture &file)
{
	if (heap_)
		heap_->Pack();

	address_ = file.AddressTell();
	uint64_t pos = file.Tell();

	size_t i;
	file.WriteDWord(signature_);
	file.WriteWord(major_version_);
	file.WriteWord(minor_version_);
	file.WriteDWord(reserved_);
	file.WriteDWord(static_cast<uint32_t>(version_.size()));
	if (!version_.empty())
		file.Write(version_.data(), version_.size());
	file.WriteWord(flags_);
	file.WriteWord(static_cast<uint16_t>(count()));

	std::vector<uint64_t> pos_list;
	for (i = 0; i < count(); i++) {
		ILStream *stream = item(i);
		pos_list.push_back(file.Tell());
		file.WriteDWord(0);
		file.WriteDWord(0);
		std::string stream_name = stream->name();
		file.Write(stream_name.c_str(), stream_name.size() + 1);
		for (size_t str_size = stream_name.size() + 1, pad_size = AlignValue(str_size, sizeof(uint32_t)); str_size < pad_size; str_size++) {
			file.WriteByte(0);
		}
	}

	if (heap_)
		heap_->WriteToFile(file);
	for (i = 0; i < count(); i++) {
		ILStream *stream = item(i);
		if (stream == heap_)
			continue;
		stream->WriteToFile(file);
	}

	size_ = static_cast<uint32_t>(file.Tell() - pos);

	for (i = 0; i < count(); i++) {
		ILStream *stream = item(i);
		file.Seek(pos_list[i]);
		file.WriteDWord(static_cast<uint32_t>(stream->address() - address_));
		file.WriteDWord(stream->size());
	}
}

ILStream *ILMetaData::item(size_t index) const 
{ 
	return reinterpret_cast<ILStream *>(BaseCommandList::item(index));
}

ILStream *ILMetaData::GetStreamByName(const std::string &name) const
{
	for (size_t i = 0; i < count(); i++) {
		ILStream *stream = item(i);
		if (stream->name() == name)
			return stream;
	}

	return NULL;
}

ILTable *ILMetaData::table(ILTokenType type) const
{
	if (type == ttUserString)
		return us_table_;

	size_t i = type >> 24;
	if (i >= heap_->count())
		return NULL;

	return heap_->table(i);
}

uint32_t ILMetaData::token_count(ILTokenType type) const
{
	ILTable *table = this->table(type);
	if (!table)
		return 0;

	return static_cast<uint32_t>(table->count());
}

ILToken *ILMetaData::token(uint32_t id) const
{
	ILTokenType type = static_cast<ILTokenType>(TOKEN_TYPE(id));
	uint32_t value = TOKEN_VALUE(id);
	if (type == ttUserString)
		return us_table_->GetTokenByPos(value);

	ILTable *table = this->table(type);
	if (!table || value < 1 || value > table->count())
		return NULL;

	return table->item(value - 1);
}

size_t ILMetaData::field_size(ILTokenType type) const
{ 
	return (token_count(type) < 0x10000) ? sizeof(uint16_t) : sizeof(uint32_t);
}

size_t ILMetaData::field_size(const EncodingDesc &desc) const
{
	uint32_t count = 0;
	for (size_t i = 0; i < desc.size; i++) {
		count = std::max(count, token_count(desc.types[i]));
	}
	return (count < uint32_t(1 << (16 - desc.bits))) ? sizeof(uint16_t) : sizeof(uint32_t);
}

std::string ILMetaData::GetUserString(uint32_t pos) const
{
	if (!user_strings_)
		throw std::runtime_error("Invalid .NET format");
	return user_strings_->GetString(pos);
}

ILData ILMetaData::GetUserData(uint32_t pos, uint64_t *address) const
{
	if (!user_strings_)
		throw std::runtime_error("Invalid .NET format");
	return user_strings_->GetData(pos, address);
}

std::string ILMetaData::GetString(uint32_t pos) const
{
	if (!strings_)
		throw std::runtime_error("Invalid .NET format");
	return strings_->GetString(pos);
}

ILData ILMetaData::GetGuid(uint32_t pos) const
{
	if (!pos)
		return ILData();

	if (!guid_)
		throw std::runtime_error("Invalid .NET format");
	return guid_->GetData(pos - 1);
}

ILData ILMetaData::GetBlob(uint32_t pos) const
{
	if (!pos)
		return ILData();

	if (!blob_)
		throw std::runtime_error("Invalid .NET format");
	return blob_->GetData(pos);
}

uint32_t ILMetaData::AddUserString(const std::string &str) const
{
	if (!user_strings_)
		throw std::runtime_error("Invalid .NET format");
	return user_strings_->AddString(str);
}

uint32_t ILMetaData::AddString(const std::string &str) const
{
	if (!strings_)
		throw std::runtime_error("Invalid .NET format");
	return strings_->AddString(str);
}

uint32_t ILMetaData::AddGuid(const ILData &data) const
{
	if (data.empty())
		return 0;

	if (!guid_)
		throw std::runtime_error("Invalid .NET format");
	return guid_->AddData(data) + 1;
}

uint32_t ILMetaData::AddBlob(const ILData &data) const
{
	if (data.empty())
		return 0;

	if (!blob_)
		throw std::runtime_error("Invalid .NET format");
	return blob_->AddData(data);
}

void ILMetaData::UpdateTokens()
{
	uint64_t pos = owner()->Tell();
	if (heap_)
		heap_->UpdateTokens();
	if (us_table_)
		us_table_->UpdateTokens();
	owner()->Seek(pos);
}

void ILMetaData::Rebase(uint64_t delta_base)
{
	BaseCommandList::Rebase(delta_base);
	us_table_->Rebase(delta_base);
}

const char *ElementTypeToName[ELEMENT_TYPE_MAX] =
{
	"", "Void", "Boolean", "Char",
	"SByte", "Byte", "Int16", "UInt16",
	"Int32", "UInt32", "Int64", "UInt64",
	"Single", "Double", "String", "", "", "", "", "", "", "", "", "", "IntPtr", "UIntPtr", "", "", "Object", "Array", "", "", "", ""
};

ILToken *ILMetaData::GetType(const ILElement &element) const
{
	std::string type_name;
	ILTable *table;
	size_t i;
	ILToken *assembly_ref;

	switch (element.type()) {
	case ELEMENT_TYPE_VOID:
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_I:
	case ELEMENT_TYPE_U:
	case ELEMENT_TYPE_OBJECT:
	case ELEMENT_TYPE_SZARRAY:
		assembly_ref = GetCoreLib();
		if (!assembly_ref)
			throw std::runtime_error("Unknown framework");

		return GetTypeRef(assembly_ref, "System", ElementTypeToName[element.type()]);
		break;
	case ELEMENT_TYPE_GENERICINST:
	case ELEMENT_TYPE_PTR:
	case ELEMENT_TYPE_ARRAY:
		type_name = element.name();
		table = this->table(ttTypeSpec);
		for (i = 0; i < table->count(); i++) {
			ILTypeSpec *ref = reinterpret_cast<ILTypeSpec *>(table->item(i));
			if (ref->name() == type_name)
				return ref;
		}
		break;
	case ELEMENT_TYPE_CLASS:
	case ELEMENT_TYPE_VALUETYPE:
		return element.token();
	default:
		throw std::runtime_error("Invalid element type");
	}

	return NULL;
}

ILTypeRef *ILMetaData::ImportTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name)
{
	ILTypeRef *res = GetTypeRef(resolution_scope, name_space, name);
	if (!res)
		res = AddTypeRef(resolution_scope, name_space, name);
	return res;
}

ILTypeRef *ILMetaData::GetTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name) const
{
	ILTable *table = this->table(ttTypeRef);
	for (size_t i = 0; i < table->count(); i++) {
		ILTypeRef *ref = reinterpret_cast<ILTypeRef *>(table->item(i));
		if (ref->resolution_scope() == resolution_scope && ref->name_space() == name_space && ref->name() == name)
			return ref;
	}
	return NULL;
}

ILTypeRef *ILMetaData::AddTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name)
{
	ILTable *table = this->table(ttTypeRef);
	ILTypeRef *res = new ILTypeRef(this, table, resolution_scope, name_space, name);
	table->AddObject(res);
	return res;
}

ILToken *ILMetaData::AddType(const ILElement &element)
{
	ILData data;
	ILTable *table;
	ILToken *assembly_ref;
	ILToken *res;

	switch (element.type()) {
	case ELEMENT_TYPE_VOID:
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_I:
	case ELEMENT_TYPE_U:
	case ELEMENT_TYPE_OBJECT:
	case ELEMENT_TYPE_SZARRAY:
		assembly_ref = GetCoreLib();
		if (!assembly_ref)
			throw std::runtime_error("Unknown framework");

		return AddTypeRef(assembly_ref, "System", ElementTypeToName[element.type()]);
	case ELEMENT_TYPE_GENERICINST:
	case ELEMENT_TYPE_PTR:
	case ELEMENT_TYPE_ARRAY:
		element.WriteToData(data);
		table = this->table(ttTypeSpec);
		res = new ILTypeSpec(this, table, data);
		table->AddObject(res);
		return res;
	default:
		throw std::runtime_error("Invalid element type");
	}

	return NULL;
}

ILToken *ILMetaData::ImportType(CorElementType type)
{
	ILElement element(NULL, NULL, type);
	return ImportType(element);
}

ILToken *ILMetaData::ImportType(const ILElement &element)
{
	ILToken *res = GetType(element);
	if (!res)
		res = AddType(element);
	return res;
}

ILMethodDef *ILMetaData::GetMethod(uint64_t address) const
{
	ILTable *table = this->table(ttMethodDef);
	for (size_t i = 0; i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef*>(table->item(i));
		if (method->address() == address)
			return method;
	}
	return NULL;
}

ILTypeDef *ILMetaData::GetTypeDef(const std::string &name) const
{
	ILTable *table = this->table(ttTypeDef);
	for (size_t i = 0; i < table->count(); i++) {
		ILTypeDef *type_def = reinterpret_cast<ILTypeDef*>(table->item(i));
		if (type_def->full_name() == name)
			return type_def;
	}
	return NULL;
}

ILExportedType *ILMetaData::GetExportedType(const std::string &name) const
{
	ILTable *table = this->table(ttExportedType);
	for (size_t i = 0; i < table->count(); i++) {
		ILExportedType *exported_type = reinterpret_cast<ILExportedType*>(table->item(i));
		if (exported_type->full_name() == name)
			return exported_type;
	}
	return NULL;
}

ILAssemblyRef *ILMetaData::GetAssemblyRef(const std::string &name) const
{
	ILTable *table = this->table(ttAssemblyRef);
	bool is_full_name = (name.find(',') != std::string::npos);
	for (size_t i = 0; i < table->count(); i++) {
		ILAssemblyRef *ref = reinterpret_cast<ILAssemblyRef*>(table->item(i));
		if (is_full_name) {
			if (ref->full_name() == name)
				return ref;
		}
		else {
			if (ref->name() == name)
				return ref;
		}
	}
	return NULL;
}

ILStandAloneSig *ILMetaData::AddStandAloneSig(const ILData &data)
{
	ILTable *table = this->table(ttStandAloneSig);
	ILStandAloneSig *signature = new ILStandAloneSig(this, table, data);
	table->AddObject(signature);
	signature->set_value(static_cast<uint32_t>(table->count()));
	return signature;
}

ILAssemblyRef *ILMetaData::GetCoreLib() const
{
	ILTable *table = this->table(ttAssemblyRef);
	for (size_t i = 0; i < table->count(); i++) {
		ILAssemblyRef *ref = reinterpret_cast<ILAssemblyRef *>(table->item(i));
		if (ref->name() == "mscorlib" || ref->name() == "System.Runtime" || ref->name() == "netstandard")
			return ref;
	}
	return NULL;
}

ILTypeDef *ILMetaData::AddTypeDef(ILToken *base_type, const std::string &name_space, const std::string &name, CorTypeAttr flags)
{
	ILTable *table = this->table(ttTypeDef);
	ILTypeDef *res = new ILTypeDef(this, table, base_type, name_space, name, flags);
	table->AddObject(res);
	return res;
}

ILMethodDef *ILMetaData::AddMethod(ILTypeDef *declaring_type, const std::string &name, const ILData &signature, CorMethodAttr flags, CorMethodImpl impl_flags)
{
	ILMethodDef *res = new ILMethodDef(this, table(ttMethodDef), name, signature, flags, impl_flags);
	res->set_declaring_type(declaring_type);
	declaring_type->AddMethod(res);
	return res;
}

ILMetaData *ILMetaData::ResolveAssembly(const std::string &name, bool show_error) const
{
	if (ILAssembly *assembly = reinterpret_cast<ILAssembly *>(token(ttAssembly | 1))) {
		std::string self_name = (name.find(',') == std::string::npos) ? assembly->name() : assembly->full_name();
		if (self_name == name)
			return const_cast<ILMetaData *>(this);
	}

	ILMetaData *res = resolver.Resolve(*this, name);
	if (!res && show_error)
		throw std::runtime_error(string_format("Can't resolve assembly: %s", name.c_str()));

	return res;
}

ILTypeDef *ILMetaData::ResolveType(const std::string &name, bool show_error)
{
	size_t pos = 0;
	size_t length = name.size();

	std::string type_name;
	while (pos < length) {
		char c = name[pos];
		if (c == ',' || c == '+')
			break;
		pos++;
	}
	type_name = name.substr(0, pos);

	std::vector<std::string> nested_names;
	while (pos < length && name[pos] == '+') {
		pos++;
		size_t start = pos;
		while (pos < length) {
			char c = name[pos];
			if (c == ',' || c == '+')
				break;
			pos++;
		}
		nested_names.push_back(name.substr(start, pos - start));
	}

	std::string assembly_name;
	if (pos < length && name[pos] == ',') {
		pos++;
		while (pos < length && name[pos] == ' ') {
			pos++;
		}

		size_t start = pos;
		while (pos < length) {
			char c = name[pos];
			if (c == '[' || c == ']')
				break;
			pos++;
		}
		assembly_name = name.substr(start, pos - start);
	}

	if (assembly_name.empty() && nested_names.empty() && name.substr(0, 7) == "System.") {
		std::string corlib_type = name.substr(7);
		const char *corlib_types[] = { "Void", "Boolean", "Char", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64", "Single", "Double", "String", "TypedReference", "IntPtr", "UIntPtr", "Object" };
		for (size_t i = 0; i < _countof(corlib_types); i++) {
			if (corlib_types[i] == corlib_type) {
				if (ILAssemblyRef *assembly_ref = GetCoreLib())
					assembly_name = assembly_ref->full_name();
				break;
			}
		}
	}
	
	ILMetaData *assembly = assembly_name.empty() ? this : ResolveAssembly(assembly_name, show_error);
	if (assembly) {
		ILTypeDef *type_def = assembly->GetTypeDef(type_name);
		if (!type_def) {
			if (ILExportedType *exported_type = assembly->GetExportedType(type_name))
				type_def = Resolve(exported_type, show_error);
		}
		if (type_def) {
			for (size_t i = 0; i < nested_names.size(); i++) {
				type_def = type_def->GetNested(nested_names[i]);
				if (!type_def)
					break;
			}
			return type_def;
		}
	}

	if (show_error)
		throw std::runtime_error(string_format("Can't resolve type: %s", name.c_str()));

	return NULL;
}

ILTypeDef *ILMetaData::Resolve(const ILExportedType *exported_type, bool show_error) const
{
	std::string type_name = exported_type->full_name();

	std::set<const ILExportedType *> recursion_stack;
	while (true) {
		if (recursion_stack.find(exported_type) != recursion_stack.end())
			break;
		recursion_stack.insert(exported_type);

		const ILExportedType *non_nested_type = exported_type;
		while (non_nested_type->declaring_type()) {
			non_nested_type = non_nested_type->declaring_type();
		}

		ILToken *scope = non_nested_type->implementation();
		if (scope && scope->type() == ttAssemblyRef) {
			if (ILMetaData *assembly = ResolveAssembly(reinterpret_cast<ILAssemblyRef *>(scope)->full_name(), show_error)) {
				if (ILTypeDef *type_def = assembly->GetTypeDef(type_name))
					return type_def;
				exported_type = assembly->GetExportedType(type_name);
				if (exported_type)
					continue;
			}
		}
		break;
	}

	return NULL;
}

ILTypeDef *ILMetaData::Resolve(const ILTypeRef *type_ref, bool show_error) const
{
	std::string type_name = type_ref->full_name();

	const ILTypeRef *non_nested_type = type_ref;
	while (non_nested_type->declaring_type()) {
		non_nested_type = non_nested_type->declaring_type();
	}

	ILToken *scope = non_nested_type->resolution_scope();
	if (scope && scope->type() == ttAssemblyRef) {
		if (ILMetaData *assembly = ResolveAssembly(reinterpret_cast<ILAssemblyRef *>(scope)->full_name(), show_error)) {
			ILTypeDef *type_def = assembly->GetTypeDef(type_name);
			if (!type_def) {
				if (ILExportedType *exported_type = assembly->GetExportedType(type_name))
					type_def = Resolve(exported_type, show_error);
			}
			if (type_def)
				return type_def;
		}
	}

	if (show_error)
		throw std::runtime_error(string_format("Can't resolve token: %.8x", type_ref->id()));

	return NULL;
}

/**
 * TokenReference
 */

TokenReference::TokenReference(TokenReferenceList *owner, uint64_t address, uint32_t command_type)
	: IObject(), owner_(owner), address_(address), is_deleted_(false), command_type_(command_type)
{

}

TokenReference::TokenReference(TokenReferenceList *owner, const TokenReference &src)
	: IObject(src), owner_(owner)
{
	address_ = src.address_;
	is_deleted_ = src.is_deleted_;
	command_type_ = src.command_type_;
}

TokenReference::~TokenReference()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void TokenReference::set_owner(TokenReferenceList *owner)
{
	if (owner == owner_)
		return;
	if (owner_)
		owner_->RemoveObject(this);
	owner_ = owner;
	if (owner_)
		owner_->AddObject(this);
}

TokenReference *TokenReference::Clone(TokenReferenceList *owner) const
{
	TokenReference *ref = new TokenReference(owner, *this);
	return ref;
}

void TokenReference::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * TokenReferenceList
 */

TokenReferenceList::TokenReferenceList(ILToken *owner)
	: ObjectList<TokenReference>(), owner_(owner)
{

}

TokenReferenceList::TokenReferenceList(ILToken *owner, const TokenReferenceList &src)
	: ObjectList<TokenReference>(src), owner_(owner)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

TokenReferenceList *TokenReferenceList::Clone(ILToken *owner) const
{
	TokenReferenceList *list = new TokenReferenceList(owner, *this);
	return list;
}

TokenReference *TokenReferenceList::Add(uint64_t address, uint32_t command_type)
{
	TokenReference *ref = new TokenReference(this, address, command_type);
	AddObject(ref);
	return ref;
}

TokenReference *TokenReferenceList::GetReferenceByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		TokenReference *ref = item(i);
		if (ref->address() == address)
			return ref;
	}
	return NULL;
}

void TokenReferenceList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * EncodingDesc
 */

const ILTokenType resolution_scope_types[] = {ttModule, ttModuleRef, ttAssemblyRef, ttTypeRef};
const ILTokenType typedef_ref_types[] = {ttTypeDef, ttTypeRef, ttTypeSpec};
const ILTokenType type_or_methoddef_types[] = {ttTypeDef, ttMethodDef};
const ILTokenType has_semantics_types[] = {ttEvent, ttProperty};
const ILTokenType methoddef_ref_types[] = {ttMethodDef, ttMemberRef};
const ILTokenType member_forwarded_types[] = {ttField, ttMethodDef};
const ILTokenType has_field_marshal_types[] = {ttField, ttParam};
const ILTokenType implementation_types[] = {ttFile, ttAssemblyRef, ttExportedType};
const ILTokenType member_ref_parent_types[] = {ttTypeDef, ttTypeRef, ttModuleRef, ttMethodDef, ttTypeSpec};
const ILTokenType has_constant_types[] = {ttField, ttParam, ttProperty};
const ILTokenType custom_attribute_types[] = {ttInvalid, ttInvalid, ttMethodDef, ttMemberRef};
const ILTokenType has_custom_attribute_types[] = {ttMethodDef, ttField, ttTypeRef, ttTypeDef, ttParam, ttInterfaceImpl, ttMemberRef, ttModule, ttDeclSecurity,
	ttProperty, ttEvent, ttStandAloneSig, ttModuleRef, ttTypeSpec, ttAssembly, ttAssemblyRef, ttFile, ttExportedType,
	ttManifestResource, ttGenericParam, ttGenericParamConstraint, ttMethodSpec};
const ILTokenType has_decl_security_types[] = {ttTypeDef, ttMethodDef, ttAssembly};

const EncodingDesc ResolutionScope = EncodingDesc(resolution_scope_types, _countof(resolution_scope_types), 2);
const EncodingDesc TypeDefRef = EncodingDesc(typedef_ref_types, _countof(typedef_ref_types), 2);
const EncodingDesc TypeMethodDef = EncodingDesc(type_or_methoddef_types, _countof(type_or_methoddef_types), 1);
const EncodingDesc HasSemantics = EncodingDesc(has_semantics_types, _countof(has_semantics_types), 1);
const EncodingDesc MethodDefRef = EncodingDesc(methoddef_ref_types, _countof(methoddef_ref_types), 1);
const EncodingDesc MemberForwarded = EncodingDesc(member_forwarded_types, _countof(member_forwarded_types), 1);
const EncodingDesc HasFieldMarshal = EncodingDesc(has_field_marshal_types, _countof(has_field_marshal_types), 1);
const EncodingDesc Implementation = EncodingDesc(implementation_types, _countof(implementation_types), 2);
const EncodingDesc MemberRefParent = EncodingDesc(member_ref_parent_types, _countof(member_ref_parent_types), 3);
const EncodingDesc HasConstant = EncodingDesc(has_constant_types, _countof(has_constant_types), 2);
const EncodingDesc CustomAttribute = EncodingDesc(custom_attribute_types, _countof(custom_attribute_types), 3);
const EncodingDesc HasCustomAttribute = EncodingDesc(has_custom_attribute_types, _countof(has_custom_attribute_types), 5);
const EncodingDesc HasDeclSecurity = EncodingDesc(has_decl_security_types, _countof(has_decl_security_types), 2);

/**
 * ILToken
 */

ILToken::ILToken(ILMetaData *meta, ILTable *owner, uint32_t id)
	: IObject(), meta_(meta), owner_(owner), id_(id), is_deleted_(false), can_rename_(true)
{
	reference_list_ = new TokenReferenceList(this);
}

ILToken::ILToken(ILMetaData *meta, ILTable *owner, const ILToken &src)
	: IObject(), meta_(meta), owner_(owner)
{
	id_ = src.id_;
	is_deleted_ = src.is_deleted_;
	can_rename_ = src.can_rename_;
	reference_list_ = src.reference_list_->Clone(this);
}

ILToken::~ILToken()
{
	if (owner_)
		owner_->RemoveObject(this);
	delete reference_list_;
}

ILToken *ILToken::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILToken *token = new ILToken(meta, owner, *this);
	return token;
}

void ILToken::set_value(uint32_t value, IArchitecture *file)
{
	if (value == this->value())
		return;

	id_ = TOKEN_TYPE(id_) | TOKEN_VALUE(value);
	if (file) {
		for (size_t i = 0; i < reference_list_->count(); i++) {
			TokenReference *reference = reference_list_->item(i);
			if (reference->is_deleted() || !reference->address())
				continue;

			if (!file->AddressSeek(reference->address()))
				throw std::runtime_error("Invalid reference address");
			file->WriteDWord(id_);
		}
	}
}

void ILToken::set_owner(ILMetaData *meta, ILTable *owner)
{
	meta_ = meta;
	if (owner == owner_)
		return;
	if (owner_)
		owner_->RemoveObject(this);
	owner_ = owner;
	if (owner_)
		owner_->AddObject(this);
}

ILToken *ILToken::next() const 
{ 
	return meta_->token(id() + 1);
}

std::string ILToken::ReadStringFromFile(NETArchitecture &file) const
{
	uint32_t value = (meta_->string_field_size() == sizeof(uint16_t)) ? file.ReadWord() : file.ReadDWord();
	return meta_->GetString(value);
}

std::string ILToken::ReadUserString(uint32_t value) const
{
	return meta_->GetUserString(value);
}

ILData ILToken::ReadBlobFromFile(NETArchitecture &file) const
{
	uint32_t value = (meta_->blob_field_size() == sizeof(uint16_t)) ? file.ReadWord() : file.ReadDWord();
	return meta_->GetBlob(value);
}

ILData ILToken::ReadGUIDFromFile(NETArchitecture &file) const
{
	std::string res;
	uint32_t value = (meta_->guid_field_size() == sizeof(uint16_t)) ? file.ReadWord() : file.ReadDWord();
	return meta_->GetGuid(value);
}

ILToken *ILToken::ReadTokenFromFile(NETArchitecture &file, const EncodingDesc &desc) const
{
	uint32_t value = (meta_->field_size(desc) == sizeof(uint16_t)) ? file.ReadWord() : file.ReadDWord();
	size_t i = (value & ((1 << desc.bits) - 1));
	if (i >= desc.size || desc.types[i] == ttInvalid)
		throw std::runtime_error("Unknown ref type");
	return meta_->token(desc.types[i] | (value >> desc.bits));
}

ILToken *ILToken::ReadTokenFromFile(NETArchitecture &file, ILTokenType type) const
{
	uint32_t value = (meta_->field_size(type) == sizeof(uint16_t)) ? file.ReadWord() : file.ReadDWord();
	return meta_->token(type | value);
}

void ILToken::WriteStringToFile(NETArchitecture &file, uint32_t pos) const
{
	if (meta_->string_field_size() == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(pos));
	else
		file.WriteDWord(pos);
}

void ILToken::WriteGuidToFile(NETArchitecture &file, uint32_t pos) const
{
	if (meta_->guid_field_size() == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(pos));
	else
		file.WriteDWord(pos);
}

void ILToken::WriteBlobToFile(NETArchitecture &file, uint32_t pos) const
{
	if (meta_->blob_field_size() == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(pos));
	else
		file.WriteDWord(pos);
}

void ILToken::WriteTokenToFile(NETArchitecture &file, ILTokenType type, ILToken *token) const
{
	uint32_t value = token ? token->value() : 0;
	if (meta_->field_size(type) == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(value));
	else
		file.WriteDWord(value);
}

void ILToken::WriteTokenListToFile(NETArchitecture &file, ILTokenType type, ILToken *token) const
{
	uint32_t value = token ? token->value() : static_cast<uint32_t>(meta_->table(type)->count() + 1);
	if (meta_->field_size(type) == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(value));
	else
		file.WriteDWord(value);
}

uint32_t ILToken::Encode(const EncodingDesc &desc) const
{
	uint32_t res = value() << desc.bits;
	for (size_t i = 0; i < desc.size; i++) {
		if (type() == desc.types[i]) {
			res |= i;
			break;
		}
	}
	return res;
}

void ILToken::WriteTokenToFile(NETArchitecture &file, const EncodingDesc &desc, ILToken *token) const
{
	uint32_t value = token ? token->Encode(desc) : 0;
	if (meta_->field_size(desc) == sizeof(uint16_t)) 
		file.WriteWord(static_cast<uint16_t>(value));
	else
		file.WriteDWord(value);
}

void ILToken::Rebase(uint64_t delta_base)
{
	reference_list_->Rebase(delta_base);
}

int ILToken::CompareWith(const ILToken &other) const
{
	throw std::runtime_error("Abstract error");
}

/**
 * ILAssembly
 */

ILAssembly::ILAssembly(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), hash_id_(0), major_version_(0), minor_version_(0), build_number_(0), revision_number_(0), flags_(0), 
	name_pos_(0), public_key_pos_(0), culture_pos_(0)
{

}

ILAssembly::ILAssembly(ILMetaData *meta, ILTable *owner, const std::string &name)
	: ILToken(meta, owner, ttAssembly), hash_id_(0), major_version_(0), minor_version_(0), build_number_(0), revision_number_(0), flags_(0),
	name_pos_(0), public_key_pos_(0), culture_pos_(0), name_(name)
{

}

ILAssembly::ILAssembly(ILMetaData *meta, ILTable *owner, const ILAssembly &src)
	: ILToken(meta, owner, src), name_pos_(0), public_key_pos_(0), culture_pos_(0)
{
	hash_id_ = src.hash_id_;
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	build_number_ = src.build_number_;
	revision_number_ = src.revision_number_;
	flags_ = src.flags_;
	public_key_ = src.public_key_;
	name_ = src.name_;
	culture_ = src.culture_;
}

ILToken *ILAssembly::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILAssembly *token = new ILAssembly(meta, owner, *this);
	return token;
}

void ILAssembly::ReadFromFile(NETArchitecture &file)
{
	hash_id_ = file.ReadDWord();
	major_version_ = file.ReadWord();
	minor_version_ = file.ReadWord();
	build_number_ = file.ReadWord();
	revision_number_ = file.ReadWord();
	flags_ = file.ReadDWord();
	public_key_ = ReadBlobFromFile(file);
	name_ = ReadStringFromFile(file);
	culture_ = ReadStringFromFile(file);
}

void ILAssembly::WriteToStreams(ILMetaData &data)
{
	public_key_pos_ = data.AddBlob(public_key_);
	name_pos_ = data.AddString(name_);
	culture_pos_ = data.AddString(culture_);
}

void ILAssembly::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(hash_id_);
	file.WriteWord(major_version_);
	file.WriteWord(minor_version_);
	file.WriteWord(build_number_);
	file.WriteWord(revision_number_);
	file.WriteDWord(flags_);
	WriteBlobToFile(file,public_key_pos_);
	WriteStringToFile(file, name_pos_);
	WriteStringToFile(file, culture_pos_);
}

std::string ILAssembly::full_name() const
{
	std::string res = name_;
	res += ", Version=" + string_format("%d.%d.%d.%d", major_version_, minor_version_, build_number_, revision_number_);
	res += ", Culture=" + (!culture_.empty() ? culture_ : "neutral");
	res += ", PublicKeyToken=";
	if (public_key_.empty())
		res += "null";
	else {
		SHA1 sha;
		sha.Input(public_key_.data(), public_key_.size());
		const uint8_t *p = sha.Result();
		for (size_t i = 0; i < 8; i++) {
			res += string_format("%.2x", p[sha.ResultSize() - 1 - i]);
		}
	}
	return res;
}

/**
 * ILAssemblyOS
 */

ILAssemblyOS::ILAssemblyOS(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), os_platform_id_(0), os_major_version_(0), os_minor_version_(0)
{

}

void ILAssemblyOS::ReadFromFile(NETArchitecture &file)
{
	os_platform_id_ = file.ReadDWord();
	os_major_version_ = file.ReadDWord();
	os_minor_version_ = file.ReadDWord();
}

void ILAssemblyOS::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(os_platform_id_);
	file.WriteDWord(os_major_version_);
	file.WriteDWord(os_minor_version_);
}

/**
 * ILAssemblyProcessor
 */

ILAssemblyProcessor::ILAssemblyProcessor(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), processor_(0)
{

}

void ILAssemblyProcessor::ReadFromFile(NETArchitecture &file)
{
	processor_ = file.ReadDWord();
}

void ILAssemblyProcessor::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(processor_);
}

/**
 * ILAssemblyRef
 */

ILAssemblyRef::ILAssemblyRef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), major_version_(0), minor_version_(0), build_number_(0), revision_number_(0), flags_(0), name_pos_(0),
	public_key_or_token_pos_(0), culture_pos_(0), hash_value_pos_(0)
{

}

ILAssemblyRef::ILAssemblyRef(ILMetaData *meta, ILTable *owner, const std::string &name)
	: ILToken(meta, owner, ttAssemblyRef), major_version_(0), minor_version_(0), build_number_(0), revision_number_(0), flags_(0), name_pos_(0),
	public_key_or_token_pos_(0), culture_pos_(0), hash_value_pos_(0), name_(name)
{

}

ILAssemblyRef::ILAssemblyRef(ILMetaData *meta, ILTable *owner, const ILAssemblyRef &src)
	: ILToken(meta, owner, src), name_pos_(0), public_key_or_token_pos_(0), culture_pos_(0), hash_value_pos_(0)
{
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	build_number_ = src.build_number_;
	revision_number_ = src.revision_number_;
	flags_ = src.flags_;
	public_key_or_token_ = src.public_key_or_token_;
	name_ = src.name_;
	culture_ = src.culture_;
	hash_value_ = src.hash_value_;
}

ILToken *ILAssemblyRef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILAssemblyRef *token = new ILAssemblyRef(meta, owner, *this);
	return token;
}

void ILAssemblyRef::ReadFromFile(NETArchitecture &file)
{
	major_version_ = file.ReadWord();
	minor_version_ = file.ReadWord();
	build_number_ = file.ReadWord();
	revision_number_ = file.ReadWord();
	flags_ = file.ReadDWord();
	public_key_or_token_ = ReadBlobFromFile(file);
	name_ = ReadStringFromFile(file);
	culture_ = ReadStringFromFile(file);
	hash_value_ = ReadBlobFromFile(file);
}

void ILAssemblyRef::WriteToStreams(ILMetaData &data)
{
	public_key_or_token_pos_ = data.AddBlob(public_key_or_token_);
	name_pos_ = data.AddString(name_);
	culture_pos_ = data.AddString(culture_);
	hash_value_pos_ = data.AddBlob(hash_value_);
}

void ILAssemblyRef::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(major_version_);
	file.WriteWord(minor_version_);
	file.WriteWord(build_number_);
	file.WriteWord(revision_number_);
	file.WriteDWord(flags_);
	WriteBlobToFile(file, public_key_or_token_pos_);
	WriteStringToFile(file, name_pos_);
	WriteStringToFile(file, culture_pos_);
	WriteBlobToFile(file, hash_value_pos_);
}

std::string ILAssemblyRef::full_name() const
{
	std::string res = name_;
	res += ", Version=" + string_format("%d.%d.%d.%d", major_version_, minor_version_, build_number_, revision_number_);
	res += ", Culture=" + (!culture_.empty() ? culture_ : "neutral");
	res += ", PublicKeyToken=";
	if (public_key_or_token_.empty())
		res += "null";
	else {
		if (flags_ & afPublicKey) {
			SHA1 sha;
			sha.Input(public_key_or_token_.data(), public_key_or_token_.size());
			const uint8_t *p = sha.Result();
			for (size_t i = 0; i < 8; i++) {
				res += string_format("%.2x", p[sha.ResultSize() - 1 - i]);
			}
		}
		else {
			for (size_t i = 0; i < public_key_or_token_.size(); i++) {
				res += string_format("%.2x", public_key_or_token_[i]);
			}
		}
	}
	return res;
}

/**
 * ILAssemblyRefOS
 */

ILAssemblyRefOS::ILAssemblyRefOS(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), os_platform_id_(0), os_major_version_(0), os_minor_version_(0), assembly_ref_(0)
{

}

void ILAssemblyRefOS::ReadFromFile(NETArchitecture &file)
{
	os_platform_id_ = file.ReadDWord();
	os_major_version_ = file.ReadDWord();
	os_minor_version_ = file.ReadDWord();
	assembly_ref_ = reinterpret_cast<ILAssemblyRef *>(ReadTokenFromFile(file, ttAssemblyRef));
}

void ILAssemblyRefOS::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(os_platform_id_);
	file.WriteDWord(os_major_version_);
	file.WriteDWord(os_minor_version_);
	WriteTokenToFile(file, ttAssemblyRef, assembly_ref_);
}

void ILAssemblyRefOS::UpdateTokens()
{
	if (assembly_ref_)
		assembly_ref_ = reinterpret_cast<ILAssemblyRef *>(meta()->token(assembly_ref_->id()));
}

/**
 * ILAssemblyRefProcessor
 */

ILAssemblyRefProcessor::ILAssemblyRefProcessor(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), processor_(0), assembly_ref_(0)
{

}

void ILAssemblyRefProcessor::ReadFromFile(NETArchitecture &file)
{
	processor_ = file.ReadDWord();
	assembly_ref_ = reinterpret_cast<ILAssemblyRef *>(ReadTokenFromFile(file, ttAssemblyRef));
}

void ILAssemblyRefProcessor::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(processor_);
	WriteTokenToFile(file, ttAssemblyRef, assembly_ref_);
}

void ILAssemblyRefProcessor::UpdateTokens()
{
	if (assembly_ref_)
		assembly_ref_ = reinterpret_cast<ILAssemblyRef *>(meta()->token(assembly_ref_->id()));
}

/**
 * ILClassLayout
 */

ILClassLayout::ILClassLayout(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), packing_size_(0), class_size_(0), parent_(0)
{

}

ILClassLayout::ILClassLayout(ILMetaData *meta, ILTable *owner, const ILClassLayout &src)
	: ILToken(meta, owner, src)
{
	packing_size_ = src.packing_size_;
	class_size_ = src.class_size_;
	parent_ = src.parent_;
}

ILToken *ILClassLayout::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILClassLayout *token = new ILClassLayout(meta, owner, *this);
	return token;
}

void ILClassLayout::ReadFromFile(NETArchitecture &file)
{
	packing_size_ = file.ReadWord();
	class_size_ = file.ReadDWord();
	parent_ = reinterpret_cast<ILTypeDef *>(ReadTokenFromFile(file, ttTypeDef));
}

void ILClassLayout::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(packing_size_);
	file.WriteDWord(class_size_);
	WriteTokenToFile(file, ttTypeDef, parent_);
}

void ILClassLayout::UpdateTokens()
{
	if (parent_)
		parent_ = reinterpret_cast<ILTypeDef *>(meta()->token(parent_->id()));
}

void ILClassLayout::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!parent_)
			set_deleted(true);
	}
}

/**
 * ILConstant
 */

ILConstant::ILConstant(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), type_(0), padding_zero_(0), parent_(NULL), value_pos_(0)
{

}

ILConstant::ILConstant(ILMetaData *meta, ILTable *owner, const ILConstant &src)
	: ILToken(meta, owner, src), value_pos_(0)
{
	type_ = src.type_;
	padding_zero_ = src.padding_zero_;
	parent_ = src.parent_;
	value_ = src.value_;
}

ILToken *ILConstant::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILConstant *token = new ILConstant(meta, owner, *this);
	return token;
}

void ILConstant::ReadFromFile(NETArchitecture &file)
{
	type_ = file.ReadByte();
	padding_zero_ = file.ReadByte();
	parent_ = ReadTokenFromFile(file, HasConstant);
	value_ = ReadBlobFromFile(file);
}

void ILConstant::WriteToStreams(ILMetaData &data)
{
	value_pos_ = data.AddBlob(value_);
}

void ILConstant::WriteToFile(NETArchitecture &file)
{
	file.WriteByte(type_);
	file.WriteByte(padding_zero_);
	WriteTokenToFile(file, HasConstant, parent_);
	WriteBlobToFile(file, value_pos_);
}

void ILConstant::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
}

void ILConstant::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = it->second;
		if (!parent_)
			set_deleted(true);
	}
}

int ILConstant::CompareWith(const ILToken &other) const
{
	if (other.type() == ttConstant) {
		const ILConstant &obj = reinterpret_cast<const ILConstant &>(other);
		uint32_t self_value = parent_ ? parent_->Encode(HasConstant) : 0;
		uint32_t other_value = obj.parent_ ? obj.parent_->Encode(HasConstant) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILCustomAttribute
 */

ILCustomAttribute::ILCustomAttribute(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(NULL), type_(NULL), value_pos_(0)
{
	parser_ = new CustomAttributeValue(meta);
}

ILCustomAttribute::ILCustomAttribute(ILMetaData *meta, ILTable *owner, const ILCustomAttribute &src)
	: ILToken(meta, owner, src), value_pos_(0)
{
	parser_ = new CustomAttributeValue(meta);

	parent_ = src.parent_;
	type_ = src.type_;
	value_ = src.value_;
}

ILCustomAttribute::~ILCustomAttribute()
{
	delete parser_;
}

ILToken *ILCustomAttribute::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILCustomAttribute *token = new ILCustomAttribute(meta, owner, *this);
	return token;
}

void ILCustomAttribute::ReadFromFile(NETArchitecture &file)
{
	parent_ = ReadTokenFromFile(file, HasCustomAttribute);
	type_ = ReadTokenFromFile(file, CustomAttribute);
	value_ = ReadBlobFromFile(file);
}

void ILCustomAttribute::WriteToStreams(ILMetaData &data)
{
	value_pos_ = data.AddBlob(value_);
}

void ILCustomAttribute::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, HasCustomAttribute, parent_);
	WriteTokenToFile(file, CustomAttribute, type_);
	WriteBlobToFile(file, value_pos_);
}

void ILCustomAttribute::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
	if (type_)
		type_ = meta()->token(type_->id());
}

void ILCustomAttribute::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = it->second;
		if (!parent_)
			set_deleted(true);
	}
	it = token_map.find(type_);
	if (it != token_map.end()) {
		type_ = it->second;
		if (!type_)
			set_deleted(true);
	}
}

int ILCustomAttribute::CompareWith(const ILToken &other) const
{
	if (other.type() == ttCustomAttribute) {
		const ILCustomAttribute &obj = reinterpret_cast<const ILCustomAttribute &>(other);
		uint32_t self_value = parent_ ? parent_->Encode(HasCustomAttribute) : 0;
		uint32_t other_value = obj.parent_ ? obj.parent_->Encode(HasCustomAttribute) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

CustomAttributeValue *ILCustomAttribute::ParseValue() const
{
	parser_->Parse(type_, value_);
	return parser_;
}

void ILCustomAttribute::UpdateValue()
{
	value_.clear();
	parser_->Write(value_);
}

/**
 * ILDeclSecurity
 */

ILDeclSecurity::ILDeclSecurity(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), action_(0), parent_(NULL), permission_set_pos_(0)
{

}

ILDeclSecurity::ILDeclSecurity(ILMetaData *meta, ILTable *owner, const ILDeclSecurity &src)
	: ILToken(meta, owner, src), permission_set_pos_(0)
{
	action_ = src.action_;
	parent_ = src.parent_;
	permission_set_ = src.permission_set_;
}

ILToken *ILDeclSecurity::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILDeclSecurity *token = new ILDeclSecurity(meta, owner, *this);
	return token;
}

void ILDeclSecurity::ReadFromFile(NETArchitecture &file)
{
	action_ = file.ReadWord();
	parent_ = ReadTokenFromFile(file, HasDeclSecurity);
	permission_set_ = ReadBlobFromFile(file);
}

void ILDeclSecurity::WriteToStreams(ILMetaData &data)
{
	permission_set_pos_ = data.AddBlob(permission_set_);
}

void ILDeclSecurity::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(action_);
	WriteTokenToFile(file, HasDeclSecurity, parent_);
	WriteBlobToFile(file, permission_set_pos_);
}

void ILDeclSecurity::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
}

/**
 * ILEvent
 */

ILEvent::ILEvent(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), parent_(NULL), name_pos_(0), declaring_type_(NULL)
{

}

ILEvent::ILEvent(ILMetaData *meta, ILTable *owner, const ILEvent &src)
	: ILToken(meta, owner, src), name_pos_(0)
{
	flags_ = src.flags_;
	name_ = src.name_;
	parent_ = src.parent_;
	declaring_type_ = src.declaring_type_;
}

ILToken *ILEvent::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILEvent *token = new ILEvent(meta, owner, *this);
	return token;
}

void ILEvent::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadWord();
	name_ = ReadStringFromFile(file);
	parent_ = ReadTokenFromFile(file, TypeDefRef);
}

void ILEvent::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
}

void ILEvent::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteTokenToFile(file, TypeDefRef, parent_);
}

void ILEvent::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef *>(meta()->token(declaring_type_->id()));
	if (parent_)
		parent_ = meta()->token(parent_->id());
}

void ILEvent::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(declaring_type_);
	if (it != token_map.end()) {
		declaring_type_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!declaring_type_)
			set_deleted(true);
	}

	it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!parent_)
			set_deleted(true);
	}
}

bool ILEvent::is_exported() const
{
	return parent_->is_exported();
}

/**
 * ILEventMap
 */

ILEventMap::ILEventMap(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(0), event_list_(0)
{

}

ILEventMap::ILEventMap(ILMetaData *meta, ILTable *owner, const ILEventMap &src)
	: ILToken(meta, owner, src)
{
	parent_ = src.parent_;
	event_list_ = src.event_list_;
}

ILToken *ILEventMap::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILEventMap *token = new ILEventMap(meta, owner, *this);
	return token;
}

void ILEventMap::ReadFromFile(NETArchitecture &file)
{
	parent_ = reinterpret_cast<ILTypeDef *>(ReadTokenFromFile(file, ttTypeDef));
	event_list_ = reinterpret_cast<ILEvent *>(ReadTokenFromFile(file, ttEvent));
}

void ILEventMap::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttTypeDef, parent_);

	ILEvent *event_list = NULL;
	for (ILEventMap *src = this; src != NULL && event_list == NULL; src = src->next())
		event_list = src->event_list_;
	WriteTokenListToFile(file, ttEvent, event_list);
}

void ILEventMap::UpdateTokens()
{
	if (parent_)
		parent_ = reinterpret_cast<ILTypeDef *>(meta()->token(parent_->id()));
	if (event_list_)
		event_list_ = reinterpret_cast<ILEvent *>(meta()->token(event_list_->id()));
}

/**
 * ILField
 */

ILField::ILField(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), declaring_type_(NULL), signature_pos_(0), name_pos_(0)
{
	signature_ = new ILSignature(meta);
}

ILField::ILField(ILMetaData *meta, ILTable *owner, const ILField &src)
	: ILToken(meta, owner, src), signature_pos_(0), name_pos_(0)
{
	flags_ = src.flags_;
	name_ = src.name_;
	declaring_type_ = src.declaring_type_;
	signature_ = src.signature_->Clone(meta);
}

ILField::~ILField()
{
	delete signature_;
}

ILToken *ILField::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILField *token = new ILField(meta, owner, *this);
	return token;
}

void ILField::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadWord();
	name_ = ReadStringFromFile(file);
	signature_->Parse(ReadBlobFromFile(file));
}

void ILField::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef *>(meta()->token(declaring_type_->id()));

	signature_->UpdateTokens();
}

void ILField::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(declaring_type_);
	if (it != token_map.end()) {
		declaring_type_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!declaring_type_)
			set_deleted(true);
	}
	signature_->RemapTokens(token_map);
}

void ILField::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	signature_pos_ = data.AddBlob(signature_->data());
}

void ILField::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteBlobToFile(file, signature_pos_);
}

bool ILField::is_exported() const
{
	if (declaring_type_->is_exported()) {
		switch (flags_ & fdFieldAccessMask) {
		case fdFamily:
		case fdFamORAssem:
		case fdPublic:
			return true;
		}
	}

	return false;
}

std::string ILField::full_name(bool mode) const
{
	return ILName(signature_->ret_name(mode), declaring_type_ ? declaring_type_->full_name() : "<NULL>", name_, "");
}

/**
 * ILFieldLayout
 */

ILFieldLayout::ILFieldLayout(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), offset_(0), field_(NULL)
{

}

ILFieldLayout::ILFieldLayout(ILMetaData *meta, ILTable *owner, const ILFieldLayout &src)
	: ILToken(meta, owner, src)
{
	offset_ = src.offset_;
	field_ = src.field_;
}

ILToken *ILFieldLayout::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILFieldLayout *token = new ILFieldLayout(meta, owner, *this);
	return token;
}

void ILFieldLayout::ReadFromFile(NETArchitecture &file)
{
	offset_ = file.ReadDWord();
	field_ = reinterpret_cast<ILField *>(ReadTokenFromFile(file, ttField));
}

void ILFieldLayout::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(offset_);
	WriteTokenToFile(file, ttField, field_);
}

void ILFieldLayout::UpdateTokens()
{
	if (field_)
		field_ = reinterpret_cast<ILField *>(meta()->token(field_->id()));
}

void ILFieldLayout::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(field_);
	if (it != token_map.end()) {
		field_ = reinterpret_cast<ILField *>(it->second);
		if (!field_)
			set_deleted(true);
	}
}

/**
 * ILFieldMarshal
 */

ILFieldMarshal::ILFieldMarshal(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(NULL), native_type_pos_(0)
{

}

ILFieldMarshal::ILFieldMarshal(ILMetaData *meta, ILTable *owner, const ILFieldMarshal &src)
	: ILToken(meta, owner, src), native_type_pos_(0)
{
	parent_ = src.parent_;
	native_type_ = src.native_type_;
}

ILToken *ILFieldMarshal::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILFieldMarshal *token = new ILFieldMarshal(meta, owner, *this);
	return token;
}

void ILFieldMarshal::ReadFromFile(NETArchitecture &file)
{
	parent_ = ReadTokenFromFile(file, HasFieldMarshal);
	native_type_ = ReadBlobFromFile(file);
}

void ILFieldMarshal::WriteToStreams(ILMetaData &data)
{
	native_type_pos_ = data.AddBlob(native_type_);
}

void ILFieldMarshal::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, HasFieldMarshal, parent_);
	WriteBlobToFile(file, native_type_pos_);
}

void ILFieldMarshal::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
}

/**
 * ILFieldRVA
 */

ILFieldRVA::ILFieldRVA(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), address_(0), field_(NULL)
{

}

ILFieldRVA::ILFieldRVA(ILMetaData *meta, ILTable *owner, const ILFieldRVA &src)
	: ILToken(meta, owner, src)
{
	address_ = src.address_;
	field_ = src.field_;
}

ILToken *ILFieldRVA::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILFieldRVA *token = new ILFieldRVA(meta, owner, *this);
	return token;
}

void ILFieldRVA::ReadFromFile(NETArchitecture &file)
{
	address_ = file.ReadDWord() + file.image_base();
	field_ = reinterpret_cast<ILField *>(ReadTokenFromFile(file, ttField));
}

void ILFieldRVA::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(static_cast<uint32_t>(address_ - file.image_base()));
	WriteTokenToFile(file, ttField, field_);
}

void ILFieldRVA::UpdateTokens()
{
	if (field_)
		field_ = reinterpret_cast<ILField *>(meta()->token(field_->id()));
}

void ILFieldRVA::Rebase(uint64_t delta_base)
{
	ILToken::Rebase(delta_base);

	address_ += delta_base;
}

void ILFieldRVA::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(field_);
	if (it != token_map.end()) {
		field_ = reinterpret_cast<ILField *>(it->second);
		if (!field_)
			set_deleted(true);
	}
}

/**
 * ILFile
 */

ILFile::ILFile(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), name_pos_(0), value_pos_(0)
{

}

void ILFile::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadDWord();
	name_ = ReadStringFromFile(file);
	value_ = ReadBlobFromFile(file);
}

void ILFile::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	value_pos_ = data.AddBlob(value_);
}

void ILFile::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteBlobToFile(file, value_pos_);
}

/**
 * ILGenericParam
 */

ILGenericParam::ILGenericParam(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), number_(0), flags_(0), parent_(NULL), name_pos_(0)
{

}

ILGenericParam::ILGenericParam(ILMetaData *meta, ILTable *owner, const ILGenericParam &src)
	: ILToken(meta, owner, src), name_pos_(0)
{
	number_ = src.number_;
	flags_ = src.flags_;
	parent_ = src.parent_;
	name_ = src.name_;
}

ILToken *ILGenericParam::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILGenericParam *token = new ILGenericParam(meta, owner, *this);
	return token;
}

void ILGenericParam::ReadFromFile(NETArchitecture &file)
{
	number_ = file.ReadWord();
	flags_ = file.ReadWord();
	parent_ = ReadTokenFromFile(file, TypeMethodDef);
	name_ = ReadStringFromFile(file);
}

void ILGenericParam::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
}

void ILGenericParam::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(number_);
	file.WriteWord(flags_);
	WriteTokenToFile(file, TypeMethodDef, parent_);
	WriteStringToFile(file, name_pos_);
}

void ILGenericParam::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
}

void ILGenericParam::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = it->second;
		if (!parent_)
			set_deleted(true);
	}
}

int ILGenericParam::CompareWith(const ILToken &other) const
{
	if (other.type() == ttGenericParam) {
		const ILGenericParam &obj = reinterpret_cast<const ILGenericParam &>(other);
		uint32_t self_value = parent_ ? parent_->Encode(TypeMethodDef) : 0;
		uint32_t other_value = obj.parent_ ? obj.parent_->Encode(TypeMethodDef) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;

		if (number_ < obj.number_)
			return -1;
		if (number_ > obj.number_)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILGenericParamConstraint
 */

ILGenericParamConstraint::ILGenericParamConstraint(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(0), constraint_(0)
{

}

ILGenericParamConstraint::ILGenericParamConstraint(ILMetaData *meta, ILTable *owner, const ILGenericParamConstraint &src)
	: ILToken(meta, owner, src)
{
	parent_ = src.parent_;
	constraint_ = src.constraint_;
}

ILToken *ILGenericParamConstraint::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILGenericParamConstraint *token = new ILGenericParamConstraint(meta, owner, *this);
	return token;
}

void ILGenericParamConstraint::ReadFromFile(NETArchitecture &file)
{
	parent_ = reinterpret_cast<ILGenericParam *>(ReadTokenFromFile(file, ttGenericParam));
	constraint_ = ReadTokenFromFile(file, TypeDefRef);
}

void ILGenericParamConstraint::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttGenericParam, parent_);
	WriteTokenToFile(file, TypeDefRef, constraint_);
}

void ILGenericParamConstraint::UpdateTokens()
{
	if (parent_)
		parent_ = reinterpret_cast<ILGenericParam *>(meta()->token(parent_->id()));
	if (constraint_)
		constraint_ = meta()->token(constraint_->id());
}


void ILGenericParamConstraint::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = reinterpret_cast<ILGenericParam *>(it->second);
		if (!parent_)
			set_deleted(true);
	}
	it = token_map.find(constraint_);
	if (it != token_map.end()) {
		constraint_ = it->second;
		if (!constraint_)
			set_deleted(true);
	}
}

int ILGenericParamConstraint::CompareWith(const ILToken &other) const
{
	if (other.type() == ttGenericParamConstraint) {
		const ILGenericParamConstraint &obj = reinterpret_cast<const ILGenericParamConstraint &>(other);
		uint32_t self_value = parent_->value();
		uint32_t other_value = obj.parent_->value();
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILImplMap
 */

ILImplMap::ILImplMap(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), mapping_flags_(0), member_forwarded_(0), import_scope_(0), import_name_pos_(0)
{

}

ILImplMap::ILImplMap(ILMetaData *meta, ILTable *owner, const ILImplMap &src)
	: ILToken(meta, owner, src), import_name_pos_(0)
{
	mapping_flags_ = src.mapping_flags_;
	member_forwarded_ = src.member_forwarded_;
	import_name_ = src.import_name_;
	import_scope_ = src.import_scope_;
}

ILToken *ILImplMap::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILImplMap *token = new ILImplMap(meta, owner, *this);
	return token;
}

void ILImplMap::ReadFromFile(NETArchitecture &file)
{
	mapping_flags_ = file.ReadWord();
	member_forwarded_ = ReadTokenFromFile(file, MemberForwarded);
	import_name_ = ReadStringFromFile(file);
	import_scope_ = reinterpret_cast<ILModuleRef *>(ReadTokenFromFile(file, ttModuleRef));
}

void ILImplMap::WriteToStreams(ILMetaData &data)
{
	import_name_pos_ = data.AddString(import_name_);
}

void ILImplMap::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(mapping_flags_);
	WriteTokenToFile(file, MemberForwarded, member_forwarded_);
	WriteStringToFile(file, import_name_pos_);
	WriteTokenToFile(file, ttModuleRef, import_scope_);
}

void ILImplMap::UpdateTokens()
{
	if (member_forwarded_)
		member_forwarded_ = meta()->token(member_forwarded_->id());
	if (import_scope_)
		import_scope_ = reinterpret_cast<ILModuleRef *>(meta()->token(import_scope_->id()));
}

void ILImplMap::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(member_forwarded_);
	if (it != token_map.end()) {
		member_forwarded_ = it->second;
		if (!member_forwarded_)
			set_deleted(true);
	}
	it = token_map.find(import_scope_);
	if (it != token_map.end()) {
		import_scope_ = reinterpret_cast<ILModuleRef *>(it->second);
		if (!import_scope_)
			set_deleted(true);
	}
}

int ILImplMap::CompareWith(const ILToken &other) const
{
	if (other.type() == ttImplMap) {
		const ILImplMap &obj = reinterpret_cast<const ILImplMap &>(other);
		uint32_t self_value = member_forwarded_ ? member_forwarded_->Encode(MemberForwarded) : 0;
		uint32_t other_value = obj.member_forwarded_ ? obj.member_forwarded_->Encode(MemberForwarded) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILInterfaceImpl
 */

ILInterfaceImpl::ILInterfaceImpl(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), class_(0), interface_(0)
{

}

ILInterfaceImpl::ILInterfaceImpl(ILMetaData *meta, ILTable *owner, const ILInterfaceImpl &src)
	: ILToken(meta, owner, src)
{
	class_ = src.class_;
	interface_ = src.interface_;
}

ILToken *ILInterfaceImpl::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILInterfaceImpl *token = new ILInterfaceImpl(meta, owner, *this);
	return token;
}

void ILInterfaceImpl::ReadFromFile(NETArchitecture &file)
{
	class_ = reinterpret_cast<ILTypeDef *>(ReadTokenFromFile(file, ttTypeDef));
	interface_ = ReadTokenFromFile(file, TypeDefRef);
}

void ILInterfaceImpl::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttTypeDef, class_);
	WriteTokenToFile(file, TypeDefRef, interface_);
}

void ILInterfaceImpl::UpdateTokens()
{
	if (class_)
		class_ = reinterpret_cast<ILTypeDef *>(meta()->token(class_->id()));
	if (interface_)
		interface_ = meta()->token(interface_->id());
}

void ILInterfaceImpl::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(class_);
	if (it != token_map.end()) {
		class_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!class_)
			set_deleted(true);
	}
	it = token_map.find(interface_);
	if (it != token_map.end()) {
		interface_ = it->second;
		if (!interface_)
			set_deleted(true);
	}
}

int ILInterfaceImpl::CompareWith(const ILToken &other) const
{
	if (other.type() == ttInterfaceImpl) {
		const ILInterfaceImpl &obj = reinterpret_cast<const ILInterfaceImpl &>(other);
		uint32_t self_value = class_ ? class_->value() : 0;
		uint32_t other_value = obj.class_ ? obj.class_->value() : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;

		self_value = interface_ ? interface_->Encode(TypeDefRef) : 0;
		other_value = obj.interface_ ? obj.interface_->Encode(TypeDefRef) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILManifestResource
 */

ILManifestResource::ILManifestResource(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), offset_(0), flags_(0), implementation_(NULL), name_pos_(0)
{
	value_ = new ManifestResourceValue();
}

ILManifestResource::ILManifestResource(ILMetaData *meta, ILTable *owner, const ILManifestResource &src)
	: ILToken(meta, owner, src), name_pos_(0)
{
	offset_ = src.offset_;
	flags_ = src.flags_;
	name_ = src.name_;
	implementation_ = src.implementation_;
	value_ = src.value_->Clone();
}

ILManifestResource::~ILManifestResource()
{
	delete value_;
}

ILToken *ILManifestResource::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILManifestResource *token = new ILManifestResource(meta, owner, *this);
	return token;
}

void ILManifestResource::ReadFromFile(NETArchitecture &file)
{
	offset_ = file.ReadDWord();
	flags_ = file.ReadDWord();
	name_ = ReadStringFromFile(file);
	implementation_ = ReadTokenFromFile(file, Implementation);

	if (!implementation_) {
		if (uint64_t resource_address = file.header().Resources.VirtualAddress) {
			uint64_t pos = file.Tell();
			value_->ReadFromFile(file, resource_address + file.image_base() + offset_);
			file.Seek(pos);
		}
	}
}

void ILManifestResource::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
}

void ILManifestResource::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(offset_);
	file.WriteDWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteTokenToFile(file, Implementation, implementation_);
}

void ILManifestResource::UpdateTokens()
{
	if (implementation_)
		implementation_ = meta()->token(implementation_->id());
}

void ILManifestResource::UpdateValue()
{
	NETStream stream;

	value_->WriteToStream(stream);
	data_ = stream.data();
}

/**
* ManifestResourceItem
*/

ManifestResourceItem::ManifestResourceItem(ManifestResourceValue *owner, const std::string &name, uint32_t type, uint64_t address, uint32_t size, const std::vector<uint8_t> &data)
	: IObject(), owner_(owner), name_(name), type_(type), address_(address), size_(size), data_(data), baml_(NULL)
{

}

ManifestResourceItem::ManifestResourceItem(ManifestResourceValue *owner, const ManifestResourceItem &src)
	: IObject(), owner_(owner), baml_(NULL)
{
	name_ = src.name_;
	type_ = src.type_;
	address_ = src.address_;
	size_ = src.size_;
	data_ = src.data_;
}

ManifestResourceItem::~ManifestResourceItem()
{
	if (owner_)
		owner_->RemoveObject(this);
	delete baml_;
}

ManifestResourceItem *ManifestResourceItem::Clone(ManifestResourceValue *owner) const
{
	ManifestResourceItem *item = new ManifestResourceItem(owner, *this);
	return item;
}

BamlDocument *ManifestResourceItem::ParseBaml()
{
	if (baml_)
		delete baml_;

	NETStream stream;
	stream.Write(data_.data(), data_.size());
	stream.Seek(0, soBeginning);

	baml_ = new BamlDocument();
	if (baml_->ReadFromStream(stream))
		return baml_;
	return NULL;
}

int32_t ManifestResourceItem::name_hash() const
{
	os::unicode_string name = os::FromUTF8(name_);

	uint32_t hash = 5381;
	for (size_t j = 0; j < name.size(); j++) {
		hash = ((hash << 5) + hash) ^ name[j];
	}
	return hash;
}

std::vector<uint8_t> ManifestResourceItem::data() const
{
	if (baml_) {
		NETStream stream;
		baml_->WriteToStream(stream);
		return stream.data();
	}

	return data_;
}

/**
* ManifestResourceValue
*/

ManifestResourceValue::ManifestResourceValue()
	: ObjectList<ManifestResourceItem>()
{

}

ManifestResourceValue::ManifestResourceValue(const ManifestResourceValue &src)
{
	address_ = src.address_;
	size_ = src.size_;

	magic_ = src.magic_;
	header_version_ = src.header_version_;
	reader_type_name_ = src.reader_type_name_;
	set_type_name_ = src.set_type_name_;
	reader_version_ = src.reader_version_;

	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ManifestResourceValue *ManifestResourceValue::Clone() const
{
	ManifestResourceValue *value = new ManifestResourceValue(*this);
	return value;
}

enum ResourceTypeCode {
	// Primitives
	Null = 0,
	String = 1,
	Boolean = 2,
	Char = 3,
	Byte = 4,
	SByte = 5,
	Int16 = 6,
	UInt16 = 7,
	Int32 = 8,
	UInt32 = 9,
	Int64 = 0xa,
	UInt64 = 0xb,
	Single = 0xc,
	Double = 0xd,
	Decimal = 0xe,
	DateTime = 0xf,
	TimeSpan = 0x10,

	// Types with a special representation, like byte[] and Stream
	ByteArray = 0x20,
	Stream = 0x21,

	// User types - serialized using the binary formatter.
	StartOfUserTypes = 0x40
};

void ManifestResourceValue::ReadFromFile(NETArchitecture &file, uint64_t address)
{
	if (!file.AddressSeek(address))
		return;

	size_ = file.ReadDWord();
	address_ = address + sizeof(uint32_t);

	if (!size_)
		return;

	NETStream stream(*file.owner()->stream(), size_);

	magic_ = stream.ReadDWord();
	if (magic_ != 0xBEEFCACE)
		return;

	size_t i;
	header_version_ = stream.ReadDWord();
	uint32_t skip_bytes = stream.ReadDWord();
	if (header_version_ > 1) {
		stream.Seek(skip_bytes, soCurrent);
	} else {
		reader_type_name_ = stream.ReadString();
		set_type_name_ = stream.ReadString();
	}

	reader_version_ = stream.ReadDWord();
	if (reader_version_ != 1 && reader_version_ != 2)
		return;

	uint32_t num_resources = stream.ReadDWord();
	uint32_t num_types = stream.ReadDWord();
	for (i = 0; i < num_types; i++) {
		type_names_.push_back(stream.ReadString());
	}
	while (stream.Tell() & 7)
		stream.ReadByte();

	std::vector<uint32_t> name_hashes;
	for (i = 0; i < num_resources; i++) {
		name_hashes.push_back(stream.ReadDWord());
	}

	std::vector<uint32_t> name_offsets;
	for (i = 0; i < num_resources; i++) {
		name_offsets.push_back(stream.ReadDWord());
	}
	uint32_t data_section_offset = stream.ReadDWord();
	uint32_t name_section_offset = static_cast<uint32_t>(stream.Tell());

	struct ResourceInfo {
		std::string name;
		uint32_t offset;
		ResourceInfo(std::string _name, uint32_t _offset) : name(_name), offset(_offset) {}
	};

	std::sort(name_offsets.begin(), name_offsets.end());
	std::vector<ResourceInfo> info_list;
	for (i = 0; i < num_resources; i++) {
		stream.Seek(name_section_offset + name_offsets[i], soBeginning);
		std::string name = stream.ReadUnicodeString();
		uint32_t offset = data_section_offset + stream.ReadDWord();
		info_list.push_back(ResourceInfo(name, offset));
	}

	for (i = 0; i < num_resources; i++) {
		ResourceInfo info = info_list[i];
		stream.Seek(info.offset, soBeginning);
		uint32_t type = stream.ReadEncoded();
		uint32_t size;
		if (reader_version_ == 1) {
			throw std::runtime_error("Unsupported reader version");
		}
		else {
			switch ((ResourceTypeCode)type) {
			case Null:
				size = 0;
				break;
			case String:
				size = stream.ReadEncoded7Bit();
				break;
			case Boolean:
			case Byte:
			case SByte:
				size = sizeof(uint8_t);
				break;
			case Char:
			case Int16:
			case UInt16:
				size = sizeof(uint16_t);
				break;
			case Int32:
			case UInt32:
				size = sizeof(uint32_t);
				break;
			case Int64:
			case UInt64:
				size = sizeof(uint64_t);
				break;
			case Single:
				size = 4;
				break;
			case Double:
				size = 8;
				break;
			case Decimal:
				size = 16;
				break;
			case DateTime:
			case TimeSpan:
				size = sizeof(uint64_t);
				break;
			case ByteArray:
			case Stream:
				size = stream.ReadDWord();
				break;
			default:
				if (type >= StartOfUserTypes) {
					// search next data offset
					uint32_t next_offset = static_cast<uint32_t>(size_);
					for (size_t j = 0; j < info_list.size(); j++) {
						uint32_t cur_offset = info_list[j].offset;
						if (cur_offset > stream.Tell() && next_offset > cur_offset)
							next_offset = cur_offset;
					}
					size = next_offset - static_cast<uint32_t>(stream.Tell());
				}
				else
					throw std::runtime_error("Unknown type of resource");
				break;
			}
		}
		std::vector<uint8_t> data;
		if (size) {
			data.resize(size);
			stream.Read(data.data(), data.size());
		}
		Add(info.name, type, address_ + stream.Tell(), size, data);
	}
}

ManifestResourceItem *ManifestResourceValue::Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size, const std::vector<uint8_t> &data)
{
	ManifestResourceItem *item = new ManifestResourceItem(this, name, id, address, size, data);
	AddObject(item);
	return item;
}

void ManifestResourceValue::WriteToStream(NETStream &stream)
{
	size_t i;
	uint64_t pos;

	stream.WriteDWord(magic_);
	stream.WriteDWord(header_version_);

	if (header_version_ > 1) {
		stream.WriteDWord(0);
	}
	else {
		stream.WriteDWord(0);
		pos = stream.Size();
		stream.WriteString(reader_type_name_);
		stream.WriteString(set_type_name_);
		uint32_t skip_bytes = static_cast<uint32_t>(stream.Size() - pos);

		stream.Seek(pos - sizeof(uint32_t) , soBeginning);
		stream.WriteDWord(skip_bytes);
		stream.Seek(0, soEnd);
	}
	stream.WriteDWord(reader_version_);

	stream.WriteDWord(static_cast<uint32_t>(count()));
	stream.WriteDWord(static_cast<uint32_t>(type_names_.size()));
	for (i = 0; i < type_names_.size(); i++) {
		stream.WriteString(type_names_[i]);
	}
	const char *pad_str = "PAD";
	for (i = 0; static_cast<uint32_t>(stream.Size()) & 7; i++) {
		stream.WriteByte(pad_str[i % 3]);
	}

	std::vector<int32_t> name_hashes;
	for (i = 0; i < count(); i++) {
		name_hashes.push_back(item(i)->name_hash());
	}
	std::sort(name_hashes.begin(), name_hashes.end());
	for (i = 0; i < name_hashes.size(); i++) {
		stream.WriteDWord(name_hashes[i]);
	}

	uint64_t name_offsets_pos = stream.Size();
	for (i = 0; i < count(); i++) {
		stream.WriteDWord(0);
	}

	uint64_t data_section_offset_pos = stream.Size();
	stream.WriteDWord(0);

	uint64_t name_section_offset = stream.Size();

	std::vector<uint64_t> data_offset_pos;
	for (i = 0; i < count(); i++) {
		ManifestResourceItem *resource = item(i);

		pos = stream.Size();

		std::vector<int32_t>::const_iterator it = std::find(name_hashes.begin(), name_hashes.end(), resource->name_hash());
		size_t hash_index = it - name_hashes.begin();
		stream.Seek(name_offsets_pos + hash_index * sizeof(uint32_t), soBeginning);
		stream.WriteDWord(static_cast<uint32_t>(pos - name_section_offset));
		stream.Seek(0, soEnd);

		stream.WriteUnicodeString(os::FromUTF8(resource->name()));
		data_offset_pos.push_back(stream.Size());
		stream.WriteDWord(0);
	}

	uint32_t data_section_offset = static_cast<uint32_t>(stream.Size());
	stream.Seek(data_section_offset_pos, soBeginning);
	stream.WriteDWord(data_section_offset);
	stream.Seek(0, soEnd);

	for (i = 0; i < count(); i++) {
		ManifestResourceItem *resource = item(i);

		pos = stream.Size();

		stream.Seek(data_offset_pos[i], soBeginning);
		stream.WriteDWord(static_cast<uint32_t>(pos - data_section_offset));
		stream.Seek(0, soEnd);

		stream.WriteEncoded(resource->type());

		std::vector<uint8_t> data = resource->data();
		if (reader_version_ == 1) {
			// FIXME
		} else {
			switch ((ResourceTypeCode)resource->type()) {
			case String:
				stream.WriteEncoded7Bit(static_cast<uint32_t>(data.size()));
				break;
			case ByteArray:
			case Stream:
				stream.WriteDWord(static_cast<uint32_t>(data.size()));
				break;
			}
		}

		if (!data.empty())
			stream.Write(data.data(), data.size());
	}
}

/**
 * ILMemberRef
 */

ILMemberRef::ILMemberRef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), declaring_type_(NULL), signature_pos_(0), name_pos_(0)
{
	signature_ = new ILSignature(meta);
}

ILMemberRef::ILMemberRef(ILMetaData *meta, ILTable *owner, const ILMemberRef &src)
	: ILToken(meta, owner, src), signature_pos_(0), name_pos_(0)
{
	declaring_type_ = src.declaring_type_;
	name_ = src.name_;
	signature_ = src.signature_->Clone(meta);
}

ILMemberRef::~ILMemberRef()
{
	delete signature_;
}

ILToken *ILMemberRef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILMemberRef *token = new ILMemberRef(meta, owner, *this);
	return token;
}

std::string ILMemberRef::full_name(bool mode, ILSignature *method_gen_signature) const
{
	std::string type_name;
	if (declaring_type_) {
		switch (declaring_type_->type()) {
		case ttTypeDef:
			type_name = reinterpret_cast<ILTypeDef *>(declaring_type_)->name();
			break;
		case ttTypeRef:
			type_name = reinterpret_cast<ILTypeRef *>(declaring_type_)->full_name(mode);
			break;
		case ttTypeSpec:
			type_name = reinterpret_cast<ILTypeSpec *>(declaring_type_)->name(mode);
			break;
		}
	}
	else
		type_name = "<NULL>";

	return ILName(signature_->ret_name(mode), type_name, name(), signature_->is_method() ? signature_->name(mode, method_gen_signature) : "");
}

void ILMemberRef::ReadFromFile(NETArchitecture &file)
{
	declaring_type_ = ReadTokenFromFile(file, MemberRefParent);
	name_ = ReadStringFromFile(file);
	signature_->Parse(ReadBlobFromFile(file));
}

void ILMemberRef::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	signature_pos_ = data.AddBlob(signature_->data());
}

void ILMemberRef::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, MemberRefParent, declaring_type_);
	WriteStringToFile(file, name_pos_);
	WriteBlobToFile(file, signature_pos_);
}

void ILMemberRef::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = meta()->token(declaring_type_->id());
	signature_->UpdateTokens();
}

void ILMemberRef::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(declaring_type_);
	if (it != token_map.end()) {
		declaring_type_ = it->second;
		if (!declaring_type_)
			set_deleted(true);
	}
	signature_->RemapTokens(token_map);
}

/**
 * ILCustomMod
 */

ILCustomMod::ILCustomMod(ILMetaData *meta)
	: IObject(), meta_(meta), type_(ELEMENT_TYPE_END), token_(NULL)
{

}

ILCustomMod::ILCustomMod(ILMetaData *meta, const ILCustomMod &src)
	: IObject(), meta_(meta)
{
	type_ = src.type_;
	token_ = src.token_;
}

ILCustomMod *ILCustomMod::Clone(ILMetaData *meta)
{
	ILCustomMod *mod = new ILCustomMod(meta, *this);
	return mod;
}

void ILCustomMod::Parse(const ILData &data, uint32_t &id)
{
	type_ = static_cast<CorElementType>(data.ReadByte(id));
	switch (type_) {
	case ELEMENT_TYPE_CMOD_REQD:
	case ELEMENT_TYPE_CMOD_OPT:
		// do nothing
		break;
	default:
		throw std::runtime_error("Invalid mod type");
	}

	uint32_t value = data.ReadEncoded(id);
	ILTokenType ref_type;
	switch (value & 3) {
	case 0:
		ref_type = ttTypeDef;
		break;
	case 1:
		ref_type = ttTypeRef;
		break;
	case 2:
		ref_type = ttTypeSpec;
		break;
	default:
		throw std::runtime_error("Invalid token");
	}

	token_ = meta_->token(ref_type | (value >> 2));
	if (!token_)
		throw std::runtime_error("Invalid token");
}

void ILCustomMod::WriteToData(ILData &data)
{
	data.push_back(type_);
	uint32_t value = token_->value() << 2;
	switch (token_->type()) {
	case ttTypeDef:
		value |= 0;
		break;
	case ttTypeRef:
		value |= 1;
		break;
	case ttTypeSpec:
		value |= 2;
		break;
	}
	data.WriteEncoded(value);
}

std::string ILCustomMod::name(bool mode) const
{
	std::string res = (type_ == ELEMENT_TYPE_CMOD_REQD) ? "modreq" : "modopt";

	res += '(';
	switch (token_->type()) {
	case ttTypeRef:
		res += reinterpret_cast<ILTypeRef*>(token_)->full_name(mode);
		break;
	case ttTypeDef:
		res += reinterpret_cast<ILTypeDef*>(token_)->full_name();
		break;
	}
	res += ')';

	return res;
}

void ILCustomMod::UpdateTokens()
{
	if (token_)
		token_ = meta_->token(token_->id());
}

void ILCustomMod::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	if (token_) {
		std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(token_);
		if (it != token_map.end())
			token_ = it->second;
	}
}

/**
 * ILElement
 */

ILElement::ILElement(ILMetaData *meta, ILSignature *owner, CorElementType type, ILToken *token, ILElement *next)
	: IObject(), meta_(meta), owner_(owner), type_(type), next_(next), byref_(false),
	token_(token), method_(NULL), generic_param_(0), array_shape_(NULL), pinned_(false), is_predicate_(false)
{

}

ILElement::ILElement(ILMetaData *meta, ILSignature *owner, const ILElement &src)
	: IObject(), meta_(meta), owner_(owner), next_(NULL), method_(NULL), array_shape_(NULL)
{
	type_ = src.type_;
	byref_ = src.byref_;
	pinned_ = src.pinned_;
	generic_param_ = src.generic_param_;
	is_predicate_ = src.is_predicate_;
	if (src.next_)
		next_ = src.next_->Clone(meta, NULL);
	token_ = src.token_;
	if (src.method_)
		method_ = src.method_->Clone(meta);
	if (src.array_shape_)
		array_shape_ = src.array_shape_->Clone();
	for (size_t i = 0; i < src.mod_list_.size(); i++) {
		mod_list_.push_back(src.mod_list_[i]->Clone(meta));
	}
	for (size_t i = 0; i < src.child_list_.size(); i++) {
		child_list_.push_back(src.child_list_[i]->Clone(meta, NULL));
	}
}

ILElement::~ILElement()
{
	if (owner_)
		owner_->RemoveObject(this);

	for (size_t i = 0; i < mod_list_.size(); i++) {
		delete mod_list_[i];
	}
	for (size_t i = 0; i < child_list_.size(); i++) {
		delete child_list_[i];
	}

	delete next_;
	delete method_;
	delete array_shape_;
}

ILElement *ILElement::Clone(ILMetaData *meta, ILSignature *owner)
{
	ILElement *element = new ILElement(meta, owner, *this);
	return element;
}

ILCustomMod *ILElement::AddMod()
{
	ILCustomMod *mod = new ILCustomMod(meta_);
	mod_list_.push_back(mod);
	return mod;
}

ILElement *ILElement::AddElement()
{
	ILElement *element = new ILElement(meta_, NULL);
	child_list_.push_back(element);
	return element;
}

void ILElement::Parse(const ILData &data)
{
	uint32_t id = 0;
	Parse(data, id);
}

void ILElement::Parse(const ILData &data, uint32_t &id)
{
	for (bool mod_found = true; mod_found;) {
		switch (data.ReadByte(id)) {
		case ELEMENT_TYPE_BYREF:
			byref_ = true;
			break;
		case ELEMENT_TYPE_PINNED:
			pinned_ = true;
			break;
		case ELEMENT_TYPE_CMOD_REQD:
		case ELEMENT_TYPE_CMOD_OPT:
			{
				id--;
				ILCustomMod *mod = AddMod();
				mod->Parse(data, id);
			}
			break;
		default:
			mod_found = false;
			id--;
			break;
		}
	}

	type_ = static_cast<CorElementType>(data.ReadEncoded(id));

	switch (type_){
	case ELEMENT_TYPE_VOID:
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_I:
	case ELEMENT_TYPE_U:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_OBJECT:
	case ELEMENT_TYPE_TYPEDBYREF:
	case ELEMENT_TYPE_SENTINEL:
	case ELEMENT_TYPE_PINNED:
		break;
	case ELEMENT_TYPE_VALUETYPE:
	case ELEMENT_TYPE_CLASS: 
		{
			uint32_t value = data.ReadEncoded(id);
			ILTokenType ref_type;
			switch (value & 3) {
			case 0:
				ref_type = ttTypeDef;
				break;
			case 1:
				ref_type = ttTypeRef;
				break;
			case 2:
				ref_type = ttTypeSpec;
				break;
			default:
				throw std::runtime_error("Invalid token");
			}

			token_ = meta_->token(ref_type | (value >> 2));
			if (!token_)
				throw std::runtime_error("Invalid token");
		}
		break;
	case ELEMENT_TYPE_SZARRAY:
		next_ = new ILElement(meta_, NULL);
		next_->Parse(data, id);
		break;
	case ELEMENT_TYPE_PTR:
		next_ = new ILElement(meta_, NULL);
		next_->Parse(data, id);
		break;
	case ELEMENT_TYPE_FNPTR:
		method_ = new ILSignature(meta_);
		method_->Parse(data, id);
		if (!method_->is_method())
			throw std::runtime_error("Invalid signature type");
		break;
	case ELEMENT_TYPE_ARRAY:
		next_ = new ILElement(meta_, NULL);
		next_->Parse(data, id);
		array_shape_ = new ILArrayShape();
		array_shape_->Parse(data, id);
		break;
	case ELEMENT_TYPE_MVAR:
	case ELEMENT_TYPE_VAR:
		generic_param_ = data.ReadEncoded(id);
		break;
	case ELEMENT_TYPE_GENERICINST:
		next_ = new ILElement(meta_, NULL);
		next_->Parse(data, id);
		{
			uint32_t count = data.ReadEncoded(id);
			for (size_t i = 0; i < count; i++) {
				ILElement *element = AddElement();
				element->Parse(data, id);
			}
		}
		break;
	default:
		throw std::runtime_error("Invalid element type");
	}
}

std::string ILElement::name(bool mode) const
{
	std::string res;

	switch (type_) {
	case ELEMENT_TYPE_VOID:
		res += "void";
		break;
	case ELEMENT_TYPE_BOOLEAN:
		res += "bool";
		break;
	case ELEMENT_TYPE_CHAR:
		res += "char";
		break;
	case ELEMENT_TYPE_I1:
		res += "int8";
		break;
	case ELEMENT_TYPE_U1:
		res += "unsigned int8";
		break;
	case ELEMENT_TYPE_I2:
		res += "int16";
		break;
	case ELEMENT_TYPE_U2:
		res += "unsigned int16";
		break;
	case ELEMENT_TYPE_I4:
		res += "int32";
		break;
	case ELEMENT_TYPE_U4:
		res += "unsigned int32";
		break;
	case ELEMENT_TYPE_I8:
		res += "int64";
		break;
	case ELEMENT_TYPE_U8:
		res += "unsigned int64";
		break;
	case ELEMENT_TYPE_R4:
		res += "float32";
		break;
	case ELEMENT_TYPE_R8:
		res += "float64";
		break;
	case ELEMENT_TYPE_I:
		res += "native int";
		break;
	case ELEMENT_TYPE_U:
		res += "native unsigned int";
		break;
	case ELEMENT_TYPE_OBJECT:
		res += "object";
		break;
	case ELEMENT_TYPE_STRING:
		res += "string";
		break;
	case ELEMENT_TYPE_SZARRAY:
		res += next_->name(mode);
		res += "[]";
		break;
	case ELEMENT_TYPE_PTR:
		res += next_->name(mode);
		res += '*';
		break;
	case ELEMENT_TYPE_TYPEDBYREF:
		res += "typedref";
		break;
	case ELEMENT_TYPE_FNPTR:
		{
			std::string call_type = method_->type_name();
			res = string_format("method %s%s%s *%s", call_type.c_str(), call_type.empty() ? "" :  " ", method_->ret_name(mode).c_str(), method_->name(mode).c_str());
		}
		break;
	case ELEMENT_TYPE_ARRAY:
		res += next_->name(mode);
		res += ' ';
		res += array_shape_->name();
		break;
	case ELEMENT_TYPE_VALUETYPE:
	case ELEMENT_TYPE_CLASS:
		res += (type_ == ELEMENT_TYPE_VALUETYPE) ? "valuetype " : "class ";

		switch (token_->type()) {
		case ttTypeRef:
			res += reinterpret_cast<ILTypeRef*>(token_)->full_name(mode);
			break;
		case ttTypeDef:
			res += reinterpret_cast<ILTypeDef*>(token_)->full_name();
			break;
		}
		break;
	case ELEMENT_TYPE_GENERICINST:
		res += next_->name(mode);
		res += '<';
		for (size_t i = 0; i < child_list_.size(); i++) {
			if (i > 0)
				res += ", ";
			res += child_list_[i]->name(mode);
		}
		res += '>';
		break;
	case ELEMENT_TYPE_MVAR:
	case ELEMENT_TYPE_VAR:
		res += string_format("%s%d", (type_ == ELEMENT_TYPE_VAR) ? "!" : "!!", generic_param_);
		break;
	default:
		// FIXME
		res += "???";
	}

	for (size_t i = 0; i < mod_list_.size(); i++) {
		res += ' ';
		res += mod_list_[i]->name(mode);
	}

	if (byref_)
		res += '&';

	if (pinned_)
		res += " pinned";

	return res;
}

void ILElement::UpdateTokens()
{
	if (token_)
		token_ = meta_->token(token_->id());
	if (method_)
		method_->UpdateTokens();
	if (next_)
		next_->UpdateTokens();
	for (size_t i = 0; i < mod_list_.size(); i++) {
		mod_list_[i]->UpdateTokens();
	}
	for (size_t i = 0; i < child_list_.size(); i++) {
		child_list_[i]->UpdateTokens();
	}
}

void ILElement::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	if (token_) {
		std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(token_);
		if (it != token_map.end())
			token_ = it->second;
	}
	if (method_)
		method_->RemapTokens(token_map);
	if (next_)
		next_->RemapTokens(token_map);
	for (size_t i = 0; i < mod_list_.size(); i++) {
		mod_list_[i]->RemapTokens(token_map);
	}
	for (size_t i = 0; i < child_list_.size(); i++) {
		child_list_[i]->RemapTokens(token_map);
	}
}

ILData ILElement::data() const
{
	ILData data;
	WriteToData(data);
	return data;
}

void ILElement::WriteToData(ILData &data) const
{
	if (pinned_)
		data.push_back(ELEMENT_TYPE_PINNED);
	if (byref_)
		data.push_back(ELEMENT_TYPE_BYREF);

	for (size_t i = 0; i < mod_list_.size(); i++) {
		ILCustomMod *mod = mod_list_[i];
		mod->WriteToData(data);
	}
	
	data.WriteEncoded(type_);
	switch (type_) {
	case ELEMENT_TYPE_VALUETYPE:
	case ELEMENT_TYPE_CLASS: 
		{
			uint32_t value = token_->value() << 2;
			switch (token_->type()) {
			case ttTypeDef:
				value |= 0;
				break;
			case ttTypeRef:
				value |= 1;
				break;
			case ttTypeSpec:
				value |= 2;
				break;
			}
			data.WriteEncoded(value);
		}
		break;
	case ELEMENT_TYPE_SZARRAY:
	case ELEMENT_TYPE_PTR:
		next_->WriteToData(data);
		break;
	case ELEMENT_TYPE_FNPTR:
		method_->WriteToData(data);
		break;
	case ELEMENT_TYPE_ARRAY:
		next_->WriteToData(data);
		array_shape_->WriteToData(data);
		break;
	case ELEMENT_TYPE_MVAR:
	case ELEMENT_TYPE_VAR:
		data.WriteEncoded(generic_param_);
		break;
	case ELEMENT_TYPE_GENERICINST:
		next_->WriteToData(data);
		{
			data.WriteEncoded(static_cast<uint32_t>(child_list_.size()));
			for (size_t i = 0; i < child_list_.size(); i++) {
				ILElement *element = child_list_[i];
				element->WriteToData(data);
			}
		}
		break;
	}
}

/**
 * ILArrayShape
 */

ILArrayShape::ILArrayShape()
	: IObject(), rank_(0)
{

}

ILArrayShape::ILArrayShape(const ILArrayShape &src)
	: IObject()
{
	rank_ = src.rank_;
	sizes_ = src.sizes_;
	lo_bounds_ = src.lo_bounds_;
}

ILArrayShape *ILArrayShape::Clone()
{
	ILArrayShape *shape = new ILArrayShape(*this);
	return shape;
}

std::string ILArrayShape::name() const
{
	std::string res;

	res += '[';
	for (size_t i = 0; i < rank_; i++) {
		if (i > 0)
			res += ',';
		if (i < lo_bounds_.size()) {
			uint32_t lo_bound = lo_bounds_[i];
			res += string_format("%d..", lo_bound);
			if (i < sizes_.size())
				res += string_format("%d", lo_bound + sizes_[rank_] - 1);
			else
				res += '.';
		}
	}
	res += ']';
	return res;
}

void ILArrayShape::Parse(const ILData &data, uint32_t &id)
{
	rank_ = data.ReadEncoded(id);
	uint32_t count = data.ReadEncoded(id);
	for (uint32_t i = 0; i < count; i++) {
		sizes_.push_back(data.ReadEncoded(id));
	}
	count = data.ReadEncoded(id);
	for (uint32_t i = 0; i < count; i++) {
		lo_bounds_.push_back(data.ReadEncoded(id));
	}
}

void ILArrayShape::WriteToData(ILData &data) const
{
	data.WriteEncoded(rank_);
	data.WriteEncoded(static_cast<uint32_t>(sizes_.size()));
	for (uint32_t i = 0; i < sizes_.size(); i++) {
		data.WriteEncoded(sizes_[i]);
	}
	data.WriteEncoded(static_cast<uint32_t>(lo_bounds_.size()));
	for (uint32_t i = 0; i < lo_bounds_.size(); i++) {
		data.WriteEncoded(lo_bounds_[i]);
	}
}

/**
 * ILSignature
 */

ILSignature::ILSignature(ILMetaData *meta)
	: ObjectList<ILElement>(), meta_(meta), type_(stDefault), gen_param_count_(0)
{
	ret_ = new ILElement(meta, NULL);
}

ILSignature::ILSignature(ILMetaData *meta, const ILSignature &src)
	: ObjectList<ILElement>(), meta_(meta)
{
	type_ = src.type_;
	gen_param_count_ = src.gen_param_count_;
	ret_ = src.ret_->Clone(meta, this);

	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(meta, this));
	}
}

ILSignature::~ILSignature()
{
	delete ret_;
}

ILSignature *ILSignature::Clone(ILMetaData *meta)
{
	ILSignature *sign = new ILSignature(meta, *this);
	return sign;
}

ILElement *ILSignature::Add()
{
	ILElement *element = new ILElement(meta_, this);
	AddObject(element);
	return element;
}

void ILSignature::Parse(const ILData &data)
{
	uint32_t id = 0;
	Parse(data, id);
}

bool ILSignature::Parse(const ILData &data, uint32_t &id)
{
	type_ = static_cast<ILSignatureType>(data.ReadByte(id));
	if (type_ & stGeneric)
		gen_param_count_ = data.ReadEncoded(id);

	switch (type_ & 0x0f) {
	case stDefault:
	case stC:
	case stStdCall:
	case stThisCall:
	case stFastCall:
	case stVarArg:
	case stUnmanaged:
	case stNativeVarArg:
	case stProperty:
		{
			uint32_t param_count = data.ReadEncoded(id);
			ret_->Parse(data, id);
			for (uint32_t i = 0; i < param_count; i++) {
				ILElement *element = Add();
				element->Parse(data, id);
			}
		}
		break;
	case stField:
		ret_->Parse(data, id);
		break;
	case stLocal:
	case stGenericinst:
		{
			uint32_t param_count = data.ReadEncoded(id);
			for (uint32_t i = 0; i < param_count; i++) {
				ILElement *element = Add();
				element->Parse(data, id);
			}
		}
		break;
	default:
		throw std::runtime_error("Unknown signature type");
	}

	return true;
}

std::string ILSignature::name(bool mode, ILSignature *gen_signature) const
{
	std::string res;

	bool is_generic_inst = false;
	switch (type_ & 0x0f) {
	case stGenericinst:
		is_generic_inst = true;
		// fall through
	case stDefault:
	case stC:
	case stStdCall:
	case stThisCall:
	case stFastCall:
	case stVarArg:
	case stLocal:
	case stUnmanaged:
	case stNativeVarArg:
		if (type_ & stGeneric) {
			if (gen_signature)
				res += gen_signature->name(mode);
			else {
				res += '<';
				for (size_t i = 0; i < gen_param_count_; i++) {
					if (i > 0)
						res += ", ";
					res += string_format("%d", i);
				}
				res += '>';
			}
		}

		res += is_generic_inst ? '<' : '(';
		for (size_t i = 0; i < count(); i++) {
			if (i > 0)
				res += ", ";
			res += item(i)->name(mode);
		}
		res += is_generic_inst ? '>' : ')';
		break;
	}

	return res;
}

std::string ILSignature::type_name() const
{
	std::string res;

	switch (type_ & 0x0f) {
	case stDefault:
		break;
	case stC:
		break;
	case stStdCall:
		res = "stdcall";
		break;
	case stThisCall:
		res = "thiscall";
		break;
	case stFastCall:
		res = "fastcall";
		break;
	}

	return res;
}

std::string ILSignature::ret_name(bool mode) const
{
	std::string res;

	if (has_this())
		res = "instance ";

	res += ret_->name(mode);
	return res;
}

void ILSignature::WriteToData(ILData &data) const
{
	data.push_back(type_);

	if (type_ & stGeneric)
		data.WriteEncoded(gen_param_count_);

	switch (type_ & 0x0f) {
	case stDefault:
	case stC:
	case stStdCall:
	case stThisCall:
	case stFastCall:
	case stVarArg:
	case stProperty:
	case stUnmanaged:
	case stNativeVarArg:
		{
			data.WriteEncoded(static_cast<uint32_t>(count()));
			ret_->WriteToData(data);
			for (size_t i = 0; i < count(); i++) {
				item(i)->WriteToData(data);
			}
		}
		break;
	case stField:
		ret_->WriteToData(data);
		break;
	case stLocal:
	case stGenericinst:
		{
			data.WriteEncoded(static_cast<uint32_t>(count()));
			for (size_t i = 0; i < count(); i++) {
				item(i)->WriteToData(data);
			}
		}
		break;
	}
}

void ILSignature::UpdateTokens()
{
	if (ret_)
		ret_->UpdateTokens();
	for (size_t i = 0; i < count(); i++) {
		item(i)->UpdateTokens();
	}
}

void ILSignature::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	if (ret_)
		ret_->RemapTokens(token_map);
	for (size_t i = 0; i < count(); i++) {
		item(i)->RemapTokens(token_map);
	}
}

ILData ILSignature::data() const
{
	ILData data;
	WriteToData(data);
	return data;
}

bool ILSignature::is_method() const
{
	switch (type_ & 0x0f) {
	case stDefault:
	case stC:
	case stStdCall:
	case stThisCall:
	case stFastCall:
	case stVarArg:
	case stUnmanaged:
	case stNativeVarArg:
		return true;
	default:
		return false;
	}
}

bool ILSignature::is_field() const
{
	return ((type_ & 0x0f) == stField);
}
	
/**
 * ILMethod
 */

ILMethodDef::ILMethodDef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), address_(0), impl_flags_(miIL), flags_(mdPrivateScope), param_list_(NULL),
	declaring_type_(NULL), locals_(NULL), fat_size_(0), code_size_(0), signature_pos_(0), name_pos_(0)
{
	signature_ = new ILSignature(meta);
}

ILMethodDef::ILMethodDef(ILMetaData *meta, ILTable *owner, const ILMethodDef &src)
	: ILToken(meta, owner, src), signature_pos_(0), name_pos_(0)
{
	address_ = src.address_;
	impl_flags_ = src.impl_flags_;
	flags_ = src.flags_;
	name_ = src.name_;
	signature_ = src.signature_->Clone(meta);
	param_list_ = src.param_list_;
	declaring_type_ = src.declaring_type_;
	fat_size_ = src.fat_size_;
	code_size_ = src.code_size_;
	locals_ = src.locals_;
}

ILMethodDef::ILMethodDef(ILMetaData *meta, ILTable *owner, const std::string &name, const ILData &signature, CorMethodAttr flags, CorMethodImpl impl_flags)
	: ILToken(meta, owner, ttMethodDef), name_(name), flags_(flags), param_list_(NULL), locals_(NULL), fat_size_(0), code_size_(0),
	impl_flags_(impl_flags), declaring_type_(NULL), signature_pos_(0), name_pos_(0), address_(0)
{
	signature_ = new ILSignature(meta);
	signature_->Parse(signature);
}

ILMethodDef::~ILMethodDef()
{
	delete signature_;
}

ILToken *ILMethodDef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILMethodDef *method = new ILMethodDef(meta, owner, *this);
	return method;
}

void ILMethodDef::ReadFromFile(NETArchitecture &file)
{
	address_ = file.ReadDWord();
	if (address_)
		address_ += file.image_base();
	impl_flags_ = static_cast<CorMethodImpl>(file.ReadWord());
	flags_ = static_cast<CorMethodAttr>(file.ReadWord());
	name_ = ReadStringFromFile(file);
	signature_->Parse(ReadBlobFromFile(file));
	param_list_ = reinterpret_cast<ILParam *>(ReadTokenFromFile(file, ttParam));

	if (address_ && (impl_flags_ & miCodeTypeMask) == miIL) {
		uint64_t pos = file.Tell();
		if (file.AddressSeek(address_)) {
			IMAGE_COR_ILMETHOD_FAT fat = IMAGE_COR_ILMETHOD_FAT();
			file.Read(&fat, sizeof(uint8_t));

			switch (fat.Flags & CorILMethod_FormatMask) {
			case CorILMethod_TinyFormat:
				fat_size_ = 1;
				code_size_ = (fat.Flags >> CorILMethod_FormatShift);
				break;
			case CorILMethod_FatFormat:
				file.Read(reinterpret_cast<uint8_t *>(&fat) + 1, sizeof(fat) - 1);
				fat_size_ = fat.Size * sizeof(uint32_t);
				code_size_ = fat.CodeSize;
				if (fat.LocalVarSigTok) {
					ILToken *token = meta()->token(fat.LocalVarSigTok);
					if (!token || token->type() != ttStandAloneSig)
						throw std::runtime_error("Invalid method signature");
					locals_ = reinterpret_cast<ILStandAloneSig *>(token);
				}
				break;
			}
		}
		file.Seek(pos);
	}
}

void ILMethodDef::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	signature_pos_ = data.AddBlob(signature_->data());
}

void ILMethodDef::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(address_ ? static_cast<uint32_t>(address_ - file.image_base()) : 0);
	file.WriteWord(impl_flags_);
	file.WriteWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteBlobToFile(file, signature_pos_);

	ILParam *param_list = NULL;
	for (ILMethodDef *src = this; src != NULL && param_list == NULL; src = src->next())
		param_list = src->param_list_;
	WriteTokenListToFile(file, ttParam, param_list);
}

void ILMethodDef::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef *>(meta()->token(declaring_type_->id()));
	if (param_list_)
		param_list_ = reinterpret_cast<ILParam *>(meta()->token(param_list_->id()));
	if (locals_)
		locals_ = reinterpret_cast<ILStandAloneSig *>(meta()->token(locals_->id()));
	signature_->UpdateTokens();
}

void ILMethodDef::Rebase(uint64_t delta_base)
{
	ILToken::Rebase(delta_base);

	if (address_)
		address_ += delta_base;
}

void ILMethodDef::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(declaring_type_);
	if (it != token_map.end()) {
		declaring_type_ = reinterpret_cast<ILTypeDef*>(it->second);
		if (!declaring_type_)
			set_deleted(true);
	}
	signature_->RemapTokens(token_map);
}

bool ILMethodDef::is_exported() const
{
	if (declaring_type_->is_exported()) {
		switch (flags_ & mdMemberAccessMask) {
		case mdFamily:
		case mdFamORAssem:
		case mdPublic:
			return true;
		}
	}

	return false;
}

std::string ILMethodDef::full_name(bool mode, ILSignature *method_gen_signature) const
{
	return ILName(signature_->ret_name(mode), declaring_type_ ? declaring_type_->full_name() : "<NULL>", name_, signature_->name(mode, method_gen_signature));
}

/**
 * ILMethodImpl
 */

ILMethodImpl::ILMethodImpl(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), class_(0), method_body_(0), method_declaration_(0)
{

}

ILMethodImpl::ILMethodImpl(ILMetaData *meta, ILTable *owner, const ILMethodImpl &src)
	: ILToken(meta, owner, src)
{
	class_ = src.class_;
	method_body_ = src.method_body_;
	method_declaration_ = src.method_declaration_;
}
	
ILToken *ILMethodImpl::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILMethodImpl *token = new ILMethodImpl(meta, owner, *this);
	return token;
}

void ILMethodImpl::ReadFromFile(NETArchitecture &file)
{
	class_ = reinterpret_cast<ILTypeDef *>(ReadTokenFromFile(file, ttTypeDef));
	method_body_ = ReadTokenFromFile(file, MethodDefRef);
	method_declaration_ = ReadTokenFromFile(file, MethodDefRef);
}

void ILMethodImpl::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttTypeDef, class_);
	WriteTokenToFile(file, MethodDefRef, method_body_);
	WriteTokenToFile(file, MethodDefRef, method_declaration_);
}

void ILMethodImpl::UpdateTokens()
{
	if (class_)
		class_ = reinterpret_cast<ILTypeDef *>(meta()->token(class_->id()));
	if (method_body_)
		method_body_ = meta()->token(method_body_->id());
	if (method_declaration_)
		method_declaration_ = meta()->token(method_declaration_->id());
}

int ILMethodImpl::CompareWith(const ILToken &other) const
{
	if (other.type() == ttMethodImpl) {
		const ILMethodImpl &obj = reinterpret_cast<const ILMethodImpl &>(other);
		uint32_t self_value = class_ ? class_->value() : 0;
		uint32_t other_value = obj.class_ ? obj.class_->value() : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILMethodImpl
 */

ILMethodSemantics::ILMethodSemantics(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(CorMethodSemanticsAttr(0)), method_(0), association_(0)
{

}

ILMethodSemantics::ILMethodSemantics(ILMetaData *meta, ILTable *owner, const ILMethodSemantics &src)
	: ILToken(meta, owner, src)
{
	flags_ = src.flags_;
	method_ = src.method_;
	association_ = src.association_;
}

ILToken *ILMethodSemantics::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILMethodSemantics *token = new ILMethodSemantics(meta, owner, *this);
	return token;
}

void ILMethodSemantics::ReadFromFile(NETArchitecture &file)
{
	flags_ = (CorMethodSemanticsAttr)file.ReadWord();
	method_ = reinterpret_cast<ILMethodDef *>(ReadTokenFromFile(file, ttMethodDef));
	association_ = ReadTokenFromFile(file, HasSemantics);
}

void ILMethodSemantics::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(flags_);
	WriteTokenToFile(file, ttMethodDef, method_);
	WriteTokenToFile(file, HasSemantics, association_);
}

void ILMethodSemantics::UpdateTokens()
{
	if (method_)
		method_ = reinterpret_cast<ILMethodDef *>(meta()->token(method_->id()));
	if (association_)
		association_ = meta()->token(association_->id());
}

void ILMethodSemantics::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(method_);
	if (it != token_map.end()) {
		method_ = reinterpret_cast<ILMethodDef*>(it->second);
		if (!method_)
			set_deleted(true);
	}
	it = token_map.find(association_);
	if (it != token_map.end())
		association_ = it->second;
}

int ILMethodSemantics::CompareWith(const ILToken &other) const
{
	if (other.type() == ttMethodSemantics) {
		const ILMethodSemantics &obj = reinterpret_cast<const ILMethodSemantics &>(other);
		uint32_t self_value = association_ ? association_->Encode(HasSemantics) : 0;
		uint32_t other_value = obj.association_ ? obj.association_->Encode(HasSemantics) : 0;
		if (self_value < other_value)
			return -1;
		if (self_value > other_value)
			return 1;
		return 0;
	}
	return ILToken::CompareWith(other);
}

/**
 * ILMethodSpec
 */

ILMethodSpec::ILMethodSpec(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(NULL), instantiation_pos_(0)
{
	signature_ = new ILSignature(meta);
}

ILMethodSpec::ILMethodSpec(ILMetaData *meta, ILTable *owner, const ILMethodSpec &src)
	: ILToken(meta, owner, src), instantiation_pos_(0)
{
	parent_ = src.parent_;
	signature_ = src.signature_->Clone(meta);
}

ILMethodSpec::~ILMethodSpec()
{
	delete signature_;
}

ILToken *ILMethodSpec::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILMethodSpec *token = new ILMethodSpec(meta, owner, *this);
	return token;
}

void ILMethodSpec::ReadFromFile(NETArchitecture &file)
{
	parent_ = ReadTokenFromFile(file, MethodDefRef);
	signature_->Parse(ReadBlobFromFile(file));
}

void ILMethodSpec::WriteToStreams(ILMetaData &data)
{
	instantiation_pos_ = data.AddBlob(signature_->data());
}

void ILMethodSpec::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, MethodDefRef, parent_);
	WriteBlobToFile(file, instantiation_pos_);
}

void ILMethodSpec::UpdateTokens()
{
	if (parent_)
		parent_ = meta()->token(parent_->id());
	signature_->UpdateTokens();
}

void ILMethodSpec::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = it->second;
		if (!parent_)
			set_deleted(true);
	}
	signature_->RemapTokens(token_map);
}

std::string ILMethodSpec::full_name(bool mode) const
{
	if (parent_) {
		switch (parent_->type()) {
		case ttMethodDef:
			return reinterpret_cast<ILMethodDef*>(parent_)->full_name(mode, signature_);
		case ttMemberRef:
			return reinterpret_cast<ILMemberRef*>(parent_)->full_name(mode, signature_);
		}
	}

	return "<NULL>";
}

/**
 * ILModule
 */

ILModule::ILModule(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), generation_(0), name_pos_(0), mv_id_pos_(0), enc_id_pos_(0), enc_base_id_pos_(0)
{

}

ILModule::ILModule(ILMetaData *meta, ILTable *owner, const std::string &name)
	: ILToken(meta, owner, ttModule), generation_(0), name_pos_(0), mv_id_pos_(0), enc_id_pos_(0), enc_base_id_pos_(0), name_(name)
{
	for (size_t i = 0; i < 16; i++) {
		mv_id_.push_back(rand());
	}
}

ILModule::ILModule(ILMetaData *meta, ILTable *owner, const ILModule &src)
	: ILToken(meta, owner, src)
{
	generation_ = src.generation_;
	name_ = src.name_;
	mv_id_ = src.mv_id_;
	enc_id_ = src.enc_id_;
	enc_base_id_ = src.enc_base_id_;
}

ILToken *ILModule::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILModule *token = new ILModule(meta, owner, *this);
	return token;
}

void ILModule::ReadFromFile(NETArchitecture &file)
{
	generation_ = file.ReadWord();
	name_ = ReadStringFromFile(file);
	mv_id_ = ReadGUIDFromFile(file);
	enc_id_ = ReadGUIDFromFile(file);
	enc_base_id_ = ReadGUIDFromFile(file);
}

void ILModule::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	mv_id_pos_ = data.AddGuid(mv_id_);
	enc_id_pos_ = data.AddGuid(enc_id_);
	enc_base_id_pos_ = data.AddGuid(enc_base_id_);
}

void ILModule::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(generation_);
	WriteStringToFile(file, name_pos_);
	WriteGuidToFile(file, mv_id_pos_);
	WriteGuidToFile(file, enc_id_pos_);
	WriteGuidToFile(file, enc_base_id_pos_);
}

/**
 * ILModuleRef
 */

ILModuleRef::ILModuleRef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), name_pos_(0)
{

}

ILModuleRef::ILModuleRef(ILMetaData *meta, ILTable *owner, const ILModuleRef &src)
	: ILToken(meta, owner, src), name_pos_(0)
{
	name_ = src.name_;
}

ILToken *ILModuleRef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILModuleRef *token = new ILModuleRef(meta, owner, *this);
	return token;
}

void ILModuleRef::ReadFromFile(NETArchitecture &file)
{
	name_ = ReadStringFromFile(file);
}

void ILModuleRef::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
}

void ILModuleRef::WriteToFile(NETArchitecture &file)
{
	WriteStringToFile(file, name_pos_);
}

/**
 * ILNestedClass
 */

ILNestedClass::ILNestedClass(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), nested_type_(NULL), declaring_type_(NULL)
{

}

ILNestedClass::ILNestedClass(ILMetaData *meta, ILTable *owner, const ILNestedClass &src)
	: ILToken(meta, owner, src)
{
	nested_type_ = src.nested_type_;
	declaring_type_ = src.declaring_type_;
}

ILToken *ILNestedClass::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILNestedClass *token = new ILNestedClass(meta, owner, *this);
	return token;
}

void ILNestedClass::ReadFromFile(NETArchitecture &file)
{
	nested_type_ = reinterpret_cast<ILTypeDef*>(ReadTokenFromFile(file, ttTypeDef));
	declaring_type_ = reinterpret_cast<ILTypeDef*>(ReadTokenFromFile(file, ttTypeDef));
}

void ILNestedClass::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttTypeDef, nested_type_);
	WriteTokenToFile(file, ttTypeDef, declaring_type_);
}

void ILNestedClass::UpdateTokens()
{
	if (nested_type_)
		nested_type_ = reinterpret_cast<ILTypeDef*>(meta()->token(nested_type_->id()));
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef*>(meta()->token(declaring_type_->id()));
}

void ILNestedClass::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(nested_type_);
	if (it != token_map.end()) {
		nested_type_ = reinterpret_cast<ILTypeDef*>(it->second);
		if (!nested_type_)
			set_deleted(true);
	}

	it = token_map.find(declaring_type_);
	if (it != token_map.end())
		declaring_type_ = reinterpret_cast<ILTypeDef *>(it->second);
}

/**
 * ILParam
 */

ILParam::ILParam(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), sequence_(0), parent_(NULL), name_pos_(0)
{

}

ILParam::ILParam(ILMetaData *meta, ILTable *owner, const ILParam &src)
	: ILToken(meta, owner, src), name_pos_(0)
{
	flags_ = src.flags_;
	sequence_ = src.sequence_;
	name_ = src.name_;
	parent_ = src.parent_;
}

ILToken *ILParam::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILParam *param = new ILParam(meta, owner, *this);
	return param;
}

void ILParam::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadWord();
	sequence_ = file.ReadWord();
	name_ = ReadStringFromFile(file);
}

void ILParam::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
}

void ILParam::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(flags_);
	file.WriteWord(sequence_);
	WriteStringToFile(file, name_pos_);
}

void ILParam::UpdateTokens()
{
	if (parent_)
		parent_ = reinterpret_cast<ILMethodDef *>(meta()->token(parent_->id()));
}

void ILParam::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = reinterpret_cast<ILMethodDef *>(it->second);
		if (!parent_)
			set_deleted(true);
	}
}

/**
 * ILProperty
 */

ILProperty::ILProperty(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), name_pos_(0), type_pos_(0), declaring_type_(NULL)
{
	type_ = new ILSignature(meta);
}

ILProperty::ILProperty(ILMetaData *meta, ILTable *owner, const ILProperty &src)
	: ILToken(meta, owner, src), name_pos_(0), type_pos_(0)
{
	flags_ = src.flags_;
	name_ = src.name_;
	type_ = src.type_->Clone(meta);
	declaring_type_ = src.declaring_type_;
	name_pos_ = src.name_pos_;
	type_pos_ = src.type_pos_;
}

ILProperty::~ILProperty()
{
	delete type_;
}

ILToken *ILProperty::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILProperty *token = new ILProperty(meta, owner, *this);
	return token;
}

void ILProperty::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadWord();
	name_ = ReadStringFromFile(file);
	type_->Parse(ReadBlobFromFile(file));
}

void ILProperty::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	type_pos_ = data.AddBlob(type_->data());
}

void ILProperty::WriteToFile(NETArchitecture &file)
{
	file.WriteWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteBlobToFile(file, type_pos_);
}

void ILProperty::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef *>(meta()->token(declaring_type_->id()));

	type_->UpdateTokens();
}

void ILProperty::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(declaring_type_);
	if (it != token_map.end()) {
		declaring_type_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!declaring_type_)
			set_deleted(true);
	}
	type_->RemapTokens(token_map);
}

bool ILProperty::is_exported() const
{
	return declaring_type_->is_exported();
}

/**
 * ILPropertyMap
 */

ILPropertyMap::ILPropertyMap(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), parent_(0), property_list_(0)
{

}

ILPropertyMap::ILPropertyMap(ILMetaData *meta, ILTable *owner, const ILPropertyMap &src)
	: ILToken(meta, owner, src)
{
	parent_ = src.parent_;
	property_list_ = src.property_list_;
}
	
ILToken *ILPropertyMap::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILPropertyMap *token = new ILPropertyMap(meta, owner, *this);
	return token;
}

void ILPropertyMap::ReadFromFile(NETArchitecture &file)
{
	parent_ = reinterpret_cast<ILTypeDef *>(ReadTokenFromFile(file, ttTypeDef));
	property_list_ = reinterpret_cast<ILProperty *>(ReadTokenFromFile(file, ttProperty));
}

void ILPropertyMap::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ttTypeDef, parent_);

	ILProperty *property_list = NULL;
	for (ILPropertyMap *src = this; src != NULL && property_list == NULL; src = src->next())
		property_list = src->property_list_;
	WriteTokenListToFile(file, ttProperty, property_list);
}

void ILPropertyMap::UpdateTokens()
{
	if (parent_)
		parent_ = reinterpret_cast<ILTypeDef *>(meta()->token(parent_->id()));
}

void ILPropertyMap::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(parent_);
	if (it != token_map.end()) {
		parent_ = reinterpret_cast<ILTypeDef *>(it->second);
		if (!parent_)
			set_deleted(true);
	}
	it = token_map.find(property_list_);
	if (it != token_map.end())
		property_list_ = reinterpret_cast<ILProperty *>(it->second);
}

bool ILPropertyMap::is_exported() const
{
	return parent_->is_exported();
}

/**
 * ILStandAloneSig
 */

ILStandAloneSig::ILStandAloneSig(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), signature_(0), signature_pos_(0)
{
	signature_ = new ILSignature(meta);
}

ILStandAloneSig::ILStandAloneSig(ILMetaData *meta, ILTable *owner, const ILStandAloneSig &src)
	: ILToken(meta, owner, src), signature_pos_(0)
{
	signature_ = src.signature_->Clone(meta);
}

ILStandAloneSig::ILStandAloneSig(ILMetaData *meta, ILTable *owner, const ILData &data)
	: ILToken(meta, owner, ttStandAloneSig)
{
	signature_ = new ILSignature(meta);
	signature_->Parse(data);
}

ILStandAloneSig *ILStandAloneSig::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILStandAloneSig *token = new ILStandAloneSig(meta, owner, *this);
	return token;
}

ILStandAloneSig::~ILStandAloneSig()
{
	delete signature_;
}

void ILStandAloneSig::ReadFromFile(NETArchitecture &file)
{
	signature_->Parse(ReadBlobFromFile(file));
}

void ILStandAloneSig::UpdateTokens()
{
	signature_->UpdateTokens();
}

void ILStandAloneSig::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	signature_->RemapTokens(token_map);
}

void ILStandAloneSig::WriteToStreams(ILMetaData &data)
{
	signature_pos_ = data.AddBlob(signature_->data());
}

void ILStandAloneSig::WriteToFile(NETArchitecture &file)
{
	WriteBlobToFile(file, signature_pos_);
}

std::string ILStandAloneSig::name(bool mode) const
{
	std::string res;
	if (signature_->is_method())
		res += signature_->ret_name(mode);

	if (!res.empty())
		res += ' ';

	return res + signature_->name(mode);
}

/**
 * ILTypeDef
 */

ILTypeDef::ILTypeDef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(tdNotPublic), base_type_(NULL), field_list_(NULL), method_list_(NULL), declaring_type_(NULL),
	class_size_(0), name_pos_(0), namespace_pos_(0)
{

}

ILTypeDef::ILTypeDef(ILMetaData *meta, ILTable *owner, ILToken *base_type, const std::string &name_space, const std::string &name, CorTypeAttr flags)
	: ILToken(meta, owner, ttTypeDef), flags_(flags), base_type_(base_type), field_list_(NULL), method_list_(NULL), declaring_type_(NULL),
	class_size_(0), name_pos_(0), namespace_pos_(0), namespace_(name_space), name_(name)
{

}

ILTypeDef::ILTypeDef(ILMetaData *meta, ILTable *owner, const ILTypeDef &src)
	: ILToken(meta, owner, src), name_pos_(0), namespace_pos_(0)
{
	flags_ = src.flags_;
	name_ = src.name_;
	namespace_ = src.namespace_;
	base_type_ = src.base_type_;
	field_list_ = src.field_list_;
	method_list_ = src.method_list_;
	class_size_ = src.class_size_;
	declaring_type_ = src.declaring_type_;
}

ILToken *ILTypeDef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILTypeDef *token = new ILTypeDef(meta, owner, *this);
	return token;
}

void ILTypeDef::ReadFromFile(NETArchitecture &file)
{
	flags_ = static_cast<CorTypeAttr>(file.ReadDWord());
	name_ = ReadStringFromFile(file);
	namespace_ = ReadStringFromFile(file);
	base_type_ = ReadTokenFromFile(file, TypeDefRef);
	field_list_ = reinterpret_cast<ILField *>(ReadTokenFromFile(file, ttField));
	method_list_ = reinterpret_cast<ILMethodDef *>(ReadTokenFromFile(file, ttMethodDef));
}

void ILTypeDef::UpdateTokens()
{
	if (declaring_type_)
		declaring_type_ = reinterpret_cast<ILTypeDef *>(meta()->token(declaring_type_->id()));
	if (base_type_)
		base_type_ = meta()->token(base_type_->id());
	if (field_list_)
		field_list_ = reinterpret_cast<ILField *>(meta()->token(field_list_->id()));
	if (method_list_)
		method_list_ = reinterpret_cast<ILMethodDef *>(meta()->token(method_list_->id()));
}

void ILTypeDef::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(base_type_);
	if (it != token_map.end())
		base_type_ = it->second;
	it = token_map.find(field_list_);
	if (it != token_map.end())
		field_list_ = reinterpret_cast<ILField *>(it->second);
	it = token_map.find(method_list_);
	if (it != token_map.end())
		method_list_ = reinterpret_cast<ILMethodDef *>(it->second);
}

void ILTypeDef::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	namespace_pos_ = data.AddString(namespace_);
}

void ILTypeDef::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(flags_);
	WriteStringToFile(file, name_pos_);
	WriteStringToFile(file, namespace_pos_);
	WriteTokenToFile(file, TypeDefRef, base_type_);

	ILField *field_list = NULL;
	for (ILTypeDef *src = this; src != NULL && field_list == NULL; src = src->next())
		field_list = src->field_list_;
	WriteTokenListToFile(file, ttField, field_list);

	ILMethodDef *method_list = NULL;
	for (ILTypeDef *src = this; src != NULL && method_list == NULL; src = src->next())
		method_list = src->method_list_;
	WriteTokenListToFile(file, ttMethodDef, method_list);
}

std::string ILTypeDef::full_name() const
{
	std::string res;
	if (declaring_type_)
		res = declaring_type_->full_name() + '/';
	else {
		res = namespace_;
		if (!res.empty())
			res += '.';
	}
	res += name_;

	return res;
}

std::string ILTypeDef::reflection_name() const
{
	std::string res;
	if (declaring_type_)
		res = declaring_type_->full_name() + '+';
	else {
		res = namespace_;
		if (!res.empty())
			res += '.';
	}
	res += name_;

	return res;
}

bool ILTypeDef::is_exported() const
{
	const ILTypeDef *type_def = this;
	while (type_def->declaring_type()) {
		if ((type_def->flags() & (tdNestedPublic | tdNestedFamily | tdNestedFamORAssem)) == 0)
			return false;
		type_def = type_def->declaring_type();
	}
	return (type_def->flags() & tdPublic) != 0;
}

void ILTypeDef::AddMethod(ILMethodDef *method)
{
	size_t pos = NOT_ID;
	ILTable *type_table = meta()->table(ttTypeDef);
	ILTable *method_table = meta()->table(ttMethodDef);
	for (size_t i = type_table->IndexOf(this) + 1; i < type_table->count(); i++) {
		ILTypeDef *next = reinterpret_cast<ILTypeDef *>(type_table->item(i));
		if (next->method_list()) {
			pos = method_table->IndexOf(next->method_list());
			break;
		}
	}
	if (pos == NOT_ID)
		method_table->AddObject(method);
	else 
		method_table->InsertObject(pos, method);

	if (!method_list_)
		method_list_ = method;
}

ILField *ILTypeDef::GetField(const std::string &name) const
{
	if (field_list_) {
		ILTable *table = meta()->table(ttField);
		for (size_t i = table->IndexOf(field_list_); i < table->count(); i++) {
			ILField *field = reinterpret_cast<ILField *>(table->item(i));
			if (field->declaring_type() != this)
				break;

			if (field->name() == name)
				return field;
		}
	}

	return NULL;
}

ILMethodDef *ILTypeDef::GetMethod(const std::string &name, ILSignature *signature) const
{
	if (method_list_) {
		std::string full_name;
		if (signature)
			full_name = ILName(signature->ret_name(), this->full_name(), name, signature->name());
		ILTable *table = meta()->table(ttMethodDef);
		for (size_t i = table->IndexOf(method_list_); i < table->count(); i++) {
			ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
			if (method->declaring_type() != this)
				break;

			if (signature) {
				if (method->full_name() == full_name)
					return method;
			}
			else {
				if (method->name() == name)
					return method;
			}
		}
	}

	return NULL;
}

ILProperty *ILTypeDef::GetProperty(const std::string &name) const
{
	ILTable *table = meta()->table(ttProperty);
	for (size_t i = 0; i < table->count(); i++) {
		ILProperty *prop = reinterpret_cast<ILProperty *>(table->item(i));
		if (prop->declaring_type() == this && prop->name() == name)
			return prop;
	}

	return NULL;
}

ILEvent *ILTypeDef::GetEvent(const std::string &name) const
{
	ILTable *table = meta()->table(ttEvent);
	for (size_t i = 0; i < table->count(); i++) {
		ILEvent *event = reinterpret_cast<ILEvent *>(table->item(i));
		if (event->declaring_type() == this && event->name() == name)
			return event;
	}

	return NULL;
}

ILTypeDef *ILTypeDef::GetNested(const std::string &name) const
{
	ILTable *table = meta()->table(ttTypeDef);
	for (size_t i = 0; i < table->count(); i++) {
		ILTypeDef *type_def = reinterpret_cast<ILTypeDef*>(table->item(i));
		if (type_def->declaring_type() == this && type_def->name() == name)
			return type_def;
	}
	return NULL;
}

ILElement *ILTypeDef::GetEnumUnderlyingType() const
{
	ILTable *table = meta()->table(ttField);
	for (size_t i = 0; i < table->count(); i++) {
		ILField *field = reinterpret_cast<ILField*>(table->item(i));
		if (field->declaring_type() == this && (field->flags() & fdStatic) == 0)
			return field->signature()->ret();
	}
	return NULL;
}

bool ILTypeDef::FindBaseType(const std::string &name) const
{
	ILToken *base_type = base_type_;
	while (base_type) {
		ILTypeDef *type_def;
		switch (base_type->type()) {
		case ttTypeRef:
			type_def = reinterpret_cast<ILTypeRef *>(base_type)->Resolve(true);
			break;
		case ttTypeDef:
			type_def = reinterpret_cast<ILTypeDef *>(base_type);
			break;
		default:
			type_def = NULL;
			break;
		}

		if (!type_def)
			break;

		if (type_def->full_name() == name)
			return true;

		base_type = type_def->base_type();
	}
	return false;
}

bool ILTypeDef::FindImplement(const std::string &name) const
{
	const ILTypeDef *type_def = this;
	while (type_def) {
		ILTable *table = type_def->meta()->table(ttInterfaceImpl);
		for (size_t i = 0; i < table->count(); i++) {
			ILInterfaceImpl *impl = reinterpret_cast<ILInterfaceImpl *>(table->item(i));
			if (impl->parent() == type_def) {
				ILToken *token = impl->_interface();
				switch (token->type()) {
				case ttTypeRef:
					if (reinterpret_cast<ILTypeRef *>(token)->full_name() == name)
						return true;
					break;
				case ttTypeDef:
					if (reinterpret_cast<ILTypeDef *>(token)->full_name() == name)
						return true;
					break;
				}
			}
		}

		if (!type_def->base_type())
			break;
		
		ILToken *base_type = type_def->base_type();
		switch (base_type->type()) {
		case ttTypeRef:
			type_def = reinterpret_cast<ILTypeRef *>(base_type)->Resolve(true);
			break;
		case ttTypeDef:
			type_def = reinterpret_cast<ILTypeDef *>(base_type);
			break;
		default:
			type_def = NULL;
			break;
		}
	}
	return false;
}

/**
 * ILTypeRef
 */

ILTypeRef::ILTypeRef(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), resolution_scope_(NULL), name_pos_(0), namespace_pos_(0)
{

}

ILTypeRef::ILTypeRef(ILMetaData *meta, ILTable *owner, ILToken *resolution_scope, const std::string &name_space, const std::string &name)
	: ILToken(meta, owner, ttTypeRef), resolution_scope_(resolution_scope), namespace_(name_space), name_(name), name_pos_(0), namespace_pos_(0)
{

}

ILTypeRef::ILTypeRef(ILMetaData *meta, ILTable *owner, const ILTypeRef &src)
	: ILToken(meta, owner, src), name_pos_(0), namespace_pos_(0)
{
	resolution_scope_ = src.resolution_scope_;
	name_ = src.name_;
	namespace_ = src.namespace_;
}

ILToken *ILTypeRef::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILTypeRef *token = new ILTypeRef(meta, owner, *this);
	return token;
}

std::string ILTypeRef::module_name() const
{
	ILTypeRef *declaring_type = this->declaring_type();
	if (declaring_type)
		return declaring_type->module_name();

	ILToken *scope = this->resolution_scope();
	if (scope) {
		switch (scope->type()) {
		case ttAssemblyRef:
			return reinterpret_cast<ILAssemblyRef*>(scope)->name();
		case ttModuleRef:
			return reinterpret_cast<ILModuleRef*>(scope)->name();
		}
	}

	return std::string();
}

ILTypeRef *ILTypeRef::declaring_type() const
{
	ILToken *scope = this->resolution_scope();
	if (scope && scope->type() == ttTypeRef)
		return reinterpret_cast<ILTypeRef*>(scope);
	return NULL;
}

std::string ILTypeRef::full_name(bool mode) const
{
	std::string res;

	res = namespace_;
	if (!res.empty())
		res += '.';
	res += name_;

	ILTypeRef *declaring_type = this->declaring_type();
	while (declaring_type) {
		res = declaring_type->full_name() + '/' + res;
		declaring_type = declaring_type->declaring_type();
	}

	if (mode)
		res = '[' + module_name() + ']' + res;

	return res;
}

void ILTypeRef::ReadFromFile(NETArchitecture &file)
{
	resolution_scope_ = ReadTokenFromFile(file, ResolutionScope);
	name_ = ReadStringFromFile(file);
	namespace_ = ReadStringFromFile(file);
}

void ILTypeRef::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	namespace_pos_ = data.AddString(namespace_);
}

void ILTypeRef::WriteToFile(NETArchitecture &file)
{
	WriteTokenToFile(file, ResolutionScope, resolution_scope_);
	WriteStringToFile(file, name_pos_);
	WriteStringToFile(file, namespace_pos_);
}

void ILTypeRef::UpdateTokens()
{
	if (resolution_scope_)
		resolution_scope_ = meta()->token(resolution_scope_->id());
}

void ILTypeRef::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	std::map<ILToken*, ILToken*>::const_iterator it = token_map.find(resolution_scope_);
	if (it != token_map.end())
		resolution_scope_ = it->second;
}

ILTypeDef *ILTypeRef::Resolve(bool show_error) const
{
	return meta()->Resolve(this, show_error);
}

/**
 * ILTypeSpec
 */

ILTypeSpec::ILTypeSpec(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), signature_pos_(0)
{
	signature_ = new ILElement(meta, NULL);
}

ILTypeSpec::ILTypeSpec(ILMetaData *meta, ILTable *owner, const ILTypeSpec &src)
	: ILToken(meta, owner, src), signature_pos_(0)
{
	signature_ = src.signature_->Clone(meta, NULL);
}

ILTypeSpec::ILTypeSpec(ILMetaData *meta, ILTable *owner, ILData &data)
	: ILToken(meta, owner, ttTypeSpec)
{
	signature_ = new ILElement(meta, NULL);
	signature_->Parse(data);
}

ILTypeSpec::~ILTypeSpec()
{
	delete signature_;
}

ILToken *ILTypeSpec::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILTypeSpec *token = new ILTypeSpec(meta, owner, *this);
	return token;
}

void ILTypeSpec::ReadFromFile(NETArchitecture &file)
{
	signature_->Parse(ReadBlobFromFile(file));
}

void ILTypeSpec::WriteToStreams(ILMetaData &data)
{
	signature_pos_ = data.AddBlob(signature_->data());
}

void ILTypeSpec::WriteToFile(NETArchitecture &file)
{
	WriteBlobToFile(file, signature_pos_);
}

void ILTypeSpec::UpdateTokens()
{
	signature_->UpdateTokens();
}

void ILTypeSpec::RemapTokens(const std::map<ILToken*, ILToken*> &token_map)
{
	signature_->RemapTokens(token_map);
}

/**
 * ILUserString
 */

ILUserString::ILUserString(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id)
{
	name_ = ReadUserString(value());
}
	
ILUserString::ILUserString(ILMetaData *meta, ILTable *owner, const ILUserString &src)
	: ILToken(meta, owner, src)
{
	name_ = src.name_;
}

ILToken *ILUserString::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILToken *token = new ILUserString(meta, owner, *this);
	return token;
}

/**
* ILExportedType
*/

ILExportedType::ILExportedType(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), flags_(0), type_def_id_(0), implementation_(NULL)
{

}

ILExportedType::ILExportedType(ILMetaData *meta, ILTable *owner, const ILExportedType &src)
	: ILToken(meta, owner, src)
{
	flags_ = src.flags_;
	type_def_id_ = src.type_def_id_;
	name_ = src.name_;
	namespace_ = src.namespace_;
	implementation_ = src.implementation_;
}

ILToken *ILExportedType::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILToken *token = new ILExportedType(meta, owner, *this);
	return token;
}

void ILExportedType::ReadFromFile(NETArchitecture &file)
{
	flags_ = file.ReadDWord();
	type_def_id_ = file.ReadDWord();
	name_ = ReadStringFromFile(file);
	namespace_ = ReadStringFromFile(file);
	implementation_ = ReadTokenFromFile(file, Implementation);
}

void ILExportedType::WriteToStreams(ILMetaData &data)
{
	name_pos_ = data.AddString(name_);
	namespace_pos_ = data.AddString(namespace_);
}

void ILExportedType::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(flags_);
	file.WriteDWord(type_def_id_);
	WriteStringToFile(file, name_pos_);
	WriteStringToFile(file, namespace_pos_);
	WriteTokenToFile(file, Implementation, implementation_);
}

void ILExportedType::UpdateTokens()
{
	if (implementation_)
		implementation_ = meta()->token(implementation_->id());
}

ILExportedType *ILExportedType::declaring_type() const
{
	ILToken *scope = implementation();
	if (scope && scope->type() == ttExportedType)
		return reinterpret_cast<ILExportedType*>(scope);
	return NULL;
}

std::string ILExportedType::full_name() const
{
	std::string res;

	res = namespace_;
	if (!res.empty())
		res += '.';
	res += name_;

	ILExportedType *declaring_type = this->declaring_type();
	while (declaring_type) {
		res = declaring_type->full_name() + '/' + res;
		declaring_type = declaring_type->declaring_type();
	}

	return res;
}

/**
* ILENCLog
*/

ILEncLog::ILEncLog(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), token_(0), func_code_(0)
{

}

ILEncLog::ILEncLog(ILMetaData *meta, ILTable *owner, const ILEncLog &src)
	: ILToken(meta, owner, src)
{
	token_ = src.token_;
	func_code_ = src.func_code_;
}

ILToken *ILEncLog::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILToken *token = new ILEncLog(meta, owner, *this);
	return token;
}

void ILEncLog::ReadFromFile(NETArchitecture &file)
{
	token_ = file.ReadDWord();
	func_code_ = file.ReadDWord();
}

void ILEncLog::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(token_);
	file.WriteDWord(func_code_);
}

/**
* ILENCMap
*/

ILEncMap::ILEncMap(ILMetaData *meta, ILTable *owner, uint32_t id)
	: ILToken(meta, owner, id), token_(0)
{

}

ILEncMap::ILEncMap(ILMetaData *meta, ILTable *owner, const ILEncMap &src)
	: ILToken(meta, owner, src)
{
	token_ = src.token_;
}

ILToken *ILEncMap::Clone(ILMetaData *meta, ILTable *owner) const
{
	ILToken *token = new ILEncMap(meta, owner, *this);
	return token;
}

void ILEncMap::ReadFromFile(NETArchitecture &file)
{
	token_ = file.ReadDWord();
}

void ILEncMap::WriteToFile(NETArchitecture &file)
{
	file.WriteDWord(token_);
}

/**
 * ILTable
 */

ILTable::ILTable(ILMetaData *meta, ILTokenType type, uint32_t token_count)
	: ObjectList<ILToken>(), meta_(meta), type_(type)
{
	for (uint32_t i = 1; i <= token_count; i++) {
		Add(type_ | i);
	}
}

ILTable::ILTable(ILMetaData *meta, const ILTable &src)
	: ObjectList<ILToken>(), meta_(meta)
{
	type_ = src.type_;

	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(meta, this));
	}
}

ILTable *ILTable::Clone(ILMetaData *meta) const
{
	ILTable *table = new ILTable(meta, *this);
	return table;
}

ILToken *ILTable::Add(uint32_t id)
{
	ILToken *token = NULL;
	switch (type_) {
	case ttAssembly:
		token = new ILAssembly(meta_, this, id);
		break;
	case ttAssemblyOS:
		token = new ILAssemblyOS(meta_, this, id);
		break;
	case ttAssemblyProcessor:
		token = new ILAssemblyProcessor(meta_, this, id);
		break;
	case ttAssemblyRef:
		token = new ILAssemblyRef(meta_, this, id);
		break;
	case ttAssemblyRefOS:
		token = new ILAssemblyRefOS(meta_, this, id);
		break;
	case ttAssemblyRefProcessor:
		token = new ILAssemblyRefProcessor(meta_, this, id);
		break;
	case ttClassLayout:
		token = new ILClassLayout(meta_, this, id);
		break;
	case ttConstant:
		token = new ILConstant(meta_, this, id);
		break;
	case ttCustomAttribute:
		token = new ILCustomAttribute(meta_, this, id);
		break;
	case ttDeclSecurity:
		token = new ILDeclSecurity(meta_, this, id);
		break;
	case ttEvent:
		token = new ILEvent(meta_, this, id);
		break;
	case ttEventMap:
		token = new ILEventMap(meta_, this, id);
		break;
	case ttField:
		token = new ILField(meta_, this, id);
		break;
	case ttFieldLayout:
		token = new ILFieldLayout(meta_, this, id);
		break;
	case ttFieldMarshal:
		token = new ILFieldMarshal(meta_, this, id);
		break;
	case ttFieldRVA:
		token = new ILFieldRVA(meta_, this, id);
		break;
	case ttFile:
		token = new ILFile(meta_, this, id);
		break;
	case ttGenericParam:
		token = new ILGenericParam(meta_, this, id);
		break;
	case ttGenericParamConstraint:
		token = new ILGenericParamConstraint(meta_, this, id);
		break;
	case ttImplMap:
		token = new ILImplMap(meta_, this, id);
		break;
	case ttInterfaceImpl:
		token = new ILInterfaceImpl(meta_, this, id);
		break;
	case ttManifestResource:
		token = new ILManifestResource(meta_, this, id);
		break;
	case ttMemberRef:
		token = new ILMemberRef(meta_, this, id);
		break;
	case ttMethodDef:
		token = new ILMethodDef(meta_, this, id);
		break;
	case ttMethodImpl:
		token = new ILMethodImpl(meta_, this, id);
		break;
	case ttMethodSemantics:
		token = new ILMethodSemantics(meta_, this, id);
		break;
	case ttMethodSpec:
		token = new ILMethodSpec(meta_, this, id);
		break;
	case ttModule:
		token = new ILModule(meta_, this, id);
		break;
	case ttModuleRef:
		token = new ILModuleRef(meta_, this, id);
		break;
	case ttNestedClass:
		token = new ILNestedClass(meta_, this, id);
		break;
	case ttParam:
		token = new ILParam(meta_, this, id);
		break;
	case ttProperty:
		token = new ILProperty(meta_, this, id);
		break;
	case ttPropertyMap:
		token = new ILPropertyMap(meta_, this, id);
		break;
	case ttStandAloneSig:
		token = new ILStandAloneSig(meta_, this, id);
		break;
	case ttTypeDef:
		token = new ILTypeDef(meta_, this, id);
		break;
	case ttTypeRef:
		token = new ILTypeRef(meta_, this, id);
		break;
	case ttTypeSpec:
		token = new ILTypeSpec(meta_, this, id);
		break;
	case ttUserString:
		token = new ILUserString(meta_, this, id);
		break;
	case ttExportedType:
		token = new ILExportedType(meta_, this, id);
		break;
	case ttEncLog:
		token = new ILEncLog(meta_, this, id);
		break;
	case ttEncMap:
		token = new ILEncMap(meta_, this, id);
		break;
	default:
		throw std::runtime_error("Unknown token type");
	}

	AddObject(token);
	return token;
}

void ILTable::ReadFromFile(NETArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

void ILTable::WriteToStreams(ILMetaData &data)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToStreams(data);
	}
}

void ILTable::WriteToFile(NETArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

void ILTable::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

void ILTable::Pack()
{
	size_t i;
	ILToken *token;

	for (i = count(); i > 0; i--) {
		token = item(i - 1);
		if (token->is_deleted())
			delete token;
	}
}

void ILTable::UpdateTokens()
{
	size_t i, j;
	ILToken *token;

	IArchitecture *file = meta_->owner();
	if (type_ == ttUserString) {
		for (i = 0; i < count(); i++) {
			ILUserString *user_string = reinterpret_cast<ILUserString*>(item(i));
			bool has_references = false;
			for (j = 0; j < user_string->reference_list()->count(); j++) {
				if (!user_string->reference_list()->item(j)->is_deleted()) {
					has_references = true;
					break;
				}
			}
			if (has_references)
				user_string->set_value(meta_->AddUserString(user_string->name()), file);
		}
	} else {
		uint32_t id = 1;
		for (i = 0; i < count(); i++) {
			token = item(i);
			if (token->is_deleted())
				continue;

			token->set_value(id++, file);
		}
	}
}

/**
 * NETImportFunction
 */

NETImportFunction::NETImportFunction(NETImport *owner, uint32_t token, const std::string &name, size_t name_pos)
	: BaseImportFunction(owner), token_(token), name_(name), name_pos_(name_pos)
{

}

NETImportFunction::NETImportFunction(NETImport *owner, const NETImportFunction &src)
	: BaseImportFunction(owner, src)
{
	token_ = src.token_;
	name_ = src.name_;
	name_pos_ = src.name_pos_;
}

NETImportFunction *NETImportFunction::Clone(IImport *owner) const
{
	NETImportFunction *func = new NETImportFunction(reinterpret_cast<NETImport *>(owner), *this);
	return func;
}

/**
 * NETImport
 */

NETImport::NETImport(NETImportList *owner, uint32_t token, const std::string &name)
	: BaseImport(owner), token_(token), name_(name), is_sdk_(false)
{

}

NETImport::NETImport(NETImportList *owner, const NETImport &src)
	: BaseImport(owner, src)
{
	token_ = src.token_;
	name_ = src.name_;
	is_sdk_ = src.is_sdk_;
}

NETImportFunction *NETImport::item(size_t index) const
{
	return reinterpret_cast<NETImportFunction *>(BaseImport::item(index));
}

NETImport *NETImport::Clone(IImportList *owner) const
{
	NETImport *import = new NETImport(reinterpret_cast<NETImportList *>(owner), *this);
	return import;
}

NETImportFunction *NETImport::Add(uint32_t token, const std::string &name, size_t name_pos)
{
	NETImportFunction *func = new NETImportFunction(this, token, name, name_pos);
	AddObject(func);
	return func;
}

NETImportFunction *NETImport::GetFunctionByToken(uint32_t token) const
{
	for (size_t i = 0; i < count(); i++) {
		NETImportFunction *func = item(i);
		if (func->token() == token)
			return func;
	}

	return NULL;
}

/**
 * NETImportList
 */

NETImportList::NETImportList(NETArchitecture *owner)
	: BaseImportList(owner)
{

}

NETImportList::NETImportList(NETArchitecture *owner, const NETImportList &src)
	: BaseImportList(owner, src)
{

}

NETImportList *NETImportList::Clone(NETArchitecture *owner) const
{
	NETImportList *list = new NETImportList(owner, *this);
	return list;
}

NETImport *NETImportList::GetImportByName(const std::string &name) const
{
	return reinterpret_cast<NETImport *>(BaseImportList::GetImportByName(name));
}

NETImport *NETImportList::item(size_t index) const
{
	return reinterpret_cast<NETImport *>(BaseImportList::item(index));
}

NETImportFunction *NETImportList::GetFunctionByToken(uint32_t token) const
{
	for (size_t i = 0; i < count(); i++) {
		NETImportFunction *func = item(i)->GetFunctionByToken(token);
		if (func)
			return func;
	}

	return NULL;
}

NETImport *NETImportList::Add(uint32_t token, const std::string &name)
{
	NETImport *import = new NETImport(this, token, name);
	AddObject(import);
	return import;
}

void NETImportList::ReadFromFile(NETArchitecture &file)
{
	static const ImportInfo sdk_info[] = {
		{atBegin, "void VMProtect.Begin::.ctor()", ioNone, ctNone},
		{atBegin, "void VMProtect.BeginVirtualization::.ctor()", ioHasCompilationType, ctVirtualization},
		{atBegin, "void VMProtect.BeginMutation::.ctor()", ioHasCompilationType, ctMutation},
		{atBegin, "void VMProtect.BeginUltra::.ctor()", ioHasCompilationType, ctUltra},
		{atBegin, "void VMProtect.BeginVirtualizationLockByKey::.ctor()", ioHasCompilationType | ioLockToKey, ctVirtualization},
		{atBegin, "void VMProtect.BeginUltraLockByKey::.ctor()", ioHasCompilationType | ioLockToKey, ctUltra},
		{atIsProtected, "bool VMProtect.SDK::IsProtected()", ioNone, ctNone},
		{atIsVirtualMachinePresent, "bool VMProtect.SDK::IsVirtualMachinePresent()", ioNone, ctNone},
		{atIsDebuggerPresent, "bool VMProtect.SDK::IsDebuggerPresent(bool)", ioNone, ctNone},
		{atIsValidImageCRC, "bool VMProtect.SDK::IsValidImageCRC()", ioNone, ctNone},
		{atDecryptStringA, "string VMProtect.SDK::DecryptString(string)", ioNone, ctNone},
		{atFreeString, "bool VMProtect.SDK::FreeString(string&)", ioNone, ctNone },
		{atSetSerialNumber, "valuetype VMProtect.SerialState VMProtect.SDK::SetSerialNumber(string)", ioNone, ctNone},
		{atGetSerialNumberState, "valuetype VMProtect.SerialState VMProtect.SDK::GetSerialNumberState()", ioNone, ctNone},
		{atGetSerialNumberData, "bool VMProtect.SDK::GetSerialNumberData(class VMProtect.SerialNumberData&)", ioNone, ctNone},
		{atGetCurrentHWID, "string VMProtect.SDK::GetCurrentHWID()", ioNone, ctNone},
		{atActivateLicense, "valuetype VMProtect.ActivationStatus VMProtect.SDK::ActivateLicense(string, string&)", ioNone, ctNone},
		{atDeactivateLicense, "valuetype VMProtect.ActivationStatus VMProtect.SDK::DeactivateLicense(string)", ioNone, ctNone},
		{atGetOfflineActivationString, "valuetype VMProtect.ActivationStatus VMProtect.SDK::GetOfflineActivationString(string, string&)", ioNone, ctNone},
		{atGetOfflineDeactivationString, "valuetype VMProtect.ActivationStatus VMProtect.SDK::GetOfflineDeactivationString(string, string&)", ioNone, ctNone}
	};

	size_t i;
	std::map<ILToken *, NETImport *> import_map;
	std::map<ILToken *, NETImport *> type_map;
	NETImport *import;

	ILTable *table = file.command_list()->table(ttAssemblyRef);
	for (i = 0; i < table->count(); i++) {
		ILAssemblyRef *assembly_ref = reinterpret_cast<ILAssemblyRef *>(table->item(i));
		std::string module_name = assembly_ref->name();

		import = GetImportByName(module_name);
		if (!import) {
			import = Add(assembly_ref->id(), module_name);
			std::transform(module_name.begin(), module_name.end(), module_name.begin(), tolower);
			if (module_name == "vmprotect.sdk")
				import->set_is_sdk(true);
		}
		import_map[assembly_ref] = import;
	}

	table = file.command_list()->table(ttTypeRef);
	for (i = 0; i < table->count(); i++) {
		ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(table->item(i));
		std::map<ILToken *, NETImport *>::const_iterator it = import_map.find(type_ref->resolution_scope());
		if (it == import_map.end())
			continue;

		import = it->second;
		type_map[type_ref] = import;
		import->Add(type_ref->id(), type_ref->full_name(), 0);
	}


	table = file.command_list()->table(ttTypeSpec);
	for (i = 0; i < table->count(); i++) {
		ILTypeSpec *type_spec = reinterpret_cast<ILTypeSpec *>(table->item(i));
		ILToken *token = NULL;
		
		switch (type_spec->info()->type()) {
		case ELEMENT_TYPE_GENERICINST:
			token = type_spec->info()->next()->token();
			break;
		case ELEMENT_TYPE_VALUETYPE:
		case ELEMENT_TYPE_CLASS:
			token = type_spec->info()->token();
			break;
		}

		if (!token || (token->type() != ttTypeRef))
			continue;

		std::map<ILToken *, NETImport *>::const_iterator it = import_map.find(reinterpret_cast<ILTypeRef *>(token)->resolution_scope());
		if (it == import_map.end())
			continue;

		import = it->second;
		type_map[type_spec] = import;
		import->Add(type_spec->id(), type_spec->name(), 0);
	}

	table = file.command_list()->table(ttMemberRef);
	for (i = 0; i < table->count(); i++) {
		ILMemberRef *member_ref = reinterpret_cast<ILMemberRef*>(table->item(i));
		std::map<ILToken *, NETImport *>::const_iterator it = type_map.find(member_ref->declaring_type());
		if (it == type_map.end())
			continue;

		import = it->second;
		std::string ret_name = member_ref->signature()->ret_name();
		NETImportFunction *func = import->Add(member_ref->id(), member_ref->full_name(), ret_name.empty() ? 0 : ret_name.size() + 1);
		if (import->is_sdk()) {
			for (size_t i = 0; i < _countof(sdk_info); i++) {
				const ImportInfo *import_info = &sdk_info[i];
				if (func->name().compare(import_info->name) == 0) {
					func->set_type(import_info->type);
					if (import_info->options & ioHasCompilationType) {
						func->include_option(ioHasCompilationType);
						func->set_compilation_type(import_info->compilation_type);
						if (import_info->options & ioLockToKey)
							func->include_option(ioLockToKey);
					}
				}
			}
		}
	}

	table = file.command_list()->table(ttModuleRef);
	for (i = 0; i < table->count(); i++) {
		ILModuleRef *module_ref = reinterpret_cast<ILModuleRef *>(table->item(i));

		import = GetImportByName(module_ref->name());
		if (!import)
			import = Add(module_ref->id(), module_ref->name());
		import_map[module_ref] = import;
	}

	table = file.command_list()->table(ttImplMap);
	for (i = 0; i < table->count(); i++) {
		ILImplMap *impl_map = reinterpret_cast<ILImplMap*>(table->item(i));
		std::map<ILToken *, NETImport *>::const_iterator it = import_map.find(impl_map->import_scope());
		if (it == import_map.end())
			continue;

		import = it->second;
		NETImportFunction *func = import->Add(impl_map->id(), impl_map->import_name(), 0);
	}
}

void NETImportList::Pack()
{
	size_t i, j, k;
	std::map<ILToken *, ILToken *> type_map;
	ILMetaData *meta = reinterpret_cast<NETArchitecture*>(owner())->command_list();

	ILTable *type_def_table = meta->table(ttTypeDef);
	ILTable *field_table = meta->table(ttField);
	ILTable *method_table = meta->table(ttMethodDef);
	ILTable *custom_attribute_table = meta->table(ttCustomAttribute);
	for (i = 0; i < count(); i++) {
		NETImport *import = item(i);
		if (!import->is_sdk())
			continue;

		ILToken *token = meta->token(import->token());
		token->set_deleted(true);

		for (j = 0; j < import->count(); j++) {
			NETImportFunction *func = import->item(j);

			token = meta->token(func->token());
			switch (token->type()) {
			case ttTypeRef:
				{
					ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(token);
					for (k = 0; k < type_def_table->count(); k++) {
						ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(type_def_table->item(k));
						if (type_def->full_name() == type_ref->full_name()) {
							type_map[type_ref] = type_def;
							break;
						}
					}
				}
				break;
			case ttMemberRef:
				{
					ILMemberRef *member_ref = reinterpret_cast<ILMemberRef*>(meta->token(func->token()));
					ILToken *declaring_type = member_ref->declaring_type();
					if (declaring_type && declaring_type->type() == ttTypeRef) {
						ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(declaring_type);
						if (func->type() == atNone) {
							std::string name = member_ref->full_name();
							if (member_ref->signature()->is_field()) {
								// field
								for (k = 0; k < field_table->count(); k++) {
									ILField *field = reinterpret_cast<ILField *>(field_table->item(k));
									if (field->full_name() == name) {
										type_map[member_ref] = field;
										break;
									}
								}
							} else {
								// method
								for (k = 0; k < method_table->count(); k++) {
									ILMethodDef *method = reinterpret_cast<ILMethodDef *>(method_table->item(k));
									if (method->full_name() == name) {
										type_map[member_ref] = method;
										break;
									}
								}
							}
						} else {
							type_ref->set_deleted(true);
						}
					}
				}
				break;
			}
		}
	}

	if (!type_map.empty()) {
		for (std::map<ILToken *, ILToken *>::const_iterator it = type_map.begin(); it != type_map.end(); it++) {
			ILToken *src_token = it->first;
			ILToken *dst_token = it->second;
			src_token->set_deleted(true);

			while (src_token->reference_list()->count()) {
				TokenReference *reference = src_token->reference_list()->item(0);
				reference->set_owner(dst_token->reference_list());
			}
			if (dst_token->value() != src_token->value())
				dst_token->set_value(0);
		}

		for (i = 0; i < 64; i++) {
			ILTable *table = meta->table(static_cast<ILTokenType>(i << 24));
			for (size_t j = 0; j < table->count(); j++) {
				table->item(j)->RemapTokens(type_map);
			}
		}
	}
}

/**
 * NETExport
 */

NETExport::NETExport(NETExportList *owner, uint64_t address, const std::string &name, size_t name_pos)
	: BaseExport(owner), address_(address), name_(name), name_pos_(name_pos)
{

}

NETExport::NETExport(NETExportList *owner, const NETExport &src)
	: BaseExport(owner, src)
{
	address_ = src.address_;
	name_ = src.name_;
	name_pos_ = src.name_pos_;
}

NETExport *NETExport::Clone(IExportList *owner) const
{
	NETExport *exp = new NETExport(reinterpret_cast<NETExportList *>(owner), *this);
	return exp;
}

void NETExport::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * NETExportList
 */

NETExportList::NETExportList(NETArchitecture *owner)
	: BaseExportList(owner)
{

}

NETExportList::NETExportList(NETArchitecture *owner, const NETExportList &src)
	: BaseExportList(owner, src)
{

}
	
NETExportList *NETExportList::Clone(NETArchitecture *owner) const
{
	NETExportList *list = new NETExportList(owner, *this);
	return list;
}

IExport *NETExportList::Add(uint64_t address)
{
	return Add(address, "", 0);
}

NETExport *NETExportList::Add(uint64_t address, const std::string &name, size_t name_pos)
{
	NETExport *exp = new NETExport(this, address, name, name_pos);
	AddObject(exp);
	return exp;
}

void NETExportList::ReadFromFile(NETArchitecture &file)
{
	ILTable *table = file.command_list()->table(ttMethodDef);
	for (size_t i = 0; i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
		if (!method->declaring_type() || (method->flags() & mdMemberAccessMask) != mdPublic)
			continue;

		std::string ret_name = method->signature()->ret_name();
		Add(method->address(), method->full_name(), ret_name.empty() ? 0 : ret_name.size() + 1);
	}
}

void NETExportList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	static const APIType export_function_types[] = {
		atSetupImage,
		atDecryptStringA,
		atDecryptStringW,
		atFreeString,
		atSetSerialNumber,
		atGetSerialNumberState,
		atGetSerialNumberData,
		atGetCurrentHWID,
		atActivateLicense,
		atDeactivateLicense,
		atGetOfflineActivationString,
		atGetOfflineDeactivationString,
		atIsValidImageCRC,
		atIsDebuggerPresent,
		atIsVirtualMachinePresent,
		atDecryptBuffer,
		atIsProtected,
		atLoaderData,
		atRandom,
		atCalcCRC,
		atBoxPointer,
		atUnboxPointer
	};

	clear();
	BaseExportList::ReadFromBuffer(buffer, file);

	assert(count() == _countof(export_function_types));
	for (size_t i = 0; i < count(); i++) {
		item(i)->set_type(export_function_types[i]);
	}
}

/**
 * NETRuntimeFunction
 */

NETRuntimeFunction::NETRuntimeFunction(NETRuntimeFunctionList *owner, uint64_t begin, uint64_t end, uint64_t start, ILMethodDef *method)
	: BaseRuntimeFunction(owner), begin_(begin), end_(end), start_(start)
{
	if (method)
		method_list_.push_back(method);
}

NETRuntimeFunction::NETRuntimeFunction(NETRuntimeFunctionList *owner, const NETRuntimeFunction &src)
	: BaseRuntimeFunction(owner)
{
	begin_ = src.begin_;
	end_ = src.end_;
	start_ = src.start_;
	method_list_ = src.method_list_;
}

NETRuntimeFunction *NETRuntimeFunction::Clone(IRuntimeFunctionList *owner) const
{
	NETRuntimeFunction *func = new NETRuntimeFunction(reinterpret_cast<NETRuntimeFunctionList *>(owner), *this);
	return func;
}

void NETRuntimeFunction::Rebase(uint64_t delta_base)
{
	begin_ += delta_base;
	end_ += delta_base;
	start_ += delta_base;
}

void NETRuntimeFunction::set_begin(uint64_t begin)
{
	begin_ = begin;
	for (size_t i = 0; i < method_list_.size(); i++) {
		method_list_[i]->set_address(begin_);
	}
}

/**
 * NETRuntimeFunctionList
 */

NETRuntimeFunctionList::NETRuntimeFunctionList()
	: BaseRuntimeFunctionList()
{

}

NETRuntimeFunctionList::NETRuntimeFunctionList(const NETRuntimeFunctionList &src)
	: BaseRuntimeFunctionList(src)
{

}

NETRuntimeFunctionList *NETRuntimeFunctionList::Clone() const
{
	NETRuntimeFunctionList *list = new NETRuntimeFunctionList(*this);
	return list;
}

NETRuntimeFunction *NETRuntimeFunctionList::item(size_t index) const
{
	return reinterpret_cast<NETRuntimeFunction *>(IRuntimeFunctionList::item(index));
}

NETRuntimeFunction *NETRuntimeFunctionList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<NETRuntimeFunction *>(BaseRuntimeFunctionList::GetFunctionByAddress(address));
}

NETRuntimeFunction * NETRuntimeFunctionList::Add(uint64_t begin, uint64_t end, uint64_t start, ILMethodDef *method)
{
	NETRuntimeFunction *func = new NETRuntimeFunction(this, begin, end, start, method);
	AddObject(func);
	return func;
}

void NETRuntimeFunctionList::ReadFromFile(NETArchitecture &file)
{
	ILTable *table = file.command_list()->table(ttMethodDef);
	for (size_t i = 0; i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
		if (!method->address() || (method->impl_flags() & miCodeTypeMask) != miIL)
			continue;

		uint64_t address = method->address();
		uint64_t start_address = address + method->fat_size();
		uint64_t end_address = start_address + method->code_size();
		NETRuntimeFunction *func = GetFunctionByAddress(address);
		if (func)
			func->AddMethod(method);
		else
			Add(address, end_address, start_address, method);
	}
}

IRuntimeFunction *NETRuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions)
{
	if (source) {
		NETRuntimeFunction *runtime_func = reinterpret_cast<NETRuntimeFunction *>(source);
		runtime_func->set_begin(begin);
		runtime_func->set_end(end);
	}

	return NULL;
}

/**
 * NETResource
 */

NETResource::NETResource(IResource *owner, const std::string &name, uint32_t id, uint64_t address, uint32_t size)
	: BaseResource(owner), name_(name), id_(id), address_(address), size_(size)
{

}

NETResource::NETResource(IResource *owner, const NETResource &src)
	: BaseResource(owner, src)
{
	name_ = src.name_;
	id_ = src.id_;
	address_ = src.address_;
	size_ = src.size_;
}

IResource *NETResource::Clone(IResource *owner) const
{
	NETResource *resource = new NETResource(owner, *this);
	return resource;
}

NETResource *NETResource::item(size_t index) const
{
	return reinterpret_cast<NETResource *>(BaseResource::item(index));
}

NETResource *NETResource::Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size)
{
	NETResource *resource = new NETResource(this, name, id, address, size);
	AddObject(resource);
	return resource;
}

/**
 * NETResourceList
 */

NETResourceList::NETResourceList(NETArchitecture *owner)
	: BaseResourceList(owner)
{

}

NETResourceList::NETResourceList(NETArchitecture *owner, const NETResourceList &src)
	: BaseResourceList(owner, src)
{

}

NETResourceList *NETResourceList::Clone(NETArchitecture *owner) const
{
	NETResourceList *list = new NETResourceList(owner, *this);
	return list;
}

NETResource *NETResourceList::item(size_t index) const
{
	return reinterpret_cast<NETResource *>(BaseResourceList::item(index));
}

NETResource *NETResourceList::Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size)
{
	NETResource *resource = new NETResource(this, name, id, address, size);
	AddObject(resource);
	return resource;
}

void NETResourceList::ReadFromFile(NETArchitecture &file)
{
	ILTable *table = file.command_list()->table(ttManifestResource);
	for (size_t i = 0; i < table->count(); i++) {
		ILManifestResource *resource = reinterpret_cast<ILManifestResource*>(table->item(i));
		ILToken *implementation = resource->implementation();
		if (implementation) {
			NETResource *folder = reinterpret_cast<NETResource*>(GetResourceById(string_format("%.4X", implementation->id())));
			if (!folder) {
				std::string name;
				switch (implementation->type()) {
				case ttFile:
					folder = Add(reinterpret_cast<ILFile*>(implementation)->name(), implementation->id(), 0, 0);
					break;
				case ttAssemblyRef:
					folder = Add(reinterpret_cast<ILAssemblyRef*>(implementation)->name(), implementation->id(), 0, 0);
					break;
				}
				if (!folder)
					continue;
			}
			folder->Add(resource->name(), resource->id(), resource->offset(), 0);
		} else {
			NETResource *folder = Add(resource->name(), resource->id(), resource->value()->address(), resource->value()->size());
			for (size_t j = 0; j < resource->value()->count(); j++) {
				ManifestResourceItem *item = resource->value()->item(j);
				folder->Add(item->name(), static_cast<uint32_t>(j), item->address(), item->size());
			}
		}
	}
}

/**
 * NETArchitecture
 */

NETArchitecture::NETArchitecture(PEFile *owner)
	: BaseArchitecture(owner, owner->arch_pe()->offset(), owner->arch_pe()->size()), function_list_(NULL),
	entry_point_(0), pe_(owner->arch_pe()), virtual_machine_list_(NULL), optimized_section_count_(0), resize_header_(0)
{
	meta_data_ = new ILMetaData(this);
	import_list_ = new NETImportList(this);
	export_list_ = new NETExportList(this);
	resource_list_ = new NETResourceList(this);
	runtime_function_list_ = new NETRuntimeFunctionList();
}

NETArchitecture::NETArchitecture(PEFile *owner, const NETArchitecture &src)
	: BaseArchitecture(owner, src), pe_(owner->arch_pe()), function_list_(NULL), entry_point_(NULL),
	virtual_machine_list_(NULL), optimized_section_count_(0), resize_header_(0)
{
	size_t i, j;

	header_ = src.header_;
	meta_data_ = src.meta_data_->Clone(this);
	import_list_ = src.import_list_->Clone(this);
	export_list_ = src.export_list_->Clone(this);
	resource_list_ = src.resource_list_->Clone(this);
	runtime_function_list_ = src.runtime_function_list_->Clone();

	if (src.entry_point_)
		entry_point_ = reinterpret_cast<ILMethodDef *>(meta_data_->token(src.entry_point_->id()));

	if (src.function_list_)
		function_list_ = src.function_list_->Clone(this);
	if (src.virtual_machine_list_)
		virtual_machine_list_ = src.virtual_machine_list_->Clone();

	for (i = 0; i < runtime_function_list_->count(); i++) {
		NETRuntimeFunction *func = runtime_function_list_->item(i);
		std::vector<ILMethodDef *> method_list = func->method_list();
		for (j = 0; j < method_list.size(); j++) {
			method_list[j] = reinterpret_cast<ILMethodDef *>(meta_data_->token(method_list[j]->id()));
		}
		func->set_method_list(method_list);
	}

	if (function_list_) {
		for (i = 0; i < function_list_->count(); i++) {
			ILFunction *func = reinterpret_cast<ILFunction *>(function_list_->item(i));
			for (j = 0; j < func->count(); j++) {
				ILCommand *command = func->item(j);
				if (command->token_reference())
					command->set_token_reference(meta_data_->token(command->token_reference()->owner()->owner()->id())->reference_list()->GetReferenceByAddress(command->token_reference()->address()));
			}
			for (j = 0; j < func->function_info_list()->count(); j++) {
				FunctionInfo *info = func->function_info_list()->item(j);
				if (info->source())
					info->set_source(runtime_function_list_->GetFunctionByAddress(info->source()->begin()));
			}
		}
	}
}

NETArchitecture::~NETArchitecture()
{
	delete import_list_;
	delete export_list_;
	delete resource_list_;
	delete runtime_function_list_;
	delete meta_data_;
	delete function_list_;
}

NETArchitecture *NETArchitecture::Clone(IFile *file) const
{
	NETArchitecture *arch = new NETArchitecture(dynamic_cast<PEFile *>(file), *this);
	return arch;
}

OpenStatus NETArchitecture::ReadFromFile(uint32_t mode)
{
	PEDirectory *dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	if (!dir || !dir->address() || !AddressSeek(dir->address()))
		return osUnknownFormat;

	Read(&header_, sizeof(header_));
	if (!header_.MetaData.VirtualAddress)
		return osInvalidFormat;

	entry_point_ = NULL;
	meta_data_->ReadFromFile(*this, header_.MetaData.VirtualAddress + image_base());
	if (header_.u.EntryPointToken && (header_.Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT) == 0) {
		ILToken *token = meta_data_->token(header_.u.EntryPointToken);
		if (!token || token->type() != ttMethodDef)
			return osInvalidFormat;
		entry_point_ = reinterpret_cast<ILMethodDef*>(token);
	}

	size_t i, j;

	if (header_.VTableFixups.VirtualAddress) {
		if (!AddressSeek(image_base() + header_.VTableFixups.VirtualAddress))
			return osInvalidFormat;

		for (i = 0; i < header_.VTableFixups.Size / sizeof(IMAGE_COR_VTABLEFIXUP); i++) {
			IMAGE_COR_VTABLEFIXUP fixup;
			Read(&fixup, sizeof(fixup));
			uint64_t pos = Tell();
			uint64_t address = image_base() + fixup.RVA;
			if (!AddressSeek(address))
				return osInvalidFormat;

			for (j = 0; j < fixup.Count; j++) {
				ILToken *token = meta_data_->token(ReadDWord());
				if (!token)
					return osInvalidFormat;

				token->reference_list()->Add(address);

				address += 4;
				if (fixup.Type & COR_VTABLE_64BIT) {
					ReadDWord();
					address += 4;
				}
			}
			Seek(pos);
		}
	}

	import_list_->ReadFromFile(*this);
	resource_list_->ReadFromFile(*this);
	runtime_function_list_->ReadFromFile(*this);

	if ((mode & foHeaderOnly) == 0) {
		ILAssemblyRef *corelib = meta_data_->GetCoreLib();
		bool is_supported_framework = false;
		if (corelib) {
			if (corelib->name() == "mscorlib") {
				// .NET Framework
				if (corelib->major_version() >= 2)
					is_supported_framework = true;
			}
			else if (corelib->name() == "System.Runtime") {
				// .NET Core
				if (corelib->major_version() == 4) {
					if (corelib->minor_version() == 2 && corelib->build_number() >= 1)
						is_supported_framework = true;
					if (corelib->minor_version() > 2)
						is_supported_framework = true;
				}
				else if (corelib->major_version() > 4)
					is_supported_framework = true;
			}
			else if (corelib->name() == "netstandard") {
				// .NET Standard
				if (corelib->major_version() == 2 && corelib->minor_version() >= 1)
					is_supported_framework = true;
				else if (corelib->major_version() > 2)
					is_supported_framework = true;
			}
		}
		if (!is_supported_framework)
			return osUnsupportedSubsystem;

		enum ObfuscationType {
			ftUnknown,
			ftRenaming,
			ftCompilationType,
			ftStrings
		};

		struct ObfuscationInfo {
			bool exclude;
			bool apply_to_members;
			size_t value;
			ObfuscationInfo() : exclude(false), apply_to_members(false), value(0) {}
		};

		std::map<ILToken *, NETImportFunction *> import_map;
		std::map<ILToken *, std::map<ObfuscationType, ObfuscationInfo> > attribute_map;
		ILTable *table = meta_data_->table(ttCustomAttribute);
		for (i = 0; i < table->count(); i++) {
			ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(table->item(i));
			if (!attribute->parent() || !attribute->type())
				continue;

			if (attribute->type()->type() == ttMemberRef) {
				ILMemberRef *member_ref = reinterpret_cast<ILMemberRef *>(attribute->type());
				if (member_ref->name() == ".ctor") {
					if (ILToken *declaring_type = member_ref->declaring_type()) {
						std::string full_name;
						switch (declaring_type->type()) {
						case ttTypeRef:
							full_name = reinterpret_cast<ILTypeRef *>(declaring_type)->full_name();
							break;
						}
						if (full_name == "System.Reflection.ObfuscationAttribute") {
							std::map<ObfuscationType, ObfuscationInfo> feature_map;
							std::map<ILToken *, std::map<ObfuscationType, ObfuscationInfo> >::iterator attribute_it = attribute_map.find(attribute->parent());
							if (attribute_it != attribute_map.end())
								feature_map = attribute_it->second;

							ObfuscationType feature = ftUnknown;
							ObfuscationInfo info;
							info.exclude = true; // default value of ObfuscationAttribute.Exclude
							bool need_delete = true;
							CustomAttributeValue *value = attribute->ParseValue();
							for (j = 0; j < value->named_list()->count(); j++) {
								CustomAttributeNamedArgument *arg = value->named_list()->item(j);
								if (arg->is_field())
									continue;

								if (arg->name() == "Exclude")
									info.exclude = arg->ToBoolean();
								else if (arg->name() == "ApplyToMembers")
									info.apply_to_members = arg->ToBoolean();
								else if (arg->name() == "StripAfterObfuscation")
									need_delete = arg->ToBoolean();
								else if (arg->name() == "Feature") {
									std::string str = arg->ToString();
									if (str == "renaming")
										feature = ftRenaming;
									else if (str == "virtualization") {
										feature = ftCompilationType;
										info.value = ctVirtualization;
									}
									else if (str == "mutation") {
										feature = ftCompilationType;
										info.value = ctMutation;
									}
									else if (str == "ultra") {
										feature = ftCompilationType;
										info.value = ctUltra;
									}
									else if (str == "virtualizationlockbykey") {
										feature = ftCompilationType;
										info.value = ctVirtualization | 0x80;
									}
									else if (str == "ultralockbykey") {
										feature = ftCompilationType;
										info.value = ctUltra | 0x80;
									}
									else if (str == "strings")
										feature = ftStrings;
								}
							}

							if (need_delete)
								attribute->set_deleted(true);

							std::map<ObfuscationType, ObfuscationInfo>::const_iterator feature_it = feature_map.find(feature);
							if (feature_it != feature_map.end()) {
								if (feature_it->second.exclude != info.exclude)
									feature_map.erase(feature_it);
							} else
								feature_map[feature] = info;

							attribute_map[attribute->parent()] = feature_map;
							continue;
						}
					}
				}
			}

			NETImportFunction *import_function = import_list_->GetFunctionByToken(attribute->type()->id());
			if (import_function && import_function->owner()->is_sdk())
				import_map[attribute->parent()] = import_function;
		}

		if (!attribute_map.empty()) {
			std::map<ObfuscationType, ObfuscationInfo> assembly_map;
			if (ILToken *assembly = meta_data_->token(ttAssembly | 1)) {
				std::map<ILToken *, std::map<ObfuscationType, ObfuscationInfo> >::const_iterator attribute_it = attribute_map.find(assembly);
				if (attribute_it != attribute_map.end())
					assembly_map = attribute_it->second;
			}

			const ILTokenType table_types[] = {ttTypeDef, ttMethodDef, ttField, ttProperty};
			for (i = 0; i < _countof(table_types); i++) {
				table = meta_data_->table(table_types[i]);
				for (j = 0; j < table->count(); j++) {
					std::map<ObfuscationType, ObfuscationInfo> feature_map;
					ILToken *token = table->item(j);

					std::map<ILToken *, std::map<ObfuscationType, ObfuscationInfo> >::const_iterator attribute_it = attribute_map.find(token);
					if (attribute_it != attribute_map.end())
						feature_map = attribute_it->second;

					ILTypeDef *declaring_type = NULL;
					switch (token->type()) {
					case ttTypeDef:
						declaring_type = reinterpret_cast<ILTypeDef *>(token)->declaring_type();
						break;
					case ttMethodDef:
						declaring_type = reinterpret_cast<ILMethodDef *>(token)->declaring_type();
						break;
					case ttField:
						declaring_type = reinterpret_cast<ILField *>(token)->declaring_type();
						break;
					case ttProperty:
						declaring_type = reinterpret_cast<ILProperty *>(token)->declaring_type();
						break;
					}

					while (declaring_type) {
						attribute_it = attribute_map.find(declaring_type);
						if (attribute_it != attribute_map.end()) {
							std::map<ObfuscationType, ObfuscationInfo> parent_map = attribute_it->second;
							for (std::map<ObfuscationType, ObfuscationInfo>::const_iterator parent_feature_it = parent_map.begin(); parent_feature_it != parent_map.end(); parent_feature_it++) {
								ObfuscationType feature = parent_feature_it->first;
								ObfuscationInfo info = parent_feature_it->second;
								if (info.apply_to_members && feature_map.find(feature) == feature_map.end())
									feature_map[feature] = info;
							}
						}
						declaring_type = declaring_type->declaring_type();
					}

					for (std::map<ObfuscationType, ObfuscationInfo>::const_iterator parent_feature_it = assembly_map.begin(); parent_feature_it != assembly_map.end(); parent_feature_it++) {
						ObfuscationType feature = parent_feature_it->first;
						ObfuscationInfo info = parent_feature_it->second;
						if (feature_map.find(feature) == feature_map.end())
							feature_map[feature] = info;
					}

					std::map<ObfuscationType, ObfuscationInfo>::const_iterator feature_it = feature_map.find(ftRenaming);
					if (feature_it != feature_map.end() && feature_it->second.exclude)
						token->set_can_rename(false);

					if (token->type() == ttMethodDef)
						attribute_map[token] = feature_map;
				}
			}
		}

		table = meta_data_->table(ttMethodDef);
		for (i = 0; i < table->count(); i++) {
			ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
			if (!method->declaring_type())
				continue;

			MapFunctionList *map_function_list = (method->impl_flags() & miCodeTypeMask) == miNative ? pe_->map_function_list() : this->map_function_list();

			std::string ret_name = method->signature()->ret_name();
			MapFunction *map_function = map_function_list->Add(method->address(), 0, ((method->flags() & mdMemberAccessMask) == mdPublic) ? otExport : otCode, 
				FunctionName(method->full_name(), ret_name.empty() ? 0 : ret_name.size() + 1));

			std::map<ILToken *, std::map<ObfuscationType, ObfuscationInfo> >::const_iterator attribute_it = attribute_map.find(method);
			if (attribute_it != attribute_map.end()) {
				std::map<ObfuscationType, ObfuscationInfo> feature_map = attribute_it->second;
				std::map<ObfuscationType, ObfuscationInfo>::const_iterator feature_it = feature_map.find(ftStrings);
				if (feature_it != feature_map.end()) {
					ObfuscationInfo info = feature_it->second;
					if (!info.exclude)
						map_function->set_strings_protection(true);
				}
			}

			std::map<ILToken *, NETImportFunction *>::const_iterator it = import_map.find(method);
			if (it != import_map.end()) {
				map_function->set_type(otMarker);
				NETImportFunction *import_function = it->second;
				if (import_function->compilation_type() != ctNone) {
					map_function->set_compilation_type(import_function->compilation_type());
					map_function->set_lock_to_key((import_function->options() & ioLockToKey) != 0);
				}
			}
			else if (attribute_it != attribute_map.end()) {
				std::map<ObfuscationType, ObfuscationInfo> feature_map = attribute_it->second;
				std::map<ObfuscationType, ObfuscationInfo>::const_iterator feature_it = feature_map.find(ftCompilationType);
				if (feature_it != feature_map.end()) {
					ObfuscationInfo info = feature_it->second;
					if (!info.exclude) {
						map_function->set_type(otMarker);
						if (info.value != ctNone) {
							map_function->set_compilation_type((CompilationType)(info.value & 0x7f));
							map_function->set_lock_to_key((info.value & 0x80) != 0);
						}
					}
				}
			}
		}
		map_function_list()->ReadFromFile(*this);
		export_list_->ReadFromFile(*this);
	}

	function_list_ = new ILFunctionList(this);
	virtual_machine_list_ = new ILVirtualMachineList();
	if ((mode & foHeaderOnly) == 0) {
		ILFileHelper helper;
		helper.Parse(*this);
	}

	return osSuccess;
}

bool NETArchitecture::Prepare(CompileContext &ctx)
{
	ctx.vm_runtime = ctx.runtime;
	if (!BaseArchitecture::Prepare(ctx))
		return false;

	size_t i;
	PESegment *section;
	std::vector<PESegment *> optimized_section_list;

	// optimize sections
	if (pe_->resource_section())
		optimized_section_list.push_back(pe_->resource_section());
	if (pe_->fixup_section())
		optimized_section_list.push_back(pe_->fixup_section());

	optimized_section_count_ = segment_list()->count();
	for (i = segment_list()->count(); i > 0; i--) {
		section = segment_list()->item(i - 1);

		std::vector<PESegment *>::iterator it = std::find(optimized_section_list.begin(), optimized_section_list.end(), section);
		if (it != optimized_section_list.end()) {
			optimized_section_list.erase(it);
			optimized_section_count_--;
		} else {
			break;
		}
	}

	// calc new header size
	uint32_t new_section_count = static_cast<uint32_t>(segment_list()->count() + 1);
	if (ctx.runtime)
		new_section_count++;

	// calc header resizes
	uint32_t new_header_size = header_offset() + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size() + new_section_count * sizeof(IMAGE_SECTION_HEADER);
	uint32_t aligned_header_size = AlignValue(new_header_size, file_alignment());
	resize_header_ = 0;
	for (size_t i = 0; i < segment_list()->count(); i++) {
		section = segment_list()->item(i);
		if (section->physical_size() == 0 || section->physical_offset() == 0 || aligned_header_size <= section->physical_offset())
			continue;

		uint32_t rva = static_cast<uint32_t>(section->address() - image_base());
		if (aligned_header_size > rva) {
			Notify(mtError, NULL, language[lsCreateSegmentError]);
			return false;
		}
		if (aligned_header_size > section->physical_offset())
			resize_header_ = std::max(resize_header_, aligned_header_size - section->physical_offset());
	}

	if (optimized_section_count_ > 0) {
		section = segment_list()->item(optimized_section_count_ - 1);
		if (ctx.runtime) {
			NETArchitecture *runtime = reinterpret_cast<NETArchitecture *>(ctx.runtime);
			if (runtime->segment_list()->count()) {
				MemoryManager runtime_manager(runtime);
				if (runtime->segment_list()->last() == runtime->pe_->fixup_section())
					delete runtime->pe_->fixup_section();
				if (runtime->segment_list()->last() == runtime->pe_->resource_section())
					delete runtime->pe_->resource_section();
				runtime->Rebase(AlignValue(section->address() + section->size(), segment_alignment()) - runtime->segment_list()->item(0)->address());
				runtime->meta_data_->FreeByManager(runtime_manager);
				runtime_manager.Pack();
				for (i = 0; i < runtime_manager.count(); i++) {
					MemoryRegion *region = runtime_manager.item(i);
					ctx.manager->Add(region->address(), region->size(), region->type());
				}
				section = runtime->segment_list()->last();
			} else {
				runtime->Rebase(UINT32_MAX);
			}
		}
		else if (ctx.vm_runtime) {
			NETArchitecture *runtime = reinterpret_cast<NETArchitecture *>(ctx.vm_runtime);
			runtime->segment_list()->clear();
			runtime->Rebase(UINT32_MAX);
		}

		// add new section
		assert(section);
		ctx.manager->Add(AlignValue(section->address() + section->size(), segment_alignment()), static_cast<uint32_t>(-1),  mtReadable | mtExecutable | mtWritable | mtNotPaged | (runtime_function_list()->count() ? mtSolid : mtNone));
	}

	if (ctx.options.flags & (cpPack | cpStripDebugInfo)) {
		PEDirectory *dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DEBUG);
		if (dir && dir->address() && dir->size()) {
			ctx.manager->Add(dir->address(), dir->size());
			if (AddressSeek(dir->address())) {
				IMAGE_DEBUG_DIRECTORY debug_directory_data;
				size_t c = dir->size() / sizeof(debug_directory_data);
				for (i = 0; i < c; i++) {
					Read(&debug_directory_data, sizeof(debug_directory_data));
					if (debug_directory_data.AddressOfRawData)
						ctx.manager->Add(debug_directory_data.AddressOfRawData + image_base(), debug_directory_data.SizeOfData);
				}
			}
		}
	}
	rename_map_.clear();
	if (ctx.options.flags & cpStripDebugInfo)
		RenameSymbols();

	meta_data_->FreeByManager(*ctx.manager);
	meta_data_->Prepare();

	resolver.Prepare();

	return true;
}

void NETArchitecture::RenameToken(ILToken *token)
{
	bool need_first_char = false;
	if (token->type() == ttTypeDef) {
		ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(token);
		if (type_def->flags() & tdSerializable)
			need_first_char = true;
	}
		
	std::string new_name;
	do {
		uint32_t id = rand32();
		if (need_first_char)
			id |= 0xA0000000;
		new_name = string_format("%.8X", id);
	} while (rename_map_.find(new_name) != rename_map_.end());
	rename_map_.insert(new_name);

	switch (token->type()) {
	case ttParam:
		{
			ILParam *param = reinterpret_cast<ILParam *>(token);
			param->set_name(new_name);
		}
		break;
	case ttTypeDef:
		{
			ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(token);
			type_def->set_name(new_name);
			type_def->set_namespace("");
		}
		break;
	case ttMethodDef:
		{
			ILMethodDef *mehod = reinterpret_cast<ILMethodDef *>(token);
			mehod->set_name(new_name);
		}
		break;
	case ttField:
		{
			ILField *field = reinterpret_cast<ILField *>(token);
			field->set_name(new_name);
		}
		break;
	case ttProperty:
		{
			ILProperty *prop = reinterpret_cast<ILProperty *>(token);
			prop->set_name(new_name);
		}
		break;
	case ttEvent:
		{
			ILEvent *event = reinterpret_cast<ILEvent *>(token);
			event->set_name(new_name);
		}
		break;
	case ttGenericParam:
		{
			ILGenericParam *generic_param = reinterpret_cast<ILGenericParam *>(token);
			generic_param->set_name(new_name);
		}
		break;
	}
}

void NETArchitecture::RenameSymbols()
{
	size_t i, j, k;
	ILTable *table;
	ILToken *token;
	std::vector<ILToken *> token_list;
	NameReferenceList reference_list;
	bool is_dll = (pe_->image_type() == itLibrary);
	if (ILToken *assembly = meta_data_->token(ttAssembly | 1)) {
		table = meta_data_->table(ttCustomAttribute);
		for (i = 0; i < table->count(); i++) {
			ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(table->item(i));
			if (attribute->parent() != assembly || !attribute->type())
				continue;

			if (attribute->type()->type() == ttMemberRef) {
				ILMemberRef *member_ref = reinterpret_cast<ILMemberRef *>(attribute->type());
				if (member_ref->name() == ".ctor") {
					if (ILToken *declaring_type = member_ref->declaring_type()) {
						std::string full_name;
						switch (declaring_type->type()) {
						case ttTypeRef:
							full_name = reinterpret_cast<ILTypeRef *>(declaring_type)->full_name();
							break;
						}
						if (full_name == "System.Reflection.ObfuscateAssemblyAttribute") {
							bool need_delete = true;
							CustomAttributeValue *value = attribute->ParseValue();
							if (value->fixed_list()->count() == 1)
								is_dll = !value->fixed_list()->item(0)->ToBoolean();
							for (j = 0; j < value->named_list()->count(); j++) {
								CustomAttributeNamedArgument *arg = value->named_list()->item(j);
								if (arg->is_field())
									continue;

								if (arg->name() == "StripAfterObfuscation")
									need_delete = arg->ToBoolean();
							}

							if (need_delete)
								attribute->set_deleted(true);

							break;
						}
					}
				}
			}
		}
	}

	ILTokenType source_types[] = { ttTypeDef, ttMethodDef, ttField, ttProperty, ttEvent, ttParam, ttGenericParam };
	for (i = 0; i < _countof(source_types); i++) {
		table = meta_data_->table(source_types[i]);
		for (j = 0; j < table->count(); j++) {
			token = table->item(j);
			if (!token->can_rename())
				continue;

			if (is_dll && token->is_exported()) {
				token->set_can_rename(false);
				continue;
			}

			switch (token->type()) {
			case ttTypeDef:
				{
					ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(token);
					if (type_def->value() == 1 || (type_def->flags() & tdRTSpecialName)) {
						token->set_can_rename(false);
						continue;
					}

					if (type_def->FindBaseType("System.Configuration.SettingsBase")) {
						token->set_can_rename(false);
						continue;
					}
				}
				break;
			case ttMethodDef:
				{
					ILMethodDef *method = reinterpret_cast<ILMethodDef *>(token);
					if (method->flags() & mdRTSpecialName) {
						token->set_can_rename(false);
						continue;
					}

					if (method->flags() & mdVirtual) {
						token->set_can_rename(false);
						continue;
					}

					if ((method->declaring_type()->flags() & tdClassSemanticsMask) == tdClass) {
						// check delegate
						ILToken *base_type = method->declaring_type()->base_type();
						if (base_type && base_type->type() == ttTypeRef) {
							ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(base_type);
							if (type_ref->resolution_scope() == meta_data_->GetCoreLib() && type_ref->name_space() == "System" && type_ref->name() == "MulticastDelegate") {
								token->set_can_rename(false);
								continue;
							}
						}
					}
				}
				break;
			case ttField:
				{
					ILField *field = reinterpret_cast<ILField *>(token);
					if (field->flags() & fdRTSpecialName) {
						token->set_can_rename(false);
						continue;
					}
					if ((field->declaring_type()->flags() & tdSerializable) && (field->flags() & fdNotSerialized) == 0) {
						token->set_can_rename(false);
						continue;
					}
					if (field->flags() & fdLiteral) {
						// check enum
						if ((field->declaring_type()->flags() & tdClassSemanticsMask) == tdClass) {
							ILToken *base_type = field->declaring_type()->base_type();
							if (base_type && base_type->type() == ttTypeRef) {
								ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(base_type);
								if (type_ref->resolution_scope() == meta_data_->GetCoreLib() && type_ref->name_space() == "System" && type_ref->name() == "Enum") {
									token->set_can_rename(false);
									continue;
								}
							}
						}
					}
				}
				break;
			case ttProperty:
				{
					ILProperty *prop = reinterpret_cast<ILProperty *>(token);
					if (prop->flags() & prRTSpecialName) {
						token->set_can_rename(false);
						continue;
					}

					if (prop->declaring_type()->FindImplement("System.ComponentModel.INotifyPropertyChanged")) {
						token->set_can_rename(false);
						continue;
					}
				}
				break;
			case ttEvent:
				{
					ILEvent *event = reinterpret_cast<ILEvent *>(token);
					if (event->flags() & evRTSpecialName) {
						token->set_can_rename(false);
						continue;
					}
				}
				break;
			}

			token_list.push_back(token);
		}
	}

	table = meta_data_->table(ttUserString);
	for (i = 0; i < table->count(); i++) {
		ILUserString *str = reinterpret_cast<ILUserString *>(table->item(i));
		ILTypeDef *type_def = meta_data_->GetTypeDef(str->name());
		if (type_def && type_def->can_rename()) {
			reference_list.AddStringReference(str, type_def);

			uint64_t address;
			if (!meta_data_->GetUserData(str->value(), &address).empty()) {
				if (ILCommand *command = reinterpret_cast<ILCommand *>(function_list_->GetCommandByAddress(address, true)))
					reference_list.AddCommandReference(command, type_def);
			}
		}
	}

	table = meta_data_->table(ttMethodSpec);
	for (i = 0; i < table->count(); i++) {
		ILMethodSpec *method_spec = reinterpret_cast<ILMethodSpec *>(table->item(i));
		if (method_spec->parent() && method_spec->parent()->type() == ttMemberRef) {
			std::string name = reinterpret_cast<ILMemberRef*>(method_spec->parent())->full_name();
			if (name.size() > 54 && name.substr(0, 54) == "!!0 Newtonsoft.Json.JsonConvert::DeserializeObject<0>(") {
				token = method_spec->signature()->item(0)->token();
				if (token && token->type() == ttTypeDef) {
					ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(token);
					ILTable *ref_table = meta_data_->table(ttProperty);
					for (size_t r = 0; r < ref_table->count(); r++) {
						ILProperty *property = reinterpret_cast<ILProperty*>(ref_table->item(r));
						if (property->declaring_type() == type_def)
							property->set_can_rename(false);
					}
				}
			}
		}
	}

	table = meta_data_->table(ttMemberRef);
	for (i = 0; i < table->count(); i++) {
		ILMemberRef *member_ref = reinterpret_cast<ILMemberRef *>(table->item(i));
		ILToken *declaring_type = member_ref->declaring_type();
		if (declaring_type->type() == ttTypeSpec) {
			ILTypeSpec *type_spec = reinterpret_cast<ILTypeSpec *>(declaring_type);
			if (type_spec->info()->type() == ELEMENT_TYPE_GENERICINST) {
				if (type_spec->info()->next()->token()->type() == ttTypeDef) {
					ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(type_spec->info()->next()->token());
					if (member_ref->signature()->is_method()) {
						ILMethodDef *method = type_def->GetMethod(member_ref->name(), member_ref->signature());
						if (method && method->can_rename())
							reference_list.AddMemberReference(member_ref, method);
					}
					else if (member_ref->signature()->is_field()) {
						ILField *field = type_def->GetField(member_ref->name());
						if (field && field->can_rename())
							reference_list.AddMemberReference(member_ref, field);
					}
				}
			}
		}
	}

	table = meta_data_->table(ttCustomAttribute);
	for (i = 0; i < table->count(); i++) {
		ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(table->item(i));
		CustomAttributeValue *value = attribute->ParseValue();

		std::vector<CustomAttributeArgument *> arg_list;
		for (j = 0; j < value->fixed_list()->count(); j++) {
			arg_list.push_back(value->fixed_list()->item(j));
		}
		for (j = 0; j < value->named_list()->count(); j++) {
			arg_list.push_back(value->named_list()->item(j));
		}

		for (j = 0; j < arg_list.size(); j++) {
			CustomAttributeArgument *arg = arg_list[j];
			if (arg->reference() && arg->reference()->meta() == meta_data_) {
				if (arg->reference()->can_rename())
					reference_list.AddCustomAttribute(attribute);
			}

			if (arg->children()) {
				for (size_t k = 0; k < arg->children()->count(); k++) {
					arg_list.push_back(arg->children()->item(k));
				}
			}
		}

		ILToken *type;
		switch (attribute->type()->type()) {
		case ttMethodDef:
			type = reinterpret_cast<ILMethodDef *>(attribute->type())->declaring_type();
			break;
		case ttMemberRef:
			type = reinterpret_cast<ILMemberRef *>(attribute->type())->declaring_type();
			break;
		default:
			type = NULL;
			break;
		}

		for (size_t j = 0; j < value->named_list()->count(); j++) {
			CustomAttributeNamedArgument *arg = value->named_list()->item(j);
			while (type && type->type() == ttTypeDef) {
				ILTypeDef *declaring_type = reinterpret_cast<ILTypeDef *>(type);
				if (arg->is_field()) {
					if (ILField *field = declaring_type->GetField(arg->name())) {
						if (field->can_rename())
							reference_list.AddCustomAttributeNameReference(attribute, arg, field);
						break;
					}
				}
				else {
					if (ILProperty *prop = declaring_type->GetProperty(arg->name())) {
						if (prop->can_rename())
							reference_list.AddCustomAttributeNameReference(attribute, arg, prop);
						break;
					}
				}
				type = declaring_type->base_type();
			}
		}
	}

	std::map<std::string, std::set<ILProperty *> > property_map;
	table = meta_data_->table(ttProperty);
	ILTable *method_semantics_table = meta_data_->table(ttMethodSemantics);
	for (i = 0; i < table->count(); i++) {
		ILProperty *prop = reinterpret_cast<ILProperty *>(table->item(i));

		CorMethodAttr method_flags = (CorMethodAttr)0;
		for (j = 0; j < method_semantics_table->count(); j++) {
			ILMethodSemantics *semantics = reinterpret_cast<ILMethodSemantics *>(method_semantics_table->item(j));
			if (semantics->association() == prop) {
				method_flags = semantics->method()->flags();
				break;
			}
		}

		if ((method_flags & mdMemberAccessMask) == mdPublic && (method_flags & mdStatic) == 0) {
			std::map<std::string, std::set<ILProperty *> >::iterator it = property_map.find(prop->name());
			if (it == property_map.end())
				property_map[prop->name()].insert(prop);
			else
				it->second.insert(prop);
		}
	}

	table = meta_data_->table(ttManifestResource);
	for (i = 0; i < table->count(); i++) {
		ILManifestResource *resource = reinterpret_cast<ILManifestResource *>(table->item(i));
		if (resource->implementation())
			continue;

		std::string name = resource->name();
		if (name.size() >= 10 && name.substr(name.size() - 10) != ".resources")
			continue;

		if (name.size() >= 2 && name.substr(name.size() - 2) == ".g")
			continue;

		name = name.substr(0, name.size() - 10);
		ILTypeDef *type_def = meta_data_->GetTypeDef(name);
		if (type_def && type_def->can_rename())
			reference_list.AddResourceReference(resource, type_def, ".resources");

		ManifestResourceValue *resource_value = resource->value();
		for (j = 0; j < resource_value->count(); j++) {
			ManifestResourceItem *resource_item = resource_value->item(j);
			std::string name = resource_item->name();
			if (name.size() >= 5 && name.substr(name.size() - 5) == ".baml") {
				BamlDocument *baml = resource_item->ParseBaml();
				if (!baml)
					continue;

				for (k = baml->count(); k > 0; k--) {
					BamlRecord *record = baml->item(k - 1);
					if (record->type() == LineNumberAndPosition || record->type() == LinePosition) {
						delete record;
						reference_list.AddManifestResource(resource);
					}
				}

				BamlElement root(NULL);
				if (root.Parse(baml)) {
					std::map<uint16_t, ILMetaData *> assembly_info_map;
					std::map<uint16_t, ILTypeDef *> type_info_map;
					std::map<uint16_t, AttributeInfoRecord *> attribute_info_map;

					for (k = 0; k < baml->count(); k++) {
						BamlRecord *record = baml->item(k);
						switch (record->type()) {
						case AssemblyInfo:
							{
								AssemblyInfoRecord *assembly_info = reinterpret_cast<AssemblyInfoRecord *>(record);
								if (ILMetaData *assembly = meta_data_->ResolveAssembly(assembly_info->assembly_name(), false))
									assembly_info_map[assembly_info->assembly_id()] = assembly;
							}
							break;
						case AttributeInfo:
							{
								AttributeInfoRecord *attribute_info = reinterpret_cast<AttributeInfoRecord *>(record);
								attribute_info_map[attribute_info->attribute_id()] = attribute_info;
							}
							break;
						case TypeInfo:
						case TypeSerializerInfo:
							{
								TypeInfoRecord *type_info = reinterpret_cast<TypeInfoRecord *>(record);
								std::map<uint16_t, ILMetaData *>::const_iterator it = assembly_info_map.find(type_info->assembly_id());
								if (it != assembly_info_map.end()) {
									if (ILTypeDef *type_def = it->second->GetTypeDef(type_info->type_name()))
										type_info_map[type_info->type_id()] = type_def;
								}
							}
							break;
						}
					}

					std::vector<BamlElement *> stack;
					stack.push_back(&root);
					BamlElement *type_root = root.children().empty() ? NULL : root.children().at(0);
					while (!stack.empty()) {
						BamlElement *elem = stack.back();
						stack.pop_back();

						// process header
						switch (elem->header()->type()) {
						case ConstructorParametersStart:
							elem->set_type(elem->parent()->type());
							break;
						case ElementStart:
						case NamedElementStart:
							{
								ElementStartRecord *element_record = reinterpret_cast<ElementStartRecord *>(elem->header());
								std::map<uint16_t, ILTypeDef *>::const_iterator it = type_info_map.find(element_record->type_id());
								if (it != type_info_map.end())
									elem->set_type(it->second);
							}
							break;
						}

						// process body
						std::vector<BamlRecord *> body = elem->body();
						for (k = 0; k < body.size(); k++) {
							BamlRecord *record = body[k];
							switch (record->type()) {
							case Property:
							case PropertyWithConverter:
								{
									if (!type_root || !type_root->type())
										break;

									PropertyRecord *property = reinterpret_cast<PropertyRecord *>(record);
									std::map<uint16_t, AttributeInfoRecord *>::const_iterator attribute_it = attribute_info_map.find(property->attribute_id());
									if (attribute_it != attribute_info_map.end()) {
										AttributeInfoRecord *attribute_info = attribute_it->second;
										std::map<uint16_t, ILTypeDef *>::const_iterator it = type_info_map.find(attribute_info->owner_type_id());
										if (it != type_info_map.end()) {
											ILTypeDef *type_def = it->second;
											while (type_def) {
												if (ILEvent *event = type_def->GetEvent(attribute_info->name())) {
													if (ILMethodDef *method = type_root->type()->GetMethod(property->value(), NULL)) {
														if (method->meta() == meta_data_ && method->can_rename())
															reference_list.AddPropertyRecordReference(resource, property, method);
														break;
													}
												}

												ILToken *base_type = type_def->base_type();
												if (!base_type)
													break;

												switch (base_type->type()) {
												case ttTypeDef:
													type_def = reinterpret_cast<ILTypeDef *>(base_type);
													break;
												case ttTypeRef:
													type_def = reinterpret_cast<ILTypeRef *>(base_type)->Resolve(true);
													break;
												default:
													type_def = NULL;
													break;
												}
											}
										}
									}
								}
								break;
							case Text:
								{
									TextRecord *text = reinterpret_cast<TextRecord *>(record);
									std::map<std::string, std::set<ILProperty *> >::const_iterator it = property_map.find(text->value());
									if (it != property_map.end()) {
										for (std::set<ILProperty *>::const_iterator prop_it = it->second.begin(); prop_it != it->second.end(); prop_it++) {
											(*prop_it)->set_can_rename(false);
										}
									}
								}
								break;
							case TypeInfo:
							case TypeSerializerInfo:
								{
									TypeInfoRecord *type_info = reinterpret_cast<TypeInfoRecord *>(record);
									std::map<uint16_t, ILTypeDef *>::const_iterator it = type_info_map.find(type_info->type_id());
									if (it != type_info_map.end()) {
										ILTypeDef *type_def = it->second;
										if (type_def->meta() == meta_data_ && type_def->can_rename())
											reference_list.AddTypeInfoRecordReference(resource, type_info, type_def);
									}
								}
								break;
							case AttributeInfo:
								{
									AttributeInfoRecord *attribute_info = reinterpret_cast<AttributeInfoRecord *>(record);
									std::map<uint16_t, ILTypeDef *>::const_iterator it = type_info_map.find(attribute_info->owner_type_id());
									if (it != type_info_map.end()) {
										ILTypeDef *type_def = it->second;
										while (type_def) {
											if (ILProperty *prop = type_def->GetProperty(attribute_info->name())) {
												if (prop->meta() == meta_data_ && prop->can_rename())
													reference_list.AddAttributeInfoRecordReference(resource, attribute_info, prop);
												break;
											}

											if (ILEvent *event = type_def->GetEvent(attribute_info->name())) {
												if (event->meta() == meta_data_ && event->can_rename())
													reference_list.AddAttributeInfoRecordReference(resource, attribute_info, event);
												break;
											}

											ILToken *base_type = type_def->base_type();
											if (!base_type)
												break;

											switch (base_type->type()) {
											case ttTypeDef:
												type_def = reinterpret_cast<ILTypeDef *>(base_type);
												break;
											case ttTypeRef:
												type_def = reinterpret_cast<ILTypeRef *>(base_type)->Resolve(true);
												break;
											default:
												type_def = NULL;
												break;
											}
										}
									}
								}
								break;
							}
						}

						// process children
						std::vector<BamlElement *> children = elem->children();
						for (k = 0; k < children.size(); k++) {
							stack.push_back(children[k]);
						}
					}
				}
			}
		}
	}

	for (i = 0; i < token_list.size(); i++) {
		token = token_list[i];
		if (token->can_rename())
			RenameToken(token);
	}

	reference_list.UpdateNames();
}

void NETArchitecture::Save(CompileContext &ctx)
{
	PESegment *vmp_segment, *last_segment, *segment;
	uint64_t address, file_crc_address, loader_crc_address, loader_crc_size_address, loader_crc_hash_address, file_crc_size_address;
	uint32_t size;
	PEDirectory *dir;
	size_t i, c, j, vmp_index;
	uint64_t pos;
	uint32_t fixup_section_flags, resource_section_flags, file_crc_size, loader_crc_size;
	std::string fixup_section_name, resource_section_name;
	MemoryRegion *region;
	std::set<ILTypeDef *> runtime_type_list;

	MemoryManager *manager = memory_manager();
	NETArchitecture *source = (NETArchitecture *)this->source();

	// erase sections area
	{
		IMAGE_SECTION_HEADER section_header = IMAGE_SECTION_HEADER();
		Seek(header_offset() + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size());
		for (i = 0; i < segment_list()->count(); i++) {
			Write(&section_header, sizeof(section_header));
		}
	}

	// resize header
	if (resize_header_) {
		size_t total_header_size = offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size() + segment_list()->count() * sizeof(IMAGE_SECTION_HEADER);
		if (resize_header_) {
			source->Seek(header_offset() + total_header_size);
			Seek(header_offset() + total_header_size);
			for (i = 0; i < resize_header_; i++) {
				WriteByte(0);
			}
			CopyFrom(*source, this->size() - source->Tell());
			for (i = 0; i < segment_list()->count(); i++) {
				segment = segment_list()->item(i);
				if (segment->physical_offset())
					segment->set_physical_offset(segment->physical_offset() + resize_header_);
			}
		}
	}

	// calc progress maximum
	c = 0;
	if (ctx.runtime)
		c += ctx.runtime->segment_list()->count();
	for (i = 0; i < function_list_->count(); i++) {
		IFunction *func = function_list_->item(i);
		if (!func->need_compile())
			continue;

		for (j = 0; j < func->block_list()->count(); j++) {
			CommandBlock *block = func->block_list()->item(j);
			c += block->end_index() - block->start_index() + 1;
		}
	}
	StartProgress(string_format("%s...", language[lsSaving].c_str()), c);

	resource_section_flags = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
	resource_section_name = ".rsrc";

	fixup_section_flags = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA;
	fixup_section_name = ".reloc";

	// need erase optimized sections
	for (i = segment_list()->count(); i > optimized_section_count_; i--) {
		PESegment *section = segment_list()->item(i - 1);
		if (pe_->resource_section() == section) {
			resource_section_flags = section->flags();
			resource_section_name = section->name();
		} else if (pe_->fixup_section() == section) {
			fixup_section_flags = section->flags();
			fixup_section_name = section->name();
		}
		delete section;
	}

	vmp_index = 0;
	last_segment = segment_list()->last();
	address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment());
	pos = Resize(AlignValue(this->size(), file_alignment()));
	vmp_segment = segment_list()->Add(address, -1, static_cast<uint32_t>(pos), -1, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE, "");

	std::vector<IFunction *> ref_function_list = this->function_list()->processor_list();
	NETArchitecture *runtime = reinterpret_cast<NETArchitecture *>(ctx.vm_runtime);
	if (ctx.runtime) {
		// create EntryPoint method
		ILTypeDef *module = reinterpret_cast<ILTypeDef *>(meta_data_->token(ttTypeDef | 1));
		if (!module)
			throw std::runtime_error("Runtime error at Save");

		ILData signature;
		signature.push_back(0x00);
		signature.push_back(0x00);
		signature.push_back(0x01);
		ILMethodDef *module_cctor = meta_data_->AddMethod(module, ".cctor", signature, (CorMethodAttr)(mdPrivate | mdHideBySig | mdStatic | mdSpecialName | mdRTSpecialName), miIL);
		runtime->set_entry_point_method(module_cctor);

		// merge runtime objects
		if (runtime->segment_list()->count()) {
			// merge segments
			for (i = 0; i < runtime->segment_list()->count(); i++) {
				segment = runtime->segment_list()->item(i);
				if (segment->physical_offset() && segment->physical_size()) {
					runtime->Seek(segment->physical_offset());
					size = static_cast<uint32_t>(segment->physical_size());
					uint8_t *buffer = new uint8_t[size];
					runtime->Read(buffer, size);
					Write(buffer, size);
					delete[] buffer;
				}
				size = static_cast<uint32_t>(AlignValue(segment->size(), runtime->segment_alignment()) - segment->physical_size());
				for (j = 0; j < size; j++) {
					WriteByte(0);
				}
				vmp_segment->include_write_type(segment->memory_type());

				StepProgress();
			}
			ref_function_list.clear();
		}
		else {
			for (i = 0; i < runtime->function_list()->count(); i++) {
				IFunction *func = runtime->function_list()->item(i);
				if (func->tag() == ftLoader)
					ref_function_list.push_back(func);
			}
		}
	}
	else {
		if (ref_function_list.empty())
			runtime = NULL;
	}

	if (runtime) {
		if (ref_function_list.empty()) {
			ILToken *token = runtime->meta_data_->token(ttTypeDef | 1);
			if (token)
				token->set_deleted(true);
		} else {
			std::set<ILToken *> filter_token_list;
			for (i = 0; i < ref_function_list.size(); i++) {
				ILFunction *func = reinterpret_cast<ILFunction *>(ref_function_list[i]);
				for (j = 0; j < func->count(); j++) {
					ILCommand *command = func->item(j);
					if (command->token_reference()) {
						ILToken *token = command->token_reference()->owner()->owner();
						ILSignature *signature = NULL;
						switch (token->type()) {
						case ttMethodDef:
							filter_token_list.insert(token);
							filter_token_list.insert(reinterpret_cast<ILMethodDef *>(token)->declaring_type());
							signature = reinterpret_cast<ILMethodDef *>(token)->signature();
							break;
						case ttMethodSpec:
							filter_token_list.insert(token);
							filter_token_list.insert(reinterpret_cast<ILMethodSpec *>(token)->parent());
							signature = reinterpret_cast<ILMethodSpec *>(token)->signature();
							break;
						case ttMemberRef:
							filter_token_list.insert(token);
							filter_token_list.insert(reinterpret_cast<ILMemberRef *>(token)->declaring_type());
							signature = reinterpret_cast<ILMemberRef *>(token)->signature();
							break;
						case ttField:
							filter_token_list.insert(reinterpret_cast<ILField *>(token)->declaring_type());
							signature = reinterpret_cast<ILField *>(token)->signature();
							break;
						case ttStandAloneSig:
							filter_token_list.insert(token);
							signature = reinterpret_cast<ILStandAloneSig *>(token)->signature();
							break;
						default:
							filter_token_list.insert(token);
							break;
						}

						if (signature) {
							std::vector<ILElement *> element_list;
							for (c = 0; c < signature->count(); c++) {
								element_list.push_back(signature->item(c));
							}
							if (signature->ret())
								element_list.push_back(signature->ret());
							for (c = 0; c < element_list.size(); c++) {
								ILElement *element = element_list[c];
								do {
									if (element->token())
										filter_token_list.insert(element->token());
									element = element->next();
								} while (element);
							}
						}
					}
				}
			}

			ILTokenType link_types[] = { ttTypeDef, ttMethodDef, ttUserString, ttStandAloneSig, ttImplMap, ttModuleRef, ttMemberRef, ttTypeSpec, ttTypeRef };
			for (i = 0; i < _countof(link_types); i++) {
				ILTokenType type = link_types[i];
				ILTable *table = runtime->meta_data_->table(type);
				for (j = 0; j < table->count(); j++) {
					ILToken *token = table->item(j);
					if (token->type() == ttImplMap) {
						ILImplMap *impl_map = reinterpret_cast<ILImplMap *>(token);
						if (filter_token_list.find(impl_map->member_forwarded()) != filter_token_list.end())
							filter_token_list.insert(impl_map->import_scope());
					}
					else if (filter_token_list.find(token) != filter_token_list.end()) {
						if (token->type() == ttTypeDef) {
							ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(token);
							if (type_def->base_type())
								filter_token_list.insert(type_def->base_type());
						}
					} else {
						// token not found
						if (token->type() == ttTypeDef) {
							bool has_nested = false;
							for (std::set<ILToken *>::const_iterator it = filter_token_list.begin(); it != filter_token_list.end(); it++) {
								ILToken *tmp = *it;
								if (tmp->type() == ttTypeDef) {
									ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(tmp);
									while (type_def) {
										ILTypeDef *declaring_type = type_def->declaring_type();
										if (declaring_type == token) {
											has_nested = true;
											break;
										}
										type_def = declaring_type;
									}
								}
								if (has_nested)
									break;
							}
							if (has_nested)
								continue;
						} else if (token->type() == ttMethodDef) {
							ILMethodDef *method = reinterpret_cast<ILMethodDef *>(token);
							if (filter_token_list.find(method->declaring_type()) != filter_token_list.end()) {
								if (method->flags() & (mdRTSpecialName | mdVirtual)) {
									if (!method->declaring_type()->is_deleted()) {
										ILSignature *signature = method->signature();
										std::vector<ILElement *> element_list;
										for (c = 0; c < signature->count(); c++) {
											element_list.push_back(signature->item(c));
										}
										if (signature->ret())
											element_list.push_back(signature->ret());
										for (c = 0; c < element_list.size(); c++) {
											ILElement *element = element_list[c];
											do {
												if (element->token())
													filter_token_list.insert(element->token());
												element = element->next();
											} while (element);
										}
									}
									continue;
								}

								if (method->declaring_type()->method_list() == method) {
									ILMethodDef *next_method = method->next();
									method->declaring_type()->set_method_list(next_method->declaring_type() == method->declaring_type() ? next_method : NULL);
								}
							}
						}

						if (token->type() == ttUserString)
							token->reference_list()->clear();
						else 
							token->set_deleted(true);
					}
				}
			}
		}

		// merge meta data
		ILTable *src_table = runtime->meta_data_->us_table();
		ILTable *dst_table = meta_data_->us_table();
		for (i = 0; i < src_table->count(); i++) {
			ILToken *src_token = src_table->item(i);
			ILToken *dst_token = src_token->Clone(meta_data_, dst_table);
			dst_token->reference_list()->clear();
			while (src_token->reference_list()->count()) {
				TokenReference *reference = src_token->reference_list()->item(0);
				reference->set_owner(dst_token->reference_list());
				if (runtime->segment_list()->count() == 0)
					reference->set_address(0);
			}
			dst_table->AddObject(dst_token);
		}

		src_table = runtime->meta_data_->table(ttTypeDef);
		for (i = 0; i < src_table->count(); i++) {
			ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(src_table->item(i));
			if (type_def->full_name() == "VMProtect.DeleteOnCompilation") {
				type_def->set_deleted(true);
				src_table = runtime->meta_data_->table(ttCustomAttribute);
				for (j = 0; j < src_table->count(); j++) {
					ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(src_table->item(j));
					if (attribute->type() == type_def->method_list())
						attribute->parent()->set_deleted(true);
				}
				break;
			}
		}

		src_table = runtime->meta_data_->table(ttCustomAttribute);
		for (i = 0; i < src_table->count(); i++) {
			ILCustomAttribute *attribute = reinterpret_cast<ILCustomAttribute *>(src_table->item(i));
			if (attribute->parent()->is_deleted())
				continue;

			switch (attribute->parent()->type()) {
			case ttModule:
			case ttAssembly:
				attribute->set_deleted(true);
				{
					ILToken *token = attribute->type();
					if (token->type() == ttMemberRef) {
						ILMemberRef *member_ref = reinterpret_cast<ILMemberRef *>(token);
						if (member_ref->declaring_type()->type() == ttTypeRef && reinterpret_cast<ILTypeRef *>(member_ref->declaring_type())->full_name() == "System.Runtime.CompilerServices.SuppressIldasmAttribute") {
							member_ref->set_deleted(false);
							member_ref->declaring_type()->set_deleted(false);
							attribute->set_deleted(false);
						}
					}
				}
				break;
			}
		}

		ILTokenType types[] = { ttModule, ttAssembly, ttAssemblyRef, ttModuleRef, ttTypeRef, ttTypeDef, ttTypeSpec, ttNestedClass, ttClassLayout, ttMemberRef, ttField,
			ttFieldLayout, ttFieldRVA, ttMethodDef, ttMethodSpec, ttImplMap, ttParam, ttGenericParam, ttConstant, ttCustomAttribute, ttPropertyMap, ttProperty,
			ttMethodSemantics, ttStandAloneSig, ttInterfaceImpl};
		std::map<ILToken *, ILToken *> token_map;
		for (i = 0; i < _countof(types); i++) {
			ILTokenType type = types[i];
			src_table = runtime->meta_data_->table(type);
			dst_table = meta_data_->table(type);
			c = dst_table->count();
			for (size_t k = 0; k < src_table->count();) {
				ILToken *src_token = src_table->item(k);

				if (runtime->segment_list()->count() == 0) {
					for (j = 0; j < src_token->reference_list()->count(); j++) {
						src_token->reference_list()->item(j)->set_address(0);
					}
				}

				src_token->RemapTokens(token_map);
				bool is_equal = false;
				if (!src_token->is_deleted()) {
					if (type == ttModule || type == ttAssembly || type == ttAssemblyRef || type == ttModuleRef || type == ttTypeRef || type == ttMemberRef || type == ttCustomAttribute) {
						for (j = 0; j < c; j++) {
							ILToken *dst_token = dst_table->item(j);
							switch (type) {
							case ttModule:
							case ttAssembly:
								is_equal = src_token->id() == dst_token->id();
								break;
							case ttAssemblyRef:
								{
									ILAssemblyRef *src = reinterpret_cast<ILAssemblyRef *>(src_token);
									ILAssemblyRef *dst = reinterpret_cast<ILAssemblyRef *>(dst_token);
									is_equal = (src->name() == dst->name());
								}
								break;
							case ttModuleRef:
								{
									ILModuleRef *src = reinterpret_cast<ILModuleRef *>(src_token);
									ILModuleRef *dst = reinterpret_cast<ILModuleRef *>(dst_token);
									is_equal = (src->name() == dst->name());
								}
								break;
							case ttTypeRef:
								{
									ILTypeRef *src = reinterpret_cast<ILTypeRef *>(src_token);
									ILTypeRef *dst = reinterpret_cast<ILTypeRef *>(dst_token);
									is_equal = (src->resolution_scope() == dst->resolution_scope() && src->name_space() == dst->name_space() && src->name() == dst->name());
								}
								break;
							case ttMemberRef:
								{
									ILMemberRef *src = reinterpret_cast<ILMemberRef *>(src_token);
									ILMemberRef *dst = reinterpret_cast<ILMemberRef *>(dst_token);
									is_equal = (src->declaring_type() == dst->declaring_type() && src->name() == dst->name() && src->signature()->ret_name(true) == dst->signature()->ret_name(true) && src->signature()->name(true) == dst->signature()->name(true));
								}
								break;
							case ttCustomAttribute:
								{
									ILCustomAttribute *src = reinterpret_cast<ILCustomAttribute *>(src_token);
									ILCustomAttribute *dst = reinterpret_cast<ILCustomAttribute *>(dst_token);
									is_equal = (src->parent() == dst->parent() && src->type() == dst->type());
								}
								break;
							}
							if (is_equal) {
								token_map[src_token] = dst_token;
								while (src_token->reference_list()->count()) {
									TokenReference *reference = src_token->reference_list()->item(0);
									reference->set_owner(dst_token->reference_list());
								}
								if (dst_token->value() != src_token->value())
									dst_token->set_value(0);
								k++;
								break;
							}
						}
					}
				}
				if (!is_equal) {
					src_token->set_owner(meta_data_, dst_table);
					if (src_token->type() == ttTypeDef)
						runtime_type_list.insert(reinterpret_cast<ILTypeDef*>(src_token));
				}
			}
		}
	}
	import_list_->Pack();
	meta_data_->UpdateTokens();

	// update tokens
	for (i = 0; i < function_list_->count(); i++) {
		ILFunction *func = reinterpret_cast<ILFunction *>(function_list_->item(i));
		for (j = 0; j < func->count(); j++) {
			ILCommand *command = func->item(j);
			if (!command->block())
				continue;

			if (command->block()->type() & mtExecutable) {
				TokenReference *reference = command->token_reference();
				if (reference) {
					uint32_t id = reference->owner()->owner()->id();
					if (command->operand_value() != id) {
						if (command->type() == icComment) {
							Data data;
							data.PushDWord(id);
							command->set_dump(data.data(), data.size());
						}
						else {
							command->set_operand_value(0, id);
							command->CompileToNative();
						}
					}
				}
			}
			else {
				for (c = 0; c < command->count(); c++) {
					ILVMCommand *vm_command = command->item(c);
					TokenReference *reference = vm_command->token_reference();
					if (reference) {
						uint32_t id = reference->owner()->owner()->id();
						if (vm_command->value() != id) {
							vm_command->set_value(id);
							vm_command->Compile();
						}
					}
				}
			}
		}
	}

	// write functions
	for (i = 0; i < function_list_->count(); i++) {
		function_list_->item(i)->WriteToFile(*this);
	}

	// erase not used memory regions
	if (manager->count() > 1) {
		// need skip last big region
		for (i = 0; i < manager->count() - 1; i++) {
			region = manager->item(i);
			if (!AddressSeek(region->address()))
				continue;

			for (j = 0; j < region->size(); j++) {
				WriteByte((region->type() & mtReadable) ? rand() : 0);
			}
		}
	}

	if (vmp_segment->write_type() == mtNone)
		delete vmp_segment;
	else {
		vmp_segment->set_name(string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
	
		size = static_cast<uint32_t>(this->size() - vmp_segment->physical_offset());
		vmp_segment->set_size(size);
		vmp_segment->set_physical_size(AlignValue(size, file_alignment()));
		vmp_segment->update_type(vmp_segment->write_type());

		Resize(vmp_segment->physical_offset() + vmp_segment->physical_size());
	}

	//if (false)
	if (!runtime_type_list.empty()) {
		// rename runtime symbols
		NameReferenceList reference_list;
		std::vector<ILToken *> rename_token_list;
		for (std::set<ILTypeDef *>::const_iterator it = runtime_type_list.begin(); it != runtime_type_list.end(); it++) {
			rename_token_list.push_back(*it);
		}

		ILTable *table = meta_data_->table(ttMethodDef);
		for (j = 0; j < table->count(); j++) {
			ILMethodDef *method = reinterpret_cast<ILMethodDef*>(table->item(j));
			if (runtime_type_list.find(method->declaring_type()) != runtime_type_list.end()) {
				if (method->flags() & mdRTSpecialName)
					continue;

				if (method->flags() & mdVirtual) {
					if (method->name() == "Invoke" || method->name() == "BeginInvoke" || method->name() == "EndInvoke" || method->name() == "ToString")
						continue;

					ILToken *base_type = method->declaring_type()->base_type();
					ILMethodDef *base_method = NULL;
					while (base_type && base_type->type() == ttTypeDef) {
						ILTypeDef *type_def = reinterpret_cast<ILTypeDef *>(base_type);
						if (ILMethodDef *tmp_method = type_def->GetMethod(method->name(), method->signature()))
							base_method = tmp_method;
						base_type = type_def->base_type();
					}

					if (base_method) {
						reference_list.AddMethodReference(method, base_method);
						continue;
					}
				}

				rename_token_list.push_back(method);
			}
		}

		table = meta_data_->table(ttParam);
		for (j = 0; j < table->count(); j++) {
			ILParam *param = reinterpret_cast<ILParam*>(table->item(j));
			if (param->parent() && runtime_type_list.find(param->parent()->declaring_type()) != runtime_type_list.end())
				rename_token_list.push_back(param);
		}

		table = meta_data_->table(ttField);
		for (j = 0; j < table->count(); j++) {
			ILField *field = reinterpret_cast<ILField*>(table->item(j));
			if (field->flags() & fdRTSpecialName)
				continue;

			if (runtime_type_list.find(field->declaring_type()) != runtime_type_list.end()) {
				if (field->flags() & fdLiteral) {
					std::string type_name = field->declaring_type()->full_name();
					if (type_name == "VMProtect.SerialState" || type_name == "VMProtect.ActivationStatus")
						continue;
				}

				rename_token_list.push_back(field);
			}
		}

		table = meta_data_->table(ttProperty);
		for (j = 0; j < table->count(); j++) {
			ILProperty *prop = reinterpret_cast<ILProperty*>(table->item(j));
			if (prop->flags() & prRTSpecialName)
				continue;

			if (runtime_type_list.find(prop->declaring_type()) != runtime_type_list.end())
				rename_token_list.push_back(prop);
		}

		for (i = 0; i < rename_token_list.size(); i++) {
			RenameToken(rename_token_list[i]);
		}
		reference_list.UpdateNames();
	}

	// write memory CRC table
	if (function_list_->crc_table()) {
		ILCRCTable *il_crc = reinterpret_cast<ILCRCTable *>(function_list_->crc_table());
		CRCTable crc_table(function_list_->crc_cryptor(), il_crc->table_size());

		// add non writable sections
		for (i = 0; i < segment_list()->count(); i++) {
			segment = segment_list()->item(i);
			if ((segment->memory_type() & (mtReadable | mtWritable)) != mtReadable || segment->excluded_from_memory_protection())
				continue;

			size = std::min(static_cast<uint32_t>(segment->size()), segment->physical_size());
			if (size)
				crc_table.Add(segment->address(), size);
		}

		// skip writable runtime's sections
		if (runtime) {
			for (i = 0; i < runtime->segment_list()->count(); i++) {
				segment = runtime->segment_list()->item(i);
				if (segment->memory_type() & mtWritable)
					crc_table.Remove(segment->address(), static_cast<uint32_t>(segment->size()));
			}
		}

		// skip IAT
		size = OperandSizeToValue(pe_->cpu_address_size());
		size_t k = (runtime && runtime->segment_list()->count() > 0) ? 2 : 1;
		for (size_t n = 0; n < k; n++) {
			PEImportList *import_list = (n == 0) ? pe_->import_list() : runtime->pe_->import_list();
			for (i = 0; i < import_list->count(); i++) {
				PEImport *import = import_list->item(i);
				if (import->count() > 0)
					crc_table.Remove(import->item(0)->address(), size * import->count());
			}
		}

		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < pe_->fixup_list()->count(); i++) {
				PEFixup *fixup = pe_->fixup_list()->item(i);
				if (!fixup->is_deleted())
					crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
			}
		}

		// skip memory CRC table
		crc_table.Remove(il_crc->table_entry()->address(), il_crc->table_size());
		crc_table.Remove(il_crc->size_entry()->address(), sizeof(uint32_t));
		crc_table.Remove(il_crc->hash_entry()->address(), sizeof(uint32_t));

		// write to file
		AddressSeek(il_crc->table_entry()->address());
		uint32_t hash;
		size = static_cast<uint32_t>(crc_table.WriteToFile(*this, false, &hash));
		AddressSeek(il_crc->size_entry()->address());
		WriteDWord(size);
		AddressSeek(il_crc->hash_entry()->address());
		WriteDWord(hash);

		il_crc->size_entry()->set_operand_value(0, size);
		il_crc->hash_entry()->set_operand_value(0, hash);
	}
	EndProgress();

	file_crc_address = 0;
	file_crc_size = 0;
	file_crc_size_address = 0;
	loader_crc_address = 0;
	loader_crc_size = 0;
	loader_crc_size_address = 0;
	loader_crc_hash_address = 0;
	if (ctx.runtime) {
		std::vector<IFunction *> processor_list = function_list_->processor_list();

		NETLoader *loader = new NETLoader(NULL, cpu_address_size());

		last_segment = segment_list()->last();
		address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment());

		manager->clear();
		manager->Add(address, -1, mtReadable | mtExecutable | mtWritable | mtNotPaged | (runtime_function_list()->count() ? mtSolid : mtNone));

		if (!loader->Prepare(ctx)) {
			delete loader;
			throw std::runtime_error("Runtime error at Save");
		}
		size_t processor_count = 0;
		for (i = 0; i < processor_list.size(); i++) {
			processor_count += processor_list[i]->count();
		}
		ctx.file->StartProgress(string_format("%s...", language[lsSavingStartupCode].c_str()), loader->count() + processor_count);
		loader->Compile(ctx);

		pos = Resize(AlignValue(this->size(), file_alignment()));
		segment = segment_list()->Add(address, -1, static_cast<uint32_t>(pos), -1, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
		loader->WriteToFile(*this);
		segment->update_type(segment->write_type());
		for (i = 0; i < processor_list.size(); i++) {
			processor_list[i]->WriteToFile(*this);
		}

		// copy directories
		if (ctx.options.flags & cpPack) {
			IArchitecture *source = const_cast<IArchitecture *>(this->source());
			last_segment = segment_list()->last();

			dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
			if (dir && dir->address() && dir->physical_size() && source->AddressSeek(dir->address())) {
				size = dir->physical_size();
				pos = Resize(AlignValue(this->size(), 0x10));
				CopyFrom(*source, size);
				address = last_segment->address() + pos - last_segment->physical_offset();
				for (i = 0; i < size; i++) {
					PEFixup *fixup = reinterpret_cast<PEFixup *>(source->fixup_list()->GetFixupByAddress(dir->address() + i));
					if (fixup) {
						fixup = fixup->Clone(fixup_list());
						fixup->set_address(address + i);
						fixup_list()->AddObject(fixup);
					}
				}
				dir->set_address(address);
			}

			if ((ctx.options.flags & cpStripDebugInfo) == 0) {
				dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DEBUG);
				if (dir && dir->address() && dir->physical_size() && source->AddressSeek(dir->address())) {
					IMAGE_DEBUG_DIRECTORY debug_directory_data;
					std::vector<IMAGE_DEBUG_DIRECTORY> debug_directory_data_list;
					c = dir->size() / sizeof(debug_directory_data);
					for (i = 0; i < c; i++) {
						source->Read(&debug_directory_data, sizeof(debug_directory_data));
						debug_directory_data_list.push_back(debug_directory_data);
					}
					for (i = 0; i < debug_directory_data_list.size(); i++) {
						debug_directory_data = debug_directory_data_list[i];
						if (source->Seek(debug_directory_data.PointerToRawData)) {
							size = debug_directory_data.SizeOfData;
							pos = Resize(AlignValue(this->size(), 0x10));
							CopyFrom(*source, size);
							address = last_segment->address() + pos - last_segment->physical_offset();

							debug_directory_data.PointerToRawData = static_cast<uint32_t>(pos);
							debug_directory_data.AddressOfRawData = static_cast<uint32_t>(address - image_base());
							debug_directory_data_list[i] = debug_directory_data;
						}
					}
					pos = Resize(AlignValue(this->size(), 0x10));
					address = last_segment->address() + pos - last_segment->physical_offset();
					for (i = 0; i < debug_directory_data_list.size(); i++) {
						debug_directory_data = debug_directory_data_list[i];
						Write(&debug_directory_data, sizeof(debug_directory_data));
					}
					dir->set_address(address);
				}
			}
		}

		if (loader->import_entry()) {
			dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IMPORT);
			if (dir) {
				dir->set_address(loader->import_entry()->address());
				dir->set_size(loader->import_size());
			}

			dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IAT);
			if (dir) {
				dir->set_address(loader->iat_entry()->address());
				dir->set_size(loader->iat_size());
			}
		}

		if (loader->tls_entry()) {
			dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_TLS);
			if (dir) {
				if (loader->tls_size()) {
					dir->set_address(loader->tls_entry()->address());
					dir->set_size(loader->tls_size());
				}
				else {
					dir->clear();
				}
			}
		}

		if (loader->pe_entry())
			pe_->set_entry_point(loader->pe_entry()->address());

		if (loader->strong_name_signature_entry())
			header_.StrongNameSignature.VirtualAddress = static_cast<uint32_t>(loader->strong_name_signature_entry()->address() - image_base());

		if (loader->vtable_fixups_entry())
			header_.VTableFixups.VirtualAddress = static_cast<uint32_t>(loader->vtable_fixups_entry()->address() - image_base());

		if (loader->file_crc_entry()) {
			file_crc_address = loader->file_crc_entry()->address();
			file_crc_size = loader->file_crc_size();
			file_crc_size_address = loader->file_crc_size_entry()->address();
		}

		if (loader->loader_crc_entry()) {
			loader_crc_address = loader->loader_crc_entry()->address();
			loader_crc_size = loader->loader_crc_size();
			loader_crc_size_address = loader->loader_crc_size_entry()->address();
			loader_crc_hash_address = loader->loader_crc_hash_entry()->address();
		}

		delete loader;

		ctx.file->EndProgress();
	}

	// write NET header
	vmp_segment = segment_list()->last();
	pos = Resize(AlignValue(this->size(), 0x10));
	address = vmp_segment->address() + pos - vmp_segment->physical_offset();
	vmp_segment->set_physical_size(-1);
	dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	dir->set_address(address);
	Write(&header_, sizeof(header_));

	// write NET resources
	if (header_.Resources.VirtualAddress) {
		if (ctx.options.flags & cpResourceProtection) {
			header_.Resources.VirtualAddress = 0;
			header_.Resources.Size = 0;
		}
		else {
			Resize(AlignValue(this->size(), sizeof(uint32_t)));
			address = AddressTell();
			ILTable *table = meta_data_->table(ttManifestResource);
			for (i = 0; i < table->count(); i++) {
				ILManifestResource *manifest_resource = reinterpret_cast<ILManifestResource *>(table->item(i));;
				if (manifest_resource->implementation())
					continue;

				std::vector<uint8_t> data = manifest_resource->data();
				if (data.empty()) {
					source->AddressSeek(header_.Resources.VirtualAddress + image_base() + manifest_resource->offset());
					size = source->ReadDWord();
					manifest_resource->set_offset(static_cast<uint32_t>(AddressTell() - address));
					WriteDWord(size);
					CopyFrom(*source, size);
				}
				else {
					manifest_resource->set_offset(static_cast<uint32_t>(AddressTell() - address));
					WriteDWord(static_cast<uint32_t>(data.size()));
					Write(data.data(), data.size());
				}
			}
			header_.Resources.VirtualAddress = static_cast<uint32_t>(address - image_base());
		}
	}

	// write metadata
	Resize(AlignValue(this->size(), sizeof(uint32_t)));
	meta_data_->WriteToFile(*this);
	
	size = static_cast<uint32_t>(this->size() - vmp_segment->physical_offset());
	vmp_segment->set_size(size);
	vmp_segment->set_physical_size(AlignValue(size, pe_->file_alignment()));
	Resize(vmp_segment->physical_offset() + vmp_segment->physical_size());

	// write PE resources
	dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if (dir && dir->address() && dir->physical_size() && source->AddressSeek(dir->address())) {
		last_segment = segment_list()->last();
		address = AlignValue(last_segment->address() + last_segment->size(), pe_->segment_alignment());
		pos = Resize(AlignValue(this->size(), pe_->file_alignment()));
		segment = segment_list()->Add(address, dir->size(), static_cast<uint32_t>(pos), AlignValue(dir->size(), pe_->file_alignment()), resource_section_flags, resource_section_name);

		uint32_t delta_rva = static_cast<uint32_t>(address - dir->address());
		CopyFrom(*source, dir->size());

		std::vector<uint64_t> directory_list;
		directory_list.push_back(pos);

		for (i = 0; i < directory_list.size(); i++) {
			Seek(directory_list[i]);
			IMAGE_RESOURCE_DIRECTORY directory;
			Read(&directory, sizeof(directory));
			for (size_t j = 0; j < static_cast<size_t>(directory.NumberOfNamedEntries) + static_cast<size_t>(directory.NumberOfIdEntries); j++) {
				IMAGE_RESOURCE_DIRECTORY_ENTRY entry;
				Read(&entry, sizeof(entry));
				if (entry.u2.OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
					directory_list.push_back(directory_list[0] + (entry.u2.OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY));
				else {
					pos = Tell();
					Seek(directory_list[0] + entry.u2.OffsetToData);
					IMAGE_RESOURCE_DATA_ENTRY item;
					Read(&item, sizeof(item));

					Seek(directory_list[0] + entry.u2.OffsetToData);
					item.OffsetToData += delta_rva;
					Write(&item, sizeof(item));
					Seek(pos);
				}
			}
		}

		Resize(segment->physical_offset() + segment->physical_size());

		dir->set_address(address);
	}

	// write relocations
	dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_BASERELOC);
	if (dir) {
		if (pe_->fixup_list()->Pack() == 0 || (ctx.options.flags & cpStripFixups) != 0) {
			dir->clear();
		} else {
			last_segment = segment_list()->last();
			address = AlignValue(last_segment->address() + last_segment->size(), pe_->segment_alignment());
			pos = Resize(AlignValue(this->size(), pe_->file_alignment()));

			size = static_cast<uint32_t>(pe_->fixup_list()->WriteToFile(*pe_));
		
			segment = segment_list()->Add(address, size, static_cast<uint32_t>(pos), AlignValue(size, file_alignment()), fixup_section_flags, fixup_section_name);
			Resize(segment->physical_offset() + segment->physical_size());

			dir->set_address(address);
			dir->set_size(size);
		}
	}

	// clear directories
	if (ctx.options.flags & cpStripDebugInfo) {
		dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DEBUG);
		if (dir)
			dir->clear();
	}

	if (ctx.options.script)
		ctx.options.script->DoAfterSaveFile();

	// write header
	WriteToFile();

	// write loader CRC table
	if (loader_crc_address) {
		CRCTable crc_table(function_list_->crc_cryptor(), loader_crc_size);

		// add loader sections
		j = segment_list()->IndexOf(segment_list()->GetSectionByAddress(loader_crc_address));
		if (j != NOT_ID) {
			c = (ctx.options.flags & cpLoaderCRC) ? j + 1 : segment_list()->count();
			for (i = j; i < c; i++) {
				segment = segment_list()->item(i);
				if (segment->memory_type() & mtWritable)
					continue;

				size = std::min(static_cast<uint32_t>(segment->size()), segment->physical_size());
				if (size)
					crc_table.Add(segment->address(), size);
			}
		}

		// skip IAT directory
		dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IAT);
		if (dir)
			crc_table.Remove(dir->address(), dir->size());

		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < pe_->fixup_list()->count(); i++) {
				PEFixup *fixup = pe_->fixup_list()->item(i);
				if (!fixup->is_deleted())
					crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
			}
		}
		// skip loader CRC table
		crc_table.Remove(loader_crc_address, loader_crc_size);
		crc_table.Remove(loader_crc_size_address, sizeof(uint32_t));
		crc_table.Remove(loader_crc_hash_address, sizeof(uint32_t));
		// skip file CRC table
		if (file_crc_address)
			crc_table.Remove(file_crc_address, file_crc_size);
		if (file_crc_size_address)
			crc_table.Remove(file_crc_size_address, sizeof(uint32_t));

		// write to file
		AddressSeek(loader_crc_address);
		uint32_t hash;
		size = static_cast<uint32_t>(crc_table.WriteToFile(*this, false, &hash));
		AddressSeek(loader_crc_size_address);
		WriteDWord(size);
		AddressSeek(loader_crc_hash_address);
		WriteDWord(hash);
	}

	// write file CRC table
	if (file_crc_address) {
		CRCTable crc_table(function_list_->crc_cryptor(), file_crc_size - sizeof(uint32_t));

		// add file range
		crc_table.Add(1, static_cast<size_t>(this->size()) - 1);
		// skip IMAGE_OPTIONAL_HEADER.CheckSum
		crc_table.Remove(header_offset() + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) +
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum) : offsetof(IMAGE_OPTIONAL_HEADER64, CheckSum)),
			sizeof(uint32_t));
		// skip position of security directory
		crc_table.Remove(header_offset() + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + 
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory) : offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory)) + 
			IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY), 
			sizeof(IMAGE_DATA_DIRECTORY));
		// skip file CRC table
		if (AddressSeek(file_crc_address))
			crc_table.Remove(Tell(), file_crc_size);
		if (AddressSeek(file_crc_size_address))
			crc_table.Remove(Tell(), sizeof(uint32_t));

		// write to file
		AddressSeek(file_crc_address);
		size = static_cast<uint32_t>(this->size());
		WriteDWord(size);
		size = static_cast<uint32_t>(crc_table.WriteToFile(*this, true));
		AddressSeek(file_crc_size_address);
		WriteDWord(size);
	}

	pe_->WriteCheckSum();
	
	EndProgress();
}

bool NETArchitecture::WriteToFile()
{
	PEDirectory *dir = pe_->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	if (!dir || !AddressSeek(dir->address()))
		return false;

	if ((header_.Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT) == 0)
		header_.u.EntryPointToken = entry_point_ ? entry_point_->id() : 0;
	header_.MetaData.VirtualAddress = static_cast<uint32_t>(meta_data_->address() - image_base());
	header_.MetaData.Size = meta_data_->size();
	Write(&header_, sizeof(header_));
	return pe_->WriteToFile();
}

bool NETArchitecture::WriteResources(const std::string module_name, Data &dest_data)
{
	dest_data.clear();

	if (!header_.Resources.VirtualAddress)
		return false;

	size_t i;
	std::vector<ILManifestResource *> resource_list;

	ILTable *src_table = command_list()->table(ttManifestResource);
	for (i = 0; i < src_table->count(); i++) {
		ILManifestResource *resource = reinterpret_cast<ILManifestResource *>(src_table->item(i));
		if (resource->is_deleted() || resource->implementation())
			continue;

		resource_list.push_back(resource);
	}
	if (resource_list.empty())
		return false;

	Data data;

	// DOS header
	data.PushByte(0x4d); data.PushByte(0x5a); data.PushByte(0x90); data.PushByte(0x00); data.PushByte(0x03); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x04); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0xff); data.PushByte(0xff); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0xb8); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x40); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x80); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);
	data.PushByte(0x0e); data.PushByte(0x1f); data.PushByte(0xba); data.PushByte(0x0e); data.PushByte(0x00); data.PushByte(0xb4); data.PushByte(0x09); data.PushByte(0xcd);
	data.PushByte(0x21); data.PushByte(0xb8); data.PushByte(0x01); data.PushByte(0x4c); data.PushByte(0xcd); data.PushByte(0x21); data.PushByte(0x54); data.PushByte(0x68);
	data.PushByte(0x69); data.PushByte(0x73); data.PushByte(0x20); data.PushByte(0x70); data.PushByte(0x72); data.PushByte(0x6f); data.PushByte(0x67); data.PushByte(0x72);
	data.PushByte(0x61); data.PushByte(0x6d); data.PushByte(0x20); data.PushByte(0x63); data.PushByte(0x61); data.PushByte(0x6e); data.PushByte(0x6e); data.PushByte(0x6f);
	data.PushByte(0x74); data.PushByte(0x20); data.PushByte(0x62); data.PushByte(0x65); data.PushByte(0x20); data.PushByte(0x72); data.PushByte(0x75); data.PushByte(0x6e);
	data.PushByte(0x20); data.PushByte(0x69); data.PushByte(0x6e); data.PushByte(0x20); data.PushByte(0x44); data.PushByte(0x4f); data.PushByte(0x53); data.PushByte(0x20);
	data.PushByte(0x6d); data.PushByte(0x6f); data.PushByte(0x64); data.PushByte(0x65); data.PushByte(0x2e); data.PushByte(0x0d); data.PushByte(0x0d); data.PushByte(0x0a);
	data.PushByte(0x24); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00); data.PushByte(0x00);

	// PE header
	data.PushDWord(IMAGE_NT_SIGNATURE); // Magic
	data.PushWord(IMAGE_FILE_MACHINE_I386); // Machine
	data.PushWord(1); // NumberOfSections
	data.PushDWord(static_cast<uint32_t>(pe()->time_stamp())); // TimeDateStamp
	data.PushDWord(0); // PointerToSymbolTable
	data.PushDWord(0); // NumberOfSymbols
	data.PushWord(sizeof(IMAGE_OPTIONAL_HEADER32)); // SizeOfOptionalHeader
	data.PushWord(IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_32BIT_MACHINE); // Characteristics

	// optional header
	data.PushWord(IMAGE_NT_OPTIONAL_HDR32_MAGIC); // Magic
	data.PushByte(0x30); // MajorLinkerVersion
	data.PushByte(0); // MinorLinkerVersion
	data.PushDWord(0); // SizeOfCode
	data.PushDWord(0); // SizeOfInitializedData
	data.PushDWord(0); // SizeOfUninitializedData
	data.PushDWord(0); // AddressOfEntryPoint
	data.PushDWord(0); // BaseOfCode
	data.PushDWord(0); // BaseOfData
	data.PushDWord(0x10000000); // ImageBase
	data.PushDWord(0x2000); // SectionAlignment
	data.PushDWord(0x200); // FileAlignment
	data.PushWord(4); // MajorOperatingSystemVersion
	data.PushWord(0); // MinorOperatingSystemVersion
	data.PushWord(0); // MajorImageVersion
	data.PushWord(0); // MinorImageVersion
	data.PushWord(4); // MajorSubsystemVersion
	data.PushWord(0); // MinorSubsystemVersion
	data.PushDWord(0); // Win32VersionValue
	data.PushDWord(0); // SizeOfImage
	data.PushDWord(0); // SizeOfHeaders
	data.PushDWord(0); // CheckSum
	data.PushWord(IMAGE_SUBSYSTEM_WINDOWS_GUI); // Subsystem
	data.PushWord(IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE | IMAGE_DLLCHARACTERISTICS_NO_SEH | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE); // DllCharacteristics
	data.PushDWord(0x100000); // SizeOfStackReserve
	data.PushDWord(0x1000); // SizeOfStackCommit
	data.PushDWord(0x100000); // SizeOfHeapReserve
	data.PushDWord(0x1000); // SizeOfHeapCommit
	data.PushDWord(0); // LoaderFlags
	data.PushDWord(0x10); // NumberOfRvaAndSizes

	// directories
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0);
	data.PushDWord(0x2000);
	data.PushDWord(0x48);
	data.PushDWord(0);
	data.PushDWord(0);

	// sections
	std::string str = ".text";
	str.resize(8);
	data.PushBuff(str.c_str(), str.size());
	data.PushDWord(0x1000); // VirtualSize
	data.PushDWord(0x2000); // VirtualAddress
	data.PushDWord(0x1000); // SizeOfRawData
	data.PushDWord(0x200); // PointerToRawData
	data.PushDWord(0); // PointerToRelocations
	data.PushDWord(0); // PointerToLineNumbers
	data.PushWord(0); // NumberOfRelocations
	data.PushWord(0); // NumberOfLineNumbers
	data.PushDWord(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE); // Characteristics

	// ".text"
	data.resize(AlignValue(data.size(), 0x200));
	data.PushDWord(0x48);
	data.PushWord(header_.MajorRuntimeVersion);
	data.PushWord(header_.MinorRuntimeVersion);
	data.PushDWord(0x2048);
	data.PushDWord(0x1000);
	data.PushDWord(1);
	data.PushDWord(0); // EntryPoint
	data.PushDWord(0); data.PushDWord(0); // Resources
	data.PushDWord(0); data.PushDWord(0); // StrongNameSignature
	data.PushDWord(0); data.PushDWord(0); // CodeManagerTable
	data.PushDWord(0); data.PushDWord(0); // VTableFixups
	data.PushDWord(0); data.PushDWord(0); // ExportAddressTableJumps
	data.PushDWord(0); data.PushDWord(0); // ManagedNativeHeader

	size_t start_offset = data.size();
	data.PushDWord(0x424a5342);	// Signature
	data.PushWord(1); // MajorVersion
	data.PushWord(1); // MinorVersion
	data.PushDWord(0); // Reserved
	str = meta_data_->version();
	data.PushDWord(static_cast<uint32_t>(str.size()));
	data.PushBuff(str.c_str(), str.size());
	data.PushWord(0); // Flags
	data.PushWord(5); // StreamCount

	size_t heap_offset = data.size();
	data.PushDWord(0);
	data.PushDWord(0);
	str = "#~";
	str.resize(AlignValue(str.size() + 1, sizeof(uint32_t)));
	data.PushBuff(str.c_str(), str.size());

	data.PushDWord(0);
	data.PushDWord(0);
	str = "#Strings";
	str.resize(AlignValue(str.size() + 1, sizeof(uint32_t)));
	data.PushBuff(str.c_str(), str.size());

	data.PushDWord(0);
	data.PushDWord(0);
	str = "#US";
	str.resize(AlignValue(str.size() + 1, sizeof(uint32_t)));
	data.PushBuff(str.c_str(), str.size());

	data.PushDWord(0);
	data.PushDWord(0);
	str = "#GUID";
	str.resize(AlignValue(str.size() + 1, sizeof(uint32_t)));
	data.PushBuff(str.c_str(), str.size());

	data.PushDWord(0);
	data.PushDWord(0);
	str = "#Blob";
	str.resize(AlignValue(str.size() + 1, sizeof(uint32_t)));
	data.PushBuff(str.c_str(), str.size());

	data.WriteDWord(heap_offset, static_cast<uint32_t>(data.size() - start_offset));
	data.PushDWord(0);
	data.PushByte(2);
	data.PushByte(0);
	data.PushByte(0);
	data.PushByte(1);
	data.PushQWord(0);
	data.PushQWord(0x000016003301FA00);

	PEFile pe_file;
	pe_file.OpenResource(data.data(), data.size(), false);
	NETArchitecture *dst = reinterpret_cast<NETArchitecture *>(pe_file.item(1));
	dst->set_append_mode(true);

	ILTable *dst_table = dst->command_list()->table(ttModule);
	ILModule *module_def = new ILModule(dst->command_list(), dst_table, module_name + ".dll");
	dst_table->AddObject(module_def);

	dst_table = dst->command_list()->table(ttAssembly);
	ILAssembly *assembly_def = new ILAssembly(dst->command_list(), dst_table, module_name);
	dst_table->AddObject(assembly_def);

	dst_table = dst->command_list()->table(ttTypeDef);
	ILTypeDef *type_def = new ILTypeDef(dst->command_list(), dst_table, NULL, "", "<Module>");
	dst_table->AddObject(type_def);

	dst_table = command_list()->table(ttAssemblyRef);
	ILAssemblyRef *assembly_ref = new ILAssemblyRef(command_list(), dst_table, module_name);
	dst_table->AddObject(assembly_ref);

	// ".text"
	PESegment *segment = dst->segment_list()->item(0);
	segment->set_size(UINT32_MAX);
	segment->set_physical_size(UINT32_MAX);
	dst->Resize(segment->physical_offset());

	uint64_t address = dst->AddressTell();
	PEDirectory *iat = dst->pe()->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IAT);
	iat->set_address(address);
	iat->set_size(8);
	dst->WriteDWord(0);
	dst->WriteDWord(0);

	address = dst->AddressTell();
	PEDirectory *dir = dst->pe()->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	dir->set_address(address);

	IMAGE_COR20_HEADER header = {};
	dst->Write(&header, sizeof(header));

	uint32_t size;
	uint64_t pos = dst->Resize(AlignValue(dst->size(), sizeof(uint32_t)));
	address = dst->AddressTell();
	dst_table = dst->command_list()->table(ttManifestResource);
	for (i = 0; i < resource_list.size(); i++) {
		uint32_t offset = static_cast<uint32_t>(dst->Resize(AlignValue(dst->size(), sizeof(uint32_t))) - pos);
		ILManifestResource *resource = resource_list[i];
		size_t resource_offset = resource->offset();
		ILManifestResource *dest_resource = reinterpret_cast<ILManifestResource *>(resource->Clone(dst->command_list(), dst_table));
		resource->set_offset(0);
		resource->set_implementation(assembly_ref);

		dest_resource->set_flags(mrPublic);
		dest_resource->set_offset(offset);
		dst_table->AddObject(dest_resource);

		std::vector<uint8_t> data = resource->data();
		if (data.empty()) {
			AddressSeek(image_base() + header_.Resources.VirtualAddress + resource_offset);
			size = ReadDWord();
			dst->WriteDWord(size);
			dst->CopyFrom(*this, size);
		}
		else {
			dst->WriteDWord(static_cast<uint32_t>(data.size()));
			dst->Write(data.data(), data.size());
		}
	}
	dst->header_.Resources.VirtualAddress = static_cast<uint32_t>(address - dst->image_base());
	dst->header_.Resources.Size = static_cast<uint32_t>(dst->size() - pos);

	dst->meta_data_->Prepare();
	dst->meta_data_->UpdateTokens();
	dst->Resize(AlignValue(dst->size(), sizeof(uint32_t)));
	dst->meta_data_->WriteToFile(*dst);

	dst->Resize(AlignValue(dst->size(), sizeof(uint32_t)));
	address = dst->AddressTell();
	dir = dst->pe()->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IMPORT);
	dir->set_address(address);
	dir->set_size(0x4f);
	dst->WriteDWord(static_cast<uint32_t>(address - dst->image_base() + 40));
	dst->WriteDWord(0);
	dst->WriteDWord(0);
	dst->WriteDWord(static_cast<uint32_t>(address - dst->image_base() + 66));
	dst->WriteDWord(static_cast<uint32_t>(iat->address() - dst->image_base()));

	dst->WriteDWord(0);
	dst->WriteDWord(0);
	dst->WriteDWord(0);
	dst->WriteDWord(0);
	dst->WriteDWord(0);

	dst->WriteDWord(static_cast<uint32_t>(address - dst->image_base() + 52));
	dst->WriteDWord(0);
	dst->WriteDWord(0);
	address = dst->AddressTell();
	dst->WriteWord(0);
	str = "_CorDllMain";
	dst->Write(str.c_str(), str.size() + 1);
	str = "mscoree.dll";
	dst->Write(str.c_str(), str.size() + 1);
	dst->WriteByte(0);
	dst->AddressSeek(iat->address());
	dst->WriteDWord(static_cast<uint32_t>(address - dst->image_base()));
	dst->Seek(dst->size());

	address = dst->AddressTell();
	dst->pe()->set_entry_point(address);
	dst->WriteByte(0xff);
	dst->WriteByte(0x25);
	dst->WriteDWord(static_cast<uint32_t>(iat->address()));

	size = static_cast<uint32_t>(dst->size() - segment->physical_offset());
	segment->set_size(size);
	segment->set_physical_size(AlignValue(size, dst->file_alignment()));
	dst->Resize(segment->physical_offset() + segment->physical_size());

	// ".reloc"
	address = AlignValue(segment->address() + segment->size(), dst->segment_alignment());
	segment = dst->segment_list()->Add(address, UINT32_MAX, segment->physical_offset() + segment->physical_size(), UINT32_MAX, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA, ".reloc");
	uint32_t reloc_rva = static_cast<uint32_t>(dst->pe()->entry_point() - dst->image_base()) + 2;
	uint32_t page_rva = reloc_rva & ~0xfff;
	dst->WriteDWord(page_rva);
	dst->WriteDWord(0x0c);
	dst->WriteDWord((IMAGE_REL_BASED_HIGHLOW << 12) + reloc_rva - page_rva);
	size = static_cast<uint32_t>(dst->size() - segment->physical_offset());
	segment->set_size(size);
	segment->set_physical_size(AlignValue(size, dst->file_alignment()));
	dir = dst->pe()->command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_BASERELOC);
	dir->set_address(segment->address());
	dir->set_size(size);
	dst->Resize(segment->physical_offset() + segment->physical_size());

	dst->Seek(dst->pe()->header_offset() + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfCode));
	dst->WriteDWord(static_cast<uint32_t>(dst->segment_list()->item(0)->physical_size()));
	dst->WriteDWord(static_cast<uint32_t>(dst->segment_list()->item(1)->physical_size()));
	dst->Seek(dst->pe()->header_offset() + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + offsetof(IMAGE_OPTIONAL_HEADER32, BaseOfCode));
	dst->WriteDWord(static_cast<uint32_t>(dst->segment_list()->item(0)->address() - dst->image_base()));
	dst->WriteDWord(static_cast<uint32_t>(dst->segment_list()->item(1)->address() - dst->image_base()));

	dst->WriteToFile();

	dst->set_append_mode(false);

	dst->Seek(0);
	dest_data.resize(static_cast<size_t>(pe_file.stream()->Size()));
	dst->Read(&dest_data[0], dest_data.size());

	return true;
}

void NETArchitecture::Rebase(uint64_t delta_base)
{
	BaseArchitecture::Rebase(delta_base);

	meta_data_->Rebase(delta_base);
	segment_list()->Rebase(delta_base);
	runtime_function_list_->Rebase(delta_base);
	export_list_->Rebase(delta_base);
	function_list_->Rebase(delta_base);

	if (entry_point_)
		entry_point_ += delta_base;
	//image_base_ += delta_base;
}

bool NETArchitecture::is_executable() const
{
	return pe_->is_executable();
}

std::string NETArchitecture::full_name() const
{
	if (ILAssembly *assembly = reinterpret_cast<ILAssembly *>(meta_data_->token(ttAssembly | 1)))
		return assembly->full_name();
	return std::string();
}

/**
 * ILUserStringsTable
 */

ILUserStringsTable::ILUserStringsTable(ILMetaData *meta) 
	: ILTable(meta, ttUserString, 0)
{

}

ILUserStringsTable::ILUserStringsTable(ILMetaData *meta, const ILUserStringsTable &src) 
	: ILTable(meta, src)
{
	for (size_t i = 0; i < count(); i++) {
		ILToken *token = item(i);
		map_[token->value()] = token;
	}
}

ILUserStringsTable *ILUserStringsTable::Clone(ILMetaData *meta) const
{
	ILUserStringsTable *table = new ILUserStringsTable(meta, *this);
	return table;
}

ILToken *ILUserStringsTable::GetTokenByPos(uint32_t pos) const
{
	auto it = map_.find(pos);
	if (it != map_.end())
		return it->second;

	return NULL;
}

void ILUserStringsTable::AddObject(ILToken *token)
{
	ILTable::AddObject(token);
	map_[token->value()] = token;
}

/**
* BaseNameReference
*/

BaseNameReference::BaseNameReference(NameReferenceList *owner)
	: INameReference(), owner_(owner)
{

}

BaseNameReference::~BaseNameReference()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
* ResourceNameReference
*/

ResourceNameReference::ResourceNameReference(NameReferenceList *owner, ILManifestResource *resource, ILTypeDef *type, const std::string &add)
	: BaseNameReference(owner), resource_(resource), type_(type), add_(add)
{

}

void ResourceNameReference::UpdateName()
{
	resource_->set_name(type_->full_name() + add_);
}

/**
* StringNameReference
*/

StringNameReference::StringNameReference(NameReferenceList *owner, ILUserString *string, ILTypeDef *type)
	: BaseNameReference(owner), string_(string), type_(type)
{

}

void StringNameReference::UpdateName()
{
	string_->set_name(type_->full_name());
}

/**
* CommandNameReference
*/

CommandNameReference::CommandNameReference(NameReferenceList *owner, ILCommand *command, ILTypeDef *type)
	: BaseNameReference(owner), command_(command), type_(type)
{

}

void CommandNameReference::UpdateName()
{
	os::unicode_string unicode_name = os::FromUTF8(type_->full_name());
	command_->set_dump(unicode_name.data(), unicode_name.size() * sizeof(os::unicode_char));
}

/**
* CustomAttributeNameReference
*/

CustomAttributeNameReference::CustomAttributeNameReference(NameReferenceList *owner, CustomAttributeNamedArgument *arg, ILToken *token)
	: BaseNameReference(owner), arg_(arg), token_(token)
{

}

void CustomAttributeNameReference::UpdateName()
{
	std::string name;
	switch (token_->type()) {
	case ttField:
		name = reinterpret_cast<ILField *>(token_)->name();
		break;
	case ttProperty:
		name = reinterpret_cast<ILProperty *>(token_)->name();
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}
	arg_->set_name(name);
}

/**
* MemberReference
*/

MemberReference::MemberReference(NameReferenceList *owner, ILMemberRef *member, ILToken *token)
	: BaseNameReference(owner), member_(member), token_(token)
{

}

void MemberReference::UpdateName()
{
	std::string name;
	switch (token_->type()) {
	case ttField:
		name = reinterpret_cast<ILField *>(token_)->name();
		break;
	case ttMethodDef:
		name = reinterpret_cast<ILMethodDef *>(token_)->name();
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}

	member_->set_name(name);
}

/**
* MethodReference
*/

MethodReference::MethodReference(NameReferenceList *owner, ILMethodDef *method, ILMethodDef *source)
	: BaseNameReference(owner), method_(method), source_(source)
{

}

void MethodReference::UpdateName()
{
	method_->set_name(source_->name());
}

/**
* BamlTypeInfoRecordReference
*/

BamlTypeInfoRecordReference::BamlTypeInfoRecordReference(NameReferenceList *owner, TypeInfoRecord *record, ILToken *token)
	: BaseNameReference(owner), record_(record), token_(token)
{

}

void BamlTypeInfoRecordReference::UpdateName()
{
	std::string name;
	switch (token_->type()) {
	case ttTypeDef:
		name = reinterpret_cast<ILTypeDef *>(token_)->name();
		break;
	case ttTypeRef:
		name = reinterpret_cast<ILTypeRef *>(token_)->name();
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}

	record_->set_type_name(name);
}

/**
* BamlAttributeInfoRecordReference
*/

BamlAttributeInfoRecordReference::BamlAttributeInfoRecordReference(NameReferenceList *owner, AttributeInfoRecord *record, ILToken *token)
	: BaseNameReference(owner), record_(record), token_(token)
{

}

void BamlAttributeInfoRecordReference::UpdateName()
{
	std::string name;
	switch (token_->type()) {
	case ttProperty:
		name = reinterpret_cast<ILProperty *>(token_)->name();
		break;
	case ttEvent:
		name = reinterpret_cast<ILEvent *>(token_)->name();
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}

	record_->set_name(name);
}

/**
* BamlPropertyRecordReference
*/

BamlPropertyRecordReference::BamlPropertyRecordReference(NameReferenceList *owner, PropertyRecord *record, ILToken *token)
	: BaseNameReference(owner), record_(record), token_(token)
{

}

void BamlPropertyRecordReference::UpdateName()
{
	std::string name;
	switch (token_->type()) {
	case ttMethodDef:
		name = reinterpret_cast<ILMethodDef *>(token_)->name();
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}

	record_->set_value(name);
}

/**
* NameReferenceList
*/

NameReferenceList::NameReferenceList()
	: ObjectList<INameReference>()
{

}

void NameReferenceList::AddResourceReference(ILManifestResource *resource, ILTypeDef *type, const std::string &add)
{
	AddObject(new ResourceNameReference(this, resource, type, add));
}

void NameReferenceList::AddStringReference(ILUserString *string, ILTypeDef *type)
{
	AddObject(new StringNameReference(this, string, type));
}

void NameReferenceList::AddCommandReference(ILCommand *command, ILTypeDef *type)
{
	AddObject(new CommandNameReference(this, command, type));
}

void NameReferenceList::AddCustomAttributeNameReference(ILCustomAttribute *attribute, CustomAttributeNamedArgument *arg, ILToken *token)
{
	AddObject(new CustomAttributeNameReference(this, arg, token));
	if (attribute)
		custom_attribute_list_.insert(attribute);
}

void NameReferenceList::AddCustomAttribute(ILCustomAttribute *attribute)
{
	if (attribute)
		custom_attribute_list_.insert(attribute);
}

void NameReferenceList::AddMemberReference(ILMemberRef *member, ILToken *token)
{
	AddObject(new MemberReference(this, member, token));
}

void NameReferenceList::UpdateNames()
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->UpdateName();
	}

	for (std::set<ILCustomAttribute *>::const_iterator it = custom_attribute_list_.begin(); it != custom_attribute_list_.end(); it++) {
		(*it)->UpdateValue();
	}

	for (std::set<ILManifestResource *>::const_iterator it = manifest_resource_list_.begin(); it != manifest_resource_list_.end(); it++) {
		(*it)->UpdateValue();
	}
}

void NameReferenceList::AddTypeInfoRecordReference(ILManifestResource *resource, TypeInfoRecord *record, ILToken *token)
{
	AddObject(new BamlTypeInfoRecordReference(this, record, token));
	if (resource)
		manifest_resource_list_.insert(resource);
}

void NameReferenceList::AddAttributeInfoRecordReference(ILManifestResource *resource, AttributeInfoRecord *record, ILToken *token)
{
	AddObject(new BamlAttributeInfoRecordReference(this, record, token));
	if (resource)
		manifest_resource_list_.insert(resource);
}

void NameReferenceList::AddPropertyRecordReference(ILManifestResource *resource, PropertyRecord *record, ILToken *token)
{
	AddObject(new BamlPropertyRecordReference(this, record, token));
	if (resource)
		manifest_resource_list_.insert(resource);
}

void NameReferenceList::AddMethodReference(ILMethodDef *method, ILMethodDef *source)
{
	AddObject(new MethodReference(this, method, source));
}

void NameReferenceList::AddManifestResource(ILManifestResource *resource)
{
	if (resource)
		manifest_resource_list_.insert(resource);
}

/**
* BamlRecord
*/

BamlRecord::BamlRecord(BamlDocument *owner)
	: IObject(), owner_(owner), position_(0)
{
	
}

BamlRecord::~BamlRecord()
{
	if (owner_)
		owner_->RemoveObject(this);
}

uint64_t BamlRecord::GetPosition(size_t index)
{
	while (index < owner()->count()) {
		bool keys = true;
		switch (owner()->item(index)->type()) {
		case DefAttributeKeyString:
		case DefAttributeKeyType:
		case OptimizedStaticResource:
			break;
		case StaticResourceStart:
			index = owner()->GetClosedIndex(StaticResourceStart, StaticResourceEnd, index + 1);
			if (index == NOT_ID)
				throw std::runtime_error("Invalid record index");
			break;
		case KeyElementStart:
			index = owner()->GetClosedIndex(KeyElementStart, KeyElementEnd, index + 1);
			if (index == NOT_ID)
				throw std::runtime_error("Invalid record index");
			break;
		default:
			keys = false;
			break;
		}
		if (!keys)
			break;

		index++;
	}
	return owner()->item(index)->position();
}

/**
* SizedBamlRecord
*/

SizedBamlRecord::SizedBamlRecord(BamlDocument *owner)
	: BamlRecord(owner), data_size_(0)
{

}

void SizedBamlRecord::Read(NETStream &stream)
{
	uint64_t pos = stream.Tell();
	uint32_t size = stream.ReadEncoded7Bit();
	data_size_ = size - static_cast<uint32_t>(stream.Tell() - pos);
	ReadData(stream);
}

uint32_t SizeOfEncodedInt(uint32_t value)
{
	if ((value & ~0x7F) == 0)
		return 1;
	if ((value & ~0x3FFF) == 0)
 		return 2;
	if ((value & ~0x1FFFFF) == 0)
		return 3;
	if ((value & ~0xFFFFFFF) == 0)
		return 4;
	return 5;
}

void SizedBamlRecord::Write(NETStream &stream)
{
	uint64_t pos = stream.Tell();
	WriteData(stream);
	uint32_t size = static_cast<uint32_t>(stream.Tell() - pos);
	size = SizeOfEncodedInt(SizeOfEncodedInt(size) + size) + size;
	stream.Seek(pos, soBeginning);
	stream.WriteEncoded7Bit(size);
	WriteData(stream);
}

/**
* DocumentStartRecord
*/

DocumentStartRecord::DocumentStartRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

void DocumentStartRecord::Read(NETStream &stream)
{
	load_async_ = stream.ReadBoolean();
	max_async_records_ = stream.ReadDWord();
	debug_baml_ = stream.ReadBoolean();
}

void DocumentStartRecord::Write(NETStream &stream)
{
	stream.WriteBoolean(load_async_);
	stream.WriteDWord(max_async_records_);
	stream.WriteBoolean(debug_baml_);
}

/**
* DocumentEndRecord
*/

DocumentEndRecord::DocumentEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* ElementStartRecord
*/

ElementStartRecord::ElementStartRecord(BamlDocument *owner)
	: BamlRecord(owner), type_id_(0), flags_(0)
{

}

void ElementStartRecord::Read(NETStream &stream)
{
	type_id_ = stream.ReadWord();
	flags_ = stream.ReadByte();
}

void ElementStartRecord::Write(NETStream &stream)
{
	stream.WriteWord(type_id_);
	stream.WriteByte(flags_);
}

/**
* ElementEndRecord
*/

ElementEndRecord::ElementEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* AssemblyInfoRecord
*/

AssemblyInfoRecord::AssemblyInfoRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), assembly_id_(0)
{

}

void AssemblyInfoRecord::ReadData(NETStream &stream)
{
	assembly_id_ = stream.ReadWord();
	assembly_name_ = stream.ReadString();
}

void AssemblyInfoRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(assembly_id_);
	stream.WriteString(assembly_name_);
}

/**
* TypeInfoRecord
*/

TypeInfoRecord::TypeInfoRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), type_id_(0), assembly_id_(0)
{

}

void TypeInfoRecord::ReadData(NETStream &stream)
{
	type_id_ = stream.ReadWord();
	assembly_id_ = stream.ReadWord();
	type_name_ = stream.ReadString();
}

void TypeInfoRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(type_id_);
	stream.WriteWord(assembly_id_);
	stream.WriteString(type_name_);
}

/**
* XmlnsPropertyRecord
*/

XmlnsPropertyRecord::XmlnsPropertyRecord(BamlDocument *owner)
	: SizedBamlRecord(owner)
{

}

void XmlnsPropertyRecord::ReadData(NETStream &stream)
{
	prefix_ = stream.ReadString();
	xml_namespace_ = stream.ReadString();
	assembly_ids_.resize(stream.ReadWord());
	for (size_t i = 0; i < assembly_ids_.size(); i++) {
		assembly_ids_[i] = stream.ReadWord();
	}
}

void XmlnsPropertyRecord::WriteData(NETStream &stream)
{
	stream.WriteString(prefix_);
	stream.WriteString(xml_namespace_);
	stream.WriteWord(static_cast<uint16_t>(assembly_ids_.size()));
	for (size_t i = 0; i < assembly_ids_.size(); i++) {
		stream.WriteWord(assembly_ids_[i]);
	}
}

/**
* AttributeInfoRecord
*/

AttributeInfoRecord::AttributeInfoRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), attribute_id_(0), owner_type_id_(0), attribute_usage_(0)
{

}

void AttributeInfoRecord::ReadData(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
	owner_type_id_ = stream.ReadWord();
	attribute_usage_ = stream.ReadByte();
	name_ = stream.ReadString();
}

void AttributeInfoRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	stream.WriteWord(owner_type_id_);
	stream.WriteByte(attribute_usage_);
	stream.WriteString(name_);
}

/**
* PropertyDictionaryStartRecord
*/

PropertyDictionaryStartRecord::PropertyDictionaryStartRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

void PropertyDictionaryStartRecord::Read(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
}

void PropertyDictionaryStartRecord::Write(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
}

/**
* PropertyDictionaryEndRecord
*/

PropertyDictionaryEndRecord::PropertyDictionaryEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* StringInfoRecord
*/

StringInfoRecord::StringInfoRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), string_id_(0)
{

}

void StringInfoRecord::ReadData(NETStream &stream)
{
	string_id_ = stream.ReadWord();
	value_ = stream.ReadString();
}

void StringInfoRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(string_id_);
	stream.WriteString(value_);
}

/**
* DeferableContentStartRecord
*/

DeferableContentStartRecord::DeferableContentStartRecord(BamlDocument *owner)
	: BamlRecord(owner), offset_(0), offset_pos_(0), record_(NULL)
{

}

void DeferableContentStartRecord::Read(NETStream &stream)
{
	offset_pos_ = stream.Tell();
	offset_ = stream.ReadDWord();
}

void DeferableContentStartRecord::Write(NETStream &stream)
{
	offset_pos_ = stream.Tell();
	stream.WriteDWord(0);
}

void DeferableContentStartRecord::ReadDefer(size_t index)
{
	record_ = owner()->GetRecordByPosition(offset_pos_ + sizeof(offset_) + offset_);
}

void DeferableContentStartRecord::WriteDefer(size_t index, NETStream &stream)
{
	stream.Seek(offset_pos_, soBeginning);
	stream.WriteDWord(static_cast<uint32_t>(record_->position() - (offset_pos_ + sizeof(offset_))));
}

/**
* DefAttributeKeyStringRecord
*/

DefAttributeKeyStringRecord::DefAttributeKeyStringRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), value_id_(0), offset_(0), shared_(false), shared_set_(false), record_(NULL), offset_pos_(0)
{

}

void DefAttributeKeyStringRecord::ReadData(NETStream &stream)
{
	value_id_ = stream.ReadWord();
	offset_ = stream.ReadDWord();
	shared_ = stream.ReadBoolean();
	shared_set_ = stream.ReadBoolean();
}

void DefAttributeKeyStringRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(value_id_);
	offset_pos_ = static_cast<uint32_t>(stream.Tell());
	stream.WriteDWord(offset_);
	stream.WriteBoolean(shared_);
	stream.WriteBoolean(shared_set_);
}

void DefAttributeKeyStringRecord::ReadDefer(size_t index)
{
	record_ = owner()->GetRecordByPosition(GetPosition(index) + offset_);
}

void DefAttributeKeyStringRecord::WriteDefer(size_t index, NETStream &stream)
{
	stream.Seek(offset_pos_, soBeginning);
	stream.WriteDWord(static_cast<uint32_t>(record_->position() - GetPosition(index)));
}

/**
* DefAttributeKeyTypeRecord
*/

DefAttributeKeyTypeRecord::DefAttributeKeyTypeRecord(BamlDocument *owner)
	: ElementStartRecord(owner), offset_(0), record_(NULL), offset_pos_(0)
{

}

void DefAttributeKeyTypeRecord::Read(NETStream &stream)
{
	ElementStartRecord::Read(stream);
	offset_ = stream.ReadDWord();
	shared_ = stream.ReadBoolean();
	shared_set_ = stream.ReadBoolean();
}

void DefAttributeKeyTypeRecord::Write(NETStream &stream)
{
	ElementStartRecord::Write(stream);
	offset_pos_ = stream.Tell();
	stream.WriteDWord(0);
	stream.WriteBoolean(shared_);
	stream.WriteBoolean(shared_set_);
}

void DefAttributeKeyTypeRecord::ReadDefer(size_t index)
{
	record_ = owner()->GetRecordByPosition(GetPosition(index) + offset_);
}

void DefAttributeKeyTypeRecord::WriteDefer(size_t index, NETStream &stream)
{
	stream.Seek(offset_pos_, soBeginning);
	stream.WriteDWord(static_cast<uint32_t>(record_->position() - GetPosition(index)));
}

/**
* PropertyComplexStartRecord
*/

PropertyComplexStartRecord::PropertyComplexStartRecord(BamlDocument *owner)
	: BamlRecord(owner), attribute_id_(0)
{

}

void PropertyComplexStartRecord::Read(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
}

void PropertyComplexStartRecord::Write(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
}

/**
* PropertyComplexEndRecord
*/

PropertyComplexEndRecord::PropertyComplexEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* PropertyTypeReferenceRecord
*/

PropertyTypeReferenceRecord::PropertyTypeReferenceRecord(BamlDocument *owner)
	: PropertyComplexStartRecord(owner), type_id_(0)
{

}

void PropertyTypeReferenceRecord::Read(NETStream &stream)
{
	PropertyComplexStartRecord::Read(stream);
	type_id_ = stream.ReadWord();
}

void PropertyTypeReferenceRecord::Write(NETStream &stream)
{
	PropertyComplexStartRecord::Write(stream);
	stream.WriteWord(type_id_);
}

/**
* ContentPropertyRecord
*/

ContentPropertyRecord::ContentPropertyRecord(BamlDocument *owner)
	: BamlRecord(owner), attribute_id_(0)
{

}

void ContentPropertyRecord::Read(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
}

void ContentPropertyRecord::Write(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
}

/**
* PropertyCustomRecord
*/

PropertyCustomRecord::PropertyCustomRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), attribute_id_(0), serializer_type_id_(0)
{

}

void PropertyCustomRecord::ReadData(NETStream &stream)
{
	uint64_t pos = stream.Tell();
	attribute_id_ = stream.ReadWord();
	serializer_type_id_ = stream.ReadWord();
	data_.resize(data_size() - static_cast<uint32_t>(stream.Tell() - pos));
	stream.Read(data_.data(), data_.size());
}

void PropertyCustomRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	stream.WriteWord(serializer_type_id_);
	stream.Write(data_.data(), data_.size());
}

/**
* PropertyRecord
*/

PropertyRecord::PropertyRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), attribute_id_(0)
{

}

void PropertyRecord::ReadData(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
	value_ = stream.ReadString();
}

void PropertyRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	stream.WriteString(value_);
}

/**
* PropertyWithConverterRecord
*/

PropertyWithConverterRecord::PropertyWithConverterRecord(BamlDocument *owner)
	: PropertyRecord(owner), converter_type_id_(0)
{

}

void PropertyWithConverterRecord::ReadData(NETStream &stream)
{
	PropertyRecord::ReadData(stream);
	converter_type_id_ = stream.ReadWord();
}

void PropertyWithConverterRecord::WriteData(NETStream &stream)
{
	PropertyRecord::WriteData(stream);
	stream.WriteWord(converter_type_id_);
}

/**
* PIMappingRecord
*/

PIMappingRecord::PIMappingRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), assembly_id_(0)
{

}

void PIMappingRecord::ReadData(NETStream &stream)
{
	xml_namespace_ = stream.ReadString();
	clr_namespace_ = stream.ReadString();
	assembly_id_ = stream.ReadWord();
}

void PIMappingRecord::WriteData(NETStream &stream)
{
	stream.WriteString(xml_namespace_);
	stream.WriteString(clr_namespace_);
	stream.WriteWord(assembly_id_);
}

/**
* PropertyListStartRecord
*/

PropertyListStartRecord::PropertyListStartRecord(BamlDocument *owner)
	: PropertyComplexStartRecord(owner)
{

}

/**
* PropertyListEndRecord
*/

PropertyListEndRecord::PropertyListEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* ConnectionIdRecord
*/

ConnectionIdRecord::ConnectionIdRecord(BamlDocument *owner)
	: BamlRecord(owner), connection_id_(0)
{

}

void ConnectionIdRecord::Read(NETStream &stream)
{
	connection_id_ = stream.ReadDWord();
}

void ConnectionIdRecord::Write(NETStream &stream)
{
	stream.WriteDWord(connection_id_);
}

/**
* OptimizedStaticResourceRecord
*/

OptimizedStaticResourceRecord::OptimizedStaticResourceRecord(BamlDocument *owner)
	: BamlRecord(owner), flags_(0), value_id_(0)
{

}

void OptimizedStaticResourceRecord::Read(NETStream &stream)
{
	flags_ = stream.ReadByte();
	value_id_ = stream.ReadWord();
}

void OptimizedStaticResourceRecord::Write(NETStream &stream)
{
	stream.WriteByte(flags_);
	stream.WriteWord(value_id_);
}

/**
* ConstructorParametersStartRecord
*/

ConstructorParametersStartRecord::ConstructorParametersStartRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* ConstructorParametersEndRecord
*/

ConstructorParametersEndRecord::ConstructorParametersEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* TextRecord
*/

TextRecord::TextRecord(BamlDocument *owner)
	: SizedBamlRecord(owner)
{

}

void TextRecord::ReadData(NETStream &stream)
{
	value_ = stream.ReadString();
}

void TextRecord::WriteData(NETStream &stream)
{
	stream.WriteString(value_);
}

/**
* StaticResourceIdRecord
*/

StaticResourceIdRecord::StaticResourceIdRecord(BamlDocument *owner)
	: BamlRecord(owner), static_resource_id_(0)
{

}

void StaticResourceIdRecord::Read(NETStream &stream)
{
	static_resource_id_ = stream.ReadWord();
}

void StaticResourceIdRecord::Write(NETStream &stream)
{
	stream.WriteWord(static_resource_id_);
}

/**
* StaticResourceEndRecord
*/

StaticResourceEndRecord::StaticResourceEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* StaticResourceStartRecord
*/

StaticResourceStartRecord::StaticResourceStartRecord(BamlDocument *owner)
	: ElementStartRecord(owner)
{

}

/**
* PropertyWithStaticResourceIdRecord
*/

PropertyWithStaticResourceIdRecord::PropertyWithStaticResourceIdRecord(BamlDocument *owner)
	: StaticResourceIdRecord(owner), attribute_id_(0)
{

}

void PropertyWithStaticResourceIdRecord::Read(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
	StaticResourceIdRecord::Read(stream);
}

void PropertyWithStaticResourceIdRecord::Write(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	StaticResourceIdRecord::Write(stream);
}

/**
* PropertyWithExtensionRecord
*/

PropertyWithExtensionRecord::PropertyWithExtensionRecord(BamlDocument *owner)
	: BamlRecord(owner), attribute_id_(0), flags_(0), value_id_(0)
{

}

void PropertyWithExtensionRecord::Read(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
	flags_ = stream.ReadWord();
	value_id_ = stream.ReadWord();
}

void PropertyWithExtensionRecord::Write(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	stream.WriteWord(flags_);
	stream.WriteWord(value_id_);
}

/**
* DefAttributeRecord
*/

DefAttributeRecord::DefAttributeRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), name_id_(0)
{

}

void DefAttributeRecord::ReadData(NETStream &stream)
{
	value_ = stream.ReadString();
	name_id_ = stream.ReadWord();
}

void DefAttributeRecord::WriteData(NETStream &stream)
{
	stream.WriteString(value_);
	stream.WriteWord(name_id_);
}

/**
* KeyElementStartRecord
*/

KeyElementStartRecord::KeyElementStartRecord(BamlDocument *owner)
	: DefAttributeKeyTypeRecord(owner)
{

}

/**
* KeyElementEndRecord
*/

KeyElementEndRecord::KeyElementEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* ConstructorParameterTypeRecord
*/

ConstructorParameterTypeRecord::ConstructorParameterTypeRecord(BamlDocument *owner)
	: BamlRecord(owner), type_id_(0)
{

}

void ConstructorParameterTypeRecord::Read(NETStream &stream)
{
	type_id_ = stream.ReadWord();
}

void ConstructorParameterTypeRecord::Write(NETStream &stream)
{
	stream.WriteWord(type_id_);
}

/**
* TextWithConverterRecord
*/

TextWithConverterRecord::TextWithConverterRecord(BamlDocument *owner)
	: TextRecord(owner), converter_type_id_(0)
{

}

void TextWithConverterRecord::ReadData(NETStream &stream)
{
	TextRecord::ReadData(stream);
	converter_type_id_ = stream.ReadWord();
}

void TextWithConverterRecord::WriteData(NETStream &stream)
{
	TextRecord::WriteData(stream);
	stream.WriteWord(converter_type_id_);
}

/**
* TextWithIdRecord
*/

TextWithIdRecord::TextWithIdRecord(BamlDocument *owner)
	: TextRecord(owner), value_id_(0)
{

}

void TextWithIdRecord::ReadData(NETStream &stream)
{
	TextRecord::ReadData(stream);
	value_id_ = stream.ReadWord();
}

void TextWithIdRecord::WriteData(NETStream &stream)
{
	TextRecord::WriteData(stream);
	stream.WriteWord(value_id_);
}

/**
* LineNumberAndPositionRecord
*/

LineNumberAndPositionRecord::LineNumberAndPositionRecord(BamlDocument *owner)
	: BamlRecord(owner), line_number_(0), line_position_(0)
{

}

void LineNumberAndPositionRecord::Read(NETStream &stream)
{
	line_number_ = stream.ReadDWord();
	line_position_ = stream.ReadDWord();
}

void LineNumberAndPositionRecord::Write(NETStream &stream)
{
	stream.WriteDWord(line_number_);
	stream.WriteDWord(line_position_);
}

/**
* LinePositionRecord
*/

LinePositionRecord::LinePositionRecord(BamlDocument *owner)
	: BamlRecord(owner), line_position_(0)
{

}

void LinePositionRecord::Read(NETStream &stream)
{
	line_position_ = stream.ReadDWord();
}

void LinePositionRecord::Write(NETStream &stream)
{
	stream.WriteDWord(line_position_);
}

/**
* LiteralContentRecord
*/

LiteralContentRecord::LiteralContentRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), reserved0_(0), reserved1_(0)
{

}

void LiteralContentRecord::ReadData(NETStream &stream)
{
	value_ = stream.ReadString();
	reserved0_ = stream.ReadDWord();
	reserved1_ = stream.ReadDWord();
}

void LiteralContentRecord::WriteData(NETStream &stream)
{
	stream.WriteString(value_);
	stream.WriteDWord(reserved0_);
	stream.WriteDWord(reserved1_);
}

/**
* PresentationOptionsAttributeRecord
*/

PresentationOptionsAttributeRecord::PresentationOptionsAttributeRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), name_id_(0)
{

}

void PresentationOptionsAttributeRecord::ReadData(NETStream &stream)
{
	value_ = stream.ReadString();
	name_id_ = stream.ReadWord();
}

void PresentationOptionsAttributeRecord::WriteData(NETStream &stream)
{
	stream.WriteString(value_);
	stream.WriteWord(name_id_);
}

/**
* PropertyArrayStartRecord
*/

PropertyArrayStartRecord::PropertyArrayStartRecord(BamlDocument *owner)
	: PropertyComplexStartRecord(owner)
{

}

/**
* PropertyArrayStartRecord
*/

PropertyArrayEndRecord::PropertyArrayEndRecord(BamlDocument *owner)
	: BamlRecord(owner)
{

}

/**
* PropertyStringReferenceRecord
*/

PropertyStringReferenceRecord::PropertyStringReferenceRecord(BamlDocument *owner)
	: PropertyComplexStartRecord(owner), string_id_(0)
{

}

void PropertyStringReferenceRecord::Read(NETStream &stream)
{
	PropertyComplexStartRecord::Read(stream);
	string_id_ = stream.ReadWord();
}

void PropertyStringReferenceRecord::Write(NETStream &stream)
{
	PropertyComplexStartRecord::Write(stream);
	stream.WriteWord(string_id_);
}

/**
* RoutedEventRecord
*/

RoutedEventRecord::RoutedEventRecord(BamlDocument *owner)
	: SizedBamlRecord(owner), attribute_id_(0)
{

}

void RoutedEventRecord::ReadData(NETStream &stream)
{
	attribute_id_ = stream.ReadWord();
	value_ = stream.ReadString();
}

void RoutedEventRecord::WriteData(NETStream &stream)
{
	stream.WriteWord(attribute_id_);
	stream.WriteString(value_);
}

/**
* TypeSerializerInfoRecord
*/

TypeSerializerInfoRecord::TypeSerializerInfoRecord(BamlDocument *owner)
	: TypeInfoRecord(owner), type_id_(0)
{

}

void TypeSerializerInfoRecord::ReadData(NETStream &stream)
{
	TypeInfoRecord::ReadData(stream);
	type_id_ = stream.ReadWord();
}

void TypeSerializerInfoRecord::WriteData(NETStream &stream)
{
	TypeInfoRecord::WriteData(stream);
	stream.WriteWord(type_id_);
}

/**
* NETStream
*/

NETStream::NETStream() 
	: MemoryStream() 
{

}

NETStream::NETStream(AbstractStream &stream, size_t size)
{
	CopyFrom(stream, size);
	Seek(0, soBeginning);
}

size_t NETStream::Read(void *Buffer, size_t Count)
{
	size_t res = MemoryStream::Read(Buffer, Count);
	if (res != Count)
		throw std::runtime_error("Runtime error at Read");
	return res;
}

uint8_t NETStream::ReadByte()
{
	uint8_t b;
	Read(&b, sizeof(b));
	return b;
}

uint16_t NETStream::ReadWord()
{
	uint16_t w;
	Read(&w, sizeof(w));
	return w;
}

uint32_t NETStream::ReadDWord()
{
	uint32_t dw;
	Read(&dw, sizeof(dw));
	return dw;
}

bool NETStream::ReadBoolean()
{
	return (ReadByte() != 0);
}

uint32_t NETStream::ReadEncoded()
{
	uint32_t res;
	uint8_t b = ReadByte();
	if ((b & 0x80) == 0) {
		res = b & 0x7f;
	}
	else if ((b & 0x40) == 0) {
		res = ((b & 0x3f) << 8) + ReadByte();
	}
	else {
		res = ((b & 0x1f) << 24) + (ReadByte() << 16) + (ReadByte() << 8) + ReadByte();
	}
	return res;
}

uint32_t NETStream::ReadEncoded7Bit()
{
	uint32_t res = 0;
	size_t shift = 0;
	uint8_t b;
	do {
		b = ReadByte();
		res |= (b & 0x7F) << shift;
		shift += 7;
	} while ((b & 0x80) != 0);
	return res;
}

std::string NETStream::ReadString()
{
	std::string res;
	uint32_t size = ReadEncoded7Bit();
	for (size_t i = 0; i < size; i++) {
		res.push_back(ReadByte());
	}
	return res;
}

std::string NETStream::ReadUnicodeString()
{
	os::unicode_string res;
	uint32_t size = ReadEncoded7Bit() >> 1;
	for (size_t i = 0; i < size; i++) {
		res.push_back(ReadWord());
	}
	return os::ToUTF8(res);
}

void NETStream::WriteByte(uint8_t value)
{
	Write(&value, sizeof(value));
}

void NETStream::WriteWord(uint16_t value)
{
	Write(&value, sizeof(value));
}

void NETStream::WriteDWord(uint32_t value)
{
	Write(&value, sizeof(value));
}

void NETStream::WriteEncoded7Bit(uint32_t value)
{
	while (value >= 0x80) {
		WriteByte(static_cast<uint8_t>(value) | 0x80);
		value >>= 7;
	}
	WriteByte(static_cast<uint8_t>(value));
}

void NETStream::WriteString(const std::string &value)
{
	WriteEncoded7Bit(static_cast<uint32_t>(value.size()));
	Write(value.c_str(), value.size());
}

void NETStream::WriteBoolean(bool value)
{
	WriteByte(value ? 1 : 0);
}

void NETStream::WriteUnicodeString(const os::unicode_string &value)
{
	WriteEncoded7Bit(static_cast<uint32_t>(value.size()) << 1);
	for (size_t i = 0; i < value.size(); i++) {
		WriteWord(value[i]);
	}
}

void NETStream::WriteEncoded(uint32_t value)
{
	if (value < 0x80) {
		WriteByte(static_cast<uint8_t>(value));
	}
	else if (value < 0x4000) {
		WriteByte(static_cast<uint8_t>(value >> 8) | 0x80);
		WriteByte(static_cast<uint8_t>(value));
	}
	else if (value < 0x02000000) {
		WriteByte(static_cast<uint8_t>(value >> 24) | 0xc0);
		WriteByte(static_cast<uint8_t>(value >> 16));
		WriteByte(static_cast<uint8_t>(value >> 8));
		WriteByte(static_cast<uint8_t>(value));
	}
	else {
		throw std::runtime_error("Value can not be encoded");
	}
}

/**
* BamlDocument
*/

BamlDocument::BamlDocument()
	: ObjectList<BamlRecord>(), reader_version_major_(0), reader_version_minor_(0), updater_version_major_(0), updater_version_minor_(0),
	writer_version_major_(0), writer_version_minor_(0)
{

}

void BamlDocument::clear()
{
	ObjectList<BamlRecord>::clear();
	reader_version_major_ = 0;
	reader_version_minor_ = 0;
	updater_version_major_ = 0;
	updater_version_minor_ = 0;
	writer_version_major_ = 0;
	writer_version_minor_ = 0;
}

BamlRecord *BamlDocument::GetRecordByPosition(uint64_t pos) const
{
	for (size_t i = 0; i < count(); i++) {
		BamlRecord *record = item(i);
		if (record->position() == pos)
			return record;
	}
	return NULL;
}

size_t BamlDocument::GetClosedIndex(BamlRecordType start, BamlRecordType end, size_t index)
{
	while (index < count()) {
		BamlRecord *record = item(index);
		if (record->type() == start) {
			index = GetClosedIndex(start, end, index + 1);
			if (index == NOT_ID)
				break;
		} else if (record->type() == end)
			return index;
		index++;
	}
	return NOT_ID;
}

bool BamlDocument::ReadFromStream(NETStream &stream)
{
	clear();

	const os::unicode_char MSBAML[6] = { 'M', 'S', 'B', 'A', 'M', 'L' };

	uint32_t signature_size = stream.ReadDWord();
	signature_.resize(signature_size >> 1);
	stream.Read(&signature_[0], signature_.size() * sizeof(os::unicode_char));

	if (signature_.size() != _countof(MSBAML) || memcmp(signature_.data(), MSBAML, signature_.size() * sizeof(os::unicode_char)))
		return false;

	reader_version_major_ = stream.ReadWord();
	reader_version_minor_ = stream.ReadWord();
	updater_version_major_ = stream.ReadWord();
	updater_version_minor_ = stream.ReadWord();
	writer_version_major_ = stream.ReadWord();
	writer_version_minor_ = stream.ReadWord();

	if (reader_version_major_ != 0 || reader_version_minor_ != 0x60 ||
		updater_version_major_ != 0 || updater_version_minor_ != 0x60 ||
		writer_version_major_ != 0 || writer_version_minor_ != 0x60)
		return false;

	uint64_t size = stream.Size();
	while (stream.Tell() < size) {
		uint64_t pos = stream.Tell();
		uint8_t type = stream.ReadByte();
		BamlRecord *rec;
		switch (type) {
		case AssemblyInfo:
			rec = new AssemblyInfoRecord(this);
			break;
		case AttributeInfo:
			rec = new AttributeInfoRecord(this);
			break;
		case ConstructorParametersStart:
			rec = new ConstructorParametersStartRecord(this);
			break;
		case ConstructorParametersEnd:
			rec = new ConstructorParametersEndRecord(this);
			break;
		case ConstructorParameterType:
			rec = new ConstructorParameterTypeRecord(this);
			break;
		case ConnectionId:
			rec = new ConnectionIdRecord(this);
			break;
		case ContentProperty:
			rec = new ContentPropertyRecord(this);
			break;
		case DefAttribute:
			rec = new DefAttributeRecord(this);
			break;
		case DefAttributeKeyString:
			rec = new DefAttributeKeyStringRecord(this);
			break;
		case DefAttributeKeyType:
			rec = new DefAttributeKeyTypeRecord(this);
			break;
		case DeferableContentStart:
			rec = new DeferableContentStartRecord(this);
			break;
		case DocumentEnd:
			rec = new DocumentEndRecord(this);
			break;
		case DocumentStart:
			rec = new DocumentStartRecord(this);
			break;
		case ElementEnd:
			rec = new ElementEndRecord(this);
			break;
		case ElementStart:
			rec = new ElementStartRecord(this);
			break;
		case KeyElementEnd:
			rec = new KeyElementEndRecord(this);
			break;
		case KeyElementStart:
			rec = new KeyElementStartRecord(this);
			break;
		case LineNumberAndPosition:
			rec = new LineNumberAndPositionRecord(this);
			break;
		case LinePosition:
			rec = new LinePositionRecord(this);
			break;
		case LiteralContent:
			rec = new LiteralContentRecord(this);
			break;
		case OptimizedStaticResource:
			rec = new OptimizedStaticResourceRecord(this);
			break;
		case PIMapping:
			rec = new PIMappingRecord(this);
			break;
		case PresentationOptionsAttribute:
			rec = new PresentationOptionsAttributeRecord(this);
			break;
		case Property:
			rec = new PropertyRecord(this);
			break;
		case PropertyArrayEnd:
			rec = new PropertyArrayEndRecord(this);
			break;
		case PropertyArrayStart:
			rec = new PropertyArrayStartRecord(this);
			break;
		case PropertyComplexEnd:
			rec = new PropertyComplexEndRecord(this);
			break;
		case PropertyComplexStart:
			rec = new PropertyComplexStartRecord(this);
			break;
		case PropertyCustom:
			rec = new PropertyCustomRecord(this);
			break;
		case PropertyDictionaryEnd:
			rec = new PropertyDictionaryEndRecord(this);
			break;
		case PropertyDictionaryStart:
			rec = new PropertyDictionaryStartRecord(this);
			break;
		case PropertyListEnd:
			rec = new PropertyListEndRecord(this);
			break;
		case PropertyListStart:
			rec = new PropertyListStartRecord(this);
			break;
		case PropertyStringReference:
			rec = new PropertyStringReferenceRecord(this);
			break;
		case PropertyTypeReference:
			rec = new PropertyTypeReferenceRecord(this);
			break;
		case PropertyWithConverter:
			rec = new PropertyWithConverterRecord(this);
			break;
		case PropertyWithExtension:
			rec = new PropertyWithExtensionRecord(this);
			break;
		case PropertyWithStaticResourceId:
			rec = new PropertyWithStaticResourceIdRecord(this);
			break;
		case RoutedEvent:
			rec = new RoutedEventRecord(this);
			break;
		case StaticResourceEnd:
			rec = new StaticResourceEndRecord(this);
			break;
		case StaticResourceId:
			rec = new StaticResourceIdRecord(this);
			break;
		case StaticResourceStart:
			rec = new StaticResourceStartRecord(this);
			break;
		case StringInfo:
			rec = new StringInfoRecord(this);
			break;
		case Text:
			rec = new TextRecord(this);
			break;
		case TextWithConverter:
			rec = new TextWithConverterRecord(this);
			break;
		case TextWithId:
			rec = new TextWithIdRecord(this);
			break;
		case TypeInfo:
			rec = new TypeInfoRecord(this);
			break;
		case XmlnsProperty:
			rec = new XmlnsPropertyRecord(this);
			break;
		case TypeSerializerInfo:
			rec = new TypeSerializerInfoRecord(this);
			break;
		default:
			throw std::runtime_error(string_format("Unknown BAML record type: %x", type));
		}
		rec->set_position(pos);
		AddObject(rec);

		rec->Read(stream);
	}

	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadDefer(i);
	}

	return true;
}

void BamlDocument::WriteToStream(NETStream &stream)
{
	stream.WriteDWord(static_cast<uint32_t>(signature_.size() * sizeof(os::unicode_char)));
	stream.Write(signature_.data(), signature_.size() * sizeof(os::unicode_char));
	stream.WriteWord(reader_version_major_);
	stream.WriteWord(reader_version_minor_);
	stream.WriteWord(updater_version_major_);
	stream.WriteWord(updater_version_minor_);
	stream.WriteWord(writer_version_major_);
	stream.WriteWord(writer_version_minor_);

	size_t i;
	for (i = 0; i < count(); i++) {
		BamlRecord *rec = item(i);
		rec->set_position(stream.Tell());
		stream.WriteByte(rec->type());
		rec->Write(stream);
	}
	for (i = 0; i < count(); i++) {
		item(i)->WriteDefer(i, stream);
	}
}

/**
* BamlElement
*/

BamlElement::BamlElement(BamlElement *parent)
	: IObject(), parent_(parent), header_(NULL), footer_(NULL), type_(NULL)
{

}

BamlElement::~BamlElement()
{
	for (size_t i = 0; i < children_.size(); i++) {
		delete children_[i];
	}
}

bool BamlElement::is_header(BamlRecordType type)
{
	switch (type) {
	case ConstructorParametersStart:
	case DocumentStart:
	case ElementStart:
	case KeyElementStart:
	case NamedElementStart:
	case PropertyArrayStart:
	case PropertyComplexStart:
	case PropertyDictionaryStart:
	case PropertyListStart:
	case StaticResourceStart:
		return true;
	}
	return false;
}

bool BamlElement::is_footer(BamlRecordType type)
{
	switch (type) {
	case ConstructorParametersEnd:
	case DocumentEnd:
	case ElementEnd:
	case KeyElementEnd:
	case PropertyArrayEnd:
	case PropertyComplexEnd:
	case PropertyDictionaryEnd:
	case PropertyListEnd:
	case StaticResourceEnd:
		return true;
	}
	return false;
}

bool BamlElement::is_match(BamlRecordType header, BamlRecordType footer)
{
	switch (header) {
	case ConstructorParametersStart:
		return footer == ConstructorParametersEnd;

	case DocumentStart:
		return footer == DocumentEnd;

	case KeyElementStart:
		return footer == KeyElementEnd;

	case PropertyArrayStart:
		return footer == PropertyArrayEnd;

	case PropertyComplexStart:
		return footer == PropertyComplexEnd;

	case PropertyDictionaryStart:
		return footer == PropertyDictionaryEnd;

	case PropertyListStart:
		return footer == PropertyListEnd;

	case StaticResourceStart:
		return footer == StaticResourceEnd;

	case ElementStart:
	case NamedElementStart:
		return footer == ElementEnd;
	}
	return false;
}

bool BamlElement::Parse(BamlDocument *document)
{
	std::vector<BamlElement *> stack;
	BamlElement *current = NULL;

	for (size_t i = 0; i < document->count(); i++) {
		BamlRecord *record = document->item(i);
		if (is_header(record->type())) {
			BamlElement *prev = current;

			current = prev ? new BamlElement(prev) : this;
			current->header_ = record;
			if (prev) {
				prev->children_.push_back(current);
				stack.push_back(prev);
			}
		}
		else if (is_footer(record->type())) {
			if (!current)
				return false;

			while (!is_match(current->header_->type(), record->type())) {
				if (stack.empty())
					return false;

				current = stack.back();
				stack.pop_back();
			}

			current->footer_ = record;
			if (!stack.empty()) {
				current = stack.back();
				stack.pop_back();
			}
		}
		else {
			if (!current)
				return false;

			current->body_.push_back(record);
		}
	}

	return (header_ && header_->type() == DocumentStart && footer_ && footer_->type() == DocumentEnd);
}

/**
* CustomAttributeValue
*/

CustomAttributeValue::CustomAttributeValue(ILMetaData *meta)
	: IObject(), meta_(meta)
{
	fixed_list_ = new CustomAttributeArgumentList();
	named_list_ = new CustomAttributeNamedArgumentList();
}

CustomAttributeValue::~CustomAttributeValue()
{
	delete fixed_list_;
	delete named_list_;
}

void CustomAttributeValue::clear()
{
	fixed_list_->clear();
	named_list_->clear();
}

void CustomAttributeValue::Parse(ILToken *type, const ILData &data)
{
	clear();

	uint32_t id = 0;
	if (data.ReadWord(id) != 1)
		throw std::runtime_error("Invalid format");

	ILSignature *signature;
	switch (type->type()) {
	case ttMethodDef:
		signature = reinterpret_cast<ILMethodDef *>(type)->signature();
		break;
	case ttMemberRef:
		signature = reinterpret_cast<ILMemberRef *>(type)->signature();
		break;
	default:
		throw std::runtime_error("Invalid format");
	}

	size_t i, c;

	for (i = 0; i < signature->count(); i++) {
		CustomAttributeFixedArgument *arg = new CustomAttributeFixedArgument(meta_, fixed_list_, signature->item(i));
		fixed_list_->AddObject(arg);
		arg->Read(data, id);
	}

	c = data.ReadWord(id);
	for (i = 0; i < c; i++) {
		CustomAttributeNamedArgument *arg = new CustomAttributeNamedArgument(meta_, named_list_);
		named_list_->AddObject(arg);
		arg->Read(data, id);
	}
}

void CustomAttributeValue::Write(ILData &data)
{
	size_t i;

	data.WriteWord(1);
	for (i = 0; i < fixed_list_->count(); i++) {
		fixed_list_->item(i)->Write(data);
	}

	data.WriteWord(static_cast<uint16_t>(named_list_->count()));
	for (i = 0; i < named_list_->count(); i++) {
		named_list_->item(i)->Write(data);
	}
}

/**
* BaseCustomAttributeArgument
*/

CustomAttributeArgument::CustomAttributeArgument(ILMetaData *meta, CustomAttributeArgumentList *owner)
	: IObject(), meta_(meta), owner_(owner), children_(NULL), reference_(NULL)
{

}

CustomAttributeArgument::~CustomAttributeArgument()
{
	if (owner_)
		owner_->RemoveObject(this);
	delete children_;
}

ILElement *CustomAttributeArgument::ReadFieldOrPropType(const ILData &data, uint32_t &id)
{
	CorElementType type = static_cast<CorElementType>(data.ReadByte(id));
	switch (type) {
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_TYPE:
	case ELEMENT_TYPE_TAGGED_OBJECT:
		return new ILElement(meta_, NULL, type);
	case ELEMENT_TYPE_SZARRAY:
		return new ILElement(meta_, NULL, ELEMENT_TYPE_SZARRAY, NULL, ReadFieldOrPropType(data, id));
	case ELEMENT_TYPE_ENUM:
		return new ILElement(meta_, NULL, ELEMENT_TYPE_ENUM, ReadType(data, id), NULL);
	default:
		throw std::runtime_error("Invalid serialization type");
	}
}

CustomAttributeFixedArgument *CustomAttributeArgument::AddChild(ILElement *type)
{
	if (!children_)
		children_ = new CustomAttributeArgumentList();
	CustomAttributeFixedArgument *res = new CustomAttributeFixedArgument(meta_, children_, type);
	children_->AddObject(res);
	return res;
}

void CustomAttributeArgument::Read(const ILData &data, uint32_t &id)
{
	ReadValue(data, id, type());
}

void CustomAttributeArgument::ReadEnum(const ILData &data, uint32_t &id, ILToken *type)
{
	ILTypeDef *type_def;
	switch (type->type()) {
	case ttTypeDef:
		type_def = reinterpret_cast<ILTypeDef *>(type);
		break;
	case ttTypeRef:
		type_def = reinterpret_cast<ILTypeRef *>(type)->Resolve(true);
		break;
	default:
		throw std::runtime_error("Invalid token type");
	}

	ILElement *underlying_type = type_def->GetEnumUnderlyingType();
	if (!underlying_type || underlying_type->type() < ELEMENT_TYPE_BOOLEAN || underlying_type->type() > ELEMENT_TYPE_U8)
		throw std::runtime_error("Invalid enum underlying type");

	ReadValue(data, id, underlying_type);
}

void CustomAttributeArgument::ReadObject(const ILData &data, uint32_t &id)
{
	AddChild(ReadFieldOrPropType(data, id))->Read(data, id);
}

void CustomAttributeArgument::ReadArray(const ILData &data, uint32_t &id, ILElement *type)
{
	size_t size = data.ReadDWord(id);
	if (size == UINT32_MAX)
		return;

	for (size_t i = 0; i < size; i++) {
		AddChild(type)->Read(data, id);
	}
}

std::string CustomAttributeArgument::ReadString(const ILData &data, uint32_t &id)
{
	std::string res;
	uint32_t old_pos = id;
	if (data.ReadByte(id) != 0xff) {
		--id;
		res = data.ReadString(id);
	}
	value_.Write(data.data() + old_pos, id - old_pos);
	return res;
}

ILTypeDef *CustomAttributeArgument::ReadType(const ILData &data, uint32_t &id)
{
	std::string type_name = data.ReadString(id);
	if (type_name.empty())
		throw std::runtime_error("Invalid format");

	return meta_->ResolveType(type_name, true);
}

void CustomAttributeArgument::ReadValue(const ILData &data, uint32_t &id, ILElement *type)
{
	switch (type->type()) {
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
		value_.WriteByte(data.ReadByte(id));
		break;
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
		value_.WriteWord(data.ReadWord(id));
		break;
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_R4:
		value_.WriteDWord(data.ReadDWord(id));
		break;
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R8:
		value_.WriteQWord(data.ReadQWord(id));
		break;
	case ELEMENT_TYPE_STRING:
		ReadString(data, id);
		break;
	case ELEMENT_TYPE_TYPE:
		reference_ = ReadType(data, id);
		break;
	case ELEMENT_TYPE_SZARRAY:
		ReadArray(data, id, type->next());
		break;
	case ELEMENT_TYPE_OBJECT:
	case ELEMENT_TYPE_TAGGED_OBJECT:
		ReadObject(data, id);
		break;
	case ELEMENT_TYPE_ENUM:
		reference_ = reinterpret_cast<ILTypeDef *>(type->token());
		ReadEnum(data, id, type->token());
		break;
	case ELEMENT_TYPE_VALUETYPE:
		ReadEnum(data, id, type->token());
		break;
	case ELEMENT_TYPE_CLASS:
		if (type->token()->type() == ttTypeRef) {
			ILTypeRef *type_ref = reinterpret_cast<ILTypeRef *>(type->token());
			if (type_ref->resolution_scope() == type_ref->meta()->GetCoreLib() && type_ref->name_space() == "System") {
				if (type_ref->name() == "Type") {
					reference_ = ReadType(data, id);
					break;
				} else if (type_ref->name() == "String") {
					ReadString(data, id);
					break;
				}
				else if (type_ref->name() == "Object") {
					ReadObject(data, id);
					break;
				}
			}
		}
		ReadEnum(data, id, type->token());
		break;
	default:
		throw std::runtime_error("Invalid element type");
	}
}

void CustomAttributeArgument::Write(ILData &data) const
{
	WriteValue(data, type());
}

void CustomAttributeArgument::WriteArray(ILData &data) const
{
	if (!children_) {
		data.WriteDWord(UINT32_MAX);
		return;
	}

	data.WriteDWord(static_cast<uint32_t>(children_->count()));
	for (size_t i = 0; i < children_->count(); i++) {
		children_->item(i)->Write(data);
	}
}

void CustomAttributeArgument::WriteObject(ILData &data) const
{
	CustomAttributeArgument *child = children_->item(0);
	WriteFieldOrPropType(data, child->type());
	child->Write(data);
}

void CustomAttributeArgument::WriteType(ILData &data, ILTypeDef *type_def) const
{
	std::string type_name = type_def->reflection_name();
	if (type_def->meta() != meta_) {
		ILAssembly *assembly = reinterpret_cast<ILAssembly *>(type_def->meta()->token(ttAssembly | 1));
		type_name += ", " + assembly->full_name();
	}
	data.WriteString(type_name);
}

void CustomAttributeArgument::WriteFieldOrPropType(ILData &data, ILElement *type) const
{
	data.WriteByte(type->type());
	switch (type->type()) {
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_TYPE:
	case ELEMENT_TYPE_TAGGED_OBJECT:
		break;
	case ELEMENT_TYPE_SZARRAY:
		WriteFieldOrPropType(data, type->next());
		break;
	case ELEMENT_TYPE_ENUM:
		WriteType(data, reinterpret_cast<ILTypeDef *>(type->token()));
		break;
	}
}

void CustomAttributeArgument::WriteValue(ILData &data, ILElement *type) const
{
	switch (type->type()) {
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_I1:
	case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_I2:
	case ELEMENT_TYPE_U2:
	case ELEMENT_TYPE_I4:
	case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_R4:
	case ELEMENT_TYPE_I8:
	case ELEMENT_TYPE_U8:
	case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_STRING:
	case ELEMENT_TYPE_TYPE:
	case ELEMENT_TYPE_VALUETYPE:
	case ELEMENT_TYPE_ENUM:
		data.Write(value_.data(), value_.size());
		break;
	case ELEMENT_TYPE_SZARRAY:
		WriteArray(data);
		break;
	case ELEMENT_TYPE_OBJECT:
	case ELEMENT_TYPE_TAGGED_OBJECT:
		WriteObject(data);
		break;
	case ELEMENT_TYPE_CLASS:
		if (reference_) // System.Type
			WriteType(data, reference_);
		else if (children_) // System.Object
			WriteObject(data);
		else 
			data.Write(value_.data(), value_.size());
		break;
	default:
		throw std::runtime_error("Invalid element type");
	}
}

bool CustomAttributeArgument::ToBoolean() const
{
	if (type()->type() != ELEMENT_TYPE_BOOLEAN)
		throw std::runtime_error("Invalid element type");
	if (value_.size() != 1)
		throw std::runtime_error("Invalid value size");
	return (value_[0] != 0);
}

std::string CustomAttributeArgument::ToString() const
{
	if (type()->type() != ELEMENT_TYPE_STRING)
		throw std::runtime_error("Invalid element type");
	uint32_t pos = 0;
	return value_.ReadString(pos);
}

/**
* CustomAttributeArgument
*/

CustomAttributeFixedArgument::CustomAttributeFixedArgument(ILMetaData *meta, CustomAttributeArgumentList *owner, ILElement *type)
	: CustomAttributeArgument(meta, owner), type_(type)
{

}

/**
* CustomAttributeArgumentList
*/

CustomAttributeArgumentList::CustomAttributeArgumentList()
	: ObjectList<CustomAttributeArgument>()
{

}

/**
* CustomAttributeNamedArgument
*/

CustomAttributeNamedArgument::CustomAttributeNamedArgument(ILMetaData *meta, CustomAttributeArgumentList *owner)
	: CustomAttributeArgument(meta, owner), is_field_(false), type_(NULL)
{

}

CustomAttributeNamedArgument::~CustomAttributeNamedArgument()
{
	delete type_;
}

void CustomAttributeNamedArgument::Read(const ILData &data, uint32_t &id)
{
	switch (data.ReadByte(id)) {
	case 0x53:
		is_field_ = true;
		break;
	case 0x54:
		is_field_ = false;
		break;
	default:
		throw std::runtime_error("Invalid format");
	}
	type_ = ReadFieldOrPropType(data, id);
	name_ = data.ReadString(id);
	CustomAttributeArgument::Read(data, id);
}

void CustomAttributeNamedArgument::Write(ILData &data) const
{
	data.WriteByte(is_field_ ? 0x53 : 0x54);
	WriteFieldOrPropType(data, type());
	data.WriteString(name_);
	CustomAttributeArgument::Write(data);
}

/**
* CustomAttributeNamedArgumentList
*/

CustomAttributeNamedArgumentList::CustomAttributeNamedArgumentList()
	: CustomAttributeArgumentList()
{

}

CustomAttributeNamedArgument *CustomAttributeNamedArgumentList::item(size_t index) const
{
	return reinterpret_cast<CustomAttributeNamedArgument *>(CustomAttributeArgumentList::item(index));
}

/**
* FrameworkVersion
*/

bool try_stou(const std::string &str, uint32_t *value)
{
	if (str.empty())
		return false;

	if (str.find_first_not_of("0123456789") != std::string::npos)
		return false;

	*value = std::stoul(str);
	return true;
}

bool FrameworkVersion::Parse(const std::string &version)
{
	size_t pos;
	size_t start = 0;
	size_t value_index = 0;
	uint32_t values[3] = { 0 };
	uint32_t num;
	while (value_index < _countof(values)) {
		pos = version.find('.', start);
		if (pos != std::string::npos) {
			if (try_stou(version.substr(start, pos - start), &num)) {
				values[value_index] = num;
				value_index++;
				start = pos + 1;
				continue;
			}
		}
		pos = version.find_first_not_of("0123456789", start);
		if (try_stou(version.substr(start, (pos != std::string::npos) ? pos - start : std::string::npos), &num))
			values[value_index] = num;
		break;
	}

	if (value_index == 0)
		return false;

	major = values[0];
	minor = values[1];
	patch = values[2];
	return true;
}

/**
* AssemblyName
*/

AssemblyName::AssemblyName(const std::string &name)
{
	size_t start, end;
	for (start = 0; ; start = end + 1) {
		end = name.find(',', start);
		while (name[start] == ' ')
			start++;
		std::string str = (end == std::string::npos) ? name.substr(start) : name.substr(start, end - start);
		if (start == 0)
			this->name = str;
		else {
			size_t p = str.find('=');
			if (p == std::string::npos)
				throw std::runtime_error("Malformed assembly name: " + name);

			std::string key = str.substr(0, p);
			std::string value = str.substr(p + 1);
			if (key == "Version")
				version = value;
			else if (key == "Culture")
				culture = value;
			else if (key == "PublicKeyToken")
				public_key_token = value;
		}

		if (end == std::string::npos)
			break;
	}

	if (culture == "neutral")
		culture.clear();
}

std::string AssemblyName::value() const
{
	std::string res = name;
	res += ", Version=" + version;
	res += ", Culture=" + (culture.empty() ? "neutral" : culture);
	res += ", PublicKeyToken=" + (public_key_token.empty() ? "null" : public_key_token);
	return res;
}

/**
* AssemblyResolver
*/

AssemblyResolver::AssemblyResolver()
	: IObject()
{
#ifdef VMP_GNU
#else
	win_dir_ = os::GetEnvironmentVariable("WINDIR");

	std::string root = os::GetEnvironmentVariable("ProgramFiles");
	if (!root.empty()) {
		root = os::CombinePaths(root.c_str(), "dotnet");
		if (os::FileExists(root.c_str())) {
			root = os::CombinePaths(root.c_str(), "shared");
			if (os::FileExists(root.c_str())) {
				std::vector<std::string> root_list = os::FindFiles(root.c_str(), "*", true);
				for (size_t i = 0; i < root_list.size(); i++) {
					std::string path = os::CombinePaths(root.c_str(), root_list[i].c_str());
					std::vector<std::string> path_list = os::FindFiles(path.c_str(), "*", true);

					for (size_t j = 0; j < path_list.size(); j++) {
						FrameworkVersion version;
						if (!version.Parse(os::ExtractFileName(path_list[j].c_str())))
							continue;

						dotnet_dir_list_.push_back(FrameworkPathInfo(os::CombinePaths(os::CombinePaths(path.c_str(), path_list[j].c_str()).c_str(), ""), version));
					}
				}

				std::sort(dotnet_dir_list_.begin(), dotnet_dir_list_.end(), [](const FrameworkPathInfo &left, const FrameworkPathInfo &right)
				{
					return (left.version < right.version);
				});
			}
		}
	}

	redirect_v2_map_["Accessibility"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["cscompmgd"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["CustomMarshalers"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["IEExecRemote"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["IEHost"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["IIEHost"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["ISymWrapper"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["Microsoft.JScript"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.VisualBasic"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.VisualBasic.Compatibility"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.VisualBasic.Compatibility.Data"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.VisualBasic.Vsa"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.VisualC"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.Vsa"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft.Vsa.Vb.CodeDOMProcessor"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["Microsoft_VsaVb"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "8.0.0.0");
	redirect_v2_map_["mscorcfg"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["mscorlib"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Configuration"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Configuration.Install"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Data"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Data.OracleClient"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Data.SqlXml"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Deployment"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Design"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.DirectoryServices"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.DirectoryServices.Protocols"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Drawing"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Drawing.Design"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.EnterpriseServices"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Management"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Messaging"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Runtime.Remoting"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Runtime.Serialization.Formatters.Soap"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Security"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.ServiceProcess"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Transactions"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Web"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Web.Mobile"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Web.RegularExpressions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Web.Services"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["System.Windows.Forms"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["System.Xml"] = FrameworkRedirectInfo("b77a5c561934e089", "2.0.0.0");
	redirect_v2_map_["vjscor"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["VJSharpCodeProvider"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjsJBC"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjslib"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjslibcw"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["Vjssupuilib"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjsvwaux"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjswfc"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["VJSWfcBrowserStubLib"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjswfccw"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v2_map_["vjswfchtml"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");

	redirect_v4_map_["Accessibility"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["CustomMarshalers"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["ISymWrapper"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.JScript"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["Microsoft.VisualBasic"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["Microsoft.VisualBasic.Compatibility"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["Microsoft.VisualBasic.Compatibility.Data"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["Microsoft.VisualC"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["mscorlib"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Configuration"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Configuration.Install"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Data"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.OracleClient"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.SqlXml"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Deployment"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Design"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.DirectoryServices"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.DirectoryServices.Protocols"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Drawing"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Drawing.Design"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.EnterpriseServices"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Management"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Messaging"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Remoting"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Serialization.Formatters.Soap"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Security"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceProcess"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Transactions"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Web"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Web.Mobile"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Web.RegularExpressions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Web.Services"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Windows.Forms"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Xml"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["AspNetMMCExt"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["sysglobl"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build.Engine"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build.Framework"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["PresentationCFFRasterizer"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationCore"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.Aero"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.Classic"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.Luna"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.Royale"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationUI"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["ReachFramework"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Printing"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Speech"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["UIAutomationClient"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["UIAutomationClientsideProviders"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["UIAutomationProvider"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["UIAutomationTypes"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["WindowsBase"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["WindowsFormsIntegration"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["SMDiagnostics"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IdentityModel"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IdentityModel.Selectors"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IO.Log"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Serialization"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Install"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.WasHosting"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Workflow.Activities"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Workflow.ComponentModel"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Workflow.Runtime"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["Microsoft.Transactions.Bridge"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Transactions.Bridge.Dtc"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.AddIn"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.AddIn.Contract"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel.Composition"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Core"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.DataSetExtensions"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.Linq"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Xml.Linq"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.DirectoryServices.AccountManagement"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Management.Instrumentation"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Net"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Web"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.Extensions"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.Extensions.Design"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Windows.Presentation"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.WorkflowServices"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel.DataAnnotations"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Data.Entity"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.Entity.Design"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.Services"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.Services.Client"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Data.Services.Design"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Web.Abstractions"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.DynamicData"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.DynamicData.Design"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.Entity"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Web.Entity.Design"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Web.Routing"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.CSharp"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Dynamic"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Numerics"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Xaml"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["Microsoft.Workflow.Compiler"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["Microsoft.Activities.Build"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build.Conversion.v4.0"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build.Tasks.v4.0"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Build.Utilities.v4.0"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["Microsoft.Internal.Tasks.Dataflow"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["Microsoft.VisualBasic.Activities.Compiler"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "10.0.0.0");
	redirect_v4_map_["Microsoft.VisualC.STLCLR"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "2.0.0.0");
	redirect_v4_map_["Microsoft.Windows.ApplicationServer.Applications"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationBuildTasks"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.Aero2"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework.AeroLite"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["PresentationFramework-SystemCore"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["PresentationFramework-SystemData"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["PresentationFramework-SystemDrawing"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["PresentationFramework-SystemXml"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["PresentationFramework-SystemXmlLinq"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Activities"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Activities.Core.Presentation"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Activities.DurableInstancing"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Activities.Presentation"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel.Composition.Registration"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Device"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IdentityModel.Services"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IO.Compression"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.IO.Compression.FileSystem"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Net.Http"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Net.Http.WebRequest"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Context"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Caching"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.DurableInstancing"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Runtime.WindowsRuntime"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Runtime.WindowsRuntime.UI.Xaml"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Activation"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Activities"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Channels"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Discovery"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Internals"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Routing"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.ServiceMoniker40"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Web.ApplicationServices"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.DataVisualization"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Web.DataVisualization.Design"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Windows.Controls.Ribbon"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Windows.Forms.DataVisualization"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Windows.Forms.DataVisualization.Design"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Windows.Input.Manipulations"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
	redirect_v4_map_["System.Xaml.Hosting"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["XamlBuildTask"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["XsdBuildTask"] = FrameworkRedirectInfo("31bf3856ad364e35", "4.0.0.0");
	redirect_v4_map_["System.Collections"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Collections.Concurrent"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel.Annotations"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ComponentModel.EventBasedAsync"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Diagnostics.Contracts"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Diagnostics.Debug"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Diagnostics.Tools"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Diagnostics.Tracing"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Dynamic.Runtime"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Globalization"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.IO"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Linq"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Linq.Expressions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Linq.Parallel"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Linq.Queryable"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Net.NetworkInformation"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Net.Primitives"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Net.Requests"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ObjectModel"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Emit"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Emit.ILGeneration"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Emit.Lightweight"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Extensions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Reflection.Primitives"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Resources.ResourceManager"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Extensions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.InteropServices"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.InteropServices.WindowsRuntime"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Numerics"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Serialization.Json"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Serialization.Primitives"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Runtime.Serialization.Xml"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Security.Principal"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Duplex"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Http"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.NetTcp"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Primitives"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.ServiceModel.Security"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Text.Encoding"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Text.Encoding.Extensions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Text.RegularExpressions"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Threading"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Threading.Timer"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Threading.Tasks"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Threading.Tasks.Parallel"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Xml.ReaderWriter"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Xml.XDocument"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Xml.XmlSerializer"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Net.Http.Rtc"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Windows"] = FrameworkRedirectInfo("b03f5f7f11d50a3a", "4.0.0.0");
	redirect_v4_map_["System.Xml.Serialization"] = FrameworkRedirectInfo("b77a5c561934e089", "4.0.0.0");
#endif
}

AssemblyResolver::~AssemblyResolver()
{
	for (std::map<std::string, PEFile *>::const_iterator it = cache_.begin(); it != cache_.end(); it++) {
		delete it->second;
	}
}

void AssemblyResolver::Prepare()
{
	framework_path_map_.clear();
	for (std::map<std::string, PEFile *>::const_iterator it = cache_.begin(); it != cache_.end();) {
		if (!it->second)
			it = cache_.erase(it);
		else
			it++;
	}
}

ILMetaData *AssemblyResolver::Resolve(const ILMetaData &source, const std::string &orig_name)
{
	std::map<std::string, PEFile *>::const_iterator cache_it = cache_.find(orig_name);
	if (cache_it != cache_.end())
		return cache_it->second ? reinterpret_cast<NETArchitecture *>(cache_it->second->item(1))->command_list() : NULL;

	std::string name = orig_name;
	AssemblyName assembly(name);

	if (assembly.culture.empty()) {
		const std::map<std::string, FrameworkRedirectInfo, ci_less> *map;
		FrameworkInfo info = source.framework();
		if (info.type == fwFramework)
			map = (info.version.major >= 4) ? &redirect_v4_map_ : &redirect_v2_map_;
		else 
			map = NULL;

		if (map) {
			std::map<std::string, FrameworkRedirectInfo>::const_iterator redirect_it = map->find(assembly.name);
			if (redirect_it != map->end() && assembly.public_key_token == redirect_it->second.public_key_token) {
				assembly.version = redirect_it->second.version;
				name = assembly.value();

				// find redirected name
				cache_it = cache_.find(name);
				if (cache_it != cache_.end())
					return cache_it->second ? reinterpret_cast<NETArchitecture *>(cache_it->second->item(1))->command_list() : NULL;
			}
		}
	}

	std::vector<std::string> file_names = FindAssemblies(source, assembly);
	PEFile *file = new PEFile();
	for (size_t i = 0; i < file_names.size(); i++) {
		try {
			if (file->Open(file_names[i].c_str(), foRead | foHeaderOnly) == osSuccess) {
				if (file->count() > 1) {
					NETArchitecture *net = reinterpret_cast<NETArchitecture *>(file->item(1));
					std::string loaded_name = net->full_name();

					if (loaded_name != name) {
						AssemblyName loaded(loaded_name);
						if (_strcmpi(loaded.name.c_str(), assembly.name.c_str()) != 0)
							continue;

						if (loaded.version != assembly.version) {
							FrameworkVersion loaded_version;
							if (!loaded_version.Parse(loaded.version))
								continue;

							FrameworkVersion assembly_version;
							if (!assembly_version.Parse(assembly.version))
								continue;

							if (loaded_version.major != assembly_version.major)
								continue;

							if (loaded_version.minor < assembly_version.minor)
								continue;
						}

						if (loaded.culture != assembly.culture)
							continue;

						if (loaded.public_key_token != assembly.public_key_token)
							continue;
					}

					cache_[name] = file;
					return net->command_list();
				}
			}
		}
		catch (...)
		{

		}
		file->Close();
	}
	delete file;

	cache_[name] = NULL;
	return NULL;
}

std::vector<std::string> AssemblyResolver::FindAssemblies(const ILMetaData &source, const AssemblyName &assembly)
{
	std::vector<std::string> res;
	size_t i;

	std::string full_dll_name = assembly.name + ".dll";
	std::string source_path = os::ExtractFilePath(source.owner()->owner()->file_name().c_str());
	FrameworkInfo framework = source.framework();
	if (framework.type != fwUnknown) {
		if (source.owner()->owner()->is_executable())
			framework_path_map_[source_path] = framework;
	}
	if (framework.type == fwUnknown || framework.type == fwStandard) {
		std::map<std::string, FrameworkInfo>::const_iterator it = framework_path_map_.find(source_path);
		if (it != framework_path_map_.end())
			framework = it->second;
	}

	std::string file_name = os::CombinePaths(source_path.c_str(), full_dll_name.c_str());
	if (std::find(res.begin(), res.end(), file_name) == res.end()) {
		if (os::FileExists(file_name.c_str()))
			res.push_back(file_name);
	}
	
	switch (framework.type) {
	case fwFramework:
	case fwStandard:
		if (!win_dir_.empty()) {
			// search in GAC
			std::string prefix, path;
			std::vector<std::string> search_paths;

			if (framework.type == fwStandard || framework.version.major >= 4) {
				prefix = "v4.0_";
				path = os::CombinePaths(os::CombinePaths(win_dir_.c_str(), "Microsoft.NET").c_str(), "assembly");
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_32"));
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_64"));
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_MSIL"));
			} else {
				prefix.clear();
				path = os::CombinePaths(win_dir_.c_str(), "assembly");
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_32"));
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_64"));
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC_MSIL"));
				search_paths.push_back(os::CombinePaths(path.c_str(), "GAC"));
			}

			for (i = 0; i < search_paths.size(); i++) {
				path = os::CombinePaths(os::CombinePaths(search_paths[i].c_str(), assembly.name.c_str()).c_str(), string_format("%s%s_%s_%s", prefix.c_str(), assembly.version.c_str(), assembly.culture.c_str(), assembly.public_key_token.c_str()).c_str());
				std::string file_name = os::CombinePaths(path.c_str(), full_dll_name.c_str());
				if (os::FileExists(file_name.c_str()))
					res.push_back(file_name);
			}
		}
		break;
	case fwCore:
		if (!dotnet_dir_list_.empty()) {
			std::vector<std::string> search_paths;
			std::map<uint32_t, std::vector<std::string> > minor_list;
			for (i = dotnet_dir_list_.size(); i > 0; i--) {
				FrameworkPathInfo info = dotnet_dir_list_[i - 1];
				if (framework.version.major) {
					if (info.version.major != framework.version.major)
						continue;

					// select best minor version
					uint32_t d;
					if (info.version.minor >= framework.version.minor)
						d = info.version.minor - framework.version.minor;
					else
						d = 0x80000000 + framework.version.minor - info.version.minor - 1;

					std::map<uint32_t, std::vector<std::string> >::iterator it = minor_list.find(d);
					if (it == minor_list.end()) {
						std::vector<std::string> list;
						list.push_back(info.path);
						minor_list[d] = list;
					}
					else
						it->second.push_back(info.path);
				} else 
					search_paths.push_back(info.path);
			}

			if (!minor_list.empty()) {
				std::map<uint32_t, std::vector<std::string> >::const_iterator it = minor_list.begin();
				for (i = 0; i < it->second.size(); i++) {
					search_paths.push_back(it->second[i]);
				}
			}

			for (i = 0; i < search_paths.size(); i++) {
				std::string path = search_paths[i];
				std::string file_name = os::CombinePaths(path.c_str(), full_dll_name.c_str());
				if (os::FileExists(file_name.c_str()))
					res.push_back(file_name);
			}
		}
		break;
	}

	return res;
}