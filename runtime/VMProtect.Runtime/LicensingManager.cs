using System;
using System.Globalization;
using System.Net;
using System.Net.Cache;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using Numerics;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public class LicensingManager
	{
		public class BaseRequest
		{
			protected bool BuildUrl(byte[] licenseData)
			{
				var urlSize = BitConverter.ToInt32(licenseData, (int)Fields.ActivationUrlSize * sizeof(uint));
				if (urlSize == 0)
					return false;

				var urlOffset = BitConverter.ToInt32(licenseData, (int)Fields.ActivationUrlOffset * sizeof(uint));
				Url = Encoding.UTF8.GetString(licenseData, urlOffset, urlSize);
				if (Url[Url.Length - 1] != '/')
					Url += '/';
				return true;
			}
			protected void EncodeUrl()
			{
				Url = Convert.ToBase64String(Encoding.UTF8.GetBytes(Url));
			}
			protected void AppendUrlParam(string param, string value)
			{
				AppendUrl(param, false);
				AppendUrl(value, true);
			}
			private void AppendUrl(string str, bool escape)
			{
				if (escape) 
				{
					var sb = new StringBuilder(Url);
					foreach (var c in str)
					{
						switch (c) 
						{
						case '+':
							sb.Append("%2B");
							break;
						case '/':
							sb.Append("%2F");
							break;
						case '=':
							sb.Append("%3D");
							break;
						default:
							sb.Append(c);
							break;
						}
					}
					Url = sb.ToString();
				} else
				{
					Url += str;
				}
			}

			public bool Send()
			{
				try
				{
					using (var wc = new WebClient())
					{
						ServicePointManager.ServerCertificateValidationCallback +=
							(sender, certificate, chain, sslPolicyErrors) => true;
						wc.CachePolicy = new RequestCachePolicy(RequestCacheLevel.NoCacheNoStore);
						wc.Headers.Add("user-agent", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)");
						Response = wc.DownloadString(Url);
						try
						{
							var strDt = wc.ResponseHeaders.Get("Date");
							var dt = DateTime.ParseExact(strDt, "ddd, dd MMM yyyy HH:mm:ss 'GMT'",
									CultureInfo.InvariantCulture.DateTimeFormat, DateTimeStyles.AssumeUniversal);
							GlobalData.SetServerDate((uint)((dt.Year << 16) + (dt.Month << 8) + dt.Day));
						}
						catch (Exception)
						{
							//не смогли вытащить дату из заголовков - прощаем?
						}
						return true;
					}
				}
				catch (Exception)
				{
					return false;
				}
			}

			public string Url { get; private set; }
			public string Response { get; private set; }
		}

		public class ActivationRequest : BaseRequest
		{
			public ActivationStatus Process(byte[] licenseData, string code, bool offline)
			{
				if (!VerifyCode(code))
					return ActivationStatus.BadCode;

				if (!BuildUrl(licenseData, code, offline))
					return ActivationStatus.NotAvailable;

				if (offline)
					return ActivationStatus.Ok;

				if (!Send())
					return ActivationStatus.NoConnection;

				var res = Response;
				if (string.IsNullOrEmpty(res))
					return ActivationStatus.BadReply;

				// possible answers: OK, BAD, BANNED, USED, EXPIRED
				// if OK - see the Serial number below

				if (res == "BAD")
					return ActivationStatus.BadCode;

				if (res == "BANNED")
					return ActivationStatus.Banned;

				if (res == "USED")
					return ActivationStatus.AlreadyUsed;

				if (res == "EXPIRED")
					return ActivationStatus.Expired;

				var crPos = res.IndexOf('\n');
				if (crPos != 2)
					return ActivationStatus.BadReply;

				if (res[0] != 'O' || res[1] != 'K')
					return ActivationStatus.BadReply;
	
				if (res.Length - 3 < 64)
					return ActivationStatus.BadReply;

				Serial = res.Substring(3);
				return ActivationStatus.Ok;
			}
			public string Serial
			{
				get; private set; 
			}
			private bool VerifyCode(string code)
			{
				if (string.IsNullOrEmpty(code) || code.Length > 32) 
					return false;
				return code.ToLower().TrimEnd("0123456789abcdefghijklmnopqrstuvwxyz-".ToCharArray()).Length == 0;
			}
			private bool BuildUrl(byte[] licenseData, string code, bool offline)
			{
				if (!offline) {
					if (!base.BuildUrl(licenseData))
						return false;
				}

				// hwid -> base64
				var hwid = Convert.ToBase64String(Core.Instance.HWID.GetBytes());

				// hash -> base64
				var modSize = BitConverter.ToInt32(licenseData, (int)Fields.ModulusSize * sizeof(uint));
				var modOffset = BitConverter.ToInt32(licenseData, (int)Fields.ModulusOffset * sizeof(uint));
				using (var sha = new SHA1Managed())
				{
					var modulus = sha.ComputeHash(licenseData, modOffset, modSize);
					var hash = Convert.ToBase64String(modulus, 0, 20);

					// build Url
					AppendUrlParam(offline ? "type=activation&code=" : "activation.php?code=", code);
					AppendUrlParam("&hwid=", hwid);
					AppendUrlParam("&hash=", hash);
				}

				if (offline)
					EncodeUrl();

				return true;
			}
		}

		public class DeactivationRequest : BaseRequest
		{
			public ActivationStatus Process(byte[] licenseData, string serial, bool offline)
			{
				if (!VerifySerial(serial))
					return ActivationStatus.BadCode;

				if (!BuildUrl(licenseData, serial, offline)) 
					return ActivationStatus.NotAvailable;

				if (offline)
					return ActivationStatus.Ok;

				if (!Send())
					return ActivationStatus.NoConnection;

				var res = Response;
				if (string.IsNullOrEmpty(res))
					return ActivationStatus.BadReply;

				if (res == "OK")
					return ActivationStatus.Ok;

				if (res == "ERROR")
					return ActivationStatus.Corrupted;

				if (res == "UNKNOWN")
					return ActivationStatus.SerialUnknown;

				return ActivationStatus.BadReply;
			}

			private bool VerifySerial(string serial)
			{
				return !string.IsNullOrEmpty(serial);
			}

			private bool BuildUrl(byte[] licenseData, string serial, bool offline)
			{
				if (!offline) {
					if (!base.BuildUrl(licenseData))
						return false;
				}

				try
				{
					var serialBytes = Convert.FromBase64String(serial);
					using (var sha = new SHA1Managed())
					{
						var serialHash = sha.ComputeHash(serialBytes);
						var hash = Convert.ToBase64String(serialHash, 0, 20);

						AppendUrlParam(offline ? "type=deactivation&hash=" : "deactivation.php?hash=", hash);
					}
				}
				catch (FormatException)
				{
					return false;
				}

				if (offline)
					EncodeUrl();

				return true;
			}
		}
		public LicensingManager(long instance)
		{
			_sessionKey = 0 - GlobalData.SessionKey();
			_licenseData = new byte[(uint)Faces.LICENSE_INFO_SIZE];
			Marshal.Copy(new IntPtr(instance + (uint)Faces.LICENSE_INFO), _licenseData, 0, _licenseData.Length);
			_startTickCount = Environment.TickCount;
		}
		public LicensingManager(byte [] licenseData)
		{
			_licenseData = licenseData;
		}
		public SerialState GetSerialNumberState()
		{
			lock (_lock)
			{
				return (_state & (SerialState.Corrupted | SerialState.Invalid | SerialState.BadHwid | SerialState.Blacklisted)) != 0 ? _state : ParseSerial(null);
			}
		}
		public ActivationStatus ActivateLicense(string code, out string serial)
		{
			serial = string.Empty;
			if (!CheckLicenseDataCRC())
				return ActivationStatus.Corrupted;

			var request = new ActivationRequest();
			var res = request.Process(_licenseData, code, false);
			if (res == ActivationStatus.Ok)
			{
				serial = request.Serial;
			}
			return res;
		}

		public ActivationStatus DeactivateLicense(string serial)
		{
			return CheckLicenseDataCRC() ?
				new DeactivationRequest().Process(_licenseData, serial, false) : 
				ActivationStatus.Corrupted;
		}

		public ActivationStatus GetOfflineActivationString(string code, out string buf)
		{
			buf = string.Empty;
			if (!CheckLicenseDataCRC())
				return ActivationStatus.Corrupted;

			var request = new ActivationRequest();
			var res = request.Process(_licenseData, code, true);
			if (res == ActivationStatus.Ok)
			{
				buf = request.Url;
			}
			return res;
		}

		public ActivationStatus GetOfflineDeactivationString(string serial, out string buf)
		{
			buf = string.Empty;
			if (!CheckLicenseDataCRC())
				return ActivationStatus.Corrupted;

			var request = new DeactivationRequest();
			var res = request.Process(_licenseData, serial, true);
			if (res == ActivationStatus.Ok)
			{
				buf = request.Url;
			}
			return res;
		}

		private static BigInteger B2Bi(byte[] b) //reverse & make positive
		{
			Array.Reverse(b);
			var b2 = new byte[b.Length + 1];
			Array.Copy(b, b2, b.Length);
			return new BigInteger(b2);
		}

		public SerialState SetSerialNumber(string serial)
		{
			lock (_lock)
			{
				SaveState(SerialState.Invalid);

				if (string.IsNullOrEmpty(serial)) 
					return SerialState.Invalid; // the key is empty
		
				// decode serial number from base64
				byte[] binarySerial;
				try
				{
					binarySerial = Convert.FromBase64String(serial);
					if (binarySerial.Length < 16)
						return SerialState.Invalid;
				}
				catch (Exception)
				{
					return SerialState.Invalid;
				}

				// check license data integrity
				if (!CheckLicenseDataCRC()) {
					return SaveState(SerialState.Corrupted);
				}

				// check serial by black list
				var blackListSize = BitConverter.ToInt32(_licenseData, (int)Fields.BlacklistSize * sizeof(uint));
				if (blackListSize != 0) {
					var blackListOffset = BitConverter.ToInt32(_licenseData, (int)Fields.BlacklistOffset * sizeof(uint));

					using (var hash = new SHA1Managed())
					{
						var p = hash.ComputeHash(binarySerial);
						int min = 0;
						int max = blackListSize / 20 - 1;
						while (min <= max)
						{
							int i = (min + max) / 2;
							var blocked = true;
							for (var j = 0; j < 20 / sizeof(uint); j++) {
								var dw = BitConverter.ToUInt32(_licenseData, blackListOffset + i * 20 + j * sizeof(uint));
								var v = BitConverter.ToUInt32(p, j * sizeof(uint));
								if (dw == v)
									continue;

								if (BitRotate.Swap(dw) > BitRotate.Swap(v))
								{
									max = i - 1;
								}
								else
								{
									min = i + 1;
								}
								blocked = false;
								break;
							}
							if (blocked) {
								return SaveState(SerialState.Blacklisted);
							}
						}
					}
				}

				// decode serial number
				var ebytes = new byte[BitConverter.ToInt32(_licenseData, (int)Fields.PublicExpSize * sizeof(uint))];
				Array.Copy(_licenseData, BitConverter.ToInt32(_licenseData, (int)Fields.PublicExpOffset * sizeof(uint)), ebytes, 0, ebytes.Length);
				var e = B2Bi(ebytes);
				var nbytes = new byte[BitConverter.ToInt32(_licenseData, (int)Fields.ModulusSize * sizeof(uint))];
				Array.Copy(_licenseData, BitConverter.ToInt32(_licenseData, (int)Fields.ModulusOffset * sizeof(uint)), nbytes, 0, nbytes.Length);
				var n = B2Bi(nbytes);
				var x = B2Bi(binarySerial);

				if (n < x) {
					// data is too long to crypt
					return SerialState.Invalid;
				}

				_serial = BigInteger.ModPow(x, e, n).ToByteArray();
				Array.Reverse(_serial);

				if (_serial[0] != 0 || _serial[1] != 2)
					return SerialState.Invalid;

				int pos;
				for (pos = 2; pos < _serial.Length; pos++) {
					if (_serial[pos] == 0) {
						pos++;
						break;
					}
				}
				if (pos == _serial.Length)
					return SerialState.Invalid;

				_start = pos;
				return ParseSerial(null);
			}
		}

		public bool GetSerialNumberData(SerialNumberData data)
		{
			lock (_lock)
			{
				if (_state == SerialState.Corrupted)
					return false;

				data.State = (_state & (SerialState.Invalid | SerialState.Blacklisted | SerialState.BadHwid)) != 0 ? _state : ParseSerial(data);
				return true;
			}
		}

		public uint DecryptBuffer(uint p3, uint p2, uint p1, uint p0)
		{
			uint key0 = (uint)_productCode;
			uint key1 = (uint)(_productCode >> 32) + _sessionKey;

			p0 = BitRotate.Left((p0 + _sessionKey) ^ key0, 7) + key1;
			p1 = BitRotate.Left((p1 + _sessionKey) ^ key0, 11) + key1;
			p2 = BitRotate.Left((p2 + _sessionKey) ^ key0, 17) + key1;
			p3 = BitRotate.Left((p3 + _sessionKey) ^ key0, 23) + key1;

			if (p0 + p1 + p2 + p3 != _sessionKey * 4)
			{
				Core.ShowMessage("This code requires valid serial number to run.\nProgram will be terminated.");
				Environment.Exit(1);
			}

			return p3;
		}

		[VMProtect.DeleteOnCompilation]
		private enum ChunkType
		{
			Version					= 0x01,	//	1 byte of data - version
			UserName				= 0x02,	//	1 + N bytes - length + N bytes of customer's name (without enging \0).
			Email					= 0x03,	//	1 + N bytes - length + N bytes of customer's email (without ending \0).
			HWID					= 0x04,	//	1 + N bytes - length + N bytes of hardware id (N % 4 == 0)
			ExpDate					= 0x05,	//	4 bytes - (year << 16) + (month << 8) + (day)
			RunningTimeLimit		= 0x06,	//	1 byte - number of minutes
			ProductCode				= 0x07,	//	8 bytes - used for decrypting some parts of exe-file
			UserData				= 0x08,	//	1 + N bytes - length + N bytes of user data
			MaxBuild				= 0x09,	//	4 bytes - (year << 16) + (month << 8) + (day)
			End						= 0xFF	//	4 bytes - checksum: the first four bytes of sha-1 hash from the data before that chunk
		}

		private SerialState SaveState(SerialState state)
		{
			_state = state;
			if ((_state & (SerialState.Invalid | SerialState.BadHwid)) != 0)
			{
				_serial = null;
				_productCode = 0;
			}
			return _state;
		}
		private SerialState ParseSerial(SerialNumberData data)
		{
			if (_serial == null)
				return SerialState.Invalid;

			var newState = _state & (SerialState.MaxBuildExpired | SerialState.DateExpired | SerialState.RunningTimeOver);
			var pos = _start;
			while (pos < _serial.Length) {
				var b = _serial[pos++];
				byte s;
				switch (b) {
				case (byte)ChunkType.Version:
					if (_serial[pos] != 1)
						return SaveState(SerialState.Invalid);
					pos += 1;
					break;
				case (byte)ChunkType.ExpDate:
					var expDate = BitConverter.ToUInt32(_serial, pos);
					if ((newState & SerialState.DateExpired) == 0) {
						if (BitConverter.ToUInt32(_licenseData, (int)Fields.BuildDate * sizeof(uint)) > expDate || GetCurrentDate() > expDate)
							newState |= SerialState.DateExpired;
					}
					if (data != null) {
						data.Expires = new DateTime((int)(expDate >> 16), (byte)(expDate >> 8), (byte)expDate);
					}
					pos += 4;
					break;
				case (byte)ChunkType.RunningTimeLimit:
					s = _serial[pos];
					if ((newState & SerialState.RunningTimeOver) == 0) {
						var curTime = (Environment.TickCount - _startTickCount) / 1000 / 60;
						if (curTime > s)
							newState |= SerialState.RunningTimeOver;
					}
					if (data != null)
						data.RunningTime = s;
					pos += 1;
					break;
				case (byte)ChunkType.ProductCode:
					if ((_state & SerialState.Invalid) != 0)
						_productCode = BitConverter.ToInt64(_serial, pos);
					pos += 8;
					break;
				case (byte)ChunkType.MaxBuild:
					var maxBuildDate = BitConverter.ToUInt32(_serial, pos);
					if ((newState & SerialState.MaxBuildExpired) == 0) {
						if (BitConverter.ToUInt32(_licenseData, (int)Fields.BuildDate * sizeof(uint)) > maxBuildDate)
							newState |= SerialState.MaxBuildExpired;
					}
					if (data != null)
					{
						data.MaxBuild = new DateTime((int)(maxBuildDate >> 16), (byte)(maxBuildDate >> 8), (byte)maxBuildDate);
					}
					pos += 4;
					break;
				case (byte)ChunkType.UserName:
					s = _serial[pos++];
					if (data != null)
						data.UserName = Encoding.UTF8.GetString(_serial, pos, s);
					pos += s;
					break;
				case (byte)ChunkType.Email:
					s = _serial[pos++];
					if (data != null)
						data.EMail = Encoding.UTF8.GetString(_serial, pos, s);
					pos += s;
					break;
				case (byte)ChunkType.HWID:
					s = _serial[pos++];
					if ((_state & SerialState.Invalid) != 0)
					{
						var shwid = new byte[s];
						Array.Copy(_serial, pos, shwid, 0, s);
						if (!Core.Instance.HWID.IsCorrect(shwid)) 
							return SaveState(SerialState.BadHwid);
					}
					pos += s;
					break;
				case (byte)ChunkType.UserData:
					s = _serial[pos++];
					if (data != null) {
						data.UserData = new byte[s];
						for (var i = 0; i < s; i++) {
							data.UserData[i] = _serial[pos + i];
						}
					}
					pos += s;
					break;
				case (byte)ChunkType.End:
					if (pos + 4 > _serial.Length)
						return SaveState(SerialState.Invalid);

					if ((_state & SerialState.Invalid) != 0) {
						// calc hash without last chunk
						using (var hash = new SHA1Managed())
						{
							var p = hash.ComputeHash(_serial, _start, pos - _start - 1);

							// check CRC
							for (var i = 0; i < 4; i++) {
								if (_serial[pos + i] != p[3 - i])
									return SaveState(SerialState.Invalid);
							}
						}
					}

					return SaveState(newState);
				}
			}

			// SERIAL_CHUNK_END not found
			return SaveState(SerialState.Invalid);
		}

		[VMProtect.DeleteOnCompilation]
		internal enum Fields 
		{
			BuildDate,
			PublicExpOffset,
			PublicExpSize,
			ModulusOffset,
			ModulusSize,
			BlacklistOffset,
			BlacklistSize,
			ActivationUrlOffset,
			ActivationUrlSize,
			CRCOffset,
			Count
		}
		private bool CheckLicenseDataCRC()
		{
			var crcPos = BitConverter.ToInt32(_licenseData, (int)Fields.CRCOffset * sizeof(uint));
			var size = crcPos + 16;
			if (size != _licenseData.Length)
				return false; // bad key size

			// CRC check
			using (var hash = new SHA1Managed())
			{
				var h = hash.ComputeHash(_licenseData, 0, crcPos);
				for (var i = crcPos; i < size; i++)
				{
					if (_licenseData[i] != h[i - crcPos])
						return false;
				}
			}

			return true;
		}

		internal static uint GetCurrentDate()
		{
			var dt = DateTime.Now;
			var curDate = (uint)((dt.Year << 16) + (dt.Month << 8) + dt.Day);
			var serverDate = GlobalData.ServerDate();
			return serverDate > curDate ? serverDate : curDate;
		}

		private readonly byte[] _licenseData;
		private byte[] _serial;
		private readonly object _lock = new object();
		private SerialState _state = SerialState.Invalid;
		//TODO 
		//ReSharper disable once NotAccessedField.Local
		private long _productCode;
		private readonly int _startTickCount;
		private int _start;
		private uint _sessionKey;
		/*
		CryptoContainer *_serial;
		*/
	}
}