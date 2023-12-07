using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.ExceptionServices;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTestProject
{
    public abstract class MsilToVmTestCompiler
    {
        public enum LongEnum : long { Eval1 = 1, Evalm1 = -1 }
        public enum ULongEnum : ulong { Evalm1 = 0xFFFFFFFFFFFFFFFF }
        public enum ByteEnum : byte {}
        public enum SByteEnum : sbyte { }
        public enum IntEnum /*: int*/ { }
        public enum UIntEnum : uint { }
        public enum ShortEnum : short { }
        public enum UShortEnum : ushort { }

        public static byte[] GetIlBytes(DynamicMethod dynamicMethod)
        {
            // ReSharper disable once PossibleNullReferenceException
            var resolver = typeof(DynamicMethod).GetField("m_resolver", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(dynamicMethod);
            if (resolver == null) throw new ArgumentException("The dynamic method's IL has not been finalized.");
            // ReSharper disable once PossibleNullReferenceException
            return (byte[])resolver.GetType().GetField("m_code", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(resolver);
        }

        public abstract Stream CreateVmStream(Type rt, ParameterInfo[] pi, Type[] locals, byte[] ilBytes);

        public enum InvokeTestCombineError
        {
            NoError,
            NoErrorByStdEx,            // текущий стандарт ECMA-335 оставляет некоторые сценарии как "неопределенное поведение", но их можно определить
            VmOtherExceptionExpected,
            VmOtherTypeExpected,
            VmOtherValueExpected,
            Cnt
        };
        // dyn/vm обязаны что-то возвратить, либо выбросить исключение 
        // (в тестах не должно быть методов, возвращающих void, null или исключение в качестве retVal)
        [HandleProcessCorruptedStateExceptions]
        public InvokeTestCombineError InvokeTestCombine(DynamicMethod dyn, object[] parameters, out object dynRet, out object vmRet, out string err)
        {
            Type exceptionType = null;
            err = "";
            try
            {
                dynRet = dyn.Invoke(null, parameters);
                Assert.IsNotNull(dynRet);
                Assert.IsInstanceOfType(dynRet, dyn.ReturnType);
            }
            catch (Exception e)
            {
                Assert.IsInstanceOfType(e, typeof(TargetInvocationException));
                Assert.IsNotNull(e.InnerException);
                exceptionType = e.InnerException.GetType();
                dynRet = e.InnerException;
            }
            var vmStream = CreateVmStream(dyn.ReturnType, dyn.GetBaseDefinition().GetParameters(), 
                //FIXME dyn.GetMethodBody().LocalVariables.Select(o => o.LocalType).ToArray(),
                new Type[] {}, 
                GetIlBytes(dyn));
            try
            {
                vmRet = Invoke(parameters, vmStream);
                if(exceptionType != null)
                {
                    err = $"VmOtherExceptionExpected: {exceptionType.FullName}, actual: {vmRet}";
                    return InvokeTestCombineError.VmOtherExceptionExpected;
                }
                if (vmRet == null || vmRet.GetType() != dyn.ReturnType)
                {
                    err = $"VmOtherTypeExpected: {dyn.ReturnType.FullName}, actual: {((vmRet != null) ? vmRet.GetType().FullName : "null")}";
                    return InvokeTestCombineError.VmOtherTypeExpected;
                }
            }
            catch (Exception e)
            {
                vmRet = e;
                if (e.GetType() != exceptionType)
                {
                    err = $"VmOtherExceptionExpected: {((exceptionType != null) ? exceptionType.FullName : dynRet)}, actual: {e.GetType().FullName}";
                    return InvokeTestCombineError.VmOtherExceptionExpected;
                }
                return InvokeTestCombineError.NoError;
            }
            if (dynRet.Equals(vmRet))
            {
                return InvokeTestCombineError.NoError;
            }
            err = $"VmOtherValueExpected: {dynRet}, actual: {vmRet}";
            return InvokeTestCombineError.VmOtherValueExpected;
        }

        public abstract object Invoke(object[] parameters, Stream vmStream);
    }
}
