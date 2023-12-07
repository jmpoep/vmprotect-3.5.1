#include "common.h"
#include "loader.h"
#include "crypto.h"
#include "../third-party/lzma/LzmaDecode.h"

#define VIRTUAL_PROTECT_ERROR reinterpret_cast<const void *>(1)
#define UNPACKER_ERROR reinterpret_cast<const void *>(2)
#define INTERNAL_GPA_ERROR reinterpret_cast<const void *>(3)
#define CPU_HASH_ERROR reinterpret_cast<const void *>(5)

#define DECRYPT(str, pos) (str[pos] ^ (_rotl32(FACE_STRING_DECRYPT_KEY, static_cast<int>(pos)) + pos))

static void *LoaderAlloc(size_t size)
{
#ifdef VMP_GNU
	return malloc(size);
#elif defined(WIN_DRIVER)
	return ExAllocatePool((POOL_TYPE)FACE_NON_PAGED_POOL_NX, size);
#else
	return LocalAlloc(0, size);
#endif
}

static void LoaderFree(void *address)
{
#ifdef VMP_GNU
	free(address);
#elif defined(WIN_DRIVER)
	if (address)
		ExFreePool(address);
#else
	LocalFree(address);
#endif
}

NOINLINE bool LoaderFindFirmwareVendor(const uint8_t *data, size_t data_size)
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

#ifndef VMP_GNU

#define TOLOWER(c) ((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c)

int Loader_stricmp(const char *str1, const char *str2, bool is_enc)
{
	unsigned char c1;
	unsigned char c2;
	size_t pos = 0;
	do {
		c1 = *(str1++);
		c2 = *(str2++);
		if (is_enc) {
			c1 ^= static_cast<unsigned char>(_rotl32(FACE_STRING_DECRYPT_KEY, static_cast<int>(pos)) + pos);
			pos++;
		}
		c1 = TOLOWER(c1);
		c2 = TOLOWER(c2);
		if (!c1)
			break;
	} while (c1 == c2);

	if (c1 < c2)
		return -1;
	else if (c1 > c2)
		return 1;
	return 0;
}

int Loader_strcmp(const char *str1, const char *str2, bool is_enc)
{
	unsigned char c1;
	unsigned char c2;
	size_t pos = 0;
	do {
		c1 = *(str1++);
		c2 = *(str2++);
		if (is_enc) {
			c1 ^= (_rotl32(FACE_STRING_DECRYPT_KEY, static_cast<int>(pos)) + pos);
			pos++;
		}
		if (!c1)
			break;
	} while (c1 == c2);

	if (c1 < c2)
		return -1;
	else if (c1 > c2)
		return 1;
	return 0;
}

#ifdef WIN_DRIVER

int Loader_cmp_module(const char *str1, const char *str2, bool is_enc)
{
	unsigned char c1;
	unsigned char c2;
	size_t pos = 0;
	do {
		c1 = *(str1++);
		c2 = *(str2++);
		if (is_enc) {
			c1 ^= static_cast<unsigned char>(_rotl32(FACE_STRING_DECRYPT_KEY, static_cast<int>(pos)) + pos);
			pos++;
		}
		c1 = TOLOWER(c1);
		c2 = TOLOWER(c2);
		if (!c1)
			break;
	} while (c1 == c2);

	if (c1 == '.' && c2 == 0)
		return 0;
	if (c1 < c2)
		return -1;
	else if (c1 > c2)
		return 1;
	return 0;
}

HMODULE WINAPI LoaderGetModuleHandle(const char *name)
{
	HMODULE module = NULL;
	SYSTEM_MODULE_INFORMATION *buffer = NULL;
	ULONG buffer_size = 0;

	NTSTATUS status = NtQuerySystemInformation(SystemModuleInformation, &buffer, 0, &buffer_size);
	if (buffer_size) {
		buffer = static_cast<SYSTEM_MODULE_INFORMATION *>(LoaderAlloc(buffer_size * 2));
		if (buffer) {
			status = NtQuerySystemInformation(SystemModuleInformation, buffer, buffer_size * 2, &buffer_size);
			if (NT_SUCCESS(status)) {
				PVOID known_api;
				if (Loader_cmp_module(reinterpret_cast<const char *>(FACE_NTOSKRNL_NAME), name, true) == 0)
					known_api = &IoAllocateMdl;
				else if (Loader_cmp_module(reinterpret_cast<const char *>(FACE_HAL_NAME), name, true) == 0)
					known_api = &KeQueryPerformanceCounter;
				else
					known_api = NULL;

				if (known_api) {
					// search module by address
					for (size_t i = 0; i < buffer->Count; i++) {
						SYSTEM_MODULE_ENTRY *module_entry = &buffer->Module[i];
						if (module_entry->BaseAddress < known_api && static_cast<uint8_t *>(module_entry->BaseAddress) + module_entry->Size > known_api) {
							module = reinterpret_cast<HMODULE>(module_entry->BaseAddress);
							break;
						}
					}
				} else {
					// search module by name
					for (size_t i = 0; i < buffer->Count; i++) {
						SYSTEM_MODULE_ENTRY *module_entry = &buffer->Module[i];
						if (Loader_cmp_module(module_entry->Name + module_entry->PathLength, name, false) == 0) {
							module = reinterpret_cast<HMODULE>(module_entry->BaseAddress);
							break;
						}
					}
				}
			}
			LoaderFree(buffer);
		}
	}
	
	return module;
}
#else
HMODULE WINAPI LoaderGetModuleHandle(const char *name)
{
	return GetModuleHandleA(name);
}

__declspec(noinline) HMODULE LoaderLoadLibraryEnc(const char *name)
{
	// decrypt DLL name
	char dll_name[MAX_PATH];
	for (size_t j = 0; j < sizeof(dll_name); j++) {
		dll_name[j] = static_cast<char>(DECRYPT(name, j));
		if (!dll_name[j])
			break;
	}
	return LoadLibraryA(dll_name);
}

__declspec(noinline) const wchar_t *LoaderFindFileVersion(const uint8_t *ptr, size_t data_size) {
	const wchar_t *data = reinterpret_cast<const wchar_t *>(ptr);
	data_size /= sizeof(wchar_t);

	for (size_t i = 0; i < data_size; i++) {
		if (data_size >= 13) {
			if (data[i + 0] == L'F' && data[i + 1] == L'i' && data[i + 2] == L'l' && data[i + 3] == L'e' && data[i + 4] == L'V' && data[i + 5] == L'e' && data[i + 6] == L'r' &&
				data[i + 7] == L's' && data[i + 8] == L'i' && data[i + 9] == L'o' && data[i + 10] == L'n' && data[i + 11] == 0 && data[i + 12] == 0)
				return data + i + 13;
		}
		if (data_size >= 15) {
			if (data[i + 0] == L'P' && data[i + 1] == L'r' && data[i + 2] == L'o' && data[i + 3] == L'd' && data[i + 4] == L'u' && data[i + 5] == L'c' && data[i + 6] == L't' && 
				data[i + 7] == L'V' && data[i + 8] == L'e' && data[i + 9] == L'r' && data[i + 10] == L's' && data[i + 11] == L'i' && data[i + 12] == L'o' && data[i + 13] == L'n' && data[i + 14] == 0)
				return data + i + 15;
		}
	}
	return NULL;
}

__forceinline uint16_t LoaderParseOSBuildBumber(HMODULE ntdll)
{
	uint16_t os_build_number = 0;
	IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(ntdll);
	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE) {
		IMAGE_NT_HEADERS *pe_header = reinterpret_cast<IMAGE_NT_HEADERS *>(reinterpret_cast<uint8_t *>(ntdll) + dos_header->e_lfanew);
		if (pe_header->Signature == IMAGE_NT_SIGNATURE) {
			uint32_t resource_adress = pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
			if (resource_adress) {
				const uint8_t *resource_start = reinterpret_cast<const uint8_t *>(ntdll) + resource_adress;
				const uint8_t *resource_end = resource_start + pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
				while (const wchar_t *file_version = LoaderFindFileVersion(resource_start, resource_end - resource_start)) {
					os_build_number = 0;
					for (size_t i = 0; *file_version; file_version++) {
						if (*file_version == L'.')
							i++;
						else if (i == 2) {
							while (wchar_t c = *file_version++) {
								if (c >= L'0' && c <= L'9') {
									os_build_number *= 10;
									os_build_number += c - L'0';
								}
								else
									break;
							}
							break;
						}
					}

					if (IS_KNOWN_WINDOWS_BUILD(os_build_number))
						break;

					resource_start = reinterpret_cast<const uint8_t *>(file_version);
				}
			}
		}
	}
	return os_build_number;
}

__forceinline uint32_t LoaderParseSyscall(const void *p)
{
	if (const uint8_t *ptr = reinterpret_cast<const uint8_t *>(p)) {
#ifdef _WIN64
		if (ptr[0] == 0x4c && ptr[1] == 0x8b && (ptr[2] & 0xc0) == 0xc0)
			ptr += 3;
#endif
		if (ptr[0] == 0xb8)
			return *reinterpret_cast<const uint32_t *>(ptr + 1);
	}
	return 0;
}

#endif

void *LoaderGetProcAddress(HMODULE module, const char *proc_name, bool is_enc)
{
	// check input
	if (!module || !proc_name)
		return NULL;
	
	// check module's header
	IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(module);
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	// check NT header
	IMAGE_NT_HEADERS *pe_header = reinterpret_cast<IMAGE_NT_HEADERS *>(reinterpret_cast<uint8_t *>(module) + dos_header->e_lfanew);
	if (pe_header->Signature != IMAGE_NT_SIGNATURE) 
		return NULL;

	// get the export directory
	uint32_t export_adress = pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress; 
	if (!export_adress) 
		return NULL;

	uint32_t export_size = pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size; 
	uint32_t address;
	uint32_t ordinal_index = -1;
	IMAGE_EXPORT_DIRECTORY *export_directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(reinterpret_cast<uint8_t *>(module) + export_adress);

	if (proc_name <= reinterpret_cast<const char *>(0xFFFF)) { 
		// ordinal
		ordinal_index = static_cast<uint32_t>(INT_PTR(proc_name)) - export_directory->Base;
		// index is either less than base or bigger than number of functions
		if (ordinal_index >= export_directory->NumberOfFunctions) 
			return NULL;
		// get the function offset by the ordinal
		address = (reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(module) + export_directory->AddressOfFunctions))[ordinal_index]; 
		// check for empty offset
		if (!address) 
			return NULL;
	} else { 
		// name of function
		if (export_directory->NumberOfNames) {
			// start binary search
			int left_index = 0;
			int right_index = export_directory->NumberOfNames - 1;
			uint32_t *names = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(module) + export_directory->AddressOfNames);
			while (left_index <= right_index) {
				uint32_t cur_index = (left_index + right_index) >> 1;
				switch (Loader_strcmp(proc_name, (const char *)(reinterpret_cast<uint8_t *>(module) + names[cur_index]), is_enc)) {
				case 0:
					ordinal_index = (reinterpret_cast<WORD *>(reinterpret_cast<uint8_t *>(module) + export_directory->AddressOfNameOrdinals))[cur_index];
					left_index = right_index + 1;
					break;
				case -1:
					right_index = cur_index - 1;
					break;
				case 1:
					left_index = cur_index + 1;
					break;
				}
			}
		}
		// if nothing has been found
		if (ordinal_index >= export_directory->NumberOfFunctions) 
			return NULL;
		// get the function offset by the ordinal
		address = (reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(module) + export_directory->AddressOfFunctions))[ordinal_index];
		if (!address) 
			return NULL;
	}

	// if it is just a pointer - return it
	if (address < export_adress || address >= export_adress + export_size) 
		return reinterpret_cast<FARPROC>(reinterpret_cast<uint8_t *>(module) + address);

	// it is a forward
	const char *name = reinterpret_cast<const char *>(reinterpret_cast<uint8_t *>(module) + address); // get a pointer to the module's name
	const char *tmp = name;
	const char *name_dot = NULL;
	// get a pointer to the function's name
	while (*tmp) {
		if (*tmp == '.') {
			name_dot = tmp;
			break;
		}
		tmp++;
	}
	if (!name_dot) 
		return NULL;

	size_t name_len = name_dot - name;
	if (name_len >= MAX_PATH) 
		return NULL;

	// copy module name
	char file_name[MAX_PATH];
	size_t i;
	for (i = 0; i < name_len && name[i] != 0; i++) {
		file_name[i] = name[i];
	}
	file_name[i] = 0;

	HMODULE tmp_module = LoaderGetModuleHandle(file_name);
