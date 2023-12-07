#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "core.h"
#include "processors.h"
#include "lang.h"
#include <intrin.h>

/**
 * AddressRange
 */

AddressRange::AddressRange(FunctionInfo *owner, uint64_t begin, uint64_t end, ICommand *begin_entry, ICommand *end_entry, ICommand *size_entry)
	: IObject(), owner_(owner), begin_(begin), end_(end), original_begin_(0), original_end_(0), begin_entry_(begin_entry), end_entry_(end_entry),
	size_entry_(size_entry), link_info_(NULL)
{

}

AddressRange::AddressRange(FunctionInfo *owner, const AddressRange &src)
	: IObject(), owner_(owner)
{
	begin_ = src.begin_;
	end_ = src.end_;
	begin_entry_ = src.begin_entry_;
	end_entry_ = src.end_entry_;
	size_entry_ = src.size_entry_;
	original_begin_ = src.original_begin_;
	original_end_ = src.original_end_;
	link_info_ = src.link_info_;
}

AddressRange::~AddressRange()
{
	if (owner_)
		owner_->RemoveObject(this);
}

AddressRange *AddressRange::Clone(FunctionInfo *owner) const
{
	AddressRange *range = new AddressRange(owner, *this);
	return range;
}

void AddressRange::Add(uint64_t address, size_t size)
{
 	if (!begin_ || begin_ > address)
		begin_ = address;

	if (!end_ || end_ < address + size)
		end_ = address + size;

	for (size_t i = 0; i < link_list_.size(); i++) {
		link_list_[i]->Add(address, size);
	}
}

void AddressRange::Prepare()
{
	original_begin_ = begin_;
	original_end_ = end_;
	begin_ = 0;
	end_ = 0;
}

void AddressRange::Rebase(uint64_t delta_base)
{
	if (begin_)
		begin_ += delta_base;
	if (end_)
		end_ += delta_base;
}

/**
 * FunctionInfo
 */

FunctionInfo::FunctionInfo()
	: ObjectList<AddressRange>(), owner_(NULL), begin_(0), end_(0), base_type_(btValue), base_value_(0), prolog_size_(0), 
	entry_(NULL), frame_registr_(0), source_(NULL), data_entry_(NULL)
{
}

FunctionInfo::FunctionInfo(FunctionInfoList *owner, uint64_t begin, uint64_t end, AddressBaseType base_type, uint64_t base_value, size_t prolog_size, 
	uint8_t frame_registr, IRuntimeFunction *source, ICommand *entry)
	: ObjectList<AddressRange>(), owner_(owner), begin_(begin), end_(end), base_type_(base_type), base_value_(base_value), prolog_size_(prolog_size), 
	entry_(entry), frame_registr_(frame_registr), source_(source), data_entry_(NULL)
{

}

FunctionInfo::FunctionInfo(FunctionInfoList *owner, const FunctionInfo &src)
	: ObjectList<AddressRange>(), owner_(owner)
{
	begin_ = src.begin_;
	end_ = src.end_;
	base_type_ = src.base_type_;
	base_value_ = src.base_value_;
	prolog_size_ = src.prolog_size_;
	source_ = src.source_;
	entry_ = src.entry_;
	data_entry_ = src.data_entry_;
	frame_registr_ = src.frame_registr_;
	unwind_opcodes_ = src.unwind_opcodes_;
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

FunctionInfo::~FunctionInfo()
{
	if (owner_)
		owner_->RemoveObject(this);
}

FunctionInfo *FunctionInfo::Clone(FunctionInfoList *owner) const
{
	FunctionInfo *info = new FunctionInfo(owner, *this);
	return info;
}

AddressRange *FunctionInfo::Add(uint64_t begin, uint64_t end, ICommand *begin_entry, ICommand *end_entry, ICommand *size_entry)
{
	AddressRange *range = new AddressRange(this, begin, end, begin_entry, end_entry, size_entry);
	AddObject(range);
	return range;
}

AddressRange *FunctionInfo::GetRangeByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		AddressRange *range = item(i);
		if (range->begin() <= address && range->end() > address)
			return range;
	}
	return NULL;
}

void FunctionInfo::Prepare()
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Prepare();
	}
}

void FunctionInfo::Compile()
{
	begin_ = 0;
	end_ = 0;
	for (size_t i = 0; i < count(); i++) {
		AddressRange *range = item(i);
		if (!range->begin())
			continue;
		if (!begin_ || begin_ > range->begin())
			begin_ = range->begin();
		if (!end_ || end_ < range->end())
			end_ = range->end();
	}
}

void FunctionInfo::WriteToFile(IArchitecture &file)
{
	if (begin_) {
		std::vector<uint8_t> call_frame_instructions;;
		for (size_t i = 0; i < unwind_opcodes_.size(); i++) {
			ICommand *command = unwind_opcodes_[i];
			for (size_t j = 0; j < command->dump_size(); j++) {
				call_frame_instructions.push_back(command->dump(j));
			}
		}
		file.runtime_function_list()->Add(0, begin_, end_, entry_ ? entry_->address() : 0, source_, call_frame_instructions);
	}
}

void FunctionInfo::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}

	if (begin_)
		begin_ += delta_base;
	if (end_)
		end_ += delta_base;
}

/**
 * FunctionInfoList
 */

FunctionInfoList::FunctionInfoList()
	: ObjectList<FunctionInfo>()
{

}

FunctionInfoList::FunctionInfoList(const FunctionInfoList &src)
	: ObjectList<FunctionInfo>()
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

FunctionInfoList *FunctionInfoList::Clone() const
{
	FunctionInfoList *list = new FunctionInfoList(*this);
	return list;
}

FunctionInfo *FunctionInfoList::GetItemByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		FunctionInfo *info = item(i);
		if (info->begin() <= address && info->end() > address)
			return info;
	}
	return NULL;
}

AddressRange *FunctionInfoList::GetRangeByAddress(uint64_t address) const
{
	FunctionInfo *info = GetItemByAddress(address);
	return info ? info->GetRangeByAddress(address) : NULL;
}

FunctionInfo *FunctionInfoList::Add(uint64_t begin, uint64_t end, AddressBaseType base_type, uint64_t base_value, size_t prolog_size, uint8_t frame_registr, IRuntimeFunction *source, ICommand *entry)
{
	FunctionInfo *info = new FunctionInfo(this, begin, end, base_type, base_value, prolog_size, frame_registr, source, entry);
	AddObject(info);
	return info;
};

void FunctionInfoList::Prepare()
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Prepare();
	}
}

void FunctionInfoList::Compile()
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Compile();
	}
}

void FunctionInfoList::WriteToFile(IArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

void FunctionInfoList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * BaseVMCommand
 */

BaseVMCommand::BaseVMCommand(ICommand *owner)
	: IVMCommand(), owner_(owner) 
{

}

BaseVMCommand::~BaseVMCommand()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
* InternalLink
*/

InternalLink::InternalLink(InternalLinkList *owner, InternalLinkType type, IVMCommand *from_command, IObject *to_command)
	: owner_(owner), type_(type), from_command_(from_command), to_command_(to_command)
{

}

InternalLink::~InternalLink()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
* InternalLinkList
*/

InternalLink *InternalLinkList::Add(InternalLinkType type, IVMCommand *from_command, IObject *to_command)
{
	InternalLink *link = new InternalLink(this, type, from_command, to_command);
	AddObject(link);
	return link;
}

/**
 * BaseCommand
 */

BaseCommand::BaseCommand(IFunction *owner)
	: ICommand(), owner_(owner), link_(NULL), block_(NULL), vm_address_(0), address_range_(NULL),
	alignment_(0), options_(roNeedCompile)
{

}

BaseCommand::BaseCommand(IFunction *owner, const std::string &value)
	: ICommand(), owner_(owner), link_(NULL), block_(NULL), vm_address_(0), address_range_(NULL),
	alignment_(0), options_(roNeedCompile)
{
	dump_.PushBuff(value.c_str(), value.size());
	dump_.PushByte(0);
}

BaseCommand::BaseCommand(IFunction *owner, const os::unicode_string &value)
	: ICommand(), owner_(owner), link_(NULL), block_(NULL), vm_address_(0), address_range_(NULL),
	alignment_(0), options_(roNeedCompile)
{
	dump_.PushBuff(value.c_str(), value.size() * sizeof(os::unicode_char));
	dump_.PushWord(0);
}

BaseCommand::BaseCommand(IFunction *owner, const Data &value)
	: ICommand(), owner_(owner), link_(NULL), block_(NULL), vm_address_(0), address_range_(NULL),
	alignment_(0), options_(roNeedCompile)
{
	dump_.PushBuff(value.data(), value.size());
}

BaseCommand::BaseCommand(IFunction *owner, const BaseCommand &src)
	: ICommand(), owner_(owner), link_(NULL), block_(NULL)
{
	dump_ = src.dump_;
	address_range_ = src.address_range_;
	comment_ = src.comment_;
	vm_address_ = src.vm_address_;
	alignment_ = src.alignment_;
	options_ = src.options_;
}

BaseCommand::~BaseCommand()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void BaseCommand::clear()
{
	dump_.clear();
}

void BaseCommand::Read(IArchitecture &file, size_t len)
{
	uint8_t *p = new uint8_t[len];
	file.Read(p, len);
	dump_.PushBuff(p, len);
	delete [] p;
}

uint8_t BaseCommand::ReadByte(IArchitecture &file)
{
	uint8_t res = file.ReadByte();
	PushByte(res);
	return res;
}

uint16_t BaseCommand::ReadWord(IArchitecture &file)
{
	uint16_t res = file.ReadWord();
	PushWord(res);
	return res;
}

uint32_t BaseCommand::ReadDWord(IArchitecture &file)
{
	uint32_t res = file.ReadDWord();
	PushDWord(res);
	return res;
}

uint64_t BaseCommand::ReadQWord(IArchitecture &file)
{
	uint64_t res = file.ReadQWord();
	PushQWord(res);
	return res;
}

void BaseCommand::PushByte(uint8_t value)
{
	dump_.PushByte(value);
}

void BaseCommand::PushWord(uint16_t value)
{
	dump_.PushWord(value);
}

void BaseCommand::PushDWord(uint32_t value)
{
	dump_.PushDWord(value);
}

void BaseCommand::PushQWord(uint64_t value)
{
	dump_.PushQWord(value);
}

void BaseCommand::InsertByte(size_t position, uint8_t value)
{
	dump_.InsertByte(position, value);
}

void BaseCommand::WriteDWord(size_t position, uint32_t value)
{
	dump_.WriteDWord(position, value);
}

void BaseCommand::CompileInfo()
{
	if (address_range_)
		address_range_->Add(address(), dump_size());
}

void BaseCommand::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	options_ = buffer.ReadDWord() & (roNeedCompile | roInverseFlag | roClearOriginalCode | roCreateNewBlock | roLockPrefix | roExternal | roBreaked | roVexPrefix);
	alignment_ = buffer.ReadByte();
}

