// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Struct: BigInteger
**
** Purpose: Represents an arbitrary precision integer.
**
=============================================================================*/

using System;

using Contracts = System.Diagnostics.Debug;
using Contract = System.Diagnostics.Debug;
//using System.Globalization;
// ReSharper disable InconsistentNaming

// ReSharper disable once CheckNamespace
namespace Numerics
{
#if !SILVERLIGHT
	[Serializable]
#endif // !SILVERLIGHT
	internal struct BigInteger /*: IFormattable, IComparable, IComparable<BigInteger>, IEquatable<BigInteger>*/
	{
		// ---- SECTION:  members supporting exposed properties -------------*
		#region members supporting exposed properties
		private const uint kuMaskHighBit = unchecked((uint)int.MinValue);
		private const int kcbitUint = 32;
		//private const int kcbitUlong = 64;
		//private const int DecimalScaleFactorMask = 0x00FF0000;
		//private const int DecimalSignMask = unchecked((int)0x80000000);

		// For values int.MinValue < n <= int.MaxValue, the value is stored in sign
		// and _bits is null. For all other values, sign is +1 or -1 and the bits are in _bits
		private readonly int _sign;
		private readonly uint[] _bits;

		// We have to make a choice of how to represent int.MinValue. This is the one
		// value that fits in an int, but whose negation does not fit in an int.
		// We choose to use a large representation, so we're symmetric with respect to negation.
		private static readonly BigInteger s_bnMinInt = new BigInteger(-1, new[] { kuMaskHighBit });
		private static readonly BigInteger s_bnOneInt = new BigInteger(1);
		private static readonly BigInteger s_bnZeroInt = new BigInteger(0);
		private static readonly BigInteger s_bnMinusOneInt = new BigInteger(-1);

#if CONTRACTS_FULL
		[ContractInvariantMethod]
		private void ObjectInvariant()
		{
			Contract.Invariant((_bits == null) ? _sign > Int32.MinValue :
				((_sign == 1 || _sign == -1) && Length(_bits) > 0));
			Contract.Invariant(_bits == null || Length(_bits) > 1 || _bits[0] >= kuMaskHighBit
				, "One element array stores integers whose absolute value is between 0x80000000 and 0xFFFFFFFF");
		}
#endif

		[System.Diagnostics.Conditional("DEBUG")]
		private void AssertValid()
		{
			if (_bits != null)
			{
				Contracts.Assert(_sign == 1 || _sign == -1 /*, "_sign must be +1 or -1 when _bits is non-null"*/);
				Contracts.Assert(Length(_bits) > 0 /*, "_bits must contain at least 1 element or be null"*/);
				if (Length(_bits) == 1)
					Contracts.Assert(_bits[0] >= kuMaskHighBit /*, "Wasted space _bits[0] could have been packed into _sign"*/);
			}
			else
				Contracts.Assert(_sign > int.MinValue /*, "Int32.MinValue should not be stored in the _sign field"*/);
		}
		#endregion members supporting exposed properties

		// ---- SECTION: internal properties --------------*
		#region internal properties

		/*internal static BigInteger Zero
		{
			get { return s_bnZeroInt; }
		}*/

		private static BigInteger One
		{
			get { return s_bnOneInt; }
		}

/*
		internal static BigInteger MinusOne
		{
			get { return s_bnMinusOneInt; }
		}

		internal bool IsPowerOfTwo
		{
			get
			{
				AssertValid();

				if (_bits == null)
					return (_sign & (_sign - 1)) == 0 && _sign != 0;

				if (_sign != 1)
					return false;
				int iu = Length(_bits) - 1;
				if ((_bits[iu] & (_bits[iu] - 1)) != 0)
					return false;
				while (--iu >= 0)
				{
					if (_bits[iu] != 0)
						return false;
				}
				return true;
			}
		}
*/

/*
		internal bool IsZero { get { AssertValid(); return _sign == 0; } }
*/

/*
		internal bool IsOne { get { AssertValid(); return _sign == 1 && _bits == null; } }
*/

		private bool IsEven { get { AssertValid(); return _bits == null ? (_sign & 1) == 0 : (_bits[0] & 1) == 0; } }

		private int Sign
		{
			get { AssertValid(); return (_sign >> (kcbitUint - 1)) - (-_sign >> (kcbitUint - 1)); }
		}

		#endregion internal properties


		// ---- SECTION: internal instance methods --------------*
		#region internal instance methods

		public override bool Equals(object obj)
		{
			AssertValid();

			if (!(obj is BigInteger))
				return false;
			return Equals((BigInteger)obj);
		}

		public override int GetHashCode()
		{
			AssertValid();

			// ReSharper disable NonReadonlyMemberInGetHashCode
			if (_bits == null)
				return _sign;
			var hash = _sign;
			for (var iv = Length(_bits); --iv >= 0; )
				hash = NumericsHelpers.CombineHash(hash, (int)_bits[iv]);
			return hash;
		}

		/*public bool Equals(Int64 other)
		{
			AssertValid();

			if (_bits == null)
				return _sign == other;

			int cu;
			if ((_sign ^ other) < 0 || (cu = Length(_bits)) > 2)
				return false;

			var uu = other < 0 ? (ulong)-other : (ulong)other;
			if (cu == 1)
				return _bits[0] == uu;

			return NumericsHelpers.MakeUlong(_bits[1], _bits[0]) == uu;
		}

		public bool Equals(UInt64 other)
		{
			AssertValid();

			if (_sign < 0)
				return false;
			if (_bits == null)
				return (ulong)_sign == other;

			var cu = Length(_bits);
			if (cu > 2)
				return false;
			if (cu == 1)
				return _bits[0] == other;
			return NumericsHelpers.MakeUlong(_bits[1], _bits[0]) == other;
		}*/

		/*public bool Equals(BigInteger other)
		{
			AssertValid();
			other.AssertValid();

			if (_sign != other._sign)
				return false;
			if (_bits == other._bits)
				// _sign == other._sign && _bits == null && other._bits == null
				return true;

			if (_bits == null || other._bits == null)
				return false;
			var cu = Length(_bits);
			if (cu != Length(other._bits))
				return false;
			var cuDiff = GetDiffLength(_bits, other._bits, cu);
			return cuDiff == 0;
		}*/

		/*internal int CompareTo(Int64 other)
		{
			AssertValid();

			if (_bits == null)
				return ((long)_sign).CompareTo(other);
			int cu;
			if ((_sign ^ other) < 0 || (cu = Length(_bits)) > 2)
				return _sign;
			var uu = other < 0 ? (ulong)-other : (ulong)other;
			var uuTmp = cu == 2 ? NumericsHelpers.MakeUlong(_bits[1], _bits[0]) : _bits[0];
			return _sign * uuTmp.CompareTo(uu);
		}

		internal int CompareTo(UInt64 other)
		{
			AssertValid();

			if (_sign < 0)
				return -1;
			if (_bits == null)
				return ((ulong)_sign).CompareTo(other);
			var cu = Length(_bits);
			if (cu > 2)
				return +1;
			var uuTmp = cu == 2 ? NumericsHelpers.MakeUlong(_bits[1], _bits[0]) : _bits[0];
			return uuTmp.CompareTo(other);
		}*/

