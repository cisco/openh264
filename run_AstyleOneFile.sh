#!/bin/bash


runUsage() {
  echo "************************************************"
  echo "usage: "
  echo "     $0  \${filename} "
  echo "        $0  test.cpp"
  echo "    or  $0  /src/*.cpp "
  echo "************************************************"

}

runMain() {
  #copy from build/astyle.cfg
  StyleArgu="--style=google         \
             --indent=spaces=2      \
             --max-code-length=120  \
             --pad-oper             \
             --align-pointer=type   \
             --align-reference=type \
             --unpad-paren          \
             --pad-first-paren-out  \
             --lineend=linux        \
             --convert-tabs"

  echo "************************************************"
  echo "  File name is ${File}"
  echo "  StyleArgu is ${StyleArgu}"
  echo "************************************************"
  astyle ${StyleArgu} ${File}

  echo all files have been change to astyle

}

#************************************************
runUsage
File=$1

if [ ! -e ${File}  ]
then
  echo "file does not exit,please double check!"
  exit 1
fi

runMain
#************************************************




