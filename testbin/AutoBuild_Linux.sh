#!/bin/bash

CurDir="${PWD}"

if [ "$(basename ${CurDir})" != "testbin" ]; then
    usage="This file must located in ../testbin/ based on our code structure"
    echo ${usage}
    exit 1
fi

EncoderMakeDir="../codec/build/linux/enc"
DecoderMakeDir="../codec/build/linux/dec"
VPMakeDir="../processing/build/linux"

CodecBinDir="../codec/build/linux/bin"
VPBinDir="../bin/linux"

MakefileLogFile="${CurDir}/CodecVPBuild.log"

#************************************************
#call Encoder make file
echo "encoder building....."
cd ${EncoderMakeDir}
make clean >${MakefileLogFile}
make >>${MakefileLogFile}

cd ${CurDir}
cd ${CodecBinDir}
if [[ ! -e welsenc.a ]] || [[ ! -e welsenc.so ]] || [[ ! -e welsenc.exe ]]; then
    echo "encoder build failed!"
else
    echo "encoder build success!"
fi

#************************************************
#call Decoder make file
echo "decoder building....."
cd ${CurDir}
cd ${DecoderMakeDir}
make clean >>${MakefileLogFile}
make >>${MakefileLogFile}

cd ${CurDir}
cd ${CodecBinDir}
if [[ ! -e welsdec.a ]] || [[ ! -e welsdec.so ]] || [[ ! -e welsdec.exe ]]; then
    echo "decoder build failed!"
else
    echo "decoder build success!"
fi

#************************************************
#call VP make file
echo "VP building....."
cd ${CurDir}
cd ${VPMakeDir}
make clean >>${MakefileLogFile}
make >>${MakefileLogFile}

cd ${CurDir}
cd ${VPBinDir}
if [ ! -e libwelsvp.so ]; then
    echo "VP build failed!"
else
    echo "VP build success!"
fi

cd ${CurDir}
echo "executables available in ../bin/linux"
echo "log file stored in ./CodecVPBuild.log"
