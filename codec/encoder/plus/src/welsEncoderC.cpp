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

#include "codec_api.h"
#include "codec_api_c.h"

int SVCEncoder_Initialize (ISVCEncoder* pEncoder, SVCEncodingParam* pParam, const INIT_TYPE kiInitType) {
  return pEncoder->Initialize (pParam, kiInitType);
}

int SVCEncoder_Initialize2 (ISVCEncoder* pEncoder, void* pParam, const INIT_TYPE kiInitType) {
  return pEncoder->Initialize (pParam, kiInitType);
}

int SVCEncoder_Uninitialize (ISVCEncoder* pEncoder) {
  return pEncoder->Uninitialize ();
}

int SVCEncoder_EncodeFrame (ISVCEncoder* pEncoder, const unsigned char* kpSrc, SFrameBSInfo* pBsInfo) {
  return pEncoder->EncodeFrame (kpSrc, pBsInfo);
}

int SVCEncoder_EncodeFrame2 (ISVCEncoder* pEncoder, const SSourcePicture** kppSrcPicList, int nSrcPicNum,
                             SFrameBSInfo* pBsInfo) {
  return pEncoder->EncodeFrame (kppSrcPicList, nSrcPicNum, pBsInfo);
}

int SVCEncoder_EncodeParameterSets (ISVCEncoder* pEncoder, SFrameBSInfo* pBsInfo) {
  return pEncoder->EncodeParameterSets (pBsInfo);
}

int SVCEncoder_PauseFrame (ISVCEncoder* pEncoder, const unsigned char* kpSrc, SFrameBSInfo* pBsInfo) {
  return pEncoder->PauseFrame (kpSrc, pBsInfo);
}

int SVCEncoder_ForceIntraFrame (ISVCEncoder* pEncoder, bool bIDR) {
  return pEncoder->ForceIntraFrame (bIDR);
}

int SVCEncoder_SetOption (ISVCEncoder* pEncoder, ENCODER_OPTION eOptionId, void* pOption) {
  return pEncoder->SetOption (eOptionId, pOption);
}

int SVCEncoder_GetOption (ISVCEncoder* pEncoder, ENCODER_OPTION eOptionId, void* pOption) {
  return pEncoder->GetOption (eOptionId, pOption);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
