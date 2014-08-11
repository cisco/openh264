/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "wels_common_defs.h"

namespace WelsCommon {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////pNonZeroCount[16+8] mapping scan index
const uint8_t g_kuiMbCountScan4Idx[24] = {
  //  0   1 | 4  5      luma 8*8 block           pNonZeroCount[16+8]
  0,  1,  4,  5,   //  2   3 | 6  7        0  |  1                  0   1   2   3
  2,  3,  6,  7,   //---------------      ---------                 4   5   6   7
  8,  9, 12, 13,   //  8   9 | 12 13       2  |  3                  8   9  10  11
  10, 11, 14, 15,   // 10  11 | 14 15-----------------------------> 12  13  14  15
  16, 17, 20, 21,   //----------------    chroma 8*8 block          16  17  18  19
  18, 19, 22, 23   // 16  17 | 20 21        0    1                 20  21  22  23
};

const uint8_t g_kuiCache48CountScan4Idx[24] = {
  /* Luma */
  9, 10, 17, 18,	// 1+1*8, 2+1*8, 1+2*8, 2+2*8,
  11, 12, 19, 20,	// 3+1*8, 4+1*8, 3+2*8, 4+2*8,
  25, 26, 33, 34,	// 1+3*8, 2+3*8, 1+4*8, 2+4*8,
  27, 28, 35, 36,	// 3+3*8, 4+3*8, 3+4*8, 4+4*8,
  /* Cb */
  14, 15,			// 6+1*8, 7+1*8,
  22, 23,			// 6+2*8, 7+2*8,

  /* Cr */
  38, 39,			// 6+4*8, 7+4*8,
  46, 47,			// 6+5*8, 7+5*8,
};


//cache element equal to 30
const uint8_t g_kuiCache30ScanIdx[16] = { //mv or uiRefIndex cache scan index, 4*4 block as basic unit
  7,  8, 13, 14,
  9, 10, 15, 16,
  19, 20, 25, 26,
  21, 22, 27, 28
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// extern at wels_common_defs.h
const uint8_t g_kuiChromaQpTable[52] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
  37, 38, 38, 38, 39, 39, 39, 39
};

/*
 *	vcl type map for given NAL unit type and corresponding H264 type (0: AVC; 1: SVC).
 */
const EVclType g_keTypeMap[32][2] = {
  { NON_VCL,	NON_VCL },	// 0: NAL_UNIT_UNSPEC_0
  { VCL,		VCL,	},	// 1: NAL_UNIT_CODED_SLICE
  { VCL,		NOT_APP },	// 2: NAL_UNIT_CODED_SLICE_DPA
  { VCL,		NOT_APP },	// 3: NAL_UNIT_CODED_SLICE_DPB
  { VCL,		NOT_APP },	// 4: NAL_UNIT_CODED_SLICE_DPC
  { VCL,		VCL		},	// 5: NAL_UNIT_CODED_SLICE_IDR
  { NON_VCL,	NON_VCL },	// 6: NAL_UNIT_SEI
  { NON_VCL,	NON_VCL },	// 7: NAL_UNIT_SPS
  { NON_VCL,	NON_VCL },	// 8: NAL_UNIT_PPS
  { NON_VCL,	NON_VCL },	// 9: NAL_UNIT_AU_DELIMITER
  { NON_VCL,	NON_VCL },	// 10: NAL_UNIT_END_OF_SEQ
  { NON_VCL,	NON_VCL },	// 11: NAL_UNIT_END_OF_STR
  { NON_VCL,	NON_VCL	},	// 12: NAL_UNIT_FILLER_DATA
  { NON_VCL,	NON_VCL },	// 13: NAL_UNIT_SPS_EXT
  { NON_VCL,	NON_VCL },	// 14: NAL_UNIT_PREFIX, NEED associate succeeded NAL to make a VCL
  { NON_VCL,	NON_VCL },	// 15: NAL_UNIT_SUBSET_SPS
  { NON_VCL,	NON_VCL },	// 16: NAL_UNIT_RESV_16
  { NON_VCL,	NON_VCL },	// 17: NAL_UNIT_RESV_17
  { NON_VCL,	NON_VCL },	// 18: NAL_UNIT_RESV_18
  { NON_VCL,	NON_VCL },	// 19: NAL_UNIT_AUX_CODED_SLICE
  { NON_VCL,	VCL		},	// 20: NAL_UNIT_CODED_SLICE_EXT
  { NON_VCL,	NON_VCL },	// 21: NAL_UNIT_RESV_21
  { NON_VCL,	NON_VCL },	// 22: NAL_UNIT_RESV_22
  { NON_VCL,	NON_VCL },	// 23: NAL_UNIT_RESV_23
  { NON_VCL,	NON_VCL },	// 24: NAL_UNIT_UNSPEC_24
  { NON_VCL,	NON_VCL },	// 25: NAL_UNIT_UNSPEC_25
  { NON_VCL,	NON_VCL },	// 26: NAL_UNIT_UNSPEC_26
  { NON_VCL,	NON_VCL	},	// 27: NAL_UNIT_UNSPEC_27
  { NON_VCL,	NON_VCL },	// 28: NAL_UNIT_UNSPEC_28
  { NON_VCL,	NON_VCL },	// 29: NAL_UNIT_UNSPEC_29
  { NON_VCL,	NON_VCL },	// 30: NAL_UNIT_UNSPEC_30
  { NON_VCL,	NON_VCL }	// 31: NAL_UNIT_UNSPEC_31
};

ALIGNED_DECLARE (const uint16_t, g_kuiDequantCoeff[52][8], 16) = {
  /* 0*/{   10,   13,   10,   13,   13,   16,   13,   16 },	/* 1*/{   11,   14,   11,   14,   14,   18,   14,   18 },
  /* 2*/{   13,   16,   13,   16,   16,   20,   16,   20 },	/* 3*/{   14,   18,   14,   18,   18,   23,   18,   23 },
  /* 4*/{   16,   20,   16,   20,   20,   25,   20,   25 },	/* 5*/{   18,   23,   18,   23,   23,   29,   23,   29 },
  /* 6*/{   20,   26,   20,   26,   26,   32,   26,   32 },	/* 7*/{   22,   28,   22,   28,   28,   36,   28,   36 },
  /* 8*/{   26,   32,   26,   32,   32,   40,   32,   40 },	/* 9*/{   28,   36,   28,   36,   36,   46,   36,   46 },
  /*10*/{   32,   40,   32,   40,   40,   50,   40,   50 },	/*11*/{   36,   46,   36,   46,   46,   58,   46,   58 },
  /*12*/{   40,   52,   40,   52,   52,   64,   52,   64 },	/*13*/{   44,   56,   44,   56,   56,   72,   56,   72 },
  /*14*/{   52,   64,   52,   64,   64,   80,   64,   80 },	/*15*/{   56,   72,   56,   72,   72,   92,   72,   92 },
  /*16*/{   64,   80,   64,   80,   80,  100,   80,  100 },	/*17*/{   72,   92,   72,   92,   92,  116,   92,  116 },
  /*18*/{   80,  104,   80,  104,  104,  128,  104,  128 },	/*19*/{   88,  112,   88,  112,  112,  144,  112,  144 },
  /*20*/{  104,  128,  104,  128,  128,  160,  128,  160 },	/*21*/{  112,  144,  112,  144,  144,  184,  144,  184 },
  /*22*/{  128,  160,  128,  160,  160,  200,  160,  200 },	/*23*/{  144,  184,  144,  184,  184,  232,  184,  232 },
  /*24*/{  160,  208,  160,  208,  208,  256,  208,  256 },	/*25*/{  176,  224,  176,  224,  224,  288,  224,  288 },
  /*26*/{  208,  256,  208,  256,  256,  320,  256,  320 },	/*27*/{  224,  288,  224,  288,  288,  368,  288,  368 },
  /*28*/{  256,  320,  256,  320,  320,  400,  320,  400 },	/*29*/{  288,  368,  288,  368,  368,  464,  368,  464 },
  /*30*/{  320,  416,  320,  416,  416,  512,  416,  512 },	/*31*/{  352,  448,  352,  448,  448,  576,  448,  576 },
  /*32*/{  416,  512,  416,  512,  512,  640,  512,  640 },	/*33*/{  448,  576,  448,  576,  576,  736,  576,  736 },
  /*34*/{  512,  640,  512,  640,  640,  800,  640,  800 },	/*35*/{  576,  736,  576,  736,  736,  928,  736,  928 },
  /*36*/{  640,  832,  640,  832,  832, 1024,  832, 1024 },	/*37*/{  704,  896,  704,  896,  896, 1152,  896, 1152 },
  /*38*/{  832, 1024,  832, 1024, 1024, 1280, 1024, 1280 },	/*39*/{  896, 1152,  896, 1152, 1152, 1472, 1152, 1472 },
  /*40*/{ 1024, 1280, 1024, 1280, 1280, 1600, 1280, 1600 },	/*41*/{ 1152, 1472, 1152, 1472, 1472, 1856, 1472, 1856 },
  /*42*/{ 1280, 1664, 1280, 1664, 1664, 2048, 1664, 2048 },	/*43*/{ 1408, 1792, 1408, 1792, 1792, 2304, 1792, 2304 },
  /*44*/{ 1664, 2048, 1664, 2048, 2048, 2560, 2048, 2560 },	/*45*/{ 1792, 2304, 1792, 2304, 2304, 2944, 2304, 2944 },
  /*46*/{ 2048, 2560, 2048, 2560, 2560, 3200, 2560, 3200 },	/*47*/{ 2304, 2944, 2304, 2944, 2944, 3712, 2944, 3712 },
  /*48*/{ 2560, 3328, 2560, 3328, 3328, 4096, 3328, 4096 },	/*49*/{ 2816, 3584, 2816, 3584, 3584, 4608, 3584, 4608 },
  /*50*/{ 3328, 4096, 3328, 4096, 4096, 5120, 4096, 5120 },	/*51*/{ 3584, 4608, 3584, 4608, 4608, 5888, 4608, 5888 },
};

// table A-1 - Level limits
const SLevelLimits g_ksLevelLimits[LEVEL_NUMBER] = {
  {10, 1485, 99, 396, 64, 175, -256, 255, 2, 0x7fff}, /* level 1 */
  {9, 1485, 99, 396, 128, 350, -256, 255, 2, 0x7fff}, /* level 1.b */
  {11, 3000, 396, 900, 192, 500, -512, 511, 2, 0x7fff}, /* level 1.1 */
  {12, 6000, 396, 2376, 384, 1000, -512, 511, 2, 0x7fff}, /* level 1.2 */
  {13, 11880, 396, 2376, 768, 2000, -512, 511, 2, 0x7fff}, /* level 1.3 */

  {20, 11880, 396, 2376, 2000, 2000, -512, 511, 2, 0x7fff}, /* level 2 */
  {21, 19800, 792, 4752, 4000, 4000, -1024, 1023, 2, 0x7fff}, /* level 2.1 */
  {22, 20250, 1620, 8100, 4000, 4000, -1024, 1023, 2, 0x7fff}, /* level 2.2 */

  {30, 40500, 1620, 8100, 10000, 10000, -1024, 1023, 2, 32 }, /* level 3 */
  {31, 108000, 3600, 18000, 14000, 14000, -2048, 2047, 4, 16}, /* level 3.1 */
  {32, 216000, 5120, 20480, 20000, 20000, -2048, 2047, 4, 16}, /* level 3.2 */

  {40, 245760, 8192, 32768, 20000, 25000, -2048, 2047, 4, 16}, /* level 4 */
  {41, 245760, 8192, 32768, 50000, 62500, -2048, 2047, 2, 16}, /* level 4.1 */
  {42, 522240, 8704, 34816, 50000, 62500, -2048, 2047, 2, 16}, /* level 4.2 */

  {50, 589824, 22080, 110400, 135000, 135000, -2048, 2047, 2, 16}, /* level 5 */
  {51, 983040, 36864, 184320, 240000, 240000, -2048, 2047, 2, 16}, /* level 5.1 */
  {52, 2073600, 36864, 184320, 240000, 240000, -2048, 2047, 2, 16} /* level 5.2 */
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
