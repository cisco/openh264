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
 *	error_concealment.cpp:	Wels decoder error concealment implementation
 */

#include "error_code.h"
#include "expand_pic.h"
#include "manage_dec_ref.h"
#include "copy_mb.h"
#include "error_concealment.h"
#include "cpu_core.h"

namespace WelsDec {
//Init
void InitErrorCon (PWelsDecoderContext pCtx) {
  if (pCtx->eErrorConMethod == ERROR_CON_SLICE_COPY) {
    pCtx->sCopyFunc.pCopyLumaFunc = WelsCopy16x16_c;
    pCtx->sCopyFunc.pCopyChromaFunc = WelsCopy8x8_c;

#if defined(X86_ASM)
    if (pCtx->uiCpuFlag & WELS_CPU_MMXEXT) {
      pCtx->sCopyFunc.pCopyChromaFunc = WelsCopy8x8_mmx; //aligned
    }

    if (pCtx->uiCpuFlag & WELS_CPU_SSE2) {
      pCtx->sCopyFunc.pCopyLumaFunc = WelsCopy16x16_sse2; //this is aligned copy;
    }
#endif //X86_ASM

#if defined(HAVE_NEON)
    if (pCtx->uiCpuFlag & WELS_CPU_NEON) {
      pCtx->sCopyFunc.pCopyLumaFunc		= WelsCopy16x16_neon; //aligned
      pCtx->sCopyFunc.pCopyChromaFunc		= WelsCopy8x8_neon; //aligned
    }
#endif //HAVE_NEON

#if defined(HAVE_NEON_AARCH64)
    if (pCtx->uiCpuFlag & WELS_CPU_NEON) {
      pCtx->sCopyFunc.pCopyLumaFunc		= WelsCopy16x16_AArch64_neon; //aligned
      pCtx->sCopyFunc.pCopyChromaFunc		= WelsCopy8x8_AArch64_neon; //aligned
    }
#endif //HAVE_NEON_AARCH64
  } //TODO add more methods here
  return;
}

//Do error concealment using frame copy method
void DoErrorConFrameCopy (PWelsDecoderContext pCtx) {
  PPicture pDstPic = pCtx->pDec;
  PPicture pSrcPic = pCtx->pPreviousDecodedPictureInDpb;
  uint32_t uiHeightInPixelY = (pCtx->pSps->iMbHeight) << 4;
  int32_t iStrideY = pDstPic->iLinesize[0];
  int32_t iStrideUV = pDstPic->iLinesize[1];
  if (pSrcPic == NULL) { //no ref pic, assign specific data to picture
    memset (pDstPic->pData[0], 128, uiHeightInPixelY * iStrideY);
    memset (pDstPic->pData[1], 128, (uiHeightInPixelY >> 1) * iStrideUV);
    memset (pDstPic->pData[2], 128, (uiHeightInPixelY >> 1) * iStrideUV);
  } else { //has ref pic here
    memcpy (pDstPic->pData[0], pSrcPic->pData[0], uiHeightInPixelY * iStrideY);
    memcpy (pDstPic->pData[1], pSrcPic->pData[1], (uiHeightInPixelY >> 1) * iStrideUV);
    memcpy (pDstPic->pData[2], pSrcPic->pData[2], (uiHeightInPixelY >> 1) * iStrideUV);
  }
}


//Do error concealment using slice copy method
void DoErrorConSliceCopy (PWelsDecoderContext pCtx) {
  int32_t iMbWidth = (int32_t) pCtx->pSps->iMbWidth;
  int32_t iMbHeight = (int32_t) pCtx->pSps->iMbHeight;
  PPicture pDstPic = pCtx->pDec;
  PPicture pSrcPic = pCtx->pPreviousDecodedPictureInDpb;

  //uint8_t *pDstData[3], *pSrcData[3];
  bool* pMbCorrectlyDecodedFlag = pCtx->pCurDqLayer->pMbCorrectlyDecodedFlag;

  //Do slice copy late
  int32_t iMbXyIndex;
  uint8_t* pSrcData, *pDstData;
  uint32_t iSrcStride; // = pSrcPic->iLinesize[0];
  uint32_t iDstStride = pDstPic->iLinesize[0];
  for (int32_t iMbY = 0; iMbY < iMbHeight; ++iMbY) {
    for (int32_t iMbX = 0; iMbX < iMbWidth; ++iMbX) {
      iMbXyIndex = iMbY * iMbWidth + iMbX;
      if (!pMbCorrectlyDecodedFlag[iMbXyIndex]) {
        if (pSrcPic != NULL) {
          iSrcStride = pSrcPic->iLinesize[0];
          //Y component
          pDstData = pDstPic->pData[0] + iMbY * 16 * iDstStride + iMbX * 16;;
          pSrcData = pSrcPic->pData[0] + iMbY * 16 * iSrcStride + iMbX * 16;
          pCtx->sCopyFunc.pCopyLumaFunc (pDstData, iDstStride, pSrcData, iSrcStride);
          //U component
          pDstData = pDstPic->pData[1] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          pSrcData = pSrcPic->pData[1] + iMbY * 8 * iSrcStride / 2 + iMbX * 8;
          pCtx->sCopyFunc.pCopyChromaFunc (pDstData, iDstStride / 2, pSrcData, iSrcStride / 2);
          //V component
          pDstData = pDstPic->pData[2] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          pSrcData = pSrcPic->pData[2] + iMbY * 8 * iSrcStride / 2 + iMbX * 8;
          pCtx->sCopyFunc.pCopyChromaFunc (pDstData, iDstStride / 2, pSrcData, iSrcStride / 2);
        } else { //pSrcPic == NULL
          //Y component
          pDstData = pDstPic->pData[0] + iMbY * 16 * iDstStride + iMbX * 16;
          for (int32_t i = 0; i < 16; ++i) {
            memset (pDstData, 128, 16);
            pDstData += iDstStride;
          }
          //U component
          pDstData = pDstPic->pData[1] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          for (int32_t i = 0; i < 8; ++i) {
            memset (pDstData, 128, 8);
            pDstData += iDstStride / 2;
          }
          //V component
          pDstData = pDstPic->pData[2] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          for (int32_t i = 0; i < 8; ++i) {
            memset (pDstData, 128, 8);
            pDstData += iDstStride / 2;
          }
        } //
      } //!pMbCorrectlyDecodedFlag[iMbXyIndex]
    } //iMbX
  } //iMbY
}


//Mark erroneous frame as Ref Pic into DPB
int32_t MarkECFrameAsRef (PWelsDecoderContext pCtx) {
  int32_t iRet = WelsMarkAsRef (pCtx);
  if (iRet != ERR_NONE) {
    pCtx->pDec = NULL;
    return iRet;
  }
  ExpandReferencingPicture (pCtx->pDec->pData, pCtx->pDec->iWidthInPixel, pCtx->pDec->iHeightInPixel,
                            pCtx->pDec->iLinesize,
                            pCtx->sExpandPicFunc.pfExpandLumaPicture, pCtx->sExpandPicFunc.pfExpandChromaPicture);
  pCtx->pDec = NULL;

  return ERR_NONE;
}

bool NeedErrorCon (PWelsDecoderContext pCtx) {
  bool bNeedEC = false;
  int32_t iMbNum = pCtx->pSps->iMbWidth * pCtx->pSps->iMbHeight;
  for (int32_t i = 0; i < iMbNum; ++i) {
    if (!pCtx->pCurDqLayer->pMbCorrectlyDecodedFlag[i]) {
      bNeedEC = true;
      break;
    }
  }
  return bNeedEC;
}

// ImplementErrorConceal
// Do actual error concealment
void ImplementErrorCon (PWelsDecoderContext pCtx) {
  if (ERROR_CON_DISABLE == pCtx->eErrorConMethod) {
    pCtx->iErrorCode |= dsBitstreamError;
    return;
  } else if (ERROR_CON_FRAME_COPY == pCtx->eErrorConMethod) {
    DoErrorConFrameCopy (pCtx);
  } else if (ERROR_CON_SLICE_COPY == pCtx->eErrorConMethod) {
    DoErrorConSliceCopy (pCtx);
  } //TODO add other EC methods here in the future
  pCtx->iErrorCode |= dsDataErrorConcealed;
  pCtx->pDec->bIsComplete = false; // Set complete flag to false after do EC.
}

} // namespace WelsDec
