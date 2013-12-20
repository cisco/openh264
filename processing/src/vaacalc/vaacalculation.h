/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	    :  vaacalculation.h
 *
 * \brief	    :  pVaa calculation class of wels video processor class
 *
 * \date        :  2011/03/18
 *
 * \description :  1. rewrite the package code of pVaa calculation class
 *
 *************************************************************************************
 */

#ifndef _WELSVP_VAACALCULATION_H
#define _WELSVP_VAACALCULATION_H

#include "../common/util.h"
#include "../common/memory.h"
#include "../common/WelsFrameWork.h"
#include "../../interface/IWelsVP.h"

WELSVP_NAMESPACE_BEGIN

typedef void (VAACalcSadBgdFunc) (uint8_t* pCurData, uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                                  int32_t iPicStride,
                                  int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSd8x8, uint8_t* pMad8x8);

typedef void (VAACalcSadSsdBgdFunc) (uint8_t* pCurData, uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                                     int32_t iPicStride,
                                     int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* pSumSquare16x16,
                                     int32_t* pSsd16x16, int32_t* pSd8x8, uint8_t* pMad8x8);

typedef void (VAACalcSadFunc) (uint8_t* pCurData, uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                               int32_t iPicStride,
                               int32_t* pFrameSad, int32_t* pSad8x8);

typedef void (VAACalcSadVarFunc) (uint8_t* pCurData, uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                                  int32_t iPicStride,
                                  int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* pSumSquare16x16);

typedef void (VAACalcSadSsdFunc) (uint8_t* pCurData, uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                                  int32_t iPicStride,
                                  int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* pSumSquare16x16, int32_t* pSsd16x16);


typedef VAACalcSadBgdFunc*		 PVAACalcSadBgdFunc;
typedef VAACalcSadSsdBgdFunc*	 PVAACalcSadSsdBgdFunc;
typedef VAACalcSadFunc*			 PVAACalcSadFunc;
typedef VAACalcSadVarFunc*		 PVAACalcSadVarFunc;
typedef VAACalcSadSsdFunc*		 PVAACalcSadSsdFunc;

typedef  struct TagVaaFuncs {
  PVAACalcSadBgdFunc		pfVAACalcSadBgd;
  PVAACalcSadSsdBgdFunc	pfVAACalcSadSsdBgd;
  PVAACalcSadFunc			pfVAACalcSad;
  PVAACalcSadVarFunc		pfVAACalcSadVar;
  PVAACalcSadSsdFunc		pfVAACalcSadSsd;
} SVaaFuncs;


VAACalcSadBgdFunc		VAACalcSadBgd_c;
VAACalcSadSsdBgdFunc	VAACalcSadSsdBgd_c;
VAACalcSadFunc			    VAACalcSad_c;
VAACalcSadVarFunc		VAACalcSadVar_c;
VAACalcSadSsdFunc		VAACalcSadSsd_c;


#ifdef X86_ASM
WELSVP_EXTERN_C_BEGIN
VAACalcSadBgdFunc		VAACalcSadBgd_sse2;
VAACalcSadSsdBgdFunc	VAACalcSadSsdBgd_sse2;
VAACalcSadFunc			    VAACalcSad_sse2;
VAACalcSadVarFunc		VAACalcSadVar_sse2;
VAACalcSadSsdFunc		VAACalcSadSsd_sse2;
WELSVP_EXTERN_C_END
#endif

class CVAACalculation : public IStrategy {
 public:
  CVAACalculation (int32_t iCpuFlag);
  ~CVAACalculation();

  EResult Process (int32_t iType, SPixMap* pCurPixMap, SPixMap* pRefPixMap);
  EResult Set (int32_t iType, void* pParam);

 private:
  void InitVaaFuncs (SVaaFuncs& sVaaFunc, int32_t iCpuFlag);

 private:
  SVaaFuncs      m_sVaaFuncs;
  int32_t       m_iCPUFlag;
  SVAACalcParam m_sCalcParam;
};

WELSVP_NAMESPACE_END

#endif
