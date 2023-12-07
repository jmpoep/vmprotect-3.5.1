using System;
using System.Threading;

namespace forms_cil
{ 
    // Token: 0x02000057 RID: 87
    internal interface I4 // \u000E\u2008
    {
        // Token: 0x06000346 RID: 838
        bool I4M(); // \u000E\u2008\u2008\u2000\u2002\u200A\u0002

        // Token: 0x06000347 RID: 839
        object M2(); // \u000E\u2008\u2008\u2000\u2002\u200A\u0002

        // Token: 0x06000348 RID: 840
        void M3(); // \u000E\u2008\u2008\u2000\u2002\u200A\u0002
    }

    // Token: 0x02000062 RID: 98
    internal interface I5 // \u000F\u2008
    {
        // Token: 0x06000397 RID: 919
        void I5M(); // \u000F\u2008\u2008\u2000\u2002\u200A\u0002
    }
    
    // Token: 0x0200000C RID: 12
    internal interface I1<out T> : I4, I5 // \u0002\u2009
    {
        // Token: 0x06000057 RID: 87
        T I1M(); // \u000E\u2008\u2008\u2000\u2002\u200A\u0002
    }

    // Token: 0x0200004B RID: 75
    internal interface I3 // \u0008\u2008
    {
        // Token: 0x0600031E RID: 798
        I4 M1(); // \u0008\u2008\u2008\u2000\u2002\u200A\u0002
    }

    // Token: 0x0200003C RID: 60
    internal interface I2<out T> : I3 // \u0006\u2008
    {
        // Token: 0x060002CD RID: 717
        I1<T> I2M(); // \u0008\u2008\u2008\u2000\u2002\u200A\u0002
    }

    // Token: 0x02000025 RID: 37
    internal static class SdTemplateStuff // \u0005\u2008
    {
        // Token: 0x02000026 RID: 38
        internal sealed class C : I2<int>, I1<int> // \u0002
        {
            // Token: 0x06000112 RID: 274 RVA: 0x00005A28 File Offset: 0x00003C28
            public C(int val)
            {
                _i2 = val;
                _i5 = Thread.CurrentThread.ManagedThreadId;
            }

            // Token: 0x06000113 RID: 275 RVA: 0x00005A48 File Offset: 0x00003C48
            void I5.I5M()
            {
            }

            // Token: 0x06000114 RID: 276 RVA: 0x00005A4C File Offset: 0x00003C4C
            bool I4.I4M()
            {
                switch (_i2)
                {
                    case 0:
                        _i2 = -1;
                        _i3 = -1496196691;
                        _i2 = 1;
                        return true;
                    case 1:
                        _i2 = -1;
                        _i3 = _i8 ^ 70939052;
                        _i2 = 2;
                        return true;
                    case 2:
                        _i2 = -1;
                        _i3 = _i8 ^ -1812634754;
                        _i2 = 3;
                        return true;
                    case 3:
                        _i2 = -1;
                        _i3 = -5623460;
                        _i2 = 4;
                        return true;
                    case 4:
                        _i2 = -1;
                        _i3 = 401181880;
                        _i2 = 5;
                        return true;
                    case 5:
                        _i2 = -1;
                        _i3 = 2075948002;
                        _i2 = 6;
                        return true;
                    case 6:
                        _i2 = -1;
                        _i3 = _i8 ^ 70939052;
                        _i2 = 7;
                        return true;
                    case 7:
                        _i2 = -1;
                        _i3 = -783689628;
                        _i2 = 8;
                        return true;
                    case 8:
                        _i2 = -1;
                        _i3 = _i8 ^ 70939052;
                        _i2 = 9;
                        return true;
                    case 9:
                        _i2 = -1;
                        return false;
                    default:
                        return false;
                }
            }

            // Token: 0x06000115 RID: 277 RVA: 0x00005BA8 File Offset: 0x00003DA8
            int I1<int>.I1M()
            {
                return _i3;
            }

            // Token: 0x06000116 RID: 278 RVA: 0x00005BB0 File Offset: 0x00003DB0
            void I4.M3()
            {
                throw new NotSupportedException();
            }

            // Token: 0x06000117 RID: 279 RVA: 0x00005BB8 File Offset: 0x00003DB8
            object I4.M2()
            {
                return _i3;
            }

            // Token: 0x06000118 RID: 280 RVA: 0x00005BC8 File Offset: 0x00003DC8
            I1<int> I2<int>.I2M()
            {
                C ret;
                if (_i2 == -2 && _i5 == Thread.CurrentThread.ManagedThreadId)
                {
                    _i2 = 0;
                    ret = this;
                }
                else
                {
                    ret = new C(0);
                }
                ret._i8 = I6;
                return ret;
            }

            // Token: 0x06000119 RID: 281 RVA: 0x00005C10 File Offset: 0x00003E10
            I4 I3.M1()
            {
                return ((I2<int>)this).I2M();
            }

            // Token: 0x0400003D RID: 61
            private int _i2; // \u0002

            // Token: 0x0400003E RID: 62
            private int _i3; // \u0003

            // Token: 0x0400003F RID: 63
            private readonly int _i5; // \u0005

            // Token: 0x04000040 RID: 64
            private int _i8; // \u0008

            // Token: 0x04000041 RID: 65
            public int I6; // \u0006
        }
    }
}