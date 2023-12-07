using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Diagnostics;
using System.Threading;
using System.Collections.Generic;
using System.Text;

// ReSharper disable MemberCanBePrivate.Global
// ReSharper disable UnusedParameter.Global
// ReSharper disable UnusedMember.Global
// ReSharper disable once CheckNamespace
[assembly: SuppressIldasmAttribute()]
[assembly: Obfuscation(Feature = "renaming", Exclude = true)]
namespace VMProtect
{
	/// <summary>
	/// Serial number status flags
	/// </summary>
	[Flags]
	public enum SerialState
	{
		/// <summary>
		/// No problems.
		/// </summary>
		Success			= 0x00000000,
		/// <summary>
		/// Licensing module is corrupted. This may be caused by cracking attempts. Usually you will never get this flag.
		/// </summary>
		Corrupted		= 0x00000001,
		/// <summary>
		/// Serial number is invalid. You will get this flag if licensing module will be unable to decrypt serial number or 
		/// if the serial number is not yet set or it is empty.
		/// </summary>
		Invalid			= 0x00000002,
		/// <summary>
		/// Serial number is correct, but it was blacklisted in VMProtect.
		/// </summary>
		Blacklisted		= 0x00000004,
		/// <summary>
		/// Serial number is expired. You will get this flag if serial number has expiration date chunk and this date is in the past. 
		/// You may read the expiration date by calling GetSerialNumberData() function.
		/// </summary>
		DateExpired		= 0x00000008,
		/// <summary>
		/// Running time limit for this session is over. You may read limit value by calling GetSerialNumberData() function.
		/// </summary>
		RunningTimeOver	= 0x00000010,
		/// <summary>
		/// Serial number is locked to another hardware identifier.
		/// </summary>
		BadHwid			= 0x00000020,
		/// <summary>
		/// Serial number will not work with this version of application, because it has "Max Build Date" block and the application was 
		/// build after those date. You may read maximal build date by calling GetSerialNumberData() function.
		/// </summary>
		MaxBuildExpired	= 0x00000040,
	};

	/// <summary>
	/// Serial number contents
	/// </summary>
	public class SerialNumberData
	{
		/// <summary>
		/// Serial number status
		/// </summary>
		public SerialState State;
		
		/// <summary>
		/// End user name
		/// </summary>
		public string UserName;
		
		/// <summary>
		/// End user E-Mail
		/// </summary>
		public string EMail;

		/// <summary>
		/// Serial number expiration date.
		/// </summary>
		public DateTime Expires;

		/// <summary>
		/// Max date of build, that will accept this key
		/// </summary>
		public DateTime MaxBuild;

		/// <summary>
		/// Running time in minutes
		/// </summary>
		public int RunningTime;

		/// <summary>
		/// Up to 255 bytes of user data
		/// </summary>
		public byte[] UserData;

		public SerialNumberData()
		{
			State = SerialState.Invalid;
			Expires = DateTime.MaxValue;
			MaxBuild = DateTime.MaxValue;
			RunningTime = 0;
			UserData = new byte[0];
			UserName = "";
			EMail = "";
		}
	};

