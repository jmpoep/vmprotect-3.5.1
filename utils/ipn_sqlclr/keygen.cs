using System;
using System.IO;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;

namespace ipn_sqlclr
{
	enum SerialNumberChunks : byte
	{
		Version = 0x01,		//	1 byte of data - version
		UserName = 0x02,	//	1 + N bytes - length + N bytes of customer's name (without enging \0).
		Email = 0x03,		//	1 + N bytes - length + N bytes of customer's email (without ending \0).
		ProductCode = 0x07,	//	8 bytes - used for decrypting some parts of exe-file
		UserData = 0x08,	//	1 + N bytes - length + N bytes of user data
		MaxBuild = 0x09,	//	4 bytes - (year << 16) + (month << 8) + (day)
		End = 0xFF			//	4 bytes - checksum: the first four bytes of sha-1 hash from the data before that chunk
	};

	public static class Rsa
	{
		private const string PublicExpB64 = "AAEAAQ==";
		private const string PrivateExpB64 = "CXHXWx/Z9JqetQWwFpvmD72wrDiqQOXMQs18fhAMjWCfJ/f2r3p2io+iB3gqIuu3LGH3WJ8PQuIzvDMnbwAx+8BbAyYhWhGEbxDdifndjQ2KlDV2Hu8NQgCbc5Wjok0rKwQ+Bxeb2i1+Gu3FsnhRNv9RhSyiwcnH/4Q3+ySE3AFAcAUwuQABePjDKCYOfIyx7RKz5h0sG+v10nkPuuCGPSnh+AXDTBIJFH+yNIjkrfweC9A3dv7URyRJumAMgm/SnDU76rTkFw9vZpupQeMtMtIsZIkeFSngip9KImD5zzbb2vKD63Cg9W/Yvqgvro/d+cR5n6P0t4DzfanNIFRGpFrX8/Q5VjuezDKw/4YbsFYwOhzJPRxglmCEjh8cpfxJ11cUXa/hNBV4c4Dp29D0F+w01OlBnFb1Ck9VXur2qJCsqcWtjsnt/VITsxa1jzr+3C2+uvaI4JSd7yLEnTqSaSsRfWuhDXgjY/YWhmyvMzeQeXBGOXKt2j2lY2Fm0WJx";
		private const string ModulusB64 = "pwUqwaM8IOukyx06Lvi5YNQ70JE7pwg7K+pmM/vCe1CUseHKFM1v1m11geDjVsAt38AnaiFs3JhtTs80ySCIxOSyvMw6Cd52k6N6dn7LAx1mxQLJLhYeMMJYbplMHnMLwYN0+IO58OVbEqRyaJV2ExolnK2EYZL7QRXujGY7/sOoOMF3p6GsWJK6kkBJICIoL9hHWBQMO6/9rmls/+EhaWuP80Vx0+H2OlrQ58K+TJeyE393cvb4QufiEPpCNaB50Klee9QUnsjSW/bTnmGn4Bi5+cowRbawUY73Q5I58fMAXiH9ueDPuNMR9YKDgW9GxunLmYkbuwqIp/v7kw3cfMBM0ihhB0B8UhjyAMAGLzJWX3H/H6Zrz41g9PbPjTAxfsTaCrxoqjaTaO4zk9YsI//VX9Fhivcy913SevBpNandziGfYH/oHW2xDy9AfwkE1wuIBlLj7c/k8U1YmmRAmkoCzlmB7EU4ClNltboh1uARUQ6wW30upppnuYhGkTy7";

		static BigInteger B2Bi(byte[] b) //reverse & make positive
		{
			Array.Reverse(b);
			var b2 = new byte[b.Length + 1];
			Array.Copy(b, b2, b.Length);
			return new BigInteger(b2);
		}

		private static readonly BigInteger PublicExp = B2Bi(Convert.FromBase64String(PublicExpB64));
		private static readonly BigInteger PrivateExp = B2Bi(Convert.FromBase64String(PrivateExpB64));
		private static readonly BigInteger Modulus = B2Bi(Convert.FromBase64String(ModulusB64));

		public static byte[] Encrypt(byte[] paddedData)
		{
			var x = B2Bi(paddedData);
			var y = BigInteger.ModPow(x, PrivateExp, Modulus);

			byte[] ret = y.ToByteArray();
			Array.Resize(ref ret, paddedData.Length);
			Array.Reverse(ret);
			return ret;
		}

