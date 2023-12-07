#include "objects.h"

#ifdef WIN_DRIVER

extern "C" EXCEPTION_DISPOSITION __cdecl __CxxFrameHandler3(int a, int b, int c, int d)
{
	return ExceptionContinueSearch;
}

#endif

/**
 * CriticalSection
 */

CriticalSection::CriticalSection(CRITICAL_SECTION &critical_section)
	: critical_section_(critical_section)
{
#ifdef VMP_GNU
	pthread_mutex_lock(&critical_section_);
#elif defined(WIN_DRIVER)
	KeWaitForMutexObject(&critical_section_, Executive, KernelMode, FALSE, NULL);
#else
	EnterCriticalSection(&critical_section_);
#endif
}
	
CriticalSection::~CriticalSection()
{
#ifdef VMP_GNU
	pthread_mutex_unlock(&critical_section_);
#elif defined(WIN_DRIVER)
	KeReleaseMutex(&critical_section_, FALSE);
#else
	LeaveCriticalSection(&critical_section_);
#endif
}

void CriticalSection::Init(CRITICAL_SECTION &critical_section)
{
#ifdef VMP_GNU
	pthread_mutex_init(&critical_section, NULL);
#elif defined(WIN_DRIVER)
	KeInitializeMutex(&critical_section, 0);
#else
	InitializeCriticalSection(&critical_section);
#endif
}

void CriticalSection::Free(CRITICAL_SECTION &critical_section)
{
#ifdef VMP_GNU
	pthread_mutex_destroy(&critical_section);
#elif defined(WIN_DRIVER)
	// do nothing
#else
	DeleteCriticalSection(&critical_section);
#endif
}