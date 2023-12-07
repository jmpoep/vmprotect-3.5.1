using System;
using System.Runtime.InteropServices;
using System.Text;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public enum MessageBoxButtons : uint
	{
		OK,
		OKCancel,
		AbortRetryIgnore,
		YesNoCancel,
		YesNo,
		RetryCancel,
		CancelTryAgainContinue
	}

	public enum MessageBoxIcon : uint
	{
		Asterisk = 0x40,
		Error = 0x10,
		Exclamation = 0x30,
		Hand = 0x10,
		Information = 0x40,
		None = 0,
		Question = 0x20,
		Stop = 0x10,
		Warning = 0x30,
		UserIcon = 0x80
	}


	internal static class Win32
	{
		[Flags()]
		public enum AllocationType : uint
		{
			Commit = 0x1000,
			Reserve = 0x2000,
		}

		[Flags()]
		public enum FreeType : uint
		{
			Release = 0x8000,
		}

		[Flags()]
		public enum MemoryProtection : uint
		{
			NoAccess = 0x01,
			ReadOnly = 0x02,
			ReadWrite = 0x04,
			WriteCopy = 0x08,
			Execute = 0x10,
			ExecuteRead = 0x20,
			ExecuteReadWrite = 0x40,
			Guard = 0x100
		}

		[Flags()]
		public enum MapAccess : uint
		{
			Copy = 0x0001,
			Write = 0x0002,
			Read = 0x0004,
			AllAccess = 0x001f
		}

		[Flags()]
		public enum SectionType : uint
		{
			Execute = 0x20000000,
			Read = 0x40000000,
			Write = 0x80000000
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct UNICODE_STRING
		{
			public readonly ushort Length;
			public readonly ushort MaximumLength;
			private readonly IntPtr Buffer;

			public UNICODE_STRING(string str)
			{
				if (str == null)
				{
					Length = 0;
					MaximumLength = 0;
					Buffer = IntPtr.Zero;
				}
				else
				{
					Length = (ushort)(str.Length * UnicodeEncoding.CharSize);
					MaximumLength = (ushort)(Length + UnicodeEncoding.CharSize);
					Buffer = Marshal.StringToHGlobalUni(str);
				}
			}

			public void Dispose()
			{
				if (Buffer != IntPtr.Zero)
					Marshal.FreeHGlobal(Buffer);
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct OBJECT_ATTRIBUTES
		{
			public readonly uint Length;
			public readonly IntPtr RootDirectory;
			public readonly IntPtr ObjectName;
			public readonly uint Attributes;
			public readonly IntPtr SecurityDescriptor;
			public readonly IntPtr SecurityQualityOfService;

			public OBJECT_ATTRIBUTES(UNICODE_STRING name, uint attrs)
			{
				Length = (uint)Marshal.SizeOf(typeof(OBJECT_ATTRIBUTES));
				RootDirectory = IntPtr.Zero;
				ObjectName = IntPtr.Zero;
				Attributes = attrs;
				SecurityDescriptor = IntPtr.Zero;
				SecurityQualityOfService = IntPtr.Zero;

				ObjectName = Marshal.AllocHGlobal(Marshal.SizeOf(name));
				Marshal.StructureToPtr(name, ObjectName, false);
			}

			public void Dispose()
			{
				Marshal.FreeHGlobal(ObjectName);
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct IO_STATUS_BLOCK
		{
			public uint Status;
			public IntPtr Information;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct LARGE_INTEGER
		{
			public uint LowPart;
			public uint HighPart;
		}

		public enum HardErrorResponse
		{
			ReturnToCaller,
			NotHandled,
			Abort, Cancel,
			Ignore,
			No,
			Ok,
			Retry,
			Yes
		}

		public enum Values : uint
		{
			GENERIC_READ = 0x80000000,
			FILE_SHARE_READ = 0x00000001,
			FILE_SHARE_WRITE = 0x00000002,
			FILE_READ_ATTRIBUTES = 0x00000080,
			SYNCHRONIZE = 0x00100000,
			SECTION_QUERY = 0x0001,
			SECTION_MAP_WRITE = 0x0002,
			SECTION_MAP_READ = 0x0004,
			SECTION_MAP_EXECUTE = 0x0008,
			SEC_COMMIT = 0x8000000,
			FILE_SYNCHRONOUS_IO_NONALERT = 0x00000020,
			FILE_NON_DIRECTORY_FILE = 0x00000040,
		}

		public static readonly IntPtr InvalidHandleValue = new IntPtr(-1);
		public static readonly IntPtr NullHandle = IntPtr.Zero;
		public static readonly IntPtr CurrentProcess = new IntPtr(-1);
		public static readonly IntPtr CurrentThread = new IntPtr(-2);

		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtQueryInformationProcess(IntPtr ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
			IntPtr ProcessInformation, int ProcessInformationLength, out uint ReturnLength);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtSetInformationThread(IntPtr ThreadHandle, THREADINFOCLASS ThreadInformationClass, IntPtr ThreadInformation,
			uint ThreadInformationLength);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tEnumSystemFirmwareTables(uint firmwareTableProviderSignature, IntPtr firmwareTableBuffer, uint bufferSize);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tGetSystemFirmwareTable(uint firmwareTableProviderSignature, uint firmwareTableID, IntPtr firmwareTableBuffer, uint bufferSize);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtClose(IntPtr Handle);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtProtectVirtualMemory(IntPtr ProcesssHandle, ref IntPtr BaseAddress, ref UIntPtr RegionSize, MemoryProtection Protect, out MemoryProtection OldProtect);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtAllocateVirtualMemory(IntPtr ProcessHandle, ref IntPtr BaseAddress, IntPtr ZeroBits, ref UIntPtr RegionSize, AllocationType AllocationType, MemoryProtection Protect);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtFreeVirtualMemory(IntPtr ProcessHandle, ref IntPtr BaseAddress, ref UIntPtr RegionSize, FreeType FreeType);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtUnmapViewOfSection(IntPtr ProcessHandle, IntPtr BaseAddress);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate IntPtr tMapViewOfFile(IntPtr hFileMappingObject, MapAccess dwDesiredAccess, int dwFileOffsetHigh, int dwFileOffsetLow, IntPtr dwNumBytesToMap);

		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtRaiseHardError(uint ErrorStatus, uint NumberOfParameters, uint UnicodeStringParameterMask, IntPtr[] Parameters, uint ValidResponseOptions, out HardErrorResponse Response);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtOpenFile(out IntPtr FileHandle, uint DesiredAccess, ref OBJECT_ATTRIBUTES ObjectAttributes, out IO_STATUS_BLOCK IoStatusBlock, uint ShareAccess, uint OpenOptions);
		[UnmanagedFunctionPointerAttribute(CallingConvention.StdCall)]
		private delegate uint tNtCreateSection(out IntPtr SectionHandle, uint DesiredAccess, ref OBJECT_ATTRIBUTES ObjectAttributes, ref LARGE_INTEGER MaximumSize, MemoryProtection SectionPageProtection, uint AllocationAttributes, IntPtr FileHandle);


		private static IntPtr _ntdll;
		private static tNtQueryInformationProcess _nt_query_information_process;
		private static tNtSetInformationThread _nt_set_information_thread;
		private static tNtClose _nt_close;
		private static tNtProtectVirtualMemory _nt_protect_virtual_memory;
		private static tNtAllocateVirtualMemory _nt_allocate_virtual_memory;
		private static tNtFreeVirtualMemory _nt_free_virtual_memory;
		private static tNtUnmapViewOfSection _nt_unmap_view_of_section;
		private static tNtRaiseHardError _nt_raise_hard_error;
		private static tNtOpenFile _nt_open_file;
		private static tNtCreateSection _nt_create_section;
		private static IntPtr _kernel32;
		private static tEnumSystemFirmwareTables _enum_system_firmware_tables;
		private static tGetSystemFirmwareTable _get_system_firmware_table;
		private static tMapViewOfFile _map_view_of_file;


		static Win32()
		{
			_ntdll = GetModuleHandleEnc((uint)Faces.NTDLL_NAME);
			_nt_query_information_process = (tNtQueryInformationProcess)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_QUERY_INFORMATION_PROCESS_NAME), typeof(tNtQueryInformationProcess));
			_nt_set_information_thread = (tNtSetInformationThread)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_SET_INFORMATION_THREAD_NAME), typeof(tNtSetInformationThread));
			_nt_close = (tNtClose)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_CLOSE), typeof(tNtClose));
			_nt_protect_virtual_memory = (tNtProtectVirtualMemory)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_VIRTUAL_PROTECT_NAME), typeof(tNtProtectVirtualMemory));
			_nt_allocate_virtual_memory = (tNtAllocateVirtualMemory)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_ALLOCATE_VIRTUAL_MEMORY_NAME), typeof(tNtAllocateVirtualMemory));
			_nt_free_virtual_memory = (tNtFreeVirtualMemory)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_FREE_VIRTUAL_MEMORY_NAME), typeof(tNtFreeVirtualMemory));
			_nt_unmap_view_of_section = (tNtUnmapViewOfSection)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_UNMAP_VIEW_OF_SECTION), typeof(tNtUnmapViewOfSection));
			_nt_raise_hard_error = (tNtRaiseHardError)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_RAISE_HARD_ERROR_NAME), typeof(tNtRaiseHardError));
			_nt_open_file = (tNtOpenFile)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_OPEN_FILE_NAME), typeof(tNtOpenFile));
			_nt_create_section = (tNtCreateSection)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_ntdll, (uint)Faces.NT_CREATE_SECTION_NAME), typeof(tNtCreateSection));

			_kernel32 = GetModuleHandleEnc((uint)Faces.KERNEL32_NAME);
			IntPtr proc = GetProcAddressEnc(_kernel32, (uint)Faces.ENUM_SYSTEM_FIRMWARE_NAME);
			if (proc != IntPtr.Zero)
				_enum_system_firmware_tables = (tEnumSystemFirmwareTables)Marshal.GetDelegateForFunctionPointer(proc, typeof(tEnumSystemFirmwareTables));
			proc = GetProcAddressEnc(_kernel32, (uint)Faces.GET_SYSTEM_FIRMWARE_NAME);
			if (proc != IntPtr.Zero)
				_get_system_firmware_table = (tGetSystemFirmwareTable)Marshal.GetDelegateForFunctionPointer(proc, typeof(tGetSystemFirmwareTable));
			_map_view_of_file = (tMapViewOfFile)Marshal.GetDelegateForFunctionPointer(GetProcAddressEnc(_kernel32, (uint)Faces.NT_MAP_VIEW_OF_SECTION), typeof(tMapViewOfFile));
		}

		internal static IntPtr GetModuleHandleEnc(uint position)
		{
			long instance = Marshal.GetHINSTANCE(typeof(Loader).Module).ToInt64();
			var buffer = new byte[100];
			for (var pos = 0; pos < buffer.Length; pos++)
			{
				var c = (byte)(Marshal.ReadByte(new IntPtr(instance + position + pos)) ^ (BitRotate.Left((uint)Faces.STRING_DECRYPT_KEY, pos) + pos));
				if (c == 0)
				{
					if (pos > 0)
						return Win32.GetModuleHandle(Encoding.ASCII.GetString(buffer, 0, pos));
					break;
				}

				buffer[pos] = c;
			}

			return IntPtr.Zero;
		}

		[DllImport("kernel32", SetLastError = true)]
		public static extern uint GetFileSize(IntPtr handle, IntPtr lpFileSizeHigh);

		[DllImport("kernel32", SetLastError = true)]
		public static extern bool GetVolumeInformation(
			string pathName,
			StringBuilder volumeNameBuffer,
			uint volumeNameSize,
			ref uint volumeSerialNumber,
			ref uint maximumComponentLength,
			ref uint fileSystemFlags,
			StringBuilder fileSystemNameBuffer,
			uint fileSystemNameSize);

		[DllImport("kernel32", SetLastError = true, CharSet = CharSet.Auto)]
		public static extern IntPtr GetModuleHandle(String lpModuleName);

		[StructLayout(LayoutKind.Sequential)]
		public struct PROCESS_BASIC_INFORMATION
		{
			public IntPtr Reserved1;
			public IntPtr PebBaseAddress;
			public IntPtr Reserved2_0;
			public IntPtr Reserved2_1;
			public IntPtr UniqueProcessId;
			public IntPtr InheritedFromUniqueProcessId;
		}

		public enum PROCESSINFOCLASS
		{
			ProcessBasicInformation = 0x00,
			ProcessDebugPort = 0x07,
			ProcessDebugObjectHandle = 0x1e
		}

		public enum THREADINFOCLASS
		{
			ThreadHideFromDebugger = 0x11
		}

		internal static IntPtr GetProcAddress(IntPtr module, object proc_name)
		{
			if (module == IntPtr.Zero || proc_name == null)
				return IntPtr.Zero;

			unsafe
			{
				byte* ptr = (byte*)module.ToPointer();

				PE.IMAGE_DOS_HEADER* dos_header = (PE.IMAGE_DOS_HEADER*)ptr;
				if (dos_header->e_magic != PE.IMAGE_DOS_SIGNATURE)
					return IntPtr.Zero;

				PE.IMAGE_NT_HEADERS* pe_header = (PE.IMAGE_NT_HEADERS*)(ptr + dos_header->e_lfanew);
				if (pe_header->Signature != PE.IMAGE_NT_SIGNATURE)
					return IntPtr.Zero;

				if (pe_header->FileHeader.SizeOfOptionalHeader == 0)
					return IntPtr.Zero;

				PE.IMAGE_DATA_DIRECTORY* export_table;
				byte* optional_header = (byte*)pe_header + Marshal.SizeOf(typeof(PE.IMAGE_NT_HEADERS));
				switch (*(UInt16*)(optional_header))
				{
					case PE.IMAGE_NT_OPTIONAL_HDR32_MAGIC:
						export_table = &((PE.IMAGE_OPTIONAL_HEADER32*)(optional_header))->ExportTable;
						break;
					case PE.IMAGE_NT_OPTIONAL_HDR64_MAGIC:
						export_table = &((PE.IMAGE_OPTIONAL_HEADER64*)(optional_header))->ExportTable;
						break;
					default:
						return IntPtr.Zero;
				}

				if (export_table->VirtualAddress == 0)
					return IntPtr.Zero;

				uint address = 0;
				PE.IMAGE_EXPORT_DIRECTORY* export_directory = (PE.IMAGE_EXPORT_DIRECTORY*)(ptr + export_table->VirtualAddress);
				if (proc_name is string)
				{
					string name = (string)proc_name;
					if (export_directory->NumberOfNames > 0)
					{
						int left_index = 0;
						int right_index = (int)export_directory->NumberOfNames - 1;
						uint* names = (UInt32*)(ptr + export_directory->AddressOfNames);
						while (left_index <= right_index)
						{
							int cur_index = (left_index + right_index) >> 1;
							int cmp = String.CompareOrdinal(name, Marshal.PtrToStringAnsi(new IntPtr(module.ToInt64() + names[cur_index])));
							if (cmp < 0)
								right_index = cur_index - 1;
							else if (cmp > 0)
								left_index = cur_index + 1;
							else
							{
								uint ordinal_index = ((UInt16*)(ptr + export_directory->AddressOfNameOrdinals))[cur_index];
								if (ordinal_index < export_directory->NumberOfFunctions)
									address = ((UInt32*)(ptr + export_directory->AddressOfFunctions))[ordinal_index];
								break;
							}
						}
					}
				}
				else
				{
					uint ordinal_index = System.Convert.ToUInt32(proc_name) - export_directory->Base;
					if (ordinal_index < export_directory->NumberOfFunctions)
						address = ((UInt32*)(ptr + export_directory->AddressOfFunctions))[ordinal_index];
				}

				if (address == 0)
					return IntPtr.Zero;

				if (address < export_table->VirtualAddress || address >= export_table->VirtualAddress + export_table->Size)
					return new IntPtr(module.ToInt64() + address);

				string forwarded_name = Marshal.PtrToStringAnsi(new IntPtr(module.ToInt64() + address));
				int dot_pos = forwarded_name.IndexOf('.');
				if (dot_pos > 0)
				{
					IntPtr forwarded_module = Win32.GetModuleHandle(forwarded_name.Substring(0, dot_pos));
					if (forwarded_name[dot_pos + 1] == '#')
					{
						uint ordinal;
						if (UInt32.TryParse(forwarded_name.Substring(dot_pos + 2), out ordinal))
							return GetProcAddress(forwarded_module, ordinal);
					}
					else
						return GetProcAddress(forwarded_module, forwarded_name.Substring(dot_pos + 1));
				}
			}

			return IntPtr.Zero;
		}

		internal static IntPtr GetProcAddressEnc(IntPtr module, uint position)
		{
			long instance = Marshal.GetHINSTANCE(typeof(Loader).Module).ToInt64();
			var buffer = new byte[100];
			for (var pos = 0; pos < buffer.Length; pos++)
			{
				var c = (byte)(Marshal.ReadByte(new IntPtr(instance + position + pos)) ^ (BitRotate.Left((uint)Faces.STRING_DECRYPT_KEY, pos) + pos));
				if (c == 0)
				{
					if (pos > 0)
						return GetProcAddress(module, Encoding.ASCII.GetString(buffer, 0, pos));
					break;
				}

				buffer[pos] = c;
			}

			return IntPtr.Zero;
		}

		public static uint NtQueryInformationProcess(IntPtr processHandle, Win32.PROCESSINFOCLASS processInformationClass,
			out object processInformation, int processInformationLength, out uint returnLength)
		{
			IntPtr mem = Marshal.AllocHGlobal(processInformationLength);
			uint res = _nt_query_information_process(processHandle, processInformationClass, mem, processInformationLength, out returnLength);
			if (res == 0)
			{
				Type type;
				switch (processInformationClass)
				{
					case Win32.PROCESSINFOCLASS.ProcessBasicInformation:
						type = typeof(Win32.PROCESS_BASIC_INFORMATION);
						break;
					case Win32.PROCESSINFOCLASS.ProcessDebugPort:
					case Win32.PROCESSINFOCLASS.ProcessDebugObjectHandle:
						type = typeof(IntPtr);
						break;
					default:
						type = null;
						break;
				}
				processInformation = Marshal.PtrToStructure(mem, type);
			}
			else
				processInformation = null;

			Marshal.FreeHGlobal(mem);
			return res;
		}

		public static uint NtSetInformationThread(IntPtr ThreadHandle, Win32.THREADINFOCLASS ThreadInformationClass, IntPtr ThreadInformation, uint ThreadInformationLength)
		{
			return _nt_set_information_thread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
		}

		public static uint EnumSystemFirmwareTables(uint firmwareTableProviderSignature, IntPtr firmwareTableBuffer, uint bufferSize)
		{
			return _enum_system_firmware_tables(firmwareTableProviderSignature, firmwareTableBuffer, bufferSize);
		}

		public static uint GetSystemFirmwareTable(uint firmwareTableProviderSignature, uint firmwareTableID, IntPtr firmwareTableBuffer, uint bufferSize)
		{
			return _get_system_firmware_table(firmwareTableProviderSignature, firmwareTableID, firmwareTableBuffer, bufferSize);
		}

		public static bool CloseHandle(IntPtr handle)
		{
			return (_nt_close(handle) == 0);
		}

		public static IntPtr OpenFile(String FileName, uint DesiredAccess, uint ShareMode)
		{
			char[] preffix;
			if (FileName[0] == '\\' && FileName[1] == '\\')
			{
				preffix = new char[7];

				preffix[0] = '\\';
				preffix[1] = '?';
				preffix[2] = '?';
				preffix[3] = '\\';
				preffix[4] = 'U';
				preffix[5] = 'N';
				preffix[6] = 'C';
			}
			else
			{
				preffix = new char[4];

				preffix[0] = '\\';
				preffix[1] = '?';
				preffix[2] = '?';
				preffix[3] = '\\';
			}

			var str = new UNICODE_STRING(new string(preffix) + FileName);
			var objectAttributes = new OBJECT_ATTRIBUTES(str, 0);
			try
			{
				IntPtr res;
				IO_STATUS_BLOCK io_status_block;
				if (_nt_open_file(out res, DesiredAccess | (uint)Values.SYNCHRONIZE | (uint)Values.FILE_READ_ATTRIBUTES, ref objectAttributes, out io_status_block, ShareMode, (uint)Values.FILE_SYNCHRONOUS_IO_NONALERT | (uint)Values.FILE_NON_DIRECTORY_FILE) == 0)
					return res;
			}
			finally
			{
				str.Dispose();
				objectAttributes.Dispose();
			}

			return InvalidHandleValue;
		}

		public static bool VirtualProtect(IntPtr BaseAddress, UIntPtr RegionSize, MemoryProtection Protect, out MemoryProtection OldProtect)
		{
			return (_nt_protect_virtual_memory(CurrentProcess, ref BaseAddress, ref RegionSize, Protect, out OldProtect) == 0);
		}

		public static IntPtr VirtualAlloc(IntPtr BaseAddress, UIntPtr RegionSize, AllocationType AllocationType, MemoryProtection Protect)
		{
			if (_nt_allocate_virtual_memory(CurrentProcess, ref BaseAddress, IntPtr.Zero, ref RegionSize, AllocationType, Protect) == 0)
				return BaseAddress;
			return IntPtr.Zero;
		}

		public static bool VirtualFree(IntPtr BaseAddress, UIntPtr RegionSize, FreeType FreeType)
		{
			return (_nt_free_virtual_memory(CurrentProcess, ref BaseAddress, ref RegionSize, FreeType) == 0);
		}

		public static IntPtr CreateFileMapping(IntPtr FileHandle, IntPtr Attributes, MemoryProtection Protect, uint MaximumSizeLow, uint MaximumSizeHigh, String Name)
		{
			uint desiredAccess = (uint)Values.SECTION_MAP_READ;
			if (Protect == MemoryProtection.ReadWrite)
				desiredAccess |= (uint)Values.SECTION_MAP_WRITE;
			else if (Protect == MemoryProtection.ExecuteReadWrite)
				desiredAccess |= (uint)Values.SECTION_MAP_WRITE | (uint)Values.SECTION_MAP_EXECUTE;
			else if (Protect == MemoryProtection.ExecuteRead)
				desiredAccess |= (uint)Values.SECTION_MAP_EXECUTE;

			IntPtr res;
			var str = new UNICODE_STRING(null);
			var objectAttributes = new OBJECT_ATTRIBUTES(str, 0);
			try
			{
				var size = new LARGE_INTEGER();
				size.LowPart = MaximumSizeLow;
				size.HighPart = MaximumSizeHigh;
				if (_nt_create_section(out res, desiredAccess, ref objectAttributes, ref size, Protect, (uint)Values.SEC_COMMIT, FileHandle) == 0)
					return res;
			}
			finally
			{
				str.Dispose();
				objectAttributes.Dispose();
			}

			return NullHandle;
		}

		public static IntPtr MapViewOfFile(IntPtr hFileMappingObject, MapAccess dwDesiredAccess, int dwFileOffsetHigh, int dwFileOffsetLow, IntPtr dwNumBytesToMap)
		{
			return _map_view_of_file(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumBytesToMap);
		}

		public static bool UnmapViewOfFile(IntPtr BaseAddress)
		{
			return (_nt_unmap_view_of_section(CurrentProcess, BaseAddress) == 0);
		}

		public static bool IsDebuggerPresent()
		{
			object obj;
			if (NtQueryInformationProcess(CurrentProcess, PROCESSINFOCLASS.ProcessBasicInformation, out obj, Marshal.SizeOf(typeof(PROCESS_BASIC_INFORMATION)), out _) == 0)
			{
				if (Marshal.ReadByte(new IntPtr(((PROCESS_BASIC_INFORMATION)obj).PebBaseAddress.ToInt64() + 2)) != 0) // PEB.BeingDebugged
					return true;
			}
			return false;
		}

		public static bool CheckRemoteDebuggerPresent()
		{
			object obj;
			if (NtQueryInformationProcess(CurrentProcess, PROCESSINFOCLASS.ProcessDebugPort, out obj, IntPtr.Size, out _) == 0)
			{
				if ((IntPtr)obj != IntPtr.Zero)
					return true;
			}

			if (NtQueryInformationProcess(CurrentProcess, PROCESSINFOCLASS.ProcessDebugObjectHandle, out _, IntPtr.Size, out _) == 0)
				return true;

			return false;
		}

		public static void ShowMessage(string Text, string Caption, MessageBoxButtons Buttons, MessageBoxIcon Icon)
		{
			IntPtr[] p = new IntPtr[4];
			var strText = new UNICODE_STRING(Text);
			var strCaption = new UNICODE_STRING(Caption);
			p[0] = Marshal.AllocHGlobal(Marshal.SizeOf(strText));
			p[1] = Marshal.AllocHGlobal(Marshal.SizeOf(strCaption));
			try
			{
				Marshal.StructureToPtr(strText, p[0], false);
				Marshal.StructureToPtr(strCaption, p[1], false);
				p[2] = new IntPtr((uint)Buttons | (uint)Icon);
				p[3] = new IntPtr(-1);

				_nt_raise_hard_error(0x40000018 | 0x10000000, 4, 3, p, 0, out _);
			}
			finally
			{
				strText.Dispose();
				strCaption.Dispose();
				Marshal.FreeHGlobal(p[0]);
				Marshal.FreeHGlobal(p[1]);
			}
		}
	}

	internal class PE
	{
		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_DOS_HEADER
		{
			public UInt16 e_magic;
			public UInt16 e_cblp;
			public UInt16 e_cp;
			public UInt16 e_crlc;
			public UInt16 e_cparhdr;
			public UInt16 e_minalloc;
			public UInt16 e_maxalloc;
			public UInt16 e_ss;
			public UInt16 e_sp;
			public UInt16 e_csum;
			public UInt16 e_ip;
			public UInt16 e_cs;
			public UInt16 e_lfarlc;
			public UInt16 e_ovno;
			public UInt16 e_res_0;
			public UInt16 e_res_1;
			public UInt16 e_res_2;
			public UInt16 e_res_3;
			public UInt16 e_oemid;
			public UInt16 e_oeminfo;
			public UInt16 e_res2_0;
			public UInt16 e_res2_1;
			public UInt16 e_res2_2;
			public UInt16 e_res2_3;
			public UInt16 e_res2_4;
			public UInt16 e_res2_5;
			public UInt16 e_res2_6;
			public UInt16 e_res2_7;
			public UInt16 e_res2_8;
			public UInt16 e_res2_9;
			public UInt32 e_lfanew;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_FILE_HEADER
		{
			public UInt16 Machine;
			public UInt16 NumberOfSections;
			public UInt32 TimeDateStamp;
			public UInt32 PointerToSymbolTable;
			public UInt32 NumberOfSymbols;
			public UInt16 SizeOfOptionalHeader;
			public UInt16 Characteristics;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_NT_HEADERS
		{
			public UInt32 Signature;
			public IMAGE_FILE_HEADER FileHeader;
			//IMAGE_OPTIONAL_HEADER OptionalHeader;
		}

		public const UInt16 IMAGE_DOS_SIGNATURE = 0x5A4D;
		public const UInt32 IMAGE_NT_SIGNATURE = 0x00004550;
		public const UInt16 IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b;
		public const UInt16 IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b;

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_DATA_DIRECTORY
		{
			public UInt32 VirtualAddress;
			public UInt32 Size;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_OPTIONAL_HEADER32
		{
			public UInt16 Magic;
			public Byte MajorLinkerVersion;
			public Byte MinorLinkerVersion;
			public UInt32 SizeOfCode;
			public UInt32 SizeOfInitializedData;
			public UInt32 SizeOfUninitializedData;
			public UInt32 AddressOfEntryPoint;
			public UInt32 BaseOfCode;
			public UInt32 BaseOfData;
			public UInt32 ImageBase;
			public UInt32 SectionAlignment;
			public UInt32 FileAlignment;
			public UInt16 MajorOperatingSystemVersion;
			public UInt16 MinorOperatingSystemVersion;
			public UInt16 MajorImageVersion;
			public UInt16 MinorImageVersion;
			public UInt16 MajorSubsystemVersion;
			public UInt16 MinorSubsystemVersion;
			public UInt32 Win32VersionValue;
			public UInt32 SizeOfImage;
			public UInt32 SizeOfHeaders;
			public UInt32 CheckSum;
			public UInt16 Subsystem;
			public UInt16 DllCharacteristics;
			public UInt32 SizeOfStackReserve;
			public UInt32 SizeOfStackCommit;
			public UInt32 SizeOfHeapReserve;
			public UInt32 SizeOfHeapCommit;
			public UInt32 LoaderFlags;
			public UInt32 NumberOfRvaAndSizes;

			public IMAGE_DATA_DIRECTORY ExportTable;
			public IMAGE_DATA_DIRECTORY ImportTable;
			public IMAGE_DATA_DIRECTORY ResourceTable;
			public IMAGE_DATA_DIRECTORY ExceptionTable;
			public IMAGE_DATA_DIRECTORY CertificateTable;
			public IMAGE_DATA_DIRECTORY BaseRelocationTable;
			public IMAGE_DATA_DIRECTORY Debug;
			public IMAGE_DATA_DIRECTORY Architecture;
			public IMAGE_DATA_DIRECTORY GlobalPtr;
			public IMAGE_DATA_DIRECTORY TLSTable;
			public IMAGE_DATA_DIRECTORY LoadConfigTable;
			public IMAGE_DATA_DIRECTORY BoundImport;
			public IMAGE_DATA_DIRECTORY IAT;
			public IMAGE_DATA_DIRECTORY DelayImportDescriptor;
			public IMAGE_DATA_DIRECTORY CLRRuntimeHeader;
			public IMAGE_DATA_DIRECTORY Reserved;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct IMAGE_OPTIONAL_HEADER64
		{
			public UInt16 Magic;
			public Byte MajorLinkerVersion;
			public Byte MinorLinkerVersion;
			public UInt32 SizeOfCode;
			public UInt32 SizeOfInitializedData;
			public UInt32 SizeOfUninitializedData;
			public UInt32 AddressOfEntryPoint;
			public UInt32 BaseOfCode;
			public UInt64 ImageBase;
			public UInt32 SectionAlignment;
			public UInt32 FileAlignment;
			public UInt16 MajorOperatingSystemVersion;
			public UInt16 MinorOperatingSystemVersion;
			public UInt16 MajorImageVersion;
			public UInt16 MinorImageVersion;
			public UInt16 MajorSubsystemVersion;
			public UInt16 MinorSubsystemVersion;
			public UInt32 Win32VersionValue;
			public UInt32 SizeOfImage;
			public UInt32 SizeOfHeaders;
			public UInt32 CheckSum;
			public UInt16 Subsystem;
			public UInt16 DllCharacteristics;
			public UInt64 SizeOfStackReserve;
			public UInt64 SizeOfStackCommit;
			public UInt64 SizeOfHeapReserve;
			public UInt64 SizeOfHeapCommit;
			public UInt32 LoaderFlags;
			public UInt32 NumberOfRvaAndSizes;

			public IMAGE_DATA_DIRECTORY ExportTable;
			public IMAGE_DATA_DIRECTORY ImportTable;
			public IMAGE_DATA_DIRECTORY ResourceTable;
			public IMAGE_DATA_DIRECTORY ExceptionTable;
			public IMAGE_DATA_DIRECTORY CertificateTable;
			public IMAGE_DATA_DIRECTORY BaseRelocationTable;
			public IMAGE_DATA_DIRECTORY Debug;
			public IMAGE_DATA_DIRECTORY Architecture;
			public IMAGE_DATA_DIRECTORY GlobalPtr;
			public IMAGE_DATA_DIRECTORY TLSTable;
			public IMAGE_DATA_DIRECTORY LoadConfigTable;
			public IMAGE_DATA_DIRECTORY BoundImport;
			public IMAGE_DATA_DIRECTORY IAT;
			public IMAGE_DATA_DIRECTORY DelayImportDescriptor;
			public IMAGE_DATA_DIRECTORY CLRRuntimeHeader;
			public IMAGE_DATA_DIRECTORY Reserved;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct IMAGE_EXPORT_DIRECTORY
		{
			public UInt32 Characteristics;
			public UInt32 TimeDateStamp;
			public UInt16 MajorVersion;
			public UInt16 MinorVersion;
			public UInt32 Name;
			public UInt32 Base;
			public UInt32 NumberOfFunctions;
			public UInt32 NumberOfNames;
			public UInt32 AddressOfFunctions;
			public UInt32 AddressOfNames;
			public UInt32 AddressOfNameOrdinals;
		}
	}
}