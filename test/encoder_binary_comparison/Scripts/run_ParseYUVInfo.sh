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
#             -- Parse sequence basic info such as picw pich and fps via name
#             -- Usage:  run_ParseYUVInfo.sh   ${TestSequenceName}
#
# date:    10/06/2014 Created
#*******************************************************************************
#usage runGetYUVInfo    $TestSequenceName
#eg. input:    ABC_1920X1080_30fps_XXX.yuv  output: 1920 1080 30
#eg. input:    ABC_1920X1080_XXX.yuv            output: 1920 1080 0
#eg. input:    ABC_XXX.yuv                      output: 0    0    0
runGetYUVInfo()
{
    if [ ! $# -eq 1  ]
    then
        echo "runGetYUVInfo  \$TestSequenceName"
        echo "detected by run_ParseYUVInfo.sh"
        return 1
    fi

    local SequenceName=$1
    local PicWidth="0"
    local PicHeight="0"
    local FPS="0"
    declare -a aPicInfo
    aPicInfo=(`echo ${SequenceName} | awk 'BEGIN {FS="[_.]"} {for(i=1;i<=NF;i++) printf("%s  ",$i)}'`)
    local Iterm
    local Index=""
    local Pattern_01="[xX]"
    local Pattern_02="^[1-9][0-9]"
    local Pattern_03="[0-9][0-9]$"
    local Pattern_04="fps$"
    #get PicW PicH info
    let "Index=0"
    for  Iterm in ${aPicInfo[@]}
    do
        if [[ $Iterm =~ $Pattern_01 ]] && [[ $Iterm =~ $Pattern_02 ]] && [[ $Iterm =~ $Pattern_03 ]]
        then
            PicWidth=`echo $Iterm | awk 'BEGIN {FS="[xX]"} {print $1}'`
            PicHeight=`echo $Iterm | awk 'BEGIN {FS="[xX]"} {print $2}'`
            break
        fi
        let "Index++"
    done
    #get fps info
    let "Index++"
    if [ $Index -le ${#aPicInfo[@]} ]
    then
        if [[ ${aPicInfo[$Index]} =~ ^[1-9]  ]] || [[ ${aPicInfo[$Index]} =~ $Pattern_04 ]]
        then
            FPS=`echo ${aPicInfo[$Index]} | awk 'BEGIN {FS="[a-zA-Z]" } {print $1} '`
        fi
    fi
    echo "$PicWidth $PicHeight $FPS"
}
TestSequenceName=$1
runGetYUVInfo    ${TestSequenceName}


