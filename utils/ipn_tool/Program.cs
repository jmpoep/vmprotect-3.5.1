using System;
using System.Data;
using System.Data.SqlClient;
using System.Globalization;
using System.IO;
using System.Text;
using System.Xml;

namespace ipn_tool
{
	static class Program
	{
		static int Main(string[] args)
		{
			switch (args.Length)
			{
				case 2:
					if (args[0] == "-export_wm")
						return ExportWatermarks(args[1]);
					break;
				case 3:
					switch (args[0])
					{
						case "-export_bl":
							return MergeBlacklist(args[1], args[2]);
						case "-export_tasks":
							return ExportTasks(null, args[1], args[2]);
						case "-check_ssvtest":
							return CheckSsvTest.Result(args[1], args[2]);
					}
					break;
				case 4:
					if (args[0] == "-export_task")
						return ExportTasks(args[1], args[2], args[3]);
					if (args[0] == "-check_wm")
						return Watermarks.CheckIfPresent(args[1], args[2], args[3]);
					break;
				case 5:
					if (args[0] == "-register_result")
						return RegisterResult(args[1], args[2], args[3], args[4]);
					break;
			}
			Console.WriteLine("IPN database tool. USAGE:");
			Console.WriteLine("a) Export watermarks: ipn_tool -export_wm <filename.dat>");
			Console.WriteLine("b) Merge project with blacklist: ipn_tool -export_bl <filenamein.vmp> <filenameout.vmp>");
			Console.WriteLine("c) Prepare for all actual end-user builds: ipn_tool -export_tasks <destPath> <version>");
			Console.WriteLine("d) Prepare for actual end-user build: ipn_tool -export_task <licenseID> <destPath> <version>");
			Console.WriteLine("e) Register build result: ipn_tool -register_result <licenseID> <bambooBuildUrl> <true|false> <version>");
			Console.WriteLine("f) Parse and check ssvtest.log: ipn_tool -check_ssvtest <ssvtest.log> <taggant.pem>");
			Console.WriteLine("g) Check if watermark present: ipn_tool -check_wm <filename.dat> <wm_id> <binary_file>");
			return 1;
		}

		private static int RegisterResult(string licenseId, string bambooBuildUrl, string success, string version)
		{
			int ret;
			using (var con = IpnConn())
			{
				using (var cmd = new SqlCommand("dbo.RegisterBuildTaskResult", con))
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.CommandTimeout = 600;
					cmd.Parameters.AddWithValue("@licenseID", int.Parse(licenseId));
					cmd.Parameters.AddWithValue("@bambooBuildUrl", bambooBuildUrl);
					cmd.Parameters.AddWithValue("@success", bool.Parse(success));
					cmd.Parameters.AddWithValue("@version", version);

					con.Open();
					ret = (int)cmd.ExecuteScalar();
					Console.WriteLine("RegisterResult (0 - OK): {0}.", ret);
				}
			}
			return ret;
		}

		private enum Platform
		{
			Windows,
			Linux,
			Mac
		}
		
		private static Platform RunningPlatform()
		{
			switch (Environment.OSVersion.Platform)
			{
			case PlatformID.Unix:
				// Well, there are chances MacOSX is reported as Unix instead of MacOSX.
				// Instead of platform check, we'll do a feature checks (Mac specific root folders)
				if (Directory.Exists("/Applications")
					& Directory.Exists("/System")
					& Directory.Exists("/Users")
					& Directory.Exists("/Volumes"))
					return Platform.Mac;
				else
					return Platform.Linux;

			case PlatformID.MacOSX:
				return Platform.Mac;

			default:
				return Platform.Windows;
			}
		}

