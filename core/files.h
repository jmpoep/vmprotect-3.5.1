/**
 * Operations with executable files.
 */

#ifndef FILES_H
#define FILES_H

#include "../runtime/common.h"

std::string NameToString(const char name[], size_t name_size);
std::string DisplayString(const std::string &str);

/// Given a number of bits, return how many bytes are needed to represent that.
#define BITS_TO_BYTES(x) (((x)+7)>>3)
#define BYTES_TO_BITS(x) ((x)<<3)

uint16_t OperandSizeToValue(OperandSize os);
uint16_t OperandSizeToStack(OperandSize os);
std::string DisplayValue(OperandSize size, uint64_t value);

struct FunctionName {
	FunctionName()
		: name_pos_(0) {}
	FunctionName(const std::string &name, size_t name_pos = 0) 
		: name_(name), name_pos_(name_pos) {}
	std::string name() const { return name_.substr(name_pos_); }
	std::string display_name(bool show_ret = true) const { return DisplayString(show_ret ? name_ : name_.substr(name_pos_)); }
	void clear() 
	{
		name_.clear();
		name_pos_ = 0;
	}
	bool operator==(const FunctionName &name) const
	{
		return (name_ == name.name_) && (name_pos_ == name.name_pos_);
	}
	bool operator!=(const FunctionName &name) const
	{
		return !(operator==(name));
	}
private:
	std::string name_;
	size_t name_pos_;
};

FunctionName DemangleName(const std::string &name);

template<typename V, typename A>
V AlignValue(V value, A alignment)
{
	return (value % alignment == 0) ? value : value + alignment - (value % alignment);
}

class Core;
class Watermark;
class WatermarkManager;
#ifdef ULTIMATE
class LicensingManager;
class FileManager;
#endif
class Script;
class IFile;
class IArchitecture;
class IFunctionList;
class IFunction;
class ICommand;
class CommandBlock;
class IVirtualMachineList;
class MemoryManager;
class ILoadCommandList;
class ISectionList;
class IImport;
class IImportList;
class IExportList;
class IFixupList;
class ISEHandlerList;
class MapFunction;
class MapFunctionList;
class ReferenceList;
class Buffer;
class IRuntimeFunctionList;
class ValueCryptor;
class CRCValueCryptor;
class ISymbol;

class ILog : public IObject
{
public:
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") = 0;
	virtual void StartProgress(const std::string &message, unsigned long long max) = 0;
	virtual void StepProgress(unsigned long long value = 1, bool is_project = false) = 0;
	virtual void EndProgress() = 0;
	virtual void set_warnings_as_errors(bool value) = 0;
	virtual void set_arch_name(const std::string &arch_name) = 0;
};

class ILoadCommand : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual uint32_t size() const = 0;
	virtual uint32_t type() const = 0;
	virtual std::string name() const = 0;
	virtual bool visible() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual ILoadCommand *Clone(ILoadCommandList *owner) const = 0;
	virtual OperandSize address_size() const = 0;
	virtual ILoadCommandList *owner() const = 0;
};

class ILoadCommandList : public ObjectList<ILoadCommand>
{
public:
	virtual ILoadCommand *GetCommandByType(uint32_t type) const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IArchitecture *owner() const = 0;
};

class BaseLoadCommand : public ILoadCommand
{
public:
	explicit BaseLoadCommand(ILoadCommandList *owner);
	explicit BaseLoadCommand(ILoadCommandList *owner, const BaseLoadCommand &src);
	~BaseLoadCommand();
	virtual std::string name() const;
	virtual bool visible() const { return true; }
	virtual OperandSize address_size() const;
	ILoadCommandList *owner() const { return owner_; }
private:
	ILoadCommandList *owner_;
};

class BaseCommandList : public ILoadCommandList
{
public:
	explicit BaseCommandList(IArchitecture *owner);
	explicit BaseCommandList(IArchitecture *owner, const BaseCommandList &src);
	virtual ILoadCommand *GetCommandByType(uint32_t type) const;
	virtual void Rebase(uint64_t delta_base);
	virtual IArchitecture *owner() const { return owner_; }
private:
	IArchitecture *owner_;
};

enum MemoryTypeFlags
{
	mtNone = 0x0,
	mtReadable = 0x1,
	mtExecutable = 0x2,
	mtWritable = 0x4,
	mtNotDiscardable = 0x8,
	mtDiscardable = 0x10,
	mtNotPaged = 0x20,
	mtShared = 0x40,
	mtSolid = 0x80
};

class ISection : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual uint64_t size() const = 0;
	virtual uint32_t physical_offset() const = 0;
	virtual uint32_t physical_size() const = 0;
	virtual std::string name() const = 0;
	virtual uint32_t memory_type() const = 0;
	virtual void include_write_type(uint32_t write_type) = 0;
	virtual uint32_t write_type() const = 0;
	virtual void update_type(uint32_t mt) = 0;
	virtual ISection *parent() const = 0;
	virtual uint32_t flags() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual ISection *Clone(ISectionList *owner) const = 0;
	virtual bool excluded_from_packing() const = 0;
	virtual void set_excluded_from_packing(bool value) = 0;
	virtual bool excluded_from_memory_protection() const = 0;
	virtual void set_excluded_from_memory_protection(bool value) = 0;
	virtual OperandSize address_size() const = 0;
	virtual Data hash() const = 0;
	virtual bool need_parse() const = 0;
	virtual ISectionList *owner() const = 0;
};

class ISectionList : public ObjectList<ISection>
{
public:
	virtual ISection *GetSectionByAddress(uint64_t address) const = 0;
	virtual ISection *GetSectionByOffset(uint64_t offset) const = 0;
	virtual ISection *GetSectionByName(const std::string &name) const = 0;
	virtual ISection *GetSectionByName(ISection *segment, const std::string &name) const = 0;
	virtual uint32_t GetMemoryTypeByAddress(uint64_t address) const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IArchitecture *owner() const = 0;
};

class BaseSection : public ISection
{
public:
	explicit BaseSection(ISectionList *owner);
	explicit BaseSection(ISectionList *owner, const BaseSection &src);
	~BaseSection();
	virtual uint32_t write_type() const { return write_type_; }
	virtual void include_write_type(uint32_t write_type) { write_type_ |= write_type; }
	virtual ISection *parent() const { return NULL; }
	virtual bool excluded_from_packing() const { return excluded_from_packing_; };
	virtual void set_excluded_from_packing(bool value);
	virtual bool excluded_from_memory_protection() const { return excluded_from_memory_protection_; };
	virtual void set_excluded_from_memory_protection(bool value);
	virtual OperandSize address_size() const;
	virtual Data hash() const;
	virtual bool need_parse() const { return need_parse_; }
	void set_need_parse(bool value) { need_parse_ = value; }
	virtual ISectionList *owner() const { return owner_; }
private:
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	ISectionList *owner_;
	uint32_t write_type_;
	bool excluded_from_packing_;
	bool excluded_from_memory_protection_;
	bool need_parse_;
};

class BaseSectionList : public ISectionList
{
public:
	explicit BaseSectionList(IArchitecture *owner);
	explicit BaseSectionList(IArchitecture *owner, const BaseSectionList &src);
	virtual ISection *GetSectionByAddress(uint64_t address) const;
	virtual ISection *GetSectionByOffset(uint64_t offset) const;
	virtual ISection *GetSectionByName(const std::string &name) const;
	virtual ISection *GetSectionByName(ISection *segment, const std::string &name) const;
	virtual uint32_t GetMemoryTypeByAddress(uint64_t address) const;
	virtual void Rebase(uint64_t delta_base);
	virtual IArchitecture *owner() const { return owner_; }
private:
	IArchitecture *owner_;
};

