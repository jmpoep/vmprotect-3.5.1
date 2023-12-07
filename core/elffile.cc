/**
 * Support of ELF executable files.
 */

#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "dwarf.h"
#include "elffile.h"
#include "processors.h"
#include "intel.h"
#include "core.h"
#include "lang.h"
#include "script.h"

#include "lin_runtime32.so.inc"
#include "lin_runtime64.so.inc"

/**
 * ELFDirectory
 */

ELFDirectory::ELFDirectory(ELFDirectoryList *owner)
	: BaseLoadCommand(owner), type_(0), value_(0)
{

}

ELFDirectory::ELFDirectory(ELFDirectoryList *owner, size_t type)
	: BaseLoadCommand(owner), type_(type), value_(0)
{

}

ELFDirectory::ELFDirectory(ELFDirectoryList *owner, const ELFDirectory &src)
	: BaseLoadCommand(owner)
{
	type_ = src.type_;
	value_ = src.value_;
	str_value_ = src.str_value_;
}

ELFDirectory *ELFDirectory::Clone(ILoadCommandList *owner) const
{
	ELFDirectory *dir = new ELFDirectory(reinterpret_cast<ELFDirectoryList *>(owner), *this);
	return dir;
}

void ELFDirectory::ReadFromFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Dyn dyn;
		file.Read(&dyn, sizeof(dyn));
		type_ = dyn.d_tag;
		value_ = dyn.d_un.d_val;
	} else {
		Elf64_Dyn dyn;
		file.Read(&dyn, sizeof(dyn));
		type_ = dyn.d_tag;
		value_ = dyn.d_un.d_val;
	}
}

void ELFDirectory::ReadStrings(ELFStringTable &string_table)
{
	if (type_ == DT_NEEDED || type_ == DT_RPATH || type_ == DT_RUNPATH || type_ == DT_SONAME) {
		if (value_ >> 32)
			throw std::runtime_error("Invalid format");
		str_value_ = string_table.GetString(static_cast<uint32_t>(value_));
	}
}

void ELFDirectory::WriteStrings(ELFStringTable &string_table)
{
	if (type_ == DT_NEEDED || type_ == DT_RPATH || type_ == DT_RUNPATH || type_ == DT_SONAME)
		value_ = string_table.AddString(str_value_);
}

size_t ELFDirectory::WriteToFile(ELFArchitecture &file)
{
	size_t res = 0;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Dyn dyn;
		dyn.d_tag = static_cast<uint32_t>(type_);
		dyn.d_un.d_val = static_cast<uint32_t>(value_);
		res += file.Write(&dyn, sizeof(dyn));
	} else {
		Elf64_Dyn dyn;
		dyn.d_tag = type_;
		dyn.d_un.d_val = value_;
		res += file.Write(&dyn, sizeof(dyn));
	}
	return res;
}

std::string ELFDirectory::name() const
{
	switch (type_) {
	case DT_NULL:
		return std::string("DT_NULL");
	case DT_NEEDED:
		return std::string("DT_NEEDED");
	case DT_PLTRELSZ:
		return std::string("DT_PLTRELSZ");
	case DT_PLTGOT:
		return std::string("DT_PLTGOT");
	case DT_HASH:
		return std::string("DT_HASH");
	case DT_STRTAB:
		return std::string("DT_STRTAB");
	case DT_SYMTAB:
		return std::string("DT_SYMTAB");
	case DT_RELA:
		return std::string("DT_RELA");
	case DT_RELASZ:
		return std::string("DT_RELASZ");
	case DT_RELAENT:
		return std::string("DT_RELAENT");
	case DT_STRSZ:
		return std::string("DT_STRSZ");
	case DT_SYMENT:
		return std::string("DT_SYMENT");
	case DT_INIT:
		return std::string("DT_INIT");
	case DT_FINI:
		return std::string("DT_FINI");
	case DT_SONAME:
		return std::string("DT_SONAME");
	case DT_RPATH:
		return std::string("DT_RPATH");
	case DT_SYMBOLIC:
		return std::string("DT_SYMBOLIC");
	case DT_REL:
		return std::string("DT_REL");
	case DT_RELSZ:
		return std::string("DT_RELSZ");
	case DT_RELENT:
		return std::string("DT_RELENT");
	case DT_PLTREL:
		return std::string("DT_PLTREL");
	case DT_DEBUG:
		return std::string("DT_DEBUG");
	case DT_TEXTREL:
		return std::string("DT_TEXTREL");
	case DT_JMPREL:
		return std::string("DT_JMPREL");
	case DT_BIND_NOW:
		return std::string("DT_BIND_NOW");
	case DT_INIT_ARRAY:
		return std::string("DT_INIT_ARRAY");
	case DT_FINI_ARRAY:
		return std::string("DT_FINI_ARRAY");
	case DT_INIT_ARRAYSZ:
		return std::string("DT_INIT_ARRAYSZ");
	case DT_FINI_ARRAYSZ:
		return std::string("DT_FINI_ARRAYSZ");
	case DT_RUNPATH:
		return std::string("DT_RUNPATH");
	case DT_FLAGS:
		return std::string("DT_FLAGS");
	case DT_PREINIT_ARRAY:
		return std::string("DT_PREINIT_ARRAY");
	case DT_PREINIT_ARRAYSZ:
		return std::string("DT_PREINIT_ARRAYSZ");
	case DT_GNU_HASH:
		return std::string("DT_GNU_HASH");
	case DT_RELACOUNT:
		return std::string("DT_RELACOUNT");
	case DT_RELCOUNT:
		return std::string("DT_RELCOUNT");
	case DT_FLAGS_1:
		return std::string("DT_FLAGS_1");
	case DT_VERSYM:
		return std::string("DT_VERSYM");
	case DT_VERDEF:
		return std::string("DT_VERDEF");
	case DT_VERDEFNUM:
		return std::string("DT_VERDEFNUM");
	case DT_VERNEED:
		return std::string("DT_VERNEED");
	case DT_VERNEEDNUM:
		return std::string("DT_VERNEEDNUM");
	}
	return BaseLoadCommand::name();
}

void ELFDirectory::Rebase(uint64_t delta_base)
{
	switch (type_) {
	case DT_PLTGOT:
		value_ += delta_base;
		break;
	}
}

/**
 * ELFDirectoryList
 */

ELFDirectoryList::ELFDirectoryList(ELFArchitecture *owner)
	: BaseCommandList(owner)
{
	
}

ELFDirectoryList::ELFDirectoryList(ELFArchitecture *owner, const ELFDirectoryList &src)
	: BaseCommandList(owner, src)
{

}

ELFDirectory *ELFDirectoryList::item(size_t index) const
{
	return reinterpret_cast<ELFDirectory *>(BaseCommandList::item(index));
}

ELFDirectory *ELFDirectoryList::GetCommandByType(uint32_t type) const
{
	return reinterpret_cast<ELFDirectory *>(BaseCommandList::GetCommandByType(type));
}

ELFDirectoryList *ELFDirectoryList::Clone(ELFArchitecture *owner) const
{
	ELFDirectoryList *list = new ELFDirectoryList(owner, *this);
	return list;
}

ELFDirectory *ELFDirectoryList::Add()
{
	ELFDirectory *dir = new ELFDirectory(this);
	AddObject(dir);
	return dir;
}

ELFDirectory *ELFDirectoryList::Add(size_t type)
{
	ELFDirectory *dir = new ELFDirectory(this, type);
	AddObject(dir);
	return dir;
}

void ELFDirectoryList::ReadFromFile(ELFArchitecture &file)
{
	ELFSegment *segment = file.segment_list()->GetSectionByType(PT_DYNAMIC);
	if (!segment)
		return;

	file.Seek(segment->physical_offset());
	size_t entry_size = file.cpu_address_size() == osDWord ? sizeof(Elf32_Dyn) : sizeof(Elf64_Dyn);
	for (uint64_t i = 0; i < segment->size(); i += entry_size) {
		ELFDirectory *dir = Add();
		dir->ReadFromFile(file);
		if (dir->type() == DT_NULL) {
			delete dir;
			break;
		}
	}
}

void ELFDirectoryList::WriteToFile(ELFArchitecture &file)
{
	ELFSegment *segment = file.segment_list()->GetSectionByType(PT_DYNAMIC);
	if (!segment)
		return;

	uint64_t address = file.AddressTell();
	uint64_t pos = file.Tell();
	size_t size = 0;
	for (size_t i = 0; i < count(); i++) {
		size += item(i)->WriteToFile(file);
	}
	ELFDirectory dt_null(NULL, DT_NULL);
	size += dt_null.WriteToFile(file);

	segment->Rebase(address - segment->address());
	segment->set_physical_offset(static_cast<uint32_t>(pos));
	segment->set_size(static_cast<uint32_t>(size));

	ELFSection *section = file.section_list()->GetSectionByType(SHT_DYNAMIC);
	if (section) {
		section->Rebase(address - section->address());
		section->set_physical_offset(static_cast<uint32_t>(pos));
		section->set_size(static_cast<uint32_t>(size));
	}
}

void ELFDirectoryList::ReadStrings(ELFStringTable &string_table)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadStrings(string_table);
	}
}

void ELFDirectoryList::WriteStrings(ELFStringTable &string_table)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteStrings(string_table);
	}
}

/**
 * ELFSegment
 */

ELFSegment::ELFSegment(ELFSegmentList *owner)
	: BaseSection(owner), type_(PT_NULL), address_(0), size_(0),
		physical_offset_(0), physical_size_(0), flags_(0), alignment_(0)
{

}

ELFSegment::ELFSegment(ELFSegmentList *owner, uint64_t address, uint64_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t flags, uint32_t type, uint64_t alignment)
	: BaseSection(owner), address_(address), size_(size), physical_offset_(physical_offset), physical_size_(physical_size),
	flags_(flags), type_(type), alignment_(alignment)
{

}

ELFSegment::ELFSegment(ELFSegmentList *owner, const ELFSegment &src)
	: BaseSection(owner, src)
{
	type_ = src.type_;
	address_ = src.address_;
	size_ = src.size_;
	physical_offset_ = src.physical_offset_;
	physical_size_ = src.physical_size_;
	flags_ = src.flags_;
	alignment_ = src.alignment_;
}

ELFSegment *ELFSegment::Clone(ISectionList *owner) const
{
	ELFSegment *segment = new ELFSegment(reinterpret_cast<ELFSegmentList *>(owner), *this);
	return segment;
}

std::string ELFSegment::name() const
{
	switch (type_) {
	case PT_NULL:
		return std::string("PT_NULL");
	case PT_LOAD:
		return std::string("PT_LOAD");
	case PT_DYNAMIC:
		return std::string("PT_DYNAMIC");
	case PT_INTERP:
		return std::string("PT_INTERP");
	case PT_NOTE:
		return std::string("PT_NOTE");
	case PT_SHLIB:
		return std::string("PT_SHLIB");
	case PT_PHDR:
		return std::string("PT_PHDR");
	case PT_TLS:
		return std::string("PT_TLS");
	case PT_GNU_EH_FRAME:
		return std::string("PT_GNU_EH_FRAME");
	case PT_GNU_STACK:
		return std::string("PT_GNU_STACK");
	case PT_GNU_RELRO:
		return std::string("PT_GNU_RELRO");
	}
	return string_format("%d", type_);
}

uint32_t ELFSegment::memory_type() const
{
	uint32_t res = mtNone;
	if (flags_ & PF_R)
		res |= mtReadable;
	if (flags_ & PF_W)
		res |= mtWritable;
	if (flags_ & PF_X)
		res |= mtExecutable;
	return res;
}

uint32_t ELFSegment::prot() const
{
	uint32_t res = PROT_NONE;
	if (flags_ & PF_R)
		res |= PROT_READ;
	if (flags_ & PF_W)
		res |= PROT_WRITE;
	if (flags_ & PF_X)
		res |= PROT_EXEC;
	return res;
}

void ELFSegment::ReadFromFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Phdr hdr;
		file.Read(&hdr, sizeof(hdr));
		type_ = hdr.p_type;
		address_ = hdr.p_paddr;
		size_ = hdr.p_memsz;
		physical_offset_ = hdr.p_offset;
		physical_size_ = hdr.p_filesz;
		flags_ = hdr.p_flags;
		alignment_ = hdr.p_align;
	} else {
		Elf64_Phdr hdr;
		file.Read(&hdr, sizeof(hdr));
		type_ = hdr.p_type;
		address_ = hdr.p_paddr;
		size_ = hdr.p_memsz;
		if (hdr.p_offset >> 32)
			throw std::runtime_error("Section size is too large");
		if (hdr.p_filesz >> 32)
			throw std::runtime_error("Section offset is too large");
		physical_offset_ = static_cast<uint32_t>(hdr.p_offset);
		physical_size_ = static_cast<uint32_t>(hdr.p_filesz);
		flags_ = hdr.p_flags;
		alignment_ = hdr.p_align;
	}
}

size_t ELFSegment::WriteToFile(ELFArchitecture &file)
{
	size_t res = 0;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Phdr hdr;
		hdr.p_type = type_;
		hdr.p_paddr = static_cast<uint32_t>(address_);
		hdr.p_memsz = static_cast<uint32_t>(size_);
		hdr.p_offset = physical_offset_;
		hdr.p_vaddr = static_cast<uint32_t>(address_);
		hdr.p_filesz = physical_size_;
		hdr.p_flags = flags_;
		hdr.p_align = static_cast<uint32_t>(alignment_);
		res = file.Write(&hdr, sizeof(hdr));
	} else {
		Elf64_Phdr hdr;
		hdr.p_type = type_;
		hdr.p_paddr = address_;
		hdr.p_memsz = size_;
		hdr.p_offset = physical_offset_;
		hdr.p_vaddr = address_;
		hdr.p_filesz = physical_size_;
		hdr.p_flags = flags_;
		hdr.p_align = alignment_;
		res = file.Write(&hdr, sizeof(hdr));
	}
	return res;
}

void ELFSegment::update_type(uint32_t mt) 
{
	if (mt & mtReadable)
		flags_ |= PF_R;
	if (mt & mtWritable)
		flags_ |= PF_W;
	if (mt & mtExecutable)
		flags_ |= PF_X;
}

void ELFSegment::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * ELFSegmentList
 */

ELFSegmentList::ELFSegmentList(ELFArchitecture *owner)
	: BaseSectionList(owner)
{

}

ELFSegmentList::ELFSegmentList(ELFArchitecture *owner, const ELFSegmentList &src)
	: BaseSectionList(owner, src)
{

}

ELFSegmentList *ELFSegmentList::Clone(ELFArchitecture *owner) const
{
	ELFSegmentList *list = new ELFSegmentList(owner, *this);
	return list;
}

ELFSegment *ELFSegmentList::item(size_t index) const
{ 
	return reinterpret_cast<ELFSegment*>(BaseSectionList::item(index));
}

ELFSegment *ELFSegmentList::last() const
{
	for (size_t i = count(); i > 0 ; i--) {
		ELFSegment *segment = item(i - 1);
		if (segment->type() == PT_LOAD)
			return segment;
	}
	return NULL;
}

ELFSegment *ELFSegmentList::Add()
{
	ELFSegment *segment = new ELFSegment(this);
	AddObject(segment);
	return segment;
}

void ELFSegmentList::ReadFromFile(ELFArchitecture &file, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		Add()->ReadFromFile(file);
	}
}

size_t ELFSegmentList::WriteToFile(ELFArchitecture &file)
{
	size_t res = 0;
	for (size_t i = 0; i < count(); i++) {
		res += item(i)->WriteToFile(file);
	}
	return res;
}

ELFSegment *ELFSegmentList::Add(uint64_t address, uint64_t size, uint32_t physical_offset, uint32_t physical_size,
	uint32_t initprot, uint32_t type, uint64_t alignment)
{
	ELFSegment *segment = new ELFSegment(this, address, size, physical_offset, physical_size, initprot, type, alignment);
	AddObject(segment);
	return segment;
}

ELFSegment *ELFSegmentList::GetSectionByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFSegment *segment = item(i);
		if (segment->type() != PT_LOAD)
			continue;

		if (address >= segment->address() && address < segment->address() + segment->size())
			return segment;
	}
	return NULL;
}

ELFSegment *ELFSegmentList::GetSectionByOffset(uint64_t offset) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFSegment *segment = item(i);
		if (segment->type() != PT_LOAD)
			continue;

		if (offset >= segment->physical_offset() && offset < static_cast<uint64_t>(segment->physical_offset()) + static_cast<uint64_t>(segment->physical_size()))
			return segment;
	}
	return NULL;
}

ELFSegment *ELFSegmentList::GetSectionByType(uint32_t type) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFSegment *segment = item(i);
		if (segment->type() == type)
			return segment;
	}
	return NULL;
}

/**
 * ELFStringTable
 */

ELFStringTable *ELFStringTable::Clone()
{
	ELFStringTable *table = new ELFStringTable(*this);
	return table;
}

std::string ELFStringTable::GetString(uint32_t pos) const 
{
	size_t i, len;

	if (pos >= data_.size())
		throw std::runtime_error("Invalid index for string table");

	len = data_.size() - pos;
	for (i = 0; i < len; i++) {
		if (data_[pos + i] == 0) {
			len = i;
			break;
		}
	}
	if (len == data_.size() - pos)
		throw std::runtime_error("Invalid format");

	return std::string(&data_[pos], len);
}

uint32_t ELFStringTable::AddString(const std::string &str)
{
	if (str.empty())
		return 0;

	std::map<std::string, uint32_t>::const_iterator it = map_.find(str);
	if (it != map_.end())
		return it->second;

	uint32_t res = static_cast<uint32_t>(data_.size());
	data_.insert(data_.end(), str.c_str(), str.c_str() + str.size() + 1);
	map_[str] = res;
	return res;
};

void ELFStringTable::clear() 
{ 
	data_.clear();
	data_.push_back(0);
	map_.clear();
}

void ELFStringTable::ReadFromFile(ELFArchitecture &file)
{
	ELFDirectory *dir = file.command_list()->GetCommandByType(DT_STRTAB);
	if (!dir)
		return;

	ELFDirectory *size = file.command_list()->GetCommandByType(DT_STRSZ);
	if (!size || !file.AddressSeek(dir->value()))
		throw std::runtime_error("Invalid format");

	data_.resize(static_cast<size_t>(size->value()));
	file.Read(data_.data(), data_.size());
}

void ELFStringTable::ReadFromFile(ELFArchitecture &file, const ELFSection &section)
{
	if (section.type() != SHT_STRTAB)
		throw std::runtime_error("Invalid format");

	file.Seek(section.physical_offset());
	data_.resize(static_cast<uint32_t>(section.size()));
	file.Read(data_.data(), data_.size());
}

size_t ELFStringTable::WriteToFile(ELFArchitecture &file)
{
	size_t res = file.Write(data_.data(), data_.size());
	return res;
}

/**
 * ELFSection
 */

ELFSection::ELFSection(ELFSectionList *owner)
	: BaseSection(owner), address_(0), size_(0), type_(0), physical_offset_(0), name_idx_(0),
	link_(0), flags_(0), entry_size_(0), parent_(0), info_(0), addralign_(1)
{

}

ELFSection::ELFSection(ELFSectionList *owner, uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t flags, uint32_t type, const std::string &name)
	: BaseSection(owner), address_(address), size_(size), type_(type), physical_offset_(physical_offset), name_idx_(0),
	link_(0), flags_(flags), entry_size_(0), parent_(0), name_(name), info_(0), addralign_(1)
{

}

ELFSection::ELFSection(ELFSectionList *owner, const ELFSection &src)
	: BaseSection(owner, src), parent_(0)
{
	address_ = src.address_;
	size_ = src.size_;
	type_ = src.type_;
	physical_offset_ = src.physical_offset_;
	flags_ = src.flags_;
	entry_size_ = src.entry_size_;
	link_ = src.link_;
	info_ = src.info_;
	addralign_ = src.addralign_;
	name_ = src.name_;
	name_idx_ = src.name_idx_;
	if (src.parent_)
	{
		ELFArchitecture *thisArc = dynamic_cast<ELFArchitecture *>(owner->owner());
		assert(thisArc);
		assert(thisArc->segment_list());
		if (thisArc && thisArc->segment_list())
			parent_ = thisArc->segment_list()->GetSectionByAddress(address_);
		assert(parent_);
	}
}

ELFSection *ELFSection::Clone(ISectionList *owner) const
{
	ELFSection *section = new ELFSection(reinterpret_cast<ELFSectionList *>(owner), *this);
	return section;
}

void ELFSection::ReadFromFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Shdr hdr;
		file.Read(&hdr, sizeof(hdr));
		name_idx_ = hdr.sh_name;
		type_ = hdr.sh_type;
		address_ = hdr.sh_addr;
		size_ = hdr.sh_size;
		physical_offset_ = hdr.sh_offset;
		flags_ = hdr.sh_flags;
		entry_size_ = hdr.sh_entsize;
		link_ = hdr.sh_link;
		info_ = hdr.sh_info;
		addralign_ = hdr.sh_addralign;
	} else {
		Elf64_Shdr hdr;
		file.Read(&hdr, sizeof(hdr));
		if (hdr.sh_size >> 32)
			throw std::runtime_error("Section size is too large");
		if (hdr.sh_offset >> 32)
			throw std::runtime_error("Section offset is too large");
		name_idx_ = hdr.sh_name;
		type_ = hdr.sh_type;
		address_ = hdr.sh_addr;
		size_ = static_cast<uint32_t>(hdr.sh_size);
		physical_offset_ = static_cast<uint32_t>(hdr.sh_offset);
		flags_ = hdr.sh_flags;
		entry_size_ = hdr.sh_entsize;
		link_ = hdr.sh_link;
		info_ = hdr.sh_info;
		addralign_ = static_cast<uint32_t>(hdr.sh_addralign);
	}

	if (address_)
		parent_ = file.segment_list()->GetSectionByAddress(address_);
}

void ELFSection::ReadName(ELFStringTable &strtab)
{
	name_ = strtab.GetString(name_idx_);
}

void ELFSection::WriteName(ELFStringTable &strtab)
{
	name_idx_ = strtab.AddString(name_);
}

