using System;

// ReSharper disable once CheckNamespace
// ReSharper disable InconsistentNaming
namespace VMProtect
{
	[AttributeUsage(AttributeTargets.Enum)]
	public sealed class DeleteOnCompilation : Attribute { }

	[VMProtect.DeleteOnCompilation]
	public enum Variables : uint
	{
		IS_PATCH_DETECTED,
		IS_DEBUGGER_DETECTED,
		LOADER_CRC_INFO,
		LOADER_CRC_INFO_SIZE,
		LOADER_CRC_INFO_HASH,
		SESSION_KEY,
		DRIVER_UNLOAD,
		CRC_IMAGE_SIZE,
		LOADER_STATUS,
		SERVER_DATE,
		RESERVED,
		CPU_COUNT,
		CPU_HASH,
		COUNT = CPU_HASH + 32
	}

	[VMProtect.DeleteOnCompilation]
	public enum Faces : uint
	{
		MASK = 0xFACE0000U,

        RC5_P,
        RC5_Q,

        STRING_INFO,
        RESOURCE_INFO,
        STORAGE_INFO,
        REGISTRY_INFO,
        LICENSE_INFO,
        LICENSE_INFO_SIZE,
        KEY_INFO,
        RUNTIME_ENTRY,
        CRC_INFO_SALT,
        CRC_TABLE_ENTRY,
        CRC_TABLE_SIZE,
        CRC_TABLE_HASH,
        TRIAL_HWID,
        TRIAL_HWID_SIZE,
        CORE_OPTIONS,
        IMAGE_BASE,
        FILE_BASE,

        NTOSKRNL_NAME,
        HAL_NAME,
        USER32_NAME,
        MESSAGE_BOX_NAME,
        KERNEL32_NAME,
        CREATE_FILE_NAME,
        CLOSE_HANDLE_NAME,
        INITIALIZATION_ERROR,
        PROC_NOT_FOUND,
        ORDINAL_NOT_FOUND,
        STRING_DECRYPT_KEY,
        DRIVER_FORMAT_VALUE,
        FILE_CORRUPTED,
        LOADER_OPTIONS,
        LOADER_DATA,
        DEBUGGER_FOUND,
        NT_SET_INFORMATION_PROCESS_NAME,
        NT_RAISE_HARD_ERROR_NAME,
        IS_WOW64_PROCESS_NAME,
        WINE_GET_VERSION_NAME,
        MACOSX_FORMAT_VALUE,
        GNU_PTRACE,
        UNREGISTERED_VERSION,
        WTSAPI32_NAME,
        WTS_SEND_MESSAGE_NAME,
        NTDLL_NAME,
        NT_QUERY_INFORMATION_NAME,
        NT_SET_INFORMATION_THREAD_NAME,
        SICE_NAME,
        SIWVID_NAME,
        NTICE_NAME,
        ICEEXT_NAME,
        SYSER_NAME,
        VIRTUAL_MACHINE_FOUND,
        SBIEDLL_NAME,
        QUERY_VIRTUAL_MEMORY_NAME,
        ENUM_SYSTEM_FIRMWARE_NAME,
        GET_SYSTEM_FIRMWARE_NAME,
        NT_QUERY_INFORMATION_PROCESS_NAME,
        NT_VIRTUAL_PROTECT_NAME,
        NT_OPEN_FILE_NAME,
        NT_CREATE_SECTION_NAME,
        NT_OPEN_SECTION_NAME,
        NT_MAP_VIEW_OF_SECTION,
        NT_UNMAP_VIEW_OF_SECTION,
        NT_CLOSE,
        SYSCALL,
		NT_ALLOCATE_VIRTUAL_MEMORY_NAME,
		NT_FREE_VIRTUAL_MEMORY_NAME,

		PACKER_INFO = 0xFACE0100U,
        PACKER_INFO_SIZE,
        FILE_CRC_INFO,
        FILE_CRC_INFO_SIZE,
        LOADER_CRC_INFO,
        LOADER_CRC_INFO_SIZE,
        SECTION_INFO,
        SECTION_INFO_SIZE,
        FIXUP_INFO,
        FIXUP_INFO_SIZE,
        RELOCATION_INFO,
        RELOCATION_INFO_SIZE,
        IAT_INFO,
        IAT_INFO_SIZE,
        IMPORT_INFO,
        IMPORT_INFO_SIZE,
        INTERNAL_IMPORT_INFO,
        INTERNAL_IMPORT_INFO_SIZE,
        MEMORY_CRC_INFO,
        MEMORY_CRC_INFO_SIZE,
        DELAY_IMPORT_INFO,
        DELAY_IMPORT_INFO_SIZE,
        LOADER_CRC_INFO_HASH,
        MEMORY_CRC_INFO_HASH,
		TLS_INDEX_INFO,

