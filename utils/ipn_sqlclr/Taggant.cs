using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Data.SqlTypes;
using System.IO;
using System.Security.Cryptography.X509Certificates;
using System.Xml;
using Microsoft.SqlServer.Server;
using Org.BouncyCastle.Crypto;
using Org.BouncyCastle.Crypto.Generators;
using Org.BouncyCastle.OpenSsl;
using Org.BouncyCastle.Security;
using System.Linq;

namespace ipn_sqlclr
{
	public class TaggantConfig : Dictionary<string, string>
	{
		public X509Certificate ClientCertificate { get; set; }
	}
	public partial class UserDefinedFunctions
	{
		public static X509Certificate LocateCertificate(string subjectName)
		{
			var certStore = new X509Store(StoreName.My, StoreLocation.LocalMachine);
			certStore.Open(OpenFlags.ReadOnly);
			X509Certificate2Collection certCollection = certStore.Certificates.Find(X509FindType.FindBySubjectName, subjectName, true);
			certStore.Close();

			if (0 == certCollection.Count)
			{
				throw new ArgumentException(string.Format("No valid client certificate found at LocalMachine.My by SubjectName '{0}'", subjectName), "subjectName");
			}
			if (1 == certCollection.Count)
			{
				return certCollection[0];
			}
			throw new ArgumentException(string.Format("More than one client certificate found at LocalMachine.My by SubjectName '{0}'", subjectName), "subjectName");

		}
		public static TaggantConfig GetTaggantConfig(SqlInt32 taggantConfigId)
		{
			var config = new TaggantConfig();
			using (var conn = new SqlConnection("context connection=true"))
			{
				conn.Open();
				var readConfigCmd = conn.CreateCommand();
				readConfigCmd.Parameters.Add(new SqlParameter("@taggantConfigId", taggantConfigId.Value));
				readConfigCmd.CommandText =
					"SELECT Name, Value FROM dbo.TaggantConfig WHERE ID=@taggantConfigId";
				using (var reader = readConfigCmd.ExecuteReader())
				{
					while(reader.Read())
					{
						config[reader.GetString(0)] = reader[1] as string;
					}
				}
			}
			config.ClientCertificate = LocateCertificate(config["ClientCertificate"]);
			return config;
		}

		[SqlFunction]
		public static SqlString TaggantPrivateKeyGenerateNew()
		{
			var g = new RsaKeyPairGenerator();
			g.Init(new KeyGenerationParameters(new SecureRandom(), 2048));
			var pair = g.GenerateKeyPair();

			using (var sw = new StringWriter())
			{
				new PemWriter(sw).WriteObject(pair);
				sw.Flush();
				return new SqlString(sw.ToString());
			}
		}

		[SqlProcedure]
		public static int TaggantCertRevoke(SqlInt32 taggantConfigId, SqlInt32 customerId)
		{
			var tc = GetTaggantConfig(taggantConfigId);
			var log = new List<LogItem>();
			try
			{
				string mail;
				using (var conn = new SqlConnection("context connection=true"))
				{
					conn.Open();
					var readCustomerCmd = conn.CreateCommand();
					readCustomerCmd.Parameters.Add(new SqlParameter("@CustomerID", customerId.Value));
					readCustomerCmd.CommandText =
						"SELECT EMail FROM dbo.Customer WHERE ID=@CustomerID";
					using (var reader = readCustomerCmd.ExecuteReader())
					{
						if (reader.Read())
						{
							mail = reader[0] as string;
						}
						else
						{
							throw new ArgumentException("Customer not found", "customerId");
						}
						if (string.IsNullOrWhiteSpace(mail))
							throw new InvalidOperationException("Customer EMail is not set");
					}
				}
				log.Add(new LogItem { MsgId = 1033, P = new[] { customerId.ToString(), tc["CertificateProfileOid"] } });

				TaggantWebService.CertRevoke(tc, mail, log);
				return 0;
			}
			catch (Exception ex)
			{
				log.Add(new LogItem { MsgId = 15, P = new[] { ex.Source, ex.Message, ex.StackTrace } });
				throw;
			}
			finally
			{
				FlushLog("TaggantCertRevoke", new SqlInt32(2), customerId, log);
			}
		}

