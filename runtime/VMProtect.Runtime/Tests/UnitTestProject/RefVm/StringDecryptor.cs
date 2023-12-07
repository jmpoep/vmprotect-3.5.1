using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using forms_cil;

// Token: 0x0200003D RID: 61
namespace UnitTestProject.RefVm
{
    public static class StringDecryptor // \u0006\u2009
    {
        private static BinStreamReader _binStreamReader; // \u0003
        private static readonly DecryptedStrings DecryptedStringsGlobal; // \u0002
        private static byte[] _commonKey; // \u0005
        private static short _keyLength; // \u0008
        private static readonly Enum1 Enum1Dummy; // \u0003\u2000
        private static int _int1Dummy; // \u0002\u2000
        private static int _int2Dummy; // \u000F
        private static int _int3Dummy; // \u0006
        private static byte[] _pkt; // \u000E

        // Token: 0x060002CE RID: 718 RVA: 0x00013060 File Offset: 0x00011260
        [MethodImpl(MethodImplOptions.NoInlining)]
        static StringDecryptor()
        {
            var num = -42518532;
            var num2 = num ^ 1885636661;
            DecryptedStringsGlobal = new DecryptedStrings(1970604170 + num + num2 /*9*/);

            var num3 = 2;
            var stackTrace = new StackTrace(num3, false);
            num3 -= 2;
            var frame = stackTrace.GetFrame(num3);
            var index = num3;
            if (frame == null)
            {
                stackTrace = new StackTrace();
                index = 1;
                frame = stackTrace.GetFrame(index);
            }
            var num4 = ~- -~~-~-~(1341405001 ^ num ^ num2) ^ -~~- -~-~~(2095196650 + num - num2);
            var methodBase = frame?.GetMethod();
            if (frame != null)
            {
                num4 ^= ~- -~~-~-~(num ^ -1658751032 ^ num2);
            }
            var type = methodBase?.DeclaringType;
            if (type == typeof(RuntimeMethodHandle))
            {
                Enum1Dummy = (Enum1)(4 | (int)Enum1Dummy);
                num4 ^= 1970604898 + num + num2 + num3;
            }
            else if (type == null)
            {
                if (CheckStack(stackTrace, index))
                {
                    num4 ^= ~-~- -~~-~(-1970628130 - num - num2) - num3;
                    Enum1Dummy = (Enum1)(16 | (int)Enum1Dummy);
                }
                else
                {
                    num4 ^= ~- -~~-~-~(num ^ -1885608078 ^ num2);
                    Enum1Dummy = (Enum1)(1 | (int)Enum1Dummy);
                }
            }
            else
            {
                num4 ^= ~-~- -~~-~(1885542988 - num + num2) - num3;
                Enum1Dummy = (Enum1)(16 | (int)Enum1Dummy);
            }
            _int1Dummy += num4;
        }

        // Token: 0x060002CF RID: 719 RVA: 0x000131D4 File Offset: 0x000113D4
        public static string GetString(int id) // \u0002
        {
            lock (DecryptedStringsGlobal)
            {
                return DecryptedStringsGlobal[id] ?? DecryptString(id, true);
            }
        }
        public static void SetString(int id, string val) // for unit tests
        {
            lock (DecryptedStringsGlobal)
            {
                DecryptedStringsGlobal[id] = val;
            }
        }

