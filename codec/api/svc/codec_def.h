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

#ifndef WELS_VIDEO_CODEC_DEFINITION_H__
#define WELS_VIDEO_CODEC_DEFINITION_H__

typedef enum {
  /*rgb color formats*/
  videoFormatRGB        = 1,
  videoFormatRGBA       = 2,
  videoFormatRGB555     = 3,
  videoFormatRGB565     = 4,
  videoFormatBGR        = 5,
  videoFormatBGRA       = 6,
  videoFormatABGR       = 7,
  videoFormatARGB       = 8,

  /*yuv color formats*/
  videoFormatYUY2       = 20,
  videoFormatYVYU       = 21,
  videoFormatUYVY       = 22,
  videoFormatI420       = 23,                        //same as IYUV
  videoFormatYV12       = 24,
  videoFormatInternal   = 25,                        // Only Used for SVC decoder testbed

  videoFormatNV12		  = 26,						// new format for output by DXVA decoding

  videoFormatVFlip      = 0x80000000
} EVideoFormatType;

typedef enum {
  videoFrameTypeInvalid,		/* Encoder not ready or parameters are invalidate */
  videoFrameTypeIDR,		/* This type is only available for H264 if this frame is key frame, then return this type */
  videoFrameTypeI,		/* I frame type */
  videoFrameTypeP,		/* P frame type */
  videoFrameTypeSkip,		/* Skip the frame based encoder kernel */
  videoFrameTypeIPMixed 		/* Frame type introduced I and P slices are mixing */
} EVideoFrameType;

typedef enum {
  cmResultSuccess,
  cmInitParaError,                  /*Parameters are invalid */
  cmUnkonwReason,
  cmMallocMemeError,                /*Malloc a memory error*/
  cmInitExpected,			  /*Initial action is expected*/
  cmUnsupportedData
} CM_RETURN;

/* nal unit type */
enum ENalUnitType {
  NAL_UNKNOWN = 0,
  NAL_SLICE   = 1,
  NAL_SLICE_DPA   = 2,
  NAL_SLICE_DPB   = 3,
  NAL_SLICE_DPC   = 4,
  NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
  NAL_SEI         = 6,    /* ref_idc == 0 */
  NAL_SPS         = 7,
  NAL_PPS         = 8
                    /* ref_idc == 0 for 6,9,10,11,12 */
};
/* NRI: eNalRefIdc */
enum ENalPriority {
  NAL_PRIORITY_DISPOSABLE = 0,
  NAL_PRIORITY_LOW        = 1,
  NAL_PRIORITY_HIGH       = 2,
  NAL_PRIORITY_HIGHEST    = 3
};

#define IS_PARAMETER_SET_NAL(eNalRefIdc, eNalType) \
( (eNalRefIdc == NAL_PRIORITY_HIGHEST) && (eNalType == (NAL_SPS|NAL_PPS) || eNalType == NAL_SPS) )

#define IS_IDR_NAL(eNalRefIdc, eNalType) \
( (eNalRefIdc == NAL_PRIORITY_HIGHEST) && (eNalType == NAL_SLICE_IDR) )

#define FRAME_NUM_PARAM_SET		(-1)
#define FRAME_NUM_IDR			0

/* Error Tools definition */
typedef unsigned short ERR_TOOL;
enum {
  ET_NONE = 0x00,					// NONE Error Tools
  ET_IP_SCALE = 0x01,				// IP Scalable
  ET_FMO = 0x02,					// Flexible Macroblock Ordering
  ET_IR_R1 = 0x04,				// Intra Refresh in predifined 2% MB
  ET_IR_R2 = 0x08,				// Intra Refresh in predifined 5% MB
  ET_IR_R3 = 0x10,				// Intra Refresh in predifined 10% MB
  ET_FEC_HALF = 0x20,				// Forward Error Correction in 50% redundency mode
  ET_FEC_FULL	= 0x40,				// Forward Error Correction in 100% redundency mode
  ET_RFS = 0x80 					// Reference Frame Selection
};

/* information of coded Slice(=NAL)(s) */
typedef struct SliceInformation {
  unsigned char*	pBufferOfSlices;		// base buffer of coded slice(s)
  int				iCodedSliceCount;	// number of coded slices
  unsigned int*	pLengthOfSlices;		// array of slices length accordingly by number of slice
  int				iFecType;			// FEC type[0, 50%FEC, 100%FEC]
  unsigned char	uiSliceIdx;		// index of slice in frame [FMO: 0,..,uiSliceCount-1; No FMO: 0]
  unsigned char	uiSliceCount;		// count number of slice in frame [FMO: 2-8; No FMO: 1]
  char			iFrameIndex;		// index of frame[-1, .., idr_interval-1]
  unsigned char	uiNalRefIdc;		// NRI, priority level of slice(NAL)
  unsigned char	uiNalType;			// NAL type
  unsigned char
  uiContainingFinalNal;	// whether final NAL is involved in buffer of coded slices, flag used in Pause feature in T27
} SliceInfo, *PSliceInfo;



#define CIF_WIDTH		352
#define CIF_HEIGHT		288
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240
#define QCIF_WIDTH		176
#define QCIF_HEIGHT		144
#define SQCIF_WIDTH		128
#define SQCIF_HEIGHT	96

/* thresholds of the initial, maximal and minimal rate */
typedef struct {
  int	iWidth;			// frame width
  int	iHeight;			// frame height
  int	iThresholdOfInitRate;	// threshold of initial rate
  int	iThresholdOfMaxRate;	// threshold of maximal rate
  int	iThresholdOfMinRate;	// threshold of minimal rate
  int iMinThresholdFrameRate;		//min frame rate min
  int	iSkipFrameRate;	//skip to frame rate min
  int iSkipFrameStep;	//how many frames to skip
} SRateThresholds, *PRateThresholds;

typedef struct TagSysMemBuffer {
  int	iWidth;			//width of decoded pic for display
  int iHeight;			//height of decoded pic for display
  int iFormat; 		// type is "EVideoFormatType"
  int iStride[2];		//stride of 2 component
} SSysMEMBuffer;

typedef struct TagBufferInfo {
  int iBufferStatus;  // 0: one frame data is not ready; 1: one frame data is ready
  union {
    SSysMEMBuffer sSystemBuffer;
  } UsrData;
} SBufferInfo;

/* Constants related to transmission rate at various resolutions */
static const SRateThresholds ksRateThrMap[4] = {
  // initial-maximal-minimal
  {CIF_WIDTH, CIF_HEIGHT, 225000, 384000, 96000, 3, 1, 1},		// CIF
  {QVGA_WIDTH, QVGA_HEIGHT, 192000, 320000, 80000, -1, -1, -1},	// QVGA
  {QCIF_WIDTH, QCIF_HEIGHT, 150000, 256000, 64000, 8, 4, 2},		// QCIF
  {SQCIF_WIDTH, SQCIF_HEIGHT, 120000, 192000, 48000, 5, 3, 1}	// SQCIF
};


// In a GOP, multiple of the key frame number, derived from
// the number of layers(index or array below)
static const char kiKeyNumMultiple[] = {
  1, 1, 2, 4, 8, 16,
};

#endif//WELS_VIDEO_CODEC_DEFINITION_H__
