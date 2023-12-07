#include "common.h"
#include "utils.h"
#include "objects.h"

#include "crypto.h"
#include "core.h"
#include "string_manager.h"
#include "licensing_manager.h"
#include "hwid.h"

#ifdef VMP_GNU
#include "loader.h"
#elif defined(WIN_DRIVER)
#include "loader.h"
#else
#include "resource_manager.h"
#include "file_manager.h"
#include "registry_manager.h"
#include "hook_manager.h"
#endif

GlobalData *loader_data = NULL;
#ifdef WIN_DRIVER
__declspec(noinline) void * ExAllocateNonPagedPoolNx(size_t size)
{
	return ExAllocatePool((POOL_TYPE)FACE_NON_PAGED_POOL_NX, size);
}

void * __cdecl operator new(size_t size)
{
	if (size)
		return ExAllocateNonPagedPoolNx(size);

	return NULL;
}

void __cdecl operator delete(void* p)
{
	if (p)
		ExFreePool(p);
}

void __cdecl operator delete(void* p, size_t)
{
	if (p)
		ExFreePool(p);
}

void * __cdecl operator new[](size_t size)
{
	if (size)
		return ExAllocateNonPagedPoolNx(size);

	return NULL;
}

void __cdecl operator delete[](void *p)
{
	if (p)
		ExFreePool(p);
}
#endif

/**
 * initialization functions
 */

#ifdef VMP_GNU

EXPORT_API bool WINAPI DllMain(HMODULE hModule, bool is_init) __asm__ ("DllMain");
bool WINAPI DllMain(HMODULE hModule, bool is_init)
{
	if (is_init) {
		if (!Core::Instance()->Init(hModule)) {
			Core::Free();
			return false;
		}
	} else {
		Core::Free();
	}
	return true;
}

#elif defined(WIN_DRIVER)

NTSTATUS DllMain(HMODULE hModule, bool is_init)
{
	if (is_init) {
		if (!Core::Instance()->Init(hModule)) {
			Core::Free();
			return STATUS_ACCESS_DENIED;
		}
	} else {
		Core::Free();
	}
	return STATUS_SUCCESS;
}

