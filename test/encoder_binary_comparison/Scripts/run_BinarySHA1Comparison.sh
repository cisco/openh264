#!/bin/bash
#***************************************************************************************************************
# Encoder binary comparison test model
#             -- Compared with Benchmark version using SHA-1 string
#             -- Test bit stream under folder  openh264/res
#             -- SHA-1 string of Benchmark version for  all cases  of all bit streams
#                   under folder  openh264/test/encoder_binary_comparion/SHA1Table
#             -- For more detail,please refer to file AboutTest?¡ê
#brief:
#             -- Test all cases in SHA1TableFile
#                  1) Generate SHA1 string for each case
#                  2) Compare with benchmark SHA1 string
#                  3) If the SHA1 string are the same with benchmark version, test case is marked as passed
#                  4) Otherwise, test case is marked as unpass! this may caused by:
#                      --the design of encoder has been changed and the bit stream also change
#                      --there may be some bugs in encoder and bit stream is not right
#             -- Usage:  ./run_BinarySHA1Comparison.sh  ${TestYUVName}   ${AllCaseSHA1TaleFile}
#
# date:    10/06/2014 Created
#***************************************************************************************************************
#global variable definition
#usage runGlobalVariableDef
runGlobalVariableDef()
{
    WorkingDirDir=""
    #test data space
    FinalResultPath=""
    IssueDataPath=""
    TempDataPath=""
    #for test sequence info
    TestSequenceName=""
    PicW=""
    PicH=""
    #test cfg file and test info output file
    ConfigureFile=""
    AllCaseFile=""
    #xxx.csv
    AllCasePassStatusFile=""
    #for encoder command
    declare -a aEncoderCommandSet
    declare -a aEncoderCommandName
    declare -a aEncoderCommandValue
    declare -a aRecYUVFileList
    #encoder parameters  change based on the case info
    let "EncoderFlag = 0"
    CaseInfo=""
    BitStreamFile=""
    DiffFlag=""
    #pass number
    TotalCaseNum=""
    PassCaseNum=""
    UnpassCaseNum=""
}
#called by runGlobalVariableInitial
#usage runEncoderCommandInital
runEncoderCommandInital()
{

    aEncoderCommandSet=( -utype  -frms  -numl   -numtl \
                         -sw -sh    "-dw 0"  "-dh 0" "-dw 1" "-dh 1" "-dw 2" "-dh 2" "-dw 3" "-dh 3" \
                         "-frout 0" "-frout 1" "-frout 2" "-frout 3" \
                         "-lqp 0" "-lqp 1" "-lqp 2" "-lqp 3" \
                         -rc -fs -tarb "-ltarb 0"   "-ltarb 1" "-ltarb 2" "-ltarb 3" \
                         "-slcmd 0" "-slcnum 0" "-slcmd 1" "-slcnum 1"\
                         "-slcmd 2" "-slcnum 2" "-slcmd 3" "-slcnum 3"\
                         -nalsize \
                         -iper     -thread    -ltr \
                         -db    -denois    -scene    -bgd    -aq )

    aEncoderCommandName=(usagetype    frms  numl   numtl \
                         sw sh    dw0 dh0 dw1 dh1 dw2 dh2 dw3 dh3 \
                         frout0 frout1 frout2 frout3 \
                         lqp0 lqp1 lqp2 lqp3 \
                         rc FrSkip tarb ltarb0  ltarb1 ltarb2 ltarb3 \
                         slcmd0 slcnum0 slcmd1 slcnum1 \
                         slcmd2 slcnum2 slcmd3 slcnum3 \
                         MaxNalSZ    \
                         iper     thread  ltr \
                         db    denois  scene  bgd  aq )


    NumParameter=${#aEncoderCommandSet[@]}
    for ((i=0;i<NumParameter; i++))
    do
        aEncoderCommandValue[$i]=0
    done

}
runGlobalVariableInitial()
{
    #TestSpaceDir=../AllTestData  CurrentDir=../AllTestData/TestSetXXX/***.264  eg ../AllTestData/TestSetCIF/BA1_MWD.264
    CurrentDir=`pwd`
    #test data space
    FinalResultPath="${CurrentDir}/result"
    IssueDataPath="${CurrentDir}/issue"
    TempDataPath="TempData"
    TestSequencePath="${CurrentDir}"
    #get YUV detail info $picW $picH $FPS
    declare -a aYUVInfo
    aYUVInfo=(`./run_ParseYUVInfo.sh  ${TestSequenceName}`)
    PicW=${aYUVInfo[0]}
    PicH=${aYUVInfo[1]}
    #test cfg file and test info output file
    ConfigureFile=welsenc.cfg
    AllCasePassStatusFile="${FinalResultPath}/${TestSequenceName}_AllCaseOutput.csv"
    UnpassCaseFile="${FinalResultPath}/${TestSequenceName}_unpassCaseOutput.csv"
    UpdateSHA1TableFile="${FinalResultPath}/${TestSequenceName}_UpdateSHA1Table.csv"

   HeadLine1="EncoderFlag, DecoderFlag, FPS, BitSreamSHA1, BitSreamMD5, InputYUVSHA1, InputYUVMD5,\
              -utype,    -frms,  -numl,  -numtl, -sw, -sh,\
              -dw 0, -dh 0, -dw 1, -dh 1, -dw 2, -dh 2, -dw 3, -dh 3,\
              -frout 0,    -frout 1, -frout 2, -frout 3,\
              -lqp 0, -lqp 1, -lqp 2, -lqp 3,\
              -rc,-fs, -tarb, -ltarb 0, -ltarb 1, -ltarb 2, -ltarb 3,\
              -slcmd 0, -slcnum 0, -slcmd 1, -slcnum 1,\
              -slcmd 2, -slcnum 2, -slcmd 3, -slcnum 3,\
              -nalsize,\
              -iper, -thread, -ltr, -db, -denois,\
              -scene,    -bgd ,  -aq, "

    HeadLine2="BitSreamSHA1, BitSreamMD5, InputYUVSHA1, InputYUVMD5,\
              -utype,    -frms,  -numl,  -numtl, -sw, -sh,\
              -dw 0, -dh 0, -dw 1, -dh 1,-dw 2, -dh 2, -dw 3, -dh 3,\
              -frout 0,    -frout 1, -frout 2, -frout 3,\
              -lqp 0, -lqp 1, -lqp 2, -lqp 3,\
              -rc, -fs, -tarb, -ltarb 0, -ltarb 1, -ltarb 2, -ltarb 3,\
              -slcmd 0, -slcnum 0, -slcmd 1, -slcnum 1,\
              -slcmd 2, -slcnum 2, -slcmd 3, -slcnum 3,\
              -nalsize,\
              -iper, -thread, -ltr, -db, -denois,\
              -scene    , bgd  , -aq "



    echo ${HeadLine1}>${AllCasePassStatusFile}
    echo ${HeadLine1}>${UnpassCaseFile}
    echo ${HeadLine2}>${UpdateSHA1TableFile}

    #intial Commandline parameters
    runEncoderCommandInital
    let "TotalCaseNum=0"
    let "PassCaseNum=0"
    let "UnpassCaseNum=0"
    EncoderCommand=""
    EncoderLogFile="${TempDataPath}/Encoder.log"
    TargetSHA1=""
    TargetMD5=""
    TargetYUVSHA1=""
    TargetYUVMD5=""
    BenchmarkSHA1=""
    BenchmarkMD5=""
    BenchmarkYUVSHA1=""
    BenchmarkYUVMD5=""
}
#***********************************************************
#called by    runAllCaseTest
# parse case info --encoder preprocess
#usage    runParseCaseInfo $CaseData
runParseCaseInfo()
{
    if [ $#  -lt 1  ]
    then
        echo "no parameter!"
        return 1
    fi
    local TempData=""
    local CaseData=$@
    local BitstreamPrefix=""
    BenchmarkSHA1=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $1} ' `
    BenchmarkMD5=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $2} ' `
    BenchmarkYUVSHA1=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $3} ' `
    BenchmarkYUVMD5=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $4} ' `


    declare -a aTempParamIndex=( 6 7 8 9 10 11 12 13        15 16 17   19 20 21     25 26 27 28   31 32 33 34 35 36 )
    TempData=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {for(i=5;i<=NF;i++) printf(" %s",$i)} ' `
    aEncoderCommandValue=(${TempData})
    let "TempParamFlag=0"

    for((i=0; i<$NumParameter; i++))
    do
        for ParnmIndex in ${aTempParamIndex[@]}
        do
            if [  $i -eq ${ParnmIndex} ]
            then
                let "TempParamFlag=1"
            fi
        done

        if [ ${TempParamFlag} -eq 0 ]
        then
            BitstreamPrefix=${BitstreamPrefix}_${aEncoderCommandName[$i]}_${aEncoderCommandValue[$i]}
        fi
        let "TempParamFlag=0"
    done

    BitstreamTarget=${TempDataPath}/${TestSequenceName}_${BitstreamPrefix}_codec_target.264
    echo ""
    echo "BitstreamPrefix is ${BitstreamPrefix}"
    echo ""
}

