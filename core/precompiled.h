#pragma once
#ifndef CORE_PCH
#define CORE_PCH

#include "../runtime/precommon.h"
#include <vector>
#include <list>
#include <iostream>
#include <stdexcept>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <assert.h>
#include <unordered_map>
#include <queue>
#include <time.h>
#include <memory>

#ifdef VMP_GNU
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef __APPLE__
#include <copyfile.h>
#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <mach/mach_vm.h>
#include <sys/syslimits.h>
#else
#include <memory>
#ifndef O_EXLOCK // not available at linux
#define O_EXLOCK 0
#endif
#endif
#include <sys/mman.h>
#include <sys/sysctl.h>

#define DUMMYUNIONNAME   u
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#define DUMMYUNIONNAME6  u6
#define DUMMYUNIONNAME7  u7
#define DUMMYUNIONNAME8  u8
#define DUMMYUNIONNAME9  u9

#define DUMMYSTRUCTNAME  s
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#define DUMMYSTRUCTNAME4 s4
#define DUMMYSTRUCTNAME5 s5

#else

#define NONAMELESSUNION

#include <windows.h>
#include <psapi.h>
#include <io.h>

#define isatty _isatty
#define fileno _fileno

#endif

#include "pe.h"
#include "mach-o.h"
#include "elf.h"
#include "../third-party/tinyxml/tinyxml.h"

#endif //CORE_PCH
