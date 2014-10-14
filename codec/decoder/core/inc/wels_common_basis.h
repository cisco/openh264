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

//wels_common_basis.h
#ifndef WELS_COMMON_BASIS_H__
#define WELS_COMMON_BASIS_H__

#include "typedefs.h"
#include "macros.h"

#include "wels_common_defs.h"

using namespace WelsCommon;

namespace WelsDec {

/*common use table*/
extern const uint8_t g_kuiScan8[24];
extern const uint8_t g_kuiLumaDcZigzagScan[16];
extern const uint8_t g_kuiChromaDcScan[4];
/* Profile IDC */
typedef uint8_t		ProfileIdc;
enum {
PRO_BASELINE	= 66,
PRO_MAIN		= 77,
PRO_EXTENDED	= 88,
PRO_HIGH		= 100,
PRO_HIGH10		= 110,
PRO_HIGH422		= 122,
PRO_HIGH444		= 144,
PRO_CAVLC444	= 244,

PRO_SCALABLE_BASELINE	= 83,
PRO_SCALABLE_HIGH		= 86
};

/* Picture Size */
typedef struct TagPictureSize {
int32_t	iWidth;
int32_t iHeight;
} SPictureSize;


/* Position Offset structure */
typedef struct TagPosOffset {
int32_t	iLeftOffset;
int32_t	iTopOffset;
int32_t	iRightOffset;
int32_t	iBottomOffset;
} SPosOffset;

enum EMbPosition { //
MB_LEFT     = 0x01,	// A
MB_TOP      = 0x02,	// B
MB_TOPRIGHT = 0x04,	// C
MB_TOPLEFT	= 0x08,	// D,
MB_PRIVATE  = 0x10
};
/* MB Type & Sub-MB Type */
typedef int32_t MbType;
typedef int32_t SubMbType;

#define MB_TYPE_INTRA4x4       0x01
#define MB_TYPE_INTRA16x16     0x02
#define MB_TYPE_INTRA8x8       0x03
#define MB_TYPE_INTRA_PCM      0x04

#define MB_TYPE_INTRA_BL       0x05// I_BL new MB type

#define MB_TYPE_16x16          0x06
#define MB_TYPE_16x8           0x07
#define MB_TYPE_8x16           0x08
#define MB_TYPE_8x8            0x09
#define MB_TYPE_8x8_REF0       0x0a

#define SUB_MB_TYPE_8x8        0x0b
#define SUB_MB_TYPE_8x4        0x0c
#define SUB_MB_TYPE_4x8        0x0d
#define SUB_MB_TYPE_4x4        0x0e
#define MB_TYPE_SKIP           0x0f
#define MB_TYPE_DIRECT2        0x10
#define not_available		   0x20

#define IS_INTRA4x4(type) ( MB_TYPE_INTRA4x4 == (type) )
#define IS_INTRA16x16(type) ( MB_TYPE_INTRA16x16 == (type) )
#define IS_INTRA(type) ( (type) > 0 && (type) < 5 )
#define IS_INTER(type) ( (type) > 5 && (type) < 16 )

#define IS_I_BL(type) ( (type) == MB_TYPE_INTRA_BL )
#define IS_SUB8x8(type) (MB_TYPE_8x8 == (type) || MB_TYPE_8x8_REF0 == (type))



} // namespace WelsDec

#endif//WELS_COMMON_BASIS_H__
