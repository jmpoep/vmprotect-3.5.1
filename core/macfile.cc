/**
 * Support of Mach-O executable files.
 */

#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "dwarf.h"
#include "macfile.h"
#include "processors.h"
#include "intel.h"
#include "lang.h"
#include "core.h"
#include "script.h"

#include "mac_runtime32.dylib.inc"
#include "mac_runtime64.dylib.inc"

/**
 * MacLoadCommand
 */

MacLoadCommand::MacLoadCommand(MacLoadCommandList *owner)
	: BaseLoadCommand(owner), address_(0), size_(0), type_(0), object_(NULL), offset_(0)
{

}

MacLoadCommand::MacLoadCommand(MacLoadCommandList *owner, uint64_t address, uint32_t size, uint32_t type)
	: BaseLoadCommand(owner), address_(address), size_(size), type_(type), object_(NULL), offset_(0)
{

}

MacLoadCommand::MacLoadCommand(MacLoadCommandList *owner, uint32_t type, IObject *object)
	: BaseLoadCommand(owner), address_(0), size_(0), type_(type), object_(object), offset_(0)
{

}

MacLoadCommand::MacLoadCommand(MacLoadCommandList *owner, const MacLoadCommand &src)
	: BaseLoadCommand(owner, src), object_(NULL), offset_(0)
{
	address_ = src.address_;
	size_ = src.size_;
	type_ = src.type_;
}

MacLoadCommand *MacLoadCommand::Clone(ILoadCommandList *owner) const
{
	MacLoadCommand *command = new MacLoadCommand(reinterpret_cast<MacLoadCommandList *>(owner), *this);
	return command;
}

void MacLoadCommand::ReadFromFile(MacArchitecture &file)
{
	load_command lc;

	/* Store current position into address_
	 * and read struct load_command */
	address_ = file.Tell();
	file.Read(&lc, sizeof(lc));
	type_ = lc.cmd;
	size_ = lc.cmdsize;
	if (size_ < sizeof(lc))
		throw std::runtime_error("Invalid format");
}

std::string MacLoadCommand::name() const
{
	switch (type_) {
	case LC_SEGMENT:
		return std::string("LC_SEGMENT");
	case LC_SYMTAB:
		return std::string("LC_SYMTAB");
	case LC_SYMSEG:
		return std::string("LC_SYMSEG");
	case LC_THREAD:
		return std::string("LC_THREAD");
	case LC_UNIXTHREAD:
		return std::string("LC_UNIXTHREAD");
	case LC_LOADFVMLIB:
		return std::string("LC_LOADFVMLIB");
	case LC_IDFVMLIB:
		return std::string("LC_IDFVMLIB");
	case LC_IDENT:
		return std::string("LC_IDENT");
	case LC_FVMFILE:
		return std::string("LC_FVMFILE");
	case LC_PREPAGE:
		return std::string("LC_PREPAGE");
	case LC_DYSYMTAB:
		return std::string("LC_DYSYMTAB");
	case LC_LOAD_DYLIB:
		return std::string("LC_LOAD_DYLIB");
	case LC_ID_DYLIB:
		return std::string("LC_ID_DYLIB");
	case LC_LOAD_DYLINKER:
		return std::string("LC_LOAD_DYLINKER");
	case LC_ID_DYLINKER:
		return std::string("LC_ID_DYLINKER");
	case LC_PREBOUND_DYLIB:
		return std::string("LC_PREBOUND_DYLIB");
	case LC_ROUTINES:
		return std::string("LC_ROUTINES");
	case LC_SUB_FRAMEWORK:
		return std::string("LC_SUB_FRAMEWORK");
	case LC_SUB_UMBRELLA:
		return std::string("LC_SUB_UMBRELLA");
	case LC_SUB_CLIENT:
		return std::string("LC_SUB_CLIENT");
	case LC_SUB_LIBRARY:
		return std::string("LC_SUB_LIBRARY");
	case LC_TWOLEVEL_HINTS:
		return std::string("LC_TWOLEVEL_HINTS");
	case LC_PREBIND_CKSUM:
		return std::string("LC_PREBIND_CKSUM");
	case LC_LOAD_WEAK_DYLIB:
		return std::string("LC_LOAD_WEAK_DYLIB");
	case LC_SEGMENT_64:
		return std::string("LC_SEGMENT_64");
	case LC_ROUTINES_64:
		return std::string("LC_ROUTINES_64");
	case LC_UUID:
		return std::string("LC_UUID");
	case LC_RPATH:
		return std::string("LC_RPATH");
	case LC_CODE_SIGNATURE:
		return std::string("LC_CODE_SIGNATURE");
	case LC_SEGMENT_SPLIT_INFO:
		return std::string("LC_SEGMENT_SPLIT_INFO");
	case LC_DYLD_INFO:
		return std::string("LC_DYLD_INFO");
	case LC_DYLD_INFO_ONLY:
		return std::string("LC_DYLD_INFO_ONLY");
	case LC_VERSION_MIN_MACOSX:
		return std::string("LC_VERSION_MIN_MACOSX");
	case LC_FUNCTION_STARTS:
		return std::string("LC_FUNCTION_STARTS");
	case LC_DYLD_ENVIRONMENT:
		return std::string("LC_DYLD_ENVIRONMENT");
	case LC_MAIN:
		return std::string("LC_MAIN");
	case LC_DATA_IN_CODE:
		return std::string("LC_DATA_IN_CODE");
	case LC_SOURCE_VERSION:
		return std::string("LC_SOURCE_VERSION");
	case LC_DYLIB_CODE_SIGN_DRS:
		return std::string("LC_DYLIB_CODE_SIGN_DRS");
	case LC_ENCRYPTION_INFO_64:
		return std::string("LC_ENCRYPTION_INFO_64");
	case LC_LINKER_OPTION:
		return std::string("LC_LINKER_OPTION");
	case LC_LINKER_OPTIMIZATION_HINT:
		return std::string("LC_LINKER_OPTIMIZATION_HINT");
	case LC_VERSION_MIN_TVOS:
		return std::string("LC_VERSION_MIN_TVOS");
	case LC_VERSION_MIN_WATCHOS:
		return std::string("LC_VERSION_MIN_WATCHOS");
	case LC_NOTE:
		return std::string("LC_NOTE");
	case LC_BUILD_VERSION:
		return std::string("LC_BUILD_VERSION");
	}

	return BaseLoadCommand::name();
}

void MacLoadCommand::WriteToFile(MacArchitecture &file)
{
	const IArchitecture *source = file.source();
	uint64_t position = file.Tell();
	bool need_copy = true;

	switch (type_) {
	case LC_SEGMENT:
	case LC_SEGMENT_64:
		if (object_) {
			reinterpret_cast<MacSegment *>(object_)->WriteToFile(file);
			need_copy = false;
		}
		break;
	case LC_LOAD_DYLIB:
	case LC_LOAD_WEAK_DYLIB:
		if (object_) {
			reinterpret_cast<MacImport *>(object_)->WriteToFile(file);
			need_copy = false;
		}
		break;
	case LC_SYMTAB:
		{
			symtab_command *symtab = file.symtab();
			file.Write(symtab, sizeof(symtab_command));
			need_copy = false;
		}
		break;
	case LC_DYSYMTAB:
		{
			dysymtab_command *dysymtab = file.dysymtab();
			file.Write(dysymtab, sizeof(dysymtab_command));
			need_copy = false;
		}
		break;
	case LC_DYLD_INFO:
    case LC_DYLD_INFO_ONLY:
		{
			dyld_info_command *dyld_info = file.dyld_info();
			file.Write(dyld_info, sizeof(dyld_info_command));
			need_copy = false;
		}
		break;
	case LC_UNIXTHREAD:
		{
			thread_command command;
			source->Seek(address_);
			source->Read(&command, sizeof(command));
			x86_state_hdr_t state_hdr;
			source->Read(&state_hdr, sizeof(state_hdr));

			switch (file.type()) {
			case CPU_TYPE_I386:
				if (state_hdr.flavor == x86_THREAD_STATE32) {
					x86_thread_state32_t thread_state;
					source->Read(&thread_state, sizeof(thread_state));
					thread_state.__eip = static_cast<uint32_t>(file.entry_point());
					file.Write(&command, sizeof(command));
					file.Write(&state_hdr, sizeof(state_hdr));
					file.Write(&thread_state, sizeof(thread_state));
					need_copy = false;
				}
				break;
			case CPU_TYPE_X86_64:
				if (state_hdr.flavor == x86_THREAD_STATE64) {
					x86_thread_state64_t thread_state;
					source->Read(&thread_state, sizeof(thread_state));
					thread_state.__rip = file.entry_point();
					file.Write(&command, sizeof(command));
					file.Write(&state_hdr, sizeof(state_hdr));
					file.Write(&thread_state, sizeof(thread_state));
					need_copy = false;
				}
				break;
			}
		}
		break;
	case LC_MAIN:
		{
			entry_point_command command;
			source->Seek(address_);
			source->Read(&command, sizeof(command));

			command.entryoff = file.entry_point() - file.segment_list()->GetBaseSegment()->address();

			file.Write(&command, sizeof(command));
			need_copy = false;
		}
		break;
	case LC_FUNCTION_STARTS:
	case LC_DATA_IN_CODE:
	case LC_DYLIB_CODE_SIGN_DRS:
		{
			linkedit_data_command command;
			source->Seek(address_);
			source->Read(&command, sizeof(command));
			command.dataoff = offset_;
			file.Write(&command, sizeof(command));
			need_copy = false;
		}
		break;
	case LC_ID_DYLIB:
		if (file.export_list()->name() != source->export_list()->name()) {
			std::string name = file.export_list()->name();
			name.resize(AlignValue(name.size() + 1, (file.cpu_address_size() == osDWord) ? sizeof(uint32_t) : sizeof(uint64_t)) - 1, 0);
			dylib_command command;
			source->Seek(address_);
			source->Read(&command, sizeof(command));
			command.cmdsize = static_cast<uint32_t>(sizeof(command) + name.size() + 1);
			command.dylib.name.offset = sizeof(command);
			file.Write(&command, sizeof(command));
			file.Write(name.c_str(), name.size() + 1);
			need_copy = false;
		}
		break;
	}

	if (need_copy) {
		source->Seek(address_);
		file.CopyFrom(*source, size_);
	}

	address_ = position;
}

void MacLoadCommand::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * MacLoadCommandList
 */

MacLoadCommandList::MacLoadCommandList(MacArchitecture *owner)
	: BaseCommandList(owner)
{

}

MacLoadCommandList::MacLoadCommandList(MacArchitecture *owner, const MacLoadCommandList &src)
	: BaseCommandList(owner, src)
{

}

MacLoadCommandList *MacLoadCommandList::Clone(MacArchitecture *owner) const
{
	MacLoadCommandList *load_command_list = new MacLoadCommandList(owner, *this);
	return load_command_list;
}

MacLoadCommand *MacLoadCommandList::item(size_t index) const
{ 
	return reinterpret_cast<MacLoadCommand*>(BaseCommandList::item(index));
}

void MacLoadCommandList::ReadFromFile(MacArchitecture &file, size_t count)
{
	uint64_t pos = file.Tell();
	Reserve(count);
	for (size_t i = 0; i < count; i++) {
		file.Seek(pos);
		MacLoadCommand *command = new MacLoadCommand(this);
		AddObject(command);
		command->ReadFromFile(file);
		pos += command->size();
	}
}

void MacLoadCommandList::WriteToFile(MacArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

void MacLoadCommandList::Pack()
{
	for (size_t i = count(); i > 0 ; i--) {
		MacLoadCommand *load_command = item(i - 1);
		switch (load_command->type()) {
		case LC_SEGMENT:
		case LC_SEGMENT_64:
		case LC_LOAD_DYLIB:
		case LC_LOAD_WEAK_DYLIB:
		case LC_CODE_SIGNATURE:
			delete load_command;
			break;
		}
	}
}

void MacLoadCommandList::Add(uint32_t type, IObject *object)
{
	MacLoadCommand *command = new MacLoadCommand(this, type, object);
	if (type == LC_SEGMENT || type == LC_SEGMENT_64) {
		// insert segment commands to the top of list
		for (size_t i = 0; i < count(); i++) {
			MacLoadCommand *lc = item(i);
			if (lc->type() == LC_SEGMENT || lc->type() == LC_SEGMENT_64)
				continue;
			InsertObject(i, command);
			return;
		}
	}
	AddObject(command);
}

MacLoadCommand *MacLoadCommandList::GetCommandByObject(void *object) const
{
	for (size_t i = 0; i < count(); i++) {
		MacLoadCommand *load_command = item(i);
		if (load_command->object() == object)
			return load_command;
	}
	return NULL;
}

/**
 * MacSegment
 */

MacSegment::MacSegment(MacSegmentList *owner)
	: BaseSection(owner), address_(0), size_(0),
	physical_offset_(0), physical_size_(0), nsects_(0), maxprot_(VM_PROT_NONE), initprot_(VM_PROT_NONE)
{

}

MacSegment::MacSegment(MacSegmentList *owner,uint64_t address, uint64_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t initprot, const std::string &name)
	: BaseSection(owner), address_(address), size_(size), physical_offset_(physical_offset), physical_size_(physical_size),
	nsects_(0), maxprot_(initprot), initprot_(initprot), name_(name)
{

}

MacSegment::MacSegment(MacSegmentList *owner, const MacSegment &src)
	: BaseSection(owner, src), nsects_(0)
{
	address_ = src.address_;
	size_ = src.size_;
	physical_offset_ = src.physical_offset_;
	physical_size_ = src.physical_size_;
	maxprot_ = src.maxprot_;
	initprot_ = src.initprot_;
	name_ = src.name_;
}

MacSegment *MacSegment::Clone(ISectionList *owner) const
{
	MacSegment *segment = new MacSegment(reinterpret_cast<MacSegmentList *>(owner), *this);
	return segment;
}

uint32_t MacSegment::memory_type() const
{
	uint32_t res = mtNone;
	if (initprot_ & VM_PROT_READ)
		res |= mtReadable;
	if (initprot_ & VM_PROT_WRITE)
		res |= mtWritable;
	if (initprot_ & VM_PROT_EXECUTE)
		res |= mtExecutable;
	return res;
}

void MacSegment::ReadFromFile(MacArchitecture &file)
{
	uint32_t need_type = (file.cpu_address_size() == osDWord) ? LC_SEGMENT : LC_SEGMENT_64;
	uint32_t type = file.ReadDWord();

	if (type != need_type)
		throw std::runtime_error("Invalid segment type");

	if (file.cpu_address_size() == osDWord) {
		segment_command command = segment_command();
		file.Read(&command.cmdsize, sizeof(command) - offsetof(segment_command, cmdsize));
		address_ = command.vmaddr;
		size_ = command.vmsize;
		physical_offset_ = command.fileoff;
		physical_size_ = command.filesize;
		nsects_ = command.nsects;
		maxprot_ = command.maxprot;
		initprot_ = command.initprot;
		name_ = std::string(command.segname, strnlen(command.segname, sizeof(command.segname)));
	} else {
		segment_command_64 command = segment_command_64();
		file.Read(&command.cmdsize, sizeof(command) - offsetof(segment_command_64, cmdsize));
		if (command.filesize >> 32)
			throw std::runtime_error("Segment size is too large");
		if (command.fileoff >> 32)
			throw std::runtime_error("Segment offset is too large");
		address_ = command.vmaddr;
		size_ = command.vmsize;
		physical_offset_ = static_cast<uint32_t>(command.fileoff);
		physical_size_ = static_cast<uint32_t>(command.filesize);
		nsects_ = command.nsects;
		maxprot_ = command.maxprot;
		initprot_ = command.initprot;
		name_ = std::string(command.segname, strnlen(command.segname, sizeof(command.segname)));
	}

	// read sections for this segment
	file.section_list()->ReadFromFile(file, nsects_, this);
}

void MacSegment::WriteToFile(MacArchitecture &file)
{
	size_t i;
	MacSectionList *section_list = file.section_list();

	// calc child section sount
	nsects_ = 0;
	for (i = 0; i < section_list->count(); i++) {
		if (section_list->item(i)->parent() == this)
			nsects_++;
	}

	// write header
	uint64_t command_pos = file.Tell();
	if (file.cpu_address_size() == osDWord) {
		segment_command cmd = segment_command();
		cmd.cmd = LC_SEGMENT;
		cmd.vmaddr = static_cast<uint32_t>(address_);
		cmd.vmsize = static_cast<uint32_t>(size_);
		cmd.fileoff = static_cast<uint32_t>(physical_offset_);
		cmd.filesize = static_cast<uint32_t>(physical_size_);
		cmd.nsects = nsects_;
		cmd.maxprot = maxprot_;
		cmd.initprot = initprot_;
		memcpy(cmd.segname, name_.c_str(), std::min(name_.size(), sizeof(cmd.segname)));
		file.Write(&cmd, sizeof(cmd));
	} else {
		segment_command_64 cmd = segment_command_64();
		cmd.cmd = LC_SEGMENT_64;
		cmd.vmaddr = address_;
		cmd.vmsize = size_;
		cmd.fileoff = physical_offset_;
		cmd.filesize = physical_size_;
		cmd.nsects = nsects_;
		cmd.maxprot = maxprot_;
		cmd.initprot = initprot_;
		memcpy(cmd.segname, name_.c_str(), std::min(name_.size(), sizeof(cmd.segname)));
		file.Write(&cmd, sizeof(cmd));
	}

	// write child sections
	for (i = 0; i < section_list->count(); i++) {
		MacSection *section = section_list->item(i);
		if (section->parent() == this)
			section->WriteToFile(file);
	}

	// write command size
	uint64_t current_pos = file.Tell();
	file.Seek(command_pos + offsetof(segment_command, cmdsize));
	uint32_t command_size = static_cast<uint32_t>(current_pos - command_pos);
	file.Write(&command_size, sizeof(command_size));
	file.Seek(current_pos);
}

void MacSegment::update_type(uint32_t mt) 
{ 
	if (mt & mtReadable)
		initprot_ |= VM_PROT_READ;
	if (mt & mtWritable)
		initprot_ |= VM_PROT_WRITE;
	if (mt & mtExecutable)
		initprot_ |= VM_PROT_EXECUTE;

	maxprot_ = (initprot_ == VM_PROT_NONE) ? initprot_ : VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
}

void MacSegment::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * MacSegmentList
 */

MacSegmentList::MacSegmentList(MacArchitecture *owner)
	: BaseSectionList(owner)
{

}

MacSegmentList::MacSegmentList(MacArchitecture *owner, const MacSegmentList &src)
	: BaseSectionList(owner, src)
{

}

MacSegmentList *MacSegmentList::Clone(MacArchitecture *owner) const
{
	MacSegmentList *list = new MacSegmentList(owner, *this);
	return list;
}

MacSegment *MacSegmentList::item(size_t index) const
{ 
	return reinterpret_cast<MacSegment*>(BaseSectionList::item(index));
}

MacSegment *MacSegmentList::Add()
{
	MacSegment *segment = new MacSegment(this);
	AddObject(segment);
	return segment;
}

void MacSegmentList::ReadFromFile(MacArchitecture &file)
{
	uint32_t cmd_type = (file.cpu_address_size() == osDWord) ? LC_SEGMENT : LC_SEGMENT_64;
	MacLoadCommandList *command_list = file.command_list();
	for (size_t i = 0; i < command_list->count(); i++) {
		MacLoadCommand *command = command_list->item(i);
		if (command->type() == cmd_type) {
			file.Seek(command->address());
			Add()->ReadFromFile(file);
		}
	}
}

void MacSegmentList::WriteToFile(MacArchitecture &file)
{
	uint32_t cmd_type = (file.cpu_address_size() == osDWord) ? LC_SEGMENT : LC_SEGMENT_64;
	for (size_t i = 0; i < count(); i++) {
		MacSegment *segment = item(i);
		file.command_list()->Add(cmd_type, segment);
	}
}

MacSegment *MacSegmentList::last() const
{
	return reinterpret_cast<MacSegment *>(BaseSectionList::last());
}

MacSegment *MacSegmentList::Add(uint64_t address, uint64_t size, uint32_t physical_offset, uint32_t physical_size, uint32_t initprot, const std::string &name)
{
	MacSegment *segment = new MacSegment(this, address, size, physical_offset, physical_size, initprot, name);
	AddObject(segment);
	return segment;
}

MacSegment *MacSegmentList::GetSectionByName(const std::string &name) const
{
	return reinterpret_cast<MacSegment *>(BaseSectionList::GetSectionByName(name));
}

MacSegment *MacSegmentList::GetSectionByAddress(uint64_t address) const
{
	return reinterpret_cast<MacSegment *>(BaseSectionList::GetSectionByAddress(address));
}

MacSegment *MacSegmentList::GetBaseSegment() const
{
	for (size_t i = 0; i < count(); i++) {
		MacSegment *segment = item(i);
		if (segment->physical_offset() == 0 && segment->physical_size())
			return segment;
	}
	return NULL;
}

/**
 * MacStringTable
 */

std::string MacStringTable::GetString(uint32_t pos) const 
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

uint32_t MacStringTable::AddString(const std::string &str)
{
	if (str.empty())
		return 1;

	uint32_t res = static_cast<uint32_t>(data_.size());
	data_.insert(data_.end(), str.c_str(), str.c_str() + str.size() + 1);

	return res;
};

void MacStringTable::clear() 
{ 
	data_.clear(); 
	AddString(" ");
}

void MacStringTable::ReadFromFile(MacArchitecture &file)
{
	symtab_command *symtab = file.symtab();

	if (!symtab->cmd)
		return;

	data_.resize(symtab->strsize);
	file.Seek(symtab->stroff);
	file.Read(data_.data(), data_.size());
}

void MacStringTable::WriteToFile(MacArchitecture &file)
{
	symtab_command *symtab = file.symtab();
	// align table size
	size_t aligned_size = AlignValue(data_.size(), OperandSizeToValue(file.cpu_address_size()));
	while (data_.size() < aligned_size) {
		data_.push_back(0);
	}

	symtab->strsize = (uint32_t)data_.size();
	if (!symtab->strsize) {
		symtab->stroff = 0;
		return;
	}

	symtab->stroff = static_cast<uint32_t>(file.Tell());
	file.Write(data_.data(), data_.size());
}

/**
 * MacSymbol
 */

MacSymbol::MacSymbol(MacSymbolList *owner)
	: IObject(), owner_(owner), type_(0), sect_(0), desc_(0), value_(0), is_deleted_(false)
{

}

MacSymbol::MacSymbol(MacSymbolList *owner, const MacSymbol &src)
	: IObject(src), owner_(owner)
{
	type_ = src.type_;
	sect_ = src.sect_;
	desc_ = src.desc_;
	value_ = src.value_;
	name_ = src.name_;
	is_deleted_ = src.is_deleted_;
}

MacSymbol::~MacSymbol()
{
	if (owner_)
		owner_->RemoveObject(this);
}

MacSymbol *MacSymbol::Clone(MacSymbolList *owner) const
{
	MacSymbol *symbol = new MacSymbol(owner, *this);
	return symbol;
}

void MacSymbol::ReadFromFile(MacArchitecture &file)
{
	uint32_t strx;
	if (file.cpu_address_size() == osDWord) {
		struct nlist nl;
		file.Read(&nl, sizeof(nl));
		strx = nl.n_un.n_strx;
		type_ = nl.n_type;
		sect_ = nl.n_sect;
		desc_ = nl.n_desc;
		value_ = nl.n_value;
	} else {
		nlist_64 nl;
		file.Read(&nl, sizeof(nl));
		strx = nl.n_un.n_strx;
		type_ = nl.n_type;
		sect_ = nl.n_sect;
		desc_ = nl.n_desc;
		value_ = nl.n_value;
	}

	name_ = file.string_table()->GetString(strx);
}

void MacSymbol::WriteToFile(MacArchitecture &file)
{
	uint32_t strx = file.string_table()->AddString(name_);

	if ((type_ & (N_STAB | N_TYPE)) == N_SECT && sect_ != NO_SECT) {
		for (size_t i = 0; i < file.section_list()->count(); i++) {
			MacSection *section = file.section_list()->item(i);
			if (value_ < section->address() + section->size()) {
				sect_ = static_cast<uint8_t>(i + 1);
				break;
			}
		}
	}
	
	if (file.cpu_address_size() == osDWord) {
		struct nlist nl;
		nl.n_un.n_strx = strx;
		nl.n_type = type_;
		nl.n_sect = sect_;
		nl.n_desc = desc_;
		nl.n_value = static_cast<uint32_t>(value_);
		file.Write(&nl, sizeof(nl));
	} else {
		nlist_64 nl;
		nl.n_un.n_strx = strx;
		nl.n_type = type_;
		nl.n_sect = sect_;
		nl.n_desc = desc_;
		nl.n_value = value_;
		file.Write(&nl, sizeof(nl));
	}
}

uint8_t MacSymbol::library_ordinal() const
{
	if ((type_ & (N_STAB | N_TYPE)) == N_UNDF)
		return (desc_ >> 8) & 0xff;

	return 0;
}

void MacSymbol::set_library_ordinal(uint8_t library_ordinal)
{
	if ((type_ & (N_STAB | N_TYPE)) == N_UNDF) {
		desc_ &= 0xff;
		desc_ |= library_ordinal << 8;
	}
}

/**
 * MacSymbolList
 */

MacSymbolList::MacSymbolList()
	: ObjectList<MacSymbol>()
{
	
}

MacSymbolList::MacSymbolList(const MacSymbolList &src)
	: ObjectList<MacSymbol>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

MacSymbolList *MacSymbolList::Clone() const
{
	MacSymbolList *list = new MacSymbolList(*this);
	return list;
}

void MacSymbolList::ReadFromFile(MacArchitecture &file, size_t count)
{
	symtab_command *symtab = file.symtab();
	if (!symtab->cmd)
		return;

	file.Seek(symtab->symoff);
	for (size_t i = 0; i < count; i++) {
		MacSymbol *symbol = new MacSymbol(this);
		symbol->ReadFromFile(file);
		AddObject(symbol);
	}
}

void MacSymbolList::WriteToFile(MacArchitecture &file)
{
	symtab_command *symtab = file.symtab();
	symtab->nsyms = static_cast<uint32_t>(count());
	symtab->symoff = static_cast<uint32_t>(file.Tell());

	dysymtab_command *dysymtab = file.dysymtab();
	if (!dysymtab->cmd)
		return;

	dysymtab->ilocalsym = 0;
	dysymtab->nlocalsym = 0;
	dysymtab->iextdefsym = 0;
	dysymtab->nextdefsym = 0;
	dysymtab->iundefsym = 0;
	dysymtab->nundefsym = 0;

	for (size_t i = 0; i < count(); i++) {
		MacSymbol *symbol = item(i);
		symbol->WriteToFile(file);

		if ((symbol->type() & (N_STAB | N_TYPE)) == N_UNDF) {
			// undefined symbol
			if (!dysymtab->nundefsym)
				dysymtab->iundefsym = static_cast<uint32_t>(i);
			dysymtab->nundefsym++;
		} else if (symbol->type() & N_EXT) {
			// external symbol
			if (!dysymtab->nextdefsym)
				dysymtab->iextdefsym = static_cast<uint32_t>(i);
			dysymtab->nextdefsym++;
		} else {
			// local symbol
			if (!dysymtab->nlocalsym)
				dysymtab->ilocalsym = static_cast<uint32_t>(i);
			dysymtab->nlocalsym++;
		}
	}
}

MacSymbol *MacSymbolList::GetSymbol(const std::string &name, int library_ordinal) const
{
	for (size_t i = 0; i < count(); i++) {
		MacSymbol *symbol = item(i);
		if ((symbol->type() & (N_STAB | N_TYPE)) == N_UNDF && symbol->name() == name) {
			if (library_ordinal < 0 || library_ordinal == symbol->library_ordinal())
				return symbol;
		}
	}
	return NULL;
}

void MacSymbolList::Pack()
{
	for (size_t i = count(); i > 0 ; i--) {
		MacSymbol *symbol = item(i - 1);
		if (symbol->is_deleted())
			delete symbol;
	}
}

/**
 * MacSection
 */

MacSection::MacSection(MacSectionList *owner, MacSegment *parent)
	: BaseSection(owner), address_(0), size_(0), offset_(0), align_(0), reloff_(0),
	nreloc_(0), flags_(0), reserved1_(0), reserved2_(0), parent_(parent)
{

}

MacSection::MacSection(MacSectionList *owner, MacSegment *parent, uint64_t address, uint32_t size, uint32_t offset, uint32_t flags, const std::string &name, const std::string &segment_name)
	: BaseSection(owner), name_(name), address_(address), size_(size), offset_(offset), align_(0), reloff_(0),
	nreloc_(0), flags_(flags), reserved1_(0), reserved2_(0), parent_(parent), segment_name_(segment_name)
{

}

MacSection::~MacSection()
{

}

MacSection::MacSection(MacSectionList *owner, const MacSection &src)
	: BaseSection(owner, src)
{
	parent_ = src.parent_;
	address_ = src.address_;
	size_ = src.size_;
	offset_ = src.offset_;
	name_ = src.name_;
	align_ = src.align_;
	reloff_ = src.reloff_;
	nreloc_ = src.nreloc_;
	flags_ = src.flags_;
	reserved1_ = src.reserved1_;
	reserved2_ = src.reserved2_;
	segment_name_ = src.segment_name_;
}

MacSection *MacSection::Clone(ISectionList *owner) const
{
	MacSection *section = new MacSection(reinterpret_cast<MacSectionList *>(owner), *this);
	return section;
}

void MacSection::ReadFromFile(MacArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		section sec;
		file.Read(&sec, sizeof(sec));
		name_ = std::string(sec.sectname, strnlen(sec.sectname, sizeof(sec.sectname)));
		segment_name_ = std::string(sec.segname, strnlen(sec.segname, sizeof(sec.segname)));
		address_ = sec.addr;
		size_ = sec.size;
		offset_ = sec.offset;
		align_ = sec.align;
		reloff_ = sec.reloff;
		nreloc_ = sec.nreloc;
		flags_ = sec.flags;
		reserved1_ = sec.reserved1;
		reserved2_ = sec.reserved2;
	} else {
		section_64 sec;
		file.Read(&sec, sizeof(sec));
		name_ = std::string(sec.sectname, strnlen(sec.sectname, sizeof(sec.sectname)));
		segment_name_ = std::string(sec.segname, strnlen(sec.segname, sizeof(sec.segname)));
		address_ = sec.addr;
		size_ = static_cast<uint32_t>(sec.size);
		offset_ = sec.offset;
		align_ = sec.align;
		reloff_ = sec.reloff;
		nreloc_ = sec.nreloc;
		flags_ = sec.flags;
		reserved1_ = sec.reserved1;
		reserved2_ = sec.reserved2;
	}
}

