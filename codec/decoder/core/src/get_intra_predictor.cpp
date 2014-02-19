/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
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
 *
 * \file	get_intra_predictor.c
 *
 * \brief	implementation for get intra predictor about 16x16, 4x4, chroma.
 *
 * \date	4/2/2009 Created
 *			9/14/2009 C level based optimization with high performance gained.
 *				[const, using ST32/ST64 to replace memset, memcpy and memmove etc.]
 *
 *************************************************************************************
 */
#include <string.h>

#include "macros.h"
#include "ls_defines.h"
#include "get_intra_predictor.h"

namespace WelsDec {

#define I4x4_COUNT 4
#define I8x8_COUNT 8
#define I16x16_COUNT 16

void WelsI4x4LumaPredV_c (uint8_t* pPred, const int32_t kiStride) {
  const uint32_t kuiVal = LD32 (pPred - kiStride);

  ST32 (pPred						    , kuiVal);
  ST32 (pPred + kiStride				, kuiVal);
  ST32 (pPred + (kiStride << 1)			, kuiVal);
  ST32 (pPred + (kiStride << 1) + kiStride	, kuiVal);
}

void WelsI4x4LumaPredH_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride2 + kiStride;
  const uint32_t kuiL0 = 0x01010101U * pPred[-1          ];
  const uint32_t kuiL1 = 0x01010101U * pPred[-1 + kiStride ];
  const uint32_t kuiL2 = 0x01010101U * pPred[-1 + kiStride2];
  const uint32_t kuiL3 = 0x01010101U * pPred[-1 + kiStride3];

  ST32 (pPred          , kuiL0);
  ST32 (pPred + kiStride , kuiL1);
  ST32 (pPred + kiStride2, kuiL2);
  ST32 (pPred + kiStride3, kuiL3);
}

void WelsI4x4LumaPredDc_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride2 + kiStride;
  const uint8_t kuiMean	= (pPred[-1] + pPred[-1 + kiStride] + pPred[-1 + kiStride2] + pPred[-1 + kiStride3] +
                           pPred[-kiStride] + pPred[-kiStride + 1] + pPred[-kiStride + 2] + pPred[-kiStride + 3] + 4) >> 3;
  const uint32_t kuiMean32 = 0x01010101U * kuiMean;

  ST32 (pPred          , kuiMean32);
  ST32 (pPred + kiStride , kuiMean32);
  ST32 (pPred + kiStride2, kuiMean32);
  ST32 (pPred + kiStride3, kuiMean32);
}

void WelsI4x4LumaPredDcLeft_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride2 + kiStride;
  const uint8_t kuiMean	= (pPred[-1] + pPred[-1 + kiStride] + pPred[-1 + kiStride2] + pPred[-1 + kiStride3] + 2) >> 2;
  const uint32_t kuiMean32 = 0x01010101U * kuiMean;

  ST32 (pPred          , kuiMean32);
  ST32 (pPred + kiStride , kuiMean32);
  ST32 (pPred + kiStride2, kuiMean32);
  ST32 (pPred + kiStride3, kuiMean32);
}

void WelsI4x4LumaPredDcTop_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride2 + kiStride;
  const uint8_t kuiMean	= (pPred[-kiStride] + pPred[-kiStride + 1] + pPred[-kiStride + 2] + pPred[-kiStride + 3] + 2) >>
                          2;
  const uint32_t kuiMean32 = 0x01010101U * kuiMean;

  ST32 (pPred          , kuiMean32);
  ST32 (pPred + kiStride , kuiMean32);
  ST32 (pPred + kiStride2, kuiMean32);
  ST32 (pPred + kiStride3, kuiMean32);
}

void WelsI4x4LumaPredDcNA_c (uint8_t* pPred, const int32_t kiStride) {
  const uint32_t kuiDC32		= 0x80808080U;

  ST32 (pPred                       , kuiDC32);
  ST32 (pPred + kiStride              , kuiDC32);
  ST32 (pPred + (kiStride << 1)         , kuiDC32);
  ST32 (pPred + (kiStride << 1) + kiStride, kuiDC32);
}

