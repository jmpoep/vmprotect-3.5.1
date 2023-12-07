#ifndef OSUTILS_H
#define OSUTILS_H

/**
 * Flags for opening files.
 */

enum FileMode
{
	fmOpenRead       = 0x0000,
	fmOpenWrite      = 0x0001,
	fmOpenReadWrite  = 0x0002,
	fmShareExclusive = 0x0010,
	fmShareDenyWrite = 0x0020,
	fmShareDenyRead  = 0x0030,
	fmShareDenyNone  = 0x0040,
	fmCreate         = 0x0100
};

enum SeekOrigin
{
	soBeginning,
	soCurrent,
	soEnd
};

struct PROCESS_ITEM {
	uint32_t id;
	std::string name;
};

struct MODULE_ITEM {
	HMODULE handle;
	std::string name;
};

struct MODULE_INFO {
	void *address;
	size_t size;
};

struct SYSTEM_TIME {
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

namespace os
{
unicode_string FromUTF8(const std::string &src);
std::string ToUTF8(const unicode_string &src);
#ifndef VMP_GNU
std::string ToOEM(const unicode_string &src);
unicode_string FromACP(const std::string &src);
bool ValidateUTF8(const std::string &src);
#endif
std::string ExtractFilePath(const char *name);
std::string ExtractFileName(const char *name);
std::string ExtractFileExt(const char *name);
std::string CombinePaths(const char *path, const char *file_name);
std::string SubtractPath(const char *path, const char *file_name);
std::string ChangeFileExt(const char *name, const char *ext);
std::string GetCurrentPath();
std::string GetExecutablePath();
std::string GetTempFilePathName(const char *pathname_template = NULL);
std::string GetTempFilePathNameFor(const char *pathname);
bool FileExists(const char *name);
bool FileDelete(const char *name, bool toRecycleBin = false);
bool FileCopy(const char *src, const char *dest);

HANDLE FileCreate(const char *file_name, uint32_t mode);
bool FileClose(HANDLE h);
size_t FileRead(HANDLE h, void *buf, size_t size);
size_t FileWrite(HANDLE h, const void *buf, size_t size);
uint64_t FileSeek(HANDLE h, uint64_t offset, SeekOrigin origin);
bool FileSetEnd(HANDLE h);
bool FileGetCheckSum(const char *file_name, uint32_t *check_sum);
void Print(const char *text);
std::vector<std::string> CommandLine();
std::vector<std::string> FindFiles(const char *path, const char *mask, bool only_directories = false);
uint32_t GetTickCount();
bool WriteIniString(const char *section, const char *key, const char *value, const char *file_name);
std::string ReadIniString(const char *section, const char *key, const char *default_value, const char *file_name);
HPROCESS ProcessOpen(uint32_t process_id);
bool ProcessClose(HPROCESS h);
size_t ProcessRead(HPROCESS h, void *base_address, void *buf, size_t size);
size_t ProcessWrite(HPROCESS h, void *base_address, const void *buf, size_t size);
uint64_t GetLastWriteTime(const char *name);
std::vector<PROCESS_ITEM> EnumProcesses();
std::vector<MODULE_ITEM> EnumModules(uint32_t process_id);
bool GetModuleInformation(HANDLE process, HMODULE module, MODULE_INFO *info, size_t size);
std::string GetSysAppDataDirectory();
std::string CombineThisAppDataDirectory(const char *lastPathPart);
bool PathCreate(const char *name);
std::string GetLocaleName(const char *code);
std::string GetCurrentLocale();
void GetLocalTime(SYSTEM_TIME *res);
bool FileMove(const char *oldName, const char *newName);
#ifdef __APPLE__
std::string GetMainExeFileName(const char *file_name);
#endif
HMODULE LibraryOpen(const std::string &name);
bool LibraryClose(HMODULE h);
void *GetFunction(HMODULE h, const std::string &name);
std::string ExpandEnvironmentVariables(const char *path);
std::string GetEnvironmentVariable(const char *name);
void SetEnvironmentVariable(const char *name, const char *value);

}

#endif