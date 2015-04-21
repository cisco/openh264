@echo off
rem *************************************************************************************************
rem   usage:
rem      AutoBuildForWPAndWindows.bat % Configuration %
rem      --For debug  version:
rem          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-Debug-C
rem          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-Debug-ASM
rem          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-Debug-C
rem          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-Debug-ASM
rem          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-Debug-C
rem          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-Debug-ASM
rem      --For release version:
rem          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-Release-C
rem          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-Release-ASM
rem          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-Release-C
rem          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-Release-ASM
rem          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-Release-C
rem          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-Release-ASM
rem      --For debug and release version:
rem          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-All-C
rem          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-All-ASM
rem          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-All-C
rem          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-All-ASM
rem          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-All-C
rem          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-All-ASM
rem      --For default:
rem         AutoBuildForWPAndWindows.bat
rem           ARM-All-ASM
rem
rem      --lib/dll files will be copied to folder .\bin
rem         --win32 folder bin\i386*
rem         --win64 folder bin\x86_64*
rem         --arm   folder bin\arm*
rem
rem   Environment:
rem        ----for windows phone, Visual studio with update 3 or later is needed
rem        ----gas-preprocessor(windows phone build only)
rem           --you can clone it from git://git.libav.org/gas-preprocessor.git
rem           --for more detail, please refer to https://git.libav.org/?p=gas-preprocessor.git
rem           -- and then set gas-preprocessor path to the %GasScriptPath% variable in this script
rem             or just copy to VC2013 bin's path,you can refer to variable %VC12Path%
rem
rem        ----MinGW
rem           --install MinGW tools
rem           --more detail, please refer to http://www.mingw.org/
rem
rem   2015/03/15 huashi@cisco.com
rem *************************************************************************************************
call :BasicSetting
call :PathSetting
call :SetBuildOption %1
if not %ERRORLEVEL%==0 (
    echo not suppot option!
    goto :ErrorReturn
)
call :EnvSetting     %1
call :BuildResultInit
call :RunBuild
call :OutputBuildResult
call :GetFinalReturnCode
echo  ReturnCode is %ReturnCode%
cd %WorkingDir%
goto :End

:RunBuild
    for %%j in ( %aConfigurationList% ) do  (
        set BuildFlag=0
        call :Build       %%j
        call :BuildCheck  %%j
        call :CopyDll     %%j
  )
goto :EOF


rem function for setting
rem ***********************************************
:BasicSetting
  set DllFile=openh264.dll
  set LibFile=openh264.lib
  set PDBFile=openh264.pdb
  set UTDllFile=ut.dll
  set UTBinFile=codec_unittest.exe
  set EncBinFile=h264enc.exe
  set DecBinFile=h264dec.exe
goto :EOF

:PathSetting
  set WorkingDir=%cd%
  cd ..
  set RootDir=%cd%
  set BinDir=%RootDir%\bin
  cd %WorkingDir%
goto :EOF

:EnvSetting
  set MinGWPath=C:\MinGW\bin
  set MsysPath=C:\MinGW\msys\1.0\bin
  set GitPath=C:\Program Files (x86)\Git\bin
  set GasScriptPath=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin

  set VC12Path=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
  set VC11Path=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC
  set VC10Path=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC
  set VC9Path=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC

  set VC12ArmLib01=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\store\arm
  set VC12ArmLib02=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\arm
  set WP8KitLib=C:\Program Files (x86)\Windows Phone Kits\8.1\lib\arm

  if exist "%VC9Path%"  set VCPATH=%VC9Path%
  if exist "%VC10Path%" set VCPATH=%VC10Path%
  if exist "%VC11Path%" set VCPATH=%VC11Path%
  if exist "%VC12Path%" set VCPATH=%VC12Path%

  if "%vArcType%" =="i386"   set PATH=%MinGWPath%;%MsysPath%;%VCPATH%\bin;%GitPath%;%PATH%
  if "%vArcType%" =="x86_64" set PATH=%MinGWPath%;%MsysPath%;%VCPATH%\bin;%GitPath%;%PATH%
  if "%vArcType%" =="arm"    set PATH=%MinGWPath%;%MsysPath%;%VCPATH%\bin;%GitPath%;%PATH%
  rem if "%vArcType%" =="arm"    set PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin;C:\Program Files (x86)\Git\bin;%PATH%

  if "%vArcType%" =="i386"   call "%VCPATH%\vcvarsall.bat" x86
  if "%vArcType%" =="x86_64" call "%VCPATH%\vcvarsall.bat" x64
  if "%vArcType%" =="arm"    call "%VCPATH%\vcvarsall.bat" x86_arm
  if "%vArcType%" =="arm"    call :WPSetting

  echo PATH is %PATH%
  echo LIB  is %LIB%
