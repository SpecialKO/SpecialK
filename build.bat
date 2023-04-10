@echo off

@REM Set to pause after a successful build process
set __SK_PAUSE_SUCCESS=0

@REM Set to allow using prerelease version of Visual Studio
set __SK_VS_PRERELEASE=0

@REM Call the VS Developer Command Prompt if not already done
IF NOT DEFINED VSCMD_VER (
  IF %__SK_VS_PRERELEASE% EQU 1 (
    @REM Use prerelease version
    FOR /f "usebackq delims=" %%i IN (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -prerelease -latest -property installationPath`) DO (
      IF EXIST "%%i\Common7\Tools\vsdevcmd.bat" (
        call "%%i\Common7\Tools\vsdevcmd.bat"
        goto :sk_continue
      )
    )
  ) ELSE (
    @REM Use latest version
    FOR /f "usebackq delims=" %%i IN (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) DO (
      IF EXIST "%%i\Common7\Tools\vsdevcmd.bat" (
        call "%%i\Common7\Tools\vsdevcmd.bat"
        goto :sk_continue
      )
    )
  )
)
:sk_continue

@REM Build Special K
cd /D "%~dp0"
call "%~dp0buildx86.bat"
call "%~dp0buildx64.bat"

@REM Post-build tasks
IF %ERRORLEVEL% EQU 0 (
  IF %__SK_PAUSE_SUCCESS% EQU 1 (
    @REM Pause even after a successful build process
    pause
  )
)