/*down pLeft*/
void WelsI4x4LumaPredDDL_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
  /*get pTop*/
  uint8_t* ptop			= &pPred[-kiStride];
  const uint8_t kuiT0		= *ptop;
  const uint8_t kuiT1		= * (ptop + 1);
  const uint8_t kuiT2		= * (ptop + 2);
  const uint8_t kuiT3		= * (ptop + 3);
  const uint8_t kuiT4		= * (ptop + 4);
  const uint8_t kuiT5		= * (ptop + 5);
  const uint8_t kuiT6		= * (ptop + 6);
  const uint8_t kuiT7		= * (ptop + 7);
  const uint8_t kuiDDL0	= (2 + kuiT0 + kuiT2 + (kuiT1 << 1)) >> 2;	// kDDL0
  const uint8_t kuiDDL1	= (2 + kuiT1 + kuiT3 + (kuiT2 << 1)) >> 2;	// kDDL1
  const uint8_t kuiDDL2	= (2 + kuiT2 + kuiT4 + (kuiT3 << 1)) >> 2;	// kDDL2
  const uint8_t kuiDDL3	= (2 + kuiT3 + kuiT5 + (kuiT4 << 1)) >> 2;	// kDDL3
  const uint8_t kuiDDL4	= (2 + kuiT4 + kuiT6 + (kuiT5 << 1)) >> 2;	// kDDL4
  const uint8_t kuiDDL5	= (2 + kuiT5 + kuiT7 + (kuiT6 << 1)) >> 2;	// kDDL5
  const uint8_t kuiDDL6	= (2 + kuiT6 + kuiT7 + (kuiT7 << 1)) >> 2;	// kDDL6
  const uint8_t kuiList[8] = { kuiDDL0, kuiDDL1, kuiDDL2, kuiDDL3, kuiDDL4, kuiDDL5, kuiDDL6, 0 };

  ST32 (pPred          , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 1));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList + 3));
}

/*down pLeft*/
void WelsI4x4LumaPredDDLTop_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
  /*get pTop*/
  uint8_t* ptop			= &pPred[-kiStride];
  const uint8_t kuiT0		= *ptop;
  const uint8_t kuiT1		= * (ptop + 1);
  const uint8_t kuiT2		= * (ptop + 2);
  const uint8_t kuiT3		= * (ptop + 3);
  const uint16_t kuiT01	= 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12	= 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23	= 1 + kuiT2 + kuiT3;
  const uint16_t kuiT33	= 1 + (kuiT3 << 1);
  const uint8_t kuiDLT0	= (kuiT01 + kuiT12) >> 2;	// kDLT0
  const uint8_t kuiDLT1	= (kuiT12 + kuiT23) >> 2;	// kDLT1
  const uint8_t kuiDLT2	= (kuiT23 + kuiT33) >> 2;	// kDLT2
  const uint8_t kuiDLT3	= kuiT33 >> 1;			// kDLT3
  const uint8_t kuiList[8] = { kuiDLT0, kuiDLT1, kuiDLT2, kuiDLT3, kuiDLT3, kuiDLT3, kuiDLT3 , kuiDLT3 };

  ST32 (pPred,           LD32 (kuiList));
  ST32 (pPred + kiStride,  LD32 (kuiList + 1));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList + 3));
}


/*down right*/
void WelsI4x4LumaPredDDR_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
  uint8_t* ptopleft		= &pPred[- (kiStride + 1)];
  uint8_t* pleft			= &pPred[-1];
  const uint8_t kuiLT		= *ptopleft;
  /*get pLeft and pTop*/
  const uint8_t kuiL0		= *pleft;
  const uint8_t kuiL1		= * (pleft + kiStride);
  const uint8_t kuiL2		= * (pleft + kiStride2);
  const uint8_t kuiL3		= * (pleft + kiStride3);
  const uint8_t kuiT0		= * (ptopleft + 1);
  const uint8_t kuiT1		= * (ptopleft + 2);
  const uint8_t kuiT2		= * (ptopleft + 3);
  const uint8_t kuiT3		= * (ptopleft + 4);
  const uint16_t kuiTL0	= 1 + kuiLT + kuiL0;
  const uint16_t kuiLT0	= 1 + kuiLT + kuiT0;
  const uint16_t kuiT01	= 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12	= 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23	= 1 + kuiT2 + kuiT3;
  const uint16_t kuiL01	= 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12	= 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23	= 1 + kuiL2 + kuiL3;
  const uint8_t kuiDDR0	= (kuiTL0 + kuiLT0) >> 2;	// kuiDDR0
  const uint8_t kuiDDR1	= (kuiLT0 + kuiT01) >> 2;	// kuiDDR1
  const uint8_t kuiDDR2	= (kuiT01 + kuiT12) >> 2;	// kuiDDR2
  const uint8_t kuiDDR3	= (kuiT12 + kuiT23) >> 2;	// kuiDDR3
  const uint8_t kuiDDR4	= (kuiTL0 + kuiL01) >> 2;	// kuiDDR4
  const uint8_t kuiDDR5	= (kuiL01 + kuiL12) >> 2;	// kuiDDR5
  const uint8_t kuiDDR6	= (kuiL12 + kuiL23) >> 2;	// kuiDDR6
  const uint8_t kuiList[8] = { kuiDDR6, kuiDDR5, kuiDDR4, kuiDDR0, kuiDDR1, kuiDDR2, kuiDDR3, 0	};

  ST32 (pPred          , LD32 (kuiList + 3));
  ST32 (pPred + kiStride , LD32 (kuiList + 2));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList));
}