        // Token: 0x060002D0 RID: 720 RVA: 0x00013224 File Offset: 0x00011424
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static string DecryptString(int id, bool dummy) // \u0002
        {
            var num = 1500675437;
            var num2 = 1065028357 - num;
            byte[] key;
            string str;
            int num6;
            if (_binStreamReader == null)
            {
                var executingAssembly = Assembly.GetExecutingAssembly();
                _int3Dummy |= num ^ -1084071305 ^ num2;
                var stringBuilder = new StringBuilder();
                var num3 = -1821765671 - num + num2;
                stringBuilder.Append((char)(num3 >> 16)).Append((char)num3);
                num3 = 1822175277 + num ^ num2;
                stringBuilder.Append((char)(num3 >> 16)).Append((char)num3);
                num3 = (1619914499 ^ num) + num2;
                stringBuilder.Append((char)num3).Append((char)(num3 >> 16));
                num3 = 1602104074 - num - num2;
                stringBuilder.Append((char)num3).Append((char)(num3 >> 16));
                num3 = (num ^ 1619980037) + num2;
                stringBuilder.Append((char)num3).Append((char)(num3 >> 16));
                num3 = num + -1398984659 - num2;
                stringBuilder.Append((char)num3).Append((char)(num3 >> 16));
                var manifestResourceStream = executingAssembly.GetManifestResourceStream(/* added by ursoft */ "forms_cil." +
                    stringBuilder.ToString()
                    /* added by ursoft */.GetHashCode().ToString());
                var num4 = 2;
                var stackTrace = new StackTrace(num4, false);
                _int3Dummy ^= (1082521283 ^ num) + num2 | num4;
                num4 -= 2;
                var frame = stackTrace.GetFrame(num4);
                var index = num4;
                if (frame == null)
                {
                    stackTrace = new StackTrace();
                    index = 1;
                    frame = stackTrace.GetFrame(index);
                }
                var methodBase = frame?.GetMethod();
                _int3Dummy ^= num4 + (num + -1936322645 ^ num2);
                var type = methodBase?.DeclaringType;
                if (frame == null)
                {
                    _int3Dummy ^= 1065247672 - num - num2;
                }
                var flag = type == typeof(RuntimeMethodHandle);
                _int3Dummy ^= (num ^ 1082461797) + num2;
                if (!flag)
                {
                    flag = type == null;
                    if (flag)
                    {
                        if (CheckStack(stackTrace, index))
                        {
                            flag = false;
                        }
                        else
                        {
                            _int3Dummy ^= 1065247640 - num - num2;
                        }
                    }
                }
                if (flag)
                {
                    _int3Dummy ^= 32;
                }
                _int3Dummy ^= (num ^ 1082521251) + num2 | num4 + 1;
                _binStreamReader = new BinStreamReader(manifestResourceStream);
                var commonKeyLength = (short)(_binStreamReader.ReadHeaderInt16() ^ ~(short)-(short)-(short)~(short)~(short)-(short)~(short)-(short)~(short)-(short)~(short)(-1065050389 + num ^ num2));
                if (commonKeyLength == 0)
                {
                    _keyLength = (short)(_binStreamReader.ReadHeaderInt16() ^ -(short)~(short)~(short)-(short)-(short)~(short)~(short)-(short)-(short)~(short)~(short)((-1082482306 ^ num) - num2));
                }
                else
                {
                    _commonKey = _binStreamReader.Read(commonKeyLength);
                }
                var assembly = executingAssembly;
                var assemblyName = SafeAssemblyName(assembly);
                _pkt = SafeAssemblyPkt(assemblyName);
                num6 = _int1Dummy;
                _int1Dummy = 0;
                var num7 = SdMetadataTokens.GetLong();
                num6 ^= (int)(uint)num7;
                num6 ^= -888987382 - num + num2;
                var num8 = num6;
                var num9 = num8;
                var num10 = 0;
                var num11 = num9 ^ -1693408934 + num - num2;
                var num12 = num11 * (1936327810 - num + num2) % ((num ^ -1092770072) - num2);
                var num13 = num12;
                var obj = ((I2<int>)new SdTemplateStuff.C(-1065028359 + num | num2)
                {
                    I6 = num13
                }).I2M();
                try
                {
                    while (obj.I4M())
                    {
                        var num14 = obj.I1M();
                        num12 ^= num10 - num14 << 2;
                        num10 += num12 >> 3;
                    }
                }
                finally
                {
                    obj?.I5M();
                }
                num6 ^= ~- -~-~~- -~~(num ^ 1140387705 ^ num2);
                var num15 = num12;
                num6 = num15 + num6;
                _int2Dummy = num6;
                _int3Dummy = (_int3Dummy & -1667887203 + num - num2) ^ (num ^ 1082519937) + num2;
                if ((Enum1Dummy & (Enum1)(-~~- -~-~~(1936322518 - num ^ num2))) == 0)
                {
                    _int3Dummy = (-1082440641 ^ num) - num2;
                }
            }
            else
            {
                num6 = _int2Dummy;
            }
            if (_int3Dummy == 1936366479 - num + num2)
            {
                return new string(new[] { (char)((-1082462051 ^ num) - num2), '0', (char)(num + -1065028269 + num2) });
            }
            var num16 = id ^ -1010833342 ^ num ^ num2 ^ num6;
            num16 ^= -1459130838 - num + num2;
            _binStreamReader.GetStream().Position = num16;
            if (_commonKey != null)
            {
                key = _commonKey;
            }
            else
            {
                short keyLength;
                if (_keyLength == -1)
                {
                    keyLength = (short)(_binStreamReader.ReadHeaderInt32() ^ num + -1936293566 - num2 ^ num16);
                }
                else
                {
                    keyLength = _keyLength;
                }
                if (keyLength == 0)
                {
                    key = null;
                }
                else
                {
                    key = _binStreamReader.Read(keyLength);
                    for (var i = 0; i != key.Length; i = 1 + i)
                    {
                        key[i] ^= (byte)(_int2Dummy >> ((3 & i) << 3));
                    }
                }
            }
            var stringHeader = _binStreamReader.ReadHeaderInt32() ^ num16 ^ -~~-~-~-~((num ^ -1882046960) + num2) ^ num6;
            if (stringHeader != (1936322515 - num | num2))
            {
                var flagAnsi = (stringHeader & 211161131 + num - num2) != 0;
                var flagCompressed = (stringHeader & (-8713467 - num ^ num2)) != 0;
                var flagSecure = (stringHeader & (1619332869 ^ num) + num2) != 0;
                stringHeader &= num + -1870334726 ^ num2;
                var packedStringBuf = _binStreamReader.Read(stringHeader);
                // ReSharper disable once PossibleNullReferenceException
                var key1 = key[1];
                var length = packedStringBuf.Length;
                var key1XorLen = (byte)(11 + length ^ 7 + key1);
                var keysXorLen = (uint)((key[0] | key[2] << 8) + (key1XorLen << 3));
                ushort keysXorLenLowWord = 0;
                for (var index = 0; index < length; ++index)
                {
                    if ((index & 1) == 0)
                    {
                        keysXorLen = keysXorLen * (uint)((num ^ -1082544904) - num2) + (uint)(num + -1933863442 ^ num2);
                        keysXorLenLowWord = (ushort)(keysXorLen >> 16);
                    }
                    var keysXorLenLowByte = (byte)keysXorLenLowWord;
                    keysXorLenLowWord = (ushort)(keysXorLenLowWord >> 8);
                    var inByte = packedStringBuf[index];
                    packedStringBuf[index] = (byte)(inByte ^ key1 ^ 3 + key1XorLen ^ keysXorLenLowByte);
                    key1XorLen = inByte;
                }
                if (_pkt != null != (_int3Dummy != (-1085052045 ^ num) - num2))
                {
                    for (var j = 0; j < stringHeader; j = 1 + j)
                    {
                        // ReSharper disable once PossibleNullReferenceException
                        var b5 = _pkt[7 & j];
                        b5 = (byte)(b5 << 3 | b5 >> 5);
                        packedStringBuf[j] ^= b5;
                    }
                }
                var num23 = _int3Dummy - 12;
                int unpackedLength;
                byte[] unpackedStringBuf;
                if (!flagCompressed)
                {
                    unpackedLength = stringHeader;
                    unpackedStringBuf = packedStringBuf;
                }
                else
                {
                    unpackedLength = packedStringBuf[2] | packedStringBuf[0] << 16 | packedStringBuf[3] << 8 | packedStringBuf[1] << 24;
                    unpackedStringBuf = new byte[unpackedLength];
                    Unpack(packedStringBuf, 4, unpackedStringBuf);
                }
                if (flagAnsi && num23 == (num + -1935832971 ^ num2))
                {
                    var chArray = new char[unpackedLength];
                    for (var k = 0; k < unpackedLength; k++)
                    {
                        chArray[k] = (char)unpackedStringBuf[k];
                    }
                    str = new string(chArray);
                }
                else
                {
                    str = Encoding.Unicode.GetString(unpackedStringBuf, 0, unpackedStringBuf.Length);
                }
                num23 += (-1082461318 ^ num) - num2 + (3 & num23) << 5;
                if (num23 != (1065521775 - num ^ num2))
                {
                    var num25 = stringHeader + id ^ -1935385949 + num - num2 ^ (num23 & (-1082460680 ^ num ^ num2));
                    var stringBuilder = new StringBuilder();
                    var num3 = 1065028445 - num - num2;
                    stringBuilder.Append((char)(byte)num3);
                    str = num25.ToString(stringBuilder.ToString());
                }
                if (!flagSecure & dummy)
                {
                    str = string.Intern(str);
                    DecryptedStringsGlobal[id] = str;
                    if (DecryptedStringsGlobal.GetCachedPairs() == (num + -1936322454 ^ num2))
                    {
                        _binStreamReader.Close();
                        _binStreamReader = null;
                        _commonKey = null;
                        _pkt = null;
                    }
                }
                return str;
            }
            var refId = _binStreamReader.Read(4);
            id = -64102883 ^ num ^ num2 ^ num6;
            id = (refId[2] | refId[3] << 16 | refId[0] << 8 | refId[1] << 24) ^ -id;
            str = DecryptedStringsGlobal[id];
            return str;
        }

