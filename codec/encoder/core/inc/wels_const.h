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

#include "as264_common.h"	//  to communicate with specific macros there, 3/18/2010
#include "codec_app_def.h"

/* To control number of spatial, quality and temporal layers constraint by application layer? */
#define NUM_SPATIAL_LAYERS_CONSTRAINT
#define NUM_QUALITY_LAYERS_CONSTRAINT


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
#define MB_REQ_LUMA_CACHE_SIZE	40	// 8x5 Size of MB cache only luma component required to store
#define MB_REQ_ALL_CACHE_SIZE	48	// 8x6 Size of MB cache all components required to store
#define MB_LUMA_CACHE_SIZE		26	// 5x5+1
#define MB_CHROMA_CACHE_SIZE	10	// 3x3+1

#define MB_PARTITION_SIZE		4	// Macroblock partition size in 8x8 sub-blocks
#define MB_SUB_PARTITION_SIZE	4	// Sub partition size in a 8x8 sub-block
#define MB_BLOCK4x4_NUM				16
#define INTRA_4x4_MODE_NUM		8
#define MB_BLOCK8x8_NUM				4
#define MB_LUMA_CHROMA_BLOCK4x4_NUM  24

#define NAL_UNIT_HEADER_SVC_EXT_SIZE	3	// Size of NAL unit header for SVC extension in byte

#define MAX_SPS_COUNT			32	// Count number of SPS
#define MAX_PPS_COUNT_LIMITED 	57// limit the max ID of PPS because of known limitation of receiver endpoints
#define MAX_PPS_COUNT 			(MAX_PPS_COUNT_LIMITED)//in Standard is 256	// Count number of PPS

#define PARA_SET_TYPE			3 // SPS+PPS
#define PARA_SET_TYPE_AVCSPS	0
#define PARA_SET_TYPE_SUBSETSPS	1
#define PARA_SET_TYPE_PPS		2

#define MAX_FRAME_RATE			30	// maximal frame rate to support
#define MIN_FRAME_RATE			1	// minimal frame rate need support

#define MAX_BIT_RATE			INT_MAX	// maximal bit rate to support
//TODO {Sijia}: 30fps*MaxCPB in level5.1 = 30*240000*1000bits = 7 200 000 000, larger than INT_MAX which is 2147483647, but this is also very big and abnormal number, should figure out a reasonable number after discussion
#define MIN_BIT_RATE			1	// minimal bit rate need support

#define SVC_QUALITY_BASE_QP		26
#define SVC_QUALITY_DELTA_QP	(-3)

#define MAX_SLICEGROUP_IDS		8	// Count number of SSlice Groups
#define MAX_THREADS_NUM			4	// assume to support up to 4 logical cores(threads)

#define ALIGN_RBSP_LEN_FIX		4

#define PADDING_LENGTH			32 // reference extension
#define INTPEL_NEEDED_MARGIN	(3)  // for safe sub-pel MC

#define I420_PLANES				3

#define COMPRESS_RATIO_THR (1.0f)	//set to size of the original data, which will be large enough considering MinCR

#if !defined(SSEI_BUFFER_SIZE)
#define SSEI_BUFFER_SIZE	128
#endif//SSEI_BUFFER_SIZE

#if !defined(SPS_BUFFER_SIZE)
#define SPS_BUFFER_SIZE		32
#endif//SPS_BUFFER_SIZE

#if !defined(PPS_BUFFER_SIZE)
#define PPS_BUFFER_SIZE		16
#endif//PPS_BUFFER_SIZE

#if !defined(MAX_MACROBLOCK_SIZE_IN_BYTE)
#define MAX_MACROBLOCK_SIZE_IN_BYTE		800 //3200*2/8
#endif

#if defined(NUM_SPATIAL_LAYERS_CONSTRAINT)
#define MAX_DEPENDENCY_LAYER		MAX_SPATIAL_LAYER_NUM	// Maximal dependency layer
#else
#define MAX_DEPENDENCY_LAYER		8	// Maximal dependency layer
#endif//NUM_SPATIAL_LAYERS_CONSTRAINT