#ifndef WIN_DRIVER
	if (!tmp_module)
		tmp_module = LoadLibraryA(file_name);
#endif
	if (!tmp_module) 
		return NULL;

	if (tmp_module == module) {
		// forwarded to itself
#ifdef WIN_DRIVER
		return NULL;
#else
		if (is_enc) {
			for (i = 0; i < sizeof(file_name); i++) {
				char c = proc_name[i];
				c ^= (_rotl32(FACE_STRING_DECRYPT_KEY, static_cast<int>(i)) + i);
				file_name[i] = c;
				if (!c)
					break;
			}
			proc_name = file_name;
		}
		return GetProcAddress(module, proc_name);
#endif
	}

	// now the function's name
	// if it is not an ordinal, just forward it
	if (name_dot[1] != '#') 
		return LoaderGetProcAddress(tmp_module, name_dot + 1, false);

	// it is an ordinal
	tmp = name_dot + 2;
	int ordinal = 0;
	while (*tmp) {
		char c = *(tmp++);
		if (c >= '0' && c <= '9') {
			ordinal = ordinal * 10 + c - '0';
		} else {
			break;
		}
	}
	return LoaderGetProcAddress(tmp_module, reinterpret_cast<const char *>(INT_PTR(ordinal)), false);
}

__declspec(noinline) HMODULE LoaderGetModuleHandleEnc(const char *name)
{
	// decrypt DLL name
	char dll_name[MAX_PATH];
	for (size_t j = 0; j < sizeof(dll_name); j++) {
		dll_name[j] = static_cast<char>(DECRYPT(name, j));
		if (!dll_name[j])
			break;
	}
	return LoaderGetModuleHandle(dll_name);
}

__forceinline void InitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString)
{
	if (SourceString)
		DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(UNICODE_NULL);
	else
		DestinationString->MaximumLength = DestinationString->Length = 0;

	DestinationString->Buffer = (PWCH)SourceString;
}

#endif // VMP_GNU

#if defined(__unix__)
struct LoaderString {
public:
	LoaderString()
	{
		capacity_ = 2;
		buffer_ = reinterpret_cast<char *>(LoaderAlloc(capacity_));
		buffer_[0] = 0;
		size_ = 0;
	}
	~LoaderString()
	{
		LoaderFree(buffer_);
	}
	const char *c_str() const { return buffer_; }
	size_t size() const { return size_; }
	LoaderString &operator +=(char c)
	{
		add_size(1);

		buffer_[size_++] = c;
		buffer_[size_] = 0;
		return *this;
	}
	LoaderString &operator +=(const char *str)
	{
		size_t str_len = strlen(str);

		add_size(str_len);

		__movsb(buffer_ + size_, str, str_len);
		size_ += str_len;
		buffer_[size_] = 0;
		return *this;
	}
private:
	void add_size(size_t size)
	{
		size_t new_size = size_ + size + 1;
		if (new_size <= capacity_)
			return;

		while (capacity_ < new_size) {
			capacity_ <<= 1;
		}
		char *new_buffer = reinterpret_cast<char *>(LoaderAlloc(capacity_));
		__movsb(new_buffer, buffer_, size_ + 1);
		LoaderFree(buffer_);
		buffer_ = new_buffer;
	}

	char *buffer_;
	size_t size_;
	size_t capacity_;
};
#endif

enum MessageType {
	mtInitializationError,
	mtProcNotFound,
	mtOrdinalNotFound,
	mtFileCorrupted,
	mtDebuggerFound,
	mtUnregisteredVersion,
	mtVirtualMachineFound
};

void LoaderMessage(MessageType type, const void *param1 = NULL, const void *param2 = NULL)
{
	const VMP_CHAR *message;
	bool need_format = false;
	switch (type) {
	case mtDebuggerFound:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_DEBUGGER_FOUND);
		break;
	case mtVirtualMachineFound:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_VIRTUAL_MACHINE_FOUND);
		break;
	case mtFileCorrupted:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_FILE_CORRUPTED);
		break;
	case mtUnregisteredVersion:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_UNREGISTERED_VERSION);
		break;
	case mtInitializationError: 
		message = reinterpret_cast<const VMP_CHAR *>(FACE_INITIALIZATION_ERROR);
		need_format = true;
		break;
	case mtProcNotFound:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_PROC_NOT_FOUND);
		need_format = true;
		break;
	case mtOrdinalNotFound:
		message = reinterpret_cast<const VMP_CHAR *>(FACE_ORDINAL_NOT_FOUND);
		need_format = true;
		break;
	default:
		return;
	}

	VMP_CHAR message_buffer[1024];
	VMP_CHAR *dst = message_buffer;
	VMP_CHAR *end = dst + _countof(message_buffer) - 1;
	size_t param_index = 0;

	for (size_t j = 0; dst < end; j++) {
		VMP_CHAR c = static_cast<VMP_CHAR>(DECRYPT(message, j));
		if (!c)
			break;

		if (need_format && c == '%') {
			j++;
			const void *param = (param_index == 0) ? param1 : param2;
			param_index++;
			c = static_cast<VMP_CHAR>(DECRYPT(message, j));
			if (c == 's') {
				const char *src = reinterpret_cast<const char *>(param);

				while (*src && dst < end) {
					*(dst++) = *(src++);
				}
			} else if (c == 'c') {
				const char *src = reinterpret_cast<const char *>(param);

				for (size_t k = 0; dst < end; k++) {
					char b = static_cast<char>(DECRYPT(src, k));
					if (!b)
						break;
					*(dst++) = b;
				}
			} else if (c == 'd') {
				size_t value = reinterpret_cast<size_t>(param);
				char buff[20];
				char *src = buff + _countof(buff) - 1;

				*src = 0;
				do {
					*(--src) = '0' + value % 10;
					value /= 10;
				} while (value);

				while (*src && dst < end) {
					*(dst++) = *(src++);
				}
			}
		} else {
			*(dst++) = c;
		}
	}
	*dst = 0;
	message = message_buffer;

	if (!message[0])
		return;

#ifdef __APPLE__
	char file_name[PATH_MAX];
	uint32_t name_size = sizeof(file_name);
	const char *title = file_name;
	if (_NSGetExecutablePath(file_name, &name_size) == 0) {
		for (size_t i = 0; i < sizeof(file_name); i++) {
			if (!file_name[i])
				break;
			if (file_name[i] == '/')
				title = file_name + i + 1;
		}
	} else {
		file_name[0] = 'F';
		file_name[1] = 'a';
		file_name[2] = 't';
		file_name[3] = 'a';
		file_name[4] = 'l';
		file_name[5] = ' ';
		file_name[6] = 'E';
		file_name[7] = 'r';
		file_name[8] = 'r';
		file_name[9] = 'o';
		file_name[10] = 'r';
		file_name[11] = 0;
	}

	CFStringRef title_ref = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
	CFStringRef message_ref = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	CFOptionFlags result;

	CFUserNotificationDisplayAlert(
									0, // no timeout
									(type == mtUnregisteredVersion) ? kCFUserNotificationCautionAlertLevel : kCFUserNotificationStopAlertLevel, //change it depending message_type flags ( MB_ICONASTERISK.... etc.)
									NULL, //icon url, use default, you can change it depending message_type flags
									NULL, //not used
									NULL, //localization of strings
									title_ref, //title text 
									message_ref, //message text
									NULL, //default "ok" text in button
									NULL, //alternate button title
									NULL, //other button title, null--> no other button
									&result //response flags
									);

	CFRelease(title_ref);
	CFRelease(message_ref);
#elif defined(__unix__)
	char file_name[PATH_MAX];
	char self_exe[] = {'/', 'p', 'r', 'o', 'c', '/', 's', 'e', 'l', 'f', '/', 'e', 'x', 'e', 0};
	const char *title = file_name;
	ssize_t len = ::readlink(self_exe, file_name, sizeof(file_name) - 1);
	if (len != -1) {
		for (ssize_t i = 0; i < len; i++) {
			if (file_name[i] == '/')
				title = file_name + i + 1;
			if (file_name[i] == '\'' || file_name[i] == '\\')
				file_name[i] = '"';
		}
		file_name[len] = '\0';
	} else {
		file_name[0] = 'F';
		file_name[1] = 'a';
		file_name[2] = 't';
		file_name[3] = 'a';
		file_name[4] = 'l';
		file_name[5] = ' ';
		file_name[6] = 'E';
		file_name[7] = 'r';
		file_name[8] = 'r';
		file_name[9] = 'o';
		file_name[10] = 'r';
		file_name[11] = 0;
	}
	LoaderString cmd_line;
	{
		char str[] = {'z', 'e', 'n', 'i', 't', 'y', 0};
		cmd_line += str;
	}
	if (type == mtUnregisteredVersion)
	{
		char str[] = {' ', '-', '-', 'w', 'a', 'r', 'n', 'i', 'n', 'g', 0};
		cmd_line += str;
	}
	else
	{
		char str[] = {' ', '-', '-', 'e', 'r', 'r', 'o', 'r', 0};
		cmd_line += str;
	}
	{
		char str[] = {' ', '-', '-', 'n', 'o', '-', 'm', 'a', 'r', 'k', 'u', 'p', 0};
		cmd_line += str;
	}
	{
		char str[] = {' ', '-', '-', 't', 'i', 't', 'l', 'e', '=', '\'', 0};
		cmd_line += str;
		cmd_line += title;
		cmd_line += '\'';
	}		
	{
		char str[] = {' ', '-', '-', 't', 'e', 'x', 't', '=', '\'', 0};
		cmd_line += str;
		char *msg_ptr = message_buffer;
		while (*msg_ptr)
		{
			if (*msg_ptr == '\'' || *msg_ptr == '\\')
				*msg_ptr = '"';
			msg_ptr++;
		}
		cmd_line += message;
		cmd_line += '\'';
	}
	int status = system(cmd_line.c_str());
	if (status == -1 || WEXITSTATUS(status) == 127)
		puts(message);
#elif defined(WIN_DRIVER)
	DbgPrint(reinterpret_cast<const char *>(FACE_DRIVER_FORMAT_VALUE), message);
#else
	wchar_t file_name[MAX_PATH];
	const wchar_t *title = file_name;
	if (GetModuleFileNameW(reinterpret_cast<HMODULE>(FACE_IMAGE_BASE), file_name, _countof(file_name))) {
		for (size_t i = 0; i < _countof(file_name); i++) {
			if (!file_name[i])
				break;
			if (file_name[i] == '\\')
				title = file_name + i + 1;
		}
	} else {
		file_name[0] = 'F';
		file_name[1] = 'a';
		file_name[2] = 't';
		file_name[3] = 'a';
		file_name[4] = 'l';
		file_name[5] = ' ';
		file_name[6] = 'E';
		file_name[7] = 'r';
		file_name[8] = 'r';
		file_name[9] = 'o';
		file_name[10] = 'r';
		file_name[11] = 0;
	}

	HMODULE ntdll = LoaderGetModuleHandleEnc(reinterpret_cast<const char *>(FACE_NTDLL_NAME));

	typedef NTSTATUS(NTAPI tNtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters,
		ULONG UnicodeStringParameterMask, PULONG_PTR Parameters,
		ULONG ValidResponseOptions,
		HardErrorResponse *Response);

	if (tNtRaiseHardError *raise_hard_error = reinterpret_cast<tNtRaiseHardError *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_RAISE_HARD_ERROR_NAME), true))) {
		UNICODE_STRING message_str;
		UNICODE_STRING title_str;

		InitUnicodeString(&message_str, (PWSTR)message);
		InitUnicodeString(&title_str, (PWSTR)title);

		ULONG_PTR params[4] = {
			(ULONG_PTR)&message_str,
			(ULONG_PTR)&title_str,
			(
				(ULONG)ResponseButtonOK |
				(type == mtUnregisteredVersion ? IconWarning : IconError)
			),
			INFINITE
		};

		HardErrorResponse response;
		raise_hard_error(STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE, 4, 3, params, 0, &response);
	}
