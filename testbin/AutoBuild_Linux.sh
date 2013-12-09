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
echo "encoder building....."
cd ${EncoderMakeDir}
make clean >${MakefileLogFile}
make >>${MakefileLogFile}

cd ${CurDir}
cd ${CodecBinDir}
if [ !  -e welsenc.a  ]
then
	let "EncoderBuildFlag=0"
fi

if [ !  -e welsenc.so  ]
then
	let "EncoderBuildFlag=0"
fi

if [ !  -e welsenc.exe  ]
then
	let "EncoderBuildFlag=0"
fi

if [  "$EncoderBuildFlag" -eq 1  ]
then
	echo "encoder build success!"
else
	echo "encoder build failed!"
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
if [ !  -e welsdec.a  ]
then
	let "DecoderBuildFlag=0"
fi

if [ !  -e welsdec.so  ]
then
	let "DecoderBuildFlag=0"
fi

if [ !  -e welsdec.exe  ]
then
	let "DecoderBuildFlag=0"
fi

if [  "$DecoderBuildFlag" -eq 1  ]
then
	echo "decoder build success!"
else
	echo "decoder build failed!"
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
if [ !  -e libwelsvp.so  ]
then
	let "VPBuildFlag=0"
fi

if [  "$VPBuildFlag" -eq 1  ]
then
	echo "VP build success!"
else
	echo "VP  build failed!"
fi


#****************************
cd ${CurDir}
rm -f *.a *.exe *.so

for file in ${CodecBinDir}/*
do
	cp ${file}  ./
	echo "file ${file}   under  ../openh264/bin/"
done

for file in ${VPBinDir}/*
do
	cp ${file}  ./
	echo "file ${file}   under  ../openh264/bin/"
done









