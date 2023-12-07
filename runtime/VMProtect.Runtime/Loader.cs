using System;
using System.IO;
using System.Text;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Diagnostics;
using System.Reflection.Emit;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	[VMProtect.DeleteOnCompilation]
	enum MessageType
	{
		InitializationError,
		ProcNotFound,
		OrdinalNotFound,
		FileCorrupted,
		DebuggerFound,
		UnregisteredVersion,
		VirtualMachineFound
	}

	[VMProtect.DeleteOnCompilation]
	enum ErrorType
	{
		VIRTUAL_PROTECT_ERROR = 1,
		UNPACKER_ERROR = 2
	}

	[VMProtect.DeleteOnCompilation]
	enum LoaderOption
	{
		CHECK_PATCH = 0x1,
		CHECK_DEBUGGER = 0x2,
		CHECK_KERNEL_DEBUGGER = 0x4,
		EXIT_PROCESS = 0x8,
		CHECK_VIRTUAL_MACHINE = 0x10
	}

	[VMProtect.DeleteOnCompilation]
	enum FixupType
	{
		Absolute = 0,
		High = 1,
		Low = 2,
		HighLow = 3,
		Dir64 = 10
	}

	public static class Loader
	{
		public static object[] IAT;

		internal static bool FindFirmwareVendor(byte[] data)
		{
			for (var i = 0; i < data.Length; i++)
			{
				if (i + 3 < data.Length && data[i + 0] == 'Q' && data[i + 1] == 'E' && data[i + 2] == 'M' && data[i + 3] == 'U')
					return true;
				if (i + 8 < data.Length && data[i + 0] == 'M' && data[i + 1] == 'i' && data[i + 2] == 'c' && data[i + 3] == 'r' && data[i + 4] == 'o' && data[i + 5] == 's' && data[i + 6] == 'o' && data[i + 7] == 'f' && data[i + 8] == 't')
					return true;
				if (i + 6 < data.Length && data[i + 0] == 'i' && data[i + 1] == 'n' && data[i + 2] == 'n' && data[i + 3] == 'o' && data[i + 4] == 't' && data[i + 5] == 'e' && data[i + 6] == 'k')
					return true;
				if (i + 9 < data.Length && data[i + 0] == 'V' && data[i + 1] == 'i' && data[i + 2] == 'r' && data[i + 3] == 't' && data[i + 4] == 'u' && data[i + 5] == 'a' && data[i + 6] == 'l' && data[i + 7] == 'B' && data[i + 8] == 'o' && data[i + 9] == 'x')
					return true;
				if (i + 5 < data.Length && data[i + 0] == 'V' && data[i + 1] == 'M' && data[i + 2] == 'w' && data[i + 3] == 'a' && data[i + 4] == 'r' && data[i + 5] == 'e')
					return true;
				if (i + 8 < data.Length && data[i + 0] == 'P' && data[i + 1] == 'a' && data[i + 2] == 'r' && data[i + 3] == 'a' && data[i + 4] == 'l' && data[i + 5] == 'l' && data[i + 6] == 'e' && data[i + 7] == 'l' && data[i + 8] == 's')
					return true;
			}
			return false;
		}

		private static void ShowMessage(uint type, uint param = 0)
		{
			uint position;
			switch (type)
			{
				case (uint)MessageType.DebuggerFound:
					position = (uint)Faces.DEBUGGER_FOUND;
					break;
				case (uint)MessageType.VirtualMachineFound:
					position = (uint)Faces.VIRTUAL_MACHINE_FOUND;
					break;
				case (uint)MessageType.FileCorrupted:
					position = (uint)Faces.FILE_CORRUPTED;
					break;
				case (uint)MessageType.UnregisteredVersion:
					position = (uint)Faces.UNREGISTERED_VERSION;
					break;
				case (uint)MessageType.InitializationError:
					position = (uint)Faces.INITIALIZATION_ERROR;
					break;
				default:
					return;
			}

			long instance = Marshal.GetHINSTANCE(typeof(Loader).Module).ToInt64();
			String message = null;
			var buffer = new byte[1024];
			for (var pos = 0; pos < 512; pos++)
			{
				var c = (ushort)(Marshal.ReadInt16(new IntPtr(instance + position + pos * 2)) ^ (BitRotate.Left((uint)Faces.STRING_DECRYPT_KEY, pos) + pos));
				if (c == 0)
				{
					if (pos > 0)
						message = Encoding.Unicode.GetString(buffer, 0, pos * 2);
					break;
				}

				buffer[pos * 2] = (byte)c;
				buffer[pos * 2 + 1] = (byte)(c >> 8);
			}
			if (message != null)
			{
				if (type == (uint)MessageType.InitializationError)
					message = String.Format(message, param);
				Win32.ShowMessage(message, Assembly.GetExecutingAssembly().GetName().Name, MessageBoxButtons.OK, type == (uint)MessageType.UnregisteredVersion ? MessageBoxIcon.Warning : MessageBoxIcon.Error);
			}
		}

		internal static bool LzmaDecode(IntPtr dst, IntPtr src, byte[] properties, ref uint dstSize, out uint srcSize)
		{
			srcSize = 0;
			unsafe
			{
				using (
					var srcStream = new UnmanagedMemoryStream((byte*)src.ToPointer(), int.MaxValue, int.MaxValue, FileAccess.Read)
					)
				{
					using (
						var dstStream = new UnmanagedMemoryStream((byte*) dst.ToPointer(), dstSize, dstSize,
							FileAccess.Write))
					{
						try
						{
							var coder = new SevenZip.Compression.LZMA.Decoder();
							coder.SetDecoderProperties(properties);
							coder.Code(srcStream, dstStream, dstSize);
							dstStream.Flush();

							srcSize = (uint)srcStream.Position;
							dstSize = (uint)dstStream.Position;
						}
						catch (Exception)
						{
							return false;
						}

						return true;
					}
				}
			}
		}

		public static void Main()
		{
			if (GlobalData.LoaderStatus() != 0)
				return;

			var module = typeof(Loader).Module;
			long instance = Marshal.GetHINSTANCE(module).ToInt64();
			var options = (uint)Faces.LOADER_OPTIONS;
			GlobalData.SetIsPatchDetected(false);
			GlobalData.SetIsDebuggerDetected(false);
			GlobalData.SetLoaderCrcInfo((uint)Faces.LOADER_CRC_INFO);
			GlobalData.SetLoaderCrcSize((uint)Faces.LOADER_CRC_INFO_SIZE);
			GlobalData.SetLoaderCrcHash((uint)Faces.LOADER_CRC_INFO_HASH);
			GlobalData.SetServerDate(0);

			// detect a debugger
			if ((options & (uint)LoaderOption.CHECK_DEBUGGER) != 0)
			{
				if (Debugger.IsAttached || Debugger.IsLogging())
				{
					ShowMessage((uint)MessageType.DebuggerFound);
					Environment.Exit(1);
					return;
				}

				if (Win32.IsDebuggerPresent() || Win32.CheckRemoteDebuggerPresent())
				{
					ShowMessage((uint)MessageType.DebuggerFound);
					Environment.Exit(1);
					return;
				}

				Win32.NtSetInformationThread(Win32.CurrentThread, Win32.THREADINFOCLASS.ThreadHideFromDebugger, IntPtr.Zero, 0);
			}

			// check header and loader CRC
			var crcSize = (uint)Faces.LOADER_CRC_INFO_SIZE;
			if (crcSize != 0)
			{
				crcSize = (uint)Marshal.ReadInt32(new IntPtr(instance + crcSize));
				var crcPosition = (uint)Faces.LOADER_CRC_INFO;
				var crcCryptor = new CRCValueCryptor();
				bool isValid = true;
				var crcInfo = new byte[12];
				var crc32 = new CRC32();
				var crcHash = (uint)Marshal.ReadInt32(new IntPtr(instance + (uint)Faces.LOADER_CRC_INFO_HASH));
				if (crcHash != crc32.Hash(new IntPtr(instance + crcPosition), crcSize))
					isValid = false;
				for (var i = 0; i < crcSize; i += crcInfo.Length)
				{
					Marshal.Copy(new IntPtr(instance + crcPosition + i), crcInfo, 0, crcInfo.Length);
					uint address = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 0));
					uint size = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 4));
					uint hash = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 8));

					if (crc32.Hash(new IntPtr(instance + address), size) != hash)
						isValid = false;
				}

				if (!isValid)
				{
					if ((options & (uint)LoaderOption.CHECK_PATCH) != 0)
					{
						ShowMessage((uint)MessageType.FileCorrupted);
						Environment.Exit(1);
						return;
					}
					GlobalData.SetIsPatchDetected(true);
				}
			}

			// check file CRC
			crcSize = (uint)Faces.FILE_CRC_INFO_SIZE;
			if (crcSize != 0) //-V3022
			{
				crcSize = (uint)Marshal.ReadInt32(new IntPtr(instance + crcSize));
				var crcPosition = (uint)Faces.FILE_CRC_INFO;
				bool isValid = true;
				var file = Win32.OpenFile(Assembly.GetExecutingAssembly().Location, (uint)Win32.Values.GENERIC_READ, (uint)Win32.Values.FILE_SHARE_READ | (uint)Win32.Values.FILE_SHARE_WRITE);
				if (file != Win32.InvalidHandleValue)
				{
					var fileSize = Win32.GetFileSize(file, IntPtr.Zero);
					if (fileSize < (uint)Marshal.ReadInt32(new IntPtr(instance + crcPosition)))
						isValid = false;
					else
					{
						var map = Win32.CreateFileMapping(file, IntPtr.Zero, Win32.MemoryProtection.ReadOnly, 0, 0, null);
						if (map != Win32.NullHandle)
						{
							IntPtr baseAddress = Win32.MapViewOfFile(map, Win32.MapAccess.Read, 0, 0, IntPtr.Zero);
							if (baseAddress != IntPtr.Zero)
							{
								var crcInfo = new byte[12];
								var crc32 = new CRC32();
								var crcCryptor = new CRCValueCryptor();
								for (var i = 4; i < crcSize; i += crcInfo.Length)
								{
									Marshal.Copy(new IntPtr(instance + crcPosition + i), crcInfo, 0, crcInfo.Length);
									uint address = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 0));
									uint size = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 4));
									uint hash = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 8));

									if (crc32.Hash(new IntPtr(baseAddress.ToInt64() + address), size) != hash)
										isValid = false;
								}
								Win32.UnmapViewOfFile(baseAddress);
							}
							Win32.CloseHandle(map);
						}
					}
					Win32.CloseHandle(file);
				}

				if (!isValid)
				{
					if ((options & (uint)LoaderOption.CHECK_PATCH) != 0)
					{
						ShowMessage((uint)MessageType.FileCorrupted);
						Environment.Exit(1);
						return;
					}
					GlobalData.SetIsPatchDetected(true);
				}
			}

			// setup WRITABLE flag for memory pages
			var sectionSize = (uint)Faces.SECTION_INFO_SIZE;
			if (sectionSize != 0)
			{
				var sectionPos = (uint)Faces.SECTION_INFO;
				var sectionInfo = new byte[12];
				for (var i = 0; i < sectionSize; i += sectionInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + sectionPos + i), sectionInfo, 0, sectionInfo.Length);
					uint address = BitConverter.ToUInt32(sectionInfo, 0);
					uint size = BitConverter.ToUInt32(sectionInfo, 4);
					uint type = BitConverter.ToUInt32(sectionInfo, 8);

					Win32.MemoryProtection protect, old_protect;
					protect = ((type & (uint)Win32.SectionType.Execute) != 0) ? Win32.MemoryProtection.ExecuteReadWrite : Win32.MemoryProtection.ReadWrite;
					if (!Win32.VirtualProtect(new IntPtr(instance + address), new UIntPtr(size), protect, out old_protect))
					{
						ShowMessage((uint)MessageType.InitializationError, (uint)ErrorType.VIRTUAL_PROTECT_ERROR);
						Environment.Exit(1);
						return;
					}
					if (((uint)old_protect & ((uint)Win32.MemoryProtection.NoAccess | (uint)Win32.MemoryProtection.Guard)) != 0)
					{
						if ((options & (uint)LoaderOption.CHECK_DEBUGGER) != 0)
						{
							ShowMessage((uint)MessageType.DebuggerFound);
							Environment.Exit(1);
							return;
						}
						GlobalData.SetIsDebuggerDetected(true);
					}
				}
			}

			// unpack regions
			var packerSize = (uint)Faces.PACKER_INFO_SIZE;
			if (packerSize != 0)
			{
				int tlsIndex = 0;
				var tlsIndexPos = (uint)Faces.TLS_INDEX_INFO;
				if (tlsIndexPos != 0)
					tlsIndex = Marshal.ReadInt32(new IntPtr(instance + tlsIndexPos));

				var packerPos = (uint)Faces.PACKER_INFO;
				var packerInfo = new byte[8];
				Marshal.Copy(new IntPtr(instance + packerPos), packerInfo, 0, packerInfo.Length);
				uint src = BitConverter.ToUInt32(packerInfo, 0);
				uint dst = BitConverter.ToUInt32(packerInfo, 4);
				var properties = new byte[dst];
				Marshal.Copy(new IntPtr(instance + src), properties, 0, properties.Length);
				for (var i = packerInfo.Length; i < packerSize; i += packerInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + packerPos + i), packerInfo, 0, packerInfo.Length);
					src = BitConverter.ToUInt32(packerInfo, 0);
					dst = BitConverter.ToUInt32(packerInfo, 4);
					uint dstSize = int.MaxValue, srcSize;

					if (!LzmaDecode(new IntPtr(instance + dst), new IntPtr(instance + src), properties, ref dstSize, out srcSize))
					{
						ShowMessage((uint)MessageType.InitializationError, (uint)ErrorType.UNPACKER_ERROR);
						Environment.Exit(1);
						return;
					}
				}

				if (tlsIndexPos != 0)
					Marshal.WriteInt32(new IntPtr(instance + tlsIndexPos), tlsIndex);
			}

			// setup fixups
			long fileBase = (uint)Faces.FILE_BASE;
			long deltaBase = instance - fileBase;
			if (deltaBase != 0)
			{
				var fixupSize = (uint)Faces.FIXUP_INFO_SIZE;
				if (fixupSize != 0)
				{
					var fixupPos = (uint)Faces.FIXUP_INFO;
					var fixupInfo = new byte[8];
					uint blockSize = 0;
					for (uint i = 0; i < fixupSize; i += blockSize)
					{
						Marshal.Copy(new IntPtr(instance + fixupPos + i), fixupInfo, 0, fixupInfo.Length);
						uint address = BitConverter.ToUInt32(fixupInfo, 0);
						blockSize = BitConverter.ToUInt32(fixupInfo, 4);
						if (blockSize < fixupInfo.Length)
							break;

						var c = blockSize - fixupInfo.Length;
						var block = new byte[c];
						Marshal.Copy(new IntPtr(instance + fixupPos + i + fixupInfo.Length), block, 0, block.Length);
						for (var j = 0; j < c; j += 2)
						{
							ushort typeOffset = BitConverter.ToUInt16(block, j);
							var type = (typeOffset & 0xf);
							var ptr = new IntPtr(instance + address + (typeOffset >> 4));

							if (type == (uint)FixupType.HighLow)
							{
								int value = Marshal.ReadInt32(ptr);
								value += (int)deltaBase;
								Marshal.WriteInt32(ptr, value);
							}
							else if (type == (uint)FixupType.Dir64)
							{
								long value = Marshal.ReadInt64(ptr);
								value += deltaBase;
								Marshal.WriteInt64(ptr, value);
							}
							else if (type == (uint)FixupType.High)
							{
								short value = Marshal.ReadInt16(ptr);
								value += (short)(deltaBase >> 16);
								Marshal.WriteInt16(ptr, value);
							}
							else if (type == (uint)FixupType.Low)
							{
								short value = Marshal.ReadInt16(ptr);
								value += (short)deltaBase;
								Marshal.WriteInt16(ptr, value);
							}
						}
					}
				}
			}

			// setup IAT
			var iatSize = (uint)Faces.IAT_INFO_SIZE;
			if (iatSize != 0)
			{
				var iatPos = (uint)Faces.IAT_INFO;
				var iatInfo = new byte[12];
				for (var i = 0; i < iatSize; i += iatInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + iatPos + i), iatInfo, 0, iatInfo.Length);
					uint src = BitConverter.ToUInt32(iatInfo, 0);
					uint dst = BitConverter.ToUInt32(iatInfo, 4);
					uint size = BitConverter.ToUInt32(iatInfo, 8);

					var data = new byte[size];
					Marshal.Copy(new IntPtr(instance + src), data, 0, data.Length);
					Marshal.Copy(data, 0, new IntPtr(instance + dst), data.Length);
				}
			}

			// reset WRITABLE flag for memory pages
			if (sectionSize != 0) //-V3022
			{
				var sectionPos = (uint)Faces.SECTION_INFO;
				var sectionInfo = new byte[12];
				for (var i = 0; i < sectionSize; i += sectionInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + sectionPos + i), sectionInfo, 0, sectionInfo.Length);
					uint address = BitConverter.ToUInt32(sectionInfo, 0);
					uint size = BitConverter.ToUInt32(sectionInfo, 4);
					uint type = BitConverter.ToUInt32(sectionInfo, 8);

					Win32.MemoryProtection protect, old_protect;
					if ((type & (uint)Win32.SectionType.Read) != 0)
					{
						if ((type & (uint)Win32.SectionType.Write) != 0)
							protect = ((type & (uint)Win32.SectionType.Execute) != 0) ? Win32.MemoryProtection.ExecuteReadWrite : Win32.MemoryProtection.ReadWrite;
						else
							protect = ((type & (uint)Win32.SectionType.Execute) != 0) ? Win32.MemoryProtection.ExecuteRead : Win32.MemoryProtection.ReadOnly;
					}
					else
					{
						protect = ((type & (uint)Win32.SectionType.Execute) != 0) ? Win32.MemoryProtection.Execute : Win32.MemoryProtection.NoAccess;
					}
					if (!Win32.VirtualProtect(new IntPtr(instance + address), new UIntPtr(size), protect, out old_protect))
					{
						ShowMessage((uint)MessageType.InitializationError, (uint)ErrorType.VIRTUAL_PROTECT_ERROR);
						Environment.Exit(1);
					}
					if (((uint)old_protect & ((uint)Win32.MemoryProtection.NoAccess | (uint)Win32.MemoryProtection.Guard)) != 0)
					{
						if ((options & (uint)LoaderOption.CHECK_DEBUGGER) != 0)
						{
							ShowMessage((uint)MessageType.DebuggerFound);
							Environment.Exit(1);
							return;
						}
						GlobalData.SetIsDebuggerDetected(true);
					}
				}
			}

			// detect a virtual machine
			if ((options & (uint)LoaderOption.CHECK_VIRTUAL_MACHINE) != 0)
			{
				var info = CpuId.Invoke(1);
				if (((info[2] >> 31) & 1) != 0)
				{
					// hypervisor found
					bool isFound = true;
					// check Hyper-V root partition
					info = CpuId.Invoke(0x40000000);
					if (info[1] == 0x7263694d && info[2] == 0x666f736f && info[3] == 0x76482074) // "Microsoft Hv"
					{
						info = CpuId.Invoke(0x40000003);
						if ((info[1] & 1) != 0)
							isFound = false;
					}
					if (isFound)
					{
						ShowMessage((uint)MessageType.VirtualMachineFound);
						Environment.Exit(1);
						return;
					}
				}
				else
				{
					int size;
					uint tableSignature = (byte)'R' << 24 | (byte)'S' << 16 | (byte)'M' << 8 | (byte)'B';
					try
					{
						size = (int)Win32.EnumSystemFirmwareTables(tableSignature, IntPtr.Zero, 0);
					}
					catch (EntryPointNotFoundException) { size = 0; }

					if (size > 0)
					{
						IntPtr nativeBuffer = Marshal.AllocHGlobal(size);
						Win32.EnumSystemFirmwareTables(tableSignature, nativeBuffer, (uint)size);
						byte[] buffer = new byte[size];
						Marshal.Copy(nativeBuffer, buffer, 0, size);
						Marshal.FreeHGlobal(nativeBuffer);
						for (var i = 0; i < size / sizeof(uint); i += sizeof(uint))
						{
							uint tableId = BitConverter.ToUInt32(buffer, i);
							int dataSize = (int)Win32.GetSystemFirmwareTable(tableSignature, tableId, IntPtr.Zero, 0);
							if (dataSize > 0)
							{
								nativeBuffer = Marshal.AllocHGlobal(dataSize);
								Win32.GetSystemFirmwareTable(tableSignature, tableId, nativeBuffer, (uint)dataSize);
								byte[] data = new byte[dataSize];
								Marshal.Copy(nativeBuffer, data, 0, dataSize);
								Marshal.FreeHGlobal(nativeBuffer);
								if (FindFirmwareVendor(data))
								{
									ShowMessage((uint)MessageType.VirtualMachineFound);
									Environment.Exit(1);
									return;
								}
							}
						}
					}
				}
			}

			// check memory CRC
			crcSize = (uint)Faces.MEMORY_CRC_INFO_SIZE;
			if (crcSize != 0) //-V3022
			{
				var crcPosition = (uint)Faces.MEMORY_CRC_INFO;
				var crcCryptor = new CRCValueCryptor();
				bool isValid = true;
				var crcInfo = new byte[12];
				var crc32 = new CRC32();
				var crcHash = (uint)Faces.MEMORY_CRC_INFO_HASH;
				if (crcHash != crc32.Hash(new IntPtr(instance + crcPosition), crcSize))
					isValid = false;
				for (var i = 0; i < crcSize; i += crcInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + crcPosition + i), crcInfo, 0, crcInfo.Length);
					uint address = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 0));
					uint size = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 4));
					uint hash = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 8));

					if (crc32.Hash(new IntPtr(instance + address), size) != hash)
						isValid = false;
				}

				if (!isValid)
				{
					if ((options & (uint)LoaderOption.CHECK_PATCH) != 0)
					{
						ShowMessage((uint)MessageType.FileCorrupted);
						Environment.Exit(1);
						return;
					}
					GlobalData.SetIsPatchDetected(true);
				}
			}

			GlobalData.SetSessionKey((uint)new Random().Next());

			if (!Core.Instance.Init(instance))
			{
				Environment.Exit(1);
				return;
			}

			// setup Import
			var importSize = (uint)Faces.IMPORT_INFO_SIZE;
			if (importSize != 0)
			{
				var importPos = (uint)Faces.IMPORT_INFO;
				var key = (uint)Faces.STRING_DECRYPT_KEY;
				var importInfo = new byte[12];
				object[] iat = new object[importSize / importInfo.Length];
				uint index = 0;
				for (var i = 0; i < importSize; i += importInfo.Length) //-V3022
				{
					Marshal.Copy(new IntPtr(instance + importPos + i), importInfo, 0, importInfo.Length);
					uint token = BitConverter.ToUInt32(importInfo, 0) ^ key;
					uint delegateToken = BitConverter.ToUInt32(importInfo, 4) ^ key;
					uint callType = BitConverter.ToUInt32(importInfo, 8);

					MethodBase method = null;
					FieldInfo field = null;
					try
					{
						if ((callType & 8) != 0)
							field = module.ResolveField((int)token);
						else
							method = module.ResolveMethod((int)token);
					}
					catch (Exception e)
					{
						Win32.ShowMessage(e.InnerException.Message, Assembly.GetExecutingAssembly().GetName().Name, MessageBoxButtons.OK, MessageBoxIcon.Error);
						Environment.Exit(1);
						return;
					}

					var invoke = (MethodInfo)module.ResolveMethod((int)delegateToken);
					var delegateType = invoke.DeclaringType;
					object delegateObject;
					if (callType == 0 && method.IsStatic)
						delegateObject = Delegate.CreateDelegate(delegateType, (MethodInfo)method);
					else
					{
						ParameterInfo[] parameters = invoke.GetParameters();
						Type[] paramTypes = new Type[parameters.Length];
						for (var j = 0; j < paramTypes.Length; j++)
							paramTypes[j] = parameters[j].ParameterType;

						Type declType = (method != null) ? method.DeclaringType : field.DeclaringType;
						DynamicMethod dynamicMethod = new DynamicMethod("", invoke.ReturnType, paramTypes, (declType.IsInterface || declType.IsArray) ? delegateType : declType, true);
						DynamicILInfo dynamicInfo = dynamicMethod.GetDynamicILInfo();
						dynamicInfo.SetLocalSignature(new byte[] { 0x7, 0x0 });

						if (method != null)
						{
							parameters = method.GetParameters();
							int s = 0;
							if (paramTypes.Length > parameters.Length)
							{
								paramTypes[0] = method.DeclaringType;
								s = 1;
							}
							for (var j = 0; j < parameters.Length; j++)
								paramTypes[s + j] = parameters[j].ParameterType;
						}
						else
						{
							if (paramTypes.Length > 0)
								paramTypes[0] = field.DeclaringType;
						}

						var code = new byte[7 * paramTypes.Length + 6];
						int codePos = 0;
						int dynamicToken;
						for (var j = 0; j < paramTypes.Length; j++)
						{
							code[codePos++] = 0x0e; // Ldarg_s
							code[codePos++] = (byte)j;

							Type type = paramTypes[j];
							if ((type.IsClass || type.IsInterface) && !type.IsPointer && !type.IsByRef)
							{
								code[codePos++] = 0x74; // Castclass
								dynamicToken = dynamicInfo.GetTokenFor(type.TypeHandle);
								code[codePos++] = (byte)dynamicToken;
								code[codePos++] = (byte)(dynamicToken >> 8);
								code[codePos++] = (byte)(dynamicToken >> 16);
								code[codePos++] = (byte)(dynamicToken >> 24);
							}
							else
								codePos += 5;
						}

						switch (callType)
						{
							case 1:
								code[codePos++] = 0x73;  // Newobj
								break;
							case 2:
								code[codePos++] = 0x6f;  // Callvirt
								break;
							case 8:
								code[codePos++] = 0x7e;  // Ldsfld
								break;
							case 9:
								code[codePos++] = 0x7b;  // Ldfld
								break;
							default:
								code[codePos++] = 0x28;  // Call
								break;
						}
						dynamicToken = (method != null) ? dynamicInfo.GetTokenFor(method.MethodHandle) : dynamicInfo.GetTokenFor(field.FieldHandle);
						code[codePos++] = (byte)dynamicToken;
						code[codePos++] = (byte)(dynamicToken >> 8);
						code[codePos++] = (byte)(dynamicToken >> 16);
						code[codePos++] = (byte)(dynamicToken >> 24);

						code[codePos] = 0x2a; // Ret
						dynamicInfo.SetCode(code, paramTypes.Length + 1);

						delegateObject = dynamicMethod.CreateDelegate(delegateType);
					}
					iat[index++] = delegateObject;
				}
				IAT = iat;
			}

			ShowMessage((uint)MessageType.UnregisteredVersion);

			GlobalData.SetLoaderStatus(1);
		}
	}
}