#endif
}

enum {
	LOADER_SUCCESS = 
#ifdef WIN_DRIVER
		STATUS_SUCCESS
#else
		TRUE
#endif
	,

	LOADER_ERROR =
#ifdef WIN_DRIVER
		STATUS_DRIVER_INTERNAL_ERROR
#else
		FALSE
#endif
};

#ifdef VMP_GNU
EXPORT_API void FreeImage() __asm__ ("FreeImage");
#endif

#ifdef WIN_DRIVER
void WINAPI FreeImage(PDRIVER_OBJECT driver_object)
#else
void WINAPI FreeImage()
#endif
{
	SETUP_IMAGE_DATA data;
	
	uint8_t *image_base = data.image_base();
	uint32_t loader_status = LOADER_ERROR;
	GlobalData *loader_data = *(reinterpret_cast<GlobalData**>(image_base + data.storage()));
	if (loader_data) {
#ifdef WIN_DRIVER
		if (loader_data->driver_unload())
			reinterpret_cast<PDRIVER_UNLOAD>(loader_data->driver_unload())(driver_object);
#endif

		if (uint32_t data_runtime_entry = data.runtime_entry()) {
#ifdef VMP_GNU
			typedef void (WINAPI tRuntimeEntry)(HMODULE hModule, bool is_init);
			reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), false);
#elif defined(WIN_DRIVER)
			typedef NTSTATUS (tRuntimeEntry)(HMODULE hModule, bool is_init);
			reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), false);
#else
			typedef BOOL (WINAPI tRuntimeEntry)(HMODULE hModule, DWORD dwReason, LPVOID lpReserved);
			reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), DLL_PROCESS_DETACH, NULL);
#endif
		}

		loader_status = loader_data->loader_status();
		LoaderFree(loader_data);
	}

	if (loader_status == LOADER_ERROR) {
#ifdef VMP_GNU
		exit(0xDEADC0DE);
#elif defined(WIN_DRIVER)
		// do nothing
#else
		if (data.options() & LOADER_OPTION_EXIT_PROCESS)
			ExitProcess(0xDEADC0DE);
#endif
	}
}

void LoaderProcessFixups(ptrdiff_t delta_base, uint32_t data_fixup_info, uint32_t data_fixup_info_size, uint8_t *image_base, uint8_t *dst_image_base)
{
	size_t i, j, c;
	FIXUP_INFO *fixup_info;
	for (i = 0; i < data_fixup_info_size; i += fixup_info->BlockSize) {
		fixup_info = reinterpret_cast<FIXUP_INFO *>(image_base + data_fixup_info + i);
		if (fixup_info->BlockSize < sizeof(FIXUP_INFO))
			break;

		c = (fixup_info->BlockSize - sizeof(FIXUP_INFO)) >> 1;
		for (j = 0; j < c; j++) {
			uint16_t type_offset = reinterpret_cast<uint16_t *>(fixup_info + 1)[j];
			uint8_t *address = dst_image_base + fixup_info->Address + (type_offset >> 4);

			// need use "if" instead "switch"
			uint8_t type = (type_offset & 0x0f);
#ifdef __APPLE__
			if (type == REBASE_TYPE_POINTER)
				*(reinterpret_cast<ptrdiff_t *>(address)) += delta_base;
			else if (type == REBASE_TYPE_TEXT_ABSOLUTE32)
				*(reinterpret_cast<ptrdiff_t *>(address)) += delta_base;
#elif defined(__unix__)
			if (type == 8) // R_386_RELATIVE
				*(reinterpret_cast<ptrdiff_t *>(address)) += delta_base;
#else
			if (type == IMAGE_REL_BASED_HIGHLOW)
				*(reinterpret_cast<uint32_t *>(address)) += static_cast<uint32_t>(delta_base);
			else if (type == IMAGE_REL_BASED_DIR64)
				*(reinterpret_cast<uint64_t *>(address)) += delta_base;
			else if (type == IMAGE_REL_BASED_HIGH)
				*(reinterpret_cast<uint16_t *>(address)) += static_cast<uint16_t>(delta_base >> 16);
			else if (type == IMAGE_REL_BASED_LOW)
				*(reinterpret_cast<uint16_t *>(address)) += static_cast<uint16_t>(delta_base);
#endif
		}
	}
}

#ifdef VMP_GNU
EXPORT_API uint32_t WINAPI SetupImage() __asm__ ("SetupImage");
#endif
#ifdef WIN_DRIVER
uint32_t WINAPI SetupImage(bool is_init, PDRIVER_OBJECT driver_object)
#else
uint32_t WINAPI SetupImage()
#endif
{
	size_t i, j;
	uint64_t session_key;
	SETUP_IMAGE_DATA data;
	
	uint8_t *image_base = data.image_base();
	uint8_t *file_base = data.file_base();
	uint8_t *dst_image_base = image_base;
	ptrdiff_t delta_base = image_base - file_base;

	GlobalData *tmp_loader_data = *(reinterpret_cast<GlobalData**>(image_base + data.storage()));
	if (tmp_loader_data) {
#ifdef WIN_DRIVER
		if (!is_init && tmp_loader_data->loader_status() == LOADER_SUCCESS && driver_object->DriverUnload) {
			tmp_loader_data->set_driver_unload(reinterpret_cast<size_t>(driver_object->DriverUnload));
			driver_object->DriverUnload = FreeImage;
		}
#endif
		return tmp_loader_data->loader_status();
	}

	tmp_loader_data = reinterpret_cast<GlobalData *>(LoaderAlloc(sizeof(GlobalData)));
	tmp_loader_data->set_loader_status(LOADER_ERROR);
	tmp_loader_data->set_is_patch_detected(false);
	tmp_loader_data->set_is_debugger_detected(false);
	tmp_loader_data->set_server_date(0);
	tmp_loader_data->set_loader_crc_info(data.loader_crc_info());
	tmp_loader_data->set_loader_crc_size(0);
	size_t crc_image_size = 0;
	tmp_loader_data->set_crc_image_size(crc_image_size);
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
	tmp_loader_data->set_driver_unload(0);
#else 
	tmp_loader_data->set_os_build_number(0);
#endif

	size_t cpu_salt = 0;
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
	HMODULE ntoskrnl = LoaderGetModuleHandleEnc(reinterpret_cast<const char *>(FACE_NTOSKRNL_NAME));
#else
	typedef NTSTATUS(NTAPI tNtClose)(HANDLE Handle);
	typedef NTSTATUS(NTAPI tNtOpenFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
	typedef NTSTATUS(NTAPI tNtCreateSection)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);
	typedef NTSTATUS(NTAPI tNtMapViewOfSection)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
	typedef NTSTATUS(NTAPI tNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);
	typedef NTSTATUS(NTAPI tNtQueryInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
	typedef NTSTATUS(NTAPI tNtSetInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
	typedef NTSTATUS(NTAPI tNtProtectVirtualMemory)(HANDLE ProcesssHandle, LPVOID *BaseAddress, SIZE_T *Size, DWORD NewProtect, PDWORD OldProtect);
	typedef NTSTATUS(NTAPI tNtOpenSection)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
	typedef NTSTATUS(NTAPI tNtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);
	typedef NTSTATUS(NTAPI tNtSetInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);

	HMODULE kernel32 = LoaderGetModuleHandleEnc(reinterpret_cast<const char *>(FACE_KERNEL32_NAME));
	HMODULE ntdll = LoaderGetModuleHandleEnc(reinterpret_cast<const char *>(FACE_NTDLL_NAME));
	HANDLE process = NtCurrentProcess();
	HANDLE thread = NtCurrentThread();
	size_t syscall = FACE_SYSCALL;
	uint32_t sc_close = 0;
	uint32_t sc_virtual_protect = 0;
	uint32_t sc_open_file = 0;
	uint32_t sc_create_section = 0;
	uint32_t sc_map_view_of_section = 0;
	uint32_t sc_unmap_view_of_section = 0;
	uint32_t sc_query_information_process = 0;
	uint32_t sc_set_information_thread = 0;
	uint32_t sc_query_virtual_memory = 0;
#ifdef _WIN64
	PEB64 *peb = reinterpret_cast<PEB64 *>(__readgsqword(0x60));
#else
	PEB32 *peb = reinterpret_cast<PEB32 *>(__readfsdword(0x30));
#endif
	cpu_salt = peb->OSBuildNumber << 7;

#ifndef DEMO
	if (LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_WINE_GET_VERSION_NAME), true) == NULL) {
#ifndef _WIN64
		BOOL is_wow64 = FALSE;
		typedef BOOL(WINAPI tIsWow64Process)(HANDLE Process, PBOOL Wow64Process);
		tIsWow64Process *is_wow64_process = reinterpret_cast<tIsWow64Process *>(LoaderGetProcAddress(kernel32, reinterpret_cast<const char *>(FACE_IS_WOW64_PROCESS_NAME), true));
		if (is_wow64_process)
			is_wow64_process(process, &is_wow64);
#endif

		uint16_t os_build_number = peb->OSBuildNumber;
		if (!IS_KNOWN_WINDOWS_BUILD(os_build_number)) {
			// parse FileVersion/ProductVersion from NTDLL resources
			os_build_number = LoaderParseOSBuildBumber(ntdll);
			if (!IS_KNOWN_WINDOWS_BUILD(os_build_number)) {
				// load copy of NTDLL
				tNtQueryVirtualMemory *query_virtual_memory = reinterpret_cast<tNtQueryVirtualMemory *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_QUERY_VIRTUAL_MEMORY_NAME), true));
				tNtOpenFile *open_file = reinterpret_cast<tNtOpenFile *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_OPEN_FILE_NAME), true));
				tNtCreateSection *create_section = reinterpret_cast<tNtCreateSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_CREATE_SECTION_NAME), true));
				tNtMapViewOfSection *map_view_of_section = reinterpret_cast<tNtMapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_MAP_VIEW_OF_SECTION), true));
				tNtUnmapViewOfSection *unmap_view_of_section = reinterpret_cast<tNtUnmapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_UNMAP_VIEW_OF_SECTION), true));
				tNtClose *close = reinterpret_cast<tNtClose *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_CLOSE), true));

				if (!query_virtual_memory
					|| !create_section
					|| !open_file
					|| !map_view_of_section
					|| !unmap_view_of_section
					|| !close) {
					LoaderMessage(mtInitializationError, INTERNAL_GPA_ERROR);
					return LOADER_ERROR;
				}

				os_build_number = 0;
				uint8_t buffer[MAX_PATH * sizeof(wchar_t)];
				if (NT_SUCCESS(query_virtual_memory(process, ntdll, MemoryMappedFilenameInformation, buffer, sizeof(buffer), NULL))) {
					HANDLE file_handle;
					OBJECT_ATTRIBUTES object_attributes;
					IO_STATUS_BLOCK io_status_block;

					InitializeObjectAttributes(&object_attributes, reinterpret_cast<UNICODE_STRING *>(buffer), 0, NULL, NULL);
					if (NT_SUCCESS(open_file(&file_handle, GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &object_attributes, &io_status_block, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT))) {
						HANDLE file_map;
						InitializeObjectAttributes(&object_attributes, NULL, 0, NULL, NULL);
						if (NT_SUCCESS(create_section(&file_map, SECTION_MAP_READ, &object_attributes, NULL, PAGE_READONLY, SEC_IMAGE_NO_EXECUTE, file_handle))) {
							void *copy_ntdll = NULL;
							SIZE_T file_size = 0;
							LARGE_INTEGER offset;
							offset.LowPart = 0;
							offset.HighPart = 0;
							if (NT_SUCCESS(map_view_of_section(file_map, process, &copy_ntdll, NULL, 0, &offset, &file_size, ViewShare, 0, PAGE_READONLY))) {
								os_build_number = LoaderParseOSBuildBumber((HMODULE)copy_ntdll);

								if (!IS_KNOWN_WINDOWS_BUILD(os_build_number)) {
									sc_close = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_CLOSE), true));
									sc_virtual_protect = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_VIRTUAL_PROTECT_NAME), true));
									sc_open_file = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_OPEN_FILE_NAME), true));
									sc_create_section = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_CREATE_SECTION_NAME), true));
									sc_map_view_of_section = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_MAP_VIEW_OF_SECTION), true));
									sc_unmap_view_of_section = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_UNMAP_VIEW_OF_SECTION), true));
									sc_query_information_process = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_QUERY_INFORMATION_PROCESS_NAME), true));
									sc_set_information_thread = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_NT_SET_INFORMATION_THREAD_NAME), true));
									sc_query_virtual_memory = LoaderParseSyscall(LoaderGetProcAddress((HMODULE)copy_ntdll, reinterpret_cast<const char *>(FACE_QUERY_VIRTUAL_MEMORY_NAME), true));