goto :EOF

:WPSetting
  set LIB=%VC12ArmLib01%;%VC12ArmLib02%;%WP8KitLib%
  if  not exist "%VC12Path%" (
      echo    VC12 does not exist,
      echo ******************************************
      echo    VC12 does not exist,
      echo    which is needed for windows phone
      echo ******************************************
      goto :ErrorReturn
    )
goto :EOF

:SetBuildOption
  if "aaa%1"=="aaa" (
      set aConfigurationList=Debug Release
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo default setting
  ) else if "%1"=="Win32-Debug-C" (
      set aConfigurationList=Debug
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo Win32-Debug-C setting
  )  else if "%1"=="Win32-Release-C" (
      set aConfigurationList=Release
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo Win32-Release-C setting
  )  else if "%1"=="Win64-Debug-C" (
      set aConfigurationList=Debug
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=No
    echo All-C setting
  )  else if "%1"=="Win64-Release-C" (
      set aConfigurationList=Release
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=No
    echo Win64-Release-C setting
  )  else if "%1"=="ARM-Debug-C" (
      set aConfigurationList=Debug
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo ARM-Debug-C setting
  )  else if "%1"=="ARM-Release-C" (
      set aConfigurationList=Debug Release
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo ARM-Release-C setting
  )   else if "%1"=="Win32-All-C" (
      set aConfigurationList=Debug Release
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo Win32-All-C setting
  )  else if "%1"=="Win64-All-C" (
      set aConfigurationList=Debug Release
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=No
    echo All-C setting
  )  else if "%1"=="ARM-All-C" (
      set aConfigurationList=Debug Release
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=No
    echo ARM-All-C setting
  )  else if "%1"=="Win32-Debug-ASM" (
      set aConfigurationList=Debug
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo Win32-Debug-ASM setting
  )  else if "%1"=="Win32-Release-ASM" (
      set aConfigurationList=Release
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo Win32-Release-ASM setting
  )  else if "%1"=="Win64-Debug-ASM" (
      set aConfigurationList=Debug
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=Yes
    echo All-ASM setting
  )  else if "%1"=="Win64-Release-ASM" (
      set aConfigurationList=Release
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=Yes
    echo Win64-Release-ASM setting
  )  else if "%1"=="ARM-Debug-ASM" (
      set aConfigurationList=Debug
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo ARM-Debug-ASM setting
  )  else if "%1"=="ARM-Release-ASM" (
      set aConfigurationList=Release
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo ARM-Release-ASM setting
  )   else if "%1"=="Win32-All-ASM" (
      set aConfigurationList=Debug Release
      set vArcType=i386
      set vOSType=msvc
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo Win32-All-ASM setting
  )  else if "%1"=="Win64-All-ASM" (
      set aConfigurationList=Debug Release
      set vArcType=x86_64
      set vOSType=msvc
      set vEnable64BitFlag=Yes
      set vASMFlag=Yes
    echo All-ASM setting
  )  else if "%1"=="ARM-All-ASM" (
      set aConfigurationList=Debug Release
      set vArcType=arm
      set vOSType=msvc-wp
      set vEnable64BitFlag=No
      set vASMFlag=Yes
    echo ARM-All-ASM setting
  )  else (
      call :help
      goto :ErrorReturn
  )
  echo aConfigurationList is %aConfigurationList%
  echo vArcType is           %vArcType%
  echo vOSType  is           %vOSType%
  echo vEnable64BitFlag is   %vEnable64BitFlag%
  echo vASMFlag is           %vASMFlag%
goto :EOF

rem function for build result
rem ***********************************************
:BuildResultInit
  set BuildDebugFlag=0
  set BuildReleaseFlag=0
  set BuildDebugInfo=NULL
  set BuildReleaseInfo=NULL
goto :EOF

:OutputBuildResult
  echo  BuildDebugFlag   =%BuildDebugFlag%
  echo  BuildReleaseFlag =%BuildReleaseFlag%
  echo  BuildDebugInfo   =%BuildDebugInfo%
  echo  BuildReleaseInfo =%BuildReleaseInfo%
  goto :EOF

