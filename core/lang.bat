echo lang.bat: generating default lang strings...
set lang_in="%1langs\en.lng"
set lang_out_def="%1core\lang_def.inc"
set lang_out_enum="%1core\lang_enum.inc"
set lang_out_info="%1core\lang_info.inc"
set lang_def=%lang_out_def%
set lang_enum=%lang_out_enum%
set lang_info=%lang_out_info%
set check_def=0
if exist %lang_out_def% (
    set lang_out_def="%1core\lang_def.tmp"
    set check_def=1
)
set check_enum=0
if exist %lang_out_enum% (
    set lang_out_enum="%1core\lang_enum.tmp"
    set check_enum=1
)
set check_info=0
if exist %lang_out_info% (
    set lang_out_info="%1core\lang_info.tmp"
    set check_info=1
)
set area=[Main]
set currarea=

echo {> %lang_out_def%

echo enum LangString {> %lang_out_enum%

echo static const struct {> %lang_out_info%
echo 	size_t id;>> %lang_out_info%
echo 	const char *name;>> %lang_out_info%
echo } key_info[] = {>> %lang_out_info%

setlocal enableextensions enabledelayedexpansion
for /f "usebackq delims=" %%a in (!lang_in!) do (
    set ln=%%a
    if "x!ln:~0,1!"=="x[" (
        set currarea=!ln!
    ) else if "x!area!"=="x!currarea!" (
        rem for preserving exclamation marks in data:
        setlocal disabledelayedexpansion
        for /f "tokens=1* delims==" %%b in ("%%a") do (
            set currkey=%%b
            set currval=%%c
            setlocal enabledelayedexpansion
            echo 	default_values_[ls!currkey!] = replace_escape_chars^("!currval:"=\"!"^);>> !lang_out_def!
            echo 	ls!currkey!,>> !lang_out_enum!
            echo 	{ls!currkey!, "!currkey:"=\"!"},>> !lang_out_info!
            endlocal
        )
        endlocal
    )
)
echo };>> %lang_out_def%
echo lsCNT };>> %lang_out_enum%
echo };>> %lang_out_info%
endlocal

call :ReplaceOld %check_def% %lang_out_def% %lang_def% default
call :ReplaceOld %check_enum% %lang_out_enum% %lang_enum% enum
call :ReplaceOld %check_info% %lang_out_info% %lang_info% info
goto :EOF

:ReplaceOld
setlocal enableextensions enabledelayedexpansion
if "%1" == "1" (
  fc %2 %3 /B>>nul 2>&1
  if !ERRORLEVEL! == 1 (
   copy /y %2 %3>>nul 2>&1
   del %2>>nul 2>&1
   echo  lang.bat: %4 lang strings '%3' are updated 
  ) else (
   echo  lang.bat: %4 lang strings '%3' are up to date
  )
) else (
 echo  lang.bat: %4 lang strings '%3' are generated
)
endlocal
goto :EOF