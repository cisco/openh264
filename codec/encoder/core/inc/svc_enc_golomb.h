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
 * \file	golomb.h
 *
 * \brief	Exponential Golomb entropy coding/decoding routine
 *
 * \date	03/13/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_EXPONENTIAL_GOLOMB_ENTROPY_CODING_H__
#define WELS_EXPONENTIAL_GOLOMB_ENTROPY_CODING_H__

#include "typedefs.h"
#include "bit_stream.h"
#include "macros.h"

namespace WelsSVCEnc {

#define WRITE_BE_32(ptr, val) do { \
        (ptr)[0] = (val) >> 24; \
        (ptr)[1] = (val) >> 16; \
        (ptr)[2] = (val) >>  8; \
        (ptr)[3] = (val) >>  0; \
    } while (0)
/************************************************************************/
/* GOLOMB CODIMG FOR WELS ENCODER                                       */
/************************************************************************/

/*
 *	Exponential Golomb codes encoding routines
 */

#define    CAVLC_BS_INIT( pBs )  \
	uint8_t  * pBufPtr = pBs->pBufPtr; \
	uint32_t   uiCurBits = pBs->uiCurBits; \
	int32_t    iLeftBits = pBs->iLeftBits;

#define    CAVLC_BS_UNINIT( pBs ) \
	pBs->pBufPtr = pBufPtr;  \
	pBs->uiCurBits = uiCurBits;  \
	pBs->iLeftBits = iLeftBits;

#define    CAVLC_BS_WRITE( n,  v ) \
	{  \
	if ( (n) < iLeftBits ) {\
	    uiCurBits = (uiCurBits<<(n))|(v);\
		iLeftBits -= (n);\
	}\
	else {\
	    (n) -= iLeftBits;\
		uiCurBits = (uiCurBits<<iLeftBits) | ((v)>>(n));\
		WRITE_BE_32(pBufPtr, uiCurBits);\
		pBufPtr += 4;\
		uiCurBits = (v) & ((1<<(n))-1);\
		iLeftBits = 32 - (n);\
	}\
	} ;

extern const uint32_t g_uiGolombUELength[256];


/*
 *	Get size of unsigned exp golomb codes
 */
static inline uint32_t BsSizeUE (const uint32_t kiValue) {
  if (256 > kiValue) {
    return g_uiGolombUELength[kiValue];
  } else {
    uint32_t n = 0;
    uint32_t iTmpValue = kiValue + 1;

    if (iTmpValue & 0xffff0000) {
      iTmpValue >>= 16;
      n += 16;
    }
    if (iTmpValue & 0xff00) {
      iTmpValue >>= 8;
      n += 8;
    }

    //n += (g_uiGolombUELength[iTmpValue] >> 1);
    n += (g_uiGolombUELength[iTmpValue - 1] >> 1);
    return ((n << 1) + 1);

  }
}

/*
 *	Get size of signed exp golomb codes
 */
static inline uint32_t BsSizeSE (const int32_t kiValue) {
  uint32_t iTmpValue;
  if (0 == kiValue) {
    return 1;
  } else if (0 < kiValue) {
    iTmpValue = (kiValue << 1) - 1;
    return BsSizeUE (iTmpValue);
  } else {
    iTmpValue = ((-kiValue) << 1);
    return BsSizeUE (iTmpValue);
  }
}

/*
 *	Get size of truncated exp golomb codes
 */
static inline int32_t BsSizeTE (const int32_t kiX, const int32_t kiValue) {
  return 0;
}



static inline int32_t BsWriteBits (SBitStringAux* pBs, int32_t n, const uint32_t kuiValue) {
  if (n < pBs->iLeftBits) {
    pBs->uiCurBits = (pBs->uiCurBits << n) | kuiValue;
    pBs->iLeftBits -= n;
  } else {
    n -= pBs->iLeftBits;
    pBs->uiCurBits = (pBs->uiCurBits << pBs->iLeftBits) | (kuiValue >> n);
    WRITE_BE_32 (pBs->pBufPtr, pBs->uiCurBits);
    pBs->pBufPtr += 4;
    pBs->uiCurBits = kuiValue & ((1 << n) - 1);
    pBs->iLeftBits = 32 - n;
  }
  return 0;
}

/*
 *	Write 1 bit
 */
static inline int32_t BsWriteOneBit (SBitStringAux* pBs, const uint32_t kuiValue) {
  BsWriteBits (pBs, 1, kuiValue);

  return 0;
}


static inline void BsFlush (SBitStringAux* pBs) {
  WRITE_BE_32 (pBs->pBufPtr, pBs->uiCurBits << pBs->iLeftBits);
  pBs->pBufPtr += 4 - pBs->iLeftBits / 8;
  pBs->iLeftBits = 32;
  pBs->uiCurBits = 0;	//  for future writing safe, 5/19/2010
}

/*
 *	Write unsigned exp golomb codes
 */
static inline void BsWriteUE (SBitStringAux* pBs, const uint32_t kuiValue) {
  uint32_t iTmpValue = kuiValue + 1;
  if (256 > kuiValue)	{
    BsWriteBits (pBs, g_uiGolombUELength[kuiValue], kuiValue + 1);
  } else {
    uint32_t n = 0;

    if (iTmpValue & 0xffff0000) {
      iTmpValue >>= 16;
      n += 16;
    }
    if (iTmpValue & 0xff00) {
      iTmpValue >>= 8;
      n += 8;
    }

    //n += (g_uiGolombUELength[iTmpValue] >> 1);

    n += (g_uiGolombUELength[iTmpValue - 1] >> 1);
    BsWriteBits (pBs, (n << 1) + 1, kuiValue + 1);
  }
  return;
}

/*
 *	Write signed exp golomb codes
 */
static inline void BsWriteSE (SBitStringAux* pBs, int32_t iValue) {
  uint32_t iTmpValue;
  if (0 == iValue) {
    BsWriteOneBit (pBs, 1);
  } else if (0 < iValue) {
    iTmpValue = (iValue << 1) - 1;
    BsWriteUE (pBs, iTmpValue);
  } else {
    iTmpValue = ((-iValue) << 1);
    BsWriteUE (pBs, iTmpValue);
  }
  return;
}

/*
 *	Write truncated exp golomb codes
 */
static inline void BsWriteTE (SBitStringAux* pBs, const int32_t kiX, const uint32_t kuiValue) {
  if (1 == kiX) {
    BsWriteOneBit (pBs, !kuiValue);
  } else {
    BsWriteUE (pBs, kuiValue);
  }
}


/*
 *	Write RBSP trailing bits
 */
static inline void BsRbspTrailingBits (SBitStringAux* pBs) {
  BsWriteOneBit (pBs, 1);
  BsFlush (pBs);
}


static inline bool   BsCheckByteAlign (SBitStringAux* pBs) {
  return ! (pBs->iLeftBits & 0x7);
}


static inline int32_t BsGetBitsPos (SBitStringAux* pBs) {
  return (((pBs->pBufPtr - pBs->pBuf) << 3) + 32 - pBs->iLeftBits);
}

}
#endif//WELS_EXPONENTIAL_GOLOMB_ENTROPY_CODING_H__
