using System;
using System.Diagnostics;

namespace UnitTestProject.RefVm
{
    // Token: 0x02000048 RID: 72
    internal static class ElementedTypeHelper // \u0008\u2005
    {
        // Token: 0x06000307 RID: 775 RVA: 0x000148D8 File Offset: 0x00012AD8
        public static Type TryGoToElementType(Type t) // \u0002
        {
            if (t.IsByRef || t.IsArray || t.IsPointer)
            {
                return TryGoToElementType(t.GetElementType());
            }
            return t;
        }

        // Token: 0x06000308 RID: 776 RVA: 0x00014900 File Offset: 0x00012B00
        public static Type TryGoToPointerOrReferenceElementType(Type t) // \u0003
        {
            if (t.HasElementType && !t.IsArray)
            {
                t = t.GetElementType();
            }
            return t;
        }

        // Token: 0x06000309 RID: 777 RVA: 0x00014920 File Offset: 0x00012B20
        public static MyCollection<ElementedTypeDescrItem> NestedElementTypes(Type type) // \u0002
        {
            var collection = new MyCollection<ElementedTypeDescrItem>();
            while (true)
            {
                Debug.Assert(type != null, "type != null");
                if(type == null) return collection;
                if (type.IsArray)
                {
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.Array1,
                        ArrayRank = type.GetArrayRank()
                    });
                }
                else if (type.IsByRef)
                {
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.ByRef2
                    });
                }
                else
                {
                    if (!type.IsPointer)
                    {
                        break;
                    }
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.Ponter0
                    });
                }
                type = type.GetElementType();
            }
            return collection;
        }

        // Token: 0x0600030A RID: 778 RVA: 0x000149B0 File Offset: 0x00012BB0
        public static MyCollection<ElementedTypeDescrItem> NestedElementTypes(string text) // \u0002
        {
            var collection = new MyCollection<ElementedTypeDescrItem>();
            while (true)
            {
                if (text.EndsWith(StringDecryptor.GetString(-1550346966) /* & */, StringComparison.Ordinal))
                {
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.ByRef2
                    });
                    text = text.Substring(0, text.Length - 1);
                }
                else if (text.EndsWith(StringDecryptor.GetString(-1550346958) /* * */, StringComparison.Ordinal))
                {
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.Ponter0
                    });
                    text = text.Substring(0, text.Length - 1);
                }
                else if (text.EndsWith(StringDecryptor.GetString(-1550346950) /* [] */, StringComparison.Ordinal))
                {
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.Array1,
                        ArrayRank = 1
                    });
                    text = text.Substring(0, text.Length - 2);
                }
                else
                {
                    if (!text.EndsWith(StringDecryptor.GetString(-1550346811) /* ,] */, StringComparison.Ordinal))
                    {
                        return collection;
                    }
                    var rank = 1;
                    var remainLen = -1;
                    for (var i = text.Length - 2; i >= 0; i--)
                    {
                        var c = text[i];
                        if (c != ',')
                        {
                            if (c != '[')
                            {
                                throw new InvalidOperationException(StringDecryptor.GetString(-1550346804) /* VM-3012 */);
                            }
                            remainLen = i;
                            i = -1;
                        }
                        else
                        {
                            rank++;
                        }
                    }
                    if (remainLen < 0)
                    {
                        throw new InvalidOperationException(StringDecryptor.GetString(-1550346790) /* VM-3014 */);
                    }
                    text = text.Substring(0, remainLen);
                    collection.PushBack(new ElementedTypeDescrItem
                    {
                        K = ElementedTypeDescrItem.Kind.Array1,
                        ArrayRank = rank
                    });
                }
            }
        }

        // Token: 0x0600030B RID: 779 RVA: 0x00014B30 File Offset: 0x00012D30
        public static Type PopType(Type type, MyCollection<ElementedTypeDescrItem> descr) // \u0002
        {
            while (descr.Count > 0)
            {
                var p = descr.PopBack();
                switch (p.K)
                {
                    case ElementedTypeDescrItem.Kind.Ponter0:
                        type = type.MakePointerType();
                        break;
                    case ElementedTypeDescrItem.Kind.Array1:
                        type = (p.ArrayRank == 1) ? type.MakeArrayType() : type.MakeArrayType(p.ArrayRank);
                        break;
                    case ElementedTypeDescrItem.Kind.ByRef2:
                        type = type.MakeByRefType();
                        break;
                }
            }
            return type;
        }
    }

    // Token: 0x02000054 RID: 84
    internal struct ElementedTypeDescrItem // \u000E\u2005
    {
        internal enum Kind { Ponter0, Array1, ByRef2 }

        // Token: 0x04000183 RID: 387
        public Kind K; // \u0002

        // Token: 0x04000184 RID: 388
        public int ArrayRank; // \u0003
    }
}