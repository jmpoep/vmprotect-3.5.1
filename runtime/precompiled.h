#pragma once
#ifndef WR_PCH
#define WR_PCH

#ifndef VMP_GNU
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#endif

#include "precommon.h"

#ifdef WIN_DRIVER
void * __cdecl operator new(size_t size);
void __cdecl operator delete(void* p);
void __cdecl operator delete(void* p, size_t);
void * __cdecl operator new[](size_t size);
void __cdecl operator delete[](void *p);
#endif

#define RUNTIME

#ifdef VMP_GNU
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/ptrace.h>

#ifdef __APPLE__
#include <net/if_dl.h>
#include <net/if_types.h>
#include <sys/syslimits.h>
#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <mach/vm_map.h>
#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#define WINAPI
#ifdef __APPLE__
#define EXPORT_API __attribute__ ((visibility ("default")))
#else
#define EXPORT_API __attribute__ ((visibility ("protected")))
#endif

#else

#define EXPORT_API __declspec(dllexport)

#ifdef WIN_DRIVER
#include <windef.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif
void __cpuid(int a[4], int b);
void __nop();

#ifdef _WIN64
unsigned __int64 __readeflags(void);
void __writeeflags(unsigned __int64);
#else
unsigned __readeflags(void);
void __writeeflags(unsigned);
#endif

#if defined(__cplusplus)
}
#endif // WIN_DRIVER

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef HANDLE              HGLOBAL;
typedef unsigned int        UINT;
#define WINAPI      __stdcall
#define CRITICAL_SECTION KMUTEX

#ifdef _WIN64
typedef INT_PTR (FAR WINAPI *FARPROC)();
#else
typedef int (FAR WINAPI *FARPROC)();
#endif  // _WIN64

#else
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <wtsapi32.h>
#include <winternl.h>
#include <tlhelp32.h>
#endif
#endif // VMP_GNU

#include <cassert>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

#ifndef OBJ_CASE_INSENSITIVE
#define OBJ_CASE_INSENSITIVE    0x00000040L
#endif

#endif //WR_PCH