enum APIType {
	atNone,
	atBegin,
	atEnd,
	atIsVirtualMachinePresent,
	atIsDebuggerPresent,
	atIsValidImageCRC,
	atDecryptStringA,
	atDecryptStringW,
	atFreeString,
	atActivateLicense,
	atDeactivateLicense,
	atGetOfflineActivationString,
	atGetOfflineDeactivationString,
	atSetSerialNumber,
	atGetSerialNumberState,
	atGetSerialNumberData,
	atGetCurrentHWID,
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
	atEnumResourceTypesW,
	atDecryptBuffer,
	atRuntimeInit,
	atLoaderData,
	atIsProtected,
	atSetupImage,
	atFreeImage,
	atCalcCRC,
	atRandom,
	atBoxPointer,
	atUnboxPointer
};

enum ImportOption {
	ioNone = 0x0000,
	ioNoReturn = 0x0001,
	ioHasDataReference = 0x0002,
	ioNative = 0x0004,
	ioHasCompilationType = 0x0008,
	ioLockToKey = 0x0010,
	ioFromRuntime = 0x0020,
	ioNoReferences = 0x0040,
	ioIsRelative = 0x0080,
	ioHasDirectReference = 0x0100,
	ioHasCallPrefix = 0x0200
};

enum RuntimeOptions {
	roNone,
	roHWID = 0x0001,
	roKey = 0x0002,
	roResources = 0x0004,
	roStrings = 0x0008,
	roBundler = 0x0010,
	roRegistry = 0x0020,
	roActivation = 0x0040,
	roMemoryProtection = 0x0080
};

enum CompilationType : uint8_t {
	ctVirtualization,
	ctMutation,
	ctUltra,
	ctNone = 0xFF
};

class IImportFunction : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual std::string name() const = 0;
	virtual std::string full_name() const = 0;
	virtual APIType type() const = 0;
	virtual uint32_t options() const = 0;
	virtual CompilationType compilation_type() const = 0;
	virtual MapFunction *map_function() const = 0;
	virtual void set_map_function(MapFunction *map_function) = 0;
	virtual void set_type(APIType type) = 0;
	virtual void include_option(ImportOption option) = 0;
	virtual void exclude_option(ImportOption option) = 0;
	virtual IImport *owner() const = 0;
	virtual IImportFunction *Clone(IImport *owner) const = 0;
	virtual uint32_t GetRuntimeOptions() const = 0;
	virtual uint32_t GetSDKOptions() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual std::string display_name(bool show_ret = true) const = 0;
	virtual OperandSize address_size() const = 0;
};

class IImport : public ObjectList<IImportFunction>
{
public:
	virtual std::string name() const = 0;
	virtual bool is_sdk() const = 0;
	virtual IImportFunction *GetFunctionByAddress(uint64_t address) const = 0;
	virtual uint32_t GetRuntimeOptions() const = 0;
	virtual uint32_t GetSDKOptions() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual	IImportFunction *Add(uint64_t address, APIType type, MapFunction *map_function) = 0;
	virtual	bool CompareName(const std::string &name) const = 0;
	virtual IImport *Clone(IImportList *owner) const = 0;
	virtual IImportList *owner() const = 0;
	virtual bool excluded_from_import_protection() const = 0;
	virtual void set_excluded_from_import_protection(bool value) = 0;
	virtual Data hash() const = 0;
};

struct ImportInfo {
	APIType type;
	const char *name;
	uint32_t options;
	CompilationType compilation_type;
	uint64_t encode() const
	{
		return (compilation_type << 16) | (options << 8) | type; //-V629
	};
	void decode(uint64_t value)
	{
		type = static_cast<APIType>(value & 0xff);
		options = (value >> 8) & 0xff;
		compilation_type = static_cast<CompilationType>((value >> 16) & 0xff);
	};
};

class IImportList : public ObjectList<IImport>
{
public:
	virtual IImportFunction *GetFunctionByAddress(uint64_t address) const = 0;
	virtual uint32_t GetRuntimeOptions() const = 0;
	virtual uint32_t GetSDKOptions() const = 0;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file) = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual bool has_sdk() const = 0;
	virtual IImport *GetImportByName(const std::string &name) const = 0;
	virtual const ImportInfo *GetSDKInfo(const std::string &name) const = 0;
	virtual IArchitecture *owner() const = 0;
protected:
	virtual IImport *AddSDK() = 0;
};

class BaseImportFunction : public IImportFunction
{
public:
	explicit BaseImportFunction(IImport *owner);
	explicit BaseImportFunction(IImport *owner, const BaseImportFunction &src); 
	~BaseImportFunction();
	virtual IImport *owner() const { return owner_; }
	void set_owner(IImport *value);
	virtual APIType type() const { return type_; }
	virtual void set_type(APIType type) { type_ = type; }
	virtual uint32_t GetRuntimeOptions() const;
	virtual uint32_t GetSDKOptions() const;
	virtual MapFunction *map_function() const { return map_function_; }
	virtual void set_map_function(MapFunction *map_function) { map_function_ = map_function; }
	virtual CompilationType compilation_type() const { return compilation_type_; }
	virtual void set_compilation_type(CompilationType compilation_type) { compilation_type_ = compilation_type; }
	virtual uint32_t options() const { return options_; }
	virtual void include_option(ImportOption option) { options_ |= option; }
	virtual void exclude_option(ImportOption option) { options_ &= ~option; }
	virtual std::string full_name() const;
	virtual OperandSize address_size() const;
private:
	IImport *owner_;
	APIType type_;
	MapFunction *map_function_;
	CompilationType compilation_type_;
	uint32_t options_;
};

class BaseImport : public IImport
{
public:
	explicit BaseImport(IImportList *owner);
	explicit BaseImport(IImportList *owner, const BaseImport &src);
	~BaseImport();
	virtual void clear();
	virtual IImportFunction *GetFunctionByAddress(uint64_t address) const;
	virtual uint32_t GetRuntimeOptions() const;
	virtual uint32_t GetSDKOptions() const;
	virtual void Rebase(uint64_t delta_base);
	virtual bool CompareName(const std::string &name) const;
	virtual void AddObject(IImportFunction *obj);
	virtual IImportList *owner() const { return owner_; }
	virtual bool excluded_from_import_protection() const { return excluded_from_import_protection_; };
	virtual void set_excluded_from_import_protection(bool value);
	virtual Data hash() const;
private:
	IImportList *owner_;
	std::map<uint64_t, IImportFunction *> map_;
	bool excluded_from_import_protection_;
};

class BaseImportList : public IImportList
{
public:
	explicit BaseImportList(IArchitecture *owner);
	explicit BaseImportList(IArchitecture *owner, const BaseImportList &src);
	virtual IImportFunction *GetFunctionByAddress(uint64_t address) const;
	virtual uint32_t GetRuntimeOptions() const;
	virtual uint32_t GetSDKOptions() const;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void Rebase(uint64_t delta_base);
	virtual bool has_sdk() const;
	virtual IImport *GetImportByName(const std::string &name) const;
	virtual const ImportInfo *GetSDKInfo(const std::string &name) const;
	virtual IArchitecture *owner() const { return owner_; }
private:
	IArchitecture *owner_;
};

