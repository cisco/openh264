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

//bit_stream.h	-	bit-stream reading and / writing auxiliary pData
#ifndef WELS_BIT_STREAM_H__
#define WELS_BIT_STREAM_H__

#include "typedefs.h"

namespace WelsEnc {
//#include "macros.h"

/*
 *	auxiliary struct for bit-stream reading / writing
 */
typedef struct TagBitStringAux {
  uint8_t*		pBuf;		// pBuffer to start position
  uint8_t*		pBufEnd;	// pBuffer + length
  uint8_t*		pBufPtr;	// current writing position
  uint32_t    uiCurBits;
  int32_t		iLeftBits;	// count number of available bits left ([1, 8]),
// need pointer to next byte start position in case 0 bit left then 8 instead
} SBitStringAux;

/*!
 * \brief	input bits for decoder or initialize bitstream writing in encoder
 *
 * \param	pBs		Bit string auxiliary pointer
 * \param	pBuf		bit-stream pBuffer
 * \param	iSize	iSize in bits for decoder; iSize in bytes for encoder
 *
 * \return	iSize of pBuffer pData in byte; failed in -1 return
 */
static inline int32_t InitBits (SBitStringAux* pBs, const uint8_t* kpBuf, const int32_t kiSize) {
  uint8_t* ptr = (uint8_t*)kpBuf;

  pBs->pBuf			= ptr;
  pBs->pBufPtr		= ptr;
  pBs->pBufEnd		= ptr + kiSize;
  pBs->iLeftBits	= 32;
  pBs->uiCurBits = 0;

  return kiSize;
}

}

#endif//WELS_BIT_STREAM_H__
