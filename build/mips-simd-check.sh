#!/bin/bash
#**********************************************************************************
#    This script is using in build/arch.mk for mips to detect the simd instructions:
#    mmi, msa (maybe more in the future).
#
#   --usage:
#             ./mips-simd-check.sh $(CC) mmi
#         or  ./mips-simd-check.sh $(CC) msa
#
# date:  10/17/2019 Created
#**********************************************************************************

TMPC=$(mktemp tmp.XXXXXX.c)
TMPO=$(mktemp tmp.XXXXXX.o)
if [ $2 == "mmi" ]
then
   echo "void main(void){ __asm__ volatile(\"punpcklhw \$f0, \$f0, \$f0\"); }" > $TMPC
   $1 -Wa,-mloongson-mmi $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
elif [ $2 == "msa" ]
then
   echo "void main(void){ __asm__ volatile(\"addvi.b \$w0, \$w1, 1\"); }" > $TMPC
   $1 -mmsa $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
fi
rm -f $TMPC $TMPO
