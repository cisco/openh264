#!/bin/bash

##############################################################
#Build ios test ref app

#set the default configuration
CODEC_TEST_IOS_ARCH="armv7"
CODEC_TEST_IOS_PLATFORM="iphoneos"
CODEC_TEST_IOS_DEBUG_RELEASE="Release"
CODEC_TEST_IOS_REPORT_SUBFOLDER="release"


buildXcodeProject()
{
 xcodebuild ARCHS="${CODEC_TEST_IOS_ARCH}" VALID_ARCHS="${CODEC_TEST_IOS_ARCH}" ONLY_ACTIVE_ARCH=NO -project $1 -target $2 -configuration $3 -sdk ${CODEC_TEST_IOS_PLATFORM} clean build

if [ $? -eq 0 ]; then
 echo "build $1 $3 successfully"
 else
 echo "build $1 $3  fail"
 exit 1
 fi
}



iosUnitTest()
{

if [ $# -gt 2 ]; then
echo "Please use command $0 [armv7/armv7s/arm64] [release/debug]"
exit 1
fi

     CODEC_TEST_XCODE_PROJECT_NAME="${AUTO_TEST_SRC_PATH}/test/build/ios/codec_unittest/codec_unittest.xcodeproj/"
     CODEC_TEST_IOS_PROJECT_NAME="codec_unittest"
     CODEC_TEST_IOS_PROJECT_PATH="${AUTO_TEST_SRC_PATH}/test/build/ios/codec_unittest/build"
     CODEC_TEST_IOS_APP=${CODEC_TEST_IOS_PROJECT_PATH}/${CODEC_TEST_IOS_DEBUG_RELEASE}-iphoneos/${CODEC_TEST_IOS_PROJECT_NAME}.app
     CODEC_TEST_IOS_APP_ID="com.cisco.codec-unittest"
     CODEC_TEST_RES=${AUTO_TEST_SRC_PATH}/res
     CODEC_TEST_LOG="codec_unittest"
for PARAM in $*; do
   if [ "release" = "${PARAM}" ]; then
     CODEC_TEST_IOS_DEBUG_RELEASE="Release"
     CODEC_TEST_IOS_REPORT_SUBFOLDER="release"
 elif [ "debug" = "${PARAM}" ]; then
     CODEC_TEST_IOS_DEBUG_RELEASE="Debug"
     CODEC_TEST_IOS_REPORT_SUBFOLDER="debug"
 elif [ "armv7" = "${PARAM}" ];then
      CODEC_TEST_IOS_ARCH="armv7"
 elif [ "armv7s" = "${PARAM}" ];then
     CODEC_TEST_IOS_ARCH="armv7s"
 elif [ "arm64" = "${PARAM}" ];then
    CODEC_TEST_IOS_ARCH="arm64"
 else
    echo parameters are illegal!!!, please have a check.
    exit 1
 fi
 done
cd ${AUTO_TEST_SRC_PATH}
IOS_MAKE_PARAMS="OS=ios ARCH=${CODEC_TEST_IOS_ARCH}"
############make build
find ./ -name *.o -exec rm -rf {} \;
find ./ -name *.d -exec rm -rf {} \;
rm -f *.so
make clean
make ${IOS_MAKE_PARAMS} test
echo "Codec test will run on ${CODEC_TEST_IOS_PLATFORM} with ${CODEC_TEST_IOS_DEBUG_RELEASE}"
cd ${AUTO_TEST_IOS_PATH}
buildXcodeProject ${CODEC_TEST_XCODE_PROJECT_NAME} ${CODEC_TEST_IOS_PROJECT_NAME} ${CODEC_TEST_IOS_DEBUG_RELEASE} ${CODEC_TEST_IOS_PLATFORM}




##############run on ios devices#########################
# for real device
if [ ! -d ${CODEC_TEST_IOS_APP} ] ; then
echo "${CODEC_TEST_IOS_APP} is not found"
exit 1
else
echo "Find app ${CODEC_TEST_IOS_APP}"
fi

 #ensure instruments not runing
echo "Try to kill the runing instruments"
pids_str=`ps x -o pid,command | grep -v grep | grep "instruments" | awk '{printf "%s,", $1}'`
instruments_pids="${pids_str//,/ }"
for pid in ${instruments_pids}; do
echo "Found instruments ${pid}. Killing..."
kill -9 ${pid} && wait ${pid} &> /dev/null
done



DEVICES=`system_profiler SPUSBDataType | sed -n -e '/iPad/,/Serial/p' -e '/iPhone/,/Serial/p' | grep "Serial Number:" | awk -F ": " '{print $2}'`
if [ "${DEVICES}#" == "#" ]
then
echo "Can not find any connected device! please check device is connected to MAC!"
exit 1
else
rand=`date +%s`
for DEVICE_ID in ${DEVICES}
do
echo "Try to run on device:${DEVICE_ID}"

#uninstall the application from device to remove the last result
${AUTO_TEST_IOS_SCRIPT_PATH}/fruitstrap uninstall --bundle ${CODEC_TEST_IOS_APP_ID} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo uninstall application: ${CODEC_TEST_IOS_APP} from device: ${DEVICE_ID} is failed!
fi
#install the application
${AUTO_TEST_IOS_SCRIPT_PATH}/fruitstrap install --bundle ${CODEC_TEST_IOS_APP} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo install application: ${CODEC_TEST_IOS_APP} to device: ${DEVICE_ID} is failed!
exit 1
fi

${AUTO_TEST_IOS_SCRIPT_PATH}/iFileTransfer -o copy -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from ${CODEC_TEST_RES}
instruments -w ${DEVICE_ID}  -t /Applications/Xcode.app/Contents/Applications/Instruments.app/Contents/PlugIns/AutomationInstrument.bundle/Contents/Resources/Automation.tracetemplate ${CODEC_TEST_IOS_APP} -e UIASCRIPT ${AUTO_TEST_IOS_SCRIPT_PATH}/uiascript.js -e UIARRESULTPATH /tmp/
#copy to report folder
${AUTO_TEST_IOS_SCRIPT_PATH}/iFileTransfer -o download -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from /Documents/${CODEC_TEST_LOG}.xml -to ${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}_${CODEC_TEST_IOS_ARCH}.xml
if [ $? -ne 0 ]; then
echo "download file: ${CODEC_TEST_LOG}.xml from ${CODEC_TEST_IOS_APP_ID} is failed!"
exit 1
fi

done
fi
}

AUTO_TEST_IOS_PATH=`pwd`
AUTO_TEST_SRC_PATH="../../.."
AUTO_TEST_IOS_SCRIPT_PATH="../../performanceTest/ios"
CODEC_TEST_IOS_REPORT_PATH="${AUTO_TEST_IOS_PATH}/report"
if [ ! -d ${CODEC_TEST_IOS_REPORT_PATH} ]
then
 mkdir -p ${CODEC_TEST_IOS_REPORT_PATH}
else
 echo "Will delete those outdate xml in the report"
 rm -f ${CODEC_TEST_IOS_REPORT_PATH}/*.xml
fi

#start to run unittest,default run the xcode at arch armv7 with release
iosUnitTest armv7 release

if [ $? -ne 0 ]; then
echo "Running Unittest demo with armv7 is failed!"
exit 1
else
echo Finished unittest with armv7 on ios devices
echo the test result is generated at ./ios/report/xx.xml
fi
#start to run unittest,run the xcode at arch arm64 with release
iosUnitTest arm64 release
if [ $? -ne 0 ]; then
echo "Running Unittest demo with arm64 is failed!"
exit 1
else
echo Finished unittest with arm64 on ios devices
echo the test result is generated at ./ios/report/xx.xml
fi

echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxIOS unittest  Endxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#TODO:according to the trace of instruments to do some analysis
#find ${AUTO_TEST_IOS_SCRIPT_PATH} -name *.trace -exec rm -rf {} \;
rm -rf *.trace