	/// <summary>
	/// Activation API status codes
	/// </summary>
	public enum ActivationStatus
	{
		/// <summary>
		/// Activation successful, the serial field contains a serial number.
		/// </summary>
		Ok,
		/// <summary>
		/// The activation module was unable to connect the Web License Manager.
		/// </summary>
		NoConnection,
		/// <summary>
		/// The server returned unexpected reply. This means configuration problems or probably a cracking attempt.
		/// </summary>
		BadReply,
		/// <summary>
		/// The activation code is blocked on the server. This doesn't mean that the number of possible activations is exceeded,
		/// this means that this exactly code is banned by the vendor, so the user is trying to cheat you.
		/// </summary>
		Banned,
		/// <summary>
		/// Something goes really wrong here. The error means that the internal activation stuff is not working, 
		/// it is not safe to continue with the activation, registration and so on.
		/// </summary>
		Corrupted,
		/// <summary>
		/// The code has been successfully passed to the server and it was unable to find it in the database.
		/// Probably the user made a mistake, so it is worth asking the user to check the entered code for mistakes.
		/// </summary>
		BadCode,
		/// <summary>
		/// The code has been used too many times and cannot be used more. This doesn't mean the code is bad or banned, it is OK. 
		/// The user may contact vendor to ask/purchase more activations, or the user may deactivate some installations to increase 
		/// the number of activations available for that code.
		/// </summary>
		AlreadyUsed,
		/// <summary>
		/// This is the deactivation error code that means that the server can't find the serial number that the user tries to deactivate.
		/// </summary>
		SerialUnknown,
		/// <summary>
		/// The code is expired.
		/// </summary>
		Expired,
		/// <summary>
		/// Activation/deactivation features are not available.
		/// </summary>
		NotAvailable
	};

	[Flags]
	[VMProtect.DeleteOnCompilation]
	enum CoreOption
	{
		MEMORY_PROTECTION = 0x1,
		CHECK_DEBUGGER = 0x2
	}

	public class Core
	{
		public static bool IsProtected() { return true; }
		public static bool IsDebuggerPresent(bool checkKernelMode)
		{
			if (GlobalData.IsDebuggerDetected())
				return true;

			if (Debugger.IsAttached || Debugger.IsLogging())
				return true;

			if (Win32.IsDebuggerPresent() || Win32.CheckRemoteDebuggerPresent())
				return true;

			return false;
		}

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

