using System;
using System.Collections.Generic;
using System.Security.Cryptography;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using VMProtect;

namespace UnitTestProject
{
	[TestClass]
	public class LicensingManagerTests
	{
		private class Data
		{
			public void PushByte(byte b) { _data.Add(b); }
			//void PushDWord(int d) { PushBuff(BitConverter.GetBytes(d)); }
			/*void PushQWord(ulong q) { PushBuff(&q, sizeof(q)); }
			void PushWord(ushort w) { PushBuff(&w, sizeof(w)); }*/
			public void PushBuff(byte[] buff)
			{
				_data.AddRange(buff);
			}
			/*public void InsertByte(int position, byte value)
			{
				_data.Insert(position, value);
			}*/

			/*uint32_t ReadDWord(size_t nPosition) const
			{
				return *reinterpret_cast<const uint32_t *>(&m_vData[nPosition]);
			}*/

			public void WriteDWord(int position, int value)
			{
				foreach (var b in BitConverter.GetBytes(value))
				{
					_data[position++] = b;
				}
			}

			public int Size => _data.Count;
			//public void clear() { _data.Clear(); }
			//public bool empty() { return _data.Count == 0; }
			//void resize(int size) { _data.resize(size); }
			//void resize(int size, uint8_t value) { m_vData.resize(size, value); }
			public byte[] ToArray() { return _data.ToArray(); }

			private readonly List<byte> _data;

			public Data(int sz)
			{
				_data = new List<byte>(new byte[sz]);
			}
		}

