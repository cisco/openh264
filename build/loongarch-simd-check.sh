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
#             ./loongarch-simd-check.sh $(CC) lsx
#         or  ./loongarch-simd-check.sh $(CC) lasx
#
# date:  11/23/2021 Created
#***************************************************************************************

TMPC=$(mktemp tmp.XXXXXX.c)
TMPO=$(mktemp tmp.XXXXXX.o)
if [ $2 == "lsx" ]
then
   echo "void main(void){ __asm__ volatile(\"vadd.b \$vr0, \$vr1, \$vr1\"); }" > $TMPC
   $1 -mlsx $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
elif [ $2 == "lasx" ]
then
   echo "void main(void){ __asm__ volatile(\"xvadd.b \$xr0, \$xr1, \$xr1\"); }" > $TMPC
   $1 -mlasx $TMPC -o $TMPO &> /dev/null
   if test -s $TMPO
   then
      echo "Yes"
   fi
fi
rm -f $TMPC $TMPO