/*vertical pLeft*/
void WelsI4x4LumaPredVL_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
  uint8_t* ptopleft		= &pPred[- (kiStride + 1)];
  /*get pTop*/
  const uint8_t kuiT0		    = * (ptopleft + 1);
  const uint8_t kuiT1		    = * (ptopleft + 2);
  const uint8_t kuiT2		    = * (ptopleft + 3);
  const uint8_t kuiT3		    = * (ptopleft + 4);
  const uint8_t kuiT4		    = * (ptopleft + 5);
  const uint8_t kuiT5		    = * (ptopleft + 6);
  const uint8_t kuiT6		    = * (ptopleft + 7);
  const uint16_t kuiT01		= 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12		= 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23		= 1 + kuiT2 + kuiT3;
  const uint16_t kuiT34		= 1 + kuiT3 + kuiT4;
  const uint16_t kuiT45		= 1 + kuiT4 + kuiT5;
  const uint16_t kuiT56		= 1 + kuiT5 + kuiT6;
  const uint8_t kuiVL0		= kuiT01 >> 1;			// kuiVL0
  const uint8_t kuiVL1		= kuiT12 >> 1;			// kuiVL1
  const uint8_t kuiVL2		= kuiT23 >> 1;			// kuiVL2
  const uint8_t kuiVL3		= kuiT34 >> 1;			// kuiVL3
  const uint8_t kuiVL4		= kuiT45 >> 1;			// kuiVL4
  const uint8_t kuiVL5		= (kuiT01 + kuiT12) >> 2;	// kuiVL5
  const uint8_t kuiVL6		= (kuiT12 + kuiT23) >> 2;	// kuiVL6
  const uint8_t kuiVL7		= (kuiT23 + kuiT34) >> 2;	// kuiVL7
  const uint8_t kuiVL8		= (kuiT34 + kuiT45) >> 2;	// kuiVL8
  const uint8_t kuiVL9		= (kuiT45 + kuiT56) >> 2;	// kuiVL9
  const uint8_t kuiList[10]	= { kuiVL0, kuiVL1, kuiVL2, kuiVL3, kuiVL4, kuiVL5, kuiVL6, kuiVL7, kuiVL8, kuiVL9 };

  ST32 (pPred,           LD32 (kuiList));
  ST32 (pPred + kiStride,  LD32 (kuiList + 5));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}

/*vertical pLeft*/
void WelsI4x4LumaPredVLTop_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	    = kiStride << 1;
  const int32_t kiStride3	    = kiStride + kiStride2;
  uint8_t* ptopleft		    = &pPred[- (kiStride + 1)];
  /*get pTop*/
  const uint8_t kuiT0		    = * (ptopleft + 1);
  const uint8_t kuiT1		    = * (ptopleft + 2);
  const uint8_t kuiT2		    = * (ptopleft + 3);
  const uint8_t kuiT3		    = * (ptopleft + 4);
  const uint16_t kuiT01		= 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12		= 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23		= 1 + kuiT2 + kuiT3;
  const uint16_t kuiT33		= 1 + (kuiT3 << 1);
  const uint8_t kuiVL0		= kuiT01 >> 1;
  const uint8_t kuiVL1		= kuiT12 >> 1;
  const uint8_t kuiVL2		= kuiT23 >> 1;
  const uint8_t kuiVL3		= kuiT33 >> 1;
  const uint8_t kuiVL4		= (kuiT01 + kuiT12) >> 2;
  const uint8_t kuiVL5		= (kuiT12 + kuiT23) >> 2;
  const uint8_t kuiVL6		= (kuiT23 + kuiT33) >> 2;
  const uint8_t kuiVL7		= kuiVL3;
  const uint8_t kuiList[10]	= { kuiVL0, kuiVL1, kuiVL2, kuiVL3, kuiVL3, kuiVL4, kuiVL5, kuiVL6, kuiVL7, kuiVL7 };

  ST32 (pPred          , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 5));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}