		public static byte[] Decrypt(byte[] data)
		{
			var x = B2Bi(data);
			var y = BigInteger.ModPow(x, PublicExp, Modulus);

			byte[] ret = y.ToByteArray();
			Array.Reverse(ret);
			return ret;
		}

	}
	public static class Keygen
	{
		public static void ParseKey(string key, out int productId, out string customerName, out string eMail, out DateTime maxBuildDt)
		{
			productId = -1;
			customerName = null;
			eMail = null;
			maxBuildDt = new DateTime();

			var crypted = Convert.FromBase64String(key);
			var data = Rsa.Decrypt(crypted);
			int i;
			for (i = 2; i < data.Length && data[i] != 0; i++) {
			}

			i++;
			var pos = i;
			while (pos < data.Length)
			{
				var b = data[pos++];
				switch (b)
				{
					case (byte) SerialNumberChunks.Version:
						b = data[pos++];
						if (b < 1 || b > 2)
							throw new InvalidDataException("SerialNumberChunks.Version");
						break;
					case (byte) SerialNumberChunks.UserName:
						b = data[pos++];
						customerName = Encoding.UTF8.GetString(data, pos, b);
						pos += b;
						break;
					case (byte) SerialNumberChunks.Email:
						b = data[pos++];
						eMail = Encoding.UTF8.GetString(data, pos, b);
						pos += b;
						break;
					case (byte)SerialNumberChunks.ProductCode:
						pos += 8;
						break;
					case (byte) SerialNumberChunks.UserData:
						b = data[pos++];
						if (b == 0)
							productId = 0;
						else if(b != 1)
							throw new InvalidDataException("Invalid ProductID");
						else
							productId = data[pos];
						pos += b;
						break;
					case (byte) SerialNumberChunks.MaxBuild:
						maxBuildDt = new DateTime(data[pos + 2] + 256 * data[pos + 3], data[pos + 1],data[pos]);
						pos += 4;
						break;
					case (byte) SerialNumberChunks.End:
						if (pos + 4 > data.Length)
							throw new InvalidDataException("No checksum");
						{
							SHA1 sha = new SHA1Managed();
							sha.Initialize();
							var hash = sha.ComputeHash(data, i, pos - 1 - i);
							for (int j = 0; j < 4; j++)
							{
								if(data[pos + j] == hash[3 - j])
									continue;
								throw new InvalidDataException("Invalid checksum");
							}
						}
						return;
				}
			}
			throw new InvalidDataException("No checksum");
		}

		public static string GenerateKey(int productId, string customerName, string eMail, DateTime maxBuildDt)
		{
			var data = new MemoryStream();
			data.WriteByte((byte)SerialNumberChunks.Version);
			data.WriteByte(1);

			data.WriteByte((byte)SerialNumberChunks.UserName);
			var utfCustomer = Encoding.UTF8.GetBytes(customerName);
			if (utfCustomer.Length > 255)
				throw new ArgumentException("Customer name too long", "customerName");
			data.WriteByte((byte)utfCustomer.Length);
			data.Write(utfCustomer, 0, utfCustomer.Length);

			data.WriteByte((byte)SerialNumberChunks.Email);
			byte[] utfeMail = Encoding.UTF8.GetBytes(eMail);
			if (utfeMail.Length > 255)
				throw new ArgumentException("EMail too long", "eMail");
			data.WriteByte((byte)utfeMail.Length);
			data.Write(utfeMail, 0, utfeMail.Length);

			data.WriteByte((byte)SerialNumberChunks.ProductCode);
			data.Write(new byte[] { 41, 65, 36, 150, 5, 175, 174, 137 }, 0, 8);

			data.WriteByte((byte)SerialNumberChunks.UserData);
			data.WriteByte(1);
			data.WriteByte((byte)productId);

			data.WriteByte((byte)SerialNumberChunks.MaxBuild);
			data.WriteByte((byte)maxBuildDt.Day);
			data.WriteByte((byte)maxBuildDt.Month);
			data.WriteByte((byte)maxBuildDt.Year);
			data.WriteByte((byte)(maxBuildDt.Year >> 8));

			SHA1 sha = new SHA1Managed();
			sha.Initialize();
			data.Position = 0;
			var hash = sha.ComputeHash(data);
			data.WriteByte((byte)SerialNumberChunks.End);
			data.WriteByte(hash[3]);
			data.WriteByte(hash[2]);
			data.WriteByte(hash[1]);
			data.WriteByte(hash[0]);

			const int minPadding = 8 + 3;
			const int maxPadding = minPadding + 16;
			const int maxBytes = 3072 / 8;
			if (data.Length + minPadding > maxBytes)
				throw new ApplicationException("Serial number too long");

			var rnd = new Random();
			var paddingBytes = rnd.Next(minPadding, maxPadding + 1);
			if (data.Length + paddingBytes > maxBytes)
				paddingBytes = maxBytes - (int)data.Length;

			var paddedData = new byte[maxBytes];
			var nonPaddedData = data.ToArray();
			Array.Copy(nonPaddedData, paddedData, paddingBytes);
			Array.Copy(nonPaddedData, 0, paddedData, paddingBytes, data.Length);
			paddedData[0] = 0;
			paddedData[1] = 2;
			paddedData[paddingBytes - 1] = 0;
			var i = 2;
			for (; i < paddingBytes - 1; i++) {
				byte b = 0;
				while (b == 0) {
					b = (byte)rnd.Next(256);
				}
				paddedData[i] = b;
			}
			i = nonPaddedData.Length + paddingBytes;
			while (i < maxBytes) {
				paddedData[i++] = (byte)rnd.Next(256);
			}

			var res = Convert.ToBase64String(Rsa.Encrypt(paddedData), Base64FormattingOptions.InsertLineBreaks);
			return res;
		}
	}
}
