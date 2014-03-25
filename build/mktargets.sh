#!/bin/sh
python build/mktargets.py --directory codec/decoder --library decoder
python build/mktargets.py --directory codec/encoder --library encoder --exclude DllEntry.cpp
python build/mktargets.py --directory codec/common --library common --exclude asm_inc.asm --exclude arm_arch_common_macro.S
python build/mktargets.py --directory codec/processing --library processing

python build/mktargets.py --directory codec/console/dec --binary h264dec
python build/mktargets.py --directory codec/console/enc --binary h264enc
python build/mktargets.py --directory test/encoder --prefix encoder_unittest
python build/mktargets.py --directory test/decoder --prefix decoder_unittest
python build/mktargets.py --directory test/api --prefix api_test
python build/mktargets.py --directory gtest --library gtest --out build/gtest-targets.mk --cpp-suffix .cc --include gtest-all.cc