void ELFSection::WriteToFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Shdr hdr;
		hdr.sh_name = name_idx_;
		hdr.sh_type = type_;
		hdr.sh_addr = static_cast<uint32_t>(address_);
		hdr.sh_size = size_;
		hdr.sh_offset = physical_offset_;
		hdr.sh_flags = static_cast<uint32_t>(flags_);
		hdr.sh_entsize = static_cast<uint32_t>(entry_size_);
		hdr.sh_link = link_;
		hdr.sh_info = info_;
		hdr.sh_addralign = static_cast<uint32_t>(addralign_);
		file.Write(&hdr, sizeof(hdr));
	} else {
		Elf64_Shdr hdr;
		hdr.sh_name = name_idx_;
		hdr.sh_type = type_;
		hdr.sh_addr = address_;
		hdr.sh_size = size_;
		hdr.sh_offset = physical_offset_;
		hdr.sh_flags = flags_;
		hdr.sh_entsize = entry_size_;
		hdr.sh_link = link_;
		hdr.sh_info = info_;
		hdr.sh_addralign = addralign_;
		file.Write(&hdr, sizeof(hdr));
	}
}

void ELFSection::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

void ELFSection::RemapLinks(const std::map<size_t, size_t> &index_map)
{
	std::map<size_t, size_t>::const_iterator it;

	switch (type_) {
	case SHT_DYNAMIC:
	case SHT_HASH:
	case SHT_REL:
	case SHT_RELA:
	case SHT_SYMTAB:
	case SHT_DYNSYM:
	case SHT_GNU_HASH:
	case SHT_GNU_versym:
	case SHT_GNU_verneed:
		it = index_map.find(link_);
		if (it == index_map.end() || it->second == NOT_ID)
			throw std::runtime_error("Invalid section index");

		link_ = static_cast<uint32_t>(it->second);
		break;
	}

	switch (type_) {
	case SHT_REL:
	case SHT_RELA:
		it = index_map.find(info_);
		if (it == index_map.end() || it->second == NOT_ID)
			throw std::runtime_error("Invalid section index");

		info_ = static_cast<uint32_t>(it->second);
		break;
	}
}

/**
 * ELFSectionList
 */

ELFSectionList::ELFSectionList(ELFArchitecture *owner)
	: BaseSectionList(owner)
{

}

ELFSectionList::ELFSectionList(ELFArchitecture *owner, const ELFSectionList &src)
	: BaseSectionList(owner, src)
{

}

ELFSection *ELFSectionList::item(size_t index) const
{
	return reinterpret_cast<ELFSection *>(BaseSectionList::item(index));
}

ELFSectionList *ELFSectionList::Clone(ELFArchitecture *owner) const
{
	ELFSectionList *section_list = new ELFSectionList(owner, *this);
	return section_list;
}

ELFSection *ELFSectionList::Add()
{
	ELFSection *section = new ELFSection(this);
	AddObject(section);
	return section;
}

ELFSection *ELFSectionList::Add(uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t flags, uint32_t type, const std::string &name)
{
	ELFSection *section = new ELFSection(this, address, size, physical_offset, flags, type, name);
	AddObject(section);
	return section;
}

void ELFSectionList::ReadFromFile(ELFArchitecture &file, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		ELFSection *section = Add();
		section->ReadFromFile(file);
	}

	if (file.shstrndx() != SHN_UNDEF) {
		string_table_.ReadFromFile(file, *file.section_list()->item(file.shstrndx()));
		for (i = 0; i < count; i++) {
			item(i)->ReadName(string_table_);
		}
	}
}

uint64_t ELFSectionList::WriteToFile(ELFArchitecture &file)
{
	string_table_.clear();
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteName(string_table_);
	}
	uint64_t pos = file.Tell();
	size_t size = string_table_.WriteToFile(file);
	ELFSection *section = item(file.shstrndx());
	section->set_physical_offset(static_cast<uint32_t>(pos));
	section->set_size(static_cast<uint32_t>(size));

	pos = file.Tell();
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
	return pos;
}

ELFSection *ELFSectionList::GetSectionByType(uint32_t type) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFSection *section = item(i);
		if (section->type() == type)
			return section;
	}
	return NULL;
}

ELFSection *ELFSectionList::GetSectionByAddress(uint64_t address) const
{
	return reinterpret_cast<ELFSection *>(BaseSectionList::GetSectionByAddress(address));
}

ELFSection *ELFSectionList::GetSectionByName(const std::string &name) const
{
	return reinterpret_cast<ELFSection *>(BaseSectionList::GetSectionByName(name));
}

void ELFSectionList::RemapLinks(const std::map<size_t, size_t> &index_map)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->RemapLinks(index_map);
	}
}

/**
 * ELFImportFunction
 */

ELFImportFunction::ELFImportFunction(ELFImport *owner, uint64_t address, const std::string &name, ELFSymbol *symbol)
		: BaseImportFunction(owner), address_(address), name_(name), symbol_(symbol)
{

}

ELFImportFunction::ELFImportFunction(ELFImport *owner, uint64_t address, APIType type, MapFunction *map_function)
	: BaseImportFunction(owner), address_(address), symbol_(NULL)
{
	set_type(type);
	set_map_function(map_function);
}

ELFImportFunction::ELFImportFunction(ELFImport *owner, const ELFImportFunction &src)
	: BaseImportFunction(owner, src)
{
	address_ = src.address_;
	name_ = src.name_;
	symbol_ = src.symbol_;
}

ELFImportFunction *ELFImportFunction::Clone(IImport *owner) const
{
	ELFImportFunction *func = new ELFImportFunction(reinterpret_cast<ELFImport*>(owner), *this);
	return func;
}

/**
 * ELFImport
 */

ELFImport::ELFImport(ELFImportList *owner, bool is_sdk)
	: BaseImport(owner), is_sdk_(is_sdk)
{

}

ELFImport::ELFImport(ELFImportList *owner, const std::string &name)
	: BaseImport(owner), name_(name), is_sdk_(false)
{

}

ELFImport::ELFImport(ELFImportList *owner, const ELFImport &src)
	: BaseImport(owner, src)
{
	name_ = src.name_;
	is_sdk_ = src.is_sdk_;
}

ELFImportFunction *ELFImport::item(size_t index) const 
{ 
	return reinterpret_cast<ELFImportFunction*>(BaseImport::item(index));
}

ELFImportFunction *ELFImport::GetFunctionBySymbol(ELFSymbol *symbol) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFImportFunction *func = item(i);
		if (func->symbol() == symbol)
			return func;
	}
	return NULL;
}

ELFImport *ELFImport::Clone(IImportList *owner) const
{
	ELFImport *list = new ELFImport(reinterpret_cast<ELFImportList *>(owner), *this);
	return list;
}

IImportFunction *ELFImport::Add(uint64_t address, APIType type, MapFunction *map_function)
{
	ELFImportFunction *import_function = new ELFImportFunction(this, address, type, map_function);
	AddObject(import_function);
	return import_function;
}

ELFImportFunction *ELFImport::Add(uint64_t address, const std::string &name, ELFSymbol *symbol)
{
	ELFImportFunction *import_function = new ELFImportFunction(this, address, name, symbol);
	AddObject(import_function);
	return import_function;
}

void ELFImportFunction::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
}

std::string ELFImportFunction::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

/**
 * ELFImportList
 */

ELFImportList::ELFImportList(ELFArchitecture *owner)
	: BaseImportList(owner)
{

}

ELFImportList::ELFImportList(ELFArchitecture *owner, const ELFImportList &src)
	: BaseImportList(owner, src)
{

}

ELFImportList *ELFImportList::Clone(ELFArchitecture *owner) const
{
	ELFImportList *list = new ELFImportList(owner, *this);
	return list;
}

ELFImport *ELFImportList::item(size_t index) const 
{ 
	return reinterpret_cast<ELFImport*>(IImportList::item(index));
}

ELFImportFunction *ELFImportList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<ELFImportFunction*>(BaseImportList::GetFunctionByAddress(address));
}

ELFImport *ELFImportList::Add(const std::string &name)
{
	ELFImport *import = new ELFImport(this, name);
	AddObject(import);
	return import;
}

ELFImport *ELFImportList::AddSDK()
{
	ELFImport *sdk = new ELFImport(this, true);
	AddObject(sdk);
	return sdk;
}

void ELFImportList::ReadFromFile(ELFArchitecture &file)
{
	static const ImportInfo default_info[] = {
		{atNone, "exit", ioNoReturn, ctNone},
		{atNone, "abort", ioNoReturn, ctNone},
		{atNone, "longjmp", ioNoReturn, ctNone},
		{atNone, "longjmp_chk", ioNoReturn, ctNone},
		{atNone, "_Unwind_Resume", ioNoReturn, ctNone},
		{atNone, "__stack_chk_fail", ioNoReturn, ctNone},
		{atNone, "__cxa_throw", ioNoReturn, ctNone},
		{atNone, "__cxa_end_cleanup", ioNoReturn, ctNone},
		{atNone, "__cxa_rethrow", ioNoReturn, ctNone},
		{atNone, "__cxa_bad_cast", ioNoReturn, ctNone},
		{atNone, "__cxa_bad_typeid", ioNoReturn, ctNone},
		{atNone, "__cxa_call_terminate", ioNoReturn, ctNone},
		{atNone, "__cxa_pure_virtual", ioNoReturn, ctNone},
		{atNone, "__cxa_call_unexpected", ioNoReturn, ctNone},
		{atNone, "_ZSt9terminatev", ioNoReturn, ctNone},
		{atNone, "_ZSt16__throw_bad_castv", ioNoReturn, ctNone},
		{atNone, "_ZSt17__throw_bad_allocv", ioNoReturn, ctNone},
		{atNone, "_ZSt19__throw_logic_errorPKc", ioNoReturn, ctNone},
		{atNone, "_ZSt20__throw_system_errori", ioNoReturn, ctNone},
		{atNone, "_ZSt20__throw_length_errorPKc", ioNoReturn, ctNone},
		{atNone, "_ZSt24__throw_invalid_argumentPKc", ioNoReturn, ctNone},
		{atNone, "_ZSt20__throw_out_of_rangePKc", ioNoReturn, ctNone},
		{atNone, "_ZSt24__throw_out_of_range_fmtPKcz", ioNoReturn, ctNone}
	};

	size_t i, j, k;

	ELFImport *sdk_import = NULL;
	std::string sdk_name = string_format("libvmprotectsdk%d.so", (file.cpu_address_size() == osDWord) ? 32 : 64);
	ELFSymbolList *symbol_list = file.dynsymbol_list();
	for (i = 0; i < file.command_list()->count(); i++) {
		ELFDirectory *dir = file.command_list()->item(i);
		switch (dir->type()) {
		case DT_NEEDED:
			{
				ELFImport *import = Add(dir->str_value());
				std::string dll_name = os::ExtractFileName(import->name().c_str());
				std::transform(dll_name.begin(), dll_name.end(), dll_name.begin(), tolower);
				if (dll_name.compare(sdk_name) == 0) {
					import->set_is_sdk(true);
					if (!sdk_import)
						sdk_import = import;
				}
			}
			break;
		}
	}

	std::map<ELFSymbol *, std::vector<uint64_t> > symbol_map;
	ELFRelocationList *relocation_list = file.relocation_list();
	for (i = 0; i < relocation_list->count(); i++) {
		ELFRelocation *reloc = relocation_list->item(i);
		std::map<ELFSymbol *, std::vector<uint64_t> >::iterator it = symbol_map.find(reloc->symbol());
		if (it != symbol_map.end())
			it->second.push_back(reloc->address());
		else
			symbol_map[reloc->symbol()].push_back(reloc->address());
	}

	std::map<uint16_t, ELFImport *> version_map;
	for (i = 0; i < file.verneed_list()->count(); i++) {
		ELFVerneed *verneed = file.verneed_list()->item(i);
		ELFImport *import = GetImportByName(verneed->file());
		for (j = 0; j < verneed->count(); j++) {
			ELFVernaux *vernaux = verneed->item(j);
			version_map[vernaux->other()] = import;
		}
	}

	ELFImport *empty_import = NULL;
	for (i = 0; i < symbol_list->count(); i++) {
		ELFSymbol *symbol = symbol_list->item(i);
		if (symbol->section_idx() || symbol->name().empty())
			continue;

		ELFImport *import = (sdk_import && GetSDKInfo(symbol->name())) ? sdk_import : NULL;
		if (!import && symbol->version() > 1) {
			std::map<uint16_t, ELFImport *>::const_iterator it = version_map.find(symbol->version());
			if (it != version_map.end())
				import = it->second;
		}
		if (!import) {
			if (!empty_import)
				empty_import = Add("");
			import = empty_import;
		}

		std::vector<uint64_t> address_list;
		std::map<ELFSymbol *, std::vector<uint64_t> >::const_iterator it = symbol_map.find(symbol);
		if (it != symbol_map.end())
			address_list = it->second;
		else
			address_list.push_back(0);

		for (j = 0; j < address_list.size(); j++) {
			import->Add(address_list[j], symbol->name(), symbol);
		}
	}

	ELFImportFunction *func;
	for (k = 0; k < count(); k++) {
		ELFImport *import = item(k);

		if (import->is_sdk()) {
			import->set_is_sdk(true);
			for (i = 0; i < import->count(); i++) {
				func = import->item(i);
				const ImportInfo *import_info = GetSDKInfo(func->name());
				if (import_info) {
					func->set_type(import_info->type);
					if (import_info->options & ioHasCompilationType) {
						func->include_option(ioHasCompilationType);
						func->set_compilation_type(import_info->compilation_type);
						if (import_info->options & ioLockToKey)
							func->include_option(ioLockToKey);
					}
				}
			}
		} else {
			size_t c = _countof(default_info);
			const ImportInfo *import_info = default_info;

			if (import_info) {
				for (i = 0; i < import->count(); i++) {
					func = import->item(i);
					for (j = 0; j < c; j++) {
						if (func->name().compare(import_info[j].name) == 0) {
							func->set_type(import_info[j].type);
							if (import_info[j].options & ioNative)
								func->include_option(ioNative);
							if (import_info[j].options & ioNoReturn)
								func->include_option(ioNoReturn);
							break;
						}
					}
				}
			}
		}
	}
}

void ELFImportList::Pack()
{
	for (size_t i = count(); i > 0; i--) {
		ELFImport *import = item(i - 1);
		if (!import->is_sdk())
			continue;

		for (size_t j = 0; j < import->count(); j++) {
			import->item(j)->symbol()->set_deleted(true);
		}
		delete import;
	}
}

void ELFImportList::WriteToFile(ELFArchitecture &file)
{
	size_t i;
	ELFDirectory *dir;

	ELFDirectoryList *directory_list = file.command_list();
	for (i = directory_list->count(); i > 0; i--) {
		dir = directory_list->item(i - 1);
		if (dir->type() == DT_NEEDED)
			delete dir;
	}

	size_t j = 0;
	for (i = 0; i < count(); i++) {
		ELFImport *import = item(i);
		if (import->name().empty())
			continue;

		dir = new ELFDirectory(directory_list, DT_NEEDED);
		directory_list->InsertObject(j++, dir);
		dir->set_str_value(import->name());
	}
}

ELFImport *ELFImportList::GetImportByName(const std::string &name) const
{
	return reinterpret_cast<ELFImport *>(BaseImportList::GetImportByName(name));
}

/**
 * ELFFixup
 */

ELFFixup::ELFFixup(ELFFixupList *owner, uint64_t address, OperandSize size)
	: BaseFixup(owner), address_(address), size_(size)
{

}

ELFFixup::ELFFixup(ELFFixupList *owner, const ELFFixup &src)
	: BaseFixup(owner, src)
{
	address_ = src.address_;
	size_ = src.size_;
}

ELFFixup *ELFFixup::Clone(IFixupList *owner) const
{
	ELFFixup *fixup = new ELFFixup(reinterpret_cast<ELFFixupList *>(owner), *this);
	return fixup;
}

void ELFFixup::Rebase(IArchitecture &file, uint64_t delta_base)
{ 
	if (!file.AddressSeek(address_))
		return;

	uint64_t value = 0;
	uint64_t pos = file.Tell();
	size_t value_size = OperandSizeToValue(size_);
	value = 0;
	file.Read(&value, value_size);
	value += delta_base;
	file.Seek(pos);
	file.Write(&value, value_size);

	address_ += delta_base;
}

/**
 * ELFFixupList
 */

ELFFixupList::ELFFixupList()
	: BaseFixupList()
{

}

ELFFixupList::ELFFixupList(const ELFFixupList &src)
	: BaseFixupList(src)
{

}

ELFFixupList *ELFFixupList::Clone() const
{
	ELFFixupList *list  = new ELFFixupList(*this);
	return list;
}

ELFFixup *ELFFixupList::item(size_t index) const
{ 
	return reinterpret_cast<ELFFixup *>(BaseFixupList::item(index));
}

IFixup *ELFFixupList::AddDefault(OperandSize cpu_address_size, bool is_code)
{
	ELFFixup *fixup = new ELFFixup(this, 0, cpu_address_size);
	AddObject(fixup);
	return fixup;
}

ELFFixup *ELFFixupList::Add(uint64_t address, OperandSize size)
{
	ELFFixup *fixup = new ELFFixup(this, address, size);
	AddObject(fixup);
	return fixup;
}

void ELFFixupList::WriteToData(Data &data, uint64_t image_base)
{
	size_t i, size_pos;
	ELFFixup *fixup;
	IMAGE_BASE_RELOCATION reloc;
	uint32_t rva, block_rva;
	uint16_t type_offset, empty_offset;

	Sort();

	size_pos = 0;
	reloc.VirtualAddress = 0;
	reloc.SizeOfBlock = 0;

	for (i = 0; i < count(); i++) {
		fixup = item(i);

		rva = static_cast<uint32_t>(fixup->address() - image_base);
		block_rva = rva & 0xfffff000;
		if (reloc.SizeOfBlock == 0 || block_rva != reloc.VirtualAddress) {
			if (reloc.SizeOfBlock > 0) {
				if ((reloc.SizeOfBlock & 3) != 0) {
					data.PushWord(empty_offset);
					reloc.SizeOfBlock += sizeof(empty_offset);
				}
				data.WriteDWord(size_pos, reloc.SizeOfBlock);
			}
			size_pos = data.size() + 4;
			reloc.VirtualAddress = block_rva;
			reloc.SizeOfBlock = sizeof(reloc);
			data.PushBuff(&reloc, sizeof(reloc));
			empty_offset = (static_cast<uint16_t>(rva - block_rva) & 0xf00) << 4 | R_386_NONE;
		}
		type_offset = (static_cast<uint16_t>(rva - block_rva) & 0xfff) << 4 | R_386_RELATIVE;
		data.PushWord(type_offset);
		reloc.SizeOfBlock += sizeof(type_offset);
	}

	if (reloc.SizeOfBlock > 0) {
		if ((reloc.SizeOfBlock & 3) != 0) {
			data.PushWord(empty_offset);
			reloc.SizeOfBlock += sizeof(empty_offset);
		}
		data.WriteDWord(size_pos, reloc.SizeOfBlock);
	}
}

/**
 * ELFExport
 */

ELFExport::ELFExport(IExportList *parent, uint64_t address)
	: BaseExport(parent), symbol_(NULL), address_(address), type_(atNone)
{

}

ELFExport::ELFExport(IExportList *parent, ELFSymbol *symbol)
	: BaseExport(parent), symbol_(symbol), address_(0), type_(atNone)
{
	if (symbol_) {
		address_ = symbol_->address();
		name_ = symbol_->name();
	}
}

ELFExport::ELFExport(IExportList *parent, const ELFExport &src)
	: BaseExport(parent, src)
{
	address_ = src.address_;
	name_ = src.name_;
	symbol_ = src.symbol_;
	type_ = src.type_;
}

ELFExport::~ELFExport()
{
	if (symbol_)
		symbol_->set_bind(STB_LOCAL);
}

ELFExport *ELFExport::Clone(IExportList *parent) const
{
	ELFExport *exp = new ELFExport(parent, *this);
	return exp;
}

std::string ELFExport::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

void ELFExport::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * ELFExportList
 */

ELFExportList::ELFExportList(ELFArchitecture *owner)
	: BaseExportList(owner)
{

}

ELFExportList::ELFExportList(ELFArchitecture *owner, const ELFExportList &src)
	: BaseExportList(owner, src)
{

}

ELFExportList *ELFExportList::Clone(ELFArchitecture *owner) const
{
	ELFExportList *list = new ELFExportList(owner, *this);
	return list;
}

ELFExport *ELFExportList::item(size_t index) const 
{ 
	return reinterpret_cast<ELFExport *>(IExportList::item(index));
}

ELFExport *ELFExportList::Add(ELFSymbol *symbol)
{
	ELFExport *exp = new ELFExport(this, symbol);
	AddObject(exp);
	return exp;
}

IExport *ELFExportList::Add(uint64_t address)
{
	ELFExport *exp = new ELFExport(this, address);
	AddObject(exp);
	return exp;
}

void ELFExportList::ReadFromFile(ELFArchitecture &file)
{
	ELFSymbolList *symbol_list = file.dynsymbol_list();
	for (size_t i = 0; i < symbol_list->count(); i++) {
		ELFSymbol *symbol = symbol_list->item(i);
		if (symbol->section_idx() && symbol->bind() == STB_GLOBAL && (symbol->type() == STT_FUNC || symbol->type() == STT_OBJECT))
			Add(symbol);
	}
}

ELFExport *ELFExportList::GetExportByAddress(uint64_t address) const
{
	return reinterpret_cast<ELFExport *>(BaseExportList::GetExportByAddress(address));
}

void ELFExportList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	static const APIType export_function_types[] = {
		atSetupImage,
		atFreeImage,
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
		atCalcCRC,
		atLoaderData,
		atRuntimeInit
	};

	BaseExportList::ReadFromBuffer(buffer, file);

	assert(count() == _countof(export_function_types));
	for (size_t i = 0; i < count(); i++) {
		item(i)->set_type(export_function_types[i]);
	}
}

/**
 * ELFSymbol
 */

ELFSymbol::ELFSymbol(ELFSymbolList *owner)
	: ISymbol(), owner_(owner), address_(0), info_(STT_FUNC), other_(0), section_idx_(0), name_idx_(0), value_(0), size_(0), 
	is_deleted_(false), version_(0)
{
}

ELFSymbol::ELFSymbol(ELFSymbolList *owner, const ELFSymbol &src)
	: ISymbol(src), owner_(owner)
{
	address_ = src.address_;
	name_ = src.name_;
	info_ = src.info_;
	other_ = src.other_;
	section_idx_ = src.section_idx_;
	name_idx_ = src.name_idx_;
	value_ = src.value_;
	size_ = src.size_;
	is_deleted_ = src.is_deleted_;
	version_ = src.version_;
}

ELFSymbol::~ELFSymbol()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ELFSymbol *ELFSymbol::Clone(ELFSymbolList *owner) const
{
	ELFSymbol *symbol = new ELFSymbol(owner, *this);
	return symbol;
}

