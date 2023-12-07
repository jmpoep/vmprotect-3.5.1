/**
 * Support of PE executable files.
 */

#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "pefile.h"
#include "dotnetfile.h"
#include "processors.h"
#include "intel.h"
#include "lang.h"
#include "core.h"
#include "script.h"
#include "pdb.h"

#ifdef DEMO
#include "win_runtime32demo.dll.inc"
#include "win_runtime64demo.dll.inc"
#include "win_runtime32demo.sys.inc"
#include "win_runtime64demo.sys.inc"
#else
#include "win_runtime32.dll.inc"
#include "win_runtime64.dll.inc"
#include "win_runtime32.sys.inc"
#include "win_runtime64.sys.inc"
#endif

#include "dotnet20_runtime32.dll.inc"
#include "dotnet20_runtime64.dll.inc"
#include "dotnet40_runtime32.dll.inc"
#include "dotnet40_runtime64.dll.inc"
#include "netstandard_runtime32.dll.inc"
#include "netstandard_runtime64.dll.inc"
#include "netcore_runtime32.dll.inc"
#include "netcore_runtime64.dll.inc"

/**
 * PESegment
 */

PESegment::PESegment(PESegmentList *owner)
	: BaseSection(owner), address_(0), size_(0), physical_offset_(0), physical_size_(0), flags_(0)
{

}

PESegment::PESegment(PESegmentList *owner, uint64_t address, uint32_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t flags, const std::string &name)
	: BaseSection(owner), name_(name), address_(address), size_(size), physical_offset_(physical_offset), physical_size_(physical_size), flags_(flags)
{

}

PESegment::PESegment(PESegmentList *owner, const PESegment &src)
	: BaseSection(owner, src) 
{
	address_ = src.address_;
	size_ = src.size_;
	physical_offset_ = src.physical_offset_;
	physical_size_ = src.physical_size_;
	flags_ = src.flags_;
	name_ = src.name_;
}

PESegment *PESegment::Clone(ISectionList *owner) const
{
	PESegment *section = new PESegment(reinterpret_cast<PESegmentList *>(owner), *this);
	return section;
}

void PESegment::ReadFromFile(PEArchitecture &file)
{
	IMAGE_SECTION_HEADER section_header;

	file.Read(&section_header, sizeof(section_header));
	name_ = std::string(reinterpret_cast<char *>(&section_header.Name), strnlen(reinterpret_cast<char *>(&section_header.Name), sizeof(section_header.Name)));
	size_ = section_header.Misc.VirtualSize;
	address_ = section_header.VirtualAddress + file.image_base();
	physical_offset_ = section_header.PointerToRawData;
	physical_size_ = section_header.SizeOfRawData;
	flags_ = section_header.Characteristics;
}

void PESegment::WriteToFile(PEArchitecture &file) const
{
	IMAGE_SECTION_HEADER section_header = IMAGE_SECTION_HEADER();

	memcpy(section_header.Name, name_.c_str(), std::min(name_.size(), sizeof(section_header.Name)));
	section_header.Misc.VirtualSize = size_;
	section_header.VirtualAddress = static_cast<uint32_t>(address_ - file.image_base());
	section_header.PointerToRawData = (physical_size_) ? physical_offset_ : 0;
	section_header.SizeOfRawData = physical_size_;
	section_header.Characteristics = flags_;

	file.Write(&section_header, sizeof(section_header));
}

uint32_t PESegment::memory_type() const
{
	uint32_t res = mtNone;
	if (flags_ & IMAGE_SCN_MEM_READ)
		res |= mtReadable;
	if (flags_ & IMAGE_SCN_MEM_WRITE)
		res |= mtWritable;
	if (flags_ & IMAGE_SCN_MEM_EXECUTE)
		res |= mtExecutable;
	if (flags_ & IMAGE_SCN_MEM_DISCARDABLE)
		res |= mtDiscardable;
	if (flags_ & IMAGE_SCN_MEM_NOT_PAGED)
		res |= mtNotPaged;
	if (flags_ & IMAGE_SCN_MEM_SHARED)
		res |= mtShared;
	return res;
}

void PESegment::update_type(uint32_t mt)
{
	if (mt & mtReadable) {
		flags_ |= IMAGE_SCN_MEM_READ;
		if ((mt & mtExecutable) == 0)
			flags_ |= IMAGE_SCN_CNT_INITIALIZED_DATA;
	}
	if (mt & mtWritable)
		flags_ |= IMAGE_SCN_MEM_WRITE;
	if (mt & mtExecutable)
		flags_ |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
	if ((mt & (mtDiscardable | mtNotDiscardable)) == mtDiscardable)
		flags_ |= IMAGE_SCN_MEM_DISCARDABLE;
	else
		flags_ &= ~IMAGE_SCN_MEM_DISCARDABLE;
	if (mt & mtNotPaged)
		flags_ |= IMAGE_SCN_MEM_NOT_PAGED;
	if (mt & mtShared)
		flags_ |= IMAGE_SCN_MEM_SHARED;
}

void PESegment::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * PESegmentList
 */

PESegmentList::PESegmentList(PEArchitecture *owner)
	: BaseSectionList(owner), header_segment_(NULL)
{

}

PESegmentList::PESegmentList(PEArchitecture *owner, const PESegmentList &src)
	: BaseSectionList(owner, src), header_segment_(NULL)
{
	if (src.header_segment_)
		header_segment_ = src.header_segment_->Clone(NULL);
}

PESegmentList::~PESegmentList()
{
	delete header_segment_;
}

PESegmentList *PESegmentList::Clone(PEArchitecture *owner) const
{
	PESegmentList *section_list = new PESegmentList(owner, *this);
	return section_list;
}

PESegment *PESegmentList::Add()
{
	PESegment *section = new PESegment(this);
	AddObject(section);
	return section;
}

PESegment *PESegmentList::Add(uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t physical_size, uint32_t flags, const std::string &name)
{
	PESegment *section = new PESegment(this, address, size, physical_offset, physical_size, flags, name);
	AddObject(section);
	return section;
}

PESegment *PESegmentList::item(size_t index) const
{ 
	return reinterpret_cast<PESegment *>(BaseSectionList::item(index));
}

PESegment *PESegmentList::GetSectionByAddress(uint64_t address) const
{
	PESegment *res = reinterpret_cast<PESegment *>(BaseSectionList::GetSectionByAddress(address));
	if (!res && header_segment_ && address >= header_segment_->address() && address < header_segment_->address() + header_segment_->size())
		res = header_segment_;
	return res;
}

PESegment *PESegmentList::last() const
{
	return reinterpret_cast<PESegment *>(BaseSectionList::last());
}

void PESegmentList::ReadFromFile(PEArchitecture &file, uint32_t count)
{
	Reserve(count);
	for (size_t i = 0; i < count; i++) {
		Add()->ReadFromFile(file);
	}
	if (header_segment_) {
		delete header_segment_;
		header_segment_ = NULL;
	}
	if (this->count()) {
		PESegment *first_segment = item(0);
		header_segment_ = new PESegment(NULL, file.image_base(), static_cast<uint32_t>(first_segment->address() - file.image_base()), 0, first_segment->physical_offset(), 0, ".header");
	}
}

void PESegmentList::WriteToFile(PEArchitecture &file) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

/**
 * PESection
 */

PESection::PESection(PESectionList *owner, PESegment *parent, uint64_t address, uint64_t size, const std::string &name)
	: BaseSection(owner), name_(name), address_(address), size_(size), parent_(parent)
{

}

PESection::PESection(PESectionList *owner, const PESection &src)
	: BaseSection(owner, src) 
{
	address_ = src.address_;
	size_ = src.size_;
	name_ = src.name_;
	parent_ = src.parent_;
}

PESection *PESection::Clone(ISectionList *owner) const
{
	PESection *section = new PESection(reinterpret_cast<PESectionList *>(owner), *this);
	return section;
}

void PESection::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
 * PESectionList
 */

PESectionList::PESectionList(PEArchitecture *owner)
	: BaseSectionList(owner)
{

}

PESectionList::PESectionList(PEArchitecture *owner, const PESectionList &src)
	: BaseSectionList(owner, src)
{

}

PESectionList *PESectionList::Clone(PEArchitecture *owner) const
{
	PESectionList *section_list = new PESectionList(owner, *this);
	return section_list;
}

PESection *PESectionList::item(size_t index) const
{ 
	return reinterpret_cast<PESection *>(BaseSectionList::item(index));
}

PESection *PESectionList::Add(PESegment *parent, uint64_t address, uint64_t size, const std::string &name)
{
	PESection *section = new PESection(this, parent, address, size, name);
	AddObject(section);
	return section;
}

/**
 * PEDirectory
 */

PEDirectory::PEDirectory(PEDirectoryList *owner, uint32_t type)
	: BaseLoadCommand(owner), address_(0), size_(0), type_(type), physical_size_(0)
{

}

PEDirectory::PEDirectory(PEDirectoryList *owner, const PEDirectory &src)
	: BaseLoadCommand(owner, src)
{
	address_ = src.address_;
	size_ = src.size_;
	type_ = src.type_;
	physical_size_ = src.physical_size_;
}

PEDirectory *PEDirectory::Clone(ILoadCommandList *owner) const
{
	PEDirectory *dir = new PEDirectory(reinterpret_cast<PEDirectoryList *>(owner), *this);
	return dir;
}

void PEDirectory::clear()
{
	address_ = 0;
	size_ = 0;
	physical_size_ = 0;
}

void PEDirectory::ReadFromFile(PEArchitecture &file)
{
	IMAGE_DATA_DIRECTORY dir;

	file.Read(&dir, sizeof(IMAGE_DATA_DIRECTORY));

	address_ = (dir.VirtualAddress == 0) ? 0 : dir.VirtualAddress + file.image_base();
	size_ = dir.Size;
}

void PEDirectory::WriteToFile(PEArchitecture &file) const
{
	IMAGE_DATA_DIRECTORY dir;

	dir.VirtualAddress = address_ ? static_cast<uint32_t>(address_ - file.image_base()) : 0;
	dir.Size = size_;

	file.Write(&dir, sizeof(IMAGE_DATA_DIRECTORY));
}

std::string PEDirectory::name() const
{
	switch (type_) {
	case IMAGE_DIRECTORY_ENTRY_EXPORT:
		return std::string("Export");
	case IMAGE_DIRECTORY_ENTRY_IMPORT:
		return std::string("Import");
	case IMAGE_DIRECTORY_ENTRY_RESOURCE:
		return std::string("Resource");
	case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
		return std::string("Exception");
	case IMAGE_DIRECTORY_ENTRY_SECURITY:
		return std::string("Security");
	case IMAGE_DIRECTORY_ENTRY_BASERELOC:
		return std::string("Relocation");
	case IMAGE_DIRECTORY_ENTRY_DEBUG:
		return std::string("Debug");
	case IMAGE_DIRECTORY_ENTRY_ARCHITECTURE:
		return std::string("Architecture");
	case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
		return std::string("Reserved");
	case IMAGE_DIRECTORY_ENTRY_TLS:
		return std::string("Thread Local Storage");
	case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
		return std::string("Configuration");
	case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
		return std::string("Bound Import");
	case IMAGE_DIRECTORY_ENTRY_IAT:
		return std::string("Import Address Table");
	case IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
		return std::string("Delay Import");
	case IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR:
		return std::string(".NET MetaData");
	}

	return BaseLoadCommand::name();
}

void PEDirectory::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
}

void PEDirectory::FreeByManager(MemoryManager &manager)
{
	if (!address() || !physical_size())
		return;

	size_t size = physical_size();
	manager.Add(address_, size);
	IArchitecture *file = manager.owner();
	for (size_t i = 0; i < size; i++) {
		IFixup *fixup = file->fixup_list()->GetFixupByAddress(address_ + i);
		if (fixup)
			fixup->set_deleted(true);
	}
}

/**
 * PEDirectoryList
 */

PEDirectoryList::PEDirectoryList(PEArchitecture *owner)
	: BaseCommandList(owner)
{

}

PEDirectoryList::PEDirectoryList(PEArchitecture *owner, const PEDirectoryList &src)
	: BaseCommandList(owner, src)
{

}

PEDirectory *PEDirectoryList::item(size_t index) const 
{ 
	return reinterpret_cast<PEDirectory *>(BaseCommandList::item(index));
}

PEDirectoryList *PEDirectoryList::Clone(PEArchitecture *owner) const
{
	PEDirectoryList *directory_list = new PEDirectoryList(owner, *this);
	return directory_list;
}

PEDirectory *PEDirectoryList::Add(uint32_t type)
{
	PEDirectory *dir = new PEDirectory(this, type);
	AddObject(dir);
	return dir;
}

PEDirectory *PEDirectoryList::GetCommandByType(uint32_t type) const
{
	return reinterpret_cast<PEDirectory *>(BaseCommandList::GetCommandByType(type));
}

PEDirectory *PEDirectoryList::GetCommandByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		PEDirectory *dir = item(i);
		if (dir->address() == address)
			return dir;
	}

	return NULL;
}

void PEDirectoryList::ReadFromFile(PEArchitecture &file, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		Add(i)->ReadFromFile(file);
	}
}

void PEDirectoryList::WriteToFile(PEArchitecture &file) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

/**
 * PEImportFunction
 */

PEImportFunction::PEImportFunction(PEImport *owner)
	: BaseImportFunction(owner), name_address_(0), address_(0), is_ordinal_(false), ordinal_(0)
{

}

PEImportFunction::PEImportFunction(PEImport *owner, const std::string &name)
	: BaseImportFunction(owner), name_(name), name_address_(0), address_(0), is_ordinal_(false), ordinal_(0)
{

}

PEImportFunction::PEImportFunction(PEImport *owner, uint64_t address, APIType type, MapFunction *map_function)
	: BaseImportFunction(owner), name_address_(0), address_(address), ordinal_(0), is_ordinal_(false)
{
	set_type(type);
	set_map_function(map_function);
}

PEImportFunction::PEImportFunction(PEImport *owner, const PEImportFunction &src)
	: BaseImportFunction(owner, src)
{
	name_ = src.name_;
	name_address_ = src.name_address_;
	address_ = src.address_;
	is_ordinal_ = src.is_ordinal_;
	ordinal_ = src.ordinal_;
}

PEImportFunction *PEImportFunction::Clone(IImport *owner) const
{
	PEImportFunction *func = new PEImportFunction(reinterpret_cast<PEImport *>(owner), *this);
	return func;
}

bool PEImportFunction::ReadFromFile(PEArchitecture &file, uint32_t &rva)
{
	address_ = rva + file.image_base();
	if (file.cpu_address_size() == osDWord) {
		IMAGE_THUNK_DATA32 thunk;

		file.Read(&thunk, sizeof(thunk));
		name_address_ = thunk.u1.AddressOfData;
		if (!name_address_)
			return false;
		is_ordinal_ = IMAGE_SNAP_BY_ORDINAL32(name_address_);
		rva += sizeof(uint32_t);
	} else {
		IMAGE_THUNK_DATA64 thunk;

		file.Read(&thunk, sizeof(thunk));
		name_address_ = thunk.u1.AddressOfData;
		if (!name_address_)
			return false;
		is_ordinal_ = IMAGE_SNAP_BY_ORDINAL64(name_address_);
		rva += sizeof(uint64_t);
	}

	if (is_ordinal_) {
		ordinal_ = IMAGE_ORDINAL32(name_address_);
		name_address_ = 0;
		name_ = string_format("Ordinal: %.4X", ordinal_);
	} else {
		name_address_ += file.image_base();
		uint64_t pos = file.Tell();
		if (!file.AddressSeek(name_address_ + sizeof(WORD)))
			throw std::runtime_error("Format error");
		name_ = file.ReadString();
		file.Seek(pos);
	}

	return true;
}

void PEImportFunction::FreeByManager(MemoryManager &manager, bool free_iat)
{
	if (name_address_)
		manager.Add(name_address_, sizeof(uint16_t) + name_.size() + 1);

	if (address_ && free_iat && (options() & (ioHasDataReference | ioNoReferences)) == 0)
		manager.Add(address_, OperandSizeToValue(manager.owner()->cpu_address_size()));
}

void PEImportFunction::Rebase(uint64_t delta_base)
{
	if (name_address_)
		name_address_ += delta_base;
	if (address_)
		address_ += delta_base;
}

bool PEImportFunction::IsInternal(const CompileContext &ctx) const
{
	if ((options() & ioFromRuntime) == 0) {
		if (ctx.options.flags & cpResourceProtection) {
			if (type() >= atLoadResource && type() <= atEnumResourceTypesW)
				return true;
		}
	}
	return false;
}

std::string PEImportFunction::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

/**
 * PEImport
 */

PEImport::PEImport(PEImportList *owner)
	: BaseImport(owner), name_address_(0), is_sdk_(false), original_first_thunk_address_(0), first_thunk_address_(0), time_stamp_(0), forwarder_chain_(0)
{

}

PEImport::PEImport(PEImportList *owner, bool is_sdk)
	: BaseImport(owner), name_address_(0), is_sdk_(is_sdk), original_first_thunk_address_(0), first_thunk_address_(0), time_stamp_(0), forwarder_chain_(0)
{

}

PEImport::PEImport(PEImportList *owner, const std::string &name)
	: BaseImport(owner), name_(name), name_address_(0), is_sdk_(false), original_first_thunk_address_(0), first_thunk_address_(0), time_stamp_(0), forwarder_chain_(0)
{

}

PEImport::PEImport(PEImportList *owner, const PEImport &src)
	: BaseImport(owner, src)
{
	name_ = src.name_;
	is_sdk_ = src.is_sdk_;
	name_address_ = src.name_address_;
	original_first_thunk_address_ = src.original_first_thunk_address_;
	first_thunk_address_ = src.first_thunk_address_;
	time_stamp_ = src.time_stamp_;
	forwarder_chain_ = src.forwarder_chain_;
}

PEImport *PEImport::Clone(IImportList *owner) const
{
	PEImport *import = new PEImport(reinterpret_cast<PEImportList *>(owner), *this);
	return import;
}

PEImportFunction *PEImport::item(size_t index) const 
{ 
	return reinterpret_cast<PEImportFunction *>(IImport::item(index));
}