/*vertical right*/
void WelsI4x4LumaPredVR_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	    = kiStride << 1;
  const int32_t kiStride3	    = kiStride + kiStride2;
  const uint8_t kuiLT		    = pPred[-kiStride - 1];
  /*get pLeft and pTop*/
  const uint8_t kuiL0		    = pPred[         -1];
  const uint8_t kuiL1		    = pPred[kiStride - 1];
  const uint8_t kuiL2		    = pPred[kiStride2 - 1];
  const uint8_t kuiT0		    = pPred[ -kiStride];
  const uint8_t kuiT1		    = pPred[1 - kiStride];
  const uint8_t kuiT2		    = pPred[2 - kiStride];
  const uint8_t kuiT3		    = pPred[3 - kiStride];
  const uint8_t kuiVR0		= (1 + kuiLT + kuiT0) >> 1;	// kuiVR0
  const uint8_t kuiVR1		= (1 + kuiT0 + kuiT1) >> 1;	// kuiVR1
  const uint8_t kuiVR2		= (1 + kuiT1 + kuiT2) >> 1;	// kuiVR2
  const uint8_t kuiVR3		= (1 + kuiT2 + kuiT3) >> 1;	// kuiVR3
  const uint8_t kuiVR4		= (2 + kuiL0 + (kuiLT << 1) + kuiT0) >> 2;	// kuiVR4
  const uint8_t kuiVR5		= (2 + kuiLT + (kuiT0 << 1) + kuiT1) >> 2;	// kuiVR5
  const uint8_t kuiVR6		= (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;	// kuiVR6
  const uint8_t kuiVR7		= (2 + kuiT1 + (kuiT2 << 1) + kuiT3) >> 2;	// kuiVR7
  const uint8_t kuiVR8		= (2 + kuiLT + (kuiL0 << 1) + kuiL1) >> 2;	// kuiVR8
  const uint8_t kuiVR9		= (2 + kuiL0 + (kuiL1 << 1) + kuiL2) >> 2;	// kuiVR9
  const uint8_t kuiList[10]	= { kuiVR8, kuiVR0, kuiVR1, kuiVR2, kuiVR3, kuiVR9, kuiVR4, kuiVR5, kuiVR6, kuiVR7 };

  ST32 (pPred          , LD32 (kuiList + 1));
  ST32 (pPred + kiStride , LD32 (kuiList + 6));
  ST32 (pPred + kiStride2, LD32 (kuiList));
  ST32 (pPred + kiStride3, LD32 (kuiList + 5));
}

/*horizontal up*/
void WelsI4x4LumaPredHU_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2	    = kiStride << 1;
  const int32_t kiStride3	    = kiStride + kiStride2;
  /*get pLeft*/
  const uint8_t kuiL0		    = pPred[         -1];
  const uint8_t kuiL1		    = pPred[kiStride - 1];
  const uint8_t kuiL2		    = pPred[kiStride2 - 1];
  const uint8_t kuiL3		    = pPred[kiStride3 - 1];
  const uint16_t kuiL01		= 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12		= 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23		= 1 + kuiL2 + kuiL3;
  const uint8_t kuiHU0		= kuiL01 >> 1;
  const uint8_t kuiHU1		= (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiHU2		= kuiL12 >> 1;
  const uint8_t kuiHU3		= (kuiL12 + kuiL23) >> 2;
  const uint8_t kuiHU4		= kuiL23 >> 1;
  const uint8_t kuiHU5		= (1 + kuiL23 + (kuiL3 << 1)) >> 2;
  const uint8_t kuiList[10]	= { kuiHU0, kuiHU1, kuiHU2, kuiHU3, kuiHU4, kuiHU5, kuiL3, kuiL3, kuiL3, kuiL3 };

  ST32 (pPred          , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 2));
  ST32 (pPred + kiStride2, LD32 (kuiList + 4));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}

