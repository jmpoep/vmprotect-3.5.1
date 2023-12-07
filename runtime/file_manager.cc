#ifdef WIN_DRIVER
#else
#include "common.h"
#include "objects.h"

#include "crypto.h"
#include "core.h"
#include "file_manager.h"
#include "hook_manager.h"
#include "utils.h"
#include "registry_manager.h"

/**
 * hooked functions
 */

NTSTATUS WINAPI HookedNtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQueryAttributesFile(ObjectAttributes, FileInformation);
}

NTSTATUS WINAPI HookedNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS WINAPI HookedNtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

NTSTATUS WINAPI HookedNtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQueryInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS WINAPI HookedNtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQueryVolumeInformationFile(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

NTSTATUS WINAPI HookedNtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtSetInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS WINAPI HookedNtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
	ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQueryDirectoryFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

NTSTATUS WINAPI HookedNtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS WINAPI HookedNtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtCreateSection(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
}

NTSTATUS WINAPI HookedNtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQuerySection(SectionHandle, InformationClass, InformationBuffer, InformationBufferSize, ResultLength);
}

NTSTATUS WINAPI HookedNtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

NTSTATUS WINAPI HookedNtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtUnmapViewOfSection(ProcessHandle, BaseAddress);
}

NTSTATUS WINAPI HookedNtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength)
{
	FileManager *file_manager = Core::Instance()->file_manager();
	return file_manager->NtQueryVirtualMemory(ProcessHandle, BaseAddress, MemoryInformationClass, Buffer, Length, ResultLength);
}

/**
 * FileManager
 */

FileManager::FileManager(const uint8_t *data, HMODULE instance, const uint8_t *key, VirtualObjectList *objects)
	: instance_(instance)
	, data_(data)
	, objects_(objects)
	, nt_query_attributes_file_(NULL)
	, nt_create_file_(NULL)
	, nt_open_file_(NULL)
	, nt_read_file_(NULL)
	, nt_query_information_file_(NULL)
	, nt_query_volume_information_file_(NULL)
	, nt_set_information_file_(NULL)
	, nt_query_directory_file_(NULL)
	, nt_close_(NULL)
	, nt_create_section_(NULL)
	, nt_query_section_(NULL)
	, nt_map_view_of_section_(NULL)
	, nt_unmap_view_of_section_(NULL)
	, nt_query_virtual_memory_(NULL)
{
	wchar_t dos_path[MAX_PATH];
	wchar_t nt_path[MAX_PATH];

	key_ = *(reinterpret_cast<const uint32_t *>(key));

	if (GetModuleFileNameW(NULL, dos_path, _countof(dos_path))) {
		wchar_t *name = wcsrchr(dos_path, L'\\');
		name = (name ? name + 1 : dos_path);
		*name = 0;
	} else {
		dos_path[0] = 0;
	}

	nt_path[0] = 0;
	HANDLE dir = CreateFileW(dos_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (dir != INVALID_HANDLE_VALUE) {
		typedef NTSTATUS (WINAPI tNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
		HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
		tNtQueryObject *nt_query_object = static_cast<tNtQueryObject *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("NtQueryObject")));
		if (nt_query_object) {
			DWORD size = 0x800;
			OBJECT_NAME_INFORMATION *info = reinterpret_cast<OBJECT_NAME_INFORMATION *>(new uint8_t[size]);
			if (NT_SUCCESS(nt_query_object(dir, ObjectNameInformation, info, size, NULL))) {
				memcpy(nt_path, info->Name.Buffer, info->Name.Length);
				size = info->Name.Length / sizeof(wchar_t);
				nt_path[size] = '\\';
				nt_path[size + 1] = 0;
			}
			delete [] info;
		}
		CloseHandle(dir);
	}

	wchar_t *dos_end = dos_path + wcslen(dos_path);
	wchar_t *nt_end = nt_path + wcslen(nt_path);
	while (dos_end > dos_path && nt_end > nt_path) {
		if (_wcsnicmp(dos_end, nt_end, 1) != 0)
			break;
		dos_end--;
		nt_end--;
	}
	dos_end++;
	nt_end++;
	if (*dos_end == '\\') {
		dos_end++;
		nt_end++;
	}
	dos_root_.append(dos_path, dos_end - dos_path);
	nt_root_.append(nt_path, nt_end - nt_path);

	Path path(dos_end);
	const OBJECT_DIRECTORY *directory_enc = reinterpret_cast<const OBJECT_DIRECTORY *>(data_);
	const OBJECT_ENTRY *entry_enc = reinterpret_cast<const OBJECT_ENTRY *>(directory_enc + 1);
	OBJECT_DIRECTORY directory = DecryptDirectory(directory_enc);
	for (size_t i = 0; i < directory.NumberOfEntries; i++)	{
		OBJECT_ENTRY entry = DecryptEntry(&entry_enc[i]);
		if (DecryptString(reinterpret_cast<LPCWSTR>(reinterpret_cast<uint8_t *>(instance_) + entry.NameOffset), dos_path, _countof(dos_path))) {
			file_name_list_.push_back(path.Combine(dos_path));
		} else {
			file_name_list_.push_back(UnicodeString());
		}
	}
}

