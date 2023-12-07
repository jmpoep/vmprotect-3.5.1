using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml;

namespace ipn_tool
{
	internal static class Watermarks
	{
		internal static int CheckIfPresent(string xmlDb, string id, string binFile)
		{
			try
			{
				var wm = LoadWm(xmlDb, id);
				return FindWm(wm, binFile) ? 0 : 1;
			}
			catch (Exception ex)
			{
				Console.Error.WriteLine(ex);
				return 1;
			}
		}

		private static string LoadWm(string xmlDb, string id)
		{
			var s = new FileStream(xmlDb, FileMode.Open, FileAccess.Read);
			using (var x = new XmlTextReader(s))
			{
				while (x.Read())
				{
					if (x.NodeType == XmlNodeType.Element && x.Name == "Watermark" && x.GetAttribute("Name") == id)
					{
						if(x.Read() && x.NodeType == XmlNodeType.Text && !string.IsNullOrEmpty(x.Value))
							return x.Value;
					}
				}
			}
			throw new KeyNotFoundException("WM.id=" + id);
		}

		private static bool FindWm(string wm, string binFile)
		{
			var wmna = WmStringToNibbleArray(wm.ToUpper());
			var filesToCheck = new List<string>();
			if (Directory.Exists(binFile))
			{
				filesToCheck.AddRange(Directory.EnumerateFiles(binFile));
			}
			else
			{
				filesToCheck.Add(binFile);
			}
			return filesToCheck.All(s => CheckFile(s, wmna));
		}

		private static bool CheckFile(string binFile, int[] wmna)
		{
			var filena = new List<int>(wmna.Length + 1);
			using (var s = new FileStream(binFile, FileMode.Open, FileAccess.Read))
			{
				while (true)
				{
					var nextByte = s.ReadByte();
					if (nextByte == -1)
					{
						Console.Error.WriteLine("Watermark was not found in {0}", binFile);
						return false;
					}
					filena.Add(nextByte >> 4);
					filena.Add(nextByte & 15);
					while (filena.Count >= wmna.Length)
					{
						if (MatchWm(wmna, filena))
						{
							Console.WriteLine("Watermark in {0}: FOUND", binFile);
							return true;
						}
						filena.RemoveAt(0);
					}
				}
			}
		}

		private static bool MatchWm(IEnumerable<int> wmna, IList<int> filena)
		{
			return !wmna.Where((t, i) => t != -1 && t != filena[i]).Any();
		}

		private static int[] WmStringToNibbleArray(string wm)
		{
			if(!Regex.IsMatch(wm, @"[A-Z0-9\?]+"))
				throw new ArgumentException("Watermark contains bad symbols: " + wm, "wm");

			var ret = new int[wm.Length];
			var idx = 0;
			foreach (var ch in wm)
			{
				if (ch == '?')
					ret[idx++] = -1;
				else if (ch <= '9') 
					ret[idx++] = ch - '0';
				else 
					ret[idx++] = ch - 'A' + 10;
			}
			return ret;
		}

	}
}