void BaseCommand::WriteToFile(IArchitecture &file)
{
	file.Write(dump_.data(), dump_.size());
}

CommandLink *BaseCommand::AddLink(int operand_index, LinkType type, uint64_t to_address)
{
	return owner_->link_list()->Add(this, operand_index, type, to_address);
}

CommandLink *BaseCommand::AddLink(int operand_index, LinkType type, ICommand *to_command)
{
	return owner_->link_list()->Add(this, operand_index, type, to_command);
}

size_t BaseCommand::vm_dump_size() const
{
	size_t res = 0;
	for (size_t i = 0; i < count(); i++) {
		res += item(i)->dump_size();
	}

	return res;
}

void BaseCommand::set_vm_address(uint64_t address)
{
	vm_address_ = address;
	bool backward_direction = (section_options() & rtBackwardDirection) != 0;
	for (size_t i = 0; i < count(); i++) {
		IVMCommand *vm_command = item(i);
		vm_command->set_address(address);
		if (backward_direction) {
			address -= vm_command->dump_size();
		} else {
			address += vm_command->dump_size();
		}
	}
}

void BaseCommand::set_dump(const void *buffer, size_t size)
{
	dump_.clear();
	dump_.PushBuff(buffer, size);
}

bool BaseCommand::CompareDump(const uint8_t *buffer, size_t size) const
{
	if (dump_.size() != size)
		return false;

	for (size_t i = 0; i < size; i++) {
		if (dump_[i] != buffer[i])
			return false;
	}
	return true;
}

std::string BaseCommand::dump_str() const
{
	std::string res;

	for (size_t i = 0; i < dump_size(); i++) {
		res += string_format("%.2X", dump(i));
	}

	return res;
}

uint64_t BaseCommand::dump_value(size_t pos, OperandSize size) const
{
	if (size > osQWord)
		throw std::runtime_error("Invalid value size");

	uint64_t res = 0;
	memcpy(&res, &dump_[pos], OperandSizeToValue(size));
	return res;
}

/**
 * ValueCommand
 */

ValueCommand::ValueCommand(ValueCryptor *owner, OperandSize size, CryptCommandType type, uint64_t value)
	: IObject(), owner_(owner), type_(type), size_(size)
{
	switch (size_) {
	case osByte:
		value_ = static_cast<uint8_t>(value);
		break;
	case osWord:
		value_ = static_cast<uint16_t>(value);
		break;
	case osDWord:
		value_ = static_cast<uint32_t>(value);
		break;
	default:
		value_ = value;
	}
}

ValueCommand::ValueCommand(ValueCryptor *owner, const ValueCommand &src)
	: IObject(), owner_(owner)
{
	size_ = src.size_;
	type_ = src.type_;
	value_ = src.value_;
}

ValueCommand *ValueCommand::Clone(ValueCryptor *owner) const
{
	ValueCommand *command = new ValueCommand(owner, *this);
	return command;
}

ValueCommand::~ValueCommand()
{
	if (owner_)
		owner_->RemoveObject(this);
}

CryptCommandType ValueCommand::type(bool is_decrypt) const
{
	CryptCommandType command = type_;
	if (is_decrypt) {
		switch (command) {
		case ccAdd:
			command = ccSub;
			break;
		case ccSub:
			command = ccAdd;
			break;
		case ccInc:
			command = ccDec;
			break;
		case ccDec:
			command = ccInc;
			break;
		case ccRol:
			command = ccRor;
			break;
		case ccRor:
			command = ccRol;
			break;
		}
	}
	return command;
}

uint64_t ValueCommand::Encrypt(uint64_t value)
{
	return Calc(value, false);
}

uint64_t ValueCommand::Decrypt(uint64_t value)
{
	return Calc(value, true);
}

uint64_t ValueCommand::Calc(uint64_t value, bool is_decrypt)
{
	switch (type(is_decrypt)) {
	case ccAdd: case ccInc:
		value += value_;
		break;
	case ccSub: case ccDec:
		value -= value_;
		break;
	case ccXor:
		value ^= value_;
		break;
	case ccNot:
		value = ~value;
		break;
	case ccNeg:
		value = 0 - value;
		break;
	case ccBswap:
		switch (size_) {
		case osWord:
			value = __builtin_bswap16(static_cast<uint16_t>(value));
			break;
		case osDWord:
			value = __builtin_bswap32(static_cast<uint32_t>(value));
			break;
		case osQWord:
			value = __builtin_bswap64(value);
			break;
		}
		break;
	case ccRol:
		switch (size_) {
		case osByte:
			value = _rotl8(static_cast<uint8_t>(value), static_cast<int>(value_));
			break;
		case osWord:
			value = _rotl16(static_cast<uint16_t>(value), static_cast<int>(value_));
			break;
		case osDWord:
			value = _rotl32(static_cast<uint32_t>(value), static_cast<int>(value_));
			break;
		case osQWord:
			value = _rotl64(value, static_cast<int>(value_));
			break;
		}
		break;
	case ccRor:
		switch (size_) {
		case osByte:
			value = _rotr8(static_cast<uint8_t>(value), static_cast<int>(value_));
			break;
		case osWord:
			value = _rotr16(static_cast<uint16_t>(value), static_cast<int>(value_));
			break;
		case osDWord:
			value = _rotr32(static_cast<uint32_t>(value), static_cast<int>(value_));
			break;
		case osQWord:
			value = _rotr64(value, static_cast<int>(value_));
			break;
		}
		break;
	}

	return value;
}

/**
 * ValueCryptor
 */

ValueCryptor::ValueCryptor()
	: ObjectList<ValueCommand>(), size_(osByte)
{

}

ValueCryptor::ValueCryptor(const ValueCryptor &src)
	: ObjectList<ValueCommand>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
	size_ = src.size_;
}

ValueCryptor *ValueCryptor::Clone() const
{
	ValueCryptor *cryptor = new ValueCryptor(*this);
	return cryptor;
}

uint64_t ValueCryptor::Encrypt(uint64_t value)
{
	size_t i;
	for (i = 0; i < count(); i++) {
		value = item(i)->Encrypt(value);
	}
	return value;
}

uint64_t ValueCryptor::Decrypt(uint64_t value)
{
	size_t i;
	for (i = count(); i > 0; i--) {
		value = item(i - 1)->Decrypt(value);
	}
	return value;
}

void ValueCryptor::Init(OperandSize size)
{
	clear();

	size_ = size;
	CryptCommandType last_command = ccUnknown;
	for (;;) {
		CryptCommandType command = static_cast<CryptCommandType>(rand() % ccUnknown);
		if (command == last_command)
			continue;

		uint64_t value = 0;
		switch (command) {
		case ccAdd: case ccSub: 
			if (last_command == ccAdd || last_command == ccSub || last_command == ccInc || last_command == ccDec)
				continue;
			value = DWordToInt64(rand32());
			break;
		case ccInc: case ccDec:
			if (last_command == ccAdd || last_command == ccSub || last_command == ccInc || last_command == ccDec)
				continue;
			value = 1;
			break;
		case ccXor:
			value = DWordToInt64(rand32());
			break;
		case ccBswap:
			if (size_ == osByte || size_ == osWord)
				continue;
			break;
		case ccRol: case ccRor:
			if (last_command == ccRol || last_command == ccRor)
				continue;
			
			value = rand() % BYTES_TO_BITS(OperandSizeToValue(size_));
			if (!value)
				value = 1;
			break;
		}
		last_command = command;

		Add(command, value);

		size_t c = count();
		if (c > 100 || (c > 3 && (rand() & 1)))
			break;
	}
}

void ValueCryptor::Add(CryptCommandType command, uint64_t value)
{
	AddObject(new ValueCommand(this, size_, command, value));
}

/**
 * OpcodeCryptor
 */

OpcodeCryptor::OpcodeCryptor()
	: ValueCryptor(), type_(ccUnknown)
{

}

void OpcodeCryptor::Init(OperandSize size)
{
	//static CryptCommandType opcode_commands[] = {ccAdd, ccSub, ccXor};
	//type_ = opcode_commands[rand() % _countof(opcode_commands)];
	type_ = ccXor;
	ValueCryptor::Init(size);
}

uint64_t OpcodeCryptor::EncryptOpcode(uint64_t value1, uint64_t value2)
{
	return Calc(value1, value2, false);
}

uint64_t OpcodeCryptor::DecryptOpcode(uint64_t value1, uint64_t value2)
{
	return Calc(value1, value2, true);
}

