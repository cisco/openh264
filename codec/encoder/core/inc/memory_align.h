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

#if !defined(WELS_ENCODER_MEMORY_ALIGN_H__)
#define WELS_ENCODER_MEMORY_ALIGN_H__

#include "typedefs.h"
#include "as264_common.h"
#ifdef MEMORY_CHECK
#include <stdio.h>
#endif//MEMORY_CHECK

namespace WelsEnc {

class CMemoryAlign {
 public:
CMemoryAlign (const uint32_t kuiCacheLineSize);
virtual ~CMemoryAlign();

void* WelsMallocz (const uint32_t kuiSize, const char* kpTag);
void* WelsMalloc (const uint32_t kuiSize, const char* kpTag);
void WelsFree (void* pPointer, const char* kpTag);
const uint32_t WelsGetCacheLineSize() const;
const uint32_t WelsGetMemoryUsage() const;

 private:
// private copy & assign constructors adding to fix klocwork scan issues
CMemoryAlign (const CMemoryAlign& kcMa);
CMemoryAlign& operator= (const CMemoryAlign& kcMa);

 protected:
uint32_t	m_nCacheLineSize;

#ifdef MEMORY_MONITOR
uint32_t	m_nMemoryUsageInBytes;
#endif//MEMORY_MONITOR

#ifdef MEMORY_CHECK
FILE*		m_fpMemChkPoint;
uint32_t	m_nCountRequestNum;
#endif//MEMORY_CHECK
};

}

#endif//WELS_ENCODER_MEMORY_ALIGN_H__
