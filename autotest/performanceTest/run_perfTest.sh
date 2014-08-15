#!bin/bash
IOS=1
ANDROID=1
ENC=1
DEC=1
AUTO_TEST_PATH=`pwd`

#Judge to run the test on which kind of mobile
if [ $# -eq 0 ];then
echo Default testing will run on android and ios devices meanwhile
else
for params in $*; do
if [ $params = "ios" ];then
  echo Running the test just on ios devices
  ANDROID=0
elif [ $params = "android" ];then
  echo Running the test just on android devices
  IOS=0
elif [ $params = "enc" ];then
  echo Running the encoder performance test
  DEC=0
elif [ $params = "dec" ];then
  echo Running the decoder performance test
  ENC=0
else
  echo parameters are illegal!!!, ${0} [ios/android] [enc/dec]
  exit 1
fi
done
fi

#Prepare encoder resources
if [ ${ENC} = "1" ]
then
if [ ! -d ./EncoderPerTestRes ]
then
mkdir -p ./EncoderPerfTestRes
fi
if [ "#`ls ./EncoderPerfTestRes`" = "#" ]
then
echo put yuv and cfg file into ./EncoderPerfTest folder as
echo case_720p
echo case_720p/welsenc.cfg
echo case_720p/layer2.cfg
echo case_720p/yuv
echo case_720p/yuv/xxx1.yuv
echo case_720p/yuv/xxx2.yuv
echo case_360p
echo case_360p/welsenc.cfg
echo ......
else
#Run the encoder performance test
if [ ${IOS} = "1" ]
then
echo xxxxxxxxxxxxxxxxIOS ENC Startxxxxxxxxxxxxxxxxxx
echo Run the Encoder performance test on ios devices
cd ./ios
bash run_AutoTest_ios.sh enc
cd ${AUTO_TEST_PATH}
fi

if [ ${ANDROID} = "1" ]
then
echo xxxxxxxxxxxxxxAndroid ENC Startxxxxxxxxxxxxxxxxxxxx
echo Run the Encoder performance test on android devices
cd ./android
bash run_AutoTest_android.sh enc
cd ${AUTO_TEST_PATH}
fi
fi
fi

#Prepare decoder resources
if [ ${DEC} = "1" ]
then
if [ ! -d ./DecoderPerfTestRes ]
then
mkdir -p ./DecoderPerfTestRes
fi

if [ "#`ls ./DecoderPerfTestRes`" = "#" ]
then
echo put decoded bitstreams into such folder as
echo xxx1.264
echo xxx2.264
echo ........
else
#Run the decoder performance test
if [ ${IOS} = "1" ]
then
echo xxxxxxxxxxxxxxxxIOS DEC Startxxxxxxxxxxxxxxxxxx
echo Run the Decoder performance test on ios devices
cd ./ios
bash run_AutoTest_ios.sh dec
cd ${AUTO_TEST_PATH}
fi

if [ ${ANDROID} = "1" ]
then
echo xxxxxxxxxxxxxxAndroid DEC Startxxxxxxxxxxxxxxxxxxxx
echo Run the Decoder performance test on android devices
cd ./android
bash run_AutoTest_android.sh dec
cd ${AUTO_TEST_PATH}
fi
fi
fi

#TODO:NOW just generate csv file to display performance data
cd ${AUTO_TEST_PATH}
if [[ "#`ls ./ios/report`" == "#" && "#`ls ./android/report`" == "#" ]]
then
echo There is nothing result log generated at ios or android devices
else
echo Start to generate test result csv file
#Test result
mkdir -p ./TestResultCSV
bash parsePerfData.sh
echo The csv file locate ./TestResultCSV/xxx.csv
fi