		private int CompareTo(BigInteger other)
		{
			AssertValid();
			other.AssertValid();

			if ((_sign ^ other._sign) < 0)
			{
				// Different signs, so the comparison is easy.
				return _sign < 0 ? -1 : +1;
			}

			// Same signs
			if (_bits == null)
			{
				if (other._bits == null)
					return _sign < other._sign ? -1 : _sign > other._sign ? +1 : 0;
				return -other._sign;
			}
			int cuThis, cuOther;
			if (other._bits == null || (cuThis = Length(_bits)) > (cuOther = Length(other._bits)))
				return _sign;
			if (cuThis < cuOther)
				return -_sign;

			var cuDiff = GetDiffLength(_bits, other._bits, cuThis);
			if (cuDiff == 0)
				return 0;
			return _bits[cuDiff - 1] < other._bits[cuDiff - 1] ? -_sign : _sign;
		}

/*
		public int CompareTo(Object obj)
		{
			if (obj == null)
				return 1;
			if (!(obj is BigInteger))
				throw new ArgumentException("Argument must be BigInteger", "obj");
			return CompareTo((BigInteger)obj);
		}
*/


		// Return the value of this BigInteger as a little-endian twos-complement
		// byte array, using the fewest number of bytes possible. If the value is zero,
		// return an array of one byte whose element is 0x00.
		internal byte[] ToByteArray()
		{
			if (_bits == null && _sign == 0)
				return new byte[] { 0 };

			// We could probably make this more efficient by eliminating one of the passes.
			// The current code does one pass for uint array -> byte array conversion,
			// and then another pass to remove unneeded bytes at the top of the array.
			uint[] dwords;
			byte highByte;

			if (_bits == null)
			{
				dwords = new[] { (uint)_sign };
				highByte = (byte)(_sign < 0 ? 0xff : 0x00);
			}
			else if (_sign == -1)
			{
				dwords = (uint[])_bits.Clone();
				NumericsHelpers.DangerousMakeTwosComplement(dwords);  // mutates dwords
				highByte = 0xff;
			}
			else
			{
				dwords = _bits;
				highByte = 0x00;
			}

			var bytes = new byte[checked(4 * dwords.Length)];
			var curByte = 0;
			foreach (var t in dwords)
			{
				var dword = t;
				for (var j = 0; j < 4; j++)
				{
					bytes[curByte++] = (byte)(dword & 0xff);
					dword >>= 8;
				}
			}

			// ensure high bit is 0 if positive, 1 if negative
			if ((bytes[bytes.Length - 1] & 0x80) == (highByte & 0x80))
				return bytes;

			var trimmedBytes = new byte[bytes.Length + 1];
			Array.Copy(bytes, trimmedBytes, bytes.Length);
			trimmedBytes[trimmedBytes.Length - 1] = highByte;
			return trimmedBytes;
		}

		// Return the value of this BigInteger as a little-endian twos-complement
		// uint array, using the fewest number of uints possible. If the value is zero,
		// return an array of one uint whose element is 0.
/*
		private UInt32[] ToUInt32Array()
		{
			if (_bits == null && _sign == 0)
				return new uint[] { 0 };

			uint[] dwords;
			uint highDWord;

			if (_bits == null)
			{
				dwords = new[] { (uint)_sign };
				highDWord = _sign < 0 ? UInt32.MaxValue : 0;
			}
			else if (_sign == -1)
			{
				dwords = (uint[])_bits.Clone();
				NumericsHelpers.DangerousMakeTwosComplement(dwords);  // mutates dwords
				highDWord = UInt32.MaxValue;
			}
			else
			{
				dwords = _bits;
				highDWord = 0;
			}

			// find highest significant byte
			int msb;
			for (msb = dwords.Length - 1; msb > 0; msb--)
			{
				if (dwords[msb] != highDWord) break;
			}
			// ensure high bit is 0 if positive, 1 if negative
			var needExtraByte = (dwords[msb] & 0x80000000) != (highDWord & 0x80000000);

			var trimmed = new uint[msb + 1 + (needExtraByte ? 1 : 0)];
			Array.Copy(dwords, trimmed, msb + 1);

			if (needExtraByte) trimmed[trimmed.Length - 1] = highDWord;
			return trimmed;
		}
*/

		/*public override String ToString()
		{
			return BigNumber.FormatBigInteger(this, null, NumberFormatInfo.CurrentInfo);
		}

		internal String ToString(IFormatProvider provider)
		{
			return BigNumber.FormatBigInteger(this, null, NumberFormatInfo.GetInstance(provider));
		}

		internal String ToString(String format)
		{
			return BigNumber.FormatBigInteger(this, format, NumberFormatInfo.CurrentInfo);
		}

		public String ToString(String format, IFormatProvider provider)
		{
			return BigNumber.FormatBigInteger(this, format, NumberFormatInfo.GetInstance(provider));
		}*/
		#endregion internal instance methods

		// -------- SECTION: constructors -----------------*
		#region constructors

		private BigInteger(int value)
		{
			if (value == Int32.MinValue)
				this = s_bnMinInt;
			else
			{
				_sign = value;
				_bits = null;
			}
			AssertValid();
		}

/*
		internal BigInteger(uint value)
		{
			if (value <= Int32.MaxValue)
			{
				_sign = (int)value;
				_bits = null;
			}
			else
			{
				_sign = +1;
				_bits = new uint[1];
				_bits[0] = value;
			}
			AssertValid();
		}
*/

/*
		internal BigInteger(Int64 value)
		{
			if (Int32.MinValue <= value && value <= Int32.MaxValue)
			{
				if (value == Int32.MinValue)
					this = s_bnMinInt;
				else
				{
					_sign = (int)value;
					_bits = null;
				}
				AssertValid();
				return;
			}

			ulong x;
			if (value < 0)
			{
				x = (ulong)-value;
				_sign = -1;
			}
			else
			{
				Contract.Assert(value != 0);
				x = (ulong)value;
				_sign = +1;
			}

			_bits = new uint[2];
			_bits[0] = (uint)x;
			_bits[1] = (uint)(x >> kcbitUint);
			AssertValid();
		}
*/

/*
		internal BigInteger(UInt64 value)
		{
			if (value <= Int32.MaxValue)
			{
				_sign = (int)value;
				_bits = null;
			}
			else
			{
				_sign = +1;
				_bits = new uint[2];
				_bits[0] = (uint)value;
				_bits[1] = (uint)(value >> kcbitUint);
			}
			AssertValid();
		}
*/

