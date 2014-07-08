#!/bin/bash
#*******************************************************************************
# Encoder binary comparison test model
#       -- Compared with Benchmark version using SHA-1 string
#       -- Test bit stream under folder  openh264/res
#       -- SHA-1 string of Benchmark version for  all cases  of all bit streams
#           under folder  openh264/test/encoder_binary_comparion/SHA1Table
#       -- For more detail,please refer to file AboutTest¡£
#
#brief:
#       -- Test all cases in SHA1TableFile
#          1) Generate SHA1 string for each case
#          2) Compare with benchmark SHA1 string
#          3) If the SHA1 string are the same with benchmark version, test case is marked as passed
#          4) Otherwise, test case is marked as unpass! this may caused by:
#              --the design of encoder has been changed and the bit stream also change
#              --there may be some bugs in encoder and bit stream is not right
#       -- Usage:  ./run_BinarySHA1Comparison.sh  ${TestYUVName}   ${AllCaseSHA1TaleFile}
#
# date:  10/06/2014 Created
#*******************************************************************************
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
  declare -a EncoderCommandSet
  declare -a EncoderCommandName
  declare -a EncoderCommandValue
  #encoder parameters  change based on the case info
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
    EncoderCommandSet=(-frms        \
                        -numtl      \
                        -scrsig     \
                        -rc         \
                        -tarb       \
                        "-lqp 0"    \
                        -iper       \
                        "-slcmd 0"  \
                        "-slcnum 0" \
                        -thread     \
                        -ltr        \
                        -db         \
                        -nalsize    \
                        -denois     \
                        -scene      \
                        -bgd        \
                        -aq)
    EncoderCommandName=(FrEcoded     \
                        NumTempLayer \
                        ContentSig   \
                        RC           \
                        BitRate      \
                        QP           \
                        IntraPeriod  \
                        SlcMd        \
                        SlcMum       \
                        ThrMum       \
                        LTR          \
                        LFilterIDC   \
                        MacNalSize   \
                        DenoiseFlag  \
                        SceneChangeFlag \
                        BackgroundFlag  \
                        AQFlag)
  EncoderCommandValue=(0 0 0 0 0    0 0 0 0 0    0 0 0 0 0   0 0)
  NumParameter=${#EncoderCommandSet[@]}
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
  echo    "BitMatched Status,                      \
            SHA-1-Target, SHA-1-Benchmark,         \
            MD5-Target, MD5-Benchmark,             \
            Bitstream-Target,Bitstream-Benchmark,  \
            YUV-Target,YUV-Benchmark,              \
            -frms, -numtl, -scrsig, -rc,           \
            -tarb, -lqp 0, -iper,                  \
            -slcmd 0,-slcnum 0, -thread,           \
            -ltr, -db, -nalsize,-denois,           \
            -scene,     -bgd,    -aq">${AllCasePassStatusFile}

  echo   "BitMatched Status,                   \
            SHA-1-Target, SHA-1-Benchmark,       \
            MD5-Target, MD5-Benchmark,           \
            Bitstream-Target,Bitstream-Benchmark,\
            YUV-Target,YUV-Benchmark,            \
            -frms, -numtl, -scrsig, -rc,         \
            -tarb, -lqp 0, -iper,                \
            -slcmd 0,-slcnum 0, -thread,         \
            -ltr, -db, -nalsize,-denois,         \
            -scene,     -bgd,    -aq">${UnpassCaseFile}

  echo    "SHA-1 Value,                        \
            MD5String, BitStreamSize, YUVSize, \
            -frms, -numtl, -scrsig, -rc,       \
            -tarb, -lqp 0, -iper,              \
            -slcmd 0,-slcnum 0, -thread,       \
            -ltr, -db, -MaxNalSize,-denois,    \
            -scene,     -bgd,    -aq">${UpdateSHA1TableFile}
  #intial Commandline parameters
  runEncoderCommandInital
  let "TotalCaseNum=0"
  let "PassCaseNum=0"
  let "UnpassCaseNum=0"
  EncoderCommand=""
  EncoderLogFile="${TempDataPath}/Encoder.log"
  TargetSHA1=""
  TargetMD5=""
  TargetYUVSize=""
  TargetBitstreamSize=""
  BenchmarkSHA1=""
  BenchmarkMD5=""
  BenchmarkYUVSize=""
  BenchmarkBitstreamSize=""
}
#***********************************************************
#called by  runAllCaseTest
# parse case info --encoder preprocess
#usage  runParseCaseInfo $CaseData
runParseCaseInfo()
{
  if [ $#  -lt 1  ]
  then
    echo "runParseCaseInfo \$CaseData"
    return 1
  fi
    local TempData=""
  local BitstreamPrefix=""
  local CaseData=$@
  BenchmarkSHA1=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $1} ' `
  BenchmarkMD5=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $2} ' `
  BenchmarkYUVSize=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $4} ' `
  BenchmarkBitstreamSize=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {print $3} ' `
  TempData=`echo $CaseData |awk 'BEGIN {FS="[,\r]"} {for(i=5;i<=NF;i++) printf(" %s",$i)} ' `
  EncoderCommandValue=(${TempData})
  for((i=0; i<$NumParameter; i++))
  do
    BitstreamPrefix=${BitstreamPrefix}_${EncoderCommandName[$i]}_${EncoderCommandValue[$i]}
  done
  BitstreamTarget=${TempDataPath}/${TestSequenceName}_${BitstreamPrefix}_codec_target.264
  echo ""
  echo "BitstreamPrefix is ${BitstreamPrefix}"
  echo ""
}
#called  by  runAllCaseTest
#usage  runEncodeOneCase
runEncodeOneCase()
{
        BitStreamFile=${BitstreamTarget}
        CaseCommand=" ${ConfigureFile} \
                -numl 1                      \
                -lconfig 0 layer2.cfg        \
                -sw   ${PicW} -sh   ${PicH}  \
                -dw 0 ${PicW} -dh 0 ${PicH}  \
                -frout 0  30                 \
                -ltarb 0  ${EncoderCommandValue[4]}                 \
                ${EncoderCommandSet[0]}  ${EncoderCommandValue[0]}  \
                ${EncoderCommandSet[1]}  ${EncoderCommandValue[1]}  \
                ${EncoderCommandSet[2]}  ${EncoderCommandValue[2]}  \
                ${EncoderCommandSet[3]}  ${EncoderCommandValue[3]}  \
                ${EncoderCommandSet[4]}  ${EncoderCommandValue[4]}  \
                ${EncoderCommandSet[5]}  ${EncoderCommandValue[5]}  \
                ${EncoderCommandSet[6]}  ${EncoderCommandValue[6]}  \
                ${EncoderCommandSet[7]}  ${EncoderCommandValue[7]}  \
                ${EncoderCommandSet[8]}  ${EncoderCommandValue[8]}  \
                ${EncoderCommandSet[9]}  ${EncoderCommandValue[9]}  \
                ${EncoderCommandSet[10]} ${EncoderCommandValue[10]} \
                ${EncoderCommandSet[11]} ${EncoderCommandValue[11]} \
                ${EncoderCommandSet[12]} ${EncoderCommandValue[12]} \
                ${EncoderCommandSet[13]} ${EncoderCommandValue[13]} \
                ${EncoderCommandSet[14]} ${EncoderCommandValue[14]} \
                ${EncoderCommandSet[15]} ${EncoderCommandValue[15]} \
                ${EncoderCommandSet[16]} ${EncoderCommandValue[16]}"
  echo ${EncoderCommandSet[@]}
  echo ${EncoderCommandValue[@]}
  echo ${EncoderCommandSet[11]}
  echo ${EncoderCommandValue[11]}
  echo ${EncoderCommandSet[12]}
  echo ${EncoderCommandValue[12]}
  EncoderCommand="./h264enc  ${CaseCommand} -bf   ${BitStreamFile}  -org   ${TestSequencePath}/${TestSequenceName} "
  echo ""
  echo "case line is :"
  echo "   ${EncoderCommand}"
  echo -e  "\n\n"
        ./h264enc ${CaseCommand}      \
                -bf ${BitStreamFile}  \
                -org  ${TestSequencePath}/${TestSequenceName} 2>${EncoderLogFile}
}
#usage? runGetFileSize  $FileName
runGetFileSize()
{
  if [ $#  -lt 1  ]
  then
    echo "usage runGetFileSize  $FileName!"
    return 1
  fi
  local FileName=$1
  local FileSize=""
  local TempInfo=""
  TempInfo=`ls -l $FileName`
  FileSize=`echo $TempInfo | awk '{print $5}'`
  echo $FileSize
}
#called by  runAllCaseTest
#usage  runJSVMVerify
runBitStreamVerify()
{
  echo ""
  echo "******************************************"
  echo "Bit streamSHA1 value comparison.... "
  #*******************************************
  if [ ! -s ${BitStreamFile} ]
  then
    let "UnpassCaseNum++"
    DiffFlag="2:unpassed! 0 bits--bit stream"
    return 1
  fi
  #*******************************************
  #*******************************************
  #SHA1(run_Test.sh)= fde74d9e8194d0cf28991a0481c7158a033ec920
  TargetSHA1=`openssl sha1  ${BitStreamFile}`
  TargetSHA1=`echo ${TargetSHA1} | awk '{print $2}' `
  TargetMD5=`openssl md5   ${BitStreamFile}`
  TargetMD5=`echo ${TargetMD5} | awk '{print $2}' `
  TargetYUVSize=`runGetFileSize  ${TestSequencePath}/${TestSequenceName} `
  TargetBitstreamSize=`runGetFileSize   ${BitStreamFile}`
  if [[   "${TargetSHA1}"  =~  "${BenchmarkSHA1}"  ]]
  then
    echo "bitstream pass!      SHA1--${TargetSHA1}  ----- ${BenchmarkSHA1}"
    echo "bitstream pass!      MD5-- ${TargetMD5}  ----- ${BenchmarkMD5}"
    echo "YUV size  pass!      size--${TargetYUVSize}--${BenchmarkYUVSize} "
    echo "BitStreamSize pass!  size--${TargetBitstreamSize}--${BenchmarkBitstreamSize}"
    DiffFlag="0:passed!"
    let "PassCaseNum++"
    return 0
  else
    echo "!!! SHA1 string not match: ${TargetSHA1}  ----- ${BenchmarkSHA1}  "
    echo "bitstream pass!      MD5-- ${TargetMD5}  ----- ${BenchmarkMD5}"
    echo "YUV size  pass!      size--${TargetYUVSize}--${BenchmarkYUVSize} "
    echo "BitStreamSize pass!  size--${TargetBitstreamSize}--${BenchmarkBitstreamSize}"
    DiffFlag="1:unpassed!"
    let "UnpassCaseNum++"
    return 1
  fi
}
#called by  runAllCaseTest
#delete temp data  files and output single case test result to log file
#usage  runSingleCasePostAction $CaseData
runSingleCasePostAction()
{
  if [ $#  -lt 1  ]
  then
    echo "no parameter!"
    return 1
  fi
  local CaseData=$@
  CaseInfo=`echo $CaseData | awk 'BEGIN {FS="[,\r]"} {for(i=5;i<=NF;i++) printf(" %s,",$i)} '`
    echo "${DiffFlag}, ${TargetSHA1}, ${BenchmarkSHA1}, {TargetMD5}, ${BenchmarkMD5},  \
    ${TargetBitstreamSize} , ${BenchmarkBitstreamSize},\
    ${TargetYUVSize},  ${BenchmarkYUVSize},\
    ${CaseInfo}, ${EncoderCommand} "  >>${AllCasePassStatusFile}

    echo "${TargetSHA1}, ${TargetMD5},${TargetBitstreamSize}, ${TargetYUVSize},${CaseInfo}">>${UpdateSHA1TableFile}

  ./run_SafeDelete.sh ${BitstreamTarget} >>${AllCaseConsoleLogFile}
}
#usage runOutputPassNum
runOutputPassNum()
{
  # output file locate in ../result
  echo ""
  echo "***********************************************************"
  echo "${TestSequenceName}"
  echo "total case  Num is :  ${TotalCaseNum}"
  echo "pass  case  Num is : ${PassCaseNum}"
  echo "unpass case Num is : ${UnpassCaseNum} "
  echo "***********************************************************"
  echo ""
}
# run all test case based on XXXcase.csv file
#usage  runAllCaseTest
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
      echo ""  >>${AllCaseConsoleLogFile}
      runEncodeOneCase    >>${AllCaseConsoleLogFile}
      cat  ${EncoderLogFile}>>${AllCaseConsoleLogFile}
      runBitStreamVerify     >>${AllCaseConsoleLogFile}
      let "DisplayFlag=TotalCaseNum%100"
      if [  ${DisplayFlag} -eq 0  ]
      then
        if [  "$DiffFlag" = "0:passed!"   ]
        then
          echo -e "\033[32m    OK!   ${TestSequenceName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}  ----- ${BenchmarkSHA1}  \033[0m"
          echo -e "\033[32m            ----MD5 (Current--Benchmark): ${TargetMD5}, ${BenchmarkMD5} \033[0m"
          echo -e "\033[32m            ----BitstreamSize:  ${TargetBitstreamSize},  ${BenchmarkBitstreamSize}   YUVSize:  ${TargetYUVSize}, ${BenchmarkYUVSize}  \033[0m"
        fi
      fi
      #******************************************
      if [ ! "$DiffFlag" = "0:passed!"  ]
      then
        echo -e "\033[31m    Failed! ${TestSequenceName} Case Index ${TotalCaseNum}:SHA-1(Current--Benchmark): ${TargetSHA1}  ----- ${BenchmarkSHA1}  \033[0m"
        echo -e "\033[31m            ----MD5 (Current--Benchmark): ${TargetMD5}, ${BenchmarkMD5} \033[0m"
        echo -e "\033[31m            ----BitstreamSize:  ${TargetBitstreamSize},  ${BenchmarkBitstreamSize}   YUVSize:  ${TargetYUVSize}, ${BenchmarkYUVSize}  \033[0m"
        EncoderLogInfo=`cat  ${EncoderLogFile}`
        echo -e "\033[31m           ${EncoderLogInfo}   \033[0m"
        echo "${DiffFlag}, ${TargetSHA1}, ${BenchmarkSHA1}, ${TargetMD5}, ${BenchmarkMD5},  \
            ${TargetBitstreamSize} , ${BenchmarkBitstreamSize},   \
            ${TargetYUVSize},  ${BenchmarkYUVSize},  \
            ${CaseInfo}, ${EncoderCommand}  ">>${UnpassCaseFile}
      fi
      runSingleCasePostAction  ${CaseData}
      let "TotalCaseNum++"
    fi
  done <$AllCaseFile
}
#***********************************************************
# usage: runMain $TestYUV  $AllCaseFile
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
  echo "${TestSequenceName},        \
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
runMain  ${TestYUVName}   ${AllCaseFile}