#else

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		if (!Core::Instance()->Init(hModule)) {
			Core::Free();
			return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH:
		Core::Free();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
#endif

/**
 * exported functions
 */

NOINLINE bool InternalFindFirmwareVendor(const uint8_t *data, size_t data_size)
{
	for (size_t i = 0; i < data_size; i++) {
#ifdef __unix__
		if (i + 3 < data_size && data[i + 0] == 'Q' && data[i + 1] == 'E' && data[i + 2] == 'M' && data[i + 3] == 'U')
			return true;
		if (i + 8 < data_size && data[i + 0] == 'M' && data[i + 1] == 'i' && data[i + 2] == 'c' && data[i + 3] == 'r' && data[i + 4] == 'o' && data[i + 5] == 's' && data[i + 6] == 'o' && data[i + 7] == 'f' && data[i + 8] == 't')
			return true;
		if (i + 6 < data_size && data[i + 0] == 'i' && data[i + 1] == 'n' && data[i + 2] == 'n' && data[i + 3] == 'o' && data[i + 4] == 't' && data[i + 5] == 'e' && data[i + 6] == 'k')
			return true;
#else
		if (i + 9 < data_size && data[i + 0] == 'V' && data[i + 1] == 'i' && data[i + 2] == 'r' && data[i + 3] == 't' && data[i + 4] == 'u' && data[i + 5] == 'a' && data[i + 6] == 'l' && data[i + 7] == 'B' && data[i + 8] == 'o' && data[i + 9] == 'x')
			return true;
#endif
		if (i + 5 < data_size && data[i + 0] == 'V' && data[i + 1] == 'M' && data[i + 2] == 'w' && data[i + 3] == 'a' && data[i + 4] == 'r' && data[i + 5] == 'e')
			return true;
		if (i + 8 < data_size && data[i + 0] == 'P' && data[i + 1] == 'a' && data[i + 2] == 'r' && data[i + 3] == 'a' && data[i + 4] == 'l' && data[i + 5] == 'l' && data[i + 6] == 'e' && data[i + 7] == 'l' && data[i + 8] == 's')
			return true;
	}
	return false;
}

#ifdef VMP_GNU
EXPORT_API bool WINAPI ExportedIsValidImageCRC() __asm__ ("ExportedIsValidImageCRC");
EXPORT_API bool WINAPI ExportedIsDebuggerPresent(bool check_kernel_mode) __asm__ ("ExportedIsDebuggerPresent");
EXPORT_API bool WINAPI ExportedIsVirtualMachinePresent() __asm__ ("ExportedIsVirtualMachinePresent");
EXPORT_API bool WINAPI ExportedIsProtected() __asm__ ("ExportedIsProtected");
#endif

struct CRCData {
	uint8_t *ImageBase;
	uint32_t Table;
	uint32_t Size;
	uint32_t Hash;
	NOINLINE CRCData()
	{
		ImageBase = reinterpret_cast<uint8_t *>(FACE_IMAGE_BASE);
		Table = FACE_CRC_TABLE_ENTRY;
		Size = FACE_CRC_TABLE_SIZE;
		Hash = FACE_CRC_TABLE_HASH;
	}
};

bool WINAPI ExportedIsValidImageCRC()
{
	if (loader_data->is_patch_detected())
		return false;

	const CRCData crc_data;

	bool res = true;
	uint8_t *image_base = crc_data.ImageBase;
	uint8_t *crc_table = image_base + crc_data.Table;
	uint32_t crc_table_size = *reinterpret_cast<uint32_t *>(image_base + crc_data.Size);
	uint32_t crc_table_hash = *reinterpret_cast<uint32_t *>(image_base + crc_data.Hash);

#ifdef WIN_DRIVER
	uint32_t image_size = 0;
	if (loader_data->loader_status() == STATUS_SUCCESS) {
		IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image_base);
		if (dos_header->e_magic == IMAGE_DOS_SIGNATURE) {
			IMAGE_NT_HEADERS *pe_header = reinterpret_cast<IMAGE_NT_HEADERS *>(image_base + dos_header->e_lfanew);
			if (pe_header->Signature == IMAGE_NT_SIGNATURE) {
				IMAGE_SECTION_HEADER *sections = reinterpret_cast<IMAGE_SECTION_HEADER *>(reinterpret_cast<uint8_t *>(pe_header) + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + pe_header->FileHeader.SizeOfOptionalHeader);
				for (size_t i = 0; i < pe_header->FileHeader.NumberOfSections; i++) {
					IMAGE_SECTION_HEADER *section = sections + i;
					if (section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
						image_size = section->VirtualAddress;
						break;
					}
				}
			}
		}
	}
#endif

	// check memory CRC
	{
		if (crc_table_hash != CalcCRC(crc_table, crc_table_size))
			res = false;
		CRCValueCryptor crc_cryptor;
		for (size_t i = 0; i < crc_table_size; i += sizeof(CRC_INFO)) {
			CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(crc_table + i);
			crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
			crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
			crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);
#ifdef WIN_DRIVER
			if (image_size && image_size < crc_info.Address + crc_info.Size)
				continue;
#endif
		
			if (crc_info.Hash != CalcCRC(image_base + crc_info.Address, crc_info.Size))
				res = false;
		}
	}

	// check header and loader CRC
	crc_table = image_base + loader_data->loader_crc_info();
	crc_table_size = static_cast<uint32_t>(loader_data->loader_crc_size());
	crc_table_hash = static_cast<uint32_t>(loader_data->loader_crc_hash());
	{
		if (crc_table_hash != CalcCRC(crc_table, crc_table_size))
			res = false;
		CRCValueCryptor crc_cryptor;
		for (size_t i = 0; i < crc_table_size; i += sizeof(CRC_INFO)) {
			CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(crc_table + i);
			crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
			crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
			crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);
#ifdef WIN_DRIVER
			if (image_size && image_size < crc_info.Address + crc_info.Size)
				continue;
#endif
		
			if (crc_info.Hash != CalcCRC(image_base + crc_info.Address, crc_info.Size))
				res = false;
		}
	}

#ifndef DEMO
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	// check memory type of loader_data
	HMODULE ntdll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	typedef NTSTATUS(NTAPI tNtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);
	tNtQueryVirtualMemory *query_virtual_memory = reinterpret_cast<tNtQueryVirtualMemory *>(InternalGetProcAddress(ntdll, VMProtectDecryptStringA("NtQueryVirtualMemory")));
	if (query_virtual_memory) {
		MEMORY_BASIC_INFORMATION memory_info;
		NTSTATUS status = query_virtual_memory(NtCurrentProcess(), loader_data, MemoryBasicInformation, &memory_info, sizeof(memory_info), NULL);
		if (NT_SUCCESS(status) && memory_info.AllocationBase == image_base)
			res = false;
	}
#endif
#endif

	return res;
}