bool PEImport::ReadFromFile(PEArchitecture &file)
{
	static const ImportInfo kernel32_info[] = {
		{atLoadResource, "LoadResource", ioNone, ctNone},
		{atFindResourceA, "FindResourceA", ioNone, ctNone},
		{atFindResourceExA, "FindResourceExA", ioNone, ctNone},
		{atFindResourceW, "FindResourceW", ioNone, ctNone},
		{atFindResourceExW, "FindResourceExW", ioNone, ctNone},
		{atEnumResourceNamesA, "EnumResourceNamesA", ioNone, ctNone},
		{atEnumResourceNamesW, "EnumResourceNamesW", ioNone, ctNone},
		{atEnumResourceLanguagesA, "EnumResourceLanguagesA", ioNone, ctNone},
		{atEnumResourceLanguagesW, "EnumResourceLanguagesW", ioNone, ctNone},
		{atEnumResourceTypesA, "EnumResourceTypesA", ioNone, ctNone},
		{atEnumResourceTypesW, "EnumResourceTypesW", ioNone, ctNone},
		{atNone, "ExitProcess", ioNoReturn, ctNone},
		{atNone, "ExitThread", ioNoReturn, ctNone},
		{atNone, "FreeLibraryAndExitThread", ioNoReturn, ctNone},
		{atNone, "GetVersion", ioNative, ctNone},
		{atNone, "GetVersionExA", ioNative, ctNone},
		{atNone, "GetVersionExW", ioNative, ctNone}
	};

	static const ImportInfo user32_info[] = {
		{atLoadStringA, "LoadStringA", ioNone, ctNone},
		{atLoadStringW, "LoadStringW", ioNone, ctNone}
	};

	static const ImportInfo msvbvm_info[] = {
		{atNone, "__vbaError", ioNoReturn, ctNone},
		{atNone, "__vbaErrorOverflow", ioNoReturn, ctNone},
		{atNone, "__vbaStopExe", ioNoReturn, ctNone},
		{atNone, "__vbaFailedFriend", ioNoReturn, ctNone},
		{atNone, "__vbaEnd", ioNoReturn, ctNone},
		{atNone, "__vbaFPException", ioNoReturn, ctNone},
		{atNone, "__vbaGenerateBoundsError", ioNoReturn, ctNone},
		{atNone, "Ordinal: 0064", ioNoReturn, ctNone},
	};

	static const ImportInfo default_info[] = {
		{atNone, "@System@@Halt0$qqrv", ioNoReturn, ctNone},
		{atNone, "exit", ioNoReturn, ctNone},
		{atNone, "abort", ioNoReturn, ctNone},
		{atNone, "?terminate@@YAXXZ", ioNoReturn, ctNone},
		{atNone, "?unexpected@@YAXXZ", ioNoReturn, ctNone},
		{atNone, "__std_terminate", ioNoReturn, ctNone},
		{atNone, "?_Xout_of_range@std@@YAXPEBD@Z", ioNoReturn, ctNone},
		{atNone, "?_Xlength_error@std@@YAXPEBD@Z", ioNoReturn, ctNone},
		{atNone, "_CxxThrowException", ioNoReturn, ctNone},
	};

	IMAGE_IMPORT_DESCRIPTOR import_descriptor;
	uint64_t pos;
	PEImportFunction *func;
	size_t i, j;
	std::string dll_name;
	
	file.Read(&import_descriptor, sizeof(import_descriptor));
	if (!import_descriptor.FirstThunk)
		return false;

	pos = file.Tell();
	name_address_ = import_descriptor.Name + file.image_base();
	if (!file.AddressSeek(name_address_))
		throw std::runtime_error("Format error");

	name_ = file.ReadString();

	original_first_thunk_address_ = import_descriptor.u.OriginalFirstThunk;
	if (original_first_thunk_address_)
		original_first_thunk_address_ += file.image_base();

	first_thunk_address_ = import_descriptor.FirstThunk;
	if (first_thunk_address_)
		first_thunk_address_ += file.image_base();

	if (!file.AddressSeek((original_first_thunk_address_ != 0) ? original_first_thunk_address_ : first_thunk_address_))
		throw std::runtime_error("Format error");

	time_stamp_ = import_descriptor.TimeDateStamp;
	forwarder_chain_ = import_descriptor.ForwarderChain;
	uint32_t rva = import_descriptor.FirstThunk;
	while (true) {
		func = new PEImportFunction(this);
		if (!func->ReadFromFile(file, rva)) {
			delete func;
			break;
		}
		AddObject(func);
	}

	file.Seek(pos);

	dll_name = name_;
	std::transform(dll_name.begin(), dll_name.end(), dll_name.begin(), tolower);
	std::string sdk_name;
	if (file.image_type() == itDriver) {
		if (dll_name.find('.') == (size_t)-1)
			dll_name += ".sys";
		sdk_name = string_format("vmprotectddk%d.sys", (file.cpu_address_size() == osDWord) ? 32 : 64);
	} else {
		if (dll_name.find('.') == (size_t)-1)
			dll_name += ".dll";
		sdk_name = string_format("vmprotectsdk%d.dll", (file.cpu_address_size() == osDWord) ? 32 : 64);
	}

	if (dll_name.compare(sdk_name) == 0) {
		is_sdk_ = true;
		for (i = 0; i < count(); i++) {
			func = item(i);
			const ImportInfo *import_info = owner()->GetSDKInfo(func->name());
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
		size_t c;
		const ImportInfo *import_info;
		if (dll_name.compare("kernel32.dll") == 0) {
			import_info = kernel32_info;
			c = _countof(kernel32_info);
		} else if (dll_name.compare("user32.dll") == 0) {
			import_info = user32_info;
			c = _countof(user32_info);
		} else if (dll_name.compare("msvbvm50.dll") == 0 || dll_name.compare("msvbvm60.dll") == 0) {
			import_info = msvbvm_info;
			c = _countof(msvbvm_info);
		} else {
			import_info = default_info;
			c = _countof(default_info);
		}

		if (import_info) {
			for (i = 0; i < count(); i++) {
				func = item(i);
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

	return true;
}

bool PEImport::FreeByManager(MemoryManager &manager, bool free_iat)
{
	if (!name_address_)
		return false;

	manager.Add(name_address_, name_.size() + 1);

	for (size_t i = 0; i < count(); i++) {
		item(i)->FreeByManager(manager, free_iat);
	}

	if (original_first_thunk_address_ && original_first_thunk_address_ != first_thunk_address_)
		manager.Add(original_first_thunk_address_, count() * OperandSizeToValue(manager.owner()->cpu_address_size()));

	return true;
}

void PEImport::Rebase(uint64_t delta_base)
{
	BaseImport::Rebase(delta_base);

	if (name_address_)
		name_address_ += delta_base;
	if (original_first_thunk_address_)
		original_first_thunk_address_ += delta_base;
	if (first_thunk_address_)
		first_thunk_address_ += delta_base;
}

PEImportFunction *PEImport::Add(uint64_t address, APIType type, MapFunction *map_function)
{
	PEImportFunction *import_function = new PEImportFunction(this, address, type, map_function);
	AddObject(import_function);
	return import_function;
}

void PEImport::WriteToFile(PEArchitecture &file) const
{
	IMAGE_IMPORT_DESCRIPTOR import_descriptor;

	import_descriptor.u.OriginalFirstThunk = original_first_thunk_address_ ? static_cast<uint32_t>(original_first_thunk_address_ - file.image_base()) : 0;
	import_descriptor.TimeDateStamp = time_stamp_;
	import_descriptor.ForwarderChain = forwarder_chain_;
	import_descriptor.Name = static_cast<uint32_t>(name_address_ - file.image_base());
	import_descriptor.FirstThunk = first_thunk_address_ ? static_cast<uint32_t>(first_thunk_address_ - file.image_base()) : 0;
	file.Write(&import_descriptor, sizeof(import_descriptor));
}

/**
 * PEImportList
 */

PEImportList::PEImportList(PEArchitecture *owner)
	: BaseImportList(owner), address_(0)
{

}

PEImportList::PEImportList(PEArchitecture *owner, const PEImportList &src)
	: BaseImportList(owner, src) 
{
	address_ = src.address_;
}

PEImportList *PEImportList::Clone(PEArchitecture *owner) const
{
	PEImportList *import_list = new PEImportList(owner, *this);
	return import_list;
}

PEImport *PEImportList::item(size_t index) const 
{ 
	return reinterpret_cast<PEImport *>(BaseImportList::item(index));
}

PEImportFunction *PEImportList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<PEImportFunction*>(BaseImportList::GetFunctionByAddress(address));
}

void PEImportList::ReadFromFile(PEArchitecture &file, PEDirectory &dir)
{
	if (!dir.address())
		return;

	address_ = dir.address();
	if (!file.AddressSeek(address_))
		throw std::runtime_error("Format error");

	while (true) {
		PEImport *imp = new PEImport(this);
		if (!imp->ReadFromFile(file)) {
			delete imp;
			break;
		}
		AddObject(imp);
	}
}

void PEImportList::WriteToFile(PEArchitecture &file, bool skip_sdk) const
{
	PEDirectory *dir = file.command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (!dir)
		return;

	if (!file.AddressSeek(dir->address()))
		return;

	for (size_t i = 0; i < count(); i++) {
		PEImport *import = item(i);
		if (skip_sdk && import->is_sdk())
			continue;

		import->WriteToFile(file);
	}

	IMAGE_IMPORT_DESCRIPTOR import_descriptor = IMAGE_IMPORT_DESCRIPTOR();
	file.Write(&import_descriptor, sizeof(import_descriptor));
}

void PEImportList::FreeByManager(MemoryManager &manager, bool free_iat)
{
	if (!address_)
		return;

	size_t c = 0;
	for (size_t i = 0; i < count(); i++) {
		if (item(i)->FreeByManager(manager, free_iat))
			c++;
	}

	manager.Add(address_, c * sizeof(IMAGE_IMPORT_DESCRIPTOR));
}

void PEImportList::Rebase(uint64_t delta_base)
{
	if (!address_)
		return;

	BaseImportList::Rebase(delta_base);

	address_ += delta_base;
}

PEImport *PEImportList::AddSDK()
{
	PEImport *sdk = new PEImport(this, true);
	AddObject(sdk);
	return sdk;
}

/**
 * PEDelayImportFunction
 */

PEDelayImportFunction::PEDelayImportFunction(PEDelayImport *owner)
	: IObject(), owner_(owner), is_ordinal_(false), ordinal_(0)
{

}

PEDelayImportFunction::PEDelayImportFunction(PEDelayImport *owner, const PEDelayImportFunction &src)
	: IObject(), owner_(owner)
{
	name_ = src.name_;
	is_ordinal_ = src.is_ordinal_;
	ordinal_ = src.ordinal_;
}

PEDelayImportFunction::~PEDelayImportFunction()
{
	if (owner_)
		owner_->RemoveObject(this);
}

PEDelayImportFunction *PEDelayImportFunction::Clone(PEDelayImport *owner) const
{
	PEDelayImportFunction *func = new PEDelayImportFunction(owner, *this);
	return func;
}

bool PEDelayImportFunction::ReadFromFile(PEArchitecture &file, uint64_t add_value)
{
	uint64_t name_address;
	if (file.cpu_address_size() == osDWord) {
		IMAGE_THUNK_DATA32 thunk;

		file.Read(&thunk, sizeof(thunk));
		name_address = thunk.u1.AddressOfData;
		if (!name_address)
			return false;
		is_ordinal_ = IMAGE_SNAP_BY_ORDINAL32(name_address);
	} else {
		IMAGE_THUNK_DATA64 thunk;

		file.Read(&thunk, sizeof(thunk));
		name_address = thunk.u1.AddressOfData;
		if (!name_address)
			return false;
		is_ordinal_ = IMAGE_SNAP_BY_ORDINAL64(name_address);
	}

	if (is_ordinal_) {
		ordinal_ = IMAGE_ORDINAL32(name_address);
		name_address = 0;
		name_ = string_format("Ordinal: %.4X", ordinal_);
	} else {
		name_address += add_value;
		uint64_t pos = file.Tell();
		if (!file.AddressSeek(name_address + sizeof(uint16_t)))
			throw std::runtime_error("Format error");
		name_ = file.ReadString();
		file.Seek(pos);
	}

	return true;
}

/**
 * PEDelayImport
 */

PEDelayImport::PEDelayImport(PEDelayImportList *owner)
	: ObjectList<PEDelayImportFunction>(), owner_(owner)
{

}

PEDelayImport::PEDelayImport(PEDelayImportList *owner, const PEDelayImport &src)
	: ObjectList<PEDelayImportFunction>(), owner_(owner)
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
	name_ = src.name_;
	flags_ = src.flags_;
	module_ = src.module_;
	iat_ = src.iat_;
	bound_iat_ = src.bound_iat_;
	unload_iat_ = src.unload_iat_;
	time_stamp_ = src.time_stamp_;
}

PEDelayImport::~PEDelayImport()
{
	if (owner_)
		owner_->RemoveObject(this);
}

PEDelayImport *PEDelayImport::Clone(PEDelayImportList *owner) const
{
	PEDelayImport *imp = new PEDelayImport(owner, *this);
	return imp;
}

bool PEDelayImport::ReadFromFile(PEArchitecture &file)
{
	IMAGE_DELAY_IMPORT_DESCRIPTOR import_descriptor;
	file.Read(&import_descriptor, sizeof(import_descriptor));

	if (!import_descriptor.DllName)
		return false;

	flags_ = import_descriptor.Attrs;
	uint64_t name_address = import_descriptor.DllName;
	module_ = import_descriptor.Hmod;
	iat_ = import_descriptor.IAT;
	uint64_t address = import_descriptor.INT;
	bound_iat_ = import_descriptor.BoundIAT;
	unload_iat_ = import_descriptor.UnloadIAT;
	time_stamp_ = import_descriptor.TimeStamp;

	uint64_t add_value;
	if (flags_ & 1) {
		add_value = file.image_base();
		if (name_address)
			name_address += add_value;
		if (module_)
			module_ += add_value;
		if (iat_)
			iat_ += add_value;
		if (address)
			address += add_value;
		if (bound_iat_)
			bound_iat_ += add_value;
		if (unload_iat_)
			unload_iat_ += add_value;
	} else {
		if (file.cpu_address_size() != osDWord)
			throw std::runtime_error("Format error");
		add_value = 0;
	}

	uint64_t pos = file.Tell();
	if (!file.AddressSeek(name_address))
		throw std::runtime_error("Format error");
	name_ = file.ReadString();

	if (!file.AddressSeek(address))
		throw std::runtime_error("Format error");
	while (true) {
		PEDelayImportFunction *func = new PEDelayImportFunction(this);
		if (!func->ReadFromFile(file, add_value)) {
			delete func;
			break;
		}
		AddObject(func);
	}

	file.Seek(pos);

	return true;
}

/**
 * PEDelayImportList
 */

PEDelayImportList::PEDelayImportList()
	: ObjectList<PEDelayImport>()
{

}

PEDelayImportList::PEDelayImportList(const PEDelayImportList &src)
	: ObjectList<PEDelayImport>() 
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

PEDelayImportList *PEDelayImportList::Clone() const
{
	PEDelayImportList *import_list = new PEDelayImportList(*this);
	return import_list;
}

void PEDelayImportList::ReadFromFile(PEArchitecture &file, PEDirectory &dir)
{
	if (!dir.address())
		return;

	if (!file.AddressSeek(dir.address()))
		throw std::runtime_error("Format error");

	for (size_t i = 0; i < dir.size(); i += sizeof(IMAGE_DELAY_IMPORT_DESCRIPTOR)) {
		PEDelayImport *imp = new PEDelayImport(this);
		if (!imp->ReadFromFile(file)) {
			delete imp;
			break;
		}
		AddObject(imp);
	}
}

/**
 * PEExport
 */

PEExport::PEExport(PEExportList *owner, uint64_t address, uint32_t ordinal)
	: BaseExport(owner), address_(address), ordinal_(ordinal), address_of_name_(0)
{

}

PEExport::PEExport(PEExportList *owner, const PEExport &src)
	: BaseExport(owner, src)
{
	address_ = src.address_;
	ordinal_ = src.ordinal_;
	name_ = src.name_;
	forwarded_name_ = src.forwarded_name_;
	address_of_name_ = src.address_of_name_;
}

PEExport *PEExport::Clone(IExportList *owner) const
{
	PEExport *exp = new PEExport(reinterpret_cast<PEExportList *>(owner), *this);
	return exp;
}

int PEExport::CompareWith(const IObject &obj) const
{
	const PEExport &exp = reinterpret_cast<const PEExport &>(obj);
	if (ordinal() < exp.ordinal())
		return -1;
	if (ordinal() > exp.ordinal())
		return 1;
	return 0;
}

void PEExport::ReadFromFile(PEArchitecture &file, uint64_t address_of_name, bool is_forwarded)
{
	address_of_name_  = address_of_name;
	if (address_of_name_) {
		if (!file.AddressSeek(address_of_name_))
			throw std::runtime_error("Format error");
		name_ = file.ReadString();
	}

	if (is_forwarded) {
		if (!file.AddressSeek(address_))
			throw std::runtime_error("Format error");
		forwarded_name_ = file.ReadString();
	}
}

void PEExport::FreeByManager(MemoryManager &manager)
{
	if (!forwarded_name_.empty())
		manager.Add(address_, forwarded_name_.size() + 1);

	if (address_of_name_)
		manager.Add(address_of_name_, name_.size() + 1);
}

void PEExport::Rebase(uint64_t delta_base)
{
	if (address_)
		address_ += delta_base;
	if (address_of_name_)
		address_of_name_ += delta_base;
}

std::string PEExport::display_name(bool show_ret) const
{
	return DemangleName(name_).display_name(show_ret);
}

/**
 * PEExportList
 */

PEExportList::PEExportList(PEArchitecture *owner)
	: BaseExportList(owner), address_(0), name_address_(0), characteristics_(0), time_date_stamp_(0), major_version_(0), minor_version_(0),
	number_of_functions_(0), address_of_functions_(0), number_of_names_(0), address_of_names_(0), address_of_name_ordinals_(0)
{

}

PEExportList::PEExportList(PEArchitecture *owner, const PEExportList &src)
	: BaseExportList(owner, src)
{
	address_ = src.address_;
	name_address_ = src.name_address_;
	characteristics_ = src.characteristics_;
	time_date_stamp_ = src.time_date_stamp_;
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	name_ = src.name_;
	number_of_functions_ = src.number_of_functions_;
	address_of_functions_ = src.address_of_functions_;
	number_of_names_ = src.number_of_names_;
	address_of_names_ = src.address_of_names_;
	address_of_name_ordinals_ = src.address_of_name_ordinals_;
}

PEExportList *PEExportList::Clone(PEArchitecture *owner) const
{
	PEExportList *export_list = new PEExportList(owner, *this);
	return export_list;
}

PEExport *PEExportList::item(size_t index) const 
{ 
	return reinterpret_cast<PEExport *>(IExportList::item(index));
}

PEExport *PEExportList::Add(uint64_t address, uint32_t ordinal)
{
	PEExport *exp = new PEExport(this, address, ordinal);
	AddObject(exp);
	return exp;
}

void PEExportList::AddAntidebug()
{
	size_t i;
	PEExport *exp;
	std::map<uint32_t, PEExport *> map;
	for (i = 0; i < count(); i++) {
		exp = item(i);
		map[exp->ordinal()] = exp;
	}
	uint32_t free_ordinal = 0;
	for (uint32_t ordinal = 1; ordinal <= 0xffff; ordinal++) {
		if (map.find(ordinal) == map.end()) {
			free_ordinal = ordinal;
			break;
		}
	}
	if (!free_ordinal)
		return;

	exp = Add(0, free_ordinal);
	std::string name;
	name.resize(3100);
	for (i = 0; i < name.size(); i++) {
		name[i] = 1 + rand() % 0xff;
	}
	exp->set_name(name);
}

PEExport *PEExportList::GetExportByOrdinal(uint32_t ordinal)
{
	for (size_t i = 0; i < count(); i++) {
		PEExport *exp = item(i);
		if (exp->ordinal() == ordinal)
			return exp;
	}

	return NULL;
}

void PEExportList::ReadFromFile(PEArchitecture &file, PEDirectory &dir)
{
	IMAGE_EXPORT_DIRECTORY export_directory;
	uint32_t i;
	uint32_t rva;
	PEExport *export_function;
	std::vector<NameInfo> name_info_list;

	if (!dir.address())
		return;

	address_ = dir.address();
	if (!file.AddressSeek(address_))
		throw std::runtime_error("Format error");

	file.Read(&export_directory, sizeof(export_directory));
	characteristics_ =  export_directory.Characteristics;
	time_date_stamp_ =  export_directory.TimeDateStamp;
	major_version_ =  export_directory.MajorVersion;
	minor_version_ =  export_directory.MinorVersion;

	name_address_ = export_directory.Name;
	if (name_address_) {
		name_address_ += file.image_base();
		if (!file.AddressSeek(name_address_))
			throw std::runtime_error("Format error");
		name_ = file.ReadString();
	}

	number_of_functions_ = export_directory.NumberOfFunctions;
	if (number_of_functions_) {
		address_of_functions_ = export_directory.AddressOfFunctions + file.image_base();
		if (!file.AddressSeek(address_of_functions_))
			throw std::runtime_error("Format error");
		for (i = 0; i < number_of_functions_; i++) {
			rva = file.ReadDWord();
			if (rva)
				Add(rva + file.image_base(), export_directory.Base + i);
		}
	}

	number_of_names_ = export_directory.NumberOfNames;
	if (number_of_names_) {
		address_of_names_ = export_directory.AddressOfNames + file.image_base();
		if (!file.AddressSeek(address_of_names_))
			throw std::runtime_error("Format error");

		for (i = 0; i < number_of_names_; i++) {
			NameInfo name_info;
			name_info.address_of_name = file.ReadDWord();
			name_info.ordinal_index = 0;
			name_info_list.push_back(name_info);
		}

		address_of_name_ordinals_ = export_directory.AddressOfNameOrdinals + file.image_base();
		if (!file.AddressSeek(address_of_name_ordinals_))
			throw std::runtime_error("Format error");
		for (i = 0; i < number_of_names_; i++) {
			name_info_list[i].ordinal_index = export_directory.Base + file.ReadWord();
		}
	}

	for (i = 0; i < count(); i++) {
		export_function = item(i);

		std::vector<NameInfo>::iterator it = std::find(name_info_list.begin(), name_info_list.end(), export_function->ordinal());
		export_function->ReadFromFile(file, (it == name_info_list.end()) ? 0 : it->address_of_name + file.image_base(), 
										(export_function->address() >= dir.address() && export_function->address() < dir.address() + dir.size()));
	}
}

void PEExportList::FreeByManager(MemoryManager &manager)
{
	if (!address_)
		return;

	manager.Add(address_, sizeof(IMAGE_EXPORT_DIRECTORY));

	if (name_address_)
		manager.Add(name_address_, name_.size() + 1);

	if (number_of_functions_)
		manager.Add(address_of_functions_, number_of_functions_ * sizeof(uint32_t));

	if (number_of_names_) {
		manager.Add(address_of_names_, number_of_names_ * sizeof(uint32_t));
		manager.Add(address_of_name_ordinals_, number_of_names_ * sizeof(uint16_t));
	}

	for (size_t i = 0; i < count(); i++) {
		item(i)->FreeByManager(manager);
	}
}

void PEExportList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
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
		atLoadResource,
		atFindResourceA,
		atFindResourceExA,
		atFindResourceW,
		atFindResourceExW,
		atLoadStringA,
		atLoadStringW,
		atEnumResourceNamesA,
		atEnumResourceNamesW,
		atEnumResourceLanguagesA,
		atEnumResourceLanguagesW,
		atEnumResourceTypesA,
		atEnumResourceTypesW
	};

	BaseExportList::ReadFromBuffer(buffer, file);

	assert(count() == _countof(export_function_types));
	for (size_t i = 0; i < count(); i++) {
		item(i)->set_type(export_function_types[i]);
	}
}

uint32_t PEExportList::WriteToData(IFunction &data, uint64_t image_base)
{
	if (!count())
		return 0;

	IntelFunction &func = reinterpret_cast<IntelFunction &>(data);

	// export functions must be sorted by ordinals
	Sort();
	size_t start_index = func.count();
	uint32_t ordinal_base = item(0)->ordinal();

	func.AddCommand(osDWord, characteristics_); 
	func.AddCommand(osDWord, time_date_stamp_); 
	func.AddCommand(osWord, major_version_); 
	func.AddCommand(osWord, minor_version_);

	IntelCommand *name_command = func.AddCommand(osDWord, 0);

	func.AddCommand(osDWord, ordinal_base); 

	IntelCommand *functions_count = func.AddCommand(osDWord, 0);
	functions_count->AddLink(0, ltOffset);

	IntelCommand *name_pointers_count = func.AddCommand(osDWord, 0);
	name_pointers_count->AddLink(0, ltOffset);

	IntelCommand *address_table = func.AddCommand(osDWord, 0);
	address_table->AddLink(0, ltOffset);

	IntelCommand *name_pointers = func.AddCommand(osDWord, 0);
	name_pointers->AddLink(0, ltOffset);

	IntelCommand *ordinal_table = func.AddCommand(osDWord, 0);
	ordinal_table->AddLink(0, ltOffset);

	// create ordinals
	size_t index = func.count();
	uint32_t last_ordinal = ordinal_base;
	std::vector<ExportInfo> export_name_list;
	PEExport *export_function;
	IntelCommand *command;
	size_t i, j;
	for (i = 0; i < count(); i++) {
		export_function = item(i);
		if (!export_function->name().empty())
			export_name_list.push_back(ExportInfo(export_function));
		for (j = last_ordinal; j < export_function->ordinal(); j++) {
			func.AddCommand(osDWord, 0);
		}
		command = func.AddCommand(osDWord, export_function->address() ? export_function->address() - image_base : 0);
		if (!export_function->forwarded_name().empty())
			command->AddLink(0, ltOffset);
		last_ordinal = export_function->ordinal() + 1;
	}
	address_table->link()->set_to_command(func.item(index));
	functions_count->set_operand_value(0, func.count() - index);

	// create forwarded names
	for (i = 0; i < count(); i++) {
		export_function = item(i);
		if (export_function->forwarded_name().empty())
			continue;

		command = func.AddCommand(export_function->forwarded_name());
		func.item(index + export_function->ordinal() - ordinal_base)->link()->set_to_command(command);
	}

	if (!export_name_list.empty()) {
		// names must be sorted
		std::sort(export_name_list.begin(), export_name_list.end());

		// create ordinal table
		index = func.count();
		for (i = 0; i < export_name_list.size(); i++) {
			export_function = export_name_list[i].export_function;
			func.AddCommand(osWord, export_function->ordinal() - ordinal_base);
		}
		name_pointers_count->set_operand_value(0, export_name_list.size());
		ordinal_table->link()->set_to_command(func.item(index));

		// create names
		index = func.count();
		for (i = 0; i < export_name_list.size(); i++) {
			command = func.AddCommand(osDWord, 0);
			command->AddLink(0, ltOffset);
		}
		for (i = 0; i < export_name_list.size(); i++) {
			export_function = export_name_list[i].export_function;
			command = func.AddCommand(export_function->name());
			func.item(index + i)->link()->set_to_command(command);
		}
		name_pointers->link()->set_to_command(func.item(index));
	}

	// create DLL name
	if (!name().empty()) {
		command = func.AddCommand(name());
		name_command->AddLink(0, ltOffset, command);
	}

	command = func.item(start_index);
	command->include_option(roCreateNewBlock);
	command->set_alignment(OperandSizeToValue(func.cpu_address_size()));
	uint32_t res = 0;
	for (i = start_index; i < func.count(); i++) {
		command = func.item(i);
		if (command->link())
			command->link()->set_sub_value(image_base);

		command->CompileToNative();
		res += (uint32_t)command->dump_size();
	}
	return res;
}

/**
 * PEFixup
 */

PEFixup::PEFixup(PEFixupList *owner, uint64_t address, uint8_t type)
	: BaseFixup(owner), address_(address), type_(type)
{

}

PEFixup::PEFixup(PEFixupList *owner, const PEFixup &src)
	: BaseFixup(owner, src)
{
	address_ = src.address_;
	type_ = src.type_;
}

PEFixup *PEFixup::Clone(IFixupList *owner) const
{
	PEFixup *fixup = new PEFixup(reinterpret_cast<PEFixupList *>(owner), *this);
	return fixup;
}

FixupType PEFixup::type() const
{
	switch (type_) {
	case IMAGE_REL_BASED_HIGH:
		return ftHigh;
	case IMAGE_REL_BASED_LOW:
		return ftLow;
	case IMAGE_REL_BASED_HIGHLOW:
	case IMAGE_REL_BASED_DIR64:
		return ftHighLow;
	default:
		return ftUnknown;
	}
}

OperandSize PEFixup::size() const
{
	return type_ == IMAGE_REL_BASED_DIR64 ? osQWord : osDWord;
}

void PEFixup::Rebase(IArchitecture &file, uint64_t delta_base)
{
	if (!file.AddressSeek(address_))
		return;

	uint64_t pos = file.Tell();
	uint64_t value;
	switch (type_) {
	case IMAGE_REL_BASED_LOW:
		value = file.ReadWord();
		value += delta_base;
		file.Seek(pos);
		file.WriteWord(static_cast<uint16_t>(value));
		break;
	case IMAGE_REL_BASED_HIGH:
		value = file.ReadWord();
		value += delta_base >> 16;
		file.Seek(pos);
		file.WriteWord(static_cast<uint16_t>(value));
		break;
	case IMAGE_REL_BASED_HIGHLOW:
		value = file.ReadDWord();
		value += delta_base;
		file.Seek(pos);
		file.WriteDWord(static_cast<uint32_t>(value));
		break;
	case IMAGE_REL_BASED_DIR64:
		value = file.ReadQWord();
		value += delta_base;
		file.Seek(pos);
		file.WriteQWord(value);
		break;
	}
	address_ += delta_base;
}

/**
 * PEFixupList
 */

PEFixupList::PEFixupList()
	: BaseFixupList()
{

}

PEFixupList::PEFixupList(const PEFixupList &src)
	: BaseFixupList(src)
{

}

PEFixup *PEFixupList::item(size_t index) const
{
	return reinterpret_cast<PEFixup *>(BaseFixupList::item(index));
}

PEFixupList *PEFixupList::Clone() const
{
	PEFixupList *fixup_list = new PEFixupList(*this);
	return fixup_list;
}

PEFixup *PEFixupList::Add(uint64_t address, uint8_t type)
{
	PEFixup *fixup = new PEFixup(this, address, type);
	AddObject(fixup);
	return fixup;
}

IFixup *PEFixupList::AddDefault(OperandSize cpu_address_size, bool is_code)
{
	return Add(0, (cpu_address_size == osDWord) ? IMAGE_REL_BASED_HIGHLOW : IMAGE_REL_BASED_DIR64);
}

void PEFixupList::ReadFromFile(PEArchitecture &file, PEDirectory &dir)
{
	if (!dir.address())
		return;

	if (!file.AddressSeek(dir.address()))
		throw std::runtime_error("Invalid address of the base relocation table");

	IMAGE_BASE_RELOCATION reloc;
	for (uint32_t processed = 0; processed < dir.size(); processed += reloc.SizeOfBlock) {
		file.Read(&reloc, sizeof(reloc));
		if (reloc.SizeOfBlock == 0)
			break;

		if (reloc.SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) || (reloc.SizeOfBlock & 1) != 0)
			throw std::runtime_error("Invalid size of the base relocation block");

		size_t c = (reloc.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1;
		for (size_t i = 0; i < c; i++) {
			uint16_t type_offset = file.ReadWord();
			uint8_t type = (type_offset >> 12);
			if (type == IMAGE_REL_BASED_ABSOLUTE)
				continue;

			PEFixup *fixup = Add(reloc.VirtualAddress + file.image_base() + (type_offset & 0xfff), type);
			if (fixup->type() == ftUnknown)
				throw std::runtime_error("Invalid base relocation type");
		}
	}
}

