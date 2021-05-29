#!/bin/bash
#*******************************************************************************
# Encoder binary comparison test model
#             -- Compared with benchmark version using SHA-1 string
#             -- Test bit stream under folder  openh264/res
#             -- SHA-1 string of benchmark version for  all cases  of all bit streams
#                 under folder  openh264/test/encoder_binary_comparion/SHA1Table
#             -- For more detail,please refer to file AboutTest.
#
#brief:
#             -- Usage:  run_OneBitStream.sh   $BitStreamName $TestType
#             -- WorkingDir
#                1) For local  test:  WorkingDir=openh264/test/encoder_binary_comparison
#                2) For travis test:  WorkingDir=openh264
#                   There will be an auto detection for working directory
#             -- Before using this script, need to run below command:
#                ./run_PrepareAllTestData.sh  32(or 64)
#                cd  $WorkingDir
#
# date:    10/06/2014 Created
#*******************************************************************************

runInitial()
{
    TestYUVName=""
    StreamName=""
    BitStreamToYUVLog="Bit2YUV.log"
    SHA1Table="${BitStreamName}_AllCases_SHA1_Table.csv"

    #dir info
    WorkingDir=`pwd`
    EncoderTestDir=""
    BitStreamTestDir=""
    FinalResultDir=""
    StreamFileFullPath=""
    BitSreamDir=""

    if [[  "${WorkingDir}" =~ "test/encoder_binary_comparison"  ]]
    then
        #for local test: working dir is  openh264/test/encoder_binary_comparison
        EncoderTestDir=${WorkingDir}
        BitStreamTestDir="${EncoderTestDir}/AllTestData/${BitStreamName}"
        FinalResultDir="${EncoderTestDir}/FinalResult"
        BitSreamDir="../../res"
    else
        #for travis test: working dir is  openh264/
        EncoderTestDir=${WorkingDir}/test/encoder_binary_comparison
        BitStreamTestDir="${EncoderTestDir}/AllTestData/${BitStreamName}"
        FinalResultDir="${EncoderTestDir}/FinalResult"
        BitSreamDir="${WorkingDir}/res"
    fi

    cd ${BitSreamDir}
    StreamFileFullPath=`pwd`
    StreamFileFullPath=${StreamFileFullPath}/${BitStreamName}
    cd ${WorkingDir}

}

runBitStreamToYUV()
{

    ./run_BitStreamToYUV.sh  ${StreamFileFullPath}>${BitStreamToYUVLog}
    if [  ! $? -eq 0 ]
    then
        echo "failed to translate bit stream to yuv !"
        exit 1
    fi

    #parse basic info
    TestYUVName=`runGetCurrentYUVName `
    TestYUVName=`echo ${TestYUVName} | awk 'BEGIN {FS="/"}  {print $NF}   ' `
    StreamName=`echo ${StreamFileFullPath} | awk 'BEGIN {FS="/"}  {print $NF}  ' `
    echo ""
    echo "TestYUVName is  ${TestYUVName}"
    echo "StreamName  is  ${StreamName}"
    echo "SHA1Table   is  ${SHA1Table}"
    echo ""
}


runGetCurrentYUVName()
 {

    local YUVName=""
    while read line
    do
        if [[ "$line" =~ "renamed as"  ]]
        then
            YUVName=`echo $line | awk 'BEGIN {FS=":"} {print $3}'`
        fi
    done <${BitStreamToYUVLog}
    echo ${YUVName}
 }

runSHA1TableCheck()
{
    if [  ! -e  "${SHA1Table}"   ]
    then
        echo "SHA1 table does not exist:  ${SHA1Table} "
        echo "SHA1 table should be named as \${StreamName}_AllCases_SHA1_Table.csv"
        exit 0
    fi
    return 0
 }

