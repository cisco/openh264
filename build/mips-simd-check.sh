#!/bin/bash
#**********************************************************************************
#    This script is using in build/arch.mk for mips to detect the simd instructions:
#    mmi, msa (maybe more in the future).
#
#   --usage:
#             ./mips-simd-check.sh mmi
#         or  ./mips-simd-check.sh msa
#
# date:  10/17/2019 Created
#**********************************************************************************

TMPC=$(mktemp tmp.XXXXXX.c)
if [ $1 == "mmi" ]
then
   echo "void main(void){ __asm__ volatile(\"punpcklhw \$f0, \$f0, \$f0\"); }" > $TMPC
   if make -f /dev/null "${TMPC/.c/.o}" &> /dev/null
   then
      echo "Yes"
   fi
elif [ $1 == "msa" ]
then
   echo "void main(void){ __asm__ volatile(\"addvi.b \$w0, \$w1, 1\"); }" > $TMPC
   if make -f /dev/null "${TMPC/.c/.o}" &> /dev/null
   then
      echo "Yes"
   fi
fi
rm -f $TMPC