void FileManager::HookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	nt_query_attributes_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryAttributesFile"), &HookedNtQueryAttributesFile);
	nt_create_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtCreateFile"), &HookedNtCreateFile);
	nt_open_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtOpenFile"), &HookedNtOpenFile);
	nt_read_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtReadFile"), &HookedNtReadFile);
	nt_query_information_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryInformationFile"), &HookedNtQueryInformationFile);
	nt_query_volume_information_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryVolumeInformationFile"), &HookedNtQueryVolumeInformationFile);
	nt_set_information_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtSetInformationFile"), &HookedNtSetInformationFile);
	nt_query_directory_file_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryDirectoryFile"), &HookedNtQueryDirectoryFile);
	nt_create_section_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtCreateSection"), &HookedNtCreateSection);
	nt_query_section_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQuerySection"), &HookedNtQuerySection);
	nt_map_view_of_section_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtMapViewOfSection"), &HookedNtMapViewOfSection);
	nt_unmap_view_of_section_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtUnmapViewOfSection"), &HookedNtUnmapViewOfSection);
	nt_query_virtual_memory_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryVirtualMemory"), &HookedNtQueryVirtualMemory);
	hook_manager.End();
}

void FileManager::UnhookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	hook_manager.UnhookAPI(nt_query_attributes_file_);
	hook_manager.UnhookAPI(nt_create_file_);
	hook_manager.UnhookAPI(nt_open_file_);
	hook_manager.UnhookAPI(nt_read_file_);
	hook_manager.UnhookAPI(nt_query_information_file_);
	hook_manager.UnhookAPI(nt_query_volume_information_file_);
	hook_manager.UnhookAPI(nt_set_information_file_);
	hook_manager.UnhookAPI(nt_query_directory_file_);
	hook_manager.UnhookAPI(nt_create_section_);
	hook_manager.UnhookAPI(nt_query_section_);
	hook_manager.UnhookAPI(nt_map_view_of_section_);
	hook_manager.UnhookAPI(nt_unmap_view_of_section_);
	hook_manager.UnhookAPI(nt_query_virtual_memory_);
	hook_manager.End();
}

bool FileManager::DecryptString(LPCWSTR str_enc, LPWSTR str, size_t count) const
{
	for (size_t i = 0; i < count; i++) {
		str[i] = static_cast<wchar_t>(str_enc[i] ^ (_rotl32(key_, static_cast<int>(i)) + i));
		if (!str[i])
			return true;
	}
	// source string is too long
	return false;
}

bool FileManager::OpenFiles(RegistryManager &registry_manager)
{
	const OBJECT_DIRECTORY *directory_enc = reinterpret_cast<const OBJECT_DIRECTORY *>(data_);
	const OBJECT_ENTRY *entry_enc = reinterpret_cast<const OBJECT_ENTRY *>(directory_enc + 1);

	wchar_t file_name[MAX_PATH];
	OBJECT_DIRECTORY directory = DecryptDirectory(directory_enc);
	for (size_t i = 0; i < directory.NumberOfEntries; i++)	{
		OBJECT_ENTRY entry = DecryptEntry(&entry_enc[i]);
		if ((entry.Options & (FILE_LOAD | FILE_REGISTER | FILE_INSTALL)) == 0)
			continue;

		if (!DecryptString(reinterpret_cast<LPCWSTR>(reinterpret_cast<uint8_t *>(instance_) + entry.NameOffset), file_name, _countof(file_name)))
			return false;

		HMODULE h = LoadLibraryW(file_name);
		if (h) {
			for (size_t j = 0; j < 2; j++) {
				size_t mask = (j == 0) ? FILE_REGISTER : FILE_INSTALL;
				if (entry.Options & mask) {
					VMP_CHAR error[MAX_PATH + 100];
					error[0] = 0;
					void *proc = InternalGetProcAddress(h, (j == 0) ? "DllRegisterServer" : "DllInstall");
					if (proc) {
						HRESULT res = S_OK;
						registry_manager.BeginRegisterServer();
						try {
							if (j == 0) {
								typedef HRESULT (WINAPI tRegisterServer)(void);
								tRegisterServer *register_server = reinterpret_cast<tRegisterServer *>(proc);
								res = register_server();
							} else {
								typedef HRESULT (WINAPI tInstallServer)(BOOL, LPCWSTR);
								tInstallServer *install_server = reinterpret_cast<tInstallServer *>(proc);
								res = install_server(TRUE, L"");
							}
						} catch(...) {
							res = 0xC0000005;
						}
						registry_manager.EndRegisterServer();
						if (res != S_OK)
							swprintf_s(error, L"Cannot %s server %s\nError: 0x%X", (j == 0) ? L"register" : L"install", file_name, res);
					} else {
						swprintf_s(error, L"The procedure entry point %s could not be located in the module %s", (j == 0) ? L"DllRegisterServer" : L"DllInstall", file_name);
					}
					if (error[0]) {
						FreeLibrary(h);
						ShowMessage(error);
						return false;
					}
				}
			}
			if ((entry.Options & FILE_LOAD) == 0)
				FreeLibrary(h);
		} else {
			VMP_CHAR error[MAX_PATH + 100];
			swprintf_s(error, L"Cannot load file %s\nError: %d", file_name, GetLastError());
			ShowMessage(error);
			return false;
		}
	}
	return true;
}