/*horizontal down*/
void WelsI4x4LumaPredHD_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2 	= kiStride << 1;
  const int32_t kiStride3	    = kiStride + kiStride2;
  const uint8_t kuiLT		    = pPred[- (kiStride + 1)];
  /*get pLeft and pTop*/
  const uint8_t kuiL0		    = pPred[-1          ];
  const uint8_t kuiL1		    = pPred[-1 + kiStride ];
  const uint8_t kuiL2		    = pPred[-1 + kiStride2];
  const uint8_t kuiL3		    = pPred[-1 + kiStride3];
  const uint8_t kuiT0		    = pPred[-kiStride   ];
  const uint8_t kuiT1		    = pPred[-kiStride + 1 ];
  const uint8_t kuiT2		    = pPred[-kiStride + 2 ];
  const uint16_t kuiTL0		= 1 + kuiLT + kuiL0;
  const uint16_t kuiLT0		= 1 + kuiLT + kuiT0;
  const uint16_t kuiT01		= 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12		= 1 + kuiT1 + kuiT2;
  const uint16_t kuiL01		= 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12		= 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23		= 1 + kuiL2 + kuiL3;
  const uint8_t kuiHD0		= kuiTL0 >> 1;
  const uint8_t kuiHD1		= (kuiTL0 + kuiLT0) >> 2;
  const uint8_t kuiHD2		= (kuiLT0 + kuiT01) >> 2;
  const uint8_t kuiHD3		= (kuiT01 + kuiT12) >> 2;
  const uint8_t kuiHD4		= kuiL01 >> 1;
  const uint8_t kuiHD5		= (kuiTL0 + kuiL01) >> 2;
  const uint8_t kuiHD6		= kuiL12 >> 1;
  const uint8_t kuiHD7		= (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiHD8		= kuiL23 >> 1;
  const uint8_t kuiHD9	    = (kuiL12 + kuiL23) >> 2;
  const uint8_t kuiList[10]	= { kuiHD8, kuiHD9, kuiHD6, kuiHD7, kuiHD4, kuiHD5, kuiHD0, kuiHD1, kuiHD2, kuiHD3 };

  ST32 (pPred          , LD32 (kuiList + 6));
  ST32 (pPred + kiStride , LD32 (kuiList + 4));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList));
}

void WelsIChromaPredV_c (uint8_t* pPred, const int32_t kiStride) {
  const uint64_t kuiVal64	= LD64 (&pPred[-kiStride]);
  const int32_t kiStride2	= kiStride  << 1;
  const int32_t kiStride4 = kiStride2 << 1;

  ST64 (pPred                        , kuiVal64);
  ST64 (pPred + kiStride               , kuiVal64);
  ST64 (pPred + kiStride2              , kuiVal64);
  ST64 (pPred + kiStride2 + kiStride     , kuiVal64);
  ST64 (pPred + kiStride4              , kuiVal64);
  ST64 (pPred + kiStride4 + kiStride     , kuiVal64);
  ST64 (pPred + kiStride4 + kiStride2    , kuiVal64);
  ST64 (pPred + (kiStride << 3) - kiStride , kuiVal64);
}

void WelsIChromaPredH_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 3) - kiStride;
  uint8_t i = 7;

  do {
    const uint8_t kuiVal8	= pPred[iTmp - 1];
    const uint64_t kuiVal64	= 0x0101010101010101ULL * kuiVal8;

    ST64 (pPred + iTmp, kuiVal64);

    iTmp -= kiStride;
  } while (i-- > 0);
}


void WelsIChromaPredPlane_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t a = 0, b = 0, c = 0, H = 0, V = 0;
  int32_t i, j;
  uint8_t* pTop = &pPred[-kiStride];
  uint8_t* pLeft = &pPred[-1];

  for (i = 0 ; i < 4 ; i ++) {
    H += (i + 1) * (pTop[4 + i] - pTop[2 - i]);
    V += (i + 1) * (pLeft[ (4 + i) * kiStride] - pLeft[ (2 - i) * kiStride]);
  }

  a = (pLeft[7 * kiStride] + pTop[7]) << 4;
  b = (17 * H + 16) >> 5;
  c = (17 * V + 16) >> 5;

  for (i = 0 ; i < 8 ; i ++) {
    for (j = 0 ; j < 8 ; j ++) {
      int32_t iTmp = (a + b * (j - 3) + c * (i - 3) + 16) >> 5;
      iTmp = WELS_CLIP1 (iTmp);
      pPred[j] = iTmp;
    }
    pPred += kiStride;
  }
}