#ifndef _WIN64
									if (is_wow64) {
										// wow64 version of ntdll uses upper 16 bits for encoding information about arguments
										sc_close = static_cast<uint16_t>(sc_close);
										sc_virtual_protect = static_cast<uint16_t>(sc_virtual_protect);
										sc_open_file = static_cast<uint16_t>(sc_open_file);
										sc_create_section = static_cast<uint16_t>(sc_create_section);
										sc_map_view_of_section = static_cast<uint16_t>(sc_map_view_of_section);
										sc_unmap_view_of_section = static_cast<uint16_t>(sc_unmap_view_of_section);
										sc_query_information_process = static_cast<uint16_t>(sc_query_information_process);
										sc_set_information_thread = static_cast<uint16_t>(sc_set_information_thread);
										sc_query_virtual_memory = static_cast<uint16_t>(sc_query_virtual_memory);
									}
#endif
								}

								unmap_view_of_section(process, copy_ntdll);
							}
							close(file_map);
						}
						close(file_handle);
					}
				}
			}

			if (!os_build_number) {
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
		}

		tmp_loader_data->set_os_build_number(os_build_number);

		if (os_build_number == WINDOWS_XP) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0019;
				sc_virtual_protect = 0x0089;
				sc_open_file = 0x0074;
				sc_create_section = 0x0032;
				sc_map_view_of_section = 0x006c;
				sc_unmap_view_of_section = 0x010b;
				sc_query_information_process = 0x009a;
				sc_set_information_thread = 0x00e5;
				sc_query_virtual_memory = 0x00b2;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_2003) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x001b;
				sc_virtual_protect = 0x008f;
				sc_open_file = 0x007a;
				sc_create_section = 0x0034;
				sc_map_view_of_section = 0x0071;
				sc_unmap_view_of_section = 0x0115;
				sc_query_information_process = 0x00a1;
				sc_set_information_thread = 0x00ee;
				sc_query_virtual_memory = 0x00ba;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_VISTA) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0030;
				sc_virtual_protect = 0x00d2;
				sc_open_file = 0x00ba;
				sc_create_section = 0x004b;
				sc_map_view_of_section = 0x00b1;
				sc_unmap_view_of_section = 0x0160;
				sc_query_information_process = 0x00e4;
				sc_set_information_thread = 0x0136;
				sc_query_virtual_memory = 0x00fd;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_VISTA_SP1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0030;
				sc_virtual_protect = 0x00d2;
				sc_open_file = 0x00ba;
				sc_create_section = 0x004b;
				sc_map_view_of_section = 0x00b1;
				sc_unmap_view_of_section = 0x015c;
				sc_query_information_process = 0x00e4;
				sc_set_information_thread = 0x0132;
				sc_query_virtual_memory = 0x00fd;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}

		}
		else if (os_build_number == WINDOWS_VISTA_SP2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0030;
				sc_virtual_protect = 0x00d2;
				sc_open_file = 0x00ba;
				sc_create_section = 0x004b;
				sc_map_view_of_section = 0x00b1;
				sc_unmap_view_of_section = 0x015c;
				sc_query_information_process = 0x00e4;
				sc_set_information_thread = 0x0132;
				sc_query_virtual_memory = 0x00fd;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_7) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0032;
				sc_virtual_protect = 0x00d7;
				sc_open_file = 0x00b3;
				sc_create_section = 0x0054;
				sc_map_view_of_section = 0x00a8;
				sc_unmap_view_of_section = 0x0181;
				sc_query_information_process = 0x00ea;
				sc_set_information_thread = 0x014f;
				sc_query_virtual_memory = 0x010b;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_7_SP1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0032;
				sc_virtual_protect = 0x00d7;
				sc_open_file = 0x00b3;
				sc_create_section = 0x0054;
				sc_map_view_of_section = 0x00a8;
				sc_unmap_view_of_section = 0x0181;
				sc_query_information_process = 0x00ea;
				sc_set_information_thread = 0x014f;
				sc_query_virtual_memory = 0x010b;
			}
			else
#endif
			{
				sc_close = 0x000c;
				sc_virtual_protect = 0x004d;
				sc_open_file = 0x0030;
				sc_create_section = 0x0047;
				sc_map_view_of_section = 0x0025;
				sc_unmap_view_of_section = 0x0027;
				sc_query_information_process = 0x0016;
				sc_set_information_thread = 0x000a;
				sc_query_virtual_memory = 0x0020;
			}
		}
		else if (os_build_number == WINDOWS_8) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0174;
				sc_virtual_protect = 0x00c3;
				sc_open_file = 0x00e8;
				sc_create_section = 0x0150;
				sc_map_view_of_section = 0x00f3;
				sc_unmap_view_of_section = 0x0013;
				sc_query_information_process = 0x00b0;
				sc_set_information_thread = 0x0048;
				sc_query_virtual_memory = 0x008f;
		}
			else
#endif
			{
				sc_close = 0x000d;
				sc_virtual_protect = 0x004e;
				sc_open_file = 0x0031;
				sc_create_section = 0x0048;
				sc_map_view_of_section = 0x0026;
				sc_unmap_view_of_section = 0x0028;
				sc_query_information_process = 0x0017;
				sc_set_information_thread = 0x000b;
				sc_query_virtual_memory = 0x0021;
			}
		}
		else if (os_build_number == WINDOWS_8_1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0179;
				sc_virtual_protect = 0x00c6;
				sc_open_file = 0x00eb;
				sc_create_section = 0x0154;
				sc_map_view_of_section = 0x00f6;
				sc_unmap_view_of_section = 0x0013;
				sc_query_information_process = 0x00b3;
				sc_set_information_thread = 0x004b;
				sc_query_virtual_memory = 0x0092;
			}
			else
#endif
			{
				sc_close = 0x000e;
				sc_virtual_protect = 0x004f;
				sc_open_file = 0x0032;
				sc_create_section = 0x0049;
				sc_map_view_of_section = 0x0027;
				sc_unmap_view_of_section = 0x0029;
				sc_query_information_process = 0x0018;
				sc_set_information_thread = 0x000c;
				sc_query_virtual_memory = 0x0022;
			}
		}
		else if (os_build_number == WINDOWS_10_TH1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0180;
				sc_virtual_protect = 0x00c8;
				sc_open_file = 0x00ee;
				sc_create_section = 0x015a;
				sc_map_view_of_section = 0x00fa;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b5;
				sc_set_information_thread = 0x004c;
				sc_query_virtual_memory = 0x0094;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_TH2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0183;
				sc_virtual_protect = 0x00c8;
				sc_open_file = 0x00ee;
				sc_create_section = 0x015c;
				sc_map_view_of_section = 0x00fa;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b5;
				sc_set_information_thread = 0x004c;
				sc_query_virtual_memory = 0x0094;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_RS1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x0185;
				sc_virtual_protect = 0x00ca;
				sc_open_file = 0x00f0;
				sc_create_section = 0x015e;
				sc_map_view_of_section = 0x00fc;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b7;
				sc_set_information_thread = 0x004c;
				sc_query_virtual_memory = 0x0095;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		} else if (os_build_number == WINDOWS_10_RS2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018a;
				sc_virtual_protect = 0x00cc;
				sc_open_file = 0x00f2;
				sc_create_section = 0x0161;
				sc_map_view_of_section = 0x00fe;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b8;
				sc_set_information_thread = 0x004c;
				sc_query_virtual_memory = 0x0096;
		}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_RS3) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018d;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0164;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
		}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_RS4) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018d;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0164;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
		}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_RS5) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018d;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_19H1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018d;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_19H2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018d;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_20H1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018e;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_20H2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018e;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_21H1) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018e;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_21H2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018e;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
		else if (os_build_number == WINDOWS_10_22H2) {
#ifndef _WIN64
			if (!is_wow64) {
				sc_close = 0x018e;
				sc_virtual_protect = 0x00ce;
				sc_open_file = 0x00f4;
				sc_create_section = 0x0163;
				sc_map_view_of_section = 0x0101;
				sc_unmap_view_of_section = 0x0014;
				sc_query_information_process = 0x00b9;
				sc_set_information_thread = 0x004d;
				sc_query_virtual_memory = 0x0097;
			}
			else
#endif
			{
				sc_close = 0x000f;
				sc_virtual_protect = 0x0050;
				sc_open_file = 0x0033;
				sc_create_section = 0x004a;
				sc_map_view_of_section = 0x0028;
				sc_unmap_view_of_section = 0x002a;
				sc_query_information_process = 0x0019;
				sc_set_information_thread = 0x000d;
				sc_query_virtual_memory = 0x0023;
			}
		}
#ifndef _WIN64
		if (is_wow64 && sc_close) {
			sc_close |= WOW64_FLAG;
			sc_virtual_protect |= WOW64_FLAG | (0x01 << 24);
			sc_set_information_thread |= WOW64_FLAG | (0x02 << 24);
			sc_query_information_process |= WOW64_FLAG | (0x03 << 24);
			sc_map_view_of_section |= WOW64_FLAG | (0x04 << 24);;
			sc_unmap_view_of_section |= WOW64_FLAG | (0x05 << 24);
			sc_open_file |= WOW64_FLAG | (0x06 << 24);
			sc_create_section |= WOW64_FLAG | (0x07 << 24);
			sc_query_virtual_memory |= WOW64_FLAG | (0x08 << 24);
		}
#endif
	}
#endif
#endif

	// detect a debugger
#ifdef __APPLE__
	if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
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
		if ((info.kp_proc.p_flag & P_TRACED) != 0) {
			LoaderMessage(mtDebuggerFound);
			return LOADER_ERROR;
		}

		// disable debugger
		void *handle = dlopen(0, RTLD_GLOBAL | RTLD_NOW);
		if (handle) {
			typedef int (ptrace_t)(int _request, pid_t _pid, caddr_t _addr, int _data);
			ptrace_t *ptrace_ptr = reinterpret_cast<ptrace_t *>(dlsym(handle, reinterpret_cast<const char *>(FACE_GNU_PTRACE)));
			if (ptrace_ptr)
				ptrace_ptr(PT_DENY_ATTACH, 0, 0, 0);
			dlclose(handle);
		}
	}
#elif defined(__unix__)
	if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
		char mode[2];
		mode[0] = 'r'; mode[1] = 0;
		char str[18];
		str[0] = '/'; str[1] = 'p'; str[2] = 'r'; str[3] = 'o'; str[4] = 'c'; str[5] = '/'; str[6] = 's'; str[7] = 'e'; str[8] = 'l'; str[9] = 'f'; str[10] = '/';
		str[11] = 's'; str[12] = 't'; str[13] = 'a'; str[14] = 't'; str[15] = 'u'; str[16] = 's'; str[17] = 0;
		FILE *file = fopen(str, mode);
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

			if (tracer_pid && tracer_pid != 1) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
		}
	}
