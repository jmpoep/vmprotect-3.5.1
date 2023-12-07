using System;
using System.IO;
using System.Runtime.InteropServices;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000024 RID: 36
    internal sealed class MyBuffer : IDisposable // \u0005\u2007
    {
	    // Token: 0x060000F2 RID: 242 RVA: 0x000051C8 File begin: 0x000033C8
	    public MyBuffer() : this(0)
	    {
	    }

	    // Token: 0x060000F3 RID: 243 RVA: 0x000051D4 File begin: 0x000033D4
	    public MyBuffer(int sz)
	    {
		    if (sz < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    _data = new byte[sz];
		    _size = sz;
		    _internal = true;
		    _writable = true;
		    _begin = 0;
		    _valid = true;
	    }

	    // Token: 0x060000F4 RID: 244 RVA: 0x00005220 File begin: 0x00003420
	    public MyBuffer(byte[] src) : this(src, true)
	    {
	    }

	    // Token: 0x060000F5 RID: 245 RVA: 0x0000522C File begin: 0x0000342C
	    public MyBuffer(byte[] src, bool writable)
	    {
		    if (src == null)
		    {
			    throw new ArgumentNullException();
		    }
		    _data = src;
		    _end = (_size = src.Length);
		    _writable = writable;
		    _begin = 0;
		    _valid = true;
	    }

	    // Token: 0x060000F6 RID: 246 RVA: 0x00005278 File begin: 0x00003478
	    public MyBuffer(byte[] src, int begin, int count) : this(src, begin, count, true)
	    {
	    }

	    // Token: 0x060000F7 RID: 247 RVA: 0x00005284 File begin: 0x00003484
	    public MyBuffer(byte[] src, int begin, int count, bool writable)
	    {
		    if (src == null)
		    {
			    throw new ArgumentNullException();
		    }
		    if (begin < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (count < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (src.Length - begin < count)
		    {
			    throw new ArgumentException();
		    }
		    _data = src;
		    _cursor = begin;
		    _begin = begin;
		    _end = (_size = begin + count);
		    _writable = writable;
		    _internal = false;
		    _valid = true;
	    }

	    // Token: 0x060000F8 RID: 248 RVA: 0x00005304 File begin: 0x00003504
	    public bool IsValid() // \u0002
	    {
		    return _valid;
	    }

	    // Token: 0x060000F9 RID: 249 RVA: 0x0000530C File begin: 0x0000350C
	    public bool IsValid2() // \u0003
        {
		    return _valid;
	    }

	    // Token: 0x060000FA RID: 250 RVA: 0x00005314 File begin: 0x00003514
	    public bool IsWritable() // \u0005
        {
		    return _writable;
	    }

        // Token: 0x060000FB RID: 251 RVA: 0x0000531C File begin: 0x0000351C
        public void DoDispose() // \u0002
        {
		    Dispose(true);
            // ReSharper disable once GCSuppressFinalizeForTypeWithoutDestructor
		    GC.SuppressFinalize(this);
	    }

        // Token: 0x060000FC RID: 252 RVA: 0x0000532C File begin: 0x0000352C
        public void Dispose()
	    {
            Dispose(false);
	    }

	    // Token: 0x060000FD RID: 253 RVA: 0x00005334 File begin: 0x00003534
	    private void Dispose(bool disposing) // \u0002
        {
		    if (!_disposed)
		    {
			    if (disposing)
			    {
				    _valid = false;
				    _writable = false;
				    _internal = false;
			    }
			    _disposed = true;
		    }
	    }

	    // Token: 0x060000FE RID: 254 RVA: 0x00005360 File begin: 0x00003560
	    private bool Expand(int newSize) // \u0002
	    {
		    if (newSize < 0)
		    {
			    throw new IOException();
		    }
		    if (newSize > _size)
		    {
			    if (newSize < 256)
			    {
                    newSize = 256;
			    }
			    if (newSize < _size * 2)
			    {
                    newSize = _size * 2;
			    }
                ExpandExact(newSize);
			    return true;
		    }
		    return false;
	    }

	    // Token: 0x060000FF RID: 255 RVA: 0x000053B0 File begin: 0x000035B0
	    public void DoNothing() // \u0003
        {}

	    // Token: 0x06000100 RID: 256 RVA: 0x000053B4 File begin: 0x000035B4
	    internal byte[] Data() // \u0002
        {
		    return _data;
	    }

	    // Token: 0x06000101 RID: 257 RVA: 0x000053BC File begin: 0x000035BC
	    internal void GetRanges(out int begin, out int end) // \u0002
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    begin = _begin;
		    end = _end;
	    }

	    // Token: 0x06000102 RID: 258 RVA: 0x000053DC File begin: 0x000035DC
	    internal int GetCursor() // \u0002
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    return _cursor;
	    }

	    // Token: 0x06000103 RID: 259 RVA: 0x000053F4 File begin: 0x000035F4
	    public int SkipBytes(int cnt) // \u0002
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    var num = _end - _cursor;
		    if (num > cnt)
		    {
			    num = cnt;
		    }
		    if (num < 0)
		    {
			    num = 0;
		    }
		    _cursor += num;
		    return num;
	    }

	    // Token: 0x06000104 RID: 260 RVA: 0x00005438 File begin: 0x00003638
	    public int Capacity() // \u0003
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    return _size - _begin;
	    }

	    // Token: 0x06000105 RID: 261 RVA: 0x00005458 File begin: 0x00003658
	    public void ExpandExact(int newSize) // \u0002
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (newSize != _size)
		    {
			    if (!_internal)
			    {
				    throw new Exception();
			    }
			    if (newSize < _end)
			    {
				    throw new ArgumentOutOfRangeException();
			    }
			    if (newSize > 0)
			    {
				    var dst = new byte[newSize];
				    if (_end > 0)
				    {
					    Buffer.BlockCopy(_data, 0, dst, 0, _end);
				    }
				    _data = dst;
			    }
			    else
			    {
				    _data = null;
			    }
			    _size = newSize;
		    }
	    }

	    // Token: 0x06000106 RID: 262 RVA: 0x000054D8 File begin: 0x000036D8
	    public long UsedSize() // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    return _end - _begin;
	    }

	    // Token: 0x06000107 RID: 263 RVA: 0x000054F8 File begin: 0x000036F8
	    public long GetPos() // \u0003
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
	        return _cursor - _begin;
        }

	    // Token: 0x06000108 RID: 264 RVA: 0x00005518 File begin: 0x00003718
	    public void SetPos(long newPos) // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (newPos < 0L)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (newPos > 2147483647L)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    _cursor = _begin + (int)newPos;
	    }

	    // Token: 0x06000109 RID: 265 RVA: 0x00005554 File begin: 0x00003754
	    public int Read([In] [Out] byte[] dest, int offset, int cnt) // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
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
		    var num = _end - _cursor;
		    if (num > cnt)
		    {
			    num = cnt;
		    }
		    if (num <= 0)
		    {
			    return 0;
		    }
		    if (num <= 8)
		    {
			    var num2 = num;
			    while (--num2 >= 0)
			    {
				    dest[offset + num2] = _data[_cursor + num2];
			    }
		    }
		    else
		    {
			    Buffer.BlockCopy(_data, _cursor, dest, offset, num);
		    }
		    _cursor += num;
		    return num;
	    }

	    // Token: 0x0600010A RID: 266 RVA: 0x00005600 File begin: 0x00003800
	    public int ReadByte() // \u0005
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (_cursor >= _end)
		    {
			    return -1;
		    }
		    var num = _cursor;
		    _cursor = num + 1;
		    return _data[num];
	    }

	    // Token: 0x0600010B RID: 267 RVA: 0x00005644 File begin: 0x00003844
	    public long Seek(long distance, SeekOrigin org) // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (distance > 2147483647L)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    switch (org)
		    {
		    case SeekOrigin.Begin:
			    if (distance < 0L)
			    {
				    throw new IOException();
			    }
			    _cursor = _begin + (int)distance;
			    break;
		    case SeekOrigin.Current:
			    if (distance + _cursor < _begin)
			    {
				    throw new IOException();
			    }
			    _cursor += (int)distance;
			    break;
		    case SeekOrigin.End:
			    if (_end + distance < _begin)
			    {
				    throw new IOException();
			    }
			    _cursor = _end + (int)distance;
			    break;
		    default:
			    throw new ArgumentException();
		    }
		    return _cursor;
	    }

	    // Token: 0x0600010C RID: 268 RVA: 0x00005700 File begin: 0x00003900
	    public void LazyShrink(long newCount) // \u0003
{
		    if (!_writable)
		    {
			    throw new Exception();
		    }
		    if (newCount > 2147483647L)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (newCount < 0L || newCount > 2147483647 - _begin)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    var num = _begin + (int)newCount;
		    if (!Expand(num) && num > _end)
		    {
			    Array.Clear(_data, _end, num - _end);
		    }
		    _end = num;
		    if (_cursor > num)
		    {
			    _cursor = num;
		    }
	    }

	    // Token: 0x0600010D RID: 269 RVA: 0x00005794 File begin: 0x00003994
	    public byte[] ToArray() // \u0003
        {
		    var array = new byte[_end - _begin];
		    Buffer.BlockCopy(_data, _begin, array, 0, _end - _begin);
		    return array;
	    }

	    // Token: 0x0600010E RID: 270 RVA: 0x000057D8 File begin: 0x000039D8
	    public void Write(byte[] src, int offset, int cnt) // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (!_writable)
		    {
			    throw new Exception();
		    }
		    if (src == null)
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
		    if (src.Length - offset < cnt)
		    {
			    throw new ArgumentException();
		    }
		    var num = _cursor + cnt;
		    if (num < 0)
		    {
			    throw new IOException();
		    }
		    if (num > _end)
		    {
			    var flag = _cursor > _end;
			    if (num > _size && Expand(num))
			    {
				    flag = false;
			    }
			    if (flag)
			    {
				    Array.Clear(_data, _end, num - _end);
			    }
			    _end = num;
		    }
		    if (cnt <= 8)
		    {
			    while (--cnt >= 0)
			    {
				    _data[_cursor + cnt] = src[offset + cnt];
			    }
		    }
		    else
		    {
			    Buffer.BlockCopy(src, offset, _data, _cursor, cnt);
		    }
		    _cursor = num;
	    }

	    // Token: 0x0600010F RID: 271 RVA: 0x000058D0 File begin: 0x00003AD0
	    public void AppendByte(byte b) // \u0002
        {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (!_writable)
		    {
			    throw new Exception();
		    }
		    if (_cursor >= _end)
		    {
			    var num = _cursor + 1;
			    var flag = _cursor > _end;
			    if (num >= _size && Expand(num))
			    {
				    flag = false;
			    }
			    if (flag)
			    {
				    Array.Clear(_data, _end, _cursor - _end);
			    }
			    _end = num;
		    }
		    _data[_cursor] = b;
	        _cursor++;
        }

	    // Token: 0x06000110 RID: 272 RVA: 0x00005974 File begin: 0x00003B74
	    public void WriteTo(Stream s) // \u0002
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    if (s == null)
		    {
			    throw new ArgumentNullException();
		    }
		    s.Write(_data, _begin, _end - _begin);
	    }

	    // Token: 0x06000111 RID: 273 RVA: 0x000059AC File begin: 0x00003BAC
	    internal int ReadInt32() // \u0008
	    {
		    if (!_valid)
		    {
			    throw new Exception();
		    }
		    var num = _cursor += 4;
		    if (num > _end)
		    {
			    _cursor = _end;
			    throw new Exception();
		    }
		    return _data[num - 2] | _data[num - 3] << 24 | _data[num - 1] << 8 | _data[num - 4] << 16;
	    }

	    // Token: 0x04000034 RID: 52
        private byte[] _data; // \u0002

	    // Token: 0x04000035 RID: 53
        private readonly int _begin; // \u0003

	    // Token: 0x04000036 RID: 54
        private int _cursor; // \u0005

	    // Token: 0x04000037 RID: 55
        private int _end; // \u0008

	    // Token: 0x04000038 RID: 56
        private int _size; // \u0006

	    // Token: 0x04000039 RID: 57
	    private bool _internal; // \u000E

	    // Token: 0x0400003A RID: 58
	    private bool _writable; // \u000F

        // Token: 0x0400003B RID: 59
        private bool _valid; // \u0002\u2000

	    // Token: 0x0400003C RID: 60
	    private bool _disposed; // \u0003\u2000
    }
}