		public static bool IsVirtualMachinePresent()
		{
			var info = CpuId.Invoke(1);
			if (((info[2] >> 31) & 1) != 0)
			{
				// check Hyper-V root partition
				info = CpuId.Invoke(0x40000000);
				if (info[1] == 0x7263694d && info[2] == 0x666f736f && info[3] == 0x76482074) // "Microsoft Hv"
				{
					info = CpuId.Invoke(0x40000003);
					if ((info[1] & 1) != 0)
						return false;
				}
				return true;
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
								return true;
						}
					}
				}
			}

			return false;
		}

		public static bool IsValidImageCRC()
		{
			if (GlobalData.IsPatchDetected())
				return false;

			long instance = Marshal.GetHINSTANCE(typeof(Core).Module).ToInt64();

			// check memory CRC
			var crcSize = (uint)Marshal.ReadInt32(new IntPtr(instance + (uint)Faces.CRC_TABLE_SIZE));
			var crcPosition = (uint)Faces.CRC_TABLE_ENTRY;
			bool isValid = true;
			var crcInfo = new byte[12];
			var crc32 = new CRC32();
			var crcHash = (uint)Marshal.ReadInt32(new IntPtr(instance + (uint)Faces.CRC_TABLE_HASH));
			if (crcHash != crc32.Hash(new IntPtr(instance + crcPosition), crcSize))
				isValid = false;
			{
				var crcCryptor = new CRCValueCryptor();
				for (var i = 0; i < crcSize; i += crcInfo.Length)
				{
					Marshal.Copy(new IntPtr(instance + crcPosition + i), crcInfo, 0, crcInfo.Length);
					uint address = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 0));
					uint size = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 4));
					uint hash = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 8));

					if (crc32.Hash(new IntPtr(instance + address), size) != hash)
						isValid = false;
				}
			}

			// check header and loader CRC
			crcSize = (uint)Marshal.ReadInt32(new IntPtr(instance + GlobalData.LoaderCrcSize()));
			crcPosition = GlobalData.LoaderCrcInfo();
			crcHash = (uint)Marshal.ReadInt32(new IntPtr(instance + GlobalData.LoaderCrcHash()));
			if (crcHash != crc32.Hash(new IntPtr(instance + crcPosition), crcSize))
				isValid = false;
			{
				var crcCryptor = new CRCValueCryptor();
				for (var i = 0; i < crcSize; i += crcInfo.Length)
				{
					Marshal.Copy(new IntPtr(instance + crcPosition + i), crcInfo, 0, crcInfo.Length);
					uint address = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 0));
					uint size = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 4));
					uint hash = crcCryptor.Decrypt(BitConverter.ToUInt32(crcInfo, 8));

					if (crc32.Hash(new IntPtr(instance + address), size) != hash)
						isValid = false;
				}
			}

			return isValid;
		}
		public static string DecryptString() { return DecryptString((uint)Faces.STORAGE_INFO); }
		public static bool FreeString(ref string value) { value = null;  return true; }
		public static SerialState SetSerialNumber(string serial)
		{
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.SetSerialNumber(serial) : SerialState.Corrupted;
		}
		public static SerialState GetSerialNumberState()
		{
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.GetSerialNumberState() : SerialState.Corrupted;
		}
		public static bool GetSerialNumberData(out SerialNumberData data)
		{
			data = new SerialNumberData();
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.GetSerialNumberData(data) : false;
		}
		public static string GetCurrentHWID() { return Instance.HWID.ToString(); }
		public static ActivationStatus ActivateLicense(string code, out string serial)
		{
			serial = string.Empty;
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.ActivateLicense(code, out serial) : ActivationStatus.NotAvailable;
		}
		public static ActivationStatus DeactivateLicense(string serial)
		{
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.DeactivateLicense(serial) : ActivationStatus.NotAvailable;
		}
		public static ActivationStatus GetOfflineActivationString(string code, out string buf)
		{
			buf = string.Empty;
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.GetOfflineActivationString(code, out buf) : ActivationStatus.NotAvailable;
		}
		public static ActivationStatus GetOfflineDeactivationString(string serial, out string buf)
		{
			buf = string.Empty;
			LicensingManager licensing_manager = Instance.LicensingManager;
			return licensing_manager != null ? licensing_manager.GetOfflineDeactivationString(serial, out buf) : ActivationStatus.NotAvailable;
		}

		public static readonly Core Instance = new Core();
		public HardwareID HWID { get; private set; }
		public StringManager StringManager { get; private set; }
		public LicensingManager LicensingManager { get; private set; }
		public ResourceManager ResourceManager { get; private set; }

		private static string DecryptString(uint stringId) { return Instance.StringManager.DecryptString(stringId); }
		public static void ShowMessage(string message) { Win32.ShowMessage(message, Assembly.GetExecutingAssembly().GetName().Name, MessageBoxButtons.OK, MessageBoxIcon.Error); }

		private static void AntidebugThread()
		{
			Win32.NtSetInformationThread(Win32.CurrentThread, Win32.THREADINFOCLASS.ThreadHideFromDebugger, IntPtr.Zero, 0);

			while (true)
			{
				if (Debugger.IsAttached || Debugger.IsLogging())
					Environment.FailFast("");

				if (Win32.IsDebuggerPresent() || Win32.CheckRemoteDebuggerPresent())
					Environment.FailFast("");

				Thread.Sleep(1000);
			}
		}

		public bool Init(long instance)
		{
			var options = (uint)Faces.CORE_OPTIONS;
			if ((options & (uint)CoreOption.CHECK_DEBUGGER) != 0)
			{
				var thread = new Thread(AntidebugThread);
				thread.IsBackground = true;
				thread.Start();
			}

			HWID = new HardwareID();

			var pos = (uint)Faces.STRING_INFO;
			if (pos != 0)
				StringManager = new StringManager(instance);

			pos = (uint)Faces.LICENSE_INFO;
			if (pos != 0)
				LicensingManager = new LicensingManager(instance);

			pos = (uint)Faces.TRIAL_HWID;
			if (pos != 0) {
				var key = new byte[8];
				Marshal.Copy(new IntPtr(instance + (uint)Faces.KEY_INFO), key, 0, key.Length);

				var entry = new byte[64];
				Marshal.Copy(new IntPtr(instance + pos), entry, 0, entry.Length);
				CipherRC5 cipher = new CipherRC5(key);
				entry = cipher.Decrypt(entry);

				var value = new byte[(uint)Faces.TRIAL_HWID_SIZE];
				Array.Copy(entry, 0, value, 0, value.Length);

				if (!HWID.IsCorrect(value))
				{
					ShowMessage("This application cannot be executed on this computer.");
					return false;
				}
			}

			pos = (uint)Faces.RESOURCE_INFO;
			if (pos != 0)
				ResourceManager = new ResourceManager(instance);

			return true;
		}

		public static uint DecryptBuffer(uint p3, uint p2, uint p1, uint p0)
		{
			return Instance.LicensingManager.DecryptBuffer(p3, p2, p1, p0);
		}