class IExport : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual std::string name() const = 0;
	virtual std::string forwarded_name() const = 0;
	virtual std::string display_name(bool show_ret = true) const = 0;
	virtual APIType type() const = 0;
	virtual void set_type(APIType type) = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IExport *Clone(IExportList *owner) const = 0;
	virtual OperandSize address_size() const = 0;
	virtual bool is_equal(const IExport &src) const = 0;
};

class IExportList : public ObjectList<IExport>
{
public:
	virtual std::string name() const = 0;
	virtual IExport *GetExportByAddress(uint64_t address) const = 0;
	virtual uint64_t GetAddressByType(APIType type) const = 0;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file) = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IArchitecture *owner() const = 0;
	virtual bool is_equal(const IExportList &src) const = 0;
protected:
	virtual IExport *Add(uint64_t address) = 0;
};

class BaseExport : public IExport
{
public:
	explicit BaseExport(IExportList *owner);
	explicit BaseExport(IExportList *owner, const BaseExport &src);
	~BaseExport();
	virtual OperandSize address_size() const;
	virtual bool is_equal(const IExport &src) const;
	virtual APIType type() const { return type_; }
	virtual void set_type(APIType type) { type_ = type; }
private:
	IExportList *owner_;
	APIType type_;
};

class BaseExportList : public IExportList
{
public:
	explicit BaseExportList(IArchitecture *owner);
	explicit BaseExportList(IArchitecture *owner, const BaseExportList &src);
	virtual uint64_t GetAddressByType(APIType type) const;
	IExport *GetExportByAddress(uint64_t address) const;
	IExport *GetExportByName(const std::string &name) const;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void Rebase(uint64_t delta_base);
	virtual IArchitecture *owner() const { return owner_; } 
	virtual bool is_equal(const IExportList &src) const;
private:
	IArchitecture *owner_;
};

#define NEED_FIXUP reinterpret_cast<IFixup *>(-1)
#define LARGE_VALUE reinterpret_cast<IFixup *>(-2)

enum FixupType
{
	ftUnknown,
	ftHigh,
	ftLow,
	ftHighLow
};

class IFixup : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual uint64_t next_address() const = 0;
	virtual FixupType type() const = 0;
	virtual OperandSize size() const = 0;
	virtual void set_address(uint64_t address) = 0;
	virtual bool is_deleted() const = 0;
	virtual void set_deleted(bool deleted) = 0;
	virtual void Rebase(IArchitecture &file, uint64_t delta_base) = 0;
	
	using IObject::CompareWith;
	int CompareWith(const IFixup &obj) const
	{
		if (address() < obj.address())
			return -1;
		if (address() > obj.address())
			return 1;
		return 0;
	}
	virtual IFixup *Clone(IFixupList *owner) const = 0;
};

class BaseFixup : public IFixup
{
public:
	explicit BaseFixup(IFixupList *owner);
	explicit BaseFixup(IFixupList *owner, const BaseFixup &src);
	~BaseFixup();
	virtual uint64_t next_address() const { return address() + OperandSizeToValue(size()); }
	virtual bool is_deleted() const { return deleted_; }
	virtual void set_deleted(bool deleted) { deleted_ = deleted; }
private:
	IFixupList *owner_;
	bool deleted_;
};

class IFixupList : public ObjectList<IFixup>
{
public:
	virtual IFixup *GetFixupByAddress(uint64_t address) const = 0;
	virtual IFixup *GetFixupByNearAddress(uint64_t address) const = 0;
	virtual IFixup *AddDefault(OperandSize cpu_address_size, bool is_code) = 0;
	virtual void Rebase(IArchitecture &file, uint64_t delta_base) = 0;
};

class BaseFixupList : public IFixupList
{
public:
	explicit BaseFixupList();
	explicit BaseFixupList(const BaseFixupList &src);
	virtual void clear();
	virtual IFixup *GetFixupByAddress(uint64_t address) const;
	virtual IFixup *GetFixupByNearAddress(uint64_t address) const;
	virtual size_t Pack();
	virtual void Rebase(IArchitecture &file, uint64_t delta_base);
	virtual void AddObject(IFixup *obj);
private:
	std::map<uint64_t, IFixup *> map_;

	// no assignment op
	BaseFixupList &operator =(const BaseFixupList &);
};

class IRelocationList;

class ISymbol : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual std::string display_name(bool show_ret = true) const = 0;
};

class IRelocation : public IObject
{
public:
	virtual IRelocation *Clone(IRelocationList *owner) const = 0;
	virtual uint64_t address() const = 0;
	virtual void set_address(uint64_t address) = 0;
	virtual void Rebase(IArchitecture &file, uint64_t delta_base) = 0;
	virtual ISymbol *symbol() const = 0;
};

class IRelocationList : public ObjectList<IRelocation>
{
public:
	virtual IRelocation *GetRelocationByAddress(uint64_t address) const = 0;
};

class BaseRelocation : public IRelocation
{
public:
	explicit BaseRelocation(IRelocationList *owner, uint64_t address, OperandSize size);
	explicit BaseRelocation(IRelocationList *owner, const BaseRelocation &src);
	~BaseRelocation();
	uint64_t address() const { return address_; }
	OperandSize size() const { return size_; }
	void set_address(uint64_t address) { address_ = address; }
	virtual void Rebase(IArchitecture &file, uint64_t delta_base) { address_ += delta_base; }
private:
	IRelocationList *owner_;
	uint64_t address_;
	OperandSize size_;
};

class BaseRelocationList : public IRelocationList
{
public:
	explicit BaseRelocationList();
	explicit BaseRelocationList(const BaseRelocationList &src);
	virtual void clear();
	virtual IRelocation *GetRelocationByAddress(uint64_t address) const;
	virtual void AddObject(IRelocation *relocation);
	void Rebase(IArchitecture &file, uint64_t delta_base);
private:
	std::map<uint64_t, IRelocation *> map_;

	// no assignment op
	BaseRelocationList &operator =(const BaseRelocationList &);
};

#define NEED_SEH_HANDLER reinterpret_cast<ISEHandler *>(-1)

class ISEHandler : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void set_address(uint64_t address) = 0;
	virtual bool is_deleted() const = 0;
	virtual void set_deleted(bool deleted) = 0;

	using IObject::CompareWith;
	int CompareWith(const ISEHandler &obj) const
	{
		if (address() < obj.address())
			return -1;
		if (address() > obj.address())
			return 1;
		return 0;
	}
	virtual ISEHandler *Clone(ISEHandlerList *owner) const = 0;
};

class BaseSEHandler : public ISEHandler
{
public:
	explicit BaseSEHandler(ISEHandlerList *owner);
	explicit BaseSEHandler(ISEHandlerList *owner, const BaseSEHandler &src);
	~BaseSEHandler();
private:
	ISEHandlerList *owner_;
};

class ISEHandlerList : public ObjectList<ISEHandler>
{
public:
	virtual ISEHandler *GetHandlerByAddress(uint64_t address) const = 0;
	virtual ISEHandler *Add(uint64_t address) = 0;
};

class BaseSEHandlerList : public ISEHandlerList
{
public:
	explicit BaseSEHandlerList();
	explicit BaseSEHandlerList(const BaseSEHandlerList &src);
	virtual void clear();
	ISEHandler *GetHandlerByAddress(uint64_t address) const;
	virtual void AddObject(ISEHandler *handler);
private:
	std::map<uint64_t, ISEHandler *> map_;

	// no assignment op
	BaseSEHandlerList &operator =(const BaseSEHandlerList &);
};

