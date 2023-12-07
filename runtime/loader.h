#ifndef LOADER_H
#define LOADER_H

#pragma pack(push, 1)

struct CRC_INFO {
	uint32_t Address;
	uint32_t Size;
	uint32_t Hash;
};

struct FILE_CRC_INFO {
	uint32_t FileSize;
	// CRCInfo crc_info[1]
};

struct SECTION_INFO {
	uint32_t Address;
	uint32_t Size;
	uint32_t Type;
};

struct PACKER_INFO {
	uint32_t Src;
	uint32_t Dst;
};

struct IAT_INFO {
	uint32_t Src;
	uint32_t Dst;
	uint32_t Size;
};

struct DLL_INFO {
	uint32_t Name;
	// IMPORT_INFO import_info[1];
};

struct IMPORT_INFO {
	uint32_t Name;
	uint32_t Address;
	int32_t Key;
};

struct FIXUP_INFO {
	uint32_t Address;
	uint32_t BlockSize;
	// uint32_t type_offset[1];
};

struct RELOCATION_INFO {
	uint32_t Address;
	uint32_t Source;
	uint32_t Type;
};

struct SETUP_IMAGE_DATA {
	NOINLINE SETUP_IMAGE_DATA() { empty_ = 0; }

	NOINLINE uint8_t *file_base() { return reinterpret_cast<uint8_t *>(FACE_FILE_BASE) - empty_; }
	NOINLINE uint8_t *image_base() { return reinterpret_cast<uint8_t *>(FACE_IMAGE_BASE) - empty_; }
	NOINLINE uint32_t options() { return FACE_LOADER_OPTIONS - empty_; }
	NOINLINE uint32_t storage() { return FACE_LOADER_DATA - empty_; }
	NOINLINE uint32_t runtime_entry() { return FACE_RUNTIME_ENTRY - empty_;	}
#ifdef __unix__
	NOINLINE uint32_t relro_info() { return FACE_GNU_RELRO_INFO - empty_; }
#elif defined(__APPLE__)
#elif defined(WIN_DRIVER)
#else
	NOINLINE uint32_t tls_index_info() { return FACE_TLS_INDEX_INFO - empty_; }
#endif

	// file CRC information
	NOINLINE uint32_t file_crc_info() { return FACE_FILE_CRC_INFO - empty_; }
	NOINLINE uint32_t file_crc_info_size() { return FACE_FILE_CRC_INFO_SIZE - empty_; }

	// header and loader CRC information
	NOINLINE uint32_t loader_crc_info() { return FACE_LOADER_CRC_INFO - empty_; }
	NOINLINE uint32_t loader_crc_info_size() { return FACE_LOADER_CRC_INFO_SIZE - empty_; }
	NOINLINE uint32_t loader_crc_info_hash() { return FACE_LOADER_CRC_INFO_HASH - empty_; }

	// section information
	NOINLINE uint32_t section_info() { return FACE_SECTION_INFO - empty_; }
	NOINLINE uint32_t section_info_size() { return FACE_SECTION_INFO_SIZE - empty_; }

	// packer information
	NOINLINE uint32_t packer_info() { return FACE_PACKER_INFO - empty_; }
	NOINLINE uint32_t packer_info_size() { return FACE_PACKER_INFO_SIZE - empty_; }

	// fixups information
	NOINLINE uint32_t fixup_info() { return FACE_FIXUP_INFO - empty_; }
	NOINLINE uint32_t fixup_info_size() { return FACE_FIXUP_INFO_SIZE - empty_; }

	// relocations information
	NOINLINE uint32_t relocation_info() { return FACE_RELOCATION_INFO - empty_; }
	NOINLINE uint32_t relocation_info_size() { return FACE_RELOCATION_INFO_SIZE - empty_; }

	// IAT information
	NOINLINE uint32_t iat_info() { return FACE_IAT_INFO - empty_; }
	NOINLINE uint32_t iat_info_size() { return FACE_IAT_INFO_SIZE - empty_; }

	// import information
	NOINLINE uint32_t import_info() { return FACE_IMPORT_INFO - empty_; }
	NOINLINE uint32_t import_info_size() { return FACE_IMPORT_INFO_SIZE - empty_; }

	// internal import information
	NOINLINE uint32_t internal_import_info() { return FACE_INTERNAL_IMPORT_INFO - empty_; }
	NOINLINE uint32_t internal_import_info_size() { return FACE_INTERNAL_IMPORT_INFO_SIZE - empty_; }