		[SqlProcedure]
		public static int TaggantCertEnsure(SqlInt32 taggantConfigId, SqlInt32 customerId)
		{
			var tc = GetTaggantConfig(taggantConfigId);
			var log = new List<LogItem>();
			var id = "ipn" + customerId.Value;
			string taggantCert = null;
			try
			{
				string mail;
				string privateKey;
				using (var conn = new SqlConnection("context connection=true"))
				{
					conn.Open();
					var readCustomerCmd = conn.CreateCommand();
					readCustomerCmd.Parameters.Add(new SqlParameter("@CustomerID", customerId.Value));
					readCustomerCmd.CommandText =
						"SELECT EMail, PrivateKeyCert, TaggantCert FROM dbo.Customer WHERE ID=@CustomerID";
					using (var reader = readCustomerCmd.ExecuteReader())
					{
						if (reader.Read())
						{
							mail = reader[0] as string;
							privateKey = reader[1] as string;
							taggantCert = reader[2] as string;
						}
						else
						{
							throw new ArgumentException("Customer not found", "customerId");
						}
						if (string.IsNullOrWhiteSpace(mail))
							throw new InvalidOperationException("Customer EMail is not set");
						if (string.IsNullOrWhiteSpace(privateKey))
							throw new InvalidOperationException("Customer PrivateKeyCert is not set");
						if (!string.IsNullOrWhiteSpace(taggantCert))
							return 0; // ensured
					}
				}
				log.Add(new LogItem { MsgId = 14, P = new[] { customerId.ToString(), mail, tc["CertificateProfileOid"] } });

				taggantCert = TaggantWebService.CertRequestNew(tc, id, mail, privateKey, log);
				using (var conn = new SqlConnection("context connection=true"))
				{
					conn.Open();
					var writeCustomerCmd = conn.CreateCommand();
					writeCustomerCmd.Parameters.Add(new SqlParameter("@CustomerID", customerId.Value));
					writeCustomerCmd.Parameters.Add(new SqlParameter("@TaggantCert", taggantCert));
					writeCustomerCmd.CommandText =
						"UPDATE dbo.Customer SET TaggantCert=@TaggantCert WHERE ID=@CustomerID";
					writeCustomerCmd.ExecuteNonQuery();
				}
				return 1; // created new
			}
			catch (Exception ex)
			{
				log.Add(new LogItem { MsgId = 15, P = new[] { ex.Source, ex.Message, ex.StackTrace, taggantCert } });
				throw;
			}
			finally
			{
				FlushLog("TaggantCertEnsure", new SqlInt32(2), customerId, log);
			}
		}

		private static void FlushLog(string src, SqlInt32 refKindId, SqlInt32 refId, IEnumerable<LogItem> log)
		{
			using (var conn = new SqlConnection("context connection=true"))
			{
				conn.Open();
				foreach (var li in log)
				{
					var insLogCmd = conn.CreateCommand();
					insLogCmd.Parameters.Add(new SqlParameter("@RefKindID", refKindId));
					insLogCmd.Parameters.Add(new SqlParameter("@RefID", refId));
					insLogCmd.Parameters.Add(new SqlParameter("@MsgID", li.MsgId));
					int i;
					for (i = 0; i < 2; i++)
					{
						insLogCmd.Parameters.Add(
							new SqlParameter(string.Format("@xml{0}", i), SqlDbType.Xml)
							{
								Value = (li.Xml == null || li.Xml[i] == null)
									? DBNull.Value
									: (object)new SqlXml(new XmlTextReader(li.Xml[i].InnerXml, XmlNodeType.Document, null))
							});
					}
					insLogCmd.Parameters.Add(new SqlParameter("@P0", src));
					i = 1;
					foreach (var p in li.P)
					{
						insLogCmd.Parameters.Add(new SqlParameter(string.Format("@P{0}", i++), p));
					}
					for (; i <= 8; i++)
					{
						insLogCmd.Parameters.Add(new SqlParameter(string.Format("@P{0}", i), DBNull.Value));
					}

					insLogCmd.CommandText =
						"INSERT dbo.Log(RefID, RefKindID, MsgID, xml, xml2, P0, P1, P2, P3, P4, P5, P6, P7, P8)" +
						" VALUES(@RefID, @RefKindID, @MsgID, @xml0, @xml1, @P0, @P1, @P2, @P3, @P4, @P5, @P6, @P7, @P8)";

					insLogCmd.ExecuteNonQuery();
				}
			}
		}

		[SqlProcedure]
		public static SqlInt32 TaggantGetPolicies(SqlInt32 taggantConfigId)
		{
			var tc = GetTaggantConfig(taggantConfigId);
			var log = new List<LogItem>();
			try
			{
				var meta = new[]
				{
					new SqlMetaData("defaultName", SqlDbType.NVarChar, -1),
					new SqlMetaData("groupId", SqlDbType.Int),
					new SqlMetaData("oIdReferenceId", SqlDbType.Int),
					new SqlMetaData("certificateProfileId", SqlDbType.NVarChar, -1)
				};
				SqlDataRecord[] records = TaggantWebService.GetPolicies(tc, log).Select(x =>
				{
					var r = new SqlDataRecord(meta);
					r.SetSqlString(0, x.defaultName);
					r.SetSqlInt32(1, (int)x.group);
					r.SetSqlInt32(2, x.oIDReferenceID);
					r.SetSqlString(3, x.value);
					return r;
				}).ToArray();

				if (SqlContext.Pipe != null)
				{
					SqlContext.Pipe.SendResultsStart(new SqlDataRecord(meta));
					foreach (var r in records)
						SqlContext.Pipe.SendResultsRow(r);
					SqlContext.Pipe.SendResultsEnd();
				}
				return records.Length;
			}
			catch (Exception ex)
			{
				log.Add(new LogItem { MsgId = 15, P = new[] { ex.Source, ex.Message, ex.StackTrace } });
				throw;
			}
			finally
			{
				FlushLog("TaggantGetPolicies", new SqlInt32(), new SqlInt32(), log);
			}
		}
	}
}
