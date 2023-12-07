using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;


namespace ipn_sqlclr.Test
{
	/// <summary>
	/// Summary description for TaggantWebTest
	/// </summary>
	[TestClass]
	public class TaggantWebTest
	{
		public TaggantWebTest()
		{
			//
			// TODO: Add constructor logic here
			//
		}

		private TestContext testContextInstance;

		/// <summary>
		///Gets or sets the test context which provides
		///information about and functionality for the current test run.
		///</summary>
		public TestContext TestContext
		{
			get { return testContextInstance; }
			set { testContextInstance = value; }
		}

		#region Additional test attributes

		//
		// You can use the following additional attributes as you write your tests:
		//
		// Use ClassInitialize to run code before running the first test in the class
		// [ClassInitialize()]
		// public static void MyClassInitialize(TestContext testContext) { }
		//
		// Use ClassCleanup to run code after all tests in a class have run
		// [ClassCleanup()]
		// public static void MyClassCleanup() { }
		//
		// Use TestInitialize to run code before running each test 
		// [TestInitialize()]
		// public void MyTestInitialize() { }
		//
		// Use TestCleanup to run code after each test has run
		// [TestCleanup()]
		// public void MyTestCleanup() { }
		//

		#endregion

		[TestMethod]
		public void PK()
		{
			string s = (string) UserDefinedFunctions.TaggantPrivateKeyGenerateNew();

		}