		/*internal BigInteger(Single value)
		{
			if (Single.IsInfinity(value))
				throw new OverflowException("Overflow BigIntInfinity");
			if (Single.IsNaN(value))
				throw new OverflowException("Overflow not a number");
			////Contract.EndContractBlock();

			_sign = 0;
			_bits = null;
			// ReSharper disable once ExpressionIsAlwaysNull
			SetBitsFromDouble(value);
			AssertValid();
		}

		internal BigInteger(Double value)
		{
			if (Double.IsInfinity(value))
				throw new OverflowException("Overflow BigIntInfinity");
			if (Double.IsNaN(value))
				throw new OverflowException("Overflow not a number");
			////Contract.EndContractBlock();

			_sign = 0;
			_bits = null;
			// ReSharper disable once ExpressionIsAlwaysNull
			SetBitsFromDouble(value);
			AssertValid();
		}*/

		/*internal BigInteger(Decimal value)
		{
			// First truncate to get scale to 0 and extract bits
			var bits = Decimal.GetBits(Decimal.Truncate(value));

			Contract.Assert(bits.Length == 4 && (bits[3] & DecimalScaleFactorMask) == 0);

			var size = 3;
			while (size > 0 && bits[size - 1] == 0)
				size--;
			if (size == 0)
			{
				this = s_bnZeroInt;
			}
			else if (size == 1 && bits[0] > 0)
			{
				// bits[0] is the absolute value of this decimal
				// if bits[0] < 0 then it is too large to be packed into _sign
				_sign = bits[0];
				_sign *= (bits[3] & DecimalSignMask) != 0 ? -1 : +1;
				_bits = null;
			}
			else
			{
				_bits = new UInt32[size];
				_bits[0] = (UInt32)bits[0];
				if (size > 1)
					_bits[1] = (UInt32)bits[1];
				if (size > 2)
					_bits[2] = (UInt32)bits[2];
				_sign = (bits[3] & DecimalSignMask) != 0 ? -1 : +1;
			}
			AssertValid();
		}*/

		//
		// Create a BigInteger from a little-endian twos-complement byte array
		//
		internal BigInteger(Byte[] value)
		{
			if (value == null)
				throw new ArgumentNullException("value");
			//Contract.EndContractBlock();

			var byteCount = value.Length;
			var isNegative = byteCount > 0 && ((value[byteCount - 1] & 0x80) == 0x80);

			// Try to conserve space as much as possible by checking for wasted leading byte[] entries 
			while (byteCount > 0 && value[byteCount - 1] == 0) byteCount--;

			if (byteCount == 0)
			{
				// BigInteger.Zero
				_sign = 0;
				_bits = null;
				// ReSharper disable once ExpressionIsAlwaysNull
				AssertValid();
				return;
			}


			if (byteCount <= 4)
			{
				if (isNegative)
					_sign = unchecked((int)0xffffffff);
				else
					_sign = 0;
				for (var i = byteCount - 1; i >= 0; i--)
				{
					_sign <<= 8;
					_sign |= value[i];
				}
				_bits = null;

				if (_sign < 0 && !isNegative)
				{
					// int32 overflow
					// example: Int64 value 2362232011 (0xCB, 0xCC, 0xCC, 0x8C, 0x0)
					// can be naively packed into 4 bytes (due to the leading 0x0)
					// it overflows into the int32 sign bit
					_bits = new uint[1];
					_bits[0] = (uint)_sign;
					_sign = +1;
				}
				if (_sign == Int32.MinValue)
					this = s_bnMinInt;
			}
			else
			{
				var unalignedBytes = byteCount % 4;
				var dwordCount = byteCount / 4 + (unalignedBytes == 0 ? 0 : 1);
				var isZero = true;
				var val = new uint[dwordCount];

				// Copy all dwords, except but don't do the last one if it's not a full four bytes
				int curDword;
				var curByte = 3;
				for (curDword = 0; curDword < dwordCount - (unalignedBytes == 0 ? 0 : 1); curDword++)
				{
					var byteInDword = 0;
					while (byteInDword < 4)
					{
						if (value[curByte] != 0x00) isZero = false;
						val[curDword] <<= 8;
						val[curDword] |= value[curByte];
						curByte--;
						byteInDword++;
					}
					curByte += 8;
				}

				// Copy the last dword specially if it's not aligned
				if (unalignedBytes != 0)
				{
					if (isNegative) val[dwordCount - 1] = 0xffffffff;
					for (curByte = byteCount - 1; curByte >= byteCount - unalignedBytes; curByte--)
					{
						if (value[curByte] != 0x00) isZero = false;
						val[curDword] <<= 8;
						val[curDword] |= value[curByte];
					}
				}

				if (isZero)
				{
					this = s_bnZeroInt;
				}
				else if (isNegative)
				{
					NumericsHelpers.DangerousMakeTwosComplement(val); // mutates val

					// pack _bits to remove any wasted space after the twos complement
					var len = val.Length;
					while (len > 0 && val[len - 1] == 0)
						len--;
					if (len == 1 && (int)val[0] > 0)
					{
						if (val[0] == 1 /* abs(-1) */)
						{
							this = s_bnMinusOneInt;
						}
						else if (val[0] == kuMaskHighBit /* abs(Int32.MinValue) */) //TODO: V3022 https://www.viva64.com/en/w/V3022 Expression 'val[0] == kuMaskHighBit' is always false.
						{
							this = s_bnMinInt;
						}
						else
						{
							_sign = -1 * (int)val[0];
							_bits = null;
						}
					}
					else if (len != val.Length)
					{
						_sign = -1;
						_bits = new uint[len];
						Array.Copy(val, _bits, len);
					}
					else
					{
						_sign = -1;
						_bits = val;
					}
				}
				else
				{
					_sign = +1;
					_bits = val;
				}
			}
			AssertValid();
		}

		internal BigInteger(int n, uint[] rgu)
		{
			_sign = n;
			_bits = rgu;
			AssertValid();
		}

		//
		// BigInteger(uint[] value, bool negative) 
		//
		// Constructor used during bit manipulation and arithmetic
		//
		// The uint[] value is expected to be the absolute value of the number
		// with the bool negative indicating the Sign of the value.
		//
		// When possible the uint[] will be packed into  _sign to conserve space
		//
/*
		internal BigInteger(uint[] value, bool negative)
		{
			if (value == null)
				throw new ArgumentNullException("value");
			//Contract.EndContractBlock();

			int len;

			// Try to conserve space as much as possible by checking for wasted leading uint[] entries 
			// sometimes the uint[] has leading zeros from bit manipulation operations & and ^
			for (len = value.Length; len > 0 && value[len - 1] == 0; len--)
			{
			}

			if (len == 0)
				this = s_bnZeroInt;
			// values like (Int32.MaxValue+1) are stored as "0x80000000" and as such cannot be packed into _sign
			else if (len == 1 && value[0] < kuMaskHighBit)
			{
				_sign = negative ? -(int)value[0] : (int)value[0];
				_bits = null;
				// Although Int32.MinValue fits in _sign, we represent this case differently for negate
				if (_sign == Int32.MinValue)
					this = s_bnMinInt;
			}
			else
			{
				_sign = negative ? -1 : +1;
				_bits = new uint[len];
				Array.Copy(value, _bits, len);
			}
			AssertValid();
		}
*/


