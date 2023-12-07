using System;
using System.Collections;
using System.Data.SqlTypes;
using Microsoft.SqlServer.Server;

namespace ipn_sqlclr
{
	public partial class UserDefinedFunctions
	{
		[SqlFunction]
		public static SqlString VmpLicenseKeyGenerateNew(SqlInt32 productId, SqlString customerName, SqlString eMail, SqlDateTime maxBuildDt)
		{
			return new SqlString (Keygen.GenerateKey(productId.Value, customerName.Value, eMail.Value, maxBuildDt.Value));
		}

		[SqlFunction(FillRowMethodName = "FillRowVmpLicenseParseKey",
			TableDefinition = "[productId] int,[customerName] nvarchar(max),[eMail] nvarchar(max),maxBuildDT datetime")]
		public static IEnumerable VmpLicenseParseKey(String key)
		{
			yield return key;
		}

		public static void FillRowVmpLicenseParseKey(Object obj, out SqlInt32 productId, out SqlString customerName, out SqlString eMail, out SqlDateTime maxBuildDt)
		{
			var key = (string)obj;
			int productIdTmp;
			string customerNameTmp, eMailTmp;
			DateTime maxBuildDtTmp;
			Keygen.ParseKey(key, out productIdTmp, out customerNameTmp, out eMailTmp, out maxBuildDtTmp);
			productId = productIdTmp;
			customerName = customerNameTmp;
			eMail = eMailTmp;
			maxBuildDt = maxBuildDtTmp;
		}
	}
}