#else
#if defined(WIN_DRIVER)
#else
	if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
		if (peb->BeingDebugged) {
			LoaderMessage(mtDebuggerFound);
			return LOADER_ERROR;
		}

		if (sc_query_information_process) {
			// disable InstrumentationCallback
			if (peb->OSMajorVersion > 6) {
				// Windows 10
				PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info;
				info.Version = 0;
				info.Reserved = 0;
				info.Callback = NULL;
				if (tNtSetInformationProcess *set_information_process = reinterpret_cast<tNtSetInformationProcess *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_SET_INFORMATION_PROCESS_NAME), true)))
					set_information_process(process, ProcessInstrumentationCallback, &info, sizeof(info));
			}

			HANDLE debug_object;
			if (NT_SUCCESS(reinterpret_cast<tNtQueryInformationProcess *>(syscall | sc_query_information_process)(process, ProcessDebugPort, &debug_object, sizeof(debug_object), NULL)) && debug_object != 0) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
			debug_object = 0;
			if (NT_SUCCESS(reinterpret_cast<tNtQueryInformationProcess *>(syscall | sc_query_information_process)(process, ProcessDebugObjectHandle, &debug_object, sizeof(debug_object), reinterpret_cast<PULONG>(&debug_object))) 
				|| debug_object == 0) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}

		} else if (tNtQueryInformationProcess *query_information_process = reinterpret_cast<tNtQueryInformationProcess *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_QUERY_INFORMATION_PROCESS_NAME), true))) {
			HANDLE debug_object;
			if (NT_SUCCESS(query_information_process(process, ProcessDebugPort, &debug_object, sizeof(debug_object), NULL)) && debug_object != 0) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
			if (NT_SUCCESS(query_information_process(process, ProcessDebugObjectHandle, &debug_object, sizeof(debug_object), NULL))) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
		}

		// disable debugger
		if (sc_set_information_thread)
			reinterpret_cast<tNtSetInformationThread *>(syscall | sc_set_information_thread)(thread, ThreadHideFromDebugger, NULL, 0);
		else if (tNtSetInformationThread *set_information_thread = reinterpret_cast<tNtSetInformationThread *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_SET_INFORMATION_THREAD_NAME), true)))
			set_information_thread(thread, ThreadHideFromDebugger, NULL, 0);
	}
#endif

#ifdef WIN_DRIVER
	if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
#else
	if (data.options() & LOADER_OPTION_CHECK_KERNEL_DEBUGGER) {
#endif
		bool is_found = false;
		typedef NTSTATUS (NTAPI tNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
#ifdef WIN_DRIVER
		tNtQuerySystemInformation *nt_query_system_information = &NtQuerySystemInformation;
#else
		tNtQuerySystemInformation *nt_query_system_information = reinterpret_cast<tNtQuerySystemInformation *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_QUERY_INFORMATION_NAME), true));
		if (nt_query_system_information) {
#endif
			SYSTEM_KERNEL_DEBUGGER_INFORMATION info;
			NTSTATUS status = nt_query_system_information(SystemKernelDebuggerInformation, &info, sizeof(info), NULL);
			if (NT_SUCCESS(status) && info.DebuggerEnabled && !info.DebuggerNotPresent) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}

			SYSTEM_MODULE_INFORMATION *buffer = NULL;
			ULONG buffer_size = 0;
			status = nt_query_system_information(SystemModuleInformation, &buffer, 0, &buffer_size);
			if (buffer_size) {
				buffer = reinterpret_cast<SYSTEM_MODULE_INFORMATION *>(LoaderAlloc(buffer_size * 2));
				if (buffer) {
					status = nt_query_system_information(SystemModuleInformation, buffer, buffer_size * 2, NULL);
					if (NT_SUCCESS(status)) {
						for (size_t i = 0; i < buffer->Count && !is_found; i++) {
							SYSTEM_MODULE_ENTRY *module_entry = &buffer->Module[i];
							for (size_t j = 0; j < 5 ; j++) {
								const char *module_name;
								switch (j) {
								case 0:
									module_name = reinterpret_cast<const char *>(FACE_SICE_NAME);
									break;
								case 1:
									module_name = reinterpret_cast<const char *>(FACE_SIWVID_NAME);
									break;
								case 2:
									module_name = reinterpret_cast<const char *>(FACE_NTICE_NAME);
									break;
								case 3:
									module_name = reinterpret_cast<const char *>(FACE_ICEEXT_NAME);
									break;
								case 4:
									module_name = reinterpret_cast<const char *>(FACE_SYSER_NAME);
									break;
								}
								if (Loader_stricmp(module_name, module_entry->Name + module_entry->PathLength, true) == 0) {
									is_found = true;
									break;
								}
							}
						}
					}
					LoaderFree(buffer);
				}
			}
#ifndef WIN_DRIVER
		}
#endif
		if (is_found) {
			LoaderMessage(mtDebuggerFound);
			return LOADER_ERROR;
		}
	}
#endif

	// check header and loader CRC
	if (data.loader_crc_info_size()) {
		uint32_t loader_crc_info_size = *reinterpret_cast<uint32_t *>(image_base + data.loader_crc_info_size());
		uint32_t loader_crc_info_hash = *reinterpret_cast<uint32_t *>(image_base + data.loader_crc_info_hash());
		bool is_valid_crc = true;
		CRCValueCryptor crc_cryptor;

		if (loader_crc_info_hash != CalcCRC(image_base + data.loader_crc_info(), loader_crc_info_size))
			is_valid_crc = false;
		for (i = 0; i < loader_crc_info_size; i += sizeof(CRC_INFO)) {
			CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(image_base + data.loader_crc_info() + i);
			crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
			crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
			crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);
		
			if (crc_info.Hash != CalcCRC(image_base + crc_info.Address, crc_info.Size))
				is_valid_crc = false;
		}

		if (!is_valid_crc) {
			if (data.options() & LOADER_OPTION_CHECK_PATCH) {
				LoaderMessage(mtFileCorrupted);
				return LOADER_ERROR;
			}
			tmp_loader_data->set_is_patch_detected(true);
		}

		tmp_loader_data->set_loader_crc_size(loader_crc_info_size);
		tmp_loader_data->set_loader_crc_hash(loader_crc_info_hash);

		if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
			// check debugger again
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
			if (sc_query_information_process) {
				HANDLE debug_object;
				if (NT_SUCCESS(reinterpret_cast<tNtQueryInformationProcess *>(syscall | sc_query_information_process)(process, ProcessDebugPort, &debug_object, sizeof(debug_object), NULL)) && debug_object != 0) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
			}
			if (sc_set_information_thread)
				reinterpret_cast<tNtSetInformationThread *>(syscall | sc_set_information_thread)(thread, ThreadHideFromDebugger, NULL, 0);
#endif
		}
	}

	// check file CRC
	if (data.file_crc_info_size()) {
		uint32_t file_crc_info_size = *reinterpret_cast<uint32_t *>(image_base + data.file_crc_info_size());
#ifdef VMP_GNU
		Dl_info info;
		if (dladdr(reinterpret_cast<const void *>(&LzmaDecode), &info)) {
			int file_handle = open(info.dli_fname, O_RDONLY);
			if (file_handle != -1) {
				FILE_CRC_INFO *file_info = reinterpret_cast<FILE_CRC_INFO *>(image_base + data.file_crc_info());

				size_t file_size = lseek(file_handle, 0, SEEK_END);
				bool is_valid_crc;
				if (file_size < file_info->FileSize) {
					is_valid_crc = false;
				} else {
					is_valid_crc = true;
					uint8_t *file_view = reinterpret_cast<uint8_t *>(mmap(0, file_size, PROT_READ, MAP_SHARED, file_handle, 0));
					if (file_view != MAP_FAILED) {
						size_t arch_offset = 0;
#ifdef __APPLE__
						fat_header *fat = reinterpret_cast<fat_header*>(file_view);
						if (fat->magic == FAT_MAGIC || fat->magic == FAT_CIGAM) {
							fat_arch *arch = reinterpret_cast<fat_arch*>(file_view + sizeof(fat_header));
							mach_header *mach = reinterpret_cast<mach_header*>(image_base);
							for (i = 0; i < fat->nfat_arch; i++) {
								fat_arch cur_arch = arch[i];
								if (fat->magic == FAT_CIGAM) {
									cur_arch.cputype = __builtin_bswap32(cur_arch.cputype);
									cur_arch.cpusubtype = __builtin_bswap32(cur_arch.cpusubtype);
									cur_arch.offset = __builtin_bswap32(cur_arch.offset);
									cur_arch.size = __builtin_bswap32(cur_arch.size);
								}
								if (cur_arch.cputype == mach->cputype && cur_arch.cpusubtype == mach->cpusubtype) {
									arch_offset = cur_arch.offset;
									if (cur_arch.size < file_info->FileSize)
										is_valid_crc = false;
									break;
								}
							}
						}
#endif
						if (is_valid_crc) {
							CRCValueCryptor crc_cryptor;
							for (i = sizeof(FILE_CRC_INFO); i < file_crc_info_size; i += sizeof(CRC_INFO)) {
								CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(image_base + data.file_crc_info() + i);
								crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
								crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
								crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);
	
								if (crc_info.Hash != CalcCRC(file_view + arch_offset + crc_info.Address, crc_info.Size))
									is_valid_crc = false;
							}
						}
						munmap(file_view, file_size);
					}
				}
				close(file_handle);

				if (!is_valid_crc) {
					if (data.options() & LOADER_OPTION_CHECK_PATCH) {
						LoaderMessage(mtFileCorrupted);
						return LOADER_ERROR;
					}
					tmp_loader_data->set_is_patch_detected(true);
				}
			}
		}
#elif defined(WIN_DRIVER)
	// FIXME
