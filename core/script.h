#ifndef SCRIPT_H
#define SCRIPT_H

#include "../third-party/lua/lua.hpp"
#include "../third-party/libffi/ffi.h"

#ifndef VMP_GNU
#ifdef _WIN64
#if NDEBUG
#pragma comment(lib, "../third-party/libffi/libffi64.lib")
#else
#pragma comment(lib, "../third-party/libffi/libffi64d.lib")
#endif // NDEBUG
#else
#if NDEBUG
#pragma comment(lib, "../third-party/libffi/libffi32.lib")
#else
#pragma comment(lib, "../third-party/libffi/libffi32d.lib")
#endif // NDEBUG
#endif // _WIN64
#endif // VMP_GNU

class Core;

class Script : public IObject
{
public:
	Script(Core *owner);
	~Script();
	void clear() { text_.clear(); }
	std::string text() const { return text_; }
	bool need_compile() const { return need_compile_; }
	void set_text(const std::string &text) { text_ = text; }
	void set_need_compile(bool need_compile);
	void DoBeforeCompilation();
	void DoAfterCompilation();
	void DoBeforeSaveFile();
	void DoAfterSaveFile();
	void DoBeforePackFile();
	static Core *core() { return core_; }
	bool Compile();
	bool LoadFromFile(const std::string &file_name);
private:
	void ExecuteFunction(const std::string &func_name);
	void close();
	void ShowError();
	static Core *core_;
	lua_State *state_;
	std::string text_;
	bool need_compile_;
};

class Uint64Binder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "uint64"; }
private:
	static int add(lua_State *L);
	static int sub(lua_State *L);
	static int mul(lua_State *L);
	static int div(lua_State *L);
	static int mod(lua_State *L);
	static int pow(lua_State *L);
	static int unm(lua_State *L);
	static int eq(lua_State *L);
	static int lt(lua_State *L);
	static int le(lua_State *L);
	static int _new(lua_State *L);
	static int tostring(lua_State *L);
};

class FFILibrary : public IObject
{
public:
	FFILibrary(const std::string &name);
	~FFILibrary();
	void close();
	HMODULE value() const { return value_; }
	std::string name() const { return name_; }
private:
	HMODULE value_;
	std::string name_;
};

class FFIFunction : public IObject
{
public:
	FFIFunction(HMODULE module, const std::string &name);
	void *value() const { return value_; }
	std::string name() const { return name_; }
	int ret() const { return ret_; }
	void set_ret(int ret) { ret_ = ret; }
	void set_abi(int abi) { abi_ = abi; }
	void add_param(int param) { params_.push_back(param); }
	ffi_status Prepare();
	void Call(void *ret, void **args);
	std::vector<int> params() const { return params_; }
private:
	std::string name_;
	void *value_;
	int ret_;
	int abi_;
	std::vector<int> params_;
	ffi_cif cif_;
	ffi_type *ffi_ret_;
	std::vector<ffi_type *> ffi_params_;
};

class FFILibraryBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FFILibrary"; }
private:
	static int tostring(lua_State *state);
	static int close(lua_State *state);
	static int gc(lua_State *state);
	static int get_function(lua_State *state);
	static int address(lua_State *state);
};

class FFIFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FFIFunction"; }
private:
	static int tostring(lua_State *state);
	static int gc(lua_State *state);
	static int address(lua_State *state);
	static int call(lua_State *state);
};

class OperandTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "OperandType"; }
};

class OperandSizeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "OperandSize"; }
};

class ObjectTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "ObjectType"; }
};

class CommandOptionBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "CommandOption"; }
};

class FoldersBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Folders"; }
private:
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int add(lua_State *state);
	static int clear(lua_State *state);
};

class FolderBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Folder"; }
private:
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int add(lua_State *state);
	static int clear(lua_State *state);
	static int name(lua_State *state);
	static int destroy(lua_State *state);
};

class PEFileBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEFile"; }
private:
	static int name(lua_State *state);
	static int format(lua_State *state);
	static int size(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int flush(lua_State *state);
	static int read(lua_State *state);
};

class PEArchitectureBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEArchitecture"; }
private:
	static int segments(lua_State *state);
	static int sections(lua_State *state);
	static int name(lua_State *state);
	static int file(lua_State *state);
	static int entry_point(lua_State *state);
	static int image_base(lua_State *state);
	static int cpu_address_size(lua_State *state);
	static int dll_characteristics(lua_State *state);
	static int size(lua_State *state);
	static int functions(lua_State *state);
	static int directories(lua_State *state);
	static int imports(lua_State *state);
	static int exports(lua_State *state);
	static int resources(lua_State *state);
	static int fixups(lua_State *state);
	static int map_functions(lua_State *state);
	static int folders(lua_State *state);
	static int address_seek(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int read(lua_State *state);
};

class PESegmentsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PESegments"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class PESegmentBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PESegment"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int set_name(lua_State *state);
	static int physical_offset(lua_State *state);
	static int physical_size(lua_State *state);
	static int flags(lua_State *state);
	static int excluded_from_packing(lua_State *state);
	static int destroy(lua_State *state);
};

class PESectionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PESections"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class PESectionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PESection"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int offset(lua_State *state);
	static int segment(lua_State *state);
};

class PEDirectoriesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEDirectories"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByType(lua_State *state);
};

class PEDirectoryBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEDirectory"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
	static int set_size(lua_State *state);
	static int set_address(lua_State *state);
	static int clear(lua_State *state);
};

class PEFormatBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "PE"; }
};

class PEImportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEImports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class PEImportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEImport"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int name(lua_State *state);
	static int is_sdk(lua_State *state);
	static int set_name(lua_State *state);
};

class PEImportFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEImportFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class APITypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "APIType"; }
};

class PEExportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEExports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int name(lua_State *state);
	static int set_name(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class PEExportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEExport"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int set_name(lua_State *state);
	static int ordinal(lua_State *state);
	static int forwarded_name(lua_State *state);
	static int destroy(lua_State *state);
};

class PEResourcesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEResources"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int GetItemByName(lua_State *state);
	static int GetItemByType(lua_State *state);
};

class PEResourceBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEResource"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int address(lua_State *state);
	static int size(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
	static int is_directory(lua_State *state);
	static int destroy(lua_State *state);
	static int GetItemByName(lua_State *state);
	static int excluded_from_packing(lua_State *state);
	static int set_excluded_from_packing(lua_State *state);
};

class PEFixupsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEFixups"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int IndexOf(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class PEFixupBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "PEFixup"; }
private:
	static int address(lua_State *state);
	static int set_deleted(lua_State *state);
};

class MacFileBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacFile"; }
private:
	static int name(lua_State *state);
	static int format(lua_State *state);
	static int size(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int flush(lua_State *state);
	static int read(lua_State *state);
};

class MacArchitectureBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacArchitecture"; }
private:
	static int segments(lua_State *state);
	static int sections(lua_State *state);
	static int name(lua_State *state);
	static int file(lua_State *state);
	static int entry_point(lua_State *state);
	static int image_base(lua_State *state);
	static int cpu_address_size(lua_State *state);
	static int size(lua_State *state);
	static int functions(lua_State *state);
	static int commands(lua_State *state);
	static int symbols(lua_State *state);
	static int imports(lua_State *state);
	static int exports(lua_State *state);
	static int fixups(lua_State *state);
	static int map_functions(lua_State *state);
	static int folders(lua_State *state);
	static int address_seek(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int read(lua_State *state);
};

class MacSegmentsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSegments"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class MacSegmentBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSegment"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int physical_offset(lua_State *state);
	static int physical_size(lua_State *state);
	static int flags(lua_State *state);
	static int excluded_from_packing(lua_State *state);
};

class MacSectionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSections"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class MacSectionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSection"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int offset(lua_State *state);
	static int segment(lua_State *state);
	static int destroy(lua_State *state);
};

class MacCommandsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacCommands"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByType(lua_State *state);
};

class MacCommandBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacCommand"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class MacFormatBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "MachO"; }
};

class MacSymbolsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSymbols"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class MacSymbolBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacSymbol"; }
private:
	static int value(lua_State *state);
	static int name(lua_State *state);
	static int sect(lua_State *state);
	static int desc(lua_State *state);
};

class MacImportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacImports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class MacImportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacImport"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int name(lua_State *state);
	static int is_sdk(lua_State *state);
	static int set_name(lua_State *state);
};

class MacImportFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacImportFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class MacFixupsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacFixups"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int IndexOf(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class MacFixupBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacFixup"; }
private:
	static int address(lua_State *state);
	static int set_deleted(lua_State *state);
};

class MacExportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacExports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int name(lua_State *state);
	static int set_name(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class MacExportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MacExport"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int forwarded_name(lua_State *state);
	static int destroy(lua_State *state);
};

class ELFFileBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFFile"; }
private:
	static int name(lua_State *state);
	static int format(lua_State *state);
	static int size(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int flush(lua_State *state);
	static int read(lua_State *state);
};

class ELFArchitectureBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFArchitecture"; }
private:
	static int segments(lua_State *state);
	static int sections(lua_State *state);
	static int name(lua_State *state);
	static int file(lua_State *state);
	static int entry_point(lua_State *state);
	static int image_base(lua_State *state);
	static int cpu_address_size(lua_State *state);
	static int size(lua_State *state);
	static int functions(lua_State *state);
	static int directories(lua_State *state);
	static int imports(lua_State *state);
	static int exports(lua_State *state);
	static int fixups(lua_State *state);
	static int map_functions(lua_State *state);
	static int folders(lua_State *state);
	static int address_seek(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int read(lua_State *state);
};

class ELFFormatBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "ELF"; }
};

class ELFSegmentsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFSegments"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class ELFSegmentBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFSegment"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int physical_offset(lua_State *state);
	static int physical_size(lua_State *state);
	static int flags(lua_State *state);
	static int excluded_from_packing(lua_State *state);
};

class ELFSectionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFSections"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class ELFSectionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFSection"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int offset(lua_State *state);
	static int segment(lua_State *state);
};

class ELFDirectoriesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFDirectories"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByType(lua_State *state);
};

class ELFDirectoryBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFDirectory"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class ELFImportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFImports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class ELFImportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFImport"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int name(lua_State *state);
	static int is_sdk(lua_State *state);
};

class ELFImportFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFImportFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class ELFExportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFExports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int name(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class ELFExportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFExport"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int ordinal(lua_State *state);
	static int forwarded_name(lua_State *state);
	static int destroy(lua_State *state);
};

class ELFFixupsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFFixups"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int IndexOf(lua_State *state);
	static int GetItemByAddress(lua_State *state);
};

class ELFFixupBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ELFFixup"; }
private:
	static int address(lua_State *state);
	static int set_deleted(lua_State *state);
};

class NETArchitectureBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETArchitecture"; }
private:
	static int segments(lua_State *state);
	static int sections(lua_State *state);
	static int name(lua_State *state);
	static int file(lua_State *state);
	static int entry_point(lua_State *state);
	static int image_base(lua_State *state);
	static int cpu_address_size(lua_State *state);
	static int size(lua_State *state);
	static int functions(lua_State *state);
	static int streams(lua_State *state);
	static int imports(lua_State *state);
	static int exports(lua_State *state);
	static int map_functions(lua_State *state);
	static int folders(lua_State *state);
	static int address_seek(lua_State *state);
	static int seek(lua_State *state);
	static int tell(lua_State *state);
	static int write(lua_State *state);
	static int read(lua_State *state);
};

class NETFormatBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "NET"; }
};

class ILStream;
class ILToken;

class NETMetaDataBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETMetaData"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int heap(lua_State *state);
	static void push_stream(lua_State *state, ILStream *stream);
	static int table(lua_State *state);
};

class NETStreamBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETStream"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
};

class NETHeapBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETHeap"; }
private:
	static int size(lua_State *state);
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByType(lua_State *state);
};

class TokenTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "TokenType"; }
};

class NETTableBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETTable"; }
private:
	static int type(lua_State *state);
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class NETTokenTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "TokenType"; }
};

class NETTokenBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETToken"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int can_rename(lua_State *state);
	static int set_can_rename(lua_State *state);
};

class NETMethodsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETMethods"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class NETFieldsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETFields"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class NETTypeDefBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETTypeDef"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name_space(lua_State *state);
	static int name(lua_State *state);
	static int full_name(lua_State *state);
	static int flags(lua_State *state);
	static int declaring_type(lua_State *state);
	static int base_type(lua_State *state);
	static int method_list(lua_State *state);
	static int field_list(lua_State *state);
	static int set_namespace(lua_State *state);
	static int set_name(lua_State *state);
};

class NETTypeRefBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETTypeRef"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name_space(lua_State *state);
	static int name(lua_State *state);
	static int full_name(lua_State *state);
	static int resolution_scope(lua_State *state);
};

class NETAssemblyRefBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETAssemblyRef"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name(lua_State *state);
	static int full_name(lua_State *state);
};

class NETParamsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETParams"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class NETMethodDefBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETMethodDef"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name(lua_State *state);
	static int full_name(lua_State *state);
	static int address(lua_State *state);
	static int flags(lua_State *state);
	static int impl_flags(lua_State *state);
	static int parent(lua_State *state);
	static int param_list(lua_State *state);
	static int set_name(lua_State *state);
};

class NETFieldBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETField"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name(lua_State *state);
	static int flags(lua_State *state);
	static int declaring_type(lua_State *state);
	static int set_name(lua_State *state);
};

class NETParamBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETParam"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name(lua_State *state);
	static int parent(lua_State *state);
	static int flags(lua_State *state);
	static int set_name(lua_State *state);
};

class NETCustomAttributeBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETCustomAttribute"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int parent(lua_State *state);
	static int ref_type(lua_State *state);
};

class NETMemberRefBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETMemberRef"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int declaring_type(lua_State *state);
	static int name(lua_State *state);
};

class NETUserStringBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETUserString"; }
private:
	static int id(lua_State *state);
	static int type(lua_State *state);
	static int value(lua_State *state);
	static int name(lua_State *state);
};

class NETImportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETImport"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int name(lua_State *state);
	static int is_sdk(lua_State *state);
};

class NETImportFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETImportFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
};

class NETImportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETImports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class NETExportsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETExports"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class NETExportBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "NETExport"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
};

class ILFunctionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ILFunctions"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
	static int AddByAddress(lua_State *state);
};

class ILFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ILFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int type(lua_State *state);
	static int compilation_type(lua_State *state);
	static int set_compilation_type(lua_State *state);
	static int lock_to_key(lua_State *state);
	static int set_lock_to_key(lua_State *state);
	static int need_compile(lua_State *state);
	static int set_need_compile(lua_State *state);
	static int links(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int IndexOf(lua_State *state);
	static int destroy(lua_State *state);
	static int info(lua_State *state);
	static int ranges(lua_State *state);
	static int x_proc(lua_State *state);
	static int folder(lua_State *state);
	static int set_folder(lua_State *state);
};

class ILCommandBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "ILCommand"; }
private:
	static int address(lua_State *state);
	static int type(lua_State *state);
	static int text(lua_State *state);
	static int size(lua_State *state);
	static int dump(lua_State *state);
	static int link(lua_State *state);
	static int operand_value(lua_State *state);
	static int options(lua_State *state);
	static int alignment(lua_State *state);
	static int token_reference(lua_State *state);
};

class ILCommandTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "ILCommandType"; }
};

class MapFunctionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MapFunctions"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
};

class MapFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "MapFunction"; }
private:
	static int address(lua_State *state);
	static int size(lua_State *state);
	static int name(lua_State *state);
	static int type(lua_State *state);
	static int references(lua_State *state);
};

class ReferencesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "References"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
};

class ReferenceBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Reference"; }
private:
	static int address(lua_State *state);
	static int operand_address(lua_State *state);
	static int tag(lua_State *state);
};

class IntelFunctionsBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "IntelFunctions"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int GetItemByName(lua_State *state);
	static int AddByAddress(lua_State *state);
};

class IntelFunctionBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "IntelFunction"; }
private:
	static int address(lua_State *state);
	static int name(lua_State *state);
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int type(lua_State *state);
	static int compilation_type(lua_State *state);
	static int set_compilation_type(lua_State *state);
	static int lock_to_key(lua_State *state);
	static int set_lock_to_key(lua_State *state);
	static int need_compile(lua_State *state);
	static int set_need_compile(lua_State *state);
	static int links(lua_State *state);
	static int GetItemByAddress(lua_State *state);
	static int IndexOf(lua_State *state);
	static int destroy(lua_State *state);
	static int info(lua_State *state);
	static int ranges(lua_State *state);
	static int x_proc(lua_State *state);
	static int folder(lua_State *state);
	static int set_folder(lua_State *state);
};

class CompilationTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "CompilationType"; }
};

class CommandLinksBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "CommandLinks"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
};

class AddressRangeBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "AddressRange"; }
private:
	static int begin(lua_State *state);
	static int end(lua_State *state);
	static int begin_entry(lua_State *state);
	static int end_entry(lua_State *state);
	static int size_entry(lua_State *state);
};

class UnwindOpcodesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "UnwindOpcodes"; }
private:
	static int count(lua_State *state);
	static int item(lua_State *state);
};

class FunctionInfoBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FunctionInfo"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int begin(lua_State *state);
	static int end(lua_State *state);
	static int base_type(lua_State *state);
	static int base_value(lua_State *state);
	static int prolog_size(lua_State *state);
	static int frame_registr(lua_State *state);
	static int entry(lua_State *state);
	static int unwind_opcodes(lua_State *state);
	static int data_entry(lua_State *state);
};

class FunctionInfoListBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FunctionInfoList"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int IndexOf(lua_State *state);
};

class IntelCommandBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "IntelCommand"; }
private:
	static int address(lua_State *state);
	static int type(lua_State *state);
	static int text(lua_State *state);
	static int size(lua_State *state);
	static int dump(lua_State *state);
	static int link(lua_State *state);
	static int flags(lua_State *state);
	static int base_segment(lua_State *state);
	static int operand(lua_State *state);
	static int preffix(lua_State *state);
	static int options(lua_State *state);
	static int alignment(lua_State *state);
	static int set_dump(lua_State *state);
};

class IntelOperandBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "IntelOperand"; }
private:
	static int type(lua_State *state);
	static int size(lua_State *state);
	static int registr(lua_State *state);
	static int base_registr(lua_State *state);
	static int scale(lua_State *state);
	static int value(lua_State *state);
	static int address_size(lua_State *state);
	static int value_size(lua_State *state);
	static int fixup(lua_State *state);
	static int is_large_value(lua_State *state);
};

class IntelCommandTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "IntelCommandType"; }
};

class IntelSegmentBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "IntelSegment"; }
};

class IntelFlagBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "IntelFlag"; }
};

class IntelRegistrBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "IntelRegistr"; }
};

class CommandLinkBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "CommandLink"; }
private:
	static int to_address(lua_State *state);
	static int type(lua_State *state);
	static int from(lua_State *state);
	static int parent(lua_State *state);
	static int operand(lua_State *state);
	static int sub_value(lua_State *state);
	static int base_function_info(lua_State *state);
};

class LinkTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "LinkType"; }
};

class CoreBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Core"; }
private:
	static int extract_file_path(lua_State *state);
	static int extract_file_name(lua_State *state);
	static int extract_file_ext(lua_State *state);
	static int expand_environment_variables(lua_State *state);
	static int set_environment_variable(lua_State *state);
	static int command_line(lua_State *state);

	static int input_file(lua_State *state);
	static int input_file_name(lua_State *state);
	static int output_file(lua_State *state);
	static int output_file_name(lua_State *state);
	static int set_output_file_name(lua_State *state);
	static int project_file_name(lua_State *state);
	static int instance(lua_State* state);
	static int watermarks(lua_State *state);
	static int watermark_name(lua_State *state);
	static int set_watermark_name(lua_State *state);
	static int options(lua_State *state);
	static int set_options(lua_State *state);
	static int vm_section_name(lua_State *state);
	static int set_vm_section_name(lua_State *state);
	static int save_project(lua_State *state);
	static int input_architecture(lua_State *state);
	static int output_architecture(lua_State *state);
#ifdef ULTIMATE
	static int licenses(lua_State *state);
	static int files(lua_State *state);
#endif
};

class ProjectOptionBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "ProjectOption"; }
};

#ifdef ULTIMATE

class LicensesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Licenses"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int public_exp(lua_State *state);
	static int private_exp(lua_State *state);
	static int modulus(lua_State *state);
	static int key_length(lua_State *state);
	static int hash(lua_State *state);
	static int GetLicenseBySerialNumber(lua_State *state);
	static int import_license(lua_State *state);
};

class LicenseInfoBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "LicenseInfo"; }
private:
	static int flags(lua_State *state);
	static int customer_name(lua_State *state);
	static int customer_email(lua_State *state);
	static int expire_date(lua_State *state);
	static int hwid(lua_State *state);
	static int running_time_limit(lua_State *state);
	static int max_build_date(lua_State *state);
	static int user_data(lua_State *state);
};

class LicenseBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "License"; }
private:
	static int date(lua_State *state);
	static int customer_name(lua_State *state);
	static int customer_email(lua_State *state);
	static int order_ref(lua_State *state);
	static int comments(lua_State *state);
	static int serial_number(lua_State *state);
	static int blocked(lua_State *state);
	static int set_blocked(lua_State *state);
	static int info(lua_State *state);
};

class FileFoldersBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FileFolders"; }
private:
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int add(lua_State *state);
	static int clear(lua_State *state);
};

class FileFolderBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "FileFolder"; }
private:
	static int count(lua_State *state);
	static int item(lua_State *state);
	static int add(lua_State *state);
	static int clear(lua_State *state);
	static int name(lua_State *state);
	static int destroy(lua_State *state);
};

class FilesBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Files"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int folders(lua_State *state);
	static int Delete(lua_State *state);
};

class FileActionTypeBinder
{
public:
	static void Register(lua_State *state);
	static const char *enum_name() { return "FileActionType"; }
};

class FileBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "File"; }
private:
	static int name(lua_State *state);
	static int file_name(lua_State *state);
	static int action(lua_State *state);
	static int folder(lua_State *state);
	static int set_name(lua_State *state);
	static int set_file_name(lua_State *state);
	static int set_action(lua_State *state);
};

#endif

class WatermarksBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Watermarks"; }
private:
	static int item(lua_State *state);
	static int count(lua_State *state);
	static int clear(lua_State *state);
	static int Delete(lua_State *state);
	static int GetItemByName(lua_State *state);
	static int add(lua_State *state);
};

class WatermarkBinder
{
public:
	static void Register(lua_State *state);
	static const char *class_name() { return "Watermark"; }
private:
	static int name(lua_State *state);
	static int value(lua_State *state);
	static int blocked(lua_State *state);
	static int set_blocked(lua_State *state);
};

#endif