		//
		// Create a BigInteger from a little-endian twos-complement UInt32 array
		// When possible, value is assigned directly to this._bits without an array copy
		// so use this ctor with care
		//
//		private BigInteger(uint[] value)
//		{
//			if (value == null)
//				throw new ArgumentNullException("value");
//
//			var dwordCount = value.Length;
//			var isNegative = dwordCount > 0 && ((value[dwordCount - 1] & 0x80000000) == 0x80000000);
//
//			// Try to conserve space as much as possible by checking for wasted leading uint[] entries 
//			while (dwordCount > 0 && value[dwordCount - 1] == 0) dwordCount--;
//
//			if (dwordCount == 0)
//			{
//				// BigInteger.Zero
//				this = s_bnZeroInt;
//				AssertValid();
//				return;
//			}
//			if (dwordCount == 1)
//			{
//				if ((int)value[0] < 0 && !isNegative)
//				{
//					_bits = new uint[1];
//					_bits[0] = value[0];
//					_sign = +1;
//				}
//				// handle the special cases where the BigInteger likely fits into _sign
//				else if (Int32.MinValue == (int)value[0])
//				{
//					this = s_bnMinInt;
//				}
//				else
//				{
//					_sign = (int)value[0];
//					_bits = null;
//				}
//				AssertValid();
//				return;
//			}
//
//			if (!isNegative)
//			{
//				// handle the simple postive value cases where the input is already in sign magnitude
//				if (dwordCount != value.Length)
//				{
//					_sign = +1;
//					_bits = new uint[dwordCount];
//					Array.Copy(value, _bits, dwordCount);
//				}
//				// no trimming is possible.  Assign value directly to _bits.  
//				else
//				{
//					_sign = +1;
//					_bits = value;
//				}
//				AssertValid();
//				return;
//			}
//
//
//			// finally handle the more complex cases where we must transform the input into sign magnitude
//			NumericsHelpers.DangerousMakeTwosComplement(value); // mutates val
//
//			// pack _bits to remove any wasted space after the twos complement
//			var len = value.Length;
//			while (len > 0 && value[len - 1] == 0) len--;
//
//			// the number is represented by a single dword
//			if (len == 1 && (int)value[0] > 0)
//			{
//				if (value[0] == 1 /* abs(-1) */)
//				{
//					this = s_bnMinusOneInt;
//				}
//				else if (value[0] == kuMaskHighBit /* abs(Int32.MinValue) */)
//				{
//					this = s_bnMinInt;
//				}
//				else
//				{
//					_sign = -1 * (int)value[0];
//					_bits = null;
//				}
//			}
//			// the number is represented by multiple dwords
//			// trim off any wasted uint values when possible
//			else if (len != value.Length)
//			{
//				_sign = -1;
//				_bits = new uint[len];
//				Array.Copy(value, _bits, len);
//			}
//			// no trimming is possible.  Assign value directly to _bits.  
//			else
//			{
//				_sign = -1;
//				_bits = value;
//			}
//			AssertValid();
//		}


		#endregion constructors


		// -------- SECTION: internal static methods -----------------*
		#region internal static methods
#if !SILVERLIGHT || FEATURE_NETCORE
/*
		internal static BigInteger Parse(String value)
		{
			return BigNumber.ParseBigInteger(value, NumberStyles.Integer, NumberFormatInfo.CurrentInfo);
		}
*/

/*
		internal static BigInteger Parse(String value, NumberStyles style)
		{
			return BigNumber.ParseBigInteger(value, style, NumberFormatInfo.CurrentInfo);
		}
*/

/*
		internal static BigInteger Parse(String value, IFormatProvider provider)
		{
			return BigNumber.ParseBigInteger(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
		}
*/

/*
		internal static BigInteger Parse(String value, NumberStyles style, IFormatProvider provider)
		{
			return BigNumber.ParseBigInteger(value, style, NumberFormatInfo.GetInstance(provider));
		}
*/

/*
		internal static Boolean TryParse(String value, out BigInteger result)
		{
			return BigNumber.TryParseBigInteger(value, NumberStyles.Integer, NumberFormatInfo.CurrentInfo, out result);
		}
*/

/*
		internal static Boolean TryParse(String value, NumberStyles style, IFormatProvider provider, out BigInteger result)
		{
			return BigNumber.TryParseBigInteger(value, style, NumberFormatInfo.GetInstance(provider), out result);
		}
*/
#endif //!SILVERLIGHT || FEATURE_NETCORE

/*
		internal static Int32 Compare(BigInteger left, BigInteger right)
		{
			return left.CompareTo(right);
		}
*/

/*
		internal static BigInteger Abs(BigInteger value)
		{
			return value >= Zero ? value : -value;
		}
*/

/*
		internal static BigInteger Add(BigInteger left, BigInteger right)
		{
			return left + right;
		}
*/

/*
		internal static BigInteger Subtract(BigInteger left, BigInteger right)
		{
			return left - right;
		}
*/

/*
		internal static BigInteger Multiply(BigInteger left, BigInteger right)
		{
			return left * right;
		}
*/

/*
		internal static BigInteger Divide(BigInteger dividend, BigInteger divisor)
		{
			return dividend / divisor;
		}
*/

/*
		internal static BigInteger Remainder(BigInteger dividend, BigInteger divisor)
		{
			return dividend % divisor;
		}
*/

/*
		internal static BigInteger DivRem(BigInteger dividend, BigInteger divisor, out BigInteger remainder)
		{
			dividend.AssertValid();
			divisor.AssertValid();

			var signNum = +1;
			var signDen = +1;
			var regNum = new BigIntegerBuilder(dividend, ref signNum);
			var regDen = new BigIntegerBuilder(divisor, ref signDen);
			var regQuo = new BigIntegerBuilder();

			// regNum and regQuo are overwritten with the remainder and quotient, respectively
			regNum.ModDiv(ref regDen, ref regQuo);
			remainder = regNum.GetInteger(signNum);
			return regQuo.GetInteger(signNum * signDen);
		}
*/


/*
		internal static BigInteger Negate(BigInteger value)
		{
			return -value;
		}
*/