void ELFSymbol::ReadFromFile(ELFArchitecture &file, const ELFStringTable &strtab)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Sym hdr;
		file.Read(&hdr, sizeof(hdr));
		name_idx_ = hdr.st_name;
		value_ = hdr.st_value;
		size_ = hdr.st_size;
		info_ = hdr.st_info;
		other_ = hdr.st_other;
		section_idx_ = hdr.st_shndx;
	} else {
		Elf64_Sym hdr;
		file.Read(&hdr, sizeof(hdr));
		name_idx_ = hdr.st_name;
		value_ = hdr.st_value;
		size_ = hdr.st_size;
		info_ = hdr.st_info;
		other_ = hdr.st_other;
		section_idx_ = hdr.st_shndx;
	}

	name_ = strtab.GetString(name_idx_);

	if (type() != STT_TLS && section_idx_)
		address_ = value_;
}

size_t ELFSymbol::WriteToFile(ELFArchitecture &file, ELFStringTable &string_table)
{
	name_idx_ = string_table.AddString(name_);

	size_t res;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Sym hdr;
		hdr.st_name = name_idx_;
		hdr.st_value = static_cast<uint32_t>(value_);
		hdr.st_size = static_cast<uint32_t>(size_);
		hdr.st_info = info_;
		hdr.st_other = other_;
		hdr.st_shndx = section_idx_;
		res = file.Write(&hdr, sizeof(hdr));
	} else {
		Elf64_Sym hdr;
		hdr.st_name = name_idx_;
		hdr.st_value = value_;
		hdr.st_size = size_;
		hdr.st_info = info_;
		hdr.st_other = other_;
		hdr.st_shndx = section_idx_;
		res = file.Write(&hdr, sizeof(hdr));
	}
	return res;
}

void ELFSymbol::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
}

std::string ELFSymbol::display_name(bool show_ret) const
{
	return DemangleName(name()).display_name(show_ret);
}

/**
 * ELFSymbolList
 */

ELFSymbolList::ELFSymbolList(bool is_dynamic)
	: ObjectList<ELFSymbol>(), is_dynamic_(is_dynamic)
{

}

ELFSymbolList::ELFSymbolList(const ELFSymbolList &src)
	: ObjectList<ELFSymbol>(src)
{
	is_dynamic_ = src.is_dynamic_;
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ELFSymbolList *ELFSymbolList::Clone() const
{
	ELFSymbolList *list = new ELFSymbolList(*this);
	return list;
}

ELFSymbol *ELFSymbolList::Add()
{
	ELFSymbol *symbol = new ELFSymbol(this);
	AddObject(symbol);
	return symbol;
}

void ELFSymbolList::ReadFromFile(ELFArchitecture &file)
{
	if (is_dynamic_) {
		ELFDirectory *dir = file.command_list()->GetCommandByType(DT_SYMTAB);
		if (!dir)
			return;

		uint64_t entry_size = file.cpu_address_size() == osDWord ? sizeof(Elf32_Sym) : sizeof(Elf64_Sym);
		uint64_t size;
		ELFDirectory *hash = file.command_list()->GetCommandByType(DT_HASH);
		if (hash) {
			if (!file.AddressSeek(hash->value() + sizeof(uint32_t)))
				throw std::runtime_error("Invalid format");
			size = entry_size * file.ReadDWord();
		}
		else {
			ELFDirectory *strtab = file.command_list()->GetCommandByType(DT_STRTAB);
			if (!strtab)
				throw std::runtime_error("Invalid format");
			size = strtab->value() - dir->value();
		}
		if (!file.AddressSeek(dir->value()))
			throw std::runtime_error("Invalid format");

		for (uint64_t i = 0; i < size; i += entry_size) {
			Add()->ReadFromFile(file, string_table_);
		}

		dir = file.command_list()->GetCommandByType(DT_VERSYM);
		if (dir) {
			if (!file.AddressSeek(dir->value()))
				throw std::runtime_error("Invalid format");
			for (size_t i = 0; i < count(); i++) {
				item(i)->set_version(file.ReadWord());
			}
		}
	}
	else {
		ELFSection *section = file.section_list()->GetSectionByType(SHT_SYMTAB);
		if (!section)
			return;

		string_table_.ReadFromFile(file, *file.section_list()->item(section->link()));
		file.Seek(section->physical_offset());
		for (uint64_t i = 0; i < section->size(); i += section->entry_size()) {
			Add()->ReadFromFile(file, string_table_);
		}
	}
}

static uint32_t elf_hash(const char *name)
{
	const unsigned char* nameu = reinterpret_cast<const unsigned char*>(name);
	uint32_t h = 0;
	unsigned char c;
	while ((c = *nameu++) != '\0')
	{
		h = (h << 4) + c;
		uint32_t g = h & 0xf0000000;
		if (g != 0)
		{
			h ^= g >> 24;
			h ^= g;
		}
	}
	return h;
}

static uint32_t gnu_hash(const char *name)
{
	const unsigned char* nameu = reinterpret_cast<const unsigned char*>(name);
	uint32_t h = 5381;
	unsigned char c;
	while ((c = *nameu++) != '\0')
		h = (h << 5) + h + c;
	return h;
}

static uint32_t compute_bucket_count(size_t symbol_count)
{
	static const uint32_t buckets[] =
	{
		1, 3, 17, 37, 67, 97, 131, 197, 263, 521, 1031, 2053, 4099, 8209,
		16411, 32771, 65537, 131101, 262147
	};

	uint32_t ret = 1;
	for (size_t i = 0; i < _countof(buckets); i++)
	{
		if (symbol_count < buckets[i])
			break;
		ret = buckets[i];
	}
	return ret;
}

size_t ELFSymbolList::WriteHash(ELFArchitecture &file)
{
	std::vector<ELFSymbol *> hashed_symbols;
	std::vector<uint32_t> hashes;
	size_t i;
	ELFSymbol *symbol;

	for (i = 0; i < count(); i++) {
		symbol = item(i);
		if (symbol->need_hash()) {
			hashed_symbols.push_back(symbol);
			hashes.push_back(elf_hash(symbol->name().c_str()));
		}
	}

	uint32_t bucket_count = compute_bucket_count(hashed_symbols.size());

	std::vector<uint32_t> buckets(bucket_count);
	std::vector<uint32_t> chains(count());

	for (i = 0; i < hashed_symbols.size(); i++) {
		symbol = hashed_symbols[i];
		uint32_t bucket = hashes[i] % bucket_count;
		uint32_t index = static_cast<uint32_t>(IndexOf(symbol));
		chains[index] = buckets[bucket];
		buckets[bucket] = index;
	}

	file.WriteDWord(static_cast<uint32_t>(buckets.size()));
	file.WriteDWord(static_cast<uint32_t>(chains.size()));
	for (i = 0; i < buckets.size(); i++) {
		file.WriteDWord(buckets[i]);
	}
	for (i = 0; i < chains.size(); i++) {
		file.WriteDWord(chains[i]);
	}
	return (2 + buckets.size() + chains.size()) * sizeof(uint32_t);
}

template <typename T>
size_t ELFSymbolList::WriteGNUHash(ELFArchitecture &file)
{
	std::vector<ELFSymbol *> hashed_symbols;
	std::vector<ELFSymbol *> unhashed_symbols;
	std::vector<uint32_t> hashes;
	size_t i;
	ELFSymbol *symbol;
	std::map<size_t, ELFSymbol *> sym_index_map;

	for (i = 0; i < count(); i++) {
		symbol = item(i);
		if (symbol->need_hash()) {
			hashed_symbols.push_back(symbol);
			hashes.push_back(gnu_hash(symbol->name().c_str()));
		} else {
			unhashed_symbols.push_back(symbol);
		}
	}

	uint32_t symbol_base = 0;
	for (i = 0; i < unhashed_symbols.size(); i++) {
		symbol = unhashed_symbols[i];
		RemoveObject(symbol);
		InsertObject(symbol_base++, symbol);
	}

	size_t symbol_count = hashed_symbols.size();
	uint32_t bucket_count = compute_bucket_count(symbol_count);

	uint32_t maskbitslog2 = 1;
	for (i = symbol_count >> 1; i != 0; i >>= 1)
		++maskbitslog2;
	if (maskbitslog2 < 3)
		maskbitslog2 = 5;
	else if (((static_cast<size_t>(1U) << (maskbitslog2 - 2)) & symbol_count) != 0)
		maskbitslog2 += 3;
	else
		maskbitslog2 += 2;
	
	uint32_t shift1;
	if (sizeof(T) == 4)
		shift1 = 5;
	else
	{
		if (maskbitslog2 == 5)
			maskbitslog2 = 6;
		shift1 = 6;
	}
	uint32_t mask = (1U << shift1) - 1U;
	uint32_t shift2 = maskbitslog2;
	uint32_t maskbits = 1U << maskbitslog2;
	uint32_t maskwords = 1U << (maskbitslog2 - shift1);

	std::vector<T> bitmask(maskwords);
	std::vector<uint32_t> counts(bucket_count);
	std::vector<uint32_t> indx(bucket_count);

	for (i = 0; i < symbol_count; i++) {
		++counts[hashes[i] % bucket_count];
	}
	uint32_t cnt = symbol_base;
	for (i = 0; i < bucket_count; ++i) {
		indx[i] = cnt;
		cnt += counts[i];
	}
	std::vector<uint32_t> buckets(bucket_count);
	for (i = 0; i < bucket_count; i++) {
		buckets[i] = counts[i] ? indx[i] : 0;
	}
	std::vector<uint32_t> chains(symbol_count);
	for (i = 0; i < symbol_count; ++i)
	{
		symbol = hashed_symbols[i];
		uint32_t hashval = hashes[i];

		uint32_t bucket = hashval % bucket_count;
		uint32_t val = ((hashval >> shift1) & ((maskbits >> shift1) - 1));
		bitmask[val] |= (static_cast<T>(1U)) << (hashval & mask);
		bitmask[val] |= (static_cast<T>(1U)) << ((hashval >> shift2) & mask);
		val = hashval & ~ 1U;
		if (counts[bucket] == 1)
			val |= 1;
		chains[indx[bucket] - symbol_base] = val;
		--counts[bucket];
		sym_index_map[indx[bucket]] = symbol;
		++indx[bucket];
	}

	for (std::map<size_t, ELFSymbol*>::const_iterator it = sym_index_map.begin(); it != sym_index_map.end(); it++) {
		i = it->first;
		symbol = it->second;
		if (item(i) != symbol) {
			RemoveObject(symbol);
			InsertObject(i, symbol);
		}
	}

	file.WriteDWord(bucket_count);
	file.WriteDWord(symbol_base);
	file.WriteDWord(maskwords);
	file.WriteDWord(shift2);
	for (i = 0; i < maskwords; i++) {
		file.Write(&bitmask[i], sizeof(bitmask[i]));
	}
	for (i = 0; i < bucket_count; i++) {
		file.WriteDWord(buckets[i]);
	}
	for (i = 0; i < symbol_count; i++) {
		file.WriteDWord(chains[i]);
	}
	return (4 + bucket_count + symbol_count) * sizeof(uint32_t) + maskbits / 8;
}

size_t ELFSymbolList::WriteVersym(ELFArchitecture &file)
{
	size_t res = 0;
	for (size_t i = 0; i < count(); i++) {
		res += file.WriteWord(item(i)->version());
	}
	return res;
}

void ELFSymbolList::WriteToFile(ELFArchitecture &file)
{
	uint64_t pos;
	uint64_t address;
	size_t size;
	ELFSection *section;

	string_table_.clear();

	if (is_dynamic_) {
		ELFDirectory *symtab = file.command_list()->GetCommandByType(DT_SYMTAB);
		if (!symtab)
			return;

		ELFDirectory *hash = file.command_list()->GetCommandByType(DT_GNU_HASH);
		if (hash) {
			section = file.section_list()->GetSectionByType(SHT_GNU_HASH);
			pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
			address = file.AddressTell();
			size = (file.cpu_address_size() == osDWord) ? WriteGNUHash<uint32_t>(file) : WriteGNUHash<uint64_t>(file);
			hash->set_value(address);

			if (section) {
				section->Rebase(address - section->address());
				section->set_physical_offset(static_cast<uint32_t>(pos));
				section->set_size(static_cast<uint32_t>(size));
			}
		}

		hash = file.command_list()->GetCommandByType(DT_HASH);
		if (hash) {
			section = file.section_list()->GetSectionByType(SHT_HASH);
			pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
			address = file.AddressTell();
			size = WriteHash(file);
			hash->set_value(address);

			if (section) {
				section->Rebase(address - section->address());
				section->set_physical_offset(static_cast<uint32_t>(pos));
				section->set_size(static_cast<uint32_t>(size));
			}
		}

		section = file.section_list()->GetSectionByType(SHT_DYNSYM);
		pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
		address = file.AddressTell();
		size = 0;
		for (size_t i = 0; i < count(); i++) {
			size += item(i)->WriteToFile(file, string_table_);
		}
		symtab->set_value(address);

		if (section) {
			if (section->address())
				section->Rebase(address - section->address());
			section->set_physical_offset(static_cast<uint32_t>(pos));
			section->set_size(static_cast<uint32_t>(size));
		}

		file.command_list()->WriteStrings(string_table_);
		file.verdef_list()->WriteStrings(string_table_);
		file.verneed_list()->WriteStrings(string_table_);

		pos = file.Tell();
		address = file.AddressTell();
		size = string_table_.WriteToFile(file);

		if (section) {
			section = file.section_list()->item(section->link());
			if (section->address())
				section->Rebase(address - section->address());
			section->set_physical_offset(static_cast<uint32_t>(pos));
			section->set_size(static_cast<uint32_t>(size));
		}

		ELFDirectory *dir = file.command_list()->GetCommandByType(DT_STRTAB);
		if (!dir)
			dir = file.command_list()->Add(DT_STRTAB);
		dir->set_value(address);

		dir = file.command_list()->GetCommandByType(DT_STRSZ);
		if (!dir)
			dir = file.command_list()->Add(DT_STRSZ);
		dir->set_value(static_cast<uint32_t>(size));

		dir = file.command_list()->GetCommandByType(DT_VERSYM);
		if (dir) {
			section = file.section_list()->GetSectionByType(SHT_GNU_versym);
			pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
			address = file.AddressTell();
			size = WriteVersym(file);
			dir->set_value(address);

			if (section) {
				section->Rebase(address - section->address());
				section->set_physical_offset(static_cast<uint32_t>(pos));
				section->set_size(static_cast<uint32_t>(size));
			}
		}
	}
	else {
		ELFSection *section = file.section_list()->GetSectionByType(SHT_SYMTAB);
		if (!section)
			return;

		pos = section->alignment() > 1 ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
		address = file.AddressTell();
		size = 0;
		for (size_t i = 0; i < count(); i++) {
			size += item(i)->WriteToFile(file, string_table_);
		}
		if (section->address())
			section->Rebase(address - section->address());
		section->set_physical_offset(static_cast<uint32_t>(pos));
		section->set_size(static_cast<uint32_t>(size));

		pos = file.Tell();
		address = file.AddressTell();
		size = string_table_.WriteToFile(file);
		section = file.section_list()->item(section->link());
		if (section->address())
			section->Rebase(address - section->address());
		section->set_physical_offset(static_cast<uint32_t>(pos));
		section->set_size(static_cast<uint32_t>(size));
	}
}

void ELFSymbolList::Pack()
{
	for (size_t i = count(); i > 0 ; i--) {
		ELFSymbol *symbol = item(i - 1);
		if (symbol->is_deleted())
			delete symbol;
	}
}

void ELFSymbolList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * ELFVernaux
 */

ELFVernaux::ELFVernaux(ELFVerneed *owner)
	: IObject(), owner_(owner), hash_(0), flags_(0), other_(0), next_(0), name_pos_(0)
{

}

ELFVernaux::ELFVernaux(ELFVerneed *owner, const ELFVernaux &src)
	: IObject(), owner_(owner), next_(0), name_pos_(0)
{
	hash_ = src.hash_;
	flags_ = src.flags_;
	other_ = src.other_;
	name_ = src.name_;
}

ELFVernaux::~ELFVernaux()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ELFVernaux *ELFVernaux::Clone(ELFVerneed *owner) const
{
	ELFVernaux *vernaux = new ELFVernaux(owner, *this);
	return vernaux;
}

void ELFVernaux::ReadFromFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Vernaux vernaux;
		file.Read(&vernaux, sizeof(vernaux));
		hash_ = vernaux.vna_hash;
		flags_ = vernaux.vna_flags;
		other_ = vernaux.vna_other;
		name_ = file.dynsymbol_list()->string_table()->GetString(vernaux.vna_name);
		next_ = vernaux.vna_next;
	} else {
		Elf64_Vernaux vernaux;
		file.Read(&vernaux, sizeof(vernaux));
		hash_ = vernaux.vna_hash;
		flags_ = vernaux.vna_flags;
		other_ = vernaux.vna_other;
		name_ = file.dynsymbol_list()->string_table()->GetString(vernaux.vna_name);
		next_ = vernaux.vna_next;
	}
}

size_t ELFVernaux::WriteToFile(ELFArchitecture &file)
{
	size_t res;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Vernaux vernaux;
		vernaux.vna_hash = hash_;
		vernaux.vna_flags = flags_;
		vernaux.vna_other = other_;
		vernaux.vna_name = name_pos_;
		vernaux.vna_next = (owner_ && owner_->last() != this) ? sizeof(vernaux) : 0;
		res = file.Write(&vernaux, sizeof(vernaux));
	} else {
		Elf64_Vernaux vernaux;
		vernaux.vna_hash = hash_;
		vernaux.vna_flags = flags_;
		vernaux.vna_other = other_;
		vernaux.vna_name = name_pos_;
		vernaux.vna_next = (owner_ && owner_->last() != this) ? sizeof(vernaux) : 0;
		res = file.Write(&vernaux, sizeof(vernaux));
	}
	return res;
}

void ELFVernaux::WriteStrings(ELFStringTable &string_table)
{
	name_pos_ = string_table.AddString(name_);
}

/**
 * ELFVerneed
 */

ELFVerneed::ELFVerneed(ELFVerneedList *owner)
	: ObjectList<ELFVernaux>(), owner_(owner), version_(0), next_(0), file_pos_(0)
{

}

ELFVerneed::ELFVerneed(ELFVerneedList *owner, const ELFVerneed &src)
	: ObjectList<ELFVernaux>(src), owner_(owner), next_(0), file_pos_(0)
{
	version_ = src.version_;
	file_ = src.file_;
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ELFVerneed::~ELFVerneed()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ELFVerneed *ELFVerneed::Clone(ELFVerneedList *owner) const
{
	ELFVerneed *verneed = new ELFVerneed(owner, *this);
	return verneed;
}

ELFVernaux *ELFVerneed::Add()
{
	ELFVernaux *vernaux = new ELFVernaux(this);
	AddObject(vernaux);
	return vernaux;
}

void ELFVerneed::ReadFromFile(ELFArchitecture &file)
{
	uint64_t pos = file.Tell();
	size_t count;
	uint32_t offset;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verneed verneed;
		file.Read(&verneed, sizeof(verneed));
		version_ = verneed.vn_version;
		count = verneed.vn_cnt;
		file_ = file.dynsymbol_list()->string_table()->GetString(verneed.vn_file);
		offset = verneed.vn_aux;
		next_ = verneed.vn_next;
	} else {
		Elf64_Verneed verneed;
		file.Read(&verneed, sizeof(verneed));
		version_ = verneed.vn_version;
		count = verneed.vn_cnt;
		file_ = file.dynsymbol_list()->string_table()->GetString(verneed.vn_file);
		offset = verneed.vn_aux;
		next_ = verneed.vn_next;
	}

	for (size_t i = 0; i < count; i++) {
		file.Seek(pos + offset);
		ELFVernaux *vernaux = Add();
		vernaux->ReadFromFile(file);
		if (!vernaux->next())
			break;

		offset += vernaux->next();
	}
}

size_t ELFVerneed::WriteToFile(ELFArchitecture &file)
{
	size_t res;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verneed verneed;
		verneed.vn_version = version_;
		verneed.vn_cnt = static_cast<uint16_t>(count());
		verneed.vn_file = file_pos_;
		verneed.vn_aux = sizeof(verneed);
		verneed.vn_next = (owner_ && owner_->last() != this) ? static_cast<uint32_t>(sizeof(Elf32_Verneed) + count() * sizeof(Elf32_Vernaux)) : 0;
		res = file.Write(&verneed, sizeof(verneed));
	} else {
		Elf64_Verneed verneed;
		verneed.vn_version = version_;
		verneed.vn_cnt = static_cast<uint16_t>(count());
		verneed.vn_file = file_pos_;
		verneed.vn_aux = sizeof(verneed);
		verneed.vn_next = (owner_ && owner_->last() != this) ? static_cast<uint32_t>(sizeof(Elf64_Verneed) + count() * sizeof(Elf64_Vernaux)) : 0;
		res = file.Write(&verneed, sizeof(verneed));
	}

	for (size_t i = 0; i < count(); i++) {
		res += item(i)->WriteToFile(file);
	}
	return res;
}

void ELFVerneed::WriteStrings(ELFStringTable &string_table)
{
	file_pos_ = string_table.AddString(file_);
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteStrings(string_table);
	}
}

ELFVernaux *ELFVerneed::GetVernaux(uint32_t hash) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFVernaux *res = item(i);
		if (res->hash() == hash)
			return res;
	}
	return NULL;
}

/**
 * ELFVerneedList
 */

ELFVerneedList::ELFVerneedList()
	: ObjectList<ELFVerneed>()
{

}

ELFVerneedList::ELFVerneedList(const ELFVerneedList &src)
	: ObjectList<ELFVerneed>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ELFVerneedList *ELFVerneedList::Clone() const
{
	ELFVerneedList *list = new ELFVerneedList(*this);
	return list;
}

ELFVerneed *ELFVerneedList::Add()
{
	ELFVerneed *verneed = new ELFVerneed(this);
	AddObject(verneed);
	return verneed;
}

void ELFVerneedList::ReadFromFile(ELFArchitecture &file)
{
	ELFDirectory *dir = file.command_list()->GetCommandByType(DT_VERNEED);
	if (!dir)
		return;

	ELFDirectory *num = file.command_list()->GetCommandByType(DT_VERNEEDNUM);
	if (!num || !file.AddressSeek(dir->value()))
		throw std::runtime_error("Invalid format");

	uint32_t offset = 0;
	uint64_t pos = file.Tell();
	for (uint64_t i = 0; i < num->value(); i++) {
		file.Seek(pos + offset);

		ELFVerneed *verneed = Add();
		verneed->ReadFromFile(file);
		if (!verneed->next())
			break;

		offset += verneed->next();
	}
}

