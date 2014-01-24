#!/bin/sh
python build/mktargets.py --directory codec/decoder --library decoder --exclude StdAfx.cpp
python build/mktargets.py --directory codec/encoder --library encoder --exclude DllEntry.cpp
python build/mktargets.py --directory codec/common --library common
python build/mktargets.py --directory codec/processing --library processing

python build/mktargets.py --directory codec/console/dec --binary h264dec
python build/mktargets.py --directory codec/console/enc --binary h264enc
python build/mktargets.py --directory test --binary codec_unittest
python build/mktargets.py --directory gtest --library gtest --out build/gtest-targets.mk --cpp-suffix .cc --include gtest-all.cc