		// Returns the natural (base e) logarithm of a specified number.
/*
		internal static Double Log(BigInteger value, Double baseValue = Math.E)
		{
			// ReSharper disable CompareOfFloatsByEqualityOperator
			if (value._sign < 0 || baseValue == 1.0D)
				return Double.NaN;
			if (baseValue == Double.PositiveInfinity)
				return value.IsOne ? 0.0D : Double.NaN;
			if (baseValue == 0.0D && !value.IsOne)
				return Double.NaN;
			if (value._bits == null)
				return Math.Log(value._sign, baseValue);
			// ReSharper restore CompareOfFloatsByEqualityOperator

			Double c = 0, d = 0.5D;
			const Double log2 = 0.69314718055994529D;

			var uintLength = Length(value._bits);
			var topbits = BitLengthOfUInt(value._bits[uintLength - 1]);
			var bitlen = (uintLength - 1) * kcbitUint + topbits;
			var indbit = (uint)(1 << (topbits - 1));

			for (var index = uintLength - 1; index >= 0; --index)
			{
				while (indbit != 0)
				{
					if ((value._bits[index] & indbit) != 0)
						c += d;
					d *= 0.5;
					indbit >>= 1;
				}
				indbit = 0x80000000;
			}
			return (Math.Log(c) + log2 * bitlen) / Math.Log(baseValue);
		}
*/


/*
		internal static Double Log10(BigInteger value)
		{
			return Log(value, 10);
		}
*/

/*
		internal static BigInteger GreatestCommonDivisor(BigInteger left, BigInteger right)
		{
			left.AssertValid();
			right.AssertValid();

			// gcd(0, 0) =  0 
			// gcd(a, 0) = |a|, for a != 0, since any number is a divisor of 0, and the greatest divisor of a is |a|
			if (left._sign == 0) return Abs(right);
			if (right._sign == 0) return Abs(left);

			var reg1 = new BigIntegerBuilder(left);
			var reg2 = new BigIntegerBuilder(right);
			BigIntegerBuilder.GCD(ref reg1, ref reg2);

			return reg1.GetInteger(+1);
		}
*/

/*
		internal static BigInteger Max(BigInteger left, BigInteger right)
		{
			if (left.CompareTo(right) < 0)
				return right;
			return left;
		}
*/

/*
		internal static BigInteger Min(BigInteger left, BigInteger right)
		{
			if (left.CompareTo(right) <= 0)
				return left;
			return right;
		}
*/


		private static void ModPowUpdateResult(ref BigIntegerBuilder regRes, ref BigIntegerBuilder regVal, ref BigIntegerBuilder regMod, ref BigIntegerBuilder regTmp)
		{
			NumericsHelpers.Swap(ref regRes, ref regTmp);
			regRes.Mul(ref regTmp, ref regVal);   // result = result * value;
			regRes.Mod(ref regMod);               // result = result % modulus;                       
		}

		private static void ModPowSquareModValue(ref BigIntegerBuilder regVal, ref BigIntegerBuilder regMod, ref BigIntegerBuilder regTmp)
		{
			NumericsHelpers.Swap(ref regVal, ref regTmp);
			regVal.Mul(ref regTmp, ref regTmp);   // value = value * value;
			regVal.Mod(ref regMod);               // value = value % modulus;
		}

		private static void ModPowInner(uint exp, ref BigIntegerBuilder regRes, ref BigIntegerBuilder regVal, ref BigIntegerBuilder regMod, ref BigIntegerBuilder regTmp)
		{
			while (exp != 0)          // !(Exponent.IsZero)
			{
				if ((exp & 1) == 1)   // !(Exponent.IsEven)
					ModPowUpdateResult(ref regRes, ref regVal, ref regMod, ref regTmp);
				if (exp == 1)         // Exponent.IsOne - we can exit early
					break;
				ModPowSquareModValue(ref regVal, ref regMod, ref regTmp);
				exp >>= 1;
			}
		}

		private static void ModPowInner32(uint exp, ref BigIntegerBuilder regRes, ref BigIntegerBuilder regVal, ref BigIntegerBuilder regMod, ref BigIntegerBuilder regTmp)
		{
			for (var i = 0; i < 32; i++)
			{
				if ((exp & 1) == 1)   // !(Exponent.IsEven)
					ModPowUpdateResult(ref regRes, ref regVal, ref regMod, ref regTmp);
				ModPowSquareModValue(ref regVal, ref regMod, ref regTmp);
				exp >>= 1;
			}
		}

		internal static BigInteger ModPow(BigInteger value, BigInteger exponent, BigInteger modulus)
		{
			if (exponent.Sign < 0)
				throw new ArgumentOutOfRangeException("exponent", "ArgumentOutOfRange must be non negative");
			//Contract.EndContractBlock();

			value.AssertValid();
			exponent.AssertValid();
			modulus.AssertValid();

			var signRes = +1;
			var signVal = +1;
			var signMod = +1;
			var expIsEven = exponent.IsEven;
			var regRes = new BigIntegerBuilder(One, ref signRes);
			var regVal = new BigIntegerBuilder(value, ref signVal);
			var regMod = new BigIntegerBuilder(modulus, ref signMod);
			var regTmp = new BigIntegerBuilder(regVal.Size);

			regRes.Mod(ref regMod);   // Handle special case of exponent=0, modulus=1

			if (exponent._bits == null)
			{   // exponent fits into an Int32
				ModPowInner((uint)exponent._sign, ref regRes, ref regVal, ref regMod, ref regTmp);
			}
			else
			{   // very large exponent
				var len = Length(exponent._bits);
				for (var i = 0; i < len - 1; i++)
				{
					var exp = exponent._bits[i];
					ModPowInner32(exp, ref regRes, ref regVal, ref regMod, ref regTmp);
				}
				ModPowInner(exponent._bits[len - 1], ref regRes, ref regVal, ref regMod, ref regTmp);
			}

			return regRes.GetInteger(value._sign > 0 ? +1 : expIsEven ? +1 : -1);
		}

/*
		internal static BigInteger Pow(BigInteger value, Int32 exponent)
		{
			if (exponent < 0)
				throw new ArgumentOutOfRangeException("exponent", "ArgumentOutOfRange must be non negative");
			//Contract.EndContractBlock();

			value.AssertValid();

			if (exponent == 0)
				return One;
			if (exponent == 1)
				return value;
			if (value._bits == null)
			{
				if (value._sign == 1)
					return value;
				if (value._sign == -1)
					return (exponent & 1) != 0 ? value : 1;
				if (value._sign == 0)
					return value;
			}

			var sign = +1;
			var regSquare = new BigIntegerBuilder(value, ref sign);

			// Get an estimate of the size needed for regSquare and regRes, so we can minimize allocations.
			var cuSquareMin = regSquare.Size;
			var cuSquareMax = cuSquareMin;
			var uSquareMin = regSquare.High;
			var uSquareMax = uSquareMin + 1;
			if (uSquareMax == 0)
			{
				cuSquareMax++;
				uSquareMax = 1;
			}
			var cuResMin = 1;
			var cuResMax = 1;
			uint uResMin = 1;
			uint uResMax = 1;

			for (var expTmp = exponent; ; )
			{
				if ((expTmp & 1) != 0)
				{
					MulUpper(ref uResMax, ref cuResMax, uSquareMax, cuSquareMax);
					MulLower(ref uResMin, ref cuResMin, uSquareMin, cuSquareMin);
				}

				if ((expTmp >>= 1) == 0)
					break;

				MulUpper(ref uSquareMax, ref cuSquareMax, uSquareMax, cuSquareMax);
				MulLower(ref uSquareMin, ref cuSquareMin, uSquareMin, cuSquareMin);
			}

			if (cuResMax > 1)
				regSquare.EnsureWritable(cuResMax, 0);
			var regTmp = new BigIntegerBuilder(cuResMax);
			var regRes = new BigIntegerBuilder(cuResMax);
			regRes.Set(1);

			if ((exponent & 1) == 0)
				sign = +1;

			for (var expTmp = exponent; ; )
			{
				if ((expTmp & 1) != 0)
				{
					NumericsHelpers.Swap(ref regRes, ref regTmp);
					regRes.Mul(ref regSquare, ref regTmp);
				}
				if ((expTmp >>= 1) == 0)
					break;
				NumericsHelpers.Swap(ref regSquare, ref regTmp);
				regSquare.Mul(ref regTmp, ref regTmp);
			}

			return regRes.GetInteger(sign);
		}
*/