#else
		wchar_t file_name[MAX_PATH];
		if (GetModuleFileNameW(reinterpret_cast<HMODULE>(image_base), file_name + 6, _countof(file_name) - 6)) {
			wchar_t *nt_file_name;
			if (file_name[6] == '\\' && file_name[7] == '\\') {
				nt_file_name = file_name;
				nt_file_name[0] = '\\';
				nt_file_name[1] = '?';
				nt_file_name[2] = '?';
				nt_file_name[3] = '\\';
				nt_file_name[4] = 'U';
				nt_file_name[5] = 'N';
				nt_file_name[6] = 'C';
			} else {
				nt_file_name = file_name + 2;
				nt_file_name[0] = '\\';
				nt_file_name[1] = '?';
				nt_file_name[2] = '?';
				nt_file_name[3] = '\\';
			}

			if (sc_open_file) {
				HANDLE file_handle;
				OBJECT_ATTRIBUTES object_attributes;
				IO_STATUS_BLOCK io_status_block;
				UNICODE_STRING str;

				InitUnicodeString(&str, nt_file_name);
				InitializeObjectAttributes(&object_attributes, &str, 0, NULL, NULL);
				if (NT_SUCCESS(reinterpret_cast<tNtOpenFile *>(syscall | sc_open_file)(&file_handle, GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &object_attributes, &io_status_block, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT))) {
					FILE_CRC_INFO *file_info = reinterpret_cast<FILE_CRC_INFO *>(image_base + data.file_crc_info());
					bool is_valid_crc = true;
					HANDLE file_map;
					InitializeObjectAttributes(&object_attributes, NULL, 0, NULL, NULL);
					if (NT_SUCCESS(reinterpret_cast<tNtCreateSection *>(syscall | sc_create_section)(&file_map, SECTION_MAP_READ, &object_attributes, NULL, PAGE_READONLY, SEC_COMMIT, file_handle))) {
						void *file_view = NULL;
						SIZE_T file_size = 0;
						LARGE_INTEGER offset;
						offset.LowPart = 0;
						offset.HighPart = 0;
						if (NT_SUCCESS(reinterpret_cast<tNtMapViewOfSection *>(syscall | sc_map_view_of_section)(file_map, process, &file_view, NULL, 0, &offset, &file_size, ViewShare, 0, PAGE_READONLY))) {
							if (file_size < file_info->FileSize)
								is_valid_crc = false;
							else {
								CRCValueCryptor crc_cryptor;
								for (i = sizeof(FILE_CRC_INFO); i < file_crc_info_size; i += sizeof(CRC_INFO)) {
									CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(image_base + data.file_crc_info() + i);
									crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
									crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
									crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);

									if (crc_info.Hash != CalcCRC(static_cast<uint8_t *>(file_view) + crc_info.Address, crc_info.Size))
										is_valid_crc = false;
								}
							}
							reinterpret_cast<tNtUnmapViewOfSection *>(syscall | sc_unmap_view_of_section)(process, file_view);
						}
						reinterpret_cast<tNtClose *>(syscall | sc_close)(file_map);
					}
					reinterpret_cast<tNtClose *>(syscall | sc_close)(file_handle);

					if (!is_valid_crc) {
						if (data.options() & LOADER_OPTION_CHECK_PATCH) {
							LoaderMessage(mtFileCorrupted);
							return LOADER_ERROR;
						}
						tmp_loader_data->set_is_patch_detected(true);
					}
				}
			}
			else {
				tNtOpenFile *open_file = reinterpret_cast<tNtOpenFile *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_OPEN_FILE_NAME), true));
				tNtCreateSection *create_section = reinterpret_cast<tNtCreateSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_CREATE_SECTION_NAME), true));
				tNtMapViewOfSection *map_view_of_section = reinterpret_cast<tNtMapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_MAP_VIEW_OF_SECTION), true));
				tNtUnmapViewOfSection *unmap_view_of_section = reinterpret_cast<tNtUnmapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_UNMAP_VIEW_OF_SECTION), true));
				tNtClose *close = reinterpret_cast<tNtClose *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_CLOSE), true));

				if (!create_section
					|| !open_file
					|| !map_view_of_section
					|| !unmap_view_of_section
					|| !close) {
					LoaderMessage(mtInitializationError, INTERNAL_GPA_ERROR);
					return LOADER_ERROR;
				}

				// check breakpoint
				uint8_t *ckeck_list[] = { reinterpret_cast<uint8_t*>(create_section),
										reinterpret_cast<uint8_t*>(open_file),
										reinterpret_cast<uint8_t*>(map_view_of_section),
										reinterpret_cast<uint8_t*>(unmap_view_of_section),
										reinterpret_cast<uint8_t*>(close) };
				for (i = 0; i < _countof(ckeck_list); i++) {
					if (*ckeck_list[i] == 0xcc) {
						if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
							LoaderMessage(mtDebuggerFound);
							return LOADER_ERROR;
						}
						tmp_loader_data->set_is_debugger_detected(true);
					}
				}

				HANDLE file_handle;
				OBJECT_ATTRIBUTES object_attributes;
				IO_STATUS_BLOCK io_status_block;
				UNICODE_STRING str;

				InitUnicodeString(&str, nt_file_name);
				InitializeObjectAttributes(&object_attributes, &str, 0, NULL, NULL);
				if (NT_SUCCESS(open_file(&file_handle, GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &object_attributes, &io_status_block, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT))) {
					FILE_CRC_INFO *file_info = reinterpret_cast<FILE_CRC_INFO *>(image_base + data.file_crc_info());
					bool is_valid_crc = true;
					HANDLE file_map;
					InitializeObjectAttributes(&object_attributes, NULL, 0, NULL, NULL);
					if (NT_SUCCESS(create_section(&file_map, SECTION_MAP_READ, &object_attributes, NULL, PAGE_READONLY, SEC_COMMIT, file_handle))) {
						void *file_view = NULL;
						SIZE_T file_size = 0;
						LARGE_INTEGER offset;
						offset.LowPart = 0;
						offset.HighPart = 0;
						if (NT_SUCCESS(map_view_of_section(file_map, process, &file_view, NULL, 0, &offset, &file_size, ViewShare, 0, PAGE_READONLY))) {
							if (file_size < file_info->FileSize)
								is_valid_crc = false;
							else {
								CRCValueCryptor crc_cryptor;
								for (i = sizeof(FILE_CRC_INFO); i < file_crc_info_size; i += sizeof(CRC_INFO)) {
									CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(image_base + data.file_crc_info() + i);
									crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
									crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
									crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);

									if (crc_info.Hash != CalcCRC(static_cast<uint8_t *>(file_view) + crc_info.Address, crc_info.Size))
										is_valid_crc = false;
								}
							}
							unmap_view_of_section(process, file_view);
						}
						close(file_map);
					}
					close(file_handle);

					if (!is_valid_crc) {
						if (data.options() & LOADER_OPTION_CHECK_PATCH) {
							LoaderMessage(mtFileCorrupted);
							return LOADER_ERROR;
						}
						tmp_loader_data->set_is_patch_detected(true);
					}
				}
			}
		}
#endif
	}

	// setup WRITABLE flag for memory pages
	uint32_t data_section_info_size = data.section_info_size();
	uint32_t data_section_info = data.section_info();
#ifdef VMP_GNU
	for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
		SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

		int protect = PROT_READ | PROT_WRITE;
		if (section_info->Type & PROT_EXEC)
			protect |= PROT_EXEC;
		if (mprotect(image_base + section_info->Address, section_info->Size, protect) != 0) {
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
	}
#elif defined(WIN_DRIVER)
	ULONG size = 0;
	for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
		SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

		if (section_info->Address + section_info->Size > size)
			size = section_info->Address + section_info->Size;
	}
	PMDL mdl = NULL;
	if (size) {
		mdl = IoAllocateMdl(image_base, size, FALSE, FALSE, NULL);
		if (!mdl) {
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
		__try {
			MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			IoFreeMdl(mdl);
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
		dst_image_base = static_cast<uint8_t *>(MmGetSystemAddressForMdlSafe(mdl, (MM_PAGE_PRIORITY)FACE_DEFAULT_MDL_PRIORITY));
		if (!dst_image_base) {
			MmUnlockPages(mdl);
			IoFreeMdl(mdl);
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
	}
#else
	tNtProtectVirtualMemory *virtual_protect = NULL;
	if (sc_virtual_protect) {
		for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
			SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

			DWORD protect, old_protect;
			protect = (section_info->Type & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			void *address = image_base + section_info->Address;
			SIZE_T size = section_info->Size;
			if (!NT_SUCCESS(reinterpret_cast<tNtProtectVirtualMemory *>(syscall | sc_virtual_protect)(process, &address, &size, protect, &old_protect))) {
				LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
				return LOADER_ERROR;
			}
			if (old_protect & PAGE_GUARD) {
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
		}
	} else {
		virtual_protect = reinterpret_cast<tNtProtectVirtualMemory *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_VIRTUAL_PROTECT_NAME), true));
		if (!virtual_protect) {
			LoaderMessage(mtInitializationError, INTERNAL_GPA_ERROR);
			return LOADER_ERROR;
		}
		// check breakpoint
		if (*reinterpret_cast<uint8_t*>(virtual_protect) == 0xcc) {
			if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
			tmp_loader_data->set_is_debugger_detected(true);
		}
		for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
			SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

			DWORD protect, old_protect;
			protect = (section_info->Type & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			void *address = image_base + section_info->Address;
			SIZE_T size = section_info->Size;
			if (!NT_SUCCESS(virtual_protect(process, &address, &size, protect, &old_protect))) {
				LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
				return LOADER_ERROR;
			}
			if (old_protect & PAGE_GUARD) {
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
		}
	}
#endif

	// unpack regions
	if (data.packer_info_size()) {
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
		uint32_t tls_index = 0;
		if (data.tls_index_info())
			tls_index = *reinterpret_cast<uint32_t *>(image_base + data.tls_index_info());
#endif
		PACKER_INFO *packer_info = reinterpret_cast<PACKER_INFO *>(image_base + data.packer_info());
		CLzmaDecoderState state;
		if (LzmaDecodeProperties(&state.Properties, image_base + packer_info->Src, packer_info->Dst) != LZMA_RESULT_OK) {
			LoaderMessage(mtInitializationError, UNPACKER_ERROR);
			return LOADER_ERROR;
		}
		state.Probs = (CProb *)LoaderAlloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
		if (state.Probs == 0) {
			LoaderMessage(mtInitializationError, UNPACKER_ERROR);
			return LOADER_ERROR;
		}
		SizeT src_processed_size;
		SizeT dst_processed_size;
		for (i = sizeof(PACKER_INFO); i < data.packer_info_size(); i += sizeof(PACKER_INFO)) {
			packer_info = reinterpret_cast<PACKER_INFO *>(image_base + data.packer_info() + i);
			if (LzmaDecode(&state, image_base + packer_info->Src, -1, &src_processed_size, dst_image_base + packer_info->Dst, -1, &dst_processed_size) != LZMA_RESULT_OK) {
				LoaderFree(state.Probs);
				LoaderMessage(mtInitializationError, UNPACKER_ERROR);
				return LOADER_ERROR;
			}
		}
		LoaderFree(state.Probs);
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
		if (data.tls_index_info())
			*reinterpret_cast<uint32_t *>(dst_image_base + data.tls_index_info()) = tls_index;
#endif
	}

	// setup fixups
	if (delta_base != 0)
		LoaderProcessFixups(delta_base, data.fixup_info(), data.fixup_info_size(), image_base, dst_image_base);

	// setup IAT
	for (i = 0; i < data.iat_info_size(); i += sizeof(IAT_INFO)) {
		IAT_INFO *iat_info = reinterpret_cast<IAT_INFO *>(image_base + data.iat_info() + i);
		__movsb(iat_info->Dst + dst_image_base, iat_info->Src + image_base, iat_info->Size);
	}

	// setup import
#ifdef VMP_GNU
#else
	for (i = 0; i < data.import_info_size(); i += sizeof(DLL_INFO)) {
		DLL_INFO *dll_info = reinterpret_cast<DLL_INFO *>(image_base + data.import_info() + i);
		const char *dll_name = reinterpret_cast<const char *>(image_base + dll_info->Name);
		HMODULE h = LoaderGetModuleHandleEnc(dll_name);

		for (IMPORT_INFO *import_info = reinterpret_cast<IMPORT_INFO *>(dll_info + 1); import_info->Name != 0; import_info++, i += sizeof(IMPORT_INFO)) {
			const char *api_name;
			if (import_info->Name & IMAGE_ORDINAL_FLAG32) {
				api_name = LPCSTR(INT_PTR((IMAGE_ORDINAL32(import_info->Name))));
			} else {
				api_name = LPCSTR(INT_PTR((image_base + import_info->Name)));
			}

			void *p = LoaderGetProcAddress(h, api_name, true);
			if (!p) {
#ifndef WIN_DRIVER
				ULONG error_mode = 0;
				if (tNtQueryInformationProcess *query_information_process = reinterpret_cast<tNtQueryInformationProcess *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_QUERY_INFORMATION_PROCESS_NAME), true))) {
					query_information_process(process, ProcessDefaultHardErrorMode, &error_mode, sizeof(error_mode), NULL);
				}
				if (error_mode & SEM_FAILCRITICALERRORS)
					return LOADER_ERROR;
#endif
				LoaderMessage((import_info->Name & IMAGE_ORDINAL_FLAG32) ? mtOrdinalNotFound : mtProcNotFound, api_name, dll_name);
				return LOADER_ERROR;
			}

			*(reinterpret_cast<void **>(dst_image_base + import_info->Address)) = reinterpret_cast<uint8_t *>(p) - import_info->Key; 
		}

		i += sizeof(uint32_t);
	}