void ELFVerneedList::WriteToFile(ELFArchitecture &file)
{
	ELFDirectory *dir = file.command_list()->GetCommandByType(DT_VERNEED);
	if (!dir)
		return;

	ELFDirectory *num = file.command_list()->GetCommandByType(DT_VERNEEDNUM);
	if (!num)
		num = file.command_list()->Add(DT_VERNEEDNUM);

	ELFSection *section = file.section_list()->GetSectionByType(SHT_GNU_verneed);

	size_t i;
	uint64_t pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
	uint64_t address = file.AddressTell();
	size_t size = 0;
	for (i = 0; i < count(); i++) {
		size += item(i)->WriteToFile(file);
	}
	dir->set_value(address);
	num->set_value(count());

	if (section) {
		if (section->address())
			section->Rebase(address - section->address());
		section->set_physical_offset(static_cast<uint32_t>(pos));
		section->set_size(static_cast<uint32_t>(size));
		section->set_info(static_cast<uint32_t>(count()));
	}
}

void ELFVerneedList::WriteStrings(ELFStringTable &string_table)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteStrings(string_table);
	}
}

ELFVerneed *ELFVerneedList::GetVerneed(const std::string &name) const
{
	for (size_t i = 0; i < count(); i++) {
		ELFVerneed *verneed = item(i);
		if (verneed->file() == name)
			return verneed;
	}
	return NULL;
}

/**
* ELFVerdaux
*/

ELFVerdaux::ELFVerdaux(ELFVerdef *owner)
	: IObject(), owner_(owner), next_(0), name_pos_(0)
{

}

ELFVerdaux::ELFVerdaux(ELFVerdef *owner, const ELFVerdaux &src)
	: IObject(), owner_(owner), next_(0), name_pos_(0)
{
	name_ = src.name_;
}

ELFVerdaux::~ELFVerdaux()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ELFVerdaux *ELFVerdaux::Clone(ELFVerdef *owner) const
{
	ELFVerdaux *verdaux = new ELFVerdaux(owner, *this);
	return verdaux;
}

void ELFVerdaux::ReadFromFile(ELFArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verdaux verdaux;
		file.Read(&verdaux, sizeof(verdaux));
		name_ = file.dynsymbol_list()->string_table()->GetString(verdaux.vda_name);
		next_ = verdaux.vda_next;
	}
	else {
		Elf64_Verdaux verdaux;
		file.Read(&verdaux, sizeof(verdaux));
		name_ = file.dynsymbol_list()->string_table()->GetString(verdaux.vda_name);
		next_ = verdaux.vda_next;
	}
}

size_t ELFVerdaux::WriteToFile(ELFArchitecture &file)
{
	size_t res;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verdaux verdaux;
		verdaux.vda_name = name_pos_;
		verdaux.vda_next = (owner_ && owner_->last() != this) ? sizeof(verdaux) : 0;
		res = file.Write(&verdaux, sizeof(verdaux));
	}
	else {
		Elf64_Verdaux verdaux;
		verdaux.vda_name = name_pos_;
		verdaux.vda_next = (owner_ && owner_->last() != this) ? sizeof(verdaux) : 0;
		res = file.Write(&verdaux, sizeof(verdaux));
	}
	return res;
}

void ELFVerdaux::WriteStrings(ELFStringTable &string_table)
{
	name_pos_ = string_table.AddString(name_);
}

/**
* ELFVerdef
*/

ELFVerdef::ELFVerdef(ELFVerdefList *owner)
	: ObjectList<ELFVerdaux>(), owner_(owner), version_(0), next_(0), flags_(0), ndx_(0), hash_(0)
{

}

ELFVerdef::ELFVerdef(ELFVerdefList *owner, const ELFVerdef &src)
	: ObjectList<ELFVerdaux>(src), owner_(owner), next_(0)
{
	version_ = src.version_;
	flags_ = src.flags_;
	ndx_ = src.ndx_;
	hash_ = src.hash_;
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ELFVerdef::~ELFVerdef()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ELFVerdef *ELFVerdef::Clone(ELFVerdefList *owner) const
{
	ELFVerdef *verdef = new ELFVerdef(owner, *this);
	return verdef;
}

ELFVerdaux *ELFVerdef::Add()
{
	ELFVerdaux *verdaux = new ELFVerdaux(this);
	AddObject(verdaux);
	return verdaux;
}

void ELFVerdef::ReadFromFile(ELFArchitecture &file)
{
	uint64_t pos = file.Tell();
	size_t count;
	uint32_t offset;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verdef verdef;
		file.Read(&verdef, sizeof(verdef));
		version_ = verdef.vd_version;
		flags_ = verdef.vd_flags;
		ndx_ = verdef.vd_ndx;
		hash_ = verdef.vd_hash;
		count = verdef.vd_cnt;
		offset = verdef.vd_aux;
		next_ = verdef.vd_next;
	}
	else {
		Elf64_Verdef verdef;
		file.Read(&verdef, sizeof(verdef));
		version_ = verdef.vd_version;
		flags_ = verdef.vd_flags;
		ndx_ = verdef.vd_ndx;
		hash_ = verdef.vd_hash;
		count = verdef.vd_cnt;
		offset = verdef.vd_aux;
		next_ = verdef.vd_next;
	}

	for (size_t i = 0; i < count; i++) {
		file.Seek(pos + offset);
		ELFVerdaux *verdaux = Add();
		verdaux->ReadFromFile(file);
		if (!verdaux->next())
			break;

		offset += verdaux->next();
	}
}

size_t ELFVerdef::WriteToFile(ELFArchitecture &file)
{
	size_t res;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Verdef verdef;
		verdef.vd_version = version_;
		verdef.vd_flags = flags_;
		verdef.vd_ndx = ndx_;
		verdef.vd_hash = hash_;
		verdef.vd_cnt = static_cast<uint16_t>(count());
		verdef.vd_aux = sizeof(verdef);
		verdef.vd_next = (owner_ && owner_->last() != this) ? static_cast<uint32_t>(sizeof(Elf32_Verdef) + count() * sizeof(Elf32_Verdaux)) : 0;
		res = file.Write(&verdef, sizeof(verdef));
	}
	else {
		Elf64_Verdef verdef;
		verdef.vd_version = version_;
		verdef.vd_flags = flags_;
		verdef.vd_ndx = ndx_;
		verdef.vd_hash = hash_;
		verdef.vd_cnt = static_cast<uint16_t>(count());
		verdef.vd_aux = sizeof(verdef);
		verdef.vd_next = (owner_ && owner_->last() != this) ? static_cast<uint32_t>(sizeof(Elf64_Verdef) + count() * sizeof(Elf64_Verdaux)) : 0;
		res = file.Write(&verdef, sizeof(verdef));
	}

	for (size_t i = 0; i < count(); i++) {
		res += item(i)->WriteToFile(file);
	}
	return res;
}

void ELFVerdef::WriteStrings(ELFStringTable &string_table)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteStrings(string_table);
	}
}

/**
* ELFVerdefList
*/

ELFVerdefList::ELFVerdefList()
	: ObjectList<ELFVerdef>()
{

}

ELFVerdefList::ELFVerdefList(const ELFVerdefList &src)
	: ObjectList<ELFVerdef>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

ELFVerdefList *ELFVerdefList::Clone() const
{
	ELFVerdefList *list = new ELFVerdefList(*this);
	return list;
}

ELFVerdef *ELFVerdefList::Add()
{
	ELFVerdef *verdef = new ELFVerdef(this);
	AddObject(verdef);
	return verdef;
}

void ELFVerdefList::ReadFromFile(ELFArchitecture &file)
{
	ELFDirectory *dir = file.command_list()->GetCommandByType(DT_VERDEF);
	if (!dir)
		return;

	ELFDirectory *num = file.command_list()->GetCommandByType(DT_VERDEFNUM);
	if (!num || !file.AddressSeek(dir->value()))
		throw std::runtime_error("Invalid format");

	uint32_t offset = 0;
	uint64_t pos = file.Tell();
	for (uint64_t i = 0; i < num->value(); i++) {
		file.Seek(pos + offset);

		ELFVerdef *verdef = Add();
		verdef->ReadFromFile(file);
		if (!verdef->next())
			break;

		offset += verdef->next();
	}
}

void ELFVerdefList::WriteToFile(ELFArchitecture &file)
{
	ELFDirectory *dir = file.command_list()->GetCommandByType(DT_VERDEF);
	if (!dir)
		return;

	ELFDirectory *num = file.command_list()->GetCommandByType(DT_VERDEFNUM);
	if (!num)
		num = file.command_list()->Add(DT_VERDEFNUM);

	ELFSection *section = file.section_list()->GetSectionByType(SHT_GNU_verdef);

	size_t i;
	uint64_t pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
	uint64_t address = file.AddressTell();
	size_t size = 0;
	for (i = 0; i < count(); i++) {
		size += item(i)->WriteToFile(file);
	}
	dir->set_value(address);
	num->set_value(count());

	if (section) {
		if (section->address())
			section->Rebase(address - section->address());
		section->set_physical_offset(static_cast<uint32_t>(pos));
		section->set_size(static_cast<uint32_t>(size));
		section->set_info(static_cast<uint32_t>(count()));
	}
}

void ELFVerdefList::WriteStrings(ELFStringTable &string_table)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteStrings(string_table);
	}
}

/**
 * ELFRelocation
 */

ELFRelocation::ELFRelocation(ELFRelocationList *owner, bool is_rela, uint64_t address, OperandSize size, uint32_t type, ELFSymbol *symbol, uint64_t addend)
	: BaseRelocation(owner, address, size), is_rela_(is_rela), type_(type), symbol_(symbol), addend_(addend), value_(0)
{

}

ELFRelocation::ELFRelocation(ELFRelocationList *owner, const ELFRelocation &src)
	: BaseRelocation(owner, src)
{
	is_rela_ = src.is_rela_;
	type_ = src.type_;
	symbol_ = src.symbol_;
	addend_ = src.addend_;
	value_ = src.value_;
}

ELFRelocation *ELFRelocation::Clone(IRelocationList *owner) const
{
	ELFRelocation *relocation = new ELFRelocation(reinterpret_cast<ELFRelocationList*>(owner), *this);
	return relocation;
}

size_t ELFRelocation::WriteToFile(ELFArchitecture &file)
{
	size_t res = 0;
	if (file.cpu_address_size() == osDWord) {
		Elf32_Rel rel;
		rel.r_offset = static_cast<uint32_t>(address());
		rel.r_info = (static_cast<uint32_t>(file.dynsymbol_list()->IndexOf(symbol_)) << 8) | type_;
		res += file.Write(&rel, sizeof(rel));
		if (is_rela_)
			res += file.WriteDWord(static_cast<uint32_t>(addend_));
	} else {
		Elf64_Rel rel;
		rel.r_offset = address();
		rel.r_type = type_;
		rel.r_ssym = static_cast<uint32_t>(file.dynsymbol_list()->IndexOf(symbol_));
		res += file.Write(&rel, sizeof(rel));
		if (is_rela_)
			res += file.WriteQWord(addend_);
	}
	return res;
}

void ELFRelocation::Rebase(IArchitecture &file, uint64_t delta_base)
{ 
	if (!file.AddressSeek(address()))
		return;

	uint64_t value;
	uint64_t pos = file.Tell();
	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	switch (type_) {
	case R_386_JMP_SLOT:
		value = 0;
		file.Read(&value, value_size);
		if (value) {
			value += delta_base;
			file.Seek(pos);
			file.Write(&value, value_size);
		}
		break;
	case R_386_RELATIVE:
		if (is_rela_) {
			value = addend_;
		} else {
			value = 0;
			file.Read(&value, value_size);
			file.Seek(pos);
		}
		value += delta_base;
		file.Write(&value, value_size);
		break;
	}

	BaseRelocation::Rebase(file, delta_base);
}

/**
 * ELFRelocationList
 */

ELFRelocationList::ELFRelocationList()
	: BaseRelocationList()
{
	
}

ELFRelocationList::ELFRelocationList(const ELFRelocationList &src)
	: BaseRelocationList(src)
{

}

ELFRelocationList *ELFRelocationList::Clone() const
{
	ELFRelocationList *list = new ELFRelocationList(*this);
	return list;
}

ELFRelocation *ELFRelocationList::item(size_t index) const
{
	return reinterpret_cast<ELFRelocation *>(BaseRelocationList::item(index));
}

ELFRelocation *ELFRelocationList::GetRelocationByAddress(uint64_t address) const
{
	return reinterpret_cast<ELFRelocation *>(BaseRelocationList::GetRelocationByAddress(address));
}

ELFRelocation *ELFRelocationList::Add(bool is_rela, uint64_t address, OperandSize size, uint32_t type, ELFSymbol *symbol, uint64_t addend)
{
	ELFRelocation *relocation = new ELFRelocation(this, is_rela, address, size, type, symbol, addend);
	AddObject(relocation);
	return relocation;
}

void ELFRelocationList::ReadFromFile(ELFArchitecture &file)
{
	const uint32_t dir_types[3][2] = {{DT_REL, DT_RELSZ}, {DT_RELA, DT_RELASZ}, {DT_JMPREL, DT_PLTRELSZ}};
	ELFDirectory *plt_rel = file.command_list()->GetCommandByType(DT_PLTREL);

	size_t i;
	OperandSize cpu_address_size = file.cpu_address_size();
	for (i = 0; i < _countof(dir_types); i++) {
		ELFDirectory *dir = file.command_list()->GetCommandByType(dir_types[i][0]);
		if (!dir)
			continue;

		ELFDirectory *sz = file.command_list()->GetCommandByType(dir_types[i][1]);
		if (!sz || !file.AddressSeek(dir->value()))
			throw std::runtime_error("Invalid format");

		size_t entry_size;
		bool is_rela;
		if (dir->type() == DT_JMPREL) {
			if (!plt_rel)
				throw std::runtime_error("Invalid format");
			is_rela = (plt_rel->value() == DT_RELA);
		} else
			is_rela = (dir->type() == DT_RELA);
		if (is_rela)
			entry_size = file.cpu_address_size() == osDWord ? sizeof(Elf32_Rela) : sizeof(Elf64_Rela);
		else
			entry_size = file.cpu_address_size() == osDWord ? sizeof(Elf32_Rel) : sizeof(Elf64_Rel);
		for (uint64_t j = 0; j < sz->value(); j += entry_size) {
			uint64_t address;
			uint32_t type;
			ELFSymbol *symbol;
			uint64_t addend = 0;
			if (cpu_address_size == osDWord) {
				Elf32_Rel rel;
				file.Read(&rel, sizeof(rel));
				address = rel.r_offset;
				type = static_cast<uint8_t>(rel.r_info);
				symbol = (type == R_386_IRELATIVE) ? NULL : file.dynsymbol_list()->item(rel.r_info >> 8);
				if (is_rela)
					addend = file.ReadDWord();
			} else {
				Elf64_Rel rel;
				file.Read(&rel, sizeof(rel));
				address = rel.r_offset;
				type = rel.r_type;
				symbol = (type == R_X86_64_IRELATIVE) ? NULL : file.dynsymbol_list()->item(rel.r_ssym);
				if (is_rela)
					addend = file.ReadQWord();
			}
			if (type == R_386_RELATIVE)
				file.fixup_list()->Add(address, cpu_address_size);
			else
				Add(is_rela, address, cpu_address_size, type, symbol, addend);
		}
	}

	if (cpu_address_size == osDWord) {
		for (i = 0; i < count(); i++) {
			ELFRelocation *reloc = item(i);
			if (file.AddressSeek(reloc->address())) {
				switch (reloc->size()) {
				case osDWord:
					reloc->set_value(file.ReadDWord());
					break;
				case osQWord:
					reloc->set_value(file.ReadQWord());
					break;
				}
			}
		}
	}
}

void ELFRelocationList::WriteToFile(ELFArchitecture &file)
{
	size_t i, j, k;
	ELFRelocation *reloc;
	ELFDirectory *dir;
	std::vector<ELFRelocation *> reloc_list[3];
	ELFSection *section_list[3] = {};
	const uint32_t dir_types[3][2] = {{DT_REL, DT_RELSZ}, {DT_RELA, DT_RELASZ}, {DT_JMPREL, DT_PLTRELSZ}};
	const uint32_t rel_dir_types[2] = {DT_RELCOUNT, DT_RELACOUNT};

	for (i = 0; i < count(); i++) {
		reloc = item(i);
		if (reloc->type() == R_386_JMP_SLOT)
			j = 2;
		else
			j = reloc->is_rela() ? 1 : 0;
		reloc_list[j].push_back(reloc);
	}

	if (file.fixup_list()->count()) {
		// convert fixups into relocations
		std::vector<ELFRelocation *> fixup_list;
		uint64_t pos = file.Tell();
		bool is_rela = reloc_list[1].size() > 0;
		for (i = 0; i < file.fixup_list()->count(); i++) {
			ELFFixup *fixup = file.fixup_list()->item(i);
			uint64_t addend = 0;
			if (is_rela && file.AddressSeek(fixup->address()))
				addend = (file.cpu_address_size() == osDWord) ? file.ReadDWord() : file.ReadQWord();
			reloc = Add(is_rela, fixup->address(), file.cpu_address_size(), R_386_RELATIVE, file.dynsymbol_list()->item(0), addend);
			fixup_list.push_back(reloc);
		}
		file.Seek(pos);
		j = is_rela ? 1 : 0;
		reloc_list[j].insert(reloc_list[j].begin(), fixup_list.begin(), fixup_list.end());

		dir = file.command_list()->GetCommandByType(rel_dir_types[j]);
		if (!dir)
			dir = file.command_list()->Add(rel_dir_types[j]);
		dir->set_value(fixup_list.size());
	} else {
		for (i = 0; i < _countof(rel_dir_types); i++) {
			dir = file.command_list()->GetCommandByType(rel_dir_types[i]);
			if (dir)
				delete dir;
		}
	}

	dir = file.command_list()->GetCommandByType(DT_JMPREL);
	ELFSection *jmprel_section = dir ? file.section_list()->GetSectionByAddress(dir->address()) : NULL;
	for (i = 0; i < file.section_list()->count(); i++) {
		ELFSection *section = file.section_list()->item(i);
		if (section->type() != SHT_REL && section->type() != SHT_RELA)
			continue;

		if (jmprel_section && section == jmprel_section)
			j = 2;
		else
			j = (section->type() == SHT_RELA) ? 1 : 0;
		section_list[j] = section;
	}

	for (i = 0; i < _countof(reloc_list); i++) {
		ELFSection *section = section_list[i];
		size_t size = 0;
		uint64_t pos = (section && section->alignment() > 1) ? file.Resize(AlignValue(file.Tell(), section->alignment())) : file.Tell();
		uint64_t address = file.AddressTell();
		for (k = 0; k < reloc_list[i].size(); k++) {
			reloc = reloc_list[i].at(k);
			size += reloc->WriteToFile(file);
		}
		if (section) {
			section->Rebase(address - section->address());
			section->set_physical_offset(static_cast<uint32_t>(pos));
			section->set_size(static_cast<uint32_t>(size));
		}

		for (k = 0; k < 2; k++) {
			dir = file.command_list()->GetCommandByType(dir_types[i][k]);
			if (dir) {
				if (size)
					dir->set_value(k == 0 ? address : size);
				else
					delete dir;
			}
		}
	}
}

void ELFRelocationList::Pack()
{
	for (size_t i = count(); i > 0 ; i--) {
		ELFRelocation *reloc = item(i - 1);
		if (reloc->symbol() && reloc->symbol()->is_deleted())
			delete reloc;
	}
}

/**
 * ELFRuntimeFunction
 */

ELFRuntimeFunction::ELFRuntimeFunction(ELFRuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address,
		CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions)
	: BaseRuntimeFunction(owner), address_(address), begin_(begin), end_(end), unwind_address_(unwind_address), cie_(cie),
		call_frame_instructions_(call_frame_instructions)
{

}

ELFRuntimeFunction::ELFRuntimeFunction(ELFRuntimeFunctionList *owner, const ELFRuntimeFunction &src)
	: BaseRuntimeFunction(owner)
{
	address_ = src.address_;
	begin_ = src.begin_;
	end_ = src.end_;
	unwind_address_ = src.unwind_address_;
	cie_ = src.cie_;
	call_frame_instructions_ = src.call_frame_instructions_;
}

ELFRuntimeFunction *ELFRuntimeFunction::Clone(IRuntimeFunctionList *owner) const
{
	ELFRuntimeFunction *func = new ELFRuntimeFunction(reinterpret_cast<ELFRuntimeFunctionList *>(owner), *this);
	return func;
}