class Reference : public IObject
{
public:
	explicit Reference(ReferenceList *owner, uint64_t address, uint64_t operand_address, size_t tag);
	explicit Reference(ReferenceList *owner, const Reference &src);
	~Reference();
	uint64_t address() const { return address_; }
	uint64_t operand_address() const { return operand_address_; }
	virtual Reference *Clone(ReferenceList *owner) const;
	void Rebase(uint64_t delta_base);
	size_t tag() const { return tag_; }
	ReferenceList *owner() const { return owner_; }
private:
	ReferenceList *owner_;
	uint64_t address_;
	uint64_t operand_address_;
	size_t tag_;
};

class ReferenceList : public ObjectList<Reference>
{
public:
	explicit ReferenceList();
	explicit ReferenceList(const ReferenceList &src);
	virtual ReferenceList *Clone() const;
	Reference *Add(uint64_t address, uint64_t operand_address, size_t tag = 0);
	Reference *GetReferenceByAddress(uint64_t address) const;
	void Rebase(uint64_t delta_base);
private:
	// no assignment op
	ReferenceList &operator =(const ReferenceList &);
};

enum ObjectType : uint8_t
{
	otCode,
	otData,
	otExport,
	otMarker,
	otAPIMarker,
	otImport,
	otString,
	otUnknown
};

struct MapFunctionHash {
	std::string name;
	ObjectType type;
	MapFunctionHash(ObjectType type_, const std::string &name_) : type(type_), name(name_) {}
	bool operator < (const MapFunctionHash &hash) const
	{
		int res = name.compare(hash.name);
		return (res != 0) ? (res < 0) : (type < hash.type);
	}
};

class MapFunction : public IObject
{
public:
	explicit MapFunction(MapFunctionList *owner, uint64_t address, ObjectType type, const FunctionName &name);
	explicit MapFunction(MapFunctionList *owner, const MapFunction &src);
	virtual ~MapFunction();
	virtual MapFunction *Clone(MapFunctionList *owner) const;
	uint64_t address() const { return address_; }
	ObjectType type() const { return type_; }
	std::string name() const { return name_.name(); }
	std::string display_name(bool show_ret = false) const { return name_.display_name(show_ret); }
	uint64_t end_address() const { return end_address_; }
	uint64_t name_address() const { return name_address_; }
	size_t name_length() const { return name_length_; }
	ReferenceList *reference_list() const { return reference_list_; }
	ReferenceList *equal_address_list() const { return equal_address_list_; }
	CompilationType compilation_type() const { return compilation_type_; }
	bool lock_to_key() const { return lock_to_key_; }
	void set_type(ObjectType type) { type_ = type; }
	void set_end_address(uint64_t end_address) { end_address_ = end_address; }
	void set_name(const FunctionName &name);
	void set_name_address(uint64_t name_address) { name_address_ = name_address; }
	void set_name_length(size_t name_length) { name_length_ = name_length; }
	void set_compilation_type(CompilationType compilation_type) { compilation_type_ = compilation_type; }
	void set_lock_to_key(bool lock_to_key) { lock_to_key_ = lock_to_key; }
	void Rebase(uint64_t delta_base);
	std::string display_address(const std::string &arch_name) const;
	MapFunctionList *owner() const { return owner_; }
	MapFunctionHash hash() const;
	FunctionName full_name() const { return name_; }
	bool is_code() const;
	bool strings_protection() const { return strings_protection_; }
	void set_strings_protection(bool value) { strings_protection_ = value; }
private:
	MapFunctionList *owner_;
	uint64_t address_;
	ObjectType type_;
	FunctionName name_;
	uint64_t end_address_;
	ReferenceList *reference_list_;
	ReferenceList *equal_address_list_;
	uint64_t name_address_;
	size_t name_length_;
	CompilationType compilation_type_;
	bool lock_to_key_;
	bool strings_protection_;

	// no copy ctr or assignment op
	MapFunction(const MapFunction &);
	MapFunction &operator =(const MapFunction &);
};

class MapFunctionList : public ObjectList<MapFunction>
{
public:
	explicit MapFunctionList(IArchitecture *owner);
	explicit MapFunctionList(IArchitecture *owner, const MapFunctionList &src);
	virtual void clear();
	void RemoveObject(MapFunction *func);
	virtual MapFunctionList *Clone(IArchitecture *owner) const;
	void ReadFromFile(IArchitecture &file);
	MapFunction *GetFunctionByAddress(uint64_t address) const;
	MapFunction *GetFunctionByName(const std::string &name) const;
	std::vector<uint64_t> GetAddressListByName(const std::string &name, bool code_only) const;
	MapFunction *Add(uint64_t address, uint64_t end_address, ObjectType type, const FunctionName &name);
	void Rebase(uint64_t delta_base);
	void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void AddObject(MapFunction *func);
	IArchitecture *owner() const { return owner_; }
private:
	IArchitecture *owner_;
	std::map<uint64_t, MapFunction*> address_map_;
	std::map<std::string, std::vector<MapFunction*> > name_map_;
};

class MapSection;
class IMapFile;

class MapObject : public IObject
{
public:
	explicit MapObject(MapSection *owner, size_t segment, uint64_t address, uint64_t size, const std::string &name);
	virtual ~MapObject();
	size_t segment() const { return segment_; }
	uint64_t address() const { return address_; }
	uint64_t size() const { return size_; }
	std::string name() const { return name_; }
private:
	MapSection *owner_;
	size_t segment_;
	uint64_t address_;
	uint64_t size_;
	std::string name_;
};

enum MapSectionType {
	msSections,
	msFunctions
};

class MapSection : public ObjectList<MapObject>
{
public:
	explicit MapSection(IMapFile *owner, MapSectionType type);
	virtual ~MapSection();
	MapSectionType type() const { return type_; }
	void Add(size_t segment, uint64_t address, uint64_t size, const std::string &name);
private:
	IMapFile *owner_;
	MapSectionType type_;
};

class IMapFile : public ObjectList<MapSection>
{
public:
	virtual MapSection *GetSectionByType(MapSectionType type) const = 0;
	virtual bool Parse(const char *file_name, const std::vector<uint64_t> &segments) = 0;
	virtual std::string file_name() const = 0;
	virtual uint64_t time_stamp() const = 0;
};

class BaseMapFile : public IMapFile
{
public:
	explicit BaseMapFile();
	virtual MapSection *GetSectionByType(MapSectionType type) const;
protected:
	MapSection *Add(MapSectionType type);
};

class MapFile : public BaseMapFile
{
public:
	explicit MapFile();
	virtual bool Parse(const char *file_name, const std::vector<uint64_t> &segments);
	virtual std::string file_name() const { return file_name_; }
	virtual uint64_t time_stamp() const { return time_stamp_; }
protected:
	void set_time_stamp(uint64_t value) { time_stamp_ = value; }
private:
	std::string file_name_;
	uint64_t time_stamp_;
};

class MapFunctionBundle;
class MapFunctionBundleList;

class MapFunctionArch : public IObject
{
public:
	explicit MapFunctionArch(MapFunctionBundle *owner, IArchitecture *arch, MapFunction *func);
	~MapFunctionArch();
	IArchitecture *arch() const { return arch_; }
	MapFunction *func() const { return func_; }
private:
	MapFunctionBundle *owner_;
	IArchitecture *arch_;
	MapFunction *func_;
};

