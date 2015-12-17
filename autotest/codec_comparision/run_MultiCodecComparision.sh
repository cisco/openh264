#!/bin/bash
#*******************************************************************
# brief: multi-encoders comparision for openh264
#        (one given sequence only)
#        comparision almong encoders in \$TestEncoderList
#
#        more detail, please refer to runUsage() and runBrief()
#
# date: 2015-12-16
#*******************************************************************

runUsage()
{
    echo -e "\033[32m ********************************************************************* \033[0m"
    echo " Usage: "
    echo "      --$0 \$TestPicW \$TestPicH \$TestEncoderList"
    echo ""
    echo "      --example:"
    echo "        $0 1280 720 h264enc_master h264enc_target1 h264enc_target2 "
    echo ""
    echo " Pre-test:"
    echo "      --1) copy welsenc.cfg from ./openh264/testbin/"
    echo "      --2) set test YUV path in welsenc.cfg "
    echo "      --3) copy layer0.cfg from ./openh264/testbin/layer2.cfg"
    echo "      --4) copy layer1.cfg from ./openh264/testbin/layer2.cfg"
    echo "      --5) copy layer2.cfg from ./openh264/testbin/layer2.cfg"
    echo "      --6) copy layer3.cfg from ./openh264/testbin/layer2.cfg"
    echo "           layer0.cfg~layer3.cfg are used for multi-layers test cases"
    echo ""
    echo "      --7) generate at least one encoder, "
    echo "           eg. h264enc_master----master branch as benchmark codec"
    echo "               h264enc_target----your branch CodecChanged as target codec"
    echo ""
    echo "      --8) copy all tests codec to folder ./Encoder"
    echo "      --9) run below command line:"
    echo "           $0 \$TestPicW \$TestPicH \$TestEncoderList"
    echo ""
    echo " Post-test:"
    echo "      --1) temp cases log will be output in ./Trace-AllTestData"
    echo "      --2) all comparision data parsed from log files will be output to "
    echo "           related .csv file under ./Trace-AllTestData "
    echo ""
    echo " example:"
    echo "     --comparison almong h264enc_master h264enc_target1 h264enc_target2"
    echo "       for Zhuling_1280x720.yuv"
    echo ""
    echo "     --run command as below:"
    echo "       $0 1280 720 h264enc_master h264enc_target1 h264enc_target2 "
    echo ""
    echo "     --get final result files(.csv) under ./Trace-AllTestData"
    echo ""
    echo -e "\033[32m ********************************************************************* \033[0m"
}

runBrief()
{
    echo -e "\033[32m ********************************************************************* \033[0m"
    echo " brief:"
    echo ""
    echo " encoder veision comparision "
    echo "       --comparision almong encoders in \$TestEncoderList"
    echo "       --please generate at least one encoder and copy to folder ./Encoder"
    echo "       --script will run all test cases for each test encoder"
    echo "         and generate related trace log files for each encoder"
    echo "       --script will parse and extact data based on keyword from trace log file"
    echo "         and output to related .csv files for all encoder"
    echo "       --the test outout file will be put under ./Trace-AllTestData"
    echo ""
    echo " test cases:"
    echo "       --add more cases in function runGlogbleInit()"
    echo "       --add new argument with for loop  like rc. etc in function "
    echo "         runAllEncodeCasesAndGenerateLog()"
    echo ""
    echo " new data:"
    echo "      --currently only memory usage, you can add new data for your comparision"
    echo "      --need to add related data parse in function runParseTraceLog()"
    echo ""
    echo -e "\033[32m ********************************************************************* \033[0m"
}

runPrompt()
{
    echo -e "\033[32m ********************************************************************* \033[0m"
    echo ""
    echo "             ------Test completed!--------"
    echo ""
    echo -e "\033[32m ********************************************************************* \033[0m"
    echo " "
    echo "   --Total ${iTotalCaseNum} cases run for encoders: ${aEncoderList[@]}"
    echo ""
    echo "   --Statistic files for comparision are list as below:"
    echo "     ${MemoryUsageStatic}"
    echo ""
    echo "   --trace log files can be found under:"
    echo "     ${LogDir}"
    echo ""
    echo -e "\033[32m ********************************************************************* \033[0m"
}

