#!/bin/bash
#*******************************************************************************
#Encoder Binary comparison test model
#             -- Compared with benchmark version using SHA-1 string
#             -- Test bit stream under folder  openh264/res
#             -- SHA-1 string of benchmark version for  all cases  of all bit streams
#                 under folder  openh264/test/encoder_binary_comparion/SHA1Table
#             -- For more detail,please refer to file AboutTest.
#
#brief:
#             -- This file is for local test under Linux/Unix OS
#             -- Start point of the test model,
#             -- For local test, please run below command:
#                   ./run_Main.sh LocalTest
#                And wait minutes for the test result.
#                Final test result, you can refer to test output files under folder
#                openh264/test/encoder_binary_comparison/FinalResult.
#             -- If there is something change in encoder design,and found that the bit stream
#                also change,you need to update the SHA1 table files in ./SHA1Table.
#                For update the SHA1 tables, please run below command:
#                   ./run_Main.sh UpdateSHA1Table
#                There will be some mismatched warning info, please ignore it.
#                Wait for minutes,the up-to-date will be copied to ./SHA1Table.
# date:    10/06/2014 Created
#*******************************************************************************

#usage: runTestTypeCheck    ${TestType}
runTestTypeCheck()
{

    echo "TestType is ${TestType}"
    if [ "${TestType}" = "LocalTest" ]
    then
        return 0
    elif [ "${TestType}" = "UpdateSHA1Table" ]
    then
        return 0
    else
        echo "usage:   --./run_Main.sh  LocalTest"
        echo "      or --./run_Main.sh  UpdateSHA1Table"
        exit 1
    fi
}

#delete temp files/folders
runLocalTestPostAction()
{
    ./Scripts/run_SafeDelete.sh   ${AllTestDataFolder} >>${DeletedLog}
    ./Scripts/run_SafeDelete.sh   ./Codec >>${DeletedLog}
    echo -e "\n\n\n"
    echo -e "\033[32m *************************************************************** \033[0m"
    echo -e "\033[32m  Local test completed!                                          \033[0m"
    if [ ${Flag} -eq  0 ]
    then
        echo -e "\033[32m  --all cases pass!! ----bit stream:  ${StreamName}          \033[0m"
    else
        echo -e "\033[31m  --not all cases passed .....                               \033[0m"
    fi
    echo ""
    echo -e "\033[32m  for more detail, please refer to *.log files and *.csv files   \033[0m"
    echo -e "\033[32m  in ./FinalResult                                               \033[0m"
    echo -e "\033[32m *************************************************************** \033[0m"
    echo ""
}

#copy up-to-date SHA1 table files to ./SHA1Table
#and delete temp files/folders
runUpdateSHA1TablePostAction()
{
    local BitStreamName=""
    local SHA1TableName=""
    local FileName=""
    for file in ${FinalResultFolder}/*
    do
        FileName=`echo $file | awk 'BEGIN {FS="/"} {print $NF}'`
        if [[  "$FileName"  =~ UpdateSHA1Table.csv$ ]]
        then
            BitStreamName=`echo $FileName | awk 'BEGIN {FS=".264"} {print $1}'`
            SHA1TableName=${BitStreamName}.264_AllCases_SHA1_Table.csv
            ./Scripts/run_SafeDelete.sh  ${SHA1TableFolder}/${SHA1TableName} >>${DeletedLog}
            cp  $file   ${SHA1TableFolder}/${SHA1TableName}
        fi
    done
    ./Scripts/run_SafeDelete.sh  ${AllTestDataFolder}>>${DeletedLog}
    ./Scripts/run_SafeDelete.sh  ${FinalResultFolder}>>${DeletedLog}
    ./Scripts/run_SafeDelete.sh  ./Codec>>${DeletedLog}
    echo -e "\n\n\n"
    echo -e "\033[32m *************************************************************** \033[0m"
    echo -e "\033[32m all SHA1 tables in ./${SHA1TableFolder} have been updated \033[0m"
    echo -e "\n"
    echo -e "\033[32m *************************************************************** \033[0m"
    echo -e "\n\n"
}
#usage:     --./run_Main.sh  LocalTest
#            or --./run_Main.sh  UpdateSHA1Table
runMain()
 {
    if [ ! $# -eq 1 ]
    then
        echo "usage:   --./run_Main.sh  LocalTest"
        echo "      or --./run_Main.sh  UpdateSHA1Table"
        exit 1
    fi

    local TestType=$1
    local BitStreamName=""

    Flag=""
    SHA1TableFolder="SHA1Table"
    FinalResultFolder="FinalResult"
    AllTestDataFolder="AllTestData"
    DeletedLog="Delete.log"
    runTestTypeCheck  ${TestType}

    # 32 -->32 bits release version;64 -->64 bits release version
    ./run_PrepareAllTestData.sh  32
    if [ ! $? -eq 0 ]
    then
        echo "failed to prepare test space for all test data!"
        exit 1
    fi

    #test all cases
    let "Flag=0"
    for Bitsream in  ./SHA1Table/*.csv
    do
        BitStreamName=`echo ${Bitsream} | awk 'BEGIN {FS="/"}  {print $NF} ' `
        BitStreamName=`echo ${BitStreamName} | awk 'BEGIN {FS=".264"}  {print $1} ' `
        BitStreamName="${BitStreamName}.264"
        echo -e  "\n\n\n"

        ./run_OneBitStream.sh   ${BitStreamName} ${TestType}
        if [ ! $? -eq 0 ]
        then
            let "Flag=1"
        fi
    done

    #post action
    if [  ${TestType} = "LocalTest" ]
    then
        runLocalTestPostAction
    elif [  ${TestType} = "UpdateSHA1Table" ]
    then
        runUpdateSHA1TablePostAction
    fi
 }
TestType=$1
runMain    ${TestType}

