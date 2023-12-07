#include "sdk.h"

#if defined(__unix__)
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
void DriverUnload(PDRIVER_OBJECT driver_object)
{

}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{ 
	pRegistryPath;
	pDriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}
#else
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	hModule;
	dwReason;
	lpReserved;
	return TRUE;
}
#endif

bool VMP_API VMProtectIsProtected()
{
	return false;
}

void VMP_API VMProtectBegin(const char *) 
{

}

void VMP_API VMProtectBeginMutation(const char *)
{

}

void VMP_API VMProtectBeginVirtualization(const char *) 
{

}

void VMP_API VMProtectBeginUltra(const char *) 
{ 

}

void VMP_API VMProtectBeginVirtualizationLockByKey(const char *) 
{ 

}

void VMP_API VMProtectBeginUltraLockByKey(const char *) 
{ 

}

void VMP_API VMProtectEnd() 
{

}

bool VMP_API VMProtectIsDebuggerPresent(bool) 
{
#ifdef VMP_GNU
	return false;
#elif defined(WIN_DRIVER)
	return false;
#else
	return IsDebuggerPresent() != FALSE; 
#endif
}

bool VMP_API VMProtectIsVirtualMachinePresent() 
{ 
	return false; 
}

bool VMP_API VMProtectIsValidImageCRC()
{ 
	return true; 
}

const char * VMP_API VMProtectDecryptStringA(const char *value) 
{ 
	return value; 
}

const VMP_WCHAR * VMP_API VMProtectDecryptStringW(const VMP_WCHAR *value) 
{ 
	return value; 
}

bool VMP_API VMProtectFreeString(void *) 
{ 
	return true; 
}

int VMP_API VMProtectGetOfflineActivationString(const char *, char *, int) 
{
	return ACTIVATION_OK;
}

int VMP_API VMProtectGetOfflineDeactivationString(const char *, char *, int) 
{
	return ACTIVATION_OK;
}

#ifdef __APPLE__
unsigned long GetTickCount()
{
	const int64_t one_million = 1000 * 1000;
	mach_timebase_info_data_t timebase_info;
	mach_timebase_info(&timebase_info);

	// mach_absolute_time() returns billionth of seconds,
	// so divide by one million to get milliseconds
	return static_cast<uint32_t>((mach_absolute_time() * timebase_info.numer) / (one_million * timebase_info.denom));
}
#endif

#ifdef __unix__
unsigned long GetTickCount()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

#ifndef WIN_DRIVER
bool g_serial_is_correct = false;
bool g_serial_is_blacklisted = false;
uint32_t g_time_of_start = GetTickCount();
#endif

#ifdef VMP_GNU

#define strcmpi strcasecmp
#define INI_MAX_LINE 1024

size_t strnlen(char *text, size_t maxlen)
{
	const char *last = (const char *)memchr(text, '\0', maxlen);
	return last ? (size_t) (last - text) : maxlen;
}

/* Strip whitespace chars off end of given string, in place. Return s. */
static char* rstrip(char* s)
{
	char* p = s + strlen(s);
	while (p > s && isspace((unsigned char)(*--p)))
		*p = '\0';
	return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char* lskip(const char* s)
{
	while (*s && isspace((unsigned char)(*s)))
		s++;
	return (char*)s;
}

/* Return pointer to first char c or ';' comment in given string, or pointer to
   null at end of string if neither found. ';' must be prefixed by a whitespace
   character to register as a comment. */
static char* find_char_or_comment(const char* s, char c)
{
	int was_whitespace = 0;
	while (*s && *s != c && !(was_whitespace && *s == ';')) {
		was_whitespace = isspace((unsigned char)(*s));
		s++;
	}
	return (char*)s;
}

/* See documentation in header file. */
static int GetPrivateProfileString(const char *section_name, const char *key_name, char *buffer, size_t size, const char *file_name)
{
	if (!buffer || !size)
		return 0;

    FILE* file = fopen(file_name, "r");
    if (!file)
        return 0;

    char line[INI_MAX_LINE];
    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
	int res = 0;
	bool section_found = false;

    /* Scan through file line by line */
    while (fgets(line, INI_MAX_LINE, file) != NULL) {
        lineno++;

        start = line;
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
            /* Per Python ConfigParser, allow '#' comments at start of line */
        } else if (*start == '[') {
            /* A "[section]" line */
            end = find_char_or_comment(start + 1, ']');
            if (*end == ']') {
                *end = '\0';
				if (section_found)
					break;

				section_found = strcmpi(start + 1, section_name) == 0;
            }
        } else if (section_found && *start && *start != ';') {
            /* Not a comment, must be a name[=:]value pair */
            end = find_char_or_comment(start, '=');
            if (*end != '=') {
                end = find_char_or_comment(start, ':');
            }
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
                end = find_char_or_comment(value, '\0');
                if (*end == ';')
                    *end = '\0';
                rstrip(value);

				if (strcmpi(name, key_name) == 0) {
					strncpy(buffer, value, size);
					res = strnlen(buffer, size);
					break;
				}
            }
        }
    }

	fclose(file);
	return res;
}