        // Token: 0x060002D1 RID: 721 RVA: 0x00013A28 File Offset: 0x00011C28
        private static AssemblyName SafeAssemblyName(Assembly a) // \u0002
        {
            AssemblyName result;
            try
            {
                result = a.GetName();
            }
            catch
            {
                result = new AssemblyName(a.FullName);
            }
            return result;
        }

        // Token: 0x060002D2 RID: 722 RVA: 0x00013A60 File Offset: 0x00011C60
        private static byte[] SafeAssemblyPkt(AssemblyName an) // \u0002
        {
            var array = an.GetPublicKeyToken();
            if (array != null && array.Length == 0)
            {
                array = null;
            }
            return array;
        }

        // Token: 0x060002D3 RID: 723 RVA: 0x00013A80 File Offset: 0x00011C80
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool CheckStack(StackTrace st, int idx) // \u0002
        {
            var a = st.GetFrame(idx + 1)?.GetMethod()?.DeclaringType?.Assembly;
            if (a != null)
            {
                var assemblyName = SafeAssemblyName(a);
                var array = SafeAssemblyPkt(assemblyName);
                if (array != null && array.Length == 8 && array[0] == 183 && array[7] == 137)
                {
                    return true;
                }
            }
            return false;
        }

        // Token: 0x060002D4 RID: 724 RVA: 0x00013AF0 File Offset: 0x00011CF0
        private static void Unpack(byte[] packedStringBuf, int idxPacked, byte[] unpackedStringBuf) // \u0002
        {
            var idxUnpacked = 0;
            var packedBlockHeader = 0;
            var byteMask = 0x80;
            var unpackLen = unpackedStringBuf.Length;
            while (idxUnpacked < unpackLen)
            {
                byteMask <<= 1;
                if (byteMask == 0x100)
                {
                    byteMask = 1;
                    packedBlockHeader = packedStringBuf[idxPacked++];
                }
                if ((packedBlockHeader & byteMask) == 0)
                {
                    unpackedStringBuf[idxUnpacked++] = packedStringBuf[idxPacked++];
                    continue;
                }
                var knownWordLen = (packedStringBuf[idxPacked] >> 2) + 3;
                var knownWordOffset = ((packedStringBuf[idxPacked] << 8) | packedStringBuf[idxPacked + 1]) & 0x3ff;
                idxPacked += 2;
                var knownWordIdx = idxUnpacked - knownWordOffset;
                if (knownWordIdx < 0)
                    return;
                while (--knownWordLen >= 0 && idxUnpacked < unpackLen)
                {
                    unpackedStringBuf[idxUnpacked++] = unpackedStringBuf[knownWordIdx++];
                }
            }
        }

