using System;
using System.IO;

namespace UnitTestProject.RefVm
{
    internal static class VmPosParser // \u0006\u2006
    {
	    // Token: 0x0600016A RID: 362 RVA: 0x00007810 File Offset: 0x00005A10
	    public static byte[] Parse(string pos) // dest
        {
		    if (pos == null)
		    {
			    throw new Exception();
            }
            var memoryStream = new MemoryStream(pos.Length * 4 / 5);
            byte[] result;
		    using(memoryStream)
		    {
			    var cnt = 0;
                var val = 0u;
			    foreach (var c in pos)
			    {
			        if (c == 'z' && cnt == 0)
			        {
			            WriteDwPart(memoryStream, val, 0);
			        }
			        else
			        {
			            if (c< '!' || c> 'u')
			            {
			                throw new Exception();
			            }
			            checked
			            {
			                val += (uint)(Arr[cnt] * (ulong)checked(c - '!'));
			            }
			            cnt++;
			            if (cnt == 5)
			            {
			                WriteDwPart(memoryStream, val, 0);
			                cnt = 0;
			                val = 0u;
			            }
			        }
			    }
			    if (cnt == 1)
			    {
				    throw new Exception();
			    }
			    if (cnt > 1)
			    {
				    for (var j = cnt; j < 5; j++)
				    {
					    checked
					    {
						    val += 84u * Arr[j];
					    }
				    }
				    WriteDwPart(memoryStream, val, 5 - cnt);
			    }
			    result = memoryStream.ToArray();
		    }
		    return result;
	    }

	    // Token: 0x0600016B RID: 363 RVA: 0x00007900 File Offset: 0x00005B00
	    private static void WriteDwPart(Stream dest, uint val, int ignoreLow) // dest
        {
		    dest.WriteByte((byte)(val >> 24));
		    if (ignoreLow == 3)
		    {
			    return;
		    }
		    dest.WriteByte((byte)(val >> 16 & 255u));
		    if (ignoreLow == 2)
		    {
			    return;
		    }
		    dest.WriteByte((byte)(val >> 8 & 255u));
		    if (ignoreLow == 1)
		    {
			    return;
		    }
		    dest.WriteByte((byte)(val & 255u));
	    }

	    // Token: 0x04000131 RID: 305
	    private static readonly uint[] Arr = {
		    52200625u,
		    614125u,
		    7225u,
		    85u,
		    1u
	    };
    }
}
