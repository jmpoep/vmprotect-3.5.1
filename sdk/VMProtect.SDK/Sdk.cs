using System;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

// ReSharper disable MemberCanBePrivate.Global
// ReSharper disable UnusedParameter.Global
// ReSharper disable UnusedMember.Global
// ReSharper disable once CheckNamespace
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

	public static class SDK
	{
		private static bool _gSerialIsCorrect;
		private static bool _gSerialIsBlacklisted;
		private static readonly Int32 GTimeOfStart = Environment.TickCount;
		private static string GetIniValue(string valueName, string defaultValue = null)
		{
			var ini = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "VMProtectLicense.ini");
            if (!File.Exists(ini))
                return defaultValue;

			var contents = File.ReadAllText(ini);
			var pattern = new Regex(@"
				^                           # Beginning of the line
				((?:\[)                     # Section Start
					 (?<Section>[^\]]*)     # Actual Section text into Section Group
				 (?:\])                     # Section End then EOL/EOB
				 (?:[\r\n]{0,}|\Z))         # Match but don't capture the CRLF or EOB
				 (                          # Begin capture groups (Key Value Pairs)
				  (?!\[)                    # Stop capture groups if a [ is found; new section
				  (?<Key>[^=]*?)            # Any text before the =, matched few as possible
				  (?:=)                     # Get the = now
				  (?<Value>[^\r\n]*)        # Get everything that is not an Line Changes
				  (?:[\r\n]{0,4})           # MBDC \r\n
				  )+                        # End Capture groups", RegexOptions.IgnorePatternWhitespace | RegexOptions.Multiline);
			var ret = defaultValue;
			foreach (Match m in pattern.Matches(contents))
			{
				var section = m.Groups["Section"].Value;
				if(section != "TestLicense")
					continue;
				var i = 0;
				foreach (Capture c in m.Groups["Key"].Captures)
				{
					if (c.Value == valueName)
					{
						ret = m.Groups["Value"].Captures[i].ToString();
						break;
					}
					i++;
				}
				break;
			}
			return ret;
		}
		private static bool GetIniDateTime(string valueName, out DateTime dt)
		{
			return DateTime.TryParseExact(GetIniValue(valueName), "yyyyMMdd", CultureInfo.InvariantCulture, DateTimeStyles.None, out dt);
		}
		private static bool GetIniTimeLimit(out int nTimeLimit)
		{
			var sTimeLimit = GetIniValue("TimeLimit");
			nTimeLimit = 0;
			if (string.IsNullOrEmpty(sTimeLimit))
				return false;

			int.TryParse(sTimeLimit, out nTimeLimit);
			if (nTimeLimit < 0)
				nTimeLimit = 0;
			else if (nTimeLimit > 255)
				nTimeLimit = 255;
			return true;
		}

		/// <summary>
		/// !!! документировать здесь и в справке
		/// </summary>
		/// <returns></returns>
		public static bool IsProtected() { return false; }

		/// <summary>
		/// Detect if the application is running in the debugger. 
		/// The result of its work (True/False) can be processed by the protection mechanisms built into the application. 
		/// If CheckKernelMode=False, the function checks for the user-mode debugger (OllyDBG, WinDBG, etc.). 
		/// If CheckKernelMode=True, both user-mode and kernel-mode debuggers will be detected (SoftICE, Syser, etc.). 
		/// </summary>
		/// <returns></returns>
		public static bool IsDebuggerPresent(bool checkKernelMode) { return System.Diagnostics.Debugger.IsAttached; }

		/// <summary>
		/// Detect if the application is running in a virtual environment: VMware, Virtual PC, VirtualBox, Sandboxie, Hyper-V. 
		/// The result of its work (True/False) can be processed by the protection mechanisms built into the application.
		/// </summary>
		public static bool IsVirtualMachinePresent() { return false; }

		/// <summary>
		/// Detect changes made in the application memory. The result of its work (True/False) 
		/// can be processed by the protection mechanisms built into the application.
		/// </summary>
		public static bool IsValidImageCRC() { return true; }

		/// <summary>
		/// Decrypt string constant. You have to add this constant to a list of protected objects.
		/// </summary>
		/// <param name="value">constant (not variable!) for decryption</param>
		/// <returns>decrypted string</returns>
		public static string DecryptString(string value) { return value; }

		/// Pass serial number to the licensing module
		/// </summary>
		/// <param name="serial">serial number</param>
		/// <returns>serial number state</returns>
		public static SerialState SetSerialNumber(string serial)
		{
			_gSerialIsCorrect = false;
			_gSerialIsBlacklisted = false;
			if (string.IsNullOrEmpty(serial))
				return SerialState.Invalid;

			var bufSerial = new StringBuilder(2048);
			foreach (var c in serial.ToCharArray())
			{
				if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')
					bufSerial.Append(c);
			}

			var iniSerial = GetIniValue("AcceptedSerialNumber", "serialnumber");
			_gSerialIsCorrect = bufSerial.ToString() == iniSerial;
			var iniBlackSerial = GetIniValue("BlackListedSerialNumber");
			_gSerialIsBlacklisted = bufSerial.ToString() == iniBlackSerial;

			return GetSerialNumberState();
		}
		
		/// <summary>
		/// Get current serial number state
		/// </summary>
		/// <returns>serial number state</returns>
		public static SerialState GetSerialNumberState()
		{
			if (!_gSerialIsCorrect)
				return SerialState.Invalid;
			if (_gSerialIsBlacklisted) 
				return SerialState.Blacklisted;

			var res = SerialState.Success;
			int runningTime;
			if(GetIniTimeLimit(out runningTime))
			{
				int d = (Environment.TickCount - GTimeOfStart) / 1000 / 60; // minutes
				if (runningTime <= d) 
					res |= SerialState.RunningTimeOver;
			}

			DateTime dt;
			if (GetIniDateTime("ExpDate", out dt)) {
				if (DateTime.Now > dt)
					res |= SerialState.DateExpired;
			}

			if (GetIniDateTime("MaxBuildDate", out dt)) {
				if (DateTime.Now > dt)
					res |= SerialState.MaxBuildExpired;
			}

			if (GetIniValue("KeyHWID") != GetIniValue("MyHWID"))
				res |= SerialState.BadHwid;
			return res;
		}
		
		/// <summary>
		/// Convert content of serial number to the structure
		/// </summary>
		/// <param name="data">struct to fill out</param>
		/// <returns>true (always) or exception</returns>
		public static bool GetSerialNumberData(out SerialNumberData data)
		{
			data = new SerialNumberData();
			data.State = GetSerialNumberState();
			if ((data.State & (SerialState.Invalid | SerialState.Blacklisted)) != 0)
				return true; // do not need to read the rest

			data.UserName = GetIniValue("UserName");
			data.EMail = GetIniValue("EMail");

			GetIniTimeLimit(out data.RunningTime);

			DateTime dt;
			if (GetIniDateTime("ExpDate", out dt)) {
				data.Expires = dt;
			}

			if (GetIniDateTime("MaxBuildDate", out dt)) {
				data.MaxBuild = dt;
			}

			var sUserData = GetIniValue("UserData");
			if (!string.IsNullOrEmpty(sUserData)) {
				var len = sUserData.Length;
				if (len > 0 && len % 2 == 0 && len <= 0x200) // otherwise UserData is empty or has bad length
				{
					data.UserData = new byte[len / 2];
					for (var index = 0; index < data.UserData.Length; index++)
					{
						var byteValue = sUserData.Substring(index * 2, 2);
						data.UserData[index] = byte.Parse(byteValue, NumberStyles.HexNumber, CultureInfo.InvariantCulture);
					}
				}
			}
			return true;
		}

		/// <summary>
		/// Get hardware identifier (hwid) of the current computer
		/// </summary>
		/// <returns>hwid as string</returns>
		public static string GetCurrentHWID() { return GetIniValue("MyHWID", "myhwid"); }
		
		/// <summary>
		/// Passes the activation code to the server for online activation and returns the serial number for that exactly installation.
		/// </summary>
		/// <param name="code">activation code, in</param>
		/// <param name="serial">serial number, out</param>
		/// <returns>activation status</returns>
		public static ActivationStatus ActivateLicense(string code, out string serial)
		{
			serial = "";
			if (code != GetIniValue("AcceptedActivationCode", "activationcode"))
				return ActivationStatus.BadCode;
			serial = GetIniValue("AcceptedSerialNumber", "serialnumber");
			return ActivationStatus.Ok;
		}

		/// <summary>
		/// Passes the serial number to the server for online deactivation.
		/// </summary>
		/// <param name="serial">contains the serial number (not the code) obtained from the server during the activation process</param>
		/// <returns>deactivation status</returns>
		public static ActivationStatus DeactivateLicense(string serial) { return ActivationStatus.Ok; }

		/// <summary>
		/// Same as ActivateLicense, but don't trying to connect the server. Instead, it returns a "magic" string that needs to be copy-pasted 
		/// to the web browser, showing the Web License Manager offline activation page.
		/// </summary>
		/// <param name="code">activation code, in</param>
		/// <param name="buf">magic offline activation string, out</param>
		/// <returns>operation status</returns>
		public static ActivationStatus GetOfflineActivationString(string code, out string buf)
		{
			buf = "Sdk OfflineActivationString"; 
			return ActivationStatus.Ok; 
		}

		/// <summary>
		/// Same as DeactivateLicense, but don't trying to connect the server. Instead, it returns a "magic" string that needs to be copy-pasted 
		/// to the web browser, showing the Web License Manager offline deactivation page.
		/// </summary>
		/// <param name="serial">contains the serial number (not the code) obtained from the server during the activation process, in</param>
		/// <param name="buf">magic offline deactivation string, out</param>
		/// <returns>operation status</returns>
		public static ActivationStatus GetOfflineDeactivationString(string serial, out string buf)
		{
			buf = "Sdk GetOfflineDeactivationString"; 
			return ActivationStatus.Ok; 
		}
	}

	/// <summary>
	/// The marker that starts the protected section of the code, must be called before the first instruction 
	/// (calling a procedure or function) in the protected block of the code. 
	/// </summary>
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class Begin : Attribute { }

	/// <summary>
	/// The marker that starts the protected section of the code with the predefined "virtualization" compilation type. 
	/// It is impossible to change the compilation type specified by the marker later in VMPotect.
	/// </summary>
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class BeginVirtualization : Attribute { }

	/// <summary>
	/// The marker that starts the protected section of the code with the predefined "mutation" compilation type. 
	/// It is impossible to change the compilation type specified by the marker later in VMPotect.
	/// </summary>
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class BeginMutation : Attribute { }

	/// <summary>
	/// The marker that starts the protected section of the code with the predefined "ultra" (mutation + virtualization) compilation type. 
	/// It is impossible to change the compilation type specified by the marker later in VMPotect.
	/// </summary>

	[AttributeUsage(AttributeTargets.Method)]
	public sealed class BeginUltra : Attribute { }

	/// <summary>
	/// The marker that starts the protected section of the code with the predefined "virtualization" compilation type 
	/// and the option "Lock to key". It is impossible to change the compilation type specified by the marker later in VMPotect.
	/// </summary>
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class BeginVirtualizationLockByKey : Attribute { }

	/// <summary>
	/// The marker that starts the protected section of the code with the predefined "ultra" (mutation + virtualization) compilation type 
	/// and the option "Lock to key". It is impossible to change the compilation type specified by the marker later in VMPotect.
	/// </summary>
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class BeginUltraLockByKey : Attribute { }
}