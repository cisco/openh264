#!/bin/bash
#*******************************************************************************
# Encoder binary comparison test model
#           -- Compared with benchmark version using SHA-1 string
#           -- Test bit stream under folder  openh264/res
#           -- SHA-1 string of benchmark version for  all cases  of all bit streams
#               under folder  openh264/test/encoder_binary_comparion/SHA1Table
#           -- For more detail,please refer to file AboutTest.
#
#brief:
#           -- Prepare test space for all test bit streams
#              1) update codec
#              2) update configure files
#              3) create test space for all test bit streams
#           -- Usage: run_PrepareAllTestFolder.sh \
#                           $AllTestDataFolder    \
#                           $TestBitStreamFolder  \
#                           $CodecFolder          \
#                           $ScriptFolder         \
#                           $SHA1TableFolder#
#          -- WorkingDir=openh264/test/encoder_binary_comparison
#           -- Before using this script, need to
#                 cd  $WorkingDir
#
# date:    10/06/2014 Created
#*******************************************************************************

#build codec
runBuildCodec()
{
    local MakeFileDir="../.."
    local CurrentDir=`pwd`

    echo "bit type is ${BitType}"
    #***************************
    #build codec
    cd ${MakeFileDir}
    if [ "${BitType}" -eq 64   ]
    then
        make clean
        make -B ENABLE64BIT=Yes h264dec h264enc
    elif [ "${BitType}" -eq 32   ]
    then
        make clean
        make -B ENABLE64BIT=No h264dec h264enc
    else
        echo "usage: runBuildCodec  \64/32  #bits"
        exit 1
    fi

    cd ${CurrentDir}

    echo ""
    if [ ! -e ${MakeFileDir}/h264enc  ]
    then
        echo "h264 Encoder build failed"
        return 1
    elif [ ! -e ${MakeFileDir}/h264dec  ]
    then
        echo "h264 Decoder build failed"
        return 1
    else
        echo "codec build succeed!"
        return 0
    fi

}


#copy codec related files to TestSpace's codec folder
runCopyFiles()
{
    local MakeFileDir="../.."
    local ConfigureFileDir="../../testbin"

    #copy codec and configure files
    cp  -p  ${MakeFileDir}/h264enc    ${CodecFolder}/
    cp  -p  ${MakeFileDir}/h264dec    ${CodecFolder}/
    cp  -p  ${ConfigureFileDir}/layer2.cfg     ${CodecFolder}/layer0.cfg
    cp  -p  ${ConfigureFileDir}/layer2.cfg     ${CodecFolder}/layer1.cfg
    cp  -p  ${ConfigureFileDir}/layer2.cfg     ${CodecFolder}/layer2.cfg
    cp  -p  ${ConfigureFileDir}/layer2.cfg     ${CodecFolder}/layer3.cfg
    cp  -p  ${ConfigureFileDir}/welsenc.cfg    ${CodecFolder}/welsenc.cfg
    return 0
}

runPrepareAllFolder()
{
    local SHA1TableName=""
    local SubFolder=""
    local IssueFolder="issue"
    local TempDataFolder="TempData"
    local ResultFolder="result"

    if [ -d $AllTestDataFolder ]
    then
        ./${ScriptFolder}/run_SafeDelete.sh  $AllTestDataFolder
    fi

    for Bitsream in ${SHA1TableFolder}/*.csv
    do
        StreamName=`echo ${Bitsream} | awk 'BEGIN {FS="/"}  {print $NF}   ' `
        StreamName=`echo ${StreamName} | awk 'BEGIN {FS=".264"}  {print $1}   ' `
        StreamName="${StreamName}.264"
        SubFolder="${AllTestDataFolder}/${StreamName}"
        SHA1TableName="${StreamName}_AllCases_SHA1_Table.csv"
        echo "BitSream is ${StreamName}"
        echo "sub folder is  ${SubFolder}"
        echo ""
        mkdir -p ${SubFolder}
        mkdir -p ${SubFolder}/${IssueFolder}
        mkdir -p ${SubFolder}/${TempDataFolder}
        mkdir -p ${SubFolder}/${ResultFolder}
        cp  ${CodecFolder}/*   ${SubFolder}
        cp  ${ScriptFolder}/*   ${SubFolder}
        if [ -e ${SHA1TableFolder}/${SHA1TableName}  ]
        then
          cp  ${SHA1TableFolder}/${SHA1TableName}   ${SubFolder}
        fi

    done

}
#usage: run_PrepareAllTestFolder.sh     ${BitType}
runMain()
{
    #parameter check!
    if [ ! $# -eq 1  ]
    then
        echo "usage: run_PrepareAllTestFolder.sh   \${BitType}"
        exit 1
    fi

    BitType=$1
    AllTestDataFolder="./AllTestData"
    CodecFolder="./Codec"
    ScriptFolder="./Scripts"
    SHA1TableFolder="./SHA1Table"

    if [ -d ./Codec ]
    then
        ./Scripts/run_SafeDelete.sh ./Codec
    fi

    if [ -d ./FinalResult  ]
    then
        ./Scripts/run_SafeDelete.sh ./FinalResult
    fi
    mkdir Codec
    mkdir FinalResult

    echo ""
    echo "building codec.........."
    echo "`gcc --version`"
    runBuildCodec  ${BitType}
    if [ ! $? -eq 0 ]
    then
        echo "codec build failed ..."
        exit 1
    fi

    echo ""
    runCopyFiles
    echo ""
    echo "preparing All test data folders...."
    echo ""
    runPrepareAllFolder   $AllTestDataFolder  $CodecFolder  $ScriptFolder  $SHA1TableFolder
    echo ""

    return 0
}

BitType=$1
runMain     ${BitType}

