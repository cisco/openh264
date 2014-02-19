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

#include "mem_align.h"

namespace WelsDec {

//#define MEMORY_CHECK
#ifdef MEMORY_CHECK

WelsFileHandle* pMemCheckMalloc = NULL;
WelsFileHandle* pMemCheckFree = NULL;

int32_t iCountMalloc = 0;
#endif
//

void* WelsMalloc (const uint32_t kuiSize, const char* kpTag) {
  const int32_t kiSizeVoidPtr	= sizeof (void**);
  const int32_t kiSizeInt		= sizeof (int32_t);
  const int32_t kiAlignBytes	= 15;
  uint8_t* pBuf		= (uint8_t*) malloc (kuiSize + kiAlignBytes + kiSizeVoidPtr + kiSizeInt);
  uint8_t* pAlignBuf;

#ifdef MEMORY_CHECK
  if (pMemCheckMalloc == NULL) {
    pMemCheckMalloc = WelsFopen ("mem_check_malloc.txt", "at+");
    pMemCheckFree   = WelsFopen ("mem_check_free.txt", "at+");
  }

  if (kpTag != NULL) {
    if (pMemCheckMalloc != NULL) {
      fprintf (pMemCheckMalloc, "0x%x, size: %d       , malloc %s\n", (void*)pBuf,
               (kuiSize + kiAlignBytes + kiSizeVoidPtr + kiSizeInt), kpTag);
    }
    if (pMemCheckMalloc != NULL) {
      fflush (pMemCheckMalloc);
    }
  }
#endif

  if (NULL == pBuf)
    return NULL;

  // to fill zero values
  memset (pBuf, 0, kuiSize + kiAlignBytes + kiSizeVoidPtr + kiSizeInt);

  pAlignBuf = pBuf + kiAlignBytes + kiSizeVoidPtr + kiSizeInt;
  pAlignBuf -= (uintptr_t) pAlignBuf & kiAlignBytes;
  * ((void**) (pAlignBuf - kiSizeVoidPtr)) = pBuf;
  * ((int32_t*) (pAlignBuf - (kiSizeVoidPtr + kiSizeInt))) = kuiSize;

  return (pAlignBuf);
}

/////////////////////////////////////////////////////////////////////////////

void WelsFree (void* pPtr, const char* kpTag) {
  if (pPtr) {
#ifdef MEMORY_CHECK
    if (NULL != pMemCheckFree && kpTag != NULL) {
      fprintf (pMemCheckFree, "0x%x, free %s\n", (void*) (* (((void**) pPtr) - 1)), kpTag);
      fflush (pMemCheckFree);
    }
#endif
    free (* (((void**) pPtr) - 1));
  }
}

/////////////////////////////////////////////////////////////////////////////
} // namespace WelsDec
