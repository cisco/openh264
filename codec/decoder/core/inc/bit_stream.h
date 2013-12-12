/*!
 * \copy
 *     Copyright (c)  2004-2013, Cisco Systems
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

 //bit_stream.h	-	bit-stream reading and / writing auxiliary data
#ifndef WELS_BIT_STREAM_H__
#define WELS_BIT_STREAM_H__

#include "typedefs.h"

namespace WelsDec {

/*
 *	Bit-stream auxiliary reading / writing
 */
typedef struct TagBitStringAux {
	uint8_t		*pStartBuf;	// buffer to start position
	uint8_t		*pEndBuf;	// buffer + length
	int32_t     iBits;       // count bits of overall bitstreaming input

	int32_t     iIndex;      //only for cavlc usage
	uint8_t		*pCurBuf;	// current reading position
	uint32_t    uiCurBits;
	int32_t		iLeftBits;	// count number of available bits left ([1, 8]),
	                        // need pointer to next byte start position in case 0 bit left then 8 instead
}SBitStringAux, *PBitStringAux;

//#pragma pack()

/*!
 * \brief	input bits for decoder or initialize bitstream writing in encoder
 *
 * \param	pBitString	Bit string auxiliary pointer
 * \param	kpBuf		bit-stream buffer
 * \param	kiSize	    size in bits for decoder; size in bytes for encoder
 *
 * \return	size of buffer data in byte; failed in -1 return
 */
int32_t InitBits( PBitStringAux pBitString, const uint8_t *kpBuf, const int32_t kiSize );

void_t InitReadBits( PBitStringAux pBitString );

uint32_t EndianFix(uint32_t uiX);



} // namespace WelsDec

#endif//WELS_BIT_STREAM_H__
