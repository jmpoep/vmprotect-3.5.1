using System;
using System.IO;
using System.Text;
using forms_cil;

namespace UnitTestProject.RefVm
{
    // Token: 0x0200000A RID: 10
    internal sealed class MyBufferReader : IDisposable // \u0002\u2007
    {
        // Token: 0x0400000C RID: 12
        private MyBuffer _src; // \u0002

	    // Token: 0x0400000D RID: 13
	    private byte[] _inputRawBuf; // \u0003

	    // Token: 0x0400000E RID: 14
	    private Decoder _decoder; // \u0005

        // Token: 0x0400000F RID: 15
        private byte[] _inputCharRawBuf; // \u0008

        // Token: 0x04000010 RID: 16
        private char[] _inputCharsDecoded; // \u0006

	    // Token: 0x04000011 RID: 17
	    private char[] _stringChunk; // \u000E

        // Token: 0x04000012 RID: 18
        private readonly int _maxCharsIn128Bytes; // \u000F

	    // Token: 0x04000013 RID: 19
        private readonly bool _isUnicode; // \u0002\u2000

	    // Token: 0x04000014 RID: 20
	    private readonly bool _alwaysTrue; // \u0003\u2000

        // Token: 0x06000034 RID: 52 RVA: 0x000027C8 File Offset: 0x000009C8
        public MyBufferReader(MyBuffer src) : this(src, new UTF8Encoding())
	    {
	    }

	    // Token: 0x06000035 RID: 53 RVA: 0x000027D8 File Offset: 0x000009D8
	    private MyBufferReader(MyBuffer src, Encoding encoding)
	    {
		    if (src == null)
		    {
			    throw new ArgumentNullException();
		    }
		    if (encoding == null)
		    {
			    throw new ArgumentNullException();
		    }
		    if (!src.IsValid())
		    {
			    throw new ArgumentException();
		    }
		    _src = src;
		    _decoder = encoding.GetDecoder();
		    _maxCharsIn128Bytes = encoding.GetMaxCharCount(128);
		    var num = encoding.GetMaxByteCount(1);
		    if (num < 16)
			    num = 16;
		    _inputRawBuf = new byte[num];
		    _stringChunk = null;
		    _inputCharRawBuf = null;
		    _isUnicode = (encoding is UnicodeEncoding);
		    _alwaysTrue = (_src != null);
	    }

	    // Token: 0x06000036 RID: 54 RVA: 0x00002878 File Offset: 0x00000A78
	    public MyBuffer GetBuffer() // \u0002
	    {
		    return _src;
	    }

	    // Token: 0x06000037 RID: 55 RVA: 0x00002880 File Offset: 0x00000A80
	    public void Dispose() // \u0002
	    {
            DoDispose(true);
	    }

	    // Token: 0x06000038 RID: 56 RVA: 0x0000288C File Offset: 0x00000A8C
	    private void DoDispose(bool disposing) // \u0002
	    {
		    if (disposing)
			    _src?.DoDispose();
		    _src = null;
		    _inputRawBuf = null;
		    _decoder = null;
		    _inputCharRawBuf = null;
		    _inputCharsDecoded = null;
		    _stringChunk = null;
	    }

	    // Token: 0x06000039 RID: 57 RVA: 0x000028E0 File Offset: 0x00000AE0
	    void IDisposable.Dispose() // \u0002\u2007\u2008\u2000\u2002\u200A\u0003
        {
		    DoDispose(true);
	    }

	    // Token: 0x0600003A RID: 58 RVA: 0x000028EC File Offset: 0x00000AEC
	    public int PeekUnicodeChar() // \u0002
	    {
            SrcPrecondition();
		    if (!_src.IsValid2())
		    {
			    return -1;
		    }
		    var pos = _src.GetPos();
		    var ret = SafeReadUnicodeChar();
		    _src.SetPos(pos);
		    return ret;
	    }

	    // Token: 0x06000051 RID: 81 RVA: 0x00002FE8 File Offset: 0x000011E8
	    private void SrcPrecondition() // \u0005
        {
		    if (_src == null)
		    {
			    throw new Exception();
		    }
	    }

	    // Token: 0x0600003B RID: 59 RVA: 0x0000292C File Offset: 0x00000B2C
	    public int SafeReadUnicodeChar() // \u0003
	    {
            SrcPrecondition();
		    return ReadUnicodeChar();
	    }

