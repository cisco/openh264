@echo off
rem *************************************************************************************************
rem   usage:
rem      AutoBuildForWP8VC2013.bat % Configuration %
rem      --For debug  version:
rem          C-Only:AutoBuildForWP8VC2013.bat  Debug-C
rem          ASM:   AutoBuildForWP8VC2013.bat  Debug-ASM
rem      --For release version:
rem          C-Only: AutoBuildForWP8VC2013.bat  Release-C
rem          ASM:    AutoBuildForWP8VC2013.bat  Release-ASM
rem      --For debug and release version:
rem          C-Only: AutoBuildForWP8VC2013.bat  All-C
rem          ASM: AutoBuildForWP8VC2013.bat  All-ASM
rem      --For default:
rem         AutoBuildForWP8VC2013.bat
rem         both debug and release with ASM=Yes
rem
rem   Environment:
rem        ----gas - preprocessor
rem           --you can clone it from git://git.libav.org/gas-preprocessor.git
rem           --for more detail, please refer to https : //git.libav.org/?p=gas-preprocessor.git
rem           -- and then set gas - preprocessor path to the % GasPrePath % variable in this script
rem             or just copy to VC2013 bin's path,you can refer to variable %VC2013Path%
rem
rem        ----MinGW
rem           --install MinGW tools
rem           --more detail, please refer to http://www.mingw.org/
rem
rem   2015/03/15 huashi@cisco.com
rem *************************************************************************************************


call :BasicSetting
call :PathSetting
call :EnvSetting
call :BuildSetting %1
call :BuildResultInit
call :RunBuild
call :OutputBuildResult
call :GetFinalReturnCode
echo  ReturnCode is %ReturnCode%
cd %WorkingDir%
goto :End

:RunBuild
  for %%j in ( %aConfigurationList% ) do  (
    echo  Configuration is %%j
    echo  ASMFlag is %ASMFlag%
    set BuildFlag=0
    echo start build
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
goto :EOF

:PathSetting
  set WorkingDir=%cd%
  cd ..
  set RootDir=%cd%
  set BinDir=%RootDir%\bin\ARM
  cd %WorkingDir%
goto :EOF

:EnvSetting
  set MinGWPath=C:\MinGW\bin
  set MsysPath=C:\MinGW\msys\1.0\bin
  set VC2013Path=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
  set GasPrePath=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
  rem set PATH=%MinGWPath%;%MsysPath%;%VC2013Path%;%GasPrePath%;%PATH%
  set PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin;%PATH%

  call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_arm

  set VC12ArmLib01=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\store\arm
  set VC12ArmLib02=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\arm
  set WP8KitLib=C:\Program Files (x86)\Windows Phone Kits\8.1\lib\arm
  set LIB=%VC12ArmLib01%;%VC12ArmLib02%;%WP8KitLib%

  echo PATH is %PATH%
  echo LIB is %LIB%
  goto :EOF

:BuildSetting
  rem default setting
  if "aaa%1"=="aaa" (
    set aConfigurationList=Debug Release
    set ASMFlag=Yes
	echo default setting
  ) else if "%1"=="All-C" (
    set aConfigurationList=Debug Release
    set ASMFlag=No
	echo All-C setting
  ) else if "%1"=="All-ASM" (
    set aConfigurationList=Debug Release
    set ASMFlag=Yes
	echo All-ASM setting
  ) else if "%1"=="Debug-C" (
    set aConfigurationList=Debug
    set ASMFlag=No
	echo Debug-C setting
  ) else if "%1"=="Debug-ASM" (
    set aConfigurationList=Debug
    set ASMFlag=Yes
	echo Debug-ASM setting
  ) else if "%1"=="Release-C" (
    set aConfigurationList=Release
    set ASMFlag=No
	echo Release-C setting
  ) else if "%1"=="Release-ASM" (
    set aConfigurationList=Release
    set ASMFlag=Yes
	echo Release-ASM setting
  ) else (
    call :help
    goto :ErrorReturn
  )
  echo aConfigurationList is %aConfigurationList%
  echo ASMFlag is %ASMFlag%
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
  echo *****************************************************************
  echo      --For debug  version:
  echo          C-Only: AutoBuildForWP8VC2013.bat  Debug-C
  echo          ASM:    AutoBuildForWP8VC2013.bat  Debug-ASM
  echo      --For release version:
  echo          C-Only: AutoBuildForWP8VC2013.bat  Release-C
  echo          ASM:    AutoBuildForWP8VC2013.bat  Release-ASM
  echo      --For debug and release version:
  echo          C-Only: AutoBuildForWP8VC2013.bat  All-C
  echo          ASM:    AutoBuildForWP8VC2013.bat  All-ASM
  echo      --For default:
  echo         AutoBuildForWP8VC2013.bat
  echo         both debug and release with ASM=Yes
  echo *****************************************************************
goto :EOF

rem function for build
rem ***********************************************
:Build
  set vConfiguration=%1
  cd  %RootDir%
  bash -c "make OS=msvc-wp ARCH=arm USE_ASM=%ASMFlag% clean"
  bash -c "make OS=msvc-wp ARCH=arm USE_ASM=%ASMFlag% BUILDTYPE=%vConfiguration%"
  echo bash -c "make OS=msvc-wp ARCH=arm USE_ASM=%ASMFlag% clean"
  echo bash -c "make OS=msvc-wp ARCH=arm USE_ASM=%ASMFlag% BUILDTYPE=%vConfiguration%"
  if not %ERRORLEVEL%==0 (
    set BuildFlag=1
  )
  cd %WorkingDir%
goto :EOF

:CopyDll
  set vConfiguration=%1
  cd %RootDir%
  set FullDesDir=%BinDir%\%vConfiguration%
  echo copying dll files to destination folder...
  echo FullDesDir is %FullDesDir%
  if exist %FullDesDir% (
    rd /s /q %FullDesDir%
  )
  md %FullDesDir%

  echo current dir is:
  cd
  set DestDir=bin/ARM/%vConfiguration%
  echo DestDir is %DestDir%
  bash -c "cp -f %DllFile% %DestDir%"
  bash -c "cp -f %LibFile%  %DestDir%"
  bash -c "cp -f %PDBFile%  %DestDir%"
  cd %WorkingDir%
goto :EOF

:ErrorReturn
  endlocal
exit /b 2

:End
  endlocal
exit /b %ReturnCode%
