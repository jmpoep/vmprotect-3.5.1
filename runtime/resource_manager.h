#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#pragma pack(push, 1)

class CipherRC5;

struct RESOURCE_DIRECTORY {
	uint32_t NumberOfNamedEntries;
	uint32_t NumberOfIdEntries;
	// RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];
};

//#pragma warning(push)
//#pragma warning(disable:4201) // named type definition in parentheses
struct RESOURCE_DIRECTORY_ENTRY {
    union {
        struct {
            uint32_t NameOffset:31;
            uint32_t NameIsString:1;
        } /*DUMMYSTRUCTNAME*/;
        uint32_t Name;
        uint16_t Id;
    } /*DUMMYUNIONNAME*/;
    union {
        uint32_t OffsetToData;
        struct {
            uint32_t OffsetToDirectory:31;
            uint32_t DataIsDirectory:1;
        } /*DUMMYSTRUCTNAME2*/;
    } /*DUMMYUNIONNAME2*/;
};
//#pragma warning(pop)

struct RESOURCE_DATA_ENTRY {
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t CodePage;
	uint32_t Reserved;
};

#pragma pack(pop)

struct LDR_RESOURCE_INFO {
	LPWSTR Type;
	LPWSTR Name;
	WORD Language;
 };

class VirtualResource
{
public:
	VirtualResource(const RESOURCE_DATA_ENTRY *entry, uint32_t key);
	~VirtualResource();
	HGLOBAL	Decrypt(HMODULE instance, uint32_t key);
	const RESOURCE_DATA_ENTRY *entry() const { return entry_; }
	HRSRC handle() const { return reinterpret_cast<HRSRC>(const_cast<IMAGE_RESOURCE_DATA_ENTRY *>(&handle_)); }
private:
	const RESOURCE_DATA_ENTRY *entry_;
	IMAGE_RESOURCE_DATA_ENTRY handle_;
	uint8_t	*address_;

	// no copy ctr or assignment op
	VirtualResource(const VirtualResource &);
	VirtualResource &operator =(const VirtualResource &);
};

class VirtualResourceList
{
public:
	~VirtualResourceList();
	VirtualResource *Add(const RESOURCE_DATA_ENTRY *entry, uint32_t key);
	void Delete(size_t index);
	size_t IndexByEntry(const RESOURCE_DATA_ENTRY *entry) const;
	size_t IndexByHandle(HRSRC handle) const;
	VirtualResource *operator [] (size_t index) const { return v_[index]; }
	size_t size() const { return v_.size(); }
private:
	vector<VirtualResource *> v_;
};

class ResourceManager
{
public:
	ResourceManager(const uint8_t *data, HMODULE instance, const uint8_t *key);
	~ResourceManager();

	void HookAPIs(HookManager &hook_manager);
	void UnhookAPIs(HookManager &hook_manager);

	HGLOBAL LoadResource(HMODULE module, HRSRC res_info);
	HRSRC FindResourceA(HMODULE module, LPCSTR name, LPCSTR type);
	HRSRC FindResourceExA(HMODULE module, LPCSTR type, LPCSTR name, WORD language);
	HRSRC FindResourceW(HMODULE module, LPCWSTR name, LPCWSTR type);
	HRSRC FindResourceExW(HMODULE module, LPCWSTR type, LPCWSTR name, WORD language);
	int LoadStringA(HMODULE module, UINT id, LPSTR buffer, int buffer_max);
	int LoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max);
	NTSTATUS LdrFindResource_U(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry);
	NTSTATUS LdrAccessResource(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *entry, void **res, ULONG *size);
	BOOL EnumResourceNamesA(HMODULE module, LPCSTR type, ENUMRESNAMEPROCA enum_func, LONG_PTR param);
	BOOL EnumResourceNamesW(HMODULE module, LPCWSTR type, ENUMRESNAMEPROCW enum_func, LONG_PTR param);
	BOOL EnumResourceLanguagesA(HMODULE module, LPCSTR type, LPCSTR name, ENUMRESLANGPROCA enum_func, LONG_PTR param);
	BOOL EnumResourceLanguagesW(HMODULE module, LPCWSTR type, LPCWSTR name, ENUMRESLANGPROCW enum_func, LONG_PTR param);
	BOOL EnumResourceTypesA(HMODULE module, ENUMRESTYPEPROCA enum_func, LONG_PTR param);
	BOOL EnumResourceTypesW(HMODULE module, ENUMRESTYPEPROCW enum_func, LONG_PTR param);
private:
	RESOURCE_DIRECTORY DecryptDirectory(const RESOURCE_DIRECTORY *directory_enc) const;
	RESOURCE_DIRECTORY_ENTRY DecryptDirectoryEntry(const RESOURCE_DIRECTORY_ENTRY *entry_enc) const;
	int CompareStringEnc(LPCWSTR name, LPCWSTR str_enc) const;
	LPWSTR DecryptStringW(LPCWSTR src_enc) const;
	LPSTR DecryptStringA(LPCWSTR src_enc) const;
	const RESOURCE_DIRECTORY *FindEntryById(const RESOURCE_DIRECTORY *directory, WORD id, DWORD dir_type);
	const RESOURCE_DIRECTORY *FindEntryByName(const RESOURCE_DIRECTORY *directory, LPCWSTR name, DWORD dir_type);
	const RESOURCE_DIRECTORY *FindEntry(const RESOURCE_DIRECTORY *directory, LPCWSTR name, DWORD dir_type);
	const RESOURCE_DIRECTORY *FindFirstEntry(const RESOURCE_DIRECTORY *directory, DWORD dir_type);
	const RESOURCE_DIRECTORY *FindResourceEntry(LPCWSTR type, LPCWSTR name, WORD language);
	HRSRC InternalFindResourceExW(HMODULE module, LPCWSTR type, LPCWSTR name, WORD language);
	HGLOBAL InternalLoadResource(HMODULE module, HRSRC res_info);
	LPWSTR InternalFindStringResource(HMODULE module, UINT id);
	int InternalLoadStringW(LPCWSTR res, LPWSTR buffer, int buffer_max);
	int InternalLoadStringA(LPCWSTR res, LPSTR buffer, int buffer_max);
	uint16_t DecryptWord(uint16_t value) const;

	int TrueLoadStringA(HMODULE module, UINT id, LPSTR buffer, int buffer_max);
	int TrueLoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max);
	NTSTATUS TrueLdrFindResource_U(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry);
	NTSTATUS TrueLdrAccessResource(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *entry, void **res, ULONG *size);

	HMODULE instance_;
	uint32_t key_;
	CRITICAL_SECTION critical_section_;
	VirtualResourceList resources_;
	const uint8_t *data_;
	void *load_resource_;
	void *load_string_a_;
	void *load_string_w_;
	void *load_string_a_kernel_;
	void *load_string_w_kernel_;
	void *ldr_find_resource_u_;
	void *ldr_access_resource_;
	void *get_thread_ui_language_;

	// no copy ctr or assignment op
	ResourceManager(const ResourceManager &);
	ResourceManager &operator =(const ResourceManager &);
};

#endif