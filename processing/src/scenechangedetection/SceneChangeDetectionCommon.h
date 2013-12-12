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
 * \file	        :  SceneChangeDetectionCommon.h
 *
 * \brief	    :  scene change detection class of wels video processor class
 *
 * \date         :  2011/03/14
 *
 * \description  :  1. rewrite the package code of scene change detection class  
 *
 */

#ifndef _WELSVP_SCENECHANGEDETECTIONCOMMON_H
#define _WELSVP_SCENECHANGEDETECTIONCOMMON_H

#include "../common/util.h"
#include "../common/memory.h"
#include "../common/WelsFrameWork.h"
#include "../../interface/IWelsVP.h"

WELSVP_NAMESPACE_BEGIN

typedef  int32_t (SadFunc) ( uint8_t * pSrcY, int32_t iSrcStrideY, uint8_t * pRefY, int32_t iRefStrideY );

typedef SadFunc  * SadFuncPtr;

SadFunc      WelsSampleSad8x8_c;

#ifdef X86_ASM
WELSVP_EXTERN_C_BEGIN
SadFunc      WelsSampleSad8x8_sse21;
WELSVP_EXTERN_C_END
#endif

WELSVP_NAMESPACE_END

#endif