        // Token: 0x0200003E RID: 62
        internal sealed class BinStreamReader // \u0003\u2002\u200A\u2008\u2008\u200B\u2002\u2006\u2002\u2009\u2009\u2006\u2004\u2006\u2002\u2007\u2006\u2003\u2001\u2007\u2003\u200A\u2009\u200B\u2003\u2001
        {
            private byte[] _header; // \u0003
            private Stream _stream; // \u0002

            // Token: 0x060002D5 RID: 725 RVA: 0x00013B94 File Offset: 0x00011D94
            public BinStreamReader(Stream stream)
            {
                _stream = stream;
                _header = new byte[4];
            }

            // Token: 0x060002D6 RID: 726 RVA: 0x00013BB0 File Offset: 0x00011DB0
            public Stream GetStream() // \u0002
            {
                return _stream;
            }

            // Token: 0x060002D7 RID: 727 RVA: 0x00013BB8 File Offset: 0x00011DB8
            public short ReadHeaderInt16() // \u0002
            {
                ReadHeader(2);
                return (short) (_header[0] | (_header[1] << 8));
            }

            // Token: 0x060002D8 RID: 728 RVA: 0x00013BD8 File Offset: 0x00011DD8
            public int ReadHeaderInt32() // \u0002
            {
                ReadHeader(4);
                return _header[0] | (_header[1] << 8) | (_header[2] << 0x10) | (_header[3] << 0x18);
            }

