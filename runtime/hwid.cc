#include "common.h"
#include "objects.h"
#include "utils.h"
#include "hwid.h"
#include "core.h"
#include "crypto.h"

#ifdef __unix__
#include <mntent.h>
#include <dirent.h>
#include <net/if.h>
#include <pthread.h>
#endif
#ifdef VMP_GNU
EXPORT_API int WINAPI ExportedGetCurrentHWID(char *buffer, int size) __asm__ ("ExportedGetCurrentHWID");
#endif
#ifdef WIN_DRIVER
#include <ntddndis.h>
extern "C" {
	NTSYSAPI
	NTSTATUS
	NTAPI
	ZwCreateEvent (
		PHANDLE EventHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		EVENT_TYPE EventType,
		BOOLEAN InitialState
		);

	NTSYSAPI
	NTSTATUS
	NTAPI
	ZwWaitForSingleObject(
		HANDLE         Handle,
		BOOLEAN        Alertable,
		PLARGE_INTEGER Timeout
);
}
#endif

int WINAPI ExportedGetCurrentHWID(char *buffer, int size)
{
	HardwareID *hardware_id = Core::Instance()->hardware_id();
	return hardware_id ? hardware_id->GetCurrent(buffer, size) : 0;
}

/**
 * HardwareID
 */

#ifdef WIN_DRIVER
bool GetRegValue(LPCWSTR reg_path, LPCWSTR value_name, LPWSTR buffer, size_t *size)
{
	UNICODE_STRING unicode_reg_path, unicode_value_name;
	OBJECT_ATTRIBUTES object_attributes;
	NTSTATUS status;
	HANDLE key_handle;
	ULONG information_size;
	KEY_VALUE_PARTIAL_INFORMATION *information;
	bool res = false;

	RtlInitUnicodeString(&unicode_reg_path, reg_path);
	RtlInitUnicodeString(&unicode_value_name, value_name);
	InitializeObjectAttributes(&object_attributes, &unicode_reg_path, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&key_handle, KEY_QUERY_VALUE, &object_attributes);
	if (NT_SUCCESS(status)) {
		information_size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH * sizeof(WCHAR);
		information = reinterpret_cast<KEY_VALUE_PARTIAL_INFORMATION*>(new uint8_t[information_size]);
		status = ZwQueryValueKey(key_handle, &unicode_value_name, KeyValuePartialInformation, information, information_size, &information_size);
		if (NT_SUCCESS(status)) {
			size_t len = information->DataLength / sizeof(wchar_t);
			if (*size < len + 1) {
				*size = len + 1;
			} else {
				RtlCopyMemory(buffer, information->Data, information->DataLength);
				buffer[len] = 0;
				*size = wcslen(buffer);
				res = true;
			}
		}
		delete [] information;
		ZwClose(key_handle);
	}
	return res;
}
#endif

HardwareID::HardwareID() 
	: block_count_(0), start_block_(0)
{
	uint64_t timestamp = __rdtsc(); // exactly 8 bytes
	timestamp ^= ~timestamp << 32; // remove zeroes at the beginning
	blocks_ = new CryptoContainer(sizeof(uint32_t) * MAX_BLOCKS, reinterpret_cast<uint8_t *>(&timestamp));

	// old methods
	GetCPU(0);
	GetCPU(1);
	start_block_ = block_count_;
	// new methods, we'll return HWID starting from this DWORD
	GetCPU(2); 
	GetMachineName();
	GetHDD();
	GetMacAddresses();
}

HardwareID::~HardwareID()
{
	delete blocks_;
}