class MapFunctionBundle : public ObjectList<MapFunctionArch>
{
public:
	explicit MapFunctionBundle(MapFunctionBundleList *owner, ObjectType type, const FunctionName &name);
	~MapFunctionBundle();
	MapFunctionArch *Add(IArchitecture *arch, MapFunction *func);
	MapFunctionHash hash() const { return MapFunctionHash(type_, name_.name()); }
	ObjectType type() const { return type_; }
	std::string name() const { return name_.name(); }
	std::string display_name(bool show_ret = true) const { return name_.display_name(show_ret); }
	MapFunction *GetFunctionByArch(IArchitecture *arch) const;
	bool is_code() const;
	std::string display_address() const;
	MapFunctionBundleList *owner() const { return owner_; }
	FunctionName full_name() const { return name_; }
private:
	MapFunctionBundleList *owner_;
	ObjectType type_;
	FunctionName name_;
};

class MapFunctionBundleList : public ObjectList<MapFunctionBundle>
{
public:
	explicit MapFunctionBundleList(IFile *owner);
	void ReadFromFile(IFile &file);
	virtual void AddObject(MapFunctionBundle *info);
	MapFunctionBundle *GetFunctionByAddress(IArchitecture *arch, uint64_t address) const;
	bool show_arch_name() const { return show_arch_name_; }
	void set_show_arch_name(bool show_arch_name) { show_arch_name_ = show_arch_name; }
	IFile *owner() const { return owner_; }
	MapFunctionBundle *Add(IArchitecture *arch, MapFunction *func);
private:
	MapFunctionBundle *GetFunctionByHash(const MapFunctionHash &hash) const;
	IFile *owner_;
	std::map<MapFunctionHash, MapFunctionBundle *> map_;
	bool show_arch_name_;
};

class IResource : public ObjectList<IResource>
{
public:
	virtual uint32_t type() const = 0;
	virtual uint64_t address() const = 0;
	virtual size_t size() const = 0;
	virtual std::string name() const = 0;
	virtual IResource *owner() const = 0;
	virtual bool is_directory() const = 0;
	virtual bool need_store() const = 0;
	virtual IResource *Clone(IResource *owner) const = 0;
	virtual IResource *GetResourceByName(const std::string &name) const = 0;
	virtual IResource *GetResourceByType(uint32_t type) const = 0;
	virtual IResource *GetResourceById(const std::string &id) const = 0;
	virtual bool excluded_from_packing() const = 0;
	virtual void set_excluded_from_packing(bool value) = 0;
	virtual OperandSize address_size() const = 0;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const = 0;
	virtual std::string id() const = 0;
	virtual Data hash() const = 0;
};

class IResourceList : public IResource
{
public:
	virtual std::vector<IResource*> GetResourceList() const = 0;
};

class BaseResource : public IResource
{
public:
	explicit BaseResource(IResource *owner);
	explicit BaseResource(IResource *owner, const BaseResource &src);
	~BaseResource();
	virtual IResource *owner() const { return owner_; }
	virtual IResource *GetResourceByName(const std::string &name) const;
	virtual IResource *GetResourceByType(uint32_t type) const;
	virtual IResource *GetResourceById(const std::string &id) const;
	virtual bool excluded_from_packing() const { return excluded_from_packing_; }
	virtual void set_excluded_from_packing(bool value);
	virtual OperandSize address_size() const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual Data hash() const;
private:
	IResource *owner_;
	bool excluded_from_packing_;
};

class BaseResourceList : public IResourceList
{
public:
	explicit BaseResourceList(IArchitecture *owner);
	explicit BaseResourceList(IArchitecture *owner, const BaseResourceList &src);
	virtual uint32_t type() const { return (uint32_t)-1; }
	virtual uint64_t address() const { return 0; }
	virtual size_t size() const { return 0; }
	virtual std::string name() const { return std::string(); }
	virtual IResource *owner() const { return NULL; }
	virtual bool is_directory() const { return true; }
	virtual IResource *Clone(IResource * /*owner*/) const { return NULL; }
	virtual IResource *GetResourceByName(const std::string &name) const;
	virtual IResource *GetResourceByType(uint32_t type) const;
	virtual IResource *GetResourceById(const std::string &id) const;
	virtual bool excluded_from_packing() const { return false; }
	virtual void set_excluded_from_packing(bool /*value*/) { }
	virtual bool need_store() const { return true; }
	virtual std::vector<IResource*> GetResourceList() const;
	virtual std::string id() const { return std::string(); }
	virtual OperandSize address_size() const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual Data hash() const { return Data(); }
private:
	IArchitecture *owner_;
};

enum CompilerFunctionType {
	cfNone,
	cfBaseRegistr,
	cfGetBaseRegistr,
	cfDllFunctionCall,
	cfCxxSEH,
	cfCxxSEH3,
	cfCxxSEH4,
	cfSEH4Prolog,
	cfVB6SEH,
	cfInitBCBSEH,
	cfBCBSEH,
	cfRelocatorMinGW,
	cfPatchImport,
	cfJmpFunction
};

enum CompilerFunctionOption {
	coUsed = 1,
	coNoReturn = 2,
};

class CompilerFunctionList;

class CompilerFunction : public IObject
{
public:
	explicit CompilerFunction(CompilerFunctionList *owner, CompilerFunctionType type, uint64_t address);
	explicit CompilerFunction(CompilerFunctionList *owner, const CompilerFunction &src);
	~CompilerFunction();
	CompilerFunctionType type() const { return type_; }
	uint64_t address() const { return address_; }
	uint64_t value(size_t index) const { return (index < value_list_.size()) ? value_list_[index] : 0; }
	void add_value(uint64_t value) { value_list_.push_back(value); } 
	virtual CompilerFunction *Clone(CompilerFunctionList *owner) const;
	uint32_t options() const { return options_; }
	void include_option(CompilerFunctionOption option) { options_ |= option; }
	size_t count() const { return value_list_.size(); }
	void Rebase(uint64_t delta_base);
private:
	CompilerFunctionList *owner_;
	uint64_t address_;
	CompilerFunctionType type_;
	uint32_t options_;
	std::vector<uint64_t> value_list_;

	// not implemented
	CompilerFunction(const CompilerFunction &);
	CompilerFunction &operator =(const CompilerFunction &);
};

class CompilerFunctionList : public ObjectList<CompilerFunction>
{
public:
	explicit CompilerFunctionList();
	explicit CompilerFunctionList(const CompilerFunctionList &src);
	CompilerFunction *Add(CompilerFunctionType type, uint64_t address);
	virtual void AddObject(CompilerFunction *func);
	CompilerFunction *GetFunctionByAddress(uint64_t address) const;
	CompilerFunction *GetFunctionByLowerAddress(uint64_t address) const;
	CompilerFunctionList *Clone() const;
	uint64_t GetRegistrValue(uint64_t address, uint64_t registr) const;
	uint32_t GetSDKOptions() const;
	uint32_t GetRuntimeOptions() const;
	void Rebase(uint64_t delta_base);
private:
	std::map<uint64_t, CompilerFunction*> map_;

	// no assignment op
	CompilerFunctionList &operator =(const CompilerFunctionList &);
};

class IRuntimeFunction : public IObject
{
public:
	virtual IRuntimeFunction *Clone(IRuntimeFunctionList *owner) const = 0;
	virtual uint64_t address() const = 0;
	virtual uint64_t begin() const = 0;
	virtual uint64_t end() const = 0;
	virtual uint64_t unwind_address() const = 0;
	virtual void set_begin(uint64_t begin) = 0;
	virtual void set_end(uint64_t end) = 0;
	virtual void set_unwind_address(uint64_t unwind_address) = 0;
	virtual void Parse(IArchitecture &file, IFunction &dest) = 0;
	virtual void Rebase(uint64_t delta_base) = 0;

