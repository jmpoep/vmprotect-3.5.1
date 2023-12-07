#include "minidumpfixer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	wprintf(L"MiniDump Fixer v 2.0 Copyright 2003-2019 VMProtect Software\n\n");
	if (argc < 3) {
		wprintf(L"Usage: %s dmp_file exe_file\n", PathFindFileNameW(argv[0]));
		return 1;
	}

	const wchar_t *dmp_file_name = argv[1];
	if (wcslen(dmp_file_name) == 0) {
		wprintf(L"Crash dump file does not specified.");
		return 1;
	}

	const wchar_t *exe_file_name = argv[2];
	if (wcslen(dmp_file_name) == 0) {
		wprintf(L"Executable file does not specified.");
		return 1;
	}

	FileInfo file_info;
	switch (GetFileInfo(exe_file_name, file_info)) {
	case ERROR_OPEN:
		wprintf(L"Can not open \"%s\".", exe_file_name);
		return 1;
		break;
	case ERROR_INVALID_FORMAT:
		wprintf(L"File \"%s\" has an incorrect format.", exe_file_name);
		return 1;
		break;
	}

	switch (FixDump(dmp_file_name, file_info)) {
	case ERROR_OPEN:
		wprintf(L"Can not open \"%s\".", dmp_file_name);
		return 1;
		break;
	case ERROR_INVALID_FORMAT:
		wprintf(L"File \"%s\" has an incorrect format.", dmp_file_name);
		return 1;
		break;
	case ERROR_MODULE_NOT_FOUND:
		wprintf(L"Module \"%s\" not found in the DMP file.", PathFindFileNameW(exe_file_name));
		return 1;
		break;
	}

	wprintf(L"DMP file sucessfully updated.");

	return 0;
}