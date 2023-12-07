#pragma once
#ifndef ICORE_C_PCH
#define ICORE_C_PCH

#include "../../runtime/precommon.h"

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <time.h>
#include <float.h>

#ifdef _WIN32
#ifdef WIN_DRIVER
#include <windef.h>
#else
#include <windows.h>
#endif
#endif

#ifdef VMP_GNU
#else
#include <process.h>
#endif

#endif //ICORE_C_PCH