	using IObject::CompareWith;
	int CompareWith(const IRuntimeFunction &obj) const
	{
		if (begin() < obj.begin())
			return -1;
		if (begin() > obj.begin())
			return 1;
		return 0;
	}
};

class IRuntimeFunctionList : public ObjectList<IRuntimeFunction>
{
public:
	virtual IRuntimeFunction *GetFunctionByAddress(uint64_t address) const = 0;
	virtual IRuntimeFunction *GetFunctionByUnwindAddress(uint64_t address) const = 0;
	virtual IRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions) = 0;
};

class BaseRuntimeFunction : public IRuntimeFunction
{
public:
	explicit BaseRuntimeFunction(IRuntimeFunctionList *owner);
	explicit BaseRuntimeFunction(IRuntimeFunctionList *owner, const BaseRuntimeFunction &src);
	~BaseRuntimeFunction();
private:
	IRuntimeFunctionList *owner_;
};

class BaseRuntimeFunctionList : public IRuntimeFunctionList
{
public:
	explicit BaseRuntimeFunctionList();
	explicit BaseRuntimeFunctionList(const BaseRuntimeFunctionList &src);
	virtual void clear();
	virtual IRuntimeFunction *GetFunctionByAddress(uint64_t address) const;
	virtual IRuntimeFunction *GetFunctionByUnwindAddress(uint64_t address) const;
	virtual void Rebase(uint64_t delta_base);
	virtual void AddObject(IRuntimeFunction *obj);
private:
	std::map<uint64_t, IRuntimeFunction *> map_;
	std::map<uint64_t, IRuntimeFunction *> unwind_map_;

	// not impl
	BaseRuntimeFunctionList &operator =(const BaseRuntimeFunctionList &);
};

struct CompileOptions {
	uint32_t flags;
	uint32_t vm_flags;
	uint32_t sdk_flags;
	size_t vm_count;
	std::string section_name;
	std::string messages[MESSAGE_COUNT];
	Watermark *watermark;
	Script *script;
	IArchitecture **architecture;
#ifdef ULTIMATE
	std::string hwid;
	LicensingManager *licensing_manager;
	FileManager *file_manager;
#endif
	CompileOptions() : flags(0), vm_flags(0), sdk_flags(0), vm_count(1), watermark(NULL), script(NULL), architecture(NULL)
#ifdef ULTIMATE
		, licensing_manager(NULL), file_manager(NULL)
#endif
		{}
};

class Folder : public ObjectList<Folder>
{
public:
	explicit Folder(Folder *owner, const std::string &name);
	explicit Folder(Folder *owner, const Folder &src);
	virtual ~Folder();
	Folder *Clone(Folder *owner) const;
	Folder *Add(const std::string &name);
	std::string name() const { return name_; }
	Folder *owner() const { return owner_; }
	void set_name(const std::string &name);
	bool read_only() const { return read_only_; }
	void set_read_only(bool read_only) { read_only_ = read_only; }
	std::string id() const;
	void set_owner(Folder *owner);
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	Folder *GetFolderById(const std::string &id) const;
private:
	void changed();
	Folder *owner_;
	std::string name_;
	bool read_only_;
};

class FunctionBundle;
class FunctionBundleList;

class FolderList : public Folder
{
public:
	explicit FolderList(IFile *owner);
	explicit FolderList(IFile *owner, const FolderList &src);
	FolderList *Clone(IFile *owner) const;
	std::vector<Folder*> GetFolderList(bool skip_read_only = false) const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	IFile *owner() const { return owner_; }
private:
	IFile *owner_;
};

class FunctionArch : public IObject
{
public:
	explicit FunctionArch(FunctionBundle *owner, IArchitecture *arch, IFunction *func);
	~FunctionArch();
	IArchitecture *arch() const { return arch_; }
	IFunction *func() const { return func_; }
private:
	FunctionBundle *owner_;
	IArchitecture *arch_;
	IFunction *func_;
};

struct FunctionBundleHash {
	std::string name;
	bool is_unknown;
	FunctionBundleHash(const std::string &name_, bool is_unknown_) : name(name_), is_unknown(is_unknown_) {}
	bool operator < (const FunctionBundleHash &hash) const
	{
		int res = name.compare(hash.name);
		return (res != 0) ? (res < 0) : (is_unknown < hash.is_unknown);
	}
};

class FunctionBundle : public ObjectList<FunctionArch>
{
public:
	explicit FunctionBundle(FunctionBundleList *owner, const FunctionName &name, bool is_unknown);
	~FunctionBundle();
	FunctionArch *Add(IArchitecture *arch, IFunction *func);
	std::string name() const { return name_.name(); }
	bool is_unknown() const { return is_unknown_; }
	std::string display_name() const { return name_.display_name(); }
	bool need_compile() const;
	void set_need_compile(bool need_compile);
	CompilationType compilation_type() const;
	CompilationType default_compilation_type() const;
	void set_compilation_type(CompilationType compilation_type);
	uint32_t compilation_options() const;
	void set_compilation_options(uint32_t compilation_options);
	Folder *folder() const;
	void set_folder(Folder *folder);
	ObjectType type() const;
	std::string display_address() const;
	std::string display_protection() const;
	std::string id() const { return (type() == otUnknown) ? display_name() : display_address(); }
	bool show_arch_name() const;
	FunctionArch *GetArchByFunction(IFunction *func) const;
	ICommand *GetCommandByAddress(IArchitecture *file, uint64_t address) const;
private:
	FunctionBundleList *owner_;
	FunctionName name_;
	bool is_unknown_;
};

class FunctionBundleList : public ObjectList<FunctionBundle>
{
public:
	explicit FunctionBundleList();
	FunctionBundle *Add(IArchitecture *arch, IFunction *func);
	virtual void AddObject(FunctionBundle *bundle);
	virtual void RemoveObject(FunctionBundle *bundle);
	FunctionBundle *GetFunctionByFunc(IFunction *func) const;
	FunctionBundle *GetFunctionByAddress(IArchitecture *arch, uint64_t address) const;
	FunctionBundle *GetFunctionById(const std::string &id) const;
	FunctionBundle *GetFunctionByName(const std::string &name, bool need_unknown = false) const;
	bool show_arch_name() const { return show_arch_name_; }
	void set_show_arch_name(bool show_arch_name) { show_arch_name_ = show_arch_name; }
private:
	std::map<FunctionBundleHash, FunctionBundle *> map_;
	bool show_arch_name_;
};

enum OpenMode {
	foRead = 0x01,
	foWrite = 0x02,
	foHeaderOnly = 0x04,
	foCopyToTemp = 0x08
};

enum OpenStatus {
	osSuccess,
	osOpenError,
	osUnknownFormat,
	osInvalidFormat,
	osUnsupportedCPU,
	osUnsupportedSubsystem
};

class AbstractStream;
class ILog;

struct ResourceInfo {
	const uint8_t *file;
	size_t size;
	const uint8_t *code;
};

