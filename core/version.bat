@ECHO OFF
echo version.bat: generating build number...

SET version_h=%1core\version.h
SET info_plist=%1app\vmprotect_gui.app\Contents\Info.plist

SET major=1
SET minor=0
SET patch=0
SET build=0

IF "%bamboo_VMP_MAJOR%" neq "" SET major=%bamboo_VMP_MAJOR%
IF "%bamboo_VMP_MINOR%" neq "" SET minor=%bamboo_VMP_MINOR%
IF "%bamboo_VMP_SUBMINOR%" neq "" SET patch=%bamboo_VMP_SUBMINOR%
IF "%bamboo_buildNumber%" neq "" SET build=%bamboo_buildNumber%

SET has_major=
SET has_minor=
SET has_patch=
SET has_build=
 
if EXIST %version_h% (
 for /f "tokens=1-3 delims= " %%A in (%version_h%) do (
 	IF "%%B"=="VER_MAJOR" (
 		SET has_major=%%C
 	)
 	IF "%%B"=="VER_MINOR" (
 		SET has_minor=%%C
 	)
 	IF "%%B"=="VER_PATCH" (
 		SET has_patch=%%C
 	)
 	IF "%%B"=="VER_BUILD" (
 		SET has_build=%%C
 	)
 )
)

if "%major%.%minor%.%patch%.%build%" neq "%has_major%.%has_minor%.%has_patch%.%has_build%" (
 ECHO  version.bat: build number incremented to %major%.%minor%.%patch%.%build% at %version_h%
 ECHO #define VER_MAJOR %major% > %version_h%
 ECHO #define VER_MINOR %minor% >> %version_h%
 ECHO #define VER_PATCH %patch% >> %version_h%
 ECHO #define VER_BUILD %build% >> %version_h%
 ECHO #define VER_FILE "%major%.%minor%.%patch%.%build%" >> %version_h%
 ECHO #define VER_PRODUCT "%major%.%minor%.%patch%" >> %version_h%
) else (
 ECHO  version.bat: build number at %version_h% = %major%.%minor%.%patch%.%build% is up to date
)

ECHO ^<?xml version="1.0" encoding="UTF-8"?^> > %info_plist%
ECHO ^<^!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"^> >> %info_plist%
ECHO ^<plist version="1.0"^> >> %info_plist%
ECHO ^<dict^> >> %info_plist%
ECHO ^<key^>CFBundleDevelopmentRegion^</key^> >> %info_plist%
ECHO ^<string^>en^</string^> >> %info_plist%
ECHO ^<key^>CFBundleExecutable^</key^> >> %info_plist%
ECHO ^<string^>vmprotect_gui^</string^> >> %info_plist%
ECHO ^<key^>CFBundleIconFile^</key^> >> %info_plist%
ECHO ^<string^>logo.icns^</string^> >> %info_plist%
ECHO ^<key^>CFBundleIdentifier^</key^> >> %info_plist%
ECHO ^<string^>com.vmpsoft.vmprotect^</string^> >> %info_plist%
ECHO ^<key^>CFBundleInfoDictionaryVersion^</key^> >> %info_plist%
ECHO ^<string^>6.0^</string^> >> %info_plist%
ECHO ^<key^>CFBundleName^</key^> >> %info_plist%
ECHO ^<string^>VMProtect^</string^> >> %info_plist%
ECHO ^<key^>CFBundlePackageType^</key^> >> %info_plist%
ECHO ^<string^>APPL^</string^> >> %info_plist%
ECHO ^<key^>CFBundleShortVersionString^</key^> >> %info_plist%
ECHO ^<string^>%major%.%minor%.%patch%^</string^> >> %info_plist%
ECHO ^<key^>CFBundleVersion^</key^> >> %info_plist%
ECHO ^<string^>%build%^</string^> >> %info_plist%
ECHO ^<key^>CFBundleSignature^</key^> >> %info_plist%
ECHO ^<string^>vmpg^</string^> >> %info_plist%
ECHO ^<key^>LSMinimumSystemVersion^</key^> >> %info_plist%
ECHO ^<string^>10.7.0^</string^> >> %info_plist%
ECHO ^<key^>CFBundleDocumentTypes^</key^> >> %info_plist%
ECHO ^<array^> >> %info_plist%
ECHO ^<dict^> >> %info_plist%
ECHO ^<key^>LSItemContentTypes^</key^> >> %info_plist%
ECHO ^<array^> >> %info_plist%
ECHO ^<string^>public.executable^</string^> >> %info_plist%
ECHO ^<string^>com.microsoft.windows-executable^</string^> >> %info_plist%
ECHO ^<string^>com.microsoft.windows-dynamic-link-library^</string^> >> %info_plist%
ECHO ^</array^> >> %info_plist%
ECHO ^<key^>CFBundleTypeRole^</key^> >> %info_plist%
ECHO ^<string^>Editor^</string^> >> %info_plist%
ECHO ^</dict^> >> %info_plist%
ECHO ^<dict^> >> %info_plist%
ECHO ^<key^>LSItemContentTypes^</key^> >> %info_plist%
ECHO ^<array^> >> %info_plist%
ECHO ^<string^>com.vmpsoft.vmprotect.vmp^</string^> >> %info_plist%
ECHO ^</array^> >> %info_plist%
ECHO ^<key^>CFBundleTypeRole^</key^> >> %info_plist%
ECHO ^<string^>Editor^</string^> >> %info_plist%
ECHO ^<key^>LSHandlerRank^</key^> >> %info_plist%
ECHO ^<string^>Owner^</string^> >> %info_plist%
ECHO ^</dict^> >> %info_plist%
ECHO ^</array^> >> %info_plist%
ECHO ^<key^>UTExportedTypeDeclarations^</key^> >> %info_plist%
ECHO ^<array^> >> %info_plist%
ECHO ^<dict^> >> %info_plist%
ECHO ^<key^>UTTypeConformsTo^</key^> >> %info_plist%
ECHO ^<array^> >> %info_plist%
ECHO ^<string^>public.xml^</string^> >> %info_plist%
ECHO ^</array^> >> %info_plist%
ECHO ^<key^>UTTypeIdentifier^</key^> >> %info_plist%
ECHO ^<string^>com.vmpsoft.vmprotect.vmp^</string^> >> %info_plist%
ECHO ^<key^>UTTypeTagSpecification^</key^> >> %info_plist%
ECHO ^<dict^> >> %info_plist%
ECHO ^<key^>public.filename-extension^</key^> >> %info_plist%
ECHO ^<string^>vmp^</string^> >> %info_plist%
ECHO ^</dict^> >> %info_plist%
ECHO ^</dict^> >> %info_plist%
ECHO ^</array^> >> %info_plist%
ECHO ^</dict^> >> %info_plist%
ECHO ^</plist^> >> %info_plist%
