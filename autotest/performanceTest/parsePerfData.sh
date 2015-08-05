#!/bin/bash

#usage  runGetPerformanceInfo   ${PerformanceLogFile}
runGetPerformanceInfo_openh264()
{

        if [ ! $# -eq 2 ]
        then
                echo "not enough parameters!"
                echo "usage: ${0} [android/ios] ${PerformanceLogFile}"
                return 1
        fi

        local PerformanceLogFile=$2
    local FileName=""
        local Width=""
        local Height=""
        local Frames=""
        local FPS=""
        local EncodeTime=""
    if [ $1 = "android" ]
    then seperatorNum=3
    else
        seperatorNum=2
    fi

        while read line
        do
                if [[ $line =~ "enc yuv file"  ]]
                then
            FileName=`echo $line | awk 'BEGIN {FS="enc yuv file"} {print $2}'`
            FileName=`echo $FileName | awk 'BEGIN {FS=":"} {print $2}'`
        fi
        if [[ $line =~ "Width" ]]
        then
            Width=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
        fi
        if [[ $line =~ "Height" ]]
        then
            Height=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
        fi
        if [[ $line =~ "Frames" ]]
        then
            Frames=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
        fi
        if [[ $line =~ "FPS" ]]
        then
            FPS=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
            FPS=`echo $FPS | awk 'BEGIN {FS="fps"} {print $1}'`
        echo "${FileName},"${Width}x${Height}",${Frames},${FPS}"
        fi

        if [[  $line =~ "encode time"  ]]
                then
                        EncodeTime=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
                fi
        if [[ $line =~ "height" ]]
        then
            Height=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
        fi
        if [[ $line =~ "H264 source file name" ]]
        then
            FileName=`echo $line | awk 'BEGIN {FS=":"} {print $'${seperatorNum}'}'`
       if [ $1 = "ios" ]
       then
            FileName=`echo $FileName | awk -F"DecoderPerfTestRes" '{print $2}'`
            FileName=`echo $FileName | awk -F"/" '{print $2}'`
       else
            FileName=`echo $FileName | awk -F"/" '{print $4}'`
       fi
      fi

        done <${PerformanceLogFile}


}
AUTO_TEST_RESULT_PATH="./TestResultCSV/"


parseLogToCSV()
{
if [ $# -ne 1 ]
then echo "Please input $0 [android/ios]"
fi
if [ $* = "android" ]
then
     Result_log_path="./android/report/"
     suffix=android
     dos2unix ${Result_log_path}*.*
else
Result_log_path="./ios/report/"
suffix=ios
fi
Result_log=`ls ${Result_log_path}`

for log in ${Result_log}
do
  PerformFile=`echo $log |awk -F"." '{print $1}'`
  PerformFile=${PerformFile}_${suffix}.csv
 #inital perfermance file
 echo "$log,,,">>${AUTO_TEST_RESULT_PATH}${PerformFile}
 echo "YUV,Resolution,Encodedframes,FPS">>${AUTO_TEST_RESULT_PATH}${PerformFile}
  runGetPerformanceInfo_openh264 ${suffix} ${Result_log_path}${log}>>${AUTO_TEST_RESULT_PATH}${PerformFile}
done
}
parseLogToCSV android
parseLogToCSV ios
