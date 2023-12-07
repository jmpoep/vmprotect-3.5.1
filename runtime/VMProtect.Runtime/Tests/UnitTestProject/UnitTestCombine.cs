using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using UnitTestProject.RefVm;
using VMProtect;
using OpCodes = System.Reflection.Emit.OpCodes;

namespace UnitTestProject
{
    [TestClass]
    public class UnitTestCombine
    {
        // шаблон для окружающего кода для некой инструкции
        public enum CodeTemplate
        {
            BranchTwoOpBool, // 2 операнда на стеке, 2 ветки исполнения (одна возвращает true, другая false)
            SingleOpAny,     // 1 операнд на стеке, инструкция, возвращающая разные типы
            TwoOpBool,       // 2 операнда на стеке, инструкция, возвращающая bool
            TwoOpAny,        // 2 операнда на стеке, инструкция, возвращающая разные типы
        }

        [TestMethod]
        public void TestMethodRefVm()
        {
            InitStrings();
            TestMethod(new MsilToVmTestCompilerRefVm());
        }

        [TestMethod]
        public void TestMethodVmp()
        {
            TestMethod(new MsilToVmTestCompilerVmp());
        }

        [SuppressMessage("ReSharper", "RedundantCast")]
        public void TestMethod(MsilToVmTestCompiler c)
        {
            // типовые и крайние значения следующих типов:
            // SByte, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64,
            // float, double,
            // LongEnum, ULongEnum, Char, SByteEnum, ByteEnum, ShortEnum, UShortEnum, IntEnum, UIntEnum
            // bool, object, IntPtr, UIntPtr
            //TODO: decimal
            #region valsVector
            // ReSharper disable once RedundantExplicitArrayCreation
            var valsVector = new object []
            {
                (SByte)0, (SByte)1, (SByte)(-1), SByte.MaxValue, SByte.MinValue, (SByte)0x55, // SByte
                (Byte)0, (Byte)1, Byte.MaxValue, (Byte)0xAA, // Byte
                (Int16)0, (Int16)1, (Int16)(-1), Int16.MaxValue, Int16.MinValue, (Int16)0x55AA, // Int16
                (UInt16)0, (UInt16)1, UInt16.MaxValue, (UInt16)0xAA55, // UInt16
                0, 1, -1, Int32.MaxValue, Int32.MinValue, 0x55AA55AA, // Int32
                (UInt32)0, (UInt32)1, UInt32.MaxValue, 0xAA5555AA, // UInt32
                (Int64)0, (Int64)1, (Int64)(-1), Int64.MaxValue, Int64.MinValue, 0x55AA55AA55AA55AA, // Int64
                (UInt64)0, (UInt64)1, UInt64.MaxValue, 0xAA5555AAAA5555AA, // UInt64
                (float)0, (float)1, (float)(-1), float.MaxValue, float.MinValue, float.Epsilon, float.NaN,
                float.NegativeInfinity, float.PositiveInfinity, (float)(5678.1234),(float)(1234.5678), (float)(-5678.1234), (float)(-1234.5678),
                (float)(2147483647.0 * 3.0), (float)(-2147483647.0 * 3.0), (float)(9223372336854775807.0 * 3.0), -(float)(9223372336854775807.0 * 3.0), // float
                (double)0, (double)1, (double)(-1), double.MaxValue, double.MinValue, double.Epsilon, double.NaN,
                double.NegativeInfinity, double.PositiveInfinity, 5678.1234, 1234.5678, -5678.1234, -1234.5678,
                (double)(2147483647.0 * 3.0), (double)(-2147483647.0 * 3.0), (double)9223372336854775807.0 * 3.0, -(double)9223372336854775807.0 * 3.0, // double
                (MsilToVmTestCompiler.LongEnum)0, (MsilToVmTestCompiler.LongEnum)1, (MsilToVmTestCompiler.LongEnum)(-1), (MsilToVmTestCompiler.LongEnum)Int64.MaxValue, (MsilToVmTestCompiler.LongEnum)Int64.MinValue, (MsilToVmTestCompiler.LongEnum)0x55AA55AA55AA55AA, // LongEnum
                (MsilToVmTestCompiler.ULongEnum)0, (MsilToVmTestCompiler.ULongEnum)1, (MsilToVmTestCompiler.ULongEnum)UInt64.MaxValue, (MsilToVmTestCompiler.ULongEnum)0xAA5555AAAA5555AA, // ULongEnum
                (Char)0, (Char)1, 'Ю', Char.MaxValue, (Char)0xAA55, // Char
                (MsilToVmTestCompiler.SByteEnum)0, (MsilToVmTestCompiler.SByteEnum)1, (MsilToVmTestCompiler.SByteEnum)(-1), (MsilToVmTestCompiler.SByteEnum)sbyte.MaxValue, (MsilToVmTestCompiler.SByteEnum)sbyte.MinValue, (MsilToVmTestCompiler.SByteEnum)0x55, // SByteEnum
                (MsilToVmTestCompiler.ByteEnum)0, (MsilToVmTestCompiler.ByteEnum)1, (MsilToVmTestCompiler.ByteEnum)Byte.MaxValue, (MsilToVmTestCompiler.ByteEnum)0xAA, // ByteEnum
                (MsilToVmTestCompiler.ShortEnum)0, (MsilToVmTestCompiler.ShortEnum)1, (MsilToVmTestCompiler.ShortEnum)(-1), (MsilToVmTestCompiler.ShortEnum)short.MaxValue, (MsilToVmTestCompiler.ShortEnum)short.MinValue, (MsilToVmTestCompiler.ShortEnum)0x55AA, // ShortEnum
                (MsilToVmTestCompiler.UShortEnum)0, (MsilToVmTestCompiler.UShortEnum)1, (MsilToVmTestCompiler.UShortEnum)ushort.MaxValue, (MsilToVmTestCompiler.UShortEnum)0xAA55, // UShortEnum
                (MsilToVmTestCompiler.IntEnum)0, (MsilToVmTestCompiler.IntEnum)1, (MsilToVmTestCompiler.IntEnum)(-1), (MsilToVmTestCompiler.IntEnum)int.MaxValue, (MsilToVmTestCompiler.IntEnum)int.MinValue, (MsilToVmTestCompiler.IntEnum)0x55AA55AA, // IntEnum
                (MsilToVmTestCompiler.UIntEnum)0, (MsilToVmTestCompiler.UIntEnum)1, (MsilToVmTestCompiler.UIntEnum)uint.MaxValue, (MsilToVmTestCompiler.UIntEnum)0xAA5555AA, // UIntEnum
                true, false, // bool
                new object(),  //object
                new IntPtr(0), new UIntPtr(0), // IntPtr, UIntPtr
            };
#endregion
            var errCounters = new int[(int)MsilToVmTestCompiler.InvokeTestCombineError.Cnt];
            var operations = new List<KeyValuePair<OpCode, CodeTemplate>>
            {
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Add, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Add_Ovf, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Add_Ovf_Un, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.And, CodeTemplate.TwoOpAny),
                //Arglist не годится для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Beq_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Bge_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Bge_Un_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Bgt_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Bgt_Un_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Ble_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Ble_Un_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Blt_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Blt_Un_S, CodeTemplate.BranchTwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Bne_Un_S, CodeTemplate.BranchTwoOpBool),
                //TODO Box
                //Br_S тестируется неявно в этом тесте
                //Break не годится для этого теста
                //TODO Brfalse_S
                //TODO Brtrue_S
                //Call тестируется в TestNewobj
                //Calli Callvirt Castclass не годятся для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Ceq, CodeTemplate.TwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Cgt, CodeTemplate.TwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Cgt_Un, CodeTemplate.TwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Ckfinite, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Clt, CodeTemplate.TwoOpBool),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Clt_Un, CodeTemplate.TwoOpBool),
                //Constrained не годится для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_I, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_I1, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_I2, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_I4, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_I8, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I1, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I1_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I2, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I2_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I4, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I4_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I8, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I8_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_I_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U1, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U1_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U2, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U2_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U4, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U4_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U8, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U8_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_Ovf_U_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_R4, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_R8, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_R_Un, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U1, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U2, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U4, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U8, CodeTemplate.SingleOpAny),
                //Cpblk Cpobj не годятся для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Div, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Div_Un, CodeTemplate.TwoOpAny),
                //TODO: Dup
                //Endfilter Endfinally Initblk Initobj Isinst Jmp Ldarg не годятся для этого теста
                //Ldarg_0 тестируется неявно в этом тесте
                //Ldarg_1 тестируется неявно в этом тесте
                //Ldarg_2 Ldarg_3 Ldarg_S Ldarga не годятся для этого теста
                //Ldarga_S тестируется в TestNewobj
                //Ldc_I4 не годится для этого теста
                //Ldc_I4_0 тестируется неявно в этом тесте
                //Ldc_I4_1 тестируется неявно в этом тесте
                //Ldc_I4_2 Ldc_I4_3 Ldc_I4_4 Ldc_I4_5 Ldc_I4_6 Ldc_I4_7 Ldc_I4_8 Ldc_I4_M1 Ldc_I4_S Ldc_I8 Ldc_R4 Ldc_R8 не годятся для этого теста 
                //Ldelem* Ldfld Ldflda Ldftn Ldind_* Ldlen Ldloc* Ldnull Ldobj Ldsfld Ldsflda Ldstr Ldtoken Ldvirtftn Leave Leave_S Localloc Mkrefany не годятся для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Mul, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Mul_Ovf, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Mul_Ovf_Un, CodeTemplate.TwoOpAny),
                //TODO new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Neg, CodeTemplate.SingleOp),
                //Newarr не годится для этого теста
                //Newobj тестируется в TestNewobj
                //Nop не годится для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Not, CodeTemplate.SingleOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Or, CodeTemplate.TwoOpAny),
                //Pop Prefix* Readonly Refanytype Refanyval не годятся для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Rem, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Rem_Un, CodeTemplate.TwoOpAny),
                //Ret тестируется неявно в этом тесте
                //Rethrow не годится для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Shl, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Shr, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Shr_Un, CodeTemplate.TwoOpAny),
                //Sizeof
                //St* не годятся для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Sub, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Sub_Ovf, CodeTemplate.TwoOpAny),
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Sub_Ovf_Un, CodeTemplate.TwoOpAny),
                //Switch Tailcall Throw Unaligned
                //TODO Unbox
                //TODO Unbox_Any
                //Volatile не годится для этого теста
                new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Xor, CodeTemplate.TwoOpAny),
            };
            foreach (var op in operations)
            {
                switch (op.Value)
                {
                    case CodeTemplate.BranchTwoOpBool:
                    case CodeTemplate.TwoOpBool:
                    case CodeTemplate.TwoOpAny:
                        TestOperationTwoOp(c, op, valsVector, errCounters);
                        break;
                    case CodeTemplate.SingleOpAny:
                        TestOperationSingleOp(c, op, valsVector, errCounters);
                        break;
                    default:
                        Assert.Fail("Unsupported op.Value: {0}", op.Value);
                        break;
                }
            }
            Console.Error.WriteLine("--- {0} results ---", IntPtr.Size == 4 ? "x86" : "x64");
            var sum = 0;
            for (var i = 0; i < errCounters.Length; i++)
            {
                Console.Error.WriteLine("{0}: {1}", (MsilToVmTestCompiler.InvokeTestCombineError)i, errCounters[i]);
                sum += errCounters[i];
            }
            Console.Error.WriteLine("--- TOTAL: {0} ---", sum);
            Assert.AreEqual(sum, errCounters[(int)MsilToVmTestCompiler.InvokeTestCombineError.NoError] +
                errCounters[(int)MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx]);
            Console.Error.WriteLine(sum == errCounters[(int) MsilToVmTestCompiler.InvokeTestCombineError.NoError] +
                                    errCounters[(int) MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx]
                ? "TEST PASSED"
                : "TEST FAILED");
        }

        private void TestOperationTwoOp(MsilToVmTestCompiler c, KeyValuePair<OpCode, CodeTemplate> op, object[] valsVector, int[] errCounters)
        {
            foreach (var i in valsVector)
            {
                foreach (var j in valsVector)
                {
                    object[] parameters = {i, j};
                    string err, istr = i.ToString(), jstr = j.ToString();
                    if (i is Char)
                        istr = $@"'\{Convert.ToInt32(i)}'";
                    if (j is Char)
                        jstr = $@"'\{Convert.ToInt32(j)}'";

                    var res = TestOperation(c, op, parameters, errCounters, out err);
                    if(res > MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx)
                        Console.Error.WriteLine("{0}.{1}(({4}){2}, ({5}){3}) = {6}", op.Key, op.Value,
                            istr, jstr, i.GetType().FullName, j.GetType().FullName, err);
                }
            }
        }

        private void TestOperationSingleOp(MsilToVmTestCompiler c, KeyValuePair<OpCode, CodeTemplate> op, object[] valsVector, int[] errCounters)
        {
            foreach (object i in valsVector)
            {
                object[] parameters = { i };
                string err, istr = i.ToString();
                if (i is Char)
                    istr = $@"'\{Convert.ToInt32(i)}'";
                var res = TestOperation(c, op, parameters, errCounters, out err);
                if (res > MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx)
                    Console.Error.WriteLine("{0}.{1}(({3}){2}) = {4}", op.Key, op.Value, istr, i.GetType().FullName, err);
            }
        }

        [TestMethod]
        public void TempRefVm()
        {
            InitStrings();
            var errCounters = new int[(int)MsilToVmTestCompiler.InvokeTestCombineError.Cnt];
            string err;
            TestOperation(new MsilToVmTestCompilerRefVm(), new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Add_Ovf_Un, CodeTemplate.TwoOpAny),
                // ReSharper disable once RedundantExplicitArrayCreation
                new object[] { new object(), (System.SByte)0 }, errCounters, out err);
            Assert.AreEqual(1, errCounters[0] + errCounters[1]);
        }

        [TestMethod]
        public void TempVmp()
        {
            var errCounters = new int[(int)MsilToVmTestCompiler.InvokeTestCombineError.Cnt];
            string err;
            TestOperation(new MsilToVmTestCompilerVmp(), new KeyValuePair<OpCode, CodeTemplate>(OpCodes.Conv_U4, CodeTemplate.SingleOpAny),
                // ReSharper disable once RedundantCast
                new object[] { (Int32)0 }, errCounters, out err);
            Console.WriteLine(err);
            Assert.AreEqual(1, errCounters[0] + errCounters[1]);
        }

        private static void InitStrings()
        {
            StringDecryptor.SetString(-1550347095, "The value is not finite real number.");
        }

        private MsilToVmTestCompiler.InvokeTestCombineError TestOperation(
            MsilToVmTestCompiler c, KeyValuePair<OpCode, CodeTemplate> op, object[] parameters, int [] errCounters, out string err)
        {
            DynamicMethod dyn;
            err = "";
            var types = parameters.Select(o => o.GetType()).ToArray();
            switch (op.Value)
            {
                case CodeTemplate.BranchTwoOpBool:
                    dyn = DynBranch2Op(op, types);
                    break;
                case CodeTemplate.SingleOpAny:
                    dyn = DynSingleOpAny(op, types);
                    break;
                case CodeTemplate.TwoOpBool:
                    dyn = Dyn2OpBool(op, types);
                    break;
                case CodeTemplate.TwoOpAny:
                    dyn = Dyn2OpAny(op, types);
                    break;
                default:
                    Assert.Fail("Unsupported op.Value: {0}", op.Value);
                    // ReSharper disable once HeuristicUnreachableCode
                    return MsilToVmTestCompiler.InvokeTestCombineError.Cnt;
            }
            object dynRet, vmRet;
            var ret = c.InvokeTestCombine(dyn, parameters, out dynRet, out vmRet, out err);
            // TODO: для регрессии надо как-то научиться закладывать сюда все правильные результаты
            if (errCounters != null)
            {
                if (ret > MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx &&
                    ExtendedEcma335.Check(op, parameters, vmRet))
                {
                    ret = MsilToVmTestCompiler.InvokeTestCombineError.NoErrorByStdEx;
                }

                errCounters[(int)ret]++;
            }
            return ret;
        }

        private DynamicMethod DynSingleOpAny(KeyValuePair<OpCode, CodeTemplate> op, Type[] types)
        {
            Type pushedT;
            switch (op.Key.StackBehaviourPush)
            {
                default:
                    Assert.Fail("DynSingleOp unsupported StackBehaviourPush={0}", op.Key.StackBehaviourPush);
                    // ReSharper disable once HeuristicUnreachableCode
                    return null;
                case StackBehaviour.Pushi:
                    pushedT = typeof(Int32);
                    break;
                case StackBehaviour.Pushi8:
                    pushedT = typeof(Int64);
                    break;
                case StackBehaviour.Pushr4:
                    pushedT = typeof(float);
                    break;
                case StackBehaviour.Pushr8:
                    pushedT = typeof(double);
                    break;
                case StackBehaviour.Push1:
                    pushedT = CompatibleType(types[0], types[0], false);
                    if (pushedT == typeof(object)) pushedT = typeof(IntPtr);
                    break;
            }
            if ((IntPtr.Size == 8) && (op.Key.Equals(OpCodes.Conv_I) || op.Key.Equals(OpCodes.Conv_Ovf_I) || op.Key.Equals(OpCodes.Conv_Ovf_I_Un)))
                pushedT = typeof(Int64);
            if (op.Key.Equals(OpCodes.Conv_U) ||op.Key.Equals(OpCodes.Conv_Ovf_U) || op.Key.Equals(OpCodes.Conv_Ovf_U_Un))
                pushedT = (IntPtr.Size == 8) ? typeof(UInt64) : typeof(UInt32);
            var dyn = new DynamicMethod(op.Key.Name, pushedT, types, typeof(void), true);
            var gen = dyn.GetILGenerator();

            gen.Emit(OpCodes.Ldarg_0);
            gen.Emit(op.Key);
            gen.Emit(OpCodes.Ret);

            return dyn;
        }

        internal class ClassNewobj
        {
            public const long Check = 0xF00D;
            public long Ptr;
            public unsafe ClassNewobj(byte *ptr)
            {
                Ptr = (long) ptr;
            } 
        }
        public static unsafe object StatNewobj(IntPtr ptr)
        {
            return new ClassNewobj((byte *)ptr.ToPointer());
        }

        [TestMethod]
        [SuppressMessage("ReSharper", "PossibleNullReferenceException")]
        public void TestNewobj() //https://ursoft.asuscomm.com:8080/browse/VMP-134?focusedCommentId=15312&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-15312
        {
            InitStrings();
            MsilToVmTestCompiler c = new MsilToVmTestCompilerVmp();
            var mi = typeof (UnitTestCombine).GetMethod("StatNewobj");
            var no = (ClassNewobj)c.Invoke(new object[] {new IntPtr(ClassNewobj.Check)},
                c.CreateVmStream(mi.ReturnType, mi.GetParameters(),
                    mi.GetMethodBody().LocalVariables.Select(o => o.LocalType).ToArray(), mi.GetMethodBody().GetILAsByteArray()));
            Assert.AreEqual(ClassNewobj.Check, no.Ptr);
        }

        private static DynamicMethod DynBranch2Op(KeyValuePair<OpCode, CodeTemplate> op, Type[] types)
        {
            var dyn = new DynamicMethod(op.Key.Name, typeof(bool), types, typeof(void), true);
            var gen = dyn.GetILGenerator();
            var retTrue = gen.DefineLabel();
            var endOfMthd = gen.DefineLabel();

            gen.Emit(OpCodes.Ldarg_1);
            gen.Emit(OpCodes.Ldarg_0);
            gen.Emit(op.Key, retTrue);

            gen.Emit(OpCodes.Ldc_I4_0);
            gen.Emit(OpCodes.Br_S, endOfMthd);

            gen.MarkLabel(retTrue);
            gen.Emit(OpCodes.Ldc_I4_1);

            gen.MarkLabel(endOfMthd);
            gen.Emit(OpCodes.Ret);
            return dyn;
        }

        private static DynamicMethod Dyn2OpAny(KeyValuePair<OpCode, CodeTemplate> op, Type[] types)
        {
            Type pushedT;
            switch (op.Key.StackBehaviourPush)
            {
                default:
                    Assert.Fail("Dyn2OpAny unsupported StackBehaviourPush={0}", op.Key.StackBehaviourPush);
                    // ReSharper disable once HeuristicUnreachableCode
                    return null;
                case StackBehaviour.Pushi:
                    pushedT = typeof(Int32);
                    break;
                case StackBehaviour.Pushi8:
                    pushedT = typeof(Int64);
                    break;
                case StackBehaviour.Pushr4:
                    pushedT = typeof(float);
                    break;
                case StackBehaviour.Pushr8:
                    pushedT = typeof(double);
                    break;
                case StackBehaviour.Push1:
                    pushedT = (op.Key.Value == OpCodes.Shl.Value || op.Key.Value == OpCodes.Shr.Value || op.Key.Value == OpCodes.Shr_Un.Value) ? types[1]: types[0];
                    pushedT = CompatibleType(pushedT, types[1], !op.Key.Name.EndsWith("_Un"));
                    if (pushedT == typeof (object)) pushedT = typeof(IntPtr);
                    break;
            }
            return Dyn2Op(op, types, pushedT);
        }

        public static Type CompatibleType(Type type0, Type type1, bool signed)
        {
            if (type0.IsEnum) type0 = type0.GetEnumUnderlyingType();
            if (type1.IsEnum) type1 = type1.GetEnumUnderlyingType();
            if (type0 == typeof (bool) && type1 == typeof (bool)) return typeof (sbyte); // true + true = 2
            if (type0 == typeof (IntPtr)) type0 = IntPtr.Size == 4 ? typeof (Int32) : typeof (Int64);
            if (type0 == typeof (UIntPtr)) type0 = IntPtr.Size == 4 ? typeof (UInt32) : typeof (UInt64);
            if (type1 == typeof (IntPtr)) type1 = IntPtr.Size == 4 ? typeof (Int32) : typeof (Int64);
            if (type1 == typeof (UIntPtr)) type1 = IntPtr.Size == 4 ? typeof (UInt32) : typeof (UInt64);
            if (type0 == type1) return type0;
            // возвращаем наиболее широкий из типов
            return TypeWidth(type0) >= TypeWidth(type1) ? EnsureSign(type0, signed) : EnsureSign(type1, signed);
        }

        private static Type EnsureSign(Type t, bool signed)
        {
            if (t == typeof (Int32)) return signed ? t : typeof (UInt32);
            if (t == typeof (UInt32)) return signed ? typeof (Int32) : t;
            if (t == typeof (Int64)) return signed ? t : typeof (UInt64);
            if (t == typeof (UInt64)) return signed ? typeof (Int64) : t;
            return t;
        }

        private static int TypeWidth(Type t)
        {
            if (t.IsEnum) t = t.GetEnumUnderlyingType();
            if (t == typeof (bool)) return 1;
            if (t == typeof (byte) || t == typeof (sbyte)) return 2;
            if (t == typeof (short) || t == typeof (ushort) || t == typeof(Char)) return 3;
            if (t == typeof (int) || t == typeof (uint)) return 4;
            if (t == typeof (long) || t == typeof (ulong)) return 5;
            if (t == typeof(float)) return 6;
            if (t == typeof(double)) return 7;
            return 8; // object etc
        }

        private static DynamicMethod Dyn2OpBool(KeyValuePair<OpCode, CodeTemplate> op, Type[] types)
        {
            Type pushedT = typeof(bool);
            return Dyn2Op(op, types, pushedT);
        }

        private static DynamicMethod Dyn2Op(KeyValuePair<OpCode, CodeTemplate> op, Type[] types, Type ret)
        {
            var dyn = new DynamicMethod(op.Key.Name, ret, types, typeof(void), true);
            var gen = dyn.GetILGenerator();

            gen.Emit(OpCodes.Ldarg_1);
            gen.Emit(OpCodes.Ldarg_0);
            gen.Emit(op.Key);

            gen.Emit(OpCodes.Ret);
            return dyn;
        }
    }

    internal class MsilToVmTestCompilerVmp : MsilToVmTestCompiler
    {
        #region opcodes
        internal static readonly Dictionary<short, KeyValuePair<OpCode, VMProtect.OpCodes>> IlToVmInstrInfo = new Dictionary<short, KeyValuePair<OpCode, VMProtect.OpCodes>>()
        {
            { OpCodes.Add.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Add, VMProtect.OpCodes.icAdd) },
            { OpCodes.Add_Ovf.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Add_Ovf, VMProtect.OpCodes.icAdd_ovf) },
            { OpCodes.Add_Ovf_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Add_Ovf_Un, VMProtect.OpCodes.icAdd_ovf_un) },
            { OpCodes.And.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.And, VMProtect.OpCodes.icAnd) },
            //{ OpCodes.Arglist.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Arglist, VMProtect.OpCodes.icArglist) },
            { OpCodes.Beq_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Beq_S, VMProtect.OpCodes.icBeq) },
            { OpCodes.Bge_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Bge_S, VMProtect.OpCodes.icBge) },
            { OpCodes.Bge_Un_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Bge_Un_S, VMProtect.OpCodes.icBge_un) },
            { OpCodes.Bgt_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Bgt_S, VMProtect.OpCodes.icBgt) },
            { OpCodes.Bgt_Un_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Bgt_Un_S, VMProtect.OpCodes.icBgt_un) },
            { OpCodes.Ble_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ble_S, VMProtect.OpCodes.icBle) },
            { OpCodes.Ble_Un_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ble_Un_S, VMProtect.OpCodes.icBle_un) },
            { OpCodes.Blt_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Blt_S, VMProtect.OpCodes.icBlt) },
            { OpCodes.Blt_Un_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Blt_Un_S, VMProtect.OpCodes.icBlt_un) },
            { OpCodes.Bne_Un_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Bne_Un_S, VMProtect.OpCodes.icBne_un) },
            //{ OpCodes.Box.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Box, VMProtect.OpCodes.icBox) },
            { OpCodes.Br_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Br_S, VMProtect.OpCodes.icBr) },
            //{ OpCodes.Break.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Break, VMProtect.OpCodes.icBreak) },
            //{ OpCodes.Brfalse_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Brfalse_S, VMProtect.OpCodes.icBrfalse_S) },
            //{ OpCodes.Brtrue_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Brtrue_S, VMProtect.OpCodes.icBrtrue_S) },
            { OpCodes.Call.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Call, VMProtect.OpCodes.icCall) },
            //{ OpCodes.Calli.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Calli, VMProtect.OpCodes.icCalli) },
            //{ OpCodes.Callvirt.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Callvirt, VMProtect.OpCodes.icCallvirt) },
            //{ OpCodes.Castclass.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Castclass, VMProtect.OpCodes.icCastclass) },
            { OpCodes.Ceq.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ceq, VMProtect.OpCodes.icCeq) },
            { OpCodes.Cgt.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Cgt, VMProtect.OpCodes.icCgt) },
            { OpCodes.Cgt_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Cgt_Un, VMProtect.OpCodes.icCgt_un) },
            { OpCodes.Ckfinite.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ckfinite, VMProtect.OpCodes.icCkfinite) },
            { OpCodes.Clt.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Clt, VMProtect.OpCodes.icClt) },
            { OpCodes.Clt_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Clt_Un, VMProtect.OpCodes.icClt_un) },
            //{ OpCodes.Constrained.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Constrained, VMProtect.OpCodes.icConstrained) },
            { OpCodes.Conv_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_I, VMProtect.OpCodes.icConv_i) },
            { OpCodes.Conv_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_I1, VMProtect.OpCodes.icConv_i1) },
            { OpCodes.Conv_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_I2, VMProtect.OpCodes.icConv_i2) },
            { OpCodes.Conv_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_I4, VMProtect.OpCodes.icConv_i4) },
            { OpCodes.Conv_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_I8, VMProtect.OpCodes.icConv_i8) },
            { OpCodes.Conv_Ovf_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I, VMProtect.OpCodes.icConv_ovf_i) },
            { OpCodes.Conv_Ovf_I_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I_Un, VMProtect.OpCodes.icConv_ovf_i_un) },
            { OpCodes.Conv_Ovf_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I1, VMProtect.OpCodes.icConv_ovf_i1) },
            { OpCodes.Conv_Ovf_I1_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I1_Un, VMProtect.OpCodes.icConv_ovf_i1_un) },
            { OpCodes.Conv_Ovf_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I2, VMProtect.OpCodes.icConv_ovf_i2) },
            { OpCodes.Conv_Ovf_I2_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I2_Un, VMProtect.OpCodes.icConv_ovf_i2_un) },
            { OpCodes.Conv_Ovf_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I4, VMProtect.OpCodes.icConv_ovf_i4) },
            { OpCodes.Conv_Ovf_I4_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I4_Un, VMProtect.OpCodes.icConv_ovf_i4_un) },
            { OpCodes.Conv_Ovf_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I8, VMProtect.OpCodes.icConv_ovf_i8) },
            { OpCodes.Conv_Ovf_I8_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_I8_Un, VMProtect.OpCodes.icConv_ovf_i8_un) },
            { OpCodes.Conv_Ovf_U.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U, VMProtect.OpCodes.icConv_ovf_u) },
            { OpCodes.Conv_Ovf_U_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U_Un, VMProtect.OpCodes.icConv_ovf_u_un) },
            { OpCodes.Conv_Ovf_U1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U1, VMProtect.OpCodes.icConv_ovf_u1) },
            { OpCodes.Conv_Ovf_U1_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U1_Un, VMProtect.OpCodes.icConv_ovf_u1_un) },
            { OpCodes.Conv_Ovf_U2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U2, VMProtect.OpCodes.icConv_ovf_u2) },
            { OpCodes.Conv_Ovf_U2_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U2_Un, VMProtect.OpCodes.icConv_ovf_u2_un) },
            { OpCodes.Conv_Ovf_U4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U4, VMProtect.OpCodes.icConv_ovf_u4) },
            { OpCodes.Conv_Ovf_U4_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U4_Un, VMProtect.OpCodes.icConv_ovf_u4_un) },
            { OpCodes.Conv_Ovf_U8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U8, VMProtect.OpCodes.icConv_ovf_u8) },
            { OpCodes.Conv_Ovf_U8_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_Ovf_U8_Un, VMProtect.OpCodes.icConv_ovf_u8_un) },
            { OpCodes.Conv_R_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_R_Un, VMProtect.OpCodes.icConv_r_un) },
            { OpCodes.Conv_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_R4, VMProtect.OpCodes.icConv_r4) },
            { OpCodes.Conv_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_R8, VMProtect.OpCodes.icConv_r8) },
            { OpCodes.Conv_U.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_U, VMProtect.OpCodes.icConv_u) },
            { OpCodes.Conv_U1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_U1, VMProtect.OpCodes.icConv_u1) },
            { OpCodes.Conv_U2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_U2, VMProtect.OpCodes.icConv_u2) },
            { OpCodes.Conv_U4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_U4, VMProtect.OpCodes.icConv_u4) },
            { OpCodes.Conv_U8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Conv_U8, VMProtect.OpCodes.icConv_u8) },
            //{ OpCodes.Cpblk.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Cpblk, VMProtect.OpCodes.icCpblk) },
            //{ OpCodes.Cpobj.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Cpobj, VMProtect.OpCodes.icCpobj) },
            { OpCodes.Div.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Div, VMProtect.OpCodes.icDiv) },
            { OpCodes.Div_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Div_Un, VMProtect.OpCodes.icDiv_un) },
            //{ OpCodes.Dup.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Dup, VMProtect.OpCodes.icDup) },
            //{ OpCodes.Endfilter.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Endfilter, VMProtect.OpCodes.icEndfilter) },
            //{ OpCodes.Endfinally.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Endfinally, VMProtect.OpCodes.icEndfinally) },
            //{ OpCodes.Initblk.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Initblk, VMProtect.OpCodes.icInitblk) },
            //{ OpCodes.Initobj.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Initobj, VMProtect.OpCodes.icInitobj) },
            //{ OpCodes.Isinst.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Isinst, VMProtect.OpCodes.icIsinst) },
            //{ OpCodes.Jmp.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Jmp, VMProtect.OpCodes.icJmp) },
            //{ OpCodes.Ldarg.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg, VMProtect.OpCodes.icLdarg) },
            { OpCodes.Ldarg_0.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg_0, VMProtect.OpCodes.icLdarg_0) },
            { OpCodes.Ldarg_1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg_1, VMProtect.OpCodes.icLdarg_1) },
            //{ OpCodes.Ldarg_2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg_2, VMProtect.OpCodes.icLdarg_2) },
            //{ OpCodes.Ldarg_3.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg_3, VMProtect.OpCodes.icLdarg_3) },
            //{ OpCodes.Ldarg_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarg_S, VMProtect.OpCodes.icLdarg_S) },
            //{ OpCodes.Ldarga.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarga, VMProtect.OpCodes.icLdarga) },
            { OpCodes.Ldarga_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldarga_S, VMProtect.OpCodes.icLdarga_s) },
            { OpCodes.Ldc_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4, VMProtect.OpCodes.icLdc_i4) },
            { OpCodes.Ldc_I4_0.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_0, VMProtect.OpCodes.icLdc_i4_0) },
            { OpCodes.Ldc_I4_1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_1, VMProtect.OpCodes.icLdc_i4_1) },
            { OpCodes.Ldc_I4_2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_2, VMProtect.OpCodes.icLdc_i4_2) },
            //{ OpCodes.Ldc_I4_3.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_3, VMProtect.OpCodes.icLdc_I4_3) },
            //{ OpCodes.Ldc_I4_4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_4, VMProtect.OpCodes.icLdc_I4_4) },
            //{ OpCodes.Ldc_I4_5.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_5, VMProtect.OpCodes.icLdc_I4_5) },
            //{ OpCodes.Ldc_I4_6.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_6, VMProtect.OpCodes.icLdc_I4_6) },
            //{ OpCodes.Ldc_I4_7.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_7, VMProtect.OpCodes.icLdc_I4_7) },
            //{ OpCodes.Ldc_I4_8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_8, VMProtect.OpCodes.icLdc_I4_8) },
            //{ OpCodes.Ldc_I4_M1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_M1, VMProtect.OpCodes.icLdc_I4_M1) },
            //{ OpCodes.Ldc_I4_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I4_S, VMProtect.OpCodes.icLdc_I4_S) },
            //{ OpCodes.Ldc_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_I8, VMProtect.OpCodes.icLdc_I8) },
            //{ OpCodes.Ldc_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_R4, VMProtect.OpCodes.icLdc_R4) },
            //{ OpCodes.Ldc_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldc_R8, VMProtect.OpCodes.icLdc_R8) },
            //{ OpCodes.Ldelem.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem, VMProtect.OpCodes.icLdelem) },
            //{ OpCodes.Ldelem_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_I, VMProtect.OpCodes.icLdelem_I) },
            //{ OpCodes.Ldelem_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_I1, VMProtect.OpCodes.icLdelem_I1) },
            //{ OpCodes.Ldelem_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_I2, VMProtect.OpCodes.icLdelem_I2) },
            //{ OpCodes.Ldelem_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_I4, VMProtect.OpCodes.icLdelem_I4) },
            //{ OpCodes.Ldelem_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_I8, VMProtect.OpCodes.icLdelem_I8) },
            //{ OpCodes.Ldelem_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_R4, VMProtect.OpCodes.icLdelem_R4) },
            //{ OpCodes.Ldelem_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_R8, VMProtect.OpCodes.icLdelem_R8) },
            //{ OpCodes.Ldelem_Ref.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_Ref, VMProtect.OpCodes.icLdelem_Ref) },
            //{ OpCodes.Ldelem_U1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_U1, VMProtect.OpCodes.icLdelem_U1) },
            //{ OpCodes.Ldelem_U2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_U2, VMProtect.OpCodes.icLdelem_U2) },
            //{ OpCodes.Ldelem_U4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelem_U4, VMProtect.OpCodes.icLdelem_U4) },
            //{ OpCodes.Ldelema.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldelema, VMProtect.OpCodes.icLdelema) },
            //{ OpCodes.Ldfld.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldfld, VMProtect.OpCodes.icLdfld) },
            //{ OpCodes.Ldflda.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldflda, VMProtect.OpCodes.icLdflda) },
            //{ OpCodes.Ldftn.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldftn, VMProtect.OpCodes.icLdftn) },
            //{ OpCodes.Ldind_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_I, VMProtect.OpCodes.icLdind_I) },
            //{ OpCodes.Ldind_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_I1, VMProtect.OpCodes.icLdind_I1) },
            //{ OpCodes.Ldind_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_I2, VMProtect.OpCodes.icLdind_I2) },
            //{ OpCodes.Ldind_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_I4, VMProtect.OpCodes.icLdind_I4) },
            //{ OpCodes.Ldind_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_I8, VMProtect.OpCodes.icLdind_I8) },
            //{ OpCodes.Ldind_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_R4, VMProtect.OpCodes.icLdind_R4) },
            //{ OpCodes.Ldind_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_R8, VMProtect.OpCodes.icLdind_R8) },
            //{ OpCodes.Ldind_Ref.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_Ref, VMProtect.OpCodes.icLdind_Ref) },
            //{ OpCodes.Ldind_U1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_U1, VMProtect.OpCodes.icLdind_U1) },
            //{ OpCodes.Ldind_U2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_U2, VMProtect.OpCodes.icLdind_U2) },
            //{ OpCodes.Ldind_U4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldind_U4, VMProtect.OpCodes.icLdind_U4) },
            //{ OpCodes.Ldlen.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldlen, VMProtect.OpCodes.icLdlen) },
            //{ OpCodes.Ldloc.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc, VMProtect.OpCodes.icLdloc) },
            { OpCodes.Ldloc_0.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc_0, VMProtect.OpCodes.icLdloc_0) },
            //{ OpCodes.Ldloc_1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc_1, VMProtect.OpCodes.icLdloc_1) },
            //{ OpCodes.Ldloc_2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc_2, VMProtect.OpCodes.icLdloc_2) },
            //{ OpCodes.Ldloc_3.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc_3, VMProtect.OpCodes.icLdloc_3) },
            //{ OpCodes.Ldloc_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloc_S, VMProtect.OpCodes.icLdloc_S) },
            //{ OpCodes.Ldloca.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloca, VMProtect.OpCodes.icLdloca) },
            //{ OpCodes.Ldloca_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldloca_S, VMProtect.OpCodes.icLdloca_S) },
            { OpCodes.Ldnull.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldnull, VMProtect.OpCodes.icLdnull) },
            //{ OpCodes.Ldobj.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldobj, VMProtect.OpCodes.icLdobj) },
            //{ OpCodes.Ldsfld.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldsfld, VMProtect.OpCodes.icLdsfld) },
            //{ OpCodes.Ldsflda.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldsflda, VMProtect.OpCodes.icLdsflda) },
            //{ OpCodes.Ldstr.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldstr, VMProtect.OpCodes.icLdstr) },
            //{ OpCodes.Ldtoken.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldtoken, VMProtect.OpCodes.icLdtoken) },
            //{ OpCodes.Ldvirtftn.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ldvirtftn, VMProtect.OpCodes.icLdvirtftn) },
            //{ OpCodes.Leave.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Leave, VMProtect.OpCodes.icLeave) },
            //{ OpCodes.Leave_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Leave_S, VMProtect.OpCodes.icLeave_S) },
            //{ OpCodes.Localloc.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Localloc, VMProtect.OpCodes.icLocalloc) },
            //{ OpCodes.Mkrefany.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Mkrefany, VMProtect.OpCodes.icMkrefany) },
            { OpCodes.Mul.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Mul, VMProtect.OpCodes.icMul) },
            { OpCodes.Mul_Ovf.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Mul_Ovf, VMProtect.OpCodes.icMul_ovf) },
            { OpCodes.Mul_Ovf_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Mul_Ovf_Un, VMProtect.OpCodes.icMul_ovf_un) },
            //{ OpCodes.Neg.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Neg, VMProtect.OpCodes.icNeg) },
            //{ OpCodes.Newarr.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Newarr, VMProtect.OpCodes.icNewarr) },
            { OpCodes.Newobj.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Newobj, VMProtect.OpCodes.icNewobj) },
            { OpCodes.Nop.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Nop, VMProtect.OpCodes.icNop) },
            { OpCodes.Not.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Not, VMProtect.OpCodes.icNot) },
            { OpCodes.Or.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Or, VMProtect.OpCodes.icOr) },
            //{ OpCodes.Pop.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Pop, VMProtect.OpCodes.icPop) },
            //{ OpCodes.Prefix1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix1, VMProtect.OpCodes.icPrefix1) },
            //{ OpCodes.Prefix2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix2, VMProtect.OpCodes.icPrefix2) },
            //{ OpCodes.Prefix3.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix3, VMProtect.OpCodes.icPrefix3) },
            //{ OpCodes.Prefix4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix4, VMProtect.OpCodes.icPrefix4) },
            //{ OpCodes.Prefix5.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix5, VMProtect.OpCodes.icPrefix5) },
            //{ OpCodes.Prefix6.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix6, VMProtect.OpCodes.icPrefix6) },
            //{ OpCodes.Prefix7.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefix7, VMProtect.OpCodes.icPrefix7) },
            //{ OpCodes.Prefixref.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Prefixref, VMProtect.OpCodes.icPrefixref) },
            //{ OpCodes.Readonly.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Readonly, VMProtect.OpCodes.icReadonly) },
            //{ OpCodes.Refanytype.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Refanytype, VMProtect.OpCodes.icRefanytype) },
            //{ OpCodes.Refanyval.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Refanyval, VMProtect.OpCodes.icRefanyval) },
            { OpCodes.Rem.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Rem, VMProtect.OpCodes.icRem) },
            { OpCodes.Rem_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Rem_Un, VMProtect.OpCodes.icRem_un) },
            { OpCodes.Ret.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Ret, VMProtect.OpCodes.icRet) },
            //{ OpCodes.Rethrow.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Rethrow, VMProtect.OpCodes.icRethrow) },
            { OpCodes.Shl.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Shl, VMProtect.OpCodes.icShl) },
            { OpCodes.Shr.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Shr, VMProtect.OpCodes.icShr) },
            { OpCodes.Shr_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Shr_Un, VMProtect.OpCodes.icShr_un) },
            //{ OpCodes.Sizeof.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Sizeof, VMProtect.OpCodes.icSizeof) },
            //{ OpCodes.Starg.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Starg, VMProtect.OpCodes.icStarg) },
            //{ OpCodes.Starg_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Starg_S, VMProtect.OpCodes.icStarg_S) },
            //{ OpCodes.Stelem.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem, VMProtect.OpCodes.icStelem) },
            //{ OpCodes.Stelem_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_I, VMProtect.OpCodes.icStelem_I) },
            //{ OpCodes.Stelem_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_I1, VMProtect.OpCodes.icStelem_I1) },
            { OpCodes.Stelem_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_I2, VMProtect.OpCodes.icStelem_i2) },
            //{ OpCodes.Stelem_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_I4, VMProtect.OpCodes.icStelem_I4) },
            //{ OpCodes.Stelem_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_I8, VMProtect.OpCodes.icStelem_I8) },
            //{ OpCodes.Stelem_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_R4, VMProtect.OpCodes.icStelem_R4) },
            //{ OpCodes.Stelem_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_R8, VMProtect.OpCodes.icStelem_R8) },
            //{ OpCodes.Stelem_Ref.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stelem_Ref, VMProtect.OpCodes.icStelem_Ref) },
            //{ OpCodes.Stfld.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stfld, VMProtect.OpCodes.icStfld) },
            //{ OpCodes.Stind_I.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_I, VMProtect.OpCodes.icStind_I) },
            //{ OpCodes.Stind_I1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_I1, VMProtect.OpCodes.icStind_I1) },
            //{ OpCodes.Stind_I2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_I2, VMProtect.OpCodes.icStind_I2) },
            //{ OpCodes.Stind_I4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_I4, VMProtect.OpCodes.icStind_I4) },
            //{ OpCodes.Stind_I8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_I8, VMProtect.OpCodes.icStind_I8) },
            //{ OpCodes.Stind_R4.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_R4, VMProtect.OpCodes.icStind_R4) },
            //{ OpCodes.Stind_R8.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_R8, VMProtect.OpCodes.icStind_R8) },
            //{ OpCodes.Stind_Ref.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stind_Ref, VMProtect.OpCodes.icStind_Ref) },
            //{ OpCodes.Stloc.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc, VMProtect.OpCodes.icStloc) },
            { OpCodes.Stloc_0.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc_0, VMProtect.OpCodes.icStloc_0) },
            //{ OpCodes.Stloc_1.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc_1, VMProtect.OpCodes.icStloc_1) },
            //{ OpCodes.Stloc_2.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc_2, VMProtect.OpCodes.icStloc_2) },
            //{ OpCodes.Stloc_3.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc_3, VMProtect.OpCodes.icStloc_3) },
            //{ OpCodes.Stloc_S.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stloc_S, VMProtect.OpCodes.icStloc_S) },
            //{ OpCodes.Stobj.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stobj, VMProtect.OpCodes.icStobj) },
            //{ OpCodes.Stsfld.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Stsfld, VMProtect.OpCodes.icStsfld) },
            { OpCodes.Sub.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Sub, VMProtect.OpCodes.icSub) },
            { OpCodes.Sub_Ovf.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Sub_Ovf, VMProtect.OpCodes.icSub_ovf) },
            { OpCodes.Sub_Ovf_Un.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Sub_Ovf_Un, VMProtect.OpCodes.icSub_ovf_un) },
            //{ OpCodes.Switch.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Switch, VMProtect.OpCodes.icSwitch) },
            //{ OpCodes.Tailcall.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Tailcall, VMProtect.OpCodes.icTailcall) },
            //{ OpCodes.Throw.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Throw, VMProtect.OpCodes.icThrow) },
            //{ OpCodes.Unaligned.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Unaligned, VMProtect.OpCodes.icUnaligned) },
            //{ OpCodes.Unbox.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Unbox, VMProtect.OpCodes.icUnbox) },
            //{ OpCodes.Unbox_Any.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Unbox_Any, VMProtect.OpCodes.icUnbox_Any) },
            //{ OpCodes.Volatile.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Volatile, VMProtect.OpCodes.icVolatile) },
            { OpCodes.Xor.Value, new KeyValuePair<OpCode, VMProtect.OpCodes>(OpCodes.Xor, VMProtect.OpCodes.icXor) },
        };
        #endregion
        private static readonly Dictionary<Type, int> TypeTokenInModule = new Dictionary<Type, int>(); //TypeDefs and TypeRefs
        private static int GetTypeTokenInModule(Type t)
        {
            var tryToken = 0x01000001; //first TypeRef
            var m = typeof (MsilToVmTestCompilerVmp).Module;
            while (true)
            {
                try
                {
                    Type t1 = m.ResolveType(tryToken);
                    TypeTokenInModule.Add(t1, tryToken++);
                }
                catch (Exception)
                {
                    break; //until last TypeRef
                }

            }
            if (TypeTokenInModule.ContainsKey(t))
                return TypeTokenInModule[t];
            if (t.Module == m)
            {
                TypeTokenInModule.Add(t, t.MetadataToken);
                return t.MetadataToken;
            }
            Assert.Fail("Bad type {0}: not accessible from this module", t);
            return 0;
        }
        public override Stream CreateVmStream(Type rt, ParameterInfo[] pi, Type[] locals, byte[] ilBytes)
        {
            var ret = new MemoryStream();
            var wr = new BinaryWriter(ret);
            var fwdTypeDefs = new Dictionary<Type, int>();
            var fwdTypeRefs = new Dictionary<int, Type>();
            var deferredBrTargets = new Dictionary<long, UInt32>();
            var srcToDstInstrPositions = new Dictionary<long, UInt32>();
            const int dummy = 0x55555555;
            fwdTypeDefs.Add(rt, dummy);
            var pars = pi;
            foreach (var parameterInfo in pars)
            {
                if (!fwdTypeDefs.ContainsKey(parameterInfo.ParameterType))
                    fwdTypeDefs.Add(parameterInfo.ParameterType, dummy);
				wr.Write((byte)VMProtect.OpCodes.icInitarg);
				fwdTypeRefs.Add((int)ret.Position, parameterInfo.ParameterType);
                wr.Write(dummy); // parTypeId fwd ref
            }
            foreach (var lt in locals)
            {
                if (!fwdTypeDefs.ContainsKey(lt))
                    fwdTypeDefs.Add(lt, dummy);
				// FIXME
				//wr.Write((byte)VMProtect.OpCodes.icInitloc);
				fwdTypeRefs.Add((int)ret.Position, lt);
                wr.Write(dummy); // parTypeId fwd ref
            }

            using (var sr = new BinaryReader(new MemoryStream(ilBytes)))
            {
                while (sr.BaseStream.Position < sr.BaseStream.Length)
                {
                    srcToDstInstrPositions[sr.BaseStream.Position] = (uint)ret.Position;
                    short ilByte = sr.ReadByte();
                    if (ilByte == 0xFE)
                        ilByte = (short)((ilByte << 8) | sr.ReadByte());
                    Assert.IsTrue(IlToVmInstrInfo.ContainsKey(ilByte), "Extend IlToVmInstrInfo with {0:X} or use short jump", ilByte);
                    var vmInstrInfo = IlToVmInstrInfo[ilByte];
                    ulong operand = 0;
                    switch (vmInstrInfo.Key.OperandType)
                    {
                        case OperandType.InlineNone:
                            break;
                        case OperandType.InlineI8:
                        case OperandType.InlineR:
                            operand = sr.ReadUInt64();
                            break;
                        case OperandType.InlineVar:
                            operand = sr.ReadUInt16();
                            break;
                        case OperandType.ShortInlineBrTarget:
                        case OperandType.ShortInlineI:
                        case OperandType.ShortInlineVar:
                            operand = sr.ReadByte();
                            break;
                        default:
                            operand = sr.ReadUInt32();
                            break;
                    }
					switch ((short)vmInstrInfo.Value)
					{
						case (short)VMProtect.OpCodes.icNop:
							continue;
						case (short)VMProtect.OpCodes.icStloc_0:
							wr.Write((byte)VMProtect.OpCodes.icStloc);
							wr.Write((ushort)0);
							break;
						case (short)VMProtect.OpCodes.icStloc_1:
							wr.Write((byte)VMProtect.OpCodes.icStloc);
							wr.Write((ushort)1);
							break;
						case (short)VMProtect.OpCodes.icStloc_2:
							wr.Write((byte)VMProtect.OpCodes.icStloc);
							wr.Write((ushort)2);
							break;
						case (short)VMProtect.OpCodes.icStloc_3:
							wr.Write((byte)VMProtect.OpCodes.icStloc);
							wr.Write((ushort)3);
							break;
						case (short)VMProtect.OpCodes.icLdloc_0:
							wr.Write((byte)VMProtect.OpCodes.icLdloc);
							wr.Write((ushort)0);
							break;
						case (short)VMProtect.OpCodes.icLdloc_1:
							wr.Write((byte)VMProtect.OpCodes.icLdloc);
							wr.Write((ushort)1);
							break;
						case (short)VMProtect.OpCodes.icLdloc_2:
							wr.Write((byte)VMProtect.OpCodes.icLdloc);
							wr.Write((ushort)2);
							break;
						case (short)VMProtect.OpCodes.icLdloc_3:
							wr.Write((byte)VMProtect.OpCodes.icLdloc);
							wr.Write((ushort)3);
							break;
						case (short)VMProtect.OpCodes.icLdarg_0:
							wr.Write((byte)VMProtect.OpCodes.icLdarg);
							wr.Write((ushort)0);
							break;
						case (short)VMProtect.OpCodes.icLdarg_1:
							wr.Write((byte)VMProtect.OpCodes.icLdarg);
							wr.Write((ushort)1);
							break;
						case (short)VMProtect.OpCodes.icLdarg_2:
							wr.Write((byte)VMProtect.OpCodes.icLdarg);
							wr.Write((ushort)2);
							break;
						case (short)VMProtect.OpCodes.icLdarg_3:
							wr.Write((byte)VMProtect.OpCodes.icLdarg);
							wr.Write((ushort)3);
							break;
						case (short)VMProtect.OpCodes.icLdarga_s:
							wr.Write((byte)VMProtect.OpCodes.icLdarga);
							wr.Write((ushort)operand);
							break;
						case (short)VMProtect.OpCodes.icRet:
							wr.Write((byte)VMProtect.OpCodes.icRet);
							fwdTypeRefs.Add((int)ret.Position, rt);
							wr.Write(dummy); // ReturnTypeId fwd ref
							break;
						case (short)VMProtect.OpCodes.icLdc_i4_0:
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							wr.Write((uint)0);
							break;
						case (short)VMProtect.OpCodes.icLdc_i4_1:
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							wr.Write((uint)1);
							break;
						case (short)VMProtect.OpCodes.icLdc_i4_2:
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							wr.Write((uint)2);
							break;
						case (short)VMProtect.OpCodes.icLdc_i4_3:
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							wr.Write((uint)3);
							break;
						case (short)VMProtect.OpCodes.icBr:
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							deferredBrTargets[ret.Position] = (uint)sr.BaseStream.Position + (uint)operand;
							wr.Write(dummy);
							wr.Write((byte)VMProtect.OpCodes.icBr);
							break;
						case (short)VMProtect.OpCodes.icBeq:
						case (short)VMProtect.OpCodes.icBne_un:
						case (short)VMProtect.OpCodes.icBlt:
						case (short)VMProtect.OpCodes.icBlt_un:
						case (short)VMProtect.OpCodes.icBle:
						case (short)VMProtect.OpCodes.icBle_un:
						case (short)VMProtect.OpCodes.icBgt:
						case (short)VMProtect.OpCodes.icBgt_un:
						case (short)VMProtect.OpCodes.icBge:
						case (short)VMProtect.OpCodes.icBge_un:
						case (short)VMProtect.OpCodes.icBrtrue:
						case (short)VMProtect.OpCodes.icBrfalse:
							switch ((short)vmInstrInfo.Value)
							{
								// FIXME
								/*
								case (short)VMProtect.OpCodes.icBeq:
								case (short)VMProtect.OpCodes.icBne_un:
									wr.Write((byte)VMProtect.OpCodes.icCeq);
									break;
								case (short)VMProtect.OpCodes.icBge:
									wr.Write((byte)VMProtect.OpCodes.icCge);
									break;
								case (short)VMProtect.OpCodes.icBge_un:
									wr.Write((byte)VMProtect.OpCodes.icCge_un);
									break;
								case (short)VMProtect.OpCodes.icBlt:
									wr.Write((byte)VMProtect.OpCodes.icClt);
									break;
								case (short)VMProtect.OpCodes.icBlt_un:
									wr.Write((byte)VMProtect.OpCodes.icClt_un);
									break;
								case (short)VMProtect.OpCodes.icBle:
									wr.Write((byte)VMProtect.OpCodes.icCle);
									break;
								case (short)VMProtect.OpCodes.icBle_un:
									wr.Write((byte)VMProtect.OpCodes.icCle_un);
									break;
								case (short)VMProtect.OpCodes.icBgt:
									wr.Write((byte)VMProtect.OpCodes.icCgt);
									break;
								case (short)VMProtect.OpCodes.icBgt_un:
									wr.Write((byte)VMProtect.OpCodes.icCgt_un);
									break;
									*/
							}
							// FIXME
							//wr.Write((byte)VMProtect.OpCodes.icConv_b);
							wr.Write((byte)VMProtect.OpCodes.icNeg);
							switch ((short)vmInstrInfo.Value)
							{
								case (short)VMProtect.OpCodes.icBne_un:
								//case (short)VMProtect.OpCodes.icBle:
								//case (short)VMProtect.OpCodes.icBle_un:
								//case (short)VMProtect.OpCodes.icBge:
								//case (short)VMProtect.OpCodes.icBge_un:
								case (short)VMProtect.OpCodes.icBrfalse:
									wr.Write((byte)VMProtect.OpCodes.icNot);
									break;
							}
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							deferredBrTargets[ret.Position] = (uint)sr.BaseStream.Position + (uint)operand;
							wr.Write(dummy);
							wr.Write((byte)VMProtect.OpCodes.icAnd);
							wr.Write((byte)VMProtect.OpCodes.icDup);
							// FIXME
							//wr.Write((byte)VMProtect.OpCodes.icConv_b);
							wr.Write((byte)VMProtect.OpCodes.icNeg);
							wr.Write((byte)VMProtect.OpCodes.icNot);
							wr.Write((byte)VMProtect.OpCodes.icLdc_i4);
							deferredBrTargets[ret.Position] = (uint)sr.BaseStream.Position;
							wr.Write(dummy);
							wr.Write((byte)VMProtect.OpCodes.icAnd);
							wr.Write((byte)VMProtect.OpCodes.icAdd);
							wr.Write((byte)VMProtect.OpCodes.icBr);
							break;
						default:
							wr.Write((byte)vmInstrInfo.Value);
							break;
					}
					wr.Flush();
				}
				foreach (var t in fwdTypeDefs.Keys.ToArray())
                {
                    fwdTypeDefs[t] = GetTypeTokenInModule(t);
                }
                foreach (var r in fwdTypeRefs)
                {
                    ret.Position = r.Key;
                    wr.Write(fwdTypeDefs[r.Value]);
                    wr.Flush();
                }
                foreach (var r in deferredBrTargets)
                {
                    ret.Position = r.Key;
                    var srcPos = r.Value;
                    wr.Write(srcToDstInstrPositions[srcPos]);
                    wr.Flush();
                }
                ret.Position = 0;
                return ret;
            }
        }

        public override object Invoke(object[] parameters, Stream vmStream)
        {
            var contents = new BinaryReader(vmStream).ReadBytes((int) vmStream.Length);
            var unmanagedPointer = Marshal.AllocHGlobal(contents.Length);
            Marshal.Copy(contents, 0, unmanagedPointer, contents.Length);
            var vm = new VirtualMachine();
            // ReSharper disable once PossibleNullReferenceException
            typeof(VirtualMachine).GetField("_instance", BindingFlags.Instance | BindingFlags.NonPublic)
                .SetValue(vm, unmanagedPointer.ToInt64());
            var vmRet = vm.Invoke(parameters, 0);
            Marshal.FreeHGlobal(unmanagedPointer);
            return vmRet;
        }
    }

    public class MsilToVmTestCompilerRefVm : MsilToVmTestCompiler
    {
        public static readonly VmInstrCodesDb InstrCodesDb = new VmInstrCodesDb();
        #region opcodes
        public static readonly Dictionary<short, KeyValuePair<OpCode, VmInstrInfo>> IlToVmInstrInfo = new Dictionary<short, KeyValuePair<OpCode, VmInstrInfo>>()
        {
            { OpCodes.Add.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Add, InstrCodesDb.Add_) },
            { OpCodes.Add_Ovf.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Add_Ovf, InstrCodesDb.Add_ovf_) },
            { OpCodes.Add_Ovf_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Add_Ovf_Un, InstrCodesDb.Add_ovf_un_) },
            { OpCodes.And.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.And, InstrCodesDb.And_) },
            //{ OpCodes.Arglist.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Arglist, InstrCodesDb.Arglist_) },
            { OpCodes.Beq_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Beq_S, InstrCodesDb.Beq_) },
            { OpCodes.Bge_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Bge_S, InstrCodesDb.Bge_) },
            { OpCodes.Bge_Un_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Bge_Un_S, InstrCodesDb.Bge_un_) },
            { OpCodes.Bgt_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Bgt_S, InstrCodesDb.Bgt_) },
            { OpCodes.Bgt_Un_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Bgt_Un_S, InstrCodesDb.Bgt_un_) },
            { OpCodes.Ble_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ble_S, InstrCodesDb.Ble_) },
            { OpCodes.Ble_Un_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ble_Un_S, InstrCodesDb.Ble_un_) },
            { OpCodes.Blt_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Blt_S, InstrCodesDb.Blt_) },
            { OpCodes.Blt_Un_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Blt_Un_S, InstrCodesDb.Blt_un_) },
            { OpCodes.Bne_Un_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Bne_Un_S, InstrCodesDb.Bne_un_) },
            //{ OpCodes.Box.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Box, InstrCodesDb.Box_) },
            { OpCodes.Br_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Br_S, InstrCodesDb.Br_) },
            //{ OpCodes.Break.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Break, InstrCodesDb.Break_) },
            //{ OpCodes.Brfalse_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Brfalse_S, InstrCodesDb.Brfalse_S_) },
            //{ OpCodes.Brtrue_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Brtrue_S, InstrCodesDb.Brtrue_S_) },
            { OpCodes.Call.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Call, InstrCodesDb.Call_) },
            //{ OpCodes.Calli.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Calli, InstrCodesDb.Calli_) },
            //{ OpCodes.Callvirt.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Callvirt, InstrCodesDb.Callvirt_) },
            //{ OpCodes.Castclass.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Castclass, InstrCodesDb.Castclass_) },
            { OpCodes.Ceq.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ceq, InstrCodesDb.Ceq_) },
            { OpCodes.Cgt.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Cgt, InstrCodesDb.Cgt_) },
            { OpCodes.Cgt_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Cgt_Un, InstrCodesDb.Cgt_un_) },
            { OpCodes.Ckfinite.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ckfinite, InstrCodesDb.Ckfinite_) },
            { OpCodes.Clt.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Clt, InstrCodesDb.Clt_) },
            { OpCodes.Clt_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Clt_Un, InstrCodesDb.Clt_un_) },
            //{ OpCodes.Constrained.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Constrained, InstrCodesDb.Constrained_) },
            { OpCodes.Conv_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_I, InstrCodesDb.Conv_i_) },
            { OpCodes.Conv_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_I1, InstrCodesDb.Conv_i1_) },
            { OpCodes.Conv_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_I2, InstrCodesDb.Conv_i2_) },
            { OpCodes.Conv_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_I4, InstrCodesDb.Conv_i4_) },
            { OpCodes.Conv_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_I8, InstrCodesDb.Conv_i8_) },
            { OpCodes.Conv_Ovf_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I, InstrCodesDb.Conv_ovf_i_) },
            { OpCodes.Conv_Ovf_I_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I_Un, InstrCodesDb.Conv_ovf_i_un_) },
            { OpCodes.Conv_Ovf_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I1, InstrCodesDb.Conv_ovf_i1_) },
            { OpCodes.Conv_Ovf_I1_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I1_Un, InstrCodesDb.Conv_ovf_i1_un_) },
            { OpCodes.Conv_Ovf_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I2, InstrCodesDb.Conv_ovf_i2_) },
            { OpCodes.Conv_Ovf_I2_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I2_Un, InstrCodesDb.Conv_ovf_i2_un_) },
            { OpCodes.Conv_Ovf_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I4, InstrCodesDb.Conv_ovf_i4_) },
            { OpCodes.Conv_Ovf_I4_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I4_Un, InstrCodesDb.Conv_ovf_i4_un_) },
            { OpCodes.Conv_Ovf_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I8, InstrCodesDb.Conv_ovf_i8_) },
            { OpCodes.Conv_Ovf_I8_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_I8_Un, InstrCodesDb.Conv_ovf_i8_un_) },
            { OpCodes.Conv_Ovf_U.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U, InstrCodesDb.Conv_ovf_u_) },
            { OpCodes.Conv_Ovf_U_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U_Un, InstrCodesDb.Conv_ovf_u_un_) },
            { OpCodes.Conv_Ovf_U1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U1, InstrCodesDb.Conv_ovf_u1_) },
            { OpCodes.Conv_Ovf_U1_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U1_Un, InstrCodesDb.Conv_ovf_u1_un_) },
            { OpCodes.Conv_Ovf_U2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U2, InstrCodesDb.Conv_ovf_u2_) },
            { OpCodes.Conv_Ovf_U2_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U2_Un, InstrCodesDb.Conv_ovf_u2_un_) },
            { OpCodes.Conv_Ovf_U4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U4, InstrCodesDb.Conv_ovf_u4_) },
            { OpCodes.Conv_Ovf_U4_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U4_Un, InstrCodesDb.Conv_ovf_u4_un_) },
            { OpCodes.Conv_Ovf_U8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U8, InstrCodesDb.Conv_ovf_u8_) },
            { OpCodes.Conv_Ovf_U8_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_Ovf_U8_Un, InstrCodesDb.Conv_ovf_u8_un_) },
            { OpCodes.Conv_R_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_R_Un, InstrCodesDb.Conv_r_un_) },
            { OpCodes.Conv_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_R4, InstrCodesDb.Conv_r4_) },
            { OpCodes.Conv_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_R8, InstrCodesDb.Conv_r8_) },
            { OpCodes.Conv_U.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_U, InstrCodesDb.Conv_u_) },
            { OpCodes.Conv_U1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_U1, InstrCodesDb.Conv_u1_) },
            { OpCodes.Conv_U2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_U2, InstrCodesDb.Conv_u2_) },
            { OpCodes.Conv_U4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_U4, InstrCodesDb.Conv_u4_) },
            { OpCodes.Conv_U8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Conv_U8, InstrCodesDb.Conv_u8_) },
            //{ OpCodes.Cpblk.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Cpblk, InstrCodesDb.Cpblk_) },
            //{ OpCodes.Cpobj.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Cpobj, InstrCodesDb.Cpobj_) },
            { OpCodes.Div.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Div, InstrCodesDb.Div_) },
            { OpCodes.Div_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Div_Un, InstrCodesDb.Div_un_) },
            //{ OpCodes.Dup.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Dup, InstrCodesDb.Dup_) },
            //{ OpCodes.Endfilter.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Endfilter, InstrCodesDb.Endfilter_) },
            //{ OpCodes.Endfinally.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Endfinally, InstrCodesDb.Endfinally_) },
            //{ OpCodes.Initblk.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Initblk, InstrCodesDb.Initblk_) },
            //{ OpCodes.Initobj.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Initobj, InstrCodesDb.Initobj_) },
            //{ OpCodes.Isinst.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Isinst, InstrCodesDb.Isinst_) },
            //{ OpCodes.Jmp.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Jmp, InstrCodesDb.Jmp_) },
            //{ OpCodes.Ldarg.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg, InstrCodesDb.Ldarg_) },
            { OpCodes.Ldarg_0.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg_0, InstrCodesDb.Ldarg_0_) },
            { OpCodes.Ldarg_1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg_1, InstrCodesDb.Ldarg_1_) },
            //{ OpCodes.Ldarg_2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg_2, InstrCodesDb.Ldarg_2_) },
            //{ OpCodes.Ldarg_3.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg_3, InstrCodesDb.Ldarg_3_) },
            //{ OpCodes.Ldarg_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarg_S, InstrCodesDb.Ldarg_S_) },
            //{ OpCodes.Ldarga.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarga, InstrCodesDb.Ldarga_) },
            { OpCodes.Ldarga_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldarga_S, InstrCodesDb.Ldarga_s_) },
            //{ OpCodes.Ldc_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4, InstrCodesDb.Ldc_i4_) },
            { OpCodes.Ldc_I4_0.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_0, InstrCodesDb.Ldc_i4_0_) },
            { OpCodes.Ldc_I4_1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_1, InstrCodesDb.Ldc_i4_1_) },
            //{ OpCodes.Ldc_I4_2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_2, InstrCodesDb.Ldc_i4_2_) },
            //{ OpCodes.Ldc_I4_3.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_3, InstrCodesDb.Ldc_I4_3_) },
            //{ OpCodes.Ldc_I4_4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_4, InstrCodesDb.Ldc_I4_4_) },
            //{ OpCodes.Ldc_I4_5.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_5, InstrCodesDb.Ldc_I4_5_) },
            //{ OpCodes.Ldc_I4_6.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_6, InstrCodesDb.Ldc_I4_6_) },
            //{ OpCodes.Ldc_I4_7.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_7, InstrCodesDb.Ldc_I4_7_) },
            //{ OpCodes.Ldc_I4_8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_8, InstrCodesDb.Ldc_I4_8_) },
            //{ OpCodes.Ldc_I4_M1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_M1, InstrCodesDb.Ldc_I4_M1_) },
            //{ OpCodes.Ldc_I4_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I4_S, InstrCodesDb.Ldc_I4_S_) },
            //{ OpCodes.Ldc_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_I8, InstrCodesDb.Ldc_I8_) },
            //{ OpCodes.Ldc_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_R4, InstrCodesDb.Ldc_R4_) },
            //{ OpCodes.Ldc_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldc_R8, InstrCodesDb.Ldc_R8_) },
            //{ OpCodes.Ldelem.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem, InstrCodesDb.Ldelem_) },
            //{ OpCodes.Ldelem_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_I, InstrCodesDb.Ldelem_I_) },
            //{ OpCodes.Ldelem_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_I1, InstrCodesDb.Ldelem_I1_) },
            //{ OpCodes.Ldelem_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_I2, InstrCodesDb.Ldelem_I2_) },
            //{ OpCodes.Ldelem_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_I4, InstrCodesDb.Ldelem_I4_) },
            //{ OpCodes.Ldelem_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_I8, InstrCodesDb.Ldelem_I8_) },
            //{ OpCodes.Ldelem_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_R4, InstrCodesDb.Ldelem_R4_) },
            //{ OpCodes.Ldelem_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_R8, InstrCodesDb.Ldelem_R8_) },
            //{ OpCodes.Ldelem_Ref.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_Ref, InstrCodesDb.Ldelem_Ref_) },
            //{ OpCodes.Ldelem_U1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_U1, InstrCodesDb.Ldelem_U1_) },
            //{ OpCodes.Ldelem_U2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_U2, InstrCodesDb.Ldelem_U2_) },
            //{ OpCodes.Ldelem_U4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelem_U4, InstrCodesDb.Ldelem_U4_) },
            //{ OpCodes.Ldelema.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldelema, InstrCodesDb.Ldelema_) },
            //{ OpCodes.Ldfld.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldfld, InstrCodesDb.Ldfld_) },
            //{ OpCodes.Ldflda.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldflda, InstrCodesDb.Ldflda_) },
            //{ OpCodes.Ldftn.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldftn, InstrCodesDb.Ldftn_) },
            //{ OpCodes.Ldind_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_I, InstrCodesDb.Ldind_I_) },
            //{ OpCodes.Ldind_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_I1, InstrCodesDb.Ldind_I1_) },
            //{ OpCodes.Ldind_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_I2, InstrCodesDb.Ldind_I2_) },
            //{ OpCodes.Ldind_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_I4, InstrCodesDb.Ldind_I4_) },
            //{ OpCodes.Ldind_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_I8, InstrCodesDb.Ldind_I8_) },
            //{ OpCodes.Ldind_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_R4, InstrCodesDb.Ldind_R4_) },
            //{ OpCodes.Ldind_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_R8, InstrCodesDb.Ldind_R8_) },
            //{ OpCodes.Ldind_Ref.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_Ref, InstrCodesDb.Ldind_Ref_) },
            //{ OpCodes.Ldind_U1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_U1, InstrCodesDb.Ldind_U1_) },
            //{ OpCodes.Ldind_U2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_U2, InstrCodesDb.Ldind_U2_) },
            //{ OpCodes.Ldind_U4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldind_U4, InstrCodesDb.Ldind_U4_) },
            //{ OpCodes.Ldlen.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldlen, InstrCodesDb.Ldlen_) },
            //{ OpCodes.Ldloc.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc, InstrCodesDb.Ldloc_) },
            //{ OpCodes.Ldloc_0.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc_0, InstrCodesDb.Ldloc_0_) },
            //{ OpCodes.Ldloc_1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc_1, InstrCodesDb.Ldloc_1_) },
            //{ OpCodes.Ldloc_2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc_2, InstrCodesDb.Ldloc_2_) },
            //{ OpCodes.Ldloc_3.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc_3, InstrCodesDb.Ldloc_3_) },
            //{ OpCodes.Ldloc_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloc_S, InstrCodesDb.Ldloc_S_) },
            //{ OpCodes.Ldloca.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloca, InstrCodesDb.Ldloca_) },
            //{ OpCodes.Ldloca_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldloca_S, InstrCodesDb.Ldloca_S_) },
            //{ OpCodes.Ldnull.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldnull, InstrCodesDb.Ldnull_) },
            //{ OpCodes.Ldobj.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldobj, InstrCodesDb.Ldobj_) },
            //{ OpCodes.Ldsfld.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldsfld, InstrCodesDb.Ldsfld_) },
            //{ OpCodes.Ldsflda.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldsflda, InstrCodesDb.Ldsflda_) },
            //{ OpCodes.Ldstr.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldstr, InstrCodesDb.Ldstr_) },
            //{ OpCodes.Ldtoken.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldtoken, InstrCodesDb.Ldtoken_) },
            //{ OpCodes.Ldvirtftn.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ldvirtftn, InstrCodesDb.Ldvirtftn_) },
            //{ OpCodes.Leave.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Leave, InstrCodesDb.Leave_) },
            //{ OpCodes.Leave_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Leave_S, InstrCodesDb.Leave_S_) },
            //{ OpCodes.Localloc.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Localloc, InstrCodesDb.Localloc_) },
            //{ OpCodes.Mkrefany.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Mkrefany, InstrCodesDb.Mkrefany_) },
            { OpCodes.Mul.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Mul, InstrCodesDb.Mul_) },
            { OpCodes.Mul_Ovf.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Mul_Ovf, InstrCodesDb.Mul_ovf_) },
            { OpCodes.Mul_Ovf_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Mul_Ovf_Un, InstrCodesDb.Mul_ovf_un_) },
            //{ OpCodes.Neg.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Neg, InstrCodesDb.Neg_) },
            //{ OpCodes.Newarr.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Newarr, InstrCodesDb.Newarr_) },
            { OpCodes.Newobj.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Newobj, InstrCodesDb.Newobj_) },
            //{ OpCodes.Nop.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Nop, InstrCodesDb.Nop_) },
            { OpCodes.Not.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Not, InstrCodesDb.Not_) },
            { OpCodes.Or.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Or, InstrCodesDb.Or_) },
            //{ OpCodes.Pop.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Pop, InstrCodesDb.Pop_) },
            //{ OpCodes.Prefix1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix1, InstrCodesDb.Prefix1_) },
            //{ OpCodes.Prefix2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix2, InstrCodesDb.Prefix2_) },
            //{ OpCodes.Prefix3.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix3, InstrCodesDb.Prefix3_) },
            //{ OpCodes.Prefix4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix4, InstrCodesDb.Prefix4_) },
            //{ OpCodes.Prefix5.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix5, InstrCodesDb.Prefix5_) },
            //{ OpCodes.Prefix6.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix6, InstrCodesDb.Prefix6_) },
            //{ OpCodes.Prefix7.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefix7, InstrCodesDb.Prefix7_) },
            //{ OpCodes.Prefixref.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Prefixref, InstrCodesDb.Prefixref_) },
            //{ OpCodes.Readonly.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Readonly, InstrCodesDb.Readonly_) },
            //{ OpCodes.Refanytype.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Refanytype, InstrCodesDb.Refanytype_) },
            //{ OpCodes.Refanyval.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Refanyval, InstrCodesDb.Refanyval_) },
            { OpCodes.Rem.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Rem, InstrCodesDb.Rem_) },
            { OpCodes.Rem_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Rem_Un, InstrCodesDb.Rem_un_) },
            { OpCodes.Ret.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Ret, InstrCodesDb.Ret_) },
            //{ OpCodes.Rethrow.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Rethrow, InstrCodesDb.Rethrow_) },
            { OpCodes.Shl.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Shl, InstrCodesDb.Shl_) },
            { OpCodes.Shr.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Shr, InstrCodesDb.Shr_) },
            { OpCodes.Shr_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Shr_Un, InstrCodesDb.Shr_un_) },
            //{ OpCodes.Sizeof.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Sizeof, InstrCodesDb.Sizeof_) },
            //{ OpCodes.Starg.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Starg, InstrCodesDb.Starg_) },
            //{ OpCodes.Starg_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Starg_S, InstrCodesDb.Starg_S_) },
            //{ OpCodes.Stelem.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem, InstrCodesDb.Stelem_) },
            //{ OpCodes.Stelem_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_I, InstrCodesDb.Stelem_I_) },
            //{ OpCodes.Stelem_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_I1, InstrCodesDb.Stelem_I1_) },
            //{ OpCodes.Stelem_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_I2, InstrCodesDb.Stelem_i2_) },
            //{ OpCodes.Stelem_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_I4, InstrCodesDb.Stelem_I4_) },
            //{ OpCodes.Stelem_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_I8, InstrCodesDb.Stelem_I8_) },
            //{ OpCodes.Stelem_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_R4, InstrCodesDb.Stelem_R4_) },
            //{ OpCodes.Stelem_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_R8, InstrCodesDb.Stelem_R8_) },
            //{ OpCodes.Stelem_Ref.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stelem_Ref, InstrCodesDb.Stelem_Ref_) },
            //{ OpCodes.Stfld.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stfld, InstrCodesDb.Stfld_) },
            //{ OpCodes.Stind_I.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_I, InstrCodesDb.Stind_I_) },
            //{ OpCodes.Stind_I1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_I1, InstrCodesDb.Stind_I1_) },
            //{ OpCodes.Stind_I2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_I2, InstrCodesDb.Stind_I2_) },
            //{ OpCodes.Stind_I4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_I4, InstrCodesDb.Stind_I4_) },
            //{ OpCodes.Stind_I8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_I8, InstrCodesDb.Stind_I8_) },
            //{ OpCodes.Stind_R4.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_R4, InstrCodesDb.Stind_R4_) },
            //{ OpCodes.Stind_R8.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_R8, InstrCodesDb.Stind_R8_) },
            //{ OpCodes.Stind_Ref.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stind_Ref, InstrCodesDb.Stind_Ref_) },
            //{ OpCodes.Stloc.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc, InstrCodesDb.Stloc_) },
            //{ OpCodes.Stloc_0.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc_0, InstrCodesDb.Stloc_0_) },
            //{ OpCodes.Stloc_1.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc_1, InstrCodesDb.Stloc_1_) },
            //{ OpCodes.Stloc_2.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc_2, InstrCodesDb.Stloc_2_) },
            //{ OpCodes.Stloc_3.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc_3, InstrCodesDb.Stloc_3_) },
            //{ OpCodes.Stloc_S.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stloc_S, InstrCodesDb.Stloc_S_) },
            //{ OpCodes.Stobj.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stobj, InstrCodesDb.Stobj_) },
            //{ OpCodes.Stsfld.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Stsfld, InstrCodesDb.Stsfld_) },
            { OpCodes.Sub.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Sub, InstrCodesDb.Sub_) },
            { OpCodes.Sub_Ovf.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Sub_Ovf, InstrCodesDb.Sub_ovf_) },
            { OpCodes.Sub_Ovf_Un.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Sub_Ovf_Un, InstrCodesDb.Sub_ovf_un_) },
            //{ OpCodes.Switch.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Switch, InstrCodesDb.Switch_) },
            //{ OpCodes.Tailcall.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Tailcall, InstrCodesDb.Tailcall_) },
            //{ OpCodes.Throw.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Throw, InstrCodesDb.Throw_) },
            //{ OpCodes.Unaligned.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Unaligned, InstrCodesDb.Unaligned_) },
            //{ OpCodes.Unbox.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Unbox, InstrCodesDb.Unbox_) },
            //{ OpCodes.Unbox_Any.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Unbox_Any, InstrCodesDb.Unbox_Any_) },
            //{ OpCodes.Volatile.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Volatile, InstrCodesDb.Volatile_) },
            { OpCodes.Xor.Value, new KeyValuePair<OpCode, VmInstrInfo>(OpCodes.Xor, InstrCodesDb.Xor_) },
        };
        #endregion
        public override Stream CreateVmStream(Type rt, ParameterInfo[] pi, Type[] locals, byte [] ilBytes /*GetIlBytes(src)*/)
        {
            var ret = new MemoryStream();
            var wr = new BinaryWriter(ret);
            var fwdTypeDefs = new Dictionary<Type, int>();
            var fwdTypeRefs = new Dictionary<int, Type>();
            var fwdMethodTokenDefs = new Dictionary<UInt32, int>();
            var fwdMethodTokenRefs = new Dictionary<long, UInt32>();
            var deferredBrTargets = new Dictionary<long, UInt32>();
            var srcToDstInstrPositions = new Dictionary<long, UInt32>();
            wr.Write(0); // ClassId
            wr.Flush();
            const int dummy = 0x55555555;
            fwdTypeDefs.Add(rt, dummy);
            fwdTypeRefs.Add((int)ret.Position, rt);
            wr.Write(dummy); // ReturnTypeId fwd ref
            wr.Write((ushort)0); // LocalVarTypes
            wr.Write((byte)2); // Flags - static
            wr.Write((byte)0); // Name
            var pars = pi;
            wr.Write((ushort)pars.Length); // ArgsTypeToOutput
            foreach (var parameterInfo in pars)
            {
                if (!fwdTypeDefs.ContainsKey(parameterInfo.ParameterType))
                    fwdTypeDefs.Add(parameterInfo.ParameterType, dummy);
                fwdTypeRefs.Add((int)ret.Position, parameterInfo.ParameterType);
                wr.Write(dummy); // parTypeId fwd ref
                wr.Write(false);
            }
            wr.Write((ushort)locals.Length); // LocalVarTypes
            foreach (var lt in locals)
            {
                if (!fwdTypeDefs.ContainsKey(lt))
                    fwdTypeDefs.Add(lt, dummy);
                fwdTypeRefs.Add((int)ret.Position, lt);
                wr.Write(dummy); // parTypeId fwd ref
            }

            var methodLengthPos = ret.Position;
            wr.Write(dummy); // methodLength
            using (var sr = new BinaryReader(new MemoryStream(ilBytes)))
            {
                while (sr.BaseStream.Position < sr.BaseStream.Length)
                {
                    srcToDstInstrPositions[sr.BaseStream.Position] = (uint)ret.Position;
                    short ilByte = sr.ReadByte();
                    if (ilByte == 0xFE)
                        ilByte = (short)((ilByte << 8) | sr.ReadByte());
                    Assert.IsTrue(IlToVmInstrInfo.ContainsKey(ilByte), "Extend IlToVmInstrInfo with {0:X} or use short jump", ilByte);
                    var vmInstrInfo = IlToVmInstrInfo[ilByte];
                    var id = vmInstrInfo.Value.Id;
                    wr.Write((ushort)(id >> 16));
                    wr.Write((ushort)(id & 0xFFFF));
                    wr.Flush();
                    ulong operand = 0;
                    switch (vmInstrInfo.Key.OperandType)
                    {
                        case OperandType.InlineNone:
                            break;
                        case OperandType.InlineI8:
                        case OperandType.InlineR:
                            operand = sr.ReadUInt64();
                            break;
                        case OperandType.InlineVar:
                            operand = sr.ReadUInt16();
                            break;
                        case OperandType.ShortInlineBrTarget:
                        case OperandType.ShortInlineI:
                        case OperandType.ShortInlineVar:
                            operand = sr.ReadByte();
                            break;
                        default:
                            operand = sr.ReadUInt32();
                            break;
                    }
                    if (vmInstrInfo.Key.OperandType == OperandType.ShortInlineBrTarget)
                    {
                        deferredBrTargets[wr.BaseStream.Position] = (uint)sr.BaseStream.Position + (uint)operand;
                    }
                    if (vmInstrInfo.Key.Value == OpCodes.Newobj.Value || vmInstrInfo.Key.Value == OpCodes.Call.Value)
                    {
                        var token = (uint)operand;
                        if (!fwdMethodTokenDefs.ContainsKey(token))
                            fwdMethodTokenDefs.Add(token, dummy);
                        fwdMethodTokenRefs[wr.BaseStream.Position] = token;
                    }
                    switch (vmInstrInfo.Value.OperandType)
                    {
                        case VmOperandType.Ot11Nope:
                            break;
                        case VmOperandType.Ot2Byte:
                        case VmOperandType.Ot6SByte:
                        case VmOperandType.Ot8Byte:
                            wr.Write((byte)operand);
                            break;
                        case VmOperandType.Ot1UShort:
                        case VmOperandType.Ot3UShort:
                            wr.Write((ushort)operand);
                            break;
                        case VmOperandType.Ot0UInt:
                        case VmOperandType.Ot12Int:
                        case VmOperandType.Ot5Int:
                        case VmOperandType.Ot10Float:
                            wr.Write((uint)operand);
                            break;
                        case VmOperandType.Ot7Long:
                        case VmOperandType.Ot4Double:
                            wr.Write(operand);
                            break;
                        default:
                            throw new InvalidDataException("OperandType");
                    }
                }
                ret.Position = methodLengthPos;
                wr.Write((int)(ret.Length - methodLengthPos - 4));
                wr.Flush();
                ret.Position = ret.Length;
                foreach (var t in fwdTypeDefs.Keys.ToArray())
                {
                    fwdTypeDefs[t] = (int)ret.Position;
                    wr.Write(true);
                    wr.Write((byte)0);
                    wr.Write(-1);
                    wr.Write(-1);
                    wr.Write(false);
                    // ReSharper disable once AssignNullToNotNullAttribute
                    wr.Write(t.AssemblyQualifiedName);
                    wr.Write(false);
                    wr.Write((ushort)0);
                    wr.Flush();
                }
                foreach (var t in fwdMethodTokenDefs.Keys.ToArray())
                {
                    fwdMethodTokenDefs[t] = (int)ret.Position;
                    wr.Write((byte)0);
                    wr.Write(t);
                    wr.Flush();
                }
                foreach (var r in fwdTypeRefs)
                {
                    ret.Position = r.Key;
                    wr.Write(fwdTypeDefs[r.Value]);
                    wr.Flush();
                }
                foreach (var t in fwdMethodTokenRefs)
                {
                    ret.Position = t.Key;
                    wr.Write((ushort)(fwdMethodTokenDefs[t.Value]>>16));
                    wr.Write((ushort)fwdMethodTokenDefs[t.Value]);
                    wr.Flush();
                }
                foreach (var r in deferredBrTargets)
                {
                    ret.Position = r.Key;
                    var srcPos = r.Value;
                    wr.Write(srcToDstInstrPositions[srcPos] - (uint)methodLengthPos - 4);
                    wr.Flush();
                }
                ret.Position = 0;
                return ret;
            }
        }
        public override object Invoke(object[] parameters, Stream vmStream)
        {
            var vmStreamWrapper = new VmStreamWrapper(vmStream, VmExecutor.VmXorKey());
            var callees = new object[]
            {
                Assembly.GetCallingAssembly()
            };
            var vm = new VmExecutor(InstrCodesDb, vmStreamWrapper);
            vm.Seek(0, vmStreamWrapper, null);
            var vmRet = vm.Invoke(parameters, null, null, callees);
            return vmRet;
        }
    }

    internal static class ExtendedEcma335
    {
        internal static object AsStackedValue(object v, bool signedComparison) // как это значение будет лежать на стеке
        {
            Type t = v.GetType();
            if (IsFloatingType(t))
                return v;
            //остальное приводится к Int32 или Int64
            if (t.IsEnum) t = t.GetEnumUnderlyingType();
            if (t == typeof (byte)) return signedComparison ? (object) (Int32) (byte) v : (UInt32) (byte)v;
            if (t == typeof (sbyte)) return signedComparison ? (object) (Int32) (sbyte) v : (UInt32) (sbyte)v;
            if (t == typeof (short)) return signedComparison ? (object) (Int32) (short) v : (UInt32) (short)v;
            if (t == typeof (ushort)) return signedComparison ? (object) (Int32) (ushort)v : (UInt32)(ushort) v;
            if (t == typeof (Char)) return signedComparison ? (object) (Int32) (Char)v : (UInt32) (Char) v;
            if (t == typeof (Int32)) return signedComparison ? (object) (Int32) v : (UInt32) (Int32)v;
            if (t == typeof (UInt32)) return signedComparison ? (object) (Int32) (UInt32) v : (UInt32)v;
            if (t == typeof(Int64)) return signedComparison ? (object) (Int64)v : (UInt64) (Int64)v;
            if (t == typeof (UInt64)) return signedComparison ? (object) (Int64) (UInt64) v : (UInt64)v;
            if (t == typeof(bool)) return signedComparison ? (object) ((bool)v ? 1 : 0) : (UInt32) ((bool)v ? 1 : 0);
            if (t == typeof(IntPtr))
            {
                if (IntPtr.Size == 4)
                    return signedComparison ? (object)((IntPtr)v).ToInt32() : (UInt32)((IntPtr)v).ToInt32();
                return signedComparison ? (object)((IntPtr)v).ToInt64() : (UInt64)((IntPtr)v).ToInt64();
            }
            if (t == typeof(UIntPtr))
            {
                if (UIntPtr.Size == 4)
                    return signedComparison ? (object)(Int32)((UIntPtr)v).ToUInt32() : ((UIntPtr)v).ToUInt32();
                return signedComparison ? (object)(Int64)((UIntPtr)v).ToUInt64() : ((UIntPtr)v).ToUInt64();
            }
            return v;
        }

        [SuppressMessage("ReSharper", "RedundantCast")]
        internal static bool Check(KeyValuePair<OpCode, UnitTestCombine.CodeTemplate> op, object[] parameters, object vmRet)
        {
            try
            {
                if (parameters.Length == 2 && op.Value == UnitTestCombine.CodeTemplate.TwoOpAny &&
                    (op.Key.Value == OpCodes.Shl.Value || op.Key.Value == OpCodes.Shr.Value ||
                     op.Key.Value == OpCodes.Shr_Un.Value))
                {
                    var t0 = parameters[0].GetType();
                    var t1 = parameters[1].GetType();
                    if (t0 == typeof (object) || t1 == typeof (object))
                        return true; // TODO понять бы, почему нет совпадения с фреймфорком
                    if (IsFloatingType(t0) || IsFloatingType(t1))
                        return true; // в зависимости от разрядности фреймворк дурит по-разному
                    bool sign = op.Key.Value != OpCodes.Shr_Un.Value;
                    var commonType = UnitTestCombine.CompatibleType(t0, t1, sign);
                    if (commonType == typeof (Int64) || commonType == typeof (UInt64))
                    {
                        if (sign)
                        {
                            var p0 = Convert.ToInt64(AsStackedValue(parameters[0], true));
                            var p1 = Convert.ToInt64(AsStackedValue(parameters[1], true));
                            return Convert.ToInt64(AsStackedValue(vmRet, true)) ==
                                   ((op.Key.Value == OpCodes.Shl.Value) ? (p1 << (int)p0) : (p1 >> (int)p0));
                        }
                        else
                        {
                            var p0 = Convert.ToUInt64(AsStackedValue(parameters[0], false));
                            var p1 = Convert.ToUInt64(AsStackedValue(parameters[1], false));
                            return Convert.ToUInt64(AsStackedValue(vmRet, false)) == (p1 >> (int) p0);
                        }
                    }
                }
                if (parameters.Length == 2 && op.Value == UnitTestCombine.CodeTemplate.TwoOpAny && 
                    (op.Key.Value == OpCodes.And.Value || op.Key.Value == OpCodes.Or.Value || op.Key.Value == OpCodes.Xor.Value))
                {
                    var t0 = parameters[0].GetType();
                    var t1 = parameters[1].GetType();
                    if (t0 == typeof (object) || t1 == typeof (object))
                        return true; // TODO понять бы, почему нет совпадения с фреймфорком
                    if (IntPtr.Size == 8)
                    {
                        if (t0 == typeof (double) || t1 == typeof (double))
                            // ReSharper disable once CompareOfFloatsByEqualityOperator
                            return Convert.ToDouble(vmRet) == ((4 == IntPtr.Size) ? Double.NaN : (double) 0);
                        if (t0 == typeof (float) || t1 == typeof (float))
                            // ReSharper disable once CompareOfFloatsByEqualityOperator
                            return Convert.ToSingle(vmRet) == ((4 == IntPtr.Size) ? Single.NaN : (float) 0);
                    }
                    //x64, как более точный в фреймворке, не даёт сюда срабатываний
                    if (IntPtr.Size == 4 && t0 != t1)
                    {
                        var p0 = Convert.ToInt64(AsStackedValue(parameters[0], true));
                        var p1 = Convert.ToInt64(AsStackedValue(parameters[1], true));
                        Int64 check = 0;
                        if (op.Key.Value == OpCodes.And.Value) check = p0 & p1;
                        if (op.Key.Value == OpCodes.Or.Value) check = p0 | p1;
                        if (op.Key.Value == OpCodes.Xor.Value) check = p0 ^ p1;
                        return Convert.ToInt64(AsStackedValue(vmRet, true)) == check;
                    }
                }
                bool bAdd = op.Key.Value == OpCodes.Add.Value || op.Key.Value == OpCodes.Add_Ovf.Value ||
                            op.Key.Value == OpCodes.Add_Ovf_Un.Value;
                bool bSub = op.Key.Value == OpCodes.Sub.Value || op.Key.Value == OpCodes.Sub_Ovf.Value ||
                            op.Key.Value == OpCodes.Sub_Ovf_Un.Value;
                bool bMul = op.Key.Value == OpCodes.Mul.Value || op.Key.Value == OpCodes.Mul_Ovf.Value ||
                            op.Key.Value == OpCodes.Mul_Ovf_Un.Value;
                bool bDiv = op.Key.Value == OpCodes.Div.Value || op.Key.Value == OpCodes.Div_Un.Value;
                bool bRem = op.Key.Value == OpCodes.Rem.Value || op.Key.Value == OpCodes.Rem_Un.Value;
                if (parameters.Length == 2 && op.Value == UnitTestCombine.CodeTemplate.TwoOpAny && (bAdd || bSub || bMul || bDiv || bRem))
                {
                    var t0 = parameters[0].GetType();
                    var t1 = parameters[1].GetType();
                    if ((t0 == typeof (object) || t1 == typeof (object)) && (!(vmRet is Exception) || (op.Key.Value == OpCodes.Mul_Ovf.Value) || (op.Key.Value == OpCodes.Mul_Ovf_Un.Value) || (op.Key.Value == OpCodes.Sub_Ovf_Un.Value)))
                        return true; // TODO понять бы, почему нет совпадения с фреймфорком
                    // баг фреймворка в x64 Mul_Ovf_Un для плавающих
                    if (t0 == t1 && !(IntPtr.Size == 8 && op.Key.Value == OpCodes.Mul_Ovf_Un.Value && IsFloatingType(t0) && t0==t1)) return false;
                    bool sign = op.Key.Value != OpCodes.Add_Ovf_Un.Value && op.Key.Value != OpCodes.Sub_Ovf_Un.Value && op.Key.Value != OpCodes.Mul_Ovf_Un.Value && op.Key.Value != OpCodes.Div_Un.Value && op.Key.Value != OpCodes.Rem_Un.Value;
                    var commonType = UnitTestCombine.CompatibleType(t0, t1, sign);
                    if (commonType == typeof (Int64) || commonType == typeof (UInt64))
                    {
                        if (sign)
                        {
                            var p0 = Convert.ToInt64(AsStackedValue(parameters[0], true));
                            var p1 = Convert.ToInt64(AsStackedValue(parameters[1], true));
                            if (vmRet is OverflowException && (op.Key.Value == OpCodes.Add_Ovf.Value || op.Key.Value == OpCodes.Sub_Ovf.Value || op.Key.Value == OpCodes.Mul_Ovf.Value))
                            {
                                decimal res = bAdd ? ((decimal)p0 + (decimal)p1) : bSub ? ((decimal)p1 - (decimal)p0) : ((decimal)p1 * (decimal)p0);
                                if (commonType == typeof(Int64))
                                    return res > (decimal)Int64.MaxValue || res < (decimal)Int64.MinValue;
                                if (commonType == typeof(UInt64))
                                    return res > (decimal)UInt64.MaxValue || res < 0;
                            }
                            else
                            {
                                if (p0 == 0 && (bDiv || bRem)) return vmRet is DivideByZeroException;
                                try
                                {
                                    return (bAdd ? (p0 + p1) : bSub ? (p1 - p0) : bMul ? (p1 * p0) : bDiv ? (p1 / p0) : (p1 % p0)) == Convert.ToInt64(AsStackedValue(vmRet, true));
                                }
                                catch (Exception e)
                                {
                                    return e.GetType() == vmRet.GetType();
                                }
                            }
                        } else
                        {
                            var p0 = Convert.ToUInt64(AsStackedValue(parameters[0], false));
                            var p1 = Convert.ToUInt64(AsStackedValue(parameters[1], false));
                            if (vmRet is OverflowException && op.Key.Value == OpCodes.Sub_Ovf_Un.Value && 4 == IntPtr.Size)
                            {
                                decimal res = (decimal)p1 - (decimal)p0;
                                if (commonType == typeof(UInt64))
                                    return res > (decimal)UInt64.MaxValue || res < 0;
                            }
                            else
                            {
                                if (p0 == 0 && (bDiv || bRem)) return vmRet is DivideByZeroException;
                                return Convert.ToUInt64(AsStackedValue(vmRet, false)) == (bAdd ? (p0 + p1) : bSub ? (p1 - p0) : bMul ? (p1 * p0) : bDiv ? (p1 / p0) : (p1 % p0));
                            }
                        }
                    } else if (commonType == typeof(float))
                    {
                        var p0 = Convert.ToSingle(AsStackedValue(parameters[0], sign));
                        var p1 = Convert.ToSingle(AsStackedValue(parameters[1], sign));
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        if (p0 == 0 && (bDiv || bRem)) return vmRet is DivideByZeroException;
                        var vm = Convert.ToSingle(AsStackedValue(vmRet, sign));
                        if (float.IsNaN(p0) || float.IsNaN(p1) || float.IsNaN(vm)) return true;
                        // ReSharper disable once RedundantCast
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return vm == (float)(bAdd ? (p0 + p1) : bSub ? (p1 - p0) : bMul ? (p1 * p0) : bDiv ? (p1 / p0) : (p1 % p0));
                    } else if (commonType == typeof(double))
                    {
                        var p0 = Convert.ToDouble(AsStackedValue(parameters[0], sign));
                        var p1 = Convert.ToDouble(AsStackedValue(parameters[1], sign));
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        if (p0 == 0 && (bDiv || bRem)) return vmRet is DivideByZeroException;
                        var vm = Convert.ToDouble(AsStackedValue(vmRet, sign));
                        if (double.IsNaN(p0) || double.IsNaN(p1) || double.IsNaN(vm)) return true;
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return vm == (bAdd ? (p0 + p1) : bSub ? (p1 - p0) : bMul ? (p1 * p0) : bDiv ? (p1 / p0) : (p1 % p0));
                    }
                }
                if (parameters.Length == 1 && op.Value == UnitTestCombine.CodeTemplate.SingleOpAny && op.Key.Value == OpCodes.Conv_U.Value)
                {
                    if (IsFloatingType(parameters[0].GetType()) && !(vmRet is Exception))
                    {
                        // Framework использует мусор, мы возврвщаем 0
                        // ReSharper disable once CompareOfFloatsByEqualityOperator
                        return Convert.ToDouble(vmRet) == 0.0;
                    }
                }
                if (parameters.Length == 1 && op.Value == UnitTestCombine.CodeTemplate.SingleOpAny && op.Key.Value == OpCodes.Ckfinite.Value)
                {
                    if (!IsFloatingType(parameters[0].GetType()) && !(vmRet is Exception))
                    {
                        double d = Double.NaN;
                        if (parameters[0] is IConvertible)
                        {
                            d = Convert.ToDouble(AsStackedValue(parameters[0], true));
                        }
                        return Convert.ToDouble(vmRet).Equals(d);
                    }
                }
                if (parameters.Length == 2 && (op.Value == UnitTestCombine.CodeTemplate.BranchTwoOpBool || op.Value == UnitTestCombine.CodeTemplate.TwoOpBool))
                {
                    var signedComparison = (op.Key.Value == OpCodes.Bgt_S.Value || op.Key.Value == OpCodes.Ble_S.Value ||
                                            op.Key.Value == OpCodes.Bge_S.Value || op.Key.Value == OpCodes.Blt_S.Value || op.Key.Value == OpCodes.Beq_S.Value ||
                                            op.Key.Value == OpCodes.Cgt.Value || op.Key.Value == OpCodes.Clt.Value || op.Key.Value == OpCodes.Ceq.Value);
                    var unsignedUnorderedComparison = (op.Key.Value == OpCodes.Bgt_Un_S.Value || op.Key.Value == OpCodes.Ble_Un_S.Value ||
                                            op.Key.Value == OpCodes.Bge_Un_S.Value || op.Key.Value == OpCodes.Blt_Un_S.Value ||
                                            op.Key.Value == OpCodes.Cgt_Un.Value || op.Key.Value == OpCodes.Clt_Un.Value || op.Key.Value == OpCodes.Bne_Un_S.Value);
                    if (signedComparison || unsignedUnorderedComparison)
                    {
                        var isFloat = parameters.Select(o => IsFloatingType(o.GetType())).ToArray();
                        // разрешаем сравнивать такие типы
                        if (isFloat[0] != isFloat[1])
                        {
                            //F and int32/64
                            var vStacked = parameters.Select(o => AsStackedValue(o, true)).ToArray();
                            if (vStacked.Any(o => !(o is IConvertible)))
                            {
                                return (bool)vmRet == (op.Key.Value == OpCodes.Bne_Un_S.Value);
                            }
                            double a = Convert.ToDouble(vStacked[1]), b = Convert.ToDouble(vStacked[0]);
                            return Compare(op, vmRet, a, b, unsignedUnorderedComparison);
                        }
                        else if (isFloat.All(o => o == false))
                        {
                            var vStacked = parameters.Select(o => AsStackedValue(o, signedComparison)).ToArray();
                            if (vStacked.Any(o => !(o is IConvertible)))
                            {
                                return (bool)vmRet == false;
                            }
                            var vStackSize = vStacked.Select(o => Marshal.SizeOf(o.GetType())).ToArray();
                            if (vStackSize[0] != vStackSize[1])
                            {
                                //32 bit and 64 bit ints
                                if (signedComparison)
                                {
                                    Int64 a = (vStackSize[1] == 8) ? (Int64)vStacked[1] : (Int32)vStacked[1];
                                    Int64 b = (vStackSize[0] == 8) ? (Int64)vStacked[0] : (Int32)vStacked[0];
                                    return Compare(op, vmRet, a, b);
                                }
                                else
                                {
                                    UInt64 a = (vStackSize[1] == 8) ? (UInt64)vStacked[1] : (UInt32)vStacked[1];
                                    UInt64 b = (vStackSize[0] == 8) ? (UInt64)vStacked[0] : (UInt32)vStacked[0];
                                    return Compare(op, vmRet, a, b);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception exc)
            {
                Console.Error.WriteLine(exc);
            }

            return false;
        }

        private static bool Compare(KeyValuePair<OpCode, UnitTestCombine.CodeTemplate> op, object vmRet, Double a, Double b, bool unordered)
        {
            if (unordered && (Double.IsNaN(a) || Double.IsNaN(b)))
            {
                return true;
            }
            if (op.Key.Value == OpCodes.Bgt_S.Value || op.Key.Value == OpCodes.Bgt_Un_S.Value || op.Key.Value == OpCodes.Cgt.Value || op.Key.Value == OpCodes.Cgt_Un.Value)
                return ((a > b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Ble_S.Value || op.Key.Value == OpCodes.Ble_Un_S.Value)
                return ((a <= b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Bge_S.Value || op.Key.Value == OpCodes.Bge_Un_S.Value)
                return ((a >= b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Blt_S.Value || op.Key.Value == OpCodes.Blt_Un_S.Value || op.Key.Value == OpCodes.Clt.Value || op.Key.Value == OpCodes.Clt_Un.Value)
                return ((a < b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Ceq.Value || op.Key.Value == OpCodes.Beq_S.Value)
                // ReSharper disable once CompareOfFloatsByEqualityOperator
                return ((a == b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Bne_Un_S.Value)
                // ReSharper disable once CompareOfFloatsByEqualityOperator
                return ((a != b) == (bool)vmRet);
            Assert.Fail("Bad op for Compare");
            return false;
        }

        private static bool Compare<T>(KeyValuePair<OpCode, UnitTestCombine.CodeTemplate> op, object vmRet, T a, T b) where T : IComparable<T>
        {
            if (op.Key.Value == OpCodes.Bgt_S.Value || op.Key.Value == OpCodes.Bgt_Un_S.Value || op.Key.Value == OpCodes.Cgt.Value || op.Key.Value == OpCodes.Cgt_Un.Value)
                return ((a.CompareTo(b) > 0) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Ble_S.Value || op.Key.Value == OpCodes.Ble_Un_S.Value)
                return ((a.CompareTo(b) <= 0) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Bge_S.Value || op.Key.Value == OpCodes.Bge_Un_S.Value)
                return ((a.CompareTo(b) >= 0) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Blt_S.Value || op.Key.Value == OpCodes.Blt_Un_S.Value || op.Key.Value == OpCodes.Clt.Value || op.Key.Value == OpCodes.Clt_Un.Value)
                return ((a.CompareTo(b) < 0) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Ceq.Value || op.Key.Value == OpCodes.Beq_S.Value)
                return (a.Equals(b) == (bool)vmRet);
            if (op.Key.Value == OpCodes.Bne_Un_S.Value)
                return ((!a.Equals(b)) == (bool)vmRet);
            Assert.Fail("Bad op for Compare");
            return false;
        }

        private static bool IsFloatingType(Type t)
        {
            return t == typeof (Single) || t == typeof (Double);
        }
    }
}
