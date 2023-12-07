using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Threading;

// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedParameter.Local
// ReSharper disable MemberCanBePrivate.Global
// ReSharper disable EmptyGeneralCatchClause
// ReSharper disable RedundantCast
// ReSharper disable PossibleNullReferenceException
// ReSharper disable AssignNullToNotNullAttribute
// ReSharper disable UnusedMember.Global

namespace UnitTestProject.RefVm
{
    // Token: 0x02000032 RID: 50
    public class VmExecutor // \u0006\u2007
    {
        #region subclasses

	    // Token: 0x02000033 RID: 51
	    [Serializable]
	    private sealed class CatchBlockComparer // \u0002
        {
		    // Token: 0x060002B0 RID: 688 RVA: 0x00012EB4 File Offset: 0x000110B4
		    internal int Compare(CatchBlock v1, CatchBlock v2) // \u0002
            {
			    if (v1.ExcTypeId == v2.ExcTypeId)
			    {
				    return v2.Start.CompareTo(v1.Start);
			    }
			    return v1.ExcTypeId.CompareTo(v2.ExcTypeId);
		    }

		    // Token: 0x04000155 RID: 341
		    public static readonly CatchBlockComparer Instance = new CatchBlockComparer(); // \u0002

		    // Token: 0x04000156 RID: 342
		    public static Comparison<CatchBlock> MyComparison; // \u0003
	    }

        // Token: 0x02000038 RID: 56
        private sealed class StringTypePair // \u0006
        {
            // Token: 0x060002C0 RID: 704 RVA: 0x00013020 File Offset: 0x00011220
            // Token: 0x060002C1 RID: 705 RVA: 0x00013028 File Offset: 0x00011228
            // Token: 0x0400015D RID: 349
            public string Str { get; set; } // \u0002

            // Token: 0x060002C3 RID: 707 RVA: 0x0001303C File Offset: 0x0001123C
            // Token: 0x060002C2 RID: 706 RVA: 0x00013034 File Offset: 0x00011234
            // Token: 0x0400015E RID: 350
            public Type T { get; set; } // \u0002
        }

        // Token: 0x02000039 RID: 57
        // (Invoke) Token: 0x060002C5 RID: 709
        private delegate object DynamicExecutor(object obj, object[] args); // \u0008

        // Token: 0x0200003A RID: 58
        // (Invoke) Token: 0x060002C9 RID: 713
        internal delegate void VmInstrImpl(VariantBase t); // \u000E

        // Token: 0x0200003B RID: 59
        private sealed class VmInstr // \u000F
        {
            // Token: 0x060002CC RID: 716 RVA: 0x00013048 File Offset: 0x00011248
            public VmInstr(VmInstrInfo id, VmInstrImpl func)
            {
                Id = id;
                Func = func;
            }

            // Token: 0x0400015F RID: 351
            public readonly VmInstrInfo Id; // \u0002

            // Token: 0x04000160 RID: 352
            public readonly VmInstrImpl Func; // \u0003
        }

        // Token: 0x02000037 RID: 55
        private sealed class ExcHandlerFrame // \u0005
        {
            // Token: 0x060002BB RID: 699 RVA: 0x00012FF0 File Offset: 0x000111F0
            // Token: 0x060002BC RID: 700 RVA: 0x00012FF8 File Offset: 0x000111F8
            // Token: 0x0400015B RID: 347
            public uint Pos { get; set; }

            // Token: 0x060002BD RID: 701 RVA: 0x00013004 File Offset: 0x00011204
            // Token: 0x060002BE RID: 702 RVA: 0x0001300C File Offset: 0x0001120C
            // Token: 0x0400015C RID: 348
            public object Exception { get; set; }
        }

        // Token: 0x02000034 RID: 52
        internal struct MethodBaseAndVirtual : IEquatable<MethodBaseAndVirtual> // \u0002\u2000
        {
            public MethodBaseAndVirtual(MethodBase mb, bool isVirtual)
            {
                Val = mb;
                IsVirtual = isVirtual;
            }

            // Token: 0x060002B1 RID: 689 RVA: 0x00012EF8 File Offset: 0x000110F8
            // Token: 0x060002B2 RID: 690 RVA: 0x00012F00 File Offset: 0x00011100
            // Token: 0x04000157 RID: 343
            public MethodBase Val /* \u0002 */ { get; }

            // Token: 0x060002B3 RID: 691 RVA: 0x00012F0C File Offset: 0x0001110C
            // Token: 0x060002B4 RID: 692 RVA: 0x00012F14 File Offset: 0x00011114
            // Token: 0x04000158 RID: 344
            public bool IsVirtual /* \u0003 */ { get; }

            // Token: 0x060002B5 RID: 693 RVA: 0x00012F20 File Offset: 0x00011120
            public override int GetHashCode()
            {
                return Val.GetHashCode() ^ IsVirtual.GetHashCode();
            }

            // Token: 0x060002B6 RID: 694 RVA: 0x00012F48 File Offset: 0x00011148
            public override bool Equals(object o)
            {
                if (o is MethodBaseAndVirtual)
                {
                    return Equals((MethodBaseAndVirtual)o);
                }
                return false;
            }

            // Token: 0x060002B7 RID: 695 RVA: 0x00012F70 File Offset: 0x00011170
            public bool Equals(MethodBaseAndVirtual val)
            {
                return IsVirtual == val.IsVirtual && Val == val.Val;
            }
        }

        // Token: 0x02000035 RID: 53
        private struct BoolHolder // \u0003
        {
            // Token: 0x04000159 RID: 345
            public bool Val; // \u0002
        }

        // Token: 0x02000036 RID: 54
        private sealed class IntToTypeComparer<T> : IComparer<KeyValuePair<int, T>> // \u0003\u2000
        {
            // Token: 0x060002B8 RID: 696 RVA: 0x00012F94 File Offset: 0x00011194
            public IntToTypeComparer(Comparison<T> c)
            {
                _c = c;
            }

            // Token: 0x060002B9 RID: 697 RVA: 0x00012FA4 File Offset: 0x000111A4
            public int Compare(KeyValuePair<int, T> v1, KeyValuePair<int, T> v2)
            {
                var num = _c(v1.Value, v2.Value);
                if (num == 0)
                {
                    return v2.Key.CompareTo(v1.Key);
                }
                return num;
            }

            // Token: 0x0400015A RID: 346
            private readonly Comparison<T> _c;
        }

        // Token: 0x0200000B RID: 11
        private static class HiByte // \u0002\u2008
        {
            // Token: 0x06000056 RID: 86 RVA: 0x00003154 File Offset: 0x00001354
            public static int Extract(int src) // \u0002
            {
                return src & -16777216; // 0xFF000000
            }
        }

        #endregion

        #region members
        // Token: 0x04000132 RID: 306
        private static readonly Type MethodBaseType = typeof(MethodBase); // \u0002\u2001

	    // Token: 0x04000133 RID: 307
	    private VmMethodHeader _methodHeader; // \u000E

	    // Token: 0x04000134 RID: 308
	    private readonly MyCollection<VariantBase> _evalStack = new MyCollection<VariantBase>(); // \u0003\u2003

	    // Token: 0x04000135 RID: 309
	    private readonly Dictionary<int, object> AllMetadataById = new Dictionary<int, object>(); // \u0006\u2003

	    // Token: 0x04000136 RID: 310
	    private readonly VmInstrCodesDb _instrCodesDb; // \u0006\u2001

        // Token: 0x04000137 RID: 311
        private object _exception; // \u0006\u2002

	    // Token: 0x04000138 RID: 312
	    private readonly Dictionary<MethodBase, object> _mbDynamicLock = new Dictionary<MethodBase, object>(); // \u0008\u2002

	    // Token: 0x04000139 RID: 313
        private BinaryReader _srcVirtualizedStreamReader; // \u0002\u2000

	    // Token: 0x0400013A RID: 314
	    private Type _currentClass; // \u000F\u2001

	    // Token: 0x0400013B RID: 315
	    private static readonly Type AssemblyType = typeof(Assembly); // \u0002\u2003

	    // Token: 0x0400013C RID: 316
	    private readonly Dictionary<MethodBase, int> _mbCallCnt = new Dictionary<MethodBase, int>(256); // \u0006

	    // Token: 0x0400013D RID: 317
	    private Stream _srcVirtualizedStream; // \u000F\u2000

        // Token: 0x0400013E RID: 318
        private object[] _callees; // \u0005\u2000

	    // Token: 0x0400013F RID: 319
	    private readonly MyCollection<ExcHandlerFrame> _ehStack = new MyCollection<ExcHandlerFrame>(); // \u000F

	    // Token: 0x04000140 RID: 320
	    private VariantBase[] _localVariables; // \u0003\u2002

	    // Token: 0x04000141 RID: 321
	    private readonly Dictionary<MethodBaseAndVirtual, DynamicExecutor> _dynamicExecutors = new Dictionary<MethodBaseAndVirtual, DynamicExecutor>(256); // \u0002\u2002

	    // Token: 0x04000142 RID: 322
	    private bool _retFound; // \u0005\u2003

	    // Token: 0x04000143 RID: 323
	    private MyBufferReader _myBufferReader; // \u0002

	    // Token: 0x04000144 RID: 324
	    private Type[] _classGenericArgs; // \u0003

	    // Token: 0x04000145 RID: 325
	    private readonly Module _module; // \u0008\u2001

        // Token: 0x04000146 RID: 326
        private long _myBufferPos; // \u0005

	    // Token: 0x04000147 RID: 327
	    private byte[] _methodBody; // \u0008

        // Token: 0x04000148 RID: 328
        private static readonly Type ObjectArrayType = typeof(object[]); // \u000E\u2001

	    // Token: 0x04000149 RID: 329
	    private static readonly Dictionary<MethodBase, DynamicMethod> DynamicMethods = new Dictionary<MethodBase, DynamicMethod>(); // \u000E\u2003

	    // Token: 0x0400014A RID: 330
	    private bool _wasException; // \u0008\u2000

	    // Token: 0x0400014B RID: 331
	    private Type[] _methodGenericArgs; // \u0005\u2001

	    // Token: 0x0400014C RID: 332
	    private CatchBlock[] _catchBlocks; // \u000F\u2003

	    // Token: 0x0400014D RID: 333
	    private static readonly object InterlockedLock = new object(); // \u0008\u2003

	    // Token: 0x0400014E RID: 334
	    private uint? _storedPos; // \u000E\u2000

	    // Token: 0x0400014F RID: 335
	    private const bool _alwaysFalse = false; // \u000F\u2002

	    // Token: 0x04000150 RID: 336
	    private Dictionary<int, VmInstr> _vmInstrDb; // \u000E\u2002

        // Token: 0x04000151 RID: 337
        private VariantBase[] _variantOutputArgs; // \u0005\u2002

	    // Token: 0x04000152 RID: 338
	    private const bool _alwaysTrue = true; // \u0003\u2001

	    // Token: 0x04000153 RID: 339
	    private static readonly Type IntPtrType = typeof(IntPtr); // \u0003\u2000

	    // Token: 0x04000154 RID: 340
	    private static readonly Type VoidType = typeof(void); // \u0006\u2000
       
        #endregion

        // Token: 0x0600016C RID: 364 RVA: 0x00007958 File Offset: 0x00005B58
        public VmExecutor(VmInstrCodesDb instrCodesDb, Module m)
	    {
            _instrCodesDb = instrCodesDb;
		    _module = m;
		    Init();
	    }

	    // Token: 0x060001B6 RID: 438 RVA: 0x000092E0 File Offset: 0x000074E0
	    private void Init() // \u000F
        {
		    if (!_instrCodesDb.IsInitialized())
		    {
			    lock (_instrCodesDb)
			    {
				    if (!_instrCodesDb.IsInitialized())
				    {
					    _vmInstrDb = CreateVmInstrDb();
					    DoNothing();
					    _instrCodesDb.SetInitialized(true);
				    }
			    }
		    }
		    if (_vmInstrDb == null)
		    {
			    _vmInstrDb = CreateVmInstrDb();
		    }
	    }

        // Token: 0x060001BD RID: 445 RVA: 0x00009534 File Offset: 0x00007734
        private void DoNothing() // \u0006
        {}

        // Token: 0x06000239 RID: 569 RVA: 0x0000F9C4 File Offset: 0x0000DBC4
        private VariantBase PopVariant() // \u0002
        {
		    return _evalStack.PopBack();
	    }

        // Token: 0x060002A0 RID: 672 RVA: 0x000127D8 File Offset: 0x000109D8
        private long PopLong() // \u0002
        {
            var top = PopVariant();
            switch (top.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum: return VariantBase.SignedLongFromEnum((EnumVariant)top); // bug was fixed and unit tested (Convert.ToInt64(((EnumVariant)top).GetValue());)
                case VariantBase.Vtc.Tc13UIntPtr: return (long)((UIntPtrVariant)top).GetValue().ToUInt64();
                case VariantBase.Vtc.Tc17IntPtr: return ((IntPtrVariant)top).GetValue().ToInt64();
                case VariantBase.Vtc.Tc19Int: return ((IntVariant)top).GetValue();
            }
            throw new Exception(StringDecryptor.GetString(-1550345551) /* Unexpected value on the stack. */);
        }

        // Token: 0x060001A1 RID: 417 RVA: 0x00008CC4 File Offset: 0x00006EC4
        private void Ldelem(Type t) // \u0002 
        {
            var index = PopLong();
            var array = (Array)PopVariant().GetValueAbstract();
            PushVariant(VariantFactory.Convert(array.GetValue(index), t));
        }

        // Token: 0x0600020C RID: 524 RVA: 0x0000C528 File Offset: 0x0000A728
        private void Ldelem_(VariantBase vTypeId) // \u0006\u200A\u2000
        {
            var typeId = ((IntVariant)vTypeId).GetValue();
            var type = GetTypeById(typeId);
            Ldelem(type);
        }

        // Token: 0x060002A5 RID: 677 RVA: 0x00012A80 File Offset: 0x00010C80
        private void PushVariant(VariantBase obj) // \u0008\u2000\u2001
        {
		    if (obj == null)
		    {
			    throw new ArgumentNullException(StringDecryptor.GetString(-1550345950) /* obj */);
		    }
		    VariantBase push;
		    if (obj.GetVariantType() != null)
		    {
			    push = obj;
		    }
		    else
		    {
			    switch (obj.GetTypeCode())
			    {
			        case VariantBase.Vtc.Tc1Bool:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((BoolVariant)obj).GetValue() ? 1 : 0);
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc6Char:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((CharVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc7Ulong:
			        {
				        var tmp = new LongVariant();
				        tmp.SetValue((long)((UlongVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc8Float:
			        {
				        var tmp = new FloatVariant();
				        tmp.SetValue(((FloatVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc9Uint:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue((int)((UintVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc10Ushort:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((UshortVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc12Sbyte:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((SbyteVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc14Byte:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((ByteVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc15Short:
			        {
				        var tmp = new IntVariant();
				        tmp.SetValue(((ShortVariant)obj).GetValue());
				        tmp.SetVariantType(obj.GetVariantType());
				        push = tmp;
				        break;
			        }
			        case VariantBase.Vtc.Tc18Object:
			        {
				        var abs = obj.GetValueAbstract();
				        if (abs == null)
				        {
					        push = obj;
					        break;
				        }
				        var type = abs.GetType();
				        if (type.HasElementType && !type.IsArray)
				        {
					        type = type.GetElementType();
				        }
				        if (type != null && !type.IsValueType && !type.IsEnum)
				        {
					        push = obj;
					        break;
				        }
				        push = VariantFactory.Convert(abs, type);
				        break;
			        }
                    case VariantBase.Vtc.Tc17IntPtr:
                    case VariantBase.Vtc.Tc13UIntPtr:
                        if(IntPtr.Size == 4)
                        {
                            var tmp = new IntVariant();
                            tmp.CopyFrom(obj);
                            tmp.SetVariantType(obj.GetVariantType());
                            push = tmp;
                        }
                        else
                        {
                            var tmp = new LongVariant();
                            tmp.CopyFrom(obj);
                            tmp.SetVariantType(obj.GetVariantType());
                            push = tmp;
                        }
                        break;
                    default:
			            push = obj;
                        break;
			    }
            }
		    _evalStack.PushBack(push);
	    }

	    // Token: 0x0600027D RID: 637 RVA: 0x00011C24 File Offset: 0x0000FE24
	    private void Conv_ovf_i4_un_(VariantBase dummy) // \u0003\u200B
        {
		    Conv_i4(true, false);
	    }

	    // Token: 0x06000283 RID: 643 RVA: 0x00011E9C File Offset: 0x0001009C
	    public static void Sort<T>(T[] arr, Comparison<T> c) // \u0002
        {
		    var array = new KeyValuePair<int, T>[arr.Length];
		    for (var i = 0; i < arr.Length; i++)
		    {
			    array[i] = new KeyValuePair<int, T>(i, arr[i]);
		    }
		    Array.Sort(array, arr, new IntToTypeComparer<T>(c));
	    }
	    
        // Token: 0x060001BE RID: 446 RVA: 0x00009538 File Offset: 0x00007738
	    private void SortCatchBlocks() // \u0003\u2000
	    {
		    if (CatchBlockComparer.MyComparison == null)
		    {
			    CatchBlockComparer.MyComparison = CatchBlockComparer.Instance.Compare;
		    }
		    Sort(_catchBlocks, CatchBlockComparer.MyComparison);
	    }

	    // Token: 0x0600016D RID: 365 RVA: 0x000079C8 File Offset: 0x00005BC8
	    public VmExecutor(VmInstrCodesDb instrCodesDb, Stream virtualizedStream = null) : this(instrCodesDb, typeof(VmExecutor).Module) // \u0006\u2007
        {
            _srcVirtualizedStream = virtualizedStream;
        }

        // Token: 0x0600023C RID: 572 RVA: 0x0000FA0C File Offset: 0x0000DC0C
        public object Invoke(Stream virtualizedStream, string pos, object[] args) // \u0002
        {
            // ReSharper disable once IntroduceOptionalParameters.Global
            return Invoke(virtualizedStream, pos, args, null, null, null);
        }

	    // Token: 0x0600017A RID: 378 RVA: 0x00007DCC File Offset: 0x00005FCC
	    public object Invoke(Stream virtualizedStream, string pos, object[] args, Type[] methodGenericArgs, Type[] classGenericArgs, object[] callees) // \u0002
        {
            _srcVirtualizedStream = virtualizedStream;
		    Seek(pos, virtualizedStream);
		    return Invoke(args, methodGenericArgs, classGenericArgs, callees);
	    }

	    // Token: 0x060001C5 RID: 453 RVA: 0x00009A34 File Offset: 0x00007C34
	    private Type GetTypeById(int id) // \u0002
	    {
		    Type result;
		    lock (AllMetadataById)
		    {
			    var flag = true;
			    object o;
			    if (AllMetadataById.TryGetValue(id, out o))
			    {
				    result = (Type)o;
			    }
			    else
			    {
				    var token = ReadToken(id);
				    if (token.IsVm == 0)
				    {
					    var type = _module.ResolveType(token.MetadataToken);
				        AllMetadataById.Add(id, type);
				        result = type;
				    }
				    else
				    {
					    var vmToken = (VmClassTokenInfo)token.VmToken;
                        if (vmToken.IsOuterClassGeneric)
					    {
						    if (vmToken.OuterClassGenericMethodIdx!= -1)
						    {
                                result = _methodGenericArgs[vmToken.OuterClassGenericMethodIdx];
						    }
						    else
						    {
							    if (vmToken.OuterClassGenericClassIdx== -1)
							    {
								    throw new Exception();
							    }
                                result = _classGenericArgs[vmToken.OuterClassGenericClassIdx];
						    }
                            result = ElementedTypeHelper.PopType(result, ElementedTypeHelper.NestedElementTypes(vmToken.ClassName));
					    }
					    else
					    {
						    var className = vmToken.ClassName.Replace("\u0005 ,", "forms_cil.Trial,"); //TODO: в общем случае это лишнее
                            result = Type.GetType(className);
						    if (result == null)
						    {
							    var num = className.IndexOf(',');
							    var shortClassName = className.Substring(0, num);
							    var asmName = className.Substring(num + 1, className.Length - num - 1).Trim();
							    var assemblies = AppDomain.CurrentDomain.GetAssemblies();
							    foreach (var assembly in assemblies)
							    {
							        string value = null;
							        try
							        {
							            value = assembly.Location;
							        }
							        catch (NotSupportedException)
							        {
							        }
							        if (string.IsNullOrEmpty(value) && assembly.FullName.Equals(asmName, StringComparison.Ordinal))
							        {
                                        result = assembly.GetType(shortClassName);
							            if (result != null)
							            {
							                break;
							            }
							        }
							    }
							    if (result == null && shortClassName.StartsWith(StringDecryptor.GetString(-1550345235) /* <PrivateImplementationDetails>< */, StringComparison.Ordinal) && shortClassName.Contains(StringDecryptor.GetString(-1550345325) /* . */))
							    {
								    try
								    {
								        var types = Assembly.Load(asmName).GetTypes();
								        foreach (var t in types.Where(type3 => type3.FullName == shortClassName))
								        {
								            result = t;
								            break;
								        }
								    }
								        // ReSharper disable once EmptyGeneralCatchClause
								    catch
								    {
								    }
							    }
						    }
						    if (vmToken.IsGeneric)
						    {
							    var array = new Type[vmToken.GenericArguments.Length];
							    for (var j = 0; j < vmToken.GenericArguments.Length; j++)
							    {
								    array[j] = GetTypeById(vmToken.GenericArguments[j].MetadataToken);
							    }
							    var genericTypeDefinition = ElementedTypeHelper.TryGoToElementType(result).GetGenericTypeDefinition();
							    var c = ElementedTypeHelper.NestedElementTypes(result);
                                result = ElementedTypeHelper.PopType(genericTypeDefinition.MakeGenericType(array), c);
							    flag = false;
						    }
						    if (flag)
						    {
							    AllMetadataById.Add(id, result);
						    }
					    }
				    }
			    }
		    }
		    return result;
	    }

	    // Token: 0x060001A6 RID: 422 RVA: 0x00008DAC File Offset: 0x00006FAC
	    public object Invoke(object[] args, Type[] methodGenericArgs, Type[] classGenericArgs, object[] callees) // \u0002
	    {
		    if (args == null)
		    {
                args = EmptyArray<object>.Data;
		    }
		    if (methodGenericArgs == null)
		    {
			    methodGenericArgs = Type.EmptyTypes;
		    }
		    if (classGenericArgs == null)
		    {
			    classGenericArgs = Type.EmptyTypes;
		    }
		    _callees = callees;
		    _methodGenericArgs = methodGenericArgs;
		    _classGenericArgs = classGenericArgs;
		    _variantOutputArgs = ArgsToVariantOutputArgs(args);
            _localVariables = CreateLocalVariables();
            object result;
		    try
		    {
                using (var b = new MyBuffer(_methodBody))
			    {
				    using (_myBufferReader = new MyBufferReader(b))
				    {
					    _retFound = false;
					    _storedPos = null;
					    _evalStack.Clear();
					    InternalInvoke();
				    }
			    }
			    var retType = GetTypeById(_methodHeader.ReturnTypeId);
                if (retType != VoidType && _evalStack.Count > 0)
                {
                    var pop = PopVariant();
                    try
			        {
                        result = VariantFactory.Convert(null, retType).CopyFrom(pop).GetValueAbstract();
                    }
                    catch (Exception)
                    {
                        result = pop.GetValueAbstract(); // example: ckfinite with no numeric
                        //throw;
                    }
			    }
			    else
			    {
				    result = null;
			    }
		    }
		    finally
		    {
			    for (var i = 0; i < _methodHeader.ArgsTypeToOutput.Length; i++)
			    {
				    var argTypeToOutput = _methodHeader.ArgsTypeToOutput[i];
				    if (argTypeToOutput.IsOutput)
				    {
					    var argOutValue = (VariantBaseHolder)_variantOutputArgs[i];
					    var argType = GetTypeById(argTypeToOutput.TypeId);
					    args[i] = VariantFactory.Convert(null, argType.GetElementType()).CopyFrom(argOutValue.GetValue()).GetValueAbstract();
				    }
			    }
			    _callees = null;
			    _variantOutputArgs = null;
			    _localVariables = null;
		    }
		    return result;
	    }

        // Token: 0x06000269 RID: 617 RVA: 0x00010F7C File Offset: 0x0000F17C
        private void Seek(string pos, Stream virtualizedStream) // \u0002
	    {
		    Seek(0L, virtualizedStream, pos);
	    }

        // Token: 0x0600025C RID: 604 RVA: 0x00010B38 File Offset: 0x0000ED38
        private void DoNothing(BinaryReader dummy) // \u0002
        {
        }

	    // Token: 0x06000250 RID: 592 RVA: 0x000106EC File Offset: 0x0000E8EC
	    private static CatchBlock ReadCatchBlock(BinaryReader r) // \u0002
        {
            return new CatchBlock
	        {
	            Kind = r.ReadByte(),
	            ExcTypeId = r.ReadInt32(),
	            Pos = r.ReadUInt32(),
	            PosKind4 = r.ReadUInt32(),
	            Start = r.ReadUInt32(),
	            Len = r.ReadUInt32()
	        };
	    }

	    // Token: 0x060001FC RID: 508 RVA: 0x0000BD50 File Offset: 0x00009F50
	    private static CatchBlock[] ReadCatchBlocks(BinaryReader r) // \u0002
	    {
		    var num = (int)r.ReadInt16();
		    var array = new CatchBlock[num];
		    for (var i = 0; i < num; i++)
		    {
			    array[i] = ReadCatchBlock(r);
		    }
		    return array;
	    }

        // Token: 0x060001BF RID: 447 RVA: 0x00009564 File Offset: 0x00007764
        private static byte[] ReadByteArray(BinaryReader r) // \u0002
	    {
		    var num = r.ReadInt32();
            var array = new byte[num];
		    r.Read(array, 0, num);
		    return array;
	    }

        // Token: 0x060001C1 RID: 449 RVA: 0x000096E4 File Offset: 0x000078E4
        public void Seek(long parsedPos, Stream virtualizedStream, string pos) // \u0002
	    {
		    var input = new VmStreamWrapper(virtualizedStream, VmXorKey());
		    _srcVirtualizedStreamReader = new BinaryReader(input);
		    var baseStream = _srcVirtualizedStreamReader.BaseStream;
		    lock (baseStream)
		    {
			    if (pos != null)
			    {
                    parsedPos = ParsePos(pos);
			    }
			    _srcVirtualizedStreamReader.BaseStream.Seek(parsedPos, SeekOrigin.Begin);
			    DoNothing(_srcVirtualizedStreamReader);
			    _methodHeader = ReadMethodHeader(_srcVirtualizedStreamReader);
			    _catchBlocks = ReadCatchBlocks(_srcVirtualizedStreamReader);
			    SortCatchBlocks();
			    _methodBody = ReadByteArray(_srcVirtualizedStreamReader);
		    }
	    }

	    // Token: 0x06000203 RID: 515 RVA: 0x0000C164 File Offset: 0x0000A364
	    private long ParsePos(string pos) // \u0002
	    {
	        using (var memoryStream = new MemoryStream(VmPosParser.Parse(pos)))
	        {
	            return new BinaryReader(new VmStreamWrapper(memoryStream, PosXorKey())).ReadInt64();
	        }
	    }

	    // Token: 0x060001B5 RID: 437 RVA: 0x000092D8 File Offset: 0x000074D8
	    private int PosXorKey() // \u0002
	    {
		    return -2023764088;
	    }

	    // Token: 0x0600017B RID: 379 RVA: 0x00007DEC File Offset: 0x00005FEC
	    public static int VmXorKey() // \u0003
	    {
		    return 1783652397;
	    }

	    // Token: 0x060001E7 RID: 487 RVA: 0x0000B0F8 File Offset: 0x000092F8
	    private LocalVarType ReadLocalVarType(BinaryReader r) // \u0002
        {
            return new LocalVarType { TypeId = r.ReadInt32() };
	    }

	    // Token: 0x060001AB RID: 427 RVA: 0x00008FF8 File Offset: 0x000071F8
	    private LocalVarType[] ReadLocalVarTypes(BinaryReader r) // \u0002
	    {
		    var array = new LocalVarType[r.ReadInt16()];
		    for (var i = 0; i < array.Length; i++)
		    {
			    array[i] = ReadLocalVarType(r);
		    }
		    return array;
	    }

	    // Token: 0x0600023B RID: 571 RVA: 0x0000F9E0 File Offset: 0x0000DBE0
	    private ArgTypeToOutput ReadArgTypeToOutput(BinaryReader r) // \u0002
	    {
	        var ret = new ArgTypeToOutput
	        {
	            TypeId = r.ReadInt32(),
	            IsOutput = r.ReadBoolean()
	        };
	        return ret;
	    }

	    // Token: 0x06000287 RID: 647 RVA: 0x00012138 File Offset: 0x00010338
	    private ArgTypeToOutput[] ReadArgsTypeToOutput(BinaryReader r) // \u0002
        {
		    var array = new ArgTypeToOutput[r.ReadInt16()];
		    for (var i = 0; i < array.Length; i++)
		    {
			    array[i] = ReadArgTypeToOutput(r);
		    }
		    return array;
	    }

        // Token: 0x06000216 RID: 534 RVA: 0x0000C790 File Offset: 0x0000A990
        private VmMethodHeader ReadMethodHeader(BinaryReader src) // \u0002
        {
            var ret = new VmMethodHeader
            {
                ClassId = src.ReadInt32(),
                ReturnTypeId = src.ReadInt32(),
                LocalVarTypes = ReadLocalVarTypes(src),
                Flags = src.ReadByte(),
                Name = src.ReadString(),
                ArgsTypeToOutput = ReadArgsTypeToOutput(src)
            };
            return ret;
	    }

        // Token: 0x06000266 RID: 614 RVA: 0x00010C54 File Offset: 0x0000EE54
        private void Shr_un_(VariantBase dummy) // \u000F\u2001
	    {
		    PushVariant(Shift(false, false));
	    }

        // Token: 0x06000176 RID: 374 RVA: 0x00007CAC File Offset: 0x00005EAC
        private void Shr_(VariantBase dummy) // \u0005\u2007\u2000
        {
            PushVariant(Shift(false, true));
        }

        private VariantBase Xor(VariantBase org_v1, VariantBase org_v2)
        {
            VariantBase v1, v2;
            var tc = CommonType(org_v1, org_v2, out v1, out v2, true);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    uvret.SetValue(uv1 ^ uv2);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    ivret.SetValue(iv1 ^ iv2);
                    break;
                case VariantBase.Vtc.Tc21Double:
                    {
                        /*double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue(); // естественный алгоритм
                        long lv1 = (dv1 < 0) ? (long)dv1 : (long)(ulong)dv1;
                        long lv2 = (dv2 < 0) ? (long)dv2 : (long)(ulong)dv2;
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        var l64 = (ulong) lv1 ^ (ulong) lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        dvret.SetValue(l64);*/
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        dvret.SetValue((4 == IntPtr.Size) ? Double.NaN : (double)0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    {
                        /*float fv1 = ((FloatVariant) v1).GetValue(), fv2 = ((FloatVariant) v2).GetValue(); // естественный алгоритм
                        long lv1 = (fv1 < 0) ? (long)fv1 : (long)(ulong)fv1;
                        long lv2 = (fv2 < 0) ? (long)fv2 : (long)(ulong)fv2;
                        var fvret = new FloatVariant();
                        ret = fvret;
                        var l64 = (ulong)lv1 ^ (ulong)lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        fvret.SetValue(l64);*/
                        var fvret = new FloatVariant();
                        ret = fvret;
                        fvret.SetValue((4 == IntPtr.Size) ? float.NaN : (float)0.0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    {
                        long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                        var lvret = new LongVariant();
                        ret = lvret;
                        lvret.SetValue(lv1 ^ lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    ulvret.SetValue(ulv1 ^ ulv2);
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(OpCodes.Xor);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x0600026C RID: 620 RVA: 0x00010FC8 File Offset: 0x0000F1C8
        private void Xor_(VariantBase dummy) // \u0008\u2001\u2000
        {
            var v1 = PopVariant();
            var v2 = PopVariant();
            PushVariant(Xor(v2, v1));
        }
        
        // Token: 0x06000189 RID: 393 RVA: 0x00008244 File Offset: 0x00006444
        private void Shl_(VariantBase dummy) // \u0006\u2002\u2001
        {
            PushVariant(Shift(true, true));
        }

        VariantBase.Vtc CommonTypeShift(VariantBase org_val, VariantBase org_shift, out VariantBase val, out VariantBase shift, bool signed)
        {
            val = org_val.Clone();
            shift = org_shift.Clone();
            var tcval = UnderlyingTypeCode(ref val);
            var tcsh = UnderlyingTypeCode(ref shift);
            if (tcval == VariantBase.Vtc.Tc18Object || tcsh == VariantBase.Vtc.Tc18Object)
                return VariantBase.Vtc.Tc18Object;
            shift = new LongVariant();
            long lsh = 0;
            switch (org_shift.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    lsh = VariantBase.SignedLongFromEnum((EnumVariant) org_shift);
                    break;
                case VariantBase.Vtc.Tc13UIntPtr:
                    lsh = (long)((UIntPtrVariant)org_shift).GetValue().ToUInt64();
                    break;
                case VariantBase.Vtc.Tc17IntPtr:
                    lsh = ((IntPtrVariant)org_shift).GetValue().ToInt64();
                    break;
                case VariantBase.Vtc.Tc19Int:
                    lsh = ((IntVariant)org_shift).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    lsh = ((LongVariant)org_shift).GetValue();
                    break;
            }
            shift.SetValueAbstract(lsh);
            VariantBase.Vtc ret = tcval;
            if (!signed)
            {
                val = AsUnsigned(val);
            }
            if (!signed) switch (ret)
                {
                    case VariantBase.Vtc.Tc19Int:
                        return VariantBase.Vtc.Tc9Uint;
                    case VariantBase.Vtc.Tc24Long:
                        return VariantBase.Vtc.Tc7Ulong;
                }
            return ret;
        }

        private VariantBase Shift(bool left, bool signed)
        {
            VariantBase val, shift;

            var org_shift = PopVariant();
            var org_val = PopVariant();
            var tc = CommonTypeShift(org_val, org_shift, out val, out shift, signed);
            var sh = (int)(long)shift.GetValueAbstract();
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)val).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    if (left)
                    {
                        uvret.SetValue(uv1 << sh);
                    }
                    else
                    {
                        uvret.SetValue(uv1 >> sh);
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)val).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    if (left)
                    {
                        ivret.SetValue(iv1 << sh);
                    }
                    else
                    {
                        ivret.SetValue(iv1 >> sh);
                    }
                    break;
                case VariantBase.Vtc.Tc21Double:
                    /*double dv1 = ((DoubleVariant)val).GetValue(), dv2 = ((DoubleVariant)shift).GetValue();
                    var dvret = new DoubleVariant();
                    ret = dvret;
                    var dmul = left ? 2 : 0.5;
                    dvret.SetValue(dv1 * Math.Pow(dmul, dv2));
                    break;*/
                case VariantBase.Vtc.Tc8Float:
                    /*float fv1 = ((FloatVariant)val).GetValue(), fv2 = ((FloatVariant)shift).GetValue();
                    var fvret = new FloatVariant();
                    ret = fvret;
                    var fmul = left ? 2f : 0.5f;
                    fvret.SetValue(fv1 * (float)Math.Pow(fmul, fv2));
                    break;*/
                    throw new InvalidProgramException();
                case VariantBase.Vtc.Tc24Long:
                    long lv1 = ((LongVariant)val).GetValue();
                    var lvret = new LongVariant();
                    ret = lvret;
                    if (left)
                    {
                        lvret.SetValue(lv1 << sh);
                    }
                    else
                    {
                        lvret.SetValue(lv1 >> sh);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)val).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    if (left)
                    {
                        ulvret.SetValue(ulv1 << sh);
                    }
                    else
                    {
                        ulvret.SetValue(ulv1 >> sh);
                    }
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (left ? OpCodes.Shl : OpCodes.Shr) : OpCodes.Shr_Un);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_val.GetValueAbstract(), org_shift.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x06000173 RID: 371 RVA: 0x00007BE0 File Offset: 0x00005DE0
        private void Initblk_(VariantBase dummy) // \u0002\u2002\u2000
        {
            throw new NotSupportedException(StringDecryptor.GetString(-1550345287) /* Initblk not supported. */);
        }

	    // Token: 0x0600020A RID: 522 RVA: 0x0000C508 File Offset: 0x0000A708
	    private void Localloc_(VariantBase dummy) // \u0008\u200A\u2000
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345866) /* Localloc not supported. */);
	    }

	    // Token: 0x06000212 RID: 530 RVA: 0x0000C73C File Offset: 0x0000A93C
	    private void Refanyval_(VariantBase dummy) // \u0005\u200A\u2000
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345900) /* Refanyval is not supported. */);
	    }

	    // Token: 0x06000237 RID: 567 RVA: 0x0000F984 File Offset: 0x0000DB84
	    private void Refanytype_(VariantBase dummy) // \u000F\u2005
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345460) /* Refanytype is not supported. */);
	    }

	    // Token: 0x0600029C RID: 668 RVA: 0x000126CC File Offset: 0x000108CC
	    private void Cpblk_(VariantBase dummy) // \u0002\u2006
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345423) /* Cpblk not supported. */);
	    }

	    // Token: 0x060001CA RID: 458 RVA: 0x00009E68 File Offset: 0x00008068
	    private void Cpobj_(VariantBase dummy) // \u0008\u2009
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345317) /* Cpobj is not supported. */);
	    }

	    // Token: 0x060001D2 RID: 466 RVA: 0x0000A338 File Offset: 0x00008538
	    private void Arglist_(VariantBase dummy) // \u000F\u2002\u2001
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345940) /* Arglist is not supported. */);
	    }

	    // Token: 0x06000187 RID: 391 RVA: 0x00008224 File Offset: 0x00006424
	    private void Mkrefany_(VariantBase dummy) // \u0002\u2007
	    {
		    throw new NotSupportedException(StringDecryptor.GetString(-1550345270) /* Mkrefany is not supported. */);
	    }

        // Token: 0x0600017E RID: 382 RVA: 0x00007ECC File Offset: 0x000060CC
        private static BindingFlags BF(bool isStatic) // \u0002
        {
		    var bindingFlags = BindingFlags.Public | BindingFlags.NonPublic;
		    if (isStatic)
		    {
			    bindingFlags |= BindingFlags.Static;
		    }
		    else
		    {
			    bindingFlags |= BindingFlags.Instance;
		    }
		    return bindingFlags;
	    }

        // Token: 0x06000192 RID: 402 RVA: 0x000084F4 File Offset: 0x000066F4
        private void Ldc_i4_0_(VariantBase dummy) // \u0005\u2001
        {
            var iv = new IntVariant();
            iv.SetValue(0);
            PushVariant(iv);
        }

        // Token: 0x0600028F RID: 655 RVA: 0x0001235C File Offset: 0x0001055C
        private void Ldc_i4_1_(VariantBase dummy) // \u0002\u2005\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(1);
            PushVariant(iv);
        }

        // Token: 0x0600022F RID: 559 RVA: 0x0000F8C8 File Offset: 0x0000DAC8
        private void Ldc_i4_2_(VariantBase dummy) // \u0005
        {
            var iv = new IntVariant();
            iv.SetValue(2);
            PushVariant(iv);
        }

        // Token: 0x06000174 RID: 372 RVA: 0x00007BF4 File Offset: 0x00005DF4
        private void Ldc_i4_3_(VariantBase dummy) // \u0008\u2006\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(3);
            PushVariant(iv);
        }

        // Token: 0x06000262 RID: 610 RVA: 0x00010BEC File Offset: 0x0000EDEC
        private void Ldc_i4_4_(VariantBase dummy) // \u000F\u2009\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(4);
            PushVariant(iv);
        }

