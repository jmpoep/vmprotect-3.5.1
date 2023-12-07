using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000006 RID: 6
    internal static class EmptyArray<T> // \u0002\u2003
    {
	    // Token: 0x04000004 RID: 4
	    public static readonly T[] Data = new T[0];
    }

    // Token: 0x02000022 RID: 34
    internal sealed class MyCollection<T> : IEnumerable<T>, ICollection // \u0005\u2006
    {
        // Token: 0x0400002C RID: 44
        internal T[] Data; // \u0002

        // Token: 0x0400002E RID: 46
        internal int ChangeCounter; // \u0005

	    // Token: 0x0400002F RID: 47
        private object _sync; // \u0008

	    // Token: 0x060000DA RID: 218 RVA: 0x00004B9C File Offset: 0x00002D9C
	    public MyCollection()
	    {
		    Data = EmptyArray<T>.Data;
		    Count = 0;
		    ChangeCounter = 0;
	    }

	    // Token: 0x060000DB RID: 219 RVA: 0x00004BC0 File Offset: 0x00002DC0
	    public MyCollection(int capacity)
	    {
		    if (capacity < 0)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    Data = new T[capacity];
		    Count = 0;
		    ChangeCounter = 0;
	    }

	    // Token: 0x060000DC RID: 220 RVA: 0x00004BEC File Offset: 0x00002DEC
	    public MyCollection(IEnumerable<T> src)
	    {
		    if (src == null)
		    {
			    throw new ArgumentNullException();
		    }
		    var collection = src as ICollection<T>;
		    if (collection != null)
		    {
			    var count = collection.Count;
			    Data = new T[count];
			    collection.CopyTo(Data, 0);
			    Count = count;
			    return;
		    }
		    Count = 0;
		    Data = new T[4];
            foreach (var i in Data)
            {
                PushBack(i);
            }
	    }

        // Token: 0x0400002D RID: 45
        // Token: 0x17000006 RID: 6
        // (get) Token: 0x060000DD RID: 221 RVA: 0x00004C88 File Offset: 0x00002E88
        public int Count { get; private set; } // \u0003

        // Token: 0x060000DE RID: 222 RVA: 0x00004C90 File Offset: 0x00002E90
        bool ICollection.IsSynchronized => false; // \u0005\u2006\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x060000DF RID: 223 RVA: 0x00004C94 File Offset: 0x00002E94
	    object ICollection.SyncRoot // \u0005\u2006\u2008\u2000\u2002\u200A\u0002
        {
	        get
	        {
	            if (_sync == null)
	            {
	                Interlocked.CompareExchange(ref _sync, new object(), null);
	            }
	            return _sync;
	        }
        }

	    // Token: 0x060000E0 RID: 224 RVA: 0x00004CB8 File Offset: 0x00002EB8
	    public void Clear() // \u0002
        {
		    Array.Clear(Data, 0, Count);
		    Count = 0;
		    ChangeCounter++;
	    }

	    // Token: 0x060000E1 RID: 225 RVA: 0x00004CE4 File Offset: 0x00002EE4
	    public bool Contains(T what) // \u0002
        {
		    var num = Count;
		    var @default = EqualityComparer<T>.Default;
		    while (num-- > 0)
		    {
			    if (what == null)
			    {
				    if (Data[num] == null)
				    {
					    return true;
				    }
			    }
			    else if (Data[num] != null && @default.Equals(Data[num], what))
			    {
				    return true;
			    }
		    }
		    return false;
	    }

	    // Token: 0x060000E2 RID: 226 RVA: 0x00004D50 File Offset: 0x00002F50
	    public void CopyTo(T[] dest, int offset) // \u0003
        {
		    if (dest == null)
		    {
			    throw new ArgumentNullException(StringDecryptor.GetString(-1550346880) /* \u0002 */);
		    }
		    if (offset < 0 || offset > dest.Length)
		    {
			    throw new ArgumentOutOfRangeException(StringDecryptor.GetString(-1550346867) /* \u0003 */, 
                    StringDecryptor.GetString(-1550346858) /* arrayIndex < 0 || arrayIndex > array.Length */);
		    }
		    if (dest.Length - offset < Count)
		    {
			    throw new ArgumentException(StringDecryptor.GetString(-1550347192) /* Invalid Off Len */);
		    }
		    Array.Copy(Data, 0, dest, offset, Count);
		    Array.Reverse(dest, offset, Count);
	    }

	    // Token: 0x060000E3 RID: 227 RVA: 0x00004DD4 File Offset: 0x00002FD4
	    void ICollection.CopyTo(Array dest, int offset) // \u0005\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    if (dest == null)
		    {
			    throw new ArgumentNullException();
		    }
		    if (dest.Rank != 1)
		    {
			    throw new ArgumentException();
		    }
		    if (dest.GetLowerBound(0) != 0)
		    {
			    throw new ArgumentException();
		    }
		    if (offset < 0 || offset > dest.Length)
		    {
			    throw new ArgumentOutOfRangeException();
		    }
		    if (dest.Length - offset < Count)
		    {
			    throw new ArgumentException();
		    }
		    try
		    {
			    Array.Copy(Data, 0, dest, offset, Count);
			    Array.Reverse(dest, offset, Count);
		    }
		    catch (ArrayTypeMismatchException)
		    {
			    throw new ArgumentException();
		    }
	    }

	    // Token: 0x060000E4 RID: 228 RVA: 0x00004E6C File Offset: 0x0000306C
	    public MyEnumerator<T> GetEnumerator() // \u0005
        {
		    return new MyEnumerator<T>(this);
	    }

	    // Token: 0x060000E5 RID: 229 RVA: 0x00004E74 File Offset: 0x00003074
	    IEnumerator<T> IEnumerable<T>.GetEnumerator() // \u0005\u2006\u2008\u2000\u2002\u200A\u0008
        {
		    return new MyEnumerator<T>(this);
	    }

	    // Token: 0x060000E6 RID: 230 RVA: 0x00004E84 File Offset: 0x00003084
	    IEnumerator IEnumerable.GetEnumerator() // \u0005\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return new MyEnumerator<T>(this);
	    }

	    // Token: 0x060000E7 RID: 231 RVA: 0x00004E94 File Offset: 0x00003094
	    public void Shrink() // \u0003
        {
		    var num = (int)(Data.Length * 0.9);
		    if (Count < num)
		    {
			    var destinationArray = new T[Count];
			    Array.Copy(Data, 0, destinationArray, 0, Count);
			    Data = destinationArray;
			    ChangeCounter++;
		    }
	    }

	    // Token: 0x060000E8 RID: 232 RVA: 0x00004EF4 File Offset: 0x000030F4
	    public T PeekBack() // \u0006
        {
		    if (Count == 0)
		    {
			    throw new InvalidOperationException();
		    }
		    return Data[Count - 1];
	    }

	    // Token: 0x060000E9 RID: 233 RVA: 0x00004F18 File Offset: 0x00003118
	    public T PopBack() // \u000E
        {
		    if (Count == 0)
		    {
			    throw new InvalidOperationException();
		    }
		    ChangeCounter++;
		    var ret = Data[--Count];
		    Data[Count] = default(T);
		    return ret;
	    }

	    // Token: 0x060000EA RID: 234 RVA: 0x00004F78 File Offset: 0x00003178
	    public void PushBack(T obj) // \u000F
        {
		    if (Count == Data.Length)
		    {
			    var destinationArray = new T[(Data.Length == 0) ? 4 : (2 * Data.Length)];
			    Array.Copy(Data, 0, destinationArray, 0, Count);
			    Data = destinationArray;
		    }
		    var num = Count;
		    Count = num + 1;
		    Data[num] = obj;
		    ChangeCounter++;
	    }

	    // Token: 0x060000EB RID: 235 RVA: 0x00004FF8 File Offset: 0x000031F8
	    public T[] Reverse() // \u0002\u2000
        {
		    var array = new T[Count];
		    for (var i = 0; i < Count; i++)
		    {
			    array[i] = Data[Count - i - 1];
		    }
		    return array;
	    }

	    // Token: 0x02000023 RID: 35
	    public struct MyEnumerator<T1> : IEnumerator<T1> // \u0002
	    {
		    // Token: 0x060000EC RID: 236 RVA: 0x00005040 File Offset: 0x00003240
		    internal MyEnumerator(MyCollection<T1> src)
		    {
			    _src = src;
			    _changeCounter = _src.ChangeCounter;
                _curPos = -2;
			    _current = default(T1);
		    }

		    // Token: 0x060000ED RID: 237 RVA: 0x00005070 File Offset: 0x00003270
		    public void Dispose()
		    {
			    _curPos = -1;
		    }

		    // Token: 0x060000EE RID: 238 RVA: 0x0000507C File Offset: 0x0000327C
		    public bool MoveNext()
		    {
			    if (_changeCounter != _src.ChangeCounter)
			    {
				    throw new InvalidOperationException(StringDecryptor.GetString(-1550346776) /* EnumFailedVersion */);
			    }
			    if (_curPos == -2)
			    {
				    _curPos = _src.Count - 1;
			        if (_curPos < 0) return false;
			        _current = _src.Data[_curPos];
			        return true;
			    }
			    if (_curPos == -1)
			    {
				    return false;
			    }
			    if (--_curPos >= 0)
			    {
				    _current = _src.Data[_curPos];
				    return true;
			    }
			    _current = default(T1);
			    return false;
		    }

		    // Token: 0x17000007 RID: 7
		    // (get) Token: 0x060000EF RID: 239 RVA: 0x00005144 File Offset: 0x00003344
		    public T1 Current
		    {
			    get
			    {
				    if (_curPos == -2)
				    {
					    throw new InvalidOperationException();
				    }
				    if (_curPos == -1)
				    {
					    throw new InvalidOperationException();
				    }
				    return _current;
			    }
		    }

		    // Token: 0x060000F0 RID: 240 RVA: 0x0000516C File Offset: 0x0000336C
		    object IEnumerator.Current // \u0002\u2008\u2000\u2002\u200A\u0002
            {
		        get
		        {
		            if (_curPos == -2)
		            {
		                throw new InvalidOperationException();
		            }
		            if (_curPos == -1)
		            {
		                throw new InvalidOperationException();
		            }
		            return _current;
		        }
            }

		    // Token: 0x060000F1 RID: 241 RVA: 0x00005198 File Offset: 0x00003398
		    void IEnumerator.Reset() // \u0002\u2008\u2000\u2002\u200A\u0002
            {
			    if (_changeCounter != _src.ChangeCounter)
			    {
				    throw new InvalidOperationException();
			    }
			    _curPos = -2;
			    _current = default(T1);
		    }

		    // Token: 0x04000030 RID: 48
		    private readonly MyCollection<T1> _src; // \u0002

		    // Token: 0x04000031 RID: 49
	        private int _curPos; // \u0003

		    // Token: 0x04000032 RID: 50
	        private readonly int _changeCounter; // \u0005

		    // Token: 0x04000033 RID: 51
	        private T1 _current; // \u0008
	    }
    }
}

