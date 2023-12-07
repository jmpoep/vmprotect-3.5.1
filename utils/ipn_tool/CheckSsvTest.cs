using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace ipn_tool
{
	internal static class CheckSsvTest
	{
		/* GOOD sequence
Taggant Test Application

Taggant Library version 1

<fist filename not chacked>
 - correct PE file
 - taggant is found
 - taggant object created
 - taggant is correct
 - taggant does not contain timestamp
 - file protected by packer with 1 id, version .+
 - hashmap covers following regions:
\d.+ any lines
 - hashmap is valid
 - full file hash is valid
 - full file hash covers first \d+ bytes
 - SPV Certificate
     30 82 05 5b 30 82 03 43 a0 03 02 01 02 02 10 25 many lines
 - User Certificate
     30 82 04 04 30 82 02 ec a0 03 02 01 02 02 10 05 many lines
<second filename not chacked> etc
*/
		internal static int Result(string logFile, string pemFile)
		{
			var expectedLines = new[]
			{
				/*00*/ new Regex("^Taggant Test Application$"), 
				/*01*/ null, 
				/*02*/ new Regex("^Taggant Library version 1$"), 
				/*03*/ null, 
				/*04*/ new Regex(@"^.+"),
				/*05*/ new Regex(@"^ - correct PE file$"),
				/*06*/ new Regex(@"^ - taggant is found$"),
				/*07*/ new Regex(@"^ - taggant object created$"),
				/*08*/ new Regex(@"^ - taggant is correct$"),
				/*09*/ new Regex(@"^ - taggant does not contain timestamp$"),
				/*10*/ new Regex(@"^ - file protected by packer with 1 id, version .+$"),
				/*11*/ new Regex(@"^ - hashmap covers following regions:$"),
				/*12*/ new Regex(@"^\d.+$"), //any lines
				/*13*/ new Regex(@"^ - hashmap is valid$"),
				/*14*/ new Regex(@"^ - full file hash is valid$"),
				/*15*/ new Regex(@"^ - full file hash covers first \d+ bytes$"),
				/*16*/ new Regex(@"^ - SPV Certificate$"),
				/*17*/ null,
				/*18*/ new Regex(@"^ - User Certificate"),
				/*19*/ null
			};
			try
			{
				var stateIndex = 0;
				var lineNo = 0;
				var spv = new MemoryStream();
				var usr = new MemoryStream();
				var curFile = "unknown";
				foreach (var line in File.ReadAllLines(logFile))
				{
					if (stateIndex == 4)
						curFile = line;
					var re = expectedLines[stateIndex];
					var needCheck = true;
					switch (stateIndex)
					{
						case 13:
							needCheck = false;
							if (re.IsMatch(line))
								++stateIndex;
							else if (!expectedLines[stateIndex - 1].IsMatch(line))
								throw new InvalidDataException(string.Format("Regex '{0}' or '{1}' match was expected at line #{2} but got '{3}'", re, expectedLines[stateIndex - 1], lineNo, line));
							break;
						case 17:
							if (!AppendToMemoryStream(line, spv))
							{
								stateIndex = 18;
								re = expectedLines[stateIndex];
							}
							else
							{
								needCheck = false;
							}
							break;
						case 19:
							if (!AppendToMemoryStream(line, usr))
							{
								if (!CompareCertificates(pemFile, curFile, spv, usr))
									return 1;
								spv = new MemoryStream();
								usr = new MemoryStream();
								stateIndex = 4;
								re = expectedLines[stateIndex];
							}
							else
							{
								needCheck = false;
							}
							break;
					}
					if(needCheck)
					{
						if (re == null && line != string.Empty)
							throw new InvalidDataException(string.Format("Empty line #{0} was expected but got '{1}'", lineNo, line));
						if (re != null && !re.IsMatch(line))
							throw new InvalidDataException(string.Format("Regex '{0}' match was expected at line #{1} but got '{2}'", re, lineNo, line));
						++stateIndex;
					}
					++lineNo;
				}
				return CompareCertificates(pemFile, curFile, spv, usr) ? 0 : 1;
			}
			catch (Exception ex)
			{
				Console.Error.WriteLine(ex);
				return 1;
			}
		}

		private static bool CompareCertificates(string pemFile, string binaryName, MemoryStream spv, MemoryStream usr)
		{
			var pemContents = File.ReadAllText(pemFile);
			var m = Regex.Match(pemContents,
				@"^-----BEGIN CERTIFICATE-----[\s]*(?<spv>([^-]+))[\s]*-----END CERTIFICATE-----[\s]*-----BEGIN CERTIFICATE-----[\s]*(?<usr>([^-]+))[\s]*-----END CERTIFICATE-----[\s]*-----BEGIN RSA PRIVATE KEY-----[\s]*(?<pkey>([^-]+))[\s]*-----END RSA PRIVATE KEY-----[\s]*$",
				RegexOptions.Multiline);
			if (!m.Success)
				throw new InvalidDataException("Cannot parse " + pemFile);
			var expectedSpv = Convert.FromBase64String(m.Groups["spv"].Value);
			var expectedUsr = Convert.FromBase64String(m.Groups["usr"].Value);
			//TODO: check private key if need
			return CompareBa("SPV", binaryName, expectedSpv, spv.ToArray()) && CompareBa("USER", binaryName, expectedUsr, usr.ToArray());
		}

		private static bool CompareBa(string partName, string binaryName, byte[] p1, byte[] p2)
		{
			if (p1.Length == p2.Length && p1.Length > 0)
			{
				for(var i = 0; i < p1.Length; i++)
					if (p1[i] != p2[i])
						throw new InvalidDataException(string.Format("taggant.pem {0} did not match to file {1} signature at position {2}.", partName, binaryName, i));
				return true;
			}
			throw new InvalidDataException(string.Format("taggant.pem {0} did not match to file {1} signature", partName, binaryName));
		}

		//     30 82 05 5b 30 82 03 43 a0 03 02 01 02 02 10 25 - typical line
		private static bool AppendToMemoryStream(string line, Stream spv)
		{
			if (!Regex.IsMatch(line, @"^[\s0-9A-F]+$", RegexOptions.IgnoreCase))
				return false;
			var chunk = StringToByteArray(line.Replace(" ", ""));
			spv.Write(chunk, 0, chunk.Length);
			return true;
		}

		private static byte[] StringToByteArray(string hex)
		{
			return Enumerable.Range(0, hex.Length)
							 .Where(x => x % 2 == 0)
							 .Select(x => Convert.ToByte(hex.Substring(x, 2), 16))
							 .ToArray();
		}
	}
}