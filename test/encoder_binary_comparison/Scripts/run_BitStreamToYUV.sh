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
#             -- Transform test bit stream into YUV as  test sequence,
#                1) Called by run_OneBitStream.sh  before testing  all cases
#                2) eg:
#                    -- input:    ./run_BitStreamToYUV.sh   TestABC.264
#                    -- output:   TestABC.264_382X288.yuv
#             -- Usage:  run_BitStreamToYUV.sh   ${BitStreamFile}
#
#
# date:    10/06/2014 Created
#*******************************************************************************

#  usage:    run_ParseDecoderLog          $Decoder_LogFile
#  eg:       input: run_ParseDecoderLog   test.264.log
#            output 1024  720
run_ParseDecoderLog()
{
    if [ ! $# -eq 1 ]
    then
        echo "usage:  run_ParseDecoderLog  \$Decoder_LogFile"
        return 1
    fi
    local LogFile=$1
    local Width=""
    local Height=""
    while read line
    do
        if [[  $line =~  "iWidth"   ]]
        then
            Width=`echo $line | awk 'BEGIN  {FS="[:\n]"} {print $2}'`
        fi
        if [[  $line =~  "height"   ]]
        then
             Height=`echo $line | awk 'BEGIN  {FS="[:\n]"} {print $2}'`
        fi
    done < ${LogFile}
    echo "${Width}  ${Height}"
}

#usage: run_BitStream2YUV    $BitstreamName  $OutputYUVName $LogFile
run_BitStream2YUV()
{
    if [ ! $# -eq 3 ]
    then
        echo "usage: run_BitStream2YUV  \$BitstreamName \$OutputYUVName \$LogFile   "
        return 1
    fi
    local BitStreamName=$1
    local OutputYUVName=$2
    local LogFile=$3
    if [ ! -f ${BitStreamName}  ]
    then
        echo "bit stream file does not exist!"
        echo "detected by run_BitStreamToYUV.sh"
        return 1
    fi
    #decode bitstream
    ./h264dec  ${BitStreamName}  ${OutputYUVName} 2> ${LogFile}
    return 0
}

#usage: run_RegularizeYUVName $BitstreamName $OutputYUVName $LogFile
run_RegularizeYUVName()
{
    if [ ! $# -eq 3 ]
    then
        echo "usage: run_RegularizeYUVName  \$BitstreamName  \$OutputYUVName \$LogFile "
        return 1
    fi
    local BitStreamName=$1
    local OrignName=$2
    local LogFile=$3
    local RegularizedYUVName=""
    declare -a aDecodedYUVInfo
    aDecodedYUVInfo=(`run_ParseDecoderLog  ${LogFile}`)
    BitStreamName=`echo ${BitStreamName} | awk 'BEGIN {FS="/"} {print $NF}'`
    RegularizedYUVName="${BitStreamName}_${aDecodedYUVInfo[0]}x${aDecodedYUVInfo[1]}.yuv"
    mv -f  ${OrignName}   ${RegularizedYUVName}
    echo ""
    echo "file: ${OrignName}   has been renamed as: ${RegularizedYUVName}"
    echo ""
    return 0
}

#usage: runMain    ${BitStreamName}
runMain()
{
    if [ ! $# -eq 1 ]
    then
        echo "usage: runMain  \${BitStreamName} "
        return 1
    fi
    local BitStreameFile=$1
    local BitSteamName=`echo ${BitStreameFile} | awk 'BEGIN {FS="/"} {print $NF}'`
    local DecoderLogFile="${BitSteamName}_h264dec.log"
    local DecodedYUVName="${BitSteamName}_dec.yuv"
    local RegularizedName=""

    #**********************
    #decoded test bit stream
    run_BitStream2YUV  ${BitStreameFile}  ${DecodedYUVName}  ${DecoderLogFile}
    if [  ! $?  -eq 0  ]
    then
        echo "bit stream decoded  failed!"
        return 1
    fi
    #*********************
    #regularized  YUV name
    run_RegularizeYUVName  ${BitStreameFile}  ${DecodedYUVName}  ${DecoderLogFile}
    return 0
}
BitStreamFile=$1
runMain    ${BitStreamFile}