#endif

	// setup internal import
	for (i = 0; i < data.internal_import_info_size(); i += sizeof(IMPORT_INFO)) {
		IMPORT_INFO *import_info = reinterpret_cast<IMPORT_INFO *>(image_base + data.internal_import_info() + i);
		*(reinterpret_cast<void **>(dst_image_base + import_info->Address)) = import_info->Name + dst_image_base - import_info->Key; 
	}

	// setup relocations
	for (i = 0; i < data.relocation_info_size(); i += sizeof(RELOCATION_INFO)) {
		RELOCATION_INFO *relocation_info = reinterpret_cast<RELOCATION_INFO *>(image_base + data.relocation_info() + i);
		uint8_t *address = dst_image_base + relocation_info->Address;
		uint8_t *source = dst_image_base + relocation_info->Source;
#ifdef __unix__
		if (relocation_info->Type == 0) {
			// R_386_IRELATIVE
			*reinterpret_cast<size_t *>(address) = reinterpret_cast<size_t(*)(void)>(source)();
		} else if (relocation_info->Type == 1) {
			// R_386_PC32
			*reinterpret_cast<uint32_t *>(address) += *reinterpret_cast<uint32_t *>(source) - static_cast<uint32_t>(reinterpret_cast<size_t>(address));
		}
#else
		if (relocation_info->Type == 0) {
			*reinterpret_cast<size_t *>(address) += relocation_info->Source;
		} else {
			size_t data = 0;
#if defined(_M_X64) || defined(__amd64__)
			if (relocation_info->Type == 4)
				data = *reinterpret_cast<int64_t *>(address);
			else 
#endif
			if (relocation_info->Type == 3)
				data = *reinterpret_cast<int32_t *>(address);
			else if (relocation_info->Type == 2)
				data = *reinterpret_cast<int16_t *>(address);
			else if (relocation_info->Type == 1)
				data = *reinterpret_cast<int8_t *>(address);

			data -= reinterpret_cast<size_t>(source);
			data += *reinterpret_cast<size_t *>(source);

#if defined(_M_X64) || defined(__amd64__)
			if (relocation_info->Type == 4)
				*reinterpret_cast<uint64_t *>(address) = static_cast<uint64_t>(data);
			else
#endif
			if (relocation_info->Type == 3)
				*reinterpret_cast<uint32_t *>(address) = static_cast<uint32_t>(data);
			else if (relocation_info->Type == 2)
				*reinterpret_cast<uint16_t *>(address) = static_cast<uint16_t>(data);
			else if (relocation_info->Type == 1)
				*reinterpret_cast<uint8_t *>(address) = static_cast<uint8_t>(data);
		}
#endif
	}

#ifndef VMP_GNU
#ifndef WIN_DRIVER
	if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
		typedef BOOL (WINAPI tCloseHandle)(HANDLE hObject);
		tCloseHandle *close_handle = reinterpret_cast<tCloseHandle *>(LoaderGetProcAddress(kernel32, reinterpret_cast<const char *>(FACE_CLOSE_HANDLE_NAME), true));
		if (close_handle) {
			__try {
				if (close_handle(HANDLE(INT_PTR(0xDEADC0DE)))) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
		}

		size_t drx;
		uint64_t val;
		CONTEXT *ctx;
		__try {
			__writeeflags(__readeflags() | 0x100);
			 val = __rdtsc();
			 __nop();
			LoaderMessage(mtDebuggerFound);
			return LOADER_ERROR;
		} __except(ctx = (GetExceptionInformation())->ContextRecord,
			drx = (ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS) ? ctx->Dr0 | ctx->Dr1 | ctx->Dr2 | ctx->Dr3 : 0,
			EXCEPTION_EXECUTE_HANDLER) {
			if (drx) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
		}
	}
#endif
#endif

	// detect a virtual machine
	if (data.options() & LOADER_OPTION_CHECK_VIRTUAL_MACHINE) {
		int cpu_info[4];
		__cpuid(cpu_info, 1);
		if ((cpu_info[2] >> 31) & 1) {
			// hypervisor found
			bool is_found = true;
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
					is_found = false;
			}
#endif
			if (is_found) {
				LoaderMessage(mtVirtualMachineFound);
				return LOADER_ERROR;
			}
		}
		else {
#ifdef __APPLE__
			// FIXME
#elif defined(__unix__)
			bool is_found = false;
			char mode[2];
			mode[0] = 'r'; mode[1] = 0;
			char str[39];
			str[0] = '/'; str[1] = 's'; str[2] = 'y'; str[3] = 's'; str[4] = '/'; str[5] = 'd'; str[6] = 'e'; str[7] = 'v'; str[8] = 'i'; str[9] = 'c'; str[10] = 'e';
			str[11] = 's'; str[12] = '/'; str[13] = 'v'; str[14] = 'i'; str[15] = 'r'; str[16] = 't'; str[17] = 'u'; str[18] = 'a'; str[19] = 'l'; str[20] = '/';
			str[21] = 'd'; str[22] = 'm'; str[23] = 'i'; str[24] = '/'; str[25] = 'i'; str[26] = 'd'; str[27] = '/'; str[28] = 's'; str[29] = 'y'; str[30] = 's';
			str[31] = '_'; str[32] = 'v'; str[33] = 'e'; str[34] = 'n'; str[35] = 'd'; str[36] = 'o'; str[37] = 'r'; str[38] = 0;
			FILE *fsys_vendor = fopen(str, mode);
			if (fsys_vendor) {
				char sys_vendor[256] = { 0 };
				fgets(sys_vendor, sizeof(sys_vendor), fsys_vendor);
				fclose(fsys_vendor);
				if (LoaderFindFirmwareVendor(reinterpret_cast<uint8_t *>(sys_vendor), sizeof(sys_vendor)))
					is_found = true;
			}

			if (is_found) {
				LoaderMessage(mtVirtualMachineFound);
				return LOADER_ERROR;
			}
#else
			uint8_t mem_val;
			uint64_t val;
			__try {
				// set T flag
				__writeeflags(__readeflags() | 0x100);
				val = __rdtsc();
				__nop();
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
			__except (mem_val = *static_cast<uint8_t *>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
				if (mem_val != 0x90) {
					LoaderMessage(mtVirtualMachineFound);
					return LOADER_ERROR;
				}
			}

			__try {
				// set T flag
				__writeeflags(__readeflags() | 0x100);
				__cpuid(cpu_info, 1);
				__nop();
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
			__except (mem_val = *static_cast<uint8_t *>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
				if (mem_val != 0x90) {
					LoaderMessage(mtVirtualMachineFound);
					return LOADER_ERROR;
				}
			}

#ifdef WIN_DRIVER
			bool is_found = false;
			ULONG table_size = 0x1000;
			SYSTEM_FIRMWARE_TABLE_INFORMATION *table = static_cast<SYSTEM_FIRMWARE_TABLE_INFORMATION *>(LoaderAlloc(table_size));
			table->Action = SystemFirmwareTable_Get;
			table->ProviderSignature = 'FIRM';
			table->TableID = 0xc0000;
			table->TableBufferLength = table_size;
			NTSTATUS status = NtQuerySystemInformation(SystemFirmwareTableInformation, table, table_size, &table_size);
			if (status == STATUS_BUFFER_TOO_SMALL) {
				LoaderFree(table);

				table = static_cast<SYSTEM_FIRMWARE_TABLE_INFORMATION *>(LoaderAlloc(table_size));
				table->Action = SystemFirmwareTable_Get;
				table->ProviderSignature = 'FIRM';
				table->TableID = 0xc0000;
				table->TableBufferLength = table_size;
			}
			status = NtQuerySystemInformation(SystemFirmwareTableInformation, table, table_size, &table_size);
			if (NT_SUCCESS(status)) {
				if (LoaderFindFirmwareVendor(reinterpret_cast<uint8_t *>(table), table_size))
					is_found = true;
			}
			LoaderFree(table);
			if (is_found) {
				LoaderMessage(mtVirtualMachineFound);
				return LOADER_ERROR;
			}
#else
			bool is_found = false;
			typedef UINT(WINAPI tEnumSystemFirmwareTables)(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize);
			typedef UINT(WINAPI tGetSystemFirmwareTable)(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize);
			tEnumSystemFirmwareTables *enum_system_firmware_tables = reinterpret_cast<tEnumSystemFirmwareTables *>(LoaderGetProcAddress(kernel32, reinterpret_cast<const char *>(FACE_ENUM_SYSTEM_FIRMWARE_NAME), true));
			tGetSystemFirmwareTable *get_system_firmware_table = reinterpret_cast<tGetSystemFirmwareTable *>(LoaderGetProcAddress(kernel32, reinterpret_cast<const char *>(FACE_GET_SYSTEM_FIRMWARE_NAME), true));
			if (enum_system_firmware_tables && get_system_firmware_table) {
				UINT tables_size = enum_system_firmware_tables('FIRM', NULL, 0);
				if (tables_size) {
					DWORD *tables = static_cast<DWORD *>(LoaderAlloc(tables_size));
					enum_system_firmware_tables('FIRM', tables, tables_size);
					for (size_t i = 0; i < tables_size / sizeof(DWORD); i++) {
						UINT data_size = get_system_firmware_table('FIRM', tables[i], NULL, 0);
						if (data_size) {
							uint8_t *data = static_cast<uint8_t *>(LoaderAlloc(data_size));
							get_system_firmware_table('FIRM', tables[i], data, data_size);
							if (LoaderFindFirmwareVendor(data, data_size))
								is_found = true;
							LoaderFree(data);
						}
					}
					LoaderFree(tables);
				}
			}
			else {
				tNtOpenSection *open_section = reinterpret_cast<tNtOpenSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_OPEN_SECTION_NAME), true));
				tNtMapViewOfSection *map_view_of_section = reinterpret_cast<tNtMapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_MAP_VIEW_OF_SECTION), true));
				tNtUnmapViewOfSection *unmap_view_of_section = reinterpret_cast<tNtUnmapViewOfSection *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_UNMAP_VIEW_OF_SECTION), true));
				tNtClose *close = reinterpret_cast<tNtClose *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_CLOSE), true));

				if (open_section && map_view_of_section && unmap_view_of_section && close) {
					HANDLE physical_memory = NULL;
					UNICODE_STRING str;
					OBJECT_ATTRIBUTES attrs;

					wchar_t buf[] = { '\\','d','e','v','i','c','e','\\','p','h','y','s','i','c','a','l','m','e','m','o','r','y',0 };

					InitUnicodeString(&str, buf);
					InitializeObjectAttributes(&attrs, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
					NTSTATUS status = open_section(&physical_memory, SECTION_MAP_READ, &attrs);
					if (NT_SUCCESS(status)) {
						void *data = NULL;
						SIZE_T data_size = 0x10000;
						LARGE_INTEGER offset;
						offset.QuadPart = 0xc0000;

						status = map_view_of_section(physical_memory, process, &data, NULL, data_size, &offset, &data_size, ViewShare, 0, PAGE_READONLY);
						if (NT_SUCCESS(status)) {
							if (LoaderFindFirmwareVendor(static_cast<uint8_t *>(data), data_size))
								is_found = true;
							unmap_view_of_section(process, data);
						}
						close(physical_memory);
					}
				}
			}
			if (is_found) {
				LoaderMessage(mtVirtualMachineFound);
				return LOADER_ERROR;
			}

			if (LoaderGetModuleHandleEnc(reinterpret_cast<const char *>(FACE_SBIEDLL_NAME))) {
				LoaderMessage(mtVirtualMachineFound);
				return LOADER_ERROR;
			}
