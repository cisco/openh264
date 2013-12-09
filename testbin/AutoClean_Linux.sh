#!/bin/bash

usage="this file must located in ../testbin/ based on our code structure  "
echo $usage

CurDir=`pwd`
EncoderMakeDir="../codec/build/linux/enc/"
DecoderMakeDir="../codec/build/linux/dec/"
VPMakeDir="../processing/build/linux/"

CodecBinDir="../codec/build/linux/bin/"
VPBinDir="../bin/linux"

let "EncoderBuildFlag=1"
let "DecoderBuildFlag=1"
let "VPBuildFlag=1"
MakefileLogFile="${CurDir}/CodecVPBuild.log"

#************************************************
#call Encoder make file
echo "encoder cleanning....."
cd ${EncoderMakeDir}
make clean >${MakefileLogFile}
#make >>${MakefileLogFile}



#************************************************
#call Decoder make file
echo "decoder cleanning....."
cd ${CurDir}
cd ${DecoderMakeDir}
make clean >>${MakefileLogFile}
#make >>${MakefileLogFile}



#************************************************
#call VP make file
echo "VP cleanning....."
cd ${CurDir}
cd ${VPMakeDir}
make clean >>${MakefileLogFile}
#make >>${MakefileLogFile}

cd ${CurDir}

rm -f *.exe *.so *.a *.log
rm -fr ../bin  # remove the bin directory