runEncodeOneCase()
{
    local ParamCommand=""
    BitStreamFile=${BitstreamTarget}

    for ((i=4; i<${NumParameter}; i++))
    do
        ParamCommand="${ParamCommand} ${aEncoderCommandSet[$i]}  ${aEncoderCommandValue[$i]} "
    done

    for((i=0;i<4;i++))
    do
        aRecYUVFileList[$i]="${TempDataPath}/${TestYUVName}_rec${i}.yuv"
    done

    ParamCommand="${aEncoderCommandSet[0]} ${aEncoderCommandValue[0]} ${aEncoderCommandSet[1]}  ${aEncoderCommandValue[1]} \
                                 ${aEncoderCommandSet[2]}  ${aEncoderCommandValue[2]} \
                                -lconfig 0 layer0.cfg -lconfig 1 layer1.cfg -lconfig 2 layer2.cfg  -lconfig 3 layer3.cfg  \
                                ${aEncoderCommandSet[3]}  ${aEncoderCommandValue[3]}  \
                                ${ParamCommand}"
    echo ""
    echo "---------------Encode One Case-------------------------------------------"
    echo "case line is :"
    EncoderCommand="./h264enc  welsenc.cfg    ${ParamCommand} -bf   ${BitStreamFile} \
                                -drec 0 ${aRecYUVFileList[0]} -drec 1 ${aRecYUVFileList[1]} \
                                -drec 2 ${aRecYUVFileList[2]} -drec 3 ${aRecYUVFileList[3]} \
                                -org ${TestSequencePath}/${TestSequenceName}"
    echo ${EncoderCommand}
    echo -e  "\n\n"
    ./h264enc  welsenc.cfg    ${ParamCommand} -bf   ${BitStreamFile} \
                                -drec 0 ${aRecYUVFileList[0]} -drec 1 ${aRecYUVFileList[1]} \
                                -drec 2 ${aRecYUVFileList[2]} -drec 3 ${aRecYUVFileList[3]} \
                                -org ${TestSequencePath}/${TestSequenceName} 2>${EncoderLogFile}
    if [ $? -eq 0  ]
    then
        let "EncoderFlag=0"
    else
        let "EncoderFlag=1"
    fi

    #delete the core down file as core down files for disk space limitation
    for file in  ./core*
    do
        if [ -e ${file} ]
        then
            ./run_SafeDelete.sh  ${file}
        fi
    done
    return 0
}