#if !RUNTIME
		public readonly Random RndGenerator = new Random();

		public uint Rand32()
		{
			var a4 = new byte[4];
			RndGenerator.NextBytes(a4);
			return BitConverter.ToUInt32(a4, 0);
		}

		public ulong Rand64()
		{
			return (Rand32() << 32) | Rand32();
		}
#endif
	}

	public class ResourceManager
	{
		public ResourceManager(long instance)
		{
			_instance = instance;
			var key = new byte[8];
			Marshal.Copy(new IntPtr(instance + (uint)Faces.KEY_INFO), key, 0, key.Length);
			_cipher = new CipherRC5(key);

			var resourceInfo = new byte[16];
			var pos = (uint)Faces.RESOURCE_INFO;
			var buffer = new byte[1024];
			for (var i = 0; ;i += resourceInfo.Length)
			{
				Marshal.Copy(new IntPtr(instance + pos + i), resourceInfo, 0, resourceInfo.Length);
				byte[] entry = _cipher.Decrypt(resourceInfo);
				uint nameOffset = BitConverter.ToUInt32(entry, 0);
				if (nameOffset == 0)
					break;

				for (var j = 0; j < buffer.Length / 2; j++)
				{
					var c = (ushort)(Marshal.ReadInt16(new IntPtr(instance + nameOffset + j * 2)) ^ (BitRotate.Left((uint)Faces.STRING_DECRYPT_KEY, j) + j));
					if (c == 0)
					{
						_entries.Add(Encoding.Unicode.GetString(buffer, 0, j * 2), (uint)(pos + i));
						break;
					}
					buffer[j * 2] = (byte)c;
					buffer[j * 2 + 1] = (byte)(c >> 8);
				}
			}

			AppDomain.CurrentDomain.AssemblyResolve += Handler;
		}

		private void DecryptData(ref byte[] data, uint key)
		{
			for (var c = 0; c < data.Length; c++)
			{
				data[c] = (byte)(data[c] ^ BitRotate.Left(key, c) + c);
			}
		}

		private Assembly Handler(object sender, ResolveEventArgs args)
		{
			uint pos;
			if (_entries.TryGetValue(args.Name, out pos))
			{
				var resourceInfo = new byte[16];
				Marshal.Copy(new IntPtr(_instance + pos), resourceInfo, 0, resourceInfo.Length);
				byte[] entry = _cipher.Decrypt(resourceInfo);
				uint dataOffset = BitConverter.ToUInt32(entry, 4);
				var data = new byte[BitConverter.ToUInt32(entry, 8)];
				Marshal.Copy(new IntPtr(_instance + dataOffset), data, 0, data.Length);
				DecryptData(ref data, (uint)Faces.STRING_DECRYPT_KEY);
				try
				{
					var assembly = Assembly.Load(data);
					return assembly;
				}
				catch (Exception)
				{

				}
			}

			return null;
		}

		private long _instance;
		private CipherRC5 _cipher;
		private readonly Dictionary<string, uint> _entries = new Dictionary<string, uint>();
	}
}