		[TestMethod]
		public void WS()
		{
			var tc = new TaggantConfig()
			{
				{ "CertificateProfileOID", "2.16.840.1.113733.1.16.1.3.1.4.1.38113944" },
				{ "ClientCertificate", "Registration Authority 1409598283849" },
				{ "CsrAlgorithm", "MD5withRSA" },
				{ "CsrSubject", "O=VMPSoft,OU=Build Server,E=support@vmpsoft.com,L=Yekaterinburg,ST=Ural,C=RU,CN=support@vmpsoft.com" },
				{ "EnrollmentUrl", "https://pki-ws.symauth.com/pki-ws/enrollmentService" },
				{ "EnrollVersion", "2.0" },
				{ "PolicyUrl", "https://pki-ws.symauth.com/pki-ws/policyService" },
				{ "PolicyVersion", "2.0" },
			};
			tc.ClientCertificate = UserDefinedFunctions.LocateCertificate(tc["ClientCertificate"]);
			var log = new List<LogItem>();
			TaggantWebService.GetPolicies(tc, log);

		}
		[TestMethod]
		public void WS2()
		{
			var tc = new TaggantConfig()
			{
				{ "CertificateProfileOid", "2.16.840.1.113733.1.16.1.3.1.4.1.38113944" },
				{ "ClientCertificate", "Registration Authority 1409598283849" },
				{ "CsrAlgorithm", "MD5withRSA" },
				{ "CsrSubject", "O=VMPSoft,OU=Build Server,E=support@vmpsoft.com,L=Yekaterinburg,ST=Ural,C=RU,CN=support@vmpsoft.com" },
				{ "EnrollmentUrl", "https://pki-ws.symauth.com/pki-ws/enrollmentService" },
				{ "EnrollVersion", "2.0" },
				{ "PolicyUrl", "https://pki-ws.symauth.com/pki-ws/policyService" },
				{ "PolicyVersion", "2.0" },
			};
			tc.ClientCertificate = UserDefinedFunctions.LocateCertificate(tc["ClientCertificate"]);
			var log = new List<LogItem>();
			TaggantWebService.CertRequestNew(tc, "ipn2217", "sqhunter123@mail.ru", "-----BEGIN RSA PRIVATE KEY-----\n" +
																				   "MIIEpAIBAAKCAQEAhjXJd3dMP5BE1mAXh3sAIiK9xUdw3R7lgRA0PtwIDdA1UX5v\n" +
			                                                                       "QHAWk7d5HwDB3V3T1WBRW3tFyjq7YRPB6QM0d2qPi+rpDsT6QlQSCAcVgdyiS1KW" +
			                                                                       "4OYVlVq2vjzVOeFhlgyaSRP06GGNYXBAL9e2yky7hs45OCdMGErLNCJ+Ja97wPtv" +
			                                                                       "Hk03C3ml9kvHegVecMp+5piy5fni+UsJ7toPZFo7HAbB23dc0Rfxj8BiGTM08yuQ" +
			                                                                       "U6nmpjDrtRrOyzIJDIriAKMJ4eTc8rF/y9uG3/ibwmkSH0MtYIvSfyim43ofL8rT" +
			                                                                       "ijhsO0yiMDMSAJyuAJCZjsCAYTMnrsas8c1P9QIDAQABAoIBAC2B43RcwT/0XUML" +
			                                                                       "hi7sKBlrCknwdXak2VEv+2ctGJYGeW3On06MMzuXRLycdx/mhsOdSzjnzbxKueqq" +
			                                                                       "1l96NLohKddZqfjWFb2T4CFUtZg5BdbghERx//OKtNhArFRZ9cr5Lv+EgtCg812M" +
			                                                                       "wFb4oARsjFGjb4d427aI9eoRoBCKt9A9CfqwxHyvclgtIG3W7FazvZ8s1KXhTh4i" +
			                                                                       "N3HHSUkxaADKl6uO4fclq+/tX1mffitIfOxlbZHUsOE1y8tKh8e0onWQ0l2F4aio" +
			                                                                       "SbBhgNo6QiihnVpfpYKn+fMqrZ3qDaky7r94avR7KHneMIwJG5diBrpf5+36xzi6" +
			                                                                       "0Jzy+WkCgYEAzltVepiWZGvmvofGO+PwbVtSYQQt1htE5bnfL4fwPYjSgPiboSTw" +
			                                                                       "5LBVHQWcaGqmNBrmnE1r0vk4Dam/a/Ok2M3+lZXzx3nJTGTjd3PzFP83WnPdplkE" +
			                                                                       "SHau7/EYc8tWGtEGzzUJiLT12n7EFxk8YASpL1znFeKhuDsVgJBkhHsCgYEApn88" +
			                                                                       "c5G7iE1GYCTSclKyU9avBJNiXNdbSy0fnizGQaSp1f3hCtiYbE7Yf6QXfu9gVKf+" +
			                                                                       "FD9a7HJa04QgRr5RmBV8ToLWzBdTZoud2fqO61lhePBQC+d1SyO5BwOzD9vk8BSR" +
			                                                                       "vmaoQoJf8GSzrmhcABo0zkD6uomYohO0wI4Q6k8CgYEAtBpU9YYdpJHkNyCrbHQZ" +
			                                                                       "0Ggm8xPBqZ/tNw9N8t8TV7GGABh7RF7IfOBFuOm/xAZo/wsHgR21YNIxEQO5VU+1" +
			                                                                       "7Z+EdiwFM3FgtnNLcGNbolTJjAGaT2hb657iOfrT26R5hzguWESzCITgGw4OuRZG" +
			                                                                       "cos+2l6cNaayfOfccXQUtucCgYBa+6qoKNoG6NttTJHnwUMLx4RKhtO4kkKkORtP" +
			                                                                       "D36jfn0EoECq8aORhCCQ17WzOtI0ULzqiZiBHxh8/3W30ua5qfwM1zjTvGdp4R+4" +
			                                                                       "b1BMUcKPGRtU9f3Futawe5gNMYfQnhzqpCSMe7w7nHwH8aVctPVoRF//MZPD9erP" +
			                                                                       "UpLxxwKBgQC121tfFNQwNuhQjdz/AD/IjkyDzhAKpOuVrbeFq+Hl31LZN/YBNJbX" +
			                                                                       "7jzQuQ4Pn+D+7ID4s/RVOg/zNvQ9AbWgDmfQ5+7RoEoooM2U0l6DzbVp6MaSw2mT" +
																				   "l6VgW2QubgrqIlt9Rk+zX6pSlN8LDjosYEbnzOhZ5B0Sc20RSkUcjg==\n" +
			                                                                       "-----END RSA PRIVATE KEY-----", log);

		}

	}
}