bool WINAPI ExportedIsVirtualMachinePresent()
{
	// hardware detection
	int cpu_info[4];
	__cpuid(cpu_info, 1);
	if ((cpu_info[2] >> 31) & 1) {
#ifndef VMP_GNU
		// check Hyper-V root partition
		cpu_info[1] = 0;
		cpu_info[2] = 0;
		cpu_info[3] = 0;
		__cpuid(cpu_info, 0x40000000);
		if (cpu_info[1] == 0x7263694d && cpu_info[2] == 0x666f736f && cpu_info[3] == 0x76482074) { // "Microsoft Hv"
			cpu_info[1] = 0;
			__cpuid(cpu_info, 0x40000003);
			if (cpu_info[1] & 1)
				return false;
		}
#endif
		return true;
	}

#ifndef VMP_GNU
	uint64_t val;
	uint8_t mem_val;
	__try {
		// set T flag
		__writeeflags(__readeflags() | 0x100);
		 val = __rdtsc();
		 __nop();
		 loader_data->set_is_debugger_detected(true);
	} __except(mem_val = *static_cast<uint8_t *>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
		if (mem_val != 0x90)
			return true;
	}

	__try {
		// set T flag
		__writeeflags(__readeflags() | 0x100);
		__cpuid(cpu_info, 1);
		__nop();
		loader_data->set_is_debugger_detected(true);
	} __except(mem_val = *static_cast<uint8_t *>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
		if (mem_val != 0x90)
			return true;
	}
#endif

	// software detection
#ifdef __APPLE__
	// FIXME
#elif defined(__unix__)
	FILE *fsys_vendor = fopen(VMProtectDecryptStringA("/sys/devices/virtual/dmi/id/sys_vendor"), "r");
	if (fsys_vendor) {
		char sys_vendor[256] = {0};
		fgets(sys_vendor, sizeof(sys_vendor), fsys_vendor);
		fclose(fsys_vendor);
		if (InternalFindFirmwareVendor(reinterpret_cast<uint8_t *>(sys_vendor), sizeof(sys_vendor)))
			return true;
	}
#elif defined(WIN_DRIVER)
	// FIXME
#else
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("kernel32.dll"));
	bool is_found = false;
	typedef UINT (WINAPI tEnumSystemFirmwareTables)(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize);
	typedef UINT (WINAPI tGetSystemFirmwareTable)(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize);
	tEnumSystemFirmwareTables *enum_system_firmware_tables = reinterpret_cast<tEnumSystemFirmwareTables *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("EnumSystemFirmwareTables")));
	tGetSystemFirmwareTable *get_system_firmware_table = reinterpret_cast<tGetSystemFirmwareTable *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("GetSystemFirmwareTable")));

	if (enum_system_firmware_tables && get_system_firmware_table) {
		UINT tables_size = enum_system_firmware_tables('FIRM', NULL, 0);
		if (tables_size) {
			DWORD *tables = new DWORD[tables_size / sizeof(DWORD)];
			enum_system_firmware_tables('FIRM', tables, tables_size);
			for (size_t i = 0; i < tables_size / sizeof(DWORD); i++) {
				UINT data_size = get_system_firmware_table('FIRM', tables[i], NULL, 0);
				if (data_size) {
					uint8_t *data = new uint8_t[data_size];
					get_system_firmware_table('FIRM', tables[i], data, data_size);
					if (InternalFindFirmwareVendor(data, data_size))
						is_found = true;
					delete [] data;
				}
			}
			delete [] tables;
		}
	} else {
		dll = LoadLibraryA(VMProtectDecryptStringA("ntdll.dll"));
		typedef NTSTATUS (WINAPI tNtOpenSection)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
		typedef NTSTATUS (WINAPI tNtMapViewOfSection)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
		typedef NTSTATUS (WINAPI tNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);
		typedef NTSTATUS (WINAPI tNtClose)(HANDLE Handle);

		tNtOpenSection *open_section = reinterpret_cast<tNtOpenSection *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("NtOpenSection")));
		tNtMapViewOfSection *map_view_of_section = reinterpret_cast<tNtMapViewOfSection *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("NtMapViewOfSection")));
		tNtUnmapViewOfSection *unmap_view_of_section = reinterpret_cast<tNtUnmapViewOfSection *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("NtUnmapViewOfSection")));
		tNtClose *close = reinterpret_cast<tNtClose *>(InternalGetProcAddress(dll, VMProtectDecryptStringA("NtClose")));

		if (open_section && map_view_of_section && unmap_view_of_section && close) {
			HANDLE process = NtCurrentProcess();
			HANDLE physical_memory = NULL;
			UNICODE_STRING str;
			OBJECT_ATTRIBUTES attrs;

			wchar_t buf[] = {'\\','d','e','v','i','c','e','\\','p','h','y','s','i','c','a','l','m','e','m','o','r','y',0};
			str.Buffer = buf;
			str.Length = sizeof(buf) - sizeof(wchar_t);
			str.MaximumLength = sizeof(buf);

			InitializeObjectAttributes(&attrs, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
			NTSTATUS status = open_section(&physical_memory, SECTION_MAP_READ, &attrs);
			if (NT_SUCCESS(status)) {
				void *data = NULL;
				SIZE_T data_size = 0x10000;
				LARGE_INTEGER offset;
				offset.QuadPart = 0xc0000;

				status = map_view_of_section(physical_memory, process, &data, NULL, data_size, &offset, &data_size, ViewShare, 0, PAGE_READONLY);
				if (NT_SUCCESS(status)) {
					if (InternalFindFirmwareVendor(static_cast<uint8_t *>(data), data_size))
						is_found = true;
					unmap_view_of_section(process, data);
				}
				close(physical_memory);
			}
		}
	}
	if (is_found)
		return true;

	if (GetModuleHandleA(VMProtectDecryptStringA("sbiedll.dll")))
		return true;