void WelsIChromaPredDc_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiL1		= kiStride - 1;
  const int32_t kiL2		= kiL1 + kiStride;
  const int32_t kiL3		= kiL2 + kiStride;
  const int32_t kiL4		= kiL3 + kiStride;
  const int32_t kiL5		= kiL4 + kiStride;
  const int32_t kiL6		= kiL5 + kiStride;
  const int32_t kiL7		= kiL6 + kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiM1		= (pPred[-kiStride] + pPred[1 - kiStride] + pPred[2 - kiStride] + pPred[3 - kiStride] +
                           pPred[-1] + pPred[kiL1] + pPred[kiL2] + pPred[kiL3] + 4) >> 3 ;
  const uint32_t kuiSum2	= pPred[4 - kiStride] + pPred[5 - kiStride] + pPred[6 - kiStride] + pPred[7 - kiStride];
  const uint32_t kuiSum3	= pPred[kiL4] + pPred[kiL5] + pPred[kiL6] + pPred[kiL7];
  const uint8_t kuiM2		= (kuiSum2 + 2) >> 2;
  const uint8_t kuiM3		= (kuiSum3 + 2) >> 2;
  const uint8_t kuiM4		= (kuiSum2 + kuiSum3 + 4) >> 3;
  const uint8_t kuiMUP[8]	= {kuiM1, kuiM1, kuiM1, kuiM1, kuiM2, kuiM2, kuiM2, kuiM2};
  const uint8_t kuiMDown[8]	= {kuiM3, kuiM3, kuiM3, kuiM3, kuiM4, kuiM4, kuiM4, kuiM4};
  const uint64_t kuiUP64		= LD64 (kuiMUP);
  const uint64_t kuiDN64		= LD64 (kuiMDown);

  ST64 (pPred       , kuiUP64);
  ST64 (pPred + kiL1 + 1, kuiUP64);
  ST64 (pPred + kiL2 + 1, kuiUP64);
  ST64 (pPred + kiL3 + 1, kuiUP64);
  ST64 (pPred + kiL4 + 1, kuiDN64);
  ST64 (pPred + kiL5 + 1, kuiDN64);
  ST64 (pPred + kiL6 + 1, kuiDN64);
  ST64 (pPred + kiL7 + 1, kuiDN64);
}

void WelsIChromaPredDcLeft_c (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiL1	=   -1 + kiStride;
  const int32_t kiL2	= kiL1 + kiStride;
  const int32_t kiL3	= kiL2 + kiStride;
  const int32_t kiL4	= kiL3 + kiStride;
  const int32_t kiL5	= kiL4 + kiStride;
  const int32_t kiL6	= kiL5 + kiStride;
  const int32_t kiL7	= kiL6 + kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiMUP   = (pPred[-1] + pPred[kiL1] + pPred[kiL2] + pPred[kiL3] + 2) >> 2 ;
  const uint8_t kuiMDown = (pPred[kiL4] + pPred[kiL5] + pPred[kiL6] + pPred[kiL7] + 2) >> 2;
  const uint64_t kuiUP64 = 0x0101010101010101ULL * kuiMUP;
  const uint64_t kuiDN64 = 0x0101010101010101ULL * kuiMDown;

  ST64 (pPred       , kuiUP64);
  ST64 (pPred + kiL1 + 1, kuiUP64);
  ST64 (pPred + kiL2 + 1, kuiUP64);
  ST64 (pPred + kiL3 + 1, kuiUP64);
  ST64 (pPred + kiL4 + 1, kuiDN64);
  ST64 (pPred + kiL5 + 1, kuiDN64);
  ST64 (pPred + kiL6 + 1, kuiDN64);
  ST64 (pPred + kiL7 + 1, kuiDN64);
}