#endif
#endif
		}
	}

	// check memory CRC
	if (data.memory_crc_info_size()) {
		bool is_valid_crc = true;

#ifndef DEMO
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
		if (sc_query_virtual_memory) {
			MEMORY_BASIC_INFORMATION memory_info;

			// check tmp_loader_data
			NTSTATUS status = reinterpret_cast<tNtQueryVirtualMemory *>(syscall | sc_query_virtual_memory)(process, tmp_loader_data, MemoryBasicInformation, &memory_info, sizeof(memory_info), NULL);
			if (NT_SUCCESS(status) && memory_info.AllocationBase == image_base)
				is_valid_crc = false;

			// check memory after image
			IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image_base);
			IMAGE_NT_HEADERS *pe_header = reinterpret_cast<IMAGE_NT_HEADERS *>(image_base + dos_header->e_lfanew);
			status = reinterpret_cast<tNtQueryVirtualMemory *>(syscall | sc_query_virtual_memory)(process, image_base + pe_header->OptionalHeader.SizeOfImage, MemoryBasicInformation, &memory_info, sizeof(memory_info), NULL);
			if (NT_SUCCESS(status) && memory_info.AllocationBase == image_base)
				is_valid_crc = false;
		} else {
			tNtQueryVirtualMemory *query_virtual_memory = reinterpret_cast<tNtQueryVirtualMemory *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_QUERY_VIRTUAL_MEMORY_NAME), true));
			if (query_virtual_memory) {
				MEMORY_BASIC_INFORMATION memory_info;

				// check tmp_loader_data
				NTSTATUS status = query_virtual_memory(process, tmp_loader_data, MemoryBasicInformation, &memory_info, sizeof(memory_info), NULL);
				if (NT_SUCCESS(status) && memory_info.AllocationBase == image_base)
					is_valid_crc = false;

				// check memory after image
				IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image_base);
				IMAGE_NT_HEADERS *pe_header = reinterpret_cast<IMAGE_NT_HEADERS *>(image_base + dos_header->e_lfanew);
				status = query_virtual_memory(process, image_base + pe_header->OptionalHeader.SizeOfImage, MemoryBasicInformation, &memory_info, sizeof(memory_info), NULL);
				if (NT_SUCCESS(status) && memory_info.AllocationBase == image_base)
					is_valid_crc = false;
			}
		}
#endif
#endif

		if (data.memory_crc_info_hash() != CalcCRC(image_base + data.memory_crc_info(), data.memory_crc_info_size()))
			is_valid_crc = false;
		CRCValueCryptor crc_cryptor;
		for (i = 0; i < data.memory_crc_info_size(); i += sizeof(CRC_INFO)) {
			CRC_INFO crc_info = *reinterpret_cast<CRC_INFO *>(image_base + data.memory_crc_info() + i);
			crc_info.Address = crc_cryptor.Decrypt(crc_info.Address);
			crc_info.Size = crc_cryptor.Decrypt(crc_info.Size);
			crc_info.Hash = crc_cryptor.Decrypt(crc_info.Hash);

			if (crc_info.Address + crc_info.Size > crc_image_size)
				crc_image_size = crc_info.Address + crc_info.Size;
		
			if (crc_info.Hash != CalcCRC(image_base + crc_info.Address, crc_info.Size))
				is_valid_crc = false;
		}
		if (!is_valid_crc) {
			if (data.options() & LOADER_OPTION_CHECK_PATCH) {
				LoaderMessage(mtFileCorrupted);
				return LOADER_ERROR;
			}
			tmp_loader_data->set_is_patch_detected(true);
		}
	}

	// calc CPU hash
	int cpu_info[4];
	size_t cpu_count = 0;
#ifdef VMP_GNU
#else
#ifdef WIN_DRIVER
	KAFFINITY system_mask = KeQueryActiveProcessors();
	KAFFINITY mask = 1;
#else
	DWORD_PTR process_mask, system_mask;
	if (GetProcessAffinityMask(process, &process_mask, &system_mask)) {
		if (process_mask != system_mask) {
			if (!SetProcessAffinityMask(process, system_mask)) {
				LoaderMessage(mtInitializationError, CPU_HASH_ERROR);
				return LOADER_ERROR;
			}
		}
		DWORD_PTR mask = 1;
#endif
		for (size_t i = 0; i < sizeof(mask) * 8; i++) {
			if (system_mask & mask) {
#ifdef WIN_DRIVER
				KeSetSystemAffinityThread(mask);
#else
				DWORD_PTR old_mask = SetThreadAffinityMask(thread, mask);
				Sleep(0);
#endif
				__cpuid(cpu_info, 1);
				if ((cpu_info[0] & 0xff0) == 0xfe0) 
					cpu_info[0] ^= 0x20; // fix Athlon bug
				cpu_info[1] &= 0x00ffffff; // mask out APIC Physical ID
				size_t cpu_hash  = (cpu_info[0] + cpu_info[1] + cpu_salt) ^ reinterpret_cast<size_t>(tmp_loader_data);
				bool cpu_found = false;
				for (j = 0; j < cpu_count; j++) {
					if (tmp_loader_data->cpu_hash(j) == cpu_hash) {
						cpu_found = true;
						break;
					}
				}
				if (!cpu_found) {
					if (cpu_count == VAR_COUNT - VAR_CPU_HASH) {
						LoaderMessage(mtInitializationError, CPU_HASH_ERROR);
						return LOADER_ERROR;
					}
					tmp_loader_data->set_cpu_hash(cpu_count++, cpu_hash);
				}
#ifdef WIN_DRIVER
				KeRevertToUserAffinityThread();
#else
				SetThreadAffinityMask(thread, old_mask);
#endif
			}
			mask <<= 1;
		}
#ifndef WIN_DRIVER
		if (process_mask != system_mask)
			SetProcessAffinityMask(process, process_mask);
	}
#endif
#endif
	if (!cpu_count)	{
		__cpuid(cpu_info, 1);
		if ((cpu_info[0] & 0xff0) == 0xfe0) 
			cpu_info[0] ^= 0x20; // fix Athlon bug
		cpu_info[1] &= 0x00ffffff; // mask out APIC Physical ID
		tmp_loader_data->set_cpu_hash(cpu_count++, (cpu_info[0] + cpu_info[1] + cpu_salt) ^ reinterpret_cast<size_t>(tmp_loader_data));
	}
	tmp_loader_data->set_cpu_count(cpu_count);

	// create session key
	session_key = __rdtsc();
	session_key ^= session_key >> 32;
	tmp_loader_data->set_session_key(static_cast<uint32_t>(session_key));

	// save pointer to loader_data
	*(reinterpret_cast<void**>(dst_image_base + data.storage())) = tmp_loader_data;

	if (uint32_t data_runtime_entry = data.runtime_entry()) {
#ifdef VMP_GNU
		typedef bool (WINAPI tRuntimeEntry)(HMODULE hModule, bool is_init);
		if (reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), true) != true)
			return LOADER_ERROR;
#elif defined(WIN_DRIVER)
		typedef NTSTATUS (tRuntimeEntry)(HMODULE hModule, bool is_init);
		if (reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), true) != STATUS_SUCCESS)
			return LOADER_ERROR;
#else
		typedef BOOL (WINAPI tRuntimeEntry)(HMODULE hModule, DWORD dwReason, LPVOID lpReserved);
		if (reinterpret_cast<tRuntimeEntry *>(image_base + data_runtime_entry)(reinterpret_cast<HMODULE>(image_base), DLL_PROCESS_ATTACH, NULL) != TRUE)
			return LOADER_ERROR;
#endif
	}

	// setup delay import
#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
	for (i = 0; i < data.delay_import_info_size(); i += sizeof(DLL_INFO)) {
		DLL_INFO *dll_info = reinterpret_cast<DLL_INFO *>(image_base + data.delay_import_info() + i);
		const char *dll_name = reinterpret_cast<const char *>(image_base + dll_info->Name);
		HMODULE h = LoaderLoadLibraryEnc(dll_name);

		for (IMPORT_INFO *import_info = reinterpret_cast<IMPORT_INFO *>(dll_info + 1); import_info->Name != 0; import_info++, i += sizeof(IMPORT_INFO)) {
			const char *api_name;
			if (import_info->Name & IMAGE_ORDINAL_FLAG32) {
				api_name = LPCSTR(INT_PTR(IMAGE_ORDINAL32(import_info->Name)));
			} else {
				api_name = LPCSTR(INT_PTR(image_base + import_info->Name));
			}

			void *p = LoaderGetProcAddress(h, api_name, true);
			if (!p) {
#ifndef WIN_DRIVER
				ULONG error_mode = 0;
				if (tNtQueryInformationProcess *query_information_process = reinterpret_cast<tNtQueryInformationProcess *>(LoaderGetProcAddress(ntdll, reinterpret_cast<const char *>(FACE_NT_QUERY_INFORMATION_PROCESS_NAME), true))) {
					query_information_process(process, ProcessDefaultHardErrorMode, &error_mode, sizeof(error_mode), NULL);
				}
				if (error_mode & SEM_FAILCRITICALERRORS)
					return LOADER_ERROR;
#endif
				LoaderMessage((import_info->Name & IMAGE_ORDINAL_FLAG32) ? mtOrdinalNotFound : mtProcNotFound, api_name, dll_name);
				return LOADER_ERROR;
			}

			*(reinterpret_cast<void **>(dst_image_base + import_info->Address)) = reinterpret_cast<uint8_t *>(p) - import_info->Key; 
		}

		i += sizeof(uint32_t);
	}
#endif

	// reset WRITABLE flag for memory pages
#ifdef VMP_GNU
	for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
		SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

		int protect = section_info->Type;
		if (mprotect(image_base + section_info->Address, section_info->Size, protect) != 0) {
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
	}
#elif defined(WIN_DRIVER)
	if (mdl) {
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
		dst_image_base = image_base;
	}
#else
	if (sc_virtual_protect) {
		for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
			SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

			DWORD protect, old_protect;
			if (section_info->Type & IMAGE_SCN_MEM_READ)
				protect = (section_info->Type & IMAGE_SCN_MEM_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
			else
				protect = PAGE_NOACCESS;
			if (section_info->Type & IMAGE_SCN_MEM_EXECUTE)
				protect <<= 4; // convert PAGE_XXX to PAGE_EXECUTE_XXX
			void *address = image_base + section_info->Address;
			SIZE_T size = section_info->Size;
			if (!NT_SUCCESS(reinterpret_cast<tNtProtectVirtualMemory *>(syscall | sc_virtual_protect)(process, &address, &size, protect, &old_protect))) {
				LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
				return LOADER_ERROR;
			}
			if (old_protect & (PAGE_NOACCESS | PAGE_GUARD)) {
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
		}
	} else {
		// check breakpoint
		if (*reinterpret_cast<uint8_t*>(virtual_protect) == 0xcc) {
			if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
				LoaderMessage(mtDebuggerFound);
				return LOADER_ERROR;
			}
			tmp_loader_data->set_is_debugger_detected(true);
		}

		for (i = 0; i < data_section_info_size; i += sizeof(SECTION_INFO)) {
			SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data_section_info + i);

			DWORD protect, old_protect;
			if (section_info->Type & IMAGE_SCN_MEM_READ)
				protect = (section_info->Type & IMAGE_SCN_MEM_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
			else
				protect = PAGE_NOACCESS;
			if (section_info->Type & IMAGE_SCN_MEM_EXECUTE)
				protect <<= 4; // convert PAGE_XXX to PAGE_EXECUTE_XXX
			void *address = image_base + section_info->Address;
			SIZE_T size = section_info->Size;
			if (!NT_SUCCESS(virtual_protect(process, &address, &size, protect, &old_protect))) {
				LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
				return LOADER_ERROR;
			}
			if (old_protect & (PAGE_NOACCESS | PAGE_GUARD)) {
				if (data.options() & LOADER_OPTION_CHECK_DEBUGGER) {
					LoaderMessage(mtDebuggerFound);
					return LOADER_ERROR;
				}
				tmp_loader_data->set_is_debugger_detected(true);
			}
		}
	}
	tmp_loader_data->set_crc_image_size(crc_image_size);
#endif

#ifdef __unix__
	if (data.relro_info()) {
		SECTION_INFO *section_info = reinterpret_cast<SECTION_INFO *>(image_base + data.relro_info());

		int protect = section_info->Type;
		if (mprotect(image_base + section_info->Address, section_info->Size, protect) != 0) {
			LoaderMessage(mtInitializationError, VIRTUAL_PROTECT_ERROR);
			return LOADER_ERROR;
		}
	}
#endif

	// show nag
	LoaderMessage(mtUnregisteredVersion);

	tmp_loader_data->set_loader_status(LOADER_SUCCESS);
	return LOADER_SUCCESS;
}