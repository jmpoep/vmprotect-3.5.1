using System;
using System.Runtime.InteropServices;

// ReSharper disable once CheckNamespace
namespace VMProtect
{
	public static class CpuId
	{
		public static int[] Invoke(int level)
		{
			var codePointer = IntPtr.Zero;
			try
			{
				// compile
				byte[] codeBytes;
				if (IntPtr.Size == 4)
				{
					codeBytes = new byte[30];
					codeBytes[0] = 0x55;                                                // push        ebp  
					codeBytes[1] = 0x8B; codeBytes[2] = 0xEC;                           // mov         ebp,esp
					codeBytes[3] = 0x53;                                                // push        ebx  
					codeBytes[4] = 0x57;                                                // push        edi
					codeBytes[5] = 0x8B; codeBytes[6] = 0x45; codeBytes[7] = 0x08;      // mov         eax, dword ptr [ebp+8] (move level into eax)
					codeBytes[8] = 0x0F; codeBytes[9] = 0xA2;                           // cpuid
					codeBytes[10] = 0x8B; codeBytes[11] = 0x7D; codeBytes[12] = 0x0C;   // mov         edi, dword ptr [ebp+12] (move address of buffer into edi)
					codeBytes[13] = 0x89; codeBytes[14] = 0x07;                         // mov         dword ptr [edi+0], eax  (write eax, ... to buffer)
					codeBytes[15] = 0x89; codeBytes[16] = 0x5F; codeBytes[17] = 0x04;   // mov         dword ptr [edi+4], ebx 
					codeBytes[18] = 0x89; codeBytes[19] = 0x4F; codeBytes[20] = 0x08;   // mov         dword ptr [edi+8], ecx 
					codeBytes[21] = 0x89; codeBytes[22] = 0x57; codeBytes[23] = 0x0C;   // mov         dword ptr [edi+12],edx 
					codeBytes[24] = 0x5F;                                               // pop         edi  
					codeBytes[25] = 0x5B;                                               // pop         ebx  
					codeBytes[26] = 0x8B; codeBytes[27] = 0xE5;                         // mov         esp,ebp  
					codeBytes[28] = 0x5D;                                               // pop         ebp 
					codeBytes[29] = 0xc3;                                               // ret
				} else
				{
					codeBytes = new byte[26];
					codeBytes[0] = 0x53;																	// push rbx    this gets clobbered by cpuid
					codeBytes[1] = 0x49; codeBytes[2] = 0x89; codeBytes[3] = 0xd0;							// mov r8,  rdx
					codeBytes[4] = 0x89; codeBytes[5] = 0xc8;												// mov eax, ecx
					codeBytes[6] = 0x0F; codeBytes[7] = 0xA2;												// cpuid
					codeBytes[8] = 0x41; codeBytes[9] = 0x89; codeBytes[10] = 0x40; codeBytes[11] = 0x00;	// mov    dword ptr [r8+0],  eax
					codeBytes[12] = 0x41; codeBytes[13] = 0x89; codeBytes[14] = 0x58; codeBytes[15] = 0x04;	// mov    dword ptr [r8+4],  ebx
					codeBytes[16] = 0x41; codeBytes[17] = 0x89; codeBytes[18] = 0x48; codeBytes[19] = 0x08;	// mov    dword ptr [r8+8],  ecx
					codeBytes[20] = 0x41; codeBytes[21] = 0x89; codeBytes[22] = 0x50; codeBytes[23] = 0x0c;	// mov    dword ptr [r8+12], edx
					codeBytes[24] = 0x5b;																	// pop rbx
					codeBytes[25] = 0xc3;																	// ret
				}

				codePointer = Win32.VirtualAlloc(
					IntPtr.Zero,
					new UIntPtr((uint)codeBytes.Length),
					Win32.AllocationType.Commit | Win32.AllocationType.Reserve,
					Win32.MemoryProtection.ExecuteReadWrite
				);

				Marshal.Copy(codeBytes, 0, codePointer, codeBytes.Length);

				var cpuIdDelg = (CpuIdDelegate)Marshal.GetDelegateForFunctionPointer(codePointer, typeof(CpuIdDelegate));

				// invoke
				var buffer = new int[4];
				var handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
				try
				{
					cpuIdDelg(level, buffer);
				}
				finally
				{
					handle.Free();
				}

				return buffer;
			}
			finally
			{
				if (codePointer != IntPtr.Zero)
				{
					Win32.VirtualFree(codePointer, UIntPtr.Zero, Win32.FreeType.Release);
				}
			}
		}

		[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
		private delegate void CpuIdDelegate(int level, int []buffer);
	}
}