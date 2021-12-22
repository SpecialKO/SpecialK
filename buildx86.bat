@echo off

Set BuildAgent=NONE

If         Exist "%SKIF_BUILDAGENT%" (                                                                          Set BuildAgent=%SKIF_BUILDAGENT%
 ) Else If Exist  SKIF.BuildAgent    ( For /f "delims=" %%i In ( 'Type SKIF.BuildAgent' ) Do ( If Exist "%%i" ( Set BuildAgent=%%i                 ) ) )
If "%BuildAgent%"==NONE              ( For /f "delims=" %%i In ( 'Where SKIF'           ) Do ( If Exist "%%i" ( Set BuildAgent=%%i
                                                                                                                          echo %%i>SKIF.BuildAgent ) ) )

If Exist "%BuildAgent%" (
   Start "Special K Build Agent" "%BuildAgent%" Stop Quit )

msbuild SpecialK.sln -t:Rebuild -p:Configuration=Release -p:Platform=Win32 -m

if %ERRORLEVEL%==0 goto build_success
 
:build_fail
Pause
Exit /b 1 

:build_success
If Exist "%BuildAgent%" (
   Start "Special K Build Agent" "%BuildAgent%" Start Minimize )
Exit /b 0