static bool GetIniValue(const char *value_name, char *buffer, size_t size)
{
	char file_name[PATH_MAX];
	file_name[0] = 0;
	uint32_t name_size = sizeof(file_name);
#if defined(__APPLE__)
	_NSGetExecutablePath(file_name, &name_size);
#else
	int sz = readlink("/proc/self/exe", file_name, name_size);
	if (sz > 0)
		file_name[sz] = 0;
#endif
	char *p = strrchr(file_name, '/');
	if (p)
		*(p + 1) = 0;
	strncat(file_name, "VMProtectLicense.ini", sizeof(file_name) - (p - file_name));
	return GetPrivateProfileString("TestLicense", value_name, buffer, size, file_name) != 0;
}

static const uint8_t utf8_limits[5] = {0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

static void ConvertUTF8ToUnicode(const uint8_t *src, size_t len, VMP_WCHAR *dest, size_t dest_size)
{
	if (!dest || dest_size == 0) return; // nothing to do

	size_t pos = 0;
	size_t dest_pos = 0;
	while (pos < len && dest_pos < dest_size) {
		uint8_t b = src[pos++];

		if (b < 0x80) {
			dest[dest_pos++] = b;
			continue;
		}

		size_t val_len;
		for (val_len = 0; val_len <= _countof(utf8_limits); val_len++) {
			if (b < utf8_limits[val_len])
				break;
		}

		if (val_len == 0)
			continue;

		uint32_t value = b - utf8_limits[val_len - 1];
		for (size_t i = 0; i < val_len; i++) {
			if (pos == len)
				break;
			b = src[pos++];
			if (b < 0x80 || b >= 0xC0)
				break;
			value <<= 6;
			value |= (b - 0x80);
		}

		if (value < 0x10000) {
			dest[dest_pos++] = static_cast<uint16_t>(value);
		} else if (value <= 0x10FFFF) {
			value -= 0x10000;
			dest[dest_pos++] = static_cast<uint16_t>(0xD800 + (value >> 10));
			dest[dest_pos++] = static_cast<uint16_t>(0xDC00 + (value & 0x3FF));
		}
	}
	
	if (dest_pos < dest_size - 1)
		dest[dest_pos] = 0;
	else
		dest[dest_pos - 1] = 0;
}

static bool GetIniValue(const char *value_name, VMP_WCHAR *buffer, size_t size)
{
	char value[INI_MAX_LINE];
	if (GetIniValue(value_name, value, sizeof(value))) {
		ConvertUTF8ToUnicode(reinterpret_cast<uint8_t *>(value), strlen(value), buffer, size);
		return true;
	}
	if (buffer && size)
		buffer[0] = 0;
	return false;
}

#elif defined(WIN_DRIVER)
#else

static bool GetIniValue(const char *value_name, wchar_t *buffer, size_t size)
{
	wchar_t file_name[MAX_PATH];
	file_name[0] = 0;
	GetModuleFileNameW(NULL, file_name, _countof(file_name));
	wchar_t *p = wcsrchr(file_name, L'\\');
	if (p)
		*(p + 1) = 0;
	wcsncat_s(file_name, L"VMProtectLicense.ini", _countof(file_name));

	wchar_t key_name[1024] = {0};
	MultiByteToWideChar(CP_ACP, 0, value_name, -1, key_name, _countof(key_name));
	return GetPrivateProfileStringW(L"TestLicense", key_name, L"", buffer, static_cast<DWORD>(size), file_name) != 0;
}

static bool GetIniValue(const char *value_name, char *buffer, size_t size)
{
	wchar_t value[2048];
	if (GetIniValue(value_name, value, sizeof(value))) {
		WideCharToMultiByte(CP_ACP, 0, value, -1, buffer, static_cast<int>(size), NULL, NULL);
		return true;
	}
	if (buffer && size)
		buffer[0] = 0;
	return false;
}

#endif

#define MAKEDATE(y, m, d) (DWORD)((y << 16) + (m << 8) + d)

int VMP_API VMProtectGetSerialNumberState()
{
#ifdef WIN_DRIVER
	return SERIAL_STATE_FLAG_INVALID;
#else
	if (!g_serial_is_correct) 
		return SERIAL_STATE_FLAG_INVALID;
	if (g_serial_is_blacklisted) 
		return SERIAL_STATE_FLAG_BLACKLISTED;

	int res = 0;

	char buf[256];
	if (GetIniValue("TimeLimit", buf, sizeof(buf))) {
		int running_time = atoi(buf);
		if (running_time >= 0 && running_time <= 255) {
			uint32_t dw = GetTickCount();
			int d = (dw - g_time_of_start) / 1000 / 60; // minutes
			if (running_time <= d) 
				res |= SERIAL_STATE_FLAG_RUNNING_TIME_OVER;
		}
	}

	if (GetIniValue("ExpDate", buf, sizeof(buf))) {
		int y, m, d;
		if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
			uint32_t ini_date = (y << 16) + (static_cast<uint8_t>(m) << 8) + static_cast<uint8_t>(d);
			uint32_t cur_date;
#ifdef VMP_GNU
			time_t rawtime;
			time(&rawtime);
			struct tm local_tm;
			tm *timeinfo = localtime_r(&rawtime, &local_tm);
			cur_date = ((timeinfo->tm_year + 1900) << 16) + (static_cast<uint8_t>(timeinfo->tm_mon + 1) << 8) + static_cast<uint8_t>(timeinfo->tm_mday);
#else
			SYSTEMTIME st;
			GetLocalTime(&st);
			cur_date = (st.wYear << 16) + (static_cast<uint8_t>(st.wMonth) << 8) + static_cast<uint8_t>(st.wDay);
#endif
			if (cur_date > ini_date)
				res |= SERIAL_STATE_FLAG_DATE_EXPIRED;
		}
	}

	if (GetIniValue("MaxBuildDate", buf, sizeof(buf))) {
		int y, m, d;
		if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
			uint32_t ini_date = (y << 16) + (static_cast<uint8_t>(m) << 8) + static_cast<uint8_t>(d);
			uint32_t cur_date;
#ifdef VMP_GNU
			time_t rawtime;
			time(&rawtime);
			struct tm local_tm;
			tm *timeinfo = localtime_r(&rawtime, &local_tm);
			cur_date = ((timeinfo->tm_year + 1900) << 16) + (static_cast<uint8_t>(timeinfo->tm_mon + 1) << 8) + static_cast<uint8_t>(timeinfo->tm_mday);
#else
			SYSTEMTIME st;
			GetLocalTime(&st);
			cur_date = (st.wYear << 16) + (static_cast<uint8_t>(st.wMonth) << 8) + static_cast<uint8_t>(st.wDay);
#endif
			if (cur_date > ini_date)
				res |= SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED;
		}
	}

	if (GetIniValue("KeyHWID", buf, sizeof(buf))) {
		char buf2[256];
		GetIniValue("MyHWID", buf2, sizeof(buf2));
		if (strcmp(buf, buf2) != 0) 
			res |= SERIAL_STATE_FLAG_BAD_HWID;
	}

	return res;
