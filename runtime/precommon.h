#pragma once
#ifndef PRECOMMON_PCH
#define PRECOMMON_PCH

#define NOMINMAX
#ifdef WIN_DRIVER
#define _STL70_
#include <ntddk.h>
#endif

#ifdef __cplusplus
#include <string> // because of non-usual new operator
#endif

#ifdef _DEBUG
#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif
#endif

#include <sys/stat.h>

#if defined(__unix__)
#include <linux/limits.h>
enum { FALSE, TRUE };
typedef void VOID;
#endif

#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define NOINLINE
#endif

#include <stdio.h>
#include <string.h>

#if defined(__APPLE__) || defined(__unix__)
#define VMP_GNU
#include <stdint.h>
#include <pthread.h>

typedef int HANDLE;
typedef void *LPVOID;
typedef void *HMODULE;

#if defined(__APPLE__)
typedef mach_port_t HPROCESS;
#endif

#if defined(__unix__)
typedef int HPROCESS;
inline size_t strlcpy(char *dst, const char *src, size_t size)
{
    dst[size - 1] = '\0';
    strncpy(dst, src, size - 1);
    return strlen(dst);
}
#endif

#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define _countof(x) (sizeof(x) / sizeof(x[0]))
#define _strtoui64 strtoull
#define _strtoi64 strtoll
#define VMP_API
typedef char VMP_CHAR;
typedef unsigned short VMP_WCHAR; 
inline void strncpy_s(char *strDestination, size_t sz, const char *strSource, size_t src_sz) { strncpy(strDestination, strSource, sz + 0 * src_sz); }
#ifdef __cplusplus
namespace os {
	typedef unsigned short unicode_char;
	typedef std::basic_string<unicode_char, std::char_traits<unicode_char>, std::allocator<unicode_char> > unicode_string;
};
template <size_t sz> void strcpy_s(char (&strDestination)[sz], const char *strSource) { strlcpy(strDestination, strSource, sz); }
template <size_t sz> void strncpy_s(char (&strDestination)[sz], const char *strSource, size_t src_sz) { strncpy_s(&strDestination[0], sz, strSource, src_sz); }
#endif
#define _TRUNCATE ((size_t)-1)
#define _vsnprintf_s(dest, dest_sz, cnt, fmt, args) _vsnprintf((dest), (dest_sz), (fmt), (args))
#define _vsnprintf vsnprintf
#define vsprintf_s vsprintf
#define sprintf_s sprintf
#define _strcmpi strcasecmp
#define _strdup strdup
#define sscanf_s sscanf
#define strcat_s strcat
#define LOWORD(l)           ((uint16_t)(((uint32_t)(l)) & 0xffff))
#define HIWORD(l)           ((uint16_t)((((uint32_t)(l)) >> 16) & 0xffff))
typedef int                 BOOL;
typedef unsigned int        UINT;

typedef char CHAR;
typedef uint8_t BYTE, *PBYTE;
typedef short SHORT;
typedef uint16_t WORD, USHORT, *PWORD, *PUSHORT;
typedef uint32_t DWORD, ULONG, ULONG32, *PDWORD, *PULONG, *PULONG32;
typedef int32_t LONG, LONG32, *PLONG, *PLONG32;
typedef uint64_t DWORD64, ULONGLONG, ULONG64, *PDWORD64, *PULONGLONG, *PULONG64;
typedef struct _GUID {
	uint32_t  Data1;
	uint16_t  Data2;
	uint16_t  Data3;
	uint8_t   Data4[8];
} GUID;

#define CRITICAL_SECTION pthread_mutex_t

#else

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

#define VMP_API __stdcall
typedef wchar_t VMP_CHAR; 
typedef wchar_t VMP_WCHAR;
#ifdef __cplusplus
namespace os {
	typedef std::wstring unicode_string;
	typedef wchar_t unicode_char;
};
#endif

typedef void *HPROCESS;

#endif

#ifdef __cplusplus
#include <algorithm>
#endif

#endif //PRECOMMON_PCH