#!/bin/bash

AUTO_TEST_ANDROID_PATH=`pwd`
AUTO_TEST_SRC_PATH="../../../"
AUTO_TEST_RES_PATH="${AUTO_TEST_ANDROID_PATH}/report"
if [ ! -d ${AUTO_TEST_RES_PATH} ]
then
mkdir -p ${AUTO_TEST_RES_PATH}
else
echo "Will delete those outdate xml in the report"
rm -f ${AUTO_TEST_RES_PATH}/*.xml
fi
#Prepare android build enviroment
echo please set the enviroment variable as:
echo export ANDROID_HOME="path of android sdk"
echo export ANDROID_NDK_HOME="path of android ndk"
ANDROID_SDK_PATH=${ANDROID_HOME}
ANDROID_NDK_PATH=${ANDROID_NDK_HOME}
ANDROID_MAKE_PARAMS="OS=android NDKROOT=${ANDROID_NDK_PATH} TARGET=android-19"

if [ "#${ANDROID_SDK_PATH}" = "#" ]
then
echo Please set ANDROID_HOME with the path of Android SDK
exit 1
fi
if [ "#${ANDROID_NDK_PATH}" = "#" ]
then
echo Please set ANDROID_NDK_HOME with the path of Android NDK
exit 1
fi
#make build
cd ${AUTO_TEST_SRC_PATH}
find ./ -name *.o -exec rm -f {} \;
find ./ -name *.d -exec rm -f {} \;
make clean
make $ANDROID_MAKE_PARAMS test

if [ $? -ne 0 ]
then
   echo Build error,check with the trace of make
   exit 1
fi

#find apk
echo Start to find unittest apk
apk_name=`find ./ -name MainActivity-debug.apk`
if [ "#${apk_name}" = "#" ]
then
  echo Fail to find encoder APK.
  exit 1
fi

#prepare devices
ADB=${ANDROID_SDK_PATH}/platform-tools/adb

#get devices
devices=`$ADB devices | awk -F" " '/\tdevice/{print $1}'`
if [ "#$devices" = "#" ];then
   echo "Have not any android devices."
   exit 1
fi

#run apk
run_apk() {
local apk=$1;
local rand=` date +%s`
apk_id="com.cisco.codec.unittest"
apk_main="com.cisco.codec.unittest/.MainActivity"
test_path="/sdcard/welsenc"
log_grep_params="welsenc"
test_res=./res
xml_file="sdcard/codec_unittest.xml"
for dev in $devices; do
    #dev_info_file=${AUTO_TEST_RES_PATH}/${dev}.log
    report_file=${AUTO_TEST_RES_PATH}/codec_unittest_${dev}_${rand}.xml
    $ADB -s $dev uninstall ${apk_id}
    $ADB -s $dev install -r ${apk}
    #TODO: output more info about android device such as name,cpu,memory,and also power comsumption.
    echo `$ADB -s $dev shell cat /system/build.prop |grep ro.product.model | awk -F"=" '{print $2}'`>${dev_info_file}
    $ADB -s $dev push ${test_res} /sdcard/res
    $ADB -s $dev shell am start --es path "$xml_file" -n ${apk_main}
    # check whetehr the app is finished every 2 sec
    for (( ; ; )); do
        $ADB -s $dev shell ps | grep ${apk_id}
        if [ $? -ne 0 ]; then
            sleep 2
            $ADB -s $dev shell ps | grep ${apk_id}
            [ $? -ne 0 ] && break
        fi
        sleep 2
    done

    # kill logcat
    $ADB -s $dev pull ${xml_file} ${report_file}
    #delete the res
    $ADB -s $dev shell rm -rf ${xml_file}
    $ADB -s $dev shell rm -rf /sdcard/res
done
}
for apk in ${apk_name};do
   run_apk $apk;
   if [ $? -ne 0 ]
   then
     echo There is something wrong happened when run ${apk_name}
     exit 1
   else
     echo Finished unit test on android
     echo The test result is at ./android/report/xxx.xml
     echo xxxxxxxxxxxxxxxAndroid unittest Endxxxxxxxxxxxxxxxx
   fi
done