void ELFRuntimeFunction::Parse(IArchitecture &file, IFunction &dest)
{
	if (!file.AddressSeek(address_) || dest.GetCommandByAddress(address_))
		return;

	uint64_t address = address_;
	IntelFunction &func = reinterpret_cast<IntelFunction &>(dest);

	size_t c = func.count();
	IntelCommand *command;
	uint64_t value;
	size_t pos;
	CommandLink *link;
	FunctionInfo *info;
	std::vector<ICommand *> unwind_opcodes;

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "FDE Length"));
	uint32_t fde_length = static_cast<uint32_t>(command->ReadValueFromFile(file, osDWord));
	address = command->next_address();

	if (fde_length) {
		EncodedData fde(command->next_address(), file.cpu_address_size());
		fde.ReadFromFile(file, fde_length);
		pos = 0;

		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "CIE Pointer"));
		value = command->ReadDataDWord(fde, &pos);
		uint64_t cie_address = address - value;
		address = command->next_address();

		IntelCommand *cie_command = func.GetCommandByAddress(cie_address);
		if (!cie_command) {
			size_t fde_pos = pos;
			uint64_t fde_address = address;

			address = cie_address;
			file.AddressSeek(address);

			command = func.Add(address);
			cie_command = command;
			command->set_comment(CommentInfo(ttComment, "CIE Length"));
			uint32_t cie_length = static_cast<uint32_t>(command->ReadValueFromFile(file, osDWord));
			address = command->next_address();

			if (cie_length) {
				EncodedData cie(command->next_address(), file.cpu_address_size());
				cie.ReadFromFile(file, cie_length);
				pos = 0;

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "CIE ID"));
				command->ReadDataDWord(cie, &pos);
				address = command->next_address();

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "CIE Version"));
				command->ReadDataByte(cie, &pos);
				address = command->next_address();

				command = func.Add(address);
				command->ReadString(cie, &pos);
				command->set_comment(CommentInfo(ttComment, string_format("Augmentation String: %s", cie_->augmentation().c_str())));
				address = command->next_address();

				command = func.Add(address);
				command->ReadUleb128(cie, &pos);
				command->set_comment(CommentInfo(ttComment, "Code Alignment Factor"));
				address = command->next_address();

				command = func.Add(address);
				command->ReadSleb128(cie, &pos);
				command->set_comment(CommentInfo(ttComment, "Data Alignment Factor"));
				address = command->next_address();

				command = func.Add(address);
				command->ReadDataByte(cie, &pos);
				command->set_comment(CommentInfo(ttComment, "Return Address Register"));
				address = command->next_address();

				if (*cie_->augmentation().c_str() == 'z') {
					command = func.Add(address);
					command->ReadUleb128(cie, &pos);
					command->set_comment(CommentInfo(ttComment, "Augmentation Length"));
					address = command->next_address();

					for (size_t j = 1; j < cie_->augmentation().size(); j++) {
						switch (cie_->augmentation().at(j)) {
						case 'L':
							command = func.Add(address);
							command->set_comment(CommentInfo(ttComment, "LSDA Encoding"));
							command->ReadDataByte(cie, &pos);
							address = command->next_address();
							break;
						case 'R':
							command = func.Add(address);
							command->set_comment(CommentInfo(ttComment, "FDE Encoding"));
							command->ReadDataByte(cie, &pos);
							address = command->next_address();
							break;
						case 'P':
							{
								command = func.Add(address);
								command->set_comment(CommentInfo(ttComment, "Personality Encoding"));
								command->ReadDataByte(cie, &pos);
								address = command->next_address();

								command = func.Add(address);
								command->ReadEncoding(cie, cie_->personality_encoding(), &pos);
								command->set_comment(CommentInfo(ttComment, "Personality Routine"));
								address = command->next_address();
							}
							break;
						}
					}
				}

				command = func.Add(address);
				command->ReadData(cie, cie.size() - pos, &pos);
				command->set_comment(CommentInfo(ttComment, "Initial Instructions"));
				address = command->next_address();
			}

			file.AddressSeek(fde_address);
			address = fde_address;
			pos = fde_pos;
		}

		command = func.Add(address);
		command->ReadEncoding(fde, cie_->fde_encoding(), &pos);
		command->set_comment(CommentInfo(ttComment, string_format("Begin: %llX", begin())));
		address = command->next_address();

		command = func.Add(address);
		command->ReadEncoding(fde, cie_->fde_encoding() & 0x0f, &pos);
		command->set_comment(CommentInfo(ttComment, string_format("End: %llX", end())));
		address = command->next_address();

		if (*cie_->augmentation().c_str() == 'z') {
			command = func.Add(address);
			value = command->ReadUleb128(fde, &pos);
			command->set_comment(CommentInfo(ttComment, "Augmentation Length"));
			address = command->next_address();

			if (cie_->augmentation().find('L') != std::string::npos) {
				command = func.Add(address);
				command->ReadEncoding(fde, cie_->lsda_encoding(), &pos);
				command->set_comment(CommentInfo(ttComment, "LSDA"));
				if (unwind_address_)
					command->AddLink(0, ltOffset, unwind_address_);

				address = command->next_address();
			}
		}

		uint64_t pc = begin();
		while (pos < fde.size()) {
			command = func.Add(address);
			size_t cur_pos = pos;
			uint8_t b = fde.ReadByte(&pos);
			switch (b) {
			case DW_CFA_nop:
				command->ReadData(fde, fde.size() - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_nop"));
				break;
			case DW_CFA_set_loc:
				pc = fde.ReadEncoding(cie_->fde_encoding(), &pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_set_loc"));
				break;
			case DW_CFA_advance_loc1:
				value = fde.ReadByte(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_advance_loc1"));
				func.range_list()->Add(pc, pc + value, NULL, NULL, command);
				pc += value;
				break;
			case DW_CFA_advance_loc2:
				value = fde.ReadWord(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_advance_loc2"));
				func.range_list()->Add(pc, pc + value, NULL, NULL, command);
				pc += value;
				break;
			case DW_CFA_advance_loc4:
				value = fde.ReadDWord(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_advance_loc4"));
				func.range_list()->Add(pc, pc + value, NULL, NULL, command);
				pc += value;
				break;
			case DW_CFA_offset_extended:
				fde.ReadUleb128(&pos);
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_offset_extended"));
				break;
			case DW_CFA_restore_extended:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_restore_extended"));
				break;
			case DW_CFA_undefined:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_undefined"));
				break;
			case DW_CFA_same_value:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_same_value"));
				break;
			case DW_CFA_register:
				fde.ReadUleb128(&pos);
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_register"));
				break;
			case DW_CFA_remember_state:
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_remember_state"));
				break;
			case DW_CFA_restore_state:
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_restore_state"));
				break;
			case DW_CFA_def_cfa:
				fde.ReadUleb128(&pos);
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa"));
				break;
			case DW_CFA_def_cfa_register:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_register"));
				break;
			case DW_CFA_def_cfa_offset:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_offset"));
				break;
			case DW_CFA_def_cfa_expression:
				value = fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_expression"));
				pos += static_cast<size_t>(value);
				break;
			case DW_CFA_expression:
				fde.ReadUleb128(&pos);
				value = fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_expression"));
				pos += static_cast<size_t>(value);
				break;
			case DW_CFA_offset_extended_sf:
				fde.ReadUleb128(&pos);
				fde.ReadSleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_offset_extended_sf"));
				break;
			case DW_CFA_def_cfa_sf:
				fde.ReadUleb128(&pos);
				fde.ReadSleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_sf"));
				break;
			case DW_CFA_def_cfa_offset_sf:
				fde.ReadSleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_def_cfa_offset_sf"));
				break;
			case DW_CFA_val_offset:
				fde.ReadUleb128(&pos);
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_val_offset"));
				break;
			case DW_CFA_val_offset_sf:
				fde.ReadUleb128(&pos);
				fde.ReadSleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_val_offset_sf"));
				break;
			case DW_CFA_val_expression:
				fde.ReadUleb128(&pos);
				value = fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_val_expression"));
				pos += static_cast<size_t>(value);
				break;
			case DW_CFA_GNU_window_save:
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_GNU_window_save"));
				break;
			case DW_CFA_GNU_args_size:
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_GNU_args_size"));
				break;
			case DW_CFA_GNU_negative_offset_extended:
				fde.ReadUleb128(&pos);
				fde.ReadUleb128(&pos);
				command->ReadData(fde, pos - cur_pos, &cur_pos);
				command->set_comment(CommentInfo(ttComment, "DW_CFA_GNU_negative_offset_extended"));
				break;
			default:
				switch (b & 0xc0) {
				case DW_CFA_advance_loc:
					value = (b & 0x3f);
					command->ReadData(fde, pos - cur_pos, &cur_pos);
					command->set_comment(CommentInfo(ttComment, "DW_CFA_advance_loc"));
					func.range_list()->Add(pc, pc + value, NULL, NULL, command);
					pc += value;
					break;
				case DW_CFA_offset:
					fde.ReadUleb128(&pos);
					command->ReadData(fde, pos - cur_pos, &cur_pos);
					command->set_comment(CommentInfo(ttComment, "DW_CFA_offset"));
					break;
				case DW_CFA_restore:
					command->ReadData(fde, pos - cur_pos, &cur_pos);
					command->set_comment(CommentInfo(ttComment, "DW_CFA_restore"));
					break;
				default:
					command->ReadData(fde, fde.size() - cur_pos, &cur_pos);
					break;
				}
			}
			pos = cur_pos;
			address = command->next_address();

			if (b != DW_CFA_nop)
				unwind_opcodes.push_back(command);
		}
	}
	for (size_t i = c; i < func.count(); i++) {
		command = func.item(i);
		command->exclude_option(roClearOriginalCode);
		command->exclude_option(roNeedCompile);
	}

	if (unwind_address_ && file.AddressSeek(unwind_address_)) {
		address = unwind_address_;

		EncodedData lsda(address, file.cpu_address_size());
		lsda.ReadFromFile(file, static_cast<size_t>(file.selected_segment()->address() + file.selected_segment()->physical_size() - address));
		pos = 0;

		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "LPStart Encoding"));
		uint8_t start_encoding = command->ReadDataByte(lsda, &pos);
		command->include_option(roCreateNewBlock);
		address = command->next_address();

		IntelCommand *entry = command;

		uint64_t start = begin();
		AddressBaseType base_type = btFunctionBegin;
		if (start_encoding != DW_EH_PE_omit) {
			command = func.Add(address);
			command->set_comment(CommentInfo(ttComment, "LPStart"));
			start = command->ReadEncoding(lsda, start_encoding, &pos);
			address = command->next_address();
			base_type = btValue;
		}

		info = func.function_info_list()->Add(begin(), end(), base_type, (base_type == btValue) ? start : 0, 0, 0xff, this, entry);
		info->set_unwind_opcodes(unwind_opcodes);

		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "TTable Encoding"));
		uint8_t ttable_encoding = command->ReadDataByte(lsda, &pos);
		address = command->next_address();

		size_t ttable_offset = 0;
		IntelCommand *ttable_offset_entry = NULL;
		if (ttable_encoding != DW_EH_PE_omit) {
			ttable_offset_entry = func.Add(address);
			ttable_offset_entry->set_comment(CommentInfo(ttComment, "TTable Offset"));
			ttable_offset_entry->include_option(roFillNop);
			ttable_offset = static_cast<size_t>(ttable_offset_entry->ReadUleb128(lsda, &pos)) + pos;
			address = ttable_offset_entry->next_address();
		}

		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "Call Site Encoding"));
		uint8_t call_site_encoding = command->ReadDataByte(lsda, &pos);
		address = command->next_address();

		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "Call Site Length"));
		uint64_t call_site_length = command->ReadUleb128(lsda, &pos);
		address = command->next_address();

		std::set<int64_t> action_list;
		size_t old_pos = pos;
		while (pos - old_pos < call_site_length) {
			IntelCommand *begin_entry = func.Add(address);
			uint64_t begin_address = start + begin_entry->ReadEncoding(lsda, call_site_encoding, &pos);
			begin_entry->set_comment(CommentInfo(ttComment, string_format("Begin: %llX", begin_address)));
			address = begin_entry->next_address();

			IntelCommand *size_entry = func.Add(address);
			uint64_t end_address = begin_address + size_entry->ReadEncoding(lsda, call_site_encoding, &pos);
			size_entry->set_comment(CommentInfo(ttComment, string_format("End: %llX", end_address)));
			address = size_entry->next_address();

			func.range_list()->Add(begin_address, end_address, begin_entry, NULL, size_entry);

			command = func.Add(address);
			value = command->ReadEncoding(lsda, call_site_encoding, &pos);
			if (value) {
				value += begin();
				link = command->AddLink(0, ltMemSEHBlock, value);
				link->set_sub_value(begin());
				link->set_base_function_info(info);
			}
			command->set_comment(CommentInfo(ttComment, string_format("Landing Pad: %llX", value)));
			address = command->next_address();

			command = func.Add(address);
			command->set_comment(CommentInfo(ttComment, "Action"));
			value = command->ReadUleb128(lsda, &pos);
			address = command->next_address();

			if (value)
				action_list.insert(value);
		}

		if (ttable_encoding != DW_EH_PE_omit) {
			std::set<int64_t> type_index_list;
			std::set<int64_t> spec_index_list;
			int64_t action = 1;
			while (action_list.size()) {
				action_list.erase(action);

				old_pos = pos;

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Type Filter"));
				int64_t index = command->ReadSleb128(lsda, &pos);
				address = command->next_address();

				if (index > 0)
					type_index_list.insert(index);
				else if (index < 0)
					spec_index_list.insert(index);

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Next Action"));
				int64_t next_action = command->ReadSleb128(lsda, &pos);
				address = command->next_address();

				action += pos - old_pos;

				if (next_action >= action)
					action_list.insert(next_action);
			}

			size_t old_count = func.count();

			pos = ttable_offset;
			address = lsda.address() + pos;
			for (size_t i = 0; i < spec_index_list.size(); i++) {
				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Exception Spec"));
				value = command->ReadUleb128(lsda, &pos);
				address = command->next_address();

				if (value > 0)
					type_index_list.insert(value);
			}

			pos = ttable_offset - type_index_list.size() * lsda.encoding_size(ttable_encoding);
			address = lsda.address() + pos;
			for (size_t i = 0; i < type_index_list.size(); i++) {
				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Type Info"));
				value = command->ReadEncoding(lsda, ttable_encoding, &pos);
				if (command->operand(0).value)
					link = command->AddLink(0, (ttable_encoding & 0x70) == DW_EH_PE_pcrel ? ltDelta : ltOffset, value);
				address = command->next_address();
			}

			if (old_count < func.count()) {
				address = func.item(old_count)->address();
				link = ttable_offset_entry->AddLink(0, ltDelta, address);
				link->set_sub_value(ttable_offset_entry->dump_size() + address - lsda.address() - ttable_offset);
			}
		}
	} else {
		// no LSDA
		info = func.function_info_list()->Add(begin(), end(), btFunctionBegin, 0, 0, 0xff, this, NULL);
		info->set_unwind_opcodes(unwind_opcodes);
	}

	for (size_t i = c; i < func.count(); i++) {
		command = func.item(i);
		command->exclude_option(roClearOriginalCode);
	}
}

void ELFRuntimeFunction::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
	begin_ += delta_base;
	end_ += delta_base;
	if (unwind_address_)
		unwind_address_ += delta_base;
}

/**
 * ELFRuntimeFunctionList
 */

ELFRuntimeFunctionList::ELFRuntimeFunctionList()
	: BaseRuntimeFunctionList(), version_(0), eh_frame_encoding_(DW_EH_PE_omit), fde_count_encoding_(DW_EH_PE_omit), fde_table_encoding_(DW_EH_PE_omit)
{
	cie_list_ = new CommonInformationEntryList();
}

ELFRuntimeFunctionList::ELFRuntimeFunctionList(const ELFRuntimeFunctionList &src)
	: BaseRuntimeFunctionList(src)
{
	cie_list_ = src.cie_list_->Clone();
	version_ = src.version_;
	eh_frame_encoding_ = src.eh_frame_encoding_;
	fde_count_encoding_ = src.fde_count_encoding_;
	fde_table_encoding_ = src.fde_table_encoding_;
	for (size_t i = 0; i < count(); i++) {
		ELFRuntimeFunction *func = item(i);
		func->set_cie(cie_list_->item(src.cie_list_->IndexOf(func->cie())));
	}
}	

ELFRuntimeFunctionList::~ELFRuntimeFunctionList()
{
	delete cie_list_;
}

ELFRuntimeFunctionList *ELFRuntimeFunctionList::Clone() const
{
	ELFRuntimeFunctionList *list = new ELFRuntimeFunctionList(*this);
	return list;
}

void ELFRuntimeFunctionList::clear()
{
	cie_list_->clear();
	BaseRuntimeFunctionList::clear();
}

ELFRuntimeFunction *ELFRuntimeFunctionList::item(size_t index) const
{
	return reinterpret_cast<ELFRuntimeFunction *>(IRuntimeFunctionList::item(index));
}

ELFRuntimeFunction *ELFRuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions)
{
	if (!source)
		throw std::runtime_error("Invalid runtime function");

	ELFRuntimeFunction *src = reinterpret_cast<ELFRuntimeFunction *>(source);
	return Add(address, begin, end, unwind_address, src->cie(), call_frame_instructions);
}

ELFRuntimeFunction *ELFRuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions)
{
	ELFRuntimeFunction *func = new ELFRuntimeFunction(this, address, begin, end, unwind_address, cie, call_frame_instructions);
	AddObject(func);
	return func;
}

ELFRuntimeFunction *ELFRuntimeFunctionList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<ELFRuntimeFunction *>(BaseRuntimeFunctionList::GetFunctionByAddress(address));
}

void ELFRuntimeFunctionList::ReadFromFile(ELFArchitecture &file)
{
	uint64_t address;
	uint32_t size;
	size_t pos;

	if (ELFSegment *hdr_segment = file.segment_list()->GetSectionByType(PT_GNU_EH_FRAME)) {
		if (!file.AddressSeek(hdr_segment->address()))
			throw std::runtime_error("Invalid format");

		EncodedData hdr(hdr_segment->address(), file.cpu_address_size());
		hdr.ReadFromFile(file, hdr_segment->physical_size());
		pos = 0;
		version_ = hdr.ReadByte(&pos);
		if (version_ != 1)
			throw std::runtime_error("Invalid format");

		eh_frame_encoding_ = hdr.ReadByte(&pos);
		fde_count_encoding_ = hdr.ReadByte(&pos);
		fde_table_encoding_ = hdr.ReadByte(&pos);
		if (eh_frame_encoding_ == DW_EH_PE_omit)
			throw std::runtime_error("Invalid format");

		address = hdr.ReadEncoding(eh_frame_encoding_, &pos);
		if (hdr_segment->address() > address)
			size = static_cast<uint32_t>(hdr_segment->address() - address);
		else {
			ELFSegment *segment = file.segment_list()->GetSectionByAddress(address);
			size = segment ? static_cast<uint32_t>(segment->address() + segment->physical_size() - address) : UINT32_MAX;
		}
	}
	else {
		ELFSection *eh_frame = file.section_list()->GetSectionByName(".eh_frame");
		if (!eh_frame)
			return;

		address = eh_frame->address();
		size = static_cast<uint32_t>(eh_frame->size());
	}

	if (!file.AddressSeek(address))
		throw std::runtime_error("Invalid format");

	std::map<uint64_t, CommonInformationEntry*> cie_map;
	for (uint32_t i = 0; i < size; ) {
		uint32_t length = file.ReadDWord();
		if (!length)
			break;

		uint64_t cur_address = address + i;
		EncodedData data(cur_address + sizeof(length), file.cpu_address_size());
		data.ReadFromFile(file, length);
		pos = 0;

		uint32_t cie_id = data.ReadDWord(&pos);
		if (cie_id == 0) {
			// CIE
			uint8_t fde_encoding = DW_EH_PE_absptr;
			uint8_t lsda_encoding = DW_EH_PE_omit;
			uint8_t personality_encoding = DW_EH_PE_omit;
			uint64_t personality_routine = 0;

			uint8_t version = data.ReadByte(&pos);
			std::string augmentation = data.ReadString(&pos);
			uint64_t code_alignment_factor = data.ReadUleb128(&pos);
			uint64_t data_alignment_factor = data.ReadSleb128(&pos);
			uint8_t return_address_register = data.ReadByte(&pos);
			if (*augmentation.c_str() == 'z') {
				data.ReadUleb128(&pos);
				for (size_t j = 1; j < augmentation.size(); j++) {
					switch (augmentation[j]) {
					case 'L':
						lsda_encoding = data.ReadByte(&pos);
						break;
					case 'R':
						fde_encoding = data.ReadByte(&pos);
						break;
					case 'P':
						{
							personality_encoding = data.ReadByte(&pos);
							personality_routine = data.ReadEncoding(personality_encoding, &pos);
						}
						break;
					}
				}
			}

			std::vector<uint8_t> initial_instructions;
			initial_instructions.resize(length - pos);
			if (!initial_instructions.empty())
				data.Read(initial_instructions.data(), initial_instructions.size(), &pos);

			CommonInformationEntry *cie = cie_list_->Add(version, augmentation, code_alignment_factor, data_alignment_factor, return_address_register, fde_encoding, lsda_encoding, personality_encoding, personality_routine, initial_instructions);
			cie_map[cur_address] = cie;
		} else {
			// FDE
			std::map<uint64_t, CommonInformationEntry*>::iterator it = cie_map.find(cur_address + sizeof(length) - cie_id);
			if (it == cie_map.end())
				throw std::runtime_error("Invalid CIE pointer");

			CommonInformationEntry *cie = it->second;
			uint64_t begin = data.ReadEncoding(cie->fde_encoding(), &pos);
			uint64_t end = begin + data.ReadEncoding(cie->fde_encoding() & 0x0f, &pos);

			uint64_t lsda_address = 0;
			if (*cie->augmentation().c_str() == 'z') {
				data.ReadUleb128(&pos);
				if (cie->augmentation().find('L') != std::string::npos) {
					size_t old_pos = pos;
					if (data.ReadEncoding(cie->lsda_encoding() & 0x0f, &pos)) {
						pos  = old_pos;
						lsda_address = data.ReadEncoding(cie->lsda_encoding(), &pos);
					}
				}
			}
			std::vector<uint8_t> call_frame_instructions;
			call_frame_instructions.resize(length - pos);
			if (!call_frame_instructions.empty())
				data.Read(call_frame_instructions.data(), call_frame_instructions.size(), &pos);

			Add(cur_address, begin, end, lsda_address, cie, call_frame_instructions);
		}

		i += sizeof(length) + length;
	}
}