void PEFixupList::WriteToData(Data &data, uint64_t image_base)
{
	Sort();

	size_t size_pos = 0;
	IMAGE_BASE_RELOCATION reloc = IMAGE_BASE_RELOCATION();
	uint16_t empty_offset = 0;
	for (size_t i = 0; i < count(); i++) {
		PEFixup *fixup = item(i);
		uint32_t rva = static_cast<uint32_t>(fixup->address() - image_base);
		uint32_t block_rva = rva & 0xfffff000;
		if (reloc.SizeOfBlock == 0 || block_rva != reloc.VirtualAddress) {
			if (reloc.SizeOfBlock) {
				if (reloc.SizeOfBlock & 3) {
					data.PushWord(empty_offset);
					reloc.SizeOfBlock += sizeof(empty_offset);
				}
				data.WriteDWord(size_pos, reloc.SizeOfBlock);
			}
			size_pos = data.size() + 4;
			reloc.VirtualAddress = block_rva;
			reloc.SizeOfBlock = sizeof(reloc);
			data.PushBuff(&reloc, sizeof(reloc));
			empty_offset = (static_cast<uint16_t>(rva - block_rva) & 0xf00) << 4 | IMAGE_REL_BASED_ABSOLUTE;
		}
		uint16_t type_offset = (static_cast<uint16_t>(rva - block_rva) & 0xfff) << 4 | fixup->internal_type();
		data.PushWord(type_offset);
		reloc.SizeOfBlock += sizeof(type_offset);
	}

	if (reloc.SizeOfBlock)  {
		if (reloc.SizeOfBlock & 3) {
			data.PushWord(empty_offset);
			reloc.SizeOfBlock += sizeof(empty_offset);
		}
		data.WriteDWord(size_pos, reloc.SizeOfBlock);
	}
}

size_t PEFixupList::WriteToFile(PEArchitecture &file)
{
	Sort();

	Data data;
	size_t size_pos = 0;
	IMAGE_BASE_RELOCATION reloc = IMAGE_BASE_RELOCATION();
	uint16_t empty_offset = 0;
	for (size_t i = 0; i < count(); i++) {
		PEFixup *fixup = item(i);
		uint32_t rva = static_cast<uint32_t>(fixup->address() - file.image_base());
		uint32_t block_rva = rva & 0xfffff000;
		if (reloc.SizeOfBlock == 0 || block_rva != reloc.VirtualAddress) {
			if (reloc.SizeOfBlock) {
				if (reloc.SizeOfBlock & 3) {
					data.PushWord(empty_offset);
					reloc.SizeOfBlock += sizeof(empty_offset);
				}
				data.WriteDWord(size_pos, reloc.SizeOfBlock);
			}
			size_pos = data.size() + 4;
			reloc.VirtualAddress = block_rva;
			reloc.SizeOfBlock = sizeof(reloc);
			data.PushBuff(&reloc, sizeof(reloc));
			empty_offset = IMAGE_REL_BASED_ABSOLUTE << 12 | (static_cast<uint16_t>(rva - block_rva) & 0xf00);
		}
		uint16_t type_offset = fixup->internal_type() << 12 | (static_cast<uint16_t>(rva - block_rva) & 0xfff);
		data.PushWord(type_offset);
		reloc.SizeOfBlock += sizeof(type_offset);
	}

	if (reloc.SizeOfBlock) {
		if (reloc.SizeOfBlock & 3) {
			data.PushWord(empty_offset);
			reloc.SizeOfBlock += sizeof(empty_offset);
		}
		data.WriteDWord(size_pos, reloc.SizeOfBlock);
	}

	return file.Write(data.data(), data.size());
}

/**
 * PERelocation
 */


PERelocation::PERelocation(PERelocationList *owner, uint64_t address, uint64_t source, OperandSize size, uint32_t addend)
	: BaseRelocation(owner, address, size), source_(source), addend_(addend)
{

}

PERelocation::PERelocation(PERelocationList *owner, const PERelocation &src)
	: BaseRelocation(owner, src)
{
	source_ = src.source_;
	addend_ = src.addend_;
}

PERelocation *PERelocation::Clone(IRelocationList *owner) const
{
	PERelocation *relocation = new PERelocation(reinterpret_cast<PERelocationList *>(owner), *this);
	return relocation;
}

/**
 * PERelocationList
 */

PERelocationList::PERelocationList()
	: BaseRelocationList(), address_(0), mem_address_(0)
{

}

PERelocationList::PERelocationList(const PERelocationList &src)
	: BaseRelocationList(src)
{
	address_ = src.address_;
	mem_address_ = src.mem_address_;
}

PERelocationList *PERelocationList::Clone() const
{
	PERelocationList *list = new PERelocationList(*this);
	return list;
}

PERelocation *PERelocationList::item(size_t index) const
{
	return reinterpret_cast<PERelocation *>(IRelocationList::item(index));
}

PERelocation *PERelocationList::Add(uint64_t address, uint64_t target, OperandSize size, uint32_t addend)
{
	PERelocation *relocation = new PERelocation(this, address, target, size, addend);
	AddObject(relocation);
	return relocation;
}

void PERelocationList::ParseMinGW(PEArchitecture &file, uint64_t address, uint64_t start, uint64_t end)
{
	if ((end < start) || (end - start < 8) ||
		((file.segment_list()->GetMemoryTypeByAddress(address) & mtWritable) == 0) ||
		((file.segment_list()->GetMemoryTypeByAddress(start) & mtReadable) == 0))
		return;

	struct RelocationHeader {
		uint32_t magic1;
		uint32_t magic2;
		uint32_t version;
	};

	struct RelocationV1 {
		uint32_t addend;
		uint32_t target;
	};

	struct RelocationV2 {
		uint32_t sym;
		uint32_t target;
		uint32_t flags;
	};

	file.AddressSeek(start);

	RelocationHeader header;
	header.magic1 = 0;
	header.magic2 = 0;
	header.version = 0;
	if (end - start >= sizeof(header)) {
		uint64_t pos = file.Tell();
		file.Read(&header, sizeof(header));
		if (header.magic1 == 0 && header.magic2 == 0) {
			start += sizeof(header);
		} else {
			file.Seek(pos);
			header.version = 0;
		}
	}

	address_ = address;
	switch (header.version) {
	case 0:
		while (start < end) {
			RelocationV1 item;
			file.Read(&item, sizeof(item));
			start += sizeof(item);

			Add(file.image_base() + item.target, 0, file.cpu_address_size(), item.addend);
		}
		break;
	case 1:
		while (start < end) {
			RelocationV2 item;
			file.Read(&item, sizeof(item));
			start += sizeof(item);

			OperandSize item_size;
			switch (item.flags) {
			case 8:
				item_size = osByte;
				break;
			case 16:
				item_size = osWord;
				break;
			case 32:
				item_size = osDWord;
				break;
			case 64:
				if (file.cpu_address_size() == osQWord)
					item_size = osQWord;
				else
					throw std::runtime_error("Invalid relocation flags");
				break;
			default:
				throw std::runtime_error("Invalid relocation flags");
			}

			Add(file.image_base() + item.target, file.image_base() + item.sym, item_size, 0);
		}
		break;
	default:
		return;
	}
}

void PERelocationList::ReadFromFile(PEArchitecture &file)
{
	for (size_t i = 0; i < file.compiler_function_list()->count(); i++) {
		CompilerFunction *compiler_function = file.compiler_function_list()->item(i);
		if (compiler_function->type() == cfRelocatorMinGW)
			ParseMinGW(file, compiler_function->value(0), compiler_function->value(1), compiler_function->value(2));
	}

	for (size_t i = 0; i < count(); i++) {
		PERelocation *relocation = item(i);

		if (relocation->source()) {
			IImportFunction *import_function = file.import_list()->GetFunctionByAddress(relocation->source());
			if (import_function) {
				import_function->exclude_option(ioNoReferences);
				import_function->include_option(ioHasDataReference);
			}
		}
	}
}

void PERelocationList::WriteToData(Data &data, uint64_t image_base)
{
	for (size_t i = 0; i < count(); i++) {
		PERelocation *relocation = item(i);

		data.PushDWord(static_cast<uint32_t>(relocation->address() - image_base));
		if (!relocation->source()) {
			data.PushDWord(relocation->addend());
			data.PushDWord(0);
		} else {
			data.PushDWord(static_cast<uint32_t>(relocation->source() - image_base));
			data.PushDWord(relocation->size() + 1);
		}
	}

	if (address_) {
		data.PushDWord(static_cast<uint32_t>(address_ - image_base));
		data.PushDWord(1);
		data.PushDWord(0);
	}
}

/**
 * PESEHandler
 */

PESEHandler::PESEHandler(ISEHandlerList *owner, uint64_t address)
	: BaseSEHandler(owner), address_(address), deleted_(false)
{

}

PESEHandler::PESEHandler(ISEHandlerList *owner, const PESEHandler &src)
	: BaseSEHandler(owner)
{
	address_ = src.address_;
	deleted_ = src.deleted_;
}

PESEHandler *PESEHandler::Clone(ISEHandlerList *owner) const
{
	PESEHandler *handler = new PESEHandler(owner, *this);
	return handler;
}

void PESEHandler::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
* PESEHandlerList
*/

PESEHandlerList::PESEHandlerList()
	: BaseSEHandlerList()
{

}

PESEHandlerList::PESEHandlerList(const PESEHandlerList &src)
	: BaseSEHandlerList(src)
{

}

PESEHandlerList *PESEHandlerList::Clone() const
{
	PESEHandlerList *list = new PESEHandlerList(*this);
	return list;
}

PESEHandler *PESEHandlerList::item(size_t index) const
{
	return reinterpret_cast<PESEHandler*>(BaseSEHandlerList::item(index));
}

PESEHandler *PESEHandlerList::Add(uint64_t address)
{
	PESEHandler *handler = new PESEHandler(this, address);
	AddObject(handler);
	return handler;
}

void PESEHandlerList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

void PESEHandlerList::Pack()
{
	for (size_t i = count(); i > 0; i--) {
		PESEHandler *handler = item(i - 1);
		if (handler->is_deleted())
			delete handler;
	}
}

/**
 * PELoadConfigDirectory
 */

PELoadConfigDirectory::PELoadConfigDirectory()
	: IObject(), seh_table_address_(0), security_cookie_(0), cfg_table_address_(0), guard_flags_(0), cfg_check_function_(0)
{
	seh_handler_list_ = new PESEHandlerList();
	cfg_address_list_ = new PECFGAddressTable();
}

PELoadConfigDirectory::PELoadConfigDirectory(const PELoadConfigDirectory &src)
	: IObject(src)
{
	seh_table_address_ = src.seh_table_address_;
	security_cookie_ = src.security_cookie_;
	cfg_table_address_ = src.cfg_table_address_;
	guard_flags_ = src.guard_flags_;
	cfg_check_function_ = src.cfg_check_function_;
	seh_handler_list_ = src.seh_handler_list_->Clone();
	cfg_address_list_ = src.cfg_address_list_->Clone();
}

PELoadConfigDirectory::~PELoadConfigDirectory()
{
	delete seh_handler_list_;
	delete cfg_address_list_;
}

PELoadConfigDirectory *PELoadConfigDirectory::Clone() const
{
	PELoadConfigDirectory *list = new PELoadConfigDirectory(*this);
	return list;
}

