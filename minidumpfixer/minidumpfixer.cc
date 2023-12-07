#include "minidumpfixer.h"

int GetFileInfo(const wchar_t *file_name, FileInfo &file_info)
{
	int res = ERROR_OPEN;
	HANDLE file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (file != INVALID_HANDLE_VALUE) {
		HANDLE map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
		if (map) {
			IMAGE_DOS_HEADER *dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0));
			if (dos_header) {
				res = ERROR_INVALID_FORMAT;
				try {
					if (dos_header->e_magic == IMAGE_DOS_SIGNATURE) {
						IMAGE_NT_HEADERS32 *nt_header32 = reinterpret_cast<IMAGE_NT_HEADERS32 *>(reinterpret_cast<byte *>(dos_header) + dos_header->e_lfanew);
						if (nt_header32->Signature == IMAGE_NT_SIGNATURE) {
							if (nt_header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
								file_info.TimeDateStamp = nt_header32->FileHeader.TimeDateStamp;
								file_info.SizeOfImage = nt_header32->OptionalHeader.SizeOfImage;
								file_info.CheckSum = nt_header32->OptionalHeader.CheckSum;
								res = ERROR_SUCCESS;
							} else if (nt_header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
								IMAGE_NT_HEADERS64 *nt_header64 = reinterpret_cast<IMAGE_NT_HEADERS64 *>(nt_header32);
								file_info.TimeDateStamp = nt_header64->FileHeader.TimeDateStamp;
								file_info.SizeOfImage = nt_header64->OptionalHeader.SizeOfImage;
								file_info.CheckSum = nt_header64->OptionalHeader.CheckSum;
								res = ERROR_SUCCESS;
							}
						}
					}
				} catch(...) {
					res = ERROR_INVALID_FORMAT;
				}
				UnmapViewOfFile(dos_header);
			}
			CloseHandle(map);
		}
		CloseHandle(file);
	}

	return res;
}

int FixDump(const wchar_t *file_name, const FileInfo &file_info)
{
	int res = ERROR_OPEN;
	HANDLE file = CreateFileW(file_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (file != INVALID_HANDLE_VALUE) {
		HANDLE map = CreateFileMapping(file, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (map) {
			MINIDUMP_HEADER *header = reinterpret_cast<MINIDUMP_HEADER *>(MapViewOfFile(map, SECTION_MAP_WRITE, 0, 0, 0));
			if (header) {
				res = ERROR_INVALID_FORMAT;
				try {
					if (header->Signature == MINIDUMP_SIGNATURE) {
						res = ERROR_MODULE_NOT_FOUND;
						MINIDUMP_DIRECTORY *directory = reinterpret_cast<MINIDUMP_DIRECTORY *>(reinterpret_cast<byte *>(header) + header->StreamDirectoryRva);
						for (ULONG32 i = 0; i < header->NumberOfStreams; i++, directory++) {
							if (directory->StreamType == ModuleListStream) {
								MINIDUMP_MODULE_LIST *module_list = reinterpret_cast<MINIDUMP_MODULE_LIST *>(reinterpret_cast<byte *>(header) + directory->Location.Rva);
								for (ULONG32 j = 0; j < module_list->NumberOfModules; j++) {
									MINIDUMP_MODULE *module = &module_list->Modules[j];
									if (module->TimeDateStamp == file_info.TimeDateStamp) {
										module->SizeOfImage = file_info.SizeOfImage;
										module->CheckSum = file_info.CheckSum;
										res = ERROR_SUCCESS;
									}
								}
							}
						}
						if (res == ERROR_SUCCESS)
							FlushViewOfFile(header, 0);
					}
				} catch(...) {
					res = ERROR_INVALID_FORMAT;
				}
				UnmapViewOfFile(header);
			}
			CloseHandle(map);
		}
		CloseHandle(file);
	}

	return res;
}