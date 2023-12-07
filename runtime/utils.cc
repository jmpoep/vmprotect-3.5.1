#include "common.h"
#include "utils.h"
#include "loader.h"

#ifdef __unix__
#ifdef __i386__
__asm__(".symver memcpy,memcpy@GLIBC_2.0");
__asm__(".symver clock_gettime,clock_gettime@LIBRT_2.12");
#else
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
__asm__(".symver clock_gettime,clock_gettime@LIBRT_2.12");
#endif
extern "C"
{
    void *__wrap_memcpy(void *dest, const void *src, size_t n)
    {
        return memcpy(dest, src, n);
    }

    #include <sys/poll.h>
    int __wrap___poll_chk (struct pollfd *fds, nfds_t nfds, int timeout, __SIZE_TYPE__ fdslen)
    {
      if (fdslen / sizeof (*fds) < nfds)
        abort();

      return poll (fds, nfds, timeout);
    }

    #include <sys/select.h>


    long int __wrap___fdelt_chk (long int d)
    {
      if (d < 0 || d >= FD_SETSIZE)
        abort();
      return d / __NFDBITS;
    }
}

extern "C" size_t __fread_chk (void *destv, size_t dest_size, size_t size, size_t nmemb, FILE *f)
{
   if (size > dest_size) {
        //_chk_fail(__FUNCTION__);
        size = dest_size;
   }
   return fread(destv, size, nmemb, f);
}

extern "C" int __isoc99_sscanf(const char *a, const char *b, va_list args)
{
   int i;
   va_list ap;
   va_copy(ap, args);
   i = sscanf(a, b, ap);
   va_end(ap);
   return i;
}

#include <setjmp.h>
extern "C" void __longjmp_chk (jmp_buf env, int val)
{
    // TODO: Glibc's __longjmp_chk additionally does some sanity checks about
    // whether we're jumping a sane stack frame.
    longjmp(env, val);
}

#endif

void ShowMessage(const VMP_CHAR *message)
{
#ifdef __APPLE__
	char file_name[PATH_MAX];
	uint32_t name_size = sizeof(file_name);
	const char *title;
	if (_NSGetExecutablePath(file_name, &name_size) == 0) {
		char *p = strrchr(file_name, '/');
		title = p ? p + 1 : file_name;
	} else {
		title = "Fatal Error";
	}

	CFStringRef title_ref = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
	CFStringRef message_ref = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	CFOptionFlags result;

	CFUserNotificationDisplayAlert(
									0, // no timeout
									kCFUserNotificationStopAlertLevel, //change it depending message_type flags ( MB_ICONASTERISK.... etc.)
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
	char file_name[PATH_MAX] = {0};
	const char *title = file_name;
	ssize_t len = ::readlink("/proc/self/exe", file_name, sizeof(file_name)-1);
	if (len != -1) {
		for (ssize_t i = 0; i < len; i++) {
			if (file_name[i] == '/')
				title = file_name + i + 1;
			if (file_name[i] == '\'' || file_name[i] == '\\')
				file_name[i] = '"';
		}
		file_name[len] = '\0';
	} else {
		title = "Fatal Error";
	}
	std::string cmd_line = "zenity";
	cmd_line += " --error --no-markup --text='";
	cmd_line += title;
	cmd_line += ": ";
	char *message_buffer = strdup(message);
	char *msg_ptr = message_buffer;
	while (*msg_ptr)
	{
		if (*msg_ptr == '\'' || *msg_ptr == '\\')
			*msg_ptr = '"';
		msg_ptr++;
	}
	cmd_line += message_buffer;
	free(message_buffer);
	cmd_line += "'";
	int status = system(cmd_line.c_str());
	if (status == -1 || WEXITSTATUS(status) == 127)
		puts(message);
#elif defined(WIN_DRIVER)
	DbgPrint("%ws\n", message);
#else
	VMP_CHAR file_name[MAX_PATH + 1] = {0};
	const VMP_CHAR *title;
	if (GetModuleFileNameW(reinterpret_cast<HMODULE>(FACE_IMAGE_BASE), file_name, _countof(file_name) - 1)) {
		wchar_t *p = wcsrchr(file_name, L'\\');
		title = p ? p + 1 : file_name;
	} else {
		title = L"Fatal Error";
	}

	HMODULE ntdll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	typedef NTSTATUS(NTAPI tNtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters,
		ULONG UnicodeStringParameterMask, PULONG_PTR Parameters,
		ULONG ValidResponseOptions,
		HardErrorResponse *Response);
	if (tNtRaiseHardError *raise_hard_error = reinterpret_cast<tNtRaiseHardError *>(InternalGetProcAddress(ntdll, VMProtectDecryptStringA("NtRaiseHardError")))) {
		UNICODE_STRING message_str;
		UNICODE_STRING title_str;

		InitUnicodeString(&message_str, (PWSTR)message);
		InitUnicodeString(&title_str, (PWSTR)title);

		ULONG_PTR params[4] = {
			(ULONG_PTR)&message_str,
			(ULONG_PTR)&title_str,
			(
				(ULONG)ResponseButtonOK | IconError
			),
			INFINITE
		};

		HardErrorResponse response;
		raise_hard_error(STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE, 4, 3, params, 0, &response);
	}
#endif
}

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
#else
void *InternalGetProcAddress(HMODULE module, const char *proc_name)
{
	// check input
	if (!module || !proc_name)
		return NULL;
	
	// check module's header
	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	// check NT header
	PIMAGE_NT_HEADERS pe_header = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t *>(module) + dos_header->e_lfanew);
	if (pe_header->Signature != IMAGE_NT_SIGNATURE) 
		return NULL;

	// get the export directory
	uint32_t export_adress = pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress; 
	if (!export_adress) 
		return NULL;

	uint32_t export_size = pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size; 
	uint32_t address;
	uint32_t ordinal_index = -1;
	PIMAGE_EXPORT_DIRECTORY export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<uint8_t *>(module) + export_adress);

	if (proc_name <= reinterpret_cast<const char *>(0xFFFF)) { 
		// ordinal
		ordinal_index = static_cast<uint32_t>(INT_PTR(proc_name)) - export_directory->Base; //-V221
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
				switch (strcmp((const char *)(reinterpret_cast<uint8_t *>(module) + names[cur_index]), proc_name)) {
				case 0:
					ordinal_index = (reinterpret_cast<WORD *>(reinterpret_cast<uint8_t *>(module) + export_directory->AddressOfNameOrdinals))[cur_index];
					left_index = right_index + 1;
					break;
				case 1:
					right_index = cur_index - 1;
					break;
				case -1:
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

	module = GetModuleHandleA(file_name);
	if (!module) 
		return NULL;

	// now the function's name
	// if it is not an ordinal, just forward it
	if (name_dot[1] != '#') 
		return InternalGetProcAddress(module, name_dot + 1);

	// is is an ordinal
	int ordinal = atoi(name_dot + 2);
	return InternalGetProcAddress(module, LPCSTR(INT_PTR(ordinal)));
}
#endif