void PELoadConfigDirectory::ReadFromFile(PEArchitecture &file, PEDirectory &dir)
{
	if (!dir.address())
		return;

	if (!file.AddressSeek(dir.address()))
		throw std::runtime_error("Invalid address of the load config directory");

	size_t handler_count;
	size_t cfg_table_count;
	uint64_t pos = file.Tell();
	size_t size = file.ReadDWord();
	file.Seek(pos);
	if (file.cpu_address_size() == osQWord) {
		IMAGE_LOAD_CONFIG_DIRECTORYEX64 load_config_directory = IMAGE_LOAD_CONFIG_DIRECTORYEX64();
		file.Read(&load_config_directory, std::min(sizeof(load_config_directory), size));
		security_cookie_ = load_config_directory.SecurityCookie;
		if (load_config_directory.Size > dir.physical_size())
			dir.set_physical_size(load_config_directory.Size);
		seh_table_address_ = load_config_directory.SEHandlerTable;
		handler_count = static_cast<size_t>(load_config_directory.SEHandlerCount);
		cfg_check_function_ = load_config_directory.GuardCFCheckFunctionPointer;
		cfg_table_address_ = load_config_directory.GuardCFFunctionTable;
		cfg_table_count = static_cast<size_t>(load_config_directory.GuardCFFunctionCount);
		guard_flags_ = load_config_directory.GuardFlags;
	} else {
		IMAGE_LOAD_CONFIG_DIRECTORYEX32 load_config_directory = IMAGE_LOAD_CONFIG_DIRECTORYEX32();
		file.Read(&load_config_directory, std::min(sizeof(load_config_directory), size));
		security_cookie_ = load_config_directory.SecurityCookie;
		if (load_config_directory.Size > dir.physical_size())
			dir.set_physical_size(load_config_directory.Size);
		seh_table_address_ = load_config_directory.SEHandlerTable;
		handler_count = load_config_directory.SEHandlerCount;
		cfg_check_function_ = load_config_directory.GuardCFCheckFunctionPointer;
		cfg_table_address_ = load_config_directory.GuardCFFunctionTable;
		cfg_table_count = load_config_directory.GuardCFFunctionCount;
		guard_flags_ = load_config_directory.GuardFlags;
	}

	if (seh_table_address_) {
		if (!file.AddressSeek(seh_table_address_))
			throw std::runtime_error("Invalid address of seh handler table");
		for (size_t i = 0; i < handler_count; i++) {
			seh_handler_list_->Add(file.ReadDWord() + file.image_base());
		}
	}

	if (cfg_table_address_) {
		if (!file.AddressSeek(cfg_table_address_))
			throw std::runtime_error("Invalid address of cfg handler table");
		size_t data_size = (guard_flags_ & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT;
		std::vector<uint8_t> data;
		data.resize(data_size);
		for (size_t i = 0; i < cfg_table_count; i++) {
			PECFGAddress *cfg_address = cfg_address_list_->Add(file.ReadDWord() + file.image_base());
			if (data_size) {
				file.Read(data.data(), data.size());
				cfg_address->set_data(data);
			}
		}
	}
}

size_t PELoadConfigDirectory::WriteToFile(PEArchitecture &file)
{
	PEDirectory *dir = file.command_list()->GetCommandByType(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
	if (!dir || !dir->address())
		return 0;

	size_t res = 0;
	PESegment *last_section = file.segment_list()->last();
	if (seh_table_address_) {
		seh_handler_list_->Pack();
		seh_handler_list_->Sort();
		seh_table_address_ = last_section->address() + file.Resize(AlignValue(file.size(), 0x10)) - last_section->physical_offset();
		for (size_t i = 0; i < seh_handler_list_->count(); i++) {
			res += file.WriteDWord(static_cast<uint32_t>(seh_handler_list_->item(i)->address() - file.image_base()));
		}
	}
	
	if (cfg_table_address_) {
		cfg_table_address_ = last_section->address() + file.Resize(AlignValue(file.size(), 0x10)) - last_section->physical_offset();
		size_t data_size = (guard_flags_ & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT;
		for (size_t i = 0; i < cfg_address_list_->count(); i++) {
			PECFGAddress *cfg_address = cfg_address_list_->item(i);
			res += file.WriteDWord(static_cast<uint32_t>(cfg_address->address() - file.image_base()));
			if (data_size) {
				std::vector<uint8_t> data = cfg_address->data();
				if (data.empty())
					data.resize(data_size);
				res += file.Write(data.data(), data.size());
			}
		}
	}

	uint64_t pos = file.Tell();
	if (file.AddressSeek(dir->address())) {
		uint64_t config_pos = file.Tell();
		size_t size = file.ReadDWord();
		file.Seek(config_pos);
		if (file.cpu_address_size() == osQWord) {
			IMAGE_LOAD_CONFIG_DIRECTORYEX64 load_config_directory = IMAGE_LOAD_CONFIG_DIRECTORYEX64();
			size = std::min(sizeof(load_config_directory), size);
			file.Read(&load_config_directory, size);
			load_config_directory.SecurityCookie = security_cookie_;
			load_config_directory.SEHandlerTable = seh_table_address_;
			load_config_directory.SEHandlerCount = seh_table_address_ ? seh_handler_list_->count() : 0;
			load_config_directory.GuardCFCheckFunctionPointer = cfg_check_function_;
			load_config_directory.GuardCFFunctionTable = cfg_table_address_;
			load_config_directory.GuardCFFunctionCount = cfg_table_address_ ? cfg_address_list_->count() : 0;
			file.Seek(config_pos);
			file.Write(&load_config_directory, size);
		}
		else {
			IMAGE_LOAD_CONFIG_DIRECTORYEX32 load_config_directory = IMAGE_LOAD_CONFIG_DIRECTORYEX32();
			size = std::min(sizeof(load_config_directory), size);
			file.Read(&load_config_directory, size);
			load_config_directory.SecurityCookie = static_cast<uint32_t>(security_cookie_);
			load_config_directory.SEHandlerTable = static_cast<uint32_t>(seh_table_address_);
			load_config_directory.SEHandlerCount = seh_table_address_ ? static_cast<uint32_t>(seh_handler_list_->count()) : 0;
			load_config_directory.GuardCFCheckFunctionPointer = static_cast<uint32_t>(cfg_check_function_);
			load_config_directory.GuardCFFunctionTable = static_cast<uint32_t>(cfg_table_address_);
			load_config_directory.GuardCFFunctionCount = cfg_table_address_ ? static_cast<uint32_t>(cfg_address_list_->count()) : 0;
			file.Seek(config_pos);
			file.Write(&load_config_directory, size);
		}
	}
	file.Seek(pos);

	return res;
}

void PELoadConfigDirectory::FreeByManager(MemoryManager &manager)
{
	if (seh_table_address_ && seh_handler_list_->count())
		manager.Add(seh_table_address_, OperandSizeToValue(osDWord) * seh_handler_list_->count());

	if (cfg_table_address_ && cfg_address_list_->count()) {
		size_t data_size = (guard_flags_ & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT;
		manager.Add(cfg_table_address_, (OperandSizeToValue(osDWord) + data_size) * cfg_address_list_->count());
	}
}

void PELoadConfigDirectory::Rebase(uint64_t delta_base)
{
	if (seh_table_address_)
		seh_table_address_ += delta_base;
	if (cfg_table_address_)
		cfg_table_address_ += delta_base;
	seh_handler_list_->Rebase(delta_base);
	cfg_address_list_->Rebase(delta_base);
}

/**
* PECFGAddress
*/

PECFGAddress::PECFGAddress(PECFGAddressTable *owner, uint64_t address)
	: IObject(), owner_(owner), address_(address)
{

}

PECFGAddress::PECFGAddress(PECFGAddressTable *owner, const PECFGAddress &src)
	: IObject(), owner_(owner)
{
	address_ = src.address_;
	data_ = src.data_;
}

PECFGAddress::~PECFGAddress()
{
	if (owner_)
		owner_->RemoveObject(this);
}

PECFGAddress *PECFGAddress::Clone(PECFGAddressTable *owner) const
{
	PECFGAddress *res = new PECFGAddress(owner, *this);
	return res;
}

void PECFGAddress::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
}

/**
* PECFGAddressTable
*/

PECFGAddressTable::PECFGAddressTable()
	: ObjectList<PECFGAddress>()
{

}

PECFGAddressTable::PECFGAddressTable(const PECFGAddressTable &src)
	: ObjectList<PECFGAddress>()
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

PECFGAddressTable *PECFGAddressTable::Clone() const
{
	return new PECFGAddressTable(*this);
}

PECFGAddress *PECFGAddressTable::Add(uint64_t address)
{
	PECFGAddress *res = new PECFGAddress(this, address);
	AddObject(res);
	return res;
}

void PECFGAddressTable::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * PEResource
 */

PEResource::PEResource(IResource *owner, PEResourceType type, uint32_t name_offset, uint32_t data_offset)
	: BaseResource(owner), type_(type), name_offset_(name_offset), data_offset_(data_offset), 
		address_(0), entry_offset_(0), data_entry_offset_(0)
{
	memset(&data_, 0, sizeof(data_));
}

PEResource::PEResource(IResource *owner, const PEResource &src)
	: BaseResource(owner, src)
{
	type_ = src.type_;
	name_offset_ = src.name_offset_;
	data_offset_ = src.data_offset_;
	data_ = src.data_;
	name_ = src.name_;
	address_ = src.address_;
	entry_offset_ = src.entry_offset_;
	data_entry_offset_ = src.data_entry_offset_;
}

PEResource *PEResource::Clone(IResource *owner) const
{
	PEResource *resource = new PEResource(owner, *this);
	return resource;
}

PEResource *PEResource::item(size_t index) const 
{ 
	return reinterpret_cast<PEResource *>(IResource::item(index));
}

PEResource *PEResource::GetResourceByName(const std::string &name) const
{
	for (size_t i = 0; i < count(); i++) {
		PEResource *resource = item(i);
		if (resource->has_name() && resource->name_ == name)
			return resource;
	}
	return NULL;
}

PEResource *PEResource::Add(PEResourceType type, uint32_t name_offset, uint32_t data_offset)
{
	PEResource *resource = new PEResource(this, type, name_offset, data_offset);
	AddObject(resource);
	return resource;
}

void PEResource::ReadFromFile(PEArchitecture &file, uint64_t root_address)
{
	if (has_name()) {
		// name_offset is a string
		if (!file.AddressSeek(root_address + (name_offset_ & ~IMAGE_RESOURCE_NAME_IS_STRING)))
			throw std::runtime_error("Format error");

		uint16_t len = file.ReadWord();
		os::unicode_string wname;
		wname.resize(len);
		if (!wname.empty())
			file.Read(&wname[0], wname.size() * sizeof(os::unicode_char));
		name_ = os::ToUTF8(wname);
	} else {
		// name_offset is an Id
		name_ = string_format("%d", name_offset_);
	}
	
	if (!file.AddressSeek(root_address + (data_offset_ & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)))
		throw std::runtime_error("Format error");

	if (is_directory()) {
		// read resource directory
		IMAGE_RESOURCE_DIRECTORY_ENTRY dir_entry;
		size_t i;
		file.Read(&data_.dir, sizeof(data_.dir));
		for (i = 0; i < static_cast<size_t>(data_.dir.NumberOfIdEntries + data_.dir.NumberOfNamedEntries); i++) {
			file.Read(&dir_entry, sizeof(dir_entry));
			Add(type_, dir_entry.u.Name, dir_entry.u2.OffsetToData);
		}
		for (i = 0; i < count(); i++) {
			item(i)->ReadFromFile(file, root_address);
		}
	} else {
		// read resource item
		file.Read(&data_.item, sizeof(data_.item));
		address_ = file.image_base() + data_.item.OffsetToData;
	}
}

void PEResource::WriteHeader(Data &data)
{
	size_t i;

	if (is_directory()) {
		// write resource directory
		data_offset_ = (uint32_t)(data.size() | IMAGE_RESOURCE_DATA_IS_DIRECTORY);
		data.WriteDWord(entry_offset_ + sizeof(uint32_t), data_offset_);

		data_.dir.NumberOfIdEntries = 0;
		data_.dir.NumberOfNamedEntries = 0;
		for (i = 0; i < count(); i++) {
			if (item(i)->has_name()) {
				data_.dir.NumberOfNamedEntries++;
			} else {
				data_.dir.NumberOfIdEntries++;
			}
		}
		data.PushBuff(&data_.dir, sizeof(data_.dir));

		for (i = 0; i < static_cast<size_t>(data_.dir.NumberOfIdEntries + data_.dir.NumberOfNamedEntries); i++) {
			item(i)->WriteEntry(data);
		}
	} else {
		// write resource item
		data_entry_offset_ = data.size();
		data.WriteDWord(entry_offset_ + sizeof(uint32_t), (uint32_t)data_entry_offset_);

		data.PushBuff(&data_.item, sizeof(data_.item));
	}
}

void PEResource::WriteHeader(IFunction &data)
{
	size_t i, size;
	IntelFunction &func = reinterpret_cast<IntelFunction &>(data);

	size = 0;
	for (i = 0; i < func.count(); i++) {
		IntelCommand *command = func.item(i);
		size += OperandSizeToValue(command->operand(0).size);
	}

	if (is_directory()) {
		// write resource directory
		func.item(entry_offset_ + 1)->set_operand_value(0, size | IMAGE_RESOURCE_DATA_IS_DIRECTORY);

		data_.dir.NumberOfIdEntries = 0;
		data_.dir.NumberOfNamedEntries = 0;
		for (i = 0; i < count(); i++) {
			if (item(i)->has_name()) {
				data_.dir.NumberOfNamedEntries++;
			} else {
				data_.dir.NumberOfIdEntries++;
			}
		}
		func.AddCommand(osDWord, data_.dir.NumberOfNamedEntries);
		func.AddCommand(osDWord, data_.dir.NumberOfIdEntries);

		for (i = 0; i < static_cast<size_t>(data_.dir.NumberOfIdEntries + data_.dir.NumberOfNamedEntries); i++) {
			item(i)->WriteEntry(data);
		}
	} else {
		// write resource item
		func.item(entry_offset_ + 1)->set_operand_value(0, size);

		data_entry_offset_ = func.count();
		func.AddCommand(osDWord, 0);
		func.AddCommand(osDWord, data_.item.Size);
		func.AddCommand(osDWord, data_.item.CodePage);
		func.AddCommand(osDWord, data_.item.Reserved);
	}
}

void PEResource::WriteEntry(Data &data)
{
	IMAGE_RESOURCE_DIRECTORY_ENTRY dir_entry;

	entry_offset_ = data.size();
	dir_entry.u.Name = name_offset_;
	dir_entry.u2.OffsetToData = data_offset_;
	data.PushBuff(&dir_entry, sizeof(dir_entry));
}

void PEResource::WriteEntry(IFunction &data)
{
	IntelFunction &func = reinterpret_cast<IntelFunction &>(data);

	entry_offset_ = func.count();
	func.AddCommand(osDWord, name_offset_);
	func.AddCommand(osDWord, 0);
}

void PEResource::WriteName(Data &data)
{
	if (!has_name())
		return;

	name_offset_ = (uint32_t)(data.size() | IMAGE_RESOURCE_NAME_IS_STRING);
	data.WriteDWord(entry_offset_, name_offset_);

	os::unicode_string wname = os::FromUTF8(name_);
	data.PushWord(static_cast<uint16_t>(wname.size()));
	data.PushBuff(wname.c_str(), wname.size() * sizeof(os::unicode_char));
}

void PEResource::WriteName(IFunction &data, size_t root_index, uint32_t key)
{
	if (!has_name())
		return;

	IntelFunction &func = reinterpret_cast<IntelFunction &>(data);

	size_t i, size;
	size = 0;
	for (i = root_index; i < func.count(); i++) {
		IntelCommand *command = func.item(i);
		size += (command->type() == cmDB) ? command->dump_size() : OperandSizeToValue(command->operand(0).size);
	}
	func.item(entry_offset_)->set_operand_value(0, size | IMAGE_RESOURCE_NAME_IS_STRING);

	os::unicode_string unicode_name = os::FromUTF8(name_);
	const os::unicode_char *p = unicode_name.c_str();
	Data str;
	for (size_t i = 0; i < unicode_name.size() + 1; i++) {
		str.PushWord(static_cast<uint16_t>(p[i] ^ (_rotl32(key, static_cast<int>(i)) + i)));
	}
	func.AddCommand(str);
}

size_t PEResource::WriteData(Data &data, PEArchitecture &file)
{
	if (is_directory())
		return -1;

	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid data address");

	// resource data must be aligned
	size_t new_size = data.size();
	new_size = AlignValue(new_size, OperandSizeToValue(file.cpu_address_size()));
	for (size_t i = data.size(); i < new_size; i++) {
		data.PushByte(0);
	}

	std::vector<uint8_t> buf;
	buf.resize(data_.item.Size);
	file.Read(buf.data(), buf.size());

	data.WriteDWord(data_entry_offset_, static_cast<uint32_t>(data.size()));
	data.PushBuff(buf.data(), buf.size());

	return data_entry_offset_;
}

void PEResource::WriteData(IFunction &func, PEArchitecture &file, uint32_t key)
{
	if (is_directory() || !data_.item.Size)
		return;

	if (!file.AddressSeek(address()))
		throw std::runtime_error("Invalid data address");

	std::vector<uint8_t> buf;
	buf.resize(data_.item.Size);
	file.Read(buf.data(), buf.size());

	Data d;
	for (size_t i = 0; i < buf.size(); i++) {
		d.PushByte(buf[i] ^ static_cast<uint8_t>(_rotl32(key, static_cast<int>(i)) + i));
	}

	ICommand *command = func.AddCommand(d);
	command->include_option(roCreateNewBlock);

	CommandLink *link = func.item(data_entry_offset_)->AddLink(0, ltOffset, command);
	link->set_sub_value(file.image_base());
}

bool PEResource::need_store() const
{
	switch (type_) {
	case rtIcon: case rtGroupIcon: case rtVersionInfo: 
	case rtManifest: case rtMessageTable: case rtHTML:
		return true;
	case rtUnknown:
		const IResource *resource = this;
		while (resource->owner() && resource->owner()->type() != (uint32_t)-1) {
			resource = resource->owner();
		}
		std::string tmp = resource->name();
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);
		return (tmp.compare("\"TYPELIB\"") == 0 
				|| tmp.compare("\"REGISTRY\"") == 0 
				|| tmp.compare("\"MUI\"") == 0);
	}
	return false;
}

std::string PEResource::id() const
{
	std::string res;
	const PEResource *resource = this;
	while (resource->owner()) {
		res = resource->name() + (res.empty() ? "" : "\\") + res;
		resource = reinterpret_cast<PEResource*>(resource->owner());
	}
	return res;
}

/**
 * PEResourceList
 */

PEResourceList::PEResourceList(PEArchitecture *owner)
	: BaseResourceList(owner), store_size_(0)
{
	memset(&dir_, 0, sizeof(dir_));
}

PEResourceList::PEResourceList(PEArchitecture *owner, const PEResourceList &src)
	: BaseResourceList(owner, src)
{
	dir_ = src.dir_;
	store_size_ = src.store_size_;
}

PEResource *PEResourceList::item(size_t index) const 
{ 
	return reinterpret_cast<PEResource *>(IResourceList::item(index));
}

PEResourceList *PEResourceList::Clone(PEArchitecture *owner) const
{
	PEResourceList *list = new PEResourceList(owner, *this);
	return list;
}

PEResource *PEResourceList::Add(PEResourceType type, uint32_t name_offset, uint32_t data_offset)
{
	PEResource *resource = new PEResource(this, type, name_offset, data_offset);
	AddObject(resource);
	return resource;
}

void PEResourceList::ReadFromFile(PEArchitecture &file, PEDirectory &directory)
{
	if (!directory.address())
		return;

	if (!file.AddressSeek(directory.address()))
		throw std::runtime_error("Format error");

	size_t i;
	IMAGE_RESOURCE_DIRECTORY_ENTRY dir_entry;
	file.Read(&dir_, sizeof(dir_));
	for (i = 0; i < static_cast<size_t>(dir_.NumberOfNamedEntries) + static_cast<size_t>(dir_.NumberOfIdEntries); i++) {
		file.Read(&dir_entry, sizeof(dir_entry));
		Add(dir_entry.u.s.NameIsString ? rtUnknown : static_cast<PEResourceType>(dir_entry.u.Id), dir_entry.u.Name, dir_entry.u2.OffsetToData);
	}

	for (i = 0; i < count(); i++) {
		PEResource *resource = item(i);
		resource->ReadFromFile(file, directory.address());
		switch (resource->type()) {
		case rtCursor:
			resource->set_name("Cursor");
			break;
		case rtBitmap:
			resource->set_name("Bitmap");
			break;
		case rtIcon:
			resource->set_name("Icon");
			break;
		case rtMenu:
			resource->set_name("Menu");
			break;
		case rtDialog:
			resource->set_name("Dialog");
			break;
		case rtStringTable:
			resource->set_name("String Table");
			break;
		case rtFontDir:
			resource->set_name("Font Directory");
			break;
		case rtFont:
			resource->set_name("Font");
			break;
		case rtAccelerators:
			resource->set_name("Accelerators");
			break;
		case rtRCData:
			resource->set_name("RCData");
			break;
		case rtMessageTable:
			resource->set_name("Message Table");
			break;
		case rtGroupCursor:
			resource->set_name("Cursor Group");
			break;
		case rtGroupIcon:
			resource->set_name("Icon Group");
			break;
		case rtVersionInfo:
			resource->set_name("Version Info");
			break;
		case rtDlgInclude:
			resource->set_name("DlgInclude");
			break;
		case rtPlugPlay:
			resource->set_name("Plug Play");
			break;
		case rtVXD:
			resource->set_name("VXD");
			break;
		case rtAniCursor:
			resource->set_name("Animated Cursor");
			break;
		case rtAniIcon:
			resource->set_name("Animated Icon");
			break;
		case rtHTML:
			resource->set_name("HTML");
			break;
		case rtManifest:
			resource->set_name("Manifest");
			break;
		case rtDialogInit:
			resource->set_name("Dialog Init");
			break;
		case rtToolbar:
			resource->set_name("Toolbar");
			break;
		}
	}
}

void PEResourceList::Compile(PEArchitecture &file, bool for_packing)
{
	std::vector<PEResource *> list;
	size_t i, j, c, pos;
	PEResource *resource;

	data_.clear();
	link_list_.clear();

	// create resource list
	for (i = 0; i < count(); i++) {
		list.push_back(item(i));
	}

	for (i = 0; i < list.size(); i++) {
		resource = list[i];
		for (j = 0; j < resource->count(); j++) {
			list.push_back(resource->item(j));
		}
	}

	// write root directory
	dir_.NumberOfIdEntries = 0;
	dir_.NumberOfNamedEntries = 0;
	for (i = 0; i < count(); i++) {
		if (item(i)->has_name()) {
			dir_.NumberOfNamedEntries++;
		} else {
			dir_.NumberOfIdEntries++;
		}
	}
	data_.PushBuff(&dir_, sizeof(dir_));
	for (i = 0; i < static_cast<size_t>(dir_.NumberOfIdEntries + dir_.NumberOfNamedEntries); i++) {
		item(i)->WriteEntry(data_);
	}

	// write items
	for (i = 0; i < list.size(); i++) {
		list[i]->WriteHeader(data_);
	}

	for (i = 0; i < list.size(); i++) {
		list[i]->WriteName(data_);
	}

	store_size_ = 0;
	c = for_packing ? 2 : 1;
	for (j = 0; j < c; j++) {
		for (i = 0; i < list.size(); i++) {
			resource = list[i];
			if (for_packing) {
				if (resource->excluded_from_packing() || resource->need_store()) {
					if (j != 0)
						continue;
				} else {
					if (j == 0)
						continue;
				}
			}

			pos = resource->WriteData(data_, file);
			if (pos != (size_t)-1)
				link_list_.push_back(pos);		
		}

		if (j == 0)
			store_size_ = data_.size();
	}
}

size_t PEResourceList::WriteToFile(PEArchitecture &file, uint64_t address)
{
	size_t i, pos;
	Data out = data_;
	uint32_t rva = static_cast<uint32_t>(address - file.image_base());

	for (i = 0; i < link_list_.size(); i++) {
		pos = link_list_[i];
		out.WriteDWord(pos, out.ReadDWord(pos) + rva);
	}

	return file.Write(out.data(), (store_size_) ? store_size_ : out.size());
}

void PEResourceList::WritePackData(Data &data)
{
	data.PushBuff(data_.data() + store_size_, data_.size() - store_size_);
}

/**
 * PERuntimeFunction
 */

PERuntimeFunction::PERuntimeFunction(PERuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address)
	: BaseRuntimeFunction(owner), address_(address), begin_(begin), end_(end), unwind_address_(unwind_address)
{

}

PERuntimeFunction::PERuntimeFunction(PERuntimeFunctionList *owner, const PERuntimeFunction &src)
	: BaseRuntimeFunction(owner)
{
	address_ = src.address_;
	begin_ = src.begin_;
	end_ = src.end_;
	unwind_address_ = src.unwind_address_;
}

PERuntimeFunction *PERuntimeFunction::Clone(IRuntimeFunctionList *owner) const
{
	PERuntimeFunction *func = new PERuntimeFunction(reinterpret_cast<PERuntimeFunctionList *>(owner), *this);
	return func;
}

void PERuntimeFunction::Rebase(uint64_t delta_base)
{
	address_ += delta_base;
	begin_ += delta_base;
	end_ += delta_base;
	unwind_address_ += delta_base;
}

void PERuntimeFunction::Parse(IArchitecture &file, IFunction &dest)
{
	union UNWIND_INFO_HELPER {
		UNWIND_INFO info;
		uint32_t value;
	};

	IntelFunction &func = reinterpret_cast<IntelFunction &>(dest);

	uint64_t address = address_;
	if (!file.AddressSeek(address) || func.GetCommandByAddress(address))
		return;

	size_t i;
	size_t c = func.count();
	IntelCommand *command;
	CommandLink *link;
	uint64_t image_base = file.image_base();

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "Begin"));
	command->ReadValueFromFile(file, osDWord);
	address = command->next_address();

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "End"));
	command->ReadValueFromFile(file, osDWord);
	address = command->next_address();

	command = func.Add(address);
	command->set_comment(CommentInfo(ttComment, "UnwindData"));
	address = command->ReadValueFromFile(file, osDWord);
	if (address) {
		address += image_base;
		link = command->AddLink(0, ltOffset, address);
		link->set_sub_value(image_base);
	}

	for (i = c; i < func.count(); i++) {
		command = func.item(i);
		command->exclude_option(roClearOriginalCode);
		command->exclude_option(roNeedCompile);
	}

	if (address) {
		command = func.GetCommandByAddress(address);
		if (command) {
			UNWIND_INFO_HELPER unwind_info_helper;
			unwind_info_helper.value = static_cast<uint32_t>(command->dump_value(0, osDWord));
			UNWIND_INFO unwind_info = unwind_info_helper.info;

			func.function_info_list()->Add(begin(), end(), btImageBase, 0, unwind_info.SizeOfProlog, unwind_info.FrameRegister ? unwind_info.FrameRegister : 0xff, this, command);
		} else if (file.AddressSeek(address)) {
			UNWIND_INFO_HELPER unwind_info_helper;
			command = func.Add(address);
			unwind_info_helper.value = static_cast<uint32_t>(command->ReadValueFromFile(file, osDWord));
			UNWIND_INFO unwind_info = unwind_info_helper.info;
			command->set_comment(CommentInfo(ttComment, string_format("Version: %.2X; Flags: %.2X; SizeOfProlog: %.2X; CountOfCodes: %.2X; FrameRegister: %.2X; FrameOffset: %.2X",
				unwind_info.Version, unwind_info.Flags, unwind_info.SizeOfProlog, unwind_info.CountOfCodes, unwind_info.FrameRegister, unwind_info.FrameOffset)));
			command->set_alignment(OperandSizeToValue(osDWord));
			command->include_option(roCreateNewBlock);
			address = command->next_address();

			func.function_info_list()->Add(begin(), end(), btImageBase, 0, unwind_info.SizeOfProlog, unwind_info.FrameRegister ? unwind_info.FrameRegister : 0xff, this, command);

			for (i = 0; i < unwind_info.CountOfCodes; i++) {
				command = func.Add(address);

				UNWIND_CODE unwind_code;
				bool is_epilog = false;
				file.Read(&unwind_code, sizeof(unwind_code));
				Data data;
				data.InsertBuff(0, &unwind_code, sizeof(unwind_code));
				switch (unwind_code.UnwindOp) {
				case UWOP_PUSH_NONVOL:
					command->set_comment(CommentInfo(ttComment, "UWOP_PUSH_NONVOL"));
					break;
				case UWOP_ALLOC_LARGE:
					data.PushWord(file.ReadWord());
					i++;
					if (unwind_code.OpInfo == 1) {
						data.PushWord(file.ReadWord());
						i++;
					}
					command->set_comment(CommentInfo(ttComment, "UWOP_ALLOC_LARGE"));
					break;
				case UWOP_ALLOC_SMALL:
					command->set_comment(CommentInfo(ttComment, "UWOP_ALLOC_SMALL"));
					break;
				case UWOP_SET_FPREG:
					command->set_comment(CommentInfo(ttComment, "UWOP_SET_FPREG"));
					break;
				case UWOP_SAVE_NONVOL:
					data.PushWord(file.ReadWord());
					i++;
					command->set_comment(CommentInfo(ttComment, "UWOP_SAVE_NONVOL"));
					break;
				case UWOP_SAVE_NONVOL_FAR:
					data.PushWord(file.ReadWord());
					data.PushWord(file.ReadWord());
					i += 2;
					command->set_comment(CommentInfo(ttComment, "UWOP_SAVE_NONVOL_FAR"));
					break;
				case UWOP_EPILOG:
					if (unwind_info.Version == 2) {
						is_epilog = true;
						if (unwind_code.CodeOffset) {
							uint64_t range_begin, range_end;
							if ((unwind_code.OpInfo & 1)) {
								range_begin = end() - unwind_code.CodeOffset;
								range_end = end();
							}
							else {
								UNWIND_CODE next_code;
								file.Read(&next_code, sizeof(next_code));
								data.PushWord(next_code.FrameOffset);
								i++;
								range_begin = end() - ((next_code.OpInfo << 8) + next_code.CodeOffset);
								range_end = range_begin + unwind_code.CodeOffset;
							}
							func.range_list()->Add(range_begin, range_end, NULL, NULL, command);
						}
						command->set_comment(CommentInfo(ttComment, "UWOP_EPILOG"));
					}
					else {
						data.PushWord(file.ReadWord());
						i++;
						command->set_comment(CommentInfo(ttComment, "UWOP_SAVE_XMM128"));
					}
					break;
				case UWOP_SAVE_XMM128:
					data.PushWord(file.ReadWord());
					i++;
					command->set_comment(CommentInfo(ttComment, "UWOP_SAVE_XMM128"));
					break;
				case UWOP_SAVE_XMM128_FAR:
					data.PushWord(file.ReadWord());
					data.PushWord(file.ReadWord());
					i += 2;
					command->set_comment(CommentInfo(ttComment, "UWOP_SAVE_XMM128_FAR"));
					break;
				case UWOP_PUSH_MACHFRAME:
					command->set_comment(CommentInfo(ttComment, "UWOP_PUSH_MACHFRAME"));
					break;
				}

				command->Init(data);
				address = command->next_address();

				if (!is_epilog)
					func.range_list()->Add(begin(), begin() + unwind_code.CodeOffset, NULL, NULL, NULL);
			}
			if (unwind_info.CountOfCodes & 1) {
				// align to DWORD
				command = func.Add(address);
				command->ReadArray(file, sizeof(UNWIND_CODE));
				address = command->next_address();
			}

			IntelCommand *handler_data_command = NULL;
			if (unwind_info.Flags & UNW_FLAG_CHAININFO) {
				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Begin"));
				command->ReadValueFromFile(file, osDWord);
				address = command->next_address();

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "End"));
				command->ReadValueFromFile(file, osDWord);
				address = command->next_address();

				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "UnwindData"));
				address = command->ReadValueFromFile(file, osDWord);
				if (address) {
					address += image_base;
					link = command->AddLink(0, ltOffset, address);
					link->set_sub_value(image_base);
				}
			}
			else if (unwind_info.Flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER)) {
				command = func.Add(address);
				command->set_comment(CommentInfo(ttComment, "Handler"));
				address = command->ReadValueFromFile(file, osDWord);
				if (address) {
					address += image_base;
					CommentInfo res;
					res.type = ttNone;
					MapFunction *map_function = file.map_function_list()->GetFunctionByAddress(address);
					if (map_function) {
						res.value = string_format("%c %s", 3, map_function->name().c_str());
						switch (map_function->type()) {
						case otString:
							res.type = ttString;
							break;
						case otExport:
							res.type = ttExport;
							break;
						default:
							res.type = ttFunction;
							break;
						}

						command->set_comment(res);
					}
					link = command->AddLink(0, ltOffset, address);
					link->set_sub_value(image_base);
				}
				address = command->next_address();

				handler_data_command = func.Add(address);
				handler_data_command->ReadValueFromFile(file, osDWord);
				handler_data_command->set_comment(CommentInfo(ttComment, "HandlerData"));
			}

			for (i = c; i < func.count(); i++) {
				command = func.item(i);
				command->exclude_option(roClearOriginalCode);
			}

			if (handler_data_command) {
				uint32_t handler_data = static_cast<uint32_t>(handler_data_command->operand(0).value);
				if (func.ParseCxxSEH(file, handler_data + image_base)) {
					link = handler_data_command->AddLink(0, ltOffset, handler_data + image_base);
					link->set_sub_value(image_base);
					address = handler_data_command->next_address();
				}
				else if (func.ParseScopeSEH(file, handler_data_command->next_address(), handler_data)) {
					handler_data_command->set_comment(CommentInfo(ttComment, "Count"));
					address = handler_data_command->next_address() + handler_data * 0x10;
				}
				else if (func.ParseCompressedCxxSEH(file, handler_data + image_base, begin())) {
					link = handler_data_command->AddLink(0, ltOffset, handler_data + image_base);
					link->set_sub_value(image_base);
					address = handler_data_command->next_address();
				} else
					address = 0;

				if (address) {
					if (!func.GetCommandByAddress(address) && !file.runtime_function_list()->GetFunctionByUnwindAddress(address) && file.AddressSeek(address)) {
						command = func.Add(address);
						uint32_t value = static_cast<uint32_t>(command->ReadValueFromFile(file, osDWord));
						command->set_comment(CommentInfo(ttComment, string_format("EHandler: %.2X; UHandler: %.2X; HasAlignment: %.2X; CookieOffset: %.8X", value & 1, (value & 2) >> 1, (value & 4) >> 2, value & 0xFFFFFFF8)));
						command->exclude_option(roClearOriginalCode);

						if (value & 4) {
							command = func.Add(command->next_address());
							command->set_comment(CommentInfo(ttComment, "AlignedBaseOffset"));
							command->ReadValueFromFile(file, osDWord);
							command->exclude_option(roClearOriginalCode);

							command = func.Add(command->next_address());
							command->set_comment(CommentInfo(ttComment, "Alignment"));
							command->ReadValueFromFile(file, osDWord);
							command->exclude_option(roClearOriginalCode);
						}
					}
				}
			}
		}
	}
}

/**
 * PERuntimeFunctionList
 */

PERuntimeFunctionList::PERuntimeFunctionList()
	: BaseRuntimeFunctionList(), address_(0)
{

}

PERuntimeFunctionList::PERuntimeFunctionList(const PERuntimeFunctionList &src)
	: BaseRuntimeFunctionList(src), address_(0)
{
	address_ = src.address_;
}

PERuntimeFunctionList *PERuntimeFunctionList::Clone() const
{
	PERuntimeFunctionList *list = new PERuntimeFunctionList(*this);
	return list;
}

PERuntimeFunction *PERuntimeFunctionList::Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions)
{
	PERuntimeFunction *func = new PERuntimeFunction(this, address, begin, end, unwind_address);
	AddObject(func);
	return func;
}

void PERuntimeFunctionList::ReadFromFile(PEArchitecture &file, PEDirectory &directory)
{
	if (!directory.address())
		return;

	if (!file.AddressSeek(directory.address()))
		throw std::runtime_error("Format error");

	address_ = directory.address();
	uint64_t image_base = file.image_base();
	RUNTIME_FUNCTION data;
	std::vector<uint8_t> call_frame_instructions;
	for (size_t i = 0; i < directory.size(); i += sizeof(data)) {
		file.Read(&data, sizeof(data));
		Add(address_ + i, data.BeginAddress + image_base, data.EndAddress + image_base, data.UnwindData + image_base, 0, call_frame_instructions);
	}
}

size_t PERuntimeFunctionList::WriteToFile(PEArchitecture &file)
{
	Sort();

	size_t res = 0;
	uint64_t image_base = file.image_base();
	RUNTIME_FUNCTION data;
	for (size_t i = 0; i < count(); i++) {
		PERuntimeFunction *runtime_function = item(i);
		data.BeginAddress = static_cast<uint32_t>(runtime_function->begin() - image_base);
		data.EndAddress = static_cast<uint32_t>(runtime_function->end() - image_base);
		data.UnwindData = static_cast<uint32_t>(runtime_function->unwind_address() - image_base);
		res += file.Write(&data, sizeof(data));
	}

	return res;
}

PERuntimeFunction *PERuntimeFunctionList::GetFunctionByAddress(uint64_t address) const
{
	return reinterpret_cast<PERuntimeFunction *>(BaseRuntimeFunctionList::GetFunctionByAddress(address));
}

PERuntimeFunction *PERuntimeFunctionList::item(size_t index) const
{
	return reinterpret_cast<PERuntimeFunction *>(BaseRuntimeFunctionList::item(index));
}

uint64_t PERuntimeFunctionList::RebaseDWord(IArchitecture &file, uint32_t delta_rva)
{
	uint64_t pos = file.Tell();
	uint64_t value = file.ReadDWord();
	if (value > 1) {
		uint64_t address = file.AddressTell() - sizeof(uint32_t);
		IntelCommand *command = reinterpret_cast<IntelCommand *>(file.function_list()->GetCommandByAddress(address, false));
		if (command && command->type() == cmDD) {
			command->set_operand_value(0, command->operand(0).value + delta_rva);
			if (command->link())
				command->link()->set_sub_value(command->link()->sub_value() - delta_rva);
		}
		file.Seek(pos);
		file.WriteDWord(static_cast<uint32_t>(value) + delta_rva);
		value += file.image_base();
	}
	return value;
}