void MacSection::WriteToFile(MacArchitecture &file)
{
	if (file.cpu_address_size() == osDWord) {
		section sec = section();
		sec.addr = static_cast<uint32_t>(address_);
		sec.size = static_cast<uint32_t>(size_);
		sec.offset = offset_;
		sec.align = align_;
		sec.reloff = reloff_;
		sec.nreloc = nreloc_;
		sec.flags = flags_;
		sec.reserved1 = reserved1_;
		sec.reserved2 = reserved2_;
		memcpy(sec.sectname, name_.c_str(), std::min(name_.size(), sizeof(sec.sectname)));
		memcpy(sec.segname, segment_name_.c_str(), std::min(segment_name_.size(), sizeof(sec.segname)));
		file.Write(&sec, sizeof(sec));
	} else {
		section_64 sec = section_64();
		sec.addr = address_;
		sec.size = size_;
		sec.offset = offset_;
		sec.align = align_;
		sec.reloff = reloff_;
		sec.nreloc = nreloc_;
		sec.flags = flags_;
		sec.reserved1 = reserved1_;
		sec.reserved2 = reserved2_;
		memcpy(sec.sectname, name_.c_str(), std::min(name_.size(), sizeof(sec.sectname)));
		memcpy(sec.segname, segment_name_.c_str(), std::min(segment_name_.size(), sizeof(sec.segname)));
		file.Write(&sec, sizeof(sec));
	}
}

void MacSection::set_alignment(size_t value)
{ 
	align_ = 0;
	for (uint32_t i = 0; value && i < 8 * sizeof(size_t); i++) {
		if (value & 1) {
			align_ = i;
			break;
		}
		value >>= 1;
	}
}

void MacSection::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * MacSectionList
 */

MacSectionList::MacSectionList(MacArchitecture *owner)
	: BaseSectionList(owner)
{

}

MacSectionList::MacSectionList(MacArchitecture *owner, const MacSectionList &src)
	: BaseSectionList(owner, src)
{

}

MacSectionList *MacSectionList::Clone(MacArchitecture *owner) const
{
	MacSectionList *list = new MacSectionList(owner, *this);
	return list;
}

MacSection *MacSectionList::Add(MacSegment *segment, uint64_t address, uint32_t size, uint32_t offset, uint32_t flags, const std::string &name, const std::string &segment_name)
{
	MacSection *section = new MacSection(this, segment, address, size, offset, flags, name, segment_name);
	AddObject(section);
	return section;
}

MacSection *MacSectionList::GetSectionByAddress(uint64_t address) const
{
	return reinterpret_cast<MacSection *>(BaseSectionList::GetSectionByAddress(address));
}

MacSection *MacSectionList::GetSectionByName(const std::string &name) const
{
	return reinterpret_cast<MacSection *>(BaseSectionList::GetSectionByName(name));
}

MacSection *MacSectionList::GetSectionByName(ISection *segment, const std::string &name) const
{
	return reinterpret_cast<MacSection *>(BaseSectionList::GetSectionByName(segment, name));
}

void MacSectionList::ReadFromFile(MacArchitecture &file, size_t count, MacSegment *segment)
{
	for (size_t i = 0; i < count; i++) {
		MacSection *section = new MacSection(this, segment);
		AddObject(section);
		section->ReadFromFile(file);
	}
}

MacSection *MacSectionList::item(size_t index) const
{ 
	return reinterpret_cast<MacSection*>(BaseSectionList::item(index));
}

/**
 * MacImportFunction
 */

MacImportFunction::MacImportFunction(IImport *owner, uint64_t address, uint8_t bind_type, size_t bind_offset, const std::string &name, uint32_t flags, int64_t addend, bool is_lazy, MacSymbol *symbol)
		: BaseImportFunction(owner), address_(address), bind_type_(bind_type), name_(name),
		flags_(flags), addend_(addend), is_lazy_(is_lazy), symbol_(symbol), bind_offset_(bind_offset)
{

}

MacImportFunction::MacImportFunction(IImport *owner, uint64_t address, APIType type, MapFunction *map_function)
	: BaseImportFunction(owner), address_(address), bind_type_(0), flags_(0), 
	addend_(0), is_lazy_(false), symbol_(NULL), bind_offset_(0)
{
	set_type(type);
	set_map_function(map_function);
}

MacImportFunction::MacImportFunction(IImport *owner, const MacImportFunction &src)
	: BaseImportFunction(owner, src)
{
	address_ = src.address_;
	bind_type_ = src.bind_type_;
	name_ = src.name_;
	flags_ = src.flags_;
	addend_ = src.addend_;
	is_lazy_ = src.is_lazy_;
	bind_offset_ = src.bind_offset_;
	symbol_ = src.symbol_;
}

MacImportFunction *MacImportFunction::Clone(IImport *owner) const
{
	MacImportFunction *func = new MacImportFunction(owner, *this);
	return func;
}

int MacImportFunction::library_ordinal() const 
{ 
	return reinterpret_cast<MacImport *>(owner())->library_ordinal(); 
}

bool MacImportFunction::is_weak() const
{
	if (!symbol_)
		return false;

	if ((symbol_->type() & N_TYPE) == N_SECT)
		return ((symbol_->desc() & N_WEAK_DEF) != 0);
	else
		return ((symbol_->desc() & N_REF_TO_WEAK) != 0); //-V
}

void MacImportFunction::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
}

std::string MacImportFunction::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

/**
 * MacImport
 */

MacImport::MacImport(MacImportList *owner, int library_ordinal, bool is_sdk)
	: BaseImport(owner), library_ordinal_(library_ordinal), is_sdk_(is_sdk), timestamp_(0), current_version_(0), compatibility_version_(0),
	is_weak_(false)
{

}

MacImport::MacImport(MacImportList *owner, int library_ordinal, const std::string &name, uint32_t current_version, uint32_t compatibility_version)
	: BaseImport(owner), library_ordinal_(library_ordinal), name_(name), is_sdk_(false), timestamp_(0), current_version_(current_version), compatibility_version_(compatibility_version),
	is_weak_(false)
{

}

MacImport::MacImport(MacImportList *owner, const MacImport &src)
	: BaseImport(owner, src)
{
	library_ordinal_ = src.library_ordinal_;
	name_ = src.name_;
	is_sdk_ = src.is_sdk_;
	timestamp_ = src.timestamp_;
	current_version_ = src.current_version_;
	compatibility_version_ = src.compatibility_version_;
	is_weak_ = src.is_weak_;
}

MacImport *MacImport::Clone(IImportList *owner) const
{
	MacImport *list = new MacImport(reinterpret_cast<MacImportList *>(owner), *this);
	return list;
}

void MacImport::ReadFromFile(MacArchitecture &file)
{
	dylib_command command;
	std::vector<char> namebuf;
	size_t namebufsize;
	uint64_t pos;

	pos = file.Tell();
	/* Read command header, verify the command */
	file.Read(&command, sizeof(command));
	if (command.cmdsize < sizeof(dylib_command) || command.dylib.name.offset >= command.cmdsize)
		throw std::runtime_error("Invalid dylib_command");

	timestamp_ = command.dylib.timestamp;
	current_version_ = command.dylib.current_version;
	compatibility_version_ = command.dylib.compatibility_version;
	namebufsize = command.cmdsize - command.dylib.name.offset;
	namebuf.resize(namebufsize);
	file.Seek(command.dylib.name.offset + pos);
	file.Read(&namebuf[0], namebuf.size());
	name_ = NameToString(&namebuf[0], namebuf.size());
}

MacImportFunction *MacImport::Add(uint64_t address, uint8_t bind_type, size_t bind_offset, const std::string & name, uint32_t flags, int64_t addend, bool is_lazy, MacSymbol *symbol)
{
	MacImportFunction *import_function = new MacImportFunction(this, address, bind_type, bind_offset, name, flags, addend, is_lazy, symbol);
	AddObject(import_function);
	return import_function;
}

void MacImport::WriteToFile(MacArchitecture &file)
{
	size_t size;
	dylib_command command;

	command.cmd = is_weak_ ? LC_LOAD_WEAK_DYLIB : LC_LOAD_DYLIB;
	command.cmdsize = (uint32_t)AlignValue(sizeof(command) + name_.size() + 1, OperandSizeToValue(file.cpu_address_size()));
	command.dylib.name.offset = sizeof(command);
	command.dylib.timestamp = timestamp_;
	command.dylib.current_version = current_version_;
	command.dylib.compatibility_version = compatibility_version_;
	// write command
	size = file.Write(&command, sizeof(command));
	size += file.Write(name_.c_str(), name_.size() + 1);
	// align
	uint8_t b = 0;
	while (size < command.cmdsize) {
		size += file.Write(&b, sizeof(b));
	}
}

MacImportFunction *MacImport::item(size_t index) const 
{ 
	return reinterpret_cast<MacImportFunction*>(BaseImport::item(index));
}

MacImportFunction *MacImport::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<MacImportFunction*>(BaseImport::GetFunctionByAddress(address));
}

MacImportFunction *MacImport::Add(uint64_t address, APIType type, MapFunction *map_function)
{
	MacImportFunction *import_function = new MacImportFunction(this, address, type, map_function);
	AddObject(import_function);
	return import_function;
}

/**
 * MacImportList
 */

MacImportList::MacImportList(MacArchitecture *owner)
	: BaseImportList(owner)
{

}

MacImportList::MacImportList(MacArchitecture *owner, const MacImportList &src)
	: BaseImportList(owner, src)
{

}

MacImportList *MacImportList::Clone(MacArchitecture *owner) const
{
	MacImportList *list = new MacImportList(owner, *this);
	return list;
}

MacImport *MacImportList::Add(int library_ordinal)
{
	MacImport *imp = new MacImport(this, library_ordinal);
	AddObject(imp);
	return imp;
}

MacImport *MacImportList::AddSDK()
{
	MacImport *sdk = new MacImport(this, 0, true);
	AddObject(sdk);
	return sdk;
}

MacImport *MacImportList::item(size_t index) const 
{ 
	return reinterpret_cast<MacImport*>(IImportList::item(index));
}

MacImportFunction *MacImportList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<MacImportFunction *>(BaseImportList::GetFunctionByAddress(address));
}

MacImport *MacImportList::GetLibraryByOrdinal(int library_ordinal) const
{
	for (size_t i = 0; i < count(); i++) {
		MacImport *import = item(i);
		if (import->library_ordinal() == library_ordinal)
			return import;
	}

	return NULL;
}

MacImport *MacImportList::GetImportByName(const std::string &name) const
{
	return reinterpret_cast<MacImport *>(BaseImportList::GetImportByName(name));
}