uint64_t OpcodeCryptor::Calc(uint64_t value1, uint64_t value2, bool is_decrypt)
{
	CryptCommandType command = type_;
	if (is_decrypt) {
		switch (command) {
		case ccAdd:
			command = ccSub;
			break;
		case ccSub:
			command = ccAdd;
			break;
		}
	}

	switch (command) {
	case ccAdd:
		return value1 + value2;
	case ccSub:
		return value1 - value2;
	case ccXor:
		return value1 ^ value2;
	}

	return 0;
}

/**
 * CommandLink
 */

CommandLink::CommandLink(CommandLinkList *owner, ICommand *from_command, int operand_index, LinkType type, uint64_t to_address)
	: IObject(), owner_(owner), parsed_(false), from_command_(from_command), parent_command_(NULL), to_command_(NULL), next_command_(NULL), 
	type_(type), to_address_(to_address), operand_index_(operand_index), sub_value_(0), cryptor_(NULL), base_function_info_(NULL), is_inverse_(false)
{
	if (from_command_)
		from_command_->set_link(this);
}

CommandLink::CommandLink(CommandLinkList *owner, ICommand *from_command, int operand_index, LinkType type, ICommand *to_command)
	: IObject(), owner_(owner), parsed_(false), from_command_(from_command), parent_command_(NULL), to_command_(to_command), next_command_(NULL), 
	type_(type), to_address_(0), operand_index_(operand_index), sub_value_(0), cryptor_(NULL), base_function_info_(NULL), is_inverse_(false)
{
	if (from_command_)
		from_command_->set_link(this);
}

CommandLink::CommandLink(CommandLinkList *owner, const CommandLink &src)
	: IObject(src), owner_(owner), from_command_(NULL), parent_command_(NULL), to_command_(NULL), next_command_(NULL)
{
	parsed_ = src.parsed_;
	type_ = src.type_;
	to_address_ = src.to_address_;
	operand_index_ = src.operand_index_;
	sub_value_ = src.sub_value_;
	cryptor_ = src.cryptor_;
	base_function_info_ = src.base_function_info_;
	is_inverse_ = src.is_inverse_;
}

CommandLink::~CommandLink() 
{
	if (from_command_)
		from_command_->set_link(NULL);
	if (owner_)
		owner_->RemoveObject(this);
	delete cryptor_;
}

CommandLink *CommandLink::Clone(CommandLinkList *owner) const
{
	CommandLink *link = new CommandLink(owner, *this);
	return link;
}

void CommandLink::set_from_command(ICommand *command) 
{
	if (from_command_ == command)
		return;
	if (from_command_)
		from_command_->set_link(NULL);
	from_command_ = command;
	if (from_command_)
		from_command_->set_link(this);
}

void CommandLink::Rebase(uint64_t delta_base)
{
	if (sub_value_)
		sub_value_ += delta_base;

	if (to_address_)
		to_address_ += delta_base;
}

void CommandLink::set_cryptor(ValueCryptor *cryptor)
{
	if (cryptor) {
		cryptor_ = cryptor->Clone();
	} else {
		delete cryptor_;
		cryptor_ = NULL;
	}
}

uint64_t CommandLink::Encrypt(uint64_t value) const
{
	uint64_t sub_value = base_function_info_ ? base_function_info_->begin() + base_function_info_->base_value() : sub_value_;
	if (is_inverse_)
		value = sub_value - value;
	else 
		value = value - sub_value;
	if (cryptor_)
		value = cryptor_->Encrypt(value);
	return value;
}

/**
 * CommandLinkList
 */

CommandLinkList::CommandLinkList()
	: ObjectList<CommandLink>()
{
	
}

CommandLinkList::CommandLinkList(const CommandLinkList &src)
	: ObjectList<CommandLink>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

CommandLinkList *CommandLinkList::Clone() const
{
	CommandLinkList *list = new CommandLinkList(*this);
	return list;
}

CommandLink *CommandLinkList::Add(ICommand *from_command, int operand_index, LinkType type, uint64_t to_address)
{
	CommandLink *link = new CommandLink(this, from_command, operand_index, type, to_address);
	AddObject(link);
	return link;
};

CommandLink *CommandLinkList::Add(ICommand *from_command, int operand_index, LinkType type, ICommand *to_command)
{
	CommandLink *link = new CommandLink(this, from_command, operand_index, type, to_command);
	AddObject(link);
	return link;
};

CommandLink *CommandLinkList::GetLinkByToAddress(LinkType type, uint64_t to_address)
{
	for (size_t i = 0; i < count(); i++) {
		CommandLink *link = item(i);
		if (link->to_address() == to_address && (type == ltNone || link->type() == type))
			return link;
	}

	return NULL;
}

void CommandLinkList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * ExtCommand
 */

ExtCommand::ExtCommand(ExtCommandList *owner, uint64_t address, ICommand *command, bool use_call)
	: IObject(), owner_(owner), address_(address), command_(command), use_call_(use_call)
{

}

ExtCommand::ExtCommand(ExtCommandList *owner, const ExtCommand &src)
	: IObject(src), owner_(owner)
{
	address_ = src.address_;
	command_ = src.command_;
	use_call_ = src.use_call_;
}

ExtCommand::~ExtCommand()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ExtCommand *ExtCommand::Clone(ExtCommandList *owner) const
{
	ExtCommand *ext_command = new ExtCommand(owner, *this);
	return ext_command;
}

int ExtCommand::CompareWith(const ExtCommand &obj) const
{
	if (address() < obj.address())
		return -1;
	if (address() > obj.address())
		return 1;
	return 0;
}

/**
 * ExtCommandList
 */

ExtCommandList::ExtCommandList(IFunction *owner)
	: ObjectList<ExtCommand>(), owner_(owner)
{
	
}

ExtCommandList::ExtCommandList(IFunction *owner, const ExtCommandList &src)
	: ObjectList<ExtCommand>(src), owner_(owner)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ExtCommandList *ExtCommandList::Clone(IFunction *owner) const
{
	ExtCommandList *list = new ExtCommandList(owner, *this);
	return list;
}

ExtCommand *ExtCommandList::GetCommandByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		ExtCommand *ext_command = item(i);
		if (ext_command->address() == address)
			return ext_command;
	}

	return NULL;
}

ExtCommand *ExtCommandList::Add(uint64_t address, ICommand *command, bool use_call)
{
	ExtCommand *ext_command = new ExtCommand(this, address, command, use_call);
	AddObject(ext_command);
	return ext_command;
}

ExtCommand *ExtCommandList::Add(uint64_t address)
{
	ExtCommand *ext_command = GetCommandByAddress(address);
	if (ext_command)
		return ext_command;

	if (owner_->address() == address || owner_->type() == otString)
		return NULL;

	ICommand *command = owner_->GetCommandByAddress(address);
	if (!command)
		return NULL;

	return Add(address, command);
}

void ExtCommandList::AddObject(ExtCommand *ext_command)
{ 
	ObjectList<ExtCommand>::AddObject(ext_command);
	if (owner_)
		owner_->Notify(mtAdded, ext_command);
}

void ExtCommandList::RemoveObject(ExtCommand *ext_command) 
{ 
	ObjectList<ExtCommand>::RemoveObject(ext_command);
	if (owner_)
		owner_->Notify(mtDeleted, ext_command);
}

/**
 * CommandBlock
 */

CommandBlock::CommandBlock(CommandBlockList *owner, uint32_t type, size_t start_index)
	: AddressableObject(), owner_(owner), type_(type), start_index_(start_index), end_index_(start_index),
	virtual_machine_(NULL), sort_index_(0)
{
	registr_count_ = (function()->cpu_address_size() == osDWord) ? 16 : 24;
	memset(registr_indexes_, 0xff, sizeof(registr_indexes_));
}

CommandBlock::CommandBlock(CommandBlockList *owner, const CommandBlock &src)
	: AddressableObject(src), owner_(owner), virtual_machine_(NULL), sort_index_(0)
{
	type_ = src.type_;
	start_index_ = src.start_index_;
	end_index_ = src.end_index_;
	registr_count_ = src.registr_count_;
}

CommandBlock::~CommandBlock()
{
	if (owner_)
		owner_->RemoveObject(this);
}

CommandBlock *CommandBlock::Clone(CommandBlockList *owner)
{
	CommandBlock *block = new CommandBlock(owner, *this);
	return block;
}

IFunction *CommandBlock::function() const
{
	return owner_->owner();
}

void CommandBlock::Compile(MemoryManager &manager)
{
	size_t i, memory_size, alignment;
	ICommand *command;
	uint64_t address;
	IFunction *func = function();

	memory_size = 0;
	if (type_ & mtExecutable) {
		alignment = func->item(start_index_)->alignment();
		for (i = start_index_; i <= end_index_; i++) {
			command = func->item(i);
			memory_size += command->dump_size();
		}
	} else {
		alignment = 0;
#ifndef DEMO
		ICommand *stor_command = NULL;
		for (i = start_index_; i <= end_index_; i++) {
			command = func->item(i);
			if (command->is_data()) {
				stor_command = NULL;
			} else if (!stor_command || (command->section_options() & rtBeginSection)) {
				stor_command = command;
			} else if (command->section_options() & rtEndSection) {
				stor_command->Merge(command);
				stor_command = NULL;
			} else if (!command->Merge(stor_command)) {
				stor_command->Merge(command);
				stor_command = command;
			}
		}
#endif
		for (i = start_index_; i <= end_index_; i++) {
			command = func->item(i);
			for (size_t j = 0; j < command->count(); j++) {
				command->item(j)->Compile();
			}
			memory_size += command->vm_dump_size();
		}
	}

	if (memory_size) {
		address = (address_) ? address_ : manager.Alloc(memory_size, type_, 0, alignment);
		if (type_ & mtExecutable) {
			// native block
			for (i = start_index_; i <= end_index_; i++) {
				command = func->item(i);
				command->set_address(address);
				address += command->dump_size();
			}
		} else {
			// VM block
			bool backward_direction = (func->item(start_index_)->section_options() & rtBackwardDirection) != 0;
			if (backward_direction)
				address += memory_size;
			for (i = start_index_; i <= end_index_; i++) {
				command = func->item(i);
				command->set_vm_address(address);
				if (backward_direction) {
					address -= command->vm_dump_size();
				} else {
					address += command->vm_dump_size();
				}
			}
		}
	}
}

