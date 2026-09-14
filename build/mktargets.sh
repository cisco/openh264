#!/bin/sh
cd "$(git rev-parse --show-toplevel 2>/dev/null)" >/dev/null 2>&1
python2 build/mktargets.py --directory codec/decoder --library decoder
python2 build/mktargets.py --directory codec/encoder --library encoder --exclude DllEntry.cpp
python2 build/mktargets.py --directory codec/common --library common --exclude asm_inc.asm --exclude arm_arch_common_macro.S --exclude arm_arch64_common_macro.S
python2 build/mktargets.py --directory codec/processing --library processing

python2 build/mktargets.py --directory codec/console/dec --binary h264dec
python2 build/mktargets.py --directory codec/console/enc --binary h264enc
python2 build/mktargets.py --directory codec/console/common --library console_common
python2 build/mktargets.py --directory test/encoder --prefix encoder_unittest
python2 build/mktargets.py --directory test/decoder --prefix decoder_unittest
python2 build/mktargets.py --directory test/processing --prefix processing_unittest
python2 build/mktargets.py --directory test/api --prefix api_test
python2 build/mktargets.py --directory test/common --prefix common_unittest
python2 build/mktargets.py --directory module --prefix module
python2 build/mktargets.py --directory gtest/googletest --library gtest --out build/gtest-targets.mk --cpp-suffix .cc --include gtest-all.cc
