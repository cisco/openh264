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

#include "SceneChangeDetectionCommon.h"
#include "../common/cpu.h"

WELSVP_NAMESPACE_BEGIN


int32_t WelsSampleSad8x8_c( uint8_t * pSrcY, int32_t iSrcStrideY, uint8_t * pRefY, int32_t iRefStrideY )
{
	int32_t iSadSum = 0;
	uint8_t* pSrcA = pSrcY;
	uint8_t* pSrcB = pRefY;
	for (int32_t i = 0; i < 8; i++ )
	{
		iSadSum += WELS_ABS( ( pSrcA[0] - pSrcB[0] ) );
		iSadSum += WELS_ABS( ( pSrcA[1] - pSrcB[1] ) );
		iSadSum += WELS_ABS( ( pSrcA[2] - pSrcB[2] ) );
		iSadSum += WELS_ABS( ( pSrcA[3] - pSrcB[3] ) );
		iSadSum += WELS_ABS( ( pSrcA[4] - pSrcB[4] ) );
		iSadSum += WELS_ABS( ( pSrcA[5] - pSrcB[5] ) );
		iSadSum += WELS_ABS( ( pSrcA[6] - pSrcB[6] ) );
		iSadSum += WELS_ABS( ( pSrcA[7] - pSrcB[7] ) );

		pSrcA += iSrcStrideY;
		pSrcB += iRefStrideY;
	}

	return iSadSum;
} 

WELSVP_NAMESPACE_END