		VAR = 0xFACE0200U,
		VAR_IS_PATCH_DETECTED =  VAR | (Variables.IS_PATCH_DETECTED << 4),
		VAR_IS_DEBUGGER_DETECTED = VAR | (Variables.IS_DEBUGGER_DETECTED << 4),
		VAR_LOADER_CRC_INFO = VAR | (Variables.LOADER_CRC_INFO << 4),
		VAR_LOADER_CRC_INFO_SIZE = VAR | (Variables.LOADER_CRC_INFO_SIZE << 4),
		VAR_LOADER_CRC_INFO_HASH = VAR | (Variables.LOADER_CRC_INFO_HASH << 4),
		VAR_SESSION_KEY = VAR | (Variables.SESSION_KEY << 4),
		VAR_LOADER_STATUS = VAR | (Variables.LOADER_STATUS << 4),
		VAR_SERVER_DATE = VAR | (Variables.SERVER_DATE << 4),

		VAR_SALT = 0xFACE0300U,
		VAR_IS_PATCH_DETECTED_SALT =  VAR_SALT | Variables.IS_PATCH_DETECTED,
		VAR_IS_DEBUGGER_DETECTED_SALT = VAR_SALT | Variables.IS_DEBUGGER_DETECTED,
		VAR_LOADER_CRC_INFO_SALT = VAR_SALT | Variables.LOADER_CRC_INFO,
		VAR_LOADER_CRC_INFO_SIZE_SALT = VAR_SALT | Variables.LOADER_CRC_INFO_SIZE,
		VAR_LOADER_CRC_INFO_HASH_SALT = VAR_SALT | Variables.LOADER_CRC_INFO_HASH,
		VAR_SERVER_DATE_SALT = VAR_SALT | Variables.SERVER_DATE,
	}

	public static class GlobalData
	{
		private static uint[] _v = new uint[(uint)Variables.COUNT];
		public static bool IsPatchDetected() { return (_v[(uint)Faces.VAR_IS_PATCH_DETECTED] ^ (uint)Faces.VAR_IS_PATCH_DETECTED_SALT) != 0; }
		public static bool IsDebuggerDetected() { return (_v[(uint)Faces.VAR_IS_DEBUGGER_DETECTED] ^ (uint)Faces.VAR_IS_DEBUGGER_DETECTED_SALT) != 0; }
		public static uint LoaderCrcInfo() { return _v[(uint)Faces.VAR_LOADER_CRC_INFO] ^ (uint)Faces.VAR_LOADER_CRC_INFO_SALT; }
		public static uint LoaderCrcSize() { return _v[(uint)Faces.VAR_LOADER_CRC_INFO_SIZE] ^ (uint)Faces.VAR_LOADER_CRC_INFO_SIZE_SALT; }
		public static uint LoaderCrcHash() { return _v[(uint)Faces.VAR_LOADER_CRC_INFO_HASH] ^ (uint)Faces.VAR_LOADER_CRC_INFO_HASH_SALT; }
		public static uint SessionKey() { return _v[(uint)Faces.VAR_SESSION_KEY]; }
		public static uint LoaderStatus() { return _v[(uint)Faces.VAR_LOADER_STATUS]; }
		public static uint ServerDate() { return _v[(uint)Faces.VAR_SERVER_DATE] ^ (uint)Faces.VAR_SERVER_DATE_SALT; }
		public static void SetIsPatchDetected(bool value) { _v[(uint)Faces.VAR_IS_PATCH_DETECTED] = (uint)(value ? 1 : 0) ^ (uint)Faces.VAR_IS_PATCH_DETECTED_SALT; }
		public static void SetIsDebuggerDetected(bool value) { _v[(uint)Faces.VAR_IS_DEBUGGER_DETECTED] = (uint)(value ? 1 : 0) ^ (uint)Faces.VAR_IS_DEBUGGER_DETECTED_SALT; }
		public static void SetLoaderCrcInfo(uint value) { _v[(uint)Faces.VAR_LOADER_CRC_INFO] = value ^ (uint)Faces.VAR_LOADER_CRC_INFO_SALT; }
		public static void SetLoaderCrcSize(uint value) { _v[(uint)Faces.VAR_LOADER_CRC_INFO_SIZE] = value ^ (uint)Faces.VAR_LOADER_CRC_INFO_SIZE_SALT; }
		public static void SetLoaderCrcHash(uint value) { _v[(uint)Faces.VAR_LOADER_CRC_INFO_HASH] = value ^ (uint)Faces.VAR_LOADER_CRC_INFO_HASH_SALT; }
		public static void SetSessionKey(uint value) { _v[(uint)Faces.VAR_SESSION_KEY] = value; }
		public static void SetLoaderStatus(uint value) { _v[(uint)Faces.VAR_LOADER_STATUS] = value; }
		public static void SetServerDate(uint value) { _v[(uint)Faces.VAR_SERVER_DATE] = value ^ (uint)Faces.VAR_SERVER_DATE_SALT; }
	}
}