void CommandBlock::CompileInfo()
{
	IFunction *func = function();
	for (size_t i = start_index_; i <= end_index_; i++) {
		ICommand *command = func->item(i);
		command->CompileInfo();
	}
}

void CommandBlock::CompileLinks(const CompileContext &ctx)
{
	IFunction *func = function();
	for (size_t i = start_index_; i <= end_index_; i++) {
		ICommand *command = func->item(i);
		command->CompileLink(ctx);
	}
}

size_t CommandBlock::WriteToFile(IArchitecture &file)
{
	size_t i, j;
	ICommand *command;
	IVMCommand *vm_command;
	uint32_t update_type;

	IFunction *func = function();

	update_type = type_;
	if (func->memory_type() != mtNone && (update_type & mtDiscardable) == 0)
		update_type |= mtNotDiscardable;

	if (type_ & mtExecutable) {
		// native block
		for (i = start_index_; i <= end_index_; i++) {
			file.StepProgress();
			command = func->item(i);
			if (!file.AddressSeek(command->address()))
				throw std::runtime_error("Invalid command address");

			file.selected_segment()->include_write_type(update_type);
			command->WriteToFile(file);
		}
	} else {
		// VM block
		if (func->item(start_index_)->section_options() & rtBackwardDirection) {
			for (i = end_index_ + 1; i > start_index_ ; i--) {
				file.StepProgress();
				command = func->item(i - 1);
				if (!file.AddressSeek(command->vm_address() - command->vm_dump_size()))
					throw std::runtime_error("Invalid command address");

				for (j = command->count(); j > 0; j--) {
					vm_command = command->item(j - 1);
					file.selected_segment()->include_write_type(update_type);
					vm_command->WriteToFile(file);
				}
			}
		} else {
			for (i = start_index_; i <= end_index_; i++) {
				file.StepProgress();
				command = func->item(i);
				if (!file.AddressSeek(command->vm_address()))
					throw std::runtime_error("Invalid command address");

				for (j = 0; j < command->count(); j++) {
					vm_command = command->item(j);
					file.selected_segment()->include_write_type(update_type);
					vm_command->WriteToFile(file);
				}
			}
		}
	}

	return end_index_ - start_index_ + 1;
}

uint8_t CommandBlock::GetRegistr(OperandSize size, uint8_t registr, bool is_write)
{
	uint8_t res;
	OperandSize cpu_address_size = function()->cpu_address_size();
	if (registr & regExtended) {
		res = (uint8_t)(registr_count_ + (registr & 0xf));
	} else if (registr == regEmpty && !is_write) {
		res = (uint8_t)(rand() % registr_count_);
	} else {
		if(registr >= _countof(registr_indexes_)) 
			throw std::runtime_error("Runtime error at GetRegistr");

		res = registr_indexes_[registr];
		if (res == 0xff || (is_write && size == cpu_address_size)) {
			uint8_t empty_registr[_countof(registr_indexes_)];
			size_t empty_registr_count = 0;
			for (size_t i = 0; i < registr_count_; i++) {
				bool is_found = false;
				for (size_t j = 0; j < regEmpty; j++) {
					if (registr_indexes_[j] == i) {
						is_found = true;
						break;
					}
				}
				if (!is_found) {
					empty_registr[empty_registr_count] = (uint8_t)i;
					empty_registr_count++;
				}
			}

			if (empty_registr_count) {
				res = empty_registr[rand() % empty_registr_count];
				if (registr != regEmpty)
					registr_indexes_[registr] = res;
			} else if (res == 0xff)
				throw std::runtime_error("Runtime error at GetRegistr");
		}
	}

	return (uint8_t)(res * OperandSizeToValue(cpu_address_size));
}

/**
 * CommandBlockList
 */

CommandBlockList::CommandBlockList(IFunction *owner)
	: ObjectList<CommandBlock>(), owner_(owner)
{

}

CommandBlockList::CommandBlockList(IFunction *owner, const CommandBlockList &src)
	: ObjectList<CommandBlock>(src), owner_(owner)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

CommandBlockList *CommandBlockList::Clone(IFunction *owner) const
{
	CommandBlockList *list = new CommandBlockList(owner, *this);
	return list;
}

CommandBlock *CommandBlockList::Add(uint32_t memory_type, size_t start_index)
{
	CommandBlock *block = new CommandBlock(this, memory_type, start_index);
	AddObject(block);
	return block;
}

void CommandBlockList::CompileBlocks(MemoryManager &manager)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Compile(manager);
	}
}

void CommandBlockList::CompileInfo()
{
	for (size_t i = 0; i < count(); i++) {
		CommandBlock *block = item(i);
		if (block->type() & mtExecutable)
			block->CompileInfo();
	}
}

void CommandBlockList::CompileLinks(const CompileContext &ctx)
{
	size_t i;
	CommandBlock *block;

	for (i = 0; i < count(); i++) {
		block = item(i);
		if ((block->type() & mtExecutable) == 0)
			continue;

		block->CompileLinks(ctx);
	}

	for (i = 0; i < count(); i++) {
		block = item(i);
		if ((block->type() & mtExecutable) != 0)
			continue;

		block->CompileLinks(ctx);
	}
}

size_t CommandBlockList::WriteToFile(IArchitecture &file)
{
	size_t res = 0;
	for (size_t i = 0; i < count(); i++) {
		CommandBlock *block = item(i);
		res += block->WriteToFile(file);
	}
	return res;
}

/**
 * BaseFunction
 */

BaseFunction::BaseFunction(IFunctionList *owner, const FunctionName &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder)
	: IFunction(), owner_(owner), name_(name), address_(0), break_address_(0), type_(otUnknown), cpu_address_size_(osDefault), compilation_type_(compilation_type), 
	compilation_options_(compilation_options), internal_lock_to_key_(false), default_compilation_type_(ctNone), need_compile_(need_compile), folder_(folder), memory_type_(mtNone), 
	tag_(0), entry_(NULL), entry_type_(etDefault), from_runtime_(false), parent_(NULL)
{
	link_list_ = new CommandLinkList();
	ext_command_list_ = new ExtCommandList(this);
	block_list_ = new CommandBlockList(this);
	function_info_list_ = new FunctionInfoList();
	range_list_ = new FunctionInfo();
}

BaseFunction::BaseFunction(IFunctionList *owner, OperandSize cpu_address_size, IFunction *parent)
	: IFunction(), owner_(owner), address_(0), break_address_(0), type_(otCode), cpu_address_size_(cpu_address_size), compilation_type_(ctVirtualization), 
	compilation_options_(0), internal_lock_to_key_(false), default_compilation_type_(ctNone), need_compile_(true), folder_(NULL), memory_type_(mtReadable), tag_(0), entry_(NULL), 
	entry_type_(etDefault),	from_runtime_(false), parent_(parent)
{
	link_list_ = new CommandLinkList();
	ext_command_list_ = new ExtCommandList(this);
	block_list_ = new CommandBlockList(this);
	function_info_list_ = new FunctionInfoList();
	range_list_ = new FunctionInfo();
}

BaseFunction::BaseFunction(IFunctionList *owner, const BaseFunction &src)
	: IFunction(src), owner_(owner), folder_(NULL), parent_(NULL)
{
	size_t i, j;
	
	address_ = src.address_;
	type_ = src.type_;
	entry_ = NULL;
	entry_type_ = src.entry_type_;
	break_address_ = src.break_address_;
	name_ = src.name_;
	cpu_address_size_ = src.cpu_address_size_;
	need_compile_ = src.need_compile_;

	compilation_type_ =src.compilation_type_;
	compilation_options_ = src.compilation_options_;
	internal_lock_to_key_ = src.internal_lock_to_key_;
	default_compilation_type_ = src.default_compilation_type_;
	memory_type_ = src.memory_type_;
	tag_ = src.tag_;
	from_runtime_ = src.from_runtime_;

	link_list_ = src.link_list_->Clone();
	ext_command_list_ = src.ext_command_list_->Clone(this);
	block_list_ = src.block_list_->Clone(this);
	function_info_list_ = src.function_info_list_->Clone();
	range_list_ = src.range_list_->Clone(NULL);

	for (i = 0; i < src.count(); i++) {
		ICommand *command = src.item(i)->Clone(this);
		AddObject(command);

		AddressRange *address_range = command->address_range();
		if (address_range) {
			FunctionInfo *info = function_info_list_->item(src.function_info_list()->IndexOf(address_range->owner()));
			command->set_address_range(info->item(address_range->owner()->IndexOf(address_range)));
		}
	}

	for (i = 0; i < range_list_->count(); i++) {
		AddressRange *range = range_list_->item(i);
		if (range->begin_entry())
			range->set_begin_entry(item(src.IndexOf(range->begin_entry())));
		if (range->end_entry())
			range->set_end_entry(item(src.IndexOf(range->end_entry())));
		if (range->size_entry())
			range->set_size_entry(item(src.IndexOf(range->size_entry())));
	}

	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		if (info->entry())
			info->set_entry(item(src.IndexOf(info->entry())));
		if (info->data_entry())
			info->set_data_entry(item(src.IndexOf(info->data_entry())));
		std::vector<ICommand *> unwind_opcodes = *info->unwind_opcodes();
		for (j = 0; j < unwind_opcodes.size(); j++) {
			unwind_opcodes[j] = item(src.IndexOf(unwind_opcodes[j]));
		}
		info->set_unwind_opcodes(unwind_opcodes);
	}

	if (src.entry_)
		entry_ = item(src.IndexOf(src.entry_));
	
	for (i = 0; i < src.count(); i++) {
		CommandLink *src_link = src.item(i)->link();
		if (!src_link)
			continue;

		CommandLink *link = link_list_->item(src.link_list()->IndexOf(src_link));
		link->set_from_command(item(i));
		if (src_link->parent_command())
			link->set_parent_command(item(src.IndexOf(src_link->parent_command())));
		if (src_link->base_function_info())
			link->set_base_function_info(function_info_list()->item(src.function_info_list()->IndexOf(src_link->base_function_info())));
	}

	for (i = 0; i < src.ext_command_list()->count(); i++) {
		ExtCommand *ext_command = src.ext_command_list()->item(i);
		if (!ext_command->command())
			continue;

		ext_command_list_->item(i)->set_command(item(src.IndexOf(ext_command->command())));
	}
}

