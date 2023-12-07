using System;
using System.Net;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using VMProtect;

namespace UnitTestProject
{
	[TestClass]
	public class UnitTest1
	{
		[TestMethod]
		// ReSharper disable once InconsistentNaming
		public void TestRC5()
		{
			var key = CipherRC5.CreateRandomKey();
			var rc5 = new CipherRC5(key);

			var dataStr = "Тестовые данные для TestRC5";
			var dataBytes = Encoding.UTF8.GetBytes(dataStr);
			Array.Resize(ref dataBytes, (dataBytes.Length + 7)/8*8);

			var encodedBytes = rc5.Encrypt(dataBytes);
			var dataBytes2 = rc5.Decrypt(encodedBytes);

			CollectionAssert.AreNotEqual(dataBytes, encodedBytes);
			CollectionAssert.AreEqual(dataBytes, dataBytes2);
		}

		[TestMethod]
		public void TestHwid()
		{
			var hwid = new HardwareID();
			Assert.AreNotSame(0, hwid.ToString().Length);
		    var machine = Dns.GetHostName().ToUpperInvariant();
		    var txtHwid = hwid.ToString();
            if (machine == "ACER-V3")
				Assert.AreEqual("2D3kAD28P7M7soVoCjyOUA==", txtHwid);
            else if (machine == "WORK")
                Assert.AreEqual("AIUkDL3k4HTLFrVrPmOIpw==", txtHwid);
            else if (machine == "BUILD7N1")
                Assert.AreEqual("HBG8w5UkOMh714KeZow8vg==", txtHwid);
            else 
                Assert.Fail("Append branch for machine '{0}' with result '{1}'", machine, txtHwid);
        }

		[TestMethod]
		public void TestHwidCompare()
		{
			var hwid = new HardwareID();
			var cur = hwid.GetBytes();
			var test = new byte[cur.Length];

			Array.Copy(cur, test, cur.Length);
			// change one id
			for (var i = 0; i < cur.Length; i += 4)
			{
				test[i] = cur[i];
				switch (test[i] & 3) {
					case 3:	
						test[i] ^= 4;
						break;
				}
			}
			Assert.IsTrue(hwid.IsCorrect(test));


			// change two ids
			Array.Copy(cur, test, cur.Length);
			for (var i = 0; i < cur.Length; i += 4)
			{
				switch (test[i] & 3)
				{
					case 2:
					case 3:
						test[i] ^= 4;
						break;
				}
			}
			Assert.IsFalse(hwid.IsCorrect(test));

			// change three ids
			Array.Copy(cur, test, cur.Length);
			for (var i = 0; i < cur.Length; i += 4)
			{
				switch (test[i] & 3)
				{
					case 2:
					case 3:
						test[i] ^= 4;
						break;
				}
			}
			Assert.IsFalse(hwid.IsCorrect(test));

			// change cpu`s id
			Array.Copy(cur, test, cur.Length);
			for (var i = 0; i < cur.Length; i += 4)
			{
				switch (test[i] & 3)
				{
					case 0:
						test[i] ^= 4;
						break;
				}
			}
			Assert.IsFalse(hwid.IsCorrect(test));
		}
	}
}