//The max temporal level support is equal or less than MAX_TEMPORAL_LAYER_NUM defined @ codec_app_def.h
#define MAX_TEMPORAL_LEVEL		MAX_TEMPORAL_LAYER_NUM	// Maximal temporal level

#if defined(NUM_QUALITY_LAYERS_CONSTRAINT)
#define MAX_QUALITY_LEVEL		MAX_QUALITY_LAYER_NUM		// Maximal quality level
#else
#define MAX_QUALITY_LEVEL		16	// Maximal quality level
#endif//NUM_QUALITY_LAYERS_CONSTRAINT

#if defined(MAX_GOP_SIZE)
#undef MAX_GOP_SIZE
#endif//MAX_GOP_SIZE
#define MAX_GOP_SIZE	(1<<(MAX_TEMPORAL_LEVEL-1))

#define MAX_SHORT_REF_COUNT		(MAX_GOP_SIZE>>1) // 16 in standard, maximal count number of short reference pictures
#define LONG_TERM_REF_NUM       2
#define MAX_LONG_REF_COUNT		2 // 16 in standard, maximal count number of long reference pictures
#define MAX_REF_PIC_COUNT		16 // 32 in standard, maximal Short + Long reference pictures
#define MIN_REF_PIC_COUNT		1		// minimal count number of reference pictures, 1 short + 2 key reference based?
//#define TOTAL_REF_MINUS_HALF_GOP	1	// last t0 in last gop
#define MAX_MMCO_COUNT			66

// adjusted numbers reference picture functionality related definition
#define MAX_REFERENCE_MMCO_COUNT_NUM		4	// adjusted MAX_MMCO_COUNT(66 in standard) definition per encoder design
#define MAX_REFERENCE_REORDER_COUNT_NUM		2	// adjusted MAX_REF_PIC_COUNT(32 in standard) for reference reordering definition per encoder design
#define MAX_REFERENCE_PICTURE_COUNT_NUM		(MAX_SHORT_REF_COUNT+MAX_LONG_REF_COUNT)	// <= MAX_REF_PIC_COUNT, memory saved if <

#define BASE_QUALITY_ID			0
#define BASE_DEPENDENCY_ID		0
#define BASE_DQ_ID				0
#define MAX_DQ_ID				((uint8_t)-1)
#define MAX_DQ_LAYER_NUM		(MAX_DEPENDENCY_LAYER/**MAX_QUALITY_LEVEL*/)

#define UNAVAILABLE_DQ_ID		((uint8_t)(-1))
#define LAYER_NUM_EXCHANGEABLE	2

#define MAX_NAL_UNIT_NUM_IN_AU	256	// predefined maximal number of NAL Units in an access unit
#define MAX_ACCESS_UINT_CAPACITY	(1<<20)	// Maximal AU capacity in bytes: 1024 KB predefined
#define MAX_ACCESS_UNIT_CACHE_NUM	2	// Maximal Access Unit(AU) cache number to be processed, denote current AU and the next coming AU.
enum {
  CUR_AU_IDX	= 0,			// index symbol for current access unit
  SUC_AU_IDX	= 1				// index symbol for successive access unit
};

enum {
  BASE_MB = 0,
  AVC_REWRITE_ENHANCE_MB = 1,
  NON_AVC_REWRITE_ENHANCE_MB = 2
};

enum {
  ENC_RETURN_SUCCESS = 0,
  ENC_RETURN_MEMALLOCERR = 0x01, //will free memory and uninit
  ENC_RETURN_UNSUPPORTED_PARA = 0x02, //unsupported setting
  ENC_RETURN_UNEXPECTED = 0x04, //unexpected value
  ENC_RETURN_CORRECTED = 0x08, //unexpected value but corrected by encoder
  ENC_RETURN_INVALIDINPUT = 0x10, //invalid input
  ENC_RETURN_MEMOVERFLOWFOUND = 0x20,
};
//TODO: need to complete the return checking in encoder and fill in more types if needed

#endif//WELS_CONSTANCE_H__