BaseFunction::~BaseFunction() 
{
	if (owner_)
		owner_->RemoveObject(this);

	delete link_list_;
	delete ext_command_list_;
	delete block_list_;
	delete function_info_list_;
	delete range_list_;
}

void BaseFunction::AddObject(ICommand *command)
{
	ObjectList<ICommand>::AddObject(command);
	if (command->address())
		map_[command->address()] = command;
}

void BaseFunction::RemoveObject(ICommand *command)
{
	for (map_command_list_t::iterator it = map_.begin(); it != map_.end(); it++) {
		if (it->second == command) {
			map_.erase(it);
			break;
		}
	}
	IFunction::RemoveObject(command);
}

ICommand *BaseFunction::GetCommandByLowerAddress(uint64_t address) const
{
	if (map_.empty())
		return NULL;

	map_command_list_t::const_iterator it = map_.upper_bound(address);
	if (it != map_.begin())
		it--;

	return it->first > address ? NULL : it->second;
}

ICommand *BaseFunction::GetCommandByUpperAddress(uint64_t address) const
{
	if (map_.empty())
		return NULL;

	map_command_list_t::const_iterator it = map_.upper_bound(address);
	if (it == map_.end())
		return NULL;

	return it->second;
}

ICommand *BaseFunction::GetCommandByNearAddress(uint64_t address) const
{
	ICommand *command = GetCommandByLowerAddress(address);
	if (command && command->address() <= address && command->address() + command->original_dump_size() > address)
		return command;

	return NULL;
}

ICommand *BaseFunction::GetCommandByAddress(uint64_t address) const
{
	ICommand *command = GetCommandByLowerAddress(address);
	if (command && command->address() == address)
		return command;

	return NULL;
}

uint64_t BaseFunction::GetNextAddress(IArchitecture &file)
{
	size_t i, j;
	uint64_t max_address;
	CommandLink *link;
	LinkType link_type;
	uint64_t to_address;

	for (i = 0; i < link_list_->count(); i++) {
		link = link_list_->item(i);
		link_type = link->type();
		if (link->parsed() || link_type == ltCall)
			continue;

		to_address = link->to_address();
		if (link_type == ltNone || link_type == ltOffset || link_type == ltDelta || GetCommandByNearAddress(to_address)) {
			link->set_parsed(true);
		} else if (to_address < address_) {
			link->set_parsed(true);
			if (parent() && (link_type == ltJmp || link_type == ltJmpWithFlag)) {
				IFunction *func = parent();
				while (func) {
					if (to_address > func->address()) {
						address_ = to_address;
						return to_address;
					}
					func = func->parent();
				}
			}
		} else if (link_type == ltJmp) {
			if (file.runtime_function_list()) {
				IRuntimeFunction *runtime_function = file.runtime_function_list()->GetFunctionByAddress(link->from_command()->address());
				if (runtime_function && to_address >= runtime_function->begin() && to_address < runtime_function->end()) {
					link->set_parsed(true);
					return to_address;
				}
			}
		} else {
			link->set_parsed(true);
			return to_address;
		}
	}

	max_address = address_;
	for (i = 0; i < link_list_->count(); i++) {
		link = link_list_->item(i);

		switch (link->type()) {
		case ltSEHBlock:
		case ltFinallyBlock:
		case ltDualSEHBlock:
		case ltFilterSEHBlock:
		case ltJmpWithFlag:
			if (link->to_address() > max_address)
				max_address = link->to_address();
			break;
		}
	}

	for (i = 0; i < link_list_->count(); i++) {
		link = link_list_->item(i);
		if (link->parsed())
			continue;

		to_address = link->to_address();
		if (link->type() == ltJmp && to_address < max_address) {
			link->set_parsed(true);
			return to_address;
		}
	}

	for (i = 0; i < link_list_->count(); i++) {
		link = link_list_->item(i);
		if (link->parsed() || link->type() != ltJmp)
			continue;

		// backward/forward jump
		bool res = false;
		to_address = link->to_address();
		if (parent()) {
			res = true;
		} else {
			bool is_forward_aligned = to_address > link->from_command()->address() && (to_address & 0x0f) == 0;
			IFunction *temp_func = CreateFunction(this);
			temp_func->ReadFromFile(file, to_address);
			for (j = 0; j < temp_func->count(); j++) {
				ICommand *command = temp_func->item(j);
				CommandLink *temp_link = command->link();
				if (temp_link && (temp_link->type() == ltJmp || temp_link->type() == ltJmpWithFlag)) {
					if (GetCommandByAddress(temp_link->to_address()) || (is_forward_aligned && temp_link->to_address() == to_address) 
						|| (temp_link->type() == ltJmpWithFlag && temp_link->to_address() == link->from_command()->next_address())) {
						res = true;
						break;
					}
				}
				if (command->is_data() || command->is_end() || (command->options() & roBreaked))
					continue;

				if (command->next_address() == to_address) {
					res = true;
					break;
				}
			}
			delete temp_func;
		}

		if (res) {
			link->set_parsed(true);
			return to_address;
		}
	}

	return 0;
}

void BaseFunction::clear()
{
	address_ = 0;
	break_address_ = 0;
	type_ = otUnknown;
	entry_ = NULL;
	entry_type_ = etDefault;
	name_.clear();
	internal_lock_to_key_ = false;
	default_compilation_type_ = ctNone;

	ClearItems();
}

void BaseFunction::ClearItems()
{
	map_.clear();
	link_list_->clear();
	ext_command_list_->clear();
	block_list_->clear();
	function_info_list_->clear();
	range_list_->clear();
	IFunction::clear();
}

size_t BaseFunction::ReadFromFile(IArchitecture &file, uint64_t address)
{
	MapFunction *map_function;
	ICommand *command;
	ISectionList *segment_list;
	uint64_t def_parsed_address, parsed_address;
	size_t i;
	Reference *ref;
	ReferenceList *reference_list;

	clear();

	address_ = address;
	cpu_address_size_ = file.cpu_address_size();
	memory_type_ = file.segment_list()->GetMemoryTypeByAddress(address);

	map_function = file.map_function_list()->GetFunctionByAddress(address);
	if (map_function) {
		default_compilation_type_ = map_function->compilation_type();
		internal_lock_to_key_ = map_function->lock_to_key();
		name_ = map_function->full_name();
		type_ = map_function->type();
	} else {
		name_.clear();
		type_ = otCode;
	}

	ParseBeginCommands(file);

	if (type_ == otString) {
		if (map_function) {
			ParseString(file, address_, static_cast<size_t>(map_function->end_address() - address_));
			reference_list = map_function->equal_address_list();
			for (i = 0; i < reference_list->count(); i++) {
				ref = reference_list->item(i);
				ParseString(file, ref->address(), static_cast<size_t>(ref->operand_address() - ref->address()));
			}
		}
	} else {
		segment_list = file.segment_list();
		def_parsed_address = -1;
		parsed_address = def_parsed_address;
		IRuntimeFunctionList *runtime_function_list = file.runtime_function_list();
		IRuntimeFunction *runtime_function = NULL;

		for (;;) {
			command = NULL;
			if (address < parsed_address && (segment_list->GetMemoryTypeByAddress(address) & mtExecutable)) {
				if (runtime_function_list) {
					if (runtime_function && (address < runtime_function->begin() || address >= runtime_function->end()))
						runtime_function = NULL;
					if (!runtime_function)
						runtime_function = runtime_function_list->GetFunctionByAddress(address);
					if (runtime_function)
						runtime_function->Parse(file, *this);
				}
				command = ParseCommand(file, address);
			}

			if (!command || command->is_end() || (command->options() & roBreaked) != 0) {
				address = GetNextAddress(file);
				if (!address)
					break;
				command = GetCommandByUpperAddress(address);
				parsed_address = command ? command->address() : def_parsed_address;
			} else {
				address = command->next_address();
			}
		}
	}

	ParseEndCommands(file);

	Sort();

	if (type_ != otString)
		entry_ = GetCommandByAddress(address_);

	return count();
}

