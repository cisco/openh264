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

//wels_const.h
#ifndef WELS_CONSTANCE_H__
#define WELS_CONSTANCE_H__

// Miscellaneous sizing infos
#ifndef MAX_FNAME_LEN
#define MAX_FNAME_LEN		256	// maximal length of file name in char size
#endif//MAX_FNAME_LEN

#ifndef WELS_LOG_BUF_SIZE
#define WELS_LOG_BUF_SIZE	4096
#endif//WELS_LOG_BUF_SIZE

#ifndef MAX_TRACE_LOG_SIZE
#define MAX_TRACE_LOG_SIZE	(50 * (1<<20))	// max trace log size: 50 MB, overwrite occur if log file size exceeds this size
#endif//MAX_TRACE_LOG_SIZE

/* MB width in pixels for specified colorspace I420 usually used in codec */
#define MB_WIDTH_LUMA		16
#define MB_WIDTH_CHROMA		(MB_WIDTH_LUMA>>1)
/* MB height in pixels for specified colorspace I420 usually used in codec */
#define MB_HEIGHT_LUMA		16
#define MB_HEIGHT_CHROMA	(MB_HEIGHT_LUMA>>1)

/* Some list size */
#define MB_COEFF_LIST_SIZE	(256+((MB_WIDTH_CHROMA*MB_HEIGHT_CHROMA)<<1))

#define MB_PARTITION_SIZE		4	// Macroblock partition size in 8x8 sub-blocks
#define MB_SUB_PARTITION_SIZE	4	// Sub partition size in a 8x8 sub-block
#define MB_BLOCK4x4_NUM				16
#define MB_BLOCK8x8_NUM				4

#define NAL_UNIT_HEADER_EXT_SIZE	3	// Size of NAL unit header for extension in byte

#define MAX_SPS_COUNT			32	// Count number of SPS
#define MAX_PPS_COUNT 			256	// Count number of PPS

#define MAX_FRAME_RATE			30	// maximal frame rate to support
#define MIN_FRAME_RATE			1	// minimal frame rate need support

#define MAX_REF_PIC_COUNT		16		// MAX Short + Long reference pictures
#define MIN_REF_PIC_COUNT		1		// minimal count number of reference pictures, 1 short + 2 key reference based?
#define MAX_SHORT_REF_COUNT		16		// maximal count number of short reference pictures
#define MAX_LONG_REF_COUNT		16		// maximal count number of long reference pictures

#define MAX_MMCO_COUNT			66

#define MAX_SLICEGROUP_IDS		8	// Count number of Slice Groups

#define ALIGN_RBSP_LEN_FIX		4


#define BASE_QUALITY_ID			0
//#define BASE_DEPENDENCY_ID		0
#define BASE_DQ_ID				0
#define MAX_DQ_ID				((uint8_t)-1)
#define MAX_LAYER_NUM   8

#define LAYER_NUM_EXCHANGEABLE	1

#define MAX_NAL_UNIT_NUM_IN_AU	32	// predefined maximal number of NAL Units in an access unit
#define MAX_ACCESS_UNIT_CAPACITY	1048576	// Maximal AU capacity in bytes: (1<<20) = 1024 KB predefined
#define BS_BUFFER_SIZE  (MAX_ACCESS_UNIT_CAPACITY * 3) //for delay case, keep three AU size to prevent buffer overwrite
#define MAX_MACROBLOCK_CAPACITY 5000 //Maximal legal MB capacity, 15000 bits is enough

#endif//WELS_CONSTANCE_H__
