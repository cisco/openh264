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
 * \file    pic_queue.c
 *
 * \brief   Recycled piture queue implementation
 *
 * \date    03/13/2009 Created
 *
 *************************************************************************************
 */
#include "pic_queue.h"
#include "decoder_context.h"
#include "codec_def.h"
#include "memory_align.h"

namespace WelsDec {

void FreePicture (PPicture pPic, CMemoryAlign* pMa);


///////////////////////////////////Recycled queue management for pictures///////////////////////////////////
/*   ______________________________________
  -->| P0 | P1 | P2 | P3 | P4 | .. | Pn-1 |-->
     --------------------------------------
 *
 *  How does it work?
 *  node <- next; ++ next;
 *
*/



PPicture AllocPicture (PWelsDecoderContext pCtx, const int32_t kiPicWidth, const int32_t kiPicHeight) {
  PPicture pPic = NULL;
  int32_t iPicWidth = 0;
  int32_t iPicHeight = 0;

  int32_t iPicChromaWidth   = 0;
  int32_t iPicChromaHeight  = 0;
  int32_t iLumaSize         = 0;
  int32_t iChromaSize       = 0;
  CMemoryAlign* pMa = pCtx->pMemAlign;

  pPic = (PPicture) pMa->WelsMallocz (sizeof (SPicture), "PPicture");
  WELS_VERIFY_RETURN_IF (NULL, NULL == pPic);

  memset (pPic, 0, sizeof (SPicture));

  iPicWidth = WELS_ALIGN (kiPicWidth + (PADDING_LENGTH << 1), PICTURE_RESOLUTION_ALIGNMENT);
  iPicHeight = WELS_ALIGN (kiPicHeight + (PADDING_LENGTH << 1), PICTURE_RESOLUTION_ALIGNMENT);
  iPicChromaWidth   = iPicWidth >> 1;
  iPicChromaHeight  = iPicHeight >> 1;

  iLumaSize     = iPicWidth * iPicHeight;
  iChromaSize   = iPicChromaWidth * iPicChromaHeight;

  if (pCtx->bParseOnly) {
    pPic->pBuffer[0] = pPic->pBuffer[1] = pPic->pBuffer[2] = NULL;
    pPic->pData[0] = pPic->pData[1] = pPic->pData[2] = NULL;
    pPic->iLinesize[0] = iPicWidth;
    pPic->iLinesize[1] = pPic->iLinesize[2] = iPicChromaWidth;
  } else {
    pPic->pBuffer[0] = static_cast<uint8_t*> (pMa->WelsMallocz (iLumaSize /* luma */
                       + (iChromaSize << 1) /* Cb,Cr */, "_pic->buffer[0]"));
    WELS_VERIFY_RETURN_PROC_IF (NULL, NULL == pPic->pBuffer[0], FreePicture (pPic, pMa));

    memset (pPic->pBuffer[0], 128, (iLumaSize + (iChromaSize << 1)));
    pPic->iLinesize[0] = iPicWidth;
    pPic->iLinesize[1] = pPic->iLinesize[2] = iPicChromaWidth;
    pPic->pBuffer[1]   = pPic->pBuffer[0] + iLumaSize;
    pPic->pBuffer[2]   = pPic->pBuffer[1] + iChromaSize;
    pPic->pData[0]     = pPic->pBuffer[0] + (1 + pPic->iLinesize[0]) * PADDING_LENGTH;
    pPic->pData[1]     = pPic->pBuffer[1] + /*WELS_ALIGN*/ (((1 + pPic->iLinesize[1]) * PADDING_LENGTH) >> 1);
    pPic->pData[2]     = pPic->pBuffer[2] + /*WELS_ALIGN*/ (((1 + pPic->iLinesize[2]) * PADDING_LENGTH) >> 1);
  }
  pPic->iPlanes        = 3;    // yv12 in default
  pPic->iWidthInPixel  = kiPicWidth;
  pPic->iHeightInPixel = kiPicHeight;
  pPic->iFrameNum      = -1;
  pPic->bAvailableFlag = true;

  return pPic;
}

void FreePicture (PPicture pPic, CMemoryAlign* pMa) {
  if (NULL != pPic) {

    if (pPic->pBuffer[0]) {
      pMa->WelsFree (pPic->pBuffer[0], "pPic->pBuffer[0]");
    }

    pMa->WelsFree (pPic, "pPic");

    pPic = NULL;
  }
}
PPicture PrefetchPic (PPicBuff pPicBuf) {
  int32_t iPicIdx = 0;
  PPicture pPic  = NULL;

  if (pPicBuf->iCapacity == 0) {
    return NULL;
  }

  for (iPicIdx = pPicBuf->iCurrentIdx + 1; iPicIdx < pPicBuf->iCapacity ; ++iPicIdx) {
    if (pPicBuf->ppPic[iPicIdx] != NULL && pPicBuf->ppPic[iPicIdx]->bAvailableFlag
        && !pPicBuf->ppPic[iPicIdx]->bUsedAsRef) {
      pPic = pPicBuf->ppPic[iPicIdx];
      break;
    }
  }
  if (pPic != NULL) {
    pPicBuf->iCurrentIdx = iPicIdx;
    return pPic;
  }
  for (iPicIdx = 0 ; iPicIdx <= pPicBuf->iCurrentIdx ; ++iPicIdx) {
    if (pPicBuf->ppPic[iPicIdx] != NULL && pPicBuf->ppPic[iPicIdx]->bAvailableFlag
        && !pPicBuf->ppPic[iPicIdx]->bUsedAsRef) {
      pPic = pPicBuf->ppPic[iPicIdx];
      break;
    }
  }

  pPicBuf->iCurrentIdx = iPicIdx;
  return pPic;
}

} // namespace WelsDec