            // Token: 0x060002D9 RID: 729 RVA: 0x00013C0C File Offset: 0x00011E0C
            private void ThrowEOS() // \u0002
            {
                throw new EndOfStreamException();
            }

            // Token: 0x060002DA RID: 730 RVA: 0x00013C14 File Offset: 0x00011E14
            private void ReadHeader(int headerSize) // \u0002
            {
                var offset = 0;
                int read;
                if (headerSize == 1)
                {
                    read = _stream.ReadByte();
                    if (read == -1)
                        ThrowEOS();
                    _header[0] = (byte) read;
                }
                else
                {
                    do
                    {
                        read = _stream.Read(_header, offset, headerSize - offset);
                        if (read == 0)
                            ThrowEOS();
                        offset += read;
                    } while (offset < headerSize);
                }
            }

            // Token: 0x060002DB RID: 731 RVA: 0x00013C74 File Offset: 0x00011E74
            public void Close() // \u0003
            {
                var stream = _stream;
                _stream = null;
                stream?.Close();
                _header = null;
            }

            // Token: 0x060002DC RID: 732 RVA: 0x00013CA0 File Offset: 0x00011EA0
            public byte[] Read(int bytesCount) // \u0002
            {
                if (bytesCount < 0)
                    throw new ArgumentOutOfRangeException();
                var buffer = new byte[bytesCount];
                var offset = 0;
                do
                {
                    var read = _stream.Read(buffer, offset, bytesCount);
                    if (read == 0)
                        break;
                    offset += read;
                    bytesCount -= read;
                } while (bytesCount > 0);
                if (offset != buffer.Length)
                {
                    var dst = new byte[offset];
                    Buffer.BlockCopy(buffer, 0, dst, 0, offset);
                    buffer = dst;
                }
                return buffer;
            }
        }

