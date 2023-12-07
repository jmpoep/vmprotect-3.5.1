echo res.bat: generating QT resources...
SET RC_DIR=%~dp0

set rc_out=%RC_DIR%resources.cc
set rc=%rc_out%
set check_rc=0
if exist %rc_out% (
    set rc_out=%RC_DIR%resources.cc.tmp
    set check_rc=1
)
%QTDIR%/msvc2015_64/bin/rcc.exe %RC_DIR%application.qrc -o %rc_out%

call :ReplaceOld %check_rc% %rc_out% %rc%
goto :EOF

:ReplaceOld
setlocal enableextensions enabledelayedexpansion
if "%1" == "1" (
  fc %2 %3 /B>>nul 2>&1
  if !ERRORLEVEL! == 1 (
   copy /y %2 %3>>nul 2>&1
   del %2>>nul 2>&1
   echo  res.bat: QT resources '%3' are updated 
  ) else (
   echo  res.bat: QT resources '%3' are up to date
  )
) else (
 echo  res.bat: QT resources '%3' are generated
)
endlocal
goto :EOF