void PERuntimeFunctionList::RebaseByFile(IArchitecture &file, uint64_t target_image_base, uint64_t delta_base)
{
	if (!address_)
		return;

	uint32_t delta_rva = static_cast<uint32_t>(file.image_base() + delta_base - target_image_base);
	std::set<uint64_t> address_list;
	std::set<uint64_t> handler_list;
	size_t i, j, k;
	for (i = 0; i < count(); i++) {
		PERuntimeFunction *func = item(i);

		if (!file.AddressSeek(func->unwind_address()) || address_list.find(func->unwind_address()) != address_list.end())
			continue;

		address_list.insert(func->unwind_address());
		
		union UNWIND_INFO_HELPER {
			UNWIND_INFO info;
			uint32_t value;
		};
		UNWIND_INFO_HELPER unwind_info_helper;
		unwind_info_helper.value = file.ReadDWord();
		UNWIND_INFO unwind_info = unwind_info_helper.info;
		size_t count_of_codes = unwind_info.CountOfCodes;
		if (count_of_codes & 1) {
			// align to DWORD
			count_of_codes++;
		}

		for (j = 0; j < count_of_codes; j++) {
			file.ReadWord();
		}

		if (unwind_info.Flags & UNW_FLAG_CHAININFO) {
			RebaseDWord(file, delta_rva);
			RebaseDWord(file, delta_rva);
			RebaseDWord(file, delta_rva);
		} else if (unwind_info.Flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER)) {
			RebaseDWord(file, delta_rva);
			uint64_t handler_data_address = file.AddressTell();
			uint32_t handler_data = file.ReadDWord();
			bool is_cxx_handler = false;
			bool is_scope_table = false;
			if (file.AddressSeek(handler_data + file.image_base())) {
				uint32_t magic = file.ReadDWord();
				if (magic == 0x19930520 || magic == 0x19930521 || magic == 0x19930522)
					is_cxx_handler = true;
			}
			if (!is_cxx_handler && file.AddressSeek(handler_data_address + sizeof(handler_data))) {
				is_scope_table = true;
				for (j = 0; j < handler_data; j++) {
					for (size_t k = 0; k < 4; k++) {
						uint64_t value = file.ReadDWord();
						if (!value || (k == 2 && value == 1))
							continue;
						if ((file.segment_list()->GetMemoryTypeByAddress(value + file.image_base()) & mtExecutable) == 0) {
							is_scope_table = false;
							break;
						}
					}
					if (!is_scope_table)
						break;
				}
			}

			if (is_cxx_handler) {
				file.AddressSeek(handler_data_address);
				RebaseDWord(file, delta_rva);
				handler_data_address = handler_data + file.image_base();
				if (handler_list.find(handler_data_address) != handler_list.end())
					continue;

				handler_list.insert(handler_data_address);
				file.AddressSeek(handler_data_address);
				file.ReadDWord();
				uint32_t max_state = file.ReadDWord();
				uint64_t unwind_map_entry = RebaseDWord(file, delta_rva);
				uint32_t try_blocks = file.ReadDWord();
				uint64_t try_blocks_entry = RebaseDWord(file, delta_rva);
				uint32_t map_count = file.ReadDWord();
				uint64_t map_entry = RebaseDWord(file, delta_rva);

				if (max_state && file.AddressSeek(unwind_map_entry)) {
					for (j = 0; j < max_state; j++) {
						file.ReadDWord();
						RebaseDWord(file, delta_rva);
					}
				}

				if (try_blocks && file.AddressSeek(try_blocks_entry)) {
					for (j = 0; j < try_blocks; j++) {
						file.ReadDWord();
						file.ReadDWord();
						file.ReadDWord();
						uint32_t catches = file.ReadDWord();
						uint64_t catches_entry = RebaseDWord(file, delta_rva);
						uint64_t pos = file.Tell();
						if (catches && file.AddressSeek(catches_entry)) {
							for (k = 0; k < catches; k++) {
								file.ReadDWord();
								file.ReadDWord();
								file.ReadDWord();
								RebaseDWord(file, delta_rva);
								if (file.cpu_address_size() == osQWord)
									file.ReadDWord();
							}
							file.Seek(pos);
						}
					}
				}

				if (map_count && file.AddressSeek(map_entry)) {
					for (j = 0; j < map_count; j++) {
						RebaseDWord(file, delta_rva);
						file.ReadDWord();
					}
				}
			} else if (is_scope_table) {
				file.AddressSeek(handler_data_address + sizeof(handler_data));
				for (j = 0; j < handler_data; j++) {
					RebaseDWord(file, delta_rva);
					RebaseDWord(file, delta_rva);
					RebaseDWord(file, delta_rva);
					RebaseDWord(file, delta_rva);
				}
			}
		}
	}

	address_ += delta_base;

	BaseRuntimeFunctionList::Rebase(delta_base);
}

void PERuntimeFunctionList::FreeByManager(MemoryManager &manager)
{
	if (!address_)
		return;

	if (count())
		manager.Add(address_, sizeof(RUNTIME_FUNCTION) * count());
}

/**
 * PETLSDirectory
 */

PETLSDirectory::PETLSDirectory()
	: ReferenceList(), address_(0), start_address_of_raw_data_(0), end_address_of_raw_data_(0), address_of_index_(0),
	address_of_call_backs_(0), size_of_zero_fill_(0), characteristics_(0)
{

}

PETLSDirectory *PETLSDirectory::Clone() const
{
	PETLSDirectory *dir = new PETLSDirectory(*this);
	return dir;
}

PETLSDirectory::PETLSDirectory(const PETLSDirectory &src)
	: ReferenceList(src)
{
	address_ = src.address_;
	start_address_of_raw_data_ = src.start_address_of_raw_data_;
	end_address_of_raw_data_ = src.end_address_of_raw_data_;
	address_of_index_ = src.address_of_index_;
	address_of_call_backs_ = src.address_of_call_backs_;
	size_of_zero_fill_ = src.size_of_zero_fill_;
	characteristics_ = src.characteristics_;
}

void PETLSDirectory::ReadFromFile(PEArchitecture &file, PEDirectory &directory)
{
	if (!directory.address())
		return;

	if (!file.AddressSeek(directory.address()))
		throw std::runtime_error("Format error");

	address_ = directory.address();

	if (file.cpu_address_size() == osDWord) {
		IMAGE_TLS_DIRECTORY32 tls;
		file.Read(&tls, sizeof(tls));
		start_address_of_raw_data_ = tls.StartAddressOfRawData;
		end_address_of_raw_data_ = tls.EndAddressOfRawData;
		address_of_index_ = tls.AddressOfIndex;
		address_of_call_backs_ = tls.AddressOfCallBacks;
		size_of_zero_fill_ = tls.SizeOfZeroFill;
		characteristics_ = tls.Characteristics;
	} else {
		IMAGE_TLS_DIRECTORY64 tls;
		file.Read(&tls, sizeof(tls));
		start_address_of_raw_data_ = tls.StartAddressOfRawData;
		end_address_of_raw_data_ = tls.EndAddressOfRawData;
		address_of_index_ = tls.AddressOfIndex;
		address_of_call_backs_ = tls.AddressOfCallBacks;
		size_of_zero_fill_ = tls.SizeOfZeroFill;
		characteristics_ = tls.Characteristics;
	}

	if (!address_of_call_backs_)
		return;

	if (!file.AddressSeek(address_of_call_backs_))
		throw std::runtime_error("Format error");
	
	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	uint64_t call_back = 0;
	while (true) {
		file.Read(&call_back, value_size);
		if (!call_back)
			break;
		Add(call_back, 0);
	}
}

void PETLSDirectory::FreeByManager(MemoryManager &manager)
{
	if (!address_)
		return;

	PEArchitecture *file = reinterpret_cast<PEArchitecture *>(manager.owner());
	size_t value_size = OperandSizeToValue(file->cpu_address_size());
	manager.Add(address_, value_size * 4 + sizeof(uint32_t) * 2);

	for (size_t i = 0; i < 4; i++) {
		IFixup *fixup = file->fixup_list()->GetFixupByAddress(address_ + value_size * i);
		if (fixup)
			fixup->set_deleted(true);
	}

	if (address_of_call_backs_) {
		manager.Add(address_of_call_backs_, value_size * (count() + 1));
		for (size_t i = 0; i < count(); i++) {
			IFixup *fixup = file->fixup_list()->GetFixupByAddress(address_of_call_backs_ + value_size * i);
			if (fixup)
				fixup->set_deleted(true);
		}
	}
}

/**
 * PEDebugData
 */

PEDebugData::PEDebugData(PEDebugDirectory *owner)
	: IObject(), owner_(owner), characteristics_(0), time_date_stamp_(0),
	major_version_(0), minor_version_(0), type_(0), size_(0), address_(0),
	offset_(0)
{

}

PEDebugData::PEDebugData(PEDebugDirectory *owner, const PEDebugData &src)
	: IObject(), owner_(owner)
{
	characteristics_ = src.characteristics_;
	time_date_stamp_ = src.time_date_stamp_;
	major_version_ = src.major_version_;
	minor_version_ = src.minor_version_;
	type_ = src.type_;
	size_ = src.size_;
	address_ = src.address_;
	offset_ = src.offset_;
}

PEDebugData::~PEDebugData()
{
	if (owner_)
		owner_->RemoveObject(this);
}

PEDebugData *PEDebugData::Clone(PEDebugDirectory *owner) const
{
	PEDebugData *data = new PEDebugData(owner, *this);
	return data;
}

void PEDebugData::ReadFromFile(PEArchitecture &file)
{
	IMAGE_DEBUG_DIRECTORY data;
	file.Read(&data, sizeof(data));
	characteristics_ = data.Characteristics;
	time_date_stamp_ = data.TimeDateStamp;
	major_version_ = data.MajorVersion;
	minor_version_ = data.MinorVersion;
	type_ = data.Type;
	size_ = data.SizeOfData;
	address_ = data.AddressOfRawData ? data.AddressOfRawData + file.image_base() : 0;
	offset_ = data.PointerToRawData;
}

void PEDebugData::WriteToFile(PEArchitecture &file)
{
	IMAGE_DEBUG_DIRECTORY data;
	data.Characteristics = characteristics_;
	data.TimeDateStamp = time_date_stamp_;
	data.MajorVersion = major_version_;
	data.MinorVersion = minor_version_;
	data.Type = type_;
	data.SizeOfData = size_;
	data.AddressOfRawData = address_ ? static_cast<uint32_t>(address_ - file.image_base()) : 0;
	data.PointerToRawData = offset_;
	file.Write(&data, sizeof(data));
}

/**
 * PEDebugDirectory
 */

PEDebugDirectory::PEDebugDirectory()
	: ObjectList<PEDebugData>(), address_(0)
{

}

PEDebugDirectory::PEDebugDirectory(const PEDebugDirectory &src)
	: ObjectList<PEDebugData>(src)
{
	address_ = src.address_;
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

PEDebugDirectory *PEDebugDirectory::Clone() const
{
	PEDebugDirectory *res = new PEDebugDirectory(*this);
	return res;
}

PEDebugData *PEDebugDirectory::Add()
{
	PEDebugData *data = new PEDebugData(this);
	AddObject(data);
	return data;
}

void PEDebugDirectory::ReadFromFile(PEArchitecture &file, PEDirectory &directory)
{
	if (!directory.address())
		return;

	if (!file.AddressSeek(directory.address()))
		throw std::runtime_error("Format error");

	address_ = directory.address();

	size_t c = directory.size() / sizeof(IMAGE_DEBUG_DIRECTORY);
	for (size_t i = 0; i < c; i++) {
		PEDebugData *data = Add();
		data->ReadFromFile(file);
	}
}

void PEDebugDirectory::WriteToFile(PEArchitecture &file)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->WriteToFile(file);
	}
}

void PEDebugDirectory::FreeByManager(MemoryManager &manager) const
{
	if (!address_)
		return;

	manager.Add(address_, count() * sizeof(IMAGE_DEBUG_DIRECTORY));
	for (size_t i = 0; i < count(); i++) {
		PEDebugData *data = item(i);
		if (data->address() && data->size())
			manager.Add(data->address(), data->size());
	}
}

/**
 * PEFile
 */

PEFile::PEFile(ILog *log) 
	: IFile(log), runtime_(NULL)
{

}

PEFile::PEFile(const PEFile &src, const char *file_name)
	: IFile(src, file_name), runtime_(NULL)
{
	for (size_t i = 0; i < src.count(); i++)
		AddObject(src.item(i)->Clone(this));
}

PEFile::~PEFile()
{
	delete runtime_;
}

OpenStatus PEFile::ReadHeader(uint32_t open_mode)
{
	PEArchitecture *arch = new PEArchitecture(this, 0, size());
	AddObject(arch);
	return arch->ReadFromFile(open_mode);
}

std::string PEFile::format_name() const
{
	return std::string("PE");
}

bool PEFile::WriteHeader()
{
	for (size_t i = 0 ; i < count(); i++) {
		if (!item(i)->WriteToFile())
			return false;
	}
	return true;
}

PEFile *PEFile::Clone(const char *file_name) const
{
	PEFile *file = new PEFile(*this, file_name);
	return file;
}

bool PEFile::Compile(CompileOptions &options)
{
	const ResourceInfo runtime_info[] = {
		{win_runtime32_dll_file, sizeof(win_runtime32_dll_file), win_runtime32_dll_code},
		{win_runtime64_dll_file, sizeof(win_runtime64_dll_file), win_runtime64_dll_code},
		{win_runtime32_sys_file, sizeof(win_runtime32_sys_file), win_runtime32_sys_code},
		{win_runtime64_sys_file, sizeof(win_runtime64_sys_file), win_runtime64_sys_code},
		{dotnet20_runtime32_dll_file, sizeof(dotnet20_runtime32_dll_file), dotnet20_runtime32_dll_code},
		{dotnet20_runtime64_dll_file, sizeof(dotnet20_runtime64_dll_file), dotnet20_runtime64_dll_code},
		{dotnet40_runtime32_dll_file, sizeof(dotnet40_runtime32_dll_file), dotnet40_runtime32_dll_code},
		{dotnet40_runtime64_dll_file, sizeof(dotnet40_runtime64_dll_file), dotnet40_runtime64_dll_code},
		{netstandard_runtime32_dll_file, sizeof(netstandard_runtime32_dll_file), netstandard_runtime32_dll_code},
		{netstandard_runtime64_dll_file, sizeof(netstandard_runtime64_dll_file), netstandard_runtime64_dll_code},
		{netcore_runtime32_dll_file, sizeof(netcore_runtime32_dll_file), netcore_runtime32_dll_code},
		{netcore_runtime64_dll_file, sizeof(netcore_runtime64_dll_file), netcore_runtime64_dll_code},
	};

	size_t index;
	if (count() > 1) {
		FrameworkInfo info = reinterpret_cast<NETArchitecture *>(item(1))->command_list()->framework();
		switch (info.type) {
		case fwFramework:
			index = (info.version.major >= 4) ? 6 : 4;
			break;
		case fwStandard:
			index = 8;
			break;
		default:
			index = 10;
			break;
		}
	}
	else
		index = (arch_pe()->image_type() == itDriver) ? 2 : 0;
	
	ResourceInfo info = runtime_info[index + (arch_pe()->cpu_address_size() == osDWord ? 0 : 1)];
	if (info.size > 1) {
		runtime_ = new PEFile(NULL);
		if (!runtime_->OpenResource(info.file, info.size, true) || count() != runtime_->count())
			throw std::runtime_error("Runtime error at OpenResource");

		Buffer buffer(info.code);
		IArchitecture *arch = runtime_->item(runtime_->count() - 1);
		arch->ReadFromBuffer(buffer);
		for (size_t i = 0; i < arch->function_list()->count(); i++) {
			arch->function_list()->item(i)->set_from_runtime(true);
		}
		for (size_t i = 0; i < arch->import_list()->count(); i++) {
			IImport *import = arch->import_list()->item(i);
			for (size_t j = 0; j < import->count(); j++) {
				import->item(j)->include_option(ioFromRuntime);
			}
		}
	}

	return IFile::Compile(options);
}

std::string PEFile::version() const
{
	struct VERSION_INFO {
		uint16_t wLength;
		uint16_t wValueLength;
		uint16_t wType;
		uint16_t szKey[1];
	};

	if (count() == 1) {
		IArchitecture *file = item(0);
		IResource *resource = file->resource_list()->GetResourceByType(rtVersionInfo);
		if (resource)
			resource = resource->GetResourceByName("1");
		if (resource && resource->count()) {
			resource = resource->item(0);
			if (resource->size() && file->AddressSeek(resource->address())) {
				uint8_t *data = new uint8_t[resource->size()];
				file->Read(data, resource->size());

				size_t len = 0;
				while (reinterpret_cast<VERSION_INFO*>(data)->szKey[len])
					len++;
				VS_FIXEDFILEINFO *file_info = reinterpret_cast<VS_FIXEDFILEINFO *>(data + AlignValue(offsetof(VERSION_INFO, szKey) + (len + 1) * sizeof(uint16_t), sizeof(uint32_t)));
				std::string res = string_format("%d.%d.%d.%d", static_cast<uint16_t>(file_info->dwFileVersionMS >> 16), static_cast<uint16_t>(file_info->dwFileVersionMS), static_cast<uint16_t>(file_info->dwFileVersionLS >> 16), static_cast<uint16_t>(file_info->dwFileVersionLS));
				delete [] data;

				return res;
			}
		}
	}

	return IFile::version();
}

bool PEFile::is_executable() const
{
#ifdef __unix__
	return false;
#elif __APPLE__
	return false;
#else
	for (size_t i = 0; i < count(); i++) {
		if (item(i)->is_executable())
			return true;
	}
#endif
	return false;
}

uint32_t PEFile::disable_options() const
{
	uint32_t res = 0;
	if (count() == 1) {
		PEArchitecture *arch = arch_pe();
		if (arch->segment_alignment() < 0x1000)
			res |= cpPack;
		if (arch->image_type() != itExe)
			res |= cpStripFixups;
		if (arch->image_type() == itDriver) {
			res |= cpResourceProtection;
			res |= cpVirtualFiles;
		}
	} else {
		res |= cpStripFixups;
	}
	return res;
}

bool PEFile::GetCheckSum(uint32_t *check_sum)
{
	Flush();
	return os::FileGetCheckSum(file_name(true).c_str(), check_sum);
}

std::string PEFile::exec_command() const
{
	if (count() > 1 && reinterpret_cast<NETArchitecture *>(item(1))->command_list()->framework().type == fwCore)
		return "dotnet.exe";
	return std::string();
}

/**
 * PEArchitecture
 */

PEArchitecture::PEArchitecture(PEFile *owner, uint64_t offset, uint64_t size)
	: BaseArchitecture(owner, offset, size), function_list_(NULL), virtual_machine_list_(NULL), 
		cpu_(0), cpu_address_size_(osDWord), time_stamp_(0), entry_point_(0),
		image_base_(0), header_offset_(0), header_size_(0), segment_alignment_(0),
		file_alignment_(0),	resource_section_(NULL), fixup_section_(NULL), 
		optimized_section_count_(0), image_type_(itExe), characterictics_(0), check_sum_(0),
		low_resize_header_(0), resize_header_(0), operating_system_version_(0), subsystem_version_(0),
		dll_characteristics_(0)
{
	directory_list_ = new PEDirectoryList(this);
	segment_list_ = new PESegmentList(this);
	section_list_ = new PESectionList(this);
	import_list_ = new PEImportList(this);
	export_list_ = new PEExportList(this);
	fixup_list_ = new PEFixupList();
	relocation_list_ = new PERelocationList();
	resource_list_ = new PEResourceList(this);
	load_config_directory_ = new PELoadConfigDirectory();
	runtime_function_list_ = new PERuntimeFunctionList();
	tls_directory_ = new PETLSDirectory();
	debug_directory_ = new PEDebugDirectory();
	delay_import_list_ = new PEDelayImportList();
}

