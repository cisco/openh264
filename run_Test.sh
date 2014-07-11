
#!/bin/bash


#usage:runMain  ${TestType}  ${TestBitStream}
runMain()
{
  local ParameterFlag=""
  if [  $# -eq 1 -a   "$1" = "UnitTest" ]
  then
    let "ParameterFlag=0"
  elif [  $# -eq 2 -a   "$1" = "BinaryCompare" ]
  then
    let "ParameterFlag=0"
  else
    let "ParameterFlag=1"
  fi

  if [  ! ${ParameterFlag}  -eq 0  ]
  then
    echo "usage:     runMain UnitTest  "
    echo "       or  runMain BinaryCompare  \${TestBitStreamName}  "
    exit 1
  fi


  local TestType=$1
  local TestBitStream=$2

  if [ "${TestType}"  = "UnitTest"  ]
  then
    set -e
    make -B ENABLE64BIT=Yes BUILDTYPE=Release all plugin test
    make -B ENABLE64BIT=Yes BUILDTYPE=Debug all plugin test
    make -B ENABLE64BIT=No BUILDTYPE=Release all plugin test
    make -B ENABLE64BIT=No BUILDTYPE=Debug all plugin test
  elif [  "${TestType}"  = "BinaryCompare"  ]
  then
    echo ""
    echo " binary compare test, test bit stream is ${TestBitStream}"
    echo ""
    ./test/encoder_binary_comparison/run_OneBitStream.sh  ${TestBitStream}
  fi

}

TestType=$1
TestBitStream=$2

runMain  ${TestType}  ${TestBitStream}





