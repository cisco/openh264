#!/bin/bash

##############################################################
#Build ios test ref app

#set the default configuration
CODEC_TEST_IOS_ARCH="armv7 armv7s arm64"
CODEC_TEST_IOS_PLATFORM="iphoneos"
CODEC_TEST_IOS_DEBUG_RELEASE="Release"
CODEC_TEST_IOS_REPORT_SUBFOLDER="release"


buildXcodeProject()
{
 xcodebuild ARCHS="${CODEC_TEST_IOS_ARCH}" VALID_ARCHS="${CODEC_TEST_IOS_ARCH}" ONLY_ACTIVE_ARCH=YES  -project $1 -target $2 -configuration $3 -sdk ${CODEC_TEST_IOS_PLATFORM} clean build

if [ $? -eq 0 ]; then
 echo "build $1 $3 successfully"
 else
 echo "build $1 $3  fail"
 exit 1
 fi
}



iosPerformanceTest()
{

if [ $# -gt 2 ]; then
echo "Please use command $0 [enc/dec] [release/debug]"
exit 1
fi

for PARAM in $*; do
 if [ "enc" = "${PARAM}" ]; then
     CODEC_TEST_XCODE_PROJECT_NAME="${AUTO_TEST_SRC_PATH}/codec/build/iOS/enc/encDemo/encDemo.xcodeproj"
     CODEC_TEST_IOS_PROJECT_NAME="encDemo"
     CODEC_TEST_IOS_PROJECT_PATH="${AUTO_TEST_SRC_PATH}/codec/build/iOS/enc/encDemo/build"
     CODEC_TEST_IOS_APP=${CODEC_TEST_IOS_PROJECT_PATH}/${CODEC_TEST_IOS_DEBUG_RELEASE}-iphoneos/${CODEC_TEST_IOS_PROJECT_NAME}.app
     CODEC_TEST_IOS_APP_ID="cisco.encDemo"
     CODEC_TEST_RES=${AUTO_TEST_IOS_PATH}/../EncoderPerfTestRes
     CODEC_TEST_LOG="encPerf"
 elif [ "dec" = "${PARAM}" ]; then
     CODEC_TEST_XCODE_PROJECT_NAME="${AUTO_TEST_SRC_PATH}/codec/build/iOS/dec/demo/demo.xcodeproj/"
     CODEC_TEST_IOS_PROJECT_NAME="demo"
     CODEC_TEST_IOS_PROJECT_PATH="${AUTO_TEST_SRC_PATH}/codec/build/iOS/dec/demo/build"
     CODEC_TEST_IOS_APP=${CODEC_TEST_IOS_PROJECT_PATH}/${CODEC_TEST_IOS_DEBUG_RELEASE}-iphoneos/${CODEC_TEST_IOS_PROJECT_NAME}.app
     CODEC_TEST_IOS_APP_ID="hf.cisco.demo"
     CODEC_TEST_RES=${AUTO_TEST_IOS_PATH}/../DecoderPerfTestRes
     CODEC_TEST_LOG="decPerf"
 elif [ "release" = "${PARAM}" ]; then
     CODEC_TEST_IOS_DEBUG_RELEASE="Release"
     CODEC_TEST_IOS_REPORT_SUBFOLDER="release"
 elif [ "debug" = "${PARAM}" ]; then
     CODEC_TEST_IOS_DEBUG_RELEASE="Debug"
     CODEC_TEST_IOS_REPORT_SUBFOLDER="debug"
 else
    echo parameters are illegal!!!, please have a check.
    exit 1
 fi
 done

echo "Codec test will run on ${CODEC_TEST_IOS_PLATFORM} with ${CODEC_TEST_IOS_DEBUG_RELEASE}"
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

#Encoder YUV file too large
if [ ${ENCDEC} = "enc" ]
then
#For limited devices space
BAKRES=${CODEC_TEST_RES}_bak
mv ${CODEC_TEST_RES} ${BAKRES}
mkdir -p ${CODEC_TEST_RES}
CODEC_CASE=`ls ${BAKRES}`
echo ${CODEC_CASE}
for CASE in ${CODEC_CASE}
do
echo ${CASE}
cp -r ${BAKRES}/${CASE} ${CODEC_TEST_RES}/.


#uninstall the application from device to remove the last result
./fruitstrap uninstall --bundle ${CODEC_TEST_IOS_APP_ID} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo uninstall application: ${CODEC_TEST_IOS_APP} from device: ${DEVICE_ID} is failed!
fi
#install the application
./fruitstrap install --bundle ${CODEC_TEST_IOS_APP} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo install application: ${CODEC_TEST_IOS_APP} to device: ${DEVICE_ID} is failed!
exit 1
fi

./iFileTransfer -o copy -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from ${CODEC_TEST_RES}
instruments -w ${DEVICE_ID}  -t /Applications/Xcode.app/Contents/Applications/Instruments.app/Contents/PlugIns/AutomationInstrument.bundle/Contents/Resources/Automation.tracetemplate ${CODEC_TEST_IOS_APP} -e UIASCRIPT ./uiascript.js -e UIARRESULTPATH /tmp/
#copy to report folder
./iFileTransfer -o download -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from /Documents/${CODEC_TEST_LOG}.log -to ${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}_${CASE}.log
if [ $? -ne 0 ]; then
echo "download file: ${CODEC_TEST_LOG}.log from ${CODEC_TEST_IOS_APP_ID} is failed!"
exit 1
fi
cat ${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}_${CASE}.log>>${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}.log
rm -f ${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}_${CASE}.log
rm -rf ${CODEC_TEST_RES}/${CASE}
done
rm -rf ${CODEC_TEST_RES}
mv ${BAKRES} ${CODEC_TEST_RES}
#Enough spaces
else
#uninstall the application from device to remove the last result
./fruitstrap uninstall --bundle ${CODEC_TEST_IOS_APP_ID} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo uninstall application: ${CODEC_TEST_IOS_APP} from device: ${DEVICE_ID} is failed!
fi
#install the application
./fruitstrap install --bundle ${CODEC_TEST_IOS_APP} --id ${DEVICE_ID}
if [ $? -ne 0 ]; then
echo install application: ${CODEC_TEST_IOS_APP} to device: ${DEVICE_ID} is failed!
exit 1
fi

./iFileTransfer -o copy -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from ${CODEC_TEST_RES}
instruments -w ${DEVICE_ID}  -t /Applications/Xcode.app/Contents/Applications/Instruments.app/Contents/PlugIns/AutomationInstrument.bundle/Contents/Resources/Automation.tracetemplate ${CODEC_TEST_IOS_APP} -e UIASCRIPT ./uiascript.js -e UIARRESULTPATH /tmp/
#copy to report folder
./iFileTransfer -o download -id ${DEVICE_ID} -app ${CODEC_TEST_IOS_APP_ID} -from /Documents/${CODEC_TEST_LOG}.log -to ${CODEC_TEST_IOS_REPORT_PATH}/${CODEC_TEST_LOG}_${DEVICE_ID}_${rand}_${CASE}.log
if [ $? -ne 0 ]; then
echo "download file: ${CODEC_TEST_LOG}.log from ${CODEC_TEST_IOS_APP_ID} is failed!"
exit 1
fi


fi
done

fi
}

AUTO_TEST_IOS_PATH=`pwd`
AUTO_TEST_SRC_PATH="../../.."
CODEC_TEST_IOS_REPORT_PATH="${AUTO_TEST_IOS_PATH}/report"
if [ ! -d ${CODEC_TEST_IOS_REPORT_PATH} ]
then
 mkdir -p ${CODEC_TEST_IOS_REPORT_PATH}
fi

ENCDEC=$1
#start to get encoder/decoder performance data,default run the xcode with release
iosPerformanceTest $ENCDEC release

if [ $? -ne 0 ]; then
echo "Running $ENCDEC demo to get encoder performance is failed!"
exit 1
else
echo Finished $ENCDEC performance test on ios devices
echo the test result is generated at ./ios/report/xx.loGbash parsePerfData.sh
echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxIOS $ENCDEC  Endxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
fi
#TODO:according to the trace of instruments to do some analysis
#find ./ -name *.trace -exec rm -rf {} \;
