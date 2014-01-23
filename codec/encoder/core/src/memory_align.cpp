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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "memory_align.h"
#include "macros.h"

namespace WelsSVCEnc {

CMemoryAlign::CMemoryAlign (const uint32_t kuiCacheLineSize)
#ifdef MEMORY_MONITOR
  :	m_nMemoryUsageInBytes (0)
#endif//MEMORY_MONITOR
{
  if ((kuiCacheLineSize == 0) || (kuiCacheLineSize & 0x0f))
    m_nCacheLineSize	= 0x10;
  else
    m_nCacheLineSize	= kuiCacheLineSize;

#ifdef MEMORY_CHECK
  m_fpMemChkPoint		= fopen ("./enc_mem_check_point.txt",  "wt+");
  m_nCountRequestNum	= 0;
#endif//MEMORY_CHECK
}

CMemoryAlign::~CMemoryAlign() {
#ifdef MEMORY_MONITOR
  assert (m_nMemoryUsageInBytes == 0);
#endif//MEMORY_MONITOR

#ifdef MEMORY_CHECK
  fclose (m_fpMemChkPoint);
  m_fpMemChkPoint = NULL;

  m_nCountRequestNum	= 0;
#endif//MEMORY_CHECK
}

void* CMemoryAlign::WelsMallocz (const uint32_t kuiSize, const str_t* kpTag) {
  void* pPointer = WelsMalloc (kuiSize, kpTag);
  if (NULL == pPointer) {
    return NULL;
  }
  // zero memory
  memset (pPointer, 0, kuiSize);

  return pPointer;
}

void* CMemoryAlign::WelsMalloc (const uint32_t kuiSize, const str_t* kpTag) {
  const int32_t kiSizeOfVoidPointer	= sizeof (void**);
  const int32_t kiSizeOfInt				= sizeof (int32_t);
  const int32_t kiAlignedBytes		= m_nCacheLineSize - 1;
  const int32_t kiTrialRequestedSize	= kuiSize + kiAlignedBytes + kiSizeOfVoidPointer + kiSizeOfInt;
  const int32_t kiActualRequestedSize	= kiTrialRequestedSize;
  const uint32_t kiPayloadSize			= kuiSize;

  uint8_t* pBuf		= (uint8_t*) malloc (kiActualRequestedSize);
#ifdef MEMORY_CHECK
  if (m_fpMemChkPoint != NULL) {
    if (kpTag != NULL)
      fprintf (m_fpMemChkPoint, "WelsMalloc(), 0x%x : actual uiSize:\t%d\tbytes, input uiSize: %d bytes, %d - %s\n",
               (void*)pBuf, kiActualRequestedSize, kuiSize, m_nCountRequestNum++, kpTag);
    else
      fprintf (m_fpMemChkPoint, "WelsMalloc(), 0x%x : actual uiSize:\t%d\tbytes, input uiSize: %d bytes, %d \n", (void*)pBuf,
               kiActualRequestedSize, kuiSize, m_nCountRequestNum++);
    fflush (m_fpMemChkPoint);
  }
#endif
  uint8_t* pAlignedBuffer;

  if (NULL == pBuf)
    return NULL;

  pAlignedBuffer = pBuf + kiAlignedBytes + kiSizeOfVoidPointer + kiSizeOfInt;
  pAlignedBuffer -= ((uintptr_t) pAlignedBuffer & kiAlignedBytes);
  * ((void**) (pAlignedBuffer - kiSizeOfVoidPointer)) = pBuf;
  * ((int32_t*) (pAlignedBuffer - (kiSizeOfVoidPointer + kiSizeOfInt))) = kiPayloadSize;

#ifdef MEMORY_MONITOR
  m_nMemoryUsageInBytes += kiActualRequestedSize;
#endif//MEMORY_MONITOR

  return pAlignedBuffer;
}

void CMemoryAlign::WelsFree (void* pPointer, const str_t* kpTag) {
  if (pPointer) {
#ifdef MEMORY_MONITOR
    const int32_t kiMemoryLength = * ((int32_t*) ((uint8_t*)pPointer - sizeof (void**) - sizeof (
                                        int32_t))) + m_nCacheLineSize - 1 + sizeof (void**) + sizeof (int32_t);
    m_nMemoryUsageInBytes -= kiMemoryLength;
#endif//MEMORY_MONITOR
#ifdef MEMORY_CHECK
    if (m_fpMemChkPoint != NULL) {
      if (kpTag != NULL)
        fprintf (m_fpMemChkPoint, "WelsFree(), 0x%x - %s: \t%d\t bytes \n", (void*) (* (((void**) pPointer) - 1)), kpTag,
                 kiMemoryLength);
      else
        fprintf (m_fpMemChkPoint, "WelsFree(), 0x%x \n", (void*) (* (((void**) pPointer) - 1)));
      fflush (m_fpMemChkPoint);
    }
#endif
    free (* (((void**) pPointer) - 1));
  }
}

const uint32_t CMemoryAlign::WelsGetCacheLineSize() const {
  return m_nCacheLineSize;
}

#if defined(MEMORY_MONITOR)
const uint32_t CMemoryAlign::WelsGetMemoryUsage() const {
  return m_nMemoryUsageInBytes;
}
#endif//MEMORY_MONITOR

} // end of namespace WelsSVCEnc