PEArchitecture::PEArchitecture(PEFile *owner, const PEArchitecture &src)
	: BaseArchitecture(owner, src), function_list_(NULL), virtual_machine_list_(NULL),
	resource_section_(NULL), fixup_section_(NULL)
{
	size_t i, j, k;

	cpu_ = src.cpu_;
	cpu_address_size_ = src.cpu_address_size_;
	entry_point_ = src.entry_point_;
	image_base_ = src.image_base_;
	header_offset_ = src.header_offset_;
	header_size_ = src.header_size_;
	segment_alignment_ = src.segment_alignment_;
	file_alignment_ = src.file_alignment_;
	characterictics_ = src.characterictics_;
	image_type_ = src.image_type_;
	check_sum_ = src.check_sum_;
	low_resize_header_ = src.low_resize_header_;
	resize_header_ = src.resize_header_;
	operating_system_version_ = src.operating_system_version_;
	subsystem_version_ = src.subsystem_version_;
	dll_characteristics_ = src.dll_characteristics_;
	time_stamp_ = src.time_stamp_;

	directory_list_ = src.directory_list_->Clone(this);
	segment_list_ = src.segment_list_->Clone(this);
	section_list_ = src.section_list_->Clone(this);
	import_list_ = src.import_list_->Clone(this);
	export_list_ = src.export_list_->Clone(this);
	fixup_list_ = src.fixup_list_->Clone();
	relocation_list_ = src.relocation_list_->Clone();
	resource_list_ = src.resource_list_->Clone(this);
	load_config_directory_ = src.load_config_directory_->Clone();
	runtime_function_list_ = src.runtime_function_list_->Clone();
	tls_directory_ = src.tls_directory_->Clone();
	debug_directory_ = src.debug_directory_->Clone();
	delay_import_list_ = src.delay_import_list_->Clone();

	if (src.function_list_)
		function_list_ = src.function_list_->Clone(this);
	if (src.virtual_machine_list_)
		virtual_machine_list_ = src.virtual_machine_list_->Clone();
	if (src.resource_section_)
		resource_section_ = segment_list_->item(src.segment_list_->IndexOf(src.resource_section_));
	if (src.fixup_section_)
		fixup_section_ = segment_list_->item(src.segment_list_->IndexOf(src.fixup_section_));

	for (i = 0; i < src.section_list()->count(); i++) {
		PESegment *segment = src.section_list()->item(i)->parent();
		if (segment)
			section_list_->item(i)->set_parent(segment_list_->item(src.segment_list_->IndexOf(segment)));
	}

	for (i = 0; i < src.import_list_->count(); i++) {
		PEImport *import = src.import_list_->item(i);
		for (j = 0; j < import->count(); j++) {
			MapFunction *map_function = import->item(j)->map_function();
			if (map_function)
				import_list_->item(i)->item(j)->set_map_function(map_function_list()->item(src.map_function_list()->IndexOf(map_function)));
		}
	}

	if (function_list_) {
		for (i = 0; i < function_list_->count(); i++) {
			IntelFunction *func = reinterpret_cast<IntelFunction *>(function_list_->item(i));
			for (j = 0; j < func->count(); j++) {
				IntelCommand *command = func->item(j);

				if (command->seh_handler())
					command->set_seh_handler(seh_handler_list()->GetHandlerByAddress(command->address()));

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

PEArchitecture::~PEArchitecture()
{ 
	delete export_list_;
	delete import_list_;
	delete segment_list_;
	delete section_list_;
	delete directory_list_;
	delete fixup_list_;
	delete relocation_list_;
	delete resource_list_;
	delete load_config_directory_;
	delete runtime_function_list_;
	delete function_list_;
	delete virtual_machine_list_;
	delete tls_directory_;
	delete debug_directory_;
	delete delay_import_list_;
}

PEArchitecture *PEArchitecture::Clone(IFile *file) const
{
	PEArchitecture *arch = new PEArchitecture(dynamic_cast<PEFile *>(file), *this);
	return arch;
}

std::string PEArchitecture::name() const
{
	switch (cpu_) {
	case IMAGE_FILE_MACHINE_I386:
		return std::string("i386");
	case IMAGE_FILE_MACHINE_R3000:
	case IMAGE_FILE_MACHINE_R4000:
	case IMAGE_FILE_MACHINE_R10000:
	case IMAGE_FILE_MACHINE_MIPS16:
	case IMAGE_FILE_MACHINE_MIPSFPU:
	case IMAGE_FILE_MACHINE_MIPSFPU16:
		return std::string("mips");
	case IMAGE_FILE_MACHINE_WCEMIPSV2:
		return std::string("mips_wce_v2");
	case IMAGE_FILE_MACHINE_ALPHA:
		return std::string("alpha_axp");
	case IMAGE_FILE_MACHINE_SH3:
	case IMAGE_FILE_MACHINE_SH3DSP:
		return std::string("sh3");
	case IMAGE_FILE_MACHINE_SH3E:
		return std::string("sh3e");
	case IMAGE_FILE_MACHINE_SH4:
		return std::string("sh4");
	case IMAGE_FILE_MACHINE_SH5:
		return std::string("sh5");
	case IMAGE_FILE_MACHINE_ARM:
		return std::string("arm");
	case IMAGE_FILE_MACHINE_THUMB:
		return std::string("thumb");
	case IMAGE_FILE_MACHINE_AM33:
		return std::string("am33");
	case IMAGE_FILE_MACHINE_POWERPC:
	case IMAGE_FILE_MACHINE_POWERPCFP:
		return std::string("ppc");
	case IMAGE_FILE_MACHINE_IA64:
		return std::string("ia64");
	case IMAGE_FILE_MACHINE_ALPHA64:
		return std::string("alpha64");
	case IMAGE_FILE_MACHINE_TRICORE:
		return std::string("infineon");
	case IMAGE_FILE_MACHINE_CEF:
		return std::string("cef");
	case IMAGE_FILE_MACHINE_EBC:
		return std::string("ebc");
	case IMAGE_FILE_MACHINE_AMD64:
		return std::string("amd64");
	case IMAGE_FILE_MACHINE_M32R:
		return std::string("m32r");
	case IMAGE_FILE_MACHINE_CEE:
		return std::string("cee");
	default:
		return string_format("unknown 0x%X", cpu_);
	}
}

OpenStatus PEArchitecture::ReadFromFile(uint32_t mode)
{
	Seek(0);

	IMAGE_DOS_HEADER dos_header;
	if (size() < sizeof(dos_header))
		return osUnknownFormat;

	Read(&dos_header, sizeof(dos_header));
	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		return osUnknownFormat;

	Seek(dos_header.e_lfanew);
	uint32_t signature = ReadDWord();
	if (signature != IMAGE_NT_SIGNATURE)
		return osUnknownFormat;

	IMAGE_FILE_HEADER image_header;
	Read(&image_header, sizeof(image_header));
	cpu_ = image_header.Machine;
	if (cpu_ != IMAGE_FILE_MACHINE_I386 && cpu_ != IMAGE_FILE_MACHINE_AMD64)
		return osUnsupportedCPU;

	uint16_t magic = ReadWord();
	assert(sizeof(magic) == sizeof(IMAGE_OPTIONAL_HEADER32().Magic));
	assert(sizeof(magic) == sizeof(IMAGE_OPTIONAL_HEADER64().Magic));

	uint16_t subsystem;
	uint32_t dir_count;
	switch (magic) {
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		{
			IMAGE_OPTIONAL_HEADER32 pe_header = IMAGE_OPTIONAL_HEADER32();
			Read(&pe_header.MajorLinkerVersion, sizeof(pe_header) - sizeof(magic) - sizeof(pe_header.DataDirectory));
			dir_count = pe_header.NumberOfRvaAndSizes;
			entry_point_ = pe_header.AddressOfEntryPoint;
			image_base_ = pe_header.ImageBase;
			segment_alignment_ = pe_header.SectionAlignment;
			file_alignment_ = pe_header.FileAlignment;
			cpu_address_size_ = osDWord;
			subsystem = pe_header.Subsystem;
			check_sum_ = pe_header.CheckSum;
			operating_system_version_ = (pe_header.MajorOperatingSystemVersion << 16) | pe_header.MinorOperatingSystemVersion;
			subsystem_version_= (pe_header.MajorSubsystemVersion << 16) | pe_header.MinorSubsystemVersion;
			dll_characteristics_ = pe_header.DllCharacteristics;
		}
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		{
			IMAGE_OPTIONAL_HEADER64 pe_header = IMAGE_OPTIONAL_HEADER64();
			Read(&pe_header.MajorLinkerVersion, sizeof(pe_header) - sizeof(magic) - sizeof(pe_header.DataDirectory));
			dir_count = pe_header.NumberOfRvaAndSizes;
			entry_point_ = pe_header.AddressOfEntryPoint;
			image_base_ = pe_header.ImageBase;
			segment_alignment_ = pe_header.SectionAlignment;
			file_alignment_ = pe_header.FileAlignment;
			cpu_address_size_ = osQWord;
			subsystem = pe_header.Subsystem;
			check_sum_ = pe_header.CheckSum;
			operating_system_version_ = (pe_header.MajorOperatingSystemVersion << 16) | pe_header.MinorOperatingSystemVersion;
			subsystem_version_= (pe_header.MajorSubsystemVersion << 16) | pe_header.MinorSubsystemVersion;
			dll_characteristics_ = pe_header.DllCharacteristics;
		}
		break;

	default:
		return osInvalidFormat;
	}

	time_stamp_ = image_header.TimeDateStamp;
	characterictics_ = image_header.Characteristics;
	switch (subsystem) {
	case IMAGE_SUBSYSTEM_NATIVE:
		image_type_ = itDriver;
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		image_type_ = (characterictics_ & IMAGE_FILE_DLL) ? itLibrary : itExe;
		break;
	default:
		return osUnsupportedSubsystem;
	}

	if (entry_point_)
		entry_point_ += image_base_;

	header_offset_ = dos_header.e_lfanew;
	header_size_ = image_header.SizeOfOptionalHeader;

	directory_list_->ReadFromFile(*this, dir_count);

	Seek(header_offset_ + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size_);
	segment_list_->ReadFromFile(*this, image_header.NumberOfSections);

	// read long section names from the COFF string table 
	if (image_header.PointerToSymbolTable) {
		Seek(image_header.PointerToSymbolTable + image_header.NumberOfSymbols * sizeof(IMAGE_SYMBOL));

		COFFStringTable string_table;
		string_table.ReadFromFile(*this);
		for (size_t i = 0; i < segment_list_->count(); i++) {
			PESegment *segment = segment_list_->item(i);
			const std::string &segment_name = segment->name();
			if (segment_name.length() && segment_name[0] == '/') {
				char *endptr = NULL;
				uint32_t name_offset = strtoul(segment_name.c_str() + 1, &endptr, 10);
				if (endptr && *endptr == 0)
					segment->set_name(string_table.GetString(name_offset));
			}
		}
	}

	resource_section_ = NULL;
	fixup_section_ = NULL;
	for (size_t i = 0; i < directory_list_->count(); i++) {
		PEDirectory *dir = directory_list_->item(i);
		switch (dir->type()) {
		case IMAGE_DIRECTORY_ENTRY_EXPORT:
			export_list_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_IMPORT:
			import_list_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_RESOURCE:
			resource_list_->ReadFromFile(*this, *dir);
			if (dir->address()) {
				resource_section_ = segment_list_->GetSectionByAddress(dir->address());
				if (!resource_section_)
					return osInvalidFormat;

				if (resource_section_->address() != dir->address() || resource_section_ == segment_list_->header_segment())
					resource_section_ = NULL;
			}
			break;
		case IMAGE_DIRECTORY_ENTRY_BASERELOC:
			fixup_list_->ReadFromFile(*this, *dir);
			if (dir->address()) {
				fixup_section_ = segment_list_->GetSectionByAddress(dir->address());
				if (!fixup_section_)
					return osInvalidFormat;

				if (fixup_section_->address() != dir->address() || fixup_section_ == segment_list_->header_segment())
					fixup_section_ = NULL;
			}
			break;
		case IMAGE_DIRECTORY_ENTRY_DEBUG:
			debug_directory_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_TLS:
			tls_directory_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
			load_config_directory_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
			runtime_function_list_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
			delay_import_list_->ReadFromFile(*this, *dir);
			break;
		case IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR:
			if (dir->address()) {
				NETArchitecture *net = new NETArchitecture(reinterpret_cast<PEFile *>(owner()));
				owner()->AddObject(net);
				OpenStatus res = net->ReadFromFile(mode);
				return res;

				/*
				if (res != osSuccess)
					return res;

				if ((net->header().Flags & COMIMAGE_FLAGS_ILONLY) == 0) {
					switch (cpu_) {
					case IMAGE_FILE_MACHINE_I386:
					case IMAGE_FILE_MACHINE_AMD64:
						function_list_ = new PEIntelFunctionList(this);
						break;
					default:
						return osUnsupportedCPU;
					}

					if ((mode & foHeaderOnly) == 0) {
						map_function_list()->ReadFromFile(*this);

						IntelFileHelper helper;
						helper.Parse(*this);
					}
				}
				return osSuccess;
				*/
			}
			break;
		}
	}
	if (fixup_section_)
		fixup_section_->set_need_parse(false);
	if (resource_section_)
		resource_section_->set_need_parse(false);

	if ((mode & foHeaderOnly) == 0) {
		if (!owner()->file_name().empty()) {
			std::vector<uint64_t> segments;
			for (size_t i = 0; i < segment_list()->count(); i++) {
				segments.push_back(segment_list()->item(i)->address());
			}
			if (std::find(segments.begin(), segments.end(), 0) == segments.end())
				segments.insert(segments.begin(), 0);

			for (size_t i = 0; i < 3; i++) {
				if (i == 0) {
					MapFile map_file;
					if (map_file.Parse(map_file_name().c_str(), segments)) {
						ReadMapFile(map_file);
						break;
					}
				} else if (i == 1) {
					PDBFile pdb_file;
					if (pdb_file.Parse(pdb_file_name().c_str(), segments)) {
						std::vector<uint8_t> guid;
						if (pdb_file.guid().size()) {
							for (size_t j = 0; j < debug_directory_->count(); j++) {
								PEDebugData *data = debug_directory_->item(j);
								if (data->type() == IMAGE_DEBUG_TYPE_CODEVIEW) {
									if (AddressSeek(data->address())) {
										struct PdbInfo
										{
											uint32_t     Signature;
											GUID         Guid;
											uint32_t     Age;
											// char      PdbFileName[1];
										} pi;
										Read(&pi, sizeof(pi));
										if (pi.Signature == 0x53445352) // RSDS
											guid.insert(guid.begin(), reinterpret_cast<const uint8_t *>(&pi.Guid), reinterpret_cast<const uint8_t *>(&pi.Guid) + sizeof(pi.Guid));
									}
									break;
								}
							}
						}
						if (guid == pdb_file.guid()) {
							pdb_file.set_time_stamp(time_stamp_);
							ReadMapFile(pdb_file);
						} else
							Notify(mtWarning, NULL, string_format(language[lsMAPFileHasIncorrectTimeStamp].c_str(), os::ExtractFileName(pdb_file.file_name().c_str()).c_str()));
						break;
					}
				}
				else if (image_header.PointerToSymbolTable) {
					COFFFile coff_file;
					if (coff_file.Parse(owner()->file_name().c_str(), segments)) {
						ReadMapFile(coff_file);
						break;
					}
				}
			}
		}

		map_function_list()->ReadFromFile(*this);
	}

	switch (cpu_) {
	case IMAGE_FILE_MACHINE_I386:
	case IMAGE_FILE_MACHINE_AMD64:
		function_list_ = new PEIntelFunctionList(this);
		virtual_machine_list_ = new IntelVirtualMachineList();
		if ((mode & foHeaderOnly) == 0) {
			IntelFileHelper helper;
			helper.Parse(*this);

			relocation_list_->ReadFromFile(*this);
		}
		break;
	default:
		return osUnsupportedCPU;
	}

	return osSuccess;
}

std::string PEArchitecture::pdb_file_name() const
{
	if (!owner())
		return std::string();
	return os::ChangeFileExt(owner()->file_name().c_str(), ".pdb");
}

bool PEArchitecture::ReadMapFile(IMapFile &map_file)
{
	if (!BaseArchitecture::ReadMapFile(map_file))
		return false;

	MapSection *sections = map_file.GetSectionByType(msSections);
	if (sections) {
		for (size_t i = 0; i < sections->count(); i++) {
			MapObject *section = sections->item(i);

			uint64_t address = section->address();
			PESegment *segment;
			if (section->segment() != (size_t)-1) { 
				if (section->segment() == 0 || section->segment() > segment_list_->count())
					continue;

				segment = segment_list_->item(section->segment() - 1);
			} else {
				segment = segment_list_->GetSectionByAddress(address);
				if (!segment)
					continue;
			}

			section_list_->Add(segment, address, section->size(), section->name());
		}
	}
	return true;
}

void PEArchitecture::WriteCheckSum()
{
	if (check_sum_ && reinterpret_cast<PEFile*>(owner())->GetCheckSum(&check_sum_)) {
		Seek(header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + ((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum) : offsetof(IMAGE_OPTIONAL_HEADER64, CheckSum)));
		WriteDWord(check_sum_);
	}
}

bool PEArchitecture::WriteToFile()
{
	IMAGE_FILE_HEADER image_header;
	IMAGE_OPTIONAL_HEADER32 pe_header32;
	IMAGE_OPTIONAL_HEADER64 pe_header64;
	uint32_t image_size, header_size;
	PESegment *last_section;

	// read header
	Seek(header_offset_ + sizeof(uint32_t));
	Read(&image_header, sizeof(image_header));
	image_header.PointerToSymbolTable = 0;
	image_header.NumberOfSymbols = 0;
	image_header.NumberOfSections = static_cast<uint16_t>(segment_list_->count());
	if (cpu_address_size_ == osDWord) {
		Read(&pe_header32, sizeof(pe_header32) - sizeof(pe_header32.DataDirectory));
	} else {
		Read(&pe_header64, sizeof(pe_header64) - sizeof(pe_header64.DataDirectory));
	}

	// write header
	directory_list_->WriteToFile(*this);
	segment_list_->WriteToFile(*this);
	header_size = AlignValue(static_cast<uint32_t>(Tell()), file_alignment_);

	last_section = segment_list_->last();
	image_size = (last_section) ? AlignValue(static_cast<uint32_t>(last_section->address() - image_base() + last_section->size()), segment_alignment_) : 0;

	Seek(header_offset_ + sizeof(uint32_t));
	image_header.Characteristics = characterictics_;
	Write(&image_header, sizeof(image_header));
	if (cpu_address_size_ == osDWord) {
		pe_header32.AddressOfEntryPoint = (entry_point_) ? static_cast<uint32_t>(entry_point_ - image_base_) : 0;
		pe_header32.SizeOfImage = image_size;
		if (header_size > pe_header32.SizeOfHeaders)
			pe_header32.SizeOfHeaders = header_size;
		pe_header32.MajorOperatingSystemVersion = operating_system_version_ >> 16;
		pe_header32.MinorOperatingSystemVersion = static_cast<uint16_t>(operating_system_version_);
		pe_header32.MajorSubsystemVersion = subsystem_version_ >> 16;
		pe_header32.MinorSubsystemVersion = static_cast<uint16_t>(subsystem_version_);
		pe_header32.DllCharacteristics = dll_characteristics_;
		Write(&pe_header32, sizeof(pe_header32) - sizeof(pe_header32.DataDirectory));
	} else {
		pe_header64.AddressOfEntryPoint = (entry_point_) ? static_cast<uint32_t>(entry_point_ - image_base_) : 0;
		pe_header64.SizeOfImage = image_size;
		if (header_size > pe_header64.SizeOfHeaders)
			pe_header64.SizeOfHeaders = header_size;
		pe_header64.MajorOperatingSystemVersion = operating_system_version_ >> 16;
		pe_header64.MinorOperatingSystemVersion = static_cast<uint16_t>(operating_system_version_);
		pe_header64.MajorSubsystemVersion = subsystem_version_ >> 16;
		pe_header64.MinorSubsystemVersion = static_cast<uint16_t>(subsystem_version_);
		pe_header64.DllCharacteristics = dll_characteristics_;
		Write(&pe_header64, sizeof(pe_header64) - sizeof(pe_header64.DataDirectory));
	}
	return true;
}

bool PEArchitecture::Prepare(CompileContext &ctx)
{
	if ((ctx.options.flags & cpPack) && segment_alignment_ < 0x1000) {
		ctx.options.flags &= ~cpPack;
		Notify(mtWarning, NULL, language[lsFileCanNotBePacked].c_str());
	}

	if (image_type() == itExe && (dll_characteristics_ & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) == 0)
		ctx.options.flags |= cpStripFixups;

	if (image_type() != itDriver) {
		if (ctx.options.flags & cpMemoryProtection)
			ctx.options.flags |= cpInternalMemoryProtection;
		if (ctx.options.flags & cpResourceProtection) {
			std::vector<IResource*> resources = resource_list()->GetResourceList();
			bool need_resource_protection = false;
			for (size_t i = 0; i < resources.size(); i++) {
				IResource *resource = resources[i];
				if (!resource->excluded_from_packing() && !resource->need_store()) {
					need_resource_protection = true;
					break;
				}
			}
			if (!need_resource_protection)
				ctx.options.flags &= ~cpResourceProtection;
		}
	} else {
		if (ctx.options.flags & cpResourceProtection)
			ctx.options.flags &= ~cpResourceProtection;
#ifdef ULTIMATE
		if (ctx.options.file_manager)
			ctx.options.file_manager = NULL;
#endif
	}

	if (!BaseArchitecture::Prepare(ctx))
		return false;

	size_t i;
	PESegment *section;
	std::vector<PESegment *> optimized_section_list;

	// optimize sections
	if (resource_section_)
		optimized_section_list.push_back(resource_section_);
	if (fixup_section_)
		optimized_section_list.push_back(fixup_section_);
	if (ctx.options.flags & cpStripDebugInfo) {
		for (i = 0; i < segment_list_->count(); i++) {
			section = segment_list_->item(i);
			if ((section->flags() & (IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_LNK_INFO)) == IMAGE_SCN_MEM_DISCARDABLE && section->name().substr(0, 6) == ".debug")
				optimized_section_list.push_back(section);
		}
	}

	optimized_section_count_ = segment_list_->count();
	for (i = segment_list_->count(); i > 0; i--) {
		section = segment_list_->item(i - 1);

		std::vector<PESegment *>::iterator it = std::find(optimized_section_list.begin(), optimized_section_list.end(), section);
		if (it != optimized_section_list.end()) {
			optimized_section_list.erase(it);
			optimized_section_count_--;
		} else {
			break;
		}
	}

	// calc new header size
	uint32_t new_section_count = static_cast<uint32_t>(optimized_section_count_ + 1);
	if ((ctx.options.flags & cpStripFixups) == 0)
		new_section_count++;
	if (resource_list_->count())
		new_section_count++;
	if (ctx.runtime)
		new_section_count += 2;

	// calc header resizes
	uint32_t new_header_size = header_offset_ + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size_ + new_section_count * sizeof(IMAGE_SECTION_HEADER);
	low_resize_header_ = 0;
	if ((ctx.options.flags & cpStripDebugInfo) && header_offset_ > MIN_HEADER_OFFSET) {
		low_resize_header_ = header_offset_ - MIN_HEADER_OFFSET;
		new_header_size -= low_resize_header_;
	}
	for (i = 0; i < directory_list_->count(); i++) {
		PEDirectory *dir = directory_list_->item(i);
		if (!dir->visible() || dir->type() == IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT)
			continue;

		uint32_t rva = static_cast<uint32_t>(dir->address() - image_base_);
		if (low_resize_header_ == 0 && new_header_size > rva && header_offset_ > MIN_HEADER_OFFSET) {
			low_resize_header_ = header_offset_ - MIN_HEADER_OFFSET;
			new_header_size -= low_resize_header_;
		}
		if (new_header_size > rva) {
			Notify(mtError, NULL, language[lsCreateSegmentError]);
			return false;
		}
	}

	uint32_t aligned_header_size = AlignValue(new_header_size, file_alignment_);
	resize_header_ = 0;
	for (i = 0; i < segment_list_->count(); i++) {
		section = segment_list_->item(i);
		if (section->physical_size() == 0 || section->physical_offset() == 0 || aligned_header_size <= section->physical_offset())
			continue;

		uint32_t rva = static_cast<uint32_t>(section->address() - image_base_);
		if (low_resize_header_ == 0 && aligned_header_size > rva && header_offset_ > MIN_HEADER_OFFSET) {
			low_resize_header_ = header_offset_ - MIN_HEADER_OFFSET;
			new_header_size -= low_resize_header_;
			aligned_header_size = AlignValue(new_header_size, file_alignment_);
		}
		if (aligned_header_size > rva) {
			Notify(mtError, NULL, language[lsCreateSegmentError]);
			return false;
		}
		if (aligned_header_size > section->physical_offset())
			resize_header_ = std::max(resize_header_, aligned_header_size - section->physical_offset());
	}

	for (i = 0; i < optimized_section_list.size(); i++) {
		section = optimized_section_list[i];
		ctx.manager->Add(section->address(), std::min(static_cast<uint32_t>(section->size()), section->physical_size()), section->memory_type());
	}

	if (optimized_section_count_ > 0) {
		section = segment_list_->item(optimized_section_count_ - 1);
		if (ctx.runtime) {
			PEArchitecture *runtime = reinterpret_cast<PEArchitecture *>(ctx.runtime);
			if (runtime->segment_list()->count()) {
				runtime->Rebase(image_base(), AlignValue(section->address() + section->size(), segment_alignment()) - runtime->segment_list()->item(0)->address());

				MemoryManager runtime_manager(runtime);
				if (runtime->segment_list()->last() == runtime->fixup_section_) {
					delete runtime->fixup_section_;
					runtime->fixup_section_ = NULL;
				}
				if (runtime->load_config_directory()->seh_table_address()) {
					section = runtime->segment_list()->last();
					if (section->address() == runtime->load_config_directory()->seh_table_address())
						delete section;
				}
				if (runtime->runtime_function_list()->address()) {
					section = runtime->segment_list()->last();
					if (section->address() == runtime->runtime_function_list()->address())
						delete section;
					else 
						runtime->runtime_function_list()->FreeByManager(runtime_manager);
				}
				runtime->import_list()->FreeByManager(runtime_manager, (ctx.options.flags & cpImportProtection) != 0);
				runtime_manager.Pack();
				for (i = 0; i < runtime_manager.count(); i++) {
					MemoryRegion *region = runtime_manager.item(i);
					ctx.manager->Add(region->address(), region->size(), region->type());
				}
				section = runtime->segment_list()->last();
			} else {
				runtime->Rebase(image_base(), image_base() - runtime->image_base());
			}
		}

		// add new section
		assert(section);
		ctx.manager->Add(AlignValue(section->address() + section->size(), segment_alignment()), UINT32_MAX,  mtReadable | mtExecutable | mtWritable | mtNotPaged | (runtime_function_list()->count() ? mtSolid : mtNone));
	}

	if (ctx.runtime)
		import_list_->FreeByManager(*ctx.manager, (ctx.options.flags & cpImportProtection) != 0);
	if (ctx.options.flags & cpPack) {
		export_list_->FreeByManager(*ctx.manager);
		tls_directory_->FreeByManager(*ctx.manager);
		PEDirectory *dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
		if (dir)
			dir->FreeByManager(*ctx.manager);
		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_ARCHITECTURE);
		if (dir)
			dir->FreeByManager(*ctx.manager);
	}
	if (ctx.options.flags & (cpPack | cpStripDebugInfo))
		debug_directory_->FreeByManager(*ctx.manager);
	load_config_directory_->FreeByManager(*ctx.manager);
	runtime_function_list_->FreeByManager(*ctx.manager);

	return true;
}

bool PEArchitecture::Compile(CompileOptions &options, IArchitecture *runtime)
{
	return visible() ? BaseArchitecture::Compile(options, runtime) : true;
}

void PEArchitecture::Save(CompileContext &ctx)
{
	PEDirectory *dir;
	PESegment *last_section, *section, *vmp_section;
	uint64_t pos, address, resource_section_info, resource_packer_info, file_crc_address, loader_crc_address, name_table,
		loader_crc_size_address, loader_crc_hash_address, file_crc_size_address;
	uint32_t size, file_crc_size, loader_crc_size, name_table_size;
	size_t i, j, c;
	MemoryRegion *region;
	uint32_t resource_section_flags, fixup_section_flags;
	std::string resource_section_name, fixup_section_name;
	int vmp_index;
	MemoryManager *manager = memory_manager();

	// erase sections area
	{
		IMAGE_SECTION_HEADER section_header = IMAGE_SECTION_HEADER();
		Seek(header_offset_ + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size_);
		for (i = 0; i < segment_list_->count(); i++) {
			Write(&section_header, sizeof(section_header));
		}
	}
	
	// resize header
	if (low_resize_header_ || resize_header_) {
		uint32_t new_header_offset = header_offset_ - low_resize_header_;
		size_t total_header_size = offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + header_size_ + segment_list_->count() * sizeof(IMAGE_SECTION_HEADER);
		const PEArchitecture *src = dynamic_cast<const PEArchitecture *>(source());
		if (low_resize_header_) {
			Seek(offsetof(IMAGE_DOS_HEADER, e_lfanew));
			WriteDWord(new_header_offset);
			src->Seek(header_offset_);
			Seek(new_header_offset);
			CopyFrom(*src, total_header_size);
			for (i = 0; i < low_resize_header_; i++) {
				WriteByte(0);
			}
		}
		if (resize_header_) {
			src->Seek(header_offset_ + total_header_size);
			Seek(header_offset_ + total_header_size);
			for (i = 0; i < resize_header_; i++) {
				WriteByte(0);
			}
			CopyFrom(*src, this->size() - src->Tell());
			section = segment_list_->header_segment();
			if (section)
				section->set_physical_size(section->physical_size() + resize_header_);
			for (i = 0; i < segment_list_->count(); i++) {
				section = segment_list_->item(i);
				if (section->physical_offset())
					section->set_physical_offset(section->physical_offset() + resize_header_);
			}

			if ((ctx.options.flags & (cpPack | cpStripDebugInfo)) == 0) {
				if (debug_directory_->address()) {
					for (i = 0; i < debug_directory_->count(); i++) {
						PEDebugData *data = debug_directory_->item(i);
						data->set_offset(data->offset() + resize_header_);
					}
					AddressSeek(debug_directory_->address());
					debug_directory_->WriteToFile(*this);
				}
			}
		}
		header_offset_ = new_header_offset;
	}

	// add antidebug export
	if ((ctx.options.flags & cpCheckDebugger) && image_type() != itDriver && !import_list()->GetImportByName("dbghelp.dll"))
		export_list_->AddAntidebug();

	// compile modified objects
	if ((ctx.options.flags & cpPack) == 0) {
		if (source() && !export_list()->is_equal(*source()->export_list())) {
			const PEArchitecture *src = dynamic_cast<const PEArchitecture *>(source());
			if(src == NULL)
				throw std::runtime_error("Runtime error at Save");

			src->export_list()->FreeByManager(*manager);

			PEIntelExport *pe_export = reinterpret_cast<PEIntelFunctionList *>(function_list())->AddExport(cpu_address_size());
			pe_export->Init(ctx);
			pe_export->Compile(ctx);

			PEDirectory *dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_EXPORT);
			if (dir) {
				if (pe_export->entry()) {
					dir->set_address(pe_export->entry()->address());
					dir->set_size(pe_export->size());
				} else {
					dir->clear();
				}
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

	if (resource_list_->count()) {
		if (ctx.options.flags & cpResourceProtection) {
			for (i = resource_list_->count(); i > 0; i--) {
				PEResource *resource = resource_list_->item(i - 1);
				if (!resource->need_store())
					delete resource;
			}
		}
		resource_list_->Compile(*this, (resource_section_ && resource_section_->excluded_from_packing()) ? false : (ctx.options.flags & cpPack) != 0);
	}

	resource_section_flags = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
	resource_section_name = ".rsrc";

	fixup_section_flags = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA;
	fixup_section_name = ".reloc";

	// need erase optimized sections
	for (i = segment_list_->count(); i > optimized_section_count_; i--) {
		section = segment_list_->item(i - 1);
		if (resource_section_ == section) {
			resource_section_flags = section->flags();
			resource_section_name = section->name();
			resource_section_ = NULL;
		} else if (fixup_section_ == section) {
			fixup_section_flags = section->flags();
			fixup_section_name = section->name();
			fixup_section_ = NULL;
		}
		delete section;
	}

	// need truncate optimized sections and overlay
	for (i = segment_list_->count(); i > 0; i--) {
		section = segment_list_->item(i - 1);
		if (section->physical_size() > 0) {
			Resize(section->physical_offset() + section->physical_size());
			break;
		}
	}

	last_section = segment_list_->last();
	address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);
	pos = Resize(AlignValue(this->size(), file_alignment_));
	vmp_section = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE, "");

	// merge runtime objects
	PEArchitecture *runtime = reinterpret_cast<PEArchitecture*>(ctx.runtime);
	if (runtime && runtime->segment_list()->count()) {
		// merge sections
		SignatureList patch_signatures;
		if (image_type() != itDriver) {
			if (cpu_address_size() == osDWord)
				patch_signatures.Add("817DD800000001741B83C8FF8B4DF0", 0);
		}
		for (i = 0; i < runtime->segment_list()->count(); i++) {
			section = runtime->segment_list()->item(i);
			if (section->physical_offset() && section->physical_size()) {
				runtime->Seek(section->physical_offset());
				size = static_cast<uint32_t>(section->physical_size());
				uint8_t *buffer = new uint8_t[size];
				runtime->Read(buffer, size);
				if ((section->memory_type() & mtExecutable) && patch_signatures.count()) {
					patch_signatures.InitSearch();
					for (c = 0; c < size; c++) {
						uint8_t b = buffer[c];
						for (j = 0; j < patch_signatures.count(); j++) {
							Signature *sign = patch_signatures.item(j);
							if (sign->SearchByte(b)) {
								size_t p = c + 1 - sign->size();
								buffer[p + 7] = 0xeb;
							}
						}
					}
				}

				Write(buffer, size);
				delete [] buffer;
			}
			size = static_cast<uint32_t>(AlignValue(section->size(), runtime->segment_alignment()) - section->physical_size());
			for (j = 0; j < size; j++) {
				WriteByte(0);
			}
			uint32_t memory_type = section->memory_type();
			if (image_type_ != itDriver)
				 memory_type &= ~mtWritable;
			vmp_section->include_write_type(memory_type);

			StepProgress();
		}
		// merge fixups
		for (i = 0; i < runtime->fixup_list()->count(); i++) {
			PEFixup *fixup = runtime->fixup_list()->item(i);
			fixup_list_->AddObject(fixup->Clone(fixup_list_));
		}
		// merge seh handlers
		for (i = 0; i < runtime->seh_handler_list()->count(); i++) {
			PESEHandler *handler = runtime->seh_handler_list()->item(i);
			load_config_directory_->seh_handler_list()->Add(handler->address());
		}
		// merge CFG addresses
		PECFGAddressTable *cfg_address_list = runtime->load_config_directory_->cfg_address_list();
		for (i = 0; i < cfg_address_list->count(); i++) {
			load_config_directory_->cfg_address_list()->Add(cfg_address_list->item(i)->address());
		}
		// merge runtime functions
		for (i = 0; i < runtime->runtime_function_list()->count(); i++) {
			PERuntimeFunction *runtime_function = runtime->runtime_function_list()->item(i);
			runtime_function_list_->AddObject(runtime_function->Clone(runtime_function_list_));
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
				WriteByte((ctx.options.flags & cpDebugMode) ? 0xcc : rand());
			}
		}
	}

	vmp_index = 0;
	// need update fixup and resource sections if they are not optimized
	if (fixup_section_) {
		fixup_section_->set_name(string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
		if (fixup_section_->write_type()) {
			fixup_section_->set_flags(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA);
			fixup_section_->update_type(fixup_section_->write_type());
		}
		fixup_section_ = NULL;
	}
	if (resource_section_) {
		resource_section_->set_name(string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
		if (resource_section_->write_type()) {
			resource_section_->set_flags(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA);
			resource_section_->update_type(resource_section_->write_type());
		}
		resource_section_ = NULL;
	}

	if (!runtime) {
		last_section = segment_list_->last();
		if (import_list_->has_sdk())
			import_list_->WriteToFile(*this, true);
		if (load_config_directory_->WriteToFile(*this))
			last_section->include_write_type(mtReadable | mtNotDiscardable);

		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
		if (dir) {
			pos = Resize(AlignValue(this->size(), 0x10));
			address = last_section->address() + pos - last_section->physical_offset();

			size = static_cast<uint32_t>(runtime_function_list_->WriteToFile(*this));
			if (size) {
				last_section->include_write_type(mtReadable | mtNotDiscardable | mtNotPaged);
				dir->set_address(address);
				dir->set_size(size);
			} else {
				dir->clear();
			}
		}
	}

	if (vmp_section->write_type() == mtNone) {
		delete vmp_section;
	} else {
		vmp_section->set_name(string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
	
		size = static_cast<uint32_t>(this->size() - vmp_section->physical_offset());
		vmp_section->set_size(size);
		vmp_section->set_physical_size(AlignValue(size, file_alignment_));
		vmp_section->update_type(vmp_section->write_type());

		Resize(vmp_section->physical_offset() + vmp_section->physical_size());
	}

	if ((ctx.options.flags & cpPack) && ctx.options.script)
		ctx.options.script->DoBeforePackFile();

	// write memory CRC table
	if (function_list_->crc_table()) {
		IntelCRCTable *intel_crc = reinterpret_cast<IntelCRCTable *>(function_list_->crc_table());
		CRCTable crc_table(function_list_->crc_cryptor(), intel_crc->table_size());

		// add non writable sections
		for (i = 0; i < segment_list_->count(); i++) {
			section = segment_list_->item(i);
			if ((section->memory_type() & (mtReadable | mtWritable)) != mtReadable || section->excluded_from_memory_protection())
				continue;

			size = std::min(static_cast<uint32_t>(section->size()), section->physical_size());
			if (size) {
				crc_table.Add(section->address(), size);
				if (ctx.options.sdk_flags & cpMemoryProtection)
					section->update_type(mtNotPaged);
			}
		}

		// skip writable runtime's sections
		if (runtime) {
			for (i = 0; i < runtime->segment_list()->count(); i++) {
				section = runtime->segment_list()->item(i);
				if (section->memory_type() & mtWritable)
					crc_table.Remove(section->address(), static_cast<uint32_t>(section->size()));
			}
		}

		// skip IAT
		IntelImport *intel_import  = reinterpret_cast<IntelFunctionList *>(function_list_)->import();
		size = OperandSizeToValue(cpu_address_size());
		size_t k = (runtime && runtime->segment_list()->count() > 0) ? 2 : 1;
		for (size_t n = 0; n < k; n++) {
			PEImportList *import_list = (n == 0) ? import_list_ : runtime->import_list();
			for (i = 0; i < import_list->count(); i++) {
				PEImport *import = import_list->item(i);
				if (ctx.options.flags & cpImportProtection) {
					for (j = 0; j < import->count(); j++) {
						PEImportFunction *import_function = import->item(j);
						address = import_function->address();
						IntelCommand *iat_command = intel_import->GetIATCommand(import_function);
						if (iat_command)
							address = iat_command->address();
						crc_table.Remove(address, size);
					}
				} else {
					if (import->count() > 0)
						crc_table.Remove(import->item(0)->address(), size * import->count());
				}
			}
		}

		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < fixup_list_->count(); i++) {
				PEFixup *fixup = fixup_list_->item(i);
				if (!fixup->is_deleted())
					crc_table.Remove(fixup->address(), OperandSizeToValue(fixup->size()));
			}
		}

		// skip relocations
		for (i = 0; i < relocation_list_->count(); i++) {
			PERelocation *relocation = relocation_list_->item(i);
			crc_table.Remove(relocation->address(), OperandSizeToValue(relocation->size()));
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

	resource_section_info = 0;
	resource_packer_info = 0;
	file_crc_address = 0;
	file_crc_size = 0;
	file_crc_size_address = 0;
	loader_crc_address = 0;
	loader_crc_size = 0;
	loader_crc_size_address = 0;
	loader_crc_hash_address = 0;
	name_table = 0;
	name_table_size = 0;
	if (runtime) {
		uint64_t iat_address = 0;
		uint64_t security_cookie_address = 0;

		last_section = segment_list_->last();
		address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);
		pos = Resize(AlignValue(this->size(), file_alignment_));
		size = 0;

		// create segment for IAT
		{
			size_t iat_count = 0;
			for (j = 0; j < 2; j++) {
				PEArchitecture *source_file = (j == 0) ? this : runtime;
				for (i = 0; i < source_file->import_list()->count(); i++) {
					IImport *import = source_file->import_list()->item(i);
					if (import->is_sdk())
						continue;

					iat_count += import->count();
					if (j == 1 && runtime->segment_list()->count())
						iat_count += import->count();
				}
			}

			if (iat_count) {
				iat_count++;
				iat_address = address + size;
				size += static_cast<uint32_t>(iat_count) * OperandSizeToValue(cpu_address_size());
			}
		}

		// create segment for security_cookie
		if ((ctx.options.flags & cpPack) && image_type() == itDriver && load_config_directory_->security_cookie() && AddressSeek(load_config_directory_->security_cookie())) {
			section = segment_list_->GetSectionByAddress(load_config_directory_->security_cookie());
			if (!(section && section->excluded_from_packing())) {
				uint32_t value_size = OperandSizeToValue(cpu_address_size());
				uint64_t security_cookie_value = 0;
				Read(&security_cookie_value, value_size);

				Resize(pos + size);
				Write(&security_cookie_value, value_size);

				security_cookie_address = address + size;
				size += value_size;
			}
		}

		// create segment for tls data
		if ((ctx.options.flags & cpPack) && tls_directory_->end_address_of_raw_data() > tls_directory_->start_address_of_raw_data()) {
			uint32_t data_size = static_cast<uint32_t>(tls_directory_->end_address_of_raw_data() - tls_directory_->start_address_of_raw_data());

			Data data;
			data.resize(data_size);
			if (AddressSeek(tls_directory_->start_address_of_raw_data()))
				Read(&data[0], data.size());

			Resize(pos + size);
			Write(data.data(), data.size());

			uint64_t tls_data_address = address + size;
			for (i = 0; i < data_size; i++) {
				if (IFixup *fixup = fixup_list()->GetFixupByAddress(tls_directory_->start_address_of_raw_data() + i))
					fixup->set_address(tls_data_address + i);
			}
			tls_directory_->set_start_address_of_raw_data(tls_data_address);
			tls_directory_->set_end_address_of_raw_data(tls_data_address + data_size);

			size += data_size;
		}

		if (size) {
			section = segment_list_->Add(address, size, static_cast<uint32_t>(pos), AlignValue(size, file_alignment_),
				IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA,
				string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
			section->set_excluded_from_packing(true);
			Resize(section->physical_offset() + section->physical_size());
		}

		std::vector<IFunction *> processor_list = function_list_->processor_list();
		IntelRuntimeCRCTable *runtime_crc_table = reinterpret_cast<IntelFunctionList *>(function_list_)->runtime_crc_table();
		PEIntelLoader *loader = new PEIntelLoader(NULL, cpu_address_size());
		if (security_cookie_address)
			loader->set_security_cookie(security_cookie_address);
		if (iat_address)
			loader->set_iat_address(iat_address);

		last_section = segment_list_->last();
		address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);

		manager->clear();
		manager->Add(address, UINT32_MAX, mtReadable | mtExecutable | mtWritable | mtNotPaged | (runtime_function_list()->count() ? mtSolid : mtNone));

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
		section = segment_list_->Add(address, UINT32_MAX, static_cast<uint32_t>(pos), UINT32_MAX,
										IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE, 
										string_format("%s%d", ctx.options.section_name.c_str(), vmp_index++));
		c = loader->WriteToFile(*this);
		section->update_type(section->write_type() | ((ctx.options.sdk_flags & cpMemoryProtection) ? mtNotPaged : mtNone));
		for (i = 0; i < processor_list.size(); i++) {
			processor_list[i]->WriteToFile(*this);
		}
		if (runtime_crc_table)
			c += runtime_crc_table->WriteToFile(*this);

		// correct progress position
		write_count -= c;
		if (write_count)
			StepProgress(write_count);

		// copy directories
		{
			IArchitecture *source = const_cast<IArchitecture *>(this->source());
			last_section = segment_list_->last();
			const uint32_t copy_dir_types[] = {IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, IMAGE_DIRECTORY_ENTRY_ARCHITECTURE};
			for (j = 0; j < _countof(copy_dir_types); j++) {
				if (copy_dir_types[j] != IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG && (ctx.options.flags & cpPack) == 0)
					continue;

				dir = directory_list_->GetCommandByType(copy_dir_types[j]);
				if (dir && dir->address() && dir->physical_size() && source->AddressSeek(dir->address())) {
					size = dir->physical_size();
					pos = Resize(AlignValue(this->size(), 0x10));
					CopyFrom(*source, size);
					address = last_section->address() + pos - last_section->physical_offset();
					for (i = 0; i < size; i++) {
						PEFixup *fixup = reinterpret_cast<PEFixup *>(source->fixup_list()->GetFixupByAddress(dir->address() + i));
						if (fixup) {
							fixup = fixup->Clone(fixup_list_);
							fixup->set_address(address + i);
							fixup_list_->AddObject(fixup);
						}
					}
					dir->set_address(address);
				}
			}

			if ((ctx.options.flags & (cpPack | cpStripDebugInfo)) == cpPack) {
				if (debug_directory_->address()) {
					for (i = 0; i < debug_directory_->count(); i++) {
						PEDebugData *data = debug_directory_->item(i);
						if (source->Seek(data->offset())) {
							size = data->size();
							pos = Resize(AlignValue(this->size(), 0x10));
							CopyFrom(*source, size);
							address = last_section->address() + pos - last_section->physical_offset();

							data->set_offset(static_cast<uint32_t>(pos));
							data->set_address(address);
						}
					}
					pos = Resize(AlignValue(this->size(), 0x10));
					address = last_section->address() + pos - last_section->physical_offset();
					debug_directory_->WriteToFile(*this);

					dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DEBUG);
					if (dir)
						dir->set_address(address);
				}
			}
		}

		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
		if (dir) {
			if (security_cookie_address)
				load_config_directory_->set_security_cookie(security_cookie_address);
			std::vector<uint64_t> cfg_address_list = loader->cfg_address_list();
			for (i = 0; i < cfg_address_list.size(); i++) {
				load_config_directory_->cfg_address_list()->Add(cfg_address_list[i]);
			}
			if (loader->cfg_check_function_entry())
				load_config_directory_->set_cfg_check_function(loader->cfg_check_function_entry()->address());
			load_config_directory_->WriteToFile(*this);
		}

		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
		if (dir) {
			pos = Resize(AlignValue(this->size(), 0x10));
			address = section->address() + pos - section->physical_offset();

			size = static_cast<uint32_t>(runtime_function_list_->WriteToFile(*this));
			if (size) {
				section->update_type(mtReadable | mtNotDiscardable | mtNotPaged);
				dir->set_address(address);
				dir->set_size(size);
			} else {
				dir->clear();
			}
		}

		size = static_cast<uint32_t>(this->size() - section->physical_offset());
		section->set_size(size);
		section->set_physical_size(AlignValue(size, file_alignment_));

		Resize(section->physical_offset() + section->physical_size());

		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IMPORT);
		if (dir) {
			dir->set_address(loader->import_entry()->address());
			dir->set_size(loader->import_size());
		}

		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IAT);
		if (dir) {
			dir->set_address(loader->iat_entry()->address());
			dir->set_size(loader->iat_size());
		}
		
		if (loader->export_entry()) {
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_EXPORT);
			if (dir) {
				if (loader->export_size()) {
					dir->set_address(loader->export_entry()->address());
					dir->set_size(loader->export_size());
				} else {
					dir->clear();
				}
			}
		}

		if (loader->tls_entry()) {
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_TLS);
			if (dir) {
				if (loader->tls_size()) {
					dir->set_address(loader->tls_entry()->address());
					dir->set_size(loader->tls_size());
				} else {
					dir->clear();
				}
			}
		}

		if (loader->delay_import_entry()) {
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
			if (dir) {
				if (loader->delay_import_size()) {
					dir->set_address(loader->delay_import_entry()->address());
					dir->set_size(loader->delay_import_size());
				} else {
					dir->clear();
				}
			}
		}

		entry_point_ = loader->entry()->address();

		if (loader->resource_section_info()) {
			resource_section_info = loader->resource_section_info()->address();
			resource_packer_info = loader->resource_packer_info()->address();
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

		if (loader->name_entry()) {
			name_table = loader->name_entry()->address();
			name_table_size = loader->name_size();
		}

		delete loader;

		// update versions
		if (operating_system_version_ < runtime->operating_system_version_)
			operating_system_version_ = runtime->operating_system_version_;
		if (subsystem_version_ < runtime->subsystem_version_)
			subsystem_version_ = runtime->subsystem_version_;

		ctx.file->EndProgress();
	}
	
	// save fixups
	dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_BASERELOC);
	if (dir) {
		if (fixup_list_->Pack() == 0 || (ctx.options.flags & cpStripFixups) != 0) {
			dir->clear();
			fixup_section_ = NULL;
		} else {
			last_section = segment_list_->last();
			address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);

			pos = Resize(AlignValue(this->size(), file_alignment_));
			size = static_cast<uint32_t>(fixup_list_->WriteToFile(*this));
			section = segment_list_->Add(address, size, static_cast<uint32_t>(pos), AlignValue(size, file_alignment_),
											fixup_section_flags, fixup_section_name);
			fixup_section_ = section;

			Resize(section->physical_offset() + section->physical_size());

			dir->set_address(address);
			dir->set_size(size);
		}
	}

	// save resources
	dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if (dir) {
		if (resource_list_->count() == 0) {
			dir->clear();
			resource_section_ = NULL;
		} else {
			last_section = segment_list_->last();
			address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);
			
			pos = Resize(AlignValue(this->size(), file_alignment_));
			address = AlignValue(last_section->address() + last_section->size(), segment_alignment_);
			size = static_cast<uint32_t>(resource_list_->WriteToFile(*this, address));
			section = segment_list_->Add(address, (uint32_t)resource_list_->size(), static_cast<uint32_t>(pos), AlignValue(size, file_alignment_),
											resource_section_flags, resource_section_name);
			resource_section_ = section;

			Resize(section->physical_offset() + section->physical_size());

			dir->set_address(address);
			dir->set_size((uint32_t)resource_list_->size());

			if (resource_section_info) {
				pos = Tell();

				AddressSeek(resource_section_info);
				WriteDWord(static_cast<uint32_t>(address - image_base_));
				WriteDWord(static_cast<uint32_t>(resource_list_->size()));
				WriteDWord(section->flags());

				AddressSeek(resource_packer_info);
				WriteDWord(static_cast<uint32_t>(address - image_base_ + resource_list_->store_size()));

				Seek(pos);
			}
		}
	}

	// clear directories
	{
		const uint32_t clear_dir_types[] = {IMAGE_DIRECTORY_ENTRY_SECURITY, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT};
		for (i = 0; i < _countof(clear_dir_types); i++) {
			dir = directory_list_->GetCommandByType(clear_dir_types[i]);
			if (dir)
				dir->clear();
		}
		if (ctx.options.flags & cpStripDebugInfo) {
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_DEBUG);
			if (dir)
				dir->clear();
		}
	}

	// check discardable sections
	for (i = segment_list_->count(); i > 1; i--) {
		section = segment_list_->item(i - 1);
		if ((section->flags() & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
			for (j = i - 1; j > 0; j--) {
				section = segment_list_->item(j - 1);
				section->set_flags(section->flags() & ~IMAGE_SCN_MEM_DISCARDABLE);
			}
			break;
		}
	}

	if (ctx.options.script)
		ctx.options.script->DoAfterSaveFile();

	// write header
	if (ctx.options.flags & cpStripFixups) {
		characterictics_ |= IMAGE_FILE_RELOCS_STRIPPED;
		dll_characteristics_ &= ~IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
	}
	if ((ctx.options.flags | ctx.options.sdk_flags) & (cpCheckDebugger | cpCheckVirtualMachine))
		dll_characteristics_ &= ~IMAGE_DLLCHARACTERISTICS_NO_SEH;
	WriteToFile();

	// write header and loader CRC table
	if (loader_crc_address) {
		CRCTable crc_table(function_list_->crc_cryptor(), loader_crc_size);

		uint64_t resources_address = 0;
		if (resource_list()->size() > resource_list()->store_size()) {
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_RESOURCE);
			if (dir)
				resources_address = dir->address();
		}

		// add header
		crc_table.Add(image_base_, header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) +
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory) : offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory)) + 
			directory_list_->count() * sizeof(IMAGE_DATA_DIRECTORY) +
			segment_list_->count() * sizeof(IMAGE_SECTION_HEADER));

		// add loader sections
		j = segment_list_->IndexOf(segment_list_->GetSectionByAddress(loader_crc_address));
		if (j != NOT_ID) {
			c = (ctx.options.flags & cpLoaderCRC) ? j + 1 : segment_list_->count();
			for (i = j; i < c; i++) {
				section = segment_list_->item(i);
				if (section->memory_type() & mtWritable)
					continue;

				if (resources_address && section->address() == resources_address) {
					size = (uint32_t)(resource_list()->store_size());
				} else {
					size = std::min(static_cast<uint32_t>(section->size()), section->physical_size());
				}
				if (size)
					crc_table.Add(section->address(), size);
			}
		}

		// skip IMAGE_DOS_HEADER.e_res
		crc_table.Remove(image_base_ + offsetof(IMAGE_DOS_HEADER, e_res), sizeof(uint16_t) * 4);
		// skip IMAGE_OPTIONAL_HEADER.CheckSum
		crc_table.Remove(image_base_ + header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) +
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum) : offsetof(IMAGE_OPTIONAL_HEADER64, CheckSum)),
			sizeof(uint32_t));
		// skip IMAGE_OPTIONAL_HEADER.ImageBase
		crc_table.Remove(image_base_ + header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) +
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, ImageBase) : offsetof(IMAGE_OPTIONAL_HEADER64, ImageBase)),
			OperandSizeToValue(cpu_address_size()));
		// skip security directory
		crc_table.Remove(image_base_ + header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + 
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory) : offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory)) +
			IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY),
			sizeof(IMAGE_DATA_DIRECTORY));
		// skip IAT directory
		dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IAT);
		if (dir)
			crc_table.Remove(dir->address(), dir->size());

		if (image_type_ == itDriver) {
			// skip part of import table
			if (name_table)
				crc_table.Remove(name_table, name_table_size);
		} else {
			// skip IMAGE_IMPORT_DESCRIPTOR.TimeDateStamp and IMAGE_IMPORT_DESCRIPTOR.ForwarderChain for each import DLL
			dir = directory_list_->GetCommandByType(IMAGE_DIRECTORY_ENTRY_IMPORT);
			if (dir) {
				address = dir->address();
				for (i = 0; i < dir->size()/sizeof(IMAGE_IMPORT_DESCRIPTOR); i++, address += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
					crc_table.Remove(address + offsetof(IMAGE_IMPORT_DESCRIPTOR, TimeDateStamp), sizeof(uint32_t) * 2);
				}
			}
		}
		if (load_config_directory_->cfg_check_function())
			crc_table.Remove(load_config_directory_->cfg_check_function(), OperandSizeToValue(cpu_address_size()));
		// skip fixups
		if ((ctx.options.flags & cpStripFixups) == 0) {
			for (i = 0; i < fixup_list_->count(); i++) {
				PEFixup *fixup = fixup_list_->item(i);
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
		crc_table.Remove(header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) +
			((cpu_address_size() == osDWord) ? offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum) : offsetof(IMAGE_OPTIONAL_HEADER64, CheckSum)),
			sizeof(uint32_t));
		// skip position of security directory
		crc_table.Remove(header_offset_ + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + 
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

	WriteCheckSum();

	EndProgress();
}

