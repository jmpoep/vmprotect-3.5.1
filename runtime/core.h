#ifndef CORE_H
#define CORE_H

class StringManager;
class LicensingManager;
class HardwareID;

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#ifndef NTDDI_WIN7 //SDK 6.0
typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation = 0,
	ObjectTypeInformation = 2
} OBJECT_INFORMATION_CLASS;

typedef struct _PUBLIC_OBJECT_BASIC_INFORMATION {
	ULONG Attributes;
	ACCESS_MASK GrantedAccess;
	ULONG HandleCount;
	ULONG PointerCount;

	ULONG Reserved[10];    // reserved for internal use

} PUBLIC_OBJECT_BASIC_INFORMATION, *PPUBLIC_OBJECT_BASIC_INFORMATION;
#endif

class ResourceManager;
class FileManager;
class RegistryManager;
class HookManager;

enum VirtualObjectType {
	OBJECT_FILE,
	OBJECT_SECTION,
	OBJECT_MAP,
	OBJECT_KEY,
};

class VirtualObject
{
public:
	VirtualObject(VirtualObjectType type, void *ref, HANDLE handle, uint32_t access);
	~VirtualObject();
	void *ref() const { return ref_; }
	VirtualObjectType type() const { return type_; }
	HANDLE handle() const { return handle_; }
	uint64_t file_position() const { return file_position_; }
	void set_file_position(uint64_t position) { file_position_ = position; }
	uint32_t attributes() const { return attributes_; }
	void set_attributes(uint32_t attributes) { attributes_ = attributes; }
	uint32_t access() const { return access_; }
private:
	void *ref_;
	HANDLE handle_;
	VirtualObjectType type_;
	uint64_t file_position_;
	uint32_t attributes_;
	uint32_t access_;
};

class VirtualObjectList
{
public:
	VirtualObjectList();
	~VirtualObjectList();
	VirtualObject *Add(VirtualObjectType type, void *ref, HANDLE handle, uint32_t access);
	void DeleteObject(HANDLE handle);
	void DeleteRef(void *ref, HANDLE handle = 0);
	VirtualObject *GetObject(HANDLE handle) const;
	VirtualObject *GetFile(HANDLE handle) const;
	VirtualObject *GetSection(HANDLE handle) const;
	VirtualObject *GetMap(HANDLE process, void *map) const;
	VirtualObject *GetKey(HANDLE handle) const;
	VirtualObject *operator [] (size_t index) const { return v_[index]; }
	size_t size() const { return v_.size(); }
	CRITICAL_SECTION &critical_section() { return critical_section_; };
	uint32_t GetHandleCount(HANDLE handle) const;
	uint32_t GetPointerCount(const void *ref) const;
private:
	void Delete(size_t index);
	CRITICAL_SECTION critical_section_;
	vector<VirtualObject *> v_;
};
#endif

#ifdef VMP_GNU
EXPORT_API extern GlobalData *loader_data;
#else
extern GlobalData *loader_data;
#endif

class Core
{
public:
	static Core *Instance();
	static void Free();
	bool Init(HMODULE instance);
	~Core();
	StringManager *string_manager() const { return string_manager_; }
	LicensingManager *licensing_manager() const { return licensing_manager_; }
	HardwareID *hardware_id();
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	NTSTATUS NtProtectVirtualMemory(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect);
	NTSTATUS NtClose(HANDLE Handle);
	NTSTATUS NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
	NTSTATUS TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
	ResourceManager *resource_manager() const { return resource_manager_; }
	FileManager *file_manager() const { return file_manager_; }
	RegistryManager *registry_manager() const { return registry_manager_; }
#endif
protected:
	Core();
private:
	StringManager *string_manager_;
	LicensingManager *licensing_manager_;
	HardwareID *hardware_id_;
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	NTSTATUS TrueNtProtectVirtualMemory(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect);
	NTSTATUS TrueNtClose(HANDLE Handle);
	void HookAPIs(HookManager &hook_manager, uint32_t options);
	void UnhookAPIs(HookManager &hook_manager);
	VirtualObjectList objects_;
	ResourceManager *resource_manager_;
	FileManager *file_manager_;
	RegistryManager *registry_manager_;
	HookManager *hook_manager_;
	void *nt_protect_virtual_memory_;
	void *nt_close_;
	void *nt_query_object_;
	void *dbg_ui_remote_breakin_;
#endif
	static Core *self_;

    // no copy ctr or assignment op
    Core(const Core &);
    Core &operator=(const Core &);
};

#endif