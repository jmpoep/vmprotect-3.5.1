using System;
using System.Runtime.Serialization;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000045 RID: 69
    internal static class VariantFactory // \u0008\u2002
    {
        // Token: 0x060002F6 RID: 758 RVA: 0x000144E0 File Offset: 0x000126E0
        private static bool IsDerivedFrom<T>(Type obj) // \u0002
	    {
		    var typeFromHandle = typeof(T);
		    return obj == typeFromHandle || obj.IsSubclassOf(typeFromHandle);
        }

	    // Token: 0x060002F7 RID: 759 RVA: 0x00014508 File Offset: 0x00012708
	    public static VariantBase Convert(object src, Type t) // \u0002
	    {
		    var ret = src as VariantBase;
		    if (ret != null)
		    {
			    return ret;
		    }
		    if (t == null)
		    {
			    if (src == null)
			    {
				    return new ObjectVariant();
			    }
			    t = src.GetType();
		    }
		    t = ElementedTypeHelper.TryGoToPointerOrReferenceElementType(t);
		    if (t == SimpleTypeHelper.ObjectType)
		    {
			    ret = new ObjectVariant();
			    if (src != null && src.GetType() != SimpleTypeHelper.ObjectType)
			    {
				    ret.SetVariantType(src.GetType());
			    }
		    }
		    else if (IsDerivedFrom<Array>(t))
		    {
			    ret = new ArrayVariant();
		    }
		    else if (IsDerivedFrom<string>(t))
		    {
			    ret = new StringVariant();
		    }
		    else if (IsDerivedFrom<IntPtr>(t))
		    {
			    ret = new IntPtrVariant();
		    }
		    else if (IsDerivedFrom<UIntPtr>(t))
		    {
			    ret = new UIntPtrVariant();
		    }
		    else if (IsDerivedFrom<ulong>(t))
		    {
			    ret = new UlongVariant();
		    }
		    else if (IsDerivedFrom<uint>(t))
		    {
			    ret = new UintVariant();
		    }
		    else if (IsDerivedFrom<ushort>(t))
		    {
			    ret = new UshortVariant();
		    }
		    else if (IsDerivedFrom<long>(t))
		    {
			    ret = new LongVariant();
		    }
		    else if (IsDerivedFrom<int>(t))
		    {
			    ret = new IntVariant();
		    }
		    else if (IsDerivedFrom<short>(t))
		    {
			    ret = new ShortVariant();
		    }
		    else if (IsDerivedFrom<byte>(t))
		    {
			    ret = new ByteVariant();
		    }
		    else if (IsDerivedFrom<sbyte>(t))
		    {
			    ret = new SbyteVariant();
		    }
		    else if (IsDerivedFrom<double>(t))
		    {
			    ret = new DoubleVariant();
		    }
		    else if (IsDerivedFrom<float>(t))
		    {
			    ret = new FloatVariant();
		    }
		    else if (IsDerivedFrom<bool>(t))
		    {
			    ret = new BoolVariant();
		    }
		    else if (IsDerivedFrom<char>(t))
		    {
			    ret = new CharVariant();
		    }
		    else if (SimpleTypeHelper.IsNullableGeneric(t))
		    {
			    var ov = new ObjectVariant();
			    ov.SetVariantType(t);
			    ret = ov;
		    }
		    else
		    {
			    if (IsDerivedFrom<Enum>(t))
			    {
				    Enum e;
				    if (src == null)
				    {
					    e = (Enum)Activator.CreateInstance(t);
				    }
				    else if (t == SimpleTypeHelper.EnumType && src is Enum)
				    {
					    e = (Enum)src;
				    }
				    else
				    {
					    e = (Enum)Enum.ToObject(t, src);
				    }
				    return new EnumVariant(e);
			    }
			    if (IsDerivedFrom<ValueType>(t))
			    {
				    if (src == null)
				    {
				        var vt = (t == SimpleTypeHelper.TypedReferenceType) ? 
                            FormatterServices.GetSafeUninitializedObject(SimpleTypeHelper.TypedReferenceType) : 
                            Activator.CreateInstance(t);
				        ret = new ValueTypeVariant(vt);
				    }
				    else
				    {
					    if (src.GetType() != t)
					    {
						    try
						    {
							    src = System.Convert.ChangeType(src, t);
						    }
						    catch
						    {
						        // ignored
						    }
					    }
					    ret = new ValueTypeVariant(src);
				    }
				    return ret;
			    }
			    ret = new ObjectVariant();
		    }
		    if (src != null)
		    {
			    ret.SetValueAbstract(src);
		    }
		    return ret;
	    }
    }
}
