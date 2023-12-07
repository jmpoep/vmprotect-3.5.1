struct FileInfo {
	DWORD TimeDateStamp;
	DWORD SizeOfImage;
	DWORD CheckSum;
};

enum {
   ERROR_OPEN = 1,
   ERROR_INVALID_FORMAT,
   ERROR_MODULE_NOT_FOUND
};

int GetFileInfo(const wchar_t *file_name, FileInfo &file_info);
int FixDump(const wchar_t *file_name, const FileInfo &file_info);