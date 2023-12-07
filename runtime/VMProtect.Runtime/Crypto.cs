using System;
using System.Runtime.InteropServices;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public class CipherRC5
	{
		private const int R = 15; // number of rounds
		private const int B = 8; // number of bytes in key
		private const int C = 8 * B / BitRotate.W; // 16 - number u32s in key = ceil(8*B/W)
		private const int T = 2 * (R + 1); // 34 - size of table _s = 2*(R+1) u32s

		private readonly uint[] _s; // expanded key table

#if RUNTIME
		private const uint P = (uint)Faces.RC5_P;
		private const uint Q = (uint)Faces.RC5_Q;
#else
		private static readonly uint P = Core.Instance.Rand32();
		private static readonly uint Q = Core.Instance.Rand32();

		public static byte[] CreateRandomKey()
		{
			var ret = new byte[B];
			Core.Instance.RndGenerator.NextBytes(ret);
			return ret;
		}
#endif

		public CipherRC5(byte[] key)
		{
			uint i, j, k, u = BitRotate.W / 8, a, b;
			var l = new uint[C];
			_s = new uint[T];

			for (i = B - 1, l[C - 1] = 0; i != uint.MaxValue; i--)
				l[i / u] = (l[i / u] << 8) + key[i];

			for (_s[0] = P, i = 1; i < T; i++)
				_s[i] = _s[i - 1] + Q;
			for (a = b = i = j = k = 0; k < 3 * T; k++, i = (i + 1) % T, j = (j + 1) % C) // 3*T > 3*C
			{
				a = _s[i] = BitRotate.Left(_s[i] + a + b, 3);
				b = l[j] = BitRotate.Left(l[j] + a + b, (int)(a + b));
			}
		}

		[StructLayout(LayoutKind.Explicit)]
		private struct Dword
		{
			[FieldOffset(0)]
			public ulong dw;

			[FieldOffset(0)]
			public uint w0;

			[FieldOffset(4)]
			public uint w1;
		}

		private void Encrypt(ref Dword io)
		{
			uint i, a = io.w0 + _s[0], b = io.w1 + _s[1];
			for (i = 1; i <= R; i++)
			{
				a = BitRotate.Left(a ^ b, (int)b) + _s[2 * i];
				b = BitRotate.Left(b ^ a, (int)a) + _s[2 * i + 1];
			}
			io.w0 = a;
			io.w1 = b;
		}

		private void Decrypt(ref Dword io)
		{
			uint i, b = io.w1, a = io.w0;
			for (i = R; i > 0; i--)
			{
				b = BitRotate.Right(b - _s[2 * i + 1], (int)a) ^ a;
				a = BitRotate.Right(a - _s[2 * i], (int)b) ^ b;
			}
			io.w1 = b - _s[1];
			io.w0 = a - _s[0];
		}

		public byte[] Encrypt(byte[] ain)
		{
			var aout = new byte[ain.Length];
			Encrypt(ain, aout);
			return aout;
		}

		public byte[] Decrypt(byte[] ain)
		{
			var aout = new byte[ain.Length];
			Decrypt(ain, aout);
			return aout;
		}

		public void Encrypt(byte[] ain, byte[] aout)
		{
			var block = new Dword();
			for (var i = 0; i < ain.Length; i += 8)
			{
				block.dw = BitConverter.ToUInt64(ain, i);
				Encrypt(ref block);
				BitConverter.GetBytes(block.dw).CopyTo(aout, i);
			}
		}

		public void Decrypt(byte[] ain, byte[] aout)
		{
			var block = new Dword();
			for (var i = 0; i < ain.Length; i += 8)
			{
				block.dw = BitConverter.ToUInt64(ain, i);
				Decrypt(ref block);
				BitConverter.GetBytes(block.dw).CopyTo(aout, i);
			}
		}
	}

	public class CRC32
	{
		static uint[] _table;

		public CRC32()
		{
			if (_table == null)
			{
				_table = new uint[256];
				for (var i = 0; i < 256; i++)
				{
					var entry = (uint)i;
					for (var j = 0; j < 8; j++)
						if ((entry & 1) == 1)
							entry = (entry >> 1) ^ 0xedb88320u;
						else
							entry = entry >> 1;
					_table[i] = entry;
				}
			}
		}

		public uint Hash(IntPtr ptr, uint size)
		{
			uint crc = 0;
			for (var i = 0; i < size; i++)
				crc = _table[(Marshal.ReadByte(new IntPtr(ptr.ToInt64() + i)) ^ crc) & 0xff] ^ (crc >> 8);
			return ~crc;
		}
	}

	public class CRCValueCryptor
	{
		private uint _key;
		public CRCValueCryptor()
		{
			_key = (uint)Faces.CRC_INFO_SALT;
		}

		public uint Decrypt(uint value)
		{
			uint res = value ^ _key;
			_key = BitRotate.Left(_key, 7) ^ res;
			return res;
		}
	}

	public class BitRotate
	{
		public const int W = 32; // u32 size in bits
		public static uint Left(uint a, int offset)
		{
			var r1 = a << offset;
			var r2 = a >> (W - offset);
			return r1 | r2;
		}

		public static uint Right(uint a, int offset)
		{
			var r1 = a >> offset;
			var r2 = a << (W - offset);
			return r1 | r2;
		}

		public static uint Swap(uint value)
		{
			uint mask_xx_zz = (value & 0x00FF00FFU);
			uint mask_ww_yy = (value & 0xFF00FF00U);
			return ((mask_xx_zz >> 8) | (mask_xx_zz << 24)) +
				   ((mask_ww_yy << 8) | (mask_ww_yy >> 24));
		}
	}
}