	    // Token: 0x0600004E RID: 78 RVA: 0x00002E38 File Offset: 0x00001038
	    private int ReadUnicodeChar() // \u0005
	    {
		    var charCnt = 0;
		    var savedPos = 0L;
		    if (_src.IsValid2())
		    {
			    savedPos = _src.GetPos();
		    }
		    if (_inputCharRawBuf == null)
		    {
			    _inputCharRawBuf = new byte[128];
		    }
		    if (_inputCharsDecoded == null)
		    {
			    _inputCharsDecoded = new char[1];
		    }
		    while (charCnt == 0)
		    {
			    var bytesToRead = _isUnicode ? 2 : 1;
			    var b = _src.ReadByte();
			    _inputCharRawBuf[0] = (byte)b;
			    if (b == -1)
			    {
				    bytesToRead = 0;
			    }
			    if (bytesToRead == 2)
			    {
				    b = _src.ReadByte();
				    _inputCharRawBuf[1] = (byte)b;
				    if (b == -1)
				    {
					    bytesToRead = 1;
				    }
			    }
			    if (bytesToRead == 0)
			    {
				    return -1;
			    }
			    try
			    {
				    charCnt = _decoder.GetChars(_inputCharRawBuf, 0, bytesToRead, _inputCharsDecoded, 0);
			    }
			    catch
			    {
				    if (_src.IsValid2())
				    {
					    _src.Seek(savedPos - _src.GetPos(), SeekOrigin.Current);
				    }
				    throw;
			    }
		    }
		    if (charCnt == 0)
		    {
			    return -1;
		    }
		    return _inputCharsDecoded[0];
	    }

	    // Token: 0x06000053 RID: 83 RVA: 0x0000305C File Offset: 0x0000125C
	    private void ReadToRawBuf(int cnt) // \u0002
	    {
            SrcPrecondition();
		    var offset = 0;
		    int bytesRead;
		    if (cnt != 1)
		    {
			    while (true)
			    {
				    bytesRead = _src.Read(_inputRawBuf, offset, cnt - offset);
				    if (bytesRead == 0)
					    break;
				    offset += bytesRead;
				    if (offset >= cnt)
					    return;
			    }
			    throw new Exception();
		    }
		    bytesRead = _src.ReadByte();
		    if (bytesRead == -1)
		    {
			    throw new Exception();
		    }
		    _inputRawBuf[0] = (byte)bytesRead;
	    }

	    // Token: 0x0600003C RID: 60 RVA: 0x0000293C File Offset: 0x00000B3C
	    public bool ReadByteInternal() // \u0002
        {
            ReadToRawBuf(1);
		    return _inputRawBuf[0] > 0;
	    }

	    // Token: 0x0600003D RID: 61 RVA: 0x00002950 File Offset: 0x00000B50
	    public byte ReadByte() // \u0002
	    {
		    SrcPrecondition();
		    var b = _src.ReadByte();
		    if (b == -1)
		    {
			    throw new Exception();
		    }
		    return (byte)b;
	    }

	    // Token: 0x0600003E RID: 62 RVA: 0x00002970 File Offset: 0x00000B70
	    public sbyte ReadSbyte() // \u0002
	    {
		    ReadToRawBuf(1);
		    return (sbyte)_inputRawBuf[0];
	    }

	    // Token: 0x0600003F RID: 63 RVA: 0x00002984 File Offset: 0x00000B84
	    public char ReadChar() // \u0002
	    {
		    var c = SafeReadUnicodeChar();
		    if (c == -1)
		    {
			    throw new Exception();
		    }
		    return (char)c;
	    }

	    // Token: 0x06000040 RID: 64 RVA: 0x00002998 File Offset: 0x00000B98
	    public short ReadShort() // \u0002
	    {
		    ReadToRawBuf(2);
		    return (short)(_inputRawBuf[0] | _inputRawBuf[1] << 8);
	    }

	    // Token: 0x06000041 RID: 65 RVA: 0x000029B8 File Offset: 0x00000BB8
	    public ushort ReadUshort() // \u0002
	    {
		    ReadToRawBuf(2);
		    return (ushort)(_inputRawBuf[0] | _inputRawBuf[1] << 8);
	    }

	    // Token: 0x06000042 RID: 66 RVA: 0x000029D8 File Offset: 0x00000BD8
	    public uint ReadUint() // \u0002
	    {
		    ReadToRawBuf(4);
		    return (uint)(_inputRawBuf[0] | _inputRawBuf[1] << 8 | _inputRawBuf[2] << 16 | _inputRawBuf[3] << 24);
	    }

	    // Token: 0x06000043 RID: 67 RVA: 0x00002A0C File Offset: 0x00000C0C
	    public long ReadLong() // \u0002
	    {
		    ReadToRawBuf(8);
		    var num = (uint)(_inputRawBuf[0] | _inputRawBuf[1] << 8 | _inputRawBuf[2] << 16 | _inputRawBuf[3] << 24);
		    return (long)((ulong)(_inputRawBuf[4] | _inputRawBuf[5] << 8 | _inputRawBuf[6] << 16 | _inputRawBuf[7] << 24) << 32 | num);
	    }

