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
echo "encoder cleaning....."
cd ${EncoderMakeDir}
make clean

#************************************************
#call Decoder make file
echo "decoder cleaning....."
cd ${CurDir}
cd ${DecoderMakeDir}
make clean

#************************************************
#call VP make file
echo "VP cleaning....."
cd ${CurDir}
cd ${VPMakeDir}
make clean

cd ${CurDir}
rm -f *.exe *.so *.a *.log ../bin