#endif

	return false;
}

bool WINAPI ExportedIsDebuggerPresent(bool check_kernel_mode)
{
	if (loader_data->is_debugger_detected())
		return true;

#if defined(__unix__)
	FILE *file = fopen(VMProtectDecryptStringA("/proc/self/status"), "r");
	if (file) {
		char data[100];
		int tracer_pid = 0;
		while (fgets(data, sizeof(data), file)) {
			if (data[0] == 'T' && data[1] == 'r' && data[2] == 'a' && data[3] == 'c' && data[4] == 'e' && data[5] == 'r' && data[6] == 'P' && data[7] == 'i' && data[8] == 'd' && data[9] == ':') {
				char *tracer_ptr = data + 10;
				// skip spaces
				while (char c = *tracer_ptr) {
					if (c == ' ' || c == '\t') {
						tracer_ptr++;
						continue;
					}
					else {
						break;
					}
				}
				// atoi
				while (char c = *tracer_ptr++) {
					if (c >= '0' && c <= '9') {
						tracer_pid *= 10;
						tracer_pid += c - '0';
					}
					else {
						if (c != '\n' && c != '\r')
							tracer_pid = 0;
						break;
					}
				}
				break;
			}
		}
		fclose(file);

		if (tracer_pid && tracer_pid != 1)
			return true;
	}
#elif defined(__APPLE__)
	(void)check_kernel_mode;

    int junk;
    int mib[4];
    kinfo_proc info;
    size_t size;

    // Initialize the flags so that, if sysctl fails for some bizarre 
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

    // We're being debugged if the P_TRACED flag is set.
	if ((info.kp_proc.p_flag & P_TRACED) != 0)
		return true;
#else
#ifdef WIN_DRIVER
#else
	HMODULE kernel32 = GetModuleHandleA(VMProtectDecryptStringA("kernel32.dll"));
	HMODULE ntdll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	HANDLE process = NtCurrentProcess();
	size_t syscall = FACE_SYSCALL;
	uint32_t sc_query_information_process = 0;

	if (ntdll) {
#ifndef DEMO
		if (InternalGetProcAddress(ntdll, VMProtectDecryptStringA("wine_get_version")) == NULL) {
#ifndef _WIN64
			BOOL is_wow64 = FALSE;
			typedef BOOL(WINAPI tIsWow64Process)(HANDLE Process, PBOOL Wow64Process);
			tIsWow64Process *is_wow64_process = reinterpret_cast<tIsWow64Process *>(InternalGetProcAddress(kernel32, VMProtectDecryptStringA("IsWow64Process")));
			if (is_wow64_process)
				is_wow64_process(process, &is_wow64);
#endif

			uint32_t os_build_number = loader_data->os_build_number();

			if (os_build_number == WINDOWS_XP) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x009a;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_2003) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00a1;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_VISTA) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00e4;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_VISTA_SP1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00e4;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}

			}
			else if (os_build_number == WINDOWS_VISTA_SP2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00e4;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_7) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00ea;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_7_SP1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00ea;
				}
				else
#endif
				{
					sc_query_information_process = 0x0016;
				}
			}
			else if (os_build_number == WINDOWS_8) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b0;
				}
				else
#endif
				{
					sc_query_information_process = 0x0017;
				}
			}
			else if (os_build_number == WINDOWS_8_1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b3;
				}
				else
#endif
				{
					sc_query_information_process = 0x0018;
				}
			}
			else if (os_build_number == WINDOWS_10_TH1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b5;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_TH2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b5;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_RS1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b7;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_RS2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b8;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_RS3) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_RS4) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_RS5) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_19H1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_19H2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_20H1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_20H2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_21H1) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_21H2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
			else if (os_build_number == WINDOWS_10_22H2) {
#ifndef _WIN64
				if (!is_wow64) {
					sc_query_information_process = 0x00b9;
				}
				else
#endif
				{
					sc_query_information_process = 0x0019;
				}
			}
#ifndef _WIN64
			if (is_wow64 && sc_query_information_process) {
				sc_query_information_process |= WOW64_FLAG | (0x03 << 24);
			}
#endif
		}
#endif
	}

#ifdef _WIN64
	PEB64 *peb = reinterpret_cast<PEB64 *>(__readgsqword(0x60));
#else
	PEB32 *peb = reinterpret_cast<PEB32 *>(__readfsdword(0x30));