void ELFRuntimeFunctionList::WriteToFile(ELFArchitecture &file)
{
	Sort();

	size_t i;
	uint64_t pos, address;

	ELFSegment *hdr_segment = file.segment_list()->GetSectionByType(PT_GNU_EH_FRAME);
	ELFSection *eh_frame = file.section_list()->GetSectionByName(".eh_frame");
	if (hdr_segment) {
		pos = hdr_segment->alignment() > 1 ? file.Resize(AlignValue(file.Tell(), hdr_segment->alignment())) : file.Tell();
		address = file.AddressTell();

		EncodedData hdr(address, file.cpu_address_size());
		// calc header size
		size_t hdr_size = 4 * sizeof(uint8_t) + hdr.encoding_size(eh_frame_encoding_);
		if (fde_count_encoding_ != DW_EH_PE_omit) {
			hdr_size += hdr.encoding_size(fde_count_encoding_);
			hdr_size += count() * 2 * hdr.encoding_size(fde_table_encoding_);
		}
		if (hdr_size < 8)
			hdr_size = 8;

		hdr_segment->Rebase(address - hdr_segment->address());
		hdr_segment->set_physical_offset(static_cast<uint32_t>(pos));
		hdr_segment->set_size(static_cast<uint32_t>(hdr_size));

		if (ELFSection *section = file.section_list()->GetSectionByName(".eh_frame_hdr")) {
			section->Rebase(address - section->address());
			section->set_physical_offset(static_cast<uint32_t>(pos));
			section->set_size(static_cast<uint32_t>(hdr_size));
		}

		pos = file.Resize(pos + hdr_size);
		address += hdr_size;
	} else {
		if (!eh_frame)
			return;

		pos = eh_frame->alignment() > 1 ? file.Resize(AlignValue(file.Tell(), eh_frame->alignment())) : file.Tell();
		address = file.AddressTell();
	}

	size_t res = 0;
	std::map<CommonInformationEntry*, uint64_t> cie_map;
	for (i = 0; i < count(); i++) {
		ELFRuntimeFunction *func = item(i);
		CommonInformationEntry *cie = func->cie();
		std::map<CommonInformationEntry*, uint64_t>::iterator it = cie_map.find(cie);
		uint64_t cie_address;
		if (it == cie_map.end()) {
			// write CIE
			cie_address = address + res;

			EncodedData data(cie_address + sizeof(uint32_t), file.cpu_address_size());
			data.WriteDWord(0);
			data.WriteByte(cie->version());
			data.WriteString(cie->augmentation());
			data.WriteUleb128(cie->code_alignment_factor());
			data.WriteSleb128(cie->data_alignment_factor());
			data.WriteByte(cie->return_address_register());
			if (*cie->augmentation().c_str() == 'z') {
				EncodedData tmp(data.address() + data.size() + 1, file.cpu_address_size());
				for (size_t j = 1; j < cie->augmentation().size(); j++) {
					switch (cie->augmentation().at(j)) {
					case 'L':
						tmp.WriteByte(cie->lsda_encoding());
						break;
					case 'R':
						tmp.WriteByte(cie->fde_encoding());
						break;
					case 'P':
						{
							tmp.WriteByte(cie->personality_encoding());
							tmp.WriteEncoding(cie->personality_encoding(), cie->personality_routine());
						}
						break;
					}
				}
				data.WriteByte(static_cast<uint8_t>(tmp.size()));
				data.Write(tmp.data(), tmp.size());
			}
			data.Write(cie->initial_instructions().data(), cie->initial_instructions().size());
			data.resize(AlignValue(data.size(), sizeof(uint32_t)), 0);

			uint32_t size = static_cast<uint32_t>(data.size());
			res += file.Write(&size, sizeof(size));
			res += file.Write(data.data(), data.size());
			cie_map[cie] = cie_address;
		} else {
			cie_address = it->second;
		}

		// write FDE
		func->set_address(address + res);
		EncodedData data(address + res + sizeof(uint32_t), file.cpu_address_size());
		data.WriteDWord(static_cast<uint32_t>(data.address() - cie_address));
		data.WriteEncoding(cie->fde_encoding(), func->begin());
		data.WriteEncoding(cie->fde_encoding() & 0x0f, func->end() - func->begin());
		if (*cie->augmentation().c_str() == 'z') {
			EncodedData tmp(data.address() + data.size() + 1, file.cpu_address_size());
			if (cie->augmentation().find('L') != std::string::npos) {
				if (func->unwind_address())
					tmp.WriteEncoding(cie->lsda_encoding(), func->unwind_address());
				else 
					tmp.WriteEncoding(cie->lsda_encoding() & 0x0f, 0);
			}
			data.WriteByte(static_cast<uint8_t>(tmp.size()));
			data.Write(tmp.data(), tmp.size());
		}
		data.Write(func->call_frame_instructions().data(), func->call_frame_instructions().size());
		data.resize(AlignValue(data.size(), sizeof(uint32_t)), 0);

		uint32_t size = static_cast<uint32_t>(data.size());
		res += file.Write(&size, sizeof(size));
		res += file.Write(data.data(), data.size());
	}
	res += file.WriteDWord(0);

	if (eh_frame) {
		eh_frame->Rebase(address - eh_frame->address());
		eh_frame->set_physical_offset(static_cast<uint32_t>(pos));
		eh_frame->set_size(static_cast<uint32_t>(res));
	}

	if (hdr_segment) {
		// write header
		EncodedData hdr(hdr_segment->address(), file.cpu_address_size());
		hdr.WriteByte(version_);
		hdr.WriteByte(eh_frame_encoding_);
		hdr.WriteByte(fde_count_encoding_);
		hdr.WriteByte(fde_table_encoding_);
		hdr.WriteEncoding(eh_frame_encoding_, address);
		if (fde_count_encoding_ != DW_EH_PE_omit) {
			hdr.WriteEncoding(fde_count_encoding_, count());
			for (i = 0; i < count(); i++) {
				ELFRuntimeFunction *func = item(i);
				hdr.WriteEncoding(fde_table_encoding_, func->begin());
				hdr.WriteEncoding(fde_table_encoding_, func->address());
			}
		}
		if (hdr.size() < 8)
			hdr.resize(8);

		pos = file.Tell();
		file.Seek(hdr_segment->physical_offset());
		file.Write(hdr.data(), hdr.size());
		file.Seek(pos);
	}
}

void ELFRuntimeFunctionList::Rebase(uint64_t delta_base)
{
	cie_list_->Rebase(delta_base);
	BaseRuntimeFunctionList::Rebase(delta_base);
}

/**
 * ELFArchitecture
 */

ELFArchitecture::ELFArchitecture(ELFFile *owner, uint64_t offset, uint64_t size)
	: BaseArchitecture(owner, offset, size), function_list_(NULL), virtual_machine_list_(NULL),
	cpu_(0), file_type_(0), image_base_(0), cpu_address_size_(osDWord), entry_point_(0), segment_alignment_(0x1000), file_alignment_(0x10),
	shstrndx_(0), shoff_(0), header_offset_(0), header_size_(0), resize_header_(0), header_segment_(NULL), overlay_offset_(0)
{
	dynsymbol_list_ = new ELFSymbolList(true);
	symbol_list_ = new ELFSymbolList(false);
	directory_list_ = new ELFDirectoryList(this);
	segment_list_ = new ELFSegmentList(this);
	import_list_ = new ELFImportList(this);
	fixup_list_ = new ELFFixupList();
	section_list_ = new ELFSectionList(this);
	export_list_ = new ELFExportList(this);
	relocation_list_ = new ELFRelocationList();
	verneed_list_ = new ELFVerneedList();
	verdef_list_ = new ELFVerdefList();
	runtime_function_list_ = new ELFRuntimeFunctionList();
}

ELFArchitecture::ELFArchitecture(ELFFile *owner, const ELFArchitecture &src)
	: BaseArchitecture(owner, src), function_list_(NULL), virtual_machine_list_(NULL), header_segment_(NULL)
{
	size_t i, j, k;

	cpu_ = src.cpu_;
	file_type_ = src.file_type_;
	image_base_ = src.image_base_;
	entry_point_ = src.entry_point_;
	cpu_address_size_ = src.cpu_address_size_;
	segment_alignment_ = src.segment_alignment_;
	file_alignment_ = src.file_alignment_; 
	shstrndx_ = src.shstrndx_;
	shoff_ = src.shoff_;
	header_offset_ = src.header_offset_;
	header_size_ = src.header_size_;
	resize_header_ = src.resize_header_;
	overlay_offset_ = src.overlay_offset_;

	dynsymbol_list_ = src.dynsymbol_list_->Clone();
	symbol_list_ = src.symbol_list_->Clone();
	directory_list_ = src.directory_list_->Clone(this);
	segment_list_ = src.segment_list_->Clone(this);
	import_list_ = src.import_list_->Clone(this);
	section_list_ = src.section_list_->Clone(this);
	export_list_ = src.export_list_->Clone(this);
	fixup_list_ = src.fixup_list_->Clone();
	relocation_list_ = src.relocation_list_->Clone();
	verneed_list_ = src.verneed_list_->Clone();
	verdef_list_ = src.verdef_list_->Clone();
	runtime_function_list_ = src.runtime_function_list_->Clone();

	if (src.header_segment_)
		header_segment_ = segment_list_->item(src.segment_list_->IndexOf(src.header_segment_));
	if (src.function_list_)
		function_list_ = src.function_list_->Clone(this);
	if (src.virtual_machine_list_)
		virtual_machine_list_ = src.virtual_machine_list_->Clone();

	for (i = 0; i < src.relocation_list()->count(); i++) {
		ELFRelocation *src_reloc = src.relocation_list()->item(i);
		ELFSymbol *src_symbol = src_reloc->symbol();
		if (!src_symbol)
			continue;

		relocation_list_->item(i)->set_symbol(dynsymbol_list_->item(src.dynsymbol_list()->IndexOf(src_symbol)));
	}

	for (i = 0; i < src.import_list()->count(); i++) {
		ELFImport *src_import = src.import_list()->item(i);
		for (j = 0; j < src_import->count(); j++) {
			ELFImportFunction *import_function = import_list_->item(i)->item(j);
			ELFImportFunction *src_import_function = src_import->item(j);
			MapFunction *src_map_function = src_import_function->map_function();
			if (src_map_function)
				import_function->set_map_function(map_function_list()->item(src.map_function_list()->IndexOf(src_map_function)));

			ELFSymbol *src_symbol = src_import_function->symbol();
			if (!src_symbol)
				continue;

			import_function->set_symbol(dynsymbol_list_->item(src.dynsymbol_list()->IndexOf(src_symbol)));
		}
	}

	for (i = 0; i < src.export_list()->count(); i++) {
		ELFSymbol *symbol = src.export_list()->item(i)->symbol();
		if (symbol)
			export_list_->item(i)->set_symbol(dynsymbol_list_->item(src.dynsymbol_list_->IndexOf(symbol)));
	}

	if (function_list_) {
		for (i = 0; i < function_list_->count(); i++) {
			IntelFunction *func = reinterpret_cast<IntelFunction *>(function_list_->item(i));
			for (j = 0; j < func->count(); j++) {
				IntelCommand *command = func->item(j);

				for (k = 0; k < 3; k++) {
					IntelOperand operand = command->operand(k);
					if (operand.type == otNone)
						break;

					if (operand.fixup)
						command->set_operand_fixup(k, fixup_list_->GetFixupByAddress(operand.fixup->address()));
					if (operand.relocation)
						command->set_operand_relocation(k, relocation_list_->GetRelocationByAddress(operand.relocation->address()));
				}
			}
			for (j = 0; j < func->function_info_list()->count(); j++) {
				FunctionInfo *info = func->function_info_list()->item(j);
				if (info->source())
					info->set_source(runtime_function_list_->GetFunctionByAddress(info->source()->begin()));
			}
		}
	}
}

ELFArchitecture::~ELFArchitecture()
{
	delete export_list_;
	delete dynsymbol_list_;
	delete directory_list_;
	delete segment_list_;
	delete import_list_;
	delete section_list_;
	delete fixup_list_;
	delete relocation_list_;
	delete symbol_list_;
	delete verneed_list_;
	delete verdef_list_;
	delete function_list_;
	delete virtual_machine_list_;
	delete runtime_function_list_;
}

ELFArchitecture *ELFArchitecture::Clone(ELFFile *file) const
{
	ELFArchitecture *arch = new ELFArchitecture(file, *this);
	return arch;
}

IArchitecture * ELFArchitecture::Clone(IFile *file) const
{
	return Clone(dynamic_cast<ELFFile *>(file));
}

OpenStatus ELFArchitecture::ReadFromFile(uint32_t mode)
{
	uint8_t ident[EI_NIDENT];

	Seek(0);

	if (size() < sizeof(ident))
		return osUnknownFormat;

	Read(&ident, sizeof(ident));
	if (ident[EI_MAG0] != 0x7f || ident[EI_MAG1] != 'E' || ident[EI_MAG2] != 'L' || ident[EI_MAG3] != 'F')
		return osUnknownFormat;

	Seek(0);

	uint16_t shnum, phnum;
	size_t i;

	switch (ident[EI_CLASS]){
	case ELFCLASS32:
		{
			Elf32_Ehdr hdr;
			Read(&hdr, sizeof(hdr));
			if (hdr.e_version != EV_CURRENT)
				return osInvalidFormat;
			entry_point_ = hdr.e_entry;
			cpu_ = hdr.e_machine;
			file_type_ = hdr.e_type;
			shoff_ = hdr.e_shoff;
			shnum = hdr.e_shnum;
			shstrndx_ = hdr.e_shstrndx;
			header_offset_ = hdr.e_phoff;
			phnum = hdr.e_phnum;
		}
		cpu_address_size_ = osDWord;
		segment_alignment_ = 0x1000;
		break;
	case ELFCLASS64:
		{
			Elf64_Ehdr hdr;
			Read(&hdr, sizeof(hdr));
			if (hdr.e_version != EV_CURRENT)
				return osInvalidFormat;
			entry_point_ = hdr.e_entry;
			cpu_ = hdr.e_machine;
			file_type_ = hdr.e_type;
			shoff_ = hdr.e_shoff;
			shnum = hdr.e_shnum;
			shstrndx_ = hdr.e_shstrndx;
			if (hdr.e_phoff >> 32)
				return osInvalidFormat;
			header_offset_ = static_cast<uint32_t>(hdr.e_phoff);
			phnum = hdr.e_phnum;
		}
		cpu_address_size_ = osQWord;
		segment_alignment_ = 0x200000;
		break;
	default:
		return osInvalidFormat;
	};

	file_alignment_ = 0x10;

	switch (ident[EI_OSABI]) {
	case ELFOSABI_NONE:
	case ELFOSABI_GNU:
		// supported type
		break;
	default:
		return osUnsupportedSubsystem;
	}

	switch (file_type_) {
	case ET_EXEC:
	case ET_DYN:
		// supported type
		break;
	default:
		return osUnsupportedSubsystem;
	}

	switch (cpu_) {
	case EM_386:
	case EM_486:
	case EM_X86_64:
		// supported cpu
		break;
	default:
		return osUnsupportedCPU;
	}

	Seek(header_offset_);
	segment_list_->ReadFromFile(*this, phnum);
	header_size_ = static_cast<uint32_t>(Tell());

	image_base_ = 0;
	for (i = 0; i < segment_list_->count(); i++) {
		ELFSegment *segment = segment_list_->item(i);
		if (segment->type() != PT_LOAD) {
			segment->set_need_parse(false);
			continue;
		}

		uint64_t segment_base = segment_list_->item(i)->address() & 0xffffffff00000000ull;
		if (!image_base_) {
			image_base_ = segment_base;
		} else if (image_base_ != segment_base) {
			return osInvalidFormat;
		}
	}

	overlay_offset_ = shoff_ + shnum * (cpu_address_size() == osDWord ? sizeof(Elf32_Shdr) : sizeof(Elf64_Shdr));
	if (overlay_offset_ == size())
		overlay_offset_ = 0 ;

	if (shnum) {
		Seek(shoff_);
		section_list_->ReadFromFile(*this, shnum);
	}
	directory_list_->ReadFromFile(*this);
	dynsymbol_list_->string_table()->ReadFromFile(*this);
	directory_list_->ReadStrings(*dynsymbol_list_->string_table());
	dynsymbol_list_->ReadFromFile(*this);
	relocation_list_->ReadFromFile(*this);
	symbol_list_->ReadFromFile(*this);
	verdef_list_->ReadFromFile(*this);
	verneed_list_->ReadFromFile(*this);
	export_list_->ReadFromFile(*this);
	import_list_->ReadFromFile(*this);
	runtime_function_list_->ReadFromFile(*this);

	header_segment_ = NULL;
	for (i = 0; i < segment_list_->count(); i++) {
		ELFSegment *segment = segment_list_->item(i);
		if (segment->type() != PT_LOAD)
			continue;

		if (segment->physical_size() && segment->physical_offset() == 0) {
			header_segment_ = segment;
			break;
		}
	}

	if ((mode & foHeaderOnly) == 0) {
		if (!owner()->file_name().empty()) {
			MapFile map_file;
			std::vector<uint64_t> segments;
			for (size_t i = 0; i < segment_list()->count(); i++) {
				segments.push_back(segment_list()->item(i)->address());
			}
			if (std::find(segments.begin(), segments.end(), 0) == segments.end())
				segments.insert(segments.begin(), 0);
			if (map_file.Parse(map_file_name().c_str(), segments))
				ReadMapFile(map_file);
		}

		for (size_t k = 0; k < 2; k++) {
			ELFSymbolList *symbol_list = (k == 0) ? dynsymbol_list_ : symbol_list_;
			for (i = 0; i < symbol_list_->count(); i++) {
				ELFSymbol *symbol = symbol_list_->item(i);
				if (symbol->type() != STT_FUNC && symbol->type() != STT_OBJECT && !symbol->section_idx())
					continue;

				MapFunction *map_function = map_function_list()->GetFunctionByAddress(symbol->address());
				if (!map_function)
					map_function = map_function_list()->Add(symbol->address(), 0, otUnknown, DemangleName(symbol->name()));

				ObjectType type = (symbol->type() == STT_FUNC && (segment_list_->GetMemoryTypeByAddress(symbol->address()) & mtExecutable)) ? otCode : otData;
				map_function->set_type(type);
			}
		}

		map_function_list()->ReadFromFile(*this);

		switch (cpu_) {
		case EM_386:
		case EM_486:
		case EM_X86_64:
			function_list_ = new ELFIntelFunctionList(this);
			virtual_machine_list_ = new IntelVirtualMachineList();
			{
				IntelFileHelper helper;
				helper.Parse(*this);
			}
			break;
		default:
			return osUnsupportedCPU;
		}
	}

	return osSuccess;
}

std::string ELFArchitecture::name() const
{
	switch (cpu_) {
	case EM_M32:
	case EM_SPARC32PLUS:
		return std::string("sparc");
	case EM_386:
		return std::string("i386");
	case EM_68K:
		return std::string("m68K");
	case EM_88K:
		return std::string("m88K");
	case EM_486:
		return std::string("i486");
	case EM_860:
		return std::string("i860");
	case EM_MIPS:
	case EM_MIPS_RS3_LE:
		return std::string("mips");
	case EM_S370:
		return std::string("s370");
	case EM_PARISC:
		return std::string("parisc");
	case EM_VPP500:
		return std::string("vpp500");
	case EM_960:
		return std::string("i960");
	case EM_PPC:
		return std::string("ppc");
	case EM_PPC64:
		return std::string("ppc64");
	case EM_S390:
		return std::string("s390");
	case EM_SPU:
		return std::string("spu");
	case EM_V800:
		return std::string("v800");
	case EM_FR20:
		return std::string("fr20");
	case EM_RH32:
		return std::string("rh32");
	case EM_RCE:
		return std::string("rce");
	case EM_ARM:
		return std::string("arm");
	case EM_ALPHA:
		return std::string("alpha");
	case EM_SH:
		return std::string("sh");
	case EM_SPARCV9:
		return std::string("sparc9");
	case EM_TRICORE:
		return std::string("tricore");
	case EM_ARC:
		return std::string("arc");
	case EM_H8_300:
		return std::string("h8/300");
	case EM_H8_300H:
		return std::string("h8/300h");
	case EM_H8S:
		return std::string("h8s");
	case EM_H8_500:
		return std::string("h8/500");
	case EM_IA_64:
		return std::string("ia64");
	case EM_MIPS_X:
		return std::string("mipsx");
	case EM_COLDFIRE:
		return std::string("coldfire");
	case EM_68HC12:
		return std::string("68hc12");
	case EM_MMA:
		return std::string("mma");
	case EM_PCP:
		return std::string("pcp");
	case EM_NCPU:
		return std::string("ncpu");
	case EM_NDR1:
		return std::string("ndr1");
	case EM_STARCORE:
		return std::string("starcore");
	case EM_ME16:
		return std::string("me16");
	case EM_ST100:
		return std::string("st100");
	case EM_TINYJ:
		return std::string("tinyj");
	case EM_X86_64:
		return std::string("amd64");
	case EM_PDSP:
		return std::string("pdsp");
	case EM_PDP10:
		return std::string("pdp10");
	case EM_PDP11:
		return std::string("pdp11");
	case EM_FX66:
		return std::string("fx66");
	case EM_ST9PLUS:
		return std::string("st9+");
	case EM_ST7:
		return std::string("st7");
	/*

	EM_ST7           = 68, // STMicroelectronics ST7 8-bit microcontroller
	EM_68HC16        = 69, // Motorola MC68HC16 Microcontroller
	EM_68HC11        = 70, // Motorola MC68HC11 Microcontroller
	EM_68HC08        = 71, // Motorola MC68HC08 Microcontroller
	EM_68HC05        = 72, // Motorola MC68HC05 Microcontroller
	EM_SVX           = 73, // Silicon Graphics SVx
	EM_ST19          = 74, // STMicroelectronics ST19 8-bit microcontroller
	EM_VAX           = 75, // Digital VAX
	EM_CRIS          = 76, // Axis Communications 32-bit embedded processor
	EM_JAVELIN       = 77, // Infineon Technologies 32-bit embedded processor
	EM_FIREPATH      = 78, // Element 14 64-bit DSP Processor
	EM_ZSP           = 79, // LSI Logic 16-bit DSP Processor
	EM_MMIX          = 80, // Donald Knuth's educational 64-bit processor
	EM_HUANY         = 81, // Harvard University machine-independent object files
	EM_PRISM         = 82, // SiTera Prism
	EM_AVR           = 83, // Atmel AVR 8-bit microcontroller
	EM_FR30          = 84, // Fujitsu FR30
	EM_D10V          = 85, // Mitsubishi D10V
	EM_D30V          = 86, // Mitsubishi D30V
	EM_V850          = 87, // NEC v850
	EM_M32R          = 88, // Mitsubishi M32R
	EM_MN10300       = 89, // Matsushita MN10300
	EM_MN10200       = 90, // Matsushita MN10200
	EM_PJ            = 91, // picoJava
	EM_OPENRISC      = 92, // OpenRISC 32-bit embedded processor
	EM_ARC_COMPACT   = 93, // ARC International ARCompact processor (old
	                       // spelling/synonym: EM_ARC_A5)
	EM_XTENSA        = 94, // Tensilica Xtensa Architecture
	EM_VIDEOCORE     = 95, // Alphamosaic VideoCore processor
	EM_TMM_GPP       = 96, // Thompson Multimedia General Purpose Processor
	EM_NS32K         = 97, // National Semiconductor 32000 series
	EM_TPC           = 98, // Tenor Network TPC processor
	EM_SNP1K         = 99, // Trebia SNP 1000 processor
	EM_ST200         = 100, // STMicroelectronics (www.st.com) ST200
	EM_IP2K          = 101, // Ubicom IP2xxx microcontroller family
	EM_MAX           = 102, // MAX Processor
	EM_CR            = 103, // National Semiconductor CompactRISC microprocessor
	EM_F2MC16        = 104, // Fujitsu F2MC16
	EM_MSP430        = 105, // Texas Instruments embedded microcontroller msp430
	EM_BLACKFIN      = 106, // Analog Devices Blackfin (DSP) processor
	EM_SE_C33        = 107, // S1C33 Family of Seiko Epson processors
	EM_SEP           = 108, // Sharp embedded microprocessor
	EM_ARCA          = 109, // Arca RISC Microprocessor
	EM_UNICORE       = 110, // Microprocessor series from PKU-Unity Ltd. and MPRC
                            // of Peking University
	EM_EXCESS        = 111, // eXcess: 16/32/64-bit configurable embedded CPU
	EM_DXP           = 112, // Icera Semiconductor Inc. Deep Execution Processor
	EM_ALTERA_NIOS2  = 113, // Altera Nios II soft-core processor
	EM_CRX           = 114, // National Semiconductor CompactRISC CRX
	EM_XGATE         = 115, // Motorola XGATE embedded processor
	EM_C166          = 116, // Infineon C16x/XC16x processor
	EM_M16C          = 117, // Renesas M16C series microprocessors
	EM_DSPIC30F      = 118, // Microchip Technology dsPIC30F Digital Signal
	                        // Controller
	EM_CE            = 119, // Freescale Communication Engine RISC core
	EM_M32C          = 120, // Renesas M32C series microprocessors
	EM_TSK3000       = 131, // Altium TSK3000 core
	EM_RS08          = 132, // Freescale RS08 embedded processor
	EM_SHARC         = 133, // Analog Devices SHARC family of 32-bit DSP
                            // processors
	EM_ECOG2         = 134, // Cyan Technology eCOG2 microprocessor
	EM_SCORE7        = 135, // Sunplus S+core7 RISC processor
	EM_DSP24         = 136, // New Japan Radio (NJR) 24-bit DSP Processor
	EM_VIDEOCORE3    = 137, // Broadcom VideoCore III processor
	EM_LATTICEMICO32 = 138, // RISC processor for Lattice FPGA architecture
	EM_SE_C17        = 139, // Seiko Epson C17 family
	EM_TI_C6000      = 140, // The Texas Instruments TMS320C6000 DSP family
	EM_TI_C2000      = 141, // The Texas Instruments TMS320C2000 DSP family
	EM_TI_C5500      = 142, // The Texas Instruments TMS320C55x DSP family
	EM_MMDSP_PLUS    = 160, // STMicroelectronics 64bit VLIW Data Signal Processor
	EM_CYPRESS_M8C   = 161, // Cypress M8C microprocessor
	EM_R32C          = 162, // Renesas R32C series microprocessors
	EM_TRIMEDIA      = 163, // NXP Semiconductors TriMedia architecture family
	EM_HEXAGON       = 164, // Qualcomm Hexagon processor
	EM_8051          = 165, // Intel 8051 and variants
	EM_STXP7X        = 166, // STMicroelectronics STxP7x family of configurable
	                        // and extensible RISC processors
	EM_NDS32         = 167, // Andes Technology compact code size embedded RISC
                            // processor family
	EM_ECOG1         = 168, // Cyan Technology eCOG1X family
	EM_ECOG1X        = 168, // Cyan Technology eCOG1X family
	EM_MAXQ30        = 169, // Dallas Semiconductor MAXQ30 Core Micro-controllers
	EM_XIMO16        = 170, // New Japan Radio (NJR) 16-bit DSP Processor
	EM_MANIK         = 171, // M2000 Reconfigurable RISC Microprocessor
	EM_CRAYNV2       = 172, // Cray Inc. NV2 vector architecture
	EM_RX            = 173, // Renesas RX family
	EM_METAG         = 174, // Imagination Technologies META processor
                            // architecture
	EM_MCST_ELBRUS   = 175, // MCST Elbrus general purpose hardware architecture
	EM_ECOG16        = 176, // Cyan Technology eCOG16 family
	EM_CR16          = 177, // National Semiconductor CompactRISC CR16 16-bit
                            // microprocessor
	EM_ETPU          = 178, // Freescale Extended Time Processing Unit
	EM_SLE9X         = 179, // Infineon Technologies SLE9X core
	EM_L10M          = 180, // Intel L10M
	EM_K10M          = 181, // Intel K10M
	EM_AARCH64       = 183, // ARM AArch64
	EM_AVR32         = 185, // Atmel Corporation 32-bit microprocessor family
	EM_STM8          = 186, // STMicroeletronics STM8 8-bit microcontroller
	EM_TILE64        = 187, // Tilera TILE64 multicore architecture family
	EM_TILEPRO       = 188, // Tilera TILEPro multicore architecture family
	EM_CUDA          = 190, // NVIDIA CUDA architecture
	EM_TILEGX        = 191, // Tilera TILE-Gx multicore architecture family
	EM_CLOUDSHIELD   = 192, // CloudShield architecture family
	EM_COREA_1ST     = 193, // KIPO-KAIST Core-A 1st generation processor family
	EM_COREA_2ND     = 194, // KIPO-KAIST Core-A 2nd generation processor family
	EM_ARC_COMPACT2  = 195, // Synopsys ARCompact V2
	EM_OPEN8         = 196, // Open8 8-bit RISC soft processor core
	EM_RL78          = 197, // Renesas RL78 family
	EM_VIDEOCORE5    = 198, // Broadcom VideoCore V processor
	EM_78KOR         = 199, // Renesas 78KOR family
	EM_56800EX       = 200, // Freescale 56800EX Digital Signal Controller (DSC)
	EM_BA1           = 201, // Beyond BA1 CPU architecture
	EM_BA2           = 202, // Beyond BA2 CPU architecture
	EM_XCORE         = 203, // XMOS xCORE processor family
	EM_MCHP_PIC      = 204, // Microchip 8-bit PIC(r) family
	EM_KM32          = 210, // KM211 KM32 32-bit processor
	EM_KMX32         = 211, // KM211 KMX32 32-bit processor
	EM_KMX16         = 212, // KM211 KMX16 16-bit processor
	EM_KMX8          = 213, // KM211 KMX8 8-bit processor
	EM_KVARC         = 214, // KM211 KVARC processor
	EM_CDP           = 215, // Paneve CDP architecture family
	EM_COGE          = 216, // Cognitive Smart Memory Processor
	EM_COOL          = 217, // iCelero CoolEngine
	EM_NORC          = 218, // Nanoradio Optimized RISC
	EM_CSR_KALIMBA   = 219  // CSR Kalimba architecture family
	*/
	default:
		return string_format("unknown 0x%X", cpu_);
	}
}

