#!/bin/bash
#**********************************************************************************
#    This script is for test on travis.Currently there are 5 jobs running on
#    travis in parallel status which are listed as below:
#        1.Unit test with gcc compiler;
#        2.Unit test with clang compiler;
#        3.Binary comparison test for test bit stream A;
#        4.Binary comparison test for test bit stream B;
#        5.Binary comparison test for test bit stream C.
#    For binary comparison test,before running all test cases, it need to prepare
#    the test space.On travis,as those parallel jobs are running on different VMs,
#    so each job need to prepare for its test space for itself.
#
#   --usage:
#             ./runTest.sh  UnitTest
#         or  ./runTest.sh  BinaryCompare  ${TestBitStreamName}
#
# date:  10/06/2014 Created
#**********************************************************************************
#usage: runInputParamCheck  ${TestType}  ${TestBitStream}
runInputParamCheck()
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
  return ${ParameterFlag}
}
#usage: runUnitTest
runUnitTest()
{
  CFLAGS=-Werror make -B ENABLE64BIT=Yes BUILDTYPE=Release all plugin test
  CFLAGS=-Werror make -B ENABLE64BIT=Yes BUILDTYPE=Debug   all plugin test
  CFLAGS=-Werror make -B ENABLE64BIT=No  BUILDTYPE=Release all plugin test
  CFLAGS=-Werror make -B ENABLE64BIT=No  BUILDTYPE=Debug   all plugin test
  return $?
}
#usage: runPrepareAndBinaryTest $TestBitStream
runPrepareAndBinaryTest()
{
  if [ ! $# -eq 2  ]
  then
    echo "usage: runPrepareAndBinaryTest  \$TestBitStream"
    exit 1
  fi
  local TestBitStream=$1
  local TestType=$2
  local WorkingDir=`pwd`
  local BinaryTestDir="test/encoder_binary_comparison"
  local TestSpacePrepareLog="AllTestSpacePrepare.log"
  cd ${BinaryTestDir}
  ./run_PrepareAllTestData.sh 64 2>${TestSpacePrepareLog}
  cd ${WorkingDir}
  echo ""
  echo " binary compare test, test bit stream is ${TestBitStream}"
  echo ""
./test/encoder_binary_comparison/run_OneBitStream.sh  ${TestBitStream} ${TestType}
  return $?
}
#usage:runMain  ${TestType}  ${TestBitStream}
runMain()
{
  local TestType=$1
  local TestBitStream=$2
  runInputParamCheck  ${TestType}  ${TestBitStream}
  if [  ! $?  -eq 0  ]
  then
    echo "usage:     ./runTest.sh  UnitTest  \${PrepareFlag}"
    echo "       or  ./runTest.sh  BinaryCompare  \${TestBitStreamName} \${PrepareFlag} "
    exit 1
  fi
  if [ "${TestType}"  = "UnitTest"  ]
  then
    set -e
    runUnitTest
    return $?
  fi
  if [  "${TestType}"  = "BinaryCompare" ]
  then
    set -e
    runPrepareAndBinaryTest ${TestBitStream} TravisTest
    return $?
  fi
}
TestType=$1
TestBitStream=$2
runMain  ${TestType}  ${TestBitStream}

