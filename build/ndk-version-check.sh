#!/bin/bash
#**********************************************************************************
#    This script is using in Makefile to check the ndk version:
#
#   --usage:
#             ./ndk-version-check.sh ndkroot
#
# date:  10/20/2023 Created
#**********************************************************************************

NDK_PATH=$1
if [ ! -n "$NDK_PATH" ]
then
    exit 1
fi
NDK_VERSION=${NDK_PATH##*/}
NDK_VERSION_NUM=`echo $NDK_VERSION | tr -cd "[0-9]"`

if [ $NDK_VERSION_NUM -le 18 ]
then
    echo "Yes"
fi