bool ELFArchitecture::Prepare(CompileContext &ctx)
{
	if ((ctx.options.flags & cpStripFixups) == 0 && file_type_ == ET_EXEC)
		ctx.options.flags |= cpStripFixups;

	if (ctx.options.flags & cpImportProtection)
		ctx.options.flags &= ~cpImportProtection;

	if (ctx.options.flags & cpResourceProtection)
		ctx.options.flags &= ~cpResourceProtection;

	if (!BaseArchitecture::Prepare(ctx))
		return false;

	ELFSegment *segment;
	size_t i, j;

	// calc new header size
	uint32_t new_segment_count = static_cast<uint32_t>(segment_list_->count() + 2);
	if (ctx.options.flags & cpStripDebugInfo) {
		for (i = 0; i < segment_list_->count(); i++) {
			segment = segment_list_->item(i);
			if (segment->type() == PT_NOTE)
				new_segment_count--;
		}
	}
	if (ctx.runtime)
		new_segment_count++;
	if (section_list_->GetSectionByName("config"))
		new_segment_count++;

	// calc header resizes
	uint32_t new_header_size = header_offset_ + new_segment_count * ((cpu_address_size() == osDWord) ? sizeof(Elf32_Phdr) : sizeof(Elf64_Phdr));
	resize_header_ = new_header_size - header_size_;
	for (i = 0; i < section_list_->count(); i++) {
		ELFSection *section = section_list_->item(i);
		if (section->physical_offset() > new_header_size)
			continue;

		switch (section->type()) {
		case SHT_NULL:
		case SHT_SYMTAB:
		case SHT_STRTAB:
		case SHT_RELA:
		case SHT_REL:
		case SHT_HASH:
		case SHT_DYNAMIC:
		case SHT_DYNSYM:
		case SHT_GNU_HASH:
		case SHT_GNU_versym:
		case SHT_GNU_verdef:
		case SHT_GNU_verneed:
			// do nothing
			break;
		case SHT_NOTE:
			if ((ctx.options.flags & cpStripDebugInfo) == 0)
				new_header_size += static_cast<uint32_t>(section->size());
			break;
		case SHT_PROGBITS:
			if (section->flags() & (SHF_WRITE | SHF_EXECINSTR)) {
				Notify(mtError, NULL, language[lsCreateSegmentError]);
				return false;
			}
			new_header_size += static_cast<uint32_t>(section->size());
			break;
		default:
			Notify(mtError, NULL, language[lsCreateSegmentError]);
			return false;
		}
	}

	segment = segment_list_->last();
	if (segment) {
		uint64_t pos = AlignValue(segment->physical_offset() + segment->physical_size(), file_alignment_);
		if (ctx.runtime) {
			ELFArchitecture *runtime = reinterpret_cast<ELFArchitecture *>(ctx.runtime);
			std::vector<std::string> static_lib_list;
			if (export_list_->GetExportByName("__cxa_guard_acquire"))
				static_lib_list.push_back("libstdc++.so");
			if (!static_lib_list.empty()) {
				for (i = 0; i < runtime->import_list()->count(); i++) {
					ELFImport *import = runtime->import_list()->item(i);
					for (j = 0; j < static_lib_list.size(); j++) {
						std::string lib_name = static_lib_list[j];
						if (import->name().substr(0, lib_name.size()) == lib_name) {
							ELFVerneed *verneed = runtime->verneed_list()->GetVerneed(import->name());
							if (verneed)
								delete verneed;
							import->set_name("");
							break;
						}
					}
				}
			}

			if (runtime->segment_list()->count()) {
				if ((import_list()->GetRuntimeOptions() & roActivation) == 0) {
					std::set<ELFSymbol *> remove_symbol_list;
					std::vector<std::string> remove_lib_list;
					remove_lib_list.push_back("libcurl.so");
					for (i = runtime->import_list()->count(); i > 0 ; i--) {
						ELFImport *import = runtime->import_list()->item(i - 1);
						for (j = 0; j < remove_lib_list.size(); j++) {
							std::string lib_name = remove_lib_list[j];
							if (import->name().substr(0, lib_name.size()) == lib_name) {
								for (size_t k = 0; k < import->count(); k++) {
									ELFImportFunction *import_func = import->item(k);
									remove_symbol_list.insert(import_func->symbol());
								}
								ELFVerneed *verneed = runtime->verneed_list()->GetVerneed(import->name());
								if (verneed)
									delete verneed;
								delete import;
								break;
							}
						}
					}
					if (!remove_symbol_list.empty()) {
						for (i = runtime->relocation_list()->count(); i > 0; i--) {
							ELFRelocation *relocation = runtime->relocation_list()->item(i - 1);
							if (!relocation->symbol())
								continue;

							if (remove_symbol_list.find(relocation->symbol()) != remove_symbol_list.end())
								delete relocation;
						}
					}
				}

				MemoryManager runtime_manager(runtime);
				for (i = runtime->segment_list()->count(); i > 0; i--) {
					ELFSegment *tmp = runtime->segment_list()->item(i - 1);
					if (tmp->type() != PT_LOAD || i > 2)
						delete tmp;
				}
				runtime->Rebase(AlignValue(segment->address() + segment->size(), segment_alignment()) + (pos & (segment_alignment() - 1)) - runtime->segment_list()->item(0)->address());
				if (runtime->header_segment_)
					runtime_manager.Add(runtime->header_segment_->address(), runtime->header_size_);
				runtime_manager.Pack();
				for (i = 0; i < runtime_manager.count(); i++) {
					MemoryRegion *region = runtime_manager.item(i);
					ctx.manager->Add(region->address(), region->size(), region->type());
				}
				segment = runtime->segment_list()->last();
			} else {
				runtime->Rebase(image_base() - runtime->image_base());
			}
		}

		// add new segment
		assert(segment);
		ctx.manager->Add(AlignValue(segment->address() + segment->size(), segment_alignment()) + (pos & (segment_alignment() - 1)), UINT32_MAX,  mtReadable | mtExecutable | mtWritable | (runtime_function_list()->count() ? mtSolid : mtNone));
	}

	for (i = 0; i < section_list_->count(); i++) {
		ELFSection *section = section_list_->item(i);
		if (!section->address())
			continue;

		switch (section->type()) {
		case SHT_STRTAB:
		case SHT_DYNSYM:
		case SHT_SYMTAB:
			if (section->physical_offset() > new_header_size)
				ctx.manager->Add(section->address(), static_cast<size_t>(section->size()));
			else if (section->physical_offset() < new_header_size && section->physical_offset() + section->size() > new_header_size) {
				uint32_t delta = new_header_size - section->physical_offset();
				ctx.manager->Add(section->address() + delta, static_cast<uint32_t>(section->size()) - delta);
			}
			break;
		}
	}

	return true;
}

void ELFArchitecture::Rebase(uint64_t delta_base)
{
	BaseArchitecture::Rebase(delta_base);

	fixup_list_->Rebase(*this, delta_base);
	relocation_list_->Rebase(*this, delta_base);
	dynsymbol_list_->Rebase(delta_base);
	export_list_->Rebase(delta_base);
	segment_list_->Rebase(delta_base);
	section_list_->Rebase(delta_base);
	import_list_->Rebase(delta_base);
	function_list_->Rebase(delta_base);
	directory_list_->Rebase(delta_base);
	runtime_function_list_->Rebase(delta_base);

	if (entry_point_)
		entry_point_ += delta_base;
	image_base_ += delta_base;
}

bool ELFArchitecture::WriteToFile()
{
	Seek(0);

	uint16_t shnum = static_cast<uint16_t>(section_list_->count());
	uint16_t phnum = static_cast<uint16_t>(segment_list_->count());
	if (cpu_address_size_ == osDWord) {
		Elf32_Ehdr hdr;
		Read(&hdr, sizeof(hdr));
		hdr.e_entry = static_cast<uint32_t>(entry_point_);
		hdr.e_shoff = static_cast<uint32_t>(shoff_);
		hdr.e_shnum = shnum;
		hdr.e_shstrndx = shstrndx_;
		hdr.e_phoff = static_cast<uint32_t>(header_offset_);
		hdr.e_phnum = phnum;
		Seek(0);
		Write(&hdr, sizeof(hdr));
	} else {
		Elf64_Ehdr hdr;
		Read(&hdr, sizeof(hdr));
		hdr.e_entry = entry_point_;
		hdr.e_shoff = shoff_;
		hdr.e_shnum = shnum;
		hdr.e_shstrndx = shstrndx_;
		hdr.e_phoff = header_offset_;
		hdr.e_phnum = phnum;
		Seek(0);
		Write(&hdr, sizeof(hdr));
	}

	ELFSegment *segment = segment_list_->GetSectionByType(PT_PHDR);
	if (segment) {
		uint32_t size = static_cast<uint32_t>(segment_list_->count() * ((cpu_address_size() == osDWord) ? sizeof(Elf32_Phdr) : sizeof(Elf64_Phdr)));
		segment->set_size(size);
		segment->set_physical_size(size);
	}

	Seek(header_offset_);
	segment_list_->WriteToFile(*this);
	header_size_ = static_cast<uint32_t>(Tell());

	return true;
}

