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
    #test data space
    FinalResultPath=""
    IssueDataPath=""
    TempDataPath=""
    #for test sequence info
    PicW=""
    PicH=""
    #xxx.csv
    AllCasePassStatusFile=""
    #for encoder command
    declare -a aEncoderCommandSet
    declare -a aEncoderCommandName
    declare -a aEncoderCommandValue
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
                         "-lmaxb 0"   "-lmaxb 1"  "-lmaxb 2"  "-lmaxb 3" \
                         "-slcmd 0" "-slcnum 0" "-slcmd 1" "-slcnum 1"\
                         "-slcmd 2" "-slcnum 2" "-slcmd 3" "-slcnum 3"\
                         -nalsize \
                         -iper     -thread  "-loadbalancing"  -ltr \
                         -db    -denois    -scene    -bgd    -aq )

    aEncoderCommandName=(usagetype    frms  numl   numtl \
                         sw sh    dw0 dh0 dw1 dh1 dw2 dh2 dw3 dh3 \
                         frout0 frout1 frout2 frout3 \
                         lqp0 lqp1 lqp2 lqp3 \
                         rc FrSkip tarb ltarb0  ltarb1 ltarb2 ltarb3 \
                         lmaxb0  lmaxb1 lmaxb2 lmaxb3 \
                         slcmd0 slcnum0 slcmd1 slcnum1 \
                         slcmd2 slcnum2 slcmd3 slcnum3 \
                         MaxNalSZ \
                         iper thread  loadbalancing ltr \
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

    #test info output file
    AllCasePassStatusFile="${FinalResultPath}/${TestYUVName}_AllCaseOutput.csv"
    UnpassCaseFile="${FinalResultPath}/${TestYUVName}_unpassCaseOutput.csv"
    UpdateSHA1TableFile="${FinalResultPath}/${TestYUVName}_UpdateSHA1Table.csv"

   HeadLine1="EncoderFlag, DecoderFlag, FPS, BitSreamSHA1, InputYUVSHA1, \
              -utype,    -frms,  -numl,  -numtl, -sw, -sh,\
              -dw 0, -dh 0, -dw 1, -dh 1, -dw 2, -dh 2, -dw 3, -dh 3,\
              -frout 0,    -frout 1, -frout 2, -frout 3,\
              -lqp 0, -lqp 1, -lqp 2, -lqp 3,\
              -rc,-fs, -tarb, -ltarb 0, -ltarb 1, -ltarb 2, -ltarb 3,\
              -lmaxb 0,   -lmaxb 1,  -lmaxb 2,  -lmaxb 3,\
              -slcmd 0, -slcnum 0, -slcmd 1, -slcnum 1,\
              -slcmd 2, -slcnum 2, -slcmd 3, -slcnum 3,\
              -nalsize,\
              -iper, -thread, -loadbalancing, -ltr, -db, -denois,\
              -scene,    -bgd ,  -aq, "

    HeadLine2="BitSreamSHA1, InputYUVSHA1,\
              -utype,    -frms,  -numl,  -numtl, -sw, -sh,\
              -dw 0, -dh 0, -dw 1, -dh 1,-dw 2, -dh 2, -dw 3, -dh 3,\
              -frout 0,    -frout 1, -frout 2, -frout 3,\
              -lqp 0, -lqp 1, -lqp 2, -lqp 3,\
              -rc, -fs, -tarb, -ltarb 0, -ltarb 1, -ltarb 2, -ltarb 3,\
              -lmaxb 0,   -lmaxb 1,  -lmaxb 2,  -lmaxb 3,\
              -slcmd 0, -slcnum 0, -slcmd 1, -slcnum 1,\
              -slcmd 2, -slcnum 2, -slcmd 3, -slcnum 3,\
              -nalsize,\
              -iper, -thread, -loadbalancing, -ltr, -db, -denois,\
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
    TargetYUVSHA1=""
    BenchmarkSHA1=""
    BenchmarkYUVSHA1=""

    RecParam=""
    for((i=0;i<4;i++))
    do
        RecParam="${RecParam} -drec ${i} ${TempDataPath}/${TestYUVName}_rec${i}.yuv"
    done
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

    local CaseData=$@
    BenchmarkSHA1=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $1} ' `
    BenchmarkYUVSHA1=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $2} ' `
    aEncoderCommandValue=(`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {for(i=3;i<=NF;i++) printf(" %s",$i)} ' `)
    BitstreamTarget=${TempDataPath}/${TestYUVName}_codec_target.264
}