void WelsIChromaPredDcTop_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp			= (kiStride << 3) - kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiM1	    = (pPred[-kiStride] + pPred[1 - kiStride] + pPred[2 - kiStride] + pPred[3 - kiStride] + 2) >> 2;
  const uint8_t kuiM2	    = (pPred[4 - kiStride] + pPred[5 - kiStride] + pPred[6 - kiStride] + pPred[7 - kiStride] + 2) >>
                            2;
  const uint8_t kuiM[8]	= {kuiM1, kuiM1, kuiM1, kuiM1, kuiM2, kuiM2, kuiM2, kuiM2};

  uint8_t i = 7;

  do {
    ST64 (pPred + iTmp, LD64 (kuiM));

    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsIChromaPredDcNA_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 3) - kiStride;
  const uint64_t kuiDC64 = 0x8080808080808080ULL;
  uint8_t i = 7;

  do {
    ST64 (pPred + iTmp, kuiDC64);

    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsI16x16LumaPredV_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp			= (kiStride << 4) - kiStride;
  const uint64_t kuiTop1	= LD64 (pPred - kiStride);
  const uint64_t kuiTop2  = LD64 (pPred - kiStride + 8);
  uint8_t i = 15;

  do {
    ST64 (pPred + iTmp  , kuiTop1);
    ST64 (pPred + iTmp + 8, kuiTop2);

    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsI16x16LumaPredH_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 4) - kiStride;
  uint8_t i = 15;

  do {
    const uint8_t kuiVal8	= pPred[iTmp - 1];
    const uint64_t kuiVal64	= 0x0101010101010101ULL * kuiVal8;

    ST64 (pPred + iTmp  , kuiVal64);
    ST64 (pPred + iTmp + 8, kuiVal64);

    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsI16x16LumaPredPlane_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t a = 0, b = 0, c = 0, H = 0, V = 0;
  int32_t i, j;
  uint8_t* pTop = &pPred[-kiStride];
  uint8_t* pLeft = &pPred[-1];

  for (i = 0 ; i < 8 ; i ++) {
    H += (i + 1) * (pTop[8 + i] - pTop[6 - i]);
    V += (i + 1) * (pLeft[ (8 + i) * kiStride] - pLeft[ (6 - i) * kiStride]);
  }

  a = (pLeft[15 * kiStride] + pTop[15]) << 4;
  b = (5 * H + 32) >> 6;
  c = (5 * V + 32) >> 6;

  for (i = 0 ; i < 16 ; i ++) {
    for (j = 0 ; j < 16 ; j ++) {
      int32_t iTmp = (a + b * (j - 7) + c * (i - 7) + 16) >> 5;
      iTmp = WELS_CLIP1 (iTmp);
      pPred[j] = iTmp;
    }
    pPred += kiStride;
  }
}

void WelsI16x16LumaPredDc_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 4) - kiStride;
  int32_t iSum = 0;
  uint8_t i = 15;
  uint8_t uiMean = 0;

  /*caculate the kMean value*/
  do {
    iSum += pPred[-1 + iTmp] + pPred[-kiStride + i];
    iTmp -= kiStride;
  } while (i-- > 0);
  uiMean = (16 + iSum) >> 5;

  iTmp = (kiStride << 4) - kiStride;
  i = 15;
  do {
    memset (&pPred[iTmp], uiMean, I16x16_COUNT);
    iTmp -= kiStride;
  } while (i-- > 0);
}


void WelsI16x16LumaPredDcTop_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 4) - kiStride;
  int32_t iSum = 0;
  uint8_t i = 15;
  uint8_t uiMean = 0;

  /*caculate the kMean value*/
  do {
    iSum += pPred[-kiStride + i];
  } while (i-- > 0);
  uiMean = (8 + iSum) >> 4;

  i = 15;
  do {
    memset (&pPred[iTmp], uiMean, I16x16_COUNT);
    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsI16x16LumaPredDcLeft_c (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 4) - kiStride;
  int32_t iSum = 0;
  uint64_t uiMean64 = 0;
  uint8_t uiMean = 0;
  uint8_t i = 15;

  /*caculate the kMean value*/
  do {
    iSum += pPred[-1 + iTmp];
    iTmp -= kiStride;
  } while (i-- > 0);
  uiMean	= (8 + iSum) >> 4;
  uiMean64	= 0x0101010101010101ULL * uiMean;

  iTmp = (kiStride << 4) - kiStride;
  i = 15;
  do {
    ST64 (pPred + iTmp  , uiMean64);
    ST64 (pPred + iTmp + 8, uiMean64);

    iTmp -= kiStride;
  } while (i-- > 0);
}

void WelsI16x16LumaPredDcNA_c (uint8_t* pPred, const int32_t kiStride) {
  const uint64_t kuiDC64 = 0x8080808080808080ULL;
  int32_t iTmp = (kiStride << 4) - kiStride;
  uint8_t i = 15;

  do {
    ST64 (pPred + iTmp, kuiDC64);
    ST64 (pPred + iTmp + 8, kuiDC64);

    iTmp -= kiStride;
  } while (i-- > 0);
}

} // namespace WelsDec