void ELFArchitecture::Save(CompileContext &ctx)
{
	size_t i, j, c;
	uint8_t b;
	MemoryManager *manager;
	MemoryRegion *region;
	ELFSegment *last_segment, *vmp_segment, *segment;
	uint64_t address, pos, file_crc_address, file_crc_size_address, loader_crc_address, loader_crc_size_address,
		loader_crc_hash_address;
	uint32_t size, file_crc_size, loader_crc_size;
	int vmp_index;
	ELFSection *section;
	std::vector<ELFSection *> stripped_section_list, copy_section_list;
	const ELFArchitecture *src = dynamic_cast<const ELFArchitecture *>(source());

	if (ctx.options.flags & cpStripDebugInfo) {
		for (i = 0; i < section_list_->count(); i++) {
			section = section_list_->item(i);
			switch (section->type()) {
			case SHT_PROGBITS:
				if ((section->flags() & SHF_ALLOC) == 0) {
					if (section->name().substr(0, 6) == ".debug" || section->name() == ".comment")
						stripped_section_list.push_back(section);
				}
				break;
			case SHT_NOTE:
				stripped_section_list.push_back(section);
				break;
			}
		}
		for (i = segment_list_->count(); i > 0; i--) {
			segment = segment_list_->item(i - 1);
			if (segment->type() == PT_NOTE)
				delete segment;
		}
	}

	// resize header
	if (resize_header_) {
		Seek(header_offset_ + header_size_);
		for (i = 0; i < resize_header_; i++) {
			WriteByte(0);
		}

		uint32_t new_header_size = header_size_ + resize_header_;
		for (i = 0; i < section_list_->count(); i++) {
			ELFSection *section = section_list_->item(i);
			if (section->physical_offset() > new_header_size || std::find(stripped_section_list.begin(), stripped_section_list.end(), section) != stripped_section_list.end())
				continue;

			switch (section->type()) {
			case SHT_NOTE:
			case SHT_PROGBITS:
				src->Seek(section->physical_offset());
				Seek(new_header_size);
				size = static_cast<uint32_t>(section->size());
				CopyFrom(*src, size);
				for (j = 0; j < segment_list_->count(); j++) {
					ELFSegment *segment = segment_list_->item(j);
					if (segment->physical_offset() == section->physical_offset()) {
						segment->Rebase(new_header_size - segment->physical_offset());
						segment->set_physical_offset(new_header_size);
					}
				}
				section->Rebase(new_header_size - section->physical_offset());
				section->set_physical_offset(new_header_size);
				new_header_size += size;
				break;
			}
		}
	}

	// calc progress maximum
	c = 0;
	if (ctx.runtime)
		c += ctx.runtime->segment_list()->count();
	for (i = 0; i < function_list_->count(); i++) {
		IFunction *func = function_list_->item(i);
		for (j = 0; j < func->block_list()->count(); j++) {
			CommandBlock *block = func->block_list()->item(j);
			c += block->end_index() - block->start_index() + 1;
		}
	}
	StartProgress(string_format("%s...", language[lsSaving].c_str()), c);

	last_segment = segment_list_->last();
	uint32_t old_image_size = last_segment->physical_offset() + last_segment->physical_size();

	for (i = 0; i < section_list_->count(); i++) {
		section = section_list_->item(i);
		if (section->physical_offset() < old_image_size || section->type() == SHT_NOBITS || section->type() == SHT_SYMTAB || section->type() == SHT_STRTAB || std::find(stripped_section_list.begin(), stripped_section_list.end(), section) != stripped_section_list.end())
			continue;

		copy_section_list.push_back(section);
	}

	pos = Resize(AlignValue(old_image_size, file_alignment_));
	address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_) + (pos & (segment_alignment_ - 1));
	vmp_segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, PF_R, PT_LOAD, segment_alignment_);

	// merge runtime objects
	ELFArchitecture *runtime = reinterpret_cast<ELFArchitecture*>(ctx.runtime);
	if (runtime && runtime->segment_list()->count()) {
		// merge segments
		for (i = 0; i < runtime->segment_list()->count(); i++) {
			segment = runtime->segment_list()->item(i);
			pos = Tell();
			if (segment->physical_size()) {
				runtime->Seek(segment->physical_offset());
				size = static_cast<uint32_t>(segment->physical_size());
				uint8_t *buffer = new uint8_t[size];
				runtime->Read(buffer, size);
				Write(buffer, size);
				delete [] buffer;
			}
			size = (i == 0) ? static_cast<uint32_t>(runtime->segment_list()->item(i + 1)->address() - segment->address() - segment->physical_size()) : 0;
			uint8_t b = 0;
			for (j = 0; j < size; j++) {
				Write(&b, sizeof(b));
			}
			vmp_segment->include_write_type(segment->memory_type() & (~mtWritable));

			StepProgress();
		}
		// merge symbol versions
		std::map<uint16_t, uint16_t> verneed_map;
		uint16_t id = 1;
		for (i = 0; i < verneed_list_->count(); i++) {
			ELFVerneed *verneed = verneed_list_->item(i);
			for (j = 0; j < verneed->count(); j++) {
				ELFVernaux *vernaux = verneed->item(j);
				if (id < vernaux->other())
					id = vernaux->other();
			}
		}
		for (i = runtime->verneed_list_->count(); i > 0; i--) {
			ELFVerneed *src_verneed = runtime->verneed_list_->item(i - 1);
			ELFVerneed *verneed = verneed_list_->GetVerneed(src_verneed->file());
			if (!verneed) {
				verneed = src_verneed->Clone(verneed_list_);
				verneed_list_->InsertObject(0, verneed);
				for (j = verneed->count(); j > 0; j--) {
					verneed->item(j - 1)->set_other(++id);
				}
			}
			for (j = src_verneed->count(); j > 0; j--) {
				ELFVernaux *src_vernaux = src_verneed->item(j - 1);
				ELFVernaux *vernaux = verneed->GetVernaux(src_vernaux->hash());
				if (!vernaux) {
					vernaux = src_vernaux->Clone(verneed);
					verneed->InsertObject(0, vernaux);
					vernaux->set_other(++id);
				}
				verneed_map[src_vernaux->other()] = vernaux->other();
			}
		}

		// merge fixups
		for (i = 0; i < runtime->fixup_list()->count(); i++) {
			ELFFixup *fixup = runtime->fixup_list()->item(i);
			fixup_list_->AddObject(fixup->Clone(fixup_list_));
		}
		// merge relocations
		ELFDirectory *jmp_rel = directory_list_->GetCommandByType(DT_JMPREL);
		std::map<ELFSymbol *, ELFSymbol *> symbol_map;
		for (i = 0; i < runtime->relocation_list()->count(); i++) {
			ELFRelocation *src_relocation = runtime->relocation_list()->item(i);
			if (src_relocation->symbol()->bind() == STB_LOCAL) {
				address = src_relocation->symbol()->address();
				if (address && AddressSeek(src_relocation->address())) {
					if (src_relocation->size() == osDWord)
						WriteDWord(static_cast<uint32_t>(address));
					else 
						WriteQWord(address);
					fixup_list_->Add(src_relocation->address(), src_relocation->size());
				}
			} else {
				ELFRelocation *relocation = src_relocation->Clone(relocation_list_);
				if (jmp_rel == NULL && relocation->type() == R_386_JMP_SLOT)
					relocation->set_type(R_386_GLOB_DAT);
				relocation_list_->AddObject(relocation);

				ELFSymbol *symbol;
				std::map<ELFSymbol *, ELFSymbol *>::const_iterator it = symbol_map.find(src_relocation->symbol());
				if (it == symbol_map.end()) {
					symbol = src_relocation->symbol()->Clone(dynsymbol_list_);
					dynsymbol_list_->AddObject(symbol);
					if (symbol->version() > 1)
						symbol->set_version(verneed_map[symbol->version()]);
					symbol_map[src_relocation->symbol()] = symbol;
				}
				else {
					symbol = it->second;
				}
				relocation->set_symbol(symbol);
			}
		}
		// merge import
		for (i = 0; i < runtime->import_list()->count(); i++) {
			ELFImport *src_import = runtime->import_list()->item(i);
			if (src_import->is_sdk())
				continue;

			ELFImport *import = import_list_->GetImportByName(src_import->name());
			if (!import) {
				import = new ELFImport(import_list_, src_import->name());
				import_list_->AddObject(import);
			}

			for (j = 0; j < src_import->count(); j++) {
				ELFImportFunction *src_import_function = src_import->item(j);
				ELFImportFunction *import_function = src_import_function->Clone(import);
				if (src_import_function->symbol()) {
					std::map<ELFSymbol *, ELFSymbol *>::const_iterator it = symbol_map.find(src_import_function->symbol());
					import_function->set_symbol(it->second);
				}
				import->AddObject(import_function);
			}
		}
		// merge runtime functions
		size_t old_count = runtime_function_list_->cie_list()->count();
		for (i = 0; i < runtime->runtime_function_list()->cie_list()->count(); i++) {
			CommonInformationEntry *cie = runtime->runtime_function_list()->cie_list()->item(i);
			runtime_function_list_->cie_list()->AddObject(cie->Clone(runtime_function_list_->cie_list()));
		}
		for (i = 0; i < runtime->runtime_function_list()->count(); i++) {
			ELFRuntimeFunction *runtime_function = runtime->runtime_function_list()->item(i)->Clone(runtime_function_list_);
			runtime_function->set_cie(runtime_function_list()->cie_list()->item(old_count + runtime->runtime_function_list()->cie_list()->IndexOf(runtime_function->cie())));
			runtime_function_list_->AddObject(runtime_function);
		}
	}

	// write functions
	for (i = 0; i < function_list_->count(); i++) {
		function_list_->item(i)->WriteToFile(*this);
	}

	// erase not used memory regions
	manager = memory_manager();
	if (manager->count() > 1) {
		// need skip last big region
		for (i = 0; i < manager->count() - 1; i++) {
			region = manager->item(i);
			if (!AddressSeek(region->address()))
				continue;

			for (j = 0; j < region->size(); j++) {
				b = (region->type() & mtReadable) ? rand() : 0xcc;
				Write(&b, sizeof(b));
			}
		}
	}

	vmp_index = 0;
	if (vmp_segment->write_type() == mtNone) {
		delete vmp_segment;
	} else {
		size = static_cast<uint32_t>(this->size() - vmp_segment->physical_offset());
		vmp_segment->set_size(size);
		vmp_segment->set_physical_size(size);
		vmp_segment->update_type(vmp_segment->write_type());

		section_list_->Add(vmp_segment->address(), static_cast<uint32_t>(vmp_segment->size()), vmp_segment->physical_offset(), SHF_ALLOC | SHF_EXECINSTR, SHT_PROGBITS, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
	}

	if ((ctx.options.flags & cpPack) && ctx.options.script)
		ctx.options.script->DoBeforePackFile();

	// write memory CRC table
	if (function_list_->crc_table()) {
		IntelCRCTable *intel_crc = reinterpret_cast<IntelCRCTable *>(function_list_->crc_table());
		CRCTable crc_table(function_list_->crc_cryptor(), intel_crc->table_size());

		// add non writable segments
		for (i = 0; i < segment_list_->count(); i++) {
			segment = segment_list_->item(i);
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

		// skip header
		if (header_segment_)
			crc_table.Remove(header_segment_->address(), header_size_ + resize_header_);

		// skip IAT
		ELFDirectory *dir = directory_list_->GetCommandByType(DT_PLTGOT);
		if (dir)
			crc_table.Remove(dir->value(), OperandSizeToValue(cpu_address_size_) * 3);

		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < fixup_list_->count(); i++) {
				ELFFixup *fixup = fixup_list_->item(i);
				if (!fixup->is_deleted())
					crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
			}
		}

		// skip relocations
		for (i = 0; i < relocation_list_->count(); i++) {
			ELFRelocation *relocation = relocation_list_->item(i);
			crc_table.Remove(relocation->address(), (relocation->type() == R_386_COPY) ? relocation->symbol()->size() : OperandSizeToValue(relocation->size()));
		}

		// skip loader_data
		IntelFunction *loader_data = reinterpret_cast<IntelFunctionList *>(function_list_)->loader_data();
		if (loader_data)
			crc_table.Remove(loader_data->entry()->address(), loader_data->entry()->dump_size());

		// skip memory CRC table
		crc_table.Remove(intel_crc->table_entry()->address(), intel_crc->table_size());
		crc_table.Remove(intel_crc->size_entry()->address(), sizeof(uint32_t));
		crc_table.Remove(intel_crc->hash_entry()->address(), sizeof(uint32_t));

		// write to file
		AddressSeek(intel_crc->table_entry()->address());
		uint32_t hash;
		size = static_cast<uint32_t>(crc_table.WriteToFile(*this, false, &hash));
		AddressSeek(intel_crc->size_entry()->address());
		WriteDWord(size);
		AddressSeek(intel_crc->hash_entry()->address());
		WriteDWord(hash);

		intel_crc->size_entry()->set_operand_value(0, size);
		intel_crc->hash_entry()->set_operand_value(0, hash);
	}
	EndProgress();

	import_list_->Pack();
	if (!runtime) {
		ELFSymbol *empty_symbol = NULL;
		for (i = 0; i < relocation_list_->count(); i++) {
			ELFRelocation *relocation = relocation_list_->item(i);
			if (relocation->symbol()->is_deleted() && relocation->type() == R_386_JMP_SLOT) {
				if (!empty_symbol) {
					empty_symbol = new ELFSymbol(dynsymbol_list_);
					dynsymbol_list_->AddObject(empty_symbol);
				}
				relocation->set_symbol(empty_symbol);
			}
		}
	}
	relocation_list_->Pack();
	if (cpu_address_size() == osDWord) {
		for (i = 0; i < function_list_->count(); i++) {
			IntelFunction *func = reinterpret_cast<IntelFunction *>(function_list_->item(i));
			for (j = 0; j < func->count(); j++) {
				IntelCommand *command = func->item(j);
				if (!command->block() || (command->block()->type() & mtExecutable) == 0)
					continue;

				for (size_t k = 0; k < 3; k++) {
					IntelOperand operand = command->operand(k);
					if (operand.type == otNone)
						break;

					ELFRelocation *reloc = reinterpret_cast<ELFRelocation *>(operand.relocation);
					if (reloc && AddressSeek(reloc->address())) {
						switch (reloc->size()) {
						case osDWord:
							WriteDWord(static_cast<uint32_t>(reloc->value()));
							break;
						case osQWord:
							WriteQWord(reloc->value());
							break;
						}
					}
				}
			}
		}
	}

	if (ctx.options.flags & cpStripDebugInfo)
		symbol_list_->clear();
	else {
		std::set<std::string> name_list;
		for (i = 0; i < dynsymbol_list_->count(); i++) {
			ELFSymbol *symbol = dynsymbol_list_->item(i);
			if (symbol->is_deleted())
				name_list.insert(symbol->name());
		}
		for (i = 0; i < symbol_list_->count(); i++) {
			ELFSymbol *symbol = symbol_list_->item(i);
			if (name_list.find(symbol->name()) != name_list.end())
				symbol->set_deleted(true);
		}
	}
	dynsymbol_list_->Pack();
	symbol_list_->Pack();

	file_crc_address = 0;
	file_crc_size = 0;
	file_crc_size_address = 0;
	loader_crc_address = 0;
	loader_crc_size = 0;
	loader_crc_size_address = 0;
	loader_crc_hash_address = 0;
	if (runtime) {
		std::vector<IFunction *> processor_list = function_list_->processor_list();
		IntelRuntimeCRCTable *runtime_crc_table = reinterpret_cast<IntelFunctionList *>(function_list_)->runtime_crc_table();
		ELFIntelLoader *loader = new ELFIntelLoader(NULL, cpu_address_size());

		last_segment = segment_list_->last();
		pos = AlignValue((ctx.options.flags & cpPack) ? loader->GetPackedSize(this) : this->size(), file_alignment_);
		address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_) + (pos & (segment_alignment_ - 1));

		manager->clear();
		manager->Add(address, UINT32_MAX, mtReadable | mtExecutable | mtWritable | (runtime_function_list()->count() ? mtSolid : mtNone));

		if (!loader->Prepare(ctx)) {
			delete loader;
			throw std::runtime_error("Runtime error at Save");
		}
		size_t write_count = loader->count() + 10000;
		size_t processor_count = 0;
		for (i = 0; i < processor_list.size(); i++) {
			processor_count += processor_list[i]->count();
		}
		ctx.file->StartProgress(string_format("%s...", language[lsSavingStartupCode].c_str()), loader->count() + write_count + processor_count);
		loader->Compile(ctx);

		segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, PF_R | PF_W | PF_X, PT_LOAD, segment_alignment_);
		c = loader->WriteToFile(*this);
		segment->update_type(segment->write_type());
		for (i = 0; i < processor_list.size(); i++) {
			processor_list[i]->WriteToFile(*this);
		}
		if (runtime_crc_table)
			c += runtime_crc_table->WriteToFile(*this);

		// correct progress position
		write_count -= c;
		if (write_count)
			StepProgress(write_count);

		size = static_cast<uint32_t>(this->size() - segment->physical_offset());
		segment->set_size(size);
		segment->set_physical_size(size);

		section_list_->Add(segment->address(), static_cast<uint32_t>(segment->size()), segment->physical_offset(), SHF_ALLOC | SHF_EXECINSTR, SHT_PROGBITS, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));

		entry_point_ = loader->entry()->address();

		if (loader->init_entry()) {
			ELFDirectory *dir = directory_list_->GetCommandByType(DT_INIT);
			if (!dir)
				directory_list_->Add(DT_INIT);
			dir->set_value(loader->init_entry()->address());
		}

		if (loader->import_entry()) {
			address = loader->import_entry()->address();
			ELFDirectory *dir = directory_list_->GetCommandByType(DT_PLTGOT);
			if (dir) {
				ELFSection *section = section_list_->GetSectionByAddress(dir->value());
				if (section) {
					section->Rebase(address - section->address());
					section->set_physical_offset(static_cast<uint32_t>(segment->physical_offset() + address - segment->address()));
					section->set_size(loader->import_size());
				}
			} else {
				dir = directory_list_->Add(DT_PLTGOT);
			}
			dir->set_value(address);
		}

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

		if (loader->preinit_entry()) {
			ELFDirectory *dir = directory_list_->GetCommandByType(DT_PREINIT_ARRAY);
			if (dir) {
				ELFSection *section = section_list_->GetSectionByAddress(dir->value());
				if (section) {
					section->Rebase(address - section->address());
					section->set_physical_offset(static_cast<uint32_t>(segment->physical_offset() + address - segment->address()));
					section->set_size(loader->preinit_size());
				}
			} else 
				dir = directory_list_->Add(DT_PREINIT_ARRAY);
			dir->set_value(loader->preinit_entry()->address());

			dir = directory_list_->GetCommandByType(DT_PREINIT_ARRAYSZ);
			if (!dir)
				dir = directory_list_->Add(DT_PREINIT_ARRAYSZ);
			dir->set_value(loader->preinit_size());
		}

		if (loader->term_entry()) {
			address = loader->term_entry()->address();
			ELFDirectory *dir = directory_list_->GetCommandByType(DT_FINI);
			if (!dir)
				dir = directory_list_->Add(DT_FINI);
			dir->set_value(address);
		}

		if (loader->tls_entry()) {
			address = loader->tls_entry()->address();
			ELFSegment *tls_segment = segment_list_->GetSectionByType(PT_TLS);
			if (tls_segment)
				tls_segment->Rebase(address - tls_segment->address());
			for (i = 0; i < section_list_->count(); i++) {
				ELFSection *section = section_list_->item(i);
				if ((section->flags() & SHF_TLS) == 0)
					continue;

				section->Rebase(address - section->address());
				section->set_physical_offset(static_cast<uint32_t>(section->address() - segment->address() + segment->physical_offset()));
				address += section->size();
			}
		}

		delete loader;

		ctx.file->EndProgress();

		for (i = 0; i < relocation_list_->count(); i++) {
			ELFRelocation *relocation = relocation_list_->item(i);
			if (relocation->type() == R_386_GLOB_DAT) {
				ELFDirectory *dir = directory_list_->GetCommandByType(relocation->is_rela() ? DT_RELA : DT_REL);
				if (!dir) {
					directory_list_->Add(relocation->is_rela() ? DT_RELA : DT_REL);
					section = section_list_->Add(0, 0, 0, SHF_ALLOC, relocation->is_rela() ? SHT_RELA : SHT_REL, relocation->is_rela() ? ".rela.dyn" : ".rel.dyn");
					section->set_link(static_cast<uint32_t>(section_list_->IndexOf(section_list_->GetSectionByType(SHT_DYNSYM))));
					if (cpu_address_size_ == osDWord)
						section->set_entry_size(relocation->is_rela() ? sizeof(Elf32_Rela) : sizeof(Elf32_Rel));
					else
						section->set_entry_size(relocation->is_rela() ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel));

					dir = directory_list_->GetCommandByType(relocation->is_rela() ? DT_RELASZ : DT_RELSZ);
					if (!dir)
						directory_list_->Add(relocation->is_rela() ? DT_RELASZ : DT_RELSZ);

					dir = directory_list_->GetCommandByType(relocation->is_rela() ? DT_RELAENT : DT_RELENT);
					if (!dir) {
						dir = directory_list_->Add(relocation->is_rela() ? DT_RELAENT : DT_RELENT);
						dir->set_value(section->entry_size());
					}
				}
				break;
			}
		}
	}

	// write ELF structures
	last_segment = segment_list_->last();
	pos = Resize(AlignValue(this->size(), file_alignment_));
	address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_) + (pos & (segment_alignment_ - 1));
	vmp_segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, PF_R | PF_W, PT_LOAD, segment_alignment_);

	import_list_->WriteToFile(*this);
	dynsymbol_list_->WriteToFile(*this);
	verdef_list_->WriteToFile(*this);
	verneed_list_->WriteToFile(*this);
	if (ctx.options.flags & cpStripFixups)
		fixup_list_->clear();
	else
		fixup_list_->Pack();
	relocation_list_->WriteToFile(*this);
	runtime_function_list_->WriteToFile(*this);

	if (!directory_list_->GetCommandByType(DT_TEXTREL)) {
		// check relocations for non-writable segments
		for (i = 0; i < relocation_list_->count(); i++) {
			ELFRelocation *relocation = relocation_list_->item(i);
			uint32_t memory_type = segment_list_->GetMemoryTypeByAddress(relocation->address());
			if (memory_type == mtNone)
				continue;

			if ((memory_type & mtWritable) == 0) {
				directory_list_->Add(DT_TEXTREL);
				break;
			}
		}
	}
	directory_list_->WriteToFile(*this);

	size = static_cast<uint32_t>(this->size() - vmp_segment->physical_offset());
	vmp_segment->set_size(size);
	vmp_segment->set_physical_size(size);

	// copy sections
	for (i = 0; i < copy_section_list.size(); i++) {
		section = copy_section_list[i];
		pos = section->alignment() > 1 ? Resize(AlignValue(this->size(), section->alignment())) : this->size();
		src->Seek(section->physical_offset());
		CopyFrom(*src, section->size());
		section->set_physical_offset(static_cast<uint32_t>(pos));
	}
	section = src->section_list_->GetSectionByName("config");
	if (section) {
		last_segment = segment_list_->last();
		pos = Resize(AlignValue(this->size(), segment_alignment_));
		address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_);
		vmp_segment = segment_list_->Add(address, static_cast<uint32_t>(section->size()), static_cast<uint32_t>(pos), static_cast<uint32_t>(section->size()), PF_R | PF_W, PT_LOAD, segment_alignment_);
		src->Seek(section->physical_offset());
		CopyFrom(*src, section->size());
	}

	if (symbol_list_->count() == 0) {
		section = section_list_->GetSectionByType(SHT_SYMTAB);
		if (section) {
			stripped_section_list.push_back(section_list_->item(section->link()));
			stripped_section_list.push_back(section);
		}
	} else {
		symbol_list_->WriteToFile(*this);
	}

	if (stripped_section_list.size()) {
		std::vector<ELFSection *> orig_section_list;
		std::map<size_t, size_t> index_map;
		for (i = 0; i < section_list_->count(); i++) {
			orig_section_list.push_back(section_list_->item(i));
		}
		for (i = stripped_section_list.size(); i > 0; i--) {
			section = stripped_section_list[i - 1];
			delete section;
		}
		for (i = 0; i < orig_section_list.size(); i++) {
			section = orig_section_list[i];
			index_map[i] = section_list_->IndexOf(section);
		}
		section_list_->RemapLinks(index_map);

		std::map<size_t, size_t>::const_iterator it = index_map.find(shstrndx_);
		if (it == index_map.end() || it->second == NOT_ID)
			throw std::runtime_error("Invalid section index");
		shstrndx_ = static_cast<uint16_t>(it->second);
	}

	if (section_list_->count())
		shoff_ = section_list_->WriteToFile(*this);
	else {
		shoff_ = 0;
		shstrndx_ = SHN_UNDEF;
	}

	// copy overlay
	if (overlay_offset_) {
		Seek(this->size());
		src->Seek(overlay_offset_);
		CopyFrom(*src, src->size() - overlay_offset_);
	}

	if (ctx.options.script)
		ctx.options.script->DoAfterSaveFile();

	// write header
	WriteToFile();

	// write header and loader CRC table
	if (loader_crc_address) {
		CRCTable crc_table(function_list_->crc_cryptor(), loader_crc_size);

		// add header
		if (header_segment_)
			crc_table.Add(header_segment_->address(), header_size_);

		// add loader segments
		j = segment_list_->IndexOf(segment_list_->GetSectionByAddress(loader_crc_address));
		if (j != NOT_ID) {
			c = (ctx.options.flags & cpLoaderCRC) ? j + 1 : segment_list_->count();
			for (i = j; i < c; i++) {
				segment = segment_list_->item(i);
				// first loader segment always has PROT_WRITE flag
				if (i > j && (segment->memory_type() & mtWritable))
					continue;

				size = std::min(static_cast<uint32_t>(segment->size()), segment->physical_size());
				if (size)
					crc_table.Add(segment->address(), size);
			}
		}

		// skip IAT
		ELFDirectory *dir = directory_list_->GetCommandByType(DT_PLTGOT);
		if (dir)
			crc_table.Remove(dir->value(), OperandSizeToValue(cpu_address_size_) * 3);
		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < fixup_list_->count(); i++) {
				ELFFixup *fixup = fixup_list_->item(i);
				if (!fixup->is_deleted())
					crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
			}
		}
		// skip relocations
		for (i = 0; i < relocation_list_->count(); i++) {
			ELFRelocation *relocation = relocation_list_->item(i);
			crc_table.Remove(relocation->address(), (relocation->type() == R_386_COPY) ? relocation->symbol()->size() : OperandSizeToValue(relocation->size()));
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

		// skip file CRC table
		if (AddressSeek(file_crc_address))
			crc_table.Remove(Tell(), file_crc_size);
		if (AddressSeek(file_crc_size_address))
			crc_table.Remove(Tell(), sizeof(uint32_t));
		section = section_list_->GetSectionByType(0x80736967);  // "signature"
		if (section)
			crc_table.Remove(section->physical_offset(), section->physical_size());

		// write to file
		AddressSeek(file_crc_address);
		size = static_cast<uint32_t>(this->size());
		Write(&size, sizeof(size));
		size = static_cast<uint32_t>(crc_table.WriteToFile(*this, true));
		AddressSeek(file_crc_size_address);
		WriteDWord(size);
	}

	EndProgress();
}

bool ELFArchitecture::is_executable() const
{
	return file_type() == ET_EXEC;
}

/**
 * ELFFile
 */

ELFFile::ELFFile(ILog *log) 
	: IFile(log), runtime_(NULL)
{
	
}

ELFFile::~ELFFile()
{
	delete runtime_;
}

ELFFile::ELFFile(const ELFFile &src, const char *file_name)
	: IFile(src, file_name), runtime_(NULL)
{
	for (size_t i = 0; i < src.count(); i++)
		AddObject(src.item(i)->Clone(this));
}

std::string ELFFile::format_name() const
{
	return std::string("ELF");
}

ELFArchitecture *ELFFile::item(size_t index) const
{ 
	return reinterpret_cast<ELFArchitecture *>(IFile::item(index));
}

ELFArchitecture *ELFFile::Add(uint64_t offset, uint64_t size)
{
	ELFArchitecture *arch = new ELFArchitecture(this, offset, size);
	AddObject(arch);
	return arch;
}

OpenStatus ELFFile::ReadHeader(uint32_t open_mode)
{
	ELFArchitecture *arch = Add(0, size());
	return arch->ReadFromFile(open_mode);
}

ELFFile *ELFFile::Clone(const char *file_name) const
{
	ELFFile *file = new ELFFile(*this, file_name);
	return file;
}

bool ELFFile::Compile(CompileOptions &options)
{
	const ResourceInfo runtime_info[] = {
		{lin_runtime32_so_file, sizeof(lin_runtime32_so_file), lin_runtime32_so_code},
		{lin_runtime64_so_file, sizeof(lin_runtime64_so_file), lin_runtime64_so_code}
	};
	
	ELFArchitecture *arch = item(0);
	ResourceInfo info = runtime_info[arch->cpu_address_size() == osDWord ? 0 : 1];
	if (info.size > 1) {
		runtime_ = new ELFFile(NULL);
		if (!runtime_->OpenResource(info.file, info.size, true))
			throw std::runtime_error("Runtime error at OpenResource");

		Buffer buffer(info.code);
		arch = runtime_->item(0);
		arch->ReadFromBuffer(buffer);
		for (size_t i = 0; i < arch->function_list()->count(); i++) {
			arch->function_list()->item(i)->set_from_runtime(true);
		}
		for (size_t i = 0; i < arch->import_list()->count(); i++) {
			ELFImport *import = arch->import_list()->item(i);
			for (size_t j = 0; j < import->count(); j++) {
				import->item(j)->include_option(ioFromRuntime);
			}
		}
	}

	return IFile::Compile(options);
}

bool ELFFile::is_executable() const
{
#ifdef __unix__
	for (size_t i = 0; i < count(); i++) {
		if (item(i)->is_executable())
			return true;
	}
#endif
	return false;
}

uint32_t ELFFile::disable_options() const
{
	uint32_t res = cpResourceProtection | cpImportProtection | cpVirtualFiles;
	for (size_t i = 0; i < count(); i++) {
		ELFArchitecture *arch = item(i);
		if (arch->file_type() != ET_EXEC)
			res |= cpStripFixups;
	}
	return res;
}