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
  if (pCtx->iErrorConMethod == ERROR_CON_SLICE_COPY) {
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
      pCtx->sCopyFunc.pCopyChromaFunc		= WelsCopy8x8_neon; //aligned
    }
#endif //HAVE_NEON
  } //TODO add more methods here
  return;
}

//Do error concealment using frame copy method
void DoErrorConFrameCopy (PWelsDecoderContext pCtx) {
  //TODO
}


//Do error concealment using slice copy method
void DoErrorConSliceCopy (PWelsDecoderContext pCtx) {
  //TODO
}

//Mark erroneous frame as Ref Pic into DPB
int32_t MarkECFrameAsRef (PWelsDecoderContext pCtx) {
  //TODO
  return ERR_NONE;
}

bool NeedErrorCon (PWelsDecoderContext pCtx) {
  bool bNeedEC = false;
  //TODO
  return bNeedEC;
}

// ImplementErrorConceal
// Do actual error concealment
int32_t ImplementErrorCon (PWelsDecoderContext pCtx) {
  int32_t iRet = ERR_NONE;
  //TODO
  return iRet;
}

} // namespace WelsDec
