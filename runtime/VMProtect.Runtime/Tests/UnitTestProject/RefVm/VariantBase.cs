using System;
using System.Linq;
using System.Reflection;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000059 RID: 89
    internal abstract class VariantBase // \u000F
    {
	    // Token: 0x0600034A RID: 842
	    public abstract object GetValueAbstract(); // \u000F\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x0600034B RID: 843
	    public abstract void SetValueAbstract(object o); // \u000F\u2008\u2000\u2002\u200A\u0002

        // Token: 0x0600034C RID: 844
        public enum Vtc
        {
            Tc0VariantBaseHolder,
            Tc1Bool,
            Tc2Array,
            Tc3Method,
            Tc4FieldInfo,
            Tc5Enum,
            Tc6Char,
            Tc7Ulong,
            Tc8Float,
            Tc9Uint,
            Tc10Ushort,
            Tc11ValueType,
            Tc12Sbyte,
            Tc13UIntPtr,
            Tc14Byte,
            Tc15Short,
            Tc16String,
            Tc17IntPtr,
            Tc18Object,
            Tc19Int,
            Tc20MdArrayValue,
            Tc21Double,
            Tc22SdArrayValue,
            Tc23LocalsIdxHolder,
            Tc24Long
        }

        public abstract Vtc GetTypeCode(); // \u000F\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x0600034D RID: 845
	    public abstract VariantBase CopyFrom(VariantBase t); // \u000F\u2008\u2000\u2002\u200A\u0002

        // Token: 0x0600034E RID: 846
        public abstract VariantBase Clone(); // \u000F\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x0600034F RID: 847 RVA: 0x000153EC File Offset: 0x000135EC
	    public virtual bool IsAddr() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return false;
	    }

	    // Token: 0x06000350 RID: 848 RVA: 0x000153F0 File Offset: 0x000135F0
	    public Type GetVariantType() // \u0002
        {
		    return _type;
	    }

	    // Token: 0x06000351 RID: 849 RVA: 0x000153F8 File Offset: 0x000135F8
	    public void SetVariantType(Type val) // \u0002
        {
		    _type = val;
	    }

	    // Token: 0x04000186 RID: 390
        private Type _type; // \u0002

        // bug was fixed and unit tested (было превращение Enum в число без учета переполнения)
        public static long SignedLongFromEnum(EnumVariant src)
        {
            var val = src.GetValue();
            switch (Convert.GetTypeCode(val))
            {
                /*case TypeCode.SByte:
                case TypeCode.Int16:
                case TypeCode.Int32:
                case TypeCode.Int64:*/
                default:
                    return Convert.ToInt64(val);
                case TypeCode.Byte:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                case TypeCode.UInt64:
                    return (long)Convert.ToUInt64(val);
            }
        }

        public static VariantBase SignedVariantFromEnum(EnumVariant src)
        {
            var val = src.GetValue();
            switch (Convert.GetTypeCode(val))
            {
                case TypeCode.SByte:
                case TypeCode.Int16:
                case TypeCode.Int32:
                    var i = Convert.ToInt32(val);
                    var reti = new IntVariant();
                    reti.SetValue(i);
                    return reti;
                case TypeCode.Int64:
                    var l = Convert.ToInt64(val);
                    var retl = new LongVariant();
                    retl.SetValue(l);
                    return retl;
                case TypeCode.Byte:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                    var u = Convert.ToUInt32(val);
                    var retu = new IntVariant();
                    retu.SetValue((int)u);
                    return retu;
                //case TypeCode.UInt64:
                default:
                    var ul = Convert.ToUInt64(val);
                    var retul = new LongVariant();
                    retul.SetValue((long)ul);
                    return retul;
            }
        }
    }
    // Token: 0x02000003 RID: 3
    internal sealed class ArrayVariant : VariantBase // \u0002\u2000
    {
	    // Token: 0x06000009 RID: 9 RVA: 0x000021A8 File Offset: 0x000003A8
	    public Array GetValue() // \u0002
        {
		    return _value;
	    }

	    // Token: 0x0600000A RID: 10 RVA: 0x000021B0 File Offset: 0x000003B0
	    public void SetValue(Array val) // \u0002
        {
            _value = val;
	    }

	    // Token: 0x0600000B RID: 11 RVA: 0x000021BC File Offset: 0x000003BC
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    return GetValue();
	    }

	    // Token: 0x0600000C RID: 12 RVA: 0x000021C4 File Offset: 0x000003C4
	    public override void SetValueAbstract(object o) // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    SetValue((Array)o);
	    }

	    // Token: 0x0600000D RID: 13 RVA: 0x000021D4 File Offset: 0x000003D4
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    var ret = new ArrayVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetType());
		    return ret;
	    }

	    // Token: 0x0600000E RID: 14 RVA: 0x000021F4 File Offset: 0x000003F4
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    return Vtc.Tc2Array;
	    }

	    // Token: 0x0600000F RID: 15 RVA: 0x000021F8 File Offset: 0x000003F8
	    public override VariantBase CopyFrom(VariantBase src) // \u000F\u2008\u2000\u2002\u200A\u0002
        {
            SetVariantType(src.GetVariantType());
		    var num = src.GetTypeCode();
		    if (num != Vtc.Tc2Array)
		    {
			    if (num != Vtc.Tc18Object)
			    {
				    throw new ArgumentOutOfRangeException();
			    }
                SetValue((Array)((ObjectVariant)src).GetValue());
		    }
		    else
		    {
			    SetValue(((ArrayVariant)src).GetValue());
		    }
		    return this;
	    }

	    // Token: 0x04000001 RID: 1
	    private Array _value; // \u0002
    }

    // Token: 0x0200002B RID: 43
    internal sealed class ObjectVariant : VariantBase // \u0006\u2002
    {
	    // Token: 0x0600013E RID: 318 RVA: 0x00006458 File Offset: 0x00004658
	    public object GetValue() // \u0002
        {
		    return _value;
	    }

	    // Token: 0x0600013F RID: 319 RVA: 0x00006460 File Offset: 0x00004660
	    public void SetValue(object val) // \u0002
        {
		    _value = val;
	    }

	    // Token: 0x06000140 RID: 320 RVA: 0x0000646C File Offset: 0x0000466C
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000141 RID: 321 RVA: 0x00006474 File Offset: 0x00004674
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
            SetValue(val);
	    }

	    // Token: 0x06000142 RID: 322 RVA: 0x00006480 File Offset: 0x00004680
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    return Vtc.Tc18Object;
	    }

	    // Token: 0x06000143 RID: 323 RVA: 0x00006484 File Offset: 0x00004684
	    public override VariantBase CopyFrom(VariantBase src) // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    SetVariantType(src.GetVariantType());
		    SetValue(src.GetValueAbstract());
		    return this;
	    }

	    // Token: 0x06000144 RID: 324 RVA: 0x000064AC File Offset: 0x000046AC
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    var ret = new ObjectVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0400004E RID: 78
	    private object _value;
    }

    // Token: 0x02000004 RID: 4
    internal sealed class ShortVariant : VariantBase
    {
	    // Token: 0x06000011 RID: 17 RVA: 0x00002260 File Offset: 0x00000460
	    public short GetValue() // \u0002
        {
		    return _value;
	    }

	    // Token: 0x06000012 RID: 18 RVA: 0x00002268 File Offset: 0x00000468
	    public void SetValue(short val) // \u0002
        {
		    _value = val;
	    }

	    // Token: 0x06000013 RID: 19 RVA: 0x00002274 File Offset: 0x00000474
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    return GetValue();
	    }

	    // Token: 0x06000014 RID: 20 RVA: 0x00002284 File Offset: 0x00000484
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    if (val is int)
		    {
			    SetValue((short)(int)val);
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((short)(long)val);
			    return;
		    }
		    if (val is ushort)
		    {
			    SetValue((short)(ushort)val);
			    return;
		    }
		    if (val is uint)
		    {
			    SetValue((short)(uint)val);
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((short)(ulong)val);
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((short)(float)val);
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((short)(double)val);
			    return;
		    }
		    SetValue(Convert.ToInt16(val));
	    }

	    // Token: 0x06000015 RID: 21 RVA: 0x00002338 File Offset: 0x00000538
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    var ret = new ShortVariant();
            ret.SetValue(_value);
            ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000016 RID: 22 RVA: 0x00002358 File Offset: 0x00000558
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    return Vtc.Tc15Short;
	    }

	    // Token: 0x06000017 RID: 23 RVA: 0x0000235C File Offset: 0x0000055C
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
        {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToInt16(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((short)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((short)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((short)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((short)((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((short)(int)((IntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToInt16(((ObjectVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((short)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((short)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((short)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000002 RID: 2
        private short _value; // \u0002
    }

    // Token: 0x02000005 RID: 5
    internal sealed class MethodVariant : VariantBase // \u0002\u2002
    {
	    // Token: 0x06000019 RID: 25 RVA: 0x00002538 File Offset: 0x00000738
	    public MethodBase GetValue() //\u0002
        {
		    return _value;
	    }

	    // Token: 0x0600001A RID: 26 RVA: 0x00002540 File Offset: 0x00000740
	    public void SetValue(MethodBase val) //\u0002
	    {
		    _value = val;
	    }

	    // Token: 0x0600001B RID: 27 RVA: 0x0000254C File Offset: 0x0000074C
	    public IntPtr GetFunctionPointer() // \u0002
	    {
		    return GetValue().MethodHandle.GetFunctionPointer();
	    }

	    // Token: 0x0600001C RID: 28 RVA: 0x0000256C File Offset: 0x0000076C
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x0600001D RID: 29 RVA: 0x00002574 File Offset: 0x00000774
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue((MethodBase)val);
	    }

	    // Token: 0x0600001E RID: 30 RVA: 0x00002584 File Offset: 0x00000784
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc3Method;
	    }

	    // Token: 0x0600001F RID: 31 RVA: 0x00002588 File Offset: 0x00000788
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num == Vtc.Tc3Method)
		    {
			    SetValue(((MethodVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x06000020 RID: 32 RVA: 0x000025C8 File Offset: 0x000007C8
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new MethodVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000003 RID: 3
	    private MethodBase _value;
    }

    // Token: 0x02000008 RID: 8
    internal sealed class StringVariant : VariantBase // \u0002\u2005
    {
	    // Token: 0x0600002B RID: 43 RVA: 0x000026BC File Offset: 0x000008BC
	    public string GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x0600002C RID: 44 RVA: 0x000026C4 File Offset: 0x000008C4
	    public void SetValue(string val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x0600002D RID: 45 RVA: 0x000026D0 File Offset: 0x000008D0
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x0600002E RID: 46 RVA: 0x000026D8 File Offset: 0x000008D8
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue((string)val);
	    }

	    // Token: 0x0600002F RID: 47 RVA: 0x000026E8 File Offset: 0x000008E8
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc16String;
	    }

	    // Token: 0x06000030 RID: 48 RVA: 0x000026EC File Offset: 0x000008EC
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num != Vtc.Tc16String)
		    {
			    if (num != Vtc.Tc18Object)
			    {
				    throw new ArgumentOutOfRangeException();
			    }
			    SetValue((string)((ObjectVariant)val).GetValue());
		    }
		    else
		    {
			    SetValue(((StringVariant)val).GetValue());
		    }
		    return this;
	    }

	    // Token: 0x06000031 RID: 49 RVA: 0x0000274C File Offset: 0x0000094C
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new StringVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000007 RID: 7
	    private string _value;
    }

    // Token: 0x0200000F RID: 15
    internal sealed class BoolVariant : VariantBase // \u0003\u2000
    {
	    // Token: 0x06000062 RID: 98 RVA: 0x00003470 File Offset: 0x00001670
	    public bool GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000063 RID: 99 RVA: 0x00003478 File Offset: 0x00001678
	    public void SetValue(bool val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000064 RID: 100 RVA: 0x00003484 File Offset: 0x00001684
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000065 RID: 101 RVA: 0x00003494 File Offset: 0x00001694
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue(Convert.ToBoolean(val));
	    }

	    // Token: 0x06000066 RID: 102 RVA: 0x000034A4 File Offset: 0x000016A4
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc1Bool;
	    }

	    // Token: 0x06000067 RID: 103 RVA: 0x000034A8 File Offset: 0x000016A8
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(((BoolVariant)val).GetValue());
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToBoolean(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue(Convert.ToBoolean(((UlongVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(Convert.ToBoolean(((UintVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(Convert.ToBoolean(((UshortVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(Convert.ToBoolean(((SbyteVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue(Convert.ToBoolean(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(Convert.ToBoolean(((ByteVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(Convert.ToBoolean(((ShortVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue(Convert.ToBoolean(((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToBoolean(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue(Convert.ToBoolean(((IntVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc24Long:
			    SetValue(Convert.ToBoolean(((LongVariant)val).GetValue()));
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x06000068 RID: 104 RVA: 0x00003694 File Offset: 0x00001894
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new BoolVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000017 RID: 23
	    private bool _value;
    }

    // Token: 0x02000010 RID: 16
    internal sealed class IntVariant : VariantBase // \u0003\u2001
    {
	    // Token: 0x0600006A RID: 106 RVA: 0x000036BC File Offset: 0x000018BC
	    public int GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x0600006B RID: 107 RVA: 0x000036C4 File Offset: 0x000018C4
	    public void SetValue(int val) // \u0002
	    {

            _value = val;
	    }

	    // Token: 0x0600006C RID: 108 RVA: 0x000036D0 File Offset: 0x000018D0
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x0600006D RID: 109 RVA: 0x000036E0 File Offset: 0x000018E0
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is long)
		    {
			    SetValue((int)((long)val));
			    return;
		    }
		    if (val is ushort)
		    {
			    SetValue((ushort)val);
			    return;
		    }
		    if (val is uint)
		    {
			    SetValue((int)((uint)val));
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((int)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((int)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((int)((double)val));
			    return;
		    }
		    SetValue(Convert.ToInt32(val));
	    }

	    // Token: 0x0600006E RID: 110 RVA: 0x0000377C File Offset: 0x0000197C
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new IntVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0600006F RID: 111 RVA: 0x0000379C File Offset: 0x0000199C
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc19Int;
	    }

	    // Token: 0x06000070 RID: 112 RVA: 0x000037A0 File Offset: 0x000019A0
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToInt32(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((int)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((int)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((int)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((int)((uint)((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((int)((IntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToInt32(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue(((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((int)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue(Convert.ToInt32(((LongVariant)val).GetValue()));
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000018 RID: 24
	    private int _value;
    }

    // Token: 0x02000011 RID: 17
    internal sealed class IntPtrVariant : VariantBase // \u0003\u2002
    {
	    // Token: 0x06000072 RID: 114 RVA: 0x00003998 File Offset: 0x00001B98
	    public IntPtr GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000073 RID: 115 RVA: 0x000039A0 File Offset: 0x00001BA0
	    public void SetValue(IntPtr val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000074 RID: 116 RVA: 0x000039AC File Offset: 0x00001BAC
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new IntPtrVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000075 RID: 117 RVA: 0x000039CC File Offset: 0x00001BCC
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000076 RID: 118 RVA: 0x000039DC File Offset: 0x00001BDC
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue((IntPtr)val);
	    }

	    // Token: 0x06000077 RID: 119 RVA: 0x000039EC File Offset: 0x00001BEC
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc17IntPtr;
	    }

	    // Token: 0x06000078 RID: 120 RVA: 0x000039F0 File Offset: 0x00001BF0
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc3Method:
		    {
			    var mv = (MethodVariant)val;
			    SetValue(mv.GetFunctionPointer());
			    return this;
		    }
		    case Vtc.Tc5Enum:
			    SetValue(new IntPtr(Convert.ToInt64(((EnumVariant)val).GetValue())));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((IntPtr)((long)((UlongVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((IntPtr)((long)((FloatVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((IntPtr)((long)((UintVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((IntPtr)((int)((UshortVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((IntPtr)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue((IntPtr)((int)((ByteVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((IntPtr)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue(((IntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue((IntPtr)((UIntPtrVariant)val).GetValue().ToUInt64());
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((IntPtr)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((IntPtr)((long)((DoubleVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((IntPtr)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000019 RID: 25
	    private IntPtr _value;
    }

    // Token: 0x02000014 RID: 20
    internal sealed class ValueTypeVariant : VariantBase // \u0003\u2005
    {
	    // Token: 0x06000086 RID: 134 RVA: 0x00003CF0 File Offset: 0x00001EF0
	    public ValueTypeVariant(object val)
	    {
		    if (val != null && !(val is ValueType))
		    {
			    throw new ArgumentException();
		    }
		    _value = val;
	    }

	    // Token: 0x06000087 RID: 135 RVA: 0x00003D10 File Offset: 0x00001F10
	    public object GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000088 RID: 136 RVA: 0x00003D18 File Offset: 0x00001F18
	    public void SetValue(object val) // \u0002
	    {
		    if (val != null && !(val is ValueType))
		    {
			    throw new ArgumentException();
		    }
		    _value = val;
	    }

	    // Token: 0x06000089 RID: 137 RVA: 0x00003D34 File Offset: 0x00001F34
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x0600008A RID: 138 RVA: 0x00003D3C File Offset: 0x00001F3C
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue(val);
	    }

	    // Token: 0x0600008B RID: 139 RVA: 0x00003D48 File Offset: 0x00001F48
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc11ValueType;
	    }

	    // Token: 0x0600008C RID: 140 RVA: 0x00003D4C File Offset: 0x00001F4C
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num != Vtc.Tc11ValueType)
		    {
			    if (num != Vtc.Tc18Object)
			    {
				    SetValue(val.GetTypeCode());
			    }
			    else
			    {
				    SetValue(((UIntPtrVariant)val).GetValue());
			    }
		    }
		    else if (GetValue() != null)
		    {
			    var obj = ((ValueTypeVariant)val).GetValue();
			    var type = GetValue().GetType();
		        // ReSharper disable once UseMethodIsInstanceOfType
			    if (obj != null && !type.IsPrimitive && !type.IsEnum && type.IsAssignableFrom(obj.GetType()))
			    {
			        var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy | BindingFlags.GetField | BindingFlags.SetField);
			        foreach (var fieldInfo in fields)
			        {
			            fieldInfo.SetValue(GetValue(), fieldInfo.GetValue(obj));
			        }
			    }
			    else
			    {
				    SetValue(obj);
			    }
		    }
		    else
		    {
			    SetValue(((ValueTypeVariant)val).GetValue());
		    }
		    return this;
	    }

	    // Token: 0x0600008D RID: 141 RVA: 0x00003E38 File Offset: 0x00002038
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new ValueTypeVariant(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0400001D RID: 29
	    private object _value;
    }

    // Token: 0x0200001C RID: 28
    internal sealed class CharVariant : VariantBase // \u0005\u2000
    {
	    // Token: 0x060000A9 RID: 169 RVA: 0x000042D0 File Offset: 0x000024D0
	    public char GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060000AA RID: 170 RVA: 0x000042D8 File Offset: 0x000024D8
	    public void SetValue(char val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x060000AB RID: 171 RVA: 0x000042E4 File Offset: 0x000024E4
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x060000AC RID: 172 RVA: 0x000042F4 File Offset: 0x000024F4
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue(Convert.ToChar(val));
	    }

	    // Token: 0x060000AD RID: 173 RVA: 0x00004304 File Offset: 0x00002504
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc6Char;
	    }

	    // Token: 0x060000AE RID: 174 RVA: 0x00004308 File Offset: 0x00002508
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToChar(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToChar(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc6Char:
			    SetValue(((CharVariant)val).GetValue());
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((char)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((char)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((char)((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((char)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((char)((uint)((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue((char)((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((char)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((char)((int)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToChar(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((char)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((char)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x060000AF RID: 175 RVA: 0x000044E0 File Offset: 0x000026E0
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new CharVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000026 RID: 38
	    private char _value;
    }

    // Token: 0x0200001D RID: 29
    internal sealed class LongVariant : VariantBase // \u0005\u2001
    {
	    // Token: 0x060000B1 RID: 177 RVA: 0x00004508 File Offset: 0x00002708
	    public long GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060000B2 RID: 178 RVA: 0x00004510 File Offset: 0x00002710
	    public void SetValue(long val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x060000B3 RID: 179 RVA: 0x0000451C File Offset: 0x0000271C
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x060000B4 RID: 180 RVA: 0x0000452C File Offset: 0x0000272C
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is ulong)
		    {
			    SetValue((long)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((long)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((long)((double)val));
			    return;
		    }
		    SetValue(Convert.ToInt64(val));
	    }

	    // Token: 0x060000B5 RID: 181 RVA: 0x00004588 File Offset: 0x00002788
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new LongVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x060000B6 RID: 182 RVA: 0x000045A8 File Offset: 0x000027A8
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc24Long;
	    }

	    // Token: 0x060000B7 RID: 183 RVA: 0x000045AC File Offset: 0x000027AC
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToInt64(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((long)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((long)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((long)((ulong)((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((long)((IntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToInt64(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue(((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((long)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue(((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000027 RID: 39
	    private long _value;
    }

    // Token: 0x0200001E RID: 30
    internal sealed class UIntPtrVariant : VariantBase // \u0005\u2002
    {
	    // Token: 0x060000B9 RID: 185 RVA: 0x000047A4 File Offset: 0x000029A4
	    public UIntPtr GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060000BA RID: 186 RVA: 0x000047AC File Offset: 0x000029AC
	    public void SetValue(UIntPtr val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x060000BB RID: 187 RVA: 0x000047B8 File Offset: 0x000029B8
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new UIntPtrVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x060000BC RID: 188 RVA: 0x000047D8 File Offset: 0x000029D8
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x060000BD RID: 189 RVA: 0x000047E8 File Offset: 0x000029E8
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue((UIntPtr)val);
	    }

	    // Token: 0x060000BE RID: 190 RVA: 0x000047F8 File Offset: 0x000029F8
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc13UIntPtr;
	    }

	    // Token: 0x060000BF RID: 191 RVA: 0x000047FC File Offset: 0x000029FC
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc5Enum:
			    SetValue(new UIntPtr(Convert.ToUInt64(((EnumVariant)val).GetValue())));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((UIntPtr)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((UIntPtr)((ulong)((FloatVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((UIntPtr)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((UIntPtr)((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue(((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue((UIntPtr)((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((UIntPtr)((ulong)((IntVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((UIntPtr)((ulong)((DoubleVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((UIntPtr)((ulong)((LongVariant)val).GetValue()));
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000028 RID: 40
	    private UIntPtr _value;
    }

    // Token: 0x02000029 RID: 41
    internal sealed class DoubleVariant : VariantBase // \u0006\u2000
    {
	    // Token: 0x0600012E RID: 302 RVA: 0x00005F98 File Offset: 0x00004198
	    public double GetValue()
	    {
		    return _value;
	    }

	    // Token: 0x0600012F RID: 303 RVA: 0x00005FA0 File Offset: 0x000041A0
	    public void SetValue(double val)
	    {
		    _value = val;
	    }

	    // Token: 0x06000130 RID: 304 RVA: 0x00005FAC File Offset: 0x000041AC
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000131 RID: 305 RVA: 0x00005FBC File Offset: 0x000041BC
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue(Convert.ToDouble(val));
	    }

	    // Token: 0x06000132 RID: 306 RVA: 0x00005FCC File Offset: 0x000041CC
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new DoubleVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000133 RID: 307 RVA: 0x00005FEC File Offset: 0x000041EC
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc21Double;
	    }

	    // Token: 0x06000134 RID: 308 RVA: 0x00005FF0 File Offset: 0x000041F0
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc7Ulong:
			    SetValue(((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue(((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue((double)((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc19Int:
			    SetValue(((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue(((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue(((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0400004C RID: 76
	    private double _value;
    }

    // Token: 0x0200002A RID: 42
    internal sealed class UshortVariant : VariantBase // \u0006\u2001
    {
	    // Token: 0x06000136 RID: 310 RVA: 0x00006164 File Offset: 0x00004364
	    public ushort GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000137 RID: 311 RVA: 0x0000616C File Offset: 0x0000436C
	    public void SetValue(ushort val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000138 RID: 312 RVA: 0x00006178 File Offset: 0x00004378
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000139 RID: 313 RVA: 0x00006188 File Offset: 0x00004388
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is short)
		    {
			    SetValue((ushort)((short)val));
			    return;
		    }
		    if (val is int)
		    {
			    SetValue((ushort)((int)val));
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((ushort)((long)val));
			    return;
		    }
		    if (val is uint)
		    {
			    SetValue((ushort)((uint)val));
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((ushort)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((ushort)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((ushort)((double)val));
			    return;
		    }
		    SetValue(Convert.ToUInt16(val));
	    }

	    // Token: 0x0600013A RID: 314 RVA: 0x0000623C File Offset: 0x0000443C
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new UshortVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0600013B RID: 315 RVA: 0x0000625C File Offset: 0x0000445C
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc10Ushort;
	    }

	    // Token: 0x0600013C RID: 316 RVA: 0x00006260 File Offset: 0x00004460
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToUInt16(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((ushort)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((ushort)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((ushort)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((ushort)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((ushort)((uint)((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((ushort)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((ushort)((int)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToUInt16(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((ushort)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((ushort)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((ushort)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0400004D RID: 77
	    private ushort _value;
    }

    // Token: 0x02000043 RID: 67
    internal sealed class EnumVariant : VariantBase // \u0008\u2000
    {
	    // Token: 0x060002E6 RID: 742 RVA: 0x00013F1C File Offset: 0x0001211C
	    public EnumVariant(Enum val)
	    {
		    if (val == null)
		    {
			    throw new ArgumentException();
		    }
		    _value = val;
	    }

	    // Token: 0x060002E7 RID: 743 RVA: 0x00013F34 File Offset: 0x00012134
	    public Enum GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060002E8 RID: 744 RVA: 0x00013F3C File Offset: 0x0001213C
	    public void SetValue(Enum val) // \u0002
	    {
		    if (val == null)
		    {
			    throw new ArgumentException();
		    }
		    _value = val;
	    }

	    // Token: 0x060002E9 RID: 745 RVA: 0x00013F50 File Offset: 0x00012150
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x060002EA RID: 746 RVA: 0x00013F58 File Offset: 0x00012158
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue((Enum)Enum.Parse(GetValue().GetType(), val.ToString()));
	    }

	    // Token: 0x060002EB RID: 747 RVA: 0x00013F88 File Offset: 0x00012188
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc5Enum;
	    }

	    // Token: 0x060002EC RID: 748 RVA: 0x00013F8C File Offset: 0x0001218C
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    switch (num)
		    {
		    case Vtc.Tc5Enum:
		    {
			    var type = _value.GetType();
			    var @enum = ((EnumVariant)val).GetValue();
			    if (@enum.GetType() == type)
			    {
				    SetValue(@enum);
				    return this;
			    }
			    SetValue((Enum)Enum.ToObject(type, @enum));
			    return this;
		    }
		    case Vtc.Tc6Char:
		    case Vtc.Tc8Float:
		    case Vtc.Tc11ValueType:
		    case Vtc.Tc13UIntPtr:
		    case Vtc.Tc16String:
		    case Vtc.Tc17IntPtr:
			    break;
		    case Vtc.Tc7Ulong:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((UlongVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((UintVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((UshortVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((SbyteVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((ByteVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((ShortVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((Enum)Enum.ToObject(_value.GetType(), ((IntVariant)val).GetValue()));
			    return this;
		    default:
			    if (num == Vtc.Tc24Long)
			    {
				    SetValue((Enum)Enum.ToObject(_value.GetType(), ((LongVariant)val).GetValue()));
				    return this;
			    }
			    break;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x060002ED RID: 749 RVA: 0x000141C0 File Offset: 0x000123C0
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new EnumVariant(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000172 RID: 370
	    private Enum _value;
    }

    // Token: 0x02000044 RID: 68
    internal sealed class SbyteVariant : VariantBase // \u0008\u2001
    {
	    // Token: 0x060002EF RID: 751 RVA: 0x000141E4 File Offset: 0x000123E4
	    public sbyte GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060002F0 RID: 752 RVA: 0x000141EC File Offset: 0x000123EC
	    public void SetValue(sbyte val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x060002F1 RID: 753 RVA: 0x000141F8 File Offset: 0x000123F8
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x060002F2 RID: 754 RVA: 0x00014208 File Offset: 0x00012408
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is byte)
		    {
			    SetValue((sbyte)((byte)val));
			    return;
		    }
		    if (val is short)
		    {
			    SetValue((sbyte)((short)val));
			    return;
		    }
		    if (val is int)
		    {
			    SetValue((sbyte)((int)val));
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((sbyte)((long)val));
			    return;
		    }
		    if (val is ushort)
		    {
			    SetValue((sbyte)((ushort)val));
			    return;
		    }
		    if (val is uint)
		    {
			    SetValue((sbyte)((uint)val));
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((sbyte)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((sbyte)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((sbyte)((double)val));
			    return;
		    }
		    SetValue(Convert.ToSByte(val));
	    }

	    // Token: 0x060002F3 RID: 755 RVA: 0x000142E8 File Offset: 0x000124E8
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new SbyteVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x060002F4 RID: 756 RVA: 0x00014308 File Offset: 0x00012508
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc12Sbyte;
	    }

	    // Token: 0x060002F5 RID: 757 RVA: 0x0001430C File Offset: 0x0001250C
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToSByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToSByte(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((sbyte)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((sbyte)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((sbyte)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((sbyte)((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue((sbyte)((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((sbyte)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((sbyte)((int)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToSByte(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((sbyte)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((sbyte)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((sbyte)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000173 RID: 371
	    private sbyte _value;
    }

    // Token: 0x0200004E RID: 78
    internal sealed class FloatVariant : VariantBase // \u000E\u2000
    {
	    // Token: 0x06000326 RID: 806 RVA: 0x00014CA4 File Offset: 0x00012EA4
	    public float GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000327 RID: 807 RVA: 0x00014CAC File Offset: 0x00012EAC
	    public void SetValue(float val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000328 RID: 808 RVA: 0x00014CB8 File Offset: 0x00012EB8
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000329 RID: 809 RVA: 0x00014CC8 File Offset: 0x00012EC8
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetValue(Convert.ToSingle(val));
	    }

	    // Token: 0x0600032A RID: 810 RVA: 0x00014CD8 File Offset: 0x00012ED8
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new FloatVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0600032B RID: 811 RVA: 0x00014CF8 File Offset: 0x00012EF8
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc8Float;
	    }

	    // Token: 0x0600032C RID: 812 RVA: 0x00014CFC File Offset: 0x00012EFC
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToSingle(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue(((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue(((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue(((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue(((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc18Object:
			    SetValue((float)((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc19Int:
			    SetValue(((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((float)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue(((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0400017E RID: 382
	    private float _value;
    }

    // Token: 0x0200004F RID: 79
    internal sealed class UintVariant : VariantBase // \u000E\u2001
    {
	    // Token: 0x0600032E RID: 814 RVA: 0x00014E94 File Offset: 0x00013094
	    public uint GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x0600032F RID: 815 RVA: 0x00014E9C File Offset: 0x0001309C
	    public void SetValue(uint val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000330 RID: 816 RVA: 0x00014EA8 File Offset: 0x000130A8
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000331 RID: 817 RVA: 0x00014EB8 File Offset: 0x000130B8
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is short)
		    {
			    SetValue((uint)((short)val));
			    return;
		    }
		    if (val is int)
		    {
			    SetValue((uint)((int)val));
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((uint)((long)val));
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((uint)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((uint)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((uint)((double)val));
			    return;
		    }
		    SetValue(Convert.ToUInt32(val));
	    }

	    // Token: 0x06000332 RID: 818 RVA: 0x00014F54 File Offset: 0x00013154
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new UintVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000333 RID: 819 RVA: 0x00014F74 File Offset: 0x00013174
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc9Uint;
	    }

	    // Token: 0x06000334 RID: 820 RVA: 0x00014F78 File Offset: 0x00013178
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToUInt32(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((uint)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((uint)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((uint)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((uint)((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((uint)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((uint)((int)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToUInt32(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((uint)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((uint)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((uint)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0400017F RID: 383
	    private uint _value;
    }

    // Token: 0x0200005A RID: 90
    internal sealed class ByteVariant : VariantBase // \u000F\u2000
    {
	    // Token: 0x06000353 RID: 851 RVA: 0x0001540C File Offset: 0x0001360C
	    public byte GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x06000354 RID: 852 RVA: 0x00015414 File Offset: 0x00013614
	    public void SetValue(byte val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x06000355 RID: 853 RVA: 0x00015420 File Offset: 0x00013620
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new ByteVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000356 RID: 854 RVA: 0x00015440 File Offset: 0x00013640
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x06000357 RID: 855 RVA: 0x00015450 File Offset: 0x00013650
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is short)
		    {
			    SetValue((byte)((short)val));
			    return;
		    }
		    if (val is int)
		    {
			    SetValue((byte)((int)val));
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((byte)((long)val));
			    return;
		    }
		    if (val is ushort)
		    {
			    SetValue((byte)((ushort)val));
			    return;
		    }
		    if (val is uint)
		    {
			    SetValue((byte)((uint)val));
			    return;
		    }
		    if (val is ulong)
		    {
			    SetValue((byte)((ulong)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((byte)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((byte)((double)val));
			    return;
		    }
		    SetValue(Convert.ToByte(val));
	    }

	    // Token: 0x06000358 RID: 856 RVA: 0x0001551C File Offset: 0x0001371C
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc14Byte;
	    }

	    // Token: 0x06000359 RID: 857 RVA: 0x00015520 File Offset: 0x00013720
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToByte(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue((byte)((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((byte)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue((byte)((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue((byte)((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((byte)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((byte)((uint)((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((byte)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((byte)((int)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToByte(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((byte)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((byte)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((byte)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000187 RID: 391
	    private byte _value;
    }

    // Token: 0x0200005B RID: 91
    internal sealed class UlongVariant : VariantBase // \u000F\u2001
    {
	    // Token: 0x0600035B RID: 859 RVA: 0x00015718 File Offset: 0x00013918
	    public ulong GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x0600035C RID: 860 RVA: 0x00015720 File Offset: 0x00013920
	    public void SetValue(ulong val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x0600035D RID: 861 RVA: 0x0001572C File Offset: 0x0001392C
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetValue();
	    }

	    // Token: 0x0600035E RID: 862 RVA: 0x0001573C File Offset: 0x0001393C
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    if (val is short)
		    {
			    SetValue((ulong)(short)val);
			    return;
		    }
		    if (val is int)
		    {
			    SetValue((ulong)(int)val);
			    return;
		    }
		    if (val is long)
		    {
			    SetValue((ulong)((long)val));
			    return;
		    }
		    if (val is float)
		    {
			    SetValue((ulong)((float)val));
			    return;
		    }
		    if (val is double)
		    {
			    SetValue((ulong)((double)val));
			    return;
		    }
		    SetValue(Convert.ToUInt64(val));
	    }

	    // Token: 0x0600035F RID: 863 RVA: 0x000157C4 File Offset: 0x000139C4
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new UlongVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000360 RID: 864 RVA: 0x000157E4 File Offset: 0x000139E4
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc7Ulong;
	    }

	    // Token: 0x06000361 RID: 865 RVA: 0x000157E8 File Offset: 0x000139E8
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    switch (val.GetTypeCode())
		    {
		    case Vtc.Tc1Bool:
			    SetValue(Convert.ToByte(((BoolVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc5Enum:
			    SetValue(Convert.ToUInt64(((EnumVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc7Ulong:
			    SetValue(((UlongVariant)val).GetValue());
			    return this;
		    case Vtc.Tc8Float:
			    SetValue((ulong)((FloatVariant)val).GetValue());
			    return this;
		    case Vtc.Tc9Uint:
			    SetValue(((UintVariant)val).GetValue());
			    return this;
		    case Vtc.Tc10Ushort:
			    SetValue(((UshortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc12Sbyte:
			    SetValue((ulong)((SbyteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc13UIntPtr:
			    SetValue((ulong)((UIntPtrVariant)val).GetValue());
			    return this;
		    case Vtc.Tc14Byte:
			    SetValue(((ByteVariant)val).GetValue());
			    return this;
		    case Vtc.Tc15Short:
			    SetValue((ulong)((ShortVariant)val).GetValue());
			    return this;
		    case Vtc.Tc17IntPtr:
			    SetValue((ulong)((long)((IntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc18Object:
			    SetValue(Convert.ToUInt64(((UIntPtrVariant)val).GetValue()));
			    return this;
		    case Vtc.Tc19Int:
			    SetValue((ulong)((IntVariant)val).GetValue());
			    return this;
		    case Vtc.Tc21Double:
			    SetValue((ulong)((DoubleVariant)val).GetValue());
			    return this;
		    case Vtc.Tc24Long:
			    SetValue((ulong)((LongVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x04000188 RID: 392
	    private ulong _value;
    }

    // Token: 0x0200005C RID: 92
    internal abstract class ReferenceVariantBase : VariantBase // \u000F\u2002
    {
	    // Token: 0x06000363 RID: 867 RVA: 0x000159E0 File Offset: 0x00013BE0
	    public override object GetValueAbstract() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    throw new InvalidOperationException();
	    }

	    // Token: 0x06000364 RID: 868 RVA: 0x000159E8 File Offset: 0x00013BE8
	    public override void SetValueAbstract(object val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    throw new InvalidOperationException();
	    }

	    // Token: 0x06000365 RID: 869 RVA: 0x000159F0 File Offset: 0x00013BF0
	    public override bool IsAddr() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return true;
	    }
    }

    // Token: 0x02000007 RID: 7
    internal sealed class FieldInfoVariant : ReferenceVariantBase // \u0002\u2004
    {
	    // Token: 0x06000023 RID: 35 RVA: 0x00002600 File Offset: 0x00000800
	    public object GetObject() // \u0002
	    {
		    return _obj;
	    }

	    // Token: 0x06000024 RID: 36 RVA: 0x00002608 File Offset: 0x00000808
	    public void SetObject(object val) // \u0002
	    {
		    _obj = val;
	    }

	    // Token: 0x06000025 RID: 37 RVA: 0x00002614 File Offset: 0x00000814
	    public FieldInfo GetValue() // \u0002
	    {
		    return _field;
	    }

	    // Token: 0x06000026 RID: 38 RVA: 0x0000261C File Offset: 0x0000081C
	    public void SetValue(FieldInfo val) // \u0002
	    {
		    _field = val;
	    }

	    // Token: 0x06000027 RID: 39 RVA: 0x00002628 File Offset: 0x00000828
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc4FieldInfo;
	    }

	    // Token: 0x06000028 RID: 40 RVA: 0x0000262C File Offset: 0x0000082C
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num == Vtc.Tc4FieldInfo)
		    {
			    SetObject(((FieldInfoVariant)val).GetObject());
			    SetValue(((FieldInfoVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x06000029 RID: 41 RVA: 0x0000267C File Offset: 0x0000087C
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new FieldInfoVariant();
		    ret.SetObject(_obj);
		    ret.SetValue(_field);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000005 RID: 5
	    private object _obj; // \u0002

	    // Token: 0x04000006 RID: 6
        private FieldInfo _field; // \u0003
    }

    // Token: 0x02000012 RID: 18
    internal sealed class VariantBaseHolder : ReferenceVariantBase // \u0003\u2003
    {
	    // Token: 0x0600007A RID: 122 RVA: 0x00003BF0 File Offset: 0x00001DF0
	    public VariantBase GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x0600007B RID: 123 RVA: 0x00003BF8 File Offset: 0x00001DF8
	    public void SetValue(VariantBase val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x0600007C RID: 124 RVA: 0x00003C04 File Offset: 0x00001E04
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc0VariantBaseHolder;
	    }

	    // Token: 0x0600007D RID: 125 RVA: 0x00003C08 File Offset: 0x00001E08
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    if (val.GetTypeCode() == Vtc.Tc0VariantBaseHolder)
		    {
			    SetValue(((VariantBaseHolder)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0600007E RID: 126 RVA: 0x00003C48 File Offset: 0x00001E48
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new VariantBaseHolder();
		    ret.SetValue(GetValue());
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0400001A RID: 26
	    private VariantBase _value;
    }

    // Token: 0x02000020 RID: 32
    internal sealed class LocalsIdxHolderVariant : ReferenceVariantBase // \u0005\u2004
    {
	    // Token: 0x060000C6 RID: 198 RVA: 0x000049A8 File Offset: 0x00002BA8
	    public int GetValue() // \u0002
	    {
		    return _value;
	    }

	    // Token: 0x060000C7 RID: 199 RVA: 0x000049B0 File Offset: 0x00002BB0
	    public void SetValue(int val) // \u0002
	    {
		    _value = val;
	    }

	    // Token: 0x060000C8 RID: 200 RVA: 0x000049BC File Offset: 0x00002BBC
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc23LocalsIdxHolder;
	    }

	    // Token: 0x060000C9 RID: 201 RVA: 0x000049C0 File Offset: 0x00002BC0
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num == Vtc.Tc23LocalsIdxHolder)
		    {
			    SetValue(((LocalsIdxHolderVariant)val).GetValue());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x060000CA RID: 202 RVA: 0x00004A00 File Offset: 0x00002C00
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new LocalsIdxHolderVariant();
		    ret.SetValue(_value);
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x04000029 RID: 41
	    private int _value;
    }

    // Token: 0x02000046 RID: 70
    internal abstract class ArrayValueVariantBase : ReferenceVariantBase // \u0008\u2003
    {
	    // Token: 0x060002F9 RID: 761 RVA: 0x00014788 File Offset: 0x00012988
	    public Type GetHeldType() // \u0002
	    {
		    return _type;
	    }

	    // Token: 0x060002FA RID: 762 RVA: 0x00014790 File Offset: 0x00012990
	    public void SetHeldType(Type val) // \u0002
	    {
		    _type = val;
	    }

	    // Token: 0x060002FB RID: 763
	    public abstract object GetValue(); // \u0008\u2003\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x060002FC RID: 764
	    public abstract void SetValue(object val); // \u0008\u2003\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x060002FD RID: 765
	    public abstract bool IsEqual(ArrayValueVariantBase val); // \u0008\u2003\u2008\u2000\u2002\u200A\u0002

	    // Token: 0x04000174 RID: 372
	    private Type _type;
    }

    // Token: 0x02000053 RID: 83
    internal static class IntArrayComparison // \u000E\u2004
    {
	    // Token: 0x06000340 RID: 832 RVA: 0x00015294 File Offset: 0x00013494
	    public static bool Execute(int[] a1, int[] a2) // \u0002
	    {
		    if (a1 == a2)
		    {
			    return true;
		    }
		    if (a1 == null || a2 == null)
		    {
			    return false;
		    }
		    if (a1.Length != a2.Length)
		    {
			    return false;
		    }
	        return !a1.Where((t, i) => t != a2[i]).Any();
	    }
    }

    // Token: 0x0200002D RID: 45
    internal sealed class MdArrayValueVariant : ArrayValueVariantBase // \u0006\u2004
    {
	    // Token: 0x0600014F RID: 335 RVA: 0x000065AC File Offset: 0x000047AC
	    public Array GetArray() // \u0002
	    {
		    return _array;
	    }

	    // Token: 0x06000150 RID: 336 RVA: 0x000065B4 File Offset: 0x000047B4
	    public void SetArray(Array val) // \u0002
	    {
		    _array = val;
	    }

	    // Token: 0x06000151 RID: 337 RVA: 0x000065C0 File Offset: 0x000047C0
	    public int[] GetIndexes() // \u0002
	    {
		    return _indexes;
	    }

	    // Token: 0x06000152 RID: 338 RVA: 0x000065C8 File Offset: 0x000047C8
	    public void SetIndexes(int[] val) // \u0002
	    {
		    _indexes = val;
	    }

	    // Token: 0x06000153 RID: 339 RVA: 0x000065D4 File Offset: 0x000047D4
	    public override object GetValue() // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    return GetArray().GetValue(GetIndexes());
	    }

	    // Token: 0x06000154 RID: 340 RVA: 0x000065E8 File Offset: 0x000047E8
	    public override void SetValue(object val) // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    GetArray().SetValue(val, GetIndexes());
	    }

	    // Token: 0x06000155 RID: 341 RVA: 0x000065FC File Offset: 0x000047FC
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new MdArrayValueVariant();
		    ret.SetArray(GetArray());
		    ret.SetIndexes(GetIndexes());
		    ret.SetHeldType(GetHeldType());
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x06000156 RID: 342 RVA: 0x00006640 File Offset: 0x00004840
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc20MdArrayValue;
	    }

	    // Token: 0x06000157 RID: 343 RVA: 0x00006644 File Offset: 0x00004844
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num == Vtc.Tc20MdArrayValue)
		    {
			    var src = (MdArrayValueVariant)val;
			    SetArray(src.GetArray());
			    SetIndexes(src.GetIndexes());
			    SetHeldType(src.GetHeldType());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x06000158 RID: 344 RVA: 0x000066A0 File Offset: 0x000048A0
	    public override bool IsEqual(ArrayValueVariantBase val) // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    var peer = (MdArrayValueVariant)val;
		    return GetArray() == peer.GetArray() && IntArrayComparison.Execute(GetIndexes(), peer.GetIndexes());
	    }

	    // Token: 0x04000050 RID: 80
	    private Array _array; // \u0002

	    // Token: 0x04000051 RID: 81
	    private int[] _indexes; // \u0003
    }

    // Token: 0x02000052 RID: 82
    internal sealed class SdArrayValueVariant : ArrayValueVariantBase // \u000E\u2003
    {
	    // Token: 0x06000336 RID: 822 RVA: 0x0001516C File Offset: 0x0001336C
	    public Array GetArray() // \u0002
	    {
		    return _array;
	    }

	    // Token: 0x06000337 RID: 823 RVA: 0x00015174 File Offset: 0x00013374
	    public void SetArray(Array val) // \u0002
	    {
		    _array = val;
	    }

	    // Token: 0x06000338 RID: 824 RVA: 0x00015180 File Offset: 0x00013380
	    public long GetIndex() // \u0002
	    {
		    return _index;
	    }

	    // Token: 0x06000339 RID: 825 RVA: 0x00015188 File Offset: 0x00013388
	    public void SetIndex(long idx) // \u0002
	    {
		    _index = idx;
	    }

	    // Token: 0x0600033A RID: 826 RVA: 0x00015194 File Offset: 0x00013394
	    public override object GetValue() // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    return _array.GetValue(_index);
	    }

	    // Token: 0x0600033B RID: 827 RVA: 0x000151A8 File Offset: 0x000133A8
	    public override void SetValue(object val) // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    _array.SetValue(val, _index);
	    }

	    // Token: 0x0600033C RID: 828 RVA: 0x000151BC File Offset: 0x000133BC
	    public override Vtc GetTypeCode() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    return Vtc.Tc22SdArrayValue;
	    }

	    // Token: 0x0600033D RID: 829 RVA: 0x000151C0 File Offset: 0x000133C0
	    public override VariantBase CopyFrom(VariantBase val) // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    SetVariantType(val.GetVariantType());
		    var num = val.GetTypeCode();
		    if (num == Vtc.Tc22SdArrayValue)
		    {
			    var src = (SdArrayValueVariant)val;
			    SetArray(src.GetArray());
			    SetIndex(src.GetIndex());
			    SetHeldType(src.GetHeldType());
			    return this;
		    }
		    throw new ArgumentOutOfRangeException();
	    }

	    // Token: 0x0600033E RID: 830 RVA: 0x0001521C File Offset: 0x0001341C
	    public override VariantBase Clone() // \u000F\u2008\u2000\u2002\u200A\u0002
	    {
		    var ret = new SdArrayValueVariant();
		    ret.SetArray(_array);
		    ret.SetIndex(_index);
		    ret.SetHeldType(GetHeldType());
		    ret.SetVariantType(GetVariantType());
		    return ret;
	    }

	    // Token: 0x0600033F RID: 831 RVA: 0x00015260 File Offset: 0x00013460
	    public override bool IsEqual(ArrayValueVariantBase val) // \u0008\u2003\u2008\u2000\u2002\u200A\u0002
	    {
		    var peer = (SdArrayValueVariant)val;
		    return GetIndex() == peer.GetIndex() && GetArray() == peer.GetArray();
	    }

	    // Token: 0x04000181 RID: 385
	    private Array _array; // \u0002

	    // Token: 0x04000182 RID: 386
	    private long _index; // \u0003
    }
}
