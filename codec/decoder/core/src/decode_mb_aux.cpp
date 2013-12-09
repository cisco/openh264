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

#include <string.h>

#include "decode_mb_aux.h"
#include "wels_common_basis.h"

namespace WelsDec {

#define MAX_NEG_CROP 1024
uint8_t g_ClipTable[256 + 2 * MAX_NEG_CROP];	//the front 1024 is 0, the back 1024 is 255, the middle 256 elements is 0-255


/* init pClip table to pClip the final dct data */
void_t InitDctClipTable(void_t)
{
	uint8_t *p		        = &g_ClipTable[0];
	const int32_t kiLength	= MAX_NEG_CROP * sizeof(uint8_t);
	int32_t i               = 0;
	
	do
    {
		const int32_t kiIdx = MAX_NEG_CROP + i;

		p[kiIdx]	= i;
		p[1+kiIdx]	= 1+i;
		p[2+kiIdx]	= 2+i;
		p[3+kiIdx]	= 3+i;

		i += 4;
	} while(i < 256);

	memset( p, 0, kiLength);
	memset( p + MAX_NEG_CROP + 256, 0xFF, kiLength);
}

//NOTE::: p_RS should NOT be modified and it will lead to mismatch with JSVM.
//        so should allocate kA array to store the temporary value (idct).
void_t IdctResAddPred_c(uint8_t *pPred, const int32_t kiStride, int16_t *pRs)
{
	int16_t iSrc[16];

	uint8_t *pDst			= pPred;
	const int32_t kiStride2	= kiStride<<1;
	const int32_t kiStride3	= kiStride + kiStride2;
	uint8_t *pClip			= &g_ClipTable[MAX_NEG_CROP];	
	int32_t i;

	for(i=0; i<4; i++)
	{
 		const int32_t kiY  = i<<2;
		const int32_t kiT0 = pRs[kiY] + pRs[kiY+2];
		const int32_t kiT1 = pRs[kiY] - pRs[kiY+2];
		const int32_t kiT2 = (pRs[kiY+1]>>1) - pRs[kiY+3];
		const int32_t kiT3 = pRs[kiY+1] + (pRs[kiY+3]>>1);

		iSrc[kiY] = kiT0 + kiT3;
		iSrc[kiY+1] = kiT1 + kiT2;
		iSrc[kiY+2] = kiT1 - kiT2;
		iSrc[kiY+3] = kiT0 - kiT3;
	}

	for(i=0; i<4; i++)
	{
		int32_t kT1	= iSrc[i]	+ iSrc[i+8];
		int32_t kT2	= iSrc[i+4] + (iSrc[i+12]>>1);
		int32_t kT3	= (32 + kT1 + kT2) >> 6;
		int32_t kT4	= (32 + kT1 - kT2) >> 6;
		
		pDst[i] = pClip[ kT3 + pPred[i] ];
		pDst[i+kiStride3] = pClip[ kT4 + pPred[i+kiStride3] ];

		kT1	= iSrc[i] - iSrc[i+8];
		kT2	= (iSrc[i+4]>>1) - iSrc[i+12];
		pDst[i+kiStride] = pClip[ ((32 + kT1 + kT2) >> 6) + pDst[i+kiStride] ];
		pDst[i+kiStride2] = pClip[ ((32 + kT1 - kT2) >> 6) + pDst[i+kiStride2] ];
	}
}

void_t GetI4LumaIChromaAddrTable(int32_t *pBlockOffset, const int32_t kiYStride, const int32_t kiUVStride)
{
	int32_t *pOffset	   = pBlockOffset;
	int32_t i;
	const uint8_t kuiScan0 = g_kuiScan8[0];

	for(i=0; i<16; i++)
	{
		const uint32_t kuiA = g_kuiScan8[i] - kuiScan0;
		const uint32_t kuiX = kuiA & 0x07;
		const uint32_t kuiY = kuiA >> 3;

		pOffset[i]= (kuiX + kiYStride* kuiY) << 2;
	}

	for(i=0; i<4; i++)
	{
		const uint32_t kuiA = g_kuiScan8[i] - kuiScan0;

		pOffset[16+i]=
		pOffset[20+i]= ((kuiA & 0x07) + (kiUVStride/*>>1*/) * (kuiA >> 3)) << 2;
	}
}

} // namespace WelsDec