	// memory CRC information
	NOINLINE uint32_t memory_crc_info() { return FACE_MEMORY_CRC_INFO - empty_; }
	NOINLINE uint32_t memory_crc_info_size() { return FACE_MEMORY_CRC_INFO_SIZE - empty_; }
	NOINLINE uint32_t memory_crc_info_hash() { return FACE_MEMORY_CRC_INFO_HASH - empty_; }

	// delay import information
	NOINLINE uint32_t delay_import_info() { return FACE_DELAY_IMPORT_INFO - empty_; }
	NOINLINE uint32_t delay_import_info_size() { return FACE_DELAY_IMPORT_INFO_SIZE - empty_; }
private:
	uint32_t empty_;
};

#pragma pack(pop)

#ifndef VMP_GNU

#define MAXIMUM_FILENAME_LENGTH 256

typedef struct _SYSTEM_MODULE_ENTRY
{
#ifdef _WIN64
	ULONGLONG Unknown1;
	ULONGLONG Unknown2;
#else
	ULONG Unknown1;
	ULONG Unknown2;
#endif
	PVOID BaseAddress;
	ULONG Size;
	ULONG Flags;
	ULONG EntryIndex;
	USHORT NameLength;  // Length of module name not including the path, this field contains valid value only for NTOSKRNL module
	USHORT PathLength;  // Length of 'directory path' part of modulename
	CHAR Name[MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG Count;
#ifdef _WIN64
	ULONG Unknown1;
#endif
	SYSTEM_MODULE_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION 
{
	BOOLEAN DebuggerEnabled;
	BOOLEAN DebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

#ifdef WIN_DRIVER

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#pragma pack(push, 2)

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#pragma pack(pop)

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_ROM_OPTIONAL_HEADER {
    WORD   Magic;
    BYTE   MajorLinkerVersion;
    BYTE   MinorLinkerVersion;
    DWORD  SizeOfCode;
    DWORD  SizeOfInitializedData;
    DWORD  SizeOfUninitializedData;
    DWORD  AddressOfEntryPoint;
    DWORD  BaseOfCode;
    DWORD  BaseOfData;
    DWORD  BaseOfBss;
    DWORD  GprMask;
    DWORD  CprMask[4];
    DWORD  GpValue;
} IMAGE_ROM_OPTIONAL_HEADER, *PIMAGE_ROM_OPTIONAL_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    WORD        MajorOperatingSystemVersion;
    WORD        MinorOperatingSystemVersion;
    WORD        MajorImageVersion;
    WORD        MinorImageVersion;
    WORD        MajorSubsystemVersion;
    WORD        MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        Subsystem;
    WORD        DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    DWORD       LoaderFlags;
    DWORD       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_ROM_HEADERS {
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_ROM_OPTIONAL_HEADER OptionalHeader;
} IMAGE_ROM_HEADERS, *PIMAGE_ROM_HEADERS;

#ifdef _WIN64
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
#endif

typedef struct _IMAGE_SECTION_HEADER {
	BYTE    Name[8];
	union {
		DWORD   PhysicalAddress;
		DWORD   VirtualSize;
	} Misc;
	DWORD   VirtualAddress;
	DWORD   SizeOfRawData;
	DWORD   PointerToRawData;
	DWORD   PointerToRelocations;
	DWORD   PointerToLinenumbers;
	WORD    NumberOfRelocations;
	WORD    NumberOfLinenumbers;
	DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_MIPS_JMPADDR16        9
#define IMAGE_REL_BASED_IA64_IMM64            9
#define IMAGE_REL_BASED_DIR64                 10

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32) != 0)

typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD   AddressOfFunctions;     // RVA from base of image
    DWORD   AddressOfNames;         // RVA from base of image
    DWORD   AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

#define MAX_PATH          260

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemModuleInformation = 0xb,
	SystemKernelDebuggerInformation = 0x23,
	SystemFirmwareTableInformation = 0x4c
} SYSTEM_INFORMATION_CLASS;

extern "C" {
NTKERNELAPI NTSTATUS NTAPI NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);
}

#else
#define FILE_OPEN                       0x00000001
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040

typedef enum _SECTION_INHERIT {
	ViewShare=1,
	ViewUnmap=2
} SECTION_INHERIT, *PSECTION_INHERIT;

#define SystemModuleInformation (SYSTEM_INFORMATION_CLASS)11
#define SystemKernelDebuggerInformation (SYSTEM_INFORMATION_CLASS)35

#define ThreadHideFromDebugger (THREADINFOCLASS)17