runGlogbleInit()
{
    CurrenDir=`pwd`
	LogDir="${CurrenDir}/Trace-AllTestData"
    EncoderDir="${CurrenDir}/Encoder"

    if [ ! -d ${LogDir} ]
    then
        mkdir ${LogDir}
    fi


    LogFile="Log_EncTraceInfo.log"
    MemoryUsageStatic="${LogDir}/MemoryUsage.csv"
    TempEncoderList=""
    for((i=0; i<${#aEncoderList[@]}; i++))
    do
        if [ -z "${TempEncoderList}" ]
        then
            TempEncoderList="${aEncoderList[$i]},"
        else
            TempEncoderList="${TempEncoderList} ${aEncoderList[$i]},"
        fi
    done

    let "iTotalCaseNum=0"
    let "MemoryUsage = 0"
    echo "LogDir is ${LogDir}"
    echo "MemoryUsageStatic file is ${MemoryUsageStatic}"
    echo "SpatialLayerNum, ThreadNum, SliceMode, SliceNum, SlicMbNum, ${TempEncoderList}" >${MemoryUsageStatic}

    echo "LogDir is ${LogDir}"
    echo "MemoryUsageStatic file is ${MemoryUsageStatic}"


    let "iTraceLevel=4"
    let "iMaxNalSize=0"
    #you can add more test case like rc, gop size, et.
    #and add "for loop" in function runAllEncodeCasesAndGenerateLog()
    aSpatialLayerNum=(1 2 3 4 )
    aThreadIdc=(1 4)
    aSliceMode=(0 1 2 3)
    aSliceNum=(0 8 16 32)
    aSliceMbNum=(0 960)

    Encoder=""
    sEncoderCommand=""
}

runCheck()
{
    if [ ! -d ${EncoderDir} ]
    then
        echo "encoder folder does not exist, please following below command to copy encoder to test folder--./Encoder"
        echo "   mkdir Encoder"
        echo "   cp \${AllVersionEncoders}  ./Encoder "
        exit 1
    fi

    let "bEncoderFlag = 0"
    echo "aEncoderList is ${aEncoderList[@]} "
    for file in ${aEncoderList[@]}
    do
        if [ -x ${EncoderDir}/${file} ]
        then
            let "bEncoderFlag = 1"
        fi
    done

    if [ ${bEncoderFlag} -eq 0 ]
    then
        echo "no encoder under test folder, please following below command to copy encoder to test folder--./Encoder"
        echo "   cp \${AllVersionEncoders}  ./Encoder "
        echo "   chmod u+x ./Encoder/* "
        exit 1
    fi
}


runGenerateSpatialLayerResolution()
{
    SpatialLayerNum=$1
    if [ -z "${SpatialLayerNum}" ]
    then
        let "SpatialLayerNum =1"
    fi

    let "PicW_L0= PicW / 8"
    let "PicW_L1= PicW / 4"
    let "PicW_L2= PicW / 2"
    let "PicW_L3= PicW"

    let "PicH_L0= PicH / 8"
    let "PicH_L1= PicH / 4"
    let "PicH_L2= PicH / 2"
    let "PicH_L3= PicH"

    if [ ${SpatialLayerNum} -eq 1 ]
    then
        aPicW=( ${PicW_L3} 0 0 0 )
        aPicH=( ${PicH_L3} 0 0 0 )

    elif [ ${SpatialLayerNum} -eq 2 ]
    then
        aPicW=( ${PicW_L2} ${PicW_L3} 0 0 )
        aPicH=( ${PicH_L2} ${PicH_L3} 0 0 )

    elif [ ${SpatialLayerNum} -eq 3 ]
    then
        aPicW=( ${PicW_L1} ${PicW_L2} ${PicW_L3} 0 )
        aPicH=( ${PicH_L1} ${PicH_L2} ${PicH_L3} 0 )

    elif [ ${SpatialLayerNum} -eq 4 ]
    then
        aPicW=( ${PicW_L0} ${PicW_L1} ${PicW_L2} ${PicW_L3} )
        aPicH=( ${PicH_L0} ${PicH_L1} ${PicH_L2} ${PicH_L3} )
    fi

    echo "*************************************************************************"
    echo "  ${SpatialLayerNum} layers spactial resolution for ${PicW}x${PicH} are:"
    echo ""
    echo "  aPicW is ${aPicW[@]}"
    echo "  aPicH is ${aPicH[@]}"
    echo "*************************************************************************"

}

#parse data from encoder trace log
#you can add more key word to extract data from log file
runParseTraceLog()
{
    TempLogFile=$1
    let "MemoryUsage = 0"

    echo "*****************************************"
    echo "parsing trace log file"
    echo "log file name is ${TempLogFile}"
    echo "*****************************************"

    if [ ! -e ${TempLogFile} ]
    then
        echo "LogFile ${TempLogFile} does not exist, please double check!"
        return 1
    fi

    MemUsageInLog=""
    while read line
    do
        if [[ "${line}" =~ "overall memory usage" ]]
        then
            #[OpenH264] this = 0x0x7fa4d2c04c30, Info:WelsInitEncoderExt() exit, overall memory usage: 40907254 bytes
            MemUsageInLog=(`echo $line | awk 'BEGIN {FS="usage:"} {print $2}' `)
        fi

        # you can add more key word to extract data from log file
        # e.g.: bit rate, fps, encoder time, psnr etc.
        # add script block like:
        # ****************************************************
        #   if [[ "${line}" =~ "KeyWordYouWantToSearch" ]]
        #   then
        #       $line in log file which contain data you want
        #       DataYouWant=(`echo $line | awk 'BEGIN {FS="keywordYourSearch"} {print $2}' `)
        #   fi
        # ****************************************************

    done < ${TempLogFile}

    let "MemoryUsage = ${MemUsageInLog}"
    echo "MemoryUsage is ${MemoryUsage}"
}

runEncodeOneCase()
{
    #encoding process
    echo "------------------------------------------------------"
    echo "${Encoder} welsenc.cfg ${sEncoderCommand}" >${LogFile}
    ${Encoder} welsenc.cfg ${sEncoderCommand}      2>>${LogFile}
    ${Encoder} welsenc.cfg ${sEncoderCommand}       >>${LogFile}
    echo "------------------------------------------------------"
}

runAllEncodeCasesAndGenerateLog()
{

    echo "aSpatialLayerNum is ${aSpatialLayerNum[@]}"
    echo "aThreadIdc       is ${aThreadIdc[@]}"
    echo "aSliceMode       is ${aSliceMode[@]}"
    echo "aSliceNum        is ${aSliceNum[@]}"
    echo "aSliceMbNum      is ${aSliceMbNum[@]}"

    sEncoderCommand1="-lconfig 0 layer0.cfg -lconfig 1 layer1.cfg -lconfig 2 layer2.cfg  -lconfig 3 layer3.cfg"
    TempMemoryUsage=""
    TempTestCase=""
    let "CaseNum=1"
    for iSLayerNum in ${aSpatialLayerNum[@]}
    do
        for iThreadNum in ${aThreadIdc[@]}
        do
            for iSliceMode in ${aSliceMode[@]}
            do
                for iSliceNum in ${aSliceNum[@]}
                do
                    #raster slice mb mode, slice-mb-num =0, switch to row-mb-mode
                    if [ ${iSliceMode} -eq 2 ]
                    then
                        aSliceMbNum=(0 960)
                    else
                        aSliceMbNum=(960)
                    fi

                    for iSlicMbNum in ${aSliceMbNum[@]}
                    do
                        TempMemoryUsage=""
                        #for cases output to statistic file
                        TempTestCase="${iSLayerNum}, ${iThreadNum}, ${iSliceMode}, ${iSliceNum}, ${iSlicMbNum}"

                        for eEncoder in ${aEncoderList[@]}
                        do
                            Encoder=${EncoderDir}/${eEncoder}
                            if [ -x ${Encoder} ]
                            then

                                if [ ${iSliceMode} -eq 3 ]
                                then
                                    iMaxNalSize=1000
                                else
                                    iMaxNalSize=0
                                fi

                                runGenerateSpatialLayerResolution ${iSLayerNum}

                                sEncoderCommand2="-slcmd 0 ${iSliceMode} -slcmd 1 ${iSliceMode} -slcmd 2 ${iSliceMode} -slcmd 3 ${iSliceMode}"
                                sEncoderCommand3="-slcnum 0 ${iSliceNum} -slcnum 1  ${iSliceNum} -slcnum 2 ${iSliceNum} -slcnum 3 ${iSliceNum}"
                                sEncoderCommand4="-slcmbnum 0 ${iSlicMbNum} -slcmbnum 1 ${iSlicMbNum} -slcmbnum 2 ${iSlicMbNum} -slcmbnum 3 ${iSlicMbNum} "

                                sEncoderCommand5="-trace ${iTraceLevel} -numl  ${iSLayerNum}  -thread  ${iThreadNum} -nalsize  ${iMaxNalSize}"

                                sEncoderCommand6="-dw 0 ${aPicW[0]} -dw 1 ${aPicW[1]}  -dw 2 ${aPicW[2]} -dw 3 ${aPicW[3]}"
                                sEncoderCommand7="-dh 0 ${aPicH[0]} -dh 1 ${aPicH[1]}  -dh 2 ${aPicH[2]} -dh 3 ${aPicH[3]}"

                                sEncoderCommand="${sEncoderCommand1} ${sEncoderCommand2} ${sEncoderCommand3} ${sEncoderCommand4} ${sEncoderCommand5} ${sEncoderCommand6} ${sEncoderCommand7}"

                                LogFile="${LogDir}/${CaseNum}_LogInfo_iSLNum_${iSLayerNum}_ThrNum_${iThreadNum}_SlcM_${iSliceMode}_SlcN_${iSliceNum}_${eEncoder}.log"

                                echo "Encode command is: "
                                echo "${Encoder} welsenc.cfg  ${sEncoderCommand}"
                                echo ""
                                echo "log file is ${LogFile}"

                                #encode one case
                                runEncodeOneCase

                                #parse trace log
                                runParseTraceLog ${LogFile}

                                #data extracted from log
                                #you can add new data here like rc, fps , etc.
                                echo "memory usage is ${MemoryUsage}"
                                if [ -z ${TempMemoryUsage} ]
                                then
                                    TempMemoryUsage="${MemoryUsage},"
                                else
                                    TempMemoryUsage="${TempMemoryUsage} ${MemoryUsage},"
                                fi
                                echo "TempMemoryUsage is ${TempMemoryUsage}"
                            fi
                        done

                        #output memory usage for all encoders
                        echo "${TempTestCase}, ${TempMemoryUsage}" >>${MemoryUsageStatic}
                        let " CaseNum ++"
                        let "iTotalCaseNum ++"

                    done

                done

            done

        done

    done

}


runMain()
{
    runGlogbleInit
    runCheck
    runAllEncodeCasesAndGenerateLog
    runPrompt
}

#*************************************************************
if [ $# -lt 3 ]
then
    runUsage
    runBrief
    exit 1
fi

declare -a aEncoderList
declare -a aParamList

aParamList=( $@ )
ParamNum=$#

PicW=${aParamList[0]}
PicH=${aParamList[1]}

for((i=2;i<$#;i++))
do
    echo "encoder is  ${aParamList[$i]}"
    aEncoderList="${aEncoderList} ${aParamList[$i]}"
done
aEncoderList=(${aEncoderList})


echo -e "\033[32m ********************************* \033[0m"
echo ""
echo "   --num parameters is ${ParamNum} "
echo "   --input parameters are:"
echo "     $0 $@"
echo ""
echo -e "\033[32m ********************************* \033[0m"

runMain
#*************************************************************