        // Token: 0x0200003F RID: 63
        internal sealed class DecryptedStrings // \u0005\u2001\u2007\u200B\u2000\u2008\u2008\u2000\u2000\u2006\u2004\u2008\u200B\u2002\u2006\u200B\u2001\u200B\u2003\u2004\u2001\u2004
        {
            private int _cachedPairs; // \u0003
            private Pair[] _arr; // \u0002

            // Token: 0x060002DD RID: 733 RVA: 0x00013CFC File Offset: 0x00011EFC
            public DecryptedStrings()
            {
                _arr = new Pair[0x10];
            }

            // Token: 0x060002DE RID: 734 RVA: 0x00013D14 File Offset: 0x00011F14
            public DecryptedStrings(int capacityHint)
            {
                var capacity = 0x10;
                capacityHint = capacityHint << 1;
                while ((capacity < capacityHint) && (capacity > 0))
                {
                    capacity = capacity << 1;
                }
                if (capacity < 0)
                {
                    capacity = 0x10;
                }
                _arr = new Pair[capacity];
            }

            // Token: 0x060002DF RID: 735 RVA: 0x00013D54 File Offset: 0x00011F54
            public int GetCachedPairs()
            {
                return _cachedPairs;
            }

            // Token: 0x060002E0 RID: 736 RVA: 0x00013D5C File Offset: 0x00011F5C
            private void GrowCache() // \u0002
            {
                var length = _arr.Length;
                var newLength = length * 2;
                if (newLength > 0)
                {
                    var newArr = new Pair[newLength];
                    var cnt = 0;
                    for (var i = 0; i < length; i++)
                    {
                        if (_arr[i].val != null)
                        {
                            var index = _arr[i].id & (newLength - 1);
                            while (true)
                            {
                                if (newArr[index].val == null)
                                {
                                    newArr[index].val = _arr[i].val;
                                    newArr[index].id = _arr[i].id;
                                    cnt++;
                                    break;
                                }
                                index++;
                                if (index >= newLength)
                                    index = 0;
                            }
                        }
                    }
                    _arr = newArr;
                    _cachedPairs = cnt;
                }
            }

            public string this[int id]
            {
                // Token: 0x060002E1 RID: 737 RVA: 0x00013E10 File Offset: 0x00012010
                get // \u0002
                {
                    var length = _arr.Length;
                    var index = id & (length - 1);
                    do
                    {
                        if (_arr[index].id == id || _arr[index].val == null)
                            return _arr[index].val;
                        index++;
                        if (index >= length)
                            index = 0;
                    } while (true);
                }
                // Token: 0x060002E2 RID: 738 RVA: 0x00013E6C File Offset: 0x0001206C
                set // \u0002
                {
                    var length = _arr.Length;
                    var halfLength = length >> 1;
                    var index = id & (length - 1);
                    do
                    {
                        var emptySlot = _arr[index].val == null;
                        if (_arr[index].id == id || emptySlot)
                        {
                            _arr[index].val = value;
                            if (emptySlot)
                            {
                                _arr[index].id = id;
                                _cachedPairs++;
                                if (_cachedPairs > halfLength)
                                    GrowCache();
                            }
                            break;
                        }
                        index++;
                        if (index >= length)
                            index = 0;
                    } while (true);
                }
            }

            // Token: 0x02000040 RID: 64
            [StructLayout(LayoutKind.Sequential)]
            private struct Pair // \u0002
            {
                public int id;
                public string val;
            }
        }

        // Token: 0x02000041 RID: 65
        [Flags]
        internal enum Enum1
        {

        }
    }
}