bool BaseFunction::FreeByManager(const CompileContext &ctx)
{
	MemoryManager *manager = ctx.manager;
	uint64_t block_address = 0;
	size_t block_size = 0;
	uint32_t block_memory_type = mtNone;
	ISectionList *segment_list = (from_runtime() ? ctx.runtime : ctx.file)->segment_list();

	for (size_t i = 0; i < count(); i++) {
		ICommand *command = item(i);
		if (command->address() && (command->options() & roClearOriginalCode) && !is_breaked_address(command->address())) {
			for (size_t j = 0; j < command->original_dump_size(); j++) {
				MemoryRegion *region = manager->GetRegionByAddress(command->address() + j);
				if (region) {
					IFunction *func = region->parent_function();
					uint64_t func_address;
					std::string func_name;
					if (func) {
						func_name = func->name();
						func_address = func->address();
					} else {
						func_name.clear();
						func_address = region->address();
					}
					if (func_name.empty())
						func_name = string_format("%.8llX", func_address);
					ctx.file->Notify(mtError, command, string_format(language[lsAddressUsedByFunction].c_str(), func_name.c_str()));
					return false;
				}
			}

			uint32_t command_memory_type = segment_list->GetMemoryTypeByAddress(command->address());
			if (block_address && ((block_address + block_size) != command->address() || block_memory_type != command_memory_type)) {
				if (block_size)
					manager->Add(block_address, block_size, block_memory_type, this);
				block_address = 0;
				block_size = 0;
				block_memory_type = mtNone;
			}
			if (!block_address) {
				block_address = command->address();
				block_memory_type = command_memory_type;
			}
			block_size += command->original_dump_size();
		}
	}
	
	if (block_size)
		manager->Add(block_address, block_size, block_memory_type, this);

	return true;
}

bool BaseFunction::PrepareExtCommands(const CompileContext &ctx)
{
	return true;
}

bool BaseFunction::PrepareLinks(const CompileContext &ctx)
{
	IFunctionList *function_list = ctx.file->function_list();
	for (size_t i = 0; i < link_list_->count(); i++) {
		CommandLink *link = link_list_->item(i);
		if (link->type() == ltNone)
			continue;

		if (link->to_address()) {
			ICommand *command = function_list->GetCommandByAddress(link->to_address(), true);
			if (is_breaked_address(link->from_command()->address())) {
				if (command && command->owner()->address() != link->to_address() && !command->owner()->ext_command_list()->GetCommandByAddress(link->to_address()))
					ctx.file->Notify(mtWarning, link->from_command(), string_format(language[lsJumpToInternalAddress].c_str(), link->to_address()));
				continue;
			} else {
				if (!command && function_list->GetCommandByNearAddress(link->to_address(), true)) {
					ctx.file->Notify(mtError, link->from_command(), language[lsJumpToCommandPart]);
					return false;
				}
				link->set_to_command(command && (command->options() & roNeedCompile) ? command : NULL);
			}
		}

		link->from_command()->PrepareLink(ctx);
	}

	return true;
}

bool BaseFunction::Init(const CompileContext &/*ctx*/)
{
	ICommand *command;
	size_t i;

	if (function_info_list()->count()) {
		std::set<uint64_t> address_list;
		FunctionInfo *info;
		AddressRange *range;
		for (i = 0; i < function_info_list_->count(); i++) {
			info = function_info_list_->item(i);
			address_list.insert(info->begin());
			address_list.insert(info->end());
		}
		for (i = 0; i < range_list_->count(); i++) {
			range = range_list_->item(i);
			info = function_info_list_->GetItemByAddress(range->begin());
			if (!info)
				continue;

			range->set_link_info(info);
			if (!range->end())
				range->set_end(info->end());
			address_list.insert(range->begin());
			address_list.insert(range->end());
		}

		uint64_t begin = 0;
		for (std::set<uint64_t>::const_iterator it = address_list.begin(); it != address_list.end(); it++) {
			if (begin) {
				uint64_t end = *it;
				info = function_info_list_->GetItemByAddress(begin);
				if (info) {
					AddressRange *dest = info->Add(begin, end, NULL, NULL, NULL);
					for (i = 0; i < range_list_->count(); i++) {
						range = range_list_->item(i);
						if (range->begin() <= begin && range->end() > begin)
							dest->AddLink(range);
					}
				}
			}
			begin = *it;
		}

		for (i = 0; i < count(); i++) {
			command = item(i);
			if (command->address_range())
				continue;

			command->set_address_range(function_info_list_->GetRangeByAddress(command->address()));
		}
	}

	return true;
}

bool BaseFunction::Prepare(const CompileContext &ctx)
{
	if (!FreeByManager(ctx))
		return false;

	range_list()->Prepare();
	function_info_list()->Prepare();

	return true;
}

bool BaseFunction::Compile(const CompileContext &ctx)
{
	return true;
}

void BaseFunction::AfterCompile(const CompileContext &ctx)
{

}

void BaseFunction::CompileInfo(const CompileContext &ctx)
{
	block_list()->CompileInfo();
	function_info_list()->Compile();
}

void BaseFunction::CompileLinks(const CompileContext &ctx)
{
	block_list()->CompileLinks(ctx);
}

size_t BaseFunction::WriteToFile(IArchitecture &file)
{
	size_t res = block_list_->WriteToFile(file);
	function_info_list_->WriteToFile(file);
	return res;
}

void BaseFunction::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	size_t i, j, c;
	ICommand *command;
	uint64_t add_address = file.image_base();

	tag_ = buffer.ReadByte();
	uint8_t b = buffer.ReadByte();

	switch (b & 0x30) {
	case 0x10:
		entry_type_ = etNone;
		break;
	case 0x20:
		entry_type_ = etRandomAddress;
		break;
	}

	compilation_type_ =static_cast<CompilationType>(b & 0xf);
	type_ = static_cast<ObjectType>(buffer.ReadByte());
	cpu_address_size_ = static_cast<OperandSize>(buffer.ReadByte());
	address_ = buffer.ReadDWord() + add_address;

	c = buffer.ReadDWord();
	for (i = 0; i < c; i++) {
		command = CreateCommand();
		command->ReadFromBuffer(buffer, file);
		AddObject(command);
	}

	c = buffer.ReadDWord();
	for (i = 0; i < c; i++) {
		uint32_t begin = buffer.ReadDWord();
		uint32_t end = buffer.ReadDWord();
		AddressBaseType base_type = static_cast<AddressBaseType>(buffer.ReadByte());
		uint32_t base_value = (base_type == btValue) ? buffer.ReadDWord() : 0;
		size_t prolog_size = buffer.ReadDWord();
		uint8_t frame_registr = buffer.ReadByte();
		uint32_t index = buffer.ReadDWord();
		uint32_t data_index = buffer.ReadDWord();
		std::vector<ICommand *> unwind_opcodes;
		unwind_opcodes.resize(buffer.ReadDWord());
		for (j = 0; j < unwind_opcodes.size(); j++) {
			unwind_opcodes[j] = item(buffer.ReadDWord() - 1);
		}
		FunctionInfo *info = function_info_list()->Add(begin + add_address, end + add_address, base_type, base_value, prolog_size, frame_registr,
			file.runtime_function_list()->GetFunctionByAddress(begin + add_address), (index != 0) ? item(index - 1) : NULL);
		if (data_index != 0)
			info->set_data_entry(item(data_index - 1));
		info->set_unwind_opcodes(unwind_opcodes);
	}

	c = buffer.ReadDWord();
	for (i = 0; i < c; i++) {
		uint32_t begin = buffer.ReadDWord();
		uint32_t end = buffer.ReadDWord();
		uint32_t begin_index = buffer.ReadDWord();
		uint32_t end_index = buffer.ReadDWord();
		uint32_t size_index = buffer.ReadDWord();
		range_list_->Add(begin + add_address, end + add_address,
			begin_index != 0 ? item(begin_index - 1) : NULL,
			end_index != 0 ? item(end_index - 1) : NULL,
			size_index != 0 ? item(size_index - 1) : NULL);
	}

	c = buffer.ReadDWord();
	for (i = 0; i < c; i++) {
		uint32_t dw = buffer.ReadDWord();
		LinkType link_type = static_cast<LinkType>(buffer.ReadByte());
		int operand_index = static_cast<int8_t>(buffer.ReadByte());
		uint8_t opt = buffer.ReadByte();
		uint64_t qw = (opt & 1) ? buffer.ReadDWord() + add_address : 0;
		CommandLink *link = item(dw - 1)->AddLink(operand_index, link_type, qw);
		if (opt & 2)
			link->set_sub_value(buffer.ReadDWord() + add_address);
		if (opt & 4) {
			dw = buffer.ReadDWord();
			if (dw != 0)
				link->set_parent_command(item(dw - 1));
		}
		if (opt & 8) {
			dw = buffer.ReadDWord();
			if (dw != 0)
				link->set_base_function_info(function_info_list()->item(dw - 1));
		}
	}

	memory_type_ = owner_->owner()->segment_list()->GetMemoryTypeByAddress(address_);
	if (type_ != otString)
		entry_ = GetCommandByAddress(address_);
}

CommandBlock *BaseFunction::AddBlock(size_t start_index, bool is_executable)
{
	return block_list_->Add((memory_type_ & (mtDiscardable | mtNotPaged)) | (is_executable ? mtExecutable : mtReadable), start_index);
}

uint8_t *version_watermark = NULL;
uint8_t *owner_watermark = NULL;

void BaseFunction::AddWatermark(Watermark *watermark, int copy_count)
{
	Watermark secure_watermark(NULL);
	std::string value;
	uint8_t *internal_watermarks[] = {version_watermark, owner_watermark};

	for (size_t k = 0; k < 1 + _countof(internal_watermarks); k++) {
		if (k == 0) {
			if (!watermark)
				continue;
		} else {
			uint8_t *ptr = internal_watermarks[k - 1];
			if (!ptr)
				continue;

			uint32_t key = *reinterpret_cast<uint32_t *>(ptr);
			uint16_t len = *reinterpret_cast<uint16_t *>(ptr + 4);
			value.resize(len);
			for (size_t i = 0; i < value.size(); i++) {
				value[i] = ptr[6 + i] ^ static_cast<uint8_t>(_rotl32(key, (int)i) + i);
			}
			secure_watermark.set_value(value);
			watermark = &secure_watermark;
		}

		for (int i = 0; i < copy_count; i++) {
			watermark->Compile();
			ICommand *command = AddCommand(Data(watermark->dump()));
			command->include_option(roCreateNewBlock);
		}
	}
}