void HardwareID::AddBlock(const void *p, size_t size, BlockType type)
{
	if (block_count_ == MAX_BLOCKS) return; // no free space

	SHA1 hash;
	hash.Input(reinterpret_cast<const uint8_t *>(p), size);
	uint32_t block = __builtin_bswap32(*reinterpret_cast<const uint32_t *>(hash.Result()));

	block &= ~TYPE_MASK; // zero two lower bits
	block |= type & TYPE_MASK; // set type bits

	// check existing blocks
	for (size_t i = block_count_; i > start_block_; i--) {
		uint32_t prev_block = blocks_->GetDWord((i - 1) * sizeof(uint32_t));
		if (prev_block == block)
			return;
		if ((prev_block & TYPE_MASK) != (block & TYPE_MASK))
			break;
	}

	blocks_->SetDWord(block_count_ * sizeof(uint32_t), block);
	block_count_++;
}

void HardwareID::GetCPU(uint8_t method)
{
	uint32_t old_block_count = block_count_;

#ifdef WIN_DRIVER
	KAFFINITY system_mask = KeQueryActiveProcessors();
	KAFFINITY mask = 1;
#endif
#ifdef VMP_GNU
	// set process affinity mask to system affinity mask
#ifdef __APPLE__
	//FIXME
	if (0) {
#else
	cpu_set_t process_mask, system_mask;
	memset(&system_mask, 0xFF, sizeof(system_mask));
	if (0 == sched_getaffinity(0, sizeof(process_mask), &process_mask)) {
		sched_setaffinity(0, sizeof(system_mask), &system_mask); //try all CPUs, will set MAX CPUs
		sched_getaffinity(0, sizeof(system_mask), &system_mask); //get MAX CPUs
#endif
#else
#ifndef WIN_DRIVER
	DWORD_PTR process_mask, system_mask;
	HANDLE process = GetCurrentProcess();
	if (GetProcessAffinityMask(process, &process_mask, &system_mask)) {
		if (process_mask != system_mask)
			SetProcessAffinityMask(process, system_mask);
		DWORD_PTR mask = 1;
		HANDLE thread = GetCurrentThread();
#endif
#endif
#ifdef VMP_GNU
		// set thread affinity mask to mask
#ifdef __APPLE__
		//FIXME
		while (false) {
			if (false) {
#else
		for (size_t i = 0; i < sizeof(system_mask) * 8; i++) {
			if (__CPU_ISSET_S(i, sizeof(system_mask), &system_mask)) {
				cpu_set_t mask;
				__CPU_ZERO_S(sizeof(mask), &mask);
				__CPU_SET_S(i, sizeof(mask), &mask);
				sched_setaffinity(0, sizeof(mask), &mask);
#endif
#else
		for (size_t i = 0; i < sizeof(mask) * 8; i++) {
			if (system_mask & mask) {
#ifdef WIN_DRIVER
				KeSetSystemAffinityThread(mask);
#else
				DWORD_PTR old_mask = SetThreadAffinityMask(thread, mask);
				Sleep(0);
#endif
#endif
				ProcessCPU(method);
#ifdef VMP_GNU
				// set thread affinity mask back to old_mask
#ifdef __APPLE__
			//FIXME
#else
			//do nothing
#endif
#else
#ifdef WIN_DRIVER
				KeRevertToUserAffinityThread();
#else
				SetThreadAffinityMask(thread, old_mask);
#endif
#endif
			}
#ifndef VMP_GNU
			mask <<= 1;
#endif
		}
#ifndef WIN_DRIVER
#ifdef VMP_GNU
		// set process affinity mask back to process_mask
#ifdef __APPLE__
		//FIXME
#else
		sched_setaffinity(0, sizeof(process_mask), &process_mask);
#endif
#else
		if (process_mask != system_mask)
			SetProcessAffinityMask(process, process_mask);
#endif
	}
#endif

	if (old_block_count == block_count_)
		ProcessCPU(method);
}

void HardwareID::ProcessCPU(uint8_t method)
{
	int info[4];
	__cpuid(info, 1);
	if ((info[0] & 0xFF0) == 0xFE0) 
		info[0] ^= 0x20; // fix Athlon bug
	info[1] &= 0x00FFFFFF; // mask out APIC Physical ID

	if (method == 2) {
		info[2] = 0;
	} else if (method == 1) {
		info[2] &= ~(1 << 27);
	}
	
	AddBlock(info, sizeof(info), BLOCK_CPU);
}

void HardwareID::GetMachineName()
{
#ifdef __APPLE__
	CFStringRef computer_name = SCDynamicStoreCopyComputerName(NULL, NULL);
	if (!computer_name)
		return;

	CFIndex len = CFStringGetLength(computer_name);
	CFIndex size = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8);
	char *buf = new char[size];
	if (CFStringGetCString(computer_name, buf, size, kCFStringEncodingUTF8))
		AddBlock(buf, strlen(buf), BLOCK_HOST);
	delete [] buf;
	CFRelease(computer_name);
#elif defined(__unix__)
	char buf[HOST_NAME_MAX+1] = {0};
	if (0 == gethostname(buf, HOST_NAME_MAX))
		AddBlock(buf, strlen(buf), BLOCK_HOST);
#elif defined(WIN_DRIVER)
	#define MAX_COMPUTERNAME_LENGTH 31

	wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
	size_t size = _countof(buf);

	if (GetRegValue(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName", L"ComputerName", buf, &size) ||
		GetRegValue(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName", L"ComputerName", buf, &size)) {
		AddBlock(buf, size * sizeof(wchar_t), BLOCK_HOST);
	}
#else
	HMODULE dll = LoadLibraryA(VMProtectDecryptStringA("kernel32.dll"));
	if (!dll) 
		return;

	typedef ULONG (WINAPI *GET_COMPUTER_NAME) (wchar_t *, uint32_t *);
	GET_COMPUTER_NAME get_computer_name = reinterpret_cast<GET_COMPUTER_NAME>(InternalGetProcAddress(dll, VMProtectDecryptStringA("GetComputerNameW")));

	if (get_computer_name) {
		wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
		uint32_t size = _countof(buf);

		if (get_computer_name(buf, &size))
			AddBlock(buf, size * sizeof(wchar_t), BLOCK_HOST);
	}

	FreeLibrary(dll);
#endif
}

void HardwareID::ProcessMAC(const uint8_t *p, size_t size)
{
	// this big IF construction allows to put constants to the code, not to the data segment
	// it is harder to find them in the code after virtualisation
	uint32_t dw = (p[0] << 16) + (p[1] << 8) + p[2];
	if (dw == 0x000569 || dw == 0x000C29 || dw == 0x001C14 || dw == 0x005056 || // vmware
		dw == 0x0003FF || dw == 0x000D3A || dw == 0x00125A || dw == 0x00155D || dw == 0x0017FA || dw == 0x001DD8 || dw == 0x002248 || dw == 0x0025AE || dw == 0x0050F2 || // microsoft
		dw == 0x001C42 || // parallels
		dw == 0x0021F6) // virtual iron
		return;

	AddBlock(p, size, BLOCK_MAC);
}

void HardwareID::GetMacAddresses()
{
#ifdef __APPLE__
	ifaddrs *addrs;
	if (getifaddrs(&addrs) == 0) {
		uint32_t block_count_no_mac = block_count_;
		const uint8_t *mac = NULL;
		size_t size = 0;
		for (ifaddrs *cur_addr = addrs; cur_addr != 0; cur_addr = cur_addr->ifa_next) {
			if (cur_addr->ifa_addr->sa_family != AF_LINK)
				continue;

			const sockaddr_dl *dl_addr = reinterpret_cast<const sockaddr_dl *>(cur_addr->ifa_addr);
			if (dl_addr->sdl_type == IFT_ETHER) {
				mac = reinterpret_cast<const uint8_t *>(&dl_addr->sdl_data[dl_addr->sdl_nlen]);
				size = dl_addr->sdl_alen;
				ProcessMAC(mac, size);
			}
		}
		if (block_count_no_mac == block_count_ && mac && size)
			AddBlock(mac, size, BLOCK_MAC);
		freeifaddrs(addrs);
	}    
#elif defined(__unix__)
	std::string dir_name("/sys/class/net/");
	if (DIR *dir = opendir(dir_name.c_str())) {
		uint32_t block_count_no_mac = block_count_;
		uint8_t mac[6];
		size_t size = 0;
		while (struct dirent *entry = readdir(dir)) {
			// skip "." and ".."
			if (entry->d_name[0] == '.') {
				if (entry->d_name[1] == 0 || (entry->d_name[1] == '.' && entry->d_name[2] == 0))
					continue;
			}

			struct stat st;
			if (fstatat(dirfd(dir), entry->d_name, &st, 0) >= 0 && S_ISDIR(st.st_mode)) {
				std::string file_name = dir_name + entry->d_name + "/address";
				if (FILE *faddr = fopen(file_name.c_str(), "r")) {
					char addr[18] = {0};
					if (fgets(addr, sizeof(addr) - 1, faddr)) {
						uint8_t m[6];
						size_t c = sscanf_s(addr, "%02hx:%02hx:%02hx:%02hx:%02hx:%02hx",
							m+0, m+1, m+2, m+3, m+4, m+5);
						if (c == 6 && m[0]+m[1]+m[2]+m[3]+m[4]+m[5] != 0) {
							memcpy(mac, m, sizeof(mac));
							size = c;
							ProcessMAC(mac, size);
						}
					}
					fclose(faddr);
				}
			}
		}
		closedir(dir);
		if (block_count_no_mac == block_count_ && size)
			AddBlock(mac, size, BLOCK_MAC);
	}
#elif defined(WIN_DRIVER)
	UNICODE_STRING unicode_string;
	OBJECT_ATTRIBUTES object_attributes;
	NTSTATUS status;
	HANDLE key_handle, file_handle, event_handle;
	IO_STATUS_BLOCK status_block;
	ULONG code = OID_802_3_CURRENT_ADDRESS;
	uint8_t mac[6];
	size_t size = 0;

	InitializeObjectAttributes(&object_attributes, NULL, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateEvent(&event_handle, EVENT_ALL_ACCESS, &object_attributes, NotificationEvent, FALSE);
	if (!NT_SUCCESS(status))
		return;

	RtlInitUnicodeString(&unicode_string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters");
	InitializeObjectAttributes(&object_attributes, &unicode_string, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&key_handle, GENERIC_READ, &object_attributes);
	if (NT_SUCCESS(status)) {
		uint32_t block_count_no_mac = block_count_;
		ULONG sub_key_size = sizeof(KEY_BASIC_INFORMATION) + sizeof(wchar_t) * MAX_PATH;
		KEY_BASIC_INFORMATION *sub_key_value = static_cast<KEY_BASIC_INFORMATION *>(ExAllocatePool(PagedPool, sub_key_size));
		wchar_t *service_name = static_cast<wchar_t *>(ExAllocatePool(PagedPool, sizeof(wchar_t) * MAX_PATH));

		ULONG ret_size;
		for (ULONG i = 0; NT_SUCCESS(ZwEnumerateKey(key_handle, i, KeyBasicInformation, sub_key_value, sub_key_size, &ret_size)); i++) {
			if (sub_key_value->NameLength > (MAX_PATH - 10) * sizeof(wchar_t))
				continue;

			if (sub_key_value->NameLength == 18 && _wcsnicmp(sub_key_value->Name, L"NdisWanIp", 9) == 0)
				continue;

			RtlZeroMemory(service_name, sizeof(wchar_t) * MAX_PATH);
#if WDK_NTDDI_VERSION > NTDDI_WIN7
			wcscat_s(service_name, MAX_PATH, L"\\??\\");
#else
            wcscat(service_name, L"\\??\\");
#endif
			RtlCopyMemory(service_name + wcslen(service_name), sub_key_value->Name, sub_key_value->NameLength);
			RtlInitUnicodeString(&unicode_string, service_name);
			InitializeObjectAttributes(&object_attributes, &unicode_string, OBJ_CASE_INSENSITIVE, NULL, NULL);

			status = ZwOpenFile(&file_handle, 0, &object_attributes, &status_block, FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
			if (NT_SUCCESS(status)) {
				status = ZwDeviceIoControlFile(file_handle, event_handle, 0, 0, &status_block, IOCTL_NDIS_QUERY_GLOBAL_STATS, &code, sizeof(code), mac, sizeof(mac));
				if (status == STATUS_PENDING)
					status = ZwWaitForSingleObject(event_handle, FALSE, NULL);
				if (NT_SUCCESS(status)) {
					size = sizeof(mac);
					ProcessMAC(mac, size);
				}
				ZwClose(file_handle);
			}
		}
		ExFreePool(service_name);
		ExFreePool(sub_key_value);
		ZwClose(key_handle);

		if (block_count_no_mac == block_count_ && size)
			AddBlock(mac, size, BLOCK_MAC);
	}
	ZwClose(event_handle);

#else
	HMODULE dll = LoadLibraryA(VMProtectDecryptStringA("iphlpapi.dll"));
	if (!dll) 
		return;

	typedef ULONG (WINAPI *GET_ADAPTERS_INFO) (PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer);
	GET_ADAPTERS_INFO get_adapters_info = reinterpret_cast<GET_ADAPTERS_INFO>(InternalGetProcAddress(dll, VMProtectDecryptStringA("GetAdaptersInfo")));

	if (get_adapters_info) {
		ULONG buf_len = 0;
		if (ERROR_BUFFER_OVERFLOW == get_adapters_info(NULL, &buf_len)) {
			uint32_t block_count_no_mac = block_count_;
			const uint8_t *mac = NULL;
			size_t size = 0;
			uint8_t *info = new uint8_t[buf_len];
			if (ERROR_SUCCESS == get_adapters_info(reinterpret_cast<IP_ADAPTER_INFO *>(info), &buf_len)) {
				for (IP_ADAPTER_INFO *adapter_info = reinterpret_cast<IP_ADAPTER_INFO *>(info); adapter_info != 0; adapter_info = adapter_info->Next) {
					mac = adapter_info->Address;
					size = adapter_info->AddressLength;
					ProcessMAC(mac, size);
				}
			}
			if (block_count_no_mac == block_count_ && mac && size)
				AddBlock(mac, size, BLOCK_MAC);
			delete [] info;
		}
	}
	FreeLibrary(dll);
#endif
}

void HardwareID::GetHDD()
{
#ifdef __APPLE__
	DASessionRef session = DASessionCreate(NULL);
	if (session) {
		struct statfs statFS;
		statfs ("/", &statFS);
		DADiskRef disk = DADiskCreateFromBSDName(NULL, session, statFS.f_mntfromname);
		if (disk) {
			CFDictionaryRef descDict = DADiskCopyDescription(disk);
			if (descDict) {
				CFUUIDRef value = (CFUUIDRef)CFDictionaryGetValue(descDict, CFSTR("DAVolumeUUID"));
				CFUUIDBytes bytes = CFUUIDGetUUIDBytes(value);
				AddBlock(&bytes, sizeof(bytes), BLOCK_HDD);
				CFRelease(descDict);
			}
			CFRelease(disk);
		}
		CFRelease(session);
	}
#elif defined(__unix__)
	std::string root_uuid;
	if (FILE *mtab = setmntent("/etc/mtab", "r")) {
		std::string root_fs_path;
		while (struct mntent *entry = getmntent(mtab)) {
			if (entry->mnt_dir[0] == '/' && entry->mnt_dir[1] == 0) {
				root_fs_path = entry->mnt_fsname;
				break;
			}
		}
		endmntent(mtab);

		if (!root_fs_path.empty()) {
			std::string dir_name("/dev/disk/by-uuid/");
			if (DIR *dir = opendir(dir_name.c_str())) {
				while (struct dirent *entry = readdir(dir)) {
					// skip "." and ".."
					if (entry->d_name[0] == '.') {
						if (entry->d_name[1] == 0 || (entry->d_name[1] == '.' && entry->d_name[2] == 0))
							continue;
					}

					char resolved_link_path[PATH_MAX];
					std::string path = dir_name + entry->d_name;
					if (realpath(path.c_str(), resolved_link_path)) {
						if (strcmp(resolved_link_path, root_fs_path.c_str()) == 0) {
							root_uuid = entry->d_name;
							break;
						}
					}
				}
				closedir(dir);
			}
		}
	}
	if (!root_uuid.empty())
		AddBlock(root_uuid.c_str(), root_uuid.size(), BLOCK_HDD);
#elif defined(WIN_DRIVER)
	wchar_t buf[MAX_PATH];
	size_t size = _countof(buf);
	UNICODE_STRING unicode_string;
	OBJECT_ATTRIBUTES object_attributes;
	NTSTATUS status;
	HANDLE handle;
	IO_STATUS_BLOCK status_block;

	if (!GetRegValue(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion", L"SystemRoot", buf, &size))
		return;

	if (buf[1] == ':' && buf[2] == '\\') {
		wchar_t system_drive[] = {'\\', 'D', 'o', 's', 'D', 'e', 'v', 'i', 'c', 'e', 's', '\\', buf[0], ':', '\\', 0};

		RtlInitUnicodeString(&unicode_string, system_drive);
		InitializeObjectAttributes(&object_attributes, &unicode_string, OBJ_CASE_INSENSITIVE, NULL, NULL);
		status = ZwCreateFile(&handle, SYNCHRONIZE | FILE_READ_ACCESS, 
								&object_attributes,
								&status_block,
								NULL,
								0,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								FILE_OPEN,
								FILE_SYNCHRONOUS_IO_NONALERT,
								NULL,
								0);
		if (NT_SUCCESS(status)) {
			RtlInitUnicodeString(&unicode_string, L"ZwQueryVolumeInformationFile");
			typedef NTSTATUS (NTAPI *QUERY_VOLUME_INFORMATION_FILE) (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FS_INFORMATION_CLASS);
			QUERY_VOLUME_INFORMATION_FILE query_volume_information_file = reinterpret_cast<QUERY_VOLUME_INFORMATION_FILE>(MmGetSystemRoutineAddress(&unicode_string));
			if (query_volume_information_file) {
				ULONG size = sizeof(FILE_FS_VOLUME_INFORMATION) + MAX_PATH * sizeof(WCHAR);
				FILE_FS_VOLUME_INFORMATION *information = reinterpret_cast<FILE_FS_VOLUME_INFORMATION*>(new uint8_t[size]);
				status = query_volume_information_file(handle, &status_block, information, size, FileFsVolumeInformation);
				if (NT_SUCCESS(status))
					AddBlock(&information->VolumeSerialNumber, sizeof(information->VolumeSerialNumber), BLOCK_HDD);
				delete [] information;
			}
			ZwClose(handle);
		}
	}
#else
	HMODULE dll = LoadLibraryA(VMProtectDecryptStringA("kernel32.dll"));
	if (!dll) 
		return;

	typedef ULONG (WINAPI *GET_WINDOWS_DIRECTORY) (wchar_t *, DWORD);
	GET_WINDOWS_DIRECTORY get_windows_directory = reinterpret_cast<GET_WINDOWS_DIRECTORY>(InternalGetProcAddress(dll, VMProtectDecryptStringA("GetWindowsDirectoryW")));
	typedef BOOL (WINAPI *GET_VOLUME_INFORMATION) (const wchar_t *, wchar_t *, DWORD, DWORD *, DWORD *, DWORD *, wchar_t *, DWORD);
	GET_VOLUME_INFORMATION get_volume_information = reinterpret_cast<GET_VOLUME_INFORMATION>(InternalGetProcAddress(dll, VMProtectDecryptStringA("GetVolumeInformationW")));

	if (get_windows_directory && get_volume_information) {
		wchar_t buf[MAX_PATH] = {0};
		UINT ures = get_windows_directory(buf, _countof(buf));
		if (ures > 0 && buf[1] == ':' && buf[2] == '\\') {
			buf[3] = 0;
			DWORD volumeSerialNumber = 0;
			if (get_volume_information(buf, NULL, 0, &volumeSerialNumber, NULL, NULL, NULL, 0)) {
				AddBlock(&volumeSerialNumber, sizeof(volumeSerialNumber), BLOCK_HDD);
			}
		}
	}
	FreeLibrary(dll);
#endif
}

size_t HardwareID::Copy(void *dest, size_t size) const
{
	uint32_t *p = reinterpret_cast<uint32_t *>(dest);
	// need skip old methods
	size_t res = std::min(static_cast<size_t>(block_count_ - start_block_), size / sizeof(uint32_t));
	for (size_t i = 0; i < res; i++) {
		p[i] = blocks_->GetDWord((start_block_ + i) * sizeof(uint32_t));
	}
	return res * sizeof(uint32_t);
}

/*
	Rules:
	1. if pointer to buffer is NULL, second parameter is ignored and returned size of buffer that will fit HWID (with trailing 0)
	2. if buffer is ok and the second parameters is zero, we should return zero and doesn't change the buffer
	3. if buffer is OK and the second parameter is less than we need, we should return as much bytes as we can and return the second parameter itself
	   and we should put 0 to the last available position at buffer
	4. if buffer is bigger that we need, we should put HWID, trailing 0 and return strlen(hwid) + 1
*/

int HardwareID::GetCurrent(char *buffer, int size)
{
	if (buffer && size == 0) 
		return 0; // see rule #2

	uint8_t b[MAX_BLOCKS *sizeof(uint32_t)];
	size_t hwid_size = Copy(b, sizeof(b));
	size_t need_size = Base64EncodeGetRequiredLength(hwid_size);
	char *p = new char[need_size + 1];
	Base64Encode(b, hwid_size, p, need_size); // it never should return false

	size_t copy = 0;
	if (buffer)	{
		// if nSize is less, we have to leave space for trailing zero (see rule #3)
		// if nNeedSize is less, we already have this space, as we allocated nNeedSize + 1 bytes
		copy = std::min(static_cast<size_t>(size - 1), need_size);
		for (size_t i = 0; i < copy; i++) {
			buffer[i] = p[i];
		}
		buffer[copy++] = 0;
	}
	else { 
		// see rule #1
		copy = need_size + 1;
	}

	delete [] p;
	return static_cast<int>(copy);
}

bool HardwareID::IsCorrect(uint8_t *p, size_t size) const
{
	if (p == 0 || size == 0 || (size & 3)) 
		return false;

	bool equals[4];
	bool found[4];
	for (size_t i = 0; i < 4; i++) {
		equals[i] = false;
		found[i] = false;
	}

	size_t blocks = size / sizeof(uint32_t);
	uint32_t *d = reinterpret_cast<uint32_t *>(p);
	for (size_t j = 0; j < block_count_; j++) {
		uint32_t id1 = blocks_->GetDWord(j * sizeof(uint32_t));
		found[id1 & 3] = true;
		for (size_t i = 0; i < blocks; i++) {
			uint32_t id2 = d[i];
			if (id1 == id2) {
				equals[id1 & 3] = true;
				break;
			}
		}
	}

	// check CPU
	if (!equals[0])
		return false;

	// check if at least 3 items are OK
	size_t n = 0;
	size_t c = 0;
	for (size_t i = 0; i < 4; i++) {
		if (found[i])
			c++;
		if (equals[i])
			n++;
	}
	return (n == c || n >= 3);
}

bool HardwareID::IsCorrect(CryptoContainer &cont, size_t offset, size_t size) const
{
	if (size == 0 || (size & 3) || size > MAX_BLOCKS * sizeof(uint32_t)) 
		return false;

	uint32_t buff[MAX_BLOCKS];
	for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
		buff[i] = cont.GetDWord(offset + i * sizeof(uint32_t));
	}
	return IsCorrect(reinterpret_cast<uint8_t *>(buff), size);
}