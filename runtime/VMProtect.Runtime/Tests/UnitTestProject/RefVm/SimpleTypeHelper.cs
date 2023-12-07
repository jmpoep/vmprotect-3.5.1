using System;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000009 RID: 9
    static class SimpleTypeHelper // \u0002\u2006
    {
	    // Token: 0x06000033 RID: 51 RVA: 0x000027AC File Offset: 0x000009AC
	    public static bool IsNullableGeneric(Type t) // \u0002
        {
		    return t.IsGenericType && t.GetGenericTypeDefinition() == NullableType; // \u0003
        }

	    // Token: 0x04000008 RID: 8
	    public static readonly Type ObjectType = typeof(object); // \u0002

	    // Token: 0x04000009 RID: 9
	    private static readonly Type NullableType = typeof(Nullable<>); // \u0003

        // Token: 0x0400000A RID: 10
        public static readonly Type TypedReferenceType = typeof(TypedReference); // \u0005

	    // Token: 0x0400000B RID: 11
	    public static readonly Type EnumType = typeof(Enum); // \u0008
    }
}