runBitStreamCheck()
{

    local BitSreamName=`echo ${StreamFileFullPath} | awk 'BEGIN {FS="/"} {print $NF}'`
    echo "bit stream is $BitSreamName"
    if [ ! -e ${StreamFileFullPath}  ]
    then
        echo -e "\033[31m   bit stream does not exist:  $BitSreamName   \033[0m"
        echo -e "\033[31m   please double check under  openh264/res folder \033[0m"
        echo -e "\033[31m     -----detected by run_OneBitStream.sh  \033[0m"
        exit 0
    fi
    return 0
 }

runTestSpaceCheck()
 {

    if [ ! -d ${BitStreamTestDir} ]
    then
        echo ""
        echo  -e "\033[31m ----Test space for bitsream does not exist!--- ${BitStreamTestDir}  \033[0m"
        echo  -e "\033[31m ---- before running this test locally, please follow step below:   \033[0m"
        echo  -e "\033[31m       ---- 1)cd test/encoder_binary_comparison/  \033[0m"
        echo  -e "\033[31m       ---- 2)run script file:  ./run_PrepareAllTestData.sh    \033[0m"
        echo ""
        exit 1
    fi
    return 0
 }

 #brief: delete temp files based on test type
runPostAction()
 {
    echo ""
    echo "deleting temp data,entire folder will be deleted........ "
    if [ -d ${BitStreamTestDir} ]
    then
        ${EncoderTestDir}/Scripts/run_SafeDelete.sh ${BitStreamTestDir}
    fi

    if [ ${TestType} = "TravisTest" ]
    then
        if [ -d ${FinalResultDir} ]
        then
            ${EncoderTestDir}/Scripts/run_SafeDelete.sh ${FinalResultDir}
        fi
    fi
 }

 #usage: usage: runMain $BitStreamName $TestType
runMain()
{
    if [ ! $# -eq 2 ]
    then
        echo "usage: runMain \$BitStreamName \$TestType"
        echo "detected by run_OneBitStream.sh"
        exit 1
    fi
    BitStreamName=$1
    TestType=$2

    runInitial
    runBitStreamCheck
    runTestSpaceCheck

    #go to Bitstream test space
    cd ${BitStreamTestDir}

    runSHA1TableCheck

    #bit stream to YUV
    runBitStreamToYUV

    #binary comparison
    ./run_BinarySHA1Comparison.sh   ${TestYUVName}   ${SHA1Table}
    if [  ! $? -eq 0 ]
    then
        echo ""
        echo  -e "\033[31m  not all cases passed .....\033[0m"
        echo  -e "\033[31m  this may caused by:      \033[0m"
        echo  -e "\033[31m   --1) you changed encoder algorithm which changed the final bit stream  \033[0m"
        echo  -e "\033[31m        if so, you need to update the SHA1 table in folder  ./test/encoder_binary_comparison/SHA1Table  \033[0m"
        echo  -e "\033[31m   --2) the decoder  has been changed and since the test YUV is generated by h264dec,the input YUV changed,so bit stream will also change \033[0m"
        echo  -e "\033[31m        if so, you need to update the SHA1 table in folder  ./test/encoder_binary_comparison/SHA1Table  \033[0m"
        echo  -e "\033[31m        for how to update, please refer to doc:  ./test/encoder_binary_comparison/AboutTest  \033[0m"
        echo  -e "\033[31m   --3) there may be something wrong in you code change (encoder or decoder) \033[0m"
        echo  -e "\033[31m        if so, please fix bug in your code \033[0m"
        cp  ./result/*    ${FinalResultDir}
        cd  ${WorkingDir}
        #delete the test data
        runPostAction
        exit 1
    else
        echo -e "\033[32m  all cases passed!! ----bit stream:  ${StreamName} \033[0m"
        cp  ./result/*    ${FinalResultDir}
        cd  ${WorkingDir}
        #delete the test data
        runPostAction
        exit 0
    fi
 }
BitSteamName=$1
TestType=$2
runMain    "${BitSteamName}" "${TestType}"