        // Token: 0x060001B3 RID: 435 RVA: 0x000092B4 File Offset: 0x000074B4
        private void Ldc_i4_5_(VariantBase dummy) // \u0008\u2004\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(5);
            PushVariant(iv);
        }

        // Token: 0x06000231 RID: 561 RVA: 0x0000F900 File Offset: 0x0000DB00
        private void Ldc_i4_6_(VariantBase dummy) // \u0006\u2009\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(6);
            PushVariant(iv);
        }

        // Token: 0x0600026A RID: 618 RVA: 0x00010F88 File Offset: 0x0000F188
        private void Ldc_i4_7_(VariantBase dummy) // \u0003\u2003\u2000
        {
            var iv = new IntVariant();
            iv.SetValue(7);
            PushVariant(iv);
        }

        // Token: 0x060001A9 RID: 425 RVA: 0x00008FA8 File Offset: 0x000071A8
        private void Ldc_i4_8_(VariantBase dummy) // \u000E\u2001
        {
            var iv = new IntVariant();
            iv.SetValue(8);
            PushVariant(iv);
        }

        // Token: 0x0600016F RID: 367 RVA: 0x00007A58 File Offset: 0x00005C58
        private void Unbox_(VariantBase vTypeId) // \u0005\u2003\u2000
        {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var val = VariantFactory.Convert(PopVariant().GetValueAbstract(), type);
		    PushVariant(val);
	    }

        // Token: 0x06000207 RID: 519 RVA: 0x0000C228 File Offset: 0x0000A428
        private VmTokenInfo ReadVmToken(BinaryReader reader) // \u0002
        {
            switch (reader.ReadByte())
            {
                case (byte)VmTokenInfo.Kind.Class0:
                    {
                        var ret = new VmClassTokenInfo
                        {
                            OuterClassGenericMethodIdx = reader.ReadInt32(),
                            OuterClassGenericClassIdx = reader.ReadInt32(),
                            IsOuterClassGeneric = reader.ReadBoolean(),
                            ClassName = reader.ReadString(),
                            IsGeneric = reader.ReadBoolean(),
                            GenericArguments = new UniversalTokenInfo[(int) reader.ReadInt16()]
                        };
                        for (var i = 0; i < ret.GenericArguments.Length; i++)
                        {
                            ret.GenericArguments[i] = new UniversalTokenInfo
                            {
                                IsVm = 1,
                                MetadataToken = reader.ReadInt32()
                            };
                        }
                        return ret;
                    }
                case (byte)VmTokenInfo.Kind.Field1:
                    return new VmFieldTokenInfo
                    {
                        Class = new UniversalTokenInfo
                        {
                            IsVm = 1,
                            MetadataToken = reader.ReadInt32()
                        },
                        Name = reader.ReadString(),
                        IsStatic = reader.ReadBoolean()
                    };
                case (byte)VmTokenInfo.Kind.Method2:
                    {
                        var ret = new VmMethodTokenInfo
                        {
                            Class = new UniversalTokenInfo
                            {
                                IsVm = 1,
                                MetadataToken = reader.ReadInt32()
                            },
                            Flags = reader.ReadByte(),
                            Name = reader.ReadString(),
                            ReturnType = new UniversalTokenInfo
                            {
                                IsVm = 1,
                                MetadataToken = reader.ReadInt32()
                            },
                            Parameters = new UniversalTokenInfo[(int)reader.ReadInt16()]
                        };
                        for (var j = 0; j < ret.Parameters.Length; j++)
                        {
                            ret.Parameters[j] = new UniversalTokenInfo
                            {
                                IsVm = 1,
                                MetadataToken = reader.ReadInt32()
                            };
                        }
                        var gaCnt = (int)reader.ReadInt16();
                        ret.GenericArguments = new UniversalTokenInfo[gaCnt];
                        for (var k = 0; k < gaCnt; k++)
                        {
                            ret.GenericArguments[k] = new UniversalTokenInfo
                            {
                                IsVm = 1,
                                MetadataToken = reader.ReadInt32()
                            };
                        }
                        return ret;
                    }
                case (byte)VmTokenInfo.Kind.String3:
                    return new VmStringTokenInfo { Value = reader.ReadString() };
                case (byte)VmTokenInfo.Kind.MethodRef4:
                    return new VmMethodRefTokenInfo
                    {
                        Flags = reader.ReadInt32(),
                        Pos = reader.ReadInt32()
                    };
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        // Token: 0x06000175 RID: 373 RVA: 0x00007C08 File Offset: 0x00005E08
        private UniversalTokenInfo ReadToken(int pos) // u0002
        {
            if (_srcVirtualizedStreamReader == null)
            {
                throw new InvalidOperationException();
            }
            var baseStream = _srcVirtualizedStreamReader.BaseStream;
            UniversalTokenInfo result;
            lock (baseStream)
            {
                _srcVirtualizedStreamReader.BaseStream.Seek(pos, SeekOrigin.Begin);
                result = new UniversalTokenInfo {IsVm = _srcVirtualizedStreamReader.ReadByte()};
                if (result.IsVm == 0)
                {
                    result.MetadataToken = _srcVirtualizedStreamReader.ReadInt32();
                }
                else
                {
                    result.VmToken = ReadVmToken(_srcVirtualizedStreamReader);
                }
            }
            return result;
        }

        // Token: 0x06000248 RID: 584 RVA: 0x000102AC File Offset: 0x0000E4AC
        private string Ldstr(int strToken) // \u0002
        {
            string result;
            lock (AllMetadataById)
            {
                object stored;
                if (AllMetadataById.TryGetValue(strToken, out stored))
                {
                    result = (string)stored;
                }
                else
                {
                    var tokenInfo = ReadToken(strToken);
                    if (tokenInfo.IsVm == 0)
                    {
                        result = _module.ResolveString(tokenInfo.MetadataToken);
                    }
                    else
                    {
                        var text = ((VmStringTokenInfo)tokenInfo.VmToken).Value;
                        AllMetadataById.Add(strToken, text);
                        result = text;
                    }
                }
            }
            return result;
        }

        // Token: 0x0600019C RID: 412 RVA: 0x00008B34 File Offset: 0x00006D34
        private void Ldstr_(VariantBase strToken) // \u000E\u2000\u2001
        {
            var tok = ((IntVariant)strToken).GetValue();
            var text = Ldstr(tok);
            var val = new StringVariant();
            val.SetValue(text);
            PushVariant(val);
        }

        // Token: 0x06000170 RID: 368 RVA: 0x00007A94 File Offset: 0x00005C94
        private VariantBase ReadOperand(MyBufferReader r, VmOperandType operandType) // \u0002
	    {
		    switch (operandType)
		    {
		    case VmOperandType.Ot0UInt:
		    {
			    var ret = new UintVariant();
			    ret.SetValue(r.ReadUint());
			    return ret;
		    }
		    case VmOperandType.Ot1UShort:
		    case VmOperandType.Ot3UShort:
		    {
			    var ret = new UshortVariant();
			    ret.SetValue(r.ReadUshort());
			    return ret;
		    }
		    case VmOperandType.Ot2Byte:
		    case VmOperandType.Ot8Byte:
		    {
			    var ret = new ByteVariant();
			    ret.SetValue(r.ReadByte());
			    return ret;
		    }
		    case VmOperandType.Ot4Double:
		    {
			    var ret = new DoubleVariant();
			    ret.SetValue(r.ReadDouble());
			    return ret;
		    }
		    case VmOperandType.Ot5Int:
		    case VmOperandType.Ot12Int:
		    {
			    var ret = new IntVariant();
			    ret.SetValue(r.ReadInt32());
			    return ret;
		    }
		    case VmOperandType.Ot6SByte:
		    {
			    var ret = new SbyteVariant();
			    ret.SetValue(r.ReadSbyte());
			    return ret;
		    }
		    case VmOperandType.Ot7Long:
		    {
			    var ret = new LongVariant();
			    ret.SetValue(r.ReadLong());
			    return ret;
		    }
		    case VmOperandType.Ot9IntArr:
		    {
			    var num = r.ReadInt32();
			    var array = new IntVariant[num];
			    for (var i = 0; i < num; i++)
			    {
				    var item = new IntVariant();
				    item.SetValue(r.ReadInt32());
                    array[i] = item;
			    }
			    var ret = new ArrayVariant();
			    ret.SetValue(array);
			    return ret;
		    }
		    case VmOperandType.Ot10Float:
		    {
			    var ret = new FloatVariant();
			    ret.SetValue(r.ReadFloat());
			    return ret;
		    }
		    case VmOperandType.Ot11Nope:
			    return null;
		    default:
			    throw new Exception(StringDecryptor.GetString(-1550347123) /* Unknown operand type. */);
		    }
	    }

        // Token: 0x06000279 RID: 633 RVA: 0x0001184C File Offset: 0x0000FA4C
        enum ComparisonKind { EQ, NEQ, GT, LE, LT, GE }
        private static bool UniCompare(VariantBase v1, VariantBase v2, ComparisonKind ck, bool unsignedNanBranch) // \u0008 - bug fixed (метод переписан)
        {
            // from stack: enum double single long int
            var t1 = v1.GetTypeCode();
            if (t1 == VariantBase.Vtc.Tc5Enum)
            {
                var vv1 = VariantBase.SignedVariantFromEnum((EnumVariant)v1);
                return UniCompare(vv1, v2, ck, unsignedNanBranch);
            }
            var t2 = v2.GetTypeCode();
            if (t2 == VariantBase.Vtc.Tc5Enum)
            {
                var vv2 = VariantBase.SignedVariantFromEnum((EnumVariant)v2);
                return UniCompare(v1, vv2, ck, unsignedNanBranch);
            }
            if (t1 == VariantBase.Vtc.Tc18Object || t2 == VariantBase.Vtc.Tc18Object)
            {
                if (ck == ComparisonKind.EQ) return v1.GetValueAbstract().Equals(v2.GetValueAbstract());
                if (ck == ComparisonKind.NEQ) return !v1.GetValueAbstract().Equals(v2.GetValueAbstract());
                return false;
            }
            if (t1 == VariantBase.Vtc.Tc21Double || t2 == VariantBase.Vtc.Tc21Double)
            {
                var d1 = (t1 == VariantBase.Vtc.Tc21Double) ? ((DoubleVariant)v1).GetValue() : Convert.ToDouble(v1.GetValueAbstract());
                var d2 = (t2 == VariantBase.Vtc.Tc21Double) ? ((DoubleVariant)v2).GetValue() : Convert.ToDouble(v2.GetValueAbstract());
                if (unsignedNanBranch) unsignedNanBranch = (double.IsNaN(d1) || double.IsNaN(d2));
                switch (ck)
                {
                    case ComparisonKind.EQ:
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return (d1 == d2) || unsignedNanBranch;
                    case ComparisonKind.GT:
                        return (d1 > d2) || unsignedNanBranch;
                    case ComparisonKind.NEQ:
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return (d1 != d2) || unsignedNanBranch;
                    case ComparisonKind.LE:
                        return (d1 <= d2) || unsignedNanBranch;
                    case ComparisonKind.LT:
                        return (d1 < d2) || unsignedNanBranch;
                    case ComparisonKind.GE:
                        return (d1 >= d2) || unsignedNanBranch;
                }
            }
            if (t1 == VariantBase.Vtc.Tc8Float || t2 == VariantBase.Vtc.Tc8Float)
            {
                var d1 = (t1 == VariantBase.Vtc.Tc8Float) ? ((FloatVariant)v1).GetValue() : Convert.ToSingle(v1.GetValueAbstract());
                var d2 = (t2 == VariantBase.Vtc.Tc8Float) ? ((FloatVariant)v2).GetValue() : Convert.ToSingle(v2.GetValueAbstract());
                if (unsignedNanBranch) unsignedNanBranch = (float.IsNaN(d1) || float.IsNaN(d2));
                switch (ck)
                {
                    case ComparisonKind.EQ:
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return (d1 == d2) || unsignedNanBranch;
                    case ComparisonKind.GT:
                        return (d1 > d2) || unsignedNanBranch;
                    case ComparisonKind.NEQ:
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return (d1 != d2) || unsignedNanBranch;
                    case ComparisonKind.LE:
                        return (d1 <= d2) || unsignedNanBranch;
                    case ComparisonKind.LT:
                        return (d1 < d2) || unsignedNanBranch;
                    case ComparisonKind.GE:
                        return (d1 >= d2) || unsignedNanBranch;
                }
            }
            if (t1 == VariantBase.Vtc.Tc24Long || t2 == VariantBase.Vtc.Tc24Long)
            {
                var d1 = (t1 == VariantBase.Vtc.Tc24Long) ? ((LongVariant)v1).GetValue() : (unsignedNanBranch ? Convert.ToInt64((uint)(int)v1.GetValueAbstract()) : Convert.ToInt64(v1.GetValueAbstract()));
                var d2 = (t2 == VariantBase.Vtc.Tc24Long) ? ((LongVariant)v2).GetValue() : (unsignedNanBranch ? Convert.ToInt64((uint)(int)v2.GetValueAbstract()) : Convert.ToInt64(v2.GetValueAbstract()));
                switch (ck)
                {
                    case ComparisonKind.EQ:
                        return d1 == d2;
                    case ComparisonKind.GT:
                        if(unsignedNanBranch) return (ulong)d1 > (ulong)d2;
                        return d1 > d2;
                    case ComparisonKind.NEQ:
                        if (unsignedNanBranch) return (ulong)d1 != (ulong)d2;
                        return d1 != d2;
                    case ComparisonKind.LE:
                        if (unsignedNanBranch) return (ulong)d1 <= (ulong)d2;
                        return d1 <= d2;
                    case ComparisonKind.LT:
                        if (unsignedNanBranch) return (ulong)d1 < (ulong)d2;
                        return d1 < d2;
                    case ComparisonKind.GE:
                        if (unsignedNanBranch) return (ulong)d1 >= (ulong)d2;
                        return d1 >= d2;
                }
            }
            if (t1 == VariantBase.Vtc.Tc19Int || t2 == VariantBase.Vtc.Tc19Int)
            {
                switch (ck)
                {
                    case ComparisonKind.EQ:
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return ((IntVariant)v1).GetValue() == ((IntVariant)v2).GetValue();
                    case ComparisonKind.GT:
                        if (unsignedNanBranch) return (uint)((IntVariant)v1).GetValue() > (uint)((IntVariant)v2).GetValue();
                        return ((IntVariant)v1).GetValue() > ((IntVariant)v2).GetValue();
                    case ComparisonKind.NEQ:
                        if (unsignedNanBranch) return (uint)((IntVariant)v1).GetValue() != (uint)((IntVariant)v2).GetValue();
                        return ((IntVariant)v1).GetValue() != ((IntVariant)v2).GetValue();
                    case ComparisonKind.LE:
                        if (unsignedNanBranch) return (uint)((IntVariant)v1).GetValue() <= (uint)((IntVariant)v2).GetValue();
                        return ((IntVariant)v1).GetValue() <= ((IntVariant)v2).GetValue();
                    case ComparisonKind.LT:
                        if (unsignedNanBranch) return (uint)((IntVariant)v1).GetValue() < (uint)((IntVariant)v2).GetValue();
                        return ((IntVariant)v1).GetValue() < ((IntVariant)v2).GetValue();
                    //case ComparisonKind.GE:
                    default:
                        if (unsignedNanBranch) return (uint)((IntVariant)v1).GetValue() >= (uint)((IntVariant)v2).GetValue();
                        return ((IntVariant)v1).GetValue() >= ((IntVariant)v2).GetValue();
                }
            }
            return false;
        }

        // Token: 0x0600017F RID: 383 RVA: 0x00007EEC File Offset: 0x000060EC
        private void Ldelema_(VariantBase vTypeId) // \u000F\u2002
        {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
            var idx = PopLong();
            var array = (Array)PopVariant().GetValueAbstract();
            var val = new SdArrayValueVariant();
            val.SetArray(array);
            val.SetHeldType(type);
            val.SetIndex(idx);
            PushVariant(val);
        }

        // Token: 0x06000252 RID: 594 RVA: 0x00010780 File Offset: 0x0000E980
        private void Ldelem_ref_(VariantBase dummy) // \u000F\u2003\u2000
        {
            Ldelem(SimpleTypeHelper.ObjectType);
        }

        // Token: 0x06000178 RID: 376 RVA: 0x00007D38 File Offset: 0x00005F38
        private static void SerializeCrossDomain(Exception ex) // \u0002
        {
            if (ex == null)
            {
                return;
            }
            try
            {
                var type = ex.GetType();
                if (type.IsSerializable)
                {
                    var context = new StreamingContext(StreamingContextStates.CrossAppDomain);
                    var om = new ObjectManager(null, context);
                    var info = new SerializationInfo(type, new FormatterConverter());
                    ex.GetObjectData(info, context);
                    om.RegisterObject(ex, 1L, info);
                    om.DoFixups();
                }
            }
            catch
            {
            }
        }

        // Token: 0x060001DF RID: 479 RVA: 0x0000ABCC File Offset: 0x00008DCC
        private static void Throw(object ex) // \u0003
        {
            throw (Exception)ex;
        }

        // Token: 0x0600024D RID: 589 RVA: 0x000105A0 File Offset: 0x0000E7A0
        private static void ThrowStoreCrossDomain(object ex) // \u0002
        {
            SerializeCrossDomain(ex as Exception);
            Throw(ex);
        }

        // Token: 0x06000293 RID: 659 RVA: 0x00012398 File Offset: 0x00010598
        private static VariantBase SubLong(VariantBase v1, VariantBase v2, bool bChecked, bool bUnsigned) // \u0005
        {
            var lvret = new LongVariant();
            if (!bUnsigned)
            {
                var l1 = ((LongVariant)v1).GetValue();
                var l2 = ((LongVariant)v2).GetValue();
                long lret;
                if (bChecked)
                {
                    lret = checked(l1 - l2);
                }
                else
                {
                    lret = l1 - l2;
                }
                lvret.SetValue(lret);
                return lvret;
            }
            var u1 = (ulong)((LongVariant)v1).GetValue();
            var u2 = (ulong)((LongVariant)v2).GetValue();
            ulong uret;
            if (bChecked)
            {
                uret = checked(u1 - u2);
            }
            else
            {
                uret = u1 - u2;
            }
            lvret.SetValue((long)uret);
            return lvret;
        }

        private VariantBase Sub(bool ovf, bool signed)
        {
            VariantBase v1, v2;
            var org_v2 = PopVariant();
            var org_v1 = PopVariant();
            var tc = CommonType(org_v1, org_v2, out v1, out v2, signed);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    if (ovf)
                    {
                        uint uiv;
                        checked
                        {
                            uiv = uv1 - uv2;
                        }
                        uvret.SetValue(uiv);
                    }
                    else
                    {
                        uvret.SetValue(uv1 - uv2);
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    if (ovf)
                    {
                        checked
                        {
                            ivret.SetValue(iv1 - iv2);
                        }
                    }
                    else
                    {
                        ivret.SetValue(iv1 - iv2);
                    }
                    break;
                case VariantBase.Vtc.Tc21Double:
                    double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue();
                    var dvret = new DoubleVariant();
                    ret = dvret;
                    dvret.SetValue(dv1 - dv2);
                    break;
                case VariantBase.Vtc.Tc8Float:
                    float fv1 = ((FloatVariant)v1).GetValue(), fv2 = ((FloatVariant)v2).GetValue();
                    var fvret = new FloatVariant();
                    ret = fvret;
                    fvret.SetValue(fv1 - fv2);
                    break;
                case VariantBase.Vtc.Tc24Long:
                    long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                    var lvret = new LongVariant();
                    ret = lvret;
                    if (ovf)
                    {
                        checked
                        {
                            lvret.SetValue(lv1 - lv2);
                        }
                    }
                    else
                    {
                        lvret.SetValue(lv1 - lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    if (ovf)
                    {
                        ulong ulv;
                        checked
                        {
                            ulv = ulv1 - ulv2;
                        }
                        ulvret.SetValue(ulv);
                    }
                    else
                    {
                        ulvret.SetValue(ulv1 - ulv2);
                    }
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Sub_Ovf : OpCodes.Sub) : OpCodes.Sub_Ovf_Un);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x0600026B RID: 619 RVA: 0x00010F9C File Offset: 0x0000F19C
        private void Sub_ovf_un_(VariantBase dummy) // \u0005\u2006\u2000
        {
            PushVariant(Sub(true, false));
        }

        // Token: 0x060001A8 RID: 424 RVA: 0x00008F7C File Offset: 0x0000717C
        private void Sub_ovf_(VariantBase dummy) // \u0005\u2008
        {
            PushVariant(Sub(true, true));
        }

        // Token: 0x0600021C RID: 540 RVA: 0x0000C84C File Offset: 0x0000AA4C
        private void Sub_(VariantBase dummy) // \u0006\u200A
        {
            PushVariant(Sub(false, true));
        }

        // Token: 0x06000271 RID: 625 RVA: 0x00011144 File Offset: 0x0000F344
        private static VariantBase MulLong(VariantBase v1, VariantBase v2, bool bChecked, bool bUnsigned) // \u0003
        {
            var lvret = new LongVariant();
            if (!bUnsigned)
            {
                var l1 = ((LongVariant)v1).GetValue();
                var l2 = ((LongVariant)v2).GetValue();
                long lret;
                if (bChecked)
                {
                    lret = checked(l1 * l2);
                }
                else
                {
                    lret = l1 * l2;
                }
                lvret.SetValue(lret);
                return lvret;
            }
            var u1 = (ulong)((LongVariant)v1).GetValue();
            var u2 = (ulong)((LongVariant)v2).GetValue();
            ulong uret;
            if (bChecked)
            {
                uret = checked(u1 * u2);
            }
            else
            {
                uret = u1 * u2;
            }
            lvret.SetValue((long)uret);
            return lvret;
        }

        private VariantBase Mul(bool ovf, bool signed)
        {
            VariantBase v1, v2;
            var org_v2 = PopVariant();
            var org_v1 = PopVariant();
            var tc = CommonType(org_v1, org_v2, out v1, out v2, signed);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    if (ovf)
                    {
                        uint uiv;
                        checked
                        {
                            uiv = uv1 * uv2;
                        }
                        uvret.SetValue(uiv);
                    }
                    else
                    {
                        uvret.SetValue(uv1 * uv2);
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    if (ovf)
                    {
                        checked
                        {
                            ivret.SetValue(iv1 * iv2);
                        }
                    }
                    else
                    {
                        ivret.SetValue(iv1 * iv2);
                    }
                    break;
                case VariantBase.Vtc.Tc21Double:
                    double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue();
                    var dvret = new DoubleVariant();
                    ret = dvret;
                    dvret.SetValue(dv1 * dv2);
                    break;
                case VariantBase.Vtc.Tc8Float:
                    float fv1 = ((FloatVariant)v1).GetValue(), fv2 = ((FloatVariant)v2).GetValue();
                    var fvret = new FloatVariant();
                    ret = fvret;
                    fvret.SetValue(fv1 * fv2);
                    break;
                case VariantBase.Vtc.Tc24Long:
                    long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                    var lvret = new LongVariant();
                    ret = lvret;
                    if (ovf)
                    {
                        checked
                        {
                            lvret.SetValue(lv1 * lv2);
                        }
                    }
                    else
                    {
                        lvret.SetValue(lv1 * lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    if (ovf)
                    {
                        ulong ulv;
                        checked
                        {
                            ulv = ulv1 * ulv2;
                        }
                        ulvret.SetValue(ulv);
                    }
                    else
                    {
                        ulvret.SetValue(ulv1 * ulv2);
                    }
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Mul_Ovf : OpCodes.Mul) : OpCodes.Mul_Ovf_Un);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x06000181 RID: 385 RVA: 0x00007F88 File Offset: 0x00006188
        private void Mul_ovf_un_(VariantBase dummy) // \u0005\u2007
        {
            PushVariant(Mul(true, false));
        }

        // Token: 0x060001E1 RID: 481 RVA: 0x0000ABDC File Offset: 0x00008DDC
        private void Mul_(VariantBase dummy) // \u0006\u2001\u2001
        {
            PushVariant(Mul(false, true));
        }

        // Token: 0x060001A0 RID: 416 RVA: 0x00008C98 File Offset: 0x00006E98
        private void Mul_ovf_(VariantBase dummy) // \u0008\u2000\u2000
        {
            PushVariant(Mul(true, true));
        }

        VariantBase.Vtc CommonType(VariantBase org_v1, VariantBase org_v2, out VariantBase v1, out VariantBase v2, bool signed)
        {
            v1 = org_v1.Clone();
            v2 = org_v2.Clone();
            var tc1 = UnderlyingTypeCode(ref v1);
            var tc2 = UnderlyingTypeCode(ref v2);
            if (tc1 == VariantBase.Vtc.Tc18Object || tc2 == VariantBase.Vtc.Tc18Object)
                return VariantBase.Vtc.Tc18Object;
            VariantBase.Vtc ret = tc1;
            if (!signed)
            {
                v1 = AsUnsigned(v1);
                v2 = AsUnsigned(v2);
            }
            if(tc1 != tc2) switch (tc1)
            {
                case VariantBase.Vtc.Tc19Int:
                    switch (tc2)
                    {
                        case VariantBase.Vtc.Tc24Long:
                            {
                                ret = tc2;
                                var new_v1 = signed ? (VariantBase)new LongVariant() : new UlongVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                        case VariantBase.Vtc.Tc21Double:
                            {
                                ret = tc2;
                                var new_v1 = new DoubleVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                        case VariantBase.Vtc.Tc8Float:
                            {
                                ret = tc2;
                                var new_v1 = new FloatVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    switch (tc2)
                    {
                        case VariantBase.Vtc.Tc19Int:
                            {
                                var new_v2 = signed ? (VariantBase)new LongVariant() : new UlongVariant();
                                new_v2.CopyFrom(v2);
                                v2 = new_v2;
                            }
                            break;
                        case VariantBase.Vtc.Tc21Double:
                            {
                                ret = tc2;
                                var new_v1 = new DoubleVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                        case VariantBase.Vtc.Tc8Float:
                            {
                                ret = tc2;
                                var new_v1 = new FloatVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                    }
                    break;
                case VariantBase.Vtc.Tc21Double:
                    switch (tc2)
                    {
                        case VariantBase.Vtc.Tc19Int:
                        case VariantBase.Vtc.Tc24Long:
                        case VariantBase.Vtc.Tc8Float:
                            {
                                var new_v2 = new DoubleVariant();
                                new_v2.CopyFrom(v2);
                                v2 = new_v2;
                            }
                            break;
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    switch (tc2)
                    {
                        case VariantBase.Vtc.Tc19Int:
                        case VariantBase.Vtc.Tc24Long:
                            {
                                var new_v2 = new FloatVariant();
                                new_v2.CopyFrom(v2);
                                v2 = new_v2;
                            }
                            break;
                        case VariantBase.Vtc.Tc21Double:
                            {
                                ret = tc2;
                                var new_v1 = new DoubleVariant();
                                new_v1.CopyFrom(v1);
                                v1 = new_v1;
                            }
                            break;
                    }
                    break;
            }
            if(!signed) switch (ret)
            {
                case VariantBase.Vtc.Tc19Int:
                    return VariantBase.Vtc.Tc9Uint;
                case VariantBase.Vtc.Tc24Long:
                    return VariantBase.Vtc.Tc7Ulong;
            }
            return ret;
        }

        private VariantBase.Vtc UnderlyingTypeCode(ref VariantBase v)
        {
            var ret = v.GetTypeCode();
            if (ret == VariantBase.Vtc.Tc5Enum)
            {
                v = VariantBase.SignedVariantFromEnum((EnumVariant) v);
                ret = Marshal.SizeOf(v.GetValueAbstract()) == 4 ? VariantBase.Vtc.Tc19Int : VariantBase.Vtc.Tc24Long;
            }
            return ret;
        }

        private VariantBase AsUnsigned(VariantBase v)
        {
            var tc = v.GetTypeCode();
            var ret = v;
            switch (tc)
            {
                case VariantBase.Vtc.Tc19Int:
                    ret = new UintVariant();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    ret = new UlongVariant();
                    break;
            }
            ret.CopyFrom(v);
            return ret;
        }

        private VariantBase Add(bool ovf, bool signed)
        {
            VariantBase v1, v2;
            var org_v2 = PopVariant();
            var org_v1 = PopVariant();
            var tc = CommonType(org_v1, org_v2, out v1, out v2, signed);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    if (ovf)
                    {
                        uint uiv;
                        checked
                        {
                            uiv = uv1 + uv2;
                        }
                        uvret.SetValue(uiv);
                    }
                    else
                    {
                        uvret.SetValue(uv1 + uv2);
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    if (ovf)
                    {
                        checked
                        {
                            ivret.SetValue(iv1 + iv2);
                        }
                    }
                    else
                    {
                        ivret.SetValue(iv1 + iv2);
                    }
                    break;
                case VariantBase.Vtc.Tc21Double:
                    double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue();
                    var dvret = new DoubleVariant();
                    ret = dvret;
                    dvret.SetValue(dv1 + dv2);
                    break;
                case VariantBase.Vtc.Tc8Float:
                    float fv1 = ((FloatVariant)v1).GetValue(), fv2 = ((FloatVariant)v2).GetValue();
                    var fvret = new FloatVariant();
                    ret = fvret;
                    fvret.SetValue(fv1 + fv2);
                    break;
                case VariantBase.Vtc.Tc24Long:
                    long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                    var lvret = new LongVariant();
                    ret = lvret;
                    if (ovf)
                    {
                        checked
                        {
                            lvret.SetValue(lv1 + lv2);
                        }
                    }
                    else
                    {
                        lvret.SetValue(lv1 + lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    if (ovf)
                    {
                        ulong ulv;
                        checked
                        {
                            ulv = ulv1 + ulv2;
                        }
                        ulvret.SetValue(ulv);
                    }
                    else
                    {
                        ulvret.SetValue(ulv1 + ulv2);
                    }
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Add_Ovf : OpCodes.Add) : OpCodes.Add_Ovf_Un);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x0600022D RID: 557 RVA: 0x0000F880 File Offset: 0x0000DA80
        private void Add_ovf_(VariantBase dummy) // \u0008\u2001
        {
            PushVariant(Add(true, true));
        }

        // Token: 0x06000238 RID: 568 RVA: 0x0000F998 File Offset: 0x0000DB98
        private void Add_(VariantBase dummy) // \u0002\u2004
        {
            PushVariant(Add(false, true));
        }

        // Token: 0x0600029B RID: 667 RVA: 0x000126A0 File Offset: 0x000108A0
        private void Add_ovf_un_(VariantBase dummy) // \u0002\u2004\u2000
        {
            PushVariant(Add(true, false));
        }

        private VariantBase Or(VariantBase org_v1, VariantBase org_v2)
        {
            VariantBase v1, v2;
            var tc = CommonType(org_v1, org_v2, out v1, out v2, true);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    uvret.SetValue(uv1 | uv2);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    ivret.SetValue(iv1 | iv2);
                    break;
                case VariantBase.Vtc.Tc21Double:
                    {
                        /*double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue(); // естественный алгоритм
                        long lv1 = (dv1 < 0) ? (long)dv1 : (long)(ulong)dv1;
                        long lv2 = (dv2 < 0) ? (long)dv2 : (long)(ulong)dv2;
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        var l64 = (ulong) lv1 | (ulong) lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        dvret.SetValue(l64);*/
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        dvret.SetValue((4 == IntPtr.Size) ? Double.NaN : (double)0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    {
                        /*float fv1 = ((FloatVariant) v1).GetValue(), fv2 = ((FloatVariant) v2).GetValue(); // естественный алгоритм
                        long lv1 = (fv1 < 0) ? (long)fv1 : (long)(ulong)fv1;
                        long lv2 = (fv2 < 0) ? (long)fv2 : (long)(ulong)fv2;
                        var fvret = new FloatVariant();
                        ret = fvret;
                        var l64 = (ulong)lv1 | (ulong)lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        fvret.SetValue(l64);*/
                        var fvret = new FloatVariant();
                        ret = fvret;
                        fvret.SetValue((4 == IntPtr.Size) ? float.NaN : (float)0.0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    {
                        long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                        var lvret = new LongVariant();
                        ret = lvret;
                        lvret.SetValue(lv1 | lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    ulvret.SetValue(ulv1 | ulv2);
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(OpCodes.Or);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x0600020F RID: 527 RVA: 0x0000C598 File Offset: 0x0000A798
        private void Or_(VariantBase dummy) // \u0006\u2002\u2000
        {
            var v2 = PopVariant();
            var v1 = PopVariant();
            PushVariant(Or(v1, v2));
        }

        // Token: 0x060001D8 RID: 472 RVA: 0x0000A718 File Offset: 0x00008918
        private void Throw_(VariantBase dummy) // \u0006\u2005\u2000
        {
            ThrowStoreCrossDomain(PopVariant().GetValueAbstract());
        }

        private VariantBase Rem(bool signed)
        {
            VariantBase v1, v2;
            var org_v2 = PopVariant();
            var org_v1 = PopVariant();
            VariantBase ret;
            var tc = CommonType(org_v1, org_v2, out v1, out v2, signed);
            if (IsFloating(org_v1) && org_v1.GetType() == org_v2.GetType() && !signed)
            {
                if (IntPtr.Size == 8) throw new InvalidProgramException();
                if (tc == VariantBase.Vtc.Tc21Double)
                {
                    ret = new DoubleVariant();
                    ret.SetValueAbstract(double.NaN);
                }
                else /*if (tc == VariantBase.Vtc.Tc8Float)*/
                {
                    ret = new FloatVariant();
                    ret.SetValueAbstract(float.NaN);
                }
            }
            else switch (tc)
                {
                    case VariantBase.Vtc.Tc9Uint:
                        uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                        var uvret = new UintVariant();
                        ret = uvret;
                        uvret.SetValue(uv1 % uv2);
                        break;
                    case VariantBase.Vtc.Tc19Int:
                        int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                        var ivret = new IntVariant();
                        ret = ivret;
                        ivret.SetValue(iv1 % iv2);
                        break;
                    case VariantBase.Vtc.Tc21Double:
                        double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue();
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        if (Math.Abs(dv2) < double.Epsilon && org_v1.GetType() != org_v2.GetType()) throw new DivideByZeroException();
                        dvret.SetValue(dv1 % dv2);
                        break;
                    case VariantBase.Vtc.Tc8Float:
                        float fv1 = ((FloatVariant)v1).GetValue(), fv2 = ((FloatVariant)v2).GetValue();
                        var fvret = new FloatVariant();
                        ret = fvret;
                        if (Math.Abs(fv2) < float.Epsilon && org_v1.GetType() != org_v2.GetType()) throw new DivideByZeroException();
                        fvret.SetValue(fv1 % fv2);
                        break;
                    case VariantBase.Vtc.Tc24Long:
                        long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                        var lvret = new LongVariant();
                        ret = lvret;
                        lvret.SetValue(lv1 % lv2);
                        break;
                    case VariantBase.Vtc.Tc7Ulong:
                        ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                        var ulvret = new UlongVariant();
                        ret = ulvret;
                        ulvret.SetValue(ulv1 % ulv2);
                        break;
                    default:
                        // это нужно будет заменить на соотв. msil-код
                        var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                        var gen = dyn.GetILGenerator();
                        gen.Emit(OpCodes.Ldarg_1);
                        gen.Emit(OpCodes.Ldarg_0);
                        gen.Emit(signed ? OpCodes.Rem : OpCodes.Rem_Un);
                        gen.Emit(OpCodes.Ret);
                        ret = new IntPtrVariant();
                        ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                        break;
                }
            return ret;
        }
        
        // Token: 0x060001CC RID: 460 RVA: 0x00009EAC File Offset: 0x000080AC
        private void Rem_(VariantBase dummy) // \u0006\u2000\u2001
        {
            PushVariant(Rem(true));
        }

        // Token: 0x060001FA RID: 506 RVA: 0x0000BC3C File Offset: 0x00009E3C
        private void Rem_un_(VariantBase dummy) // \u0006\u200B\u2000
        {
            PushVariant(Rem(false));
        }

        // Token: 0x06000254 RID: 596 RVA: 0x00010824 File Offset: 0x0000EA24
        private static VariantBase Neg(VariantBase v) // \u0002
        {
            if (v.GetTypeCode() == VariantBase.Vtc.Tc19Int)
            {
                var i = ((IntVariant)v).GetValue();
                var ivret = new IntVariant();
                ivret.SetValue(-i);
                return ivret;
            }
            if (v.GetTypeCode() == VariantBase.Vtc.Tc24Long)
            {
                var l = ((LongVariant)v).GetValue();
                var lvret = new LongVariant();
                lvret.SetValue(-l);
                return lvret;
            }
            if (v.GetTypeCode() == VariantBase.Vtc.Tc21Double)
            {
                var dvret = new DoubleVariant();
                dvret.SetValue(-((DoubleVariant)v).GetValue());
                return dvret;
            }
            if (v.GetTypeCode() != VariantBase.Vtc.Tc5Enum)
            {
                throw new InvalidOperationException();
            }
            var underlyingType = Enum.GetUnderlyingType(v.GetValueAbstract().GetType());
            if (underlyingType == typeof(long) || underlyingType == typeof(ulong))
            {
                var lvret = new LongVariant();
                lvret.SetValue(Convert.ToInt64(v.GetValueAbstract()));
                return Neg(lvret);
            }
            var ivret2 = new IntVariant();
            ivret2.SetValue(Convert.ToInt32(v.GetValueAbstract()));
            return Neg(ivret2);
        }

        // Token: 0x060001F8 RID: 504 RVA: 0x0000BA00 File Offset: 0x00009C00
        private void Neg_(VariantBase dummy) // \u0006\u2007
        {
            var v = PopVariant();
            PushVariant(Neg(v));
        }

        // Token: 0x06000242 RID: 578 RVA: 0x0000FE38 File Offset: 0x0000E038
        private static VariantBase DivLong(VariantBase v1, VariantBase v2, bool bUnsigned) // \u0003
        {
            var lvret = new LongVariant();
            if (!bUnsigned)
            {
                var l1 = ((LongVariant)v1).GetValue();
                var l2 = ((LongVariant)v2).GetValue();
                lvret.SetValue(l1 / l2);
                return lvret;
            }
            var u1 = (ulong)((LongVariant)v1).GetValue();
            var u2 = (ulong)((LongVariant)v2).GetValue();
            lvret.SetValue((long)(u1 / u2));
            return lvret;
        }

        private VariantBase Div(bool signed)
        {
            VariantBase v1, v2;
            var org_v2 = PopVariant();
            var org_v1 = PopVariant();
            VariantBase ret;
            var tc = CommonType(org_v1, org_v2, out v1, out v2, signed);
            if (IsFloating(org_v1) && org_v1.GetType() == org_v2.GetType() && !signed)
            {
                if (IntPtr.Size == 8) throw new InvalidProgramException();
                if(tc == VariantBase.Vtc.Tc21Double)
                {
                    ret = new DoubleVariant();
                    ret.SetValueAbstract(double.NaN);
                } else /*if (tc == VariantBase.Vtc.Tc8Float)*/
                {
                    ret = new FloatVariant();
                    ret.SetValueAbstract(float.NaN);
                }
            } else switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    uvret.SetValue(uv1 / uv2);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    ivret.SetValue(iv1 / iv2);
                    break;
                case VariantBase.Vtc.Tc21Double:
                    double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue();
                    var dvret = new DoubleVariant();
                    ret = dvret;
                    if(Math.Abs(dv2) < double.Epsilon && org_v1.GetType() != org_v2.GetType()) throw new DivideByZeroException();
                    dvret.SetValue(dv1 / dv2);
                    break;
                case VariantBase.Vtc.Tc8Float:
                    float fv1 = ((FloatVariant)v1).GetValue(), fv2 = ((FloatVariant)v2).GetValue();
                    var fvret = new FloatVariant();
                    ret = fvret;
                    if (Math.Abs(fv2) < float.Epsilon && org_v1.GetType() != org_v2.GetType()) throw new DivideByZeroException();
                    fvret.SetValue(fv1 / fv2);
                    break;
                case VariantBase.Vtc.Tc24Long:
                    long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                    var lvret = new LongVariant();
                    ret = lvret;
                    lvret.SetValue(lv1 / lv2);
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    ulvret.SetValue(ulv1 / ulv2);
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? OpCodes.Div : OpCodes.Div_Un);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x060001F0 RID: 496 RVA: 0x0000B664 File Offset: 0x00009864
        private void Div_(VariantBase dummy) // \u0002\u2009
        {
            PushVariant(Div(true));
        }

        // Token: 0x0600017C RID: 380 RVA: 0x00007DF4 File Offset: 0x00005FF4
        private void Div_un_(VariantBase dummy) // \u000E\u200B
        {
            PushVariant(Div(false));
        }

        // Token: 0x0600026D RID: 621 RVA: 0x00010FF4 File Offset: 0x0000F1F4
        private static void EmitLdc(ILGenerator gen, int val) // \u0002
        {
            switch (val)
            {
                case -1:
                    gen.Emit(OpCodes.Ldc_I4_M1);
                    return;
                case 0:
                    gen.Emit(OpCodes.Ldc_I4_0);
                    return;
                case 1:
                    gen.Emit(OpCodes.Ldc_I4_1);
                    return;
                case 2:
                    gen.Emit(OpCodes.Ldc_I4_2);
                    return;
                case 3:
                    gen.Emit(OpCodes.Ldc_I4_3);
                    return;
                case 4:
                    gen.Emit(OpCodes.Ldc_I4_4);
                    return;
                case 5:
                    gen.Emit(OpCodes.Ldc_I4_5);
                    return;
                case 6:
                    gen.Emit(OpCodes.Ldc_I4_6);
                    return;
                case 7:
                    gen.Emit(OpCodes.Ldc_I4_7);
                    return;
                case 8:
                    gen.Emit(OpCodes.Ldc_I4_8);
                    return;
                default:
                    if (val > -129 && val < 128)
                    {
                        gen.Emit(OpCodes.Ldc_I4_S, (sbyte)val);
                        return;
                    }
                    gen.Emit(OpCodes.Ldc_I4, val);
                    return;
            }
        }

        // Token: 0x06000264 RID: 612 RVA: 0x00010C28 File Offset: 0x0000EE28
        private static void EnsureClass(ILGenerator gen, Type t) // \u0003
        {
            if (t == SimpleTypeHelper.ObjectType)
            {
                return;
            }
            gen.Emit(OpCodes.Castclass, t);
        }

        // Token: 0x06000277 RID: 631 RVA: 0x0001180C File Offset: 0x0000FA0C
        private static void EnsureType(ILGenerator gen, Type t) // \u0005
        {
            if (t.IsValueType || ElementedTypeHelper.TryGoToElementType(t).IsGenericParameter)
            {
                gen.Emit(OpCodes.Unbox_Any, t);
                return;
            }
            EnsureClass(gen, t);
        }

        // Token: 0x06000217 RID: 535 RVA: 0x0000C7EC File Offset: 0x0000A9EC
        private static void EnsureBoxed(ILGenerator gen, Type t) // \u0002
        {
            if (t.IsValueType || ElementedTypeHelper.TryGoToElementType(t).IsGenericParameter)
            {
                gen.Emit(OpCodes.Box, t);
            }
        }

        // Token: 0x0600023E RID: 574 RVA: 0x0000FA54 File Offset: 0x0000DC54
        private DynamicExecutor DynamicFor(MethodBase mb, bool mayVirtual) // \u0002
        {
            DynamicMethod dynamicMethod; /*= null;
            if (_alwaysFalse && (!mb.IsConstructor || !typeof(Delegate).IsAssignableFrom(mb.DeclaringType)))
            {
                dynamicMethod = new DynamicMethod(string.Empty, SimpleTypeHelper.ObjectType, new Type[]
                {
                    SimpleTypeHelper.ObjectType,
                    ObjectArrayType
                }, true);
            }
            if (dynamicMethod == null)*/
            {
                dynamicMethod = new DynamicMethod(string.Empty, SimpleTypeHelper.ObjectType, new[]
                {
                    SimpleTypeHelper.ObjectType,
                    ObjectArrayType
                }, typeof(VmExecutor).Module, true);
            }
            var iLGenerator = dynamicMethod.GetILGenerator();
            var parameters = mb.GetParameters();
            var array = new Type[parameters.Length];
            var flag = false;
            for (var i = 0; i < parameters.Length; i++)
            {
                var type = parameters[i].ParameterType;
                if (type.IsByRef)
                {
                    flag = true;
                    type = type.GetElementType();
                }
                array[i] = type;
            }
            var array2 = new LocalBuilder[array.Length];
            if (array2.Length != 0)
            {
                dynamicMethod.InitLocals = true;
            }
            for (var j = 0; j < array.Length; j++)
            {
                array2[j] = iLGenerator.DeclareLocal(array[j]);
            }
            for (var k = 0; k < array.Length; k++)
            {
                iLGenerator.Emit(OpCodes.Ldarg_1);
                EmitLdc(iLGenerator, k);
                iLGenerator.Emit(OpCodes.Ldelem_Ref);
                EnsureType(iLGenerator, array[k]);
                iLGenerator.Emit(OpCodes.Stloc, array2[k]);
            }
            if (flag)
            {
                iLGenerator.BeginExceptionBlock();
            }
            if (!mb.IsStatic && !mb.IsConstructor)
            {
                iLGenerator.Emit(OpCodes.Ldarg_0);
                var declaringType = mb.DeclaringType;
                if (declaringType.IsValueType)
                {
                    iLGenerator.Emit(OpCodes.Unbox, declaringType);
                    mayVirtual = false;
                }
                else
                {
                    EnsureClass(iLGenerator, declaringType);
                }
            }
            for (var l = 0; l < array.Length; l++)
            {
                iLGenerator.Emit(parameters[l].ParameterType.IsByRef ? OpCodes.Ldloca_S : OpCodes.Ldloc, array2[l]);
            }
            if (mb.IsConstructor)
            {
                iLGenerator.Emit(OpCodes.Newobj, (ConstructorInfo)mb);
                EnsureBoxed(iLGenerator, mb.DeclaringType);
            }
            else
            {
                var methodInfo = (MethodInfo)mb;
                if (!mayVirtual || mb.IsStatic)
                {
                    iLGenerator.EmitCall(OpCodes.Call, methodInfo, null);
                }
                else
                {
                    iLGenerator.EmitCall(OpCodes.Callvirt, methodInfo, null);
                }
                if (methodInfo.ReturnType == VoidType)
                {
                    iLGenerator.Emit(OpCodes.Ldnull);
                }
                else
                {
                    EnsureBoxed(iLGenerator, methodInfo.ReturnType);
                }
            }
            if (flag)
            {
                var local = iLGenerator.DeclareLocal(SimpleTypeHelper.ObjectType);
                iLGenerator.Emit(OpCodes.Stloc, local);
                iLGenerator.BeginFinallyBlock();
                for (var m = 0; m < array.Length; m++)
                {
                    if (parameters[m].ParameterType.IsByRef)
                    {
                        iLGenerator.Emit(OpCodes.Ldarg_1);
                        EmitLdc(iLGenerator, m);
                        iLGenerator.Emit(OpCodes.Ldloc, array2[m]);
                        if (array2[m].LocalType.IsValueType || ElementedTypeHelper.TryGoToElementType(array2[m].LocalType).IsGenericParameter)
                        {
                            iLGenerator.Emit(OpCodes.Box, array2[m].LocalType);
                        }
                        iLGenerator.Emit(OpCodes.Stelem_Ref);
                    }
                }
                iLGenerator.EndExceptionBlock();
                iLGenerator.Emit(OpCodes.Ldloc, local);
            }
            iLGenerator.Emit(OpCodes.Ret);
            return (DynamicExecutor)dynamicMethod.CreateDelegate(typeof(DynamicExecutor));
        }

        // Token: 0x06000209 RID: 521 RVA: 0x0000C4D0 File Offset: 0x0000A6D0
        private static bool HasByRefParameter(MethodBase mb) // \u0002
        {
            return mb.GetParameters().Any(t => t.ParameterType.IsByRef);
        }

        // Token: 0x060001D9 RID: 473 RVA: 0x0000A72C File Offset: 0x0000892C
        private object Invoke(MethodBase mb, object obj, object[] args) // \u0002
        {
            if (mb.IsConstructor)
            {
                // ReSharper disable once AssignNullToNotNullAttribute
                return Activator.CreateInstance(mb.DeclaringType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, args, null);
            }
            return mb.Invoke(obj, args);
        }

        // Token: 0x06000286 RID: 646 RVA: 0x00011EFC File Offset: 0x000100FC
        private object InvokeDynamic(MethodBase mb, object obj, object[] args, bool mayVirtual) // \u0002
        {
            /*if (!_alwaysTrue)
            {
                return Invoke(mb, obj, args);
            }*/
            var key = new MethodBaseAndVirtual(mb, mayVirtual);
            DynamicExecutor executor;
            lock (_dynamicExecutors)
            {
                _dynamicExecutors.TryGetValue(key, out executor);
            }
            if (executor == null)
            {
                bool needFaster;
                lock (_mbCallCnt)
                {
                    int num;
                    _mbCallCnt.TryGetValue(mb, out num);
                    needFaster = num >= 50;
                    if (!needFaster)
                    {
                        _mbCallCnt[mb] = num + 1;
                    }
                }
                if (!needFaster && !mayVirtual && obj == null && !mb.IsStatic && !mb.IsConstructor)
                {
                    needFaster = true;
                }
                if (!needFaster && HasByRefParameter(mb))
                {
                    needFaster = true;
                }
                if (!needFaster)
                {
                    return Invoke(mb, obj, args);
                }
                lock (_mbDynamicLock)
                {
                    while (_mbDynamicLock.ContainsKey(mb))
                    {
                        Monitor.Wait(_mbDynamicLock);
                    }
                    _mbDynamicLock[mb] = null;
                }
                try
                {
                    lock (_dynamicExecutors)
                    {
                        _dynamicExecutors.TryGetValue(key, out executor);
                    }
                    if (executor == null)
                    {
                        executor = DynamicFor(mb, mayVirtual);
                        lock (_dynamicExecutors)
                        {
                            _dynamicExecutors[key] = executor;
                        }
                    }
                    lock (_mbCallCnt)
                    {
                        _mbCallCnt.Remove(mb);
                    }
                }
                finally
                {
                    lock (_mbDynamicLock)
                    {
                        _mbDynamicLock.Remove(mb);
                        Monitor.PulseAll(_mbDynamicLock);
                    }
                }
            }
            return executor(obj, args);
        }

        // Token: 0x0600028D RID: 653 RVA: 0x000122A4 File Offset: 0x000104A4
        private VariantBase FetchByAddr(VariantBase addr) // \u0003
        {
            if (!addr.IsAddr())
            {
                throw new ArgumentException();
            }
            var num = addr.GetTypeCode();
            if (num == VariantBase.Vtc.Tc0VariantBaseHolder)
            {
                return ((VariantBaseHolder)addr).GetValue();
            }
            if (num != VariantBase.Vtc.Tc4FieldInfo)
            {
                switch (num)
                {
                    case VariantBase.Vtc.Tc20MdArrayValue:
                    case VariantBase.Vtc.Tc22SdArrayValue:
                        {
                            var avv = (ArrayValueVariantBase)addr;
                            return VariantFactory.Convert(avv.GetValue(), avv.GetHeldType());
                        }
                    case VariantBase.Vtc.Tc23LocalsIdxHolder:
                        return _localVariables[((LocalsIdxHolderVariant)addr).GetValue()];
                }
                throw new ArgumentOutOfRangeException();
            }
            var fiv = (FieldInfoVariant)addr;
            return VariantFactory.Convert(fiv.GetValue().GetValue(fiv.GetObject()), null);
        }

        // Token: 0x06000186 RID: 390 RVA: 0x00008150 File Offset: 0x00006350
        private FieldInfo ResolveField(int id) // \u0002
        {
            FieldInfo result;
            lock (AllMetadataById)
            {
                object md;
                if (AllMetadataById.TryGetValue(id, out md))
                {
                    result = (FieldInfo)md;
                }
                else
                {
                    var U0003U2008 = ReadToken(id);
                    if (U0003U2008.IsVm == 0)
                    {
                        result = _module.ResolveField(U0003U2008.MetadataToken);
                    }
                    else
                    {
                        var u000Fu2006 = (VmFieldTokenInfo)U0003U2008.VmToken;
                        var expr_70 = GetTypeById(u000Fu2006.Class.MetadataToken);
                        var bindingAttr = BF(u000Fu2006.IsStatic);
                        var field = expr_70.GetField(u000Fu2006.Name, bindingAttr);
                        if (!expr_70.IsGenericType)
                        {
                            AllMetadataById.Add(id, field);
                        }
                        result = field;
                    }
                }
            }
            return result;
        }

        // Token: 0x06000177 RID: 375 RVA: 0x00007CD8 File Offset: 0x00005ED8
        private void Ldflda_(VariantBase vFieldId) // \u0003\u2009
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
            var reference = PopVariant();
            var obj = reference.IsAddr() ? FetchByAddr(reference).GetValueAbstract() : reference.GetValueAbstract();
            var val = new FieldInfoVariant();
            val.SetValue(fieldInfo);
            val.SetObject(obj);
            PushVariant(val);
        }

        // Token: 0x0600018F RID: 399 RVA: 0x0000840C File Offset: 0x0000660C
        private void Ldsflda_(VariantBase vFieldId) // \u000E\u2002
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
            var val = new FieldInfoVariant();
            val.SetValue(fieldInfo);
            PushVariant(val);
        }

        // Token: 0x0600017D RID: 381 RVA: 0x00007E20 File Offset: 0x00006020
        private void Ldtoken_(VariantBase vToken) // \u0005\u2002\u2000
        {
            var t = ReadToken(((IntVariant)vToken).GetValue());
            object obj;
            if (t.IsVm == 0)
            {
                obj = ResolveNativeToken(t.MetadataToken);
            }
            else
            {
                switch (t.VmToken.TokenKind())
                {
                    case VmTokenInfo.Kind.Class0:
                        obj = GetTypeById(((IntVariant)vToken).GetValue()).TypeHandle;
                        break;
                    case VmTokenInfo.Kind.Field1:
                        obj = ResolveField(((IntVariant)vToken).GetValue()).FieldHandle;
                        break;
                    case VmTokenInfo.Kind.Method2:
                        obj = FindMethodById(((IntVariant)vToken).GetValue()).MethodHandle;
                        break;
                    default:
                        throw new InvalidOperationException();
                }
            }
            var push = new ObjectVariant();
            push.SetValue(obj);
            PushVariant(push);
        }

        // Token: 0x06000206 RID: 518 RVA: 0x0000C1C8 File Offset: 0x0000A3C8
        private void Ldfld_(VariantBase vFieldId) // \u0005\u2003\u2001
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
            var reference = PopVariant();
            if (reference.IsAddr())
            {
                reference = FetchByAddr(reference);
            }
            var obj = reference.GetValueAbstract();
            if (obj == null)
            {
                throw new NullReferenceException();
            }
            PushVariant(VariantFactory.Convert(fieldInfo.GetValue(obj), fieldInfo.FieldType));
        }

        // Token: 0x06000225 RID: 549 RVA: 0x0000F2CC File Offset: 0x0000D4CC
        private void Ldsfld_(VariantBase vFieldId) // \u000F\u2004\u2000
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
            PushVariant(VariantFactory.Convert(fieldInfo.GetValue(null), fieldInfo.FieldType));
        }

        // Token: 0x0600024A RID: 586 RVA: 0x0001034C File Offset: 0x0000E54C
        private void Newobj_(VariantBase vCtrId) // \u0002\u200A
        {
            var num = ((IntVariant)vCtrId).GetValue();
            var methodBase = FindMethodById(num);
            var declaringType = methodBase.DeclaringType;
            var parameters = methodBase.GetParameters();
            var expr_2A = parameters.Length;
            var array = new object[expr_2A];
            var dictionary = new Dictionary<int, VariantBase>();
            for (var i = expr_2A - 1; i >= 0; i--)
            {
                var u000F = PopVariant();
                if (u000F.IsAddr())
                {
                    dictionary.Add(i, u000F);
                    u000F = FetchByAddr(u000F);
                }
                if (u000F.GetVariantType() != null)
                {
                    u000F = VariantFactory.Convert(null, u000F.GetVariantType()).CopyFrom(u000F);
                }
                var u000F2 = VariantFactory.Convert(null, parameters[i].ParameterType).CopyFrom(u000F);
                array[i] = u000F2.GetValueAbstract();
            }
            object obj;
            try
            {
                obj = InvokeDynamic(methodBase, null, array, false);
            }
            catch (TargetInvocationException ex)
            {
                var expr_C2 = ex.InnerException ?? ex;
                SerializeCrossDomain(expr_C2);
                throw expr_C2;
            }
            foreach (var current in dictionary)
            {
                AssignByReference(current.Value, VariantFactory.Convert(array[current.Key], null));
            }
            PushVariant(VariantFactory.Convert(obj, declaringType));
        }

        // Token: 0x06000227 RID: 551 RVA: 0x0000F3B4 File Offset: 0x0000D5B4
        private void AssignByReference(VariantBase refDest, VariantBase val) // \u0002
        {
            switch (refDest.GetTypeCode())
            {
                case VariantBase.Vtc.Tc0VariantBaseHolder:
                    ((VariantBaseHolder)refDest).GetValue().CopyFrom(val);
                    return;
                case VariantBase.Vtc.Tc4FieldInfo:
                    var refFieldInfoDest = (FieldInfoVariant)refDest;
                    var fieldInfo = refFieldInfoDest.GetValue();
                    fieldInfo.SetValue(refFieldInfoDest.GetObject(), VariantFactory.Convert(val.GetValueAbstract(), fieldInfo.FieldType).GetValueAbstract());
                    return;
                case VariantBase.Vtc.Tc20MdArrayValue:
                case VariantBase.Vtc.Tc22SdArrayValue:
                    var refArrayValueDest = (ArrayValueVariantBase)refDest;
                    refArrayValueDest.SetValue(VariantFactory.Convert(val.GetValueAbstract(), refArrayValueDest.GetHeldType()).GetValueAbstract());
                    return;
                case VariantBase.Vtc.Tc23LocalsIdxHolder:
                    _localVariables[((LocalsIdxHolderVariant)refDest).GetValue()].CopyFrom(val);
                    return;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        // Token: 0x060001E5 RID: 485 RVA: 0x0000AD10 File Offset: 0x00008F10
        private void Invoke(MethodBase mb, bool mayVirtual) // \u0002
        {
            if (!mayVirtual && IsCompatible(mb))
            {
                mb = GenerateDynamicCall(mb, false);
            }
            var parameters = mb.GetParameters();
            var num = parameters.Length;
            var poppedArgs = new VariantBase[num];
            var args = new object[num];
            var wasLocked = default(BoolHolder);
            try
            {
                LockIfInterlocked(ref wasLocked, mb, mayVirtual);
                for (var i = num - 1; i >= 0; i--)
                {
                    var u000F = PopVariant();
                    poppedArgs[i] = u000F;
                    if (u000F.IsAddr())
                    {
                        u000F = FetchByAddr(u000F);
                    }
                    if (u000F.GetVariantType() != null)
                    {
                        u000F = VariantFactory.Convert(null, u000F.GetVariantType()).CopyFrom(u000F);
                    }
                    var u000F2 = VariantFactory.Convert(null, parameters[i].ParameterType).CopyFrom(u000F);
                    args[i] = u000F2.GetValueAbstract();
                }
                VariantBase u000F3 = null;
                if (!mb.IsStatic)
                {
                    u000F3 = PopVariant();
                    if (u000F3?.GetVariantType() != null)
                    {
                        u000F3 = VariantFactory.Convert(null, u000F3.GetVariantType()).CopyFrom(u000F3);
                    }
                }
                object obj = null;
                try
                {
                    if (mb.IsConstructor)
                    {
                        obj = Activator.CreateInstance(mb.DeclaringType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, args, null);
                        if (u000F3 != null && !u000F3.IsAddr())
                        {
                            throw new InvalidOperationException();
                        }
                        AssignByReference(u000F3, VariantFactory.Convert(obj, mb.DeclaringType));
                    }
                    else
                    {
                        object poppedThis = null;
                        if (u000F3 != null)
                        {
                            var u000F4 = u000F3;
                            if (u000F3.IsAddr())
                            {
                                u000F4 = FetchByAddr(u000F3);
                            }
                            poppedThis = u000F4.GetValueAbstract();
                        }
                        try
                        {
                            if (!InvokeFilter(mb, poppedThis, ref obj, args))
                            {
                                if (mayVirtual && !mb.IsStatic && poppedThis == null)
                                {
                                    throw new NullReferenceException();
                                }
                                if (!AlwaysFalse(mb, poppedThis, poppedArgs, args, mayVirtual, ref obj))
                                {
                                    obj = InvokeDynamic(mb, poppedThis, args, mayVirtual);
                                }
                            }
                            if (u000F3 != null && u000F3.IsAddr())
                            {
                                AssignByReference(u000F3, VariantFactory.Convert(poppedThis, mb.DeclaringType));
                            }
                        }
                        catch (TargetInvocationException ex)
                        {
                            var cause = ex.InnerException ?? ex;
                            SerializeCrossDomain(cause);
                            throw cause;
                        }
                    }
                }
                finally
                {
                    for (var j = 0; j < poppedArgs.Length; j++)
                    {
                        var u000F5 = poppedArgs[j];
                        if (u000F5.IsAddr())
                        {
                            var obj3 = args[j];
                            AssignByReference(u000F5, VariantFactory.Convert(obj3, null));
                        }
                    }
                }
                var methodInfo = mb as MethodInfo;
                if (methodInfo != null)
                {
                    var returnType = methodInfo.ReturnType;
                    if (returnType != VoidType)
                    {
                        PushVariant(VariantFactory.Convert(obj, returnType));
                    }
                }
            }
            finally
            {
                UnlockInterlockedIfAny(ref wasLocked);
            }
        }

        // Token: 0x060001FD RID: 509 RVA: 0x0000BD88 File Offset: 0x00009F88
        private void DoJmp(int pos, Type[] methodGenericArgs, Type[] classGenericArgs, bool mayVirtual) // \u0002
        {
            _srcVirtualizedStreamReader.BaseStream.Seek(pos, SeekOrigin.Begin);
            DoNothing(_srcVirtualizedStreamReader);
            var u0006 = ReadMethodHeader(_srcVirtualizedStreamReader);
            var num = u0006.ArgsTypeToOutput.Length;
            var array = new object[num];
            var array2 = new VariantBase[num];
            if (_currentClass != null & mayVirtual)
            {
                var num2 = u0006.IsStatic() ? 0 : 1;
                var array3 = new Type[num - num2];
                for (var i = num - 1; i >= num2; i--)
                {
                    array3[i] = GetTypeById(u0006.ArgsTypeToOutput[i].TypeId);
                }
                var method = _currentClass.GetMethod(u0006.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.InvokeMethod | BindingFlags.GetProperty | BindingFlags.SetProperty, null, array3, null);
                _currentClass = null;
                if (method != null)
                {
                    Invoke(method, true);
                    return;
                }
            }
            for (var j = num - 1; j >= 0; j--)
            {
                var u000F = PopVariant();
                array2[j] = u000F;
                if (u000F.IsAddr())
                {
                    u000F = FetchByAddr(u000F);
                }
                if (u000F.GetVariantType() != null)
                {
                    u000F = VariantFactory.Convert(null, u000F.GetVariantType()).CopyFrom(u000F);
                }
                var u000F2 = VariantFactory.Convert(null, GetTypeById(u0006.ArgsTypeToOutput[j].TypeId)).CopyFrom(u000F);
                array[j] = u000F2.GetValueAbstract();
                if (j == 0 & mayVirtual && !u0006.IsStatic() && array[j] == null)
                {
                    throw new NullReferenceException();
                }
            }
            var u0006u2007 = new VmExecutor(_instrCodesDb);
            var callees = new object[]
            {
                _module.Assembly
            };
            object obj;
            try
            {
                obj = u0006u2007.Invoke(_srcVirtualizedStream, pos, array, methodGenericArgs, classGenericArgs, callees);
            }
            finally
            {
                for (var k = 0; k < array2.Length; k++)
                {
                    var u000F3 = array2[k];
                    if (u000F3.IsAddr())
                    {
                        var obj2 = array[k];
                        AssignByReference(u000F3, VariantFactory.Convert(obj2, null));
                    }
                }
            }
            var type = u0006u2007.GetTypeById(u0006u2007._methodHeader.ReturnTypeId);
            if (type != VoidType)
            {
                PushVariant(VariantFactory.Convert(obj, type));
            }
        }

        // Token: 0x0600027A RID: 634 RVA: 0x00011994 File Offset: 0x0000FB94
        private void JmpToRef(VmMethodRefTokenInfo mref) // \u0002
        {
            //var arg_18_0 = (U0008U2007)U0003U2008.Get_u0005();
            var methodBase = FindMethodById(mref.Pos, ReadToken(mref.Pos));
            //methodBase.GetParameters();
            var num = mref.Flags;
            var mayVirtual = (num & 1073741824) != 0;
            num &= -1073741825;
            var methodGenericArgs = _methodGenericArgs;
            var classGenericArgs = _classGenericArgs;
            try
            {
                _methodGenericArgs = methodBase is ConstructorInfo ? Type.EmptyTypes : methodBase.GetGenericArguments();
                _classGenericArgs = methodBase.DeclaringType.GetGenericArguments();
                DoJmp(num, _methodGenericArgs, _classGenericArgs, mayVirtual);
            }
            finally
            {
                _methodGenericArgs = methodGenericArgs;
                _classGenericArgs = classGenericArgs;
            }
        }

        // Token: 0x060001EE RID: 494 RVA: 0x0000B5FC File Offset: 0x000097FC
        private void Jmp_(VariantBase vPos) // \u0008\u200B\u2000
        {
            var pos = ((IntVariant)vPos).GetValue();
            var arg_29_0 = (pos & -2147483648) != 0;
            var mayVirtual = (pos & 1073741824) != 0;
            pos &= 1073741823;
            if (arg_29_0)
            {
                DoJmp(pos, null, null, mayVirtual);
                return;
            }
            JmpToRef((VmMethodRefTokenInfo)ReadToken(pos).VmToken);
        }

        // Token: 0x06000199 RID: 409 RVA: 0x00008660 File Offset: 0x00006860
        private void Calli_(VariantBase dummy) // \u0003\u2002
        {
            var methodBase = ((MethodVariant)PopVariant()).GetValue();
            Invoke(methodBase, false);
        }

        // Token: 0x06000184 RID: 388 RVA: 0x000080F4 File Offset: 0x000062F4
        private void Call_(VariantBase vMethodId) // \u000E\u2003
        {
            var methodBase = FindMethodById(((IntVariant)vMethodId).GetValue());
            foreach (var arg in _variantOutputArgs)
            {
                PushVariant(arg);
            }
            Invoke(methodBase, false);
        }

        // Token: 0x06000191 RID: 401 RVA: 0x0000845C File Offset: 0x0000665C
        private void Callvirt_(VariantBase vMethodId) // \u000E\u2005
        {
            var methodBase = FindMethodById(((IntVariant)vMethodId).GetValue());
            if (_currentClass != null)
            {
                var pars = methodBase.GetParameters();
                var types = new Type[pars.Length];
                var num = 0;
                foreach (var parameterInfo in pars)
                {
                    types[num++] = parameterInfo.ParameterType;
                }
                var method = _currentClass.GetMethod(methodBase.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.InvokeMethod | BindingFlags.GetProperty | BindingFlags.SetProperty, null, types, null);
                if (method != null)
                {
                    methodBase = method;
                }
                _currentClass = null;
            }
            Invoke(methodBase, true);
        }

        // Token: 0x060001CE RID: 462 RVA: 0x0000A1A0 File Offset: 0x000083A0
        private void Invoke(VariantBase vMethodId) // \u000E\u200A
        {
            var methodBase = FindMethodById(((IntVariant)vMethodId).GetValue());
            Invoke(methodBase, false);
        }

        // Token: 0x06000205 RID: 517 RVA: 0x0000C1A8 File Offset: 0x0000A3A8
        [DebuggerNonUserCode]
        private MethodBase FindMethodById(int methodId) // \u0002
        {
            return FindMethodById(methodId, ReadToken(methodId));
        }

        // Token: 0x06000171 RID: 369 RVA: 0x00007BBC File Offset: 0x00005DBC
        private object Invoke(Stream srcVirtualizedStream, int pos, object[] args, Type[] methodGenericArgs, Type[] classGenericArgs, object[] callees) // \u0002
        {
		    _srcVirtualizedStream = srcVirtualizedStream;
		    Seek(pos, srcVirtualizedStream, null);
		    return Invoke(args, methodGenericArgs, classGenericArgs, callees);
	    }

	    // Token: 0x06000172 RID: 370 RVA: 0x00007BDC File Offset: 0x00005DDC
	    private bool AlwaysFalse(MethodBase mb, object poppedThis, VariantBase[] poppedArgs, object[] args, bool mayVirtual, ref object obj) // \u0002
        {
		    return false;
        }

	    // Token: 0x06000179 RID: 377 RVA: 0x00007DA8 File Offset: 0x00005FA8
	    private void Leave_(VariantBase vTarget) // \u0006\u200B
        {
            OnException(null, ((UintVariant)vTarget).GetValue());
	    }

	    // Token: 0x06000180 RID: 384 RVA: 0x00007F48 File Offset: 0x00006148
	    private void Castclass_(VariantBase vTypeId) // \u0005\u2003
        {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var obj = PopVariant();
		    if (Isinst(obj, type))
		    {
			    PushVariant(obj);
			    return;
		    }
		    throw new InvalidCastException();
	    }

        // Token: 0x06000185 RID: 389 RVA: 0x00008144 File Offset: 0x00006344
        private void Ldc_i4_s_(VariantBase val) // \u000E\u2003\u2000
        {
		    PushVariant(val);
	    }

	    // Token: 0x0600018B RID: 395 RVA: 0x0000827C File Offset: 0x0000647C
	    private void Stelem_i2_(VariantBase dummy) // \u000E\u200A\u2000
        {
		    var obj = PopVariant().GetValueAbstract();
		    var idx = PopLong();
		    var array = (Array)PopVariant().GetValueAbstract();
		    var elementType = array.GetType().GetElementType();
		    checked
		    {
			    if (elementType == typeof(short))
			    {
                    ((short[])array)[(int)(IntPtr)idx] = (short)VariantFactory.Convert(obj, typeof(short)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(ushort))
			    {
                    ((ushort[])array)[(int)(IntPtr)idx] = (ushort)VariantFactory.Convert(obj, typeof(ushort)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(char))
			    {
                    ((char[])array)[(int)(IntPtr)idx] = (char)VariantFactory.Convert(obj, typeof(char)).GetValueAbstract();
				    return;
			    }
			    if (elementType.IsEnum)
			    {
                    Stelem(elementType, obj, idx, array);
				    return;
			    }
                Stelem(typeof(short), obj, idx, array);
		    }
	    }

	    // Token: 0x0600018E RID: 398 RVA: 0x00008404 File Offset: 0x00006604
	    private void Stind_i_(VariantBase dummy) // \u000F\u2006
        {
		    Stind();
	    }

	    // Token: 0x06000190 RID: 400 RVA: 0x00008440 File Offset: 0x00006640
	    private void Ldloc_0_(VariantBase dummy) // \u0006
        {
		    PushVariant(_localVariables[0].Clone());
	    }

	    // Token: 0x06000193 RID: 403 RVA: 0x00008508 File Offset: 0x00006708
	    private void And_(VariantBase dummy) // \u000F\u200B\u2000
        {
		    PushVariant(And(PopVariant(), PopVariant()));
	    }

	    // Token: 0x06000194 RID: 404 RVA: 0x00008534 File Offset: 0x00006734
	    private void Bge_un_(VariantBase vpos) // \u0006\u2009
	    {
		    var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.GE, true))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
		    }
	    }

        // Token: 0x06000198 RID: 408 RVA: 0x00008628 File Offset: 0x00006828
        private void Blt_un_(VariantBase vpos) // \u0006\u2006\u2000
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.LT, true))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x06000234 RID: 564 RVA: 0x0000F934 File Offset: 0x0000DB34
        private void Bge_(VariantBase vpos) // \u0002\u200B
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.GE, false))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x060002AB RID: 683 RVA: 0x00012D80 File Offset: 0x00010F80
        private void Blt_(VariantBase vpos) // \u0003\u2001
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.LT, false))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x0600023D RID: 573 RVA: 0x0000FA1C File Offset: 0x0000DC1C
        private void Bgt_(VariantBase vpos) // \u000F\u2000\u2001
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.GT, false))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x060001A3 RID: 419 RVA: 0x00008D10 File Offset: 0x00006F10
        private void Bgt_un_(VariantBase vpos) // \u0005\u2004
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.GT, true))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x06000261 RID: 609 RVA: 0x00010BB4 File Offset: 0x0000EDB4
        private void Bne_un_(VariantBase vpos) // \u000F\u2007
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.NEQ, true))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x0600029D RID: 669 RVA: 0x000126E0 File Offset: 0x000108E0
        private void Beq_(VariantBase vpos) // \u0002\u2007\u2000
        {
            var v2 = PopVariant();
            if (UniCompare(PopVariant(), v2, ComparisonKind.EQ, false))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x060001B9 RID: 441 RVA: 0x000093F8 File Offset: 0x000075F8
        private void Ble_un_(VariantBase vpos) // \u0003\u2007\u2000
        {
            var v2 = PopVariant();
            var v1 = PopVariant();
            if (UniCompare(v1, v2, ComparisonKind.LE, true))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        private static bool IsFloating(VariantBase v)
        {
            return v.GetTypeCode() == VariantBase.Vtc.Tc21Double || v.GetTypeCode() == VariantBase.Vtc.Tc8Float;
        }
        // Token: 0x06000297 RID: 663 RVA: 0x0001253C File Offset: 0x0001073C
        private void Ble_(VariantBase vpos) // \u0002\u2003\u2000
        {
            var v2 = PopVariant();
            var v1 = PopVariant();
            if (UniCompare(v1, v2, ComparisonKind.LE, false))
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x06000228 RID: 552 RVA: 0x0000F480 File Offset: 0x0000D680
        private void Brfalse_(VariantBase vpos) // \u0002\u2002
        {
            var val = PopVariant();
            var num = val.GetTypeCode();
            bool flag;
            switch (num)
            {
                case VariantBase.Vtc.Tc5Enum:
                    flag = !Convert.ToBoolean(((EnumVariant)val).GetValue());
                    break;
                case VariantBase.Vtc.Tc13UIntPtr:
                    flag = ((UIntPtrVariant)val).GetValue() == UIntPtr.Zero;
                    break;
                case VariantBase.Vtc.Tc17IntPtr:
                    flag = ((IntPtrVariant)val).GetValue() == IntPtr.Zero;
                    break;
                case VariantBase.Vtc.Tc18Object:
                    flag = ((ObjectVariant)val).GetValue() == null;
                    break;
                case VariantBase.Vtc.Tc19Int:
                    flag = ((IntVariant)val).GetValue() == 0;
                    break;
                case VariantBase.Vtc.Tc24Long:
                    flag = ((LongVariant)val).GetValue() == 0L;
                    break;
                default:
                    flag = val.GetValueAbstract() == null;
                    break;
            }
            if (flag)
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x06000274 RID: 628 RVA: 0x00011488 File Offset: 0x0000F688
        private void ExecuteExceptionHandler() // \u0005
        {
            if (_ehStack.Count == 0)
            {
                if (_wasException)
                {
                    _myBufferPos = _myBufferReader.GetBuffer().GetPos();
                    ThrowStoreCrossDomain(_exception);
                }
                return;
            }
            var ehFrame = _ehStack.PopBack();
            if (ehFrame.Exception != null)
            {
                var toStack = new ObjectVariant();
                toStack.SetValueAbstract(ehFrame.Exception);
                PushVariant(toStack);
            }
            else
            {
                _evalStack.Clear();
            }
            JumpToPos(ehFrame.Pos);
        }

        // Token: 0x060001F2 RID: 498 RVA: 0x0000B6C8 File Offset: 0x000098C8
        private void Switch_(VariantBase vSwitchPosArray) // \u0002\u200A\u2000
        {
            var vidx = PopVariant();
            uint idx;
            switch (vidx.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    idx = (uint)Convert.ToInt64(vidx.GetValueAbstract());
                    break;
                case VariantBase.Vtc.Tc19Int:
                    idx = (uint)((IntVariant)vidx).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    idx = (uint)((LongVariant)vidx).GetValue();
                    break;
                default:
                    throw new InvalidOperationException();
            }
            var switchPosArray = (IntVariant[])((ArrayVariant)vSwitchPosArray).GetValue();
            if (idx >= (ulong)switchPosArray.Length)
            {
                return;
            }
            JumpToPos((uint)switchPosArray[(int)idx].GetValue());
        }

        // Token: 0x060001FB RID: 507 RVA: 0x0000BC68 File Offset: 0x00009E68
        private void Brtrue_(VariantBase vpos) // \u0008\u2007
        {
            var val = PopVariant();
            var num = val.GetTypeCode();
            bool flag;
            switch (num)
            {
                case VariantBase.Vtc.Tc5Enum:
                    flag = Convert.ToBoolean(((EnumVariant)val).GetValue());
                    break;
                case VariantBase.Vtc.Tc13UIntPtr:
                    flag = ((UIntPtrVariant)val).GetValue() != UIntPtr.Zero;
                    break;
                case VariantBase.Vtc.Tc17IntPtr:
                    flag = ((IntPtrVariant)val).GetValue() != IntPtr.Zero;
                    break;
                case VariantBase.Vtc.Tc18Object:
                    flag = ((ObjectVariant)val).GetValue() != null;
                    break;
                case VariantBase.Vtc.Tc19Int:
                    flag = ((IntVariant)val).GetValue() != 0;
                    break;
                case VariantBase.Vtc.Tc24Long:
                    flag = ((LongVariant)val).GetValue() != 0L;
                    break;
                default:
                    flag = val.GetValueAbstract() != null;
                    break;
            }
            if (flag)
            {
                JumpToPos(((UintVariant)vpos).GetValue());
            }
        }

        // Token: 0x06000214 RID: 532 RVA: 0x0000C764 File Offset: 0x0000A964
        private void Br_(VariantBase vpos) // \u0005\u2008\u2000
        {
            JumpToPos(((UintVariant)vpos).GetValue());
        }

        private void Conv_r_un_(VariantBase dummy)
        {
            var pop = PopVariant();
            double val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    if (Marshal.SizeOf(v) < 8)
                        val = (double)(uint)Convert.ToInt32(v);
                    else
                        val = (double)(ulong)Convert.ToInt64(v);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    val = (double)(uint)((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    val = ((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    val = (double)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    val = (double)(ulong)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(double), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(OpCodes.Conv_R_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (double)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new DoubleVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        private void Conv_r(OpCode oc, Func<object, double> F)
        {
            var pop = PopVariant();
            double val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    val = F(Marshal.SizeOf(v) < 8 ? Convert.ToInt32(v) : Convert.ToInt64(v));
                    break;
                case VariantBase.Vtc.Tc19Int:
                    val = F(((IntVariant)pop).GetValue());
                    break;
                case VariantBase.Vtc.Tc21Double:
                    val = F(((DoubleVariant)pop).GetValue());
                    break;
                case VariantBase.Vtc.Tc8Float:
                    val = F(((FloatVariant)pop).GetValue());
                    break;
                case VariantBase.Vtc.Tc24Long:
                    val = F(((LongVariant)pop).GetValue());
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(double), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(oc);
                    gen.Emit(OpCodes.Ret);
                    val = (double)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new DoubleVariant();
            push.SetValue(val);
            PushVariant(push);
        }
        private void Conv_r4_(VariantBase dummy)
        {
            Conv_r(OpCodes.Conv_R4, o => (double)(float)Convert.ChangeType(o, typeof(float)));
        }
        private void Conv_r8_(VariantBase dummy)
        {
            Conv_r(OpCodes.Conv_R8, o => (double)Convert.ChangeType(o, typeof(double)));
        }

        // Token: 0x06000196 RID: 406 RVA: 0x00008600 File Offset: 0x00006800
        private void Ldind_i4_(VariantBase dummy) // \u0002\u2001\u2000
        {
		    Ldind(typeof(int));
	    }

	    // Token: 0x06000197 RID: 407 RVA: 0x00008614 File Offset: 0x00006814
	    private void Ldind_u1_(VariantBase dummy) // \u000E
        {
		    Ldind(typeof(byte));
	    }

	    // Token: 0x0600019A RID: 410 RVA: 0x00008688 File Offset: 0x00006888
	    private void Isinst_(VariantBase vTypeId) // \u000F\u2007\u2000 
        {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var obj = PopVariant();
		    if (Isinst(obj, type))
		    {
			    PushVariant(obj);
			    return;
		    }
		    PushVariant(new ObjectVariant());
	    }

	    // Token: 0x0600019D RID: 413 RVA: 0x00008B68 File Offset: 0x00006D68
	    private void Initobj_(VariantBase vTypeId) // \u0005\u2005\u2000 
        {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var dest = PopVariant();
		    if (!type.IsValueType)
		    {
                AssignByReference(dest, new ObjectVariant());
			    return;
		    }
		    var obj = FetchByAddr(dest).GetValueAbstract();
		    if (SimpleTypeHelper.IsNullableGeneric(type))
		    {
			    var val = new ObjectVariant();
			    val.SetVariantType(type);
                AssignByReference(dest, val);
			    return;
		    }
		    var fields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy);
		    foreach (var fieldInfo in fields)
		    {
		        fieldInfo.SetValue(obj, CreateValueTypeInstance(fieldInfo.FieldType));
		    }
	    }

	    // Token: 0x0600019E RID: 414 RVA: 0x00008C08 File Offset: 0x00006E08
	    private void Ldarg_s_(VariantBase vidx) // \u000E\u2000
        {
		    var idx = (ByteVariant)vidx;
		    PushVariant(_variantOutputArgs[idx.GetValue()].Clone());
	    }

        // Token: 0x06000244 RID: 580 RVA: 0x00010160 File Offset: 0x0000E360
        private void Ldarga_s_(VariantBase vidx) // \u0008\u2001\u2001
        {
            var idx = (ByteVariant)vidx;
            var push = new VariantBaseHolder();
            push.SetValue(_variantOutputArgs[idx.GetValue()]);
            PushVariant(push);
        }

        // Token: 0x06000255 RID: 597 RVA: 0x00010910 File Offset: 0x0000EB10
        private void Ldarga_(VariantBase vidx) // \u0006\u2001
        {
            var idx = (UshortVariant)vidx;
            var push = new VariantBaseHolder();
            push.SetValue(_variantOutputArgs[idx.GetValue()]);
            PushVariant(push);
        }

        // Token: 0x0600019F RID: 415 RVA: 0x00008C38 File Offset: 0x00006E38
        private void Endfilter_(VariantBase dummy) // \u000F\u2001\u2000
        {
		    if (((IntVariant)PopVariant()).GetValue() != 0)
		    {
                _ehStack.PushBack(new ExcHandlerFrame
                {
                    Pos = (uint)_myBufferReader.GetBuffer().GetPos(),
                    Exception = _exception
                });
			    _wasException = false;
		    }
		    ExecuteExceptionHandler();
	    }

	    // Token: 0x060001A2 RID: 418 RVA: 0x00008CFC File Offset: 0x00006EFC
	    private void Ldind_r4_(VariantBase dummy) // \u0008\u2009\u2000
        {
		    Ldind(typeof(float));
	    }

	    // Token: 0x060001A4 RID: 420 RVA: 0x00008D48 File Offset: 0x00006F48
	    private void Stsfld_(VariantBase vFieldId) // \u000F\u2008
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
		    var val = VariantFactory.Convert(PopVariant().GetValueAbstract(), fieldInfo.FieldType);
		    fieldInfo.SetValue(null, val.GetValueAbstract());
	    }

	    // Token: 0x060001A5 RID: 421 RVA: 0x00008D90 File Offset: 0x00006F90
	    private void Ldloc_3_(VariantBase dummy) // \u0003\u2001\u2001
        {
		    PushVariant(_localVariables[3].Clone());
	    }

	    // Token: 0x060001A7 RID: 423 RVA: 0x00008F68 File Offset: 0x00007168
	    private void Stelem_r8_(VariantBase dummy) // \u000E\u2006\u2000
        {
		    Stelem(typeof(double));
	    }

	    // Token: 0x060001AA RID: 426 RVA: 0x00008FBC File Offset: 0x000071BC
	    private void Ceq_(VariantBase dummy) // \u0006\u2008
        {
		    var iv = new IntVariant();
            iv.SetValue(UniCompare(PopVariant(), PopVariant(), ComparisonKind.EQ, false) ? 1 : 0);
		    PushVariant(iv);
	    }

	    // Token: 0x060001AC RID: 428 RVA: 0x00009030 File Offset: 0x00007230
	    private void Stelem_i4_(VariantBase dummy) // \u000E\u2001\u2000
        {
		    var obj = PopVariant().GetValueAbstract();
		    var idx = PopLong();
		    var array = (Array)PopVariant().GetValueAbstract();
		    var elementType = array.GetType().GetElementType();
		    checked
		    {
			    if (elementType == typeof(int))
			    {
                    ((int[])array)[(int)(IntPtr)idx] = (int)VariantFactory.Convert(obj, typeof(int)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(uint))
			    {
                    ((uint[])array)[(int)(IntPtr)idx] = (uint)VariantFactory.Convert(obj, typeof(uint)).GetValueAbstract();
				    return;
			    }
			    if (elementType.IsEnum)
			    {
                    Stelem(elementType, obj, idx, array);
				    return;
			    }
                Stelem(typeof(int), obj, idx, array);
		    }
	    }

	    // Token: 0x060001AD RID: 429 RVA: 0x00009100 File Offset: 0x00007300
	    private void Stind_i1_(VariantBase dummy) // \u0005\u2000\u2000
        {
		    Stind();
	    }

	    // Token: 0x060001AF RID: 431 RVA: 0x00009114 File Offset: 0x00007314
	    private object ResolveNativeToken(int token) // \u0002
        {
		    var num = HiByte.Extract(token);
		    object result;
		    if (num > 67108864)
		    {
			    if (num <= 167772160)
			    {
				    if (num != 100663296)
				    {
					    if (num != 167772160)
					    {
					        throw new InvalidOperationException();
					    }
                        try
					    {
						    result = _module.ModuleHandle.ResolveFieldHandle(token);
						    return result;
					    }
					    catch
					    {
						    try
						    {
							    result = _module.ModuleHandle.ResolveMethodHandle(token);
						    }
						    catch
						    {
							    throw new InvalidOperationException();
						    }
						    return result;
					    }
				    }
			    }
			    else
			    {
				    if (num == 452984832)
				    {
				        result = _module.ModuleHandle.ResolveTypeHandle(token);
				        return result;
				    }
                    if (num != 721420288)
				    {
				        throw new InvalidOperationException();
				    }
                }
			    result = _module.ModuleHandle.ResolveMethodHandle(token);
			    return result;
		    }
		    if (num != 16777216 && num != 33554432)
		    {
			    if (num != 67108864)
			    {
			        throw new InvalidOperationException();
			    }
                result = _module.ModuleHandle.ResolveFieldHandle(token);
			    return result;
		    }
		    result = _module.ModuleHandle.ResolveTypeHandle(token);
		    return result;
	    }

	    // Token: 0x060001B0 RID: 432 RVA: 0x0000923C File Offset: 0x0000743C
	    private void Ldloc_2_(VariantBase dummy) // \u0008\u2008
        {
		    PushVariant(_localVariables[2].Clone());
	    }

	    // Token: 0x060001B1 RID: 433 RVA: 0x00009258 File Offset: 0x00007458
	    private void Constrained_(VariantBase vTypeId) // \u0002\u2000\u2000
	    {
		    _currentClass = GetTypeById(((IntVariant)vTypeId).GetValue());
	    }

	    // Token: 0x060001B2 RID: 434 RVA: 0x00009280 File Offset: 0x00007480
	    private void Ldftn_(VariantBase vMethodId) // \U0003\U2008
        {
            var methodBase = FindMethodById(((IntVariant)vMethodId).GetValue());
		    var push = new MethodVariant();
		    push.SetValue(methodBase);
		    PushVariant(push);
	    }

	    // Token: 0x060001B4 RID: 436 RVA: 0x000092C8 File Offset: 0x000074C8
	    private void Ldnull_(VariantBase dummy) // \u0005\u2002\u2001
        {
		    PushVariant(new ObjectVariant());
	    }

	    private void Conv_ovf_u8_un_(VariantBase dummy)
        {
            Conv_u8(true, false);
	    }

	    // Token: 0x060001B8 RID: 440 RVA: 0x000093F0 File Offset: 0x000075F0
	    private void Stind_i2_(VariantBase dummy) // \u0003\u2000\u2000
        {
		    Stind();
	    }

	    private void Conv_ovf_u2_un_(VariantBase dummy)
        {
            Conv_u2(true, false);
	    }

        private void Conv_u2(bool ovf, bool signed)
        {
            var pop = PopVariant();
            ushort val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                        if (signed)
                            val = Convert.ToUInt16(v);
                        else
                            val = Convert.ToUInt16((ulong)Convert.ToInt64(v));
                        break;
                    }
                    val = (ushort)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && !signed)
                    {
                        val = Convert.ToUInt16((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = ovf ? Convert.ToUInt16(((IntVariant)pop).GetValue()) : (ushort)((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = checked((ushort) ((DoubleVariant) pop).GetValue());
                        break;
                    }
                    val = (ushort)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((ushort)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (ushort)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        if (signed)
                            val = checked((ushort)((LongVariant)pop).GetValue());
                        else
                            val = Convert.ToUInt16((ulong)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (ushort)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(ushort), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_U2 : OpCodes.Conv_U2) : OpCodes.Conv_Ovf_U2_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (ushort)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new UshortVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x060001AE RID: 430 RVA: 0x00009108 File Offset: 0x00007308
        private void Conv_ovf_u2_(VariantBase dummy) // \u0008\u2005\u2000
        {
            Conv_u2(true, true);
        }

        // Token: 0x06000292 RID: 658 RVA: 0x0001238C File Offset: 0x0001058C
        private void Conv_u2_(VariantBase dummy) // \u000F\u2002\u2000
        {
            Conv_u2(false, true);
        }

        private void Conv_u1(bool ovf, bool signed)
        {
            var pop = PopVariant();
            byte val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                        val = signed ? Convert.ToByte(v) : Convert.ToByte((ulong)Convert.ToInt64(v));
                        break;
                    }
                    val = (byte)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && !signed)
                    {
                        val = Convert.ToByte((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = ovf ? Convert.ToByte(((IntVariant)pop).GetValue()) : (byte)((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = signed ? checked((byte)((DoubleVariant)pop).GetValue()) : Convert.ToByte(((DoubleVariant)pop).GetValue());
                        break;
                    }
                    val = (byte)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((byte)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (byte)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        val = checked((byte)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (byte)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(byte), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_U1 : OpCodes.Conv_U1) : OpCodes.Conv_Ovf_U1_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (byte)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new ByteVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x06000182 RID: 386 RVA: 0x00007FB4 File Offset: 0x000061B4
        private void Conv_ovf_i1_(VariantBase dummy) // \u0008\u2004
        {
            Conv_i1(true, true);
        }

        // Token: 0x06000188 RID: 392 RVA: 0x00008238 File Offset: 0x00006438
        private void Conv_u4_(VariantBase dummy) // \u0008\u2003\u2001
        {
            Conv_u4(false, true);
        }

        // Token: 0x0600018A RID: 394 RVA: 0x00008270 File Offset: 0x00006470
        private void Conv_ovf_i_(VariantBase dummy) // \u0008\u2003\u2000
        {
            Conv_i(true, true);
        }

        // Token: 0x060001CF RID: 463 RVA: 0x0000A1CC File Offset: 0x000083CC
        private void Conv_i_(VariantBase dummy) // \u0008\u2005
        {
            Conv_i(false, true);
        }

        // Token: 0x060001FF RID: 511 RVA: 0x0000BFC8 File Offset: 0x0000A1C8
        private void Conv_u4(bool ovf, bool signed) // \u0005
        {
            var pop = PopVariant();
            uint val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant) pop).GetValueAbstract();
                        if (signed || Marshal.SizeOf(v) < 8)
                            val = signed ? Convert.ToUInt32(v) : (uint)Convert.ToInt32(v);
                        else
                            val = Convert.ToUInt32(Convert.ToUInt64(v));
                        break;
                    }
                    val = (uint)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && signed)
                    {
                        val = checked((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = (uint)((IntVariant)pop).GetValue(); // err fixed and unit tested by ursoft (was ushort)
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = checked((uint)((DoubleVariant)pop).GetValue());
                        break;
                    }
                    val = (uint)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((uint)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (uint)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        val = checked((uint)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (uint)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(UInt32), new []{ typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_U4 : OpCodes.Conv_U4) : OpCodes.Conv_Ovf_U4_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (uint) dyn.Invoke(null, new[] {pop.GetValueAbstract()});
                    break;
            }
            var push = new IntVariant();
            push.SetValue((int)val);
            PushVariant(push);
        }
        private void Conv_i4(bool ovf, bool signed)
        {
            var pop = PopVariant();
            int val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                        if (signed)
                            val = Convert.ToInt32(v);
                        else
                            val = Convert.ToInt32((ulong)Convert.ToInt64(v));
                        break;
                    }
                    val = (int)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && !signed)
                    {
                        val = Convert.ToInt32((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = ((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = checked((int)((DoubleVariant)pop).GetValue());
                        break;
                    }
                    val = (int)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((int)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (int)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        if(signed)
                            val = checked((int)((LongVariant)pop).GetValue());
                        else
                            val = Convert.ToInt32((ulong)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (int)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(Int32), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_I4 : OpCodes.Conv_I4) : OpCodes.Conv_Ovf_I4_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (int)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new IntVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x06000294 RID: 660 RVA: 0x00012414 File Offset: 0x00010614
        private void Conv_ovf_u4_(VariantBase dummy) // \u000F\u200A
        {
            Conv_u4(true, true);
        }

        // Token: 0x06000200 RID: 512 RVA: 0x0000C0B0 File Offset: 0x0000A2B0
        private void Conv_ovf_u1_(VariantBase dummy) // \u0008\u2007\u2000
        {
            Conv_u1(true, true);
        }

        private void Conv_ovf_i2_un_(VariantBase dummy)
        {
            Conv_i2(true, false);
        }

        private void Conv_i2(bool ovf, bool signed)
        {
            var pop = PopVariant();
            short val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                        if (signed)
                            val = Convert.ToInt16(v);
                        else
                            val = Convert.ToInt16((ulong)Convert.ToInt64(v));
                        break;
                    }
                    val = (short)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && !signed)
                    {
                        val = Convert.ToInt16((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = ovf ? Convert.ToInt16(((IntVariant)pop).GetValue()) : (short)((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = signed ? checked((short)((DoubleVariant)pop).GetValue()) :
                            (IntPtr.Size == 4 ? Convert.ToInt16(Convert.ToUInt16((long)((DoubleVariant)pop).GetValue())) :
                                               Convert.ToInt16((long)((DoubleVariant)pop).GetValue()));
                        break;
                    }
                    val = (short)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = signed ? checked((short)((FloatVariant)pop).GetValue()) :
                            (IntPtr.Size == 4 ? Convert.ToInt16(Convert.ToUInt16((long)((FloatVariant)pop).GetValue())) :
                                               Convert.ToInt16((long)((FloatVariant)pop).GetValue()));
                        break;
                    }
                    val = (short)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        if (signed)
                            val = checked((short)((LongVariant)pop).GetValue());
                        else
                            val = Convert.ToInt16((ulong)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (short)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(short), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_I2 : OpCodes.Conv_I2) : OpCodes.Conv_Ovf_I2_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (short)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new ShortVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x060001FE RID: 510 RVA: 0x0000BFBC File Offset: 0x0000A1BC
        private void Conv_i2_(VariantBase dummy) // \u0006\u2001\u2000
        {
            Conv_i2(false, true);
        }

        // Token: 0x060002A8 RID: 680 RVA: 0x00012D2C File Offset: 0x00010F2C
        private void Conv_ovf_i4_(VariantBase dummy) // \u000F\u2000\u2000
        {
            Conv_i4(true, true);
        }

        private void Conv_i8(bool ovf, bool signed)
        {
            var pop = PopVariant();
            long val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    if (!signed && ovf && Marshal.SizeOf(v) > 4)
                        val = checked((long)(ulong)Convert.ToInt64(v));
                    else
                        val = (signed || Marshal.SizeOf(v) > 4) ? Convert.ToInt64(v) :
                          (uint)(ulong)Convert.ToInt64(v);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv = ((IntVariant)pop).GetValue();
                    if (signed)
                        val = iv;
                    else
                        val = (long)(uint)iv;
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = checked((long)((DoubleVariant)pop).GetValue());
                        break;
                    }
                    val = (long)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((long)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (long)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        if (signed)
                            val = ((LongVariant)pop).GetValue();
                        else
                            val = Convert.ToInt64((ulong)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = ((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(long), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_I8 : OpCodes.Conv_I8) : OpCodes.Conv_Ovf_I8_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (long)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new LongVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x06000219 RID: 537 RVA: 0x0000C824 File Offset: 0x0000AA24
        private void Conv_ovf_i8_(VariantBase dummy) // \u0008\u2002\u2000
        {
            Conv_i8(true, true);
        }

        private void Conv_i1(bool ovf, bool signed)
        {
            var pop = PopVariant();
            sbyte val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    if (ovf)
                    {
                        var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                        val = signed ? Convert.ToSByte(v) : Convert.ToSByte((ulong)Convert.ToInt64(v));
                        break;
                    }
                    val = (sbyte)VariantBase.SignedLongFromEnum((EnumVariant)pop);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    if (ovf && !signed)
                    {
                        val = Convert.ToSByte((uint)((IntVariant)pop).GetValue());
                        break;
                    }
                    val = ovf ? Convert.ToSByte(((IntVariant)pop).GetValue()) : (sbyte)((IntVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = signed ? checked((sbyte)((DoubleVariant)pop).GetValue()) : 
                            (IntPtr.Size == 4 ? Convert.ToSByte(Convert.ToByte(((DoubleVariant)pop).GetValue())) :
                                               Convert.ToSByte((long)((DoubleVariant)pop).GetValue()));
                        break;
                    }
                    val = (sbyte)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = signed ? checked((sbyte)((FloatVariant)pop).GetValue()) :
                            (IntPtr.Size == 4 ? Convert.ToSByte(Convert.ToByte(((FloatVariant)pop).GetValue())) :
                                               Convert.ToSByte((long)((FloatVariant)pop).GetValue()));
                        break;
                    }
                    val = (sbyte)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf)
                    {
                        if (signed)
                            val = checked((sbyte)((LongVariant)pop).GetValue());
                        else
                            val = Convert.ToSByte((ulong)((LongVariant)pop).GetValue());
                        break;
                    }
                    val = (sbyte)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(SByte), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_I1 : OpCodes.Conv_I1) : OpCodes.Conv_Ovf_I1_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (sbyte)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new SbyteVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x060002A6 RID: 678 RVA: 0x00012CD0 File Offset: 0x00010ED0
        private void Conv_i1_(VariantBase dummy) // \u000E\u2006
        {
            Conv_i1(false, true);
        }

        // Token: 0x06000285 RID: 645 RVA: 0x00011EF0 File Offset: 0x000100F0
        private void Conv_ovf_i2_(VariantBase dummy) // \u0002\u2000\u2001
        {
            Conv_i2(true, true);
        }

        // Token: 0x0600018D RID: 397 RVA: 0x000083F8 File Offset: 0x000065F8
        private void Conv_u_(VariantBase dummy) // \u000F\u2003\u2001
        {
            Conv_u(false, true);
        }

        // Token: 0x060001EF RID: 495 RVA: 0x0000B658 File Offset: 0x00009858
        private void Conv_ovf_u_(VariantBase dummy) // \u0002\u2000
        {
            Conv_u(true, true);
        }

        // Token: 0x06000215 RID: 533 RVA: 0x0000C784 File Offset: 0x0000A984
        private void Conv_u1_(VariantBase dummy) // \u0008\u2002
        {
            Conv_u1(false, true);
        }

        // Token: 0x06000211 RID: 529 RVA: 0x0000C690 File Offset: 0x0000A890
        private void Conv_ovf_i_un_(VariantBase dummy) // \u000F
        {
            Conv_i(true, false);
        }

        // Token: 0x06000229 RID: 553 RVA: 0x0000F56C File Offset: 0x0000D76C
        private void Conv_i4_(VariantBase dummy) // \u0003\u2005\u2000
        {
            Conv_i4(false, true);
        }

        private void Conv_ovf_i8_un_(VariantBase dummy)
        {
            Conv_i8(true, false);
        }

        // Token: 0x06000253 RID: 595 RVA: 0x00010790 File Offset: 0x0000E990
        private void Conv_ovf_u4_un_(VariantBase dummy) // \u0002\u2009\u2000
        {
            Conv_u4(true, false);
        }

        // Token: 0x06000284 RID: 644 RVA: 0x00011EE4 File Offset: 0x000100E4
        private void Conv_i8_(VariantBase dummy) // \u0003\u2006\u2000
        {
            Conv_i8(false, true);
        }

        // Token: 0x0600028A RID: 650 RVA: 0x000121B8 File Offset: 0x000103B8
        private void Conv_ovf_u_un_(VariantBase dummy) // \u000F\u200B
        {
            Conv_u(true, false);
        }

        private unsafe void Conv_i(bool ovf, bool signed)
        {
            var pop = PopVariant();
            var push = new IntPtrVariant();
            long val;
            var tc = pop.GetTypeCode();
            if (tc == VariantBase.Vtc.Tc21Double || tc == VariantBase.Vtc.Tc8Float)
                signed = true;
            long maxVal = long.MaxValue, minVal = signed ? long.MinValue : 0;
            if (IntPtr.Size == 4)
            {
                maxVal = signed ? Int32.MaxValue : UInt32.MaxValue;
                minVal = signed ? Int32.MinValue : 0;
            }
            switch (tc)
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    if (IntPtr.Size == 4)
                    {
                        if (ovf) val = Convert.ToInt32(v);
                        else val = (int)Convert.ToInt64(v);
                    }
                    else
                    {
                        val = (signed || Marshal.SizeOf(v) > 4 || !ovf) ? Convert.ToInt64(v) : 
                            (uint)(ulong)Convert.ToInt64(v);
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv = ((IntVariant) pop).GetValue();
                    if (IntPtr.Size == 4 || signed)
                        val = iv;
                    else
                        val = (long)(uint)iv;
                    break;
                case VariantBase.Vtc.Tc21Double:
                    {
                        double dv = ((DoubleVariant)pop).GetValue();
                        if (dv <= maxVal && dv >= minVal)
                            val = (long)dv;
                        else
                        {
                            if (ovf) throw new OverflowException();
                            val = (IntPtr.Size == 4) ? Int32.MinValue : Int64.MinValue; // не мусор ли?
                        }
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    {
                        double dv = (double) ((FloatVariant) pop).GetValue();
                        if (dv <= maxVal && dv >= minVal)
                            val = (long) dv;
                        else
                        {
                            if (ovf) throw new OverflowException();
                            val = (IntPtr.Size == 4) ? Int32.MinValue : Int64.MinValue; // не мусор ли?
                        }
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    val = ((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_I : OpCodes.Conv_I) : OpCodes.Conv_Ovf_I_Un);
                    gen.Emit(OpCodes.Ret);
                    push.SetValue(((IntPtr)dyn.Invoke(null, new[] { pop.GetValueAbstract() })));
                    PushVariant(push);
                    return;
            }
            if ((ovf == false) || (val <= maxVal && val >= minVal))
            {
                push.SetValue(new IntPtr((void*)val));
                PushVariant(push);
            } else throw new OverflowException();
        }
        private unsafe void Conv_u(bool ovf, bool signed)
        {
            var pop = PopVariant();
            var push = new UIntPtrVariant();
            ulong val, maxVal = (IntPtr.Size == 4) ? UInt32.MaxValue : UInt64.MaxValue;
            var tc = pop.GetTypeCode();
            switch (tc)
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    if (IntPtr.Size == 4)
                    {
                        if (ovf) val = signed ?
                                Convert.ToUInt64(v) :
                                (Marshal.SizeOf(v) > 4) ? (ulong)Convert.ToInt64(v) : (uint)Convert.ToInt32(v);
                        else val = (uint)Convert.ToInt64(v);
                    }
                    else
                    {
                        val = (Marshal.SizeOf(v) > 4) ? 
                            ((ovf && signed) ? Convert.ToUInt64(Convert.ToInt64(v)) : (ulong)Convert.ToInt64(v)) :
                            ((ovf && signed) ? Convert.ToUInt32(Convert.ToInt32(v)) : (uint)Convert.ToInt32(v));
                    }
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv = ((IntVariant)pop).GetValue();
                    if (ovf && signed && iv < 0) throw new OverflowException();
                    val = (uint)iv;
                    break;
                case VariantBase.Vtc.Tc21Double:
                    {
                        double dv = ((DoubleVariant)pop).GetValue();
                        if (ovf && signed && dv < 0) throw new OverflowException();
                        if (dv <= maxVal && (signed || dv >= 0))
                            val = (ulong)dv;
                        else
                        {
                            if (ovf) throw new OverflowException();
                            val = 0; // мусор, индульгируем
                        }
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    {
                        double dv = (double)((FloatVariant)pop).GetValue();
                        if (ovf && signed && dv < 0) throw new OverflowException();
                        if (dv <= maxVal && (signed || dv >= 0))
                            val = (ulong)dv;
                        else
                        {
                            if (ovf) throw new OverflowException();
                            val = 0; // мусор, индульгируем
                        }
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    long lv = ((LongVariant)pop).GetValue();
                    if (ovf && signed && lv < 0) throw new OverflowException();
                    val = (ulong) lv;
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(UIntPtr), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_U : OpCodes.Conv_U) : OpCodes.Conv_Ovf_U_Un);
                    gen.Emit(OpCodes.Ret);
                    push.SetValue(((UIntPtr)dyn.Invoke(null, new[] { pop.GetValueAbstract() })));
                    PushVariant(push);
                    return;
            }
            if ((ovf == false) || (val <= maxVal))
            {
                push.SetValue(new UIntPtr((void*)val));
                PushVariant(push);
            }
            else throw new OverflowException();
        }

        private void Conv_u8(bool ovf, bool signed)
        {
            var pop = PopVariant();
            ulong val;
            switch (pop.GetTypeCode())
            {
                case VariantBase.Vtc.Tc5Enum:
                    var v = VariantBase.SignedVariantFromEnum((EnumVariant)pop).GetValueAbstract();
                    if(ovf && signed)
                        val = Convert.ToUInt64(v);
                    else
                        if (Marshal.SizeOf(v) > 4)
                            val = (ulong)Convert.ToInt64(v);
                        else
                            val = (uint)(ulong)Convert.ToInt64(v);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv = ((IntVariant)pop).GetValue();
                    val = ovf ? (signed ? checked((uint)iv) : (uint)iv) : (uint)iv;
                    break;
                case VariantBase.Vtc.Tc21Double:
                    if (ovf)
                    {
                        val = checked((ulong)((DoubleVariant)pop).GetValue());
                        break;
                    }
                    val = (ulong)((DoubleVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (ovf)
                    {
                        val = checked((ulong)((FloatVariant)pop).GetValue());
                        break;
                    }
                    val = (ulong)((FloatVariant)pop).GetValue();
                    break;
                case VariantBase.Vtc.Tc24Long:
                    if (ovf && signed)
                        val = Convert.ToUInt64(((LongVariant)pop).GetValue());
                    else
                        val = (ulong)((LongVariant)pop).GetValue();
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(ulong), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(signed ? (ovf ? OpCodes.Conv_Ovf_U8 : OpCodes.Conv_U8) : OpCodes.Conv_Ovf_U8_Un);
                    gen.Emit(OpCodes.Ret);
                    val = (ulong)dyn.Invoke(null, new[] { pop.GetValueAbstract() });
                    break;
            }
            var push = new UlongVariant();
            push.SetValue(val);
            PushVariant(push);
        }

        // Token: 0x06000204 RID: 516 RVA: 0x0000C19C File Offset: 0x0000A39C
        private void Conv_u8_(VariantBase dummy) // \u0003\u2005
        {
            Conv_u8(false, true);
        }

        // Token: 0x0600020E RID: 526 RVA: 0x0000C58C File Offset: 0x0000A78C
        private void Conv_ovf_u8_(VariantBase dummy) // \u0008\u2008\u2000
        {
            Conv_u8(true, true);
        }

        // Token: 0x06000208 RID: 520 RVA: 0x0000C43C File Offset: 0x0000A63C
        private void Conv_ovf_i1_un_(VariantBase dummy) // \u0008
        {
            Conv_i1(true, false);
        }

        private void Conv_ovf_u1_un_(VariantBase dummy)
        {
            Conv_u1(true, false);
        }

        // Token: 0x060001BB RID: 443 RVA: 0x000094E0 File Offset: 0x000076E0
        private void _u0006u2003u2001(VariantBase dummy) // \u0006\u2003\u2001
        {
	    }

	    // Token: 0x060001BC RID: 444 RVA: 0x000094E4 File Offset: 0x000076E4
	    private VariantBase[] CreateLocalVariables() // u0002
	    {
		    var array = _methodHeader.LocalVarTypes;
		    var num = array.Length;
		    var ret = new VariantBase[num];
		    for (var i = 0; i < num; i++)
		    {
			    ret[i] = VariantFactory.Convert(null, GetTypeById(array[i].TypeId));
		    }
		    return ret;
	    }

	    // Token: 0x060001C0 RID: 448 RVA: 0x0000958C File Offset: 0x0000778C
	    private MethodBase FindGenericMethod(VmMethodTokenInfo what) // \u0002
        {
		    var type = GetTypeById(what.Class.MetadataToken);
		    var bindingAttr = BF(what.IsStatic());
		    var arg_32_0 = type.GetMember(what.Name, bindingAttr);
	        var array = arg_32_0;
	        var methodInfo = (from MethodInfo methodInfo2 in array
	            where methodInfo2.IsGenericMethodDefinition
	            let parameters = methodInfo2.GetParameters()
	            where
	                parameters.Length == what.Parameters.Length &&
	                methodInfo2.GetGenericArguments().Length == what.GenericArguments.Length &&
	                AreCompatible(methodInfo2.ReturnType, what.ReturnType)
	            where !parameters.Where((t1, j) => !AreCompatible(t1.ParameterType, what.Parameters[j])).Any()
	            select methodInfo2).FirstOrDefault();
	        if (methodInfo == null)
		    {
			    throw new Exception(string.Format(StringDecryptor.GetString(-1550347247) /* Cannot bind method: {0}.{1} */, type.Name, what.Name));
		    }
		    var array2 = new Type[what.GenericArguments.Length];
		    for (var k = 0; k < array2.Length; k++)
		    {
			    array2[k] = GetTypeById(what.GenericArguments[k].MetadataToken);
		    }
		    return methodInfo.MakeGenericMethod(array2);
	    }

	    // Token: 0x060001C2 RID: 450 RVA: 0x000097A0 File Offset: 0x000079A0
	    private bool InvokeFilter(MethodBase mb, object obj, ref object result, object[] args) // \u0002
        {
		    var declaringType = mb.DeclaringType;
		    if (declaringType == null)
		    {
			    return false;
		    }
		    if (SimpleTypeHelper.IsNullableGeneric(declaringType))
		    {
			    if (string.Equals(mb.Name, StringDecryptor.GetString(-1550345611) /* get_HasValue */, StringComparison.Ordinal))
			    {
				    result = obj != null;
			    }
			    else if (string.Equals(mb.Name, StringDecryptor.GetString(-1550345722) /* get_Value */, StringComparison.Ordinal))
			    {
				    if (obj == null)
				    {
					    //return ((bool?)null).Value;
                        throw new InvalidOperationException();
				    }
				    result = obj;
			    }
			    else if (mb.Name.Equals(StringDecryptor.GetString(-1550345706) /* GetValueOrDefault */, StringComparison.Ordinal))
			    {
				    if (obj == null)
				    {
					    /*u0005 =*/ Activator.CreateInstance(Nullable.GetUnderlyingType(mb.DeclaringType));
				    }
				    result = obj;
			    }
			    else
			    {
				    if (obj != null || mb.IsStatic)
				    {
					    return false;
				    }
				    result = null;
			    }
			    return true;
		    }
		    if (declaringType == SimpleTypeHelper.TypedReferenceType)
		    {
			    var name = mb.Name;
			    var i = args.Length;
			    if (i != 1)
			    {
				    if (i == 2)
				    {
					    if (name == StringDecryptor.GetString(-1550345495) /* SetTypedReference */)
					    {
						    TypedReference.SetTypedReference((TypedReference)args[0], args[1]);
						    return true;
					    }
				    }
			    }
			    else
			    {
				    if (name == StringDecryptor.GetString(-1550345682) /* GetTargetType */)
				    {
					    result = TypedReference.GetTargetType((TypedReference)args[0]);
					    return true;
				    }
				    if (name == StringDecryptor.GetString(-1550345534) /* TargetTypeToken */)
				    {
					    result = TypedReference.TargetTypeToken((TypedReference)args[0]);
					    return true;
				    }
				    if (name == StringDecryptor.GetString(-1550345512) /* ToObject */)
				    {
					    result = TypedReference.ToObject((TypedReference)args[0]);
					    return true;
				    }
			    }
		    }
		    else if (declaringType == AssemblyType)
		    {
			    if (_callees != null && mb.Name == StringDecryptor.GetString(-1550345599) /* GetCallingAssembly */)
			    {
			        var array = _callees;
			        foreach (var t in array)
			        {
			            var assembly = t as Assembly;
			            if (assembly != null)
			            {
			                result = assembly;
			                return true;
			            }
			        }
			    }
		    }
		    else if (declaringType == MethodBaseType)
		    {
			    if (mb.Name == StringDecryptor.GetString(-1550345576) /* GetCurrentMethod */)
			    {
				    if (_callees != null)
				    {
				        var array = _callees;
				        foreach (var t in array)
				        {
				            var methodBase = t as MethodBase;
				            if (methodBase != null)
				            {
				                result = methodBase;
				                return true;
				            }
				        }
				    }
				    result = MethodBase.GetCurrentMethod();
				    return true;
			    }
		    }
		    else if (declaringType.IsArray && declaringType.GetArrayRank() >= 2)
		    {
			    return RefToMdArrayItem(mb, obj, ref result, args);
		    }
		    return false;
	    }

	    // Token: 0x060001C3 RID: 451 RVA: 0x000099F8 File Offset: 0x00007BF8
	    private void Ldloca_s_(VariantBase vLocIdx) // \u0005\u2001\u2001
        {
            var push = new LocalsIdxHolderVariant();
		    push.SetValue(((ByteVariant)vLocIdx).GetValue());
		    PushVariant(push);
	    }

	    // Token: 0x060001C4 RID: 452 RVA: 0x00009A24 File Offset: 0x00007C24
	    private void Ldind_i_(VariantBase dummy) // \u000E\u2004\u2000
        {
		    Ldind(IntPtrType);
	    }

	    // Token: 0x060001C6 RID: 454 RVA: 0x00009D14 File Offset: 0x00007F14
	    private void Stloc_(VariantBase vidx) // \u0008\u2006
        {
		    var idx = (UshortVariant)vidx;
            PopToLocal(idx.GetValue());
	    }

	    // Token: 0x060001C7 RID: 455 RVA: 0x00009D34 File Offset: 0x00007F34
	    private void Stfld_(VariantBase vFieldId) // \u0005\u200B
        {
            var fieldInfo = ResolveField(((IntVariant)vFieldId).GetValue());
		    var val = PopVariant();
		    var objRef = PopVariant();
            var obj = objRef.IsAddr() ? FetchByAddr(objRef).GetValueAbstract() : objRef.GetValueAbstract();
		    if (obj == null)
		    {
			    throw new NullReferenceException();
		    }
            fieldInfo.SetValue(obj, VariantFactory.Convert(val.GetValueAbstract(), fieldInfo.FieldType).GetValueAbstract());
		    if (objRef.IsAddr() && /*obj != null && */ obj.GetType().IsValueType)
		    {
                AssignByReference(objRef, VariantFactory.Convert(obj, null));
		    }
	    }

	    // Token: 0x060001C8 RID: 456 RVA: 0x00009DD0 File Offset: 0x00007FD0
	    /*private void u000Fu2004(VariantBase dummy) // \u000F\u2004
        {
	    }*/

	    // Token: 0x060001CB RID: 459 RVA: 0x00009E7C File Offset: 0x0000807C
	    private void Ldloc_s_(VariantBase vidx) // \u0002
        {
		    var idx = (ByteVariant)vidx;
		    PushVariant(_localVariables[idx.GetValue()].Clone());
	    }

	    // Token: 0x060001D1 RID: 465 RVA: 0x0000A328 File Offset: 0x00008528
	    private void Stelem_ref_(VariantBase dummy) // \u0008\u200A
        {
		    Stelem(SimpleTypeHelper.ObjectType);
	    }

	    // Token: 0x060001D3 RID: 467 RVA: 0x0000A34C File Offset: 0x0000854C
	    private void Ldelem_u2_(VariantBase dummy) // \u000E\u2003\u2001
        {
		    Ldelem(typeof(ushort));
	    }

	    // Token: 0x060001D5 RID: 469 RVA: 0x0000A620 File Offset: 0x00008820
	    private void Stind_i4_(VariantBase dummy) // \u0006\u2003\u2000
        {
		    Stind();
	    }

	    // Token: 0x060001D6 RID: 470 RVA: 0x0000A628 File Offset: 0x00008828
	    private void Stind_i8_(VariantBase dummy) // \u0003\u2002\u2000
        {
		    Stind();
	    }

	    // Token: 0x060001DC RID: 476 RVA: 0x0000AAD8 File Offset: 0x00008CD8
	    private void Stelem_i8_(VariantBase dummy) // \u0002\u200B\u2000
        {
		    var obj = PopVariant().GetValueAbstract();
		    var idx = PopLong();
		    var array = (Array)PopVariant().GetValueAbstract();
		    var elementType = array.GetType().GetElementType();
		    checked
		    {
			    if (elementType == typeof(long))
			    {
                    ((long[])array)[(int)(IntPtr)idx] = (long)VariantFactory.Convert(obj, typeof(long)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(ulong))
			    {
                    ((ulong[])array)[(int)(IntPtr)idx] = (ulong)VariantFactory.Convert(obj, typeof(ulong)).GetValueAbstract();
				    return;
			    }
			    if (elementType.IsEnum)
			    {
                    Stelem(elementType, obj, idx, array);
				    return;
			    }
                Stelem(typeof(long), obj, idx, array);
		    }
	    }

	    // Token: 0x060001DD RID: 477 RVA: 0x0000ABA8 File Offset: 0x00008DA8
	    private void Stelem_i_(VariantBase dummy) // \u000F\u2006\u2000
        {
		    Stelem(IntPtrType);
	    }

	    // Token: 0x060001DE RID: 478 RVA: 0x0000ABB8 File Offset: 0x00008DB8
	    private void Ldelem_i8_(VariantBase dummy) // \u0006\u2000
        {
		    Ldelem(typeof(long));
	    }

	    // Token: 0x060001E0 RID: 480 RVA: 0x0000ABD0 File Offset: 0x00008DD0
	    private void Ldc_i4_(VariantBase val) // \u0006\u2000\u2000
	    {
		    PushVariant(val);
	    }

	    // Token: 0x060001E2 RID: 482 RVA: 0x0000AC08 File Offset: 0x00008E08
	    private void Ldarg_1_(VariantBase dummy) // \u0002\u2003\u2001
	    {
		    PushVariant(_variantOutputArgs[1].Clone());
	    }

	    // Token: 0x060001E3 RID: 483 RVA: 0x0000AC24 File Offset: 0x00008E24
	    private void Ret_(VariantBase dummy) // \u000E\u2005\u2000
        {
		    Ret();
	    }

	    // Token: 0x060001E8 RID: 488 RVA: 0x0000B10C File Offset: 0x0000930C
	    private void Stelem(Type arrType, object val, long idx, Array array) // \u0002
        {
		    array.SetValue(VariantFactory.Convert(val, arrType).GetValueAbstract(), idx);
	    }

	    // Token: 0x060001E9 RID: 489 RVA: 0x0000B130 File Offset: 0x00009330
	    [DebuggerNonUserCode]
	    private MethodBase FindMethodById(int methodId, UniversalTokenInfo methodToken) // \u0002
        {
            MethodBase result = null;
		    lock (AllMetadataById)
		    {
			    //var flag = true;
			    object obj;
			    if (/*flag &&*/ AllMetadataById.TryGetValue(methodId, out obj))
			    {
				    result = (MethodBase)obj;
			    }
			    else if (methodToken.IsVm == 0)
			    {
				    var methodBase = _module.ResolveMethod(methodToken.MetadataToken);
				    //if (flag)
				    {
					    AllMetadataById.Add(methodId, methodBase);
				    }
				    result = methodBase;
			    }
			    else
			    {
				    var mti = (VmMethodTokenInfo)methodToken.VmToken;
				    if (mti.IsGeneric())
				    {
					    result = FindGenericMethod(mti);
				    }
				    else
				    {
					    var clsType = GetTypeById(mti.Class.MetadataToken);
					    var retType = GetTypeById(mti.ReturnType.MetadataToken);
					    var paramArray = new Type[mti.Parameters.Length];
					    for (var i = 0; i < paramArray.Length; i++)
					    {
						    paramArray[i] = GetTypeById(mti.Parameters[i].MetadataToken);
					    }
					    /*if (type.IsGenericType)
					    {
						    flag = false;
					    }*/
					    if (mti.Name == StringDecryptor.GetString(-1550347259) /* .ctor */)
					    {
						    var constructor = clsType.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null, CallingConventions.Any, paramArray, null);
						    if (constructor == null)
						    {
							    throw new Exception();
						    }
						    if (!clsType.IsGenericType)
						    {
							    AllMetadataById.Add(methodId, constructor);
						    }
						    result = constructor;
					    }
					    else
					    {
						    var bindingAttr = BF(mti.IsStatic());
						    try
						    {
							    result = clsType.GetMethod(mti.Name, bindingAttr, null, CallingConventions.Any, paramArray, null);
						    }
						    catch (AmbiguousMatchException)
						    {
						        var methods = clsType.GetMethods(bindingAttr);
						        foreach (var methodInfo in methods)
						        {
						            if (methodInfo.Name == mti.Name && methodInfo.ReturnType == retType)
						            {
						                var parameters = methodInfo.GetParameters();
						                if (parameters.Length == paramArray.Length)
						                {
                                            if (!(bool)paramArray.Where((t, k) => parameters[k].ParameterType != t).Any())
						                    {
						                        result = methodInfo;
						                        break;
						                    }
						                }
						            }
						        }
						    }
						    if (result == null)
						    {
							    throw new Exception(string.Format(StringDecryptor.GetString(-1550347247) /* Cannot bind method: {0}.{1} */, clsType.Name, mti.Name));
						    }
						    if (!clsType.IsGenericType)
						    {
							    AllMetadataById.Add(methodId, result);
						    }
					    }
				    }
			    }
		    }
		    return result;
	    }

	    // Token: 0x060001EA RID: 490 RVA: 0x0000B3B8 File Offset: 0x000095B8
	    private void Stloc_s_(VariantBase vidx) // \u000E\u2004
	    {
		    var idx = (ByteVariant)vidx;
            PopToLocal(idx.GetValue());
	    }

	    // Token: 0x060001EB RID: 491 RVA: 0x0000B3D8 File Offset: 0x000095D8
	    private void LockIfInterlocked(ref BoolHolder wasLocked, MethodBase mb, bool dummy) // \u0002
        {
		    if (mb.DeclaringType == typeof(Interlocked) && mb.IsStatic)
		    {
			    var name = mb.Name;
			    if (name == StringDecryptor.GetString(-1550347213) /* Add */ || name == StringDecryptor.GetString(-1550347203) /* CompareExchange */ || name == StringDecryptor.GetString(-1550347053) /* Decrement */ || name == StringDecryptor.GetString(-1550347037) /* Exchange */ || name == StringDecryptor.GetString(-1550347024) /* Increment */ || name == StringDecryptor.GetString(-1550347136) /* Read*/)
			    {
			        Monitor.Enter(InterlockedLock);
			        wasLocked.Val = true;
			    }
            }
	    }

	    // Token: 0x060001EC RID: 492 RVA: 0x0000B4A0 File Offset: 0x000096A0
	    private void Box_(VariantBase vTypeId) // \u0003\u2009\u2000
	    {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var push = VariantFactory.Convert(PopVariant().GetValueAbstract(), type);
		    push.SetVariantType(type);
		    PushVariant(push);
	    }

	    // Token: 0x060001F1 RID: 497 RVA: 0x0000B690 File Offset: 0x00009890
	    private void Sizeof_(VariantBase vTypeId) // \u000E\u2001\u2001
	    {
		    var t = GetTypeById(((IntVariant)vTypeId).GetValue());
		    var iv = new IntVariant();
		    iv.SetValue(Marshal.SizeOf(t));
		    PushVariant(iv);
	    }

	    // Token: 0x060001F3 RID: 499 RVA: 0x0000B758 File Offset: 0x00009958
	    private void Ldelem_i_(VariantBase dummy) // \u000F\u2000
	    {
		    Ldelem(IntPtrType);
	    }

	    // Token: 0x060001F4 RID: 500 RVA: 0x0000B768 File Offset: 0x00009968
	    private void InternalInvoke() // \u0002
        {
		    try
		    {
			    LoopUntilRet();
		    }
		    catch (Exception ex)
		    {
			    OnException(ex, 0u);
			    LoopUntilRet();
		    }
	    }

	    // Token: 0x060001F5 RID: 501 RVA: 0x0000B7A0 File Offset: 0x000099A0
	    private MethodBase GenerateDynamicCall(MethodBase mb, bool mayVirtual) // \u0002
        {
		    MethodBase result;
		    lock (DynamicMethods)
		    {
			    DynamicMethod dynamicMethod;
			    if (DynamicMethods.TryGetValue(mb, out dynamicMethod))
			    {
				    result = dynamicMethod;
			    }
			    else
			    {
                    var methodInfo = mb as MethodInfo;
			        var returnType = methodInfo?.ReturnType ?? VoidType;
				    var parameters = mb.GetParameters();
				    Type[] array;
				    if (mb.IsStatic)
				    {
					    array = new Type[parameters.Length];
					    for (var i = 0; i < parameters.Length; i++)
					    {
						    array[i] = parameters[i].ParameterType;
					    }
				    }
				    else
				    {
					    array = new Type[parameters.Length + 1];
					    var type = mb.DeclaringType;
					    if (type.IsValueType)
					    {
						    type = type.MakeByRefType();
						    mayVirtual = false;
					    }
					    array[0] = type;
					    for (var j = 0; j < parameters.Length; j++)
					    {
						    array[j + 1] = parameters[j].ParameterType;
					    }
				    }
				    /*if (_alwaysFalse)
				    {
					    dynamicMethod = new DynamicMethod(string.Empty, returnType, array, true);
				    }
				    if (dynamicMethod == null)*/
				    {
					    dynamicMethod = new DynamicMethod(string.Empty, returnType, array, GetTypeById(_methodHeader.ClassId), true);
				    }
				    var iLGenerator = dynamicMethod.GetILGenerator();
				    for (var k = 0; k < array.Length; k++)
				    {
					    iLGenerator.Emit(OpCodes.Ldarg, k);
				    }
				    var constructorInfo = mb as ConstructorInfo;
				    if (constructorInfo != null)
				    {
					    iLGenerator.Emit(mayVirtual ? OpCodes.Callvirt : OpCodes.Call, constructorInfo);
				    }
				    else
				    {
					    iLGenerator.Emit(mayVirtual ? OpCodes.Callvirt : OpCodes.Call, (MethodInfo)mb);
				    }
				    iLGenerator.Emit(OpCodes.Ret);
				    DynamicMethods.Add(mb, dynamicMethod);
				    result = dynamicMethod;
			    }
		    }
		    return result;
	    }

	    // Token: 0x060001F7 RID: 503 RVA: 0x0000B9EC File Offset: 0x00009BEC
	    private void Ldelem_i4_(VariantBase dummy) // \u000E\u2007
	    {
		    Ldelem(typeof(int));
	    }

	    // Token: 0x060001FD RID: 509 RVA: 0x0000BD88 File Offset: 0x00009F88
	    private void Invoke(int pos, Type[] methodGenericArgs, Type[] classGenericArgs, bool mayVirtual) // \u0002
        {
		    _srcVirtualizedStreamReader.BaseStream.Seek(pos, SeekOrigin.Begin);
		    DoNothing(_srcVirtualizedStreamReader);
		    var u0006 = ReadMethodHeader(_srcVirtualizedStreamReader);
		    var num = u0006.ArgsTypeToOutput.Length;
		    var array = new object[num];
		    var array2 = new VariantBase[num];
		    if (_currentClass != null & mayVirtual)
		    {
			    var num2 = u0006.IsStatic() ? 0 : 1;
			    var array3 = new Type[num - num2];
			    for (var i = num - 1; i >= num2; i--)
			    {
				    array3[i] = GetTypeById(u0006.ArgsTypeToOutput[i].TypeId);
			    }
			    var method = _currentClass.GetMethod(u0006.Name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.InvokeMethod | BindingFlags.GetProperty | BindingFlags.SetProperty, null, array3, null);
			    _currentClass = null;
			    if (method != null)
			    {
                    GenerateDynamicCall(method, true);
				    return;
			    }
		    }
		    for (var j = num - 1; j >= 0; j--)
		    {
			    var u000F = PopVariant();
			    array2[j] = u000F;
			    if (u000F.IsAddr())
			    {
				    u000F = FetchByAddr(u000F);
			    }
			    if (u000F.GetVariantType() != null)
			    {
				    u000F = VariantFactory.Convert(null, u000F.GetVariantType()).CopyFrom(u000F);
			    }
			    var u000F2 = VariantFactory.Convert(null, GetTypeById(u0006.ArgsTypeToOutput[j].TypeId)).CopyFrom(u000F);
			    array[j] = u000F2.GetValueAbstract();
			    if (j == 0 & mayVirtual && !u0006.IsStatic() && array[j] == null)
			    {
				    throw new NullReferenceException();
			    }
		    }
		    var executor = new VmExecutor(_instrCodesDb);
		    var callees = new object[]
		    {
			    _module.Assembly
		    };
		    object obj;
		    try
		    {
			    obj = executor.Invoke(_srcVirtualizedStream, pos, array, methodGenericArgs, classGenericArgs, callees);
		    }
		    finally
		    {
			    for (var k = 0; k < array2.Length; k++)
			    {
				    var u000F3 = array2[k];
				    if (u000F3.IsAddr())
				    {
					    var obj2 = array[k];
				        AssignByReference(u000F3, VariantFactory.Convert(obj2, null));
				    }
			    }
		    }
		    var type = executor.GetTypeById(executor._methodHeader.ReturnTypeId);
		    if (type != VoidType)
		    {
			    PushVariant(VariantFactory.Convert(obj, type));
		    }
	    }

	    // Token: 0x06000202 RID: 514 RVA: 0x0000C150 File Offset: 0x0000A350
	    public static object CreateValueTypeInstance(Type t) // \u0002
        {
		    if (t.IsValueType)
		    {
			    return Activator.CreateInstance(t);
		    }
		    return null;
	    }

	    // Token: 0x0600020B RID: 523 RVA: 0x0000C51C File Offset: 0x0000A71C
	    private void Ldc_r4_(VariantBase val) // \u0003\u200B\u2000
	    {
		    PushVariant(val);
	    }

	    // Token: 0x0600020D RID: 525 RVA: 0x0000C550 File Offset: 0x0000A750
	    private void Stelem(Type t) // \u0003
        {
		    var obj = PopVariant().GetValueAbstract();
		    var idx = PopLong();
		    var array = (Array)PopVariant().GetValueAbstract();
            Stelem(t, obj, idx, array);
	    }

	    // Token: 0x06000210 RID: 528 RVA: 0x0000C5C4 File Offset: 0x0000A7C4
	    private void Ldvirtftn_(VariantBase vMethodId) // \u0003\u200A
	    {
            var methodBase = FindMethodById(((IntVariant)vMethodId).GetValue());
		    var declaringType = methodBase.DeclaringType;
		    var type = PopVariant().GetValueAbstract().GetType();
		    var parameters = methodBase.GetParameters();
		    var paramTypes = new Type[parameters.Length];
		    for (var i = 0; i < parameters.Length; i++)
		    {
			    paramTypes[i] = parameters[i].ParameterType;
		    }
		    while (type != null && type != declaringType)
		    {
			    var method = type.GetMethod(methodBase.Name, BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.GetProperty | BindingFlags.SetProperty | BindingFlags.ExactBinding, null, CallingConventions.Any, paramTypes, null);
			    if (method != null && method.GetBaseDefinition() == methodBase)
			    {
				    methodBase = method;
				    break;
			    }
			    type = type.BaseType;
		    }
		    var push = new MethodVariant();
		    push.SetValue(methodBase);
		    PushVariant(push);
	    }

	    // Token: 0x06000213 RID: 531 RVA: 0x0000C750 File Offset: 0x0000A950
	    private void Ldind_u4_(VariantBase dummy) // \u000E\u2002\u2001
	    {
		    Ldind(typeof(uint));
	    }

        // Token: 0x06000218 RID: 536 RVA: 0x0000C810 File Offset: 0x0000AA10
        private void Ldind_i2_(VariantBase dummy) // \u0003\u2002\u2001
	    {
		    Ldind(typeof(short));
	    }

	    // Token: 0x0600021A RID: 538 RVA: 0x0000C830 File Offset: 0x0000AA30
	    private void Ldind_u2_(VariantBase dummy) // \u000F\u2001\u2001
	    {
		    Ldind(typeof(ushort));
	    }

	    // Token: 0x0600021B RID: 539 RVA: 0x0000C844 File Offset: 0x0000AA44
	    private void Break_(VariantBase dummy) // \u0005\u2002
	    {
		    Debugger.Break();
	    }

	    // Token: 0x0600021D RID: 541 RVA: 0x0000C878 File Offset: 0x0000AA78
	    private void Ldc_i4_m1_(VariantBase dummy) // \u0002\u2005
	    {
		    var iv = new IntVariant();
		    iv.SetValue(-1);
		    PushVariant(iv);
	    }

	    // Token: 0x0600021E RID: 542 RVA: 0x0000C88C File Offset: 0x0000AA8C
	    private void ExecuteNextInstruction() // \u0002\u2000
        {
		    try
		    {
			    TryExecuteNextInstruction();
		    }
		    catch (Exception ex)
		    {
			    OnException(ex, 0u);
		    }
	    }
	    
        // Token: 0x06000220 RID: 544 RVA: 0x0000F07C File Offset: 0x0000D27C
	    private void OnException(object ex, uint pos) // \u0002
        {
		    _wasException = ex != null;
		    _exception = ex;
		    if (_wasException)
		    {
			    _ehStack.Clear();
		    }
		    if (!_wasException)
		    {
                _ehStack.PushBack(new ExcHandlerFrame { Pos = pos });
		    }
		    foreach (var catchBlock in _catchBlocks)
		    {
		        if (PosInRange(_myBufferPos, catchBlock.Start, catchBlock.Len))
		        {
		            switch (catchBlock.Kind)
		            {
		                case 0:
		                    if (_wasException)
		                    {
		                        var type = ex.GetType();
		                        var type2 = GetTypeById(catchBlock.ExcTypeId);
		                        if (type == type2 || type.IsSubclassOf(type2))
		                        {
                                    _ehStack.PushBack(new ExcHandlerFrame
                                    {
                                        Pos = catchBlock.Pos,
                                        Exception = ex
                                    });
		                            _wasException = false;
		                        }
		                    }
		                    break;
		                case 1:
		                    if (_wasException)
		                    {
                                _ehStack.PushBack(new ExcHandlerFrame { Pos = catchBlock.Pos });
		                    }
		                    break;
		                case 2:
		                    if (_wasException || !PosInRange((long)(ulong)pos, catchBlock.Start, catchBlock.Len))
		                    {
                                _ehStack.PushBack(new ExcHandlerFrame { Pos = catchBlock.Pos });
		                    }
		                    break;
		                case 4:
		                    if (_wasException)
		                    {
                                _ehStack.PushBack(new ExcHandlerFrame
                                {
                                    Pos = catchBlock.PosKind4,
                                    Exception = ex
                                });
		                    }
		                    break;
		            }
		        }
		    }
		    ExecuteExceptionHandler();
	    }

	    // Token: 0x06000221 RID: 545 RVA: 0x0000F210 File Offset: 0x0000D410
	    private void Stloc_0_(VariantBase dummy) // \u0008\u2003
	    {
            PopToLocal(0);
	    }

	    // Token: 0x06000222 RID: 546 RVA: 0x0000F21C File Offset: 0x0000D41C
	    private void Ldind_ref_(VariantBase dummy) // \u0003\u2003\u2001
	    {
		    Ldind(SimpleTypeHelper.ObjectType);
	    }

	    // Token: 0x06000223 RID: 547 RVA: 0x0000F22C File Offset: 0x0000D42C
	    private void Stind_r4_(VariantBase dummy) // \u0006\u2006
	    {
		    Stind();
	    }

	    // Token: 0x06000224 RID: 548 RVA: 0x0000F234 File Offset: 0x0000D434
	    private void Newarr_(VariantBase vTypeId) // \u0002\u2001\u2001
	    {
            var vLength = PopVariant();
		    var ivLength = vLength as IntVariant;
		    int length;
		    if (ivLength != null)
		    {
			    length = ivLength.GetValue();
		    }
		    else
		    {
			    var ipvLength = vLength as IntPtrVariant;
			    if (ipvLength != null)
			    {
				    length = ipvLength.GetValue().ToInt32();
			    }
			    else
			    {
				    var uipvLength = vLength as UIntPtrVariant;
				    if (uipvLength == null)
				    {
					    throw new Exception();
				    }
				    length = (int)uipvLength.GetValue().ToUInt32();
			    }
		    }
		    var array = Array.CreateInstance(GetTypeById(((IntVariant)vTypeId).GetValue()), length);
		    var push = new ArrayVariant();
		    push.SetValue(array);
		    PushVariant(push);
	    }

	    // Token: 0x06000226 RID: 550 RVA: 0x0000F308 File Offset: 0x0000D508
	    private bool RefToMdArrayItem(MethodBase mb, object array, ref object result, object[] oidxs) // \u0003
        {
		    if (!mb.IsStatic && mb.Name == StringDecryptor.GetString(-1550345964) /* Address */)
		    {
			    var methodInfo = mb as MethodInfo;
			    if (methodInfo != null)
			    {
				    var type = methodInfo.ReturnType;
				    if (type.IsByRef)
				    {
					    type = type.GetElementType();
					    var num = oidxs.Length;
					    if (num >= 1 && oidxs[0] is int)
					    {
						    var idxs = new int[num];
						    for (var i = 0; i < num; i++)
						    {
							    idxs[i] = (int)oidxs[i];
						    }
						    var val = new MdArrayValueVariant();
						    val.SetArray((Array)array);
						    val.SetIndexes(idxs);
						    val.SetHeldType(type);
						    result = val;
						    return true;
					    }
				    }
			    }
		    }
		    return false;
	    }

	    // Token: 0x0600022C RID: 556 RVA: 0x0000F86C File Offset: 0x0000DA6C
	    private void Stelem_r4_(VariantBase dummy) // \u0005\u200A
	    {
		    Stelem(typeof(float));
	    }

	    // Token: 0x0600022E RID: 558 RVA: 0x0000F8AC File Offset: 0x0000DAAC
	    private void Ldarg_2_(VariantBase dummy) // \u0006\u2003
	    {
		    PushVariant(_variantOutputArgs[2].Clone());
	    }

	    // Token: 0x06000230 RID: 560 RVA: 0x0000F8DC File Offset: 0x0000DADC
	    private void Not_(VariantBase dummy) // \u0008\u2002\u2001
	    {
            PushVariant(Not(PopVariant()));
	    }

	    // Token: 0x06000232 RID: 562 RVA: 0x0000F914 File Offset: 0x0000DB14
	    private void Ldind_i1_(VariantBase dummy) // \u000F\u2005\u2000
	    {
		    Ldind(typeof(sbyte));
	    }

	    // Token: 0x06000233 RID: 563 RVA: 0x0000F928 File Offset: 0x0000DB28
	    private void Stloc_2_(VariantBase dummy) // \u000F\u2009
	    {
            PopToLocal(2);
	    }

	    // Token: 0x06000235 RID: 565 RVA: 0x0000F96C File Offset: 0x0000DB6C
	    private void Stloc_1_(VariantBase dummy) // \u000E\u2009\u2000
	    {
            PopToLocal(1);
	    }

	    // Token: 0x06000236 RID: 566 RVA: 0x0000F978 File Offset: 0x0000DB78
	    private void Pop_(VariantBase dummy) // \u0003
        {
		    PopVariant();
	    }

	    // Token: 0x0600023A RID: 570 RVA: 0x0000F9D4 File Offset: 0x0000DBD4
	    private void Ldc_r8_(VariantBase val) // \u0005\u200B\u2000
	    {
		    PushVariant(val);
	    }

	    // Token: 0x0600023F RID: 575 RVA: 0x0000FDF4 File Offset: 0x0000DFF4
	    private void Ldelem_r4_(VariantBase dummy) // \u0008\u200B
	    {
		    Ldelem(typeof(float));
	    }

	    // Token: 0x06000240 RID: 576 RVA: 0x0000FE08 File Offset: 0x0000E008
	    private void UnlockInterlockedIfAny(ref BoolHolder wasLocked) // \u0002
        {
		    if (wasLocked.Val)
		    {
			    Monitor.Exit(InterlockedLock);
		    }
	    }

	    // Token: 0x06000241 RID: 577 RVA: 0x0000FE1C File Offset: 0x0000E01C
	    private void Ldarg_0_(VariantBase dummy) // \u0003\u2007
	    {
		    PushVariant(_variantOutputArgs[0].Clone());
	    }

	    // Token: 0x06000245 RID: 581 RVA: 0x00010198 File Offset: 0x0000E398
	    private VariantBase Not(VariantBase val) // \u0005
        {
            switch (val.GetTypeCode())
            {
                case VariantBase.Vtc.Tc19Int:
                    var iv = new IntVariant();
			        iv.SetValue(~((IntVariant)val).GetValue());
			        return iv;
                case VariantBase.Vtc.Tc24Long:
                    var lv = new LongVariant();
			        lv.SetValue(~((LongVariant)val).GetValue());
			        return lv;
                case VariantBase.Vtc.Tc5Enum:
		            var underlyingType = Enum.GetUnderlyingType(val.GetValueAbstract().GetType());
		            if (underlyingType == typeof(ulong))
		            {
                        var ret = new UlongVariant();
			            ret.SetValue(~Convert.ToUInt64(val.GetValueAbstract()));
			            return ret;
		            }
                    if (underlyingType == typeof(long))
                    {
                        var ret = new LongVariant();
                        ret.SetValue(~Convert.ToInt64(val.GetValueAbstract()));
                        return ret;
                    }
                    if (underlyingType == typeof(uint))
                    {
                        var ret = new UintVariant();
                        ret.SetValue(~Convert.ToUInt32(val.GetValueAbstract()));
                        return ret;
                    }
                    var result = new IntVariant();
		            result.SetValue(~Convert.ToInt32(val.GetValueAbstract()));
		            return result;
                case VariantBase.Vtc.Tc21Double:
                    if (IntPtr.Size == 4)
                    {
                        var ret = new DoubleVariant();
                        ret.SetValue(double.NaN);
                        return ret;
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    if (IntPtr.Size == 4)
                    {
                        var ret = new FloatVariant();
                        ret.SetValue(float.NaN);
                        return ret;
                    }
                    break;
                case VariantBase.Vtc.Tc18Object:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(OpCodes.Not);
                    gen.Emit(OpCodes.Ret);
                    var oret = new IntPtrVariant();
                    ((IntPtrVariant)oret).SetValue(((IntPtr)dyn.Invoke(null, new[] { val.GetValueAbstract() })));
                    return oret;
            }
            throw new InvalidProgramException();
	    }

	    // Token: 0x06000246 RID: 582 RVA: 0x0001025C File Offset: 0x0000E45C
	    private void Ldind_i8_(VariantBase dummy) // \u0005\u2006
	    {
		    Ldind(typeof(long));
	    }

	    // Token: 0x06000247 RID: 583 RVA: 0x00010270 File Offset: 0x0000E470
	    private void Clt_(VariantBase dummy) // \u0002\u2004\u2001
        {
		    var v2 = PopVariant();
		    var v1 = PopVariant();
		    var push = new IntVariant();
            push.SetValue(UniCompare(v1, v2, ComparisonKind.LT, false) ? 1 : 0);
		    PushVariant(push);
	    }

	    // Token: 0x06000249 RID: 585 RVA: 0x00010348 File Offset: 0x0000E548
	    private void Nop_(VariantBase dummy) // \u0005\u2005
	    {
	    }

	    // Token: 0x0600024B RID: 587 RVA: 0x00010498 File Offset: 0x0000E698
	    private void TryExecuteNextInstruction() // \u000E
        {
		    var key = _myBufferReader.ReadInt32();
		    VmInstr instr;
		    if (!_vmInstrDb.TryGetValue(key, out instr))
		    {
			    throw new InvalidOperationException(StringDecryptor.GetString(-1550345644) /* Unsupported instruction. */);
		    }
            instr.Func(ReadOperand(_myBufferReader, instr.Id.OperandType));
		    _myBufferPos = _myBufferReader.GetBuffer().GetPos();
	    }

	    // Token: 0x0600024E RID: 590 RVA: 0x000105B4 File Offset: 0x0000E7B4
	    private void Stelem_i1_(VariantBase dummy) // \u0002\u2003
	    {
		    var obj = PopVariant().GetValueAbstract();
		    var idx = PopLong();
		    var array = (Array)PopVariant().GetValueAbstract();
		    var elementType = array.GetType().GetElementType();
		    checked
		    {
			    if (elementType == typeof(sbyte))
			    {
                    ((sbyte[])array)[(int)(IntPtr)idx] = (sbyte)VariantFactory.Convert(obj, typeof(sbyte)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(byte))
			    {
                    ((byte[])array)[(int)(IntPtr)idx] = (byte)VariantFactory.Convert(obj, typeof(byte)).GetValueAbstract();
				    return;
			    }
			    if (elementType == typeof(bool))
			    {
                    ((bool[])array)[(int)(IntPtr)idx] = (bool)VariantFactory.Convert(obj, typeof(bool)).GetValueAbstract();
				    return;
			    }
			    if (elementType.IsEnum)
			    {
                    Stelem(elementType, obj, idx, array);
				    return;
			    }
                Stelem(typeof(sbyte), obj, idx, array);
		    }
	    }

	    // Token: 0x0600024F RID: 591 RVA: 0x000106B8 File Offset: 0x0000E8B8
	    private void Starg_s_(VariantBase vidx) // \u000E\u2007\u2000
	    {
		    var idx = (ByteVariant)vidx;
		    _variantOutputArgs[idx.GetValue()].CopyFrom(PopVariant());
	    }

	    // Token: 0x06000251 RID: 593 RVA: 0x00010748 File Offset: 0x0000E948
	    private void Ldlen_(VariantBase dummy) // \u000E\u200B\u2000
	    {
		    var array = (Array)PopVariant().GetValueAbstract();
		    var len = new IntVariant();
		    len.SetValue(array.Length);
		    PushVariant(len);
	    }

	    // Token: 0x06000256 RID: 598 RVA: 0x00010948 File Offset: 0x0000EB48
	    private void Ldelem_i2_(VariantBase dummy) // \u0008\u2000
	    {
		    Ldelem(typeof(short));
	    }

	    // Token: 0x06000257 RID: 599 RVA: 0x0001095C File Offset: 0x0000EB5C
	    private void Ldarg_(VariantBase vidx) // \u000E\u2000\u2000
        {
		    var idx = (UshortVariant)vidx;
		    PushVariant(_variantOutputArgs[idx.GetValue()].Clone());
	    }

	    // Token: 0x06000258 RID: 600 RVA: 0x0001098C File Offset: 0x0000EB8C
	    private void Clt_un_(VariantBase dummy) // \u0003\u2000\u2001
	    {
		    var v2 = PopVariant();
		    var v1 = PopVariant();
		    var push = new IntVariant();
            push.SetValue(UniCompare(v1, v2, ComparisonKind.LT, true) ? 1 : 0);
		    PushVariant(push);
	    }

	    // Token: 0x06000259 RID: 601 RVA: 0x000109C8 File Offset: 0x0000EBC8
	    private void Dup_(VariantBase dummy) // \u0002\u2006\u2000
	    {
		    var v = PopVariant();
		    PushVariant(v);
		    PushVariant(v.Clone());
	    }

	    // Token: 0x0600025A RID: 602 RVA: 0x000109F4 File Offset: 0x0000EBF4
	    [Conditional("DEBUG")]
	    private void DoNothing(object dummy) // \u0002
        {
	    }

	    // Token: 0x0600025B RID: 603 RVA: 0x000109F8 File Offset: 0x0000EBF8
	    private VariantBase[] ArgsToVariantOutputArgs(object[] args) // u0002
	    {
		    var methodHeaderArgsTypeToOutput = _methodHeader.ArgsTypeToOutput;
		    var num = methodHeaderArgsTypeToOutput.Length;
		    var retArgs = new VariantBase[num];
		    for (var i = 0; i < num; i++)
		    {
			    var obj = args[i];
			    var type = GetTypeById(methodHeaderArgsTypeToOutput[i].TypeId);
			    var type2 = ElementedTypeHelper.TryGoToPointerOrReferenceElementType(type);
			    Type type3;
			    if (type2 == SimpleTypeHelper.ObjectType || SimpleTypeHelper.IsNullableGeneric(type2))
			    {
				    type3 = type;
			    }
			    else
			    {
				    type3 = obj?.GetType() ?? type;
			    }
			    if (obj != null && !type.IsAssignableFrom(type3) && type.IsByRef && !type.GetElementType().IsAssignableFrom(type3))
			    {
				    throw new ArgumentException(string.Format(StringDecryptor.GetString(-1550345390) /* Object of type {0} cannot be converted to type {1}. */, type3, type));
			    }
			    retArgs[i] = VariantFactory.Convert(obj, type3);
		    }
		    if (!_methodHeader.IsStatic() && GetTypeById(_methodHeader.ClassId).IsValueType)
		    {
			    var expr_EB = new VariantBaseHolder();
			    expr_EB.SetValue(retArgs[0]);
		        retArgs[0] = expr_EB;
		    }
		    for (var j = 0; j < num; j++)
		    {
			    if (methodHeaderArgsTypeToOutput[j].IsOutput)
			    {
				    var expr_116 = new VariantBaseHolder();
				    expr_116.SetValue(retArgs[j]);
			        retArgs[j] = expr_116;
			    }
		    }
		    return retArgs;
	    }

	    // Token: 0x0600025D RID: 605 RVA: 0x00010B3C File Offset: 0x0000ED3C
	    private void Stind() // \u0003
        {
		    var v2 = PopVariant();
		    var v1 = PopVariant();
            AssignByReference(v1, v2);
	    }

	    // Token: 0x0600025E RID: 606 RVA: 0x00010B60 File Offset: 0x0000ED60
	    private void Endfinally_(VariantBase dummy) // \u0005\u2000\u2001
	    {
		    ExecuteExceptionHandler();
	    }

	    // Token: 0x0600025F RID: 607 RVA: 0x00010B68 File Offset: 0x0000ED68
	    private bool PosInRange(long pos, uint start, uint len) // \u0002
        {
		    return pos >= (long)(ulong)start && pos <= (long)(ulong)(start + len);
	    }

	    // Token: 0x06000260 RID: 608 RVA: 0x00010B7C File Offset: 0x0000ED7C
	    private bool IsCompatible(MethodBase mb) // \u0002
        {
		    return mb.IsVirtual && GetTypeById(_methodHeader.ClassId).IsSubclassOf(mb.DeclaringType);
	    }

	    // Token: 0x06000263 RID: 611 RVA: 0x00010C00 File Offset: 0x0000EE00
	    private void Stelem_(VariantBase vTypeId) // \u0006\u2004\u2000
	    {
		    Stelem(GetTypeById(((IntVariant)vTypeId).GetValue()));
	    }

	    // Token: 0x06000265 RID: 613 RVA: 0x00010C40 File Offset: 0x0000EE40
	    private void Ldelem_u1_(VariantBase dummy) // \u0006\u2004
	    {
		    Ldelem(typeof(byte));
	    }

	    // Token: 0x06000267 RID: 615 RVA: 0x00010C80 File Offset: 0x0000EE80
	    private void Cgt_(VariantBase dummy) // \u0005\u2001\u2000
	    {
		    var v2 = PopVariant();
		    var v1 = PopVariant();
		    var push = new IntVariant();
            push.SetValue(UniCompare(v1, v2, ComparisonKind.GT, false) ? 1 : 0);
		    PushVariant(push);
	    }

	    private VariantBase And(VariantBase org_v1, VariantBase org_v2)
	    {
	        VariantBase v1, v2;
            var tc = CommonType(org_v1, org_v2, out v1, out v2, true);
            VariantBase ret;
            switch (tc)
            {
                case VariantBase.Vtc.Tc9Uint:
                    uint uv1 = ((UintVariant)v1).GetValue(), uv2 = ((UintVariant)v2).GetValue();
                    var uvret = new UintVariant();
                    ret = uvret;
                    uvret.SetValue(uv1 & uv2);
                    break;
                case VariantBase.Vtc.Tc19Int:
                    int iv1 = ((IntVariant)v1).GetValue(), iv2 = ((IntVariant)v2).GetValue();
                    var ivret = new IntVariant();
                    ret = ivret;
                    ivret.SetValue(iv1 & iv2);
                    break;
                case VariantBase.Vtc.Tc21Double:
                    {
                        /*double dv1 = ((DoubleVariant)v1).GetValue(), dv2 = ((DoubleVariant)v2).GetValue(); // естественный алгоритм
                        long lv1 = (dv1 < 0) ? (long)dv1 : (long)(ulong)dv1;
                        long lv2 = (dv2 < 0) ? (long)dv2 : (long)(ulong)dv2;
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        var l64 = (ulong) lv1 & (ulong) lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        dvret.SetValue(l64);*/
                        var dvret = new DoubleVariant();
                        ret = dvret;
                        dvret.SetValue((4 == IntPtr.Size) ? Double.NaN : (double)0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc8Float:
                    {
                        /*float fv1 = ((FloatVariant) v1).GetValue(), fv2 = ((FloatVariant) v2).GetValue(); // естественный алгоритм
                        long lv1 = (fv1 < 0) ? (long)fv1 : (long)(ulong)fv1;
                        long lv2 = (fv2 < 0) ? (long)fv2 : (long)(ulong)fv2;
                        var fvret = new FloatVariant();
                        ret = fvret;
                        var l64 = (ulong)lv1 & (ulong)lv2;
                        if (l64 >> 32 == UInt32.MaxValue) l64 &= UInt32.MaxValue;
                        fvret.SetValue(l64);*/
                        var fvret = new FloatVariant();
                        ret = fvret;
                        fvret.SetValue((4 == IntPtr.Size) ? float.NaN : (float)0.0); // иногда у фреймворка бывает мусор, но чаще эти значения...
                    }
                    break;
                case VariantBase.Vtc.Tc24Long:
                    {
                        long lv1 = ((LongVariant)v1).GetValue(), lv2 = ((LongVariant)v2).GetValue();
                        var lvret = new LongVariant();
                        ret = lvret;
                        lvret.SetValue(lv1 & lv2);
                    }
                    break;
                case VariantBase.Vtc.Tc7Ulong:
                    ulong ulv1 = ((UlongVariant)v1).GetValue(), ulv2 = ((UlongVariant)v2).GetValue();
                    var ulvret = new UlongVariant();
                    ret = ulvret;
                    ulvret.SetValue(ulv1 & ulv2);
                    break;
                default:
                    // это нужно будет заменить на соотв. msil-код
                    var dyn = new DynamicMethod(String.Empty, typeof(IntPtr), new[] { typeof(object), typeof(object) }, typeof(void), true);
                    var gen = dyn.GetILGenerator();
                    gen.Emit(OpCodes.Ldarg_1);
                    gen.Emit(OpCodes.Ldarg_0);
                    gen.Emit(OpCodes.And);
                    gen.Emit(OpCodes.Ret);
                    ret = new IntPtrVariant();
                    ((IntPtrVariant)ret).SetValue(((IntPtr)dyn.Invoke(null, new[] { org_v1.GetValueAbstract(), org_v2.GetValueAbstract() })));
                    break;
            }
            return ret;
        }

        // Token: 0x0600026E RID: 622 RVA: 0x000110D8 File Offset: 0x0000F2D8
        private void Ldind(Type t) // \u0005
        {
            PushVariant(VariantFactory.Convert(FetchByAddr(PopVariant()).GetValueAbstract(), t));
	    }

	    // Token: 0x0600026F RID: 623 RVA: 0x00011104 File Offset: 0x0000F304
	    private void Starg_(VariantBase vidx) // \u000F\u2008\u2000
	    {
		    var idx = (UshortVariant)vidx;
		    _variantOutputArgs[idx.GetValue()].CopyFrom(PopVariant());
	    }

	    // Token: 0x06000270 RID: 624 RVA: 0x00011138 File Offset: 0x0000F338
	    private void Ret() // \u0008
        {
		    _retFound = true;
	    }

	    // Token: 0x06000273 RID: 627 RVA: 0x00011480 File Offset: 0x0000F680
	    private void Stind_r8_(VariantBase dummy) // \u0005\u2004\u2000
	    {
		    Stind();
	    }

	    // Token: 0x06000275 RID: 629 RVA: 0x00011510 File Offset: 0x0000F710
	    private void Rethrow_(VariantBase dummy) // \u0003\u200A\u2000
        {
		    if (_exception == null)
		    {
			    throw new InvalidOperationException();
		    }
		    _myBufferPos = _myBufferReader.GetBuffer().GetPos();
		    ThrowStoreCrossDomain(_exception);
	    }

	    // Token: 0x06000278 RID: 632 RVA: 0x00011838 File Offset: 0x0000FA38
	    private void Ldelem_u4_(VariantBase dummy) // \u0003\u2003
	    {
		    Ldelem(typeof(uint));
	    }

	    // Token: 0x0600027A RID: 634 RVA: 0x00011994 File Offset: 0x0000FB94
	    private void Invoke(VmMethodRefTokenInfo mref) // \u0002
        {
            //var arg_18_0 = (U0008U2007)U0003U2008.Get_u0005();
            var methodBase = FindMethodById(mref.Pos, ReadToken(mref.Pos));
		    //methodBase.GetParameters();
		    var pos = mref.Flags;
		    var mayVirtual = (pos & 1073741824) != 0;
		    pos &= -1073741825;
		    var methodGenericArgs = _methodGenericArgs;
		    var classGenericArgs = _classGenericArgs;
		    try
		    {
			    _methodGenericArgs = methodBase is ConstructorInfo ? Type.EmptyTypes : methodBase.GetGenericArguments();
			    _classGenericArgs = methodBase.DeclaringType.GetGenericArguments();
                Invoke(pos, _methodGenericArgs, _classGenericArgs, mayVirtual);
		    }
		    finally
		    {
			    _methodGenericArgs = methodGenericArgs;
			    _classGenericArgs = classGenericArgs;
		    }
	    }

	    // Token: 0x0600027B RID: 635 RVA: 0x00011A5C File Offset: 0x0000FC5C
	    private void Ldc_i8_(VariantBase val) // \u0006\u2007\u2000
	    {
		    PushVariant(val);
	    }

	    // Token: 0x0600027E RID: 638 RVA: 0x00011CB8 File Offset: 0x0000FEB8
	    private void Ldelem_r8_(VariantBase dummy) // \u000E\u2002\u2000
	    {
		    Ldelem(typeof(double));
	    }

	    // Token: 0x06000280 RID: 640 RVA: 0x00011DB4 File Offset: 0x0000FFB4
	    private void Stloc_3_(VariantBase dummy) // \u0003\u2004\u2000
	    {
            PopToLocal(3);
	    }

	    // Token: 0x06000281 RID: 641 RVA: 0x00011DC0 File Offset: 0x0000FFC0
	    private void Ckfinite_(VariantBase dummy) // \u000F\u2003
	    {
	        var v = PopVariant();
            if (v.GetTypeCode() == VariantBase.Vtc.Tc5Enum)
            {
                v = VariantBase.SignedVariantFromEnum((EnumVariant)v);
            }
	        double val = double.NaN;
	        bool con = v.GetValueAbstract() is IConvertible;
	        if (con)
	        {
	            val = Convert.ToDouble(v.GetValueAbstract());
                if (double.IsNaN(val) || double.IsInfinity(val))
                {
                    throw new OverflowException(StringDecryptor.GetString(-1550347095) /* The value is not finite real number. */);
                }
            }
            if (IsFloating(v))
	        {
                PushVariant(v);
            } else
	        {
	            var push = new DoubleVariant();
                push.SetValue(val);
                PushVariant(push);
	        }
	    }

	    // Token: 0x06000288 RID: 648 RVA: 0x00012170 File Offset: 0x00010370
	    private void Stind_ref_(VariantBase dummy) // \u0002\u2008
	    {
		    Stind();
	    }

	    // Token: 0x06000289 RID: 649 RVA: 0x00012178 File Offset: 0x00010378
	    private void PopToLocal(int idx) // \u0002
        {
		    var pop = PopVariant();
		    if (pop is ReferenceVariantBase)
		    {
			    _localVariables[idx] = pop;
			    return;
		    }
		    _localVariables[idx].CopyFrom(pop);
	    }

	    // Token: 0x0600028B RID: 651 RVA: 0x00012260 File Offset: 0x00010460
	    private void Ldobj_(VariantBase vTypeId) // \u000E\u2008
	    {
            var type = GetTypeById(((IntVariant)vTypeId).GetValue());
		    Ldind(type);
	    }

	    // Token: 0x0600028C RID: 652 RVA: 0x00012288 File Offset: 0x00010488
	    private void Ldloc_1_(VariantBase dummy) // \u0003\u2008\u2000
	    {
		    PushVariant(_localVariables[1].Clone());
	    }

	    // Token: 0x0600028E RID: 654 RVA: 0x00012348 File Offset: 0x00010548
	    private void Ldelem_i1_(VariantBase dummy) // \u0005\u2009\u2000
	    {
		    Ldelem(typeof(sbyte));
	    }

	    // Token: 0x06000290 RID: 656 RVA: 0x00012370 File Offset: 0x00010570
	    private void JumpToPos(uint val) // \u0002
        {
		    _storedPos = val;
	    }

	    // Token: 0x06000291 RID: 657 RVA: 0x00012380 File Offset: 0x00010580
	    public void VoidInvoke(Stream virtualizedStream, string pos, object[] args) // \u0002
        {
		    Invoke(virtualizedStream, pos, args);
	    }

	    // Token: 0x06000295 RID: 661 RVA: 0x00012420 File Offset: 0x00010620
	    private void _u0002u2002u2001(VariantBase dummy) // \u0002\u2002\u2001
	    {
	    }

	    // Token: 0x06000298 RID: 664 RVA: 0x00012590 File Offset: 0x00010790
	    private void LoopUntilRet() // \u0005\u2000
        {
		    var usedSize = _myBufferReader.GetBuffer().UsedSize();
		    while (!_retFound)
		    {
			    if (_storedPos.HasValue)
			    {
				    _myBufferReader.GetBuffer().SetPos((long)(ulong)_storedPos.Value);
				    _storedPos = null;
			    }
			    ExecuteNextInstruction();
			    if (_myBufferReader.GetBuffer().GetPos() >= usedSize && !_storedPos.HasValue)
			    {
				    break;
			    }
		    }
	    }

	    // Token: 0x06000299 RID: 665 RVA: 0x00012614 File Offset: 0x00010814
	    private VmInstrInfo GetInstrById(int id) // \u0002
	    {
	        return _instrCodesDb.MyFieldsEnumerator().FirstOrDefault(current => current.Id == id);
	    }

	    // Token: 0x0600029A RID: 666 RVA: 0x00012670 File Offset: 0x00010870
	    private void Ldloc_(VariantBase val) // \u000F\u200A\u2000
	    {
		    PushVariant(_localVariables[((UshortVariant)val).GetValue()].Clone());
	    }

	    // Token: 0x0600029E RID: 670 RVA: 0x00012718 File Offset: 0x00010918
	    private bool Isinst(VariantBase obj, Type t) // \u0002
        {
		    if (obj.GetValueAbstract() == null)
		    {
			    return true;
		    }
		    var type = obj.GetVariantType() ?? obj.GetValueAbstract().GetType();
		    if (type == t || t.IsAssignableFrom(type))
		    {
			    return true;
		    }
		    if (!type.IsValueType && !t.IsValueType && Marshal.IsComObject(obj.GetValueAbstract()))
		    {
			    IntPtr intPtr;
			    try
			    {
				    intPtr = Marshal.GetComInterfaceForObject(obj.GetValueAbstract(), t);
			    }
			    catch (InvalidCastException)
			    {
				    intPtr = IntPtr.Zero;
			    }
			    if (intPtr != IntPtr.Zero)
			    {
				    try
				    {
					    Marshal.Release(intPtr);
				    }
				    catch
				    {
				    }
				    return true;
			    }
		    }
		    return false;
	    }

	    // Token: 0x0600029F RID: 671 RVA: 0x000127C4 File Offset: 0x000109C4
	    private void Ldind_r8_(VariantBase dummy) // \u000E\u2008\u2000
	    {
		    Ldind(typeof(double));
	    }

	    // Token: 0x060002A1 RID: 673 RVA: 0x00012870 File Offset: 0x00010A70
	    private void Stobj_(VariantBase dummy) // \u0005\u2009
	    {
		    Stind();
	    }

        // Token: 0x060002A4 RID: 676 RVA: 0x00012A44 File Offset: 0x00010C44
        private void Cgt_un_(VariantBase dummy) // \u0006\u2002
	    {
		    var v2 = PopVariant();
		    var v1 = PopVariant();
		    var push = new IntVariant();
            push.SetValue(UniCompare(v1, v2, ComparisonKind.GT, true) ? 1 : 0);
		    PushVariant(push);
	    }

	    // Token: 0x060002A7 RID: 679 RVA: 0x00012CDC File Offset: 0x00010EDC
	    private bool AreCompatible(Type t1, UniversalTokenInfo ut2) // \u0002
        {
		    var t2 = (VmClassTokenInfo)ut2.VmToken;
		    if (ElementedTypeHelper.TryGoToElementType(t1).IsGenericParameter)
		    {
			    return t2 == null || t2.IsOuterClassGeneric;
		    }
            return TypeCompatibility.Check(t1, GetTypeById(ut2.MetadataToken));
	    }

	    // Token: 0x060002A9 RID: 681 RVA: 0x00012D38 File Offset: 0x00010F38
	    private void Ldloca_(VariantBase vLocIdx) // \u0006\u2005
	    {
            var push = new LocalsIdxHolderVariant();
		    push.SetValue(((UshortVariant)vLocIdx).GetValue());
		    PushVariant(push);
	    }

	    // Token: 0x060002AA RID: 682 RVA: 0x00012D64 File Offset: 0x00010F64
	    private void Ldarg_3_(VariantBase dummy) // \u000E\u2009
	    {
		    PushVariant(_variantOutputArgs[3].Clone());
	    }

	    // Token: 0x060002AC RID: 684 RVA: 0x00012DB8 File Offset: 0x00010FB8
	    /*[Conditional("DEBUG")]
	    public static void DoNothing(string dummy) // \u0002
        {
	    }*/

        // Token: 0x0600021F RID: 543 RVA: 0x0000C8BC File Offset: 0x0000AABC
        private Dictionary<int, VmInstr> CreateVmInstrDb() // \u0002
        {
            return new Dictionary<int, VmInstr>(256)
            {
                {
                    _instrCodesDb.Conv_ovf_i4_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i4_un_, Conv_ovf_i4_un_)
                },
                {
                    _instrCodesDb.Shr_un_.Id,
                    new VmInstr(_instrCodesDb.Shr_un_, Shr_un_)
                },
                {
                    _instrCodesDb.Conv_i_.Id,
                    new VmInstr(_instrCodesDb.Conv_i_, Conv_i_)
                },
                {
                    _instrCodesDb.Conv_ovf_i_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i_un_, Conv_ovf_i_un_)
                },
                {
                    _instrCodesDb.Stelem_i_.Id,
                    new VmInstr(_instrCodesDb.Stelem_i_, Stelem_i_)
                },
                {
                    _instrCodesDb.Starg_s_.Id,
                    new VmInstr(_instrCodesDb.Starg_s_, Starg_s_)
                },
                {
                    _instrCodesDb.Sizeof_.Id,
                    new VmInstr(_instrCodesDb.Sizeof_, Sizeof_)
                },
                {
                    _instrCodesDb.Ldarg_s_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_s_, Ldarg_s_)
                },
                {
                    _instrCodesDb.Stelem_i4_.Id,
                    new VmInstr(_instrCodesDb.Stelem_i4_, Stelem_i4_)
                },
                {
                    _instrCodesDb.Calli_.Id,
                    new VmInstr(_instrCodesDb.Calli_, Calli_)
                },
                {
                    _instrCodesDb.Ldc_i4_7_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_7_, Ldc_i4_7_)
                },
                {
                    _instrCodesDb.Newobj_.Id,
                    new VmInstr(_instrCodesDb.Newobj_, Newobj_)
                },
                {
                    _instrCodesDb.Ldind_u4_.Id,
                    new VmInstr(_instrCodesDb.Ldind_u4_, Ldind_u4_)
                },
                {
                    _instrCodesDb.Cgt_un_.Id,
                    new VmInstr(_instrCodesDb.Cgt_un_, Cgt_un_)
                },
                {
                    _instrCodesDb.Conv_u1_.Id,
                    new VmInstr(_instrCodesDb.Conv_u1_, Conv_u1_)
                },
                {
                    _instrCodesDb.Ldelem_ref_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_ref_, Ldelem_ref_)
                },
                {
                    _instrCodesDb.U0006U2008U2000.Id,
                    new VmInstr(_instrCodesDb.U0006U2008U2000, _u0002u2002u2001)
                },
                {
                    _instrCodesDb.Newarr_.Id,
                    new VmInstr(_instrCodesDb.Newarr_, Newarr_)
                },
                {
                    _instrCodesDb.Ldarga_s_.Id,
                    new VmInstr(_instrCodesDb.Ldarga_s_, Ldarga_s_)
                },
                {
                    _instrCodesDb.Bgt_.Id,
                    new VmInstr(_instrCodesDb.Bgt_, Bgt_)
                },
                {
                    _instrCodesDb.Ldflda_.Id,
                    new VmInstr(_instrCodesDb.Ldflda_, Ldflda_)
                },
                {
                    _instrCodesDb.Sub_.Id,
                    new VmInstr(_instrCodesDb.Sub_, Sub_)
                },
                {
                    _instrCodesDb.Endfilter_.Id,
                    new VmInstr(_instrCodesDb.Endfilter_, Endfilter_)
                },
                {
                    _instrCodesDb.Conv_ovf_u_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u_un_, Conv_ovf_u_un_)
                },
                {
                    _instrCodesDb.Ldc_i4_1_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_1_, Ldc_i4_1_)
                },
                {
                    _instrCodesDb.Conv_ovf_i_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i_, Conv_ovf_i_)
                },
                {
                    _instrCodesDb.Add_ovf_.Id,
                    new VmInstr(_instrCodesDb.Add_ovf_, Add_ovf_)
                },
                {
                    _instrCodesDb.Ldftn_.Id,
                    new VmInstr(_instrCodesDb.Ldftn_, Ldftn_)
                },
                {
                    _instrCodesDb.Stfld_.Id,
                    new VmInstr(_instrCodesDb.Stfld_, Stfld_)
                },
                {
                    _instrCodesDb.Ldc_i4_5_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_5_, Ldc_i4_5_)
                },
                {
                    _instrCodesDb.Xor_.Id,
                    new VmInstr(_instrCodesDb.Xor_, Xor_)
                },
                {
                    _instrCodesDb.Conv_u2_.Id,
                    new VmInstr(_instrCodesDb.Conv_u2_, Conv_u2_)
                },
                {
                    _instrCodesDb.Div_un_.Id,
                    new VmInstr(_instrCodesDb.Div_un_, Div_un_)
                },
                {
                    _instrCodesDb.Stloc_3_.Id,
                    new VmInstr(_instrCodesDb.Stloc_3_, Stloc_3_)
                },
                {
                    _instrCodesDb.Ret_.Id,
                    new VmInstr(_instrCodesDb.Ret_, Ret_)
                },
                {
                    _instrCodesDb.Ldc_i4_m1_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_m1_, Ldc_i4_m1_)
                },
                {
                    _instrCodesDb.Ldarg_1_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_1_, Ldarg_1_)
                },
                {
                    _instrCodesDb.Div_.Id,
                    new VmInstr(_instrCodesDb.Div_, Div_)
                },
                {
                    _instrCodesDb.Ldnull_.Id,
                    new VmInstr(_instrCodesDb.Ldnull_, Ldnull_)
                },
                {
                    _instrCodesDb.Break_.Id,
                    new VmInstr(_instrCodesDb.Break_, Break_)
                },
                {
                    _instrCodesDb.Cgt_.Id,
                    new VmInstr(_instrCodesDb.Cgt_, Cgt_)
                },
                {
                    _instrCodesDb.Arglist_.Id,
                    new VmInstr(_instrCodesDb.Arglist_, Arglist_)
                },
                {
                    _instrCodesDb.Ldloc_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_, Ldloc_)
                },
                {
                    _instrCodesDb.Conv_u_.Id,
                    new VmInstr(_instrCodesDb.Conv_u_, Conv_u_)
                },
                {
                    _instrCodesDb.Ldelem_i_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_i_, Ldelem_i_)
                },
                {
                    _instrCodesDb.Conv_ovf_i1_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i1_un_, Conv_ovf_i1_un_)
                },
                {
                    _instrCodesDb.Cpblk_.Id,
                    new VmInstr(_instrCodesDb.Cpblk_, Cpblk_)
                },
                {
                    _instrCodesDb.Add_.Id,
                    new VmInstr(_instrCodesDb.Add_, Add_)
                },
                {
                    _instrCodesDb.Initblk_.Id,
                    new VmInstr(_instrCodesDb.Initblk_, Initblk_)
                },
                {
                    _instrCodesDb.Ldind_i_.Id,
                    new VmInstr(_instrCodesDb.Ldind_i_, Ldind_i_)
                },
                {
                    _instrCodesDb.Ldelem_u4_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_u4_, Ldelem_u4_)
                },
                {
                    _instrCodesDb.Stind_ref_.Id,
                    new VmInstr(_instrCodesDb.Stind_ref_, Stind_ref_)
                },
                {
                    _instrCodesDb.Ldelem_i1_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_i1_, Ldelem_i1_)
                },
                {
                    _instrCodesDb.Ldloc_3_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_3_, Ldloc_3_)
                },
                {
                    _instrCodesDb.Stind_i8_.Id,
                    new VmInstr(_instrCodesDb.Stind_i8_, Stind_i8_)
                },
                {
                    _instrCodesDb.Conv_i1_.Id,
                    new VmInstr(_instrCodesDb.Conv_i1_, Conv_i1_)
                },
                {
                    _instrCodesDb.Ldelem_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_, Ldelem_)
                },
                {
                    _instrCodesDb.Clt_un_.Id,
                    new VmInstr(_instrCodesDb.Clt_un_, Clt_un_)
                },
                {
                    _instrCodesDb.Ldelem_i4_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_i4_, Ldelem_i4_)
                },
                {
                    _instrCodesDb.Mkrefany_.Id,
                    new VmInstr(_instrCodesDb.Mkrefany_, Mkrefany_)
                },
                {
                    _instrCodesDb.Neg_.Id,
                    new VmInstr(_instrCodesDb.Neg_, Neg_)
                },
                {
                    _instrCodesDb.Leave_.Id,
                    new VmInstr(_instrCodesDb.Leave_, Leave_)
                },
                {
                    _instrCodesDb.Ldc_i4_2_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_2_, Ldc_i4_2_)
                },
                {
                    _instrCodesDb.Conv_ovf_i2_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i2_, Conv_ovf_i2_)
                },
                {
                    _instrCodesDb.Ldloc_2_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_2_, Ldloc_2_)
                },
                {
                    _instrCodesDb.Bgt_un_.Id,
                    new VmInstr(_instrCodesDb.Bgt_un_, Bgt_un_)
                },
                {
                    _instrCodesDb.Stsfld_.Id,
                    new VmInstr(_instrCodesDb.Stsfld_, Stsfld_)
                },
                /*{
                    _instrCodesDb.Nop_.Id,
                    new VmInstr(_instrCodesDb.Nop_, u000Fu2004)
                },*/
                {
                    _instrCodesDb.Shr_.Id,
                    new VmInstr(_instrCodesDb.Shr_, Shr_)
                },
                {
                    _instrCodesDb.Ldind_ref_.Id,
                    new VmInstr(_instrCodesDb.Ldind_ref_, Ldind_ref_)
                },
                {
                    _instrCodesDb.Ldfld_.Id,
                    new VmInstr(_instrCodesDb.Ldfld_, Ldfld_)
                },
                {
                    _instrCodesDb.Ldlen_.Id,
                    new VmInstr(_instrCodesDb.Ldlen_, Ldlen_)
                },
                {
                    _instrCodesDb.Stelem_ref_.Id,
                    new VmInstr(_instrCodesDb.Stelem_ref_, Stelem_ref_)
                },
                {
                    _instrCodesDb.Ceq_.Id,
                    new VmInstr(_instrCodesDb.Ceq_, Ceq_)
                },
                {
                    _instrCodesDb.Conv_ovf_u2_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u2_, Conv_ovf_u2_)
                },
                {
                    _instrCodesDb.Add_ovf_un_.Id,
                    new VmInstr(_instrCodesDb.Add_ovf_un_, Add_ovf_un_)
                },
                {
                    _instrCodesDb.Conv_ovf_i8_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i8_, Conv_ovf_i8_)
                },
                {
                    _instrCodesDb.Stind_i2_.Id,
                    new VmInstr(_instrCodesDb.Stind_i2_, Stind_i2_)
                },
                {
                    _instrCodesDb.Stelem_i1_.Id,
                    new VmInstr(_instrCodesDb.Stelem_i1_, Stelem_i1_)
                },
                {
                    _instrCodesDb.Ldloca_.Id,
                    new VmInstr(_instrCodesDb.Ldloca_, Ldloca_)
                },
                {
                    _instrCodesDb.Stind_r4_.Id,
                    new VmInstr(_instrCodesDb.Stind_r4_, Stind_r4_)
                },
                {
                    _instrCodesDb.Stloc_s_.Id,
                    new VmInstr(_instrCodesDb.Stloc_s_, Stloc_s_)
                },
                {
                    _instrCodesDb.Refanyval_.Id,
                    new VmInstr(_instrCodesDb.Refanyval_, Refanyval_)
                },
                {
                    _instrCodesDb.Clt_.Id,
                    new VmInstr(_instrCodesDb.Clt_, Clt_)
                },
                {
                    _instrCodesDb.Stelem_r4_.Id,
                    new VmInstr(_instrCodesDb.Stelem_r4_, Stelem_r4_)
                },
                {
                    _instrCodesDb.Stelem_r8_.Id,
                    new VmInstr(_instrCodesDb.Stelem_r8_, Stelem_r8_)
                },
                {
                    _instrCodesDb.Conv_u4_.Id,
                    new VmInstr(_instrCodesDb.Conv_u4_, Conv_u4_)
                },
                {
                    _instrCodesDb.Ldc_i8_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i8_, Ldc_i8_)
                },
                {
                    _instrCodesDb.Ldind_r4_.Id,
                    new VmInstr(_instrCodesDb.Ldind_r4_, Ldind_r4_)
                },
                {
                    _instrCodesDb.Conv_r_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_r_un_, Conv_r_un_)
                },
                {
                    _instrCodesDb.Ldtoken_.Id,
                    new VmInstr(_instrCodesDb.Ldtoken_, Ldtoken_)
                },
                {
                    _instrCodesDb.Blt_un_.Id,
                    new VmInstr(_instrCodesDb.Blt_un_, Blt_un_)
                },
                {
                    _instrCodesDb.Brtrue_.Id,
                    new VmInstr(_instrCodesDb.Brtrue_, Brtrue_)
                },
                {
                    _instrCodesDb.Switch_.Id,
                    new VmInstr(_instrCodesDb.Switch_, Switch_)
                },
                {
                    _instrCodesDb.Refanytype_.Id,
                    new VmInstr(_instrCodesDb.Refanytype_, Refanytype_)
                },
                {
                    _instrCodesDb.Stobj_.Id,
                    new VmInstr(_instrCodesDb.Stobj_, Stobj_)
                },
                {
                    _instrCodesDb.Ble_un_.Id,
                    new VmInstr(_instrCodesDb.Ble_un_, Ble_un_)
                },
                {
                    _instrCodesDb.Conv_ovf_i8_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i8_un_, Conv_ovf_i8_un_)
                },
                {
                    _instrCodesDb.Conv_ovf_u4_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u4_un_, Conv_ovf_u4_un_)
                },
                {
                    _instrCodesDb.Ldind_i8_.Id,
                    new VmInstr(_instrCodesDb.Ldind_i8_, Ldind_i8_)
                },
                {
                    _instrCodesDb.U000EU2006U2000.Id,
                    new VmInstr(_instrCodesDb.U000EU2006U2000, Invoke)
                },
                {
                    _instrCodesDb.Endfinally_.Id,
                    new VmInstr(_instrCodesDb.Endfinally_, Endfinally_)
                },
                {
                    _instrCodesDb.Conv_ovf_u8_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u8_un_, Conv_ovf_u8_un_)
                },
                {
                    _instrCodesDb.Ldelem_i2_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_i2_, Ldelem_i2_)
                },
                {
                    _instrCodesDb.Ldc_i4_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_, Ldc_i4_)
                },
                {
                    _instrCodesDb.U000FU2001.Id,
                    new VmInstr(_instrCodesDb.U000FU2001, _u0006u2003u2001)
                },
                {
                    _instrCodesDb.Conv_i4_.Id,
                    new VmInstr(_instrCodesDb.Conv_i4_, Conv_i4_)
                },
                {
                    _instrCodesDb.Ldind_u1_.Id,
                    new VmInstr(_instrCodesDb.Ldind_u1_, Ldind_u1_)
                },
                {
                    _instrCodesDb.Rethrow_.Id,
                    new VmInstr(_instrCodesDb.Rethrow_, Rethrow_)
                },
                {
                    _instrCodesDb.Conv_ovf_i1_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i1_, Conv_ovf_i1_)
                },
                {
                    _instrCodesDb.Box_.Id,
                    new VmInstr(_instrCodesDb.Box_, Box_)
                },
                {
                    _instrCodesDb.Localloc_.Id,
                    new VmInstr(_instrCodesDb.Localloc_, Localloc_)
                },
                {
                    _instrCodesDb.Ldelem_r8_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_r8_, Ldelem_r8_)
                },
                {
                    _instrCodesDb.Throw_.Id,
                    new VmInstr(_instrCodesDb.Throw_, Throw_)
                },
                {
                    _instrCodesDb.Ldvirtftn_.Id,
                    new VmInstr(_instrCodesDb.Ldvirtftn_, Ldvirtftn_)
                },
                {
                    _instrCodesDb.Mul_ovf_un_.Id,
                    new VmInstr(_instrCodesDb.Mul_ovf_un_, Mul_ovf_un_)
                },
                {
                    _instrCodesDb.Conv_ovf_i4_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i4_, Conv_ovf_i4_)
                },
                {
                    _instrCodesDb.Ldloc_0_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_0_, Ldloc_0_)
                },
                {
                    _instrCodesDb.Starg_.Id,
                    new VmInstr(_instrCodesDb.Starg_, Starg_)
                },
                {
                    _instrCodesDb.Stind_i1_.Id,
                    new VmInstr(_instrCodesDb.Stind_i1_, Stind_i1_)
                },
                {
                    _instrCodesDb.Ldind_i2_.Id,
                    new VmInstr(_instrCodesDb.Ldind_i2_, Ldind_i2_)
                },
                {
                    _instrCodesDb.And_.Id,
                    new VmInstr(_instrCodesDb.And_, And_)
                },
                {
                    _instrCodesDb.Ldc_i4_6_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_6_, Ldc_i4_6_)
                },
                {
                    _instrCodesDb.Nop_.Id,
                    new VmInstr(_instrCodesDb.Nop_, Nop_)
                },
                {
                    _instrCodesDb.Ldind_i4_.Id,
                    new VmInstr(_instrCodesDb.Ldind_i4_, Ldind_i4_)
                },
                {
                    _instrCodesDb.Dup_.Id,
                    new VmInstr(_instrCodesDb.Dup_, Dup_)
                },
                {
                    _instrCodesDb.Mul_.Id,
                    new VmInstr(_instrCodesDb.Mul_, Mul_)
                },
                {
                    _instrCodesDb.Stloc_2_.Id,
                    new VmInstr(_instrCodesDb.Stloc_2_, Stloc_2_)
                },
                {
                    _instrCodesDb.Or_.Id,
                    new VmInstr(_instrCodesDb.Or_, Or_)
                },
                {
                    _instrCodesDb.Conv_u8_.Id,
                    new VmInstr(_instrCodesDb.Conv_u8_, Conv_u8_)
                },
                {
                    _instrCodesDb.Conv_ovf_u1_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u1_, Conv_ovf_u1_)
                },
                {
                    _instrCodesDb.Sub_ovf_.Id,
                    new VmInstr(_instrCodesDb.Sub_ovf_, Sub_ovf_)
                },
                {
                    _instrCodesDb.Conv_ovf_u1_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u1_un_, Conv_ovf_u1_un_)
                },
                {
                    _instrCodesDb.Ldelem_r4_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_r4_, Ldelem_r4_)
                },
                {
                    _instrCodesDb.Conv_r8_.Id,
                    new VmInstr(_instrCodesDb.Conv_r8_, Conv_r8_)
                },
                {
                    _instrCodesDb.Stloc_0_.Id,
                    new VmInstr(_instrCodesDb.Stloc_0_, Stloc_0_)
                },
                {
                    _instrCodesDb.Conv_ovf_u8_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u8_, Conv_ovf_u8_)
                },
                {
                    _instrCodesDb.Brfalse_.Id,
                    new VmInstr(_instrCodesDb.Brfalse_, Brfalse_)
                },
                {
                    _instrCodesDb.Ldarg_3_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_3_, Ldarg_3_)
                },
                {
                    _instrCodesDb.Ldarg_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_, Ldarg_)
                },
                {
                    _instrCodesDb.Ldc_r4_.Id,
                    new VmInstr(_instrCodesDb.Ldc_r4_, Ldc_r4_)
                },
                {
                    _instrCodesDb.Initobj_.Id,
                    new VmInstr(_instrCodesDb.Initobj_, Initobj_)
                },
                {
                    _instrCodesDb.Stloc_.Id,
                    new VmInstr(_instrCodesDb.Stloc_, Stloc_)
                },
                {
                    _instrCodesDb.Stind_i4_.Id,
                    new VmInstr(_instrCodesDb.Stind_i4_, Stind_i4_)
                },
                {
                    _instrCodesDb.Callvirt_.Id,
                    new VmInstr(_instrCodesDb.Callvirt_, Callvirt_)
                },
                {
                    _instrCodesDb.Stelem_i2_.Id,
                    new VmInstr(_instrCodesDb.Stelem_i2_, Stelem_i2_)
                },
                {
                    _instrCodesDb.Conv_ovf_u_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u_, Conv_ovf_u_)
                },
                {
                    _instrCodesDb.Cpobj_.Id,
                    new VmInstr(_instrCodesDb.Cpobj_, Cpobj_)
                },
                {
                    _instrCodesDb.Rem_.Id,
                    new VmInstr(_instrCodesDb.Rem_, Rem_)
                },
                {
                    _instrCodesDb.Stind_r8_.Id,
                    new VmInstr(_instrCodesDb.Stind_r8_, Stind_r8_)
                },
                {
                    _instrCodesDb.Stloc_1_.Id,
                    new VmInstr(_instrCodesDb.Stloc_1_, Stloc_1_)
                },
                {
                    _instrCodesDb.Conv_ovf_u4_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u4_, Conv_ovf_u4_)
                },
                {
                    _instrCodesDb.Ldc_i4_0_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_0_, Ldc_i4_0_)
                },
                {
                    _instrCodesDb.Stind_i_.Id,
                    new VmInstr(_instrCodesDb.Stind_i_, Stind_i_)
                },
                {
                    _instrCodesDb.Stelem_i8_.Id,
                    new VmInstr(_instrCodesDb.Stelem_i8_, Stelem_i8_)
                },
                {
                    _instrCodesDb.Ldelema_.Id,
                    new VmInstr(_instrCodesDb.Ldelema_, Ldelema_)
                },
                {
                    _instrCodesDb.Ldsflda_.Id,
                    new VmInstr(_instrCodesDb.Ldsflda_, Ldsflda_)
                },
                {
                    _instrCodesDb.Ldsfld_.Id,
                    new VmInstr(_instrCodesDb.Ldsfld_, Ldsfld_)
                },
                {
                    _instrCodesDb.Isinst_.Id,
                    new VmInstr(_instrCodesDb.Isinst_, Isinst_)
                },
                {
                    _instrCodesDb.Conv_i2_.Id,
                    new VmInstr(_instrCodesDb.Conv_i2_, Conv_i2_)
                },
                {
                    _instrCodesDb.Stelem_.Id,
                    new VmInstr(_instrCodesDb.Stelem_, Stelem_)
                },
                {
                    _instrCodesDb.Ldind_r8_.Id,
                    new VmInstr(_instrCodesDb.Ldind_r8_, Ldind_r8_)
                },
                {
                    _instrCodesDb.Ldc_r8_.Id,
                    new VmInstr(_instrCodesDb.Ldc_r8_, Ldc_r8_)
                },
                {
                    _instrCodesDb.Bge_.Id,
                    new VmInstr(_instrCodesDb.Bge_, Bge_)
                },
                {
                    _instrCodesDb.Ldind_i1_.Id,
                    new VmInstr(_instrCodesDb.Ldind_i1_, Ldind_i1_)
                },
                {
                    _instrCodesDb.Ldelem_u1_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_u1_, Ldelem_u1_)
                },
                {
                    _instrCodesDb.Ldstr_.Id,
                    new VmInstr(_instrCodesDb.Ldstr_, Ldstr_)
                },
                {
                    _instrCodesDb.Ldloca_s_.Id,
                    new VmInstr(_instrCodesDb.Ldloca_s_, Ldloca_s_)
                },
                {
                    _instrCodesDb.Ldelem_i8_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_i8_, Ldelem_i8_)
                },
                {
                    _instrCodesDb.Ldc_i4_8_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_8_, Ldc_i4_8_)
                },
                {
                    _instrCodesDb.Blt_.Id,
                    new VmInstr(_instrCodesDb.Blt_, Blt_)
                },
                {
                    _instrCodesDb.Unbox_.Id,
                    new VmInstr(_instrCodesDb.Unbox_, Unbox_)
                },
                {
                    _instrCodesDb.Bge_un_.Id,
                    new VmInstr(_instrCodesDb.Bge_un_, Bge_un_)
                },
                {
                    _instrCodesDb.Ldelem_u2_.Id,
                    new VmInstr(_instrCodesDb.Ldelem_u2_, Ldelem_u2_)
                },
                {
                    _instrCodesDb.Ldind_u2_.Id,
                    new VmInstr(_instrCodesDb.Ldind_u2_, Ldind_u2_)
                },
                {
                    _instrCodesDb.Sub_ovf_un_.Id,
                    new VmInstr(_instrCodesDb.Sub_ovf_un_, Sub_ovf_un_)
                },
                {
                    _instrCodesDb.Ldc_i4_4_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_4_, Ldc_i4_4_)
                },
                {
                    _instrCodesDb.Ldarg_0_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_0_, Ldarg_0_)
                },
                {
                    _instrCodesDb.Rem_un_.Id,
                    new VmInstr(_instrCodesDb.Rem_un_, Rem_un_)
                },
                {
                    _instrCodesDb.Ldloc_1_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_1_, Ldloc_1_)
                },
                {
                    _instrCodesDb.Bne_un_.Id,
                    new VmInstr(_instrCodesDb.Bne_un_, Bne_un_)
                },
                {
                    _instrCodesDb.Conv_ovf_i2_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_i2_un_, Conv_ovf_i2_un_)
                },
                {
                    _instrCodesDb.Ckfinite_.Id,
                    new VmInstr(_instrCodesDb.Ckfinite_, Ckfinite_)
                },
                {
                    _instrCodesDb.Ldobj_.Id,
                    new VmInstr(_instrCodesDb.Ldobj_, Ldobj_)
                },
                {
                    _instrCodesDb.Pop_.Id,
                    new VmInstr(_instrCodesDb.Pop_, Pop_)
                },
                {
                    _instrCodesDb.Constrained_.Id,
                    new VmInstr(_instrCodesDb.Constrained_, Constrained_)
                },
                {
                    _instrCodesDb.Ldc_i4_s_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_s_, Ldc_i4_s_)
                },
                {
                    _instrCodesDb.Ldloc_s_.Id,
                    new VmInstr(_instrCodesDb.Ldloc_s_, Ldloc_s_)
                },
                {
                    _instrCodesDb.Ldarg_2_.Id,
                    new VmInstr(_instrCodesDb.Ldarg_2_, Ldarg_2_)
                },
                {
                    _instrCodesDb.Ldarga_.Id,
                    new VmInstr(_instrCodesDb.Ldarga_, Ldarga_)
                },
                {
                    _instrCodesDb.Conv_i8_.Id,
                    new VmInstr(_instrCodesDb.Conv_i8_, Conv_i8_)
                },
                {
                    _instrCodesDb.Br_.Id,
                    new VmInstr(_instrCodesDb.Br_, Br_)
                },
                {
                    _instrCodesDb.Ldc_i4_3_.Id,
                    new VmInstr(_instrCodesDb.Ldc_i4_3_, Ldc_i4_3_)
                },
                {
                    _instrCodesDb.Mul_ovf_.Id,
                    new VmInstr(_instrCodesDb.Mul_ovf_, Mul_ovf_)
                },
                {
                    _instrCodesDb.Shl_.Id,
                    new VmInstr(_instrCodesDb.Shl_, Shl_)
                },
                {
                    _instrCodesDb.Castclass_.Id,
                    new VmInstr(_instrCodesDb.Castclass_, Castclass_)
                },
                {
                    _instrCodesDb.Jmp_.Id,
                    new VmInstr(_instrCodesDb.Jmp_, Jmp_)
                },
                {
                    _instrCodesDb.Beq_.Id,
                    new VmInstr(_instrCodesDb.Beq_, Beq_)
                },
                {
                    _instrCodesDb.Conv_r4_.Id,
                    new VmInstr(_instrCodesDb.Conv_r4_, Conv_r4_)
                },
                {
                    _instrCodesDb.Ble_.Id,
                    new VmInstr(_instrCodesDb.Ble_, Ble_)
                },
                {
                    _instrCodesDb.Conv_ovf_u2_un_.Id,
                    new VmInstr(_instrCodesDb.Conv_ovf_u2_un_, Conv_ovf_u2_un_)
                },
                {
                    _instrCodesDb.Call_.Id,
                    new VmInstr(_instrCodesDb.Call_, Call_)
                },
                {
                    _instrCodesDb.Not_.Id,
                    new VmInstr(_instrCodesDb.Not_, Not_)
                }
            };
        }
    }

    // Token: 0x02000042 RID: 66
    public sealed class LocalVarType // \u0008
    {
        // Token: 0x060002E4 RID: 740 RVA: 0x00013F08 File Offset: 0x00012108
        // Token: 0x060002E5 RID: 741 RVA: 0x00013F10 File Offset: 0x00012110
        // Token: 0x04000171 RID: 369
        public int TypeId /* \u0002 */ { get; set; }
    }

    // Token: 0x0200004D RID: 77
    public sealed class ArgTypeToOutput // \u000E
    {
        // Token: 0x06000321 RID: 801 RVA: 0x00014C74 File Offset: 0x00012E74
        // Token: 0x06000322 RID: 802 RVA: 0x00014C7C File Offset: 0x00012E7C
        // Token: 0x0400017C RID: 380
        public int TypeId /* \u0002 */ { get; set; }

        // Token: 0x06000323 RID: 803 RVA: 0x00014C88 File Offset: 0x00012E88
        // Token: 0x06000324 RID: 804 RVA: 0x00014C90 File Offset: 0x00012E90
        // Token: 0x0400017D RID: 381
        public bool IsOutput /* \u0003 */ { get; set; }
    }

    // Token: 0x0200005E RID: 94
    internal sealed class CatchBlock
    {
        // Token: 0x0600036B RID: 875 RVA: 0x00015AFC File Offset: 0x00013CFC
        // Token: 0x0600036C RID: 876 RVA: 0x00015B04 File Offset: 0x00013D04
        // Token: 0x04000189 RID: 393
        public byte Kind { get; set; }

        // Token: 0x0600036D RID: 877 RVA: 0x00015B10 File Offset: 0x00013D10
        // Token: 0x0600036E RID: 878 RVA: 0x00015B18 File Offset: 0x00013D18
        // Token: 0x0400018A RID: 394
        public int ExcTypeId { get; set; }

        // Token: 0x0600036F RID: 879 RVA: 0x00015B24 File Offset: 0x00013D24
        // Token: 0x06000370 RID: 880 RVA: 0x00015B2C File Offset: 0x00013D2C
        // Token: 0x0400018B RID: 395
        public uint PosKind4 { get; set; }

        // Token: 0x06000371 RID: 881 RVA: 0x00015B38 File Offset: 0x00013D38
        // Token: 0x06000372 RID: 882 RVA: 0x00015B40 File Offset: 0x00013D40
        // Token: 0x0400018C RID: 396
        public uint Start { get; set; }

        // Token: 0x06000373 RID: 883 RVA: 0x00015B4C File Offset: 0x00013D4C
        // Token: 0x06000374 RID: 884 RVA: 0x00015B54 File Offset: 0x00013D54
        // Token: 0x0400018D RID: 397
        public uint Pos { get; set; } // \u0005

        // Token: 0x06000375 RID: 885 RVA: 0x00015B60 File Offset: 0x00013D60
        // Token: 0x06000376 RID: 886 RVA: 0x00015B68 File Offset: 0x00013D68
        // Token: 0x0400018E RID: 398
        public uint Len { get; set; }
    }

    // Token: 0x02000049 RID: 73
    internal abstract class VmTokenInfo // \u0008\u2006
    {
        internal enum Kind : byte
        {
            Class0, Field1, Method2, String3, MethodRef4
        }

        // Token: 0x0600030D RID: 781
	    public abstract Kind TokenKind(); // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
    }

    // Token: 0x02000017 RID: 23
    internal sealed class UniversalTokenInfo // \u0003\u2008
    {
        // Token: 0x06000097 RID: 151 RVA: 0x00003E9C File Offset: 0x0000209C
        // Token: 0x06000098 RID: 152 RVA: 0x00003EA4 File Offset: 0x000020A4
        // Token: 0x04000020 RID: 32
        public byte IsVm { get; set; }

        // Token: 0x06000099 RID: 153 RVA: 0x00003EB0 File Offset: 0x000020B0
        // Token: 0x0600009A RID: 154 RVA: 0x00003EB8 File Offset: 0x000020B8
        // Token: 0x04000021 RID: 33
        public int MetadataToken { get; set; }

	    // Token: 0x0600009B RID: 155 RVA: 0x00003EC4 File Offset: 0x000020C4
	    // Token: 0x0600009C RID: 156 RVA: 0x00003ECC File Offset: 0x000020CC
	    // Token: 0x04000022 RID: 34
	    public VmTokenInfo VmToken { get; set; }
    }
    
    // Token: 0x02000016 RID: 22
    internal sealed class VmMethodRefTokenInfo : VmTokenInfo // \u0003\u2007
    {
        // Token: 0x06000091 RID: 145 RVA: 0x00003E68 File Offset: 0x00002068
        // Token: 0x06000092 RID: 146 RVA: 0x00003E70 File Offset: 0x00002070
        // Token: 0x0400001E RID: 30
        public int Flags { get; set; }

        // Token: 0x06000093 RID: 147 RVA: 0x00003E7C File Offset: 0x0000207C
        // Token: 0x06000094 RID: 148 RVA: 0x00003E84 File Offset: 0x00002084
        // Token: 0x0400001F RID: 31
        public int Pos { get; set; }

	    // Token: 0x06000095 RID: 149 RVA: 0x00003E90 File Offset: 0x00002090
	    public override Kind TokenKind() // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return Kind.MethodRef4;
	    }
    }

    // Token: 0x0200004A RID: 74
    internal sealed class VmMethodTokenInfo : VmTokenInfo // \u0008\u2007
    {
        // Token: 0x0600030F RID: 783 RVA: 0x00014BB0 File Offset: 0x00012DB0
        // Token: 0x06000310 RID: 784 RVA: 0x00014BB8 File Offset: 0x00012DB8
        // Token: 0x04000176 RID: 374
        public byte Flags { get; set; }

        // Token: 0x06000311 RID: 785 RVA: 0x00014BC4 File Offset: 0x00012DC4
        public bool IsStatic() // \u0002
        {
		    return (Flags & 2) > 0;
	    }

	    // Token: 0x06000312 RID: 786 RVA: 0x00014BD4 File Offset: 0x00012DD4
	    public bool IsGeneric() // \u0003
        {
		    return (Flags & 1) > 0;
	    }

        // Token: 0x06000313 RID: 787 RVA: 0x00014BE4 File Offset: 0x00012DE4
        // Token: 0x06000314 RID: 788 RVA: 0x00014BEC File Offset: 0x00012DEC
        // Token: 0x04000177 RID: 375
        public UniversalTokenInfo Class { get; set; }

        // Token: 0x06000315 RID: 789 RVA: 0x00014BF8 File Offset: 0x00012DF8
        // Token: 0x06000316 RID: 790 RVA: 0x00014C00 File Offset: 0x00012E00
        // Token: 0x04000178 RID: 376
        public string Name { get; set; }

        // Token: 0x06000317 RID: 791 RVA: 0x00014C0C File Offset: 0x00012E0C
        // Token: 0x06000318 RID: 792 RVA: 0x00014C14 File Offset: 0x00012E14
        // Token: 0x04000179 RID: 377
        public UniversalTokenInfo[] Parameters { get; set; }

        // Token: 0x06000319 RID: 793 RVA: 0x00014C20 File Offset: 0x00012E20
        // Token: 0x0600031A RID: 794 RVA: 0x00014C28 File Offset: 0x00012E28
        // Token: 0x0400017A RID: 378
        public UniversalTokenInfo[] GenericArguments { get; set; }

        // Token: 0x0600031B RID: 795 RVA: 0x00014C34 File Offset: 0x00012E34
        // Token: 0x0600031C RID: 796 RVA: 0x00014C3C File Offset: 0x00012E3C
        // Token: 0x0400017B RID: 379
        public UniversalTokenInfo ReturnType { get; set; }

        // Token: 0x0600031D RID: 797 RVA: 0x00014C48 File Offset: 0x00012E48
        public override Kind TokenKind() // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return Kind.Method2;
	    }
    }

    // Token: 0x02000056 RID: 86
    internal sealed class VmStringTokenInfo : VmTokenInfo // \u000E\u2007
    {
	    // Token: 0x06000343 RID: 835 RVA: 0x000153CC File Offset: 0x000135CC
	    // Token: 0x06000344 RID: 836 RVA: 0x000153D4 File Offset: 0x000135D4
	    // Token: 0x04000185 RID: 389
	    public string Value { get; set; }

	    // Token: 0x06000345 RID: 837 RVA: 0x000153E0 File Offset: 0x000135E0
	    public override Kind TokenKind() // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return Kind.String3;
	    }
    }

    // Token: 0x02000060 RID: 96
    internal sealed class VmFieldTokenInfo : VmTokenInfo // \u000F\u2006
    {
        // Token: 0x06000382 RID: 898 RVA: 0x00015C38 File Offset: 0x00013E38
        // Token: 0x06000383 RID: 899 RVA: 0x00015C40 File Offset: 0x00013E40
        // Token: 0x04000191 RID: 401
        public UniversalTokenInfo Class { get; set; }

        // Token: 0x06000384 RID: 900 RVA: 0x00015C4C File Offset: 0x00013E4C
        // Token: 0x06000385 RID: 901 RVA: 0x00015C54 File Offset: 0x00013E54
        // Token: 0x04000192 RID: 402
        public string Name { get; set; }

	    // Token: 0x06000386 RID: 902 RVA: 0x00015C60 File Offset: 0x00013E60
	    // Token: 0x06000387 RID: 903 RVA: 0x00015C68 File Offset: 0x00013E68
	    // Token: 0x04000193 RID: 403
	    public bool IsStatic { get; set; } 

	    // Token: 0x06000388 RID: 904 RVA: 0x00015C74 File Offset: 0x00013E74
	    public override Kind TokenKind() // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return Kind.Field1;
	    }
    }

    // Token: 0x02000061 RID: 97
    internal sealed class VmClassTokenInfo : VmTokenInfo // \u000F\u2007
    {
        // Token: 0x04000194 RID: 404
        // Token: 0x0600038A RID: 906 RVA: 0x00015C90 File Offset: 0x00013E90
        // Token: 0x0600038B RID: 907 RVA: 0x00015C98 File Offset: 0x00013E98
        public string ClassName { get; set; }

        // Token: 0x0600038C RID: 908 RVA: 0x00015CA4 File Offset: 0x00013EA4
        // Token: 0x04000195 RID: 405
        // Token: 0x0600038D RID: 909 RVA: 0x00015CAC File Offset: 0x00013EAC
        public bool IsOuterClassGeneric { get; set; }

        // Token: 0x0600038E RID: 910 RVA: 0x00015CB8 File Offset: 0x00013EB8
        // Token: 0x04000196 RID: 406
        // Token: 0x0600038F RID: 911 RVA: 0x00015CC0 File Offset: 0x00013EC0
        public bool IsGeneric { get; set; }

        // Token: 0x06000390 RID: 912 RVA: 0x00015CCC File Offset: 0x00013ECC
        // Token: 0x04000197 RID: 407
        // Token: 0x06000391 RID: 913 RVA: 0x00015CD4 File Offset: 0x00013ED4
        public UniversalTokenInfo[] GenericArguments { get; set; }

        // Token: 0x06000392 RID: 914 RVA: 0x00015CE0 File Offset: 0x00013EE0
        // Token: 0x04000198 RID: 408
        // Token: 0x06000393 RID: 915 RVA: 0x00015CE8 File Offset: 0x00013EE8
        public int OuterClassGenericClassIdx { get; set; } = -1;

        // Token: 0x06000394 RID: 916 RVA: 0x00015CF4 File Offset: 0x00013EF4
        // Token: 0x04000199 RID: 409
        // Token: 0x06000395 RID: 917 RVA: 0x00015CFC File Offset: 0x00013EFC
        public int OuterClassGenericMethodIdx { get; set; } = -1;

        // Token: 0x06000396 RID: 918 RVA: 0x00015D08 File Offset: 0x00013F08
        public override Kind TokenKind() // \u0008\u2006\u2008\u2000\u2002\u200A\u0002
	    {
		    return Kind.Class0;
	    }
    }
}
