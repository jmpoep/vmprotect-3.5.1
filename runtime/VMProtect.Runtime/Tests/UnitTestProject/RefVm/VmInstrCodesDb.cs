using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Threading;

// ReSharper disable UnusedMember.Global

namespace UnitTestProject.RefVm
{
    // Token: 0x0200002E RID: 46
    [SuppressMessage("ReSharper", "InconsistentNaming")]
    public class VmInstrCodesDb // \u0006\u2005
    {
        // Token: 0x04000068 RID: 104
        private VmInstrInfo[] _all;

        // Token: 0x0400011D RID: 285
        private bool _initialized;

	    // Token: 0x0600015B RID: 347 RVA: 0x000075B8 File Offset: 0x000057B8
	    public bool IsInitialized() // \u0002
        {
		    return _initialized;
	    }

	    // Token: 0x0600015C RID: 348 RVA: 0x000075C0 File Offset: 0x000057C0
	    public void SetInitialized(bool val) // \u0002
	    {
            _initialized = val;
	    }

	    // Token: 0x0600015A RID: 346 RVA: 0x000075A8 File Offset: 0x000057A8
	    public IEnumerable<VmInstrInfo> MyFieldsEnumerator() // \u0002
        {
		    return new FieldsEnumerator(-2)
		    {
			    Source = this
		    };
	    }

	    // Token: 0x0600015D RID: 349 RVA: 0x000075CC File Offset: 0x000057CC
	    public VmInstrInfo[] ToSortedArray() // \u0002
	    {
		    if (_all == null)
		    {
			    lock (this)
			    {
				    if (_all == null)
				    {
					    var list = new List<VmInstrInfo>(256);
				        list.AddRange(MyFieldsEnumerator());
				        if (SortHelper.MyComparison == null)
					    {
                            SortHelper.MyComparison = SortHelper.MySortHelper.Compare;
					    }
					    list.Sort(SortHelper.MyComparison);
					    _all = list.ToArray();
				    }
			    }
		    }
		    return _all;
	    }

	    // Token: 0x0200002F RID: 47
	    [Serializable]
	    private sealed class SortHelper // \u0002
        {
		    // Token: 0x06000160 RID: 352 RVA: 0x000076A0 File Offset: 0x000058A0
		    internal int Compare(VmInstrInfo v1, VmInstrInfo v2) // \u0002
		    {
			    return v1.Id.CompareTo(v2.Id);
		    }

		    // Token: 0x04000129 RID: 297
		    public static readonly SortHelper MySortHelper = new SortHelper(); // \u0002

            // Token: 0x0400012A RID: 298
            public static Comparison<VmInstrInfo> MyComparison; // \u0003
	    }

	    // Token: 0x02000030 RID: 48
	    private sealed class FieldsEnumerator : IEnumerable<VmInstrInfo>, IEnumerator<VmInstrInfo> // \u0003
	    {
		    // Token: 0x06000161 RID: 353 RVA: 0x000076C4 File Offset: 0x000058C4
		    public FieldsEnumerator(int st)
		    {
			    _state = st;
			    _threadId = Thread.CurrentThread.ManagedThreadId;
		    }

		    // Token: 0x06000162 RID: 354 RVA: 0x000076E4 File Offset: 0x000058E4
		    void IDisposable.Dispose() // \u0003\u2008\u2000\u2002\u200A\u0002
            {
		    }

		    // Token: 0x06000163 RID: 355 RVA: 0x000076E8 File Offset: 0x000058E8
		    bool IEnumerator.MoveNext()
		    {
			    var num = _state;
			    if (num != 0)
			    {
				    if (num != 1)
				    {
					    return false;
				    }
				    _state = -1;
				    _index++;
			    }
			    else
			    {
				    _state = -1;
				    var fields = typeof(VmInstrCodesDb).GetFields(BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public);
				    _data = fields;
				    _index = 0;
			    }
			    if (_index >= _data.Length)
			    {
				    _data = null;
				    return false;
			    }
			    var p = (VmInstrInfo)_data[_index].GetValue(Source);
			    _current = p;
			    _state = 1;
			    return true;
		    }

		    // Token: 0x06000164 RID: 356 RVA: 0x00007790 File Offset: 0x00005990
		    VmInstrInfo IEnumerator<VmInstrInfo>.Current // \u0003\u2008\u2000\u2002\u200A\u0002
		        => _current;

	        // Token: 0x06000165 RID: 357 RVA: 0x00007798 File Offset: 0x00005998
		    [DebuggerHidden]
		    void IEnumerator.Reset() // \u0003\u2008\u2000\u2002\u200A\u0003
            {
			    throw new NotSupportedException();
		    }

		    // Token: 0x06000166 RID: 358 RVA: 0x000077A0 File Offset: 0x000059A0
		    [DebuggerHidden]
		    object IEnumerator.Current // \u0003\u2008\u2000\u2002\u200A\u0002
                => _current;

		    // Token: 0x06000167 RID: 359 RVA: 0x000077A8 File Offset: 0x000059A8
		    [DebuggerHidden]
		    IEnumerator<VmInstrInfo> IEnumerable<VmInstrInfo>.GetEnumerator() // \u0003\u2008\u2000\u2002\u200A\u0002
            {
                FieldsEnumerator ret;
			    if (_state == -2 && _threadId == Thread.CurrentThread.ManagedThreadId)
			    {
				    _state = 0;
				    ret = this;
			    }
			    else
			    {
			        ret = new FieldsEnumerator(0) {Source = Source};
			    }
			    return ret;
		    }

		    // Token: 0x06000168 RID: 360 RVA: 0x000077F0 File Offset: 0x000059F0
		    [DebuggerHidden]
		    IEnumerator IEnumerable.GetEnumerator() // \u0003\u2008\u2000\u2002\u200A\u0002
            {
			    return ((IEnumerable<VmInstrInfo>)this).GetEnumerator();
		    }

		    // Token: 0x0400012B RID: 299
	        private int _state; // \u0002