#endif
	if (peb->BeingDebugged)
		return true;

	{
		size_t drx;
		uint64_t val;
		CONTEXT *ctx;
		__try {
			__writeeflags(__readeflags() | 0x100);
			val = __rdtsc();
			__nop();
			return true;
		}
		__except (ctx = (GetExceptionInformation())->ContextRecord,
			drx = (ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS) ? ctx->Dr0 | ctx->Dr1 | ctx->Dr2 | ctx->Dr3 : 0,
			EXCEPTION_EXECUTE_HANDLER) {
			if (drx)
				return true;
		}
	}

	typedef NTSTATUS(NTAPI tNtQueryInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
	if (sc_query_information_process) {
		HANDLE debug_object;
		if (NT_SUCCESS(reinterpret_cast<tNtQueryInformationProcess *>(syscall | sc_query_information_process)(process, ProcessDebugPort, &debug_object, sizeof(debug_object), NULL)) && debug_object != 0)
			return true;
		debug_object = 0;
		if (NT_SUCCESS(reinterpret_cast<tNtQueryInformationProcess *>(syscall | sc_query_information_process)(process, ProcessDebugObjectHandle, &debug_object, sizeof(debug_object), reinterpret_cast<PULONG>(&debug_object)))
			|| debug_object == 0)
			return true;
	}
	else if (tNtQueryInformationProcess *query_information_process = reinterpret_cast<tNtQueryInformationProcess *>(InternalGetProcAddress(ntdll, VMProtectDecryptStringA("NtQueryInformationProcess")))) {
		HANDLE debug_object;
		if (NT_SUCCESS(query_information_process(process, ProcessDebugPort, &debug_object, sizeof(debug_object), NULL)) && debug_object != 0)
			return true;
		if (NT_SUCCESS(query_information_process(process, ProcessDebugObjectHandle, &debug_object, sizeof(debug_object), NULL)))
			return true;
	}

#endif
#ifdef WIN_DRIVER
	if (true) {
#else
	if (check_kernel_mode) {
#endif
		bool is_found = false;
		typedef NTSTATUS (NTAPI tNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
#ifdef WIN_DRIVER
		tNtQuerySystemInformation *nt_query_system_information = &NtQuerySystemInformation;
#else
		tNtQuerySystemInformation *nt_query_system_information = reinterpret_cast<tNtQuerySystemInformation *>(InternalGetProcAddress(ntdll, VMProtectDecryptStringA("NtQuerySystemInformation")));
		if (nt_query_system_information) {
#endif
			SYSTEM_KERNEL_DEBUGGER_INFORMATION info;
			NTSTATUS status = nt_query_system_information(SystemKernelDebuggerInformation, &info, sizeof(info), NULL);
			if (NT_SUCCESS(status) && info.DebuggerEnabled && !info.DebuggerNotPresent)
				return true;

			SYSTEM_MODULE_INFORMATION *buffer = NULL;
			ULONG buffer_size = 0;
			/*status = */nt_query_system_information(SystemModuleInformation, &buffer, 0, &buffer_size);
			if (buffer_size) {
				buffer = reinterpret_cast<SYSTEM_MODULE_INFORMATION *>(new uint8_t[buffer_size * 2]);
				status = nt_query_system_information(SystemModuleInformation, buffer, buffer_size * 2, NULL);
				if (NT_SUCCESS(status)) {
					for (size_t i = 0; i < buffer->Count && !is_found; i++) {
						SYSTEM_MODULE_ENTRY *module_entry = &buffer->Module[i];
						char module_name[11];
						for (size_t j = 0; j < 5; j++) {
							switch (j) {
							case 0:
								module_name[0] = 's';
								module_name[1] = 'i';
								module_name[2] = 'c';
								module_name[3] = 'e';
								module_name[4] = '.';
								module_name[5] = 's';
								module_name[6] = 'y';
								module_name[7] = 's';
								module_name[8] = 0;
								break;
							case 1:
								module_name[0] = 's';
								module_name[1] = 'i';
								module_name[2] = 'w';
								module_name[3] = 'v';
								module_name[4] = 'i';
								module_name[5] = 'd';
								module_name[6] = '.';
								module_name[7] = 's';
								module_name[8] = 'y';
								module_name[9] = 's';
								module_name[10] = 0;
								break;
							case 2:
								module_name[0] = 'n';
								module_name[1] = 't';
								module_name[2] = 'i';
								module_name[3] = 'c';
								module_name[4] = 'e';
								module_name[5] = '.';
								module_name[6] = 's';
								module_name[7] = 'y';
								module_name[8] = 's';
								module_name[9] = 0;
								break;
							case 3:
								module_name[0] = 'i';
								module_name[1] = 'c';
								module_name[2] = 'e';
								module_name[3] = 'e';
								module_name[4] = 'x';
								module_name[5] = 't';
								module_name[6] = '.';
								module_name[7] = 's';
								module_name[8] = 'y';
								module_name[9] = 's';
								module_name[10] = 0;
								break;
							case 4:
								module_name[0] = 's';
								module_name[1] = 'y';
								module_name[2] = 's';
								module_name[3] = 'e';
								module_name[4] = 'r';
								module_name[5] = '.';
								module_name[6] = 's';
								module_name[7] = 'y';
								module_name[8] = 's';
								module_name[9] = 0;
								break;
							}
							if (_stricmp(module_entry->Name + module_entry->PathLength, module_name) == 0) {
								is_found = true;
								break;
							}
						}
					}
				}
				delete [] buffer;
			}
#ifndef WIN_DRIVER
		}
#endif
		if (is_found)
			return true;
	}
#endif
	return false;
}

bool WINAPI ExportedIsProtected()
{
	return true;
}

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else

/**
 * VirtualObject
 */

VirtualObject::VirtualObject(VirtualObjectType type, void *ref, HANDLE handle, uint32_t access)
	: ref_(ref), handle_(handle), type_(type), file_position_(0), attributes_(0), access_(access)
{
	if(access & MAXIMUM_ALLOWED)
	{
		access_ |= KEY_ALL_ACCESS;
	}
}

VirtualObject::~VirtualObject()
{

}

/**
 * VirtualObjectList
 */

VirtualObjectList::VirtualObjectList()
{
	CriticalSection::Init(critical_section_);
}

VirtualObjectList::~VirtualObjectList()
{
	for (size_t i = 0; i < size(); i++) {
		VirtualObject *object = v_[i];
		delete object;
	}
	v_.clear();

	CriticalSection::Free(critical_section_);
}

VirtualObject *VirtualObjectList::Add(VirtualObjectType type, void *ref, HANDLE handle, uint32_t access)
{
	VirtualObject *object = new VirtualObject(type, ref, handle, access);
	v_.push_back(object);
	return object;
}

void VirtualObjectList::Delete(size_t index)
{
	VirtualObject *object = v_[index];
	v_.erase(index);
	delete object;
}

void VirtualObjectList::DeleteObject(HANDLE handle)
{
	handle = EXHANDLE(handle);
	for (size_t i = size(); i > 0; i--) {
		size_t index = i - 1;
		VirtualObject *object = v_[index];
		if (object->handle() == handle)
			Delete(index);
	}
}

void VirtualObjectList::DeleteRef(void *ref, HANDLE handle)
{
	handle = EXHANDLE(handle);
	for (size_t i = size(); i > 0; i--) {
		size_t index = i - 1;
		VirtualObject *object = v_[index];
		if (object->ref() == ref && (!handle || object->handle() == handle))
			Delete(index);
	}
}

VirtualObject *VirtualObjectList::GetObject(HANDLE handle) const
{
	handle = EXHANDLE(handle);
	for (size_t i = 0; i < size(); i++) {
		VirtualObject *object = v_[i];
		if (object->handle() == handle)
			return object;
	}
	return NULL;
}

VirtualObject *VirtualObjectList::GetFile(HANDLE handle) const
{
	VirtualObject *object = GetObject(handle);
	return (object && object->type() == OBJECT_FILE) ? object : NULL;
}

VirtualObject *VirtualObjectList::GetSection(HANDLE handle) const
{
	VirtualObject *object = GetObject(handle);
	return (object && object->type() == OBJECT_SECTION) ? object : NULL;
}

VirtualObject *VirtualObjectList::GetMap(HANDLE process, void *map) const
{
	for (size_t i = 0; i < size(); i++) {
		VirtualObject *object = v_[i];
		if (object->type() == OBJECT_MAP && object->handle() == process && object->ref() == map)
			return object;
	}
	return NULL;
}

VirtualObject *VirtualObjectList::GetKey(HANDLE handle) const
{
	VirtualObject *object = GetObject(handle);
	return (object && object->type() == OBJECT_KEY) ? object : NULL;
}

uint32_t VirtualObjectList::GetHandleCount(HANDLE handle) const
{
	uint32_t res = 0;
	for (size_t i = 0; i < size(); i++) {
		VirtualObject *object = v_[i];
		if (object->handle() == handle)
			res++;
	}
	return res;
}

uint32_t VirtualObjectList::GetPointerCount(const void *ref) const
{
	uint32_t res = 0;
	for (size_t i = 0; i < size(); i++) {
		VirtualObject *object = v_[i];
		if (object->ref() == ref)
			res++;
	}
	return res;
}
#endif

/**
 * Core
 */

Core *Core::self_ = NULL;

Core::Core()
	: string_manager_(NULL), licensing_manager_(NULL), hardware_id_(NULL)
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	, resource_manager_(NULL), file_manager_(NULL), registry_manager_(NULL)
	, hook_manager_(NULL), nt_protect_virtual_memory_(NULL), nt_close_(NULL)
	, nt_query_object_(NULL), dbg_ui_remote_breakin_(NULL)
#endif
{

}

Core::~Core()
{
	delete string_manager_;
	delete licensing_manager_;
	delete hardware_id_;

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	if (resource_manager_) {
		resource_manager_->UnhookAPIs(*hook_manager_);
		delete resource_manager_;
	}
	if (file_manager_) {
		file_manager_->UnhookAPIs(*hook_manager_);
		delete file_manager_;
	}
	if (registry_manager_) {
		registry_manager_->UnhookAPIs(*hook_manager_);
		delete registry_manager_;
	}

	if (nt_protect_virtual_memory_ || nt_close_ || dbg_ui_remote_breakin_)
		UnhookAPIs(*hook_manager_);

	delete hook_manager_;
#endif
}

Core *Core::Instance()
{
	if (!self_)
		self_ = new Core();
	return self_;
}

void Core::Free()
{
	if (self_) {
		delete self_;
		self_ = NULL;
	}
}

struct CoreData {
	uint32_t Strings;
	uint32_t Resources;
	uint32_t Storage;
	uint32_t Registry;
	uint32_t LicenseData;
	uint32_t LicenseDataSize;
	uint32_t TrialHWID;
	uint32_t TrialHWIDSize;
	uint32_t Key;
	uint32_t Options;

	NOINLINE CoreData()
	{
		Strings = FACE_STRING_INFO;
		Resources = FACE_RESOURCE_INFO;
		Storage = FACE_STORAGE_INFO;
		Registry = FACE_REGISTRY_INFO;
		Key = FACE_KEY_INFO;
		LicenseData = FACE_LICENSE_INFO;
		LicenseDataSize = FACE_LICENSE_INFO_SIZE;
		TrialHWID = FACE_TRIAL_HWID;
		TrialHWIDSize = FACE_TRIAL_HWID_SIZE;
		Options = FACE_CORE_OPTIONS;
	}
};

bool Core::Init(HMODULE instance)
{
	const CoreData data;

	uint8_t *key = reinterpret_cast<uint8_t *>(instance) + data.Key;
	if (data.Strings)
		string_manager_ = new StringManager(reinterpret_cast<uint8_t *>(instance) + data.Strings, instance, key);

	if (data.LicenseData)
		licensing_manager_ = new LicensingManager(reinterpret_cast<uint8_t *>(instance) + data.LicenseData, data.LicenseDataSize, key);

	if (data.TrialHWID) {
		uint8_t hwid_data[64];
		{
			CipherRC5 cipher(key);
			cipher.Decrypt(reinterpret_cast<uint8_t *>(instance) + data.TrialHWID, reinterpret_cast<uint8_t *>(&hwid_data), sizeof(hwid_data));
		}
		if (!hardware_id()->IsCorrect(hwid_data, data.TrialHWIDSize)) {
			const VMP_CHAR *message;
#ifdef VMP_GNU
			message = VMProtectDecryptStringA(MESSAGE_HWID_MISMATCHED_STR);
#else
			message = VMProtectDecryptStringW(MESSAGE_HWID_MISMATCHED_STR);
#endif
			ShowMessage(message);
			return false;
		}
	}

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	if (data.Resources || data.Storage || data.Registry || (data.Options & (CORE_OPTION_MEMORY_PROTECTION | CORE_OPTION_CHECK_DEBUGGER)))
		hook_manager_ = new HookManager();

	if (data.Resources) {
		resource_manager_ = new ResourceManager(reinterpret_cast<uint8_t *>(instance) + data.Resources, instance, key);
		resource_manager_->HookAPIs(*hook_manager_); //-V595
	}
	if (data.Storage) {
		file_manager_ = new FileManager(reinterpret_cast<uint8_t *>(instance) + data.Storage, instance, key, &objects_);
		file_manager_->HookAPIs(*hook_manager_);
	}
	if (data.Registry) {
		registry_manager_ = new RegistryManager(reinterpret_cast<uint8_t *>(instance) + data.Registry, instance, key, &objects_);
		registry_manager_->HookAPIs(*hook_manager_);
	}
	if (hook_manager_)
		HookAPIs(*hook_manager_, data.Options);
	if (file_manager_) {
		if (!file_manager_->OpenFiles(*registry_manager_))
			return false;
	}
#endif

	return true;
}

HardwareID *Core::hardware_id()
{
	if (!hardware_id_)
		hardware_id_ = new HardwareID;
	return hardware_id_;
}

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
NTSTATUS WINAPI HookedNtProtectVirtualMemory(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect)
{
	Core *core = Core::Instance();
	return core->NtProtectVirtualMemory(ProcesssHandle, BaseAddress, Size, NewProtect, OldProtect);
}

void WINAPI HookedDbgUiRemoteBreakin()
{
	::TerminateProcess(::GetCurrentProcess(), 0xDEADC0DE);
}

NTSTATUS WINAPI HookedNtClose(HANDLE Handle)
{
	Core *core = Core::Instance();
	return core->NtClose(Handle);
}

NTSTATUS WINAPI HookedNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength)
{
	Core *core = Core::Instance();
	return core->NtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

void Core::HookAPIs(HookManager &hook_manager, uint32_t options)
{
	hook_manager.Begin();
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	if (options & CORE_OPTION_MEMORY_PROTECTION)
		hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtProtectVirtualMemory"), &HookedNtProtectVirtualMemory, true, &nt_protect_virtual_memory_);
	if (options & CORE_OPTION_CHECK_DEBUGGER)
		dbg_ui_remote_breakin_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("DbgUiRemoteBreakin"), &HookedDbgUiRemoteBreakin, false);
	if (file_manager_ || registry_manager_) {
		nt_close_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtClose"), &HookedNtClose);
		nt_query_object_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryObject"), &HookedNtQueryObject);
	}
	hook_manager.End();
}

void Core::UnhookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	hook_manager.UnhookAPI(nt_protect_virtual_memory_);
	hook_manager.UnhookAPI(nt_close_);
	hook_manager.UnhookAPI(nt_query_object_);
	hook_manager.UnhookAPI(dbg_ui_remote_breakin_);
	hook_manager.End();
}