		private static int ExportTasks(string licenseId, string rootPath, string version)
		{
			var ret = 1;
			if (!rootPath.EndsWith(Path.DirectorySeparatorChar + @"licenses"))
			{
				Console.WriteLine(@"RootPath '{0}' check failed (should ends with {1}licenses)", rootPath, Path.DirectorySeparatorChar);
			}
			else using (var con = IpnConn())
			{
				using (var cmd = new SqlCommand("dbo.ExportBuildTaskInfo", con))
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.CommandTimeout = 600;
					cmd.Parameters.AddWithValue("@licenseID", licenseId == null ? DBNull.Value : (object)int.Parse(licenseId));
					cmd.Parameters.AddWithValue("@filterOutVersion", version);
					cmd.Parameters.AddWithValue("@filterByOperatingSystem", RunningPlatform().ToString());

					con.Open();
					var cnt = 0;
					using (var r = cmd.ExecuteReader())
					{
						if (licenseId == null)
						{
							Directory.Delete(rootPath, true);
						}
						while (r.Read())
						{
							++cnt;
							var lId = r.GetInt32(0);
							var task = r.GetString(1);
							var key = r.GetString(2);
							var taggant = r.GetString(3);

							Directory.CreateDirectory(Path.Combine(rootPath, lId.ToString(CultureInfo.InvariantCulture)));
							var taskIniName = Path.Combine(rootPath, lId.ToString(), "task.ini");
							using (var taskIni = new StreamWriter(new FileStream(taskIniName, FileMode.OpenOrCreate, FileAccess.Write), new UTF8Encoding(false)))
							{
								var doc = new XmlDocument();
								doc.LoadXml(task);
								// ReSharper disable once PossibleNullReferenceException
								foreach (XmlAttribute attr in doc.DocumentElement.Attributes)
								{
									taskIni.WriteLine("{0}={1}", attr.Name, attr.Value);
								}
							}
							var keyName = Path.Combine(rootPath, lId.ToString(), "VMProtect.key");
							File.WriteAllText(keyName, key);
							var tagName = Path.Combine(rootPath, lId.ToString(), "taggant.pem");
							File.WriteAllText(tagName,
								String.Join("\r\n", taggant.Split(new[] {'\r', '\n'}, StringSplitOptions.RemoveEmptyEntries)));
						}

						Console.WriteLine("ExportTasks: {0} item(s) exported.", cnt);
						if (cnt > 0)
							ret = 0;
					}
				}
			}
			return ret;
		}

		private static int MergeBlacklist(string filenameIn, string filenameOut)
		{
			var ret = 1;
			using (var con = IpnConn())
			{
				using (var cmd = new SqlCommand("dbo.ExportBlacklist", con))
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.CommandTimeout = 600;
					con.Open();
					var xml = new XmlDocument();
					xml.Load(filenameIn);
					var lm = xml.SelectSingleNode("Document/LicenseManager");
					if (lm != null)
					{
						lm.InnerXml = "";
						var cnt = 0;
						using (var r = cmd.ExecuteReader())
						{
							while (r.Read())
							{
								XmlNode license = xml.CreateElement("License");
								XmlAttribute adate = xml.CreateAttribute("Date");
								adate.Value = r.GetDateTime(1).ToString("yyyy-MM-dd");

								XmlAttribute aname = xml.CreateAttribute("CustomerName");
								aname.Value = r.GetString(2);

								XmlAttribute aemail = xml.CreateAttribute("CustomerEmail");
								aemail.Value = r.GetString(3);

								XmlAttribute akey = xml.CreateAttribute("SerialNumber");
								akey.Value = r.GetString(4);

								if (license.Attributes != null)
								{
									license.Attributes.Append(adate);
									license.Attributes.Append(aname);
									license.Attributes.Append(aemail);
									license.Attributes.Append(akey);
									XmlAttribute ablocked = xml.CreateAttribute("Blocked");
									ablocked.Value = "1";
									license.Attributes.Append(ablocked);
								}
								lm.AppendChild(license);

								cnt++;
							}
						}
						xml.Save(filenameOut);
						Console.WriteLine("MergeBlacklist: {0} item(s) exported.", cnt);
						if (cnt > 0)
							ret = 0;
					}
				}
			}
			return ret;
		}

		private static int ExportWatermarks(string filenameOut)
		{
			var ret = 1;
			using (var con = IpnConn())
			{
				using (var cmd = new SqlCommand("dbo.ExportWatermarks", con))
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.CommandTimeout = 600;
					con.Open();
					var xml = new XmlDocument();
					xml.InsertBefore(xml.CreateXmlDeclaration("1.0",null,null), xml.DocumentElement);
					XmlNode rootNode = xml.CreateElement("Document");
					xml.AppendChild(rootNode);
					XmlNode wms = xml.CreateElement("Watermarks");
					rootNode.AppendChild(wms);

					int cnt = 0;
					using (var r = cmd.ExecuteReader())
					{
						while (r.Read())
						{
							XmlNode wm = xml.CreateElement("Watermark");
							XmlAttribute aid = xml.CreateAttribute("Id");
							aid.Value = cnt.ToString(CultureInfo.InvariantCulture);

							XmlAttribute aname = xml.CreateAttribute("Name");
							aname.Value = r.GetString(0);

							XmlAttribute areadablename = xml.CreateAttribute("ReadableName");
							areadablename.Value = r.GetString(1);

							XmlAttribute aemail = xml.CreateAttribute("EMail");
							aemail.Value = r.GetString(2);

							wm.InnerText = r.GetString(3);

							if (wm.Attributes != null)
							{
								wm.Attributes.Append(aid);
								wm.Attributes.Append(aname);
								wm.Attributes.Append(areadablename);
								wm.Attributes.Append(aemail);
								if (r.GetInt32(4) == 0)
								{
									XmlAttribute aenabled = xml.CreateAttribute("Enabled");
									aenabled.Value = "0";
									wm.Attributes.Append(aenabled);
								}
							}
							wms.AppendChild(wm);

							cnt++;
						}
					}
					XmlAttribute acnt = xml.CreateAttribute("Id");
					acnt.Value = cnt.ToString(CultureInfo.InvariantCulture);
					if (wms.Attributes != null) 
						wms.Attributes.Append(acnt);

					xml.Save(filenameOut);
					Console.WriteLine("ExportWatermarks: {0} item(s) exported.", cnt);
					if (cnt > 0)
						ret = 0;
				}
			}
			return ret;
		}

		private static SqlConnection IpnConn()
		{
			return new SqlConnection("server=scb-serv;database=ipn;user id=ipn_reader;password=rAqiEiGBOh39;Connection Timeout=300");
		}

	}
}
