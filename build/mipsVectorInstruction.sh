#!/bin/bash
#-------------------------------------------------------------
# \file    mipsVectorInstruction.sh
#
# \brief   Detect the vector instruction set (mmi, msa, etc.)
#          supported by the compiler for mips platform.
#
# \date    06/06/2019 Created
#-------------------------------------------------------------

TMPC=$(mktemp test.XXXXXX.c)
TMPO=$(mktemp test.XXXXXX.o)
if [ $1 == "mmi" ]
then
   echo "void main(void){ __asm__ volatile(\"punpcklhw \$f0, \$f0, \$f0\"); }" > $TMPC
   gcc $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
elif [ $1 == "msa" ]
then
   echo "void main(void){ __asm__ volatile(\"addvi.b \$w0, \$w1, 1\"); }" > $TMPC
   gcc -mmsa $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
fi
rm -f $TMPC $TMPO