void BaseFunction::Rebase(uint64_t delta_base)
{
	map_.clear();
	for (size_t i = 0; i < count(); i++) {
		ICommand *command = item(i);
		command->Rebase(delta_base);
		if (command->address())
			map_[command->address()] = command;
	}
	link_list_->Rebase(delta_base);
	range_list_->Rebase(delta_base);
	function_info_list_->Rebase(delta_base);

	if (address_)
		address_ += delta_base;
}

void BaseFunction::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void BaseFunction::set_compilation_type(CompilationType compilation_type)
{ 
	if (default_compilation_type_ != ctNone)
		return;

	if (compilation_type_ != compilation_type) {
		compilation_type_ = compilation_type;
		Notify(mtChanged, this);
	}
}

void BaseFunction::set_compilation_options(uint32_t compilation_options)
{ 
	if (compilation_options_ != compilation_options) {
		compilation_options_ = compilation_options;
		Notify(mtChanged, this);
	}
}

void BaseFunction::set_need_compile(bool need_compile)
{
	if (need_compile_ != need_compile) {
		need_compile_ = need_compile;
		Notify(mtChanged, this);
	}
}

void BaseFunction::set_folder(Folder *folder)
{ 
	if (folder_ != folder) {
		folder_ = folder;
		Notify(mtChanged, this);
	}
}

void BaseFunction::set_break_address(uint64_t break_address)
{
	if (type_ == otString)
		return;

	if (break_address_ != break_address) {
		break_address_ = break_address;
		Notify(mtChanged, this);
	}
}

std::string BaseFunction::display_address(const std::string &arch_name) const
{
	std::string res;
	if (type() != otUnknown)
		res.append(arch_name).append(DisplayValue(cpu_address_size(), address()));
	if (type() == otString) {
		for (size_t i = 1; i < count(); i++) {
			res.append(", ").append(arch_name).append(DisplayValue(cpu_address_size(), item(i)->address()));
		}
	}

	return res;
}

Data BaseFunction::hash() const
{
	Data res;
	bool is_unknown = (type() == otUnknown);
	res.PushBuff(name().c_str(), name().size() + 1);
	res.PushByte(need_compile());
	res.PushByte(is_unknown);
	res.PushDWord(is_unknown ? tag() : -1);
	res.PushByte(compilation_type());
	res.PushDWord(compilation_options());
	res.PushDWord(break_address() ? static_cast<uint32_t>(break_address() - address()) : 0);
	res.PushDWord(static_cast<uint32_t>(ext_command_list()->count()));
	for (size_t i = 0; i < ext_command_list()->count(); i++) {
		res.PushDWord(static_cast<uint32_t>(ext_command_list()->item(i)->address() - address()));
	}
	return res;
}

#ifdef CHECKED
bool BaseFunction::check_hash() const
{
	for (size_t i = 0; i < count(); i++) {
		if (!item(i)->check_hash())
			return false;
	}
	return true;
}
#endif

IVirtualMachine *BaseFunction::virtual_machine(IVirtualMachineList *virtual_machine_list, ICommand *command) const
{
	if (virtual_machine_list) {
		std::vector<IVirtualMachine *> list;
		for (size_t i = 0; i < virtual_machine_list->count(); i++) {
			IVirtualMachine *virtual_machine = virtual_machine_list->item(i);
			if (virtual_machine->processor()->cpu_address_size() == cpu_address_size())
				list.push_back(virtual_machine);
		}
		return list[rand() % list.size()];
	}

	return NULL;
}

/**
 * Signature
 */

Signature::Signature(SignatureList *owner, const std::string &value, uint32_t tag)
	: IObject(), owner_(owner), value_(value), tag_(tag)
{
	Init();
}

Signature::~Signature()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void Signature::Init()
{
	size_t i, p;
	uint8_t m, b;
	char c;

	dump_.clear();
	mask_.clear();

	if (value_.size() == 0)
		return;

	for (i = 0; i < value_.size(); i++) {
		p = i / 2;
		if (p >= dump_.size()) {
			dump_.push_back(0);
			mask_.push_back(0);
		}

		m = 0xff;
		c = value_[i];
		if ((c >= '0') && (c <= '9')) {
			b = c - '0';
		} else if ((c >= 'A') && (c <= 'F')) {
			b = c - 'A' + 0x0a;
		} else if ((c >= 'a') && (c <= 'f')) {
			b = c - 'a' + 0x0a;
		} else {
			m = 0;
			b = 0;
		}

		if ((i & 1) == 0) {
			dump_[p] = (dump_[p] & 0x0f) | (b << 4);
			mask_[p] = (mask_[p] & 0x0f) | (m << 4);
		} else {
			dump_[p] = (dump_[p] & 0xf0) | (b & 0x0f);
			mask_[p] = (mask_[p] & 0xf0) | (m & 0x0f);
		}
	}
}

bool Signature::SearchByte(uint8_t value)
{
	int i;
	size_t p;
	bool res;

	if (dump_.size() == 0)
		return false;

	res = false;
	for (i = (int)pos_.size() - 1; i >= -1; i--)	{
		p = (i == -1) ? 0 : pos_[i];
		if ((dump_[p] & mask_[p]) == (value & mask_[p])) {
			p++;
			if (p == dump_.size()) {
				res = true;
				if (i > -1)
					pos_.erase(pos_.begin() + i);
			} else if (i == -1) {
				pos_.push_back(p);
			} else {
				pos_[i] = p;
			}
		} else if (i > -1) {
			pos_.erase(pos_.begin() + i);
		}
	}

	return res;
}

/**
 * SignatureList
 */

SignatureList::SignatureList()
	: ObjectList<Signature>() 
{

}

Signature *SignatureList::Add(const std::string &value, uint32_t tag) 
{
	Signature *sign = new Signature(this, value, tag);
	AddObject(sign);
	return sign;
}

void SignatureList::InitSearch() 
{ 
	for (size_t i = 0; i < count(); i++) {
		item(i)->InitSearch();
	}
}

/**
 * BaseFunctionList
 */

BaseFunctionList::BaseFunctionList(IArchitecture *owner)
	: IFunctionList(), owner_(owner)
{

}

BaseFunctionList::BaseFunctionList(IArchitecture *owner, const BaseFunctionList &src)
	: IFunctionList(src), owner_(owner)
{
	size_t i;
	std::vector<Folder*> src_folders, folders;
	Folder *folder;

	for (i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}

	src_folders = src.owner()->owner()->folder_list()->GetFolderList();
	FolderList *folder_list = NULL;
	if (owner && owner->owner()) folder_list = owner->owner()->folder_list();
	if (folder_list) folders = folder_list->GetFolderList();
	for (i = 0; i < src.count(); i++) {
		std::vector<Folder*>::const_iterator it = std::find(src_folders.begin(), src_folders.end(), src.item(i)->folder());
		folder = (it == src_folders.end()) ? folder_list : *it;
		if (folder) item(i)->set_folder(folder);
	}
}

IFunction *BaseFunctionList::AddUnknown(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder)
{
	IFunction *func = GetUnknownByName(name);
	if (!func) {
		func = Add(name, compilation_type, compilation_options, need_compile, folder);
		if (func)
			Notify(mtAdded, func);
	} else {
		func->set_compilation_type(compilation_type);
		func->set_compilation_options(compilation_options);
		func->set_need_compile(need_compile);
		func->set_folder(folder);
	}
	return func;
};

IFunction *BaseFunctionList::AddByAddress(uint64_t address, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder)
{
	IFunction *func = GetFunctionByAddress(address);
	if (!func) {
		// check address
		uint32_t memory_type = owner_->segment_list()->GetMemoryTypeByAddress(address);
		if ((memory_type & mtExecutable) == 0) {
			MapFunction *map_function = owner_->map_function_list()->GetFunctionByAddress(address);
			if (!map_function || map_function->type() != otString)
				return NULL;
		}

		func = Add("", compilation_type, compilation_options, need_compile, folder);
		if (func) {
			func->ReadFromFile(*owner_, address);
			Notify(mtAdded, func);
		}
	} else {
		func->set_compilation_type(compilation_type);
		func->set_compilation_options(compilation_options);
		func->set_need_compile(need_compile);
		func->set_folder(folder);
	}
	return func;
};

IFunction *BaseFunctionList::GetFunctionByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->address() == address)
			return func;
	}

	return NULL;
}

IFunction *BaseFunctionList::GetFunctionByName(const std::string &name) const
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->name().compare(name) == 0 && func->type() != otUnknown)
			return func;
	}

	return NULL;
}

IFunction *BaseFunctionList::GetUnknownByName(const std::string &name) const
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->name().compare(name) == 0 && func->type() == otUnknown)
			return func;
	}

	return NULL;
}

ICommand *BaseFunctionList::GetCommandByAddress(uint64_t address, bool need_compile) const
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (need_compile && !func->need_compile())
			continue;

		ICommand *command = func->GetCommandByAddress(address);
		if (command)
			return (need_compile && func->is_breaked_address(command->address())) ? NULL : command;
	}

	return NULL;
}

ICommand *BaseFunctionList::GetCommandByNearAddress(uint64_t address, bool need_compile) const
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (need_compile && !func->need_compile())
			continue;

		ICommand *command = func->GetCommandByNearAddress(address);
		if (command) 
			return (need_compile && func->is_breaked_address(command->address())) ? NULL : command;
	}

	return NULL;
}

