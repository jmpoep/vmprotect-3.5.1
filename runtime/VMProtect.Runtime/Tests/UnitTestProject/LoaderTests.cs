using System.Runtime.InteropServices;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using VMProtect;

namespace UnitTestProject
{
    [TestClass]
    public class LoaderTests
    {
        [TestMethod]
        public void LzmaDecode()
        {
            byte[] properties =
            {
                0x5d, 0, 0, 0, 1 // 5 bytes props
            };
            byte[] src =
            {
                0x00, 0x4e, 0x3a, 0x46, 0xeb, 0xe0, 0x06, 0x71, 0xc9, 0xe1, 
                0xe6, 0x37, 0xfd, 0x9b, 0xb6, 0xd0, 0x76, 0x3f, 0xc8, 0x73, 
                0xee, 0x11, 0xb6, 0x41, 0xaa, 0xb1, 0x7b, 0x7b, 0xef, 0xc6, 
                0x9f, 0xff, 0xf6, 0x1d, 0x28, 0x00 //36 bytes packed
            };
            byte [] dstExpected = { 0x9c, 0xe9, 0x57, 0xbe, 0x00, 0x00, 0xc3, 0xe9, 0x24, 0x2c, 0x01, 0x00, 0xe9, 0xef, 0x65, 0x00, 0x00, 0xe9, 0xc0, 0x41, 0x06, 0x00, 0xc3, 0x56, 0x57 }; 
            var pinnedSrc = GCHandle.Alloc(src, GCHandleType.Pinned);
            uint dstSize = 65536;
            var dstPtr = Marshal.AllocHGlobal((int)dstSize);
            try
            {
                dstSize = int.MaxValue;
                uint srcSize /*= (uint)src.Length*/;
                var res = Loader.LzmaDecode(dstPtr, pinnedSrc.AddrOfPinnedObject(), properties, ref dstSize, out srcSize);
                Assert.AreEqual(true, res);
                Assert.AreEqual(dstExpected.Length, (int)dstSize);
                var dst = new byte[dstSize];
                Marshal.Copy(dstPtr, dst, 0, (int)dstSize);
                CollectionAssert.AreEqual(dstExpected, dst);
                Assert.AreEqual(src.Length, (int)srcSize);
            }
            finally
            {
                pinnedSrc.Free();
                Marshal.FreeHGlobal(dstPtr);
            }
        }
    }
}