		#endregion internal static methods

		// -------- SECTION: internal static operators -----------------*
		#region internal static operators
		/*public static implicit operator BigInteger(Byte value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(SByte value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(Int16 value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(UInt16 value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(int value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(uint value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(long value)
		{
			return new BigInteger(value);
		}

		public static implicit operator BigInteger(ulong value)
		{
			return new BigInteger(value);
		}*/

		/*public static explicit operator BigInteger(Single value)
		{
			return new BigInteger(value);
		}

		public static explicit operator BigInteger(Double value)
		{
			return new BigInteger(value);
		}

		public static explicit operator BigInteger(Decimal value)
		{
			return new BigInteger(value);
		}

		public static explicit operator Byte(BigInteger value)
		{
			return checked((byte)(int)value);
		}

		public static explicit operator SByte(BigInteger value)
		{
			return checked((sbyte)(int)value);
		}

		public static explicit operator Int16(BigInteger value)
		{
			return checked((short)(int)value);
		}

		public static explicit operator UInt16(BigInteger value)
		{
			return checked((ushort)(int)value);
		}

		public static explicit operator Int32(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
			{
				return value._sign;  // value packed into int32 sign
			}
			else if (Length(value._bits) > 1)
			{ // more than 32 bits
				throw new OverflowException("Overflow Int32");
			}
			else if (value._sign > 0)
			{
				return checked((int)value._bits[0]);
			}
			else
			{
				if (value._bits[0] > kuMaskHighBit)
				{  // value > Int32.MinValue
					throw new OverflowException("Overflow Int32");
				}
				return unchecked(-(int)value._bits[0]);
			}
		}

		public static explicit operator UInt32(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
			{
				return checked((uint)value._sign);
			}
			else if (Length(value._bits) > 1 || value._sign < 0)
			{
				throw new OverflowException("Overflow UInt32");
			}
			else
			{
				return value._bits[0];
			}
		}

		public static explicit operator Int64(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
			{
				return value._sign;
			}

			var len = Length(value._bits);
			if (len > 2)
			{
				throw new OverflowException("Overflow Int64");
			}

			var uu = len > 1 ? NumericsHelpers.MakeUlong(value._bits[1], value._bits[0]) : value._bits[0];

			var ll = value._sign > 0 ? (long)uu : -(long)uu;
			if ((ll > 0 && value._sign > 0) || (ll < 0 && value._sign < 0))
			{
				// signs match, no overflow
				return ll;
			}
			throw new OverflowException("Overflow Int64");
		}

		public static explicit operator UInt64(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
			{
				return checked((ulong)value._sign);
			}

			var len = Length(value._bits);
			if (len > 2 || value._sign < 0)
			{
				throw new OverflowException("Overflow UInt64");
			}

			if (len > 1)
			{
				return NumericsHelpers.MakeUlong(value._bits[1], value._bits[0]);
			}
			else
			{
				return value._bits[0];
			}
		}*/

		/*public static explicit operator Single(BigInteger value)
		{
			return (Single)(Double)value;
		}*/