bool BaseFunctionList::Prepare(const CompileContext &ctx)
{
	size_t i, j;

	bool need_machines = (ctx.runtime != NULL);
	uint32_t memory_type = mtReadable | mtDiscardable;
	for (i = count(); i > 0; i--) {
		IFunction *func = item(i - 1);
		if (!func->need_compile())
			delete func;
		else if (func->type() != otUnknown && func->compilation_type() != ctMutation) {
			if ((func->memory_type() & mtDiscardable) == 0)
				memory_type &= ~mtDiscardable;
			if (func->memory_type() & mtNotPaged)
				memory_type |= mtNotPaged;
			need_machines = true;
		}
	}

	if (need_machines) {
		IVirtualMachineList *virtual_machine_list = ctx.file->virtual_machine_list();
		virtual_machine_list->Prepare(ctx);
		std::vector<IFunction *> processor_list = ctx.file->function_list()->processor_list();
		for (i = 0; i < processor_list.size(); i++) {
			processor_list[i]->set_memory_type(memory_type);
		}
	}

	for (j = 0; j < 4; j++) {
		for (i = 0; i < count(); i++) {
			IFunction *func = item(i);
			switch (j) {
			case 0:
				if (func->type() == otUnknown) {
					ctx.file->Notify(mtWarning, func, string_format(language[lsFunctionNotFound].c_str(), func->name().c_str()));
					continue;
				}
				if (!func->Init(ctx))
					return false;
				break;
			case 1:
				if (!func->Prepare(ctx))
					return false;
				break;
			case 2:
				if (!func->PrepareExtCommands(ctx))
					return false;
				break;
			case 3:
				if (!func->PrepareLinks(ctx))
					return false;
				break;
			}
		}
	}

	return true;
}

bool BaseFunctionList::Compile(const CompileContext &ctx)
{
	size_t i, j, k;
	IFunction *func;
	CommandBlock *block;
	std::vector<CommandBlock *> block_list;

	j = 0;
	auto jjj = count();
	for (i = 0; i < count(); i++) {
		func = item(i);
		j += func->count();
		if (func->compilation_type() == ctUltra)
			j += func->count();
	}
	ctx.file->StartProgress(string_format("%s...", language[lsCompiling].c_str()), j);
		
	for (i = 0; i < count(); i++) {
		func = item(i);
		if (!func->Compile(ctx))
			return false;
	}
	for (i = 0; i < count(); i++) {
		func = item(i);
		func->AfterCompile(ctx);
		if (!func->need_compile())
			continue;

		for (j = 0; j < func->block_list()->count(); j++) {
			block_list.push_back(func->block_list()->item(j));
		}
	}

	for (i = 0; i < block_list.size(); i++) {
		std::swap(block_list[i], block_list[rand() % block_list.size()]);
	}

	if (ctx.file->runtime_function_list() && ctx.file->runtime_function_list()->count()) {
		// sort blocks by address range
		for (i = 0; i < block_list.size(); i++) {
			block_list[i]->set_sort_index(i);
		}
		std::sort(block_list.begin(), block_list.end(), CommandBlockListCompareHelper());
		// add prolog blocks
		std::set<FunctionInfo *> function_info_list;
		Data data;
		for (i = 0; i < block_list.size(); i++) {
			block = block_list[i];
			if ((block->type() & mtExecutable) == 0)
				continue;

			AddressRange *address_range = block->function()->item(block->start_index())->address_range();
			if (!address_range)
				continue;

			FunctionInfo *function_info = address_range->owner();
			if (!function_info->prolog_size() || function_info_list.find(function_info) != function_info_list.end())
				continue;

			func = block->function();
			size_t prolog_size = 0;
			for (j = block->start_index(); j <= block->end_index(); j++) {
				prolog_size += func->item(j)->dump_size();
			}

			if (prolog_size < function_info->prolog_size()) {
				size_t size = function_info->prolog_size() - prolog_size;
				data.resize(size);
				for (k = 0; k < data.size(); k++) {
					data[k] = rand();
				}
				CommandBlock *new_block = func->AddBlock(func->count(), true);
				ICommand *command = func->AddCommand(data);
				command->set_block(new_block);
				command->set_address_range(address_range);

				block_list.insert(block_list.begin() + i + 1, new_block);
			}

			function_info_list.insert(function_info);
		}
	}

	for (i = 0; i < block_list.size(); i++) {
		block_list[i]->Compile(*ctx.manager);
	}

	CompileInfo(ctx);
	CompileLinks(ctx);

	ctx.file->EndProgress();

	return true;
}

void BaseFunctionList::CompileInfo(const CompileContext &ctx)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->CompileInfo(ctx);
	}
}

void BaseFunctionList::CompileLinks(const CompileContext &ctx)
{
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->compilation_type() != ctMutation)
			continue;

		func->CompileLinks(ctx);
	}

	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->compilation_type() == ctMutation)
			continue;

		func->CompileLinks(ctx);
	}
}

void BaseFunctionList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	size_t c = buffer.ReadDWord();
	for (size_t i = 0; i < c; i++) {
		IFunction *func = CreateFunction();
		AddObject(func);

		func->ReadFromBuffer(buffer, file);
	}
}

void BaseFunctionList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

void BaseFunctionList::RemoveObject(IFunction *func)
{
	Notify(mtDeleted, func);
	IFunctionList::RemoveObject(func);
}

void BaseFunctionList::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

std::vector<IFunction *> BaseFunctionList::processor_list() const
{
	std::vector<IFunction *> res;
	for (size_t i = 0; i < count(); i++) {
		IFunction *func = item(i);
		if (func->tag() == ftProcessor)
			res.push_back(func);
	}
	return res;
}

#ifdef CHECKED
bool BaseFunctionList::check_hash() const
{
	for (size_t i = 0; i < count(); i++) {
		if (!item(i)->check_hash())
			return false;
	}
	return true;
}
#endif

/**
 * CommandInfo
 */

CommandInfo::CommandInfo(CommandInfoList *owner, AccessType type, uint8_t value, OperandType operand_type, OperandSize size)
	: IObject(), owner_(owner), type_(type), value_(value), operand_type_(operand_type), size_(size)
{

}

CommandInfo::~CommandInfo()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * CommandInfoList
 */

CommandInfoList::CommandInfoList()
		: ObjectList<CommandInfo>(), need_flags_(0), change_flags_(0)
{

}

void CommandInfoList::Add(AccessType type, uint8_t value, OperandType operand_type, OperandSize size)
{
	CommandInfo *command_info = GetInfo(type, operand_type, value);
	if (command_info) {
		if (operand_type == otHiPartRegistr) {
			if (size == command_info->size())
				return;
		} else {
			if (size > command_info->size())
				command_info->set_size(size);
			return;
		}
	}

	if (operand_type == otRegistr) {
		command_info = GetInfo(type, otHiPartRegistr, value);
		if (command_info) {
			if (size > command_info->size()) {
				delete command_info;
			} else if (size == command_info->size()) {
				delete command_info;
				size = static_cast<OperandSize>(size + 1);
			}
		}
	} else if (operand_type == otHiPartRegistr) {
		command_info = GetInfo(type, otRegistr, value);
		if (command_info) {
			if (size < command_info->size())
				return;
			if (size == command_info->size()) {
				command_info->set_size(static_cast<OperandSize>(size + 1));
				return;
			}
		}
	}

	command_info = new CommandInfo(this, type, value, operand_type, size);
	AddObject(command_info);
}

CommandInfo *CommandInfoList::GetInfo(AccessType type, OperandType operand_type, uint8_t value) const
{
	for (size_t i = 0; i < count(); i++) {
		CommandInfo *command_info = item(i);
		if (command_info->type() == type && command_info->value() == value && command_info->operand_type() == operand_type)
			return command_info;
	}
	return NULL;
}

CommandInfo *CommandInfoList::GetInfo(AccessType type, OperandType operand_type) const
{
	for (size_t i = 0; i < count(); i++) {
		CommandInfo *command_info = item(i);
		if (command_info->type() == type && command_info->operand_type() == operand_type)
			return command_info;
	}
	return NULL;
}

CommandInfo *CommandInfoList::GetInfo(OperandType operand_type) const
{
	for (size_t i = 0; i < count(); i++) {
		CommandInfo *command_info = item(i);
		if (command_info->operand_type() == operand_type)
			return command_info;
	}
	return NULL;
}

void CommandInfoList::clear()
{
	need_flags_ = 0;
	change_flags_ = 0;
	ObjectList<CommandInfo>::clear();
}

bool CommandBlockListCompareHelper::operator()(const CommandBlock *block1, const CommandBlock *block2) const
{
	AddressRange *range1 = (block1->type() & mtExecutable) ? block1->function()->item(block1->start_index())->address_range() : NULL;
	AddressRange *range2 = (block2->type() & mtExecutable) ? block2->function()->item(block2->start_index())->address_range() : NULL;

	FunctionInfo *info1 = range1 ? range1->owner() : NULL;
	FunctionInfo *info2 = range2 ? range2->owner() : NULL;

	bool res;
	if (info1 == info2 && range1 && range2) {
		if (range1->original_begin() == range2->original_begin())
			res = range1->original_begin() ? block1->start_index() < block2->start_index() : block1->sort_index() < block2->sort_index();
		else
			res = range1->original_begin() < range2->original_begin();
	} else {
		uint64_t value1 = info1 ? info1->begin() : 0;
		uint64_t value2 = info2 ? info2->begin() : 0;
		res = (value1 == value2) ? block1->sort_index() < block2->sort_index() : (value1 < value2);
	}

	return res;
}

/**
* BaseVirtualMachine
*/

BaseVirtualMachine::BaseVirtualMachine(IVirtualMachineList *owner, uint8_t id)
	: IVirtualMachine(), owner_(owner), id_(id)
{

}

BaseVirtualMachine::~BaseVirtualMachine()
{
	if (owner_)
		owner_->RemoveObject(this);
}