	    // Token: 0x06000044 RID: 68 RVA: 0x00002A80 File Offset: 0x00000C80
	    public ulong ReadUlong() // \u0002
	    {
		    ReadToRawBuf(8);
		    var num = (uint)(_inputRawBuf[0] | _inputRawBuf[1] << 8 | _inputRawBuf[2] << 16 | _inputRawBuf[3] << 24);
		    return (ulong)(_inputRawBuf[4] | _inputRawBuf[5] << 8 | _inputRawBuf[6] << 16 | _inputRawBuf[7] << 24) << 32 | num;
	    }

	    // Token: 0x06000045 RID: 69 RVA: 0x00002AF4 File Offset: 0x00000CF4
	    private BinaryReader ReaderFor(int cnt) // \u0002
	    {
		    ReadToRawBuf(cnt);
		    return new BinaryReader(new MemoryStream(_inputRawBuf, 0, cnt, false));
	    }

	    // Token: 0x06000046 RID: 70 RVA: 0x00002B10 File Offset: 0x00000D10
	    public float ReadFloat() // \u0002
	    {
		    var r = ReaderFor(4);
		    var result = r.ReadSingle();
		    r.Close();
		    return result;
	    }

	    // Token: 0x06000047 RID: 71 RVA: 0x00002B34 File Offset: 0x00000D34
	    public double ReadDouble() // \u0002
	    {
		    var r = ReaderFor(8);
		    var result = r.ReadDouble();
		    r.Close();
		    return result;
	    }

	    // Token: 0x06000048 RID: 72 RVA: 0x00002B58 File Offset: 0x00000D58
	    private static decimal CreateDecimal(int lo, int mid, int hi, int scaleNeg) // \u0002
	    {
		    var isNegative = (scaleNeg & -2147483648) != 0;
		    var scale = (byte)(scaleNeg >> 16);
		    return new decimal(lo, mid, hi, isNegative, scale);
	    }

	    // Token: 0x06000049 RID: 73 RVA: 0x00002B80 File Offset: 0x00000D80
	    internal static decimal CreateDecimal(byte[] b) // b
	    {
		    var lo = b[0] | b[1] << 8 | b[2] << 16 | b[3] << 24;
		    var mid = b[4] | b[5] << 8 | b[6] << 16 | b[7] << 24;
		    var hi = b[8] | b[9] << 8 | b[10] << 16 | b[11] << 24;
		    var scaleNeg = b[12] | b[13] << 8 | b[14] << 16 | b[15] << 24;
		    return CreateDecimal(lo, mid, hi, scaleNeg);
	    }

	    // Token: 0x0600004A RID: 74 RVA: 0x00002BFC File Offset: 0x00000DFC
	    public decimal ReadDecimal() // \u0002
	    {
		    ReadToRawBuf(16);
		    return CreateDecimal(_inputRawBuf);
	    }

	    // Token: 0x0600004B RID: 75 RVA: 0x00002C14 File Offset: 0x00000E14
	    public string ReadString() // \u0002
	    {
		    var totalRead = 0;
		    SrcPrecondition();
		    var strLen = Read28Bit();
		    if (strLen < 0)
		    {
			    throw new IOException();
		    }
		    if (strLen == 0)
		    {
			    return string.Empty;
		    }
		    if (_inputCharRawBuf == null)
		    {
			    _inputCharRawBuf = new byte[128];
		    }
		    if (_stringChunk == null)
		    {
			    _stringChunk = new char[_maxCharsIn128Bytes];
		    }
		    StringBuilder stringBuilder = null;
	        while (true)
		    {
			    var needChunkCnt = (strLen - totalRead > 128) ? 128 : (strLen - totalRead);
			    var realChunkRead = _src.Read(_inputCharRawBuf, 0, needChunkCnt);
			    if (realChunkRead == 0)
			    {
				    break;
			    }
			    var chars = _decoder.GetChars(_inputCharRawBuf, 0, realChunkRead, _stringChunk, 0);
			    if (totalRead == 0 && realChunkRead == strLen)
			    {
                    return new string(_stringChunk, 0, chars);
                }
                if (stringBuilder == null)
			    {
				    stringBuilder = new StringBuilder(strLen);
			    }
			    stringBuilder.Append(_stringChunk, 0, chars);
			    totalRead += realChunkRead;
			    if (totalRead >= strLen)
			    {
                    return stringBuilder.ToString();
                }
            }
		    throw new Exception();
	    }

