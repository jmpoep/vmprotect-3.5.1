using System;
using System.IO;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000021 RID: 33
    public sealed class VmStreamWrapper : Stream // \u0005\u2005
    {
	    // Token: 0x060000CB RID: 203 RVA: 0x00004A20 File Offset: 0x00002C20
	    public VmStreamWrapper(Stream src, int xorKey)
	    {
            SetStream(src);
            _xorKey = xorKey;
	    }

	    // Token: 0x060000CC RID: 204 RVA: 0x00004A38 File Offset: 0x00002C38
	    public Stream GetStream() // \u0008\u2003\u2003\u2001\u2001\u2000\u2000\u2008\u2000\u2001\u2000\u2008\u200A\u2008\u200B\u2003\u200B
        {
		    return _srcStream;
	    }

	    // Token: 0x060000CD RID: 205 RVA: 0x00004A40 File Offset: 0x00002C40
	    private void SetStream(Stream src) // \u0002
        {
		    _srcStream = src;
	    }

	    // Token: 0x17000001 RID: 1
	    // (get) Token: 0x060000CE RID: 206 RVA: 0x00004A4C File Offset: 0x00002C4C
	    public override bool CanRead => GetStream().CanRead;

        // Token: 0x17000002 RID: 2
	    // (get) Token: 0x060000CF RID: 207 RVA: 0x00004A5C File Offset: 0x00002C5C
	    public override bool CanSeek => GetStream().CanSeek;

        // Token: 0x17000003 RID: 3
	    // (get) Token: 0x060000D0 RID: 208 RVA: 0x00004A6C File Offset: 0x00002C6C
	    public override bool CanWrite => GetStream().CanWrite;

        // Token: 0x060000D1 RID: 209 RVA: 0x00004A7C File Offset: 0x00002C7C
	    public override void Flush()
	    {
		    GetStream().Flush();
	    }

	    // Token: 0x17000004 RID: 4
	    // (get) Token: 0x060000D2 RID: 210 RVA: 0x00004A8C File Offset: 0x00002C8C
	    public override long Length => GetStream().Length;

        // Token: 0x17000005 RID: 5
	    // (get) Token: 0x060000D3 RID: 211 RVA: 0x00004A9C File Offset: 0x00002C9C
	    // (set) Token: 0x060000D4 RID: 212 RVA: 0x00004AAC File Offset: 0x00002CAC
	    public override long Position
	    {
		    get
		    {
			    return GetStream().Position;
		    }
		    set
		    {
			    GetStream().Position = value;
		    }
	    }

	    // Token: 0x060000D5 RID: 213 RVA: 0x00004ABC File Offset: 0x00002CBC
	    private byte Xor(byte data, long key2) // \u0002
	    {
		    var b = (byte)(_xorKey ^ -559030707 ^ (int)((uint)key2));
		    return (byte)(data ^ b);
	    }

	    // Token: 0x060000D6 RID: 214 RVA: 0x00004AE0 File Offset: 0x00002CE0
	    public override void Write(byte[] data, int offset, int size)
	    {
		    var array = new byte[size];
		    Buffer.BlockCopy(data, offset, array, 0, size);
		    var position = Position;
		    for (var i = 0; i < size; i++)
		    {
			    array[i] = Xor(array[i], position + i);
		    }
		    GetStream().Write(array, 0, size);
	    }

	    // Token: 0x060000D7 RID: 215 RVA: 0x00004B30 File Offset: 0x00002D30
	    public override int Read(byte[] data, int offset, int size)
	    {
		    var position = Position;
		    var array = new byte[size];
		    var num = GetStream().Read(array, 0, size);
		    for (var i = 0; i < num; i++)
		    {
			    data[i + offset] = Xor(array[i], position + i);
		    }
		    return num;
	    }

	    // Token: 0x060000D8 RID: 216 RVA: 0x00004B7C File Offset: 0x00002D7C
	    public override long Seek(long pos, SeekOrigin from)
	    {
		    return GetStream().Seek(pos, from);
	    }

	    // Token: 0x060000D9 RID: 217 RVA: 0x00004B8C File Offset: 0x00002D8C
	    public override void SetLength(long val)
	    {
		    GetStream().SetLength(val);
	    }

	    // Token: 0x0400002A RID: 42
        private readonly int _xorKey; // \u0002

	    // Token: 0x0400002B RID: 43
	    private Stream _srcStream; // \u0003
    }
}
