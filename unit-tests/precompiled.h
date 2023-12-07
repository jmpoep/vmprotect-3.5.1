#pragma once
#ifndef UT_PCH
#define UT_PCH

#include "../core/precompiled.h"

#ifdef WIN
#include <windows.h>
#include <crtdbg.h> /* Heap verifying */
#elif MACOSX
#define __ICONS__
#include <Cocoa/Cocoa.h>
#endif

#include "gtest/gtest.h"

#endif //UT_PCH