:BuildCheck
  set vConfiguration=%1
  if not %BuildFlag%==0 (
    if "%vConfiguration%"=="Debug" (
      set BuildDebugFlag=1
      set BuildDebugInfo="build debug--failed"
    ) else (
      set BuildReleaseFlag=1
      set BuildReleaseInfo="build release--failed"
    )
  ) else (
    if "%vConfiguration%"=="Debug" (
      set BuildDebugFlag=0
      set BuildDebugInfo="build debug--succeed"
    ) else (
      set BuildReleaseFlag=0
      set BuildReleaseInfo="build release--succeed"
    )
  )
goto :EOF

:GetFinalReturnCode
  set aBuildFlagList=%BuildDebugFlag%  %BuildReleaseFlag%
  echo  aBuildFlagList is %aBuildFlagList%
  set ReturnCode=0
  for %%k in (%aBuildFlagList%) do  (
    if not %%k == 0 (
      set ReturnCode=1
    )
  )
goto :EOF

rem function for help
rem ***********************************************
:help
  echo *******************************************************************************
  echo   usage:
  echo      AutoBuildForWPAndWindows.bat % Configuration %
  echo      --For debug  version:
  echo          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-Debug-C
  echo          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-Debug-ASM
  echo          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-Debug-C
  echo          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-Debug-ASM
  echo          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-Debug-C
  echo          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-Debug-ASM
  echo      --For release version:
  echo          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-Release-C
  echo          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-Release-ASM
  echo          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-Release-C
  echo          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-Release-ASM
  echo          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-Release-C
  echo          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-Release-ASM
  echo      --For debug and release version:
  echo          Win32-C-Only: AutoBuildForWPAndWindows.bat  Win32-All-C
  echo          Win32-ASM:    AutoBuildForWPAndWindows.bat  Win32-All-ASM
  echo          Win64-C-Only: AutoBuildForWPAndWindows.bat  Win64-All-C
  echo          Win64-ASM:    AutoBuildForWPAndWindows.bat  Win64-All-ASM
  echo          ARM-C-Only:   AutoBuildForWPAndWindows.bat  ARM-All-C
  echo          ARM-ASM:      AutoBuildForWPAndWindows.bat  ARM-All-ASM
  echo      --For default:
  echo         AutoBuildForWPAndWindows.bat
  echo           ARM-All-ASM
  echo *******************************************************************************
goto :EOF

rem function for build
rem ***********************************************
:Build
  set vConfiguration=%1
  cd  %RootDir%
  echo bash -c "make OS=%vOSType%  ARCH=%vArcType% USE_ASM=%vASMFlag% BUILDTYPE=%vConfiguration% clean"
  echo bash -c "make OS=%vOSType%  ARCH=%vArcType% USE_ASM=%vASMFlag% BUILDTYPE=%vConfiguration%"
  bash -c "make OS=%vOSType%  ARCH=%vArcType% USE_ASM=%vASMFlag% BUILDTYPE=%vConfiguration% clean"
  bash -c "make OS=%vOSType%  ARCH=%vArcType% USE_ASM=%vASMFlag% BUILDTYPE=%vConfiguration%"
  if not %ERRORLEVEL%==0 (
    set BuildFlag=1
  )
  cd %WorkingDir%
goto :EOF

:CopyDll
  set vConfiguration=%1
  set vBuildOption=%2
  cd %RootDir%
  if "%vArcType%"=="arm" (
    set vBinDirName=ARM
  ) else if "%vArcType%"=="i386" (
    set vBinDirName=Win32
  ) else (
    set vBinDirName=x64
  )
  set ArchDestDir=%BinDir%\%vBinDirName%
  set FullDestDir=%BinDir%\%vBinDirName%\%vConfiguration%
  echo copying dll files to destination folder...
  echo FullDestDir is %FullDestDir%
  if not exist %ArchDestDir% md %ArchDestDir%
  if exist %FullDestDir% (
    rd /s /q %FullDestDir%
  )
  md %FullDestDir%

  echo current dir is:
  cd
  set DestDir=bin/%vBinDirName%/%vConfiguration%
  echo DestDir is %DestDir%
  if "%vOSType%"=="msvc-wp" (
     set aFileList=%DllFile% %LibFile% %PDBFile% %UTDllFile%
  ) else (
     set aFileList=%DllFile% %LibFile% %PDBFile% %UTBinFile% %EncBinFile% %DecBinFile%
  )
  for %%k in (%aFileList%) do (
    bash -c "cp -f  %%k  %DestDir%"
  )
  cd %WorkingDir%
goto :EOF

:ErrorReturn
  endlocal
exit /b 2

:End
  endlocal
exit /b %ReturnCode%