void MacImportList::ReadFromFile(MacArchitecture &file)
{
	size_t i, j;

	ILoadCommandList *command_list = file.command_list();
	int library_ordinal = 1;
	size_t c = command_list->count();
	for (i = 0; i < c; i++) {
		ILoadCommand *command = command_list->item(i);
		if (command->type() == LC_LOAD_DYLIB || command->type() == LC_LOAD_WEAK_DYLIB) {
			file.Seek(command->address());
			MacImport *import = Add(library_ordinal++);
			import->ReadFromFile(file);
			if (command->type() == LC_LOAD_WEAK_DYLIB)
				import->set_is_weak(true);
		}
	}

	dyld_info_command *dyld_info = file.dyld_info();
	if (dyld_info->cmd) {
		ReadBindInfo(file, dyld_info->bind_off, dyld_info->bind_size);
		ReadLazyBindInfo(file, dyld_info->lazy_bind_off, dyld_info->lazy_bind_size);
	} else {
		MacIndirectSymbolList *indirect_symbol_list = file.indirect_symbol_list(); 
		for (i = 0; i < indirect_symbol_list->count(); i++) {
			MacIndirectSymbol *indirect_symbol = indirect_symbol_list->item(i);
			MacSymbol *symbol = indirect_symbol->symbol();
			if (symbol && (symbol->type() & (N_STAB | N_TYPE)) == N_UNDF) {
				MacSection *section = file.section_list()->GetSectionByAddress(indirect_symbol->address());
				if (section && (section->type() == S_LAZY_SYMBOL_POINTERS 
					|| section->type() == S_NON_LAZY_SYMBOL_POINTERS 
					|| (section->type() == S_SYMBOL_STUBS && file.cpu_address_size() == osDWord && (section->flags() & S_ATTR_SELF_MODIFYING_CODE) && section->reserved2() == 5))) {
					library_ordinal = symbol->library_ordinal();
					if (library_ordinal > 0) {
						MacImport *import = GetLibraryByOrdinal(library_ordinal);
						if (!import)
							throw std::runtime_error("Invalid library ordinal");
						MacImportFunction *import_function = import->Add(indirect_symbol->address(), 0, 0, symbol->name(), 0, 0, section->type() != S_NON_LAZY_SYMBOL_POINTERS, symbol);
						if (section->type() == S_SYMBOL_STUBS)
							import_function->include_option(ioIsRelative);
					}
				} else if (section && section->type() != S_SYMBOL_STUBS) {
					throw std::runtime_error("Invalid indirect symbol");
				}
			}
		}

		MacFixupList *fixup_list = file.fixup_list();
		for (i = 0; i < fixup_list->count(); i++) {
			MacFixup *fixup = fixup_list->item(i);
			MacSymbol *symbol = fixup->symbol();
			if (symbol && (symbol->type() & (N_STAB | N_TYPE)) == N_UNDF) {
				library_ordinal = symbol->library_ordinal();
				if (library_ordinal > 0) {
					MacImport *import = GetLibraryByOrdinal(library_ordinal);
					if (!import)
						throw std::runtime_error("Invalid library ordinal");
					import->Add(fixup->address(), 0, 0, symbol->name(), 0, 0, false, symbol);
				}
			}
		}
	}

	static const ImportInfo default_info[] = {
		{atNone, "_exit", ioNoReturn, ctNone},
		{atNone, "_abort", ioNoReturn, ctNone},
		{atNone, "__Unwind_Resume", ioNoReturn, ctNone},
		{atNone, "__Unwind_Resume_or_Rethrow", ioNoReturn, ctNone},
		{atNone, "__Unwind_RaiseException", ioNoReturn, ctNone},
		{atNone, "___assert_rtn", ioNoReturn, ctNone},
		{atNone, "___stack_chk_fail", ioNoReturn, ctNone},
		{atNone, "___cxa_throw", ioNoReturn, ctNone},
		{atNone, "___cxa_end_cleanup", ioNoReturn, ctNone},
		{atNone, "___cxa_rethrow", ioNoReturn, ctNone},
		{atNone, "___cxa_bad_cast", ioNoReturn, ctNone},
		{atNone, "___cxa_bad_typeid", ioNoReturn, ctNone},
		{atNone, "___cxa_call_terminate", ioNoReturn, ctNone},
		{atNone, "___cxa_pure_virtual", ioNoReturn, ctNone},
		{atNone, "___cxa_call_unexpected", ioNoReturn, ctNone},
		{atNone, "__ZSt9terminatev", ioNoReturn, ctNone},
		{atNone, "@System@@Halt0$qqrv", ioNoReturn, ctNone},
		{atNone, "@System@@UnhandledException$qqrv", ioNoReturn, ctNone},
	};

	MacImportFunction *func;
	for (size_t k = 0; k < count(); k++) {
		MacImport *import = item(k);

		std::string dll_name = os::ExtractFileName(import->name().c_str());
		std::transform(dll_name.begin(), dll_name.end(), dll_name.begin(), tolower);
		std::string sdk_name = "libvmprotectsdk.dylib";

		if (dll_name.compare(sdk_name) == 0) {
			import->set_is_sdk(true);
			if (import->count() == 0) {
				if (MacImport *flat_lookup_import = GetLibraryByOrdinal(BIND_SPECIAL_DYLIB_FLAT_LOOKUP)) {
					for (i = 0; i < flat_lookup_import->count(); ) {
						func = flat_lookup_import->item(i);
						if (GetSDKInfo(func->name())) {
							func->set_owner(import);
							continue;
						}
						i++;
					}
				}
			}

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

const ImportInfo *MacImportList::GetSDKInfo(const std::string &name) const
{
	if (!name.empty() && name[0] == '_')
		return BaseImportList::GetSDKInfo(name.substr(1));
	return NULL;
}

int MacImportList::GetMaxLibraryOrdinal() const
{
	int res = 0;
	for (size_t i = 0; i < count(); i++) {
		MacImport *import = item(i);
		if (import->library_ordinal() <= 0)
			continue;

		if (res < import->library_ordinal())
			res = import->library_ordinal();
	}
	return res;
}

void MacImportList::ReadBindInfo(MacArchitecture &file, uint32_t bind_off, uint32_t bind_size)
{
	size_t pos, seg_index = 0, i, c, skip, ptr_size;
	uint8_t imm, opcode, bind_type = 0, sym_flags = 0;
	int library_ordinal = 0;
	bool done;
	std::string sym_name;
	int64_t addend = 0;
	uint64_t addr, end_addr;
	ISectionList *segment_list = file.segment_list();
	ISection *segment;
	MacImport *import = NULL;
	EncodedData buf;
	MacSymbol *symbol = NULL;

	if (!bind_size)
		return;

	if (segment_list->count() < 1)
		throw std::runtime_error("Invalid segment count in bind info");
	
	file.Seek(bind_off);
	buf.ReadFromFile(file, bind_size);

	ptr_size = file.cpu_address_size() == osQWord ? 8 : 4;

	segment = segment_list->item(0);
	addr = segment->address();
	end_addr = segment->address() + segment->size();

	done = false;
	for (pos = 0; !done && pos < bind_size; ) {
		uint8_t b = buf.ReadByte(&pos);
		imm = b & BIND_IMMEDIATE_MASK;
		opcode = b & BIND_OPCODE_MASK;
		switch (opcode) {
		case BIND_OPCODE_DONE:
			done = true;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
			library_ordinal = imm;
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
			library_ordinal = static_cast<int>(buf.ReadUleb128(&pos));
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
			/* the special ordinals are negative numbers */
			if (imm == 0) {
				library_ordinal = 0;
			} else {
				int8_t sign_extended = BIND_OPCODE_MASK | imm;
				library_ordinal = sign_extended;
			}
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
			sym_flags = imm;
			sym_name = buf.ReadString(&pos);
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_TYPE_IMM:
			bind_type = imm;
			break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
			addend = buf.ReadSleb128(&pos);
			break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			seg_index = imm;
			if (seg_index >= segment_list->count())
				throw std::runtime_error("Invalid segment index in bind info");
			segment = segment_list->item(seg_index);
			addr = segment->address() + buf.ReadUleb128(&pos);
			end_addr = segment->address() + segment->size();
			break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
			addr += buf.ReadUleb128(&pos);
			break;
		case BIND_OPCODE_DO_BIND:
		case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
		case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
			if (addr >= end_addr)
				throw std::runtime_error("Invalid binding address in bind info");
			if (!import) {
				import = GetLibraryByOrdinal(library_ordinal);
				if (!import) {
					if (library_ordinal <= 0) {
						// special library
						import = Add(library_ordinal);
					}
					else {
						throw std::runtime_error("Invalid library ordinal in bind info");
					}
				}
			}
			if (!symbol) {
				symbol = file.symbol_list()->GetSymbol(sym_name, library_ordinal);
				if (!symbol)
					throw std::runtime_error("Invalid symbol in bind info");
			}
			import->Add(addr, bind_type, 0, sym_name, sym_flags, addend, false, symbol);
			if (opcode == BIND_OPCODE_DO_BIND) {
				addr += ptr_size;
			} else if (opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) {
				addr += buf.ReadUleb128(&pos) + ptr_size;
			} else if (opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED) {
				addr += (imm + 1) * ptr_size;
			}
			break;
		case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB:
			c = static_cast<size_t>(buf.ReadUleb128(&pos));
			skip = static_cast<size_t>(buf.ReadUleb128(&pos));
			for (i = 0; i < c; i++) {
				if (addr >= end_addr)
					throw std::runtime_error("Invalid binding address in bind info");
				if (!import) {
					import = GetLibraryByOrdinal(library_ordinal);
					if (!import) {
						if (library_ordinal <= 0) {
							// special library
							import = Add(library_ordinal);
						}
						else {
							throw std::runtime_error("Invalid library ordinal in bind info");
						}
					}
				}
				if (!symbol) {
					symbol = file.symbol_list()->GetSymbol(sym_name, library_ordinal);
					if (!symbol)
						throw std::runtime_error("Invalid symbol in bind info");
				}
				import->Add(addr, bind_type, 0, sym_name, sym_flags, addend, false, symbol);
				addr += skip + ptr_size;
			}
			break;
		default:
			throw std::runtime_error("Invalid bind opcode in bind info");
		}
	}
}

void MacImportList::ReadLazyBindInfo(MacArchitecture &file, uint32_t lazy_bind_off, uint32_t lazy_bind_size)
{
	uint8_t bind_type = BIND_TYPE_POINTER, imm, opcode, sym_flags = 0;
	int library_ordinal = 0;
	size_t pos, seg_index, ptrsize;
	uint64_t addr, end_addr;
	ISectionList *segment_list = file.segment_list();
	ISection *segment;
	MacImport *import = NULL;
	std::string sym_name;
	int64_t addend = 0;
	EncodedData buf;
	MacSymbol *symbol = NULL;

	if (!lazy_bind_size)
		return;

	if (segment_list->count() < 1)
		throw std::runtime_error("Invalid segment count in lazy bind info");

	file.Seek(lazy_bind_off);
	buf.ReadFromFile(file, lazy_bind_size);

	ptrsize = OperandSizeToValue(file.cpu_address_size());

	segment = segment_list->item(0);
	addr = segment->address();
	end_addr = segment->address() + segment->size();

	size_t bind_offset = (size_t)-1;
	for (pos = 0; pos < lazy_bind_size; ) {
		if (bind_offset == (size_t)-1)
			bind_offset = pos;
		uint8_t b = buf.ReadByte(&pos);
		imm = b & BIND_IMMEDIATE_MASK;
		opcode = b & BIND_OPCODE_MASK;
		switch (opcode) {
		case BIND_OPCODE_DONE:
			/* There is BIND_OPCODE_DONE at end of each lazy bind,
			 * don't stop until end of whole sequence. */
			bind_offset = (size_t)-1;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
			library_ordinal = imm;
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
			library_ordinal = static_cast<int>(buf.ReadUleb128(&pos));
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
			/* the special ordinals are negative numbers */
			if (imm == 0) {
				library_ordinal = 0;
			} else {
				int8_t sign_extended = BIND_OPCODE_MASK | imm;
				library_ordinal = sign_extended;
			}
			import = NULL;
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
			sym_flags = imm;
			sym_name = buf.ReadString(&pos);
			symbol = NULL;
			break;
		case BIND_OPCODE_SET_TYPE_IMM:
			bind_type = imm;
			break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
			addend = buf.ReadSleb128(&pos);
			break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			seg_index = imm;
			if (seg_index >= segment_list->count())
				throw std::runtime_error("Invalid segment index in lazy bind info");
			segment = segment_list->item(seg_index);
			addr = segment->address() + buf.ReadUleb128(&pos);
			end_addr = segment->address() + segment->size();
			break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
			addr += buf.ReadUleb128(&pos);
			break;
		case BIND_OPCODE_DO_BIND:
			if (addr >= end_addr)
				throw std::runtime_error("Invalid binding address in lazy bind info");
			if (!import) {
				import = GetLibraryByOrdinal(library_ordinal);
				if (!import) {
					if (library_ordinal <= 0) {
						// special library
						import = Add(library_ordinal);
					}
					else {
						throw std::runtime_error("Invalid library ordinal in lazy bind info");
					}
				}
			}
			if (!symbol) {
				symbol = file.symbol_list()->GetSymbol(sym_name, library_ordinal);
				if (!symbol)
					throw std::runtime_error("Invalid symbol in lazy bind info");
			}
			import->Add(addr, bind_type, bind_offset, sym_name, sym_flags, addend, true, symbol);
			addr += ptrsize;
			break;
		default:
			throw std::runtime_error("Invalid bind opcode in lazy bind info");
		}
	}
}

void MacImportList::Pack()
{
	if (!has_sdk())
		return;

	size_t i, j;
	MacImport *import;
	uint8_t library_ordinal = 1;

	for (i = 0; i < count(); i++) {
		import = item(i);
		if (import->is_sdk() || import->library_ordinal() <= 0)
			continue;

		import->set_library_ordinal(library_ordinal);
		for (j = 0; j < import->count(); j++) {
			import->item(j)->symbol()->set_library_ordinal(library_ordinal);
		}
		library_ordinal++;
	}

	for (i = count(); i > 0; i--) {
		import = item(i - 1);
		if (!import->is_sdk())
			continue;

		for (j = 0; j < import->count(); j++) {
			import->item(j)->symbol()->set_deleted(true);
		}
		delete import;
	}
}

void MacImportList::WriteToFile(MacArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		MacImport *import = item(i);
		if (import->library_ordinal() <= 0)
			continue;

		file.command_list()->Add(import->is_weak() ? LC_LOAD_WEAK_DYLIB : LC_LOAD_DYLIB, import);
	}

	dyld_info_command *dyld_info = file.dyld_info();
	if (!dyld_info->cmd)
		return;

	dyld_info->bind_off = static_cast<uint32_t>(file.Tell());
	dyld_info->bind_size = static_cast<uint32_t>(WriteBindInfo(file));
	if (!dyld_info->bind_size)
		dyld_info->bind_off = 0;

	dyld_info->weak_bind_off = static_cast<uint32_t>(file.Tell());
	dyld_info->weak_bind_size = static_cast<uint32_t>(WriteWeakBindInfo(file));
	if (!dyld_info->weak_bind_size)
		dyld_info->weak_bind_off = 0;

	dyld_info->lazy_bind_off = static_cast<uint32_t>(file.Tell());
	dyld_info->lazy_bind_size = static_cast<uint32_t>(WriteLazyBindInfo(file));
	if (!dyld_info->lazy_bind_size)
		dyld_info->lazy_bind_off = 0;
}

struct BindingInfo {
	uint8_t opcode;
	uint64_t operand1;
	uint64_t operand2;
	std::string name;
	BindingInfo(uint8_t opcode_, uint64_t operand1_, uint64_t operand2_ = 0, const std::string &name_ = "")
	{
		opcode = opcode_;
		operand1 = operand1_;
		operand2 = operand2_;
		name = name_;
	}
};

size_t MacImportList::WriteBindInfo(MacArchitecture &file)
{
	size_t i, j;
	std::vector<MacImportFunction *> info;
	MacImport *import;
	MacImportFunction *import_func;

	for (i = 0; i < count(); i++) {
		import = item(i);
		for (j = 0; j < import->count(); j++) {
			import_func = import->item(j);
			if (import_func->is_lazy())
				continue;
			info.push_back(import_func);
		}
	}
	if (info.size() == 0)
		return 0;

	std::sort(info.begin(), info.end(), BindInfoHelper());

	std::vector<BindingInfo> mid;
	MacSegment *segment = NULL;
    int library_ordinal = 0x80000000;
    std::string symbol_name;
    uint8_t bind_type = 0;
    uint64_t address = -1;
    int64_t addend = 0;
	size_t ptrsize = OperandSizeToValue(file.cpu_address_size());

	for (std::vector<MacImportFunction *>::iterator it = info.begin(); it != info.end() ; ++it) {
		import_func = *it;
		if (library_ordinal != import_func->library_ordinal()) {
			if (import_func->library_ordinal() <= 0) {
				// special lookups are encoded as negative numbers in BindingInfo
				mid.push_back(BindingInfo(BIND_OPCODE_SET_DYLIB_SPECIAL_IMM, import_func->library_ordinal()));
			} else {
				mid.push_back(BindingInfo(BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB, import_func->library_ordinal()));
			}
			library_ordinal = import_func->library_ordinal();
		}

		if (symbol_name != import_func->name()) {
			mid.push_back(BindingInfo(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM, import_func->flags(), 0, import_func->name()));
			symbol_name = import_func->name();
		}
		if (bind_type != import_func->bind_type()) {
			mid.push_back(BindingInfo(BIND_OPCODE_SET_TYPE_IMM, import_func->bind_type()));
			bind_type = import_func->bind_type();
		}
		if (address != import_func->address()) {
			if (!segment || (import_func->address() < segment->address()) || (import_func->address() >= segment->address() + segment->size()) || import_func->address() < address) {
				segment = file.segment_list()->GetSectionByAddress(import_func->address());
				if (!segment)
					throw std::runtime_error("Invalid binding address in bind info");
				mid.push_back(BindingInfo(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB, file.segment_list()->IndexOf(segment), import_func->address() - segment->address()));
			} else {
				mid.push_back(BindingInfo(BIND_OPCODE_ADD_ADDR_ULEB, import_func->address() - address));
			}
			address = import_func->address();
		}
		if (addend != import_func->addend()) {
			mid.push_back(BindingInfo(BIND_OPCODE_SET_ADDEND_SLEB, import_func->addend()));
			addend = import_func->addend();
		}
		mid.push_back(BindingInfo(BIND_OPCODE_DO_BIND, 0));
		address += ptrsize;
	}
	mid.push_back(BindingInfo(BIND_OPCODE_DONE, 0));

	// optimize phase 1, combine bind/add pairs
	BindingInfo *dst = &mid[0];
	for (const BindingInfo *src = &mid[0]; src->opcode != BIND_OPCODE_DONE; ++src) {
		if ((src->opcode == BIND_OPCODE_DO_BIND) 
				&& (src[1].opcode == BIND_OPCODE_ADD_ADDR_ULEB)) {
			dst->opcode = BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB;
			dst->operand1 = src[1].operand1;
			++src;
			++dst;
		} else {
			*dst++ = *src;
		}
	}
	dst->opcode = BIND_OPCODE_DONE;

	// optimize phase 2, compress packed runs of BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB with
	// same addr delta into one BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB
	dst = &mid[0];
	for (const BindingInfo *src = &mid[0]; src->opcode != BIND_OPCODE_DONE; ++src) {
		uint64_t delta = src->operand1;
		if ((src->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
				&& (src[1].opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
				&& (src[1].operand1 == delta)) {
			// found at least two in a row, this is worth compressing
			dst->opcode = BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB;
			dst->operand1 = 1;
			dst->operand2 = delta;
			++src;
			while ((src->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB)
					&& (src->operand1 == delta)) {
				dst->operand1++;
				++src;
			}
			--src;
			++dst;
		} else {
			*dst++ = *src;
		}
	}
	dst->opcode = BIND_OPCODE_DONE;
	
	// optimize phase 3, use immediate encodings
	for (BindingInfo *p = &mid[0]; p->opcode != REBASE_OPCODE_DONE; ++p) {
		if ((p->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
			&& (p->operand1 < (15 * ptrsize))
			&& ((p->operand1 % ptrsize) == 0)) {
			p->opcode = BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED;
			p->operand1 = p->operand1 / ptrsize;
		} else if ((p->opcode == BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB) && (p->operand1 <= 15)) {
			p->opcode = BIND_OPCODE_SET_DYLIB_ORDINAL_IMM;
		}
	}	
	dst->opcode = BIND_OPCODE_DONE; //-V519

	// convert to compressed encoding
	EncodedData data;
	data.reserve(info.size() * 2);
	bool done = false;
	for (std::vector<BindingInfo>::iterator it = mid.begin(); !done && it != mid.end() ; ++it) {
		switch ( it->opcode ) {
		case BIND_OPCODE_DONE:
			done = true;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_IMM | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_SPECIAL_IMM | (it->operand1 & BIND_IMMEDIATE_MASK));
			break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
			data.WriteByte(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM | static_cast<uint8_t>(it->operand1));
			data.WriteString(it->name);
			break;
		case BIND_OPCODE_SET_TYPE_IMM:
			data.WriteByte(BIND_OPCODE_SET_TYPE_IMM | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
			data.WriteByte(BIND_OPCODE_SET_ADDEND_SLEB);
			data.WriteSleb128(it->operand1);
			break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			data.WriteByte(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB | static_cast<uint8_t>(it->operand1));
			data.WriteUleb128(it->operand2);
			break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
			data.WriteByte(BIND_OPCODE_ADD_ADDR_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_DO_BIND:
			data.WriteByte(BIND_OPCODE_DO_BIND);
			break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
			data.WriteByte(BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
			data.WriteByte(BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB:
			data.WriteByte(BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB);
			data.WriteUleb128(it->operand1);
			data.WriteUleb128(it->operand2);
			break;
		}
	}
	data.resize(AlignValue(data.size(), ptrsize), 0);

	return (data.size() == 0) ? 0 : file.Write(data.data(), data.size());
}

size_t MacImportList::WriteWeakBindInfo(MacArchitecture &file)
{
	size_t i, j;
	std::vector<MacImportFunction *> info;
	std::vector<BindingInfo> mid;
	MacSegment *segment = NULL;
	std::string symbol_name;
	uint8_t bind_type = 0;
	uint64_t address = -1;
	int64_t addend = 0;
	size_t ptrsize = OperandSizeToValue(file.cpu_address_size());
	MacImport *import;
	MacImportFunction *import_func;

	for (i = 0; i < count(); i++) {
		import = item(i);
		for (j = 0; j < import->count(); j++) {
			import_func = import->item(j);
			if (!import_func->is_weak())
				continue;
			info.push_back(import_func);
		}
	}
	if (info.size() == 0) {
		// short circuit if no weak binding needed
		return 0;
	}
	std::sort(info.begin(), info.end(), WeakBindInfoHelper());

	for (std::vector<MacImportFunction *>::const_iterator it = info.begin(); it != info.end(); ++it) {
		import_func = *it;
		if (symbol_name != import_func->name()) {
			mid.push_back(BindingInfo(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM, import_func->flags(), 0, import_func->name()));
			symbol_name = import_func->name();
		}
		// non-weak symbols just have BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM
		// weak symbols have SET_SEG, ADD_ADDR, SET_ADDED, DO_BIND
		if (import_func->bind_type() != BIND_TYPE_OVERRIDE_OF_WEAKDEF_IN_DYLIB) {
			if (bind_type != import_func->bind_type()) {
				mid.push_back(BindingInfo(BIND_OPCODE_SET_TYPE_IMM, import_func->bind_type()));
				bind_type = import_func->bind_type();
			}
			if (address != import_func->address()) {
				if (!segment || (import_func->address() < segment->address()) || (import_func->address() >= segment->address() + segment->size())) {
					segment = file.segment_list()->GetSectionByAddress(import_func->address());
					if (!segment)
						throw std::runtime_error("binding address outside range of any segment");
					mid.push_back(BindingInfo(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB, file.segment_list()->IndexOf(segment), import_func->address() - segment->address()));
				}
				else {
					mid.push_back(BindingInfo(BIND_OPCODE_ADD_ADDR_ULEB, import_func->address() - address));
				}
				address = import_func->address();
			}
			if (addend != import_func->addend()) {
				mid.push_back(BindingInfo(BIND_OPCODE_SET_ADDEND_SLEB, import_func->addend()));
				addend = import_func->addend();
			}
			mid.push_back(BindingInfo(BIND_OPCODE_DO_BIND, 0));
			address += ptrsize;
		}
	}
	mid.push_back(BindingInfo(BIND_OPCODE_DONE, 0));

	// optimize phase 1, combine bind/add pairs
	BindingInfo *dst = &mid[0];
	for (const BindingInfo *src = &mid[0]; src->opcode != BIND_OPCODE_DONE; ++src) {
		if ( (src->opcode == BIND_OPCODE_DO_BIND) 
				&& (src[1].opcode == BIND_OPCODE_ADD_ADDR_ULEB) ) {
			dst->opcode = BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB;
			dst->operand1 = src[1].operand1;
			++src;
			++dst;
		}
		else {
			*dst++ = *src;
		}
	}
	dst->opcode = BIND_OPCODE_DONE;

	// optimize phase 2, compress packed runs of BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB with
	// same addr delta into one BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB
	dst = &mid[0];
	for (const BindingInfo *src = &mid[0]; src->opcode != BIND_OPCODE_DONE; ++src) {
		uint64_t delta = src->operand1;
		if ( (src->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
				&& (src[1].opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
				&& (src[1].operand1 == delta) ) {
			// found at least two in a row, this is worth compressing
			dst->opcode = BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB;
			dst->operand1 = 1;
			dst->operand2 = delta;
			++src;
			while ( (src->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB)
					&& (src->operand1 == delta) ) {
				dst->operand1++;
				++src;
			}
			--src;
			++dst;
		}
		else {
			*dst++ = *src;
		}
	}
	dst->opcode = BIND_OPCODE_DONE;
	
	// optimize phase 3, use immediate encodings
	for (BindingInfo *p = &mid[0]; p->opcode != REBASE_OPCODE_DONE; ++p) {
		if ( (p->opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) 
			&& (p->operand1 < (15 * ptrsize))
			&& ((p->operand1 % ptrsize) == 0) ) {
			p->opcode = BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED;
			p->operand1 = p->operand1 / ptrsize;
		}
	}	
	dst->opcode = BIND_OPCODE_DONE; //-V519

	// convert to compressed encoding
	EncodedData data;
	data.reserve(info.size() * 2);
	bool done = false;
	for (std::vector<BindingInfo>::iterator it = mid.begin(); !done && it != mid.end(); ++it) {
		switch (it->opcode) {
		case BIND_OPCODE_DONE:
			data.WriteByte(BIND_OPCODE_DONE);
			done = true;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_IMM | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
			data.WriteByte(BIND_OPCODE_SET_DYLIB_SPECIAL_IMM | (it->operand1 & BIND_IMMEDIATE_MASK));
			break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
			data.WriteByte(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM | static_cast<uint8_t>(it->operand1));
			data.WriteString(it->name);
			break;
		case BIND_OPCODE_SET_TYPE_IMM:
			data.WriteByte(BIND_OPCODE_SET_TYPE_IMM | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
			data.WriteByte(BIND_OPCODE_SET_ADDEND_SLEB);
			data.WriteSleb128(it->operand1);
			break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			data.WriteByte(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB | static_cast<uint8_t>(it->operand1));
			data.WriteUleb128(it->operand2);
			break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
			data.WriteByte(BIND_OPCODE_ADD_ADDR_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_DO_BIND:
			data.WriteByte(BIND_OPCODE_DO_BIND);
			break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
			data.WriteByte(BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB);
			data.WriteUleb128(it->operand1);
			break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
			data.WriteByte(BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED | static_cast<uint8_t>(it->operand1));
			break;
		case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB:
			data.WriteByte(BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB);
			data.WriteUleb128(it->operand1);
			data.WriteUleb128(it->operand2);
			break;
		}
	}
	data.resize(AlignValue(data.size(), ptrsize), 0);

	return (data.size() == 0) ? 0 : file.Write(data.data(), data.size());
}

size_t MacImportList::WriteLazyBindInfo(MacArchitecture &file)
{
	std::vector<MacImportFunction *> info;
	MacImport *import;
	MacImportFunction *import_function;

	for (size_t i = 0; i < count(); i++) {
		import = item(i);
		for (size_t j = 0; j < import->count(); j++) {
			import_function = import->item(j);
			if (!import_function->is_lazy())
				continue;
			info.push_back(import_function);
		}
	}
	if (info.empty())
		return 0;

	std::sort(info.begin(), info.end(), LazyBindInfoHelper());

	size_t ptrsize = OperandSizeToValue(file.cpu_address_size());
	EncodedData data;
	for (std::vector<MacImportFunction *>::iterator it = info.begin(); it != info.end() ; ++it) {
		import_function = *it;

		if (data.size() > import_function->bind_offset())
			throw std::runtime_error("binding offset out of range");
		for (size_t i = data.size(); i < import_function->bind_offset(); i++) {
			data.WriteByte(BIND_OPCODE_DONE);
		}
	
		// write address to bind
		MacSegment *segment = file.segment_list()->GetSectionByAddress(import_function->address());
		if (!segment)
			throw std::runtime_error("lazy binding address outside range of any segment");
		data.WriteByte(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB | static_cast<uint8_t>(file.segment_list()->IndexOf(segment)));
		data.WriteUleb128(import_function->address() - segment->address());
		
		// write ordinal
		if (import_function->library_ordinal() <= 0) {
			// special lookups are encoded as negative numbers in BindingInfo
			data.WriteByte(BIND_OPCODE_SET_DYLIB_SPECIAL_IMM | (import_function->library_ordinal() & BIND_IMMEDIATE_MASK) );
		} else if (import_function->library_ordinal() <= 15 ) {
			// small ordinals are encoded in opcode
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_IMM | import_function->library_ordinal());
		} else {
			data.WriteByte(BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB);
			data.WriteUleb128(import_function->library_ordinal());
		}
		// write symbol name
		data.WriteByte(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM | import_function->flags());
		data.WriteString(import_function->name());
		// write do bind
		data.WriteByte(BIND_OPCODE_DO_BIND);
		data.WriteByte(BIND_OPCODE_DONE);
	}
	data.resize(AlignValue(data.size(), ptrsize), 0);

	return (data.size() == 0) ? 0 : file.Write(data.data(), data.size());
}

void MacImportList::RebaseBindInfo(MacArchitecture &file, size_t delta_base)
{
	std::vector<MacImportFunction *> info;
	MacImport *import;
	MacImportFunction *import_function;

	for (size_t i = 0; i < count(); i++) {
		import = item(i);
		for (size_t j = 0; j < import->count(); j++) {
			import_function = import->item(j);
			if (!import_function->is_lazy())
				continue;
			info.push_back(import_function);
		}
	}
	if (info.empty())
		return;

	std::sort(info.begin(), info.end(), LazyBindInfoHelper());

	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	for (std::vector<MacImportFunction *>::iterator it = info.begin(); it != info.end() ; ++it) {
		import_function = *it;
		import_function->set_bind_offset(import_function->bind_offset() + delta_base);
		delta_base += 4;

		file.AddressSeek(import_function->address());
		uint64_t value = 0;
		assert(sizeof(value) >= value_size);
		file.Read(&value, value_size);
		if (value && file.AddressSeek(value)) {
			if (file.ReadByte() != 0x68) // push XXXX
				throw std::runtime_error("Runtime error at RebaseBindInfo");
			file.WriteDWord(static_cast<uint32_t>(import_function->bind_offset()));
		}
	}
}

/**
 * MacIndirectSymbol
 */

MacIndirectSymbol::MacIndirectSymbol(MacIndirectSymbolList *owner, uint64_t address, uint32_t value, MacSymbol *symbol)
	: IObject(), owner_(owner), address_(address), value_(value), symbol_(symbol)
{

}

MacIndirectSymbol::MacIndirectSymbol(MacIndirectSymbolList *owner, const MacIndirectSymbol &src)
	: IObject(src), owner_(owner)
{
	address_ = src.address_;
	value_ = src.value_;
	symbol_ = src.symbol_;
}

MacIndirectSymbol::~MacIndirectSymbol()
{
	if (owner_)
		owner_->RemoveObject(this);
}

MacIndirectSymbol *MacIndirectSymbol::Clone(MacIndirectSymbolList *owner) const
{
	MacIndirectSymbol *symbol = new MacIndirectSymbol(owner, *this);
	return symbol;
}

void MacIndirectSymbol::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
}

/**
 * MacIndirectSymbolList
 */

MacIndirectSymbolList::MacIndirectSymbolList()
	: ObjectList<MacIndirectSymbol>()
{

}

MacIndirectSymbolList::MacIndirectSymbolList(const MacIndirectSymbolList &src)
	: ObjectList<MacIndirectSymbol>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

MacIndirectSymbolList *MacIndirectSymbolList::Clone() const
{
	MacIndirectSymbolList *list = new MacIndirectSymbolList(*this);
	return list;
}

MacIndirectSymbol *MacIndirectSymbolList::Add(uint64_t address, uint32_t value, MacSymbol *symbol)
{
	MacIndirectSymbol *sym = new MacIndirectSymbol(this, address, value, symbol);
	AddObject(sym);
	return sym;
}

MacIndirectSymbol *MacIndirectSymbolList::GetSymbol(MacSymbol *symbol) const
{
	for (size_t i = 0; i < count(); i++) {
		MacIndirectSymbol *indirect_symbol = item(i);
		if (indirect_symbol->symbol() == symbol)
			return indirect_symbol;
	}
	return NULL;
}

void MacIndirectSymbolList::ReadFromFile(MacArchitecture &file)
{
	dysymtab_command *dysymtab = file.dysymtab();
	if (!dysymtab->cmd)
		return;

	file.Seek(dysymtab->indirectsymoff);
	uint32_t ptr_size = OperandSizeToValue(file.cpu_address_size());

	for (size_t i = 0; i < dysymtab->nindirectsyms; i++) {
		uint32_t value = file.ReadDWord(); 

		uint64_t address = 0;
		for (size_t j = 0; j < file.section_list()->count(); j++) {
			MacSection *section = file.section_list()->item(j);
			if (section->type() == S_SYMBOL_STUBS || section->type() == S_LAZY_SYMBOL_POINTERS || section->type() == S_NON_LAZY_SYMBOL_POINTERS || section->type() == S_THREAD_LOCAL_VARIABLE_POINTERS) {
				uint32_t value_size = (section->type() == S_SYMBOL_STUBS) ? section->reserved2() : ptr_size;
				if (i >= section->reserved1() && i < section->reserved1() + section->size() / value_size) {
					address = section->address() + (i - section->reserved1()) * value_size;
					break;
				}
			}
		}

		if (!address)
			throw std::runtime_error("Invalid indirect symbol address");

		MacSymbol *symbol;
		if (value & (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS)) {
			symbol = NULL;
		} else {
			if (value >= file.symbol_list()->count())
				throw std::runtime_error("Invalid indirect symbol value");
			symbol = file.symbol_list()->item(value);
		}

		Add(address, value, symbol);
	}
}

void MacIndirectSymbolList::Pack()
{
	for (size_t i = 0; i < count(); i++) {
		MacIndirectSymbol *indirect_symbol = item(i);
		MacSymbol *symbol = indirect_symbol->symbol();
		if (symbol && symbol->is_deleted()) {
			indirect_symbol->set_symbol(NULL);
			indirect_symbol->set_value(INDIRECT_SYMBOL_ABS);
		}
	}
}

void MacIndirectSymbolList::WriteToFile(MacArchitecture &file)
{
	dysymtab_command *dysymtab = file.dysymtab();

	dysymtab->nindirectsyms = static_cast<uint32_t>(count());
	if (!dysymtab->nindirectsyms) {
		dysymtab->indirectsymoff = 0;
		return;
	}

	dysymtab->indirectsymoff = static_cast<uint32_t>(file.Tell());
	for (size_t i = 0; i < count(); i++) {
		MacIndirectSymbol *indirect_symbol = item(i);
		uint32_t value = (indirect_symbol->symbol()) ? (uint32_t)file.symbol_list()->IndexOf(indirect_symbol->symbol()) : indirect_symbol->value();
		file.Write(&value, sizeof(value));
	}
}

void MacIndirectSymbolList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * MacExtRefSymbol
 */

MacExtRefSymbol::MacExtRefSymbol(MacExtRefSymbolList *owner, MacSymbol *symbol, uint8_t flags)
	: IObject(), owner_(owner), flags_(flags), symbol_(symbol)
{

}

MacExtRefSymbol::MacExtRefSymbol(MacExtRefSymbolList *owner, const MacExtRefSymbol &src)
	: IObject(src), owner_(owner)
{
	flags_ = src.flags_;
	symbol_ = src.symbol_;
}

MacExtRefSymbol::~MacExtRefSymbol()
{
	if (owner_)
		owner_->RemoveObject(this);
}

MacExtRefSymbol *MacExtRefSymbol::Clone(MacExtRefSymbolList *owner) const
{
	MacExtRefSymbol *symbol = new MacExtRefSymbol(owner, *this);
	return symbol;
}

/**
 * MacExtRefSymbolList
 */

MacExtRefSymbolList::MacExtRefSymbolList()
	: ObjectList<MacExtRefSymbol>()
{

}

MacExtRefSymbolList::MacExtRefSymbolList(const MacExtRefSymbolList &src)
	: ObjectList<MacExtRefSymbol>(src)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

MacExtRefSymbolList *MacExtRefSymbolList::Clone() const
{
	MacExtRefSymbolList *list = new MacExtRefSymbolList(*this);
	return list;
}

MacExtRefSymbol *MacExtRefSymbolList::Add(MacSymbol *symbol, uint8_t flags)
{
	MacExtRefSymbol *sym = new MacExtRefSymbol(this, symbol, flags);
	AddObject(sym);
	return sym;
}

MacExtRefSymbol *MacExtRefSymbolList::GetSymbol(MacSymbol *symbol) const
{
	for (size_t i = 0; i < count(); i++) {
		MacExtRefSymbol *ext_ref_symbol = item(i);
		if (ext_ref_symbol->symbol() == symbol)
			return ext_ref_symbol;
	}
	return NULL;
}

void MacExtRefSymbolList::ReadFromFile(MacArchitecture &file)
{
	dysymtab_command *dysymtab = file.dysymtab();
	if (!dysymtab->cmd)
		return;

	file.Seek(dysymtab->extrefsymoff);

	for (size_t i = 0; i < dysymtab->nextrefsyms; i++) {
		dylib_reference ref;
		file.Read(&ref, sizeof(ref));

		if (ref.isym >= file.symbol_list()->count())
			throw std::runtime_error("Invalid extern reference symbol value");

		Add(file.symbol_list()->item(ref.isym), ref.flags);
	}
}

void MacExtRefSymbolList::Pack()
{
	for (size_t i = count(); i > 0; i--) {
		MacExtRefSymbol *ext_ref_symbol = item(i - 1);
		if (ext_ref_symbol->symbol()->is_deleted())
			delete ext_ref_symbol;
	}
}

void MacExtRefSymbolList::WriteToFile(MacArchitecture &file)
{
	dysymtab_command *dysymtab = file.dysymtab();

	dysymtab->nextrefsyms = static_cast<uint32_t>(count());
	if (!dysymtab->nextrefsyms) {
		dysymtab->extrefsymoff = 0;
		return;
	}

	dysymtab->extrefsymoff = static_cast<uint32_t>(file.Tell());
	for (size_t i = 0; i < count(); i++) {
		MacExtRefSymbol *ext_ref_symbol = item(i);
		dylib_reference ref;
		ref.isym = file.symbol_list()->IndexOf(ext_ref_symbol->symbol());
		ref.flags = ext_ref_symbol->flags();
		file.Write(&ref, sizeof(ref));
	}
}

/**
 * MacExport
 */

MacExport::MacExport(IExportList *parent, uint64_t address, const std::string &name, uint64_t flags, uint64_t other)
	: BaseExport(parent), symbol_(NULL), address_(address), name_(name), flags_(flags), other_(other)
{

}

MacExport::MacExport(IExportList *parent, MacSymbol *symbol)
	: BaseExport(parent), symbol_(symbol), address_(0), flags_(0), other_(0)
{
	if (symbol_) {
		address_ = symbol_->value();
		name_ = symbol_->name();
	}
}

MacExport::MacExport(IExportList *parent, const MacExport &src)
	: BaseExport(parent, src)
{
	address_ = src.address_;
	name_ = src.name_;
	forwarded_name_ = src.forwarded_name_;
	flags_ = src.flags_;
	other_ = src.other_;
	symbol_ = src.symbol_;
}

MacExport *MacExport::Clone(IExportList *parent) const
{
	MacExport *exp = new MacExport(parent, *this);
	return exp;
}

void MacExport::set_address(uint64_t address)
{
	address_ = address;
	if (symbol_)
		symbol_->set_value(address);
}

void MacExport::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

std::string MacExport::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

/**
 * MacExportList
 */

MacExportList::MacExportList(MacArchitecture *owner)
	: BaseExportList(owner)
{

}

MacExportList::MacExportList(MacArchitecture *owner, const MacExportList &src)
	: BaseExportList(owner, src)
{
	name_ = src.name_;
}

MacExportList *MacExportList::Clone(MacArchitecture *owner) const
{
	MacExportList *list = new MacExportList(owner, *this);
	return list;
}

MacExport *MacExportList::item(size_t index) const 
{ 
	return reinterpret_cast<MacExport *>(IExportList::item(index));
}

MacExport *MacExportList::Add(uint64_t address, const std::string & name, uint64_t flags, uint64_t other)
{
	MacExport *exp = new MacExport(this, address, name, flags, other);
	AddObject(exp);
	return exp;
}

MacExport *MacExportList::Add(MacSymbol *symbol)
{
	MacExport *exp = new MacExport(this, symbol);
	AddObject(exp);
	return exp;
}

void MacExportList::ParseExportNode(const EncodedData &buf, size_t pos, const std::string &name, uint64_t base_address)
{
	uint8_t terminal_size, children_count;
	uint64_t address, flags, other;
	size_t i, children_pos;
	std::string children_name, import_name;
	uint32_t childNodeOffset;
	MacExport *exp;

	terminal_size = static_cast<uint8_t>(buf.ReadUleb128(&pos));
	children_pos = pos + terminal_size;
	if (terminal_size) {
		flags = buf.ReadUleb128(&pos);

		if (flags & EXPORT_SYMBOL_FLAGS_REEXPORT) {
			address = 0;
			other = buf.ReadUleb128(&pos);
			import_name = buf.ReadString(&pos);
		} else {
			address = base_address + buf.ReadUleb128(&pos);
			other = (flags & EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER) ? buf.ReadUleb128(&pos) : 0;
		}

		exp = Add(address, name, flags, other);
		if (import_name.size() > 0) 
			exp->set_forwarded_name(import_name);
	}

	children_count = buf[children_pos++];
	for (i = 0; i < children_count; i++) {
		children_name = name + buf.ReadString(&children_pos);
		childNodeOffset = static_cast<uint32_t>(buf.ReadUleb128(&children_pos));
		ParseExportNode(buf, childNodeOffset, children_name, base_address);
	}
}

void MacExportList::ReadFromFile(MacArchitecture &file)
{
	dyld_info_command *dyld_info = file.dyld_info();

	ILoadCommand *command = file.command_list()->GetCommandByType(LC_ID_DYLIB);
	if (command) {
		dylib_command dylib;

		file.Seek(command->address());
		file.Read(&dylib, sizeof(dylib));

		file.Seek(command->address() + dylib.dylib.name.offset);
		name_ = file.ReadString();
	}

	if (dyld_info->cmd) {
		if (!dyld_info->export_size)
			return;

		MacSegment *base_segment = file.segment_list()->GetBaseSegment();
		if (!base_segment)
			throw std::runtime_error("Invalid base segment for export info");

		EncodedData buf;
		file.Seek(dyld_info->export_off);
		buf.ReadFromFile(file, dyld_info->export_size);
		ParseExportNode(buf, 0, "", base_segment->address());

		std::map<uint64_t, MacExport *> address_map;
		for (size_t i = 0; i < count(); i++) {
			MacExport *exp = item(i);
			if (!exp->address())
				continue;
			address_map[exp->address()] = exp;
		}

		MacSymbolList *symbol_list = file.symbol_list();
		for (size_t i = 0; i < symbol_list->count(); i++) {
			MacSymbol *symbol = symbol_list->item(i);

			if (symbol->value()) {
				std::map<uint64_t, MacExport *>::const_iterator it = address_map.find(symbol->value());
				if (it != address_map.end())
					it->second->set_symbol(symbol);
			}
		}
	} else {
		MacSymbolList *symbol_list = file.symbol_list(); 
		for (size_t i = 0; i < symbol_list->count(); i++) {
			MacSymbol *symbol = symbol_list->item(i);
			if ((symbol->type() & (N_STAB | N_TYPE | N_EXT)) == (N_SECT | N_EXT))
				Add(symbol);
		}
	}
}

void MacExportList::Pack()
{
	for (size_t i = count(); i > 0 ; i--) {
		MacExport *exp = item(i - 1);
		if (exp->symbol() && exp->symbol()->is_deleted())
			delete exp;
	}
}

void MacExportList::WriteToFile(MacArchitecture &file)
{
	dyld_info_command *dyld_info = file.dyld_info();
	if (!dyld_info->cmd)
		return;

	if (count() == 0) {
		dyld_info->export_size = 0;
		dyld_info->export_off = 0;
		return;
	}

	size_t i, j;
	MacExportNode root_node;
	MacExportNode *node;
	EncodedData data;
	std::vector<MacExportNode *> stack;

	for (i = 0; i < count(); i++) {
		root_node.AddSymbol(item(i));
	}

	stack.push_back(&root_node);
	for (i = 0; i < stack.size(); i++) {
		node = stack[i];
		for (j = 0; j < node->count(); j++) {
			stack.push_back(node->item(j));
		}
	}

	uint64_t base_address = file.segment_list()->GetBaseSegment()->address();

	bool more;
	do {
		more = false;
		data.clear();
		for (i = 0; i < stack.size(); i++) {
			node = stack[i];
			if (node->offset() != data.size())
				more = true;
			stack[i]->WriteToData(data, base_address);
		}
	} while (more);
	data.resize(AlignValue(data.size(), OperandSizeToValue(file.cpu_address_size())), 0);

	dyld_info->export_size = static_cast<uint32_t>(data.size());
	if (!dyld_info->export_size) {
		dyld_info->export_off = 0;
	} else {
		dyld_info->export_off = static_cast<uint32_t>(file.Tell());
		file.Write(data.data(), data.size());
	}
}

MacExport *MacExportList::GetExportByAddress(uint64_t address) const
{
	return reinterpret_cast<MacExport *>(BaseExportList::GetExportByAddress(address));
}

void MacExportList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
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
 * MacExportNode
 */

MacExportNode::MacExportNode(MacExportNode *owner)
	: ObjectList<MacExportNode>(), owner_(owner), symbol_(NULL), offset_(0)
{

}

MacExportNode::MacExportNode(MacExportNode *owner, MacExport *symbol, const std::string &cummulative_string)
	: ObjectList<MacExportNode>(), owner_(owner), symbol_(symbol), cummulative_string_(cummulative_string), offset_(0)
{

}

MacExportNode::~MacExportNode()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void MacExportNode::set_owner(MacExportNode *owner)
{
	if (owner == owner_)
		return;
	if (owner_)
		owner_->RemoveObject(this);
	owner_ = owner;
	if (owner_)
		owner_->AddObject(this);
}

MacExportNode *MacExportNode::Add(MacExport *symbol, const std::string &cummulative_string)
{
	MacExportNode *node = new MacExportNode(this, symbol, cummulative_string);
	AddObject(node);
	return node;
}

void MacExportNode::AddSymbol(MacExport *symbol)
{
	std::string symbol_name = symbol->name();
	const char *partial_str = symbol_name.c_str() + cummulative_string_.size();
	for (size_t j = 0; j < count(); j++) {
		MacExportNode *node = item(j);
		std::string node_cummulative_string = node->cummulative_string();
		const char *sub_string = node_cummulative_string.c_str() + cummulative_string_.size();
		size_t sub_string_len = strlen(sub_string);
		if (strncmp(sub_string, partial_str, sub_string_len) == 0) {
			node->AddSymbol(symbol);
			return;
		} else for (size_t i = sub_string_len; i > 1; i--) {
			if (strncmp(sub_string, partial_str, i - 1) == 0) {
				MacExportNode *new_node = Add(NULL, node_cummulative_string.substr(0, cummulative_string_.size() + i - 1));
				node->set_owner(new_node);
				new_node->AddSymbol(symbol);
				return;
			}
		}
	}

	Add(symbol, symbol->name());
}

void MacExportNode::WriteToData(EncodedData &data, uint64_t base_address)
{
	size_t i;

	offset_ = (uint32_t)data.size();
	data.WriteByte(0);
	if (symbol_) {
		size_t old_size = data.size();
		if (symbol_->flags() & EXPORT_SYMBOL_FLAGS_REEXPORT) {
			data.WriteUleb128(symbol_->flags());
			data.WriteUleb128(symbol_->other());
			data.WriteString(symbol_->forwarded_name());
		} else if (symbol_->flags() & EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER) {
			data.WriteUleb128(symbol_->flags());
			data.WriteUleb128(symbol_->address() - base_address);
			data.WriteUleb128(symbol_->other());
		} else {
			data.WriteUleb128(symbol_->flags());
			data.WriteUleb128(symbol_->address() - base_address);
		}
		data[old_size - 1] = static_cast<uint8_t>(data.size() - old_size);
	}

	// write number of children
	data.push_back(static_cast<uint8_t>(count()));
	// write each child
	for (i = 0; i < count(); i++) {
		MacExportNode *child = item(i);
		data.WriteString(child->cummulative_string().substr(cummulative_string().size()));
		data.WriteUleb128(child->offset());
	}
}

/**
 * MacFixup
 */

MacFixup::MacFixup(MacFixupList *owner, uint64_t address, uint32_t data, OperandSize size, bool is_relocation)
	: BaseFixup(owner), address_(address), data_(data), size_(size), is_relocation_(is_relocation), symbol_(NULL)
{
	if (is_relocation && size == osDefault) {
		relocation_info reloc = relocation();
		switch (reloc.r_length) {
		case 0:
			size_ = osByte;
			break;
		case 1:
			size_ = osWord;
			break;
		case 2:
			size_ = osDWord;
			break;
		default:
			size_ = osQWord;
			break;
		}
	}
}

MacFixup::MacFixup(MacFixupList *owner, const MacFixup &src)
	: BaseFixup(owner, src)
{
	address_ = src.address_;
	data_ = src.data_;
	size_ = src.size_;
	is_relocation_ = src.is_relocation_;
	symbol_ = src.symbol_;
}

MacFixup *MacFixup::Clone(IFixupList *owner) const
{
	MacFixup *fixup = new MacFixup(reinterpret_cast<MacFixupList *>(owner), *this);
	return fixup;
}

relocation_info MacFixup::relocation() const
{
	relocation_info res = relocation_info();
	if (is_relocation_)
		reinterpret_cast<uint32_t *>(&res)[1] = data_;
	return res;
}

FixupType MacFixup::type() const
{
	switch (internal_type()) {
	case REBASE_TYPE_POINTER:
	case REBASE_TYPE_TEXT_ABSOLUTE32:
		return ftHighLow;
	default:
		return ftUnknown;
	}
}

uint8_t MacFixup::internal_type() const
{
	if (is_relocation_) {
		relocation_info reloc = relocation();
		switch (reloc.r_type) {
		case 0:
			return REBASE_TYPE_POINTER;
		default:
			return -1;
		}
	} else {
		switch (data_) {
		case REBASE_TYPE_POINTER:
		case REBASE_TYPE_TEXT_ABSOLUTE32:
			return static_cast<uint8_t>(data_);
		default:
			return -1;
		}
	}
}

void MacFixup::set_is_relocation(bool is_relocation)
{
	if (is_relocation == is_relocation_)
		return;

	if (is_relocation_) {
		data_ = REBASE_TYPE_POINTER;
	} else {
		relocation_info reloc;
		reloc.r_symbolnum = 0;
		switch (size_) {
		case osByte:
			reloc.r_length = 0;
			break;
		case osWord:
			reloc.r_length = 1;
			break;
		case osDWord:
			reloc.r_length = 2;
			break;
		default:
			reloc.r_length = 3;
			break;
		}
		reloc.r_extern = 0;
		reloc.r_type = 0;
		data_ = reinterpret_cast<uint32_t *>(&reloc)[1];
	}
	is_relocation_ = is_relocation;
}

void MacFixup::Rebase(IArchitecture &file, uint64_t delta_base)
{
	if (!file.AddressSeek(address_))
		return;

	uint64_t pos = file.Tell();
	uint64_t value = 0;
	size_t value_size = OperandSizeToValue(size_);
	switch (internal_type()) {
	case REBASE_TYPE_POINTER:
	case REBASE_TYPE_TEXT_ABSOLUTE32:
		assert(sizeof(value) >= value_size);
		file.Read(&value, value_size);
		value += delta_base;
		file.Seek(pos);
		file.Write(&value, value_size);
		break;
	}
	address_ += delta_base;
}

/**
 * MacFixupList
 */

MacFixupList::MacFixupList()
	: BaseFixupList()
{

}

MacFixupList::MacFixupList(const MacFixupList &src)
	: BaseFixupList(src)
{

}

MacFixupList *MacFixupList::Clone() const
{
	MacFixupList *list  = new MacFixupList(*this);
	return list;
}

MacFixup *MacFixupList::item(size_t index) const
{ 
	return reinterpret_cast<MacFixup *>(BaseFixupList::item(index));
}

MacFixup *MacFixupList::Add(uint64_t address, uint32_t data, OperandSize size, bool is_relocation)
{
	MacFixup *fixup = new MacFixup(this, address, data, size, is_relocation);
	AddObject(fixup);

	if (fixup->type() == ftUnknown)
		throw std::runtime_error("Invalid rebase type");

	return fixup;
}

MacFixup *MacFixupList::AddRelocation(uint64_t address, MacSymbol *symbol, OperandSize size)
{
	relocation_info reloc = relocation_info();
	reloc.r_extern = 1;
	reloc.r_length = size;
	MacFixup *fixup = Add(address, reinterpret_cast<uint32_t *>(&reloc)[1], size, true);
	fixup->set_symbol(symbol);
	return fixup;
}

void MacFixupList::ReadRebaseInfo(MacArchitecture &file, uint32_t rebase_off, uint32_t rebase_size)
{
	uint8_t bind_type = 0;
	uint64_t address, segment_end;
	uint32_t count;
	uint32_t skip;
	uint8_t immediate;
	uint8_t opcode;
	EncodedData buf;
	ISectionList *segment_list = file.segment_list();
	ISection *segment;
	size_t pos, i, ptr_size, seg_index = 0;
	OperandSize cpu_address_size;

	if (rebase_size == 0)
		return;

	if (segment_list->count() < 1)
		throw std::runtime_error("Runtime error at ReadRebaseInfo: Invalid segment count");

	file.Seek(rebase_off);
	buf.ReadFromFile(file, rebase_size);

	cpu_address_size = file.cpu_address_size();
	ptr_size = cpu_address_size == osQWord ? 8 : 4;

	segment = segment_list->item(0);
	address = segment->address();
	segment_end = segment->address() + segment->size();
	for (pos = 0; pos < rebase_size; ) {
		uint8_t b = buf.ReadByte(&pos);
		immediate = b & REBASE_IMMEDIATE_MASK;
		opcode = b & REBASE_OPCODE_MASK;
		switch (opcode) {
		case REBASE_OPCODE_DONE:
			return;
			break;
		case REBASE_OPCODE_SET_TYPE_IMM:
			bind_type = immediate;
			break;
		case REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			seg_index = immediate;
			if (seg_index >= segment_list->count())
				throw std::runtime_error("Invalid segment index");
			segment = segment_list->item(seg_index);
			address = segment->address() + buf.ReadUleb128(&pos);
			segment_end = segment->address() + segment->size();
			break;
		case REBASE_OPCODE_ADD_ADDR_ULEB:
			address += buf.ReadUleb128(&pos);
			break;
		case REBASE_OPCODE_ADD_ADDR_IMM_SCALED:
			address += immediate * ptr_size;
			break;
		case REBASE_OPCODE_DO_REBASE_IMM_TIMES:
			for (i = 0; i < immediate; i++) {
				if (address >= segment_end) 
					throw std::runtime_error("Invalid rebase address");
				Add(address, bind_type, cpu_address_size, false);
				address += ptr_size;
			}
			break;
		case REBASE_OPCODE_DO_REBASE_ULEB_TIMES:
			count = static_cast<uint32_t>(buf.ReadUleb128(&pos));
			for (i = 0; i < count; i++) {
				if (address >= segment_end) 
					throw std::runtime_error("Invalid rebase address");
				Add(address, bind_type, cpu_address_size, false);
				address += ptr_size;
			}
			break;
		case REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB:
			if (address >= segment_end) 
				throw std::runtime_error("Invalid rebase address");
			Add(address, bind_type, cpu_address_size, false);
			address += buf.ReadUleb128(&pos) + ptr_size;
			break;
		case REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB:
			count = static_cast<uint32_t>(buf.ReadUleb128(&pos));
			skip = static_cast<uint32_t>(buf.ReadUleb128(&pos));
			for (i = 0; i < count; i++) {
				if (address >= segment_end) 
					throw std::runtime_error("Invalid rebase address");
				Add(address, bind_type, cpu_address_size, false);
				address += skip + ptr_size;
			}
			break;
		default:
			throw std::runtime_error("Invalid rebase opcode");
		}
	}
}

void MacFixupList::ReadRelocations(MacArchitecture &file, uint32_t offset, uint32_t count, bool need_external)
{
	if (!offset)
		return;

	size_t i;
	relocation_info reloc;
	uint64_t reloc_base = file.GetRelocBase();

	file.Seek(offset);
	for (i = 0; i < count; i++) {
		file.Read(&reloc, sizeof(reloc));

		if (reloc.r_type != 0 || reloc.r_extern != need_external)
			throw std::runtime_error("Invalid relocation type");

		MacFixup *fixup = Add(reloc.r_address + reloc_base, reinterpret_cast<uint32_t *>(&reloc)[1], osDefault, true);
		if (reloc.r_extern) {
			if (reloc.r_symbolnum >= file.symbol_list()->count())
				throw std::runtime_error("Invalid symbol index");
			fixup->set_symbol(file.symbol_list()->item(reloc.r_symbolnum));
		}
		if (file.cpu_address_size() != fixup->size())
			throw std::runtime_error("Invalid relocation size");
	}
}

void MacFixupList::ReadFromFile(MacArchitecture &file)
{
	dyld_info_command *dyld_info = file.dyld_info();
	if (dyld_info->cmd)
		ReadRebaseInfo(file, dyld_info->rebase_off, dyld_info->rebase_size);

	dysymtab_command *dysymtab = file.dysymtab();
	if (dysymtab->cmd) {
		ReadRelocations(file, dysymtab->extreloff, dysymtab->nextrel, true);
		ReadRelocations(file, dysymtab->locreloff, dysymtab->nlocrel, false);
	}
}

size_t MacFixupList::WriteRelocations(MacArchitecture &file, bool need_external)
{
	size_t i;
	relocation_info reloc;
	size_t res = 0;
	uint64_t reloc_base = file.GetRelocBase();

	for (i = 0; i < count(); i++) {
		MacFixup *fixup = item(i);
		if (!fixup->is_relocation())
			continue;

		reloc = fixup->relocation();
		if (reloc.r_extern != need_external)
			continue;

		reloc.r_address = static_cast<uint32_t>(fixup->address() - reloc_base);
		if (fixup->symbol())
			reloc.r_symbolnum = static_cast<uint32_t>(file.symbol_list()->IndexOf(fixup->symbol()));
		file.Write(&reloc, sizeof(reloc));
		res++;
	}

	return res;
}

void MacFixupList::WriteToFile(MacArchitecture &file)
{
	Pack();

	dyld_info_command *dyld_info = file.dyld_info();
	dysymtab_command *dysymtab = file.dysymtab();
	if (dyld_info->cmd) {
		dyld_info->rebase_off = static_cast<uint32_t>(file.Tell());
		dyld_info->rebase_size = (uint32_t)WriteRebaseInfo(file);
		if (!dyld_info->rebase_size)
			dyld_info->rebase_off = 0;
	} else {
		// need convert fixups to relocations
		for (size_t i = 0; i < count(); i++) {
			MacFixup *fixup = item(i);
			if (!fixup->is_relocation())
				fixup->set_is_relocation(true);
		}
	}
	if (dysymtab->cmd) {
		dysymtab->extreloff = static_cast<uint32_t>(file.Tell());
		dysymtab->nextrel = static_cast<uint32_t>(WriteRelocations(file, true));
		if (!dysymtab->nextrel)
			dysymtab->extreloff = 0;
		dysymtab->locreloff = static_cast<uint32_t>(file.Tell());
		dysymtab->nlocrel = static_cast<uint32_t>(WriteRelocations(file, false));
		if (!dysymtab->nlocrel)
			dysymtab->locreloff = 0;
	}
}

size_t MacFixupList::WriteRebaseInfo(MacArchitecture &file)
{
	size_t i;
	std::vector<MacFixup *> info;

	for (i = 0; i < count(); i++) {
		MacFixup *fixup = item(i);
		if (fixup->is_relocation())
			continue;
		info.push_back(fixup);
	}

	if (info.size() == 0)
		return 0;

	// sort rebase info by type, then address
	std::sort(info.begin(), info.end(), RebaseInfoHelper());
	
	// convert to temp encoding that can be more easily optimized
	std::vector<RebaseInfo> mid;
	MacSegment *segment = NULL;
	uint8_t bind_type = 0;
	uint64_t address = -1;
	size_t ptrsize = OperandSizeToValue(file.cpu_address_size());
	for (std::vector<MacFixup *>::iterator it = info.begin(); it != info.end(); ++it) {
		MacFixup *fixup = *it;

		if (bind_type != fixup->bind_type()) {
			mid.push_back(RebaseInfo(REBASE_OPCODE_SET_TYPE_IMM, fixup->bind_type()));
			bind_type = fixup->bind_type();
		}
		if (address != fixup->address()) {
			if (!segment || (fixup->address() < segment->address()) || (fixup->address() >= segment->address() + segment->size())) {
				segment = file.segment_list()->GetSectionByAddress(fixup->address());
				if (!segment)
					throw std::runtime_error("binding address outside range of any segment");
				mid.push_back(RebaseInfo(REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB, file.segment_list()->IndexOf(segment), fixup->address() - segment->address()));
			}
			else {
				mid.push_back(RebaseInfo(REBASE_OPCODE_ADD_ADDR_ULEB, fixup->address() - address));
			}
			address = fixup->address();
		}
		mid.push_back(RebaseInfo(REBASE_OPCODE_DO_REBASE_ULEB_TIMES, 1));
		address += ptrsize;
	}
	mid.push_back(RebaseInfo(REBASE_OPCODE_DONE, 0));

	// optimize phase 1, compress packed runs of pointers
	RebaseInfo *dst = &mid[0];
	for (const RebaseInfo *src = &mid[0]; src->opcode != REBASE_OPCODE_DONE; ++src) {
		if ( (src->opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES) && (src->operand1 == 1) ) {
			*dst = *src++;
			while (src->opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES ) {
				dst->operand1 += src->operand1;
				++src;
			}
			--src;
			++dst;
		}
		else {
			*dst++ = *src;
		}
	}
	dst->opcode = REBASE_OPCODE_DONE;

	// optimize phase 2, combine rebase/add pairs
	dst = &mid[0];
	for (const RebaseInfo *src = &mid[0]; src->opcode != REBASE_OPCODE_DONE; ++src) {
		if ( (src->opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES) 
				&& (src->operand1 == 1) 
				&& (src[1].opcode == REBASE_OPCODE_ADD_ADDR_ULEB)) {
			dst->opcode = REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB;
			dst->operand1 = src[1].operand1;
			++src;
			++dst;
		}
		else {
			*dst++ = *src;
		}
	}
	dst->opcode = REBASE_OPCODE_DONE;
	
	// optimize phase 3, compress packed runs of REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB with
	// same addr delta into one REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB
	dst = &mid[0];
	for (const RebaseInfo *src = &mid[0]; src->opcode != REBASE_OPCODE_DONE; ++src) {
		uint64_t delta = src->operand1;
		if ( (src->opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB) 
				&& (src[1].opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB) 
				&& (src[2].opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB) 
				&& (src[1].operand1 == delta) 
				&& (src[2].operand1 == delta) ) {
			// found at least three in a row, this is worth compressing
			dst->opcode = REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB;
			dst->operand1 = 1;
			dst->operand2 = delta;
			++src;
			while ( (src->opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB)
					&& (src->operand1 == delta) ) {
				dst->operand1++;
				++src;
			}
			--src;
			++dst;
		}
		else {
			*dst++ = *src;
		}
	}
	dst->opcode = REBASE_OPCODE_DONE;
	
	// optimize phase 4, use immediate encodings
	for (RebaseInfo *p = &mid[0]; p->opcode != REBASE_OPCODE_DONE; ++p) {
		if ( (p->opcode == REBASE_OPCODE_ADD_ADDR_ULEB) 
			&& (p->operand1 < (15 * ptrsize))
			&& ((p->operand1 % ptrsize) == 0) ) {
			p->opcode = REBASE_OPCODE_ADD_ADDR_IMM_SCALED;
			p->operand1 = p->operand1 / ptrsize;
		}
		else if ( (p->opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES) && (p->operand1 < 15) ) {
			p->opcode = REBASE_OPCODE_DO_REBASE_IMM_TIMES;
		}
	}

	// convert to compressed encoding
	EncodedData data;
	data.reserve(info.size() * 2);
	bool done = false;
	for (std::vector<RebaseInfo>::iterator it = mid.begin(); !done && it != mid.end() ; ++it) {
		switch ( it->opcode ) {
			case REBASE_OPCODE_DONE:
				done = true;
				break;
			case REBASE_OPCODE_SET_TYPE_IMM:
				data.WriteByte(REBASE_OPCODE_SET_TYPE_IMM | static_cast<uint8_t>(it->operand1));
				break;
			case REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
				data.WriteByte(REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB | static_cast<uint8_t>(it->operand1));
				data.WriteUleb128(it->operand2);
				break;
			case REBASE_OPCODE_ADD_ADDR_ULEB:
				data.WriteByte(REBASE_OPCODE_ADD_ADDR_ULEB);
				data.WriteUleb128(it->operand1);
				break;
			case REBASE_OPCODE_ADD_ADDR_IMM_SCALED:
				data.WriteByte(REBASE_OPCODE_ADD_ADDR_IMM_SCALED | static_cast<uint8_t>(it->operand1));
				break;
			case REBASE_OPCODE_DO_REBASE_IMM_TIMES:
				data.WriteByte(REBASE_OPCODE_DO_REBASE_IMM_TIMES | static_cast<uint8_t>(it->operand1));
				break;
			case REBASE_OPCODE_DO_REBASE_ULEB_TIMES:
				data.WriteByte(REBASE_OPCODE_DO_REBASE_ULEB_TIMES);
				data.WriteUleb128(it->operand1);
				break;
			case REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB:
				data.WriteByte(REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB);
				data.WriteUleb128(it->operand1);
				break;
			case REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB:
				data.WriteByte(REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB);
				data.WriteUleb128(it->operand1);
				data.WriteUleb128(it->operand2);
				break;
		}
	}
	// align to pointer size
	data.resize(AlignValue(data.size(), ptrsize), 0);

	return (data.size() == 0) ? 0 : file.Write(data.data(), data.size());
}

MacFixup *MacFixupList::AddDefault(OperandSize cpu_address_size, bool is_code)
{
	return Add(0, is_code ? REBASE_TYPE_TEXT_ABSOLUTE32 : REBASE_TYPE_POINTER, cpu_address_size, false);
}

size_t MacFixupList::Pack()
{
	for (size_t i = 0; i < count(); i++) {
		MacFixup *fixup = item(i);
		MacSymbol *symbol = fixup->symbol();
		if (symbol && symbol->is_deleted())
			fixup->set_deleted(true);
	}

	return BaseFixupList::Pack();
}

void MacFixupList::WriteToData(Data &data, uint64_t image_base)
{
	size_t i, size_pos;
	MacFixup *fixup;
	IMAGE_BASE_RELOCATION reloc;
	uint32_t rva, block_rva;
	uint16_t type_offset, empty_offset;

	Sort();

	size_pos = 0;
	reloc.VirtualAddress = 0;
	reloc.SizeOfBlock = 0;

	for (i = 0; i < count(); i++) {
		fixup = item(i);
		if (fixup->symbol())
			continue;

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
			empty_offset = (static_cast<uint16_t>(rva - block_rva) & 0xf00) << 4 | R_ABS;
		}
		type_offset = (static_cast<uint16_t>(rva - block_rva) & 0xfff) << 4 | fixup->internal_type();
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
 * MacRuntimeFunction
 */

MacRuntimeFunction::MacRuntimeFunction(MacRuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address,
		CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions, uint32_t compact_encoding)
	: BaseRuntimeFunction(owner), address_(address), begin_(begin), end_(end), unwind_address_(unwind_address), cie_(cie),
		call_frame_instructions_(call_frame_instructions), compact_encoding_(compact_encoding)
{

}

MacRuntimeFunction::MacRuntimeFunction(MacRuntimeFunctionList *owner, const MacRuntimeFunction &src)
	: BaseRuntimeFunction(owner)
{
	address_ = src.address_;
	begin_ = src.begin_;
	end_ = src.end_;
	unwind_address_ = src.unwind_address_;
	cie_ = src.cie_;
	call_frame_instructions_ = src.call_frame_instructions_;
	compact_encoding_ = src.compact_encoding_;
}

MacRuntimeFunction *MacRuntimeFunction::Clone(IRuntimeFunctionList *owner) const
{
	MacRuntimeFunction *func = new MacRuntimeFunction(reinterpret_cast<MacRuntimeFunctionList *>(owner), *this);
	return func;
}

void MacRuntimeFunction::Parse(IArchitecture &file, IFunction &dest)
{
	if (!file.AddressSeek(address_) || dest.GetCommandByAddress(address_))
		return;

	if (cie_)
		ParseDwarf(file, dest);
	else 
		ParseBorland(file, dest);
}

void MacRuntimeFunction::ParseDwarf(IArchitecture &file, IFunction &dest)
{
	uint64_t address = address_;
	IntelFunction &func = reinterpret_cast<IntelFunction &>(dest);

	size_t c = func.count();
	IntelCommand *command;
	uint64_t value;
	size_t pos;
	CommandLink *link;
	FunctionInfo *info;
	std::vector<ICommand *> unwind_opcodes;

	if (cie_->version() == 0) {
		command = func.Add(address);
		command->set_comment(CommentInfo(ttComment, "Compact Entry"));
		command->ReadValueFromFile(file, osDWord);
	} else {
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

void MacRuntimeFunction::ParseBorland(IArchitecture &file, IFunction &dest)
{
	uint64_t address = address_;
	IntelFunction &func = reinterpret_cast<IntelFunction &>(dest);

	size_t c = func.count();
	IntelCommand *command;
	CommandLink *link;
	uint64_t value;

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "Begin"));
	command->ReadValueFromFile(file, osDWord);
	address = command->next_address();

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "End"));
	command->ReadValueFromFile(file, osDWord);
	/*address =*/ command->next_address();

	EncodedData data;
	data.resize(30);
	file.AddressSeek(end_ - data.size());
	for (size_t i = 0; i < data.size(); i++) {
		data[data.size() - 1 - i] = file.ReadByte();
	}

	//uint32_t v1 = 0;
	//uint32_t v2 = 0;
	uint32_t v3 = 0;
	uint32_t v4 = 0;
	uint32_t v5 = 0;
	//uint32_t v6 = 0;

	uint8_t b = data[0];
	size_t pos = 1;
	
	if (b & 0x80) {
		/*v6 = */data.ReadUnsigned(&pos);
	}

	if (b & 0x40) {
		if (b & 0x10) {
			/*v1 = */data.ReadUnsigned(&pos);
		} else { //-V523
			/*v2 = */data.ReadUnsigned(&pos);
		}
	}

	if (b & 0x20) {
		v3 = data.ReadUnsigned(&pos);
	}

	if ((b & 0x18) != 0x10) {
		v4 = data.ReadUnsigned(&pos);
		v5 = data.ReadUnsigned(&pos);
	} else if (b & 0x20) {
		v5 = data.ReadUnsigned(&pos);
	}

	address = end_ - pos - sizeof(uint32_t);
	file.AddressSeek(address);
	command = func.Add(address);
	value = command->ReadValueFromFile(file, osDWord);

	command = func.Add(address + command->dump_size());
	command->ReadArray(file, pos);

	if (v3) {
		uint64_t base_address = address - value - v3 * 12 - v4 - v5;
		address = base_address + v5;
		file.AddressSeek(address);
		for (size_t i = 0; i < v3; i++) {
			command = func.Add(address);
			command->set_comment(CommentInfo(ttComment, "Begin"));
			command->ReadValueFromFile(file, osDWord);
			//command->include_option(roRangeBeginEntry);
			address = command->next_address();

			command = func.Add(address);
			command->set_comment(CommentInfo(ttComment, "End"));
			command->ReadValueFromFile(file, osDWord);
			//command->include_option(roRangeEndEntry);
			address = command->next_address();

			command = func.Add(address);
			command->set_comment(CommentInfo(ttComment, "Handler"));
			value = command->ReadValueFromFile(file, osDWord);
			link = command->AddLink(0, ltExtSEHHandler, value + base_address);
			link->set_sub_value(base_address);
			address = command->next_address();
		}
	}

	for (size_t i = c; i < func.count(); i++) {
		command = func.item(i);
		command->exclude_option(roClearOriginalCode);
		command->exclude_option(roNeedCompile);
	}
}

void MacRuntimeFunction::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
	begin_ += delta_base;
	end_ += delta_base;
	if (unwind_address_)
		unwind_address_ += delta_base;
}

/**
 * MacRuntimeFunctionList
 */

MacRuntimeFunctionList::MacRuntimeFunctionList()
	: BaseRuntimeFunctionList(), address_(0)
{
	cie_list_ = new CommonInformationEntryList();
}

MacRuntimeFunctionList::MacRuntimeFunctionList(const MacRuntimeFunctionList &src)
	: BaseRuntimeFunctionList(src)
{
	cie_list_ = src.cie_list_->Clone();
	address_ = src.address_;
	for (size_t i = 0; i < count(); i++) {
		MacRuntimeFunction *func = item(i);
		if (func->cie())
			func->set_cie(cie_list_->item(src.cie_list_->IndexOf(func->cie())));
	}
}	

MacRuntimeFunctionList::~MacRuntimeFunctionList()
{
	delete cie_list_;
}

MacRuntimeFunctionList *MacRuntimeFunctionList::Clone() const
{
	MacRuntimeFunctionList *list = new MacRuntimeFunctionList(*this);
	return list;
}

void MacRuntimeFunctionList::clear()
{
	cie_list_->clear();
	IRuntimeFunctionList::clear();
}

MacRuntimeFunction *MacRuntimeFunctionList::item(size_t index) const
{
	return reinterpret_cast<MacRuntimeFunction *>(IRuntimeFunctionList::item(index));
}

MacRuntimeFunction *MacRuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions)
{
	if (!source)
		throw std::runtime_error("Invalid runtime function");

	MacRuntimeFunction *src = reinterpret_cast<MacRuntimeFunction *>(source);
	return Add(address, begin, end, unwind_address, src->cie(), call_frame_instructions, src->compact_encoding());
}

MacRuntimeFunction *MacRuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions, uint32_t compact_encoding)
{
	MacRuntimeFunction *func = new MacRuntimeFunction(this, address, begin, end, unwind_address, cie, call_frame_instructions, compact_encoding);
	AddObject(func);
	return func;
}

MacRuntimeFunction *MacRuntimeFunctionList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<MacRuntimeFunction *>(BaseRuntimeFunctionList::GetFunctionByAddress(address));
}

void MacRuntimeFunctionList::ReadFromFile(MacArchitecture &file)
{
	MacSection *section = file.runtime_functions_section() ? file.runtime_functions_section() : file.unwind_info_section();
	if (!section) {
		if (file.import_list()->GetImportByName("@rpath/libcgunwind.1.0.dylib")) {
			MacSegment *segment = file.segment_list()->GetSectionByName(SEG_TEXT);
			MacSection *section = segment ? file.section_list()->GetSectionByName(segment, SECT_TEXT) : NULL;
			if (segment && section) {
				Signature sign(NULL, "0700DEFB", 0);
				size_t read_size = 0;
				uint8_t buf[0x1000];
				while (read_size < segment->physical_size()) {
					file.Seek(segment->physical_offset() + read_size);
					size_t n = file.Read(buf, std::min(static_cast<size_t>(segment->physical_size() - read_size), sizeof(buf)));
					for (size_t i = 0; i < n; i++) {
						if (sign.SearchByte(buf[i])) {
							uint64_t address = segment->address() + read_size + i + 1 - sign.size();
							file.AddressSeek(address + sign.size());
							uint32_t delta = file.ReadDWord();
							if (address - delta == section->address()) {
								ReadBorlandInfo(file, address);
								return;
							}
						}
					}
					read_size += n;
				}
			}
		}
	} else if (section->name() == SECT_EH_FRAME) {
		ReadDwarfInfo(file, section->address(), static_cast<uint32_t>(section->size()));
		section = file.unwind_info_section();
		if (section && section->name() == SECT_UNWIND_INFO) {
			MacRuntimeFunctionList tmp_list;
			std::map<CommonInformationEntry *, CommonInformationEntry *> cie_map;
			tmp_list.ReadCompactInfo(file, section->address(), static_cast<uint32_t>(section->size()));
			for (size_t i = 0; i < tmp_list.count(); i++) {
				MacRuntimeFunction *func = tmp_list.item(i);
				if (!GetFunctionByAddress(func->begin())) {
					CommonInformationEntry *cie;
					MacRuntimeFunction *new_func = func->Clone(this);
					std::map<CommonInformationEntry *, CommonInformationEntry *>::const_iterator it = cie_map.find(func->cie());
					if (it != cie_map.end()) {
						cie = it->second;
					}
					else {
						cie = func->cie()->Clone(cie_list_);
						cie_list_->AddObject(cie);
						cie_map[func->cie()] = cie;
					}
					new_func->set_cie(cie);
					AddObject(new_func);
				}
			}
		}
	}
	else if (section->name() == SECT_UNWIND_INFO)
		ReadCompactInfo(file, section->address(), static_cast<uint32_t>(section->size()));
	else if (section->name() == SECT_INIT_TEXT)
		ReadBorlandInfo(file, section->address());
}

void MacRuntimeFunctionList::ReadDwarfInfo(MacArchitecture &file, uint64_t address, uint32_t size)
{
	if (!file.AddressSeek(address))
		throw std::runtime_error("Invalid format");

	size_t pos = 0;
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
			if (version != 1 && version != 3)
				throw std::runtime_error("Invalid CIE version");
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

			uint32_t compact_encoding = DwarfParser::CreateCompactEncoding(file, call_frame_instructions, cie, begin);
			if (lsda_address)
				compact_encoding |= UNWIND_HAS_LSDA;
			Add(cur_address, begin, end, lsda_address, cie, call_frame_instructions, compact_encoding);
		}

		i += sizeof(length) + length;
	}
}

void MacRuntimeFunctionList::ReadCompactInfo(MacArchitecture &file, uint64_t address, uint32_t size)
{
	if (!file.AddressSeek(address))
		throw std::runtime_error("Invalid format");

	uint64_t base_address = file.segment_list()->GetBaseSegment()->address();

	uint64_t pos = file.Tell();

	unwind_info_section_header header;
	file.Read(&header, sizeof(header));
	if (header.version != UNWIND_SECTION_VERSION)
		throw std::runtime_error("Invalid unwind section version");

	file.Seek(pos + header.commonEncodingsArraySectionOffset);
	std::vector<uint32_t> common_encoding_list;
	for (size_t i = 0; i < header.commonEncodingsArrayCount; i++) {
		uint32_t entry = file.ReadDWord();
		common_encoding_list.push_back(entry);
	}

	file.Seek(pos + header.personalityArraySectionOffset);
	std::vector<uint32_t> personality_list;
	for (size_t i = 0; i < header.personalityArrayCount; i++) {
		uint32_t entry = file.ReadDWord();
		personality_list.push_back(entry);
	}

	file.Seek(pos + header.indexSectionOffset);
	std::vector<unwind_info_section_header_index_entry> index_entry_list;
	for (size_t i = 0; i < header.indexCount; i++) {
		unwind_info_section_header_index_entry entry;
		file.Read(&entry, sizeof(entry));
		index_entry_list.push_back(entry);
	}

	size_t lsda_entry_count = (index_entry_list[index_entry_list.size() - 1].lsdaIndexArraySectionOffset - index_entry_list[0].lsdaIndexArraySectionOffset) / sizeof(unwind_info_section_header_lsda_index_entry);
	file.Seek(pos + index_entry_list[0].lsdaIndexArraySectionOffset);
	std::vector<unwind_info_section_header_lsda_index_entry> lsda_entry_list;
	for (size_t i = 0; i < lsda_entry_count; i++) {
		unwind_info_section_header_lsda_index_entry entry;
		file.Read(&entry, sizeof(entry));
		lsda_entry_list.push_back(entry);
	}

	struct Info {
		uint64_t address;
		uint32_t offset;
		uint32_t encoding;
		Info(uint64_t address_, uint32_t offset_, uint32_t encoding_) : address(address_), offset(offset_), encoding(encoding_) {}
	};

	if (index_entry_list.empty() || index_entry_list[index_entry_list.size() - 1].secondLevelPagesSectionOffset)
		throw std::runtime_error("Invalid format");

	std::vector<Info> info_list;
	for (size_t i = 0; i < index_entry_list.size() - 1; i++) {
		size_t page_offset = index_entry_list[i].secondLevelPagesSectionOffset;

		file.Seek(pos + page_offset);
		uint32_t kind = file.ReadDWord();
		file.Seek(pos + page_offset);

		switch (kind) {
		case UNWIND_SECOND_LEVEL_REGULAR:
			{
				unwind_info_regular_second_level_page_header page_header;
				file.Read(&page_header, sizeof(page_header));

				file.Seek(pos + page_offset + page_header.entryPageOffset);
				uint64_t cur_address = address + page_offset + page_header.entryPageOffset;
				for (size_t j = 0; j < page_header.entryCount; j++) {
					unwind_info_regular_second_level_entry entry;
					file.Read(&entry, sizeof(entry));

					info_list.push_back(Info(cur_address, entry.functionOffset, entry.encoding));
					cur_address += sizeof(entry);
				}
			}
			break;
		case UNWIND_SECOND_LEVEL_COMPRESSED:
			{
				unwind_info_compressed_second_level_page_header page_header;
				file.Read(&page_header, sizeof(page_header));

				file.Seek(pos + page_offset + page_header.encodingsPageOffset);
				std::vector<uint32_t> encoding_list;
				for (size_t j = 0; j < page_header.encodingsCount; j++) {
					uint32_t entry = file.ReadDWord();
					encoding_list.push_back(entry);
				}

				file.Seek(pos + page_offset + page_header.entryPageOffset);
				uint64_t cur_address = address + page_offset + page_header.entryPageOffset;
				for (size_t j = 0; j < page_header.entryCount; j++) {
					uint32_t entry = file.ReadDWord();
					uint32_t function_offset = index_entry_list[i].functionOffset + UNWIND_INFO_COMPRESSED_ENTRY_FUNC_OFFSET(entry);
					uint32_t encoding_index = UNWIND_INFO_COMPRESSED_ENTRY_ENCODING_INDEX(entry);
					uint32_t encoding = (encoding_index < common_encoding_list.size()) ? common_encoding_list[encoding_index] : encoding_list[encoding_index - common_encoding_list.size()];

					info_list.push_back(Info(cur_address, function_offset, encoding));
					cur_address += sizeof(entry);
				}
			}
			break;
		default:
			throw std::runtime_error("Invalid page header");
		}
	}
	info_list.push_back(Info(0, index_entry_list[index_entry_list.size() - 1].functionOffset - 1, 0));

	std::map<uint64_t, CommonInformationEntry *> cie_map;
	std::vector<uint8_t> dummy;
	for (size_t i = 0; i < info_list.size() - 1; i++) {
		uint32_t function_offset = info_list[i].offset;
		uint32_t encoding = info_list[i].encoding;
		uint64_t lsda_address = 0;
		if (encoding & UNWIND_HAS_LSDA) {
			for (size_t k = 0; k < lsda_entry_list.size(); k++) {
				if (lsda_entry_list[k].functionOffset == function_offset) {
					lsda_address = base_address + lsda_entry_list[k].lsdaOffset;
					break;
				}
			}
			if (!lsda_address)
				throw std::runtime_error("Invalid lsda index");
		}

		uint64_t personality_routine = 0;
		if (encoding & UNWIND_PERSONALITY_MASK) {
			uint32_t personality_index = (encoding & UNWIND_PERSONALITY_MASK) >> 28;
			if (personality_index > personality_list.size())
				throw std::runtime_error("Invalid personality index");

			personality_routine = base_address + personality_list[personality_index - 1];
			encoding &= ~UNWIND_PERSONALITY_MASK;
		}
		
		CommonInformationEntry *cie;
		std::map<uint64_t, CommonInformationEntry *>::const_iterator it = cie_map.find(personality_routine);
		if (it == cie_map.end()) {
			cie = cie_list_->Add(0, std::string(), 0, 0, 0, 0, 0, 0, personality_routine, std::vector<uint8_t>());
			cie_map[personality_routine] = cie;
		} else {
			cie = it->second;
		}

		Add(info_list[i].address, base_address + function_offset, base_address + info_list[i + 1].offset, lsda_address, cie, dummy, encoding);
	}
}

void MacRuntimeFunctionList::ReadBorlandInfo(MacArchitecture &file, uint64_t address)
{
	struct BORLAND_UNWIND_HEADER {
		uint32_t magic;
		uint32_t delta;
		uint32_t count;
	};

	struct BORLAND_UNWIND_BLOCK {
		uint32_t begin;
		uint32_t end;
		uint32_t count;
		uint32_t offset;
	};

	struct BORLAND_UNWIND_INFO {
		uint32_t begin;
		uint32_t end;
	};

	if (!file.AddressSeek(address))
		return;

	BORLAND_UNWIND_HEADER unwind_header;
	file.Read(&unwind_header, sizeof(unwind_header));
	if (unwind_header.magic != 0xFBDE0007)
		return;

	address_ = address;

	uint64_t base_address = address - unwind_header.delta;
	std::vector<uint8_t> dummy;
	for (size_t i = 0; i < unwind_header.count; i++) {
		BORLAND_UNWIND_BLOCK unwind_block;
		file.Read(&unwind_block, sizeof(unwind_block));
		if (unwind_block.end > unwind_block.begin && unwind_block.count) {
			address = address_ + unwind_block.offset;
			uint64_t pos = file.Tell();
			if (!file.AddressSeek(address))
				throw std::runtime_error("Invalid runtime function address");
			BORLAND_UNWIND_INFO unwind_info;
			for (size_t j = 0; j < unwind_block.count; j++) {
				file.Read(&unwind_info, sizeof(unwind_info));
				Add(address, unwind_info.begin + base_address, unwind_info.end + base_address, unwind_info.end + base_address - 30, NULL, dummy, 0);
				address += sizeof(unwind_info);
			}
			file.Seek(pos);
		}
	}
}

size_t MacRuntimeFunctionList::WriteToFile(MacArchitecture &file, bool compact_info)
{
	Sort();

	if (cie_list_->count())
		return compact_info ? WriteCompactInfo(file) : WriteDwarfInfo(file);
	return 0;
}

size_t MacRuntimeFunctionList::WriteDwarfInfo(MacArchitecture &file)
{
	size_t res = 0;
	uint64_t address = file.AddressTell();
	std::map<CommonInformationEntry*, uint64_t> cie_map;
	for (size_t i = 0; i < count(); i++) {
		MacRuntimeFunction *func = item(i);
		CommonInformationEntry *cie = func->cie();
		if (cie->version() == 0)
			continue;

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

	return res;
}

size_t MacRuntimeFunctionList::WriteCompactInfo(MacArchitecture &file)
{
	size_t i, j;
	MacRuntimeFunction *func;
	uint32_t encoding;
	size_t res = 0;

	std::vector<uint32_t> encoding_list;
	std::map<uint64_t, uint32_t> personality_map;
	std::map<uint32_t, uint8_t> common_encoding_map;
	std::map<uint32_t, size_t> used_encoding_map;
	std::map<uint32_t, uint32_t> lsda_map;
	std::map<uint32_t, uint32_t> lsda_offset_map;
	std::map<uint32_t, uint32_t> page_offset_map;
	std::map<uint32_t, std::vector<uint8_t> > index_map;

	uint64_t start_pos = file.Tell();

	uint64_t base_address = file.segment_list()->GetBaseSegment()->address();

	size_t max_used_count = 0;

	for (i = 0; i < count(); i++) {
		func = item(i);
		encoding = func->compact_encoding();
		CommonInformationEntry *cie = func->cie();
		if (cie->personality_routine()) {
			std::map<uint64_t, uint32_t>::const_iterator it = personality_map.find(cie->personality_routine());
			if (it == personality_map.end()) {
				uint32_t next = static_cast<uint32_t>(personality_map.size() + 1);
				personality_map[cie->personality_routine()] = next;
			}
			uint32_t personality_index = personality_map[cie->personality_routine()];
			encoding |= personality_index << __builtin_ctz(UNWIND_PERSONALITY_MASK);
		}
		encoding_list.push_back(encoding);

		if ((func->compact_encoding() & UNWIND_X86_MODE_MASK) != UNWIND_X86_MODE_DWARF) {
			used_encoding_map[encoding] += 1;
			if (max_used_count < used_encoding_map[encoding])
				max_used_count = used_encoding_map[encoding];
		}

		uint32_t function_offset = static_cast<uint32_t>(func->begin() - base_address);
		lsda_offset_map[function_offset] = static_cast<uint32_t>(lsda_map.size() * sizeof(unwind_info_section_header_lsda_index_entry));
		if (func->unwind_address())
			lsda_map[function_offset] = static_cast<uint32_t>(func->unwind_address() - base_address);
	}

	for (size_t used_count = max_used_count; used_count > 1; used_count--) {
		for (std::map<uint32_t, size_t>::const_iterator ue_it = used_encoding_map.begin(); ue_it != used_encoding_map.end(); ue_it++) {
			if (ue_it->second == used_count) {
				uint32_t next = static_cast<uint8_t>(common_encoding_map.size());
				common_encoding_map[ue_it->first] = next;
				if (common_encoding_map.size() == 127) {
					used_count = 1;
					break;
				}
			}
		}
	}

	// calc pages
	size_t page_size = 0x1000;
	for (i = count(); i > 0; ) {
		std::map<uint32_t, uint8_t> page_encoding_map;
		uint16_t entry_count = 0;
		uint16_t regular_entry_count = static_cast<uint16_t>(std::min((page_size - sizeof(unwind_info_regular_second_level_page_header)) / sizeof(unwind_info_regular_second_level_entry), i));
		size_t max_size = page_size - sizeof(unwind_info_compressed_second_level_page_header);

		uint64_t last_address = item(i - 1)->begin();
		for (j = i; j > 0; j--) {
			func = item(j - 1);
			if (last_address - func->begin() > 0x00ffffff)
				break;

			encoding = encoding_list[j - 1];
			if (common_encoding_map.find(encoding) == common_encoding_map.end() && page_encoding_map.find(encoding) == page_encoding_map.end()) {
				uint32_t encoding_index = static_cast<uint32_t>(common_encoding_map.size() + page_encoding_map.size());
				if (encoding_index > 0xff)
					break;

				page_encoding_map[encoding] = static_cast<uint8_t>(encoding_index);
			}

			if ((page_encoding_map.size() + entry_count) * sizeof(uint32_t) > max_size)
				break;

			entry_count++; 
		}

		bool is_regular_page = (regular_entry_count > entry_count);
		if (is_regular_page)
			entry_count = regular_entry_count;
		i -= entry_count;

		uint64_t start_address = item(i)->begin();
		std::vector<uint8_t> page_data;
		if (is_regular_page) {
			// build regular second level
			unwind_info_regular_second_level_page_header header;
			header.kind = UNWIND_SECOND_LEVEL_REGULAR;
			header.entryPageOffset = sizeof(header);
			header.entryCount = entry_count;

			page_data.insert(page_data.end(), reinterpret_cast<const uint8_t *>(&header), reinterpret_cast<const uint8_t *>(&header) + sizeof(header));
			for (j = 0; j < entry_count; j++) {
				func = item(i + j);
				encoding = encoding_list[i + j];

				unwind_info_regular_second_level_entry entry;
				entry.functionOffset = static_cast<uint32_t>(func->begin() - base_address);
				entry.encoding = func->compact_encoding();

				page_data.insert(page_data.end(), reinterpret_cast<const uint8_t *>(&entry), reinterpret_cast<const uint8_t *>(&entry) + sizeof(entry));
			}
		} else {
			// build compressed second level
			unwind_info_compressed_second_level_page_header header;
			header.kind = UNWIND_SECOND_LEVEL_COMPRESSED;
			header.entryPageOffset = sizeof(header);
			header.entryCount = entry_count;
			header.encodingsPageOffset = header.entryPageOffset + entry_count * sizeof(uint32_t);
			header.encodingsCount = static_cast<uint16_t>(page_encoding_map.size());

			page_data.insert(page_data.end(), reinterpret_cast<const uint8_t *>(&header), reinterpret_cast<const uint8_t *>(&header) + sizeof(header));
			for (j = 0; j < entry_count; j++) {
				func = item(i + j);
				encoding = encoding_list[i + j];

				uint8_t encoding_index;
				std::map<uint32_t, uint8_t>::const_iterator it = common_encoding_map.find(encoding);
				if (it != common_encoding_map.end()) {
					encoding_index = it->second;
				} else {
					encoding_index = page_encoding_map[encoding];
				}

				uint32_t entry = (encoding_index << 24) | static_cast<uint32_t>((func->begin() - start_address));
				page_data.insert(page_data.end(), reinterpret_cast<const uint8_t *>(&entry), reinterpret_cast<const uint8_t *>(&entry) + sizeof(entry));
			}

			if (!page_encoding_map.empty()) {
				std::vector<uint32_t> page_encoding_list;
				page_encoding_list.resize(page_encoding_map.size());
				for (std::map<uint32_t, uint8_t>::const_iterator it = page_encoding_map.begin(); it != page_encoding_map.end(); it++) {
					page_encoding_list[it->second - common_encoding_map.size()] = it->first;
				}

				for (j = 0; j < page_encoding_list.size(); j++) {
					encoding = page_encoding_list[j];
					page_data.insert(page_data.end(), reinterpret_cast<const uint8_t *>(&encoding), reinterpret_cast<const uint8_t *>(&encoding) + sizeof(encoding));
				}
			}
		}

		index_map[static_cast<uint32_t>(start_address - base_address)] = page_data;
	}

	// write section header
	unwind_info_section_header header;
	header.version = UNWIND_SECTION_VERSION;
	header.commonEncodingsArraySectionOffset = sizeof(header);
	header.commonEncodingsArrayCount = static_cast<uint32_t>(common_encoding_map.size());
	header.personalityArraySectionOffset = header.commonEncodingsArraySectionOffset + header.commonEncodingsArrayCount * sizeof(uint32_t);
	header.personalityArrayCount = static_cast<uint32_t>(personality_map.size());
	header.indexSectionOffset = header.personalityArraySectionOffset + header.personalityArrayCount * sizeof(uint32_t);
	header.indexCount = static_cast<uint32_t>(index_map.size() + 1);
	res += file.Write(&header, sizeof(header));

	// write common encoding
	if (!common_encoding_map.empty()) {
		std::vector<uint32_t> common_encoding_list;
		common_encoding_list.resize(common_encoding_map.size());
		for (std::map<uint32_t, uint8_t>::const_iterator it = common_encoding_map.begin(); it != common_encoding_map.end(); it++) {
			common_encoding_list[it->second] = it->first;
		}
		res += file.Write(common_encoding_list.data(), common_encoding_list.size() * sizeof(common_encoding_list[0]));
	}

	// write personality
	if (!personality_map.empty()) {
		std::vector<uint32_t> personality_list;
		personality_list.resize(personality_map.size());
		for (std::map<uint64_t, uint32_t>::const_iterator it = personality_map.begin(); it != personality_map.end(); it++) {
			personality_list[it->second - 1] = static_cast<uint32_t>(it->first - base_address);
		}
		res += file.Write(personality_list.data(), personality_list.size() * sizeof(personality_list[0]));
	}

	// write index
	size_t lsda_start = res + header.indexCount * sizeof(unwind_info_section_header_index_entry);
	size_t lsda_end = lsda_start + lsda_map.size() * sizeof(unwind_info_section_header_lsda_index_entry);
	size_t pages_offset = lsda_end;
	for (std::map<uint32_t, std::vector<uint8_t> >::const_iterator it = index_map.begin(); it != index_map.end(); it++) {
		if (it != index_map.begin())
			pages_offset = static_cast<uint32_t>(AlignValue(start_pos + pages_offset, page_size) - start_pos);
		page_offset_map[it->first] = static_cast<uint32_t>(pages_offset);

		unwind_info_section_header_index_entry entry;
		entry.functionOffset = it->first;
		entry.secondLevelPagesSectionOffset = static_cast<uint32_t>(pages_offset);
		entry.lsdaIndexArraySectionOffset = static_cast<uint32_t>(lsda_start + lsda_offset_map[it->first]);
		res += file.Write(&entry, sizeof(entry));

		pages_offset += it->second.size();
	}
	{
		unwind_info_section_header_index_entry entry;
		entry.functionOffset = (count() > 0) ? static_cast<uint32_t>(item(count() - 1)->end() + 1 - base_address) : 0;
		entry.secondLevelPagesSectionOffset = 0;
		entry.lsdaIndexArraySectionOffset = static_cast<uint32_t>(lsda_end);
		res += file.Write(&entry, sizeof(entry));
	}

	// write lsda index
	if (!lsda_map.empty()) {
		std::vector<unwind_info_section_header_lsda_index_entry> lsda_list;
		for (std::map<uint32_t, uint32_t>::const_iterator it = lsda_map.begin(); it != lsda_map.end(); it++) {
			unwind_info_section_header_lsda_index_entry entry;
			entry.functionOffset = it->first;
			entry.lsdaOffset = it->second;
			lsda_list.push_back(entry);
		}
		res += file.Write(lsda_list.data(), lsda_list.size() * sizeof(lsda_list[0]));
	}

	// write second level data
	for (std::map<uint32_t, std::vector<uint8_t> >::const_iterator it = index_map.begin(); it != index_map.end(); it++) {
		uint64_t old_size = file.size();
		res += static_cast<size_t>(file.Resize(start_pos + page_offset_map[it->first]) - old_size);
		res += file.Write(it->second.data(), it->second.size());
	}

	return res;
}

void MacRuntimeFunctionList::Rebase(uint64_t delta_base)
{
	cie_list_->Rebase(delta_base);
	BaseRuntimeFunctionList::Rebase(delta_base);
}

/**
 * MacArchitecture
 */

MacArchitecture::MacArchitecture(MacFile *owner, uint64_t offset, uint64_t size)
	: BaseArchitecture(owner, offset, size), function_list_(NULL), virtual_machine_list_(NULL), 
	cpu_type_(0), cpu_subtype_(0), cpu_address_size_(osDWord), image_base_(0), entry_point_(0), file_type_(0), 
	cmds_size_(0), flags_(0), header_size_(0), segment_alignment_(0x1000), file_alignment_(0x1000),
	linkedit_segment_(NULL), optimized_segment_count_(0), header_segment_(NULL), max_header_size_(0), 
	runtime_functions_section_(NULL), unwind_info_section_(0), sdk_(0)
{
	memset(&symtab_, 0, sizeof(symtab_));
	memset(&dysymtab_, 0, sizeof(dysymtab_));
	memset(&dyld_info_, 0, sizeof(dyld_info_));

	command_list_ = new MacLoadCommandList(this);
	section_list_ = new MacSectionList(this);
	segment_list_ = new MacSegmentList(this);
	symbol_list_ = new MacSymbolList();
	import_list_ = new MacImportList(this);
	export_list_ = new MacExportList(this);
	indirect_symbol_list_ = new MacIndirectSymbolList();
	ext_ref_symbol_list_ = new MacExtRefSymbolList();
	fixup_list_ = new MacFixupList();
	runtime_function_list_ = new MacRuntimeFunctionList();
}

MacArchitecture::MacArchitecture(MacFile *owner, const MacArchitecture &src)
	: BaseArchitecture(owner, src), function_list_(NULL), virtual_machine_list_(NULL),
	linkedit_segment_(NULL), runtime_functions_section_(NULL), unwind_info_section_(NULL)
{
	size_t i, j, k;

	entry_point_ = src.entry_point_;
	cpu_type_ = src.cpu_type_;
	cpu_subtype_ = src.cpu_subtype_;
	image_base_ = src.image_base_;
	file_type_ = src.file_type_;
	cmds_size_ = src.cmds_size_;
	flags_ = src.flags_;
	header_size_ = src.header_size_;
	cpu_address_size_ = src.cpu_address_size_;
	symtab_ = src.symtab_;
	string_table_ = src.string_table_;
	dysymtab_ = src.dysymtab_;
	dyld_info_ = src.dyld_info_;
	segment_alignment_ = src.segment_alignment_;
	file_alignment_ = src.file_alignment_; 
	max_header_size_ = src.max_header_size_;
	sdk_ = src.sdk_;

	command_list_ = src.command_list_->Clone(this);
	segment_list_ = src.segment_list_->Clone(this);
	section_list_ = src.section_list_->Clone(this);
	symbol_list_ = src.symbol_list_->Clone();
	import_list_ = src.import_list_->Clone(this);
	export_list_ = src.export_list_->Clone(this);
	indirect_symbol_list_ = src.indirect_symbol_list_->Clone();
	ext_ref_symbol_list_ = src.ext_ref_symbol_list_->Clone();
	fixup_list_ = src.fixup_list_->Clone();
	runtime_function_list_ = src.runtime_function_list_->Clone();

	if (src.linkedit_segment_)
		linkedit_segment_ = segment_list_->item(src.segment_list_->IndexOf(src.linkedit_segment_));
	if (src.header_segment_)
		header_segment_ = segment_list_->item(src.segment_list_->IndexOf(src.header_segment_));
	if (src.runtime_functions_section_)
		runtime_functions_section_ = section_list_->item(src.section_list_->IndexOf(src.runtime_functions_section_));
	if (src.unwind_info_section_)
		unwind_info_section_ = section_list_->item(src.section_list_->IndexOf(src.unwind_info_section_));
	if (src.function_list_)
		function_list_ = src.function_list_->Clone(this);
	if (src.virtual_machine_list_)
		virtual_machine_list_ = src.virtual_machine_list_->Clone();

	for (i = 0; i < src.section_list()->count(); i++) {
		MacSegment *segment = src.section_list()->item(i)->parent();
		if (segment)
			section_list_->item(i)->set_parent(segment_list_->item(src.segment_list_->IndexOf(segment)));
	}

	for (i = 0; i < src.indirect_symbol_list()->count(); i++) {
		MacSymbol *symbol = src.indirect_symbol_list()->item(i)->symbol();
		if (symbol)
			indirect_symbol_list_->item(i)->set_symbol(symbol_list_->item(src.symbol_list_->IndexOf(symbol)));
	}

	for (i = 0; i < src.ext_ref_symbol_list_->count(); i++) {
		MacSymbol *symbol = src.ext_ref_symbol_list_->item(i)->symbol();
		if (symbol)
			ext_ref_symbol_list_->item(i)->set_symbol(symbol_list_->item(src.symbol_list_->IndexOf(symbol)));
	}

	for (i = 0; i < src.import_list()->count(); i++) {
		MacImport *import = src.import_list()->item(i);
		for (j = 0; j < import->count(); j++) {
			MacImportFunction *import_function = import->item(j);
			MapFunction *map_function = import_function->map_function();
			if (map_function)
				import_list_->item(i)->item(j)->set_map_function(map_function_list()->item(src.map_function_list()->IndexOf(map_function)));

			MacSymbol *symbol = import_function->symbol();
			if (symbol)
				import_list_->item(i)->item(j)->set_symbol(symbol_list_->item(src.symbol_list_->IndexOf(symbol)));
		}
	}

	for (i = 0; i < src.fixup_list()->count(); i++) {
		MacSymbol *symbol = src.fixup_list()->item(i)->symbol();
		if (symbol)
			fixup_list_->item(i)->set_symbol(symbol_list_->item(src.symbol_list_->IndexOf(symbol)));
	}

	for (i = 0; i < src.export_list()->count(); i++) {
		MacSymbol *symbol = src.export_list()->item(i)->symbol();
		if (symbol)
			export_list_->item(i)->set_symbol(symbol_list_->item(src.symbol_list_->IndexOf(symbol)));
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
	
MacArchitecture::~MacArchitecture()
{
	delete segment_list_;
	delete section_list_;
	delete symbol_list_;
	delete import_list_;
	delete indirect_symbol_list_;
	delete ext_ref_symbol_list_;
	delete command_list_;
	delete export_list_;
	delete fixup_list_;
	delete runtime_function_list_;
	delete function_list_;
	delete virtual_machine_list_;
}

MacArchitecture *MacArchitecture::Clone(IFile *file) const
{
	MacArchitecture *arch = new MacArchitecture(dynamic_cast<MacFile *>(file), *this);
	return arch;
}

std::string MacArchitecture::name() const
{
	static const struct {
		const char *name;
		cpu_type_t cputype;
		cpu_subtype_t cpusubtype;
	} arch_flags[] = {
		{ "any",	CPU_TYPE_ANY,	  CPU_SUBTYPE_MULTIPLE },
		{ "little",	CPU_TYPE_ANY,	  CPU_SUBTYPE_LITTLE_ENDIAN },
		{ "big",	CPU_TYPE_ANY,	  CPU_SUBTYPE_BIG_ENDIAN },

		/* 64-bit Mach-O architectures */

		/* architecture families */
		{ "ppc64",     CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_ALL },
		{ "x86_64",    CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL },
		/* specific architecture implementations */
		{ "ppc970-64", CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_970 },

		/* 32-bit Mach-O architectures */

		/* architecture families */
		{ "ppc",    CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL },
		{ "i386",   CPU_TYPE_I386,    CPU_SUBTYPE_I386_ALL },
		{ "m68k",   CPU_TYPE_MC680x0, CPU_SUBTYPE_MC680x0_ALL },
		{ "hppa",   CPU_TYPE_HPPA,    CPU_SUBTYPE_HPPA_ALL },
		{ "sparc",	CPU_TYPE_SPARC,   CPU_SUBTYPE_SPARC_ALL },
		{ "m88k",   CPU_TYPE_MC88000, CPU_SUBTYPE_MC88000_ALL },
		{ "i860",   CPU_TYPE_I860,    CPU_SUBTYPE_I860_ALL },
		{ "arm",    CPU_TYPE_ARM,     CPU_SUBTYPE_ARM_ALL },
		/* specific architecture implementations */
		{ "ppc601", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_601 },
		{ "ppc603", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603 },
		{ "ppc603e",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603e },
		{ "ppc603ev",CPU_TYPE_POWERPC,CPU_SUBTYPE_POWERPC_603ev },
		{ "ppc604", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604 },
		{ "ppc604e",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604e },
		{ "ppc750", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_750 },
		{ "ppc7400",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7400 },
		{ "ppc7450",CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7450 },
		{ "ppc970", CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_970 },
		{ "i486",   CPU_TYPE_I386,    CPU_SUBTYPE_486 },
		{ "i486SX", CPU_TYPE_I386,    CPU_SUBTYPE_486SX },
		{ "pentium",CPU_TYPE_I386,    CPU_SUBTYPE_PENT }, /* same as i586 */
		{ "i586",   CPU_TYPE_I386,    CPU_SUBTYPE_586 },
		{ "pentpro", CPU_TYPE_I386, CPU_SUBTYPE_PENTPRO }, /* same as i686 */
		{ "i686",   CPU_TYPE_I386, CPU_SUBTYPE_PENTPRO },
		{ "pentIIm3",CPU_TYPE_I386, CPU_SUBTYPE_PENTII_M3 },
		{ "pentIIm5",CPU_TYPE_I386, CPU_SUBTYPE_PENTII_M5 },
		{ "pentium4",CPU_TYPE_I386, CPU_SUBTYPE_PENTIUM_4 },
		{ "m68030", CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68030_ONLY },
		{ "m68040", CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68040 },
		{ "hppa7100LC", CPU_TYPE_HPPA,  CPU_SUBTYPE_HPPA_7100LC },
		{ "armv4t", CPU_TYPE_ARM,     CPU_SUBTYPE_ARM_V4T},
		{ "armv5",  CPU_TYPE_ARM,     CPU_SUBTYPE_ARM_V5TEJ},
		{ "xscale", CPU_TYPE_ARM,     CPU_SUBTYPE_ARM_XSCALE},
		{ "armv6",  CPU_TYPE_ARM,     CPU_SUBTYPE_ARM_V6 },
	};

	for(size_t i = 0; i < _countof(arch_flags); i++) {
		if(arch_flags[i].cputype == cpu_type_ &&
				(arch_flags[i].cpusubtype & ~CPU_SUBTYPE_MASK) ==
				(cpu_subtype_ & ~CPU_SUBTYPE_MASK))
			return std::string(arch_flags[i].name);
	}

	return string_format("unknown 0x%X", cpu_type_);
}

uint64_t MacArchitecture::entry_point() const
{
	return entry_point_;
}

OpenStatus MacArchitecture::ReadFromFile(uint32_t mode)
{
	size_t i;
	mach_header mh;

	Seek(0);

	if (size() < sizeof(mh))
		return osUnknownFormat;

	Read(&mh, sizeof(mh));
	if (mh.magic != MH_MAGIC && mh.magic != MH_MAGIC_64)
		return osUnknownFormat;

	if (mh.magic == MH_MAGIC_64)
		ReadDWord();

	cpu_address_size_ = (mh.magic == MH_MAGIC) ? osDWord : osQWord;
	cpu_type_ = mh.cputype;
	cpu_subtype_ = mh.cpusubtype;
	switch (cpu_type_) {
	case CPU_TYPE_I386:
	case CPU_TYPE_X86_64:
		// supported cpu
		break;
	default:
		return osUnsupportedCPU;
	}

	file_type_ = mh.filetype;
	switch (file_type_) {
	case MH_EXECUTE:
	case MH_DYLIB:
	case MH_BUNDLE:
		// supported types
		break;
	default:
		return osUnsupportedSubsystem;
	}

	cmds_size_ = mh.sizeofcmds;
	flags_ = mh.flags;
	header_size_ = static_cast<uint32_t>(Tell()) + cmds_size_;

	command_list_->ReadFromFile(*this, mh.ncmds);
	segment_list_->ReadFromFile(*this);

	image_base_ = 0;
	for (i = 0; i < segment_list_->count(); i++) {
		uint64_t segment_base = segment_list_->item(i)->address() & 0xffffffff00000000ull;
		if (!image_base_) {
			image_base_ = segment_base;
		} else if (image_base_ != segment_base) {
			return osInvalidFormat;
		}
	}
	
	for (i = 0; i < command_list_->count(); i++) {
		MacLoadCommand *lc = command_list_->item(i);
		switch (lc->type()) {
		case LC_SYMTAB:
			if (lc->size() != sizeof(symtab_))
				throw std::runtime_error("Invalid symtab_command size");

			Seek(lc->address());
			Read(&symtab_, sizeof(symtab_));
			break;
		case LC_DYLD_INFO:
		case LC_DYLD_INFO_ONLY:
			if (lc->size() != sizeof(dyld_info_command))
				throw std::runtime_error("Invalid dyld_info_command size");

			Seek(lc->address());
			Read(&dyld_info_, sizeof(dyld_info_));
			break;
		case LC_DYSYMTAB:
			if (lc->size() != sizeof(dysymtab_))
				throw std::runtime_error("Invalid dysymtab_command size");

			Seek(lc->address());
			Read(&dysymtab_, sizeof(dysymtab_));
			break;
		}
	}

	// parse entry point command
	ILoadCommand *lc = command_list_->GetCommandByType(LC_MAIN);
	if (!lc)
		lc = command_list_->GetCommandByType(LC_UNIXTHREAD);
	if (lc) {
		switch (lc->type()) {
		case LC_UNIXTHREAD:
			{
				thread_command command;
				Seek(lc->address());
				Read(&command, sizeof(command));
				x86_state_hdr_t state_hdr;
				Read(&state_hdr, sizeof(state_hdr));
				if (cpu_type_ == CPU_TYPE_I386 && state_hdr.flavor == x86_THREAD_STATE32) {
					x86_thread_state32_t thread_state;
					Read(&thread_state, sizeof(thread_state));
					entry_point_ = thread_state.__eip;
				} else if (cpu_type_ == CPU_TYPE_X86_64 && state_hdr.flavor == x86_THREAD_STATE64) {
					x86_thread_state64_t thread_state;
					Read(&thread_state, sizeof(thread_state));
					entry_point_ = thread_state.__rip;
				}
			}
			break;
		case LC_MAIN:
			{
				MacSegment *base_segment = segment_list_->GetBaseSegment();
				if (!base_segment)
					throw std::runtime_error("Format error");

				entry_point_command command;
				Seek(lc->address());
				Read(&command, sizeof(command));

				entry_point_ = command.entryoff + base_segment->address();
			}
			break;
		}
	}

	// parse OSX SDK
	for (i = 0; i < command_list_->count(); i++) {
		lc = command_list_->item(i);
		if (lc->type() == LC_VERSION_MIN_MACOSX) {
			Seek(lc->address());
			version_min_command command;
			Read(&command, sizeof(command));
			sdk_ = command.sdk;
			break;
		} else if (lc->type() == LC_BUILD_VERSION) {
			Seek(lc->address());
			build_version_command command;
			Read(&command, sizeof(command));
			sdk_ = command.sdk;
			break;
		}
	}

	linkedit_segment_ = segment_list_->GetSectionByName(SEG_LINKEDIT);
	for (i = 0; i < section_list_->count(); i++) {
		MacSection *section = section_list_->item(i);
		if (section->parent()->name() == SEG_TEXT) {
			if (section->name() == SECT_EH_FRAME || section->name() == SECT_INIT_TEXT) {
				if (!runtime_functions_section_)
					runtime_functions_section_ = section;
			} else if (section->name() == SECT_UNWIND_INFO) {
				if (!unwind_info_section_)
					unwind_info_section_ = section;
			}
		}
	}

	string_table_.ReadFromFile(*this);
	symbol_list_->ReadFromFile(*this, symtab_.nsyms);
	ext_ref_symbol_list_->ReadFromFile(*this);
	indirect_symbol_list_->ReadFromFile(*this);
	fixup_list_->ReadFromFile(*this);
	import_list_->ReadFromFile(*this);
	export_list_->ReadFromFile(*this);
	runtime_function_list_->ReadFromFile(*this);

	header_segment_ = NULL;
	max_header_size_ = header_size_;
	for (i = 0; i < segment_list_->count(); i++) {
		MacSegment *segment = segment_list_->item(i);
		if (segment->physical_size() && segment->physical_offset() == 0) {
			header_segment_ = segment;
			break;
		}
	}
	if (header_segment_) {
		for (i = 0; i < section_list_->count(); i++) {
			MacSection *section = section_list_->item(i);
			if (section->parent() == header_segment_) {
				max_header_size_ = section->physical_offset();
				break;
			}
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

		map_function_list()->ReadFromFile(*this);

		for (i = 0; i < indirect_symbol_list_->count(); i++) {
			MacIndirectSymbol *indirect_symbol = indirect_symbol_list_->item(i);
			MacSymbol *symbol = indirect_symbol->symbol();
			if (!symbol || (symbol->type() & (N_STAB | N_TYPE)) != N_UNDF)
				continue;

			MacSection *section = section_list_->GetSectionByAddress(indirect_symbol->address());
			if (section->type() != S_SYMBOL_STUBS)
				continue;

			MapFunction *map_function = map_function_list()->GetFunctionByAddress(indirect_symbol->address());
			if (!map_function)
				map_function_list()->Add(indirect_symbol->address(), 0, segment_list_->GetMemoryTypeByAddress(indirect_symbol->address()) & mtExecutable ? otCode : otData, DemangleName(symbol->name()));
		}

		for (i = 0; i < symbol_list_->count() - 1; i++) {
			MacSymbol *symbol = symbol_list_->item(i);
			if ((symbol->type() & (N_STAB | N_TYPE)) != N_SECT || symbol->name().empty())
				continue;

			uint32_t memory_type = segment_list_->GetMemoryTypeByAddress(symbol->value());
			if (memory_type == mtNone)
				continue;

			MapFunction *map_function = map_function_list()->GetFunctionByAddress(symbol->value());
			if (!map_function)
				map_function = map_function_list()->Add(symbol->value(), 0, otUnknown, DemangleName(symbol->name()));

			ObjectType type = otData;
			if (memory_type & mtExecutable) {
				MacSection *section = section_list_->GetSectionByAddress(symbol->value());
				if (!section || section->name() != "__const")
					type = (symbol->type() & N_EXT) ? otExport : otCode;
			}
			map_function->set_type(type);
		}

		switch (cpu_type_) {
		case CPU_TYPE_I386:
		case CPU_TYPE_X86_64:
			function_list_ = new MacIntelFunctionList(this);
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

bool MacArchitecture::WriteToFile()
{
	mach_header mh;
	uint32_t pos;

	// read header
	Seek(0);
	Read(&mh, sizeof(mh));
	if (mh.magic == MH_MAGIC_64)
		ReadDWord();
	pos = static_cast<uint32_t>(Tell());
	mh.ncmds = static_cast<uint32_t>(command_list_->count());
	command_list_->WriteToFile(*this);
	header_size_ = static_cast<uint32_t>(Tell());
	if (header_size_ > max_header_size_)
		throw std::runtime_error("Runtime error at WriteToFile");
	mh.sizeofcmds = header_size_ - pos;
	mh.flags = flags_;
	// write header
	Seek(0);
	Write(&mh, sizeof(mh));
	return true;
}

bool MacArchitecture::Compile(CompileOptions &options, IArchitecture *runtime)
{
	if (owner()->count() > 1)
		owner()->Resize(AlignValue(owner()->size(), 0x1000));
	return BaseArchitecture::Compile(options, runtime);
}

void MacArchitecture::Save(CompileContext &ctx)
{
	uint64_t address, pos, loader_crc_address, file_crc_address, patch_section_address, loader_crc_size_address, loader_crc_hash_address, file_crc_size_address;
	size_t i, j, c;
	uint8_t b;
	uint32_t size, loader_crc_size, file_crc_size;
	MacSegment *segment, *last_segment, *vmp_segment;
	uint32_t linkedit_segment_flags;
	std::string linkedit_segment_name;
	MemoryManager *manager;
	MemoryRegion *region;
	int vmp_index;

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

	linkedit_segment_flags = VM_PROT_READ;
	linkedit_segment_name = SEG_LINKEDIT;

	// need erase optimized segments
	for (i = segment_list_->count(); i > optimized_segment_count_; i--) {
		segment = segment_list_->item(i - 1);
		if (linkedit_segment_ == segment) {
			linkedit_segment_flags = segment->flags();
			linkedit_segment_name = segment->name();
			linkedit_segment_ = NULL;
		}
		for (j = section_list_->count(); j > 0; j--) {
			MacSection *section = section_list_->item(j - 1);
			if (section->parent() == segment)
				delete section;
		}
		delete segment;
	}

	// need truncate optimized segments and overlay
	for (i = segment_list_->count(); i > 0; i--) {
		segment = segment_list_->item(i - 1);
		if (segment->physical_size() > 0) {
			Resize(segment->physical_offset() + segment->physical_size());
			break;
		}
	}

	last_segment = segment_list_->last();
	address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_);
	pos = Resize(AlignValue(this->size(), file_alignment_));
	vmp_segment = segment_list_->Add(address, 0xffffffff, static_cast<uint32_t>(pos), 0xffffffff, VM_PROT_READ, "");

	// merge runtime objects
	MacArchitecture *runtime = reinterpret_cast<MacArchitecture*>(ctx.runtime);
	if (runtime && runtime->segment_list()->count()) {
		// merge segments
		for (i = 0; i < runtime->segment_list()->count(); i++) {
			segment = runtime->segment_list()->item(i);
			if (segment->physical_size()) {
				runtime->Seek(segment->physical_offset());
				size = static_cast<uint32_t>(segment->physical_size());
				uint8_t *buffer = new uint8_t[size];
				runtime->Read(buffer, size);
				Write(buffer, size);
				delete [] buffer;
			}
			size = static_cast<uint32_t>(AlignValue(segment->size(), runtime->segment_alignment()) - segment->physical_size());
			uint8_t b = 0;
			for (j = 0; j < size; j++) {
				Write(&b, sizeof(b));
			}
			vmp_segment->include_write_type(segment->memory_type() & (~mtWritable));

			StepProgress();
		}
		// merge fixups
		for (i = 0; i < runtime->fixup_list()->count(); i++) {
			MacFixup *src_fixup = runtime->fixup_list()->item(i);
			MacFixup *fixup = src_fixup->Clone(fixup_list_);
			if (src_fixup->symbol()) {
				MacSymbol *symbol = src_fixup->symbol()->Clone(symbol_list_);
				symbol_list_->AddObject(symbol);
				fixup->set_symbol(symbol);
			}
			fixup_list_->AddObject(fixup);
		}
		// merge import
		int library_ordinal = import_list_->GetMaxLibraryOrdinal() + 1;
		for (i = 0; i < runtime->import_list()->count(); i++) {
			MacImport *src_import = runtime->import_list()->item(i);
			if (src_import->is_sdk())
				continue;

			MacImport *import = import_list_->GetImportByName(src_import->name());
			if (!import) {
				import = new MacImport(import_list_, library_ordinal++, src_import->name(), src_import->current_version(), src_import->compatibility_version());
				import_list_->AddObject(import);
			}

			for (j = 0; j < src_import->count(); j++) {
				MacImportFunction *src_import_function = src_import->item(j);
				MacImportFunction *import_function = src_import_function->Clone(import);
				if (src_import_function->symbol()) {
					MacSymbol *symbol = import_function->symbol()->Clone(symbol_list_);
					symbol_list_->AddObject(symbol);

					symbol->set_library_ordinal(import->library_ordinal());
					import_function->set_symbol(symbol);
				}
				import->AddObject(import_function);
			}
		}
		// merge runtime functions
		if (runtime_functions_section_ && runtime_functions_section_->name() == SECT_EH_FRAME) {
			// dwarf format
			size_t old_count = runtime_function_list_->cie_list()->count();
			for (i = 0; i < runtime->runtime_function_list()->cie_list()->count(); i++) {
				runtime_function_list_->cie_list()->AddObject(runtime->runtime_function_list()->cie_list()->item(i)->Clone(runtime_function_list_->cie_list()));
			}
			for (i = 0; i < runtime->runtime_function_list()->count(); i++) {
				MacRuntimeFunction *runtime_function = runtime->runtime_function_list()->item(i)->Clone(runtime_function_list_);
				if (runtime_function->cie())
					runtime_function->set_cie(runtime_function_list()->cie_list()->item(old_count + runtime->runtime_function_list()->cie_list()->IndexOf(runtime_function->cie())));
				runtime_function_list_->AddObject(runtime_function);
			}
		} else {
			// other formats are not supported
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

	std::map<MacSection *, uint64_t> patch_info_map;
	for (i = 0; i < 2; i++) {
		MacSection *section = NULL;
		switch (i) {
		case 0:
			if (runtime_functions_section_ && runtime_functions_section_->name() == SECT_EH_FRAME)
				section = runtime_functions_section_;
			break;
		case 1:
			section = unwind_info_section_;
			break;
		}
		if (!section)
			continue;

		pos = Resize(AlignValue(this->size(), 0x10));
		address = vmp_segment->address() + pos - vmp_segment->physical_offset();
		size = static_cast<uint32_t>(runtime_function_list_->WriteToFile(*this, (section == unwind_info_section_)));
		if (runtime) {
			section->set_physical_size(0);
			patch_info_map[section] = address + size;
			if (cpu_address_size() == osDWord) {
				WriteDWord(static_cast<uint32_t>(address));
				WriteDWord(size);
			} else {
				WriteQWord(address);
				WriteQWord(size);
			}
			WriteDWord(static_cast<uint32_t>(pos - vmp_segment->physical_offset()));
		} else {
			section->set_address(address);
			section->set_physical_size(size);
		}
		vmp_segment->include_write_type(mtReadable);
	}

	if (vmp_segment->write_type() == mtNone) {
		delete vmp_segment;
	} else {
		vmp_segment->set_name(string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));

		size = static_cast<uint32_t>(this->size() - vmp_segment->physical_offset());
		vmp_segment->set_size(AlignValue(size, segment_alignment_));
		vmp_segment->set_physical_size(static_cast<uint32_t>(AlignValue(size, file_alignment_)));
		vmp_segment->update_type(vmp_segment->write_type());

		Resize(vmp_segment->physical_offset() + vmp_segment->physical_size());
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
			crc_table.Remove(header_segment_->address(), max_header_size_);

		// skip IAT
		size_t ptr_size = OperandSizeToValue(cpu_address_size());
		size_t k = (runtime && runtime->segment_list()->count() > 0) ? 2 : 1;
		for (size_t n = 0; n < k; n++) {
			MacImportList *import_list = (n == 0) ? import_list_ : runtime->import_list();
			for (i = 0; i < import_list->count(); i++) {
				MacImport *import = import_list->item(i);
				for (j = 0; j < import->count(); j++) {
					MacImportFunction *import_function = import->item(j);
					size = (uint32_t)ptr_size;
					if (import_function->options() & ioIsRelative) {
						MacSection *section = section_list_->GetSectionByAddress(import_function->address());
						if (section && section->type() == S_SYMBOL_STUBS)
							size = section->reserved2();
					}
					crc_table.Remove(import_function->address(), size);
				}
			}
		}

		// skip fixups
		for (i = 0; i < fixup_list_->count(); i++) {
			MacFixup *fixup = fixup_list_->item(i);
			if (!fixup->is_deleted() || (ctx.options.flags & cpStripFixups) == 0 || fixup->symbol())
				crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
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

	if (ctx.options.flags & cpStripDebugInfo) {
		// strip symbols
		for (i = 0; i < symbol_list_->count(); i++) {
			MacSymbol *symbol = symbol_list_->item(i);
			if (symbol->is_deleted())
				continue;

			if (symbol->type() & N_EXT) {
				// global symbol
				if ((symbol->type() & N_TYPE) == N_ABS && symbol->value() != 0)
					continue;
				if ((symbol->type() & N_TYPE) == N_UNDF && symbol->value() == 0)
					continue;
				if ((symbol->type() & N_TYPE) == N_PBUD)
					continue;
				if ((symbol->type() & N_TYPE) == N_SECT)
					continue;
				if (symbol->desc() & REFERENCED_DYNAMICALLY)
					continue;
			} else {
				// local symbol
				if (symbol->type() & N_STAB) {
					// debug symbol
				} else if (symbol->type() & N_PEXT) {
					// private extern symbol
					MacExtRefSymbol *ext_ref_symbol = ext_ref_symbol_list_->GetSymbol(symbol);
					if (ext_ref_symbol && (ext_ref_symbol->flags() == REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY || ext_ref_symbol->flags() == REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY))
						continue;
					if (indirect_symbol_list_->GetSymbol(symbol))
						continue;
				}
			}

			symbol->set_deleted(true);
		}

		// strip load commands
		for (i = command_list_->count(); i > 0; i--) {
			MacLoadCommand *load_command = command_list_->item(i - 1);
			switch (load_command->type()) {
			case LC_FUNCTION_STARTS:
			case LC_SOURCE_VERSION:
				delete load_command;
				break;
			}
		}
	}

	string_table_.clear();
	import_list_->Pack();
	ext_ref_symbol_list_->Pack();
	indirect_symbol_list_->Pack();
	command_list_->Pack();
	export_list_->Pack();
	symbol_list_->Pack();

	file_crc_address = 0;
	file_crc_size = 0;
	file_crc_size_address = 0;
	loader_crc_address = 0;
	loader_crc_size = 0;
	loader_crc_size_address = 0;
	loader_crc_hash_address = 0;
	patch_section_address = 0;
	if (runtime) {
		std::vector<IFunction *> processor_list = function_list_->processor_list();
		IntelRuntimeCRCTable *runtime_crc_table = reinterpret_cast<IntelFunctionList *>(function_list_)->runtime_crc_table();
		MacIntelLoader *loader = new MacIntelLoader(NULL, cpu_address_size());
		std::vector<MacSegment *> loader_segment_list;

		last_segment = segment_list_->last();
		address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_);

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

		pos = Resize(AlignValue(this->size(), file_alignment_));
		segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, VM_PROT_READ | VM_PROT_EXECUTE, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
		loader_segment_list.push_back(segment);

		// create import segment
		if (loader->import_segment_address()) {
			size = static_cast<uint32_t>(loader->import_segment_address() - segment->address());
			segment->set_size(size);
			segment->set_physical_size(size);

			address = loader->import_segment_address();
			pos = Resize(segment->physical_offset() + segment->physical_size());
			segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_WRITE, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
			loader_segment_list.push_back(segment);
		}

		// create data segment
		if (loader->data_segment_address()) {
			size = static_cast<uint32_t>(loader->data_segment_address() - segment->address());
			segment->set_size(size);
			segment->set_physical_size(size);

			address = loader->data_segment_address();
			pos = Resize(segment->physical_offset() + segment->physical_size());
			segment = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, VM_PROT_READ | VM_PROT_WRITE, string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
			loader_segment_list.push_back(segment);
		}

		c = loader->WriteToFile(*this);
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
		segment->set_size(AlignValue(size, segment_alignment_));
		segment->set_physical_size(AlignValue(size, file_alignment_));

		Resize(segment->physical_offset() + segment->physical_size());

		if (loader->jmp_table_entry()) {
			address = loader->jmp_table_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->jmp_table_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_SYMBOL_STUBS | S_ATTR_SELF_MODIFYING_CODE, SECT_JUMP_TABLE, segment->name());
			section->set_reserved1(static_cast<uint32_t>(loader->jmp_table_entry()->operand(1).value));
			section->set_reserved2(static_cast<uint32_t>(loader->jmp_table_entry()->dump_size()));
		}

		if (loader->lazy_import_entry()) {
			address = loader->lazy_import_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->lazy_import_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_LAZY_SYMBOL_POINTERS, SECT_LAZY_SYMBOL_PTR, segment->name());
			section->set_reserved1(static_cast<uint32_t>(loader->lazy_import_entry()->operand(1).value));
			section->set_alignment(loader->lazy_import_entry()->alignment());
		}

		if (loader->import_entry()) {
			address = loader->import_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->import_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_NON_LAZY_SYMBOL_POINTERS, SECT_NON_LAZY_SYMBOL_PTR, segment->name());
			section->set_reserved1(static_cast<uint32_t>(loader->import_entry()->operand(1).value));
			section->set_alignment(loader->import_entry()->alignment());

			std::map<MacSection *, MacIndirectSymbol *> indirect_symbol_map;
			for (i = 0; i < section_list_->count(); i++) {
				section = section_list_->item(i);
				if (section->type() == S_SYMBOL_STUBS || section->type() == S_LAZY_SYMBOL_POINTERS || section->type() == S_NON_LAZY_SYMBOL_POINTERS || section->type() == S_THREAD_LOCAL_VARIABLE_POINTERS)
					indirect_symbol_map[section] = indirect_symbol_list_->item(section->reserved1());
			}

			// delete S_NON_LAZY_SYMBOL_POINTERS and S_SYMBOL_STUBS sections
			size_t ptr_size = OperandSizeToValue(cpu_address_size());
			for (i = section_list_->count(); i > 0; i--) {
				section = section_list_->item(i - 1);
				if (std::find(loader_segment_list.begin(), loader_segment_list.end(), section->parent()) != loader_segment_list.end())
					continue;

				if (section->type() == S_NON_LAZY_SYMBOL_POINTERS || section->type() == S_SYMBOL_STUBS) {
					indirect_symbol_map.erase(section);
					size_t value_size = (section->type() == S_SYMBOL_STUBS) ? section->reserved2() : ptr_size;
					size_t c = static_cast<uint32_t>(section->size()) / value_size;
					for (j = 0; j < c; j++) {
						indirect_symbol_list_->Delete(section->reserved1());
					}
					delete section;
				}
			}

			// recalc indirect symbol indexes
			for (std::map<MacSection *, MacIndirectSymbol *>::iterator it = indirect_symbol_map.begin(); it != indirect_symbol_map.end(); it++) {
				section = it->first;
				MacIndirectSymbol *indirect_symbol = it->second;
				section->set_reserved1(static_cast<uint32_t>(indirect_symbol_list_->IndexOf(indirect_symbol)));
			}
		}

		if (loader->init_entry()) {
			address = loader->init_entry()->operand(0).value;
			MacSegment *base_segment = segment_list()->GetBaseSegment();
			if (base_segment && address - base_segment->address() < max_header_size_)
				max_header_size_ = static_cast<uint32_t>(address - base_segment->address());

			// delete S_MOD_INIT_FUNC_POINTERS sections
			for (i = section_list_->count(); i > 0; i--) {
				MacSection *section = section_list_->item(i - 1);
				if (section->type() == S_MOD_INIT_FUNC_POINTERS)
					delete section;
			}

			address = loader->init_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->init_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_MOD_INIT_FUNC_POINTERS, SECT_MOD_INIT_FUNC, segment->name());
			section->set_alignment(loader->init_entry()->alignment());
		}

		if (loader->term_entry()) {
			// delete S_MOD_TERM_FUNC_POINTERS sections
			for (i = section_list_->count(); i > 0; i--) {
				MacSection *section = section_list_->item(i - 1);
				if (section->type() == S_MOD_TERM_FUNC_POINTERS)
					delete section;
			}

			address = loader->term_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->term_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_MOD_TERM_FUNC_POINTERS, SECT_MOD_TERM_FUNC, segment->name());
			section->set_alignment(loader->term_entry()->alignment());
		}

		if (loader->thread_variables_entry()) {
			// delete S_THREAD_LOCAL_VARIABLES sections
			for (i = section_list_->count(); i > 0; i--) {
				MacSection *section = section_list_->item(i - 1);
				if (section->type() == S_THREAD_LOCAL_VARIABLES)
					delete section;
			}

			address = loader->thread_variables_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->thread_variables_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_THREAD_LOCAL_VARIABLES, SECT_THREAD_LOCAL_VARIABLES, segment->name());
			section->set_alignment(loader->thread_variables_entry()->alignment());
		}

		if (loader->thread_data_entry()) {
			// delete S_THREAD_LOCAL_REGULAR and S_THREAD_LOCAL_ZEROFILL sections
			for (i = section_list_->count(); i > 0; i--) {
				MacSection *section = section_list_->item(i - 1);
				if (section->type() == S_THREAD_LOCAL_REGULAR || section->type() == S_THREAD_LOCAL_ZEROFILL)
					delete section;
			}

			address = loader->thread_data_entry()->address();
			segment = segment_list_->GetSectionByAddress(address);
			MacSection *section = section_list_->Add(segment, address, loader->thread_data_size(), static_cast<uint32_t>(segment->physical_offset() + address - segment->address()), S_THREAD_LOCAL_REGULAR, SECT_THREAD_LOCAL_REGULAR, segment->name());
			section->set_alignment(loader->thread_data_entry()->alignment());
		}

		if (loader->file_entry()) {
			address = loader->file_entry()->address();
			MacSegment *base_segment = segment_list()->GetBaseSegment();
			if (base_segment && address - base_segment->address() < max_header_size_)
				max_header_size_ = static_cast<uint32_t>(address - base_segment->address());
			entry_point_ = address;
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

		if (loader->patch_section_entry())
			patch_section_address = loader->patch_section_entry()->address();

		// codesign requires S_ZEROFILL for packed sections
		std::vector<MacSegment *> packed_segment_list = loader->packed_segment_list();
		for (i = 0; i < section_list()->count(); i++) {
			MacSection *section = section_list()->item(i);
			if (std::find(packed_segment_list.begin(), packed_segment_list.end(), section->parent()) == packed_segment_list.end())
				continue;

			section->set_physical_offset(0);
			if (section->flags() != S_THREAD_LOCAL_ZEROFILL)
				section->set_flags(S_ZEROFILL);
		}

		if (file_type_ == MH_DYLIB || sdk_ >= DYLD_MACOSX_VERSION_10_12) {
			// exclude EXECUTABLE flag for packed segments
			for (i = 0; i < packed_segment_list.size(); i++) {
				segment = packed_segment_list[i];
				if (segment == header_segment_) {
					if (header_segment_->size() > header_segment_->physical_size()) {
						header_segment_->set_physical_size(AlignValue(header_segment_->physical_size(), segment_alignment_));
						address = header_segment_->address() + header_segment_->physical_size();
						segment = new MacSegment(segment_list_, address, header_segment_->address() + header_segment_->size() - address,
							header_segment_->physical_offset() + header_segment_->physical_size(), 0, header_segment_->flags() & ~VM_PROT_EXECUTE, ctx.options.section_name);
						segment->include_maxprot(VM_PROT_EXECUTE | VM_PROT_WRITE);
						segment_list_->InsertObject(segment_list_->IndexOf(header_segment_) + 1, segment);
						header_segment_->set_size(header_segment_->physical_size());
					}
				}
				else if (segment->flags() & VM_PROT_EXECUTE)
					segment->set_flags(segment->flags() & ~VM_PROT_EXECUTE);
			}
		}

		delete loader;

		ctx.file->EndProgress();
	}

	// add LINK_EDIT segment
	last_segment = segment_list_->last();
	address = AlignValue(last_segment->address() + last_segment->size(), segment_alignment_);
	pos = Resize(AlignValue(this->size(), file_alignment_));
	linkedit_segment_ = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, linkedit_segment_flags, linkedit_segment_name);

	if (ctx.options.flags & cpStripFixups) {
		for (i = fixup_list_->count(); i > 0; i--) {
			MacFixup *fixup = fixup_list_->item(i - 1);
			if (!fixup->symbol())
				delete fixup;
		}
	}
	fixup_list_->Pack();

	fixup_list_->WriteToFile(*this);
	import_list_->WriteToFile(*this);
	export_list_->WriteToFile(*this);

	// write LINK_EDIT data
	IArchitecture *source = const_cast<IArchitecture *>(this->source());
	for (i = 0; i < command_list_->count(); i++) {
		MacLoadCommand *load_command = command_list_->item(i);
		switch (load_command->type()) {
		case LC_FUNCTION_STARTS:
		case LC_DATA_IN_CODE:
		case LC_DYLIB_CODE_SIGN_DRS:
			{
				linkedit_data_command command;
				source->Seek(load_command->address());
				source->Read(&command, sizeof(command));
				if (source->Seek(command.dataoff)) {
					load_command->set_offset(static_cast<uint32_t>(Tell()));
					CopyFrom(*source, command.datasize);
				}
			}
			break;
		}
	}

	symbol_list_->WriteToFile(*this);
	ext_ref_symbol_list_->WriteToFile(*this);
	indirect_symbol_list_->WriteToFile(*this);
	segment_list_->WriteToFile(*this);
	string_table_.WriteToFile(*this);

	size = static_cast<uint32_t>(this->size() - linkedit_segment_->physical_offset());
	linkedit_segment_->set_size(AlignValue(size, segment_alignment_));
	linkedit_segment_->set_physical_size(size);

	if (ctx.options.script)
		ctx.options.script->DoAfterSaveFile();

	// write header
	if ((ctx.options.flags & cpStripFixups) && file_type_ == MH_EXECUTE)
		flags_ &= ~MH_PIE;
	WriteToFile();

	std::map<uint64_t, size_t> patch_header_map;
	if (patch_section_address && AddressSeek(patch_section_address)) {
		for (std::map<MacSection *, uint64_t>::const_iterator it = patch_info_map.begin(); it != patch_info_map.end(); it++) {
			MacSection *patch_section = it->first;
			uint64_t patch_info_address = it->second;
			MacLoadCommand *load_command = command_list_->GetCommandByObject(patch_section->parent());
			if (load_command) {
				size_t section_index = 0;
				for (size_t i = 0; i < section_list_->count(); i++) {
					if (section_list_->item(i) == patch_section)
						break;
					if (section_list_->item(i)->parent() == patch_section->parent())
						section_index++;
				}

				// patch section.adr + section.size + section.offset
				address = segment_list_->GetBaseSegment()->address() + load_command->address() + ((cpu_address_size() == osDWord) ? sizeof(segment_command) : sizeof(segment_command_64))
					+ section_index * ((cpu_address_size() == osDWord) ? sizeof(section) : sizeof(section_64)) + offsetof(section, addr);
				size = (cpu_address_size() == osDWord) ? sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) : sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t);
				WriteDWord(static_cast<uint32_t>(patch_info_address - image_base()));
				WriteDWord(static_cast<uint32_t>(address - image_base()));
				WriteDWord(size);

				patch_header_map[address] = size;
			}
			patch_section_address += 3 * sizeof(uint32_t);
		}
	}

	// write header and loader CRC table
	if (loader_crc_address) {
		CRCTable crc_table(function_list_->crc_cryptor(), loader_crc_size);

		// add header
		if (header_segment_) {
			crc_table.Add(header_segment_->address(), max_header_size_);
			// skip place for codesign's commands
			crc_table.Remove(header_segment_->address() + header_size_, max_header_size_ - header_size_);
			// skip size of commands list
			crc_table.Remove(header_segment_->address() + offsetof(mach_header, ncmds), sizeof(uint32_t));
			crc_table.Remove(header_segment_->address() + offsetof(mach_header, sizeofcmds), sizeof(uint32_t));
			// skip sizes of LINKEDIT
			MacLoadCommand *linkedit_command = command_list()->GetCommandByObject(linkedit_segment_);
			if (linkedit_command) {
				if (cpu_address_size() == osDWord) {
					crc_table.Remove(header_segment_->address() + linkedit_command->address() + offsetof(segment_command, vmsize), sizeof(uint32_t));
					crc_table.Remove(header_segment_->address() + linkedit_command->address() + offsetof(segment_command, filesize), sizeof(uint32_t));
				} else {
					crc_table.Remove(header_segment_->address() + linkedit_command->address() + offsetof(segment_command_64, vmsize), sizeof(uint64_t));
					crc_table.Remove(header_segment_->address() + linkedit_command->address() + offsetof(segment_command_64, filesize), sizeof(uint64_t));
				}
			}
			for (std::map<uint64_t, size_t>::const_iterator it = patch_header_map.begin(); it != patch_header_map.end(); it++) {
				crc_table.Remove(it->first, it->second);
			}
		}

		// add loader segments
		j = segment_list_->IndexOf(segment_list_->GetSectionByAddress(loader_crc_address));
		if (j != NOT_ID) {
			c = (ctx.options.flags & cpLoaderCRC) ? j + 1 : segment_list_->count();
			for (i = j; i < c; i++) {
				segment = segment_list_->item(i);
				if (segment->memory_type() & mtWritable)
					continue;

				size = std::min(static_cast<uint32_t>(segment->size()), segment->physical_size());
				if (size)
					crc_table.Add(segment->address(), size);

				// skip import
				for (j = 0; j < section_list_->count(); j++) {
					MacSection *section = section_list_->item(j);
					if (section->parent() == segment && (section->type() == S_NON_LAZY_SYMBOL_POINTERS || section->type() == S_SYMBOL_STUBS))
						crc_table.Remove(section->address(), static_cast<size_t>(section->size()));
				}
			}
		}

		// skip fixups
		for (i = 0; i < fixup_list_->count(); i++) {
			MacFixup *fixup = fixup_list_->item(i);
			crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
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
		// skip place for codesign's commands
		crc_table.Remove(header_size_, max_header_size_ - header_size_);
		// skip size of commands list
		crc_table.Remove(offsetof(mach_header, ncmds), sizeof(uint32_t));
		crc_table.Remove(offsetof(mach_header, sizeofcmds), sizeof(uint32_t));
		// skip sizes of LINKEDIT
		MacLoadCommand *linkedit_command = command_list()->GetCommandByObject(linkedit_segment_);
		if (linkedit_command) {
			if (cpu_address_size() == osDWord) {
				crc_table.Remove(linkedit_command->address() + offsetof(segment_command, vmsize), sizeof(uint32_t));
				crc_table.Remove(linkedit_command->address() + offsetof(segment_command, filesize), sizeof(uint32_t));
			} else {
				crc_table.Remove(linkedit_command->address() + offsetof(segment_command_64, vmsize), sizeof(uint64_t));
				crc_table.Remove(linkedit_command->address() + offsetof(segment_command_64, filesize), sizeof(uint64_t));
			}
		}

		// skip file CRC table
		if (AddressSeek(file_crc_address))
			crc_table.Remove(Tell(), file_crc_size);
		if (AddressSeek(file_crc_size_address))
			crc_table.Remove(Tell(), sizeof(uint32_t));

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

bool MacArchitecture::Prepare(CompileContext &ctx)
{
	if ((ctx.options.flags & cpStripFixups) == 0 && file_type_ == MH_EXECUTE && (flags_ & MH_PIE) == 0)
		ctx.options.flags |= cpStripFixups;
	else if (ctx.runtime)
		ctx.options.flags |= cpLoader;

	if (ctx.options.flags & cpImportProtection)
		ctx.options.flags &= ~cpImportProtection;

	if (ctx.options.flags & cpResourceProtection)
		ctx.options.flags &= ~cpResourceProtection;

	if (ctx.runtime && ((runtime_functions_section_ && runtime_functions_section_->name() == SECT_EH_FRAME) || unwind_info_section_))
		ctx.options.flags |= cpLoader;

	if (!BaseArchitecture::Prepare(ctx))
		return false;

	size_t i;
	MacSegment *segment;
	std::vector<MacSegment *> optimized_segment_list;

	// optimize segments
	if (linkedit_segment_)
		optimized_segment_list.push_back(linkedit_segment_);

	optimized_segment_count_ = segment_list_->count();
	for (i = segment_list_->count(); i > 0; i--) {
		segment = segment_list_->item(i - 1);

		std::vector<MacSegment *>::iterator it = std::find(optimized_segment_list.begin(), optimized_segment_list.end(), segment);
		if (it != optimized_segment_list.end()) {
			optimized_segment_list.erase(it);
			optimized_segment_count_--;
		} else {
			break;
		}
	}

	// calc new header size
	size_t new_header_size = (cpu_address_size_ == osDWord) ? sizeof(mach_header) : sizeof(mach_header_64);
	for (i = 0; i < command_list_->count(); i++) {
		MacLoadCommand *load_command = command_list_->item(i);
		switch (load_command->type()) {
		case LC_SEGMENT:
		case LC_SEGMENT_64:
		case LC_LOAD_DYLIB:
		case LC_LOAD_WEAK_DYLIB:
		case LC_CODE_SIGNATURE:
			continue;
		case LC_FUNCTION_STARTS:
		case LC_SOURCE_VERSION:
			if (ctx.options.flags & cpStripDebugInfo)
				continue;
			break;
		}
		new_header_size += load_command->size();
	}
	size_t segment_size = (cpu_address_size_ == osDWord) ? sizeof(segment_command) : sizeof(segment_command_64);
	size_t section_size = (cpu_address_size_ == osDWord) ? sizeof(section) : sizeof(section_64);
	new_header_size += (optimized_segment_count_ + 2) * segment_size;
	new_header_size += section_list_->count() * section_size;
	for (i = 0; i < import_list_->count(); i++) {
		MacImport *import = import_list_->item(i);
		if (import->is_sdk())
			continue;
		new_header_size += AlignValue(sizeof(dylib_command) + import->name().size() + 1, OperandSizeToValue(cpu_address_size()));
	}

	if (ctx.runtime) {
		for (i = 0; i < section_list_->count(); i++) {
			MacSection *section = section_list_->item(i);
			if (section->type() == S_NON_LAZY_SYMBOL_POINTERS || section->type() == S_SYMBOL_STUBS ||
				section->type() == S_MOD_INIT_FUNC_POINTERS || section->type() == S_MOD_TERM_FUNC_POINTERS)
				new_header_size -= section_size;
		}
		new_header_size += 2 * segment_size;
		new_header_size += section_size * 4; // S_SYMBOL_STUBS / S_LAZY_SYMBOL_POINTERS + S_NON_LAZY_SYMBOL_POINTERS + S_MOD_INIT_FUNC_POINTERS + S_MOD_TERM_FUNC_POINTERS
		new_header_size += 5; // jmp xxxx
		if (entry_point_)
			new_header_size += 5; // jmp xxxx
		if ((ctx.options.flags & cpPack) && file_type_ == MH_DYLIB)
			new_header_size += segment_size;

		for (i = 0; i < ctx.runtime->import_list()->count(); i++) {
			IImport *import = ctx.runtime->import_list()->item(i);
			if (import->is_sdk() || import_list_->GetImportByName(import->name()))
				continue;
			new_header_size += AlignValue(sizeof(dylib_command) + import->name().size() + 1, OperandSizeToValue(cpu_address_size()));
		}
	}

	for (i = 0; i < section_list_->count(); i++) {
		MacSection *section = section_list_->item(i);
		MacSegment *segment = section->parent();

		if (segment->physical_size() && new_header_size > segment->physical_offset() && new_header_size > section->physical_offset()) {
			Notify(mtError, NULL, language[lsCreateSegmentError]);
			return false;
		}
	}

	for (i = 0; i < optimized_segment_list.size(); i++) {
		segment = optimized_segment_list[i];
		ctx.manager->Add(segment->address(), static_cast<uint32_t>(segment->physical_size()), mtReadable | mtExecutable | mtWritable | mtNotPaged, NULL);
	}

	if (optimized_segment_count_ > 0) {
		segment = segment_list_->item(optimized_segment_count_ - 1);
		if (ctx.runtime) {
			MacArchitecture *runtime = reinterpret_cast<MacArchitecture *>(ctx.runtime);
			if (runtime->segment_list()->count()) {
				MemoryManager runtime_manager(runtime);
				if (runtime->segment_list()->last() == runtime->linkedit_segment_) {
					delete runtime->linkedit_segment_;
					runtime->linkedit_segment_ = NULL;
				}
				runtime->Rebase(AlignValue(segment->address() + segment->size(), segment_alignment()) - runtime->segment_list()->item(0)->address(), dyld_info_.lazy_bind_size);
				if (runtime->header_segment_)
					runtime_manager.Add(runtime->header_segment_->address(), runtime->max_header_size_);
				if (runtime->unwind_info_section_)
					runtime_manager.Add(runtime->unwind_info_section_->address(), static_cast<size_t>(runtime->unwind_info_section_->size()));
				if (runtime->runtime_functions_section_)
					runtime_manager.Add(runtime->runtime_functions_section_->address(), static_cast<size_t>(runtime->runtime_functions_section_->size()));
				runtime_manager.Pack();
				for (i = 0; i < runtime_manager.count(); i++) {
					MemoryRegion *region = runtime_manager.item(i);
					ctx.manager->Add(region->address(), region->size(), region->type());
				}
				segment = runtime->segment_list()->last();
			} else {
				runtime->Rebase(image_base() - runtime->image_base(), 0);
			}
		}

		// add new segment
		assert(segment);
		ctx.manager->Add(AlignValue(segment->address() + segment->size(), segment_alignment()), UINT32_MAX, mtReadable | mtExecutable | mtWritable | (runtime_function_list()->count() ? mtSolid : mtNone));
	}

	if (unwind_info_section_)
		ctx.manager->Add(unwind_info_section_->address(), static_cast<size_t>(unwind_info_section_->size()));
	if (runtime_functions_section_ && runtime_functions_section_->name() == SECT_EH_FRAME)
		ctx.manager->Add(runtime_functions_section_->address(), static_cast<size_t>(runtime_functions_section_->size()));

	return true;
}

uint64_t MacArchitecture::GetRelocBase() const
{
	if (cpu_address_size() == osQWord || (flags_ & MH_SPLIT_SEGS) != 0) {
		for (size_t i = 0; i < segment_list_->count(); i++) {
			MacSegment *segment = segment_list_->item(i);
			if (segment->memory_type() & mtWritable)
				return segment->address();
		}
	} else {
		if (segment_list_->count())
			return segment_list_->item(0)->address();
	}
	throw std::runtime_error("Runtime error at GetRelocBase");
}

void MacArchitecture::Rebase(uint64_t delta_base, size_t delta_bind_info)
{
	BaseArchitecture::Rebase(delta_base);

	fixup_list_->Rebase(*this, delta_base);
	export_list_->Rebase(delta_base);
	indirect_symbol_list_->Rebase(delta_base);
	command_list_->Rebase(delta_base);
	segment_list_->Rebase(delta_base);
	section_list_->Rebase(delta_base);
	import_list_->Rebase(delta_base);
	import_list_->RebaseBindInfo(*this, delta_bind_info);
	function_list_->Rebase(delta_base);
	runtime_function_list_->Rebase(delta_base);

	if (entry_point_)
		entry_point_ += delta_base;
	image_base_ += delta_base;
}

bool MacArchitecture::is_executable() const
{
	return file_type() == MH_EXECUTE;
}

/**
 * MacFile
 */

MacFile::MacFile(ILog *log) 
	: IFile(log), fat_magic_(0), runtime_(NULL)
{
	
}

MacFile::~MacFile()
{
	delete runtime_;
}

MacFile::MacFile(const MacFile &src, const char *file_name)
	: IFile(src, file_name), runtime_(NULL)
{
	fat_magic_ = src.fat_magic_;
	for (size_t i = 0; i < src.count(); i++)
		AddObject(src.item(i)->Clone(this));
}

std::string MacFile::format_name() const
{
	return std::string("Mach-O");
}

MacArchitecture *MacFile::item(size_t index) const
{ 
	return reinterpret_cast<MacArchitecture *>(IFile::item(index));
}

MacArchitecture *MacFile::Add(uint64_t offset, uint64_t size)
{
	MacArchitecture *arch = new MacArchitecture(this, offset, size);
	AddObject(arch);
	return arch;
}

OpenStatus MacFile::ReadHeader(uint32_t open_mode)
{
	Seek(0);

	fat_header fat;
	if (size() < sizeof(fat))
		return osUnknownFormat;

	Read(&fat, sizeof(fat));
	if (fat.magic == FAT_MAGIC || fat.magic == FAT_CIGAM) {
		fat_magic_ = fat.magic;
		if (fat_magic_ == FAT_CIGAM)
			fat.nfat_arch = __builtin_bswap32(fat.nfat_arch);
		size_t i;
		for (i = 0; i < fat.nfat_arch; i++) {
			fat_arch header;
			Read(&header, sizeof(header));
			if (fat_magic_ == FAT_CIGAM) {
				header.offset = __builtin_bswap32(header.offset);
				header.size = __builtin_bswap32(header.size);
			}
			Add(header.offset, header.size);
		}
		OpenStatus res = osSuccess;
		for (i = 0; i < count(); i++) {
			MacArchitecture *arch = item(i);
			OpenStatus status = arch->ReadFromFile(open_mode);
			if (status != osSuccess) {
				res = status;
				if (res == osUnknownFormat || res == osInvalidFormat)
					break;
			}
		}
		return res;
	} else {
		fat_magic_ = 0;
		MacArchitecture *arch = Add(0, size());
		return arch->ReadFromFile(open_mode);
	}
}

MacFile *MacFile::Clone(const char *file_name) const
{
	MacFile *file = new MacFile(*this, file_name);
	return file;
}

bool MacFile::Compile(CompileOptions &options)
{
	MacArchitecture *arch;
	size_t i;

	if (fat_magic_) {
		fat_header fat;
		fat.magic = fat_magic_;
		fat.nfat_arch = static_cast<uint32_t>(count());
		if (fat_magic_ == FAT_CIGAM)
			fat.nfat_arch = __builtin_bswap32(fat.nfat_arch);
		Write(&fat, sizeof(fat));

		fat_arch header = fat_arch();
		for (i = 0; i < count(); i++) {
			Write(&header, sizeof(header));
		}
		Resize(AlignValue(size(), 0x1000));
	}

	const ResourceInfo runtime_info[] = {
		{mac_runtime32_dylib_file, sizeof(mac_runtime32_dylib_file), mac_runtime32_dylib_code},
		{mac_runtime64_dylib_file, sizeof(mac_runtime64_dylib_file), mac_runtime64_dylib_code}
	};
	
	for (size_t k = 0; k < count(); k++) {
		arch = item(k);
		if (!arch->visible())
			continue;

		ResourceInfo info = runtime_info[(arch->cpu_address_size() == osDWord) ? 0 : 1];
		if (info.size > 1) {
			if (runtime_) {
				uint32_t key = *reinterpret_cast<const uint32_t *>(info.file);
				MemoryStreamEnc *stream = new MemoryStreamEnc(info.file + sizeof(key), info.size - sizeof(key), key);
				arch = runtime_->Add(runtime_->size(), stream->Size());
				stream->Seek(0, soBeginning); 
				runtime_->stream_->Seek(0, soEnd); 
				runtime_->stream_->CopyFrom(*stream, static_cast<uint32_t>(stream->Size()));
				delete stream;

				if (arch->ReadFromFile(foRead) != osSuccess)
					throw std::runtime_error("Runtime error at OpenResource");
			} else {
				runtime_ = new MacFile(NULL);
				if (!runtime_->OpenResource(info.file, info.size, true))
					throw std::runtime_error("Runtime error at OpenResource");
				arch = runtime_->item(0);
			}

			Buffer buffer(info.code);
			arch->ReadFromBuffer(buffer);
			for (i = 0; i < arch->function_list()->count(); i++) {
				arch->function_list()->item(i)->set_from_runtime(true);
			}
			for (i = 0; i < arch->import_list()->count(); i++) {
				MacImport *import = arch->import_list()->item(i);
				for (size_t j = 0; j < import->count(); j++) {
					import->item(j)->include_option(ioFromRuntime);
				}
			}
		}
	}

	if (!IFile::Compile(options))
		return false;

	if (fat_magic_) {
		Seek(sizeof(fat_header));

		fat_arch header;
		
		for (i = 0; i < count(); i++) {
			MacArchitecture *arch = item(i);
			header.cputype = arch->type();
			header.cpusubtype = arch->cpu_subtype();
			header.offset = static_cast<uint32_t>(arch->offset());
			header.size = static_cast<uint32_t>(arch->size());
			header.align = 0x0c; // 2 << 0xc = 0x1000

			if (fat_magic_ == FAT_CIGAM) {
				header.cputype = __builtin_bswap32(header.cputype);
				header.cpusubtype = __builtin_bswap32(header.cpusubtype);
				header.offset = __builtin_bswap32(header.offset);
				header.size = __builtin_bswap32(header.size);
				header.align = __builtin_bswap32(header.align);
			}
			Write(&header, sizeof(header));
		}
	}

	return true;
}

bool MacFile::is_executable() const
{
#ifdef __unix__
	return false;
#elif __APPLE__
	for (size_t i = 0; i < count(); i++) {
		if (item(i)->is_executable())
			return true;
	}
#endif
	return false;
}

uint32_t MacFile::disable_options() const
{
	uint32_t res = cpResourceProtection | cpImportProtection | cpVirtualFiles;
	for (size_t i = 0; i < count(); i++) {
		MacArchitecture *arch = item(i);
		if (arch->file_type() != MH_EXECUTE)
			res |= cpStripFixups;
	}
	return res;
}

bool MacFixupList::RebaseInfoHelper::operator()(const MacFixup *left, const MacFixup *right) const
{
	// sort by type, then address
	if ( left->bind_type() != right->bind_type())
		return (left->bind_type() < right->bind_type());
	return (left->address() < right->address());
}