		[TestMethod]
		public void ParseSerial()
		{
			//var key = new byte[] {1, 2, 3, 4, 5, 6, 7, 8};
			var data = new Data(sizeof(uint) * (int)LicensingManager.Fields.Count);

			var publicExp = Convert.FromBase64String("AAEAAQ==");
			var modulus = Convert.FromBase64String("pwUqwaM8IOukyx06Lvi5YNQ70JE7pwg7K+pmM/vCe1CUseHKFM1v1m11geDjVsAt38AnaiFs3JhtTs80ySCIxOSyvMw6Cd52k6N6dn7LAx1mxQLJLhYeMMJYbplMHnMLwYN0+IO58OVbEqRyaJV2ExolnK2EYZL7QRXujGY7/sOoOMF3p6GsWJK6kkBJICIoL9hHWBQMO6/9rmls/+EhaWuP80Vx0+H2OlrQ58K+TJeyE393cvb4QufiEPpCNaB50Klee9QUnsjSW/bTnmGn4Bi5+cowRbawUY73Q5I58fMAXiH9ueDPuNMR9YKDgW9GxunLmYkbuwqIp/v7kw3cfMBM0ihhB0B8UhjyAMAGLzJWX3H/H6Zrz41g9PbPjTAxfsTaCrxoqjaTaO4zk9YsI//VX9Fhivcy913SevBpNandziGfYH/oHW2xDy9AfwkE1wuIBlLj7c/k8U1YmmRAmkoCzlmB7EU4ClNltboh1uARUQ6wW30upppnuYhGkTy7");
			var black = Convert.FromBase64String("A7zybXsboP8UVQ4x8mCSPVlm4W8G2gMbKOiLDFIX28dpdl6AzdqGCxIeiXRfaPBgeGeyL1BB+3pLVJ9SGkjc/BXy1hYkjJCwacADVEWoR14blpa3hdx+R/WHe8uhS9ShV1hxviIKRt4UJbIjyqH7tMu2p8KuVbB0IwM0dFPnVujgNmNN2/BeOQl/RFMmD+TpLzFHetvt0Dx7rk8MKINv3SoyL97QWNvwCbL35JIZN5DtMdZ3MfO/fxz/1kSTNbccciqEljRiC74zMsNz2LNkf1hwxES96yGBsd2oOTe+DJx43j8iZ7UD75iwIGKoPxysOYnHTewT2ofK9d6SMl4rIyxt6TI+KzXppRyhVieHtEIo9/NIm7ABnkLTJvy4tjFftPocJP3E5v9ra8YBQ2y3PKz04BkesCeKiVPyBqy9phxHtjKpIimlm73GSfDMVZ+xgLsn/Ui6XSW8kB8ai+rEA1KFosfmsVPASiuXJAHJaNfY4ULKZWOfGcoPDh1KjhmFq7INiQaMy+rml/EiKv4p9k3vomv41ll5IoIVkxZaMY8Gtkl5UYWWJUMlJgphM9+LOkWCLX1fm7ZUiJbfHkmVTTFZ6SxhxoeO73yovFdt37I/17tbM0jjQjC1Q172ZPQmWPBP2NaPTXglJdkSKWYWw8pG6EJh+eRrfZ1USmdkU2TI0FUDSwJ2F2RSkObaDx4WFrBz0xddLmEl0XEJaQI/TTRZk5hQB/20NPzavgiAQ39p62LR7hZTqnR4zshiFv2sChe4x2p4XzBx4NKx1Zw6QaZrfwX47R3dc2K80jHIisj9Ltnr3qUgr5S0Nbp6+8BKB6RFcPuPAi24SVh7e5KgvoKNB/FzJqZnE9FCiryEeRbRwU26g3l+orZTM/jm1niFKlgrvdRweVqVza4JBnbeTGF2t8PstMH6Vx4+gpoJWyoQY6acm5MSFyl2DLXCcd6MmoRyp30Ge209zoOjj7c5PGRYxjiiUO04gaGhh7YyUp7VTHmd2llSQaKhsyw/7k3OqEKTinSRmdiDNsmfp8fOqtd388jGyhME6zzda8u1Ex6uvHkrk2arjVSeQk3XED4ZPqyRbLDZYGZcp9HheDdciX56rK39NTXetOWN8p4KW6aMa91EUTaCiafQVhO6dYzO+1ybRjOR7ND7nOnt9zUEerzNbIkCFT6uGTqtue0FZe4Zxunlv9D1mA4266xWfZfIPSzXD2cVAerBjr2BtYHOolDP1dRu1JcPwXGxOMiDL76x1NghovHsQxlwcfWT9CO+ywvhEkiwirt0UsTbACSslWD2sNLTdQ1aLNkqM9FjJzN7uKHk/J4OZtmNFRsxxZUvWYKYb/+q93FEsbMr5YfSPLlezvf6fL3k6tSKB5sFuk/rsgeBQYOwUGIKDRc8c0yNd8kEXOxvmzCGEv2/95Lh7XxXEUaHYKki7TR73v+H6pIiYmTOHg/Z9F4OjzHxb6HIUq6bzNywpzguJjBKqhnoRfJMwBr9P3NQ+CU5VaBXwxTlMWtW9Ihou7a+Hio34w2YGYdtr8BzMjT03VrAOtLH4V0HZj/UTAkTFaGDK/bdOAYXH/fz9GVBVU94hB86ii4e9ulYanZkiRJMDwTlNTup5jRVAR7/nM4H0Q0evDGRo7k4IP5CtLb6fg==");

			data.WriteDWord((int)LicensingManager.Fields.PublicExpOffset * sizeof(uint), data.Size);
			data.WriteDWord((int)LicensingManager.Fields.PublicExpSize * sizeof(uint), publicExp.Length);
			data.PushBuff(publicExp);

			data.WriteDWord((int)LicensingManager.Fields.ModulusOffset * sizeof(uint), data.Size);
			data.WriteDWord((int)LicensingManager.Fields.ModulusSize * sizeof(uint), modulus.Length);
			data.PushBuff(modulus);

			data.WriteDWord((int)LicensingManager.Fields.BlacklistOffset * sizeof(uint), data.Size);
			data.WriteDWord((int)LicensingManager.Fields.BlacklistSize * sizeof(uint), black.Length);
			data.PushBuff(black);

			var crcPos = data.Size;
			data.WriteDWord((int)LicensingManager.Fields.CRCOffset * sizeof(uint), crcPos);
			var size = crcPos + 16;

			using (var hash = new SHA1Managed())
			{
				var p = hash.ComputeHash(data.ToArray());
				for (var i = crcPos; i < size; i++)
				{
					data.PushByte(p[i - crcPos]);
				}
			}

			var licensingManager = new LicensingManager(data.ToArray());
			Assert.AreEqual(SerialState.Blacklisted, licensingManager.SetSerialNumber("H2GfYz579KQ5poUTxKRWsUd7o+1XSwaJrK7CmwG15rNuo3Wa+vy660rJkRaWTrELbyyIe2k15973P13WJvxDR8myQCMi8Pv9D9k9iblCTAZL9vOcX55g2Vk4i+x+DEDn601kjvkL7RvHug1SYq6GqKm4dnApwlGvbnSebSDwrhh0E5g9I/XA5pa7hQUQwcBoXq6A7e7Blj8FbJ1JBdYUJY7RavgFE9KYfTXn5ceCwPr0gR3A++W66amQdxWnFxyyOFwfCPuZDk+LCgqqAgMyj5PRPcLA3nanXLDLPGva1wa1EEP0qvx6yCSpDURP94GxQGY1xjPlZagbuLaYyWn7bb/sLsYNXVHE1a+YNFORn890tbZ1D6i+wTEa254oF3yKa5GYTQmRWJQR+OXqaTK/wNG4y8dAUisOmQpevrSrD7pQj7ZLGOChmw+KWB6SozSHtIMY665ji9tMP8mq8OUSVSZ9N9q3Zh/xnW0W8sGck5IzTr3JtT0a3iOXSYfijpmy"));
			Assert.AreEqual(SerialState.Blacklisted, licensingManager.GetSerialNumberState());
			var snd = new SerialNumberData();
			Assert.IsTrue(licensingManager.GetSerialNumberData(snd));
			Assert.AreEqual("", snd.EMail); //a20071234@163.com
			Assert.AreEqual("", snd.UserName); //Su Ying
			Assert.AreEqual(DateTime.MaxValue, snd.Expires);
			Assert.AreEqual(DateTime.MaxValue, snd.MaxBuild);
			Assert.AreEqual(0, snd.RunningTime);
			Assert.AreEqual(SerialState.Blacklisted, snd.State);
			Assert.AreEqual(0, snd.UserData.Length);

			Assert.AreEqual(SerialState.Success, licensingManager.SetSerialNumber("V5z/4JRVkIhN/D7KkHUBfM2ww3VMxq4r7SgRViTG840294nBiqLer9QUyuX/oyS5sxwzgtMruYYK2n8PvrilVRhFj3rgoK67V + 0 / kJxNmypnWY + PbNurXbimp8KfTF6aOydLQyo26M2iRJhnPCXXInXB4QMi5dPNf41cdVelwt + C5hSfV7zPeJZRLbWv + AMScBJEmy1AW3AWUokfHUMSonq75LxBc3jLnDESz8UGTNWrIXSEiRjrueszSM7uGEmWemUjS + MzgL + F + DfsKNTc2KTEctprxQmxcJZsN4rZS0q8UJo3eA0HXgrrFlxYkLqgcq + 8 / 018W3l79nR17ZDQsUuJFfrElY8F6OGtbis2j9YyCszcWoKQLB3bSgWrKXPAlrEN4VuxMk0wbq + sYZLIt / npAmFheE7wHhnN1PubK84BpQFZKnkrIrsC43PJ2ss2WOQl / vqTxhTbbDyPEE69NW8R + fUanth5sglfableZjt4WH + hwgiGL + D8wHQlWgT7"));
			Assert.AreEqual(SerialState.Success, licensingManager.GetSerialNumberState());
			Assert.IsTrue(licensingManager.GetSerialNumberData(snd));
			Assert.AreEqual("jleber@onvisionnetworks.com", snd.EMail);
			Assert.AreEqual("Onvision Networks Limited", snd.UserName);
			Assert.AreEqual(DateTime.MaxValue, snd.Expires);
			Assert.AreEqual(new DateTime(2017, 4, 11), snd.MaxBuild);
			Assert.AreEqual(0, snd.RunningTime);
			Assert.AreEqual(SerialState.Success, snd.State);
			Assert.AreEqual(1, snd.UserData.Length);
			CollectionAssert.AreEqual(snd.UserData, new byte[] {4});
		}

		/*[TestMethod]
		public void ParseSerialTimeLimitedHWID()
		{
			//TODO
		}*/
	}
}