		    // Token: 0x0400012C RID: 300
	        private VmInstrInfo _current; // \u0003

		    // Token: 0x0400012D RID: 301
	        private readonly int _threadId; // \u0005

		    // Token: 0x0400012E RID: 302
	        public VmInstrCodesDb Source; // \u0008

		    // Token: 0x0400012F RID: 303
		    private FieldInfo[] _data; // \u0006

            // Token: 0x04000130 RID: 304
            private int _index; // \u000E
	    }

        #region all
        /*
no. { typecheck, rangecheck, nullcheck } The specified fault check(s) normally performed as part of the execution of the subsequent instruction can/shall be skipped.	Prefix to instruction
readonly.	Specify that the subsequent array address operation performs no type check at runtime, and that it returns a controlled-mutability managed pointer	Prefix to instruction         
tail.	Subsequent call terminates current method	Prefix to instruction
unaligned. (alignment)	Subsequent pointer instruction might be unaligned.	Prefix to instruction
volatile.	Subsequent pointer reference is volatile.	Prefix to instruction
             */
        // Token: 0x04000063 RID: 99
        public readonly VmInstrInfo U0002U2000 = new VmInstrInfo(690984147, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x04000077 RID: 119
        public readonly VmInstrInfo U0006U2001U2000 = new VmInstrInfo(733028785, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x0400007A RID: 122
        public readonly VmInstrInfo U0006U2001 = new VmInstrInfo(701247957, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x04000086 RID: 134
        public readonly VmInstrInfo U0006U2004 = new VmInstrInfo(-377358754, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x040000B0 RID: 176
        public readonly VmInstrInfo U0006U2008U2000 = new VmInstrInfo(1026942272, VmOperandType.Ot11Nope); // empty impl

        // Token: 0x040000B2 RID: 178
        public readonly VmInstrInfo Endfilter_ = new VmInstrInfo(-1041717787, VmOperandType.Ot11Nope);

        // Token: 0x040000C6 RID: 198
        public readonly VmInstrInfo Mul_ovf_ = new VmInstrInfo(717697778, VmOperandType.Ot11Nope);

        // Token: 0x040000CB RID: 203
        public readonly VmInstrInfo Endfinally_ = new VmInstrInfo(-860175516, VmOperandType.Ot11Nope);

        // Token: 0x040000E0 RID: 224
        public readonly VmInstrInfo U0003U2009 = new VmInstrInfo(-1535884281, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x04000080 RID: 128
        public readonly VmInstrInfo U000EU2006U2000 = new VmInstrInfo(1756761351, VmOperandType.Ot5Int); // ??? invoke some method

        // Token: 0x04000082 RID: 130
        public readonly VmInstrInfo Jmp_ = new VmInstrInfo(-512817309, VmOperandType.Ot12Int);

        // Token: 0x040000CC RID: 204
        public readonly VmInstrInfo Initobj_ = new VmInstrInfo(-647649665, VmOperandType.Ot5Int);

        // Token: 0x04000065 RID: 101
        public readonly VmInstrInfo Calli_ = new VmInstrInfo(1295283437, VmOperandType.Ot5Int);

        // Token: 0x0400006F RID: 111
        public readonly VmInstrInfo Constrained_ = new VmInstrInfo(1803463719, VmOperandType.Ot5Int);

        // Token: 0x04000098 RID: 152
        public readonly VmInstrInfo U000FU2001 = new VmInstrInfo(-1952417400, VmOperandType.Ot5Int); // empty impl

        // Token: 0x040000E1 RID: 225
        public readonly VmInstrInfo Box_ = new VmInstrInfo(1491096114, VmOperandType.Ot5Int);

        // Token: 0x040000E4 RID: 228
        public readonly VmInstrInfo U0006U200BU2000 = new VmInstrInfo(-1858492701, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x040000ED RID: 237
        public readonly VmInstrInfo U0002U2002U2001 = new VmInstrInfo(113196648, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x040000EF RID: 239
        public readonly VmInstrInfo Ldobj_ = new VmInstrInfo(-564585233, VmOperandType.Ot5Int);

        // Token: 0x04000104 RID: 260
        public readonly VmInstrInfo Rethrow_ = new VmInstrInfo(989001448, VmOperandType.Ot11Nope);

        // Token: 0x04000125 RID: 293
        public readonly VmInstrInfo U000EU2000U2000 = new VmInstrInfo(814546329, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x040000B8 RID: 184
        public readonly VmInstrInfo Newobj_ = new VmInstrInfo(783240206, VmOperandType.Ot5Int);

        // Token: 0x040000B9 RID: 185
        public readonly VmInstrInfo U0006U2000U2000 = new VmInstrInfo(569679686, VmOperandType.Ot11Nope); // not implemented

        // Token: 0x040000BD RID: 189
        public readonly VmInstrInfo U0002U200B = new VmInstrInfo(578506051, VmOperandType.Ot6SByte); // leave.s not implemented

        // Token: 0x040000BE RID: 190
        public readonly VmInstrInfo Leave_ = new VmInstrInfo(182069479, VmOperandType.Ot0UInt);

        // Token: 0x040000B3 RID: 179
        //public readonly VmInstrInfo U0003U2008 = new VmInstrInfo(56035065, VmOperandType.Nope11);

        // Token: 0x04000052 RID: 82
        public readonly VmInstrInfo Ldnull_ = new VmInstrInfo(1672432034, VmOperandType.Ot11Nope);

        // Token: 0x04000057 RID: 87
        public readonly VmInstrInfo Pop_ = new VmInstrInfo(-423590017, VmOperandType.Ot11Nope);

        // Token: 0x0400005B RID: 91
        public readonly VmInstrInfo Ckfinite_ = new VmInstrInfo(-624596400, VmOperandType.Ot11Nope);

        // Token: 0x0400005D RID: 93
        public readonly VmInstrInfo Stind_i2_ = new VmInstrInfo(81292670, VmOperandType.Ot11Nope);

        // Token: 0x0400006C RID: 108
        public readonly VmInstrInfo Stind_r8_ = new VmInstrInfo(-239256528, VmOperandType.Ot11Nope);

        // Token: 0x04000074 RID: 116
        public readonly VmInstrInfo Ldftn_ = new VmInstrInfo(-204727488, VmOperandType.Ot5Int);

        // Token: 0x04000076 RID: 118
        public readonly VmInstrInfo Ldlen_ = new VmInstrInfo(-1528794514, VmOperandType.Ot11Nope);

        // Token: 0x04000079 RID: 121
        public readonly VmInstrInfo Isinst_ = new VmInstrInfo(-1879745689, VmOperandType.Ot5Int);

        // Token: 0x04000081 RID: 129
        public readonly VmInstrInfo Stind_i8_ = new VmInstrInfo(-279385973, VmOperandType.Ot11Nope);

        // Token: 0x04000087 RID: 135
        public readonly VmInstrInfo Newarr_ = new VmInstrInfo(1211659810, VmOperandType.Ot5Int);

        // Token: 0x04000089 RID: 137
        public readonly VmInstrInfo Callvirt_ = new VmInstrInfo(497685394, VmOperandType.Ot5Int);

        // Token: 0x0400008A RID: 138
        public readonly VmInstrInfo Ldc_i8_ = new VmInstrInfo(598097099, VmOperandType.Ot7Long);

        // Token: 0x0400009A RID: 154
        public readonly VmInstrInfo Castclass_ = new VmInstrInfo(1816382558, VmOperandType.Ot5Int);

        // Token: 0x0400009C RID: 156
        public readonly VmInstrInfo Stind_i_ = new VmInstrInfo(-774914583, VmOperandType.Ot11Nope);

        // Token: 0x0400009D RID: 157
        public readonly VmInstrInfo Ldc_i4_s_ = new VmInstrInfo(1440000960, VmOperandType.Ot6SByte);

        // Token: 0x040000A7 RID: 167
        public readonly VmInstrInfo Not_ = new VmInstrInfo(2044815068, VmOperandType.Ot11Nope);

        // Token: 0x040000A8 RID: 168
        public readonly VmInstrInfo Ldtoken_ = new VmInstrInfo(757747961, VmOperandType.Ot5Int);

        // Token: 0x040000AD RID: 173
        public readonly VmInstrInfo Stind_i4_ = new VmInstrInfo(-587303415, VmOperandType.Ot11Nope);

        // Token: 0x040000B6 RID: 182
        public readonly VmInstrInfo Ldvirtftn_ = new VmInstrInfo(-1088007919, VmOperandType.Ot5Int);

        // Token: 0x040000BA RID: 186
        public readonly VmInstrInfo Stind_i1_ = new VmInstrInfo(122987244, VmOperandType.Ot11Nope);

        // Token: 0x040000BC RID: 188
        public readonly VmInstrInfo Cgt_ = new VmInstrInfo(-290816002, VmOperandType.Ot11Nope);

        // Token: 0x040000C4 RID: 196
        public readonly VmInstrInfo Stobj_ = new VmInstrInfo(-18831398, VmOperandType.Ot5Int);

        // Token: 0x040000C5 RID: 197
        public readonly VmInstrInfo Clt_un_ = new VmInstrInfo(-377042092, VmOperandType.Ot11Nope);

        // Token: 0x040000DD RID: 221
        public readonly VmInstrInfo Cgt_un_ = new VmInstrInfo(-244421767, VmOperandType.Ot11Nope);

        // Token: 0x040000D6 RID: 214
        public readonly VmInstrInfo Stind_ref_ = new VmInstrInfo(-572078212, VmOperandType.Ot11Nope);

        // Token: 0x040000DF RID: 223
        public readonly VmInstrInfo Ldloca_ = new VmInstrInfo(-1112986259, VmOperandType.Ot1UShort);

        // Token: 0x040000E9 RID: 233
        public readonly VmInstrInfo Call_ = new VmInstrInfo(-1118186024, VmOperandType.Ot5Int);

        // Token: 0x040000F9 RID: 249
        public readonly VmInstrInfo Ldc_r8_ = new VmInstrInfo(-557730397, VmOperandType.Ot4Double);

        // Token: 0x040000FD RID: 253
        public readonly VmInstrInfo Clt_ = new VmInstrInfo(-1789431058, VmOperandType.Ot11Nope);

        // Token: 0x04000107 RID: 263
        public readonly VmInstrInfo Ldc_i4_ = new VmInstrInfo(-763377227, VmOperandType.Ot12Int);

        // Token: 0x04000112 RID: 274
        public readonly VmInstrInfo Ldc_r4_ = new VmInstrInfo(-976252990, VmOperandType.Ot10Float);

        // Token: 0x04000116 RID: 278
        public readonly VmInstrInfo Stind_r4_ = new VmInstrInfo(2036802079, VmOperandType.Ot11Nope);

        // Token: 0x04000119 RID: 281
        public readonly VmInstrInfo Nop_ = new VmInstrInfo(-724560934, VmOperandType.Ot11Nope);

        // Token: 0x0400011E RID: 286
        public readonly VmInstrInfo Ldloca_s_ = new VmInstrInfo(1851592203, VmOperandType.Ot8Byte);

        // Token: 0x04000053 RID: 83
        public readonly VmInstrInfo Sizeof_ = new VmInstrInfo(-1163259743, VmOperandType.Ot5Int);

        // Token: 0x04000054 RID: 84
        public readonly VmInstrInfo Ldind_r4_ = new VmInstrInfo(1144322863, VmOperandType.Ot11Nope);

        // Token: 0x04000055 RID: 85
        public readonly VmInstrInfo Ldelem_i1_ = new VmInstrInfo(322204500, VmOperandType.Ot11Nope);

        // Token: 0x04000056 RID: 86
        public readonly VmInstrInfo Conv_r8_ = new VmInstrInfo(-195608730, VmOperandType.Ot11Nope);

        // Token: 0x04000058 RID: 88
        public readonly VmInstrInfo Stelem_i1_ = new VmInstrInfo(-1560659480, VmOperandType.Ot11Nope);

        // Token: 0x04000059 RID: 89
        public readonly VmInstrInfo Ldstr_ = new VmInstrInfo(-883753595, VmOperandType.Ot5Int);

        // Token: 0x0400005A RID: 90
        public readonly VmInstrInfo Conv_i4_ = new VmInstrInfo(1738936149, VmOperandType.Ot11Nope);

        // Token: 0x0400005C RID: 92
        public readonly VmInstrInfo Ldarg_2_ = new VmInstrInfo(917707539, VmOperandType.Ot11Nope);

        // Token: 0x0400005E RID: 94
        public readonly VmInstrInfo Conv_i1_ = new VmInstrInfo(443736782, VmOperandType.Ot11Nope);

        // Token: 0x0400005F RID: 95
        public readonly VmInstrInfo Div_ = new VmInstrInfo(873071583, VmOperandType.Ot11Nope);

        // Token: 0x04000060 RID: 96
        public readonly VmInstrInfo Conv_i_ = new VmInstrInfo(863451657, VmOperandType.Ot11Nope);

        // Token: 0x04000061 RID: 97
        public readonly VmInstrInfo Stelem_ref_ = new VmInstrInfo(1243606418, VmOperandType.Ot11Nope);

        // Token: 0x04000062 RID: 98
        public readonly VmInstrInfo Shl_ = new VmInstrInfo(1269228253, VmOperandType.Ot11Nope);

        // Token: 0x04000064 RID: 100
        public readonly VmInstrInfo Conv_u4_ = new VmInstrInfo(-1046006878, VmOperandType.Ot11Nope);

        // Token: 0x04000066 RID: 102
        public readonly VmInstrInfo Break_ = new VmInstrInfo(-979485219, VmOperandType.Ot11Nope);

        // Token: 0x04000067 RID: 103
        public readonly VmInstrInfo Ldc_i4_1_ = new VmInstrInfo(-2108713475, VmOperandType.Ot11Nope);

        // Token: 0x04000069 RID: 105
        public readonly VmInstrInfo Or_ = new VmInstrInfo(1569462844, VmOperandType.Ot11Nope);

        // Token: 0x0400006A RID: 106
        public readonly VmInstrInfo Ldelem_ = new VmInstrInfo(-1705118555, VmOperandType.Ot5Int);

        // Token: 0x0400006B RID: 107
        public readonly VmInstrInfo Conv_u1_ = new VmInstrInfo(1055970854, VmOperandType.Ot11Nope);

        // Token: 0x0400006D RID: 109
        public readonly VmInstrInfo Ldind_i1_ = new VmInstrInfo(33169414, VmOperandType.Ot11Nope);

        // Token: 0x0400006E RID: 110
        public readonly VmInstrInfo Ldind_i_ = new VmInstrInfo(-1790442498, VmOperandType.Ot11Nope);

        // Token: 0x04000070 RID: 112
        public readonly VmInstrInfo Ldsfld_ = new VmInstrInfo(-1369658342, VmOperandType.Ot5Int);

        // Token: 0x04000071 RID: 113
        public readonly VmInstrInfo Ldloc_ = new VmInstrInfo(766115889, VmOperandType.Ot1UShort);

        // Token: 0x04000072 RID: 114
        public readonly VmInstrInfo Rem_un_ = new VmInstrInfo(-2121309775, VmOperandType.Ot11Nope);

        // Token: 0x04000073 RID: 115
        public readonly VmInstrInfo Conv_ovf_i8_ = new VmInstrInfo(-287049786, VmOperandType.Ot11Nope);

        // Token: 0x04000075 RID: 117
        public readonly VmInstrInfo Ldc_i4_0_ = new VmInstrInfo(89715609, VmOperandType.Ot11Nope);

        // Token: 0x04000078 RID: 120
        public readonly VmInstrInfo Ldloc_3_ = new VmInstrInfo(1790654656, VmOperandType.Ot11Nope);

        // Token: 0x0400007B RID: 123
        public readonly VmInstrInfo Ldsflda_ = new VmInstrInfo(-2097007575, VmOperandType.Ot5Int);

        // Token: 0x0400007C RID: 124
        public readonly VmInstrInfo Add_ovf_ = new VmInstrInfo(-545700640, VmOperandType.Ot11Nope);

        // Token: 0x0400007D RID: 125
        public readonly VmInstrInfo Refanytype_ = new VmInstrInfo(-971088331, VmOperandType.Ot11Nope);

        // Token: 0x0400007E RID: 126
        public readonly VmInstrInfo Blt_ = new VmInstrInfo(1978323310, VmOperandType.Ot0UInt);

        // Token: 0x0400007F RID: 127
        public readonly VmInstrInfo Conv_ovf_u8_un_ = new VmInstrInfo(1527584358, VmOperandType.Ot11Nope);

        // Token: 0x04000083 RID: 131
        public readonly VmInstrInfo Ldelem_i8_ = new VmInstrInfo(1272142104, VmOperandType.Ot11Nope);

        // Token: 0x04000084 RID: 132
        public readonly VmInstrInfo Ldc_i4_6_ = new VmInstrInfo(871172961, VmOperandType.Ot11Nope);

        // Token: 0x04000085 RID: 133
        public readonly VmInstrInfo Starg_s_ = new VmInstrInfo(-687376789, VmOperandType.Ot8Byte);

        // Token: 0x04000088 RID: 136
        public readonly VmInstrInfo Beq_ = new VmInstrInfo(352236975, VmOperandType.Ot0UInt);

        // Token: 0x0400008B RID: 139
        public readonly VmInstrInfo Ldfld_ = new VmInstrInfo(-688284774, VmOperandType.Ot5Int);

        // Token: 0x0400008C RID: 140
        public readonly VmInstrInfo Conv_ovf_i2_un_ = new VmInstrInfo(1663762471, VmOperandType.Ot11Nope);

        // Token: 0x0400008D RID: 141
        public readonly VmInstrInfo Conv_ovf_i_un_ = new VmInstrInfo(2093357171, VmOperandType.Ot11Nope);

        // Token: 0x0400008E RID: 142
        public readonly VmInstrInfo Ldelem_u4_ = new VmInstrInfo(896332376, VmOperandType.Ot11Nope);

        // Token: 0x0400008F RID: 143
        public readonly VmInstrInfo Conv_ovf_u4_un_ = new VmInstrInfo(-107488823, VmOperandType.Ot11Nope);

        // Token: 0x04000090 RID: 144
        public readonly VmInstrInfo Ldarga_ = new VmInstrInfo(2044160323, VmOperandType.Ot1UShort);

        // Token: 0x04000091 RID: 145
        public readonly VmInstrInfo Div_un_ = new VmInstrInfo(742839562, VmOperandType.Ot11Nope);

        // Token: 0x04000092 RID: 146
        public readonly VmInstrInfo Ldelem_r4_ = new VmInstrInfo(-1447311583, VmOperandType.Ot11Nope);

        // Token: 0x04000093 RID: 147
        public readonly VmInstrInfo And_ = new VmInstrInfo(1968373082, VmOperandType.Ot11Nope);

        // Token: 0x04000094 RID: 148
        public readonly VmInstrInfo Add_ = new VmInstrInfo(-1892228817, VmOperandType.Ot11Nope);

        // Token: 0x04000095 RID: 149
        public readonly VmInstrInfo Conv_ovf_u2_ = new VmInstrInfo(1775410326, VmOperandType.Ot11Nope);

        // Token: 0x04000096 RID: 150
        public readonly VmInstrInfo Xor_ = new VmInstrInfo(-273395395, VmOperandType.Ot11Nope);

        // Token: 0x04000097 RID: 151
        public readonly VmInstrInfo Stloc_1_ = new VmInstrInfo(-1446892238, VmOperandType.Ot11Nope);

        // Token: 0x04000099 RID: 153
        public readonly VmInstrInfo Conv_ovf_u2_un_ = new VmInstrInfo(-1274139658, VmOperandType.Ot11Nope);

        // Token: 0x0400009B RID: 155
        public readonly VmInstrInfo Ldc_i4_3_ = new VmInstrInfo(-722334296, VmOperandType.Ot11Nope);

        // Token: 0x0400009E RID: 158
        public readonly VmInstrInfo Ldelem_u1_ = new VmInstrInfo(580121148, VmOperandType.Ot11Nope);

        // Token: 0x0400009F RID: 159
        public readonly VmInstrInfo Ldelem_i4_ = new VmInstrInfo(778369289, VmOperandType.Ot11Nope);

        // Token: 0x040000A0 RID: 160
        public readonly VmInstrInfo Stfld_ = new VmInstrInfo(1721102239, VmOperandType.Ot5Int);

        // Token: 0x040000A1 RID: 161
        public readonly VmInstrInfo Ldc_i4_m1_ = new VmInstrInfo(-1374936951, VmOperandType.Ot11Nope);

        // Token: 0x040000A2 RID: 162
        public readonly VmInstrInfo Brfalse_ = new VmInstrInfo(476056811, VmOperandType.Ot0UInt);

        // Token: 0x040000A3 RID: 163
        public readonly VmInstrInfo Rem_ = new VmInstrInfo(1127773841, VmOperandType.Ot11Nope);

        // Token: 0x040000A4 RID: 164
        public readonly VmInstrInfo Neg_ = new VmInstrInfo(1824866698, VmOperandType.Ot11Nope);

        // Token: 0x040000A5 RID: 165
        public readonly VmInstrInfo Initblk_ = new VmInstrInfo(1848160160, VmOperandType.Ot11Nope);

        // Token: 0x040000A6 RID: 166
        public readonly VmInstrInfo Ldelem_r8_ = new VmInstrInfo(-522987252, VmOperandType.Ot11Nope);

	    // Token: 0x040000A9 RID: 169
	    public readonly VmInstrInfo Cpobj_ = new VmInstrInfo(1238115537, VmOperandType.Ot5Int);

	    // Token: 0x040000AA RID: 170
	    public readonly VmInstrInfo Ldarga_s_ = new VmInstrInfo(-1193068213, VmOperandType.Ot8Byte);

	    // Token: 0x040000AB RID: 171
	    public readonly VmInstrInfo Br_ = new VmInstrInfo(658728581, VmOperandType.Ot0UInt);

	    // Token: 0x040000AC RID: 172
	    public readonly VmInstrInfo Conv_u2_ = new VmInstrInfo(-2099750455, VmOperandType.Ot11Nope);

	    // Token: 0x040000AE RID: 174
	    public readonly VmInstrInfo Stelem_i_ = new VmInstrInfo(-358560507, VmOperandType.Ot11Nope);

	    // Token: 0x040000AF RID: 175
	    public readonly VmInstrInfo Stloc_s_ = new VmInstrInfo(1804315644, VmOperandType.Ot8Byte);

	    // Token: 0x040000B1 RID: 177
	    public readonly VmInstrInfo Ble_un_ = new VmInstrInfo(1001656673, VmOperandType.Ot0UInt);

	    // Token: 0x040000B4 RID: 180
	    public readonly VmInstrInfo Ldc_i4_2_ = new VmInstrInfo(-2082446517, VmOperandType.Ot11Nope);

	    // Token: 0x040000B5 RID: 181
	    public readonly VmInstrInfo Blt_un_ = new VmInstrInfo(-1002275164, VmOperandType.Ot0UInt);

	    // Token: 0x040000B7 RID: 183
	    public readonly VmInstrInfo Ldind_ref_ = new VmInstrInfo(-101579585, VmOperandType.Ot11Nope);

        // Token: 0x040000BB RID: 187
        public readonly VmInstrInfo Ldind_i2_ = new VmInstrInfo(1338544134, VmOperandType.Ot11Nope);

	    // Token: 0x040000BF RID: 191
	    public readonly VmInstrInfo Shr_ = new VmInstrInfo(2061114403, VmOperandType.Ot11Nope);

	    // Token: 0x040000C0 RID: 192
	    public readonly VmInstrInfo Sub_ovf_ = new VmInstrInfo(-1326124455, VmOperandType.Ot11Nope);

	    // Token: 0x040000C1 RID: 193
	    public readonly VmInstrInfo Mul_ = new VmInstrInfo(-368354161, VmOperandType.Ot11Nope);

	    // Token: 0x040000C2 RID: 194
	    public readonly VmInstrInfo Conv_r4_ = new VmInstrInfo(461467744, VmOperandType.Ot11Nope);

	    // Token: 0x040000C3 RID: 195
	    public readonly VmInstrInfo Ldarg_s_ = new VmInstrInfo(916919316, VmOperandType.Ot8Byte);

	    // Token: 0x040000C7 RID: 199
	    public readonly VmInstrInfo Conv_ovf_u8_ = new VmInstrInfo(-1916788012, VmOperandType.Ot11Nope);

	    // Token: 0x040000C8 RID: 200
	    public readonly VmInstrInfo Ldind_u2_ = new VmInstrInfo(-1831891367, VmOperandType.Ot11Nope);

	    // Token: 0x040000C9 RID: 201
	    public readonly VmInstrInfo Ldind_u4_ = new VmInstrInfo(-1620795876, VmOperandType.Ot11Nope);

	    // Token: 0x040000CA RID: 202
	    public readonly VmInstrInfo Conv_ovf_i4_ = new VmInstrInfo(488024265, VmOperandType.Ot11Nope);

	    // Token: 0x040000CD RID: 205
	    public readonly VmInstrInfo Ldarg_1_ = new VmInstrInfo(326597331, VmOperandType.Ot11Nope);

	    // Token: 0x040000CE RID: 206
	    public readonly VmInstrInfo Conv_ovf_u_ = new VmInstrInfo(115989675, VmOperandType.Ot11Nope);

	    // Token: 0x040000CF RID: 207
	    public readonly VmInstrInfo Ldloc_s_ = new VmInstrInfo(1019004451, VmOperandType.Ot8Byte);

	    // Token: 0x040000D0 RID: 208
	    public readonly VmInstrInfo Conv_i2_ = new VmInstrInfo(-108178384, VmOperandType.Ot11Nope);

	    // Token: 0x040000D1 RID: 209
	    public readonly VmInstrInfo Conv_ovf_i_ = new VmInstrInfo(-2109763431, VmOperandType.Ot11Nope);

	    // Token: 0x040000D2 RID: 210
	    public readonly VmInstrInfo Ble_ = new VmInstrInfo(1321262543, VmOperandType.Ot0UInt);

	    // Token: 0x040000D3 RID: 211
	    public readonly VmInstrInfo Unbox_ = new VmInstrInfo(-1668682548, VmOperandType.Ot5Int);

	    // Token: 0x040000D4 RID: 212
	    public readonly VmInstrInfo Stelem_r4_ = new VmInstrInfo(-1251429380, VmOperandType.Ot11Nope);

	    // Token: 0x040000D5 RID: 213
	    public readonly VmInstrInfo Stloc_3_ = new VmInstrInfo(1073782561, VmOperandType.Ot11Nope);

	    // Token: 0x040000D7 RID: 215
	    public readonly VmInstrInfo Brtrue_ = new VmInstrInfo(1985375111, VmOperandType.Ot0UInt);

	    // Token: 0x040000D8 RID: 216
	    public readonly VmInstrInfo Stelem_ = new VmInstrInfo(-633052479, VmOperandType.Ot5Int);

	    // Token: 0x040000D9 RID: 217
	    public readonly VmInstrInfo Stelem_i4_ = new VmInstrInfo(-638226942, VmOperandType.Ot11Nope);

	    // Token: 0x040000DA RID: 218
	    public readonly VmInstrInfo Conv_ovf_u1_un_ = new VmInstrInfo(-854623375, VmOperandType.Ot11Nope);

	    // Token: 0x040000DB RID: 219
	    public readonly VmInstrInfo Add_ovf_un_ = new VmInstrInfo(-2145629048, VmOperandType.Ot11Nope);

	    // Token: 0x040000DC RID: 220
	    public readonly VmInstrInfo Conv_u8_ = new VmInstrInfo(1396092080, VmOperandType.Ot11Nope);

	    // Token: 0x040000DE RID: 222
	    public readonly VmInstrInfo Bgt_ = new VmInstrInfo(-939929863, VmOperandType.Ot0UInt);

	    // Token: 0x040000E2 RID: 226
	    public readonly VmInstrInfo Bgt_un_ = new VmInstrInfo(-73779400, VmOperandType.Ot0UInt);

	    // Token: 0x040000E3 RID: 227
	    public readonly VmInstrInfo Stelem_r8_ = new VmInstrInfo(849078739, VmOperandType.Ot11Nope);

	    // Token: 0x040000E5 RID: 229
	    public readonly VmInstrInfo Mkrefany_ = new VmInstrInfo(1810420701, VmOperandType.Ot5Int);

	    // Token: 0x040000E6 RID: 230
	    public readonly VmInstrInfo Conv_ovf_u_un_ = new VmInstrInfo(-1209242284, VmOperandType.Ot11Nope);

	    // Token: 0x040000E7 RID: 231
	    public readonly VmInstrInfo Conv_ovf_i1_ = new VmInstrInfo(-1678823314, VmOperandType.Ot11Nope);

	    // Token: 0x040000E8 RID: 232
	    public readonly VmInstrInfo Conv_ovf_i1_un_ = new VmInstrInfo(-1171707127, VmOperandType.Ot11Nope);

	    // Token: 0x040000EA RID: 234
        public readonly VmInstrInfo Stsfld_ = new VmInstrInfo(-1272257470, VmOperandType.Ot5Int);

	    // Token: 0x040000EB RID: 235
	    public readonly VmInstrInfo Starg_ = new VmInstrInfo(-1559324355, VmOperandType.Ot1UShort);

	    // Token: 0x040000EC RID: 236
	    public readonly VmInstrInfo Ldflda_ = new VmInstrInfo(685223722, VmOperandType.Ot5Int);

	    // Token: 0x040000EE RID: 238
	    public readonly VmInstrInfo Sub_ = new VmInstrInfo(1925911547, VmOperandType.Ot11Nope);

	    // Token: 0x040000F0 RID: 240
	    public readonly VmInstrInfo Conv_ovf_i2_ = new VmInstrInfo(2079826493, VmOperandType.Ot11Nope);

	    // Token: 0x040000F1 RID: 241
	    public readonly VmInstrInfo Ldarg_0_ = new VmInstrInfo(-1817778622, VmOperandType.Ot11Nope);

	    // Token: 0x040000F2 RID: 242
	    public readonly VmInstrInfo Ldelem_i2_ = new VmInstrInfo(-1703864226, VmOperandType.Ot11Nope);

	    // Token: 0x040000F3 RID: 243
	    public readonly VmInstrInfo Ceq_ = new VmInstrInfo(-490385948, VmOperandType.Ot11Nope);

	    // Token: 0x040000F4 RID: 244
	    public readonly VmInstrInfo Ldelema_ = new VmInstrInfo(-659575843, VmOperandType.Ot5Int);

	    // Token: 0x040000F5 RID: 245
	    public readonly VmInstrInfo Localloc_ = new VmInstrInfo(487454996, VmOperandType.Ot11Nope);

	    // Token: 0x040000F6 RID: 246
	    public readonly VmInstrInfo Conv_ovf_i4_un_ = new VmInstrInfo(-900057353, VmOperandType.Ot11Nope);

	    // Token: 0x040000F7 RID: 247
	    public readonly VmInstrInfo Bge_un_ = new VmInstrInfo(784647969, VmOperandType.Ot0UInt);

	    // Token: 0x040000F8 RID: 248
	    public readonly VmInstrInfo Ldelem_ref_ = new VmInstrInfo(880972378, VmOperandType.Ot11Nope);

	    // Token: 0x040000FA RID: 250
	    public readonly VmInstrInfo Conv_ovf_i8_un_ = new VmInstrInfo(20637445, VmOperandType.Ot11Nope);

	    // Token: 0x040000FB RID: 251
	    public readonly VmInstrInfo Ldind_i8_ = new VmInstrInfo(-607543449, VmOperandType.Ot11Nope);

	    // Token: 0x040000FC RID: 252
	    public readonly VmInstrInfo Refanyval_ = new VmInstrInfo(1010177566, VmOperandType.Ot5Int);

	    // Token: 0x040000FE RID: 254
	    public readonly VmInstrInfo Dup_ = new VmInstrInfo(85722172, VmOperandType.Ot11Nope);

	    // Token: 0x040000FF RID: 255
	    public readonly VmInstrInfo Stloc_0_ = new VmInstrInfo(-1071153572, VmOperandType.Ot11Nope);

	    // Token: 0x04000100 RID: 256
	    public readonly VmInstrInfo Ldc_i4_4_ = new VmInstrInfo(-72363801, VmOperandType.Ot11Nope);

	    // Token: 0x04000101 RID: 257
	    public readonly VmInstrInfo Ldind_r8_ = new VmInstrInfo(813030660, VmOperandType.Ot11Nope);

	    // Token: 0x04000102 RID: 258
	    public readonly VmInstrInfo Ldc_i4_7_ = new VmInstrInfo(-1136876649, VmOperandType.Ot11Nope);

	    // Token: 0x04000103 RID: 259
	    public readonly VmInstrInfo Stelem_i8_ = new VmInstrInfo(588832478, VmOperandType.Ot11Nope);

	    // Token: 0x04000105 RID: 261
        public readonly VmInstrInfo Mul_ovf_un_ = new VmInstrInfo(-356198078, VmOperandType.Ot11Nope);

	    // Token: 0x04000106 RID: 262
	    public readonly VmInstrInfo Conv_u_ = new VmInstrInfo(1795519976, VmOperandType.Ot11Nope);

	    // Token: 0x04000108 RID: 264
	    public readonly VmInstrInfo Ldelem_i_ = new VmInstrInfo(1499071663, VmOperandType.Ot11Nope);

	    // Token: 0x04000109 RID: 265
	    public readonly VmInstrInfo Ldarg_ = new VmInstrInfo(-1071239412, VmOperandType.Ot1UShort);

	    // Token: 0x0400010A RID: 266
	    public readonly VmInstrInfo Conv_r_un_ = new VmInstrInfo(-23925463, VmOperandType.Ot11Nope);

	    // Token: 0x0400010B RID: 267
	    public readonly VmInstrInfo Ldc_i4_8_ = new VmInstrInfo(1119515810, VmOperandType.Ot11Nope);

	    // Token: 0x0400010C RID: 268
	    public readonly VmInstrInfo Conv_i8_ = new VmInstrInfo(1980167243, VmOperandType.Ot11Nope);

	    // Token: 0x0400010D RID: 269
	    public readonly VmInstrInfo Ldloc_1_ = new VmInstrInfo(704985473, VmOperandType.Ot11Nope);

	    // Token: 0x0400010E RID: 270
	    public readonly VmInstrInfo Ldelem_u2_ = new VmInstrInfo(-1142530894, VmOperandType.Ot11Nope);

	    // Token: 0x0400010F RID: 271
	    public readonly VmInstrInfo Throw_ = new VmInstrInfo(-958454075, VmOperandType.Ot11Nope);

	    // Token: 0x04000110 RID: 272
	    public readonly VmInstrInfo Cpblk_ = new VmInstrInfo(-123910492, VmOperandType.Ot11Nope);

	    // Token: 0x04000111 RID: 273
	    public readonly VmInstrInfo Ldind_u1_ = new VmInstrInfo(1476085916, VmOperandType.Ot11Nope);

	    // Token: 0x04000113 RID: 275
	    public readonly VmInstrInfo Stloc_2_ = new VmInstrInfo(392938325, VmOperandType.Ot11Nope);

	    // Token: 0x04000114 RID: 276
	    public readonly VmInstrInfo Ldarg_3_ = new VmInstrInfo(-1756998893, VmOperandType.Ot11Nope);

	    // Token: 0x04000115 RID: 277
	    public readonly VmInstrInfo Stloc_ = new VmInstrInfo(1447397361, VmOperandType.Ot1UShort);

	    // Token: 0x04000117 RID: 279
	    public readonly VmInstrInfo Ldc_i4_5_ = new VmInstrInfo(-656328799, VmOperandType.Ot11Nope);

	    // Token: 0x04000118 RID: 280
	    public readonly VmInstrInfo Conv_ovf_u1_ = new VmInstrInfo(344575979, VmOperandType.Ot11Nope);

	    // Token: 0x0400011A RID: 282
	    public readonly VmInstrInfo Ldind_i4_ = new VmInstrInfo(234126039, VmOperandType.Ot11Nope);

	    // Token: 0x0400011B RID: 283
	    public readonly VmInstrInfo Switch_ = new VmInstrInfo(8625656, VmOperandType.Ot9IntArr);

	    // Token: 0x0400011C RID: 284
	    public readonly VmInstrInfo Arglist_ = new VmInstrInfo(1783361912, VmOperandType.Ot11Nope);

        // Token: 0x0400011F RID: 287
        public readonly VmInstrInfo Shr_un_ = new VmInstrInfo(897680915, VmOperandType.Ot11Nope);

	    // Token: 0x04000120 RID: 288
	    public readonly VmInstrInfo Ldloc_2_ = new VmInstrInfo(-17993965, VmOperandType.Ot11Nope);

	    // Token: 0x04000121 RID: 289
	    public readonly VmInstrInfo Conv_ovf_u4_ = new VmInstrInfo(1596489702, VmOperandType.Ot11Nope);

	    // Token: 0x04000122 RID: 290
	    public readonly VmInstrInfo Bge_ = new VmInstrInfo(-1225693454, VmOperandType.Ot0UInt);

	    // Token: 0x04000123 RID: 291
	    public readonly VmInstrInfo Ldloc_0_ = new VmInstrInfo(1021709264, VmOperandType.Ot11Nope);

	    // Token: 0x04000124 RID: 292
	    public readonly VmInstrInfo Bne_un_ = new VmInstrInfo(68951288, VmOperandType.Ot0UInt);

	    // Token: 0x04000126 RID: 294
	    public readonly VmInstrInfo Stelem_i2_ = new VmInstrInfo(1223054294, VmOperandType.Ot11Nope);

	    // Token: 0x04000127 RID: 295
	    public readonly VmInstrInfo Sub_ovf_un_ = new VmInstrInfo(-851734976, VmOperandType.Ot11Nope);

	    // Token: 0x04000128 RID: 296
	    public readonly VmInstrInfo Ret_ = new VmInstrInfo(1882847521, VmOperandType.Ot11Nope);
        #endregion
    }

    public enum VmOperandType
    { 
        Ot0UInt, Ot1UShort, Ot2Byte, Ot3UShort, Ot4Double, Ot5Int, Ot6SByte, Ot7Long, Ot8Byte, Ot9IntArr, Ot10Float, Ot11Nope, Ot12Int
    }
    // Token: 0x0200005F RID: 95
    public class VmInstrInfo // \u000F\u2005
    {
        // Token: 0x06000377 RID: 887 RVA: 0x00015B74 File Offset: 0x00013D74
        public VmInstrInfo(int id, VmOperandType operandType)
        {
            Id = id;
            OperandType = operandType;
        }

        // Token: 0x06000378 RID: 888 RVA: 0x00015B8C File Offset: 0x00013D8C
        // Token: 0x06000379 RID: 889 RVA: 0x00015B94 File Offset: 0x00013D94
        // Token: 0x0400018F RID: 399
        public int Id { get; }

        // Token: 0x0600037A RID: 890 RVA: 0x00015BA0 File Offset: 0x00013DA0
        // Token: 0x04000190 RID: 400
        public VmOperandType OperandType { get; }

        // Token: 0x0600037B RID: 891 RVA: 0x00015BA8 File Offset: 0x00013DA8
        public override bool Equals(object o)
        {
            var p = o as VmInstrInfo;
            return (p != null) && EqualTo(p);
        }

        // Token: 0x0600037C RID: 892 RVA: 0x00015BD0 File Offset: 0x00013DD0
        private bool EqualTo(VmInstrInfo p)
        {
            return p.Id== Id;
        }

        // Token: 0x0600037D RID: 893 RVA: 0x00015BE0 File Offset: 0x00013DE0
        public static bool operator ==(VmInstrInfo o1, VmInstrInfo o2)
        {
            // ReSharper disable once PossibleNullReferenceException
            return o1.EqualTo(o2);
        }

        // Token: 0x0600037E RID: 894 RVA: 0x00015BEC File Offset: 0x00013DEC
        public static bool operator !=(VmInstrInfo o1, VmInstrInfo o2)
        {
            return !(o1 == o2);
        }

        // Token: 0x0600037F RID: 895 RVA: 0x00015BF8 File Offset: 0x00013DF8
        public override int GetHashCode()
        {
            return Id.GetHashCode();
        }

        // Token: 0x06000380 RID: 896 RVA: 0x00015C14 File Offset: 0x00013E14
        public override string ToString()
        {
            return Id.ToString();
        }
    }
}