NTSTATUS Core::NtProtectVirtualMemory(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect)
{
	if (ProcesssHandle == GetCurrentProcess()) {
		const CRCData crc_data;

		uint8_t *image_base = crc_data.ImageBase;
		size_t crc_image_size = loader_data->crc_image_size();
		try {
			uint8_t *user_address = static_cast<uint8_t *>(*BaseAddress);
			size_t user_size = *Size;
			if (user_address + user_size > image_base && user_address < image_base + crc_image_size) {
				uint8_t *crc_table = image_base + crc_data.Table;
				uint32_t crc_table_size = *reinterpret_cast<uint32_t *>(image_base + crc_data.Size);
				CRCValueCryptor crc_cryptor;

				// check regions
				for (size_t i = 0; i < crc_table_size; i += sizeof(CRC_INFO)) {
					CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(crc_table + i);
					crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
					crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
					crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);

					uint8_t *crc_address = image_base + crc_info.Address;
					if (user_address + user_size > crc_address && user_address < crc_address + crc_info.Size)
						return STATUS_ACCESS_DENIED;
				}
			}
		} catch(...) {
			return STATUS_ACCESS_VIOLATION;
		}
	}

	return TrueNtProtectVirtualMemory(ProcesssHandle, BaseAddress, Size, NewProtect, OldProtect);
}