void PEArchitecture::Rebase(uint64_t target_image_base, uint64_t delta_base)
{
	BaseArchitecture::Rebase(delta_base);

	fixup_list_->Rebase(*this, delta_base);
	import_list_->Rebase(delta_base);
	export_list_->Rebase(delta_base);
	directory_list_->Rebase(delta_base);
	load_config_directory_->Rebase(delta_base);
	runtime_function_list_->RebaseByFile(*this, target_image_base, delta_base);
	segment_list_->Rebase(delta_base);
	section_list_->Rebase(delta_base);
	function_list_->Rebase(delta_base);

	if (entry_point_)
		entry_point_ += delta_base;
	image_base_ += delta_base;
}

bool PEArchitecture::is_executable() const
{
	return image_type() == itExe;
}

std::string PEArchitecture::ANSIToUTF8(const std::string &str) const
{
#ifndef VMP_GNU
	if (!os::ValidateUTF8(str))
		return os::ToUTF8(os::FromACP(str));
#endif
	return str;
}

void PEArchitecture::ReadFromBuffer(Buffer &buffer)
{
	BaseArchitecture::ReadFromBuffer(buffer);

	PECFGAddressTable *cfg_address_list = load_config_directory_->cfg_address_list();
	size_t c = buffer.ReadDWord();
	for (size_t i = 0; i < c; i++) {
		cfg_address_list->Add(buffer.ReadDWord() + image_base());
	}
}