class IFile : public ObjectList<IArchitecture>
{
public:
	explicit IFile(ILog *log);
	explicit IFile(const IFile &src, const char *file_name);
	virtual ~IFile();
	virtual std::string format_name() const { return std::string("Unknown"); }
	virtual bool OpenResource(const void *resource, size_t size, bool is_enc);
	virtual bool OpenModule(uint32_t process_id, HMODULE module);
	virtual OpenStatus Open(const char *file_name, uint32_t open_mode, std::string *error = NULL);
	virtual void Close();
	virtual IFile *Clone(const char *file_name) const;
	virtual std::string file_name(bool is_real = false) const { return (is_real && !file_name_tmp_.empty()) ? file_name_tmp_ : file_name_; };
	virtual std::string version() const { return std::string(); };
	virtual std::string exec_command() const { return std::string(); };
	uint8_t ReadByte();
	uint16_t ReadWord();
	uint32_t ReadDWord();
	uint64_t ReadQWord();
	size_t Read(void *buffer, size_t count);
	size_t Write(const void *buffer, size_t count);
	void Flush();
	std::string ReadString();
	uint64_t Seek(uint64_t position);
	uint64_t Tell();
	uint64_t size() const;
	uint64_t Resize(uint64_t size);
	virtual bool Compile(CompileOptions &options);
	virtual IFile *runtime() const { return NULL; }
	uint64_t CopyFrom(IFile &source, uint64_t count);
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	void StartProgress(const std::string &caption, unsigned long long max) const;
	void StepProgress(unsigned long long value = 1ull) const;
	void EndProgress() const;
	void set_log(ILog *log) { log_ = log; }
	std::map<Watermark *, size_t> SearchWatermarks(const WatermarkManager &watermark_list);
	virtual bool is_executable() const { return false; }
	size_t visible_count() const;
	IArchitecture *GetArchitectureByType(uint32_t type) const;
	IArchitecture *GetArchitectureByName(const std::string &name) const;
	FolderList *folder_list() const { return folder_list_; }
	MapFunctionBundleList *map_function_list() const { return map_function_list_; }
	FunctionBundleList *function_list() const { return function_list_; }
	virtual uint32_t disable_options() const { return 0; }
	AbstractStream *stream() const { return stream_; }
protected:
	AbstractStream *stream_;
	void CloseStream();
	virtual OpenStatus ReadHeader(uint32_t /*open_mode*/) { return osSuccess; }
private:
	std::string file_name_, file_name_tmp_;
	ILog *log_;
	bool skip_change_notifications_;
	FolderList *folder_list_;
	MapFunctionBundleList *map_function_list_;
	FunctionBundleList *function_list_;

	// no copy ctr or assignment op
	IFile(const IFile &);
	IFile &operator =(const IFile &);
};

class MemoryManager;

class MemoryRegion: public IObject
{
public:
	explicit MemoryRegion(MemoryManager *owner, uint64_t address, size_t size, 
		uint32_t type, IFunction *parent_function);
	~MemoryRegion();
	uint64_t address() const { return address_; }
	uint64_t end_address() const { return end_address_; }
	size_t size() const { return static_cast<size_t>(end_address_ - address_); }
	uint32_t type() const { return type_; }
	IFunction *parent_function() const { return parent_function_; }
	uint64_t Alloc(uint64_t memory_size, uint32_t memory_type);
	
	using IObject::CompareWith;
	int CompareWith(const MemoryRegion &obj) const;
	bool Merge(const MemoryRegion &src);
	MemoryRegion *Subtract(uint64_t remove_address, size_t size);
	void exclude_type(MemoryTypeFlags type) { type_ &= ~type; }
	void set_owner(MemoryManager *owner) { owner_ = owner; }
private:
	MemoryManager *owner_;
	uint64_t address_;
	uint64_t end_address_;
	uint32_t type_;
	IFunction *parent_function_;
};

class MemoryManager : public ObjectList<MemoryRegion>
{
public:
	explicit MemoryManager(IArchitecture *owner);
	uint64_t Alloc(size_t size, uint32_t memory_type, uint64_t address = 0, size_t alignment = 0);
	MemoryRegion *GetRegionByAddress(uint64_t address) const;
	void Add(uint64_t address, size_t size);
	void Add(uint64_t address, size_t size, uint32_t type, IFunction *parent_function = NULL);
	void Remove(uint64_t address, size_t size);
	void Pack();
	IArchitecture *owner() const { return owner_; }
private:
	size_t IndexOfAddress(uint64_t address) const;
	struct CompareHelper {
		bool operator () (const MemoryRegion *region, uint64_t address) const
		{
			return (region->address() < address);
		}

		bool operator () (uint64_t address, const MemoryRegion *region) const
		{
			return (address < region->address());
		}
	};

	IArchitecture *owner_;
};

struct CRCInfo {
	struct POD {
		uint32_t address;
		uint32_t size;
		uint32_t hash;
	} pod;

	CRCInfo() 
	{
		pod.address = 0;
		pod.size = 0;
		pod.hash = 0;
	}

	CRCInfo(uint32_t address_, const std::vector<uint8_t> &dump);
};

class CRCTable
{
public:
	CRCTable(ValueCryptor *cryptor, size_t max_size);
	~CRCTable();
	void Add(uint64_t address, size_t size);
	void Remove(uint64_t address, size_t size);
	size_t WriteToFile(IArchitecture &file, bool is_positions, uint32_t *hash = NULL);
private:
	std::vector<CRCInfo> crc_info_list_;
	MemoryManager *manager_;
	CRCValueCryptor *cryptor_;
	size_t max_size_;

	// no copy ctr or assignment op
	CRCTable(const CRCTable &);
	CRCTable &operator =(const CRCTable &);
};

struct CompileContext {
	CompileOptions options;
	IArchitecture *file;
	IArchitecture *runtime;
	IArchitecture *vm_runtime;
	MemoryManager *manager;
	size_t runtime_var_index[VAR_COUNT];
	size_t runtime_var_salt[VAR_COUNT];
	CompileContext() : file(NULL), runtime(NULL), vm_runtime(NULL), manager(NULL)
	{
		size_t i;
		for (i = 0; i <= VAR_CPU_HASH; i++) {
			runtime_var_index[i] = i;
			runtime_var_salt[i] = static_cast<uint32_t>(rand());
		}
		for (i = 0; i <= VAR_CPU_HASH; i++) {
			std::swap(runtime_var_index[i], runtime_var_index[rand() % (VAR_CPU_HASH + 1)]);
		}
		for (i = 0; i <= VAR_CPU_HASH; i++) {
			if (runtime_var_index[i] > runtime_var_index[VAR_CPU_HASH])
				runtime_var_index[i] += VAR_COUNT - VAR_CPU_HASH - 1;
		}
		for (i = VAR_CPU_HASH + 1; i < VAR_COUNT; i++) {
			runtime_var_index[i] = runtime_var_index[VAR_CPU_HASH] + i - VAR_CPU_HASH;
			runtime_var_salt[i] = runtime_var_salt[VAR_CPU_HASH];
		}
	}
};

class MarkerCommandList;

class MarkerCommand: public IObject
{
public:
	explicit MarkerCommand(MarkerCommandList *owner, uint64_t address, uint64_t operand_address, 
		uint64_t name_reference, uint64_t name_address, ObjectType type);
	explicit MarkerCommand(MarkerCommandList *owner, const MarkerCommand &src);
	~MarkerCommand();
	MarkerCommand *Clone(MarkerCommandList *owner) const;
	uint64_t address() const { return address_; }
	uint64_t operand_address() const { return operand_address_; }
	uint64_t name_address() const { return name_address_; }
	uint64_t name_reference() const { return name_reference_; }
	ObjectType type() const { return type_; }

	using IObject::CompareWith;
	int CompareWith(const MarkerCommand &obj) const;
private:
	MarkerCommandList *owner_;
	uint64_t address_;
	uint64_t operand_address_;
	uint64_t name_address_;
	uint64_t name_reference_;
	ObjectType type_;
};

class MarkerCommandList : public ObjectList<MarkerCommand>
{
public:
	explicit MarkerCommandList();
	explicit MarkerCommandList(const MarkerCommandList &src);
	MarkerCommandList *Clone() const;
	MarkerCommand *Add(uint64_t address, uint64_t operand_address, uint64_t name_reference, 
		uint64_t name_address, ObjectType type = otUnknown);
private:
	// no assignment op
	MarkerCommandList &operator =(const MarkerCommandList &);
};

