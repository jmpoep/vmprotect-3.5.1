#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "loader.h"
#include "registry_manager.h"

class CipherRC5;

typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG         FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation,
	SectionRelocationInformation
} SECTION_INFORMATION_CLASS, *PSECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
	ULONG                   BaseAddress;
	ULONG                   Attributes;
	LARGE_INTEGER           Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION {
	PVOID                   EntryPoint;
	ULONG                   StackZeroBits;
	ULONG                   StackReserved;
	ULONG                   StackCommit;
	ULONG                   ImageSubsystem;
	WORD                    SubSystemVersionLow;
	WORD                    SubSystemVersionHigh;
	ULONG                   Unknown1;
	ULONG                   ImageCharacteristics;
	ULONG                   ImageMachineType;
	ULONG                   Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL					((NTSTATUS)0xC0000023L)
#define STATUS_INVALID_IMAGE_FORMAT				((NTSTATUS)0xC000007BL)
#define STATUS_INVALID_PAGE_PROTECTION			((NTSTATUS)0xC0000045L)
#define STATUS_ACCESS_DENIED					((NTSTATUS)0xC0000022L)
#define STATUS_NOT_IMPLEMENTED					((NTSTATUS)0xC0000002L)
#define STATUS_INFO_LENGTH_MISMATCH				((NTSTATUS)0xC0000004L)
#define STATUS_BUFFER_OVERFLOW					((NTSTATUS)0x80000005L)
#define STATUS_OBJECT_NAME_NOT_FOUND			((NTSTATUS)0xC0000034L)
#define STATUS_NO_MORE_ENTRIES					((NTSTATUS)0x8000001AL)
#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER				((NTSTATUS)0xC000000DL)
#endif
#define STATUS_INVALID_DEVICE_REQUEST			((NTSTATUS)0xC0000010L)
#define STATUS_INVALID_FILE_FOR_SECTION			((NTSTATUS)0xC0000020L)

#define OBJ_HANDLE_TAGBITS  0x00000003L
#define EXHANDLE(x) ((HANDLE)((ULONG_PTR)(x) &~ OBJ_HANDLE_TAGBITS))

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

#define IMAGE_FILE_MACHINE_I486 0x14D
#define IMAGE_FILE_MACHINE_I586 0x14E

#define FileBothDirectoryInformation (FILE_INFORMATION_CLASS)0x03
#define FileBasicInformation (FILE_INFORMATION_CLASS)0x04
#define FileStandardInformation (FILE_INFORMATION_CLASS)0x05
#define FileNameInformation (FILE_INFORMATION_CLASS)0x09
#define FilePositionInformation (FILE_INFORMATION_CLASS)0x0e
#define FileIdBothDirectoryInformation (FILE_INFORMATION_CLASS)0x25
#define FileAllInformation (FILE_INFORMATION_CLASS)0x12

typedef struct _FILE_POSITION_INFORMATION {
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG         NumberOfLinks;
	BOOLEAN       DeletePending;
	BOOLEAN       Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION {
	LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
	ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
	ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
	ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
	FILE_BASIC_INFORMATION     BasicInformation;
	FILE_STANDARD_INFORMATION  StandardInformation;
	FILE_INTERNAL_INFORMATION  InternalInformation;
	FILE_EA_INFORMATION        EaInformation;
	FILE_ACCESS_INFORMATION    AccessInformation;
	FILE_POSITION_INFORMATION  PositionInformation;
	FILE_MODE_INFORMATION      ModeInformation;
	FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	FILE_NAME_INFORMATION      NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name; 
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_DEVICE_INFORMATION {
    DEVICE_TYPE DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
	ULONG FileSystemAttributes;
	LONG  MaximumComponentNameLength;
	ULONG FileSystemNameLength;
	WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
	LARGE_INTEGER VolumeCreationTime;
	ULONG VolumeSerialNumber;
	ULONG VolumeLabelLength;
	BOOLEAN SupportsObjects;
	WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	ULONG         EaSize;
	CCHAR         ShortNameLength;
	WCHAR         ShortName[12];
	WCHAR         FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	ULONG         EaSize;
	CCHAR         ShortNameLength;
	WCHAR         ShortName[12];
	LARGE_INTEGER FileId;
	WCHAR         FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

struct OBJECT_DIRECTORY {
	uint32_t NumberOfEntries;
	// OBJECT_ENTRY Entries[];
};

#define ObjectNameInformation (OBJECT_INFORMATION_CLASS)1

struct OBJECT_ENTRY {
	uint32_t NameOffset;
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t Options;
};

class Path
{
public:
	Path(wchar_t *str)
	{
		const wchar_t *last = str;
		const wchar_t *cur = str;
		while (*cur) {
			if (*cur == '\\') {
				if (cur > last)
					list_.push_back(UnicodeString(last, cur - last));
				last = cur + 1;
			}
			cur++;
		}
		if (cur > last)
			list_.push_back(UnicodeString(last, cur - last));
	}

	UnicodeString Combine(wchar_t *str)
	{
		vector<const UnicodeString *> folder_list;
		for (size_t i = 0; i < list_.size(); i++) {
			folder_list.push_back(&list_[i]);
		}

		Path p(str);
		for (size_t i = 0; i < p.list_.size(); i++) {
			const UnicodeString &folder = p.list_[i];
			if (folder == L".")
				continue;

			if (folder == L"..")
				folder_list.pop_back();
			else
				folder_list.push_back(&folder);
		}

		UnicodeString res;
		for (size_t i = 0; i < folder_list.size(); i++) {
			if (i > 0)
				res.append(L"\\", 1);
			res.append(folder_list[i]->c_str());
		}
		return res;
	}

private:
	vector<UnicodeString> list_;
};

class HookManager;
class FileManager
{
public:
	FileManager(const uint8_t *data, HMODULE instance, const uint8_t *key, VirtualObjectList *objects);
	void HookAPIs(HookManager &hook_manager);
	void UnhookAPIs(HookManager &hook_manager);
	bool OpenFiles(RegistryManager &registry_manager);
	NTSTATUS NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
	NTSTATUS NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
	NTSTATUS NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
	NTSTATUS NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	NTSTATUS NtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass);
	NTSTATUS NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	NTSTATUS NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan);
	NTSTATUS NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
	NTSTATUS NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);
	NTSTATUS NtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength);
	NTSTATUS NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
	NTSTATUS NtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress);
	NTSTATUS NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength);
