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

#ifndef TRACE_H_
#define TRACE_H_

#include "typedefs.h"
#include "bit_stream.h"
#include <stdio.h>


#define TRACE_MB_ID(fp, value) \
			{\
			fprintf(fp, "----------MB index:	%d ----------\n", value);\
			fflush(fp);\
			}
#define TRACE_FRAME_ID(fp, value) \
			{\
			fprintf(fp, "----------Frame index:	%d ----------\n", value);\
			fflush(fp);\
			}

#define TRACE_VALUE(fp, value) \
			{\
				fprintf(fp, "(%d)\n", value);\
				fflush(fp);\
			}
#define TRACE_VALUE_2(fp, value1, value2) \
			{\
			fprintf(fp, "(%d,%d)\n", value1, value2);\
			fflush(fp);\
			}

void TraceName(FILE *pFp, int8_t *pName, SBitStringAux *pBs);

void TraceBits(FILE *pFp, uint32_t uiStart, uint32_t uiEnd, SBitStringAux *pBs);


#endif