runEncodeOneCase()
{
    EncoderCommand=""
    BitStreamFile=${BitstreamTarget}

    for ((i=0; i<${NumParameter}; i++))
    do
        EncoderCommand="${EncoderCommand} ${aEncoderCommandSet[$i]}  ${aEncoderCommandValue[$i]} "
    done
    if [ "$TestWasm" = "0" ]
    then
        EncoderCommand="./h264enc welsenc.cfg -lconfig 0 layer0.cfg -lconfig 1 layer1.cfg -lconfig 2 layer2.cfg  -lconfig 3 layer3.cfg \
                    -bf ${BitStreamFile} -org ${TestSequencePath}/${TestYUVName} ${RecParam} ${EncoderCommand}"
    else 
        EncoderCommand="node ./h264enc.js welsenc.cfg -lconfig 0 layer0.cfg -lconfig 1 layer1.cfg -lconfig 2 layer2.cfg  -lconfig 3 layer3.cfg \
                    -bf ${BitStreamFile} -org ${TestSequencePath}/${TestYUVName} ${RecParam} ${EncoderCommand}"
    fi
    echo -e "\n---------------Encode One Case-------------------------------------------"
    echo -e "case encode command is : \n ${EncoderCommand} "

    echo -e  "\n\n"
    ${EncoderCommand}  2>${EncoderLogFile}
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
    echo -e "\n******************************************"
    echo -e "Bit stream SHA1 value comparison.... "
    #*******************************************
    TargetSHA1="NULL"
    TargetYUVSHA1="NULL"

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
    #SHA1
    TargetSHA1=`openssl sha1  ${BitStreamFile} | awk '{print $2}'`
    TargetYUVSHA1=`openssl sha1  ${TestSequencePath}/${TestYUVName} | awk '{print $2}'`
    if [[  "${TargetSHA1}"  =~  "${BenchmarkSHA1}"  ]]
    then
        echo "bitstream pass! SHA1--${TargetSHA1} ----- ${BenchmarkSHA1} YUV--SHA1--info: ${TargetYUVSHA1} ---- ${BenchmarkYUVSHA1}"
        DiffFlag="0:passed!"
        let "PassCaseNum++"
        return 0
    else
        echo "!!! SHA1 string not match: ${TargetSHA1}  ----- ${BenchmarkSHA1} YUV--SHA1--info: ${TargetYUVSHA1} ---- ${BenchmarkYUVSHA1}"
        DiffFlag="3:unpassed!"
        let "UnpassCaseNum++"
        return 1
    fi
}

#called by  runAllCaseTest
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
    #formating for update, keep the same with origin SHA1 table
    CaseInfo=`echo $CaseData | awk 'BEGIN {FS="[,\r]"} {for(i=3;i<=NF-1;i++) printf("%s,",$i)} '`
    LastEncCommandOption=`echo $CaseData | awk 'BEGIN {FS="[,\r]"} {print $NF} '`
    CaseInfo="${CaseInfo}${LastEncCommandOption}"

    PassStatusInfo="${DiffFlag}, ${TargetSHA1}, ${BenchmarkSHA1}, ${TargetYUVSHA1}, ${BenchmarkYUVSHA1}, ${CaseInfo}, ${EncoderCommand} "
    echo "${PassStatusInfo}">>${AllCasePassStatusFile}
    if [ "$DiffFlag" != "0:passed!"  ]
    then
        echo "${PassStatusInfo}">>${UnpassCaseFile}
    fi

    echo "${TargetSHA1}, ${TargetYUVSHA1},${CaseInfo}">>${UpdateSHA1TableFile}
    #./run_SafeDelete.sh ${BitstreamTarget} >>${AllCaseConsoleLogFile}
}

#usage runOutputPassNum
runOutputPassNum()
{
    # output file locate in ../result
    echo ""
    echo "***********************************************************"
    echo "${TestYUVName}"
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
                  echo -e "\033[32m    OK!   ${TestYUVName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}-----${BenchmarkSHA1}    \033[0m"
                  echo -e "\033[32m          ----YUVSHA1: ${TargetYUVSHA1}, ${BenchmarkYUVSHA1} \033[0m"
                fi
            fi
            #******************************************
            if [ ! "$DiffFlag" = "0:passed!"  ]
            then
                echo -e "\033[31m    Failed! ${TestYUVName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}-----${BenchmarkSHA1}  \033[0m"
                echo -e "\033[31m            ----YUVSHA1:  ${TargetYUVSHA1}, ${BenchmarkYUVSHA1}  \033[0m"
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
    runGlobalVariableDef
    runGlobalVariableInitial
    TestFolder=`echo $CurrentDir | awk 'BEGIN {FS="/"} { i=NF; print $i}'`
    AllCaseConsoleLogFile="${FinalResultPath}/${TestYUVName}.TestLog"
    CaseSummaryFile="${FinalResultPath}/${TestYUVName}.Summary"
    FlagFile=""

    #run all cases
    runAllCaseTest

    # output file locate in ./result
    echo "${TestYUVName},            \
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
#************************************************************************
# main entry
#************************************************************************
if [ ! $# -eq 2  ]
then
    echo "usage: run_BinarySHA1Comparison.sh \$TestYUV  \$AllCaseFile"
    return 1
fi
TestYUVName=$1
AllCaseFile=$2
runMain

