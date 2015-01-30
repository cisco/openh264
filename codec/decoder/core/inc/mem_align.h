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
 *	memory alignment utilization
 */

#ifndef WELS_MEM_ALIGN_H__
#define WELS_MEM_ALIGN_H__



#include <stdlib.h>
#include <string.h>
#include "utils.h"

namespace WelsDec {

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus



/*!
*************************************************************************************
* \brief	malloc with zero filled utilization in Wels
*
* \param 	kuiSize	    size of memory block required
*
* \return	allocated memory pointer exactly, failed in case of NULL return
*
* \note	N/A
*************************************************************************************
*/
void* WelsMallocz (const uint32_t kuiSize, const char* kpTag);

/*!
*************************************************************************************
* \brief	free utilization in Wels
*
* \param 	pPtr	data pointer to be free.
*			i.e, uint8_t *pPtr = actual data to be free, argv = &pPtr.
*
* \return	NONE
*
* \note	N/A
*************************************************************************************
*/
void WelsFree (void* pPtr, const char* kpTag);

#define WELS_SAFE_FREE(pPtr, pTag)		if (pPtr) { WelsFree(pPtr, pTag); pPtr = NULL; }

/*
 *	memory operation routines
 */

#ifdef __cplusplus
}
#endif//__cplusplus

} // namespace WelsDec

#endif //WELS_MEM_ALIGN_H__