private:
	NTSTATUS TrueNtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
	NTSTATUS TrueNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
	NTSTATUS TrueNtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
	NTSTATUS TrueNtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	NTSTATUS TrueNtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, ULONG Length, FS_INFORMATION_CLASS FsInformationClass);
	NTSTATUS TrueNtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
	NTSTATUS TrueNtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan);
	NTSTATUS TrueNtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
	NTSTATUS TrueNtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);
	NTSTATUS TrueNtQuerySection(HANDLE SectionHandle, SECTION_INFORMATION_CLASS InformationClass, PVOID InformationBuffer, ULONG InformationBufferSize, PULONG ResultLength);
	NTSTATUS TrueNtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
	NTSTATUS TrueNtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress);
	NTSTATUS TrueNtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength);
	NTSTATUS TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
	OBJECT_DIRECTORY DecryptDirectory(const OBJECT_DIRECTORY *directory_enc) const;
	OBJECT_ENTRY DecryptEntry(const OBJECT_ENTRY *entry_enc) const;
	bool DecryptString(LPCWSTR str_enc, LPWSTR str, size_t max_size) const;
	const OBJECT_ENTRY *FindEntry(HANDLE directory, PUNICODE_STRING object_name);
	bool ReadFile(const OBJECT_ENTRY *entry, uint64_t offset, void *dst, size_t size) const;
	NTSTATUS ReadImageHeader(const OBJECT_ENTRY *entry, IMAGE_NT_HEADERS *header) const;
	NTSTATUS ReadImage(const OBJECT_ENTRY *entry, void *image_base) const;

	HMODULE instance_;
	uint32_t key_;
	VirtualObjectList *objects_;
	const uint8_t *data_;
	void *nt_query_attributes_file_;
	void *nt_create_file_;
	void *nt_open_file_;
	void *nt_read_file_;
	void *nt_query_information_file_;
	void *nt_query_volume_information_file_;
	void *nt_set_information_file_;
	void *nt_query_directory_file_;
	void *nt_close_;
	void *nt_create_section_;
	void *nt_query_section_;
	void *nt_map_view_of_section_;
	void *nt_unmap_view_of_section_;
	void *nt_query_virtual_memory_;
	vector<UnicodeString> file_name_list_;
	UnicodeString dos_root_;
	UnicodeString nt_root_;

	// no copy ctr or assignment op
	FileManager(const FileManager &);
	FileManager &operator =(const FileManager &);
};

#endif