NTSTATUS Core::NtClose(HANDLE Handle)
{
	{
		CriticalSection	cs(objects_.critical_section());

		objects_.DeleteObject(Handle);
	}

	return TrueNtClose(Handle);
}

NTSTATUS Core::NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength)
{
	{
		CriticalSection	cs(objects_.critical_section());

		VirtualObject *object = objects_.GetObject(Handle);
		if (object) {
			try {
				switch (ObjectInformationClass) {
				case ObjectBasicInformation:
					{
						if (ObjectInformationLength != sizeof(PUBLIC_OBJECT_BASIC_INFORMATION))
							return STATUS_INFO_LENGTH_MISMATCH;

						PUBLIC_OBJECT_BASIC_INFORMATION info = {};
						info.GrantedAccess = object->access();
						info.HandleCount = objects_.GetHandleCount(Handle);
						info.PointerCount = objects_.GetPointerCount(object->ref());

						if (ReturnLength)
							*ReturnLength = sizeof(info);
					}
					return STATUS_SUCCESS;
				default:
					return STATUS_INVALID_PARAMETER;
				}
			} catch (...) {
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return TrueNtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

NTSTATUS __forceinline Core::TrueNtProtectVirtualMemory(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect)
{
	typedef NTSTATUS (WINAPI tNtProtectVirtualMemory)(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect);
	return reinterpret_cast<tNtProtectVirtualMemory *>(nt_protect_virtual_memory_)(ProcesssHandle, BaseAddress, Size, NewProtect, OldProtect);
}

NTSTATUS __forceinline Core::TrueNtClose(HANDLE Handle)
{
	typedef NTSTATUS (WINAPI tNtClose)(HANDLE Handle);
	return reinterpret_cast<tNtClose *>(nt_close_)(Handle);
}

NTSTATUS __forceinline Core::TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength)
{
	typedef NTSTATUS (WINAPI tNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
	return reinterpret_cast<tNtQueryObject *>(nt_query_object_)(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

#endif