enum CallingConvention {
	ccStdcall,
	ccCdecl,
	ccMSx64,
	ccABIx64,
	ccStdcallToMSx64
};

class IArchitecture : public IObject
{
public:
	virtual std::string name() const = 0;
	virtual uint32_t type() const = 0;
	virtual OperandSize cpu_address_size() const = 0;
	virtual uint64_t entry_point() const = 0;
	virtual uint32_t segment_alignment() const = 0;
	virtual ILoadCommandList *command_list() const = 0;
	virtual ISectionList *segment_list() const = 0;
	virtual ISectionList *section_list() const = 0;
	virtual IImportList *import_list() const = 0;
	virtual IExportList *export_list() const = 0;
	virtual IFixupList *fixup_list() const = 0;
	virtual IRelocationList *relocation_list() const = 0;
	virtual IFunctionList *function_list() const = 0;
	virtual IVirtualMachineList *virtual_machine_list() const = 0;
	virtual IResourceList *resource_list() const = 0;
	virtual ISEHandlerList *seh_handler_list() const = 0;
	virtual std::string map_file_name() const = 0;
	virtual MapFunctionList *map_function_list() const = 0;
	virtual CompilerFunctionList *compiler_function_list() const = 0;
	virtual IRuntimeFunctionList *runtime_function_list() const = 0;
	virtual MarkerCommandList *end_marker_list() const = 0;
	virtual bool visible() const = 0;
	virtual uint8_t ReadByte() = 0;
	virtual uint16_t ReadWord() = 0;
	virtual uint32_t ReadDWord() = 0;
	virtual uint64_t ReadQWord() = 0;
	virtual size_t Read(void *buffer, size_t count) const = 0;
	virtual size_t WriteByte(uint8_t value) = 0;
	virtual size_t WriteWord(uint16_t value) = 0;
	virtual size_t WriteDWord(uint32_t value) = 0;
	virtual size_t WriteQWord(uint64_t value) = 0;
	virtual size_t Write(const void *buffer, size_t count) = 0;
	virtual std::string ReadString() = 0;
	virtual std::string ReadString(uint64_t address) = 0;
	virtual uint64_t Seek(uint64_t position) const = 0;
	virtual uint64_t Tell() const = 0;
	virtual uint64_t AddressTell() = 0;
	virtual uint64_t Resize(uint64_t size) = 0;
	virtual bool AddressSeek(uint64_t address) = 0;
	virtual bool Compile(CompileOptions &options, IArchitecture *runtime) = 0;
	virtual void Save(CompileContext &ctx) = 0;
	virtual IFile *owner() const = 0;
	virtual ISection *selected_segment() const = 0;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const = 0;
	virtual void StartProgress(const std::string &caption, unsigned long long max) const = 0;
	virtual void StepProgress(unsigned long long value = 1ull) const = 0;
	virtual void EndProgress() const = 0;
	virtual const IArchitecture *source() const = 0;
	virtual uint64_t offset() const = 0;
	virtual uint64_t size() const = 0;
	virtual uint64_t image_base() const = 0;
	virtual CallingConvention calling_convention() const = 0;
	virtual uint64_t CopyFrom(const IArchitecture &src, uint64_t count) = 0;
	virtual void ReadFromBuffer(Buffer &buffer) = 0;
	virtual bool WriteToFile() = 0;
	virtual bool is_executable() const = 0;
	virtual IArchitecture *Clone(IFile *owner) const = 0;
	virtual std::string ANSIToUTF8(const std::string &str) const = 0;
#ifdef CHECKED
	virtual bool check_hash() const = 0;
#endif
};

class BaseArchitecture : public IArchitecture
{
public:
	explicit BaseArchitecture(IFile *owner, uint64_t offset, uint64_t size);
	explicit BaseArchitecture(IFile *owner, const BaseArchitecture &src);
	virtual ~BaseArchitecture();
	virtual MapFunctionList *map_function_list() const { return map_function_list_; }
	virtual CompilerFunctionList *compiler_function_list() const { return compiler_function_list_; }
	virtual MarkerCommandList *end_marker_list() const { return end_marker_list_; }
	virtual std::string map_file_name() const;
	virtual uint8_t ReadByte() { return owner_->ReadByte(); }
	virtual uint16_t ReadWord() { return owner_->ReadWord(); }
	virtual uint32_t ReadDWord() { return owner_->ReadDWord(); }
	virtual uint64_t ReadQWord() { return owner_->ReadQWord(); }
	virtual size_t Read(void *buffer, size_t count) const { return owner_->Read(buffer, count); }
	virtual size_t WriteByte(uint8_t value) { return Write(&value, sizeof(value)); }
	virtual size_t WriteWord(uint16_t value) { return Write(&value, sizeof(value)); }
	virtual size_t WriteDWord(uint32_t value) { return Write(&value, sizeof(value)); }
	virtual size_t WriteQWord(uint64_t value) { return Write(&value, sizeof(value)); }
	virtual size_t Write(const void *buffer, size_t count) { return owner_->Write(buffer, count); }
	virtual std::string ReadString() { return owner_->ReadString(); }
	virtual std::string ReadString(uint64_t address);
	virtual uint64_t Seek(uint64_t position) const;
	virtual uint64_t Tell() const;
	virtual uint64_t Resize(uint64_t size);
	virtual bool AddressSeek(uint64_t address);
	virtual uint64_t AddressTell();
	virtual bool visible() const { return (function_list() != NULL); }
	virtual bool Compile(CompileOptions &options, IArchitecture *runtime);
	virtual IFile *owner() const { return owner_; }
	virtual ISection *selected_segment() const { return selected_segment_; }
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual void StartProgress(const std::string &caption, unsigned long long max) const;
	virtual void StepProgress(unsigned long long value = 1) const;
	virtual void EndProgress() const;
	virtual const IArchitecture *source() const { return source_; }
	virtual uint64_t offset() const { return offset_; }
	virtual uint64_t size() const { return append_mode_ ? owner_->size() - offset_ : size_; }
	virtual void ReadFromBuffer(Buffer &buffer);
	virtual uint64_t CopyFrom(const IArchitecture &src, uint64_t count);
	virtual uint64_t time_stamp() const { return 0; }
	virtual std::string ANSIToUTF8(const std::string &str) const { return str; }
#ifdef CHECKED
	virtual bool check_hash() const;
#endif
protected:
	MemoryManager *memory_manager() const { return memory_manager_; }
	virtual bool Prepare(CompileContext &ctx);
	virtual bool ReadMapFile(IMapFile &map_file);
	void Rebase(uint64_t delta_base);
	void set_append_mode(bool value) { append_mode_ = value; }
private:
	std::string ReadANSIString(uint64_t address);
	std::string ReadUnicodeString(uint64_t address);
	std::string ReadANSIStringWithLength(uint64_t address);
	IFile *owner_;
	const IArchitecture *source_;
	uint64_t offset_;
	uint64_t size_;
	MapFunctionList *map_function_list_;
	CompilerFunctionList *compiler_function_list_;
	MarkerCommandList *end_marker_list_;
	MemoryManager *memory_manager_;
	ISection *selected_segment_;
	bool append_mode_;

	// no copy ctr or assignment op
	BaseArchitecture(const BaseArchitecture &);
	BaseArchitecture &operator =(const BaseArchitecture &);
};

#endif