	    // Token: 0x0600004C RID: 76 RVA: 0x00002D08 File Offset: 0x00000F08
	    public int Read(char[] dest, int offset, int cnt) // \u0002
	    {
		    if (dest == null)
		    {
			    throw new ArgumentNullException(StringDecryptor.GetString(-1550347170), /* \u0002 */
                    StringDecryptor.GetString(-1550347157) /* ArgumentNull_Buffer */);
		    }
		    if (offset < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (cnt < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (dest.Length - offset < cnt)
		    {
			    throw new ArgumentException();
		    }
		    SrcPrecondition();
		    return DoRead(dest, offset, cnt);
	    }

	    // Token: 0x0600004D RID: 77 RVA: 0x00002D64 File Offset: 0x00000F64
	    private int DoRead(char[] dest, int offset, int cnt) // \u0003
        {
		    var remainCharsToRead = cnt;
		    if (_inputCharRawBuf == null)
		    {
			    _inputCharRawBuf = new byte[128];
		    }
		    while (remainCharsToRead > 0)
		    {
			    var chunkSize = remainCharsToRead;
			    if (_isUnicode)
			    {
				    chunkSize <<= 1;
			    }
			    if (chunkSize > 128)
			    {
				    chunkSize = 128;
			    }
			    int chars;
			    if (_alwaysTrue)
			    {
				    var byteIndex = _src.GetCursor();
				    chunkSize = _src.SkipBytes(chunkSize);
				    if (chunkSize == 0)
				    {
					    return cnt - remainCharsToRead;
				    }
				    chars = _decoder.GetChars(_src.Data(), byteIndex, chunkSize, dest, offset);
			    }
			    else
			    {
				    chunkSize = _src.Read(_inputCharRawBuf, 0, chunkSize);
				    if (chunkSize == 0)
				    {
					    return cnt - remainCharsToRead;
				    }
				    chars = _decoder.GetChars(_inputCharRawBuf, 0, chunkSize, dest, offset);
			    }
			    remainCharsToRead -= chars;
			    offset += chars;
		    }
		    return cnt;
	    }


	    // Token: 0x0600004F RID: 79 RVA: 0x00002F54 File Offset: 0x00001154
	    public char[] ReadChars(int cnt) // \u0002
	    {
		    SrcPrecondition();
		    if (cnt < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    var ret = new char[cnt];
		    var realLength = DoRead(ret, 0, cnt);
		    if (realLength != cnt)
		    {
			    var shrinked = new char[realLength];
			    Buffer.BlockCopy(ret, 0, shrinked, 0, 2 * realLength);
			    ret = shrinked;
		    }
		    return ret;
	    }

	    // Token: 0x06000050 RID: 80 RVA: 0x00002F9C File Offset: 0x0000119C
	    public int Read(byte[] dest, int offset, int cnt) // dest
	    {
		    if (dest == null)
		    {
			    throw new ArgumentNullException();
		    }
		    if (offset < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (cnt < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (dest.Length - offset < cnt)
		    {
			    throw new ArgumentException();
		    }
		    SrcPrecondition();
		    return _src.Read(dest, offset, cnt);
	    }

	    // Token: 0x06000052 RID: 82 RVA: 0x00002FF8 File Offset: 0x000011F8
	    public byte[] ReadBytes(int cnt) // \u0002
	    {
		    if (cnt < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    SrcPrecondition();
		    var ret = new byte[cnt];
		    var offset = 0;
		    do
		    {
			    var chunkSize = _src.Read(ret, offset, cnt);
			    if (chunkSize == 0)
			    {
				    break;
			    }
			    offset += chunkSize;
			    cnt -= chunkSize;
		    }
		    while (cnt > 0);
		    if (offset != ret.Length)
		    {
			    var shrinked = new byte[offset];
			    Buffer.BlockCopy(ret, 0, shrinked, 0, offset);
			    ret = shrinked;
		    }
		    return ret;
	    }

	    // Token: 0x06000054 RID: 84 RVA: 0x000030C0 File Offset: 0x000012C0
	    internal int Read28Bit() // \u0008
	    {
		    var ret = 0;
		    var bitCnt = 0;
		    while (bitCnt != 35)
		    {
			    var b = ReadByte();
			    ret |= (b & 127) << bitCnt;
			    bitCnt += 7;
			    if ((b & 128) == 0)
			    {
				    return ret;
			    }
		    }
		    throw new FormatException();
	    }

	    // Token: 0x06000055 RID: 85 RVA: 0x00003100 File Offset: 0x00001300
	    public int ReadInt32() // \u0006
        {
		    if (_alwaysTrue)
		    {
			    return _src.ReadInt32();
		    }
		    ReadToRawBuf(4);
		    return _inputRawBuf[3] << 8 | _inputRawBuf[1] << 24 | _inputRawBuf[0] << 16 | _inputRawBuf[2];
	    }
    }
}
