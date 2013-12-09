echo off
SET usage="this file must located in ..\testbin\ based on our code structure  "
echo %usage%
echo "Auto build of openH264 by VS2010"

SET VCENVSETBAT="C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"

Rem ************************************************
rem reset the dev env for VS2010
echo "reset the developing env for VS2010"
call %VCENVSETBAT%


SET VCMSBUILDEXE_RELEASE=MSbuild /t:Rebuild /p:Configuration=Release
SET VCMSBUILDEXE_DEBUG=MSbuild /t:Rebuild /p:Configuration=Debug

SET CurDir=%~dp0
SET EncoderProjectDir="..\codec\build\win32\enc\"
SET DecoderProjectDir="..\codec\build\win32\dec\"
SET VPProjectDir="..\processing\build\win32\"

SET CodecBinDir="..\codec\build\win32\bin\"
SET VPBinDir="..\processing\bin\"

SET EncoderBuildFlag=1
SET DecoderBuildFlag=1
SET VPBuildFlag=1
SET MakefileLogFile="${CurDir}\CodecVPBuild.log"



rem ************************************************
rem call WelsEncoder build
echo "WelsEncoder Building....."

cd %CurDir%
cd %EncoderProjectDir%
echo current directory is %EncoderProjectDir%
rem vcclean 

echo %VCMSBUILDEXE_RELEASE% WelsEncoder_2010.sln
%VCMSBUILDEXE_RELEASE% WelsEncoder_2010.sln
%VCMSBUILDEXE_DEBUG% WelsEncoder_2010.sln

rem ************************************************
rem call WelsDecoder build
echo "WelsDecoder Building....."

cd %CurDir%
cd %DecoderProjectDir%
echo current directory is %DecoderProjectDir%
rem vcclean 

echo %VCMSBUILDEXE_RELEASE% WelsDecoder_2010.sln

%VCMSBUILDEXE_RELEASE% WelsDecoder_2010.sln
%VCMSBUILDEXE_DEBUG% WelsDecoder_2010.sln

cd %CurDir%
echo "this is the end of auto build..."
pause

rem TBD