		/*public static explicit operator Double(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
				return value._sign;

			ulong man;
			int exp;
			var sign = +1;
			var reg = new BigIntegerBuilder(value, ref sign);
			reg.GetApproxParts(out exp, out man);
			return NumericsHelpers.GetDoubleFromParts(sign, exp, man);
		}

		public static explicit operator Decimal(BigInteger value)
		{
			value.AssertValid();
			if (value._bits == null)
				return value._sign;

			var length = Length(value._bits);
			if (length > 3) throw new OverflowException("Overflow Decimal");

			int lo = 0, mi = 0, hi = 0;
			if (length > 2) hi = (Int32)value._bits[2];
			if (length > 1) mi = (Int32)value._bits[1];
			if (length > 0) lo = (Int32)value._bits[0];

			return new Decimal(lo, mi, hi, value._sign < 0, 0);
		}

		public static BigInteger operator &(BigInteger left, BigInteger right)
		{
			if (left.IsZero || right.IsZero)
			{
				return Zero;
			}

			var x = left.ToUInt32Array();
			var y = right.ToUInt32Array();
			var z = new uint[Math.Max(x.Length, y.Length)];
			var xExtend = left._sign < 0 ? UInt32.MaxValue : 0;
			var yExtend = right._sign < 0 ? UInt32.MaxValue : 0;

			for (var i = 0; i < z.Length; i++)
			{
				var xu = i < x.Length ? x[i] : xExtend;
				var yu = i < y.Length ? y[i] : yExtend;
				z[i] = xu & yu;
			}
			return new BigInteger(z);
		}

		public static BigInteger operator |(BigInteger left, BigInteger right)
		{
			if (left.IsZero)
				return right;
			if (right.IsZero)
				return left;

			var x = left.ToUInt32Array();
			var y = right.ToUInt32Array();
			var z = new uint[Math.Max(x.Length, y.Length)];
			var xExtend = left._sign < 0 ? UInt32.MaxValue : 0;
			var yExtend = right._sign < 0 ? UInt32.MaxValue : 0;

			for (var i = 0; i < z.Length; i++)
			{
				var xu = i < x.Length ? x[i] : xExtend;
				var yu = i < y.Length ? y[i] : yExtend;
				z[i] = xu | yu;
			}
			return new BigInteger(z);
		}

		public static BigInteger operator ^(BigInteger left, BigInteger right)
		{
			var x = left.ToUInt32Array();
			var y = right.ToUInt32Array();
			var z = new uint[Math.Max(x.Length, y.Length)];
			var xExtend = left._sign < 0 ? UInt32.MaxValue : 0;
			var yExtend = right._sign < 0 ? UInt32.MaxValue : 0;

			for (var i = 0; i < z.Length; i++)
			{
				var xu = i < x.Length ? x[i] : xExtend;
				var yu = i < y.Length ? y[i] : yExtend;
				z[i] = xu ^ yu;
			}

			return new BigInteger(z);
		}

		public static BigInteger operator <<(BigInteger value, int shift)
		{

			if (shift == 0) return value;
			else if (shift == Int32.MinValue) return value >> Int32.MaxValue >> 1;
			else if (shift < 0) return value >> -shift;

			var digitShift = shift / kcbitUint;
			var smallShift = shift - digitShift * kcbitUint;

			uint[] xd; int xl;
			var negx = GetPartsForBitManipulation(ref value, out xd, out xl);

			var zl = xl + digitShift + 1;
			var zd = new uint[zl];

			if (smallShift == 0)
			{
				for (var i = 0; i < xl; i++)
				{
					zd[i + digitShift] = xd[i];
				}
			}
			else
			{
				var carryShift = kcbitUint - smallShift;
				uint carry = 0;
				int i;
				for (i = 0; i < xl; i++)
				{
					var rot = xd[i];
					zd[i + digitShift] = rot << smallShift | carry;
					carry = rot >> carryShift;
				}
				zd[i + digitShift] = carry;
			}
			return new BigInteger(zd, negx);
		}

		public static BigInteger operator >>(BigInteger value, int shift)
		{
			if (shift == 0) return value;
			else if (shift == Int32.MinValue) return value << Int32.MaxValue << 1;
			else if (shift < 0) return value << -shift;

			var digitShift = shift / kcbitUint;
			var smallShift = shift - digitShift * kcbitUint;

			uint[] xd; int xl;
			var negx = GetPartsForBitManipulation(ref value, out xd, out xl);

			if (negx)
			{
				if (shift >= kcbitUint * xl)
				{
					return MinusOne;
				}
				var temp = new uint[xl];
				Array.Copy(xd , temp , xl );  // make a copy of immutable value._bits
				xd = temp;
				NumericsHelpers.DangerousMakeTwosComplement(xd); // mutates xd
			}

			var zl = xl - digitShift;
			if (zl < 0) zl = 0;
			var zd = new uint[zl];

			if (smallShift == 0)
			{
				for (var i = xl - 1; i >= digitShift; i--)
				{
					zd[i - digitShift] = xd[i];
				}
			}
			else
			{
				var carryShift = kcbitUint - smallShift;
				uint carry = 0;
				for (var i = xl - 1; i >= digitShift; i--)
				{
					var rot = xd[i];
					if (negx && i == xl - 1)
						// sign-extend the first shift for negative ints then let the carry propagate
						zd[i - digitShift] = (rot >> smallShift) | (0xFFFFFFFF << carryShift);
					else
						zd[i - digitShift] = (rot >> smallShift) | carry;
					carry = rot << carryShift;
				}
			}
			if (negx)
			{
				NumericsHelpers.DangerousMakeTwosComplement(zd); // mutates zd
			}
			return new BigInteger(zd, negx);
		}


		public static BigInteger operator ~(BigInteger value)
		{
			return -(value + One);
		}

		public static BigInteger operator -(BigInteger value)
		{
			value.AssertValid();
			value._sign = -value._sign;
			value.AssertValid();
			return value;
		}

		public static BigInteger operator +(BigInteger value)
		{
			value.AssertValid();
			return value;
		}


		public static BigInteger operator ++(BigInteger value)
		{
			return value + One;
		}

		public static BigInteger operator --(BigInteger value)
		{
			return value - One;
		}


		public static BigInteger operator +(BigInteger left, BigInteger right)
		{
			left.AssertValid();
			right.AssertValid();

			if (right.IsZero) return left;
			if (left.IsZero) return right;

			var sign1 = +1;
			var sign2 = +1;
			var reg1 = new BigIntegerBuilder(left, ref sign1);
			var reg2 = new BigIntegerBuilder(right, ref sign2);

			if (sign1 == sign2)
				reg1.Add(ref reg2);
			else
				reg1.Sub(ref sign1, ref reg2);

			return reg1.GetInteger(sign1);
		}

		public static BigInteger operator -(BigInteger left, BigInteger right)
		{
			left.AssertValid();
			right.AssertValid();

			if (right.IsZero) return left;
			if (left.IsZero) return -right;

			var sign1 = +1;
			var sign2 = -1;
			var reg1 = new BigIntegerBuilder(left, ref sign1);
			var reg2 = new BigIntegerBuilder(right, ref sign2);

			if (sign1 == sign2)
				reg1.Add(ref reg2);
			else
				reg1.Sub(ref sign1, ref reg2);

			return reg1.GetInteger(sign1);
		}

		public static BigInteger operator *(BigInteger left, BigInteger right)
		{
			left.AssertValid();
			right.AssertValid();

			var sign = +1;
			var reg1 = new BigIntegerBuilder(left, ref sign);
			var reg2 = new BigIntegerBuilder(right, ref sign);

			reg1.Mul(ref reg2);
			return reg1.GetInteger(sign);
		}

		public static BigInteger operator /(BigInteger dividend, BigInteger divisor)
		{
			dividend.AssertValid();
			divisor.AssertValid();

			var sign = +1;
			var regNum = new BigIntegerBuilder(dividend, ref sign);
			var regDen = new BigIntegerBuilder(divisor, ref sign);

			regNum.Div(ref regDen);
			return regNum.GetInteger(sign);
		}

		public static BigInteger operator %(BigInteger dividend, BigInteger divisor)
		{
			dividend.AssertValid();
			divisor.AssertValid();

			var signNum = +1;
			var signDen = +1;
			var regNum = new BigIntegerBuilder(dividend, ref signNum);
			var regDen = new BigIntegerBuilder(divisor, ref signDen);

			regNum.Mod(ref regDen);
			return regNum.GetInteger(signNum);
		}*/

		public static bool operator <(BigInteger left, BigInteger right)
		{
			return left.CompareTo(right) < 0;
		}
		public static bool operator <=(BigInteger left, BigInteger right)
		{
			return left.CompareTo(right) <= 0;
		}
		public static bool operator >(BigInteger left, BigInteger right)
		{
			return left.CompareTo(right) > 0;
		}
		public static bool operator >=(BigInteger left, BigInteger right)
		{
			return left.CompareTo(right) >= 0;
		}
		public static bool operator ==(BigInteger left, BigInteger right)
		{
			return left.Equals(right);
		}
		public static bool operator !=(BigInteger left, BigInteger right)
		{
			return !left.Equals(right);
		}

