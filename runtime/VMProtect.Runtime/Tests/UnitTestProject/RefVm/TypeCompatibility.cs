using System;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000055 RID: 85
    internal static class TypeCompatibility // \u000E\u2006
    {
	    // Token: 0x06000341 RID: 833 RVA: 0x000152D4 File Offset: 0x000134D4
	    public static bool Check(Type t1, Type t2) // t1
        {
		    if (t1 == t2)
		    {
			    return true;
		    }
		    if (t1 == null || t2 == null)
		    {
			    return false;
		    }
		    if (t1.IsByRef)
		    {
			    return t2.IsByRef && Check(t1.GetElementType(), t2.GetElementType());
		    }
		    if (t2.IsByRef)
		    {
			    return false;
		    }
		    if (t1.IsPointer)
		    {
			    return t2.IsPointer && Check(t1.GetElementType(), t2.GetElementType());
		    }
		    if (t2.IsPointer)
		    {
			    return false;
		    }
		    if (t1.IsArray)
		    {
			    return t2.IsArray && t1.GetArrayRank() == t2.GetArrayRank() && Check(t1.GetElementType(), t2.GetElementType());
		    }
		    if (t2.IsArray)
		    {
			    return false;
		    }
		    if (t1.IsGenericType && !t1.IsGenericTypeDefinition)
		    {
			    t1 = t1.GetGenericTypeDefinition();
		    }
		    if (t2.IsGenericType && !t2.IsGenericTypeDefinition)
		    {
			    t2 = t2.GetGenericTypeDefinition();
		    }
		    return t1 == t2;
	    }
    }
}
