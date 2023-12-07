using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000063 RID: 99
    internal static class SdMetadataTokens // \u000F\u2009
    {
	    // Token: 0x06000399 RID: 921 RVA: 0x00015D18 File Offset: 0x00013F18
	    [MethodImpl(MethodImplOptions.NoInlining)]
        internal static long GetLong() // \u0002
        {
		    if (Assembly.GetCallingAssembly() != typeof(SdMetadataTokens).Assembly || !CheckStack())
		    {
			    return 0L;
		    }
            long result;
		    lock (Obj)
		    {
			    var num = Obj.GetLong7();
			    if (num == 0L)
			    {
				    var executingAssembly = Assembly.GetExecutingAssembly();
                    var list = new List<byte>();
                    AssemblyName assemblyName;
				    try
				    {
					    assemblyName = executingAssembly.GetName();
				    }
				    catch
				    {
					    assemblyName = new AssemblyName(executingAssembly.FullName);
				    }
				    var array = assemblyName.GetPublicKeyToken();
				    if (array != null && array.Length == 0)
				    {
					    array = null;
				    }
				    if (array != null)
				    {
					    list.AddRange(array);
				    }
				    list.AddRange(Encoding.Unicode.GetBytes(assemblyName.Name));
			        var num2 = 0x02000063; //GetMdt(typeof(SdMetadataTokens));
				    var num3 = Class1.M();
				    list.Add((byte)(num2 >> 24));
				    list.Add((byte)(num3 >> 16));
				    list.Add((byte)(num2 >> 8));
				    list.Add((byte)num3);
				    list.Add((byte)(num2 >> 16));
				    list.Add((byte)(num3 >> 8));
				    list.Add((byte)num2);
				    list.Add((byte)(num3 >> 24));
				    var count = list.Count;
                    var num4 = 0uL;
				    for (var num5 = 0; num5 != count; num5++)
				    {
					    num4 += list[num5];
					    num4 += num4 << 20;
					    num4 ^= num4 >> 12;
					    list[num5] = 0;
				    }
				    num4 += num4 << 6;
				    num4 ^= num4 >> 22;
				    num4 += num4 << 30;
				    num = (long)num4;
				    num ^= 7895633081549295753L;
				    Obj.SetLong(num);
			    }
			    result = num;
		    }
		    return result;
	    }

	    // Token: 0x0600039A RID: 922 RVA: 0x00015EE0 File Offset: 0x000140E0
	    [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool CheckStack() // \u0002
        {
		    return CheckStackImpl();
	    }

	    // Token: 0x0600039B RID: 923 RVA: 0x00015EEC File Offset: 0x000140EC
	    [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool CheckStackImpl() // \u0003
        {
		    var stackTrace = new StackTrace();
            var frame = stackTrace.GetFrame(3);
            var methodBase = frame?.GetMethod();
            var type = methodBase?.DeclaringType;
		    return type != typeof(RuntimeMethodHandle) && type != null && type.Assembly == typeof(SdMetadataTokens).Assembly;
	    }

	    // Token: 0x0600039C RID: 924 RVA: 0x00015F50 File Offset: 0x00014150
        // ReSharper disable once UnusedMember.Local
	    private static int GetMdt(Type t) // \u0002
        {
		    return t.MetadataToken;
	    }

        // Token: 0x0400019A RID: 410
        // \u0002
        private static readonly Class7 Obj = new Class7();

	    // Token: 0x02000064 RID: 100
        public sealed class Class1 // \u0002\u2007\u2007\u2009\u2002\u2006\u2003\u2003\u2002\u2004\u2007\u200A\u2009\u200A\u2008\u200A\u2000\u2003\u200B\u2007\u200A\u2008\u200A\u2003\u2006\u200B
        {
		    // Token: 0x0600039E RID: 926 RVA: 0x00015F60 File Offset: 0x00014160
		    [MethodImpl(MethodImplOptions.NoInlining)]
		    internal static int M()
		    {
			    return Class2.M3(Class2.M2(0x02000067 /*GetMdt(typeof(Class4))*/, Class2.M3(0x02000064 /*GetMdt(typeof(Class1))*/, 0x02000066 /*GetMdt(typeof(Class3))*/)), Class5.M1());
		    }
	    }

	    // Token: 0x02000065 RID: 101
	    private static class Class2 // \u0002\u200A\u2003\u2000\u2002\u2000\u2007\u2008\u2004\u2006\u2007\u2003\u2007\u2004\u2000\u2003\u2009\u2007\u2003\u2006\u2007\u2008\u200A
        {
		    // Token: 0x0600039F RID: 927 RVA: 0x00015FB0 File Offset: 0x000141B0
		    internal static int M1(int p1, int p2) // \u0002
            {
			    return p1 ^ p2 - -~~- -~~- -~~-1683504797;
		    }

		    // Token: 0x060003A0 RID: 928 RVA: 0x00015FC8 File Offset: 0x000141C8
		    internal static int M2(int p1, int p2) // \u0003
            {
			    return p1 - -~-~-~~- -~~-1670271084 ^ p2 + -~-~-~~-~-~699406271;
		    }

		    // Token: 0x060003A1 RID: 929 RVA: 0x00015FF0 File Offset: 0x000141F0
		    internal static int M3(int p1, int p2) // \u0005
            {
			    return p1 ^ p2 - -~~-~-~- -~~-1466097638 ^ p1 - p2;
		    }
	    }

	    // Token: 0x02000066 RID: 102
        public sealed class Class3 // \u0003\u2001\u2003\u2009\u2009\u2008\u2006\u2006\u2006\u200A\u2003\u2006\u2005\u2005\u2009\u200B\u2009\u200A\u2003\u2007
        {
		    // Token: 0x060003A3 RID: 931 RVA: 0x00016014 File Offset: 0x00014214
		    [MethodImpl(MethodImplOptions.NoInlining)]
		    internal static int M1() // \u0002
            {
			    return Class2.M2(Class2.M2(Class6.M1(), Class2.M1(0x02000066 /*GetMdt(typeof(Class3))*/, Class4.M1())), 0x02000068 /*GetMdt(typeof(Class5))*/);
		    }
	    }

	    // Token: 0x02000067 RID: 103
        public sealed class Class4 // \u0003\u2007\u2006\u2000\u2001\u2003\u2006\u200B\u2003\u2009\u200B\u2008\u200A\u2008\u2004\u2005\u2006\u200A\u2008\u2000\u2000\u200B\u2008\u200A
	    {
		    // Token: 0x060003A5 RID: 933 RVA: 0x00016058 File Offset: 0x00014258
		    [MethodImpl(MethodImplOptions.NoInlining)]
            internal static int M1() // \u0002
            {
                return Class2.M1(0x02000069 /*GetMdt(typeof(Class6))*/, 0x0200006B /*GetMdt(typeof(Class8))*/ ^ Class2.M2(0x02000067 /*GetMdt(typeof(Class4))*/, Class2.M3(0x02000068 /*GetMdt(typeof(Class5))*/, Class8.M1())));
		    }
	    }

	    // Token: 0x02000068 RID: 104
        public sealed class Class5 // \u0005\u2006\u200A\u2004\u200B\u2005\u200B\u2004\u2005\u2002\u2000\u2001\u2002\u2004\u2000\u2002\u2007\u2003\u2009\u200B\u2007\u200A\u200B\u2000\u2008\u2002\u2003\u2002
	    {
		    // Token: 0x060003A7 RID: 935 RVA: 0x000160C0 File Offset: 0x000142C0
		    [MethodImpl(MethodImplOptions.NoInlining)]
            internal static int M1() // \u0002
            {
                return Class2.M1(0x02000068 /*GetMdt(typeof(Class5))*/, Class2.M3(Class2.M2(0x02000066 /*GetMdt(typeof(Class3))*/, 0x02000064 /*GetMdt(typeof(Class1))*/), Class2.M3(0x02000069 /*GetMdt(typeof(Class6))*/ ^ -~-~~- -~~-1251689633, Class3.M1())));
		    }
	    }

	    // Token: 0x02000069 RID: 105
        public sealed class Class6 // \u0008\u2007\u2007\u2004\u2006\u2006\u200A\u2009\u2005\u2006\u2008\u200A\u2000\u200A\u2008\u2002\u2009\u2003\u2006\u2008\u2000\u2005\u2004\u200A\u2004\u2008\u2008\u2001\u2004\u200B
	    {
		    // Token: 0x060003A9 RID: 937 RVA: 0x0001613C File Offset: 0x0001433C
		    [MethodImpl(MethodImplOptions.NoInlining)]
            internal static int M1() // \u0002
            {
                return Class2.M3(Class2.M1(Class4.M1() ^ ~-~- -~~- -~~-527758448, 0x0200006B /*GetMdt(typeof(Class8))*/), Class2.M2(0x02000064 /*GetMdt(typeof(Class1))*/ ^ 0x02000068 /*GetMdt(typeof(Class5))*/, -~~- -~-~~1892236202));
		    }
	    }

	    // Token: 0x0200006A RID: 106
	    internal sealed class Class7 // \u000F\u2005\u2007\u2007\u2009\u2009\u2002\u2004\u2008\u2009\u2002\u2000\u2000\u2009\u2009\u200B\u2008\u2004\u2003\u200B\u200A\u2002\u2002\u2003\u2006\u2007\u2000\u2006\u2002\u2003
        {
		    // Token: 0x060003AA RID: 938 RVA: 0x000161AC File Offset: 0x000143AC
		    internal Class7()
		    {
			    SetLong(0L);
		    }

		    // Token: 0x060003AB RID: 939 RVA: 0x000161BC File Offset: 0x000143BC
		    [MethodImpl(MethodImplOptions.NoInlining)]
		    public long GetLong7() // \u0002
            {
			    if (Assembly.GetCallingAssembly() != typeof(Class7).Assembly)
			    {
				    return 2918384L;
			    }
			    if (!CheckStack())
			    {
				    return 2918384L;
			    }
			    var array = new[]
			    {
				    0,
				    0,
				    0,
				    -~~- -~-~~1688528055
			    };
			    array[1] = ~-~- -~~-~1937298816;
			    array[2] = ~-~- -~-~~-~-2131774929;
			    array[0] = ~-~-~- -~~-~859851913;
			    var num = _i1;
			    var num2 = _i2;
			    var num3 = -~-~-~~-~1640531528;
			    var num4 = -~-~~- -~~-~957401312;
			    for (var num5 = 0; num5 != 32; num5++)
			    {
				    num2 -= (num << 4 ^ num >> 5) + num ^ num4 + array[num4 >> 11 & 3];
				    num4 -= num3;
				    num -= (num2 << 4 ^ num2 >> 5) + num2 ^ num4 + array[num4 & 3];
			    }
			    for (var num6 = 0; num6 != 4; num6++)
			    {
				    array[num6] = 0;
			    }
			    var num7 = ((ulong)num2 << 32);
		        var n = (ulong)_i1;
			    return (long)(num7 | n);
		    }

		    // Token: 0x060003AC RID: 940 RVA: 0x000162D8 File Offset: 0x000144D8
		    [MethodImpl(MethodImplOptions.NoInlining)]
		    internal void SetLong(long p) // \u0002
		    {
			    if (Assembly.GetCallingAssembly() != typeof(Class7).Assembly)
			    {
				    return;
			    }
			    if (!CheckStack())
			    {
				    return;
			    }
			    var array = new int[4];
			    array[1] = -~~-~- -~~-~1937298817;
			    array[0] = -~~-~-~-~859851914;
			    array[2] = ~-~- -~~-~-2131774930;
			    array[3] = -~-~~-~- -~~1688528054;
			    var num = -~-~~- -~-~~1640531529;
			    var num2 = (int)p;
			    var num3 = (int)(p >> 32);
			    var num4 = 0;
			    for (var num5 = 0; num5 != 32; num5++)
			    {
				    num2 += (num3 << 4 ^ num3 >> 5) + num3 ^ num4 + array[num4 & 3];
				    num4 += num;
				    num3 += (num2 << 4 ^ num2 >> 5) + num2 ^ num4 + array[num4 >> 11 & 3];
			    }
			    for (var num6 = 0; num6 != 4; num6++)
			    {
				    array[num6] = 0;
			    }
			    _i1 = num2;
			    _i2 = num3;
		    }

		    // Token: 0x0400019B RID: 411
		    private int _i1; // \u0002

            // Token: 0x0400019C RID: 412
	        private int _i2; // \u0003
	    }

	    // Token: 0x0200006B RID: 107
        public sealed class Class8 // \u000F\u200A\u2002\u2009\u2000\u2009\u2003\u200A\u2005\u2001\u2002\u2002\u2003\u200B\u2000\u2009\u2003\u2009\u2009\u2001\u2002\u200B\u2000\u200A
        {
		    // Token: 0x060003AE RID: 942 RVA: 0x000163DC File Offset: 0x000145DC
		    [MethodImpl(MethodImplOptions.NoInlining)]
		    internal static int M1()
		    {
			    return Class2.M3(0x0200006B /*GetMdt(typeof(Class8))*/, Class2.M1(0x02000064 /*GetMdt(typeof(Class1))*/, Class2.M2(0x02000067 /*GetMdt(typeof(Class4))*/, Class2.M3(0x02000069 /*GetMdt(typeof(Class6))*/, Class2.M1(0x02000066 /*GetMdt(typeof(Class3))*/, 0x02000068 /*GetMdt(typeof(Class5))*/)))));
		    }
	    }
    }
}
