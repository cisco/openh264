#!/bin/bash

AUTO_TEST_ANDROID_PATH=`pwd`
AUTO_TEST_SRC_PATH="../../../"
AUTO_TEST_RES_PATH="${AUTO_TEST_ANDROID_PATH}/report"
mkdir -p ${AUTO_TEST_RES_PATH}
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
make $ANDROID_MAKE_PARAMS

if [ $? -ne 0 ]
then
   echo Build error,check with the trace of make
   exit 1
fi

ENCDEC=$1
#find apk
if [ ${ENCDEC} = "enc" ]
then
echo Start to find enc apk
apk_name=`find ./ -name WelsEncTest-debug.apk`
if [ "#${apk_name}" = "#" ]
then
  echo Fail to find encoder APK.
  exit 1
fi
else
echo Start to find dec apk
apk_name=`find ./ -name WelsDecTest-debug.apk`
if [ "#${apk_name}" = "#" ]
then
echo Fail to find decoder APK.
exit 1
fi
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

if [[ "${apk}" =~ "WelsDecTest-debug.apk" ]]
then
   apk_id="com.wels.dec"
   apk_main="com.wels.dec/.WelsDecTest"
   test_path="/sdcard/welsdec"
   log_grep_params="welsdec"
   test_res=${AUTO_TEST_ANDROID_PATH}/../DecoderPerfTestRes
   report_file=${AUTO_TEST_RES_PATH}/decPerf

fi
if [[ "${apk}" =~ "WelsEncTest-debug.apk" ]]
then
  apk_id="com.wels.enc"
  apk_main="com.wels.enc/.WelsEncTest"
  test_path="/sdcard/welsenc"
  log_grep_params="welsenc"
  test_res=${AUTO_TEST_ANDROID_PATH}/../EncoderPerfTestRes
  report_file=${AUTO_TEST_RES_PATH}/encPerf
fi
space="limit"
for dev in $devices; do
    dev_info_file=${AUTO_TEST_RES_PATH}/${dev}.log
    $ADB -s $dev uninstall ${apk_id}
    $ADB -s $dev install -r ${apk}
    #TODO: output more info about android device such as name,cpu,memory,and also power comsumption.
    #echo `$ADB -s $dev shell cat /system/build.prop |grep ro.product.model | awk -F"=" '{print $2}'`>${dev_info_file}
    #push resources
    #For limited devices space
    if [ ${space} = "limit" ]
    then
    test_res_bak=${test_res}_bak
    mv ${test_res} ${test_res_bak}
    mkdir -p ${test_res}
    test_case=`ls ${test_res_bak}`
    for case in ${test_case}
    do
       echo ${case}
        cp -r ${test_res_bak}/${case} ${test_res}/.
        $ADB -s $dev push ${test_res} ${test_path}
        #before start logcat,kill logcat
        pid=`$ADB -s $dev shell ps | grep logcat | awk '{print $2;}'`
        [ "#$pid" != "#" ] && $ADB -s $dev shell kill $pid >/dev/null
        $ADB -s $dev logcat -c
        $ADB -s $dev logcat |grep ${log_grep_params} >>${report_file}_${dev}_${rand}.log &
        $ADB -s $dev shell am start -n ${apk_main}
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
        pid=`$ADB -s $dev shell ps | grep logcat | awk '{print $2;}'`
        [ "#$pid" != "#" ] && $ADB -s $dev shell kill $pid >/dev/null

        #delete the res
        $ADB -s $dev shell rm -rf ${test_path}
        rm -rf ${test_res}/${case}
    done
    rm -rf ${test_res}
    mv ${test_res_bak} ${test_res}
    else
    $ADB -s $dev push ${test_res} ${test_path}
    #before start logcat,kill logcat
    pid=`$ADB -s $dev shell ps | grep logcat | awk '{print $2;}'`
    [ "#$pid" != "#" ] && $ADB -s $dev shell kill $pid >/dev/null
    $ADB -s $dev logcat -c
    $ADB -s $dev logcat |grep ${log_grep_params} >${report_file}_${dev}_${rand}.log &
    $ADB -s $dev shell am start -n ${apk_main}
    # check whetehr the app is finished every 2 sec
    for (( ; ; )); do
    $ADB -s $dev shell ps | grep ${apk_id}
    if [ $? -ne 0 ]; then
        sleep 2
        $ADB -s $dev shell ps | grep ${apk_idi}
        [ $? -ne 0 ] && break
    fi
    sleep 2
    done

    # kill logcat
    pid=`$ADB -s $dev shell ps | grep logcat | awk '{print $2;}'`
    [ "#$pid" != "#" ] && $ADB -s $dev shell kill $pid >/dev/null

    #delete the res
    $ADB -s $dev shell rm -rf ${test_path}
    fi

done
}
for apk in ${apk_name};do
   run_apk $apk;
   if [ $? -ne 0 ]
   then
     echo There is something wrong happened when run ${apk_name}
     exit 1
   else
     echo Finished $ENCDEC performance test on android
     echo The test result is at ./android/report/xxx.log
     echo xxxxxxxxxxxxxxxAndroid $ENCDEC Endxxxxxxxxxxxxxxxx
   fi
done