		/*public static bool operator <(BigInteger left, Int64 right)
		{
			return left.CompareTo(right) < 0;
		}
		public static bool operator <=(BigInteger left, Int64 right)
		{
			return left.CompareTo(right) <= 0;
		}
		public static bool operator >(BigInteger left, Int64 right)
		{
			return left.CompareTo(right) > 0;
		}
		public static bool operator >=(BigInteger left, Int64 right)
		{
			return left.CompareTo(right) >= 0;
		}
		public static bool operator ==(BigInteger left, Int64 right)
		{
			return left.Equals(right);
		}
		public static bool operator !=(BigInteger left, Int64 right)
		{
			return !left.Equals(right);
		}

		public static bool operator <(Int64 left, BigInteger right)
		{
			return right.CompareTo(left) > 0;
		}
		public static bool operator <=(Int64 left, BigInteger right)
		{
			return right.CompareTo(left) >= 0;
		}
		public static bool operator >(Int64 left, BigInteger right)
		{
			return right.CompareTo(left) < 0;
		}
		public static bool operator >=(Int64 left, BigInteger right)
		{
			return right.CompareTo(left) <= 0;
		}
		public static bool operator ==(Int64 left, BigInteger right)
		{
			return right.Equals(left);
		}
		public static bool operator !=(Int64 left, BigInteger right)
		{
			return !right.Equals(left);
		}

		public static bool operator <(BigInteger left, UInt64 right)
		{
			return left.CompareTo(right) < 0;
		}
		public static bool operator <=(BigInteger left, UInt64 right)
		{
			return left.CompareTo(right) <= 0;
		}
		public static bool operator >(BigInteger left, UInt64 right)
		{
			return left.CompareTo(right) > 0;
		}
		public static bool operator >=(BigInteger left, UInt64 right)
		{
			return left.CompareTo(right) >= 0;
		}
		public static bool operator ==(BigInteger left, UInt64 right)
		{
			return left.Equals(right);
		}
		public static bool operator !=(BigInteger left, UInt64 right)
		{
			return !left.Equals(right);
		}

		public static bool operator <(UInt64 left, BigInteger right)
		{
			return right.CompareTo(left) > 0;
		}
		public static bool operator <=(UInt64 left, BigInteger right)
		{
			return right.CompareTo(left) >= 0;
		}
		public static bool operator >(UInt64 left, BigInteger right)
		{
			return right.CompareTo(left) < 0;
		}
		public static bool operator >=(UInt64 left, BigInteger right)
		{
			return right.CompareTo(left) <= 0;
		}
		public static bool operator ==(UInt64 left, BigInteger right)
		{
			return right.Equals(left);
		}
		public static bool operator !=(UInt64 left, BigInteger right)
		{
			return !right.Equals(left);
		}*/

		#endregion internal static operators


		// ----- SECTION: private serialization instance methods  ----------------
		#region private serialization instance methods
		#endregion private serialization instance methods


		// ----- SECTION: internal instance utility methods ----------------*
		#region internal instance utility methods

		/*private void SetBitsFromDouble(Double value)
		{
			int sign, exp;
			ulong man;
			bool fFinite;
			NumericsHelpers.GetDoubleParts(value, out sign, out exp, out man, out fFinite);
			Contract.Assert(sign == +1 || sign == -1);

			if (man == 0)
			{
				this = Zero;
				return;
			}

			Contract.Assert(man < 1UL << 53);
			Contract.Assert(exp <= 0 || man >= 1UL << 52);

			if (exp <= 0)
			{
				if (exp <= -kcbitUlong)
				{
					this = Zero;
					return;
				}
				this = man >> -exp;
				if (sign < 0)
					_sign = -_sign;
			}
			else if (exp <= 11)
			{
				this = man << exp;
				if (sign < 0)
					_sign = -_sign;
			}
			else
			{
				// Overflow into at least 3 uints.
				// Move the leading 1 to the high bit.
				man <<= 11;
				exp -= 11;

				// Compute cu and cbit so that exp == 32 * cu - cbit and 0 <= cbit < 32.
				var cu = (exp - 1) / kcbitUint + 1;
				var cbit = cu * kcbitUint - exp;
				Contract.Assert(0 <= cbit && cbit < kcbitUint);
				Contract.Assert(cu >= 1);

				// Populate the uints.
				_bits = new uint[cu + 2];
				_bits[cu + 1] = (uint)(man >> (cbit + kcbitUint));
				_bits[cu] = (uint)(man >> cbit);
				if (cbit > 0)
					_bits[cu - 1] = (uint)man << (kcbitUint - cbit);
				_sign = sign;
			}
		}*/
		#endregion internal instance utility methods


		// ----- SECTION: internal static utility methods ----------------*
		#region internal static utility methods
		//[Pure]
		private static int Length(uint[] rgu)
		{
			var cu = rgu.Length;
			if (rgu[cu - 1] != 0)
				return cu;
			Contract.Assert(cu >= 2 && rgu[cu - 2] != 0);
			return cu - 1;
		}

		internal int _Sign { get { return _sign; } }
		internal uint[] _Bits { get { return _bits; } }


/*
		internal static int BitLengthOfUInt(uint x)
		{
			var numBits = 0;
			while (x > 0)
			{
				x >>= 1;
				numBits++;
			}
			return numBits;
		}
*/

		//
		// GetPartsForBitManipulation -
		//
		// Encapsulate the logic of normalizing the "small" and "large" forms of BigInteger
		// into the "large" form so that Bit Manipulation algorithms can be simplified 
		//
		// uint[] xd    =    the UInt32 array containing the entire big integer in "large" (denormalized) form
		//                       E.g., the number one (1) and negative one (-1) are both stored as 0x00000001
		//                       BigInteger values Int32.MinValue < x <= Int32.MaxValue are converted to this
		//                       format for convenience.
		// int xl       =    the length of xd
		// return bool  =    true for negative numbers
		//
		/*private static bool GetPartsForBitManipulation(ref BigInteger x, out uint[] xd, out int xl)
		{
			if (x._bits == null)
			{
				xd = x._sign < 0 ? new[] { (uint)-x._sign } : new[] { (uint)x._sign };
			}
			else
			{
				xd = x._bits;
			}
			xl = x._bits == null ? 1 : x._bits.Length;
			return x._sign < 0;
		}*/

		// Gets an upper bound on the product of uHiRes * (2^32)^(cuRes-1) and uHiMul * (2^32)^(cuMul-1).
		// The result is put int uHiRes and cuRes.
/*
		private static void MulUpper(ref uint uHiRes, ref int cuRes, uint uHiMul, int cuMul)
		{
			var uu = (ulong)uHiRes * uHiMul;
			var uHi = NumericsHelpers.GetHi(uu);
			if (uHi != 0)
			{
				if (NumericsHelpers.GetLo(uu) != 0 && ++uHi == 0)
				{
					uHi = 1;
					cuRes++;
				}
				uHiRes = uHi;
				cuRes += cuMul;
			}
			else
			{
				uHiRes = NumericsHelpers.GetLo(uu);
				cuRes += cuMul - 1;
			}
		}
*/

		// Gets a lower bound on the product of uHiRes * (2^32)^(cuRes-1) and uHiMul * (2^32)^(cuMul-1).
		// The result is put int uHiRes and cuRes.
/*
		private static void MulLower(ref uint uHiRes, ref int cuRes, uint uHiMul, int cuMul)
		{
			var uu = (ulong)uHiRes * uHiMul;
			var uHi = NumericsHelpers.GetHi(uu);
			if (uHi != 0)
			{
				uHiRes = uHi;
				cuRes += cuMul;
			}
			else
			{
				uHiRes = NumericsHelpers.GetLo(uu);
				cuRes += cuMul - 1;
			}
		}
*/

		private static int GetDiffLength(uint[] rgu1, uint[] rgu2, int cu)
		{
			for (var iv = cu; --iv >= 0; )
			{
				if (rgu1[iv] != rgu2[iv])
					return iv + 1;
			}
			return 0;
		}
		#endregion internal static utility methods
	}
}