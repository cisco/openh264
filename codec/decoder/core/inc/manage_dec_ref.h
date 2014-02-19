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
 *  \file	manage_dec_ref.h
 *
 *  Abstract
 *      Interface for managing reference picture
 *
 *  History
 *      08/14/2009 Created
 *
 *****************************************************************************/
#ifndef WELS_MANAGE_DEC_REF_H__
#define WELS_MANAGE_DEC_REF_H__


#include "typedefs.h"
#include "decoder_context.h"

namespace WelsDec {

typedef enum TagRemoveFlag {
REMOVE_TARGET = 0,
REMOVE_BASE = 1,
REMOVE_BASE_FIRST = 2
} ERemoveFlag;

void  WelsResetRefPic (PWelsDecoderContext pCtx);
int32_t WelsInitRefList (PWelsDecoderContext pCtx, int32_t iPoc);
int32_t WelsReorderRefList (PWelsDecoderContext pCtx);
int32_t WelsMarkAsRef (PWelsDecoderContext pCtx, const bool kbRefBaseMarkingFlag);

static PPicture WelsDelShortFromList (PRefPic pRefPic, int32_t iFrameNum,           ERemoveFlag eRemoveFlag);
static PPicture WelsDelLongFromList (PRefPic pRefPic, uint32_t uiLongTermFrameIdx, ERemoveFlag eRemoveFlag);
static PPicture WelsDelShortFromListSetUnref (PRefPic pRefPic, int32_t iFrameNum,           ERemoveFlag eRemoveFlag);
static PPicture WelsDelLongFromListSetUnref (PRefPic pRefPic, uint32_t uiLongTermFrameIdx, ERemoveFlag eRemoveFlag);

static int32_t MMCOBase (PWelsDecoderContext pCtx, PRefBasePicMarking pRefPicBaseMarking);
static int32_t MMCO (PWelsDecoderContext pCtx, PRefPicMarking pRefPicMarking);
static int32_t MMCOProcess (PWelsDecoderContext pCtx, uint32_t uiMmcoType, bool bRefBasePic,
                            int32_t iShortFrameNum, uint32_t uiLongTermPicNum, int32_t iLongTermFrameIdx, int32_t iMaxLongTermFrameIdx);
static int32_t SlidingWindow (PWelsDecoderContext pCtx);

static int32_t AddShortTermToList (PRefPic pRefPic, PPicture pPic);
static int32_t AddLongTermToList (PRefPic pRefPic, PPicture pPic, int32_t iLongTermFrameIdx);
static int32_t AssignLongTermIdx (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx);
static int32_t MarkAsLongTerm (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx);

} // namespace WelsDec

#endif//WELS_MANAGE_DEC_REF_H__