#called by    runAllCaseTest
#usage    runJSVMVerify
runBitStreamVerify()
{
    echo ""
    echo "******************************************"
    echo "Bit streamSHA1 value comparison.... "
    #*******************************************
    TargetSHA1="NULL"
    TargetMD5="NULL"
    TargetYUVSHA1="NULL"
    TargetYUVMD5="NULL"

    if [ ${EncoderFlag} -eq 1 ]
    then
        let "UnpassCaseNum++"
    echo "1:unpassed! encoder initial failed or crash!"
        DiffFlag="1:unpassed! encoder initial failed or crash!"
        return 1
    fi

    if [ ! -s ${BitStreamFile} ]
    then
        let "UnpassCaseNum++"
    echo "2:unpassed! 0 bits--bit stream"
        DiffFlag="2:unpassed! 0 bits--bit stream"
        return 1
    fi
    #*******************************************
    #*******************************************
    echo ""
    echo "${BitStreamFile}"
    #SHA1
    TargetSHA1=`openssl sha1  ${BitStreamFile}`
    TargetSHA1=`echo ${TargetSHA1} | awk '{print $2}' `
    TargetMD5=`openssl md5   ${BitStreamFile}`
    TargetMD5=`echo ${TargetMD5} | awk '{print $2}' `

    TargetYUVSHA1=`openssl sha1  ${TestSequencePath}/${TestSequenceName} `
    TargetYUVSHA1=`echo ${TargetYUVSHA1} | awk '{print $2}' `
    TargetYUVMD5=`openssl md5  ${TestSequencePath}/${TestSequenceName} `
    TargetYUVMD5=`echo ${TargetYUVMD5} | awk '{print $2}' `

    if [[   "${TargetSHA1}"  =~  "${BenchmarkSHA1}"  ]]
    then
        echo "bitstream pass!      SHA1--${TargetSHA1} ----- ${BenchmarkSHA1}"
        echo "MD5 info:                MD5--${TargetMD5} ----- ${BenchmarkMD5}"
        echo "YUV SHA1 info:       SHA1--${TargetYUVSHA1} ---- ${BenchmarkYUVSHA1} "
        echo "YUV MD5  info:       MD5--${TargetYUVMD5} ---- ${BenchmarkYUVMD5}"
        DiffFlag="0:passed!"
        let "PassCaseNum++"
        return 0
    else
        echo "!!! SHA1 string not match: ${TargetSHA1}  ----- ${BenchmarkSHA1}  "
        echo "MD5 info:                MD5--${TargetMD5} ----- ${BenchmarkMD5}"
        echo "YUV SHA1 info:       SHA1--${TargetYUVSHA1} ---- ${BenchmarkYUVSHA1} "
        echo "YUV MD5  info:       MD5--${TargetYUVMD5} ---- ${BenchmarkYUVMD5}"
        DiffFlag="3:unpassed!"
        let "UnpassCaseNum++"
        return 1
    fi
}

