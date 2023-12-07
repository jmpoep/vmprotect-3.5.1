using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace VMProtect
{
	[VMProtect.DeleteOnCompilation]
	internal enum OpCodes
	{
		icUnknown, icByte, icWord, icDword, icQword, icComment, icData, icCase,
		icNop, icBreak, icLdarg_0, icLdarg_1, icLdarg_2, icLdarg_3, icLdloc_0, icLdloc_1, icLdloc_2, icLdloc_3, icStloc_0, icStloc_1,
		icStloc_2, icStloc_3, icLdarg_s, icLdarga_s, icStarg_s, icLdloc_s, icLdloca_s, icStloc_s, icLdnull, icLdc_i4_m1, icLdc_i4_0,
		icLdc_i4_1, icLdc_i4_2, icLdc_i4_3, icLdc_i4_4, icLdc_i4_5, icLdc_i4_6, icLdc_i4_7, icLdc_i4_8, icLdc_i4_s, icLdc_i4, icLdc_i8,
		icLdc_r4, icLdc_r8, icDup, icPop, icJmp, icCall, icCalli, icRet, icBr_s, icBrfalse_s, icBrtrue_s, icBeq_s, icBge_s, icBgt_s, icBle_s,
		icBlt_s, icBne_un_s, icBge_un_s, icBgt_un_s, icBle_un_s, icBlt_un_s, icBr, icBrfalse, icBrtrue, icBeq, icBge, icBgt, icBle, icBlt,
		icBne_un, icBge_un, icBgt_un, icBle_un, icBlt_un, icSwitch, icLdind_i1, icLdind_u1, icLdind_i2, icLdind_u2, icLdind_i4, icLdind_u4,
		icLdind_i8, icLdind_i, icLdind_r4, icLdind_r8, icLdind_ref, icStind_ref, icStind_i1, icStind_i2, icStind_i4, icStind_i8, icStind_r4,
		icStind_r8, icAdd, icSub, icMul, icDiv, icDiv_un, icRem, icRem_un, icAnd, icOr, icXor, icShl, icShr, icShr_un, icNeg, icNot, icConv_i1, icConv_i2,
		icConv_i4, icConv_i8, icConv_r4, icConv_r8, icConv_u4, icConv_u8, icCallvirt, icCpobj, icLdobj, icLdstr, icNewobj, icCastclass,
		icIsinst, icConv_r_un, icUnbox, icThrow, icLdfld, icLdflda, icStfld, icLdsfld, icLdsflda, icStsfld, icStobj, icConv_ovf_i1_un,
		icConv_ovf_i2_un, icConv_ovf_i4_un, icConv_ovf_i8_un, icConv_ovf_u1_un, icConv_ovf_u2_un, icConv_ovf_u4_un, icConv_ovf_u8_un,
		icConv_ovf_i_un, icConv_ovf_u_un, icBox, icNewarr, icLdlen, icLdelema, icLdelem_i1, icLdelem_u1, icLdelem_i2, icLdelem_u2,
		icLdelem_i4, icLdelem_u4, icLdelem_i8, icLdelem_i, icLdelem_r4, icLdelem_r8, icLdelem_ref, icStelem_i, icStelem_i1, icStelem_i2,
		icStelem_i4, icStelem_i8, icStelem_r4, icStelem_r8, icStelem_ref, icLdelem, icStelem, icUnbox_any, icConv_ovf_i1, icConv_ovf_u1,
		icConv_ovf_i2, icConv_ovf_u2, icConv_ovf_i4, icConv_ovf_u4, icConv_ovf_i8, icConv_ovf_u8, icRefanyval, icCkfinite, icMkrefany,
		icLdtoken, icConv_u2, icConv_u1, icConv_i, icConv_ovf_i, icConv_ovf_u, icAdd_ovf, icAdd_ovf_un, icMul_ovf, icMul_ovf_un, icSub_ovf,
		icSub_ovf_un, icEndfinally, icLeave, icLeave_s, icStind_i, icConv_u, icArglist, icCeq, icCgt, icCgt_un, icClt, icClt_un, icLdftn, icLdvirtftn,
		icLdarg, icLdarga, icStarg, icLdloc, icLdloca, icStloc, icLocalloc, icEndfilter, icUnaligned, icVolatile, icTail, icInitobj, icConstrained,
		icCpblk, icInitblk, icNo, icRethrow, icSizeof, icRefanytype, icReadonly,
		icConv, icConv_ovf, icConv_ovf_un, icLdmem_i4, icInitarg, icInitcatchblock, icEntertry, icCallvm, icCallvmvirt,
		icCmp, icCmp_un
	}

	public class VirtualMachine
	{
		static class Utils
		{
			public static int Random()
			{
				return new Random().Next();
			}

			public static int CalcCRC(uint offset, uint size)
			{
				long instance = Marshal.GetHINSTANCE(typeof(CRC32).Module).ToInt64();
				return (int)new CRC32().Hash(new IntPtr(instance + offset), size);
			}

			unsafe public static object BoxPointer(void* ptr)
			{
				return Pointer.Box(ptr, typeof(void*));
			}

			unsafe public static void* UnboxPointer(object ptr)
			{
				return Pointer.Unbox(ptr);
			}
		}

		abstract class BaseVariant
		{
			public abstract BaseVariant Clone();
			public abstract object Value();
			public abstract void SetValue(object value);
			public virtual void SetFieldValue(FieldInfo field, object value)
			{
				var obj = Value();
				field.SetValue(obj, value);
			}

			public virtual bool IsReference()
			{
				return false;
			}

			public virtual StackableVariant ToStack()
			{
				throw new InvalidOperationException();
			}

			public virtual BaseVariant ToUnsigned()
			{
				return this;
			}

			public virtual Type Type()
			{
				throw new InvalidOperationException();
			}

			public virtual TypeCode CalcTypeCode()
			{
				throw new InvalidOperationException();
			}

			public virtual bool ToBoolean()
			{
				return System.Convert.ToBoolean(Value());
			}
			public virtual sbyte ToSByte()
			{
				return System.Convert.ToSByte(Value());
			}
			public virtual short ToInt16()
			{
				return System.Convert.ToInt16(Value());
			}
			public virtual int ToInt32()
			{
				return System.Convert.ToInt32(Value());
			}
			public virtual long ToInt64()
			{
				return System.Convert.ToInt64(Value());
			}
			public virtual char ToChar()
			{
				return System.Convert.ToChar(Value());
			}
			public virtual byte ToByte()
			{
				return System.Convert.ToByte(Value());
			}
			public virtual ushort ToUInt16()
			{
				return System.Convert.ToUInt16(Value());
			}
			public virtual uint ToUInt32()
			{
				return System.Convert.ToUInt32(Value());
			}
			public virtual ulong ToUInt64()
			{
				return System.Convert.ToUInt64(Value());
			}
			public virtual float ToSingle()
			{
				return System.Convert.ToSingle(Value());
			}
			public virtual double ToDouble()
			{
				return System.Convert.ToDouble(Value());
			}

			public override string ToString()
			{
				var v = Value();
				return v != null ? System.Convert.ToString(v) : null;
			}

			public virtual IntPtr ToIntPtr()
			{
				var value = Value();
				if (value?.GetType() == typeof(IntPtr))
					return (IntPtr)value;

				throw new InvalidOperationException();
			}

			public virtual UIntPtr ToUIntPtr()
			{
				var value = Value();
				if (value?.GetType() == typeof(UIntPtr))
					return (UIntPtr)value;

				throw new InvalidOperationException();
			}

			public virtual unsafe void* ToPointer()
			{
				throw new InvalidOperationException();
			}

			public virtual object Conv_ovf(Type type, bool un)
			{
				throw new InvalidOperationException();
			}
		}

		abstract class StackableVariant : BaseVariant
		{
			public override StackableVariant ToStack()
			{
				return this;
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Empty;
			}
		}

		sealed class IntVariant : StackableVariant
		{
			private int _value;
			public IntVariant(int value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(int);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Int32;
			}

			public override BaseVariant Clone()
			{
				return new IntVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToInt32(value);
			}

			public override bool ToBoolean()
			{
				return _value != 0;
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}
			public override int ToInt32()
			{
				return _value;
			}

			public override long ToInt64()
			{
				return (long)_value;
			}

			public override char ToChar()
			{
				return (char)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}
			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}

			public override ulong ToUInt64()
			{
				return (uint)_value;
			}

			public override float ToSingle()
			{
				return _value;
			}

			public override double ToDouble()
			{
				return (double)_value;
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(_value);
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr((uint)_value);
			}

			public override BaseVariant ToUnsigned()
			{
				return new UintVariant((uint)_value);
			}

			public override object Conv_ovf(Type type, bool un)
			{
				if (type == typeof(IntPtr))
				{
					if (IntPtr.Size == 4)
						return new IntPtr(un ? checked((int)(uint)_value) : _value);
					return new IntPtr(un ? (long)(uint)_value : _value);
				}
				if (type == typeof(UIntPtr))
					return new UIntPtr(un ? (uint)_value : checked((uint)_value));

				switch (System.Type.GetTypeCode(type)) {
					case TypeCode.SByte:
						return un ? checked((sbyte)(uint)_value) : checked((sbyte)_value);
					case TypeCode.Int16:
						return un ? checked((short)(uint)_value) : checked((short)_value);
					case TypeCode.Int32:
						return un ? checked((int)(uint)_value) : _value;
					case TypeCode.Int64:
						return un ? (long)(uint)_value : _value;
					case TypeCode.Byte:
						return un ? checked((byte)(uint)_value) : checked((byte)_value);
					case TypeCode.UInt16:
						return un ? checked((ushort)(uint)_value) : checked((ushort)_value);
					case TypeCode.UInt32:
						return un ? (uint)_value : checked((uint)_value);
					case TypeCode.UInt64:
						return un ? (uint)_value : checked((uint)_value);
					case TypeCode.Double:
						return un ? (double)(uint)_value : (double)_value;
				}

				throw new ArgumentException();
			}
		}

		sealed class LongVariant : StackableVariant
		{
			private long _value;
			public LongVariant(long value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(long);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Int64;
			}

			public override BaseVariant ToUnsigned()
			{
				return new UlongVariant((ulong)_value);
			}

			public override BaseVariant Clone()
			{
				return new LongVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToInt64(value);
			}

			public override bool ToBoolean()
			{
				return _value != 0;
			}

			public override char ToChar()
			{
				return (char)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override int ToInt32()
			{
				return (int)_value;
			}

			public override long ToInt64()
			{
				return _value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}

			public override ulong ToUInt64()
			{
				return (ulong)_value;
			}

			public override float ToSingle()
			{
				return _value;
			}

			public override double ToDouble()
			{
				return (double)_value;
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(IntPtr.Size == 4 ? (int)_value : _value);
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr(UIntPtr.Size == 4 ? (uint)_value : (ulong)_value);
			}

			public override object Conv_ovf(Type type, bool un)
			{
				if (type == typeof(IntPtr))
					return new IntPtr(un ? checked((long)(ulong)_value) : _value);
				if (type == typeof(UIntPtr))
					return new UIntPtr(un ? (ulong)_value : checked((ulong)_value));

				switch (System.Type.GetTypeCode(type))
				{
					case TypeCode.SByte:
						return un ? checked((sbyte)(ulong)_value) : checked((sbyte)_value);
					case TypeCode.Int16:
						return un ? checked((short)(ulong)_value) : checked((short)_value);
					case TypeCode.Int32:
						return un ? checked((int)(ulong)_value) : checked((int)_value);
					case TypeCode.Int64:
						return un ? checked((long)(ulong)_value) : _value;
					case TypeCode.Byte:
						return un ? checked((byte)(ulong)_value) : checked((byte)_value);
					case TypeCode.UInt16:
						return un ? checked((ushort)(uint)_value) : checked((ushort)_value);
					case TypeCode.UInt32:
						return un ? checked((uint)(ulong)_value) : checked((uint)_value);
					case TypeCode.UInt64:
						return un ? (ulong)_value : checked((ulong)_value);
					case TypeCode.Double:
						return un ? (double)(ulong)_value : (double)_value;
				}

				throw new ArgumentException();
			}
		}

		sealed class SingleVariant : StackableVariant
		{
			private float _value;

			public SingleVariant(float value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(float);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Single;
			}

			public override BaseVariant Clone()
			{
				return new SingleVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToSingle(value);
			}

			public override bool ToBoolean()
			{
				return System.Convert.ToBoolean(_value);
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override int ToInt32()
			{
				return (int)_value;
			}

			public override long ToInt64()
			{
				return (long)_value;
			}

			public override char ToChar()
			{
				return (char)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}

			public override ulong ToUInt64()
			{
				return (ulong)_value;
			}

			public override float ToSingle()
			{
				return _value;
			}

			public override double ToDouble()
			{
				return _value;
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(IntPtr.Size == 4 ? (int)_value : (long)_value);
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr(IntPtr.Size == 4 ? (uint)_value : (ulong)_value);
			}

			public override object Conv_ovf(Type type, bool un)
			{
				if (type == typeof(IntPtr))
					return new IntPtr(checked((long)_value));
				if (type == typeof(UIntPtr))
					return new UIntPtr(checked((ulong)_value));

				switch (System.Type.GetTypeCode(type))
				{
					case TypeCode.SByte:
						return un ? checked((sbyte)(uint)_value) : checked((sbyte)_value);
					case TypeCode.Int16:
						return un ? checked((short)(uint)_value) : checked((short)_value);
					case TypeCode.Int32:
						return checked((int)_value);
					case TypeCode.Byte:
						return checked((byte)_value);
					case TypeCode.UInt16:
						return checked((ushort)_value);
					case TypeCode.UInt32:
						return checked((uint)_value);
					case TypeCode.UInt64:
						return checked((ulong)_value);
				}

				throw new ArgumentException();
			}
		}

		sealed class DoubleVariant : StackableVariant
		{
			private double _value;

			public DoubleVariant(double value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(double);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Double;
			}

			public override BaseVariant Clone()
			{
				return new DoubleVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (double)value;
			}

			public override bool ToBoolean()
			{
				return System.Convert.ToBoolean(_value);
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override int ToInt32()
			{
				return (int)_value;
			}

			public override long ToInt64()
			{
				return (long)_value;
			}

			public override char ToChar()
			{
				return (char)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}

			public override ulong ToUInt64()
			{
				return (ulong)_value;
			}

			public override float ToSingle()
			{
				return (float)_value;
			}

			public override double ToDouble()
			{
				return _value;
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(IntPtr.Size == 4 ? (int)_value : (long)_value);
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr(IntPtr.Size == 4 ? (uint)_value : (ulong)_value);
			}

			public override object Conv_ovf(Type type, bool un)
			{
				if (type == typeof(IntPtr))
					return new IntPtr(checked((long)_value));
				if (type == typeof(UIntPtr))
					return new UIntPtr(checked((ulong)_value));

				switch (System.Type.GetTypeCode(type))
				{
					case TypeCode.SByte:
						return un ? checked((sbyte)(uint)_value) : checked((sbyte)_value);
					case TypeCode.Int16:
						return un ? checked((short)(uint)_value) : checked((short)_value);
					case TypeCode.Int32:
						return checked((int)_value);
					case TypeCode.Int64:
						return checked((long)_value);
					case TypeCode.Byte:
						return checked((byte)_value);
					case TypeCode.UInt16:
						return checked((ushort)_value);
					case TypeCode.UInt32:
						return checked((uint)_value);
					case TypeCode.UInt64:
						return checked((ulong)_value);
					case TypeCode.Double:
						return _value;
				}

				throw new ArgumentException();
			}
		}

		sealed class StringVariant : StackableVariant
		{
			private string _value;

			public StringVariant(string value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(string);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Object;
			}

			public override BaseVariant Clone()
			{
				return new StringVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (value != null) ? System.Convert.ToString(value) : null;
			}

			public override bool ToBoolean()
			{
				return _value != null;
			}

			public override string ToString()
			{
				return _value;
			}
		}

		sealed class ShortVariant : BaseVariant
		{
			private short _value;

			public ShortVariant(short value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(short);
			}

			public override BaseVariant Clone()
			{
				return new ShortVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToInt16(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return _value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override int ToInt32()
			{
				return _value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}
		}

		sealed class UshortVariant : BaseVariant
		{
			private ushort _value;

			public UshortVariant(ushort value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(ushort);
			}

			public override BaseVariant Clone()
			{
				return new UshortVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToUInt16(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override ushort ToUInt16()
			{
				return _value;
			}

			public override int ToInt32()
			{
				return _value;
			}

			public override uint ToUInt32()
			{
				return _value;
			}
		}

		sealed class BoolVariant : BaseVariant
		{
			private bool _value;

			public BoolVariant(bool value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(bool);
			}

			public override BaseVariant Clone()
			{
				return new BoolVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToBoolean(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override int ToInt32()
			{
				return _value ? 1 : 0;
			}
		}

		sealed class CharVariant : BaseVariant
		{
			private char _value;

			public CharVariant(char value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(char);
			}

			public override BaseVariant Clone()
			{
				return new CharVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToChar(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override ushort ToUInt16()
			{
				return _value;
			}

			public override int ToInt32()
			{
				return _value;
			}

			public override uint ToUInt32()
			{
				return _value;
			}
		}

		sealed class ByteVariant : BaseVariant
		{
			private byte _value;

			public ByteVariant(byte value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(byte);
			}

			public override BaseVariant Clone()
			{
				return new ByteVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToByte(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return _value;
			}

			public override short ToInt16()
			{
				return _value;
			}

			public override ushort ToUInt16()
			{
				return _value;
			}

			public override int ToInt32()
			{
				return _value;
			}
			public override uint ToUInt32()
			{
				return _value;
			}
		}

		sealed class SbyteVariant : BaseVariant
		{
			private sbyte _value;

			public SbyteVariant(sbyte value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(sbyte);
			}

			public override BaseVariant Clone()
			{
				return new SbyteVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToSByte(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return _value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return _value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override int ToInt32()
			{
				return _value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}
		}

		sealed class UintVariant : BaseVariant
		{
			private uint _value;
			public UintVariant(uint value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(uint);
			}

			public override BaseVariant Clone()
			{
				return new UintVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToUInt32(value);
			}

			public override StackableVariant ToStack()
			{
				return new IntVariant(ToInt32());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override int ToInt32()
			{
				return (int)_value;
			}

			public override uint ToUInt32()
			{
				return _value;
			}
		}

		sealed class UlongVariant : BaseVariant
		{
			private ulong _value;
			public UlongVariant(ulong value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(ulong);
			}

			public override BaseVariant Clone()
			{
				return new UlongVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = System.Convert.ToUInt64(value);
			}

			public override StackableVariant ToStack()
			{
				return new LongVariant(ToInt64());
			}

			public override sbyte ToSByte()
			{
				return (sbyte)_value;
			}

			public override byte ToByte()
			{
				return (byte)_value;
			}

			public override short ToInt16()
			{
				return (short)_value;
			}

			public override ushort ToUInt16()
			{
				return (ushort)_value;
			}

			public override int ToInt32()
			{
				return (int)_value;
			}

			public override uint ToUInt32()
			{
				return (uint)_value;
			}

			public override long ToInt64()
			{
				return (long)_value;
			}

			public override ulong ToUInt64()
			{
				return _value;
			}
		}

		sealed class ObjectVariant : StackableVariant
		{
			private object _value;

			public ObjectVariant(object value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(object);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Object;
			}

			public override BaseVariant Clone()
			{
				return new ObjectVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = value;
			}

			public override bool ToBoolean()
			{
				return _value != null;
			}
		}

		sealed class PointerVariant : StackableVariant
		{
			private object _value;
			private Type _type;
			private BaseVariant _variant;

			public PointerVariant(object value, Type type)
			{
				_value = value;
				_type = type;
				_variant = ToVariant(value);
			}

			private static BaseVariant ToVariant(object value)
			{
				unsafe
				{
					IntPtr ptr = value == null ? IntPtr.Zero : new IntPtr(Pointer.Unbox(value));
					if (IntPtr.Size == 4)
						return new IntVariant(ptr.ToInt32());
					else
						return new LongVariant(ptr.ToInt64());
				}
			}

			public override Type Type()
			{
				return _type;
			}

			public override TypeCode CalcTypeCode()
			{
				return (IntPtr.Size == 4) ? TypeCode.UInt32 : TypeCode.UInt64;
			}

			public override BaseVariant Clone()
			{
				return new PointerVariant(_value, _type);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = value;
				_variant = ToVariant(value);
			}

			public override bool ToBoolean()
			{
				return _value != null;
			}

			public override sbyte ToSByte()
			{
				return _variant.ToSByte();
			}

			public override short ToInt16()
			{
				return _variant.ToInt16();
			}

			public override int ToInt32()
			{
				return _variant.ToInt32();
			}

			public override long ToInt64()
			{
				return _variant.ToInt64();
			}

			public override byte ToByte()
			{
				return _variant.ToByte();
			}

			public override ushort ToUInt16()
			{
				return _variant.ToUInt16();
			}

			public override uint ToUInt32()
			{
				return _variant.ToUInt32();
			}

			public override ulong ToUInt64()
			{
				return _variant.ToUInt64();
			}

			public override float ToSingle()
			{
				return _variant.ToSingle();
			}

			public override double ToDouble()
			{
				return _variant.ToDouble();
			}

			public override IntPtr ToIntPtr()
			{
				return _variant.ToIntPtr();
			}

			public override UIntPtr ToUIntPtr()
			{
				return _variant.ToUIntPtr();
			}

			public override unsafe void* ToPointer()
			{
				return Pointer.Unbox(_value);
			}

			public override object Conv_ovf(Type type, bool un)
			{
				return _variant.Conv_ovf(type, un);
			}
		}

		sealed class ValueTypeVariant : StackableVariant
		{
			private object _value;

			public ValueTypeVariant(object value)
			{
				if (value != null && !(value is ValueType))
					throw new ArgumentException();
				_value = value;
			}

			public override Type Type()
			{
				return typeof(ValueType);
			}

			public override BaseVariant Clone()
			{
				object value;
				if (_value == null)
				{
					value = null;
				}
				else
				{
					var type = _value.GetType();
					var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
					value = Activator.CreateInstance(type);
					foreach (var field in fields)
					{
						field.SetValue(value, field.GetValue(_value));
					}
				}

				return new ValueTypeVariant(value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				if (value != null && !(value is ValueType))
					throw new ArgumentException();
				_value = value;
			}
		}

		sealed class ArrayVariant : StackableVariant
		{
			private Array _value;

			public ArrayVariant(Array value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(Array);
			}

			public override BaseVariant Clone()
			{
				return new ArrayVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (Array)value;
			}

			public override bool ToBoolean()
			{
				return _value != null;
			}
		}

		abstract class BaseReferenceVariant : StackableVariant
		{
			public override bool IsReference()
			{
				return true;
			}
		}

		sealed class ReferenceVariant : BaseReferenceVariant
		{
			private BaseVariant _variable;

			public ReferenceVariant(BaseVariant variable)
			{
				_variable = variable;
			}

			public override Type Type()
			{
				return _variable.Type();
			}

			public override BaseVariant Clone()
			{
				return new ReferenceVariant(_variable);
			}

			public override object Value()
			{
				return _variable.Value();
			}

			public override void SetValue(object value)
			{
				_variable.SetValue(value);
			}

			public override bool ToBoolean()
			{
				return _variable != null;
			}

			public override void SetFieldValue(FieldInfo field, object value)
			{
				_variable.SetFieldValue(field, value);
			}
		}

		sealed class NullReferenceVariant : BaseReferenceVariant
		{
			private Type _type;

			public NullReferenceVariant(Type type)
			{
				_type = type;
			}

			public override Type Type()
			{
				return _type;
			}

			public override BaseVariant Clone()
			{
				return new NullReferenceVariant(_type);
			}

			public override object Value()
			{
				throw new InvalidOperationException();
			}

			public override void SetValue(object value)
			{
				throw new InvalidOperationException();
			}

			public override void SetFieldValue(FieldInfo field, object value)
			{
				throw new InvalidOperationException();
			}
		}

		sealed class ArgReferenceVariant : BaseReferenceVariant
		{
			private BaseVariant _variable;
			private BaseVariant _arg;

			public ArgReferenceVariant(BaseVariant variable, BaseVariant arg)
			{
				_variable = variable;
				_arg = arg;
			}

			public override Type Type()
			{
				return _variable.Type();
			}

			public override BaseVariant Clone()
			{
				return new ArgReferenceVariant(_variable, _arg);
			}

			public override object Value()
			{
				return _variable.Value();
			}

			public override void SetValue(object value)
			{
				_variable.SetValue(value);
				_arg.SetValue(_variable.Value());
			}

			public override bool ToBoolean()
			{
				return _variable != null;
			}
		}

		sealed class FieldReferenceVariant : BaseReferenceVariant
		{
			private FieldInfo _field;
			private BaseVariant _variable;

			public FieldReferenceVariant(FieldInfo field, BaseVariant variable)
			{
				_field = field;
				_variable = variable;
			}

			public override Type Type()
			{
				return _field.FieldType;
			}

			public override BaseVariant Clone()
			{
				return new FieldReferenceVariant(_field, _variable);
			}

			public override object Value()
			{
				return _field.GetValue(_variable.Value());
			}

			public override void SetValue(object value)
			{
				_variable.SetFieldValue(_field, value);
			}
		}

		sealed class ArrayElementVariant : BaseReferenceVariant
		{
			private Array _value;
			private int _index;

			public ArrayElementVariant(Array value, int index)
			{
				_value = value;
				_index = index;
			}

			public override Type Type()
			{
				return _value.GetType().GetElementType();
			}

			public override BaseVariant Clone()
			{
				return new ArrayElementVariant(_value, _index);
			}

			public override object Value()
			{
				return _value.GetValue(_index);
			}

			public override void SetValue(object value)
			{
				switch (System.Type.GetTypeCode(_value.GetType().GetElementType()))
				{
					case TypeCode.SByte:
						value = System.Convert.ToSByte(value);
						break;
					case TypeCode.Byte:
						value = System.Convert.ToByte(value);
						break;
					case TypeCode.Char:
						value = System.Convert.ToChar(value);
						break;
					case TypeCode.Int16:
						value = System.Convert.ToInt16(value);
						break;
					case TypeCode.UInt16:
						value = System.Convert.ToUInt16(value);
						break;
					case TypeCode.Int32:
						value = System.Convert.ToInt32(value);
						break;
					case TypeCode.UInt32:
						value = System.Convert.ToUInt32(value);
						break;
					case TypeCode.Int64:
						value = System.Convert.ToInt64(value);
						break;
					case TypeCode.UInt64:
						value = System.Convert.ToUInt64(value);
						break;
				}

				_value.SetValue(value, _index);
			}

			public override void SetFieldValue(FieldInfo field, object value)
			{
				var obj = Value();
				field.SetValue(obj, value);
				if (obj is ValueType)
					SetValue(obj);
			}

			public override UIntPtr ToUIntPtr()
			{
				var dynamicMethod = new DynamicMethod("", typeof(UIntPtr), new Type[] { _value.GetType(), typeof(int) }, typeof(VirtualMachine).Module, true);
				var gen = dynamicMethod.GetILGenerator();
				gen.Emit(System.Reflection.Emit.OpCodes.Ldarg, 0);
				gen.Emit(System.Reflection.Emit.OpCodes.Ldarg, 1);
				gen.Emit(System.Reflection.Emit.OpCodes.Ldelema, _value.GetType().GetElementType());
				gen.Emit(System.Reflection.Emit.OpCodes.Conv_U);
				gen.Emit(System.Reflection.Emit.OpCodes.Ret);

				return (UIntPtr)dynamicMethod.Invoke(null, new object[] { _value, _index });
			}
		}

		sealed class MethodVariant : StackableVariant
		{
			private MethodBase _value;

			public MethodVariant(MethodBase value)
			{
				_value = value;
			}

			public override Type Type()
			{
				return typeof(MethodBase);
			}

			public override BaseVariant Clone()
			{
				return new MethodVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (MethodBase)value;
			}

			public override bool ToBoolean()
			{
				return _value != null;
			}

			public override IntPtr ToIntPtr()
			{
				return _value.MethodHandle.GetFunctionPointer();
			}
		}

		sealed class IntPtrVariant : StackableVariant
		{
			IntPtr _value;
			private BaseVariant _variant;

			public IntPtrVariant(IntPtr value)
			{
				_value = value;
				_variant = ToVariant(_value);
			}

			private static BaseVariant ToVariant(IntPtr value)
			{
				if (IntPtr.Size == 4)
					return new IntVariant(value.ToInt32());
				else
					return new LongVariant(value.ToInt64());
			}

			public override Type Type()
			{
				return typeof(IntPtr);
			}

			public override TypeCode CalcTypeCode()
			{
				return _variant.CalcTypeCode();
			}

			public override BaseVariant Clone()
			{
				return new IntPtrVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (IntPtr)value;
				_variant = ToVariant(_value);
			}

			public override bool ToBoolean()
			{
				return _value != IntPtr.Zero;
			}

			public override sbyte ToSByte()
			{
				return _variant.ToSByte();
			}

			public override short ToInt16()
			{
				return _variant.ToInt16();
			}

			public override int ToInt32()
			{
				return _variant.ToInt32();
			}

			public override long ToInt64()
			{
				return _variant.ToInt64();
			}

			public override byte ToByte()
			{
				return _variant.ToByte();
			}

			public override ushort ToUInt16()
			{
				return _variant.ToUInt16();
			}

			public override uint ToUInt32()
			{
				return _variant.ToUInt32();
			}

			public override ulong ToUInt64()
			{
				return _variant.ToUInt64();
			}

			public override float ToSingle()
			{
				return _variant.ToSingle();
			}

			public override double ToDouble()
			{
				return _variant.ToDouble();
			}

			public override IntPtr ToIntPtr()
			{
				return _value;
			}

			public override UIntPtr ToUIntPtr()
			{
				return _variant.ToUIntPtr();
			}

			public override unsafe void* ToPointer()
			{
				return _value.ToPointer();
			}

			public override object Conv_ovf(Type type, bool un)
			{
				return _variant.Conv_ovf(type, un);
			}
		}

		sealed class UIntPtrVariant : StackableVariant
		{
			private UIntPtr _value;
			private BaseVariant _variant;

			public UIntPtrVariant(UIntPtr value)
			{
				_value = value;
				_variant = ToVariant(_value);
			}

			private static BaseVariant ToVariant(UIntPtr value)
			{
				if (IntPtr.Size == 4)
					return new IntVariant((int)value.ToUInt32());
				else
					return new LongVariant((long)value.ToUInt64());
			}

			public override Type Type()
			{
				return typeof(UIntPtr);
			}

			public override TypeCode CalcTypeCode()
			{
				return _variant.CalcTypeCode();
			}

			public override BaseVariant Clone()
			{
				return new UIntPtrVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				_value = (UIntPtr)value;
				_variant = ToVariant(_value);
			}

			public override bool ToBoolean()
			{
				return _value != UIntPtr.Zero;
			}

			public override sbyte ToSByte()
			{
				return _variant.ToSByte();
			}

			public override short ToInt16()
			{
				return _variant.ToInt16();
			}

			public override int ToInt32()
			{
				return _variant.ToInt32();
			}

			public override long ToInt64()
			{
				return _variant.ToInt64();
			}

			public override byte ToByte()
			{
				return _variant.ToByte();
			}

			public override ushort ToUInt16()
			{
				return _variant.ToUInt16();
			}

			public override uint ToUInt32()
			{
				return _variant.ToUInt32();
			}

			public override ulong ToUInt64()
			{
				return _variant.ToUInt64();
			}

			public override float ToSingle()
			{
				return _variant.ToSingle();
			}

			public override double ToDouble()
			{
				return _variant.ToDouble();
			}

			public override IntPtr ToIntPtr()
			{
				return _variant.ToIntPtr();
			}

			public override UIntPtr ToUIntPtr()
			{
				return _value;
			}

			public override unsafe void* ToPointer()
			{
				return _value.ToPointer();
			}

			public override object Conv_ovf(Type type, bool un)
			{
				return _variant.Conv_ovf(type, un);
			}
		}

		sealed class EnumVariant : StackableVariant
		{
			private Enum _value;
			private BaseVariant _variant;

			public EnumVariant(Enum value)
			{
				if (value == null)
					throw new ArgumentException();
				_value = value;
				_variant = ToVariant(_value);
			}

			private static BaseVariant ToVariant(Enum value)
			{
				switch (value.GetTypeCode())
				{
					case TypeCode.Byte:
					case TypeCode.UInt16:
					case TypeCode.UInt32:
						return new IntVariant((int)System.Convert.ToUInt32(value));
					case TypeCode.SByte:
					case TypeCode.Int16:
					case TypeCode.Int32:
						return new IntVariant(System.Convert.ToInt32(value));
					case TypeCode.UInt64:
						return new LongVariant((long)System.Convert.ToUInt64(value));
					case TypeCode.Int64:
						return new LongVariant(System.Convert.ToInt64(value));
					default:
						throw new InvalidOperationException();
				}
			}

			public override BaseVariant ToUnsigned()
			{
				return _variant.ToUnsigned();
			}

			public override Type Type()
			{
				return _value.GetType();
			}

			public override TypeCode CalcTypeCode()
			{
				return _variant.CalcTypeCode();
			}

			public override BaseVariant Clone()
			{
				return new EnumVariant(_value);
			}

			public override object Value()
			{
				return _value;
			}

			public override void SetValue(object value)
			{
				if (value == null)
					throw new ArgumentException();
				_value = (Enum)value;
				_variant = ToVariant(_value);
			}

			public override byte ToByte()
			{
				return _variant.ToByte();
			}

			public override sbyte ToSByte()
			{
				return _variant.ToSByte();
			}

			public override short ToInt16()
			{
				return _variant.ToInt16();
			}

			public override ushort ToUInt16()
			{
				return _variant.ToUInt16();
			}

			public override int ToInt32()
			{
				return _variant.ToInt32();
			}

			public override uint ToUInt32()
			{
				return _variant.ToUInt32();
			}

			public override long ToInt64()
			{
				return _variant.ToInt64();
			}

			public override ulong ToUInt64()
			{
				return _variant.ToUInt64();
			}

			public override float ToSingle()
			{
				return _variant.ToSingle();
			}
			public override double ToDouble()
			{
				return _variant.ToDouble();
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(IntPtr.Size == 4 ? ToInt32() : ToInt64());
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr(IntPtr.Size == 4 ? ToUInt32() : ToUInt64());
			}

			public override object Conv_ovf(Type type, bool un)
			{
				return _variant.Conv_ovf(type, un);
			}
		}

		sealed class PointerReference : BaseReferenceVariant
		{
			private IntPtr _value;
			private Type _type;
			public PointerReference(IntPtr value, Type type)
			{
				_value = value;
				_type = type;
			}

			public override Type Type()
			{
				return typeof(Pointer);
			}

			public override TypeCode CalcTypeCode()
			{
				return TypeCode.Empty;
			}

			public override BaseVariant Clone()
			{
				return new PointerReference(_value, _type);
			}

			public override object Value()
			{
				if (_type.IsValueType)
					return Marshal.PtrToStructure(_value, _type);

				throw new InvalidOperationException();
			}

			public override void SetValue(object value)
			{
				if (value == null)
					throw new ArgumentException();

				if (_type.IsValueType)
				{
					Marshal.StructureToPtr(value, _value, false);
					return;
				}

				switch (System.Type.GetTypeCode(value.GetType()))
				{
					case TypeCode.SByte:
						Marshal.WriteByte(_value, (byte)System.Convert.ToSByte(value));
						break;
					case TypeCode.Byte:
						Marshal.WriteByte(_value, System.Convert.ToByte(value));
						break;
					case TypeCode.Char:
						Marshal.WriteInt16(_value, System.Convert.ToChar(value));
						break;
					case TypeCode.Int16:
						Marshal.WriteInt16(_value, System.Convert.ToInt16(value));
						break;
					case TypeCode.UInt16:
						Marshal.WriteInt16(_value, (short)System.Convert.ToUInt16(value));
						break;
					case TypeCode.Int32:
						Marshal.WriteInt32(_value, System.Convert.ToInt32(value));
						break;
					case TypeCode.UInt32:
						Marshal.WriteInt32(_value, (int)System.Convert.ToUInt32(value));
						break;
					case TypeCode.Int64:
						Marshal.WriteInt64(_value, System.Convert.ToInt64(value));
						break;
					case TypeCode.UInt64:
						Marshal.WriteInt64(_value, (long)System.Convert.ToUInt64(value));
						break;
					case TypeCode.Single:
						Marshal.WriteInt32(_value, BitConverter.ToInt32(BitConverter.GetBytes(System.Convert.ToSingle(value)), 0));
						break;
					case TypeCode.Double:
						Marshal.WriteInt64(_value, BitConverter.ToInt64(BitConverter.GetBytes(System.Convert.ToDouble(value)), 0));
						break;
					default:
						throw new ArgumentException();
				}
			}

			public override sbyte ToSByte()
			{
				return (sbyte)Marshal.ReadByte(_value);
			}

			public override short ToInt16()
			{
				return Marshal.ReadInt16(_value);
			}

			public override int ToInt32()
			{
				return Marshal.ReadInt32(_value);
			}

			public override long ToInt64()
			{
				return Marshal.ReadInt64(_value);
			}

			public override char ToChar()
			{
				return (char)Marshal.ReadInt16(_value);
			}

			public override byte ToByte()
			{
				return Marshal.ReadByte(_value);
			}
			public override ushort ToUInt16()
			{
				return (ushort)Marshal.ReadInt16(_value);
			}

			public override uint ToUInt32()
			{
				return (uint)Marshal.ReadInt32(_value);
			}

			public override ulong ToUInt64()
			{
				return (ulong)Marshal.ReadInt64(_value);
			}

			public override float ToSingle()
			{
				return BitConverter.ToSingle(BitConverter.GetBytes(Marshal.ReadInt32(_value)), 0);
			}

			public override double ToDouble()
			{
				return BitConverter.ToDouble(BitConverter.GetBytes(Marshal.ReadInt64(_value)), 0);
			}

			public override IntPtr ToIntPtr()
			{
				return new IntPtr(IntPtr.Size == 4 ? Marshal.ReadInt32(_value) : Marshal.ReadInt64(_value));
			}

			public override UIntPtr ToUIntPtr()
			{
				return new UIntPtr(IntPtr.Size == 4 ? (uint)Marshal.ReadInt32(_value) : (ulong)Marshal.ReadInt64(_value));
			}
		}

		private sealed class CatchBlock
		{
			private byte _type;
			private int _handler;
			private int _filter;
			public CatchBlock(byte type, int handler, int filter)
			{
				_type = type;
				_handler = handler;
				_filter = filter;
			}

			public byte Type()
			{
				return _type;
			}
			public int Handler()
			{
				return _handler;
			}

			public int Filter()
			{
				return _filter;
			}
		}

		private sealed class TryBlock
		{
			private int _begin;
			private int _end;
			private List<CatchBlock> _catchBlocks = new List<CatchBlock>();

			public TryBlock(int begin, int end)
			{
				_begin = begin;
				_end = end;
			}

			public int Begin()
			{
				return _begin;
			}
			public int End()
			{
				return _end;
			}

			public int CompareTo(TryBlock compare)
			{
				if (compare == null)
					return 1;

				int res = _begin.CompareTo(compare.Begin());
				if (res == 0)
					res = compare.End().CompareTo(_end);
				return res;
			}

			public void AddCatchBlock(byte type, int handler, int filter)
			{
				_catchBlocks.Add(new CatchBlock(type, handler, filter));
			}

			public List<CatchBlock> CatchBlocks()
			{
				return _catchBlocks;
			}
		}

		internal delegate void Handler();
		private readonly Dictionary<uint, Handler> _handlers = new Dictionary<uint, Handler>();

		private readonly Module _module = typeof(VirtualMachine).Module;
		private readonly long _instance;
		private int _pos;
		private Type _constrained;
		private Stack<StackableVariant> _stack = new Stack<StackableVariant>();
		private static readonly Dictionary<int, object> _cache = new Dictionary<int, object>();
		private static readonly Dictionary<MethodBase, DynamicMethod> _dynamicMethodCache = new Dictionary<MethodBase, DynamicMethod>();
		private List<BaseVariant> _args = new List<BaseVariant>();
		private List<TryBlock> _tryBlocks = new List<TryBlock>();
		private Stack<TryBlock> _tryStack = new Stack<TryBlock>();
		private Stack<int> _finallyStack = new Stack<int>();
		private Exception _exception;
		private CatchBlock _filterBlock;
		private List<IntPtr> _localloc = new List<IntPtr>();

		public VirtualMachine()
		{
			_instance = Marshal.GetHINSTANCE(_module).ToInt64();

			_handlers[(uint)OpCodes.icInitarg] = Initarg;
			_handlers[(uint)OpCodes.icInitcatchblock] = Initcatchblock;
			_handlers[(uint)OpCodes.icEntertry] = Entertry;
			_handlers[(uint)OpCodes.icLdc_i4] = Ldc_i4;
			_handlers[(uint)OpCodes.icLdc_i8] = Ldc_i8;
			_handlers[(uint)OpCodes.icLdc_r4] = Ldc_r4;
			_handlers[(uint)OpCodes.icLdc_r8] = Ldc_r8;
			_handlers[(uint)OpCodes.icLdnull] = Ldnull;
			_handlers[(uint)OpCodes.icLdstr] = Ldstr;
			_handlers[(uint)OpCodes.icDup] = Dup;
			_handlers[(uint)OpCodes.icAdd] = Add_;
			_handlers[(uint)OpCodes.icAdd_ovf] = Add_ovf;
			_handlers[(uint)OpCodes.icAdd_ovf_un] = Add_ovf_un;
			_handlers[(uint)OpCodes.icSub] = Sub_;
			_handlers[(uint)OpCodes.icSub_ovf] = Sub_ovf;
			_handlers[(uint)OpCodes.icSub_ovf_un] = Sub_ovf_un;
			_handlers[(uint)OpCodes.icMul] = Mul_;
			_handlers[(uint)OpCodes.icMul_ovf] = Mul_ovf;
			_handlers[(uint)OpCodes.icMul_ovf_un] = Mul_ovf_un;
			_handlers[(uint)OpCodes.icDiv] = Div_;
			_handlers[(uint)OpCodes.icDiv_un] = Div_un;
			_handlers[(uint)OpCodes.icRem] = Rem_;
			_handlers[(uint)OpCodes.icRem_un] = Rem_un;
			_handlers[(uint)OpCodes.icAnd] = And_;
			_handlers[(uint)OpCodes.icOr] = Or_;
			_handlers[(uint)OpCodes.icXor] = Xor_;
			_handlers[(uint)OpCodes.icNot] = Not_;
			_handlers[(uint)OpCodes.icNeg] = Neg_;
			_handlers[(uint)OpCodes.icShr] = Shr_;
			_handlers[(uint)OpCodes.icShr_un] = Shr_un;
			_handlers[(uint)OpCodes.icShl] = Shl_;
			_handlers[(uint)OpCodes.icConv] = Conv;
			_handlers[(uint)OpCodes.icConv_ovf] = Conv_ovf;
			_handlers[(uint)OpCodes.icConv_ovf_un] = Conv_ovf_un;
			_handlers[(uint)OpCodes.icSizeof] = Sizeof;
			_handlers[(uint)OpCodes.icLdind_ref] = Ldind;
			_handlers[(uint)OpCodes.icLdfld] = Ldfld;
			_handlers[(uint)OpCodes.icLdflda] = Ldflda;
			_handlers[(uint)OpCodes.icStfld] = Stfld;
			_handlers[(uint)OpCodes.icStsfld] = Stsfld;
			_handlers[(uint)OpCodes.icStind_ref] = Stind;
			_handlers[(uint)OpCodes.icLdarg] = Ldarg;
			_handlers[(uint)OpCodes.icLdarga] = Ldarga;
			_handlers[(uint)OpCodes.icStarg] = Starg;
			_handlers[(uint)OpCodes.icConstrained] = Constrained;
			_handlers[(uint)OpCodes.icCall] = Call_;
			_handlers[(uint)OpCodes.icCallvirt] = Callvirt;
			_handlers[(uint)OpCodes.icCalli] = Calli;
			_handlers[(uint)OpCodes.icCallvm] = Callvm_;
			_handlers[(uint)OpCodes.icCallvmvirt] = Callvmvirt;
			_handlers[(uint)OpCodes.icNewobj] = Newobj_;
			_handlers[(uint)OpCodes.icInitobj] = Initobj;
			_handlers[(uint)OpCodes.icCmp] = Cmp;
			_handlers[(uint)OpCodes.icCmp_un] = Cmp_un;
			_handlers[(uint)OpCodes.icNewarr] = Newarr;
			_handlers[(uint)OpCodes.icStelem_ref] = Stelem;
			_handlers[(uint)OpCodes.icLdelem_ref] = Ldelem;
			_handlers[(uint)OpCodes.icLdlen] = Ldlen;
			_handlers[(uint)OpCodes.icLdelema] = Ldelema;
			_handlers[(uint)OpCodes.icLdftn] = Ldftn;
			_handlers[(uint)OpCodes.icLdvirtftn] = Ldvirtftn;
			_handlers[(uint)OpCodes.icBr] = Br;
			_handlers[(uint)OpCodes.icPop] = Pop_;
			_handlers[(uint)OpCodes.icLeave] = Leave;
			_handlers[(uint)OpCodes.icEndfinally] = Endfinally;
			_handlers[(uint)OpCodes.icEndfilter] = Endfilter;
			_handlers[(uint)OpCodes.icBox] = Box;
			_handlers[(uint)OpCodes.icUnbox] = Unbox;
			_handlers[(uint)OpCodes.icUnbox_any] = Unbox_any;
			_handlers[(uint)OpCodes.icLdmem_i4] = Ldmem_i4;
			_handlers[(uint)OpCodes.icLdtoken] = Ldtoken;
			_handlers[(uint)OpCodes.icThrow] = Throw;
			_handlers[(uint)OpCodes.icRethrow] = Rethrow;
			_handlers[(uint)OpCodes.icCastclass] = Castclass;
			_handlers[(uint)OpCodes.icIsinst] = Isinst;
			_handlers[(uint)OpCodes.icCkfinite] = Ckfinite;
			_handlers[(uint)OpCodes.icLocalloc] = Localloc;
		}

		private void Push(BaseVariant value)
		{
			_stack.Push(value.ToStack());
		}

		private BaseVariant Pop()
		{
			return _stack.Pop();
		}

		private BaseVariant Peek()
		{
			return _stack.Peek();
		}

		private byte ReadByte()
		{
			byte res = Marshal.ReadByte(new IntPtr(_instance + _pos));
			_pos += sizeof(byte);
			return res;
		}

		private short ReadInt16()
		{
			short res = Marshal.ReadInt16(new IntPtr(_instance + _pos));
			_pos += sizeof(short);
			return res;
		}

		private int ReadInt32()
		{
			int res = Marshal.ReadInt32(new IntPtr(_instance + _pos));
			_pos += sizeof(int);
			return res;
		}

		private long ReadInt64()
		{
			long res = Marshal.ReadInt64(new IntPtr(_instance + _pos));
			_pos += sizeof(long);
			return res;
		}

		private float ReadSingle()
		{
			return BitConverter.ToSingle(BitConverter.GetBytes(ReadInt32()), 0);
		}

		private double ReadDouble()
		{
			return BitConverter.ToDouble(BitConverter.GetBytes(ReadInt64()), 0);
		}

		private void Initcatchblock()
		{
			var type = ReadByte();
			var tryBegin = ReadInt32();
			var tryEnd = ReadInt32();
			var handler = ReadInt32();
			var filter = ReadInt32();
			TryBlock tryBlock = null;
			for (var i = 0; i < _tryBlocks.Count; i++)
			{
				var current = _tryBlocks[i];
				if (current.Begin() == tryBegin && current.End() == tryEnd)
				{
					tryBlock = current;
					break;
				}
			}
			if (tryBlock == null)
			{
				bool isInserted = false;
				tryBlock = new TryBlock(tryBegin, tryEnd);
				for (var i = 0; i < _tryBlocks.Count; i++)
				{
					var current = _tryBlocks[i];
					if (tryBlock.CompareTo(current) < 0)
					{
						_tryBlocks.Insert(i, tryBlock);
						isInserted = true;
						break;
					}
				}
				if (!isInserted)
					_tryBlocks.Add(tryBlock);
			}
			tryBlock.AddCatchBlock(type, handler, filter);
		}

		private TypeCode CalcTypeCode(BaseVariant v1, BaseVariant v2)
		{
			var type1 = v1.CalcTypeCode();
			var type2 = v2.CalcTypeCode();
			if (type1 == TypeCode.Empty || type1 == TypeCode.Object || type2 == TypeCode.Empty || type2 == TypeCode.Object)
				return TypeCode.Empty;

			// UInt32/UInt64 used for pointers
			if (type1 == TypeCode.UInt32)
				return (type2 == TypeCode.Int32) ? type1 : TypeCode.Empty;
			if (type2 == TypeCode.UInt32)
				return (type1 == TypeCode.Int32) ? type2 : TypeCode.Empty;
			if (type1 == TypeCode.UInt64)
				return (type2 == TypeCode.Int32 || type2 == TypeCode.Int64) ? type1 : TypeCode.Empty;
			if (type2 == TypeCode.UInt64)
				return (type1 == TypeCode.Int32 || type1 == TypeCode.Int64) ? type1 : TypeCode.Empty;

			if (type1 == TypeCode.Double || type2 == TypeCode.Double)
				return TypeCode.Double;
			if (type1 == TypeCode.Single || type2 == TypeCode.Single)
				return TypeCode.Single;
			if (type1 == TypeCode.Int64 || type2 == TypeCode.Int64)
				return TypeCode.Int64;
			return TypeCode.Int32;
		}

		private BaseVariant Add(BaseVariant v1, BaseVariant v2, bool ovf, bool un)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = ovf ? (int)checked(value1 + value2) : (int)(value1 + value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = ovf ? checked(value1 + value2) : (value1 + value2);
						}
						return new IntVariant(value);
					}
				case TypeCode.Int64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = ovf ? (long)checked(value1 + value2) : (long)(value1 + value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = ovf ? checked(value1 + value2) : (value1 + value2);
						}
						return new LongVariant(value);
					}
				case TypeCode.Single:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToSingle();
						var value2 = (un ? v2.ToUnsigned() : v2).ToSingle();
						return new SingleVariant(ovf ? checked(value1 + value2) : (value1 + value2));
					}
				case TypeCode.Double:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToDouble();
						var value2 = (un ? v2.ToUnsigned() : v2).ToDouble();
						return new DoubleVariant(ovf ? checked(value1 + value2) : (value1 + value2));
					}
				case TypeCode.UInt32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = ovf ? (int)checked(value1 + value2) : (int)(value1 + value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = ovf ? checked(value1 + value2) : (value1 + value2);
						}
						PointerVariant v = v1.CalcTypeCode() == type ? (PointerVariant)v1 : (PointerVariant)v2;
						unsafe
						{
							return new PointerVariant(Pointer.Box(new IntPtr(value).ToPointer(), v.Type()), v.Type());
						}
					}
				case TypeCode.UInt64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = ovf ? (long)checked(value1 + value2) : (long)(value1 + value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = ovf ? checked(value1 + value2) : (value1 + value2);
						}
						PointerVariant v = v1.CalcTypeCode() == type ? (PointerVariant)v1 : (PointerVariant)v2;
						unsafe
						{
							return new PointerVariant(Pointer.Box(new IntPtr(value).ToPointer(), v.Type()), v.Type());
						}
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Sub(BaseVariant v1, BaseVariant v2, bool ovf, bool un)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = ovf ? (int)checked(value1 - value2) : (int)(value1 - value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = ovf ? checked(value1 - value2) : (value1 - value2);
						}
						return new IntVariant(value);
					}
				case TypeCode.Int64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = ovf ? (long)checked(value1 - value2) : (long)(value1 - value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = ovf ? checked(value1 - value2) : (value1 - value2);
						}
						return new LongVariant(value);
					}
				case TypeCode.Single:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToSingle();
						var value2 = (un ? v2.ToUnsigned() : v2).ToSingle();
						return new SingleVariant(ovf ? checked(value1 - value2) : (value1 - value2));
					}
				case TypeCode.Double:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToDouble();
						var value2 = (un ? v2.ToUnsigned() : v2).ToDouble();
						return new DoubleVariant(ovf ? checked(value1 - value2) : (value1 - value2));
					}
				case TypeCode.UInt32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = ovf ? (int)checked(value1 - value2) : (int)(value1 - value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = ovf ? checked(value1 - value2) : (value1 - value2);
						}
						PointerVariant v = v1.CalcTypeCode() == type ? (PointerVariant)v1 : (PointerVariant)v2;
						unsafe
						{
							return new PointerVariant(Pointer.Box(new IntPtr(value).ToPointer(), v.Type()), v.Type());
						}
					}
				case TypeCode.UInt64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = ovf ? (long)checked(value1 - value2) : (long)(value1 - value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = ovf ? checked(value1 - value2) : (value1 - value2);
						}
						PointerVariant v = v1.CalcTypeCode() == type ? (PointerVariant)v1 : (PointerVariant)v2;
						unsafe
						{
							return new PointerVariant(Pointer.Box(new IntPtr(value).ToPointer(), v.Type()), v.Type());
						}
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Mul(BaseVariant v1, BaseVariant v2, bool ovf, bool un)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = ovf ? (int)checked(value1 * value2) : (int)(value1 * value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = ovf ? checked(value1 * value2) : (value1 * value2);
						}
						return new IntVariant(value);
					}
				case TypeCode.Int64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = ovf ? (long)checked(value1 * value2) : (long)(value1 * value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = ovf ? checked(value1 * value2) : (value1 * value2);
						}
						return new LongVariant(value);
					}
				case TypeCode.Single:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToSingle();
						var value2 = (un ? v2.ToUnsigned() : v2).ToSingle();
						return new DoubleVariant(ovf ? checked(value1 * value2) : (value1 * value2));
					}
				case TypeCode.Double:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToDouble();
						var value2 = (un ? v2.ToUnsigned() : v2).ToDouble();
						return new DoubleVariant(ovf ? checked(value1 * value2) : (value1 * value2));
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Div(BaseVariant v1, BaseVariant v2, bool un)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						int value;
						if (un)
						{
							var value1 = v1.ToUInt32();
							var value2 = v2.ToUInt32();
							value = (int)(value1 / value2);
						}
						else
						{
							var value1 = v1.ToInt32();
							var value2 = v2.ToInt32();
							value = value1 / value2;
						}
						return new IntVariant(value);
					}
				case TypeCode.Int64:
					{
						long value;
						if (un)
						{
							var value1 = v1.ToUInt64();
							var value2 = v2.ToUInt64();
							value = (long)(value1 / value2);
						}
						else
						{
							var value1 = v1.ToInt64();
							var value2 = v2.ToInt64();
							value = value1 / value2;
						}
						return new LongVariant(value);
					}
				case TypeCode.Single:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToSingle();
						var value2 = (un ? v2.ToUnsigned() : v2).ToSingle();
						return new SingleVariant(value1 / value2);
					}
				case TypeCode.Double:
					{
						var value1 = (un ? v1.ToUnsigned() : v1).ToDouble();
						var value2 = (un ? v2.ToUnsigned() : v2).ToDouble();
						return new DoubleVariant(value1 / value2);
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Rem(BaseVariant v1, BaseVariant v2, bool un)
		{
			var type1 = v1.CalcTypeCode();
			switch (type1)
			{
				case TypeCode.Int32:
					if (un)
					{
						var value1 = v1.ToUInt32();
						var value2 = v2.ToUInt32();
						return new IntVariant((int)(value1 % value2));
					} else {
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 % value2);
					}
				case TypeCode.Int64:
					if (un)
					{
						var value1 = v1.ToUInt64();
						var value2 = v2.ToUInt64();
						return new LongVariant((long)(value1 % value2));
					} else {
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt64();
						return new LongVariant(value1 % value2);
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Xor(BaseVariant v1, BaseVariant v2)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 ^ value2);
					}
				case TypeCode.Int64:
					{
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt64();
						return new LongVariant(value1 ^ value2);
					}
				case TypeCode.Single:
					return new SingleVariant((IntPtr.Size == 4) ? float.NaN : (float)0);
				case TypeCode.Double:
					return new DoubleVariant((IntPtr.Size == 4) ? Double.NaN : (double)0);
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Or(BaseVariant v1, BaseVariant v2)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 | value2);
					}
				case TypeCode.Int64:
					{
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt64();
						return new LongVariant(value1 | value2);
					}
				case TypeCode.Single:
					return new SingleVariant((IntPtr.Size == 4) ? float.NaN : (float)0);
				case TypeCode.Double:
					return new DoubleVariant((IntPtr.Size == 4) ? Double.NaN : (double)0);
			}
			throw new InvalidOperationException();
		}

		private BaseVariant And(BaseVariant v1, BaseVariant v2)
		{
			var type = CalcTypeCode(v1, v2);
			switch (type)
			{
				case TypeCode.Int32:
					{
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 & value2);
					}
				case TypeCode.Int64:
					{
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt64();
						return new LongVariant(value1 & value2);
					}
				case TypeCode.Single:
					return new SingleVariant((IntPtr.Size == 4) ? float.NaN : (float)0);
				case TypeCode.Double:
					return new DoubleVariant((IntPtr.Size == 4) ? Double.NaN : (double)0);
			}
			throw new InvalidOperationException();
		}

		private int Compare(BaseVariant v1, BaseVariant v2, bool un, int def)
		{
			var res = def;
			var type1 = v1.CalcTypeCode();
			var type2 = v2.CalcTypeCode();
			if (type1 == TypeCode.Object || type2 == TypeCode.Object)
			{
				var obj1 = v1.Value();
				var obj2 = v2.Value();
				if (obj1 == obj2)
					return 0;
				return (obj2 == null) ? 1 : -1;
			}
			else if (type1 == TypeCode.Double || type2 == TypeCode.Double)
				res = v1.ToDouble().CompareTo(v2.ToDouble());
			else if (type1 == TypeCode.Single || type2 == TypeCode.Single)
				res = v1.ToSingle().CompareTo(v2.ToSingle());
			else if (type1 == TypeCode.Int64 || type2 == TypeCode.Int64)
				res = un ? v1.ToUInt64().CompareTo(v2.ToUInt64()) : v1.ToInt64().CompareTo(v2.ToInt64());
			else if (type1 == TypeCode.Int32 || type2 == TypeCode.Int32)
				res = un ? v1.ToUInt32().CompareTo(v2.ToUInt32()) : v1.ToInt32().CompareTo(v2.ToInt32());

			if (res < 0)
				res = -1;
			else if (res > 0)
				res = 1;
			return res;
		}

		private BaseVariant Not(BaseVariant v)
		{
			switch (v.CalcTypeCode())
			{
				case TypeCode.Int32:
					return new IntVariant(~v.ToInt32());
				case TypeCode.Int64:
					return new LongVariant(~v.ToInt64());
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Neg(BaseVariant v)
		{
			switch (v.CalcTypeCode())
			{
				case TypeCode.Int32:
					return new IntVariant(-v.ToInt32());
				case TypeCode.Int64:
					return new LongVariant(-v.ToInt64());
				case TypeCode.Single:
					return new SingleVariant(-v.ToSingle());
				case TypeCode.Double:
					return new DoubleVariant(-v.ToDouble());
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Shr(BaseVariant v1, BaseVariant v2, bool un)
		{
			var type = v1.CalcTypeCode();
			switch (type)
			{
				case TypeCode.Int32:
					if (un)
					{
						var value1 = v1.ToUInt32();
						var value2 = v2.ToInt32();
						return new IntVariant((int)(value1 >> value2));
					} else {
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 >> value2);
					}
				case TypeCode.Int64:
					if (un)
					{
						var value1 = v1.ToUInt64();
						var value2 = v2.ToInt32();
						return new LongVariant((long)(value1 >> value2));
					} else {
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt32();
						return new LongVariant(value1 >> value2);
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Shl(BaseVariant v1, BaseVariant v2)
		{
			var type = v1.CalcTypeCode();
			switch (type)
			{
				case TypeCode.Int32:
					{
						var value1 = v1.ToInt32();
						var value2 = v2.ToInt32();
						return new IntVariant(value1 << value2);
					}
				case TypeCode.Int64:
					{
						var value1 = v1.ToInt64();
						var value2 = v2.ToInt32();
						return new LongVariant(value1 << value2);
					}
			}
			throw new InvalidOperationException();
		}

		private BaseVariant Convert(object obj, Type t)
		{
			BaseVariant v = obj as BaseVariant;

			if (t.IsEnum)
			{
				if (v != null)
					obj = v.Value();
				if (obj != null && !(obj is Enum))
					obj = Enum.ToObject(t, obj);
				return new EnumVariant(obj == null ? (Enum)Activator.CreateInstance(t) : (Enum)obj);
			}

			switch (Type.GetTypeCode(t))
			{
				case TypeCode.Boolean:
					return new BoolVariant(v != null ? v.ToBoolean() : System.Convert.ToBoolean(obj));
				case TypeCode.Char:
					return new CharVariant(v != null ? v.ToChar() : System.Convert.ToChar(obj));
				case TypeCode.SByte:
					return new SbyteVariant(v != null ? v.ToSByte() : System.Convert.ToSByte(obj));
				case TypeCode.Byte:
					return new ByteVariant(v != null ? v.ToByte() : System.Convert.ToByte(obj));
				case TypeCode.Int16:
					return new ShortVariant(v != null ? v.ToInt16() : System.Convert.ToInt16(obj));
				case TypeCode.UInt16:
					return new UshortVariant(v != null ? v.ToUInt16() : System.Convert.ToUInt16(obj));
				case TypeCode.Int32:
					return new IntVariant(v != null ? v.ToInt32() : System.Convert.ToInt32(obj));
				case TypeCode.UInt32:
					return new UintVariant(v != null ? v.ToUInt32() : System.Convert.ToUInt32(obj));
				case TypeCode.Int64:
					return new LongVariant(v != null ? v.ToInt64() : System.Convert.ToInt64(obj));
				case TypeCode.UInt64:
					return new UlongVariant(v != null ? v.ToUInt64() : System.Convert.ToUInt64(obj));
				case TypeCode.Single:
					return new SingleVariant(v != null ? v.ToSingle() : System.Convert.ToSingle(obj));
				case TypeCode.Double:
					return new DoubleVariant(v != null ? v.ToDouble() : System.Convert.ToDouble(obj));
				case TypeCode.String:
					return new StringVariant(v != null ? v.ToString() : (string)obj);
			}

			if (t == typeof(IntPtr))
			{
				if (v != null)
					return new IntPtrVariant(v.ToIntPtr());
				return new IntPtrVariant(obj != null ? (IntPtr)obj : IntPtr.Zero);
			}

			if (t == typeof(UIntPtr))
			{
				if (v != null)
					return new UIntPtrVariant(v.ToUIntPtr());
				return new UIntPtrVariant(obj != null ? (UIntPtr)obj : UIntPtr.Zero);
			}

			if (t.IsValueType)
			{
				if (v != null)
					return new ValueTypeVariant(v.Value());
				return new ValueTypeVariant(obj == null ? Activator.CreateInstance(t) : obj);
			}

			if (t.IsArray)
				return new ArrayVariant(v != null ? (Array)v.Value() : (Array)obj);

			if (t.IsPointer)
			{
				unsafe
				{
					if (v != null)
						return new PointerVariant(Pointer.Box(v.ToPointer(), t), t);
					return new PointerVariant(Pointer.Box(obj != null ? Pointer.Unbox(obj) : null, t), t);
				}
			}

			return new ObjectVariant(v != null ? v.Value() : obj);
		}

		string GetString(int token)
		{
			lock (_cache)
			{
				object stored;
				if (_cache.TryGetValue(token, out stored))
					return (string)stored;
				else
				{
					var str = _module.ResolveString(token);
					_cache.Add(token, str);
					return str;
				}
			}
		}

		private Type GetType(int token)
		{
			lock (_cache)
			{
				object stored;
				if (_cache.TryGetValue(token, out stored))
					return (Type)stored;
				else
				{
					var type = _module.ResolveType(token);
					_cache.Add(token, type);
					return type;
				}
			}
		}

		private MethodBase GetMethod(int token)
		{
			lock (_cache)
			{
				object stored;
				if (_cache.TryGetValue(token, out stored))
					return (MethodBase)stored;
				else
				{
					var method = _module.ResolveMethod(token);
					_cache.Add(token, method);
					return method;
				}
			}
		}

		private FieldInfo GetField(int token)
		{
			lock (_cache)
			{
				object stored;
				if (_cache.TryGetValue(token, out stored))
					return (FieldInfo)stored;
				else
				{
					var field = _module.ResolveField(token);
					_cache.Add(token, field);
					return field;
				}
			}
		}

		private BaseVariant Newobj(MethodBase method)
		{
			var parameters = method.GetParameters();
			var refs = new Dictionary<int, BaseVariant>();
			var args = new object[parameters.Length];
			for (var i = parameters.Length - 1; i >= 0; i--)
			{
				var v = Pop();
				if (v.IsReference())
					refs[i] = v;
				args[i] = Convert(v, parameters[i].ParameterType).Value();
			}
			var res = ((ConstructorInfo)method).Invoke(args);
			foreach (var r in refs)
			{
				r.Value.SetValue(args[r.Key]);
			}
			return Convert(res, method.DeclaringType);
		}

		private bool IsFilteredMethod(MethodBase method, object obj, ref object result, object[] args)
		{
			var t = method.DeclaringType;
			if (t == null)
				return false;

			if (Nullable.GetUnderlyingType(t) != null)
			{
				if (string.Equals(method.Name, "get_HasValue", StringComparison.Ordinal))
				{
					result = (obj != null);
					return true;
				}
				else if (string.Equals(method.Name, "get_Value", StringComparison.Ordinal))
				{
					if (obj == null)
						throw new InvalidOperationException();
					result = obj;
					return true;
				}
				else if (method.Name.Equals("GetValueOrDefault", StringComparison.Ordinal))
				{
					if (obj == null)
						obj = Activator.CreateInstance(Nullable.GetUnderlyingType(method.DeclaringType));
					result = obj;
					return true;
				}
			}
			return false;
		}

		private BaseVariant Call(MethodBase method, bool virt)
		{
			BindingFlags invokeFlags;
#if NETCOREAPP
			invokeFlags = BindingFlags.DoNotWrapExceptions;
#else
			invokeFlags = BindingFlags.Default;
#endif

			var info = method as MethodInfo;
			var parameters = method.GetParameters();
			var refs = new Dictionary<int, BaseVariant>();
			var args = new object[parameters.Length];
			BaseVariant v;
			for (var i = parameters.Length - 1; i >= 0; i--)
			{
				v = Pop();
				if (v.IsReference())
					refs[i] = v;
				args[i] = Convert(v, parameters[i].ParameterType).Value();
			}
			v = method.IsStatic ? null : Pop();
			object obj = v?.Value() ?? null;
			if (virt && obj == null)
				throw new NullReferenceException();
			object res = null;
			if (method.IsConstructor && method.DeclaringType.IsValueType)
			{
				obj = Activator.CreateInstance(method.DeclaringType, invokeFlags, null, args, null);
				if (v != null && v.IsReference())
					v.SetValue(Convert(obj, method.DeclaringType).Value());
			}
			else if (!IsFilteredMethod(method, obj, ref res, args))
			{
				if (!virt && method.IsVirtual && !method.IsFinal)
				{
					DynamicMethod dynamicMethod;
					var paramValues = new object[parameters.Length + 1];
					paramValues[0] = obj;
					for (var i = 0; i < parameters.Length; i++)
					{
						paramValues[i + 1] = args[i];
					}
					lock (_dynamicMethodCache)
					{
						if (!_dynamicMethodCache.TryGetValue(method, out dynamicMethod))
						{
							var paramTypes = new Type[paramValues.Length];
							paramTypes[0] = method.DeclaringType;
							for (var i = 0; i < parameters.Length; i++)
							{
								paramTypes[i + 1] = parameters[i].ParameterType;
							}

							dynamicMethod = new DynamicMethod("", info != null && info.ReturnType != typeof(void) ? info.ReturnType : null, paramTypes, typeof(VirtualMachine).Module, true);
							var gen = dynamicMethod.GetILGenerator();
							gen.Emit(v.IsReference() ? System.Reflection.Emit.OpCodes.Ldarga : System.Reflection.Emit.OpCodes.Ldarg, 0);
							for (var i = 1; i < paramTypes.Length; i++)
							{
								gen.Emit(refs.ContainsKey(i - 1) ? System.Reflection.Emit.OpCodes.Ldarga : System.Reflection.Emit.OpCodes.Ldarg, i);
							}
							gen.Emit(System.Reflection.Emit.OpCodes.Call, info);
							gen.Emit(System.Reflection.Emit.OpCodes.Ret);

							_dynamicMethodCache[method] = dynamicMethod;
						}
					}
					res = dynamicMethod.Invoke(null, invokeFlags, null, paramValues, null);
					foreach (var r in refs)
					{
						r.Value.SetValue(paramValues[r.Key + 1]);
					}
					refs.Clear();
				}
				else
				{
					res = method.Invoke(obj, invokeFlags, null, args, null);
				}
			}
			foreach (var r in refs)
			{
				r.Value.SetValue(args[r.Key]);
			}
			return (info != null && info.ReturnType != typeof(void)) ? Convert(res, info.ReturnType) : null;
		}

		private BaseVariant Callvm(int pos, bool virt)
		{
			var oldPos = _pos;
			_pos = pos;
			var parameterCount = (ushort)ReadInt16();
			var refs = new Dictionary<int, BaseVariant>();
			object[] args = null;
			if (parameterCount > 0)
			{
				args = new object[parameterCount];
				for (var i = parameterCount - 1; i >= 0; i--)
				{
					BaseVariant v = Pop();
					if (v.IsReference())
						refs[i] = v;
					args[i] = Convert(v, GetType(ReadInt32())).Value();
				}
			}
			var returnToken = ReadInt32();
			pos = _pos;
			_pos = oldPos;
			if (virt && (args == null || args[0] == null))
				throw new NullReferenceException();
			object res = new VirtualMachine().Invoke(args, pos);
			foreach (var r in refs)
			{
				r.Value.SetValue(args[r.Key]);
			}
			if (returnToken != 0)
			{
				var returnType = GetType(returnToken);
				if (returnType != typeof(void))
					return Convert(res, returnType);
			}
			return null;
		}

		private bool IsInst(object obj, Type t)
		{
			if (obj == null)
				return true;
			var t2 = obj.GetType();
			if (t2 == t || t.IsAssignableFrom(t2))
				return true;
			return false;
		}

		private void Unwind()
		{
			_stack.Clear();
			_finallyStack.Clear();
			while (_tryStack.Count != 0)
			{
				var catchBlocks = _tryStack.Peek().CatchBlocks();
				int startIndex = (_filterBlock == null) ? 0 : catchBlocks.IndexOf(_filterBlock) + 1;
				_filterBlock = null;
				for (var i = startIndex; i < catchBlocks.Count; i++)
				{
					var current = catchBlocks[i];
					switch (current.Type())
					{
						case 0:
							var type = _exception.GetType();
							var type2 = GetType(current.Filter());
							if (type == type2 || type.IsSubclassOf(type2))
							{
								_tryStack.Pop();
								_stack.Push(new ObjectVariant(_exception));
								_pos = current.Handler();
								return;
							}
							break;
						case 1:
							_filterBlock = current;
							_stack.Push(new ObjectVariant(_exception));
							_pos = current.Filter();
							return;
					}
				}

				_tryStack.Pop();
				for (var i = catchBlocks.Count; i > 0; i--)
				{
					var current = catchBlocks[i - 1];
					if (current.Type() == 2 || current.Type() == 4)
						_finallyStack.Push(current.Handler());
				}
				if (_finallyStack.Count != 0)
				{
					_pos = _finallyStack.Pop();
					return;
				}
			}

			throw _exception;
		}

		private void Initarg()
		{
			var type = GetType(Pop().ToInt32());
			var r = Pop();
			var v = Pop();
			if (r.IsReference())
				v = new ArgReferenceVariant(Convert(v.Value(), type), r);
			else if (r.ToInt32() == 0)
				v = Convert(v.Value(), type);
			else 
				v = new NullReferenceVariant(type);
			_args.Add(v);
		}

		private void Entertry()
		{
			var i = Pop().ToInt32();
			foreach (var current in _tryBlocks)
			{
				if (current.Begin() == i)
					_tryStack.Push(current);
			}
		}

		private void Ldc_i4()
		{
			Push(new IntVariant(ReadInt32()));
		}

		private void Ldc_i8()
		{
			Push(new LongVariant(ReadInt64()));
		}

		private void Ldc_r4()
		{
			Push(new SingleVariant(ReadSingle()));
		}

		private void Ldc_r8()
		{
			Push(new DoubleVariant(ReadDouble()));
		}

		private void Ldnull()
		{
			Push(new ObjectVariant(null));
		}

		private void Ldstr()
		{
			Push(new StringVariant(GetString(Pop().ToInt32())));
		}

		private void Dup()
		{
			Push(Peek().Clone());
		}

		private void Add_()
		{
			var v1 = Pop();
			var v2 = Pop();
			Push(Add(v1, v2, false, false));
		}

		private void Add_ovf()
		{
			var v1 = Pop();
			var v2 = Pop();
			Push(Add(v1, v2, true, false));
		}

		private void Add_ovf_un()
		{
			var v1 = Pop();
			var v2 = Pop();
			Push(Add(v1, v2, true, true));
		}

		private void Sub_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Sub(v1, v2, false, false));
		}

		private void Sub_ovf()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Sub(v1, v2, true, false));
		}

		private void Sub_ovf_un()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Sub(v1, v2, true, true));
		}

		private void Mul_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Mul(v1, v2, false, false));
		}

		private void Mul_ovf()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Mul(v1, v2, true, false));
		}

		private void Mul_ovf_un()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Mul(v1, v2, true, true));
		}

		private void Div_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Div(v1, v2, false));
		}

		private void Div_un()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Div(v1, v2, true));
		}

		private void Rem_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Rem(v1, v2, false));
		}

		private void Rem_un()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Rem(v1, v2, true));
		}

		private void And_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(And(v1, v2));
		}

		private void Or_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Or(v1, v2));
		}

		private void Xor_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Xor(v1, v2));
		}

		private void Not_()
		{
			var v = Pop();
			Push(Not(v));
		}

		private void Neg_()
		{
			var v = Pop();
			Push(Neg(v));
		}

		private void Shr_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Shr(v1, v2, false));
		}

		private void Shr_un()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Shr(v1, v2, true));
		}

		private void Shl_()
		{
			var v2 = Pop();
			var v1 = Pop();
			Push(Shl(v1, v2));
		}

		private void Conv()
		{
			var type = GetType(Pop().ToInt32());
			Push(Convert(Pop(), type));
		}

		private void Conv_ovf()
		{
			var type = GetType(Pop().ToInt32());
			Push(Convert(Pop().Conv_ovf(type, false), type));
		}

		private void Conv_ovf_un()
		{
			var type = GetType(Pop().ToInt32());
			Push(Convert(Pop().Conv_ovf(type, true), type));
		}

		private void Sizeof()
		{
			var type = GetType(Pop().ToInt32());
			var dynamicMethod = new DynamicMethod("", typeof(int), null, typeof(VirtualMachine).Module, true);
			var gen = dynamicMethod.GetILGenerator();
			gen.Emit(System.Reflection.Emit.OpCodes.Sizeof, type);
			gen.Emit(System.Reflection.Emit.OpCodes.Ret);

			Push(new IntVariant((int)dynamicMethod.Invoke(null, null)));
		}

		private void Ldind()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			if (!v.IsReference())
			{
				if (v.Value() is Pointer)
					unsafe { v = new PointerReference(new IntPtr(Pointer.Unbox(v.Value())), type); }
				else
					throw new ArgumentException();
			}
			Push(Convert(v, type));
		}

		private void Ldfld()
		{
			var field = GetField(Pop().ToInt32());
			var v = Pop();
			object obj;
			if (v.Type().IsPointer)
				obj = Marshal.PtrToStructure(v.ToIntPtr(), v.Type().GetElementType());
			else
				obj = v.Value();
			Push(Convert(field.GetValue(obj), field.FieldType));
		}

		private void Ldflda()
		{
			var field = GetField(Pop().ToInt32());
			var v = Pop();
			Push(new FieldReferenceVariant(field, v));
		}

		private void Stfld()
		{
			var field = GetField(Pop().ToInt32());
			var v = Pop();
			var baseVariant = Pop();
			var obj = baseVariant.Value();
			baseVariant.SetFieldValue(field, Convert(v, field.FieldType).Value());
		}

		private void Stsfld()
		{
			var field = GetField(Pop().ToInt32());
			var v = Pop();
			field.SetValue(null, Convert(v, field.FieldType).Value());
		}

		private void Stind()
		{
			var type = GetType(Pop().ToInt32());
			var v2 = Pop();
			var v1 = Pop();
			v2 = Convert(v2, type);
			if (v1.IsReference())
				v2 = Convert(v2, v1.Type());
			else {
				if (v1.Value() is Pointer)
					unsafe { v1 = new PointerReference(new IntPtr(Pointer.Unbox(v1.Value())), type); }
				else
					throw new ArgumentException();
			}
			v1.SetValue(v2.Value());
		}

		private void Ldarg()
		{
			Push(_args[ReadInt16()].Clone());
		}

		private void Ldarga()
		{
			var v = _args[ReadInt16()];
			if (v.IsReference())
				throw new ArgumentException();
			Push(new ReferenceVariant(v));
		}

		private void Starg()
		{
			var v2 = Pop();
			int index = ReadInt16();
			var v1 = _args[index];
			if (v1.IsReference())
			{
				if (!v2.IsReference())
					throw new ArgumentException();
				_args[index] = v2;
			}
			else
			{
				v1.SetValue(Convert(v2, v1.Type()).Value());
			}
		}

		private void Constrained()
		{
			_constrained = GetType(Pop().ToInt32());
		}

		private void Call_()
		{
			var method = GetMethod(Pop().ToInt32());
			var v = Call(method, false);
			if (v != null)
				Push(v);
		}

		private void Callvirt()
		{
			var method = GetMethod(Pop().ToInt32());
			if (_constrained != null)
			{
				var parameters = method.GetParameters();
				var types = new Type[parameters.Length];
				var num = 0;
				foreach (var param in parameters)
				{
					types[num++] = param.ParameterType;
				}
				var new_method = _constrained.GetMethod(method.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.InvokeMethod | BindingFlags.GetProperty | BindingFlags.SetProperty, null, types, null);
				if (new_method != null)
					method = new_method;
				_constrained = null;
			}
			var v = Call(method, true);
			if (v != null)
				Push(v);
		}

		private void Calli()
		{
			var method = Pop().Value() as MethodBase;
			if (method == null)
				throw new ArgumentException();
			var v = Call(method, false);
			if (v != null)
				Push(v);
		}

		private void Callvm_()
		{
			var v = Callvm(Pop().ToInt32(), false);
			if (v != null)
				Push(v);
		}

		private void Callvmvirt()
		{
			var v = Callvm(Pop().ToInt32(), true);
			if (v != null)
				Push(v);
		}

		private void Newobj_()
		{
			var method = GetMethod(Pop().ToInt32());
			var v = Newobj(method);
			if (v != null)
				Push(v);
		}

		private void Initobj()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			object obj = null;
			if (type.IsValueType)
			{
				if (Nullable.GetUnderlyingType(type) == null)
					obj = FormatterServices.GetUninitializedObject(type);
			}
			v.SetValue(obj);
		}

		private void Cmp()
		{
			var def = Pop().ToInt32();
			var v2 = Pop();
			var v1 = Pop();
			Push(new IntVariant(Compare(v1, v2, false, def)));
		}

		private void Cmp_un()
		{
			var def = Pop().ToInt32();
			var v2 = Pop();
			var v1 = Pop();
			Push(new IntVariant(Compare(v1, v2, true, def)));
		}

		private void Newarr()
		{
			var type = GetType(Pop().ToInt32());
			Push(new ArrayVariant(Array.CreateInstance(type, Pop().ToInt32())));
		}

		private void Stelem()
		{
			var type = GetType(Pop().ToInt32());
			var v2 = Pop();
			var v1 = Pop();
			var array = Pop().Value() as Array;
			if (array == null)
				throw new ArgumentException();
			array.SetValue(Convert(Convert(v2, type), array.GetType().GetElementType()).Value(), v1.ToInt32());
		}

		private void Ldelem()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			var array = Pop().Value() as Array;
			if (array == null)
				throw new ArgumentException();
			Push(Convert(array.GetValue(v.ToInt32()), type));
		}

		private void Ldlen()
		{
			var array = Pop().Value() as Array;
			if (array == null)
				throw new ArgumentException();
			Push(new IntVariant(array.Length));
		}

		private void Ldelema()
		{
			var v = Pop();
			var array = Pop().Value() as Array;
			if (array == null)
				throw new ArgumentException();
			Push(new ArrayElementVariant(array, v.ToInt32()));
		}

		private void Ldftn()
		{
			Push(new MethodVariant(GetMethod(Pop().ToInt32())));
		}

		private void Ldvirtftn()
		{
			var method = GetMethod(Pop().ToInt32());
			var type = Pop().Value().GetType();
			var declaringType = method.DeclaringType;
			var parameters = method.GetParameters();
			var types = new Type[parameters.Length];
			var num = 0;
			foreach (var param in parameters)
			{
				types[num++] = param.ParameterType;
			}
			while (type != null && type != declaringType)
			{
				var new_method = type.GetMethod(method.Name, BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty | BindingFlags.SetProperty | BindingFlags.ExactBinding, null, CallingConventions.Any, types, null);
				if (new_method != null && new_method.GetBaseDefinition() == method)
				{
					method = new_method;
					break;
				}
				type = type.BaseType;
			}
			Push(new MethodVariant(method));
		}

		private void Br()
		{
			_pos = Pop().ToInt32();
		}

		private void Pop_()
		{
			Pop();
		}

		private void Leave()
		{
			_finallyStack.Push(Pop().ToInt32());
			var offset = Pop().ToInt32();
			while (_tryStack.Count != 0 && offset > _tryStack.Peek().End())
			{
				var catchBlocks = _tryStack.Pop().CatchBlocks();
				for (var i = catchBlocks.Count; i > 0; i--)
				{
					var current = catchBlocks[i - 1];
					if (current.Type() == 2)
						_finallyStack.Push(current.Handler());
				}
			}

			_exception = null;
			_stack.Clear();
			_pos = _finallyStack.Pop();
		}

		private void Endfinally()
		{
			if (_exception == null)
			{
				_pos = _finallyStack.Pop();
				return;
			}
			Unwind();
		}

		private void Endfilter()
		{
			if (Pop().ToInt32() != 0)
			{
				_tryStack.Pop();
				_stack.Push(new ObjectVariant(_exception));
				_pos = _filterBlock.Handler();
				_filterBlock = null;
				return;
			}
			Unwind();
		}

		private void Box()
		{
			var type = GetType(Pop().ToInt32());
			Push(new ObjectVariant(Convert(Pop(), type).Value()));
		}

		private void Unbox()
		{
			var type = GetType(Pop().ToInt32());
			Push(Convert(Pop().Value(), type));
		}

		private void Unbox_any()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			var obj = v.Value();
			if (obj == null)
				throw new NullReferenceException();

			if (type.IsValueType)
			{
				if (type != obj.GetType())
					throw new InvalidCastException();
				Push(Convert(obj, type));
			}
			else switch (Type.GetTypeCode(type))
			{
				case TypeCode.Boolean:
					Push(new BoolVariant((bool)obj));
					break;
				case TypeCode.Char:
					Push(new CharVariant((char)obj));
					break;
				case TypeCode.SByte:
					Push(new SbyteVariant((sbyte)obj));
					break;
				case TypeCode.Byte:
					Push(new ByteVariant((byte)obj));
					break;
				case TypeCode.Int16:
					Push(new ShortVariant((short)obj));
					break;
				case TypeCode.UInt16:
					Push(new UshortVariant((ushort)obj));
					break;
				case TypeCode.Int32:
					Push(new IntVariant((int)obj));
					break;
				case TypeCode.UInt32:
					Push(new UintVariant((uint)obj));
					break;
				case TypeCode.Int64:
					Push(new LongVariant((long)obj));
					break;
				case TypeCode.UInt64:
					Push(new UlongVariant((ulong)obj));
					break;
				case TypeCode.Single:
					Push(new SingleVariant((float)obj));
					break;
				case TypeCode.Double:
					Push(new DoubleVariant((double)obj));
					break;
				default:
					throw new InvalidCastException();
			}
		}

		private void Ldmem_i4()
		{
			Push(new IntVariant(Marshal.ReadInt32(new IntPtr(_instance + Pop().ToUInt32()))));
		}

		private void Ldtoken()
		{
			var i = Pop().ToInt32();
			switch (i >> 24)
			{
				case 04:
					Push(new ValueTypeVariant(_module.ModuleHandle.ResolveFieldHandle(i)));
					break;
				case 0x01:
				case 0x02:
				case 0x1b:
					Push(new ValueTypeVariant(_module.ModuleHandle.ResolveTypeHandle(i)));
					break;
				case 0x06:
				case 0x2b:
					Push(new ValueTypeVariant(_module.ModuleHandle.ResolveMethodHandle(i)));
					break;
				case 0x0a:
					try
					{
						Push(new ValueTypeVariant(_module.ModuleHandle.ResolveFieldHandle(i)));
					}
					catch
					{
						Push(new ValueTypeVariant(_module.ModuleHandle.ResolveMethodHandle(i)));
					}
					break;
				default:
					throw new InvalidOperationException();
			}
		}

		private void Throw()
		{
			var e = Pop().Value() as Exception;
			if (e == null)
				throw new ArgumentException();
			throw e;
		}

		private void Rethrow()
		{
			if (_exception == null)
				throw new InvalidOperationException();
			throw _exception;
		}

		private void Castclass()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			if (!IsInst(v.Value(), type))
				throw new InvalidCastException();
			Push(v);
		}

		private void Isinst()
		{
			var type = GetType(Pop().ToInt32());
			var v = Pop();
			if (!IsInst(v.Value(), type))
				v = new ObjectVariant(null);
			Push(v);
		}

		private void Ckfinite()
		{
			var v = Pop();
			if (v.Value() is IConvertible)
			{
				var d = v.ToDouble();
				if (double.IsNaN(d) || double.IsInfinity(d))
					throw new OverflowException();
			}
			else
			{
				v = new DoubleVariant(double.NaN);
			}
			Push(v);
		}

		private void Localloc()
		{
			var ptr = Marshal.AllocHGlobal(Pop().ToIntPtr());
			_localloc.Add(ptr);
			unsafe {
				Push(new ObjectVariant(Pointer.Box(ptr.ToPointer(), typeof(void*))));
			}
		}

		private void FreeAllLocalloc()
		{
			foreach (var ptr in _localloc)
			{
				Marshal.FreeHGlobal(ptr);
			}
		}

		public object Invoke(object[] args, int pos)
		{
			_pos = pos;
			Push(new ArrayVariant(args));
			try
			{
				while (true)
				{
					try
					{
						_handlers[ReadByte()]();
						if (_pos == 0)
							break;
					}
					catch (Exception e)
					{
						if (_filterBlock == null)
							_exception = e;
						Unwind();
					}
				}
				return Pop().Value();
			}
			finally
			{
				FreeAllLocalloc();
			}
		}
	}
}