#endif
}

int VMP_API VMProtectSetSerialNumber(const char *serial)
{
#ifdef WIN_DRIVER
	serial;
	return SERIAL_STATE_FLAG_INVALID;
#else
	g_serial_is_correct = false;
	g_serial_is_blacklisted = false;
	if (!serial || !serial[0]) 
		return SERIAL_STATE_FLAG_INVALID;

	char buf_serial[2048];
	const char *src = serial;
	char *dst = buf_serial;
	while (*src) {
		char c = *src;
		// check agains base64 alphabet
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')
			*dst++ = c;
		src++;
	}
	*dst = 0;

	char ini_serial[2048];
	if (!GetIniValue("AcceptedSerialNumber", ini_serial, sizeof(ini_serial)))
		strcpy_s(ini_serial, "serialnumber");
	g_serial_is_correct = strcmp(buf_serial, ini_serial) == 0;

	if (GetIniValue("BlackListedSerialNumber", ini_serial, sizeof(ini_serial)))
		g_serial_is_blacklisted = strcmp(buf_serial, ini_serial) == 0;

	return VMProtectGetSerialNumberState();
#endif
}

bool VMP_API VMProtectGetSerialNumberData(VMProtectSerialNumberData *data, int size)
{
#ifdef WIN_DRIVER
	data; 
	size;
	return false;
#else
	if (size != sizeof(VMProtectSerialNumberData)) 
		return false;
	memset(data, 0, sizeof(VMProtectSerialNumberData));

	data->nState = VMProtectGetSerialNumberState();
	if (data->nState & (SERIAL_STATE_FLAG_INVALID | SERIAL_STATE_FLAG_BLACKLISTED))
		return true; // do not need to read the rest

	GetIniValue("UserName", data->wUserName, _countof(data->wUserName));
	GetIniValue("EMail", data->wEMail, _countof(data->wEMail));

	char buf[2048];
	if (GetIniValue("TimeLimit", buf, sizeof(buf))) {
		int running_time = atoi(buf);
		if (running_time < 0) 
			running_time = 0;
		else
		if (running_time > 255) 
			running_time = 255;
		data->bRunningTime = static_cast<unsigned char>(running_time);
	}

	if (GetIniValue("ExpDate", buf, sizeof(buf))) {
		int y, m, d;
		if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
			data->dtExpire.wYear = static_cast<unsigned short>(y);
			data->dtExpire.bMonth = static_cast<unsigned char>(m);
			data->dtExpire.bDay = static_cast<unsigned char>(d);
		}
	}

	if (GetIniValue("MaxBuildDate", buf, sizeof(buf))) {
		int y, m, d;
		if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
			data->dtMaxBuild.wYear = static_cast<unsigned short>(y);
			data->dtMaxBuild.bMonth = static_cast<unsigned char>(m);
			data->dtMaxBuild.bDay = static_cast<unsigned char>(d);
		}
	}

	if (GetIniValue("UserData", buf, sizeof(buf))) {
		size_t len = strlen(buf);
		if (len > 0 && len % 2 == 0 && len <= 255 * 2) // otherwise UserData is empty or has bad length
		{
			for (size_t src = 0, dst = 0; src < len; src++) {
				int v = 0;
				char c = buf[src];

				if (c >= '0' && c <= '9') v = c - '0';
				else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
				else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
				else {
					data->nUserDataLength = 0;
					memset(data->bUserData, 0, sizeof(data->bUserData));
					break;
				}

				if (src % 2 == 0) {
					data->bUserData[dst] = static_cast<unsigned char>(v << 4);
				} else {
					data->bUserData[dst] |= v;
					dst++;
					data->nUserDataLength = static_cast<unsigned char>(dst);
				}
			}
		}
	}

	return true;