#called by    runAllCaseTest
#delete temp data    files and output single case test result to log file
#usage    runSingleCasePostAction $CaseData
runSingleCasePostAction()
{
    if [ $#  -lt 1  ]
    then
        echo "no parameter!"
        return 1
    fi
    local CaseData=$@
    CaseInfo=`echo $CaseData | awk 'BEGIN {FS="[,\r]"} {for(i=5;i<=NF;i++) printf(" %s,",$i)} '`
    PassStatusInfo="${DiffFlag}, ${TargetSHA1}, ${BenchmarkSHA1}, ${TargetMD5}, ${BenchmarkMD5},\
        ${TargetYUVMD5}, ${BenchmarkYUVMD5}, ${TargetYUVSHA1}, ${BenchmarkYUVSHA1},\
        ${CaseInfo}, ${EncoderCommand} "
    echo "${PassStatusInfo}">>${AllCasePassStatusFile}
    if [ "$DiffFlag" != "0:passed!"  ]
    then
        echo "${PassStatusInfo}">>${UnpassCaseFile}
    fi
     echo "${TargetSHA1}, ${TargetMD5},${TargetYUVMD5}, ${TargetYUVSHA1},${CaseInfo}">>${UpdateSHA1TableFile}
    ./run_SafeDelete.sh ${BitstreamTarget} >>${AllCaseConsoleLogFile}
}

#usage runOutputPassNum
runOutputPassNum()
{
    # output file locate in ../result
    echo ""
    echo "***********************************************************"
    echo "${TestSequenceName}"
    echo "total case  Num is: ${TotalCaseNum}"
    echo "pass  case  Num is: ${PassCaseNum}"
    echo "unpass case Num is: ${UnpassCaseNum}"
    echo "***********************************************************"
    echo ""
}

# run all test case based on XXXcase.csv file
#usage    runAllCaseTest
runAllCaseTest()
{
    local EncoderLogInfo=""
    while read CaseData
    do
        #get case parameter's value
        if [[ ! $CaseData =~ "SHA"  ]]
        then
            echo "" >>${AllCaseConsoleLogFile}
            echo "" >>${AllCaseConsoleLogFile}
            echo "" >>${AllCaseConsoleLogFile}
            echo "********************case index is ${TotalCaseNum}**************************************"  >>${AllCaseConsoleLogFile}
            runParseCaseInfo ${CaseData}  >>${AllCaseConsoleLogFile}
            echo ""                       >>${AllCaseConsoleLogFile}
            runEncodeOneCase              >>${AllCaseConsoleLogFile}
            cat    ${EncoderLogFile}      >>${AllCaseConsoleLogFile}
            runBitStreamVerify            >>${AllCaseConsoleLogFile}
            let "DisplayFlag=TotalCaseNum%100"
            if [  ${DisplayFlag} -eq 0  ]
            then
                if [  "$DiffFlag" = "0:passed!"   ]
                then
                  echo -e "\033[32m    OK!   ${TestSequenceName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}-----${BenchmarkSHA1}  \033[0m"
                  echo -e "\033[32m            ----MD5 (Current--Benchmark): ${TargetMD5}-----${BenchmarkMD5} \033[0m"
                  echo -e "\033[32m            ----YUVMD5:  ${TargetYUVMD5},  ${BenchmarkYUVMD5}   YUVSHA1:  ${TargetYUVSHA1}, ${BenchmarkYUVSHA1}  \033[0m"
                fi
            fi
            #******************************************
            if [ ! "$DiffFlag" = "0:passed!"  ]
            then
                echo -e "\033[31m    Failed! ${TestSequenceName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}-----${BenchmarkSHA1}  \033[0m"
                echo -e "\033[31m            ----MD5 (Current--Benchmark): ${TargetMD5}-----${BenchmarkMD5} \033[0m"
                echo -e "\033[31m            ----YUVMD5:  ${TargetYUVMD5},  ${BenchmarkYUVMD5}   YUVSHA1:  ${TargetYUVSHA1}, ${BenchmarkYUVSHA1}  \033[0m"
                EncoderLogInfo=`cat  ${EncoderLogFile}`
                echo -e "\033[31m           ${EncoderLogInfo}   \033[0m"
            fi
            runSingleCasePostAction  ${CaseData}
            let "TotalCaseNum++"
        fi
    done <$AllCaseFile
}
#***********************************************************
# usage: runMain $TestYUV    $AllCaseFile
runMain()
{
    if [ ! $# -eq 2  ]
    then
        echo "usage: run_BinarySHA1Comparison.sh \$TestYUV  \$AllCaseFile"
        return 1
    fi

    runGlobalVariableDef

    #for test sequence info
    TestSequenceName=$1
    AllCaseFile=$2
    runGlobalVariableInitial
    TestFolder=`echo $CurrentDir | awk 'BEGIN {FS="/"} { i=NF; print $i}'`
    AllCaseConsoleLogFile="${FinalResultPath}/${TestSequenceName}.TestLog"
    CaseSummaryFile="${FinalResultPath}/${TestSequenceName}.Summary"
    FlagFile=""

    #run all cases
    runAllCaseTest

    # output file locate in ./result
    echo "${TestSequenceName},            \
         ${PassCaseNum} pass!,    \
         ${UnpassCaseNum} unpass!,\
         detail file located in ../AllTestData/${TestFolder}/result">${CaseSummaryFile}
    runOutputPassNum

    #generate All case Flag
    if [  ! ${UnpassCaseNum} -eq 0  ]
    then
        echo ""
        exit 1
    else
        echo  ""
        exit 0
    fi
}
TestYUVName=$1
AllCaseFile=$2
runMain    ${TestYUVName}   ${AllCaseFile}

