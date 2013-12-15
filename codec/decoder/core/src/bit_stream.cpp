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
#include "macros.h"

namespace WelsDec {

#ifdef WORDS_BIGENDIAN
inline uint32_t EndianFix (uint32_t uiX) {
  return uiX;
}
#else //WORDS_BIGENDIAN

#ifdef WIN32
inline uint32_t EndianFix (uint32_t uiX) {
  __asm {
    mov   eax,  uiX
    bswap   eax
    mov   uiX,    eax
  }
  return uiX;
}
#else  //_MSC_VER

inline uint32_t EndianFix (uint32_t uiX) {
#ifdef ARM_ARCHv7
  __asm__ __volatile__ ("rev %0, %0":"+r" (uiX)); //Just for the ARMv7
#elif defined (X86_ARCH)
  __asm__ __volatile__ ("bswap %0":"+r" (uiX));
#else
  uiX = ((uiX & 0xff000000) >> 24) | ((uiX & 0xff0000) >> 8) |
        ((uiX & 0xff00) << 8) | ((uiX & 0xff) << 24);
#endif
  return uiX;
}
#endif //_MSC_VER

#endif //WORDS_BIGENDIAN

inline uint32_t GetValue4Bytes (uint8_t* pDstNal) {
  uint32_t uiValue = 0;
  uiValue = (pDstNal[0] << 24) | (pDstNal[1] << 16) | (pDstNal[2] << 8) | (pDstNal[3]);
  return uiValue;
}

void_t InitReadBits (PBitStringAux pBitString) {
  pBitString->uiCurBits  = GetValue4Bytes (pBitString->pCurBuf);
  pBitString->pCurBuf  += 4;
  pBitString->iLeftBits = -16;
}

/*!
 * \brief	input bits for decoder or initialize bitstream writing in encoder
 *
 * \param	pBitString	Bit string auxiliary pointer
 * \param	kpBuf		bit-stream buffer
 * \param	kiSize	    size in bits for decoder; size in bytes for encoder
 *
 * \return	size of buffer data in byte; failed in -1 return
 */
int32_t InitBits (PBitStringAux pBitString, const uint8_t* kpBuf, const int32_t kiSize) {
  const int32_t kiSizeBuf = (kiSize + 7) >> 3;
  uint8_t* pTmp = (uint8_t*)kpBuf;

  if (NULL == pTmp)
    return -1;

  pBitString->pStartBuf   = pTmp;				// buffer to start position
  pBitString->pEndBuf	    = pTmp + kiSizeBuf;	// buffer + length
  pBitString->iBits	    = kiSize;				// count bits of overall bitstreaming inputindex;

  pBitString->pCurBuf   = pBitString->pStartBuf;
  InitReadBits (pBitString);

  return kiSizeBuf;
}

} // namespace WelsDec

