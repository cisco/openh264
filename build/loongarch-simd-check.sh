#!/bin/bash
# Copyright (c) 2021 Loongson Technology Corporation Limited
# Contributed by Xiwei Gu <guxiwei-hf@loongson.cn>
# Contributed by Lu Wang  <wanglu@loongson.cn>
#
#***************************************************************************************
#    This script is used in build/arch.mk for loongarch to detect the simd instructions:
#    lsx, lasx (maybe more in the future).
#
#   --usage:
#             ./loongarch-simd-check.sh lsx
#         or  ./loongarch-simd-check.sh lasx
#
# date:  11/23/2021 Created
#***************************************************************************************

TMPC=$(mktemp tmp.XXXXXX.c)
if [ $1 == "lsx" ]
then
   echo "void main(void){ __asm__ volatile(\"vadd.b \$vr0, \$vr1, \$vr1\"); }" > $TMPC
   if make -f /dev/null "${TMPC/.c/.o}"
   then
      echo "Yes"
   fi
elif [ $1 == "lasx" ]
then
   echo "void main(void){ __asm__ volatile(\"xvadd.b \$xr0, \$xr1, \$xr1\"); }" > $TMPC
   if make -f /dev/null "${TMPC/.c/.o}"
   then
      echo "Yes"
   fi
fi
rm -f $TMPC