#endif
}

int VMP_API VMProtectGetCurrentHWID(char *hwid, int size)
{
#ifdef WIN_DRIVER
	hwid;
	size;
	return 0;
#else
	if (hwid && size == 0) 
		return 0;

	char buf[1024];
	if (!GetIniValue("MyHWID", buf, sizeof(buf)))
		strcpy_s(buf, "myhwid");

	int res = static_cast<int>(strlen(buf));
	if (hwid) {
		if (size - 1 < res)
			res = size - 1;
		memcpy(hwid, buf, res);
		hwid[res] = 0;
	}
	return res + 1;
#endif
}

int VMP_API VMProtectActivateLicense(const char *code, char *serial, int size) 
{ 
#ifdef WIN_DRIVER
	code;
	serial;
	size;
	return ACTIVATION_NOT_AVAILABLE;
#else
	if (!code)
		return ACTIVATION_BAD_CODE;

	char buf[2048];
	if (!GetIniValue("AcceptedActivationCode", buf, sizeof(buf)))
		strcpy_s(buf, "activationcode");
	if (strcmp(code, buf) != 0)
		return ACTIVATION_BAD_CODE;

	if (!GetIniValue("AcceptedSerialNumber", buf, sizeof(buf)))
		strcpy_s(buf, "serialnumber");

	int need = static_cast<int>(strlen(buf));
	if (need > size - 1) 
		return ACTIVATION_SMALL_BUFFER;

	strncpy_s(serial, size, buf, _TRUNCATE);
	return ACTIVATION_OK;
#endif
}

int VMP_API VMProtectDeactivateLicense(const char *) 
{ 
#ifdef WIN_DRIVER
	return ACTIVATION_NOT_AVAILABLE;
#else
	return ACTIVATION_OK;
#endif
}
