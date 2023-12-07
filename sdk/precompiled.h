#pragma once
#ifndef SDK_PCH
#define SDK_PCH

#ifdef __unix__
#ifdef __i386__
__asm__(".symver memcpy,memcpy@GLIBC_2.0");
#else
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
#endif
#endif

#include "../runtime/precommon.h"

#ifdef VMP_GNU
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <time.h>
#elif !defined(WIN_DRIVER)
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#endif

#endif //SDK_PCH