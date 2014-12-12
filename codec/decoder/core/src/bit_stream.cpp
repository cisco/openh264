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
 * \file	bit_stream.cpp
 *
 * \brief	Reading / writing bit-stream
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#include "bit_stream.h"
#include "error_code.h"

namespace WelsDec {

inline uint32_t GetValue4Bytes (uint8_t* pDstNal) {
  uint32_t uiValue = 0;
  uiValue = (pDstNal[0] << 24) | (pDstNal[1] << 16) | (pDstNal[2] << 8) | (pDstNal[3]);
  return uiValue;
}

int32_t InitReadBits (PBitStringAux pBitString, intX_t iEndOffset) {
  if (pBitString->pCurBuf >= (pBitString->pEndBuf - iEndOffset)) {
    return ERR_INFO_INVALID_ACCESS;
  }
  pBitString->uiCurBits  = GetValue4Bytes (pBitString->pCurBuf);
  pBitString->pCurBuf  += 4;
  pBitString->iLeftBits = -16;
  return ERR_NONE;
}

/*!
 * \brief	input bits for decoder or initialize bitstream writing in encoder
 *
 * \param	pBitString	Bit string auxiliary pointer
 * \param	kpBuf		bit-stream buffer
 * \param	kiSize	    size in bits for decoder; size in bytes for encoder
 *
 * \return	0: success, other: fail
 */
int32_t InitBits (PBitStringAux pBitString, const uint8_t* kpBuf, const int32_t kiSize) {
  const int32_t kiSizeBuf = (kiSize + 7) >> 3;
  uint8_t* pTmp = (uint8_t*)kpBuf;

  if (NULL == pTmp)
    return ERR_INFO_INVALID_ACCESS;

  pBitString->pStartBuf   = pTmp;				// buffer to start position
  pBitString->pEndBuf	    = pTmp + kiSizeBuf;	// buffer + length
  pBitString->iBits	    = kiSize;				// count bits of overall bitstreaming inputindex;
  pBitString->pCurBuf   = pBitString->pStartBuf;
  int32_t iErr = InitReadBits (pBitString, 0);
  if (iErr) {
    return iErr;
  }
  return ERR_NONE;
}

//Following for write bs in decoder
void DecInitBitsForEncoding (PBitStringAux pBitString, uint8_t* pBuf, const int32_t kiSize) {
  uint8_t* pPtr = pBuf;
  pBitString->pStartBuf = pPtr;
  pBitString->pCurBuf = pPtr;
  pBitString->pEndBuf = pPtr + kiSize;
  pBitString->iLeftBits = 32;
  pBitString->uiCurBits = 0;
}

#define WRITE_BE_32(ptr, val) do { \
    (ptr)[0] = (val) >> 24; \
    (ptr)[1] = (val) >> 16; \
    (ptr)[2] = (val) >>  8; \
    (ptr)[3] = (val) >>  0; \
  } while (0);

int32_t DecBsWriteBits (PBitStringAux pBitString, int32_t iLen, const uint32_t kuiValue) {
  if (iLen < pBitString->iLeftBits) {
    pBitString->uiCurBits = (pBitString->uiCurBits << iLen) | kuiValue;
    pBitString->iLeftBits -= iLen;
  } else {
    iLen -= pBitString->iLeftBits;
    pBitString->uiCurBits = (pBitString->uiCurBits << pBitString->iLeftBits) | (kuiValue >> iLen);
    WRITE_BE_32 (pBitString->pCurBuf, pBitString->uiCurBits);
    pBitString->pCurBuf += 4;
    pBitString->uiCurBits = kuiValue & ((1 << iLen) - 1);
    pBitString->iLeftBits = 32 - iLen;
  }
  return 0;
}

int32_t DecBsWriteOneBit (PBitStringAux pBitString, const uint32_t kuiValue) {
  DecBsWriteBits (pBitString, 1, kuiValue);
  return 0;
}

int32_t DecBsFlush (PBitStringAux pBitString) {
  WRITE_BE_32 (pBitString->pCurBuf, pBitString->uiCurBits << pBitString->iLeftBits);
  pBitString->pCurBuf += 4 - pBitString->iLeftBits / 8;
  pBitString->iLeftBits = 32;
  pBitString->uiCurBits = 0;
  return 0;
}

const uint32_t g_kuiDecGolombUELength[256] = {
  1,  3,  3,  5,  5,  5,  5,  7,  7,  7,  7,  7,  7,  7,  7,     //14
  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9, //30
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,//46
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,//62
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,//
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  17
};

int32_t DecBsWriteUe (PBitStringAux pBitString, const uint32_t kuiValue) {
  uint32_t iTmpValue = kuiValue + 1;
  if (256 > kuiValue) {
    DecBsWriteBits (pBitString, g_kuiDecGolombUELength[kuiValue], kuiValue + 1);
  } else {
    uint32_t n = 0;
    if (iTmpValue & 0xffff0000) {
      iTmpValue >>= 16;
      n += 16;
    }
    if (iTmpValue & 0xff) {
      iTmpValue >>= 8;
      n += 8;
    }

    //n += (g_kuiDecGolombUELength[iTmpValue] >> 1);

    n += (g_kuiDecGolombUELength[iTmpValue - 1] >> 1);
    DecBsWriteBits (pBitString, (n << 1) + 1, kuiValue + 1);
  }
  return 0;
}

int32_t DecBsWriteSe (PBitStringAux pBitString, const int32_t kiValue) {
  uint32_t iTmpValue;
  if (0 == kiValue) {
    DecBsWriteOneBit (pBitString, 1);
  } else if (0 < kiValue) {
    iTmpValue = (kiValue << 1) - 1;
    DecBsWriteUe (pBitString, iTmpValue);
  } else {
    iTmpValue = ((-kiValue) << 1);
    DecBsWriteUe (pBitString, iTmpValue);
  }
  return 0;
}

int32_t DecBsRbspTrailingBits (PBitStringAux pBitString) {
  DecBsWriteOneBit (pBitString, 1);
  DecBsFlush (pBitString);

  return 0;
}

void RBSP2EBSP (uint8_t* pDstBuf, uint8_t* pSrcBuf, const int32_t kiSize) {
  uint8_t* pSrcPointer = pSrcBuf;
  uint8_t* pDstPointer = pDstBuf;
  uint8_t* pSrcEnd = pSrcBuf + kiSize;
  int32_t iZeroCount = 0;

  while (pSrcPointer < pSrcEnd) {
    if (iZeroCount == 2 && *pSrcPointer <= 3) {
      //add the code 0x03
      *pDstPointer++ = 3;
      iZeroCount = 0;
    }
    if (*pSrcPointer == 0) {
      ++ iZeroCount;
    } else {
      iZeroCount = 0;
    }
    *pDstPointer++ = *pSrcPointer++;
  }
}

} // namespace WelsDec