NTSTATUS __forceinline FileManager::TrueNtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation)
{
	typedef NTSTATUS (WINAPI tNtQueryAttributesFile)(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
	return reinterpret_cast<tNtQueryAttributesFile *>(nt_query_attributes_file_)(ObjectAttributes, FileInformation);
}

NTSTATUS __forceinline FileManager::TrueNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	typedef NTSTATUS (WINAPI tNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
	return reinterpret_cast<tNtCreateFile *>(nt_create_file_)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS __forceinline FileManager::TrueNtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions)
{
	typedef NTSTATUS (WINAPI tNtOpenFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
	return reinterpret_cast<tNtOpenFile *>(nt_open_file_)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

NTSTATUS __forceinline FileManager::TrueNtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	typedef NTSTATUS (WINAPI tNtQueryInformationFile)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	return reinterpret_cast<tNtQueryInformationFile *>(nt_query_information_file_)(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS __forceinline FileManager::TrueNtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass)
{
	typedef NTSTATUS (WINAPI tNtQueryVolumeInformationFile)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass);
	return reinterpret_cast<tNtQueryVolumeInformationFile *>(nt_query_volume_information_file_)(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

NTSTATUS __forceinline FileManager::TrueNtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	typedef NTSTATUS (WINAPI tNtSetInformationFile)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	return reinterpret_cast<tNtSetInformationFile *>(nt_set_information_file_)(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS __forceinline FileManager::TrueNtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan)
{
	typedef NTSTATUS(WINAPI tNtQueryDirectoryFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan);
	return reinterpret_cast<tNtQueryDirectoryFile *>(nt_query_directory_file_)(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

NTSTATUS __forceinline FileManager::TrueNtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key)
{
	typedef NTSTATUS (WINAPI tNtReadFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
	return reinterpret_cast<tNtReadFile *>(nt_read_file_)(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS __forceinline FileManager::TrueNtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle)
{
	typedef NTSTATUS (WINAPI tNtCreateSection)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);
	return reinterpret_cast<tNtCreateSection *>(nt_create_section_)(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
}

NTSTATUS __forceinline FileManager::TrueNtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtQuerySection)(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength);
	return reinterpret_cast<tNtQuerySection *>(nt_query_section_)(SectionHandle, InformationClass, InformationBuffer, InformationBufferSize, ResultLength);
}

NTSTATUS __forceinline FileManager::TrueNtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect)
{
	typedef NTSTATUS (WINAPI tNtMapViewOfSection)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
	return reinterpret_cast<tNtMapViewOfSection *>(nt_map_view_of_section_)(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

NTSTATUS __forceinline FileManager::TrueNtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
	typedef NTSTATUS (WINAPI tNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);
	return reinterpret_cast<tNtUnmapViewOfSection *>(nt_unmap_view_of_section_)(ProcessHandle, BaseAddress);
}

NTSTATUS __forceinline FileManager::TrueNtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength);
	return reinterpret_cast<tNtQueryVirtualMemory *>(nt_query_virtual_memory_)(ProcessHandle, BaseAddress, MemoryInformationClass, Buffer, Length, ResultLength);
}

OBJECT_DIRECTORY FileManager::DecryptDirectory(const OBJECT_DIRECTORY *directory_enc) const
{
	OBJECT_DIRECTORY res;
	res.NumberOfEntries = directory_enc->NumberOfEntries ^ key_;
	return res;
}

OBJECT_ENTRY FileManager::DecryptEntry(const OBJECT_ENTRY *entry_enc) const
{
	OBJECT_ENTRY res;
	res.NameOffset = entry_enc->NameOffset ^ key_;
	res.OffsetToData = entry_enc->OffsetToData ^ key_;
	res.Size = entry_enc->Size ^ key_;
	res.Options = entry_enc->Options ^ key_;
	return res;
}

NTSTATUS __forceinline FileManager::TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength)
{
	return Core::Instance()->TrueNtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

const OBJECT_ENTRY *FileManager::FindEntry(HANDLE directory, PUNICODE_STRING object_name)
{
	UnicodeString file_name;
	bool check_root = true;
	if (directory) {
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(directory);
		if (object) {
			check_root = false;
			const OBJECT_DIRECTORY *directory_enc = reinterpret_cast<const OBJECT_DIRECTORY *>(data_);
			for (size_t i = 0; i < file_name_list_.size(); i++) {
				if (object->ref() == reinterpret_cast<const OBJECT_ENTRY *>(directory_enc + 1) + i) {
					file_name = file_name_list_[i].c_str();
					break;
				}
			}
		} else {
			DWORD size = 0x800;
			OBJECT_NAME_INFORMATION *info = reinterpret_cast<OBJECT_NAME_INFORMATION *>(new uint8_t[size]);
			if (NT_SUCCESS(TrueNtQueryObject(directory, ObjectNameInformation, info, size, NULL)))
				file_name.append(info->Name.Buffer, info->Name.Length / sizeof(wchar_t));
			delete[] info;
		}
		if (file_name.size())
			file_name.append(L"\\");
	}
	file_name.append(object_name->Buffer, object_name->Length / sizeof(wchar_t));

	LPCWSTR name = file_name.c_str();
	if (check_root) {
		LPCWSTR path;
		if (name[0] == '\\' && name[1] == '?' && name[2] == '?' && name[3] == '\\') {
			path = dos_root_.c_str();
			name += 4;
			if (name[0] == 'U' && name[1] == 'N' && name[2] == 'C' && name[3] == '\\') {
				name += 3;
				if (path[0] == '\\' && path[1] == '\\')
					path++;
			}
		}
		else {
			path = nt_root_.c_str();
		}

		size_t len = wcslen(path);
		if (_wcsnicmp(name, path, len) != 0)
			return NULL;
		name += len;
	}

	const OBJECT_DIRECTORY *directory_enc = reinterpret_cast<const OBJECT_DIRECTORY *>(data_);
	for (size_t i = 0; i < file_name_list_.size(); i++) {
		UnicodeString *file_name = &file_name_list_[i];
		size_t len = file_name->size();
		if (_wcsnicmp(file_name->c_str(), name, len) == 0) {
			const OBJECT_ENTRY *entry_enc = reinterpret_cast<const OBJECT_ENTRY *>(directory_enc + 1) + i;
			if (name[len]) {
				OBJECT_ENTRY entry = DecryptEntry(entry_enc);
				if (entry.OffsetToData || name[len] != '\\' || name[len + 1])
					continue;
			}
			return entry_enc;
		}
	}

	return NULL;
}

__declspec(noinline) bool FileManager::ReadFile(const OBJECT_ENTRY *entry, uint64_t offset, void *dst, size_t size) const
{
	if (entry->Size < offset + size)
		return false;

	uint8_t *source = reinterpret_cast<uint8_t *>(instance_) + entry->OffsetToData;
	uint8_t *address = reinterpret_cast<uint8_t *>(dst);
	for (size_t p = 0; p < size; p++) {
		uint64_t i = offset + p;
		address[p] = source[i] ^ static_cast<uint8_t>(_rotl32(key_, static_cast<int>(i)) + i);
	}
	return true;
}

NTSTATUS FileManager::ReadImageHeader(const OBJECT_ENTRY *entry, IMAGE_NT_HEADERS *header) const
{
	IMAGE_DOS_HEADER dos_header;
	if (ReadFile(entry, 0, &dos_header, sizeof(dos_header)) && dos_header.e_magic == IMAGE_DOS_SIGNATURE && dos_header.e_lfanew) {
		IMAGE_NT_HEADERS nt_header;
		if (ReadFile(entry, dos_header.e_lfanew, &nt_header, sizeof(nt_header))) {
			if (nt_header.Signature == IMAGE_NT_SIGNATURE && (nt_header.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0) {
#ifdef _WIN64
				if (nt_header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC && nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
#else
				if (nt_header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC && (nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_I386 || nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_I486 || nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_I586))
#endif
					*header = nt_header;
					return STATUS_SUCCESS;
			}
		}
	}
	return STATUS_INVALID_IMAGE_FORMAT;
}

NTSTATUS FileManager::ReadImage(const OBJECT_ENTRY *entry, void *base) const
{
	IMAGE_NT_HEADERS tmp_header;
	NTSTATUS status = ReadImageHeader(entry, &tmp_header);
	if (status != STATUS_SUCCESS)
		return status;

	uint8_t *image_base = reinterpret_cast<uint8_t *>(base);
	if (!ReadFile(entry, 0, image_base, tmp_header.OptionalHeader.SizeOfHeaders))
		return STATUS_INVALID_IMAGE_FORMAT;

	IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image_base);
	IMAGE_NT_HEADERS *nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(image_base + dos_header->e_lfanew);
	IMAGE_SECTION_HEADER *sections = reinterpret_cast<IMAGE_SECTION_HEADER *>(reinterpret_cast<uint8_t *>(&nt_header->OptionalHeader) + nt_header->FileHeader.SizeOfOptionalHeader);

	// copy sections
	for (size_t i = 0; i < nt_header->FileHeader.NumberOfSections; i++) {
		IMAGE_SECTION_HEADER *section = sections + i;

		uint32_t virtual_size = section->Misc.VirtualSize;
		uint32_t physical_size = section->SizeOfRawData;

		uint8_t *address = image_base + section->VirtualAddress;
		::ZeroMemory(address, virtual_size);

		if (!ReadFile(entry, section->PointerToRawData, address, std::min(physical_size, virtual_size)))
			return STATUS_INVALID_IMAGE_FORMAT;
	}

	// setup relocs
	ptrdiff_t delta_base = image_base - reinterpret_cast<uint8_t *>(nt_header->OptionalHeader.ImageBase);
	if (delta_base) {
		IMAGE_DATA_DIRECTORY *reloc_directory = &nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		if (reloc_directory->VirtualAddress && reloc_directory->Size && (nt_header->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED) == 0) {
			size_t processed = 0;
			while (processed < reloc_directory->Size) {
				IMAGE_BASE_RELOCATION *reloc = reinterpret_cast<IMAGE_BASE_RELOCATION *>(image_base + reloc_directory->VirtualAddress + processed);
				if (!reloc->SizeOfBlock)
					break;

				if (reloc->SizeOfBlock <= sizeof(IMAGE_BASE_RELOCATION) || (reloc->SizeOfBlock & 1) != 0)
					return STATUS_INVALID_IMAGE_FORMAT;

				size_t c = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1;
				for (size_t i = 0; i < c; i++) {
					uint16_t type_offset = reinterpret_cast<uint16_t *>(reloc + 1)[i];
					uint8_t *address = image_base + reloc->VirtualAddress + (type_offset & 0xfff);
					switch (type_offset >> 12) {
					case IMAGE_REL_BASED_HIGHLOW:
						*(reinterpret_cast<uint32_t *>(address)) += static_cast<uint32_t>(delta_base);
						break;
					case IMAGE_REL_BASED_HIGH:
						*(reinterpret_cast<uint16_t *>(address)) += static_cast<uint16_t>(delta_base >> 16);
						break;
					case IMAGE_REL_BASED_LOW:
						*(reinterpret_cast<uint16_t *>(address)) += static_cast<uint16_t>(delta_base);
						break;
					case IMAGE_REL_BASED_DIR64:
						*(reinterpret_cast<uint64_t *>(address)) += delta_base;
						break;
					}
				}
				processed += reloc->SizeOfBlock;
			}
		}
		nt_header->OptionalHeader.ImageBase = reinterpret_cast<size_t>(image_base);
	}

	// setup pages protection
	DWORD old_protect;
	if (!VirtualProtect(image_base, nt_header->OptionalHeader.SizeOfHeaders, PAGE_READONLY, &old_protect))
		return STATUS_INVALID_PAGE_PROTECTION;
	
	for (size_t i = 0; i < nt_header->FileHeader.NumberOfSections; i++) {
		IMAGE_SECTION_HEADER *section = sections + i;

		ULONG protection = 0;
		if ((section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) != 0)
			protection |= PAGE_NOCACHE;
		if ((section->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) == (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) {
			protection |= PAGE_EXECUTE_READWRITE;
		} else if ((section->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)) == (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)) {
			protection |= PAGE_EXECUTE_READ;
		} else if ((section->Characteristics & (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) == (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) {
			protection |= PAGE_READWRITE;
		} else if ((section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0) {
			protection |= PAGE_WRITECOPY;
		} else if ((section->Characteristics & IMAGE_SCN_MEM_READ) != 0) {
			protection |= PAGE_READONLY;
		} else {
			protection |= PAGE_NOACCESS;
		}
		if (!VirtualProtect(image_base + section->VirtualAddress, section->Misc.VirtualSize, protection, &old_protect)) 
			return STATUS_INVALID_PAGE_PROTECTION;
	}
	return STATUS_SUCCESS;
}

NTSTATUS FileManager::NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation)
{
	const OBJECT_ENTRY *entry = FindEntry(ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName);
	if (entry) {
		try {
			FileInformation->CreationTime.QuadPart = 0;
			FileInformation->LastAccessTime.QuadPart = 0;
			FileInformation->LastWriteTime.QuadPart = 0;
			FileInformation->ChangeTime.QuadPart = 0;
			FileInformation->FileAttributes = FILE_ATTRIBUTE_READONLY;
			return STATUS_SUCCESS;
		} catch(...) {
			return STATUS_ACCESS_VIOLATION;
		}
	}

	return TrueNtQueryAttributesFile(ObjectAttributes, FileInformation);
}

NTSTATUS FileManager::NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	const OBJECT_ENTRY *entry = FindEntry(ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName);
	if (entry) {
		try {
			NTSTATUS status;
			switch (CreateDisposition) {
			case FILE_OPEN:
			case FILE_OPEN_IF:
				{
					CriticalSection	cs(objects_->critical_section());

					VirtualObject *object = objects_->Add(OBJECT_FILE, const_cast<OBJECT_ENTRY *>(entry), ::CreateEventA(NULL, false, false, NULL), DesiredAccess);
					*FileHandle = object->handle();
				}
				IoStatusBlock->Information = FILE_OPENED;
				status = STATUS_SUCCESS;
				break;
			case FILE_SUPERSEDE:
			case FILE_CREATE:
			case FILE_OVERWRITE:
			case FILE_OVERWRITE_IF:
				IoStatusBlock->Information = FILE_EXISTS;
				status = STATUS_ACCESS_DENIED;
				break;
			default:
				IoStatusBlock->Information = FILE_EXISTS;
				status = STATUS_NOT_IMPLEMENTED;
				break;
			}
			IoStatusBlock->Status = status;
			return status;
		} catch(...) {
			return STATUS_ACCESS_VIOLATION;
		}
	}

	return TrueNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS FileManager::NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions)
{
	const OBJECT_ENTRY *entry = FindEntry(ObjectAttributes->RootDirectory, ObjectAttributes->ObjectName);
	if (entry) {
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->Add(OBJECT_FILE, const_cast<OBJECT_ENTRY *>(entry), ::CreateEventA(NULL, false, false, NULL), DesiredAccess);
		try {
			*FileHandle = object->handle();
			NTSTATUS status = STATUS_SUCCESS;
			IoStatusBlock->Status = status;
			return status;
		} catch(...) {
			return STATUS_ACCESS_VIOLATION;
		}
	}

	return TrueNtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

NTSTATUS FileManager::NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(FileHandle);
		if (object) {
			OBJECT_ENTRY entry = DecryptEntry(static_cast<const OBJECT_ENTRY*>(object->ref()));
			try {
				NTSTATUS status;
				switch (FileInformationClass) {
				case FileBasicInformation:
					{
						if (Length < sizeof(FILE_BASIC_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_BASIC_INFORMATION *info = reinterpret_cast<FILE_BASIC_INFORMATION *>(FileInformation);
						info->CreationTime.QuadPart = 0;
						info->LastAccessTime.QuadPart = 0;
						info->LastWriteTime.QuadPart = 0;
						info->ChangeTime.QuadPart = 0;
						info->FileAttributes = entry.OffsetToData ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_DIRECTORY;
					}
					status = STATUS_SUCCESS;
					break;
				case FilePositionInformation:
					{
						if (Length < sizeof(FILE_POSITION_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_POSITION_INFORMATION *info = reinterpret_cast<FILE_POSITION_INFORMATION *>(FileInformation);
						info->CurrentByteOffset.QuadPart = object->file_position();
					}
					status = STATUS_SUCCESS;
					break;
				case FileStandardInformation:
					{
						if (Length < sizeof(FILE_STANDARD_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_STANDARD_INFORMATION *info = reinterpret_cast<FILE_STANDARD_INFORMATION *>(FileInformation);
						info->AllocationSize.QuadPart = 0;
						info->EndOfFile.QuadPart = entry.Size;
						info->NumberOfLinks = 0;
						info->DeletePending = FALSE;
						info->Directory = entry.OffsetToData ? FALSE : TRUE;
					}
					status = STATUS_SUCCESS;
					break;
				case FileAllInformation:
					{
						if (Length < sizeof(FILE_ALL_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_ALL_INFORMATION *info = reinterpret_cast<FILE_ALL_INFORMATION *>(FileInformation);
						info->BasicInformation.CreationTime.QuadPart = 0;
						info->BasicInformation.LastAccessTime.QuadPart = 0;
						info->BasicInformation.LastWriteTime.QuadPart = 0;
						info->BasicInformation.ChangeTime.QuadPart = 0;
						info->BasicInformation.FileAttributes = entry.OffsetToData ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_DIRECTORY;
						info->StandardInformation.AllocationSize.QuadPart = 0;
						info->StandardInformation.EndOfFile.QuadPart = entry.Size;
						info->StandardInformation.NumberOfLinks = 0;
						info->StandardInformation.DeletePending = FALSE;
						info->StandardInformation.Directory = entry.OffsetToData ? FALSE : TRUE;
						info->InternalInformation.IndexNumber.QuadPart = 0;
						info->EaInformation.EaSize = 0;
						info->AccessInformation.AccessFlags = object->access();
						info->PositionInformation.CurrentByteOffset.QuadPart = object->file_position();
						info->ModeInformation.Mode = 0;
						info->AlignmentInformation.AlignmentRequirement = 0;
						info->NameInformation.FileNameLength = 0;
						info->NameInformation.FileName[0] = 0;
					}
					status = STATUS_SUCCESS;
					break;
				default:
					status = STATUS_NOT_IMPLEMENTED;
					break;
				}
				IoStatusBlock->Status = status;
				return status;
			} catch(...) {
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return TrueNtQueryInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS FileManager::NtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(FileHandle);
		if (object) {
			try {
				NTSTATUS status = STATUS_SUCCESS;
				switch (FsInformationClass) {
				case FileFsVolumeInformation:
					{
						if (Length < sizeof(FILE_FS_VOLUME_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_FS_VOLUME_INFORMATION *info = reinterpret_cast<FILE_FS_VOLUME_INFORMATION *>(FsInformation);
						info->VolumeCreationTime.QuadPart = 0;
						info->VolumeSerialNumber = 0;
						info->VolumeLabelLength = 0;
						info->SupportsObjects = FALSE;
						info->VolumeLabel[0] = 0;
					}
					break;
				case FileFsDeviceInformation:
					{
						if (Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_FS_DEVICE_INFORMATION *info = reinterpret_cast<FILE_FS_DEVICE_INFORMATION *>(FsInformation);
						info->DeviceType = FILE_DEVICE_DISK;
						info->Characteristics = 0;
					}
					break;
				case FileFsAttributeInformation:
					{
						if (Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_FS_ATTRIBUTE_INFORMATION *info = reinterpret_cast<FILE_FS_ATTRIBUTE_INFORMATION *>(FsInformation);
						info->FileSystemAttributes = FILE_READ_ONLY_VOLUME;
						info->MaximumComponentNameLength = MAX_PATH;
						info->FileSystemNameLength = 0;
						info->FileSystemName[0] = 0;
					}
					break;
				default:
					status = STATUS_NOT_IMPLEMENTED;
					break;
				}
				IoStatusBlock->Status = status;
				return status;
			} catch(...) {
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return TrueNtQueryVolumeInformationFile(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

NTSTATUS FileManager::NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(FileHandle);
		if (object) {
			try {
				NTSTATUS status;
				switch (FileInformationClass) {
				case FilePositionInformation:
					{
						if (Length < sizeof(FILE_POSITION_INFORMATION)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}

						FILE_POSITION_INFORMATION *info = reinterpret_cast<FILE_POSITION_INFORMATION *>(FileInformation);
						if (info->CurrentByteOffset.HighPart)
							info->CurrentByteOffset.LowPart = -1;
						object->set_file_position(info->CurrentByteOffset.LowPart);

						status = STATUS_SUCCESS;
						break;
					}
				default:
					status = STATUS_NOT_IMPLEMENTED;
					break;
				}
				IoStatusBlock->Status = status;
				return status;
			} catch(...) {
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return TrueNtSetInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS FileManager::NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
	ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan)
{
	if (FileName) {
		try {
			bool wildcard_found = false;
			for (size_t i = 0; i < FileName->Length / sizeof(WCHAR); i++) {
				wchar_t c = FileName->Buffer[i];
				if (c == '*' || c == '?') {
					wildcard_found = true;
					break;
				}
			}
			if (!wildcard_found) {
				const OBJECT_ENTRY *enc_entry = FindEntry(FileHandle, FileName);
				if (enc_entry) {
					OBJECT_ENTRY entry = DecryptEntry(enc_entry);
					NTSTATUS status;
					switch (FileInformationClass) {
					case FileBothDirectoryInformation:
						{
							if (Length < sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + FileName->Length) {
								status = STATUS_INFO_LENGTH_MISMATCH;
								break;
							}

							FILE_BOTH_DIR_INFORMATION *info = reinterpret_cast<FILE_BOTH_DIR_INFORMATION *>(FileInformation);
							info->NextEntryOffset = 0;
							info->FileIndex = 0;
							info->CreationTime.QuadPart = 0;
							info->LastAccessTime.QuadPart = 0;
							info->LastWriteTime.QuadPart = 0;
							info->ChangeTime.QuadPart = 0;
							info->EndOfFile.QuadPart = entry.Size;
							info->AllocationSize.QuadPart = entry.Size;
							info->FileAttributes = entry.OffsetToData ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_DIRECTORY;
							info->FileNameLength = FileName->Length;
							info->ShortNameLength = 0;
							info->ShortName[0] = 0;
							memcpy(info->FileName, FileName->Buffer, FileName->Length);

							IoStatusBlock->Information = sizeof(FILE_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + info->FileNameLength;
							IoStatusBlock->Status = STATUS_SUCCESS;
							status = STATUS_SUCCESS;
						}
						break;
					case FileIdBothDirectoryInformation:
						{
							if (Length < sizeof(FILE_ID_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + FileName->Length) {
								status = STATUS_INFO_LENGTH_MISMATCH;
								break;
							}

							FILE_ID_BOTH_DIR_INFORMATION *info = reinterpret_cast<FILE_ID_BOTH_DIR_INFORMATION *>(FileInformation);
							info->NextEntryOffset = 0;
							info->FileIndex = 0;
							info->CreationTime.QuadPart = 0;
							info->LastAccessTime.QuadPart = 0;
							info->LastWriteTime.QuadPart = 0;
							info->ChangeTime.QuadPart = 0;
							info->EndOfFile.QuadPart = entry.Size;
							info->AllocationSize.QuadPart = entry.Size;
							info->FileAttributes = entry.OffsetToData ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_DIRECTORY;
							info->FileNameLength = FileName->Length;
							info->ShortNameLength = 0;
							info->ShortName[0] = 0;
							info->FileId.QuadPart = 0;
							memcpy(info->FileName, FileName->Buffer, FileName->Length);

							IoStatusBlock->Information = sizeof(FILE_ID_BOTH_DIR_INFORMATION) - sizeof(WCHAR) + info->FileNameLength;
							IoStatusBlock->Status = STATUS_SUCCESS;
							status = STATUS_SUCCESS;
						}
						break;
					default:
						status = STATUS_NOT_IMPLEMENTED;
						break;
					}
					return status;
				}
			}
		}
		catch (...) {
			return STATUS_ACCESS_VIOLATION;
		}
	}

	return TrueNtQueryDirectoryFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

NTSTATUS FileManager::NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(FileHandle);
		if (object) {
			OBJECT_ENTRY entry = DecryptEntry(static_cast<const OBJECT_ENTRY *>(object->ref()));
			if (!entry.OffsetToData)
				return STATUS_INVALID_DEVICE_REQUEST;

			try {
				uint64_t file_position;
				if (!ByteOffset || (ByteOffset->HighPart == -1 && ByteOffset->LowPart == FILE_USE_FILE_POINTER_POSITION))
					file_position = object->file_position();
				else
					file_position = ByteOffset->QuadPart;

				if (file_position + Length > entry.Size)
					Length = (file_position < entry.Size) ? entry.Size - static_cast<uint32_t>(file_position) : 0;

				if (ReadFile(&entry, file_position, Buffer, Length))
					object->set_file_position(file_position + Length);
				else
					Length = 0;
				IoStatusBlock->Information = Length;
				IoStatusBlock->Status = STATUS_SUCCESS;
				return STATUS_SUCCESS;
			} catch(...) {
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return TrueNtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS FileManager::NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetFile(FileHandle);
		if (object) {
			OBJECT_ENTRY entry = DecryptEntry(static_cast<const OBJECT_ENTRY *>(object->ref()));
			if (!entry.OffsetToData)
				return STATUS_INVALID_FILE_FOR_SECTION;

			NTSTATUS status;
			LARGE_INTEGER image_size;
			if (AllocationAttributes & SEC_IMAGE) {
				IMAGE_NT_HEADERS nt_header;
				status = ReadImageHeader(&entry, &nt_header);
				if (status != STATUS_SUCCESS)
					return status;
				image_size.QuadPart = nt_header.OptionalHeader.SizeOfImage;
			} else {
				image_size.QuadPart = entry.Size;
			}
			status = TrueNtCreateSection(SectionHandle, DesiredAccess | SECTION_MAP_WRITE, ObjectAttributes, &image_size, (AllocationAttributes & SEC_IMAGE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, SEC_COMMIT, NULL);
			if (NT_SUCCESS(status)) {
				VirtualObject *section = objects_->Add(OBJECT_SECTION, object->ref(), *SectionHandle, DesiredAccess);
				section->set_attributes(AllocationAttributes);
			}
			return status;
		}
	}

	return TrueNtCreateSection(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
}

NTSTATUS FileManager::NtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetSection(SectionHandle);
		if (object && (object->attributes() & SEC_IMAGE)) {
			IMAGE_NT_HEADERS nt_header;
			OBJECT_ENTRY entry = DecryptEntry(static_cast<const OBJECT_ENTRY *>(object->ref()));
			NTSTATUS status = ReadImageHeader(&entry, &nt_header);
			if (NT_SUCCESS(status)) {
				try {
					switch (InformationClass) {
					case SectionBasicInformation:
						{
							if (InformationBufferSize < sizeof(SECTION_BASIC_INFORMATION)) {
								status = STATUS_INFO_LENGTH_MISMATCH;
								break;
							}
							SECTION_BASIC_INFORMATION *info = reinterpret_cast<SECTION_BASIC_INFORMATION *>(InformationBuffer);
							memset(info, 0, sizeof(*info));
							info->Attributes = SEC_IMAGE;
							info->Size.QuadPart = nt_header.OptionalHeader.SizeOfImage;
							if (ResultLength)
								*ResultLength = sizeof(SECTION_BASIC_INFORMATION);
						}
						status = STATUS_SUCCESS;
						break;
					case SectionImageInformation:
						{
							if (InformationBufferSize < sizeof(SECTION_IMAGE_INFORMATION)) {
								status = STATUS_INFO_LENGTH_MISMATCH;
								break;
							}
							SECTION_IMAGE_INFORMATION *info = reinterpret_cast<SECTION_IMAGE_INFORMATION *>(InformationBuffer);
							memset(info, 0, sizeof(*info));
							info->ImageCharacteristics = nt_header.OptionalHeader.DllCharacteristics;
							info->ImageMachineType = nt_header.FileHeader.Machine;
							info->ImageSubsystem = nt_header.OptionalHeader.Subsystem;
							info->StackCommit = static_cast<ULONG>(nt_header.OptionalHeader.SizeOfStackCommit);
							info->StackReserved = static_cast<ULONG>(nt_header.OptionalHeader.SizeOfStackReserve);
							info->StackZeroBits = 0;
							info->SubSystemVersionHigh = nt_header.OptionalHeader.MajorSubsystemVersion;
							info->SubSystemVersionLow = nt_header.OptionalHeader.MinorSubsystemVersion;
							if (ResultLength)
								*ResultLength = sizeof(SECTION_IMAGE_INFORMATION);
						}
						status = STATUS_SUCCESS;
						break;
					case SectionRelocationInformation:
						{
							if (InformationBufferSize < sizeof(ULONG_PTR)) {
								status = STATUS_INFO_LENGTH_MISMATCH;
								break;
							}
							*reinterpret_cast<ULONG_PTR *>(InformationBuffer) = 0;
							if (ResultLength)
								*ResultLength = sizeof(ULONG_PTR);
						}
					default:
						status = STATUS_NOT_IMPLEMENTED;
						break;
					}
					return status;
				} catch(...) {
					return STATUS_ACCESS_VIOLATION;
				}
			}
		}
	}

	return TrueNtQuerySection(SectionHandle, InformationClass, InformationBuffer, InformationBufferSize, ResultLength);
}

NTSTATUS FileManager::NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect)
{
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetSection(SectionHandle);
		if (object) {
			OBJECT_ENTRY entry = DecryptEntry(static_cast<const OBJECT_ENTRY *>(object->ref()));
			NTSTATUS status = TrueNtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, 0, (object->attributes() & SEC_IMAGE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
			if (NT_SUCCESS(status)) {
				try {
					uint64_t offset = (SectionOffset && SectionOffset->QuadPart) ? SectionOffset->QuadPart : 0;
					if (object->attributes() & SEC_IMAGE) {
						if (offset)
							return STATUS_NOT_IMPLEMENTED;
						status = ReadImage(&entry, *BaseAddress);
					} else {
						size_t size = *ViewSize;
						if (offset + size > entry.Size)
							size = (offset < entry.Size) ? entry.Size - static_cast<uint32_t>(offset) : 0; 
						ReadFile(&entry, offset, *BaseAddress, size);
						if (Win32Protect != PAGE_READWRITE) {
							ULONG old_protect;
							if (!VirtualProtect(*BaseAddress, *ViewSize, Win32Protect, &old_protect))
								status = STATUS_INVALID_PAGE_PROTECTION;
						}
					}
					if (NT_SUCCESS(status)) {
						/*VirtualObject *map = */objects_->Add(OBJECT_MAP, *BaseAddress, ProcessHandle, 0);
					} else {
						TrueNtUnmapViewOfSection(ProcessHandle, *BaseAddress);
					}
				} catch(...) {
					return STATUS_ACCESS_VIOLATION;
				}
			}
			return status;
		}
	}

	return TrueNtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

NTSTATUS FileManager::NtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
	{
		CriticalSection	cs(objects_->critical_section());

		objects_->DeleteRef(BaseAddress, ProcessHandle);
	}

	return TrueNtUnmapViewOfSection(ProcessHandle, BaseAddress);
}

NTSTATUS FileManager::NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength)
{
	if (MemoryInformationClass == MemoryBasicInformation) {
		NTSTATUS status = TrueNtQueryVirtualMemory(ProcessHandle, BaseAddress, MemoryInformationClass, Buffer, Length, ResultLength);
		if (NT_SUCCESS(status)) {
			MEMORY_BASIC_INFORMATION *info = reinterpret_cast<MEMORY_BASIC_INFORMATION *>(Buffer);
			CriticalSection	cs(objects_->critical_section());

			VirtualObject *object = objects_->GetMap(ProcessHandle, info->AllocationBase);
			if (object && (object->attributes() & SEC_IMAGE))
				info->Type = MEM_IMAGE;
		}
		return status;
	}

	return TrueNtQueryVirtualMemory(ProcessHandle, BaseAddress, MemoryInformationClass, Buffer, Length, ResultLength);
}

#endif // WIN_DRIVER