/**
* PDBFile
*/

PDBFile::PDBFile()
	: BaseMapFile(), time_stamp_(0)
{

}

bool PDBFile::Parse(const char *file_name, const std::vector<uint64_t> &segments)
{
	clear();
	guid_.clear();
	time_stamp_ = 0;
	file_name_ = file_name;

	PdbFileStream fs;
	if (!fs.Open(file_name, fmOpenRead | fmShareDenyNone))
		return false;

	segments_ = segments;

	size_t sign_len2 = sizeof(pdb2) - 1, sign_len7 = sizeof(pdb7) - 1;
	size_t sign_len = std::max(sign_len2, sign_len7);
	std::vector<uint8_t> head(sign_len);
	if (fs.RawRead(0, &head))
	{
		if (!memcmp(&head[0], pdb2, sign_len2))
		{
			pdb_jg_reader reader(fs);
			if (!reader.init())
				return false;
			time_stamp_ = reader.root->TimeDateStamp;
			return ReadSymbols(reader);
		}
		else if (!memcmp(&head[0], pdb7, sign_len7))
		{
			pdb_ds_reader reader(fs);
			if (!reader.init())
				return false;
			guid_.insert(guid_.begin(), reinterpret_cast<const uint8_t *>(&reader.root->guid), reinterpret_cast<const uint8_t *>(&reader.root->guid) + sizeof(reader.root->guid));
			return ReadSymbols(reader);
		}
	}

	return false;
}

bool PDBFile::ReadSymbols(pdb_reader &reader)
{
	// read types
	if (reader.read_file(PDB_STREAM_TPI, types_data_)) {
		PDB_TYPES *types = reinterpret_cast<PDB_TYPES *>(types_data_.data());

		size_t offset;
		if (types->version < 19960000) {
			const PDB_TYPES_OLD *types_old = reinterpret_cast<const PDB_TYPES_OLD *>(types);
			offset = sizeof(PDB_TYPES_OLD);
			types_first_index_ = types_old->first_index;
		}
		else {
			offset = types->type_offset;
			types_first_index_ = types->first_index;
		}

		int length;
		for (size_t i = offset; i < types_data_.size(); i += length)
		{
			const union codeview_type *type = reinterpret_cast<const union codeview_type*>(types_data_.data() + i);
			length = type->generic.len + 2;
			if (!type->generic.id || length < 4) 
				break;

			types_offset_.push_back(type);
		}
	}

	PDB_SYMBOLS *symbols;
	std::vector<uint8_t> vsymbols, vmodimage;

	if (!reader.read_file(PDB_STREAM_DBI, vsymbols)) 
		return false;
	symbols = reinterpret_cast<PDB_SYMBOLS*>(vsymbols.data());

	// read global symbol table
	if (reader.read_file(symbols->gsym_file, vmodimage))
		codeview_dump_symbols(vmodimage, 0);

	// read per-module symbol / linenumber tables
	const char *file = reinterpret_cast<const char*>(symbols) + sizeof(PDB_SYMBOLS);
	while (static_cast<size_t>(file - reinterpret_cast<const char*>(symbols)) < sizeof(PDB_SYMBOLS) + symbols->module_size)
	{
		int file_nr, symbol_size;
		const char* file_name;

		if (symbols->version < 19970000)
		{
			const PDB_SYMBOL_FILE* sym_file = reinterpret_cast<const PDB_SYMBOL_FILE*>(file);
			file_nr = sym_file->file;
			file_name = sym_file->filename;
			symbol_size = sym_file->symbol_size;
		}
		else
		{
			const PDB_SYMBOL_FILE_EX* sym_file = reinterpret_cast<const PDB_SYMBOL_FILE_EX*>(file);
			file_nr = sym_file->file;
			file_name = sym_file->filename;
			symbol_size = sym_file->symbol_size;
		}
		if (symbol_size && reader.read_file(file_nr, vmodimage))
			codeview_dump_symbols(vmodimage, sizeof(uint32_t));
		file_name += strlen(file_name) + 1;
		file = reinterpret_cast<char*>(reinterpret_cast<size_t>(file_name + strlen(file_name) + 1 + 3) & ~3);
	}
	return true;
}

void PDBFile::AddSymbol(size_t segment, size_t offset, const std::string &name)
{
	if (!segment || segment >= segments_.size())
		return;

	uint64_t address = segments_[segment] + offset;
	std::pair<uint64_t, std::string> key(address, name);
	if (map_.find(key) != map_.end())
		return;
	map_.insert(key);

	MapSection *section = GetSectionByType(msFunctions);
	if (!section)
		section = Add(msFunctions);

	section->Add(NOT_ID, address, 0, name);
}

void PDBFile::AddSection(size_t segment, size_t offset, uint64_t size, const std::string &name)
{
	if (segment >= segments_.size())
		return;

	MapSection *section = GetSectionByType(msSections);
	if (!section)
		section = Add(msSections);

	section->Add(segment, segments_[segment] + offset, size, name);
}

size_t leaf_length(const uint16_t *type)
{
	size_t res = sizeof(*type);
	switch (*type++) {
	case LF_CHAR:
		res += 1;
		break;
	case LF_SHORT:
	case LF_USHORT:
		res += 2;
		break;
	case LF_LONG:
	case LF_ULONG:
		res += 4;
		break;
	case LF_REAL32:
		res += 4;
		break;
	case LF_REAL48:
		res += 6;
		break;
	case LF_REAL80:
		res += 10;
		break;
	case LF_REAL128:
		res += 16;
		break;
	case LF_QUADWORD:
	case LF_UQUADWORD:
		res += 8;
		break;
	case LF_COMPLEX32:
		res += 4;
		break;
	case LF_COMPLEX80:
		res += 10;
		break;
	case LF_COMPLEX128:
		res += 16;
		break;
	case LF_VARSTRING:
		res += 2 + *type;
		break;
	}

	return res;
}

std::string PDBFile::GetTypeName(size_t data_type, const std::string &name)
{
	std::string res;
	const p_string *p_str;

	if (data_type < types_first_index_) {
		switch (data_type) {
		case T_VOID:
			res = "void";
			break;
		case T_CHAR:
			res = "char";
			break;
		case T_SHORT:
			res = "short";
			break;
		case T_LONG:
			res = "long";
			break;
		case T_QUAD:
			res = "__int64";
			break;
		case T_UCHAR:
			res = "unsigned char";
			break;
		case T_USHORT:
			res = "unsigned short";
			break;
		case T_ULONG:
			res = "unsigned long";
			break;
		case T_UQUAD:
			res = "unsigned __int64";
			break;
		case T_BOOL08:
		case T_BOOL16:
		case T_BOOL32:
		case T_BOOL64:
			res = "bool";
			break;
		case T_REAL32:
			res = "float";
			break;
		case T_REAL64:
			res = "double";
			break;
		case T_REAL80:
			res = "long double";
			break;
		case T_RCHAR:
			res = "char";
			break;
		case T_INT4:
			res = "int";
			break;
		case T_UINT4:
			res = "unsigned int";
			break;
		case T_WCHAR:
			res = "wchar_t";
			break;
		case T_CHAR16:
			res = "char16_t";
			break;
		case T_CHAR32:
			res = "char32_t";
			break;
		case T_PVOID: case T_PCHAR: case T_PSHORT: case T_PLONG: case T_PQUAD: case T_PUCHAR: case T_PUSHORT: case T_PULONG:
		case T_PUQUAD: case T_PBOOL08: case T_PBOOL16: case T_PBOOL32: case T_PBOOL64: case T_PREAL32: case T_PREAL64: case T_PREAL80:
		case T_PREAL128: case T_PREAL48: case T_PCPLX32: case T_PCPLX64: case T_PCPLX80: case T_PCPLX128: case T_PRCHAR: case T_PWCHAR:
		case T_PINT2: case T_PUINT2: case T_PINT4: case T_PUINT4: case T_PINT8: case T_PUINT8: case T_PCHAR16: case T_PCHAR32:

		case T_PFVOID: case T_PFCHAR: case T_PFSHORT: case T_PFLONG: case T_PFQUAD: case T_PFUCHAR: case T_PFUSHORT: case T_PFULONG:
		case T_PFUQUAD: case T_PFBOOL08: case T_PFBOOL16: case T_PFBOOL32: case T_PFBOOL64: case T_PFREAL32: case T_PFREAL64: case T_PFREAL80:
		case T_PFREAL128: case T_PFREAL48: case T_PFCPLX32: case T_PFCPLX64: case T_PFCPLX80: case T_PFCPLX128: case T_PFRCHAR: case T_PFWCHAR:
		case T_PFINT2: case T_PFUINT2: case T_PFINT4: case T_PFUINT4: case T_PFINT8: case T_PFUINT8: case T_PFCHAR16: case T_PFCHAR32:

		case T_PHVOID: case T_PHCHAR: case T_PHSHORT: case T_PHLONG: case T_PHQUAD: case T_PHUCHAR: case T_PHUSHORT: case T_PHULONG:
		case T_PHUQUAD: case T_PHBOOL08: case T_PHBOOL16: case T_PHBOOL32: case T_PHBOOL64: case T_PHREAL32: case T_PHREAL64: case T_PHREAL80:
		case T_PHREAL128: case T_PHREAL48: case T_PHCPLX32: case T_PHCPLX64: case T_PHCPLX80: case T_PHCPLX128: case T_PHRCHAR: case T_PHWCHAR:
		case T_PHINT2: case T_PHUINT2: case T_PHINT4: case T_PHUINT4: case T_PHINT8: case T_PHUINT8: case T_PHCHAR16: case T_PHCHAR32:

		case T_32PVOID: case T_32PCHAR: case T_32PSHORT: case T_32PLONG: case T_32PQUAD: case T_32PUCHAR: case T_32PUSHORT: case T_32PULONG:
		case T_32PUQUAD: case T_32PBOOL08: case T_32PBOOL16: case T_32PBOOL32: case T_32PBOOL64: case T_32PREAL32: case T_32PREAL64: case T_32PREAL80:
		case T_32PREAL128: case T_32PREAL48: case T_32PCPLX32: case T_32PCPLX64: case T_32PCPLX80: case T_32PCPLX128: case T_32PRCHAR: case T_32PWCHAR:
		case T_32PINT2: case T_32PUINT2: case T_32PINT4: case T_32PUINT4: case T_32PINT8: case T_32PUINT8: case T_32PCHAR16: case T_32PCHAR32:

		case T_32PFVOID: case T_32PFCHAR: case T_32PFSHORT: case T_32PFLONG: case T_32PFQUAD: case T_32PFUCHAR: case T_32PFUSHORT: case T_32PFULONG:
		case T_32PFUQUAD: case T_32PFBOOL08: case T_32PFBOOL16: case T_32PFBOOL32: case T_32PFBOOL64: case T_32PFREAL32: case T_32PFREAL64: case T_32PFREAL80:
		case T_32PFREAL128: case T_32PFREAL48: case T_32PFCPLX32: case T_32PFCPLX64: case T_32PFCPLX80: case T_32PFCPLX128: case T_32PFRCHAR: case T_32PFWCHAR:
		case T_32PFINT2: case T_32PFUINT2: case T_32PFINT4: case T_32PFUINT4: case T_32PFINT8: case T_32PFUINT8: case T_32PFCHAR16: case T_32PFCHAR32:

		case T_64PVOID: case T_64PCHAR: case T_64PSHORT: case T_64PLONG: case T_64PQUAD: case T_64PUCHAR: case T_64PUSHORT: case T_64PULONG:
		case T_64PUQUAD: case T_64PBOOL08: case T_64PBOOL16: case T_64PBOOL32: case T_64PBOOL64: case T_64PREAL32: case T_64PREAL64: case T_64PREAL80:
		case T_64PREAL128: case T_64PREAL48: case T_64PCPLX32: case T_64PCPLX64: case T_64PCPLX80: case T_64PCPLX128: case T_64PRCHAR: case T_64PWCHAR:
		case T_64PINT2: case T_64PUINT2: case T_64PINT4: case T_64PUINT4: case T_64PINT8: case T_64PUINT8: case T_64PCHAR16: case T_64PCHAR32:
			return GetTypeName(data_type & T_BASICTYPE_MASK, (!name.empty() && name.front() != '*' ? "* " : "*") + name);
		}
	}
	else if (data_type - types_first_index_ < types_offset_.size()) {
		const union codeview_type* type = types_offset_[data_type - types_first_index_];
		switch (type->generic.id) {
		case LF_MODIFIER_V1:
			if (type->modifier_v1.attribute & 0x01)
				res += "const ";
			if (type->modifier_v1.attribute & 0x02)
				res += "volatile ";
			if (type->modifier_v1.attribute & 0x04)
				res += "unaligned ";
			if (type->modifier_v1.attribute & ~0x07)
				res += "unknown ";
			if (!res.empty())
				res.pop_back();
			res = GetTypeName(type->modifier_v1.type, res);
		case LF_MODIFIER_V2:
			if (type->modifier_v2.attribute & 0x01)
				res += "const ";
			if (type->modifier_v2.attribute & 0x02)
				res += "volatile ";
			if (type->modifier_v2.attribute & 0x04)
				res += "unaligned ";
			if (type->modifier_v2.attribute & ~0x07)
				res += "unknown ";
			if (!res.empty())
				res.pop_back();
			res = GetTypeName(type->modifier_v2.type, res);
			break;
		case LF_POINTER_V1:
			return GetTypeName(type->pointer_v1.datatype, (!name.empty() && name.front() != '*' ? "* " : "*") + name);
		case LF_POINTER_V2:
			if (type->pointer_v2.attribute & 0x80)
				res = "&&";
			else if (type->pointer_v2.attribute & 0x20)
				res = '&';
			else
				res = '*';
			if (!name.empty() && name.front() != res.back())
				res += ' ';
			return GetTypeName(type->pointer_v2.datatype, res + name);
		case LF_ARRAY_V1:
			return GetTypeName(type->array_v1.elemtype, (!name.empty() && name.front() != '*' ? "* " : "*") + name);
		case LF_ARRAY_V2:
			return GetTypeName(type->array_v2.elemtype, (!name.empty() && name.front() != '*' ? "* " : "*") + name);
		case LF_ARRAY_V3:
			return GetTypeName(type->array_v3.elemtype, (!name.empty() && name.front() != '*' ? "* " : "*") + name);
		case LF_STRUCTURE_V1:
		case LF_CLASS_V1:
			p_str = reinterpret_cast<const p_string *>(reinterpret_cast<const char *>(&type->struct_v1.structlen) + leaf_length(&type->struct_v1.structlen));
			res = (type->generic.id == LF_CLASS_V1 ? "class " : "struct ") + std::string(p_str->name, p_str->namelen);
			break;
		case LF_STRUCTURE_V2:
		case LF_CLASS_V2:
			p_str = reinterpret_cast<const p_string *>(reinterpret_cast<const char *>(&type->struct_v2.structlen) + leaf_length(&type->struct_v2.structlen));
			res = (type->generic.id == LF_CLASS_V2 ? "class " : "struct ") + std::string(p_str->name, p_str->namelen);
			break;
		case LF_STRUCTURE_V3:
		case LF_CLASS_V3:
			res = (type->generic.id == LF_CLASS_V3 ? "class " : "struct ") + std::string(reinterpret_cast<const char *>(&type->struct_v3.structlen) + leaf_length(&type->struct_v3.structlen));
			break;
		case LF_ARGLIST_V1:
		{
			const union codeview_reftype *ref_type = reinterpret_cast<const union codeview_reftype *>(type);
			if (ref_type->arglist_v2.num == 0)
				res = GetTypeName(T_VOID, "");
			else for (size_t i = 0; i < ref_type->arglist_v1.num; i++) {
				if (i > 0)
					res += ',';
				res += GetTypeName(ref_type->arglist_v1.args[i], "");
			}
		}
		break;
		case LF_ARGLIST_V2:
		{
			const union codeview_reftype *ref_type = reinterpret_cast<const union codeview_reftype *>(type);
			if (ref_type->arglist_v2.num == 0)
				res = GetTypeName(T_VOID, "");
			else for (size_t i = 0; i < ref_type->arglist_v2.num; i++) {
				if (i > 0)
					res += ',';
				res += GetTypeName(ref_type->arglist_v2.args[i], "");
			}
		}
		break;
		case LF_PROCEDURE_V1:
			return GetTypeName(type->procedure_v1.rvtype, '(' + name + ')' + '(' + GetTypeName(type->procedure_v1.arglist, "") + ')');
		case LF_PROCEDURE_V2:
			return GetTypeName(type->procedure_v2.rvtype, '(' + name + ')' + '(' + GetTypeName(type->procedure_v2.arglist, "") + ')');
		case LF_UNION_V1:
			p_str = reinterpret_cast<const p_string *>(reinterpret_cast<const char *>(&type->union_v1.un_len) + leaf_length(&type->union_v1.un_len));
			res = "union " + std::string(p_str->name, p_str->namelen);
			break;
		case LF_UNION_V2:
			p_str = reinterpret_cast<const p_string *>(reinterpret_cast<const char *>(&type->union_v2.un_len) + leaf_length(&type->union_v2.un_len));
			res = "union " + std::string(p_str->name, p_str->namelen);
			break;
		case LF_UNION_V3:
			res = "union " + std::string(reinterpret_cast<const char *>(&type->union_v3.un_len) + leaf_length(&type->union_v3.un_len));
			break;
		case LF_ENUM_V1:
			res = "enum " + std::string(type->enumeration_v1.p_name.name, type->enumeration_v1.p_name.namelen);
			break;
		case LF_ENUM_V2:
			res = "enum " + std::string(type->enumeration_v2.p_name.name, type->enumeration_v2.p_name.namelen);
			break;
		case LF_ENUM_V3:
			res = "enum " + std::string(type->enumeration_v3.name);
			break;
		default:
			res = "???";
		}
	}

	if (!res.empty() && !name.empty())
		res += ' ';
	return res + name;
}

void PDBFile::codeview_dump_symbols(const std::vector<uint8_t> &root, size_t offset)
{
	size_t i;
	int length;
	for (i = offset; i < root.size(); i += length)
	{
		const union codeview_symbol* sym = reinterpret_cast<const union codeview_symbol*>(&root[0] + i);
		length = sym->generic.len + 2;
		if (!sym->generic.id || length < 4) break;
		switch (sym->generic.id)
		{
		case S_GDATA_V2:
		case S_LDATA_V2:
			AddSymbol(sym->data_v2.segment, sym->data_v2.offset, GetTypeName(sym->data_v2.symtype, std::string(sym->data_v2.p_name.name, sym->data_v2.p_name.namelen)));
			break;
		case S_LDATA_V3:
		case S_GDATA_V3:
			AddSymbol(sym->data_v3.segment, sym->data_v3.offset, GetTypeName(sym->data_v3.symtype, std::string(sym->data_v3.name)));
			break;
		case S_PUB_V2:
			AddSymbol(sym->public_v2.segment, sym->public_v2.offset, std::string(sym->public_v2.p_name.name, sym->public_v2.p_name.namelen));
			break;
		case S_PUB_V3:
			AddSymbol(sym->public_v3.segment, sym->public_v3.offset, std::string(sym->public_v3.name));
			break;
		case S_THUNK_V1:
			AddSymbol(sym->thunk_v1.segment, sym->thunk_v1.offset, std::string(sym->thunk_v1.p_name.name, sym->thunk_v1.p_name.namelen));
			break;
		case S_THUNK_V3:
			AddSymbol(sym->thunk_v3.segment, sym->thunk_v3.offset, std::string(sym->thunk_v3.name));
			break;
			//case S_GPROC_V1:
		case S_LPROC_V1:
			AddSymbol(sym->proc_v1.segment, sym->proc_v1.offset, std::string(sym->proc_v1.p_name.name, sym->proc_v1.p_name.namelen));
			break;
			//case S_GPROC_V2:
		case S_LPROC_V2:
			AddSymbol(sym->proc_v2.segment, sym->proc_v2.offset, std::string(sym->proc_v2.p_name.name, sym->proc_v2.p_name.namelen));
			break;
		case S_LPROC_V3:
			//case S_GPROC_V3:
			AddSymbol(sym->proc_v3.segment, sym->proc_v3.offset, std::string(sym->proc_v3.name));
			break;
		case S_SUBSECTINFO_V3:
			AddSection(*reinterpret_cast<const unsigned short*>(reinterpret_cast<const char*>(sym) + 16),
				*reinterpret_cast<const unsigned*>(reinterpret_cast<const char*>(sym) + 12),
				*reinterpret_cast<const unsigned*>(reinterpret_cast<const char*>(sym) + 4),
				std::string(reinterpret_cast<const char*>(sym) + 18));
			break;
		case S_LTHREAD_V1:
		case S_GTHREAD_V1:
			AddSymbol(sym->thread_v1.segment, sym->thread_v1.offset, std::string(sym->thread_v1.p_name.name, sym->thread_v1.p_name.namelen));
			break;
		case S_LTHREAD_V2:
		case S_GTHREAD_V2:
			AddSymbol(sym->thread_v2.segment, sym->thread_v2.offset, std::string(sym->thread_v2.p_name.name, sym->thread_v2.p_name.namelen));
			break;
		case S_LTHREAD_V3:
		case S_GTHREAD_V3:
			AddSymbol(sym->thread_v3.segment, sym->thread_v3.offset, std::string(sym->thread_v3.name));
			break;
		case S_PROCREF_V1:
		case S_DATAREF_V1:
		case S_LPROCREF_V1:
			length += (*(reinterpret_cast<const char *>(sym) + length) + 1 + 3) & ~3;
			break;
		}
	}
}

/**
 * COFFStringTable
 */

std::string COFFStringTable::GetString(uint32_t pos) const 
{
	if (pos < sizeof(uint32_t))
		throw std::runtime_error("Invalid index for string table");

	if (pos >= data_.size())
		throw std::runtime_error("Invalid index for string table");

	size_t i, len;

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

void COFFStringTable::ReadFromFile(PEArchitecture &file)
{
	uint32_t size = file.ReadDWord();
	if (size < sizeof(size))
		throw std::runtime_error("Invalid format");

	data_.resize(size);
	file.Read(data_.data() + sizeof(uint32_t), data_.size() - sizeof(uint32_t));
}

void COFFStringTable::ReadFromFile(FileStream &file)
{
	uint32_t size = 0;
	file.Read(&size, sizeof(size));
	if (size < sizeof(size))
		throw std::runtime_error("Invalid format");

	data_.resize(size);
	file.Read(data_.data() + sizeof(uint32_t), data_.size() - sizeof(uint32_t));
}

/**
* COFFFile
*/

bool COFFFile::Parse(const char *file_name, const std::vector<uint64_t> &segments)
{
	clear();
	time_stamp_ = 0;
	file_name_ = file_name;

	FileStream fs;
	if (!fs.Open(file_name, fmOpenRead | fmShareDenyNone))
		return false;

	segments_ = segments;

	IMAGE_DOS_HEADER dos_header;
	if (fs.Read(&dos_header, sizeof(dos_header)) == sizeof(dos_header) && dos_header.e_magic == IMAGE_DOS_SIGNATURE) {
		if (fs.Seek(dos_header.e_lfanew, soBeginning) != (uint64_t)-1) {
			uint32_t signature = 0;
			fs.Read(&signature, sizeof(signature));
			if (signature == IMAGE_NT_SIGNATURE) {
				IMAGE_FILE_HEADER image_header = IMAGE_FILE_HEADER();
				fs.Read(&image_header, sizeof(image_header));
				if (image_header.PointerToSymbolTable) {
					time_stamp_ = image_header.TimeDateStamp;
					if (fs.Seek(image_header.PointerToSymbolTable + image_header.NumberOfSymbols * sizeof(IMAGE_SYMBOL), soBeginning) != (uint64_t)-1) {
						COFFStringTable string_table;
						string_table.ReadFromFile(fs);

						fs.Seek(image_header.PointerToSymbolTable, soBeginning);
						for (size_t i = 0; i < image_header.NumberOfSymbols; i++) {
							std::string name;
							IMAGE_SYMBOL sym;
							fs.Read(&sym, sizeof(sym));
							switch (sym.StorageClass) {
							case IMAGE_SYM_CLASS_EXTERNAL:
							case IMAGE_SYM_CLASS_STATIC:
								if (sym.N.Name.Short == 0) {
									name = string_table.GetString(sym.N.Name.Long);
								}
								else {
									name = std::string(reinterpret_cast<char *>(&sym.N.ShortName), strnlen(reinterpret_cast<char *>(&sym.N.ShortName), sizeof(sym.N.ShortName)));
								}
								AddSymbol(sym.SectionNumber, sym.Value, name);
							}
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

void COFFFile::AddSymbol(size_t segment, size_t offset, const std::string &name)
{
	if (!segment || segment >= segments_.size())
		return;

	uint64_t address = segments_[segment] + offset;
	MapSection *section = GetSectionByType(msFunctions);
	if (!section)
		section = Add(msFunctions);

	section->Add(NOT_ID, address, 0, name);
}