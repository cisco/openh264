echo off
SET usage="this file must located in ..\testbin\ based on our code structure  "
echo %usage%

echo "Auto build of openH264 by VS2008"

SET VCBUILDEXE="C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcpackages\vcbuild.exe"

SET CurDir=%~dp0
SET EncoderProjectDir="..\codec\build\win32\enc\"
SET DecoderProjectDir="..\codec\build\win32\dec\"
SET VPProjectDir="..\codec\processing\build\win32\"

SET CodecBinDir="..\codec\build\win32\bin\"
SET VPBinDir="..\processing\bin\"

SET EncoderBuildFlag=1
SET DecoderBuildFlag=1
SET VPBuildFlag=1
SET MakefileLogFile="${CurDir}\CodecVPBuild.log"

rem ************************************************
rem call VP build
echo "Welsvp Building....."
cd %VPProjectDir%
rem vcclean
%VCBUILDEXE% WelsVP.vcproj


rem ************************************************
rem call WelsEncoder build
echo "WelsEncoder Building....."

cd %CurDir%
cd %EncoderProjectDir%
rem vcclean
%VCBUILDEXE% WelsEncCore.vcproj
%VCBUILDEXE% WelsEncPlus.vcproj
%VCBUILDEXE% encConsole.vcproj

rem ************************************************
rem call WelsDecoder build
echo "WelsDecoder Building....."

cd %CurDir%
cd %DecoderProjectDir%
rem vcclean
%VCBUILDEXE% WelsDecCore.vcproj
%VCBUILDEXE% WelsDecPlus.vcproj
%VCBUILDEXE% decConsole.vcproj

cd %CurDir%
echo "this is the end of auto build..."
pause

rem TBD



