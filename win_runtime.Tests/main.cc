#ifdef WIN
#include <crtdbg.h> /* Heap verifying */
#elif MACOSX
#define __ICONS__
#include <Cocoa/Cocoa.h>
#endif


int main(int argc, char **argv)
{
#ifdef WIN
	/* Set up heap verifying. */
	_CrtSetDbgFlag(_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF);
#elif MACOSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif

	testing::InitGoogleTest(&argc, argv);
	int rc = RUN_ALL_TESTS();
    
#ifdef MACOSX
    [pool release];
#endif
    
	return rc;
}
