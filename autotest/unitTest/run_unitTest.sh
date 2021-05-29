#!/bin/bash

AUTO_TEST_PATH=`pwd`
IOS=0
ANDROID=0
#Prepare GTEST
AUTO_TEST_SRC_PATH="../../"
cd ${AUTO_TEST_SRC_PATH}
if [ ! -d "./gtest" ]
then
   make gtest-bootstrap
fi
cd ${AUTO_TEST_PATH}
#To find whether have android devices
echo please set the enviroment variable as:
echo export ANDROID_HOME="path of android sdk"
echo export ANDROID_NDK_HOME="path of android ndk"
ANDROID_SDK_PATH=${ANDROID_HOME}
ANDROID_NDK_PATH=${ANDROID_NDK_HOME}
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
#prepare devices
ADB=${ANDROID_SDK_PATH}/platform-tools/adb

#get devices
devices=`$ADB devices | awk -F" " '/\tdevice/{print $1}'`
if [ "#$devices" = "#" ];then
   echo "Can not find any android devices!"
else
   echo Start to run the unittest on android devices
   ANDROID=1
   cd ./android
   bash run_AutoTest_android.sh >/dev/null 2>&1
   if [ $? -ne 0 ];then
   echo There is something wrong happened when runing unittest on android devices,please to check
   else
   echo Finish run the unittest on android devices sucessfully
   fi
   cd ${AUTO_TEST_PATH}
fi

#To find whether have ios devices
 DEVICES=`system_profiler SPUSBDataType | sed -n -e '/iPad/,/Serial/p' -e '/iPhone/,/Serial/p' | grep "Serial Number:" | awk -F ": " '{print $2}'`
 if [ "${DEVICES}#" == "#" ]
 then
 echo "Can not find any ios devices!"
else
  echo Start to run the unittest on ios devices
  IOS=1
  cd ./ios
  bash run_AutoTest_ios.sh >/dev/null 2>&1
 if [ $? -ne 0 ];then
 echo There is something wrong happened when runing unittest on ios devices,please to check
 else
 echo Finish run the unittest on android devices sucessfully
fi
 cd ${AUTO_TEST_PATH}
fi

#To parse the unit test result file to find whether have failures
if [ ${ANDROID} = "1" ];then
echo "
<style>
.env {
    background-color: #95B9C7;
    font: 30px bold;
}</style>">>mail.log
echo "<br><font class="env">Run unit test on android devices</font>">>mail.log
bash run_ParseUTxml.sh ./android/report
ret=$?
if [ ${ret} -eq 0 ];then
echo Unit test run on the android devices have not any failure case
elif [ ${ret} -eq 2 ];then
echo Unit test have cases failed,please check
elif [ ${ret} -eq 1 ];then
echo Unit test run failed
fi
fi
if [ ${IOS} = "1" ];then
echo "<br><font class="env">Run unit test on ios devices with armv7 & arm64</font>">>mail.log
bash run_ParseUTxml.sh ./ios/report
ret=$?
if [ $ret -eq 0 ];then
echo Unit test run on the ios devices have not any failure case
elif [ $ret -eq 2 ];then
echo Unit test have cases failed,please check
elif [ $ret -eq 1 ];then
echo Unit test run failed
fi
fi
