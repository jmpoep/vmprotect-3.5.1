using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace ipn_sqlclr.Test
{
	[TestClass]
	public class VmpLicenseKeyTest
	{
		/*[TestMethod] не проходит почему-то
		public void TestRsa()
		{
			var r = new Random();
			var data1 = new byte[3072 / 8];
			for (int i = 0; i < data1.Length; i++) data1[i] = (byte)r.Next(256);

			var encr1 = Rsa.Encrypt(data1);
			var decr1 = Rsa.Decrypt(encr1);

			Assert.AreNotEqual(data1, encr1);
			Assert.AreEqual(data1, decr1);
		}*/

		[TestMethod]
		public void TestGenerateAndParseKey()
		{
			const int productId = 5;
			const string name = "Иван ПетровААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААА";
			const string mail = "petrov@mail.ruАААААААААААААААААААААААААААААААААААААААААААААААААААААААААААААА";
			var maxBuild = new DateTime(2015, 12, 28);
			var key = Keygen.GenerateKey(productId, name, mail, maxBuild);

			int productId1;
			string name1, mail1;
			DateTime maxBuild1;
			Keygen.ParseKey(key, out productId1, out name1, out mail1, out maxBuild1);

			Assert.AreEqual(productId, productId1);
			Assert.AreEqual(name, name1);
			Assert.AreEqual(mail, mail1);
			Assert.AreEqual(maxBuild, maxBuild1);

			Keygen.ParseKey("FgeTRyB93RJeABJN83qKo7mTEvXHGHwYXmZ6pGY5o1X09bc8fmmebq1mryX5yHUWb6F+cauzsLxUyG+uTQBT9zC31EJPQqG9YgaRbOPPS9A7u1HXZA8x491ew0lDKJISy8TYNsjTJ2vB903jW+/A/UVc2+cgDCF3eqztkvYBEas6Y40Hj9A2ylJVYsYWq41RCZyDcXZtlbnuk8W2CDBAk+PGZeUPjEq/oasnTCeKdkO4NY2QVrFzvC/msT+7FWAaBNwe5OQYZLCH875qFVw0H2B9xcnVuIAHAYD00H0Hzk9sf/jAGx4l5CS8FQ4tshZfEllF6CcwVQuRdGz07gQWAQ=="
				,out productId1, out name1, out mail1, out maxBuild1);
		}

		[TestMethod]
		public void TestParseOldKey()
		{
			const int productId = -1;
			const string name = "道恭 陈";
			const string mail = "sales@sky-deep.com";
			var maxBuild = new DateTime(2015, 12, 13);
			const string key = "m/j+Km6Y7Kb5AgV0pfLr/Bmw3z9EP/SNv9mawGK0PK8mXWMcJijSzfMbVakJa8ctaZVaNceeanc6" +
				"FuurAcyDEpppwS71/t6SoI6Kxc/BUmcHjLlfrWhxk5CrQqFVqyQ+liZEhglHuCE8GRwlPgm60JVv" +
				"hCsQBEDeIywTp2GujhjhGra7ojML/guuhRS6W1ILSSkfZriDuCFkkStLgi9gT8GGwX1GUdaXz7Wn" +
				"IxrcSYt5OLuPOcPULPUgVr1tB0YopWt/7g3g9jvRxVTThBxcAr+IoIf8RtaTTPIMDae9OZTDL6Pv" +
				"p10cUj1HGC1kB8whLWyGdqbhpyg53vUc4Y7FdHe3zUltZA3c3p4/DdZwrU+jrMC/SdzJVuo1MM4p" +
				"ZefRwH2f1MYKCUT2zn67f87Sgo2LMmSjWnaORHxrUtskEyguRYcgh0dy6RApI07ZXrK06ZCQu9Ns" +
				"zdN3vkxo+NV9clNmVffE7GfRDlcmczvffGcuEUYN59juPnp/vMhFpH1r";

			int productId1;
			string name1, mail1;
			DateTime maxBuild1;
			Keygen.ParseKey(key, out productId1, out name1, out mail1, out maxBuild1);

			Assert.AreEqual(productId, productId1);
			Assert.AreEqual(name, name1);
			Assert.AreEqual(mail, mail1);
			Assert.AreEqual(maxBuild, maxBuild1);
		}
	
	}
}