#define ProcessDebugPort (PROCESSINFOCLASS)0x7
#define ProcessDebugObjectHandle (PROCESSINFOCLASS)0x1e
#define ProcessDefaultHardErrorMode (PROCESSINFOCLASS)0x0c
#define ProcessInstrumentationCallback (PROCESSINFOCLASS)40

#define MemoryMappedFilenameInformation (MEMORY_INFORMATION_CLASS)2

#define STATUS_PORT_NOT_SET ((NTSTATUS)0xC0000353L)
#define STATUS_SERVICE_NOTIFICATION ((NTSTATUS)0x40000018L)
#define HARDERROR_OVERRIDE_ERRORMODE 0x10000000

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )

typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION
{
	ULONG Version;
	ULONG Reserved;
	PVOID Callback;
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

typedef enum HardErrorResponse {
	ResponseReturnToCaller,
	ResponseNotHandled,
	ResponseAbort, ResponseCancel,
	ResponseIgnore,
	ResponseNo,
	ResponseOk,
	ResponseRetry,
	ResponseYes
} HardErrorResponse;

typedef enum HardErrorResponseButton {
	ResponseButtonOK,
	ResponseButtonOKCancel,
	ResponseButtonAbortRetryIgnore,
	ResponseButtonYesNoCancel,
	ResponseButtonYesNo,
	ResponseButtonRetryCancel,
	ResponseButtonCancelTryAgainContinue
} HardErrorResponseButton;

typedef enum HardErrorResponseIcon {
	IconAsterisk = 0x40,
	IconError = 0x10,
	IconExclamation = 0x30,
	IconHand = 0x10,
	IconInformation = 0x40,
	IconNone = 0,
	IconQuestion = 0x20,
	IconStop = 0x10,
	IconWarning = 0x30,
	IconUserIcon = 0x80
} HardErrorResponseIcon;

#define SEC_IMAGE_NO_EXECUTE (SEC_IMAGE | SEC_NOCACHE)

enum {
	WINDOWS_XP = 2600,
	WINDOWS_2003 = 3790,
	WINDOWS_VISTA = 6000,
	WINDOWS_VISTA_SP1 = 6001,
	WINDOWS_VISTA_SP2 = 6002,
	WINDOWS_7 = 7600,
	WINDOWS_7_SP1 = 7601,
	WINDOWS_8 = 9200,
	WINDOWS_8_1 = 9600,
	WINDOWS_10_TH1 = 10240,
	WINDOWS_10_TH2 = 10586,
	WINDOWS_10_RS1 = 14393,
	WINDOWS_10_RS2 = 15063,
	WINDOWS_10_RS3 = 16299,
	WINDOWS_10_RS4 = 17134,
	WINDOWS_10_RS5 = 17763,
	WINDOWS_10_19H1 = 18362,
	WINDOWS_10_19H2 = 18363,
	WINDOWS_10_20H1 = 19041,
	WINDOWS_10_20H2 = 19042,
	WINDOWS_10_21H1 = 19043,
	WINDOWS_10_21H2 = 19044,
	WINDOWS_10_22H2 = 19045,
	WINDOWS_11_21H2 = 22000,
	WINDOWS_11_22H2 = 22621,
};

#define IS_KNOWN_WINDOWS_BUILD(b) ( \
	(b) == WINDOWS_XP || \
	(b) == WINDOWS_2003 || \
	(b) == WINDOWS_VISTA || \
	(b) == WINDOWS_VISTA_SP1 || \
	(b) == WINDOWS_VISTA_SP2 || \
	(b) == WINDOWS_7 || \
	(b) == WINDOWS_7_SP1 || \
	(b) == WINDOWS_8 || \
	(b) == WINDOWS_8_1 || \
	(b) == WINDOWS_10_TH1 || \
	(b) == WINDOWS_10_TH2 || \
	(b) == WINDOWS_10_RS1 || \
	(b) == WINDOWS_10_RS2 || \
	(b) == WINDOWS_10_RS3 || \
	(b) == WINDOWS_10_RS4 || \
	(b) == WINDOWS_10_RS5 || \
	(b) == WINDOWS_10_19H1 || \
	(b) == WINDOWS_10_19H2 || \
	(b) == WINDOWS_10_20H1 || \
	(b) == WINDOWS_10_20H2 || \
	(b) == WINDOWS_10_21H1 || \
	(b) == WINDOWS_10_21H2 || \
	(b) == WINDOWS_10_22H2 \
)

#endif // WIN_DRIVER

#endif // VMP_GNU

typedef struct _PEB32 {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[0xa1];
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	USHORT OSBuildNumber;
} PEB32;

typedef struct _PEB64 {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[0x115];
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	USHORT OSBuildNumber;
} PEB64;

#endif