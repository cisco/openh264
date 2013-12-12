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

#ifndef WELS_VIDEO_CODEC_APPLICATION_DEFINITION_H__
#define WELS_VIDEO_CODEC_APPLICATION_DEFINITION_H__

////////////////Data and /or structures introduced in Cisco OpenH264 application////////////////

/* Constants */
#define MAX_TEMPORAL_LAYER_NUM		5
#define MAX_SPATIAL_LAYER_NUM		4
#define MAX_QUALITY_LAYER_NUM		4

#define MAX_LAYER_NUM_OF_FRAME		128
#define MAX_NAL_UNITS_IN_LAYER		128	// predetermined here, adjust it later if need

#define MAX_RTP_PAYLOAD_LEN		1000
#define AVERAGE_RTP_PAYLOAD_LEN		800


#define SAVED_NALUNIT_NUM_TMP		( (MAX_SPATIAL_LAYER_NUM*MAX_QUALITY_LAYER_NUM) + 1 + MAX_SPATIAL_LAYER_NUM ) //SPS/PPS + SEI/SSEI + PADDING_NAL
#define MAX_SLICES_NUM_TMP			( ( MAX_NAL_UNITS_IN_LAYER - SAVED_NALUNIT_NUM_TMP ) / 3 )

typedef enum {
  /* Errors derived from bitstream parsing */
  dsErrorFree			= 0x00,	/* Bitstream error-free */
  dsFramePending		= 0x01,	/* Need more throughput to generate a frame output,  */
  dsRefLost			= 0x02,	/* layer lost at reference frame with temporal id 0  */
  dsBitstreamError	= 0x04,	/* Error bitstreams(maybe broken internal frame) the decoder cared */
  dsDepLayerLost		= 0x08,	/* Dependented layer is ever lost */
  dsNoParamSets		= 0x10, /* No parameter set NALs involved */

  /* Errors derived from logic level */
  dsInvalidArgument	= 0x1000,	/* Invalid argument specified */
  dsInitialOptExpected = 0x2000,	/* Initializing operation is expected */
  dsOutOfMemory		= 0x4000,	/* Out of memory due to new request */
  /* ANY OTHERS? */
  dsDstBufNeedExpand	= 0x8000	/* Actual picture size exceeds size of dst pBuffer feed in decoder, so need expand its size */

} DECODING_STATE;

/* Option types introduced in SVC encoder application */
typedef enum {
  ENCODER_OPTION_DATAFORMAT = 0,
  ENCODER_OPTION_IDR_INTERVAL,
  ENCODER_OPTION_SVC_ENCODE_PARAM,
  ENCODER_OPTION_FRAME_RATE,
  ENCODER_OPTION_iBitRate,
  ENCODER_OPTION_INTER_SPATIAL_PRED,
  ENCODER_OPTION_RC_MODE,
  ENCODER_PADDING_PADDING,

  ENCODER_LTR_RECOVERY_REQUEST,
  ENCODER_LTR_MARKING_FEEDBACK,
  ENCOCER_LTR_MARKING_PERIOD,
  ENCODER_OPTION_LTR,

  ENCODER_OPTION_ENABLE_SSEI,               //disable SSEI: true--disable ssei; false--enable ssei
  ENCODER_OPTION_ENABLE_PREFIX_NAL_ADDING,   //enable prefix: true--enable prefix; false--disable prefix
  ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, //disable pSps/pPps id addition: true--disable pSps/pPps id; false--enable pSps/pPps id addistion

  ENCODER_OPTION_CURRENT_PATH
} ENCODER_OPTION;

/* Option types introduced in SVC decoder application */
typedef enum {
  DECODER_OPTION_DATAFORMAT = 0,	/* Set color space of decoding output frame */
  DECODER_OPTION_TRUNCATED_MODE,	/* Used in decoding bitstream of non integrated frame, only truncated working mode is supported by tune, so skip it */
  DECODER_OPTION_END_OF_STREAM,	/* Indicate bitstream of the final frame to be decoded */
  DECODER_OPTION_VCL_NAL,        //feedback whether or not have VCL NAL in current AU for application layer
  DECODER_OPTION_TEMPORAL_ID,      //feedback temporal id for application layer
  DECODER_OPTION_MODE,             // indicates the decoding mode
  DECODER_OPTION_OUTPUT_PROPERTY,
  DECODER_OPTION_FRAME_NUM,	//feedback current decoded frame number
  DECODER_OPTION_IDR_PIC_ID,	// feedback current frame belong to which IDR period
  DECODER_OPTION_LTR_MARKING_FLAG,	// feedback wether current frame mark a LTR
  DECODER_OPTION_LTR_MARKED_FRAME_NUM,	// feedback frame num marked by current Frame
  DECODER_OPTION_DEVICE_INFO,

} DECODER_OPTION;
typedef enum { //feedback that whether or not have VCL NAL in current AU
  FEEDBACK_NON_VCL_NAL = 0,
  FEEDBACK_VCL_NAL,
  FEEDBACK_UNKNOWN_NAL
} FEEDBACK_VCL_NAL_IN_AU;
typedef enum { //feedback the iTemporalId in current AU if have VCL NAL
  FEEDBACK_TEMPORAL_ID_0 = 0,
  FEEDBACK_TEMPORAL_ID_1,
  FEEDBACK_TEMPORAL_ID_2,
  FEEDBACK_TEMPORAL_ID_3,
  FEEDBACK_TEMPORAL_ID_4,
  FEEDBACK_UNKNOWN_TEMPORAL_ID
} FEEDBACK_TEMPORAL_ID;

/* Type of layer being encoded */
typedef enum {
  NON_VIDEO_CODING_LAYER = 0,
  VIDEO_CODING_LAYER = 1
} LAYER_TYPE;

/* SVC Encoder/Decoder Initializing Parameter Types */
typedef enum {
  INIT_TYPE_PARAMETER_BASED = 0,	// For SVC DEMO Application
  INIT_TYPE_CONFIG_BASED,			// For SVC CONSOLE Application
} INIT_TYPE;

//enumerate the type of video bitstream which is provided to decoder
typedef enum {
  VIDEO_BITSTREAM_AVC               = 0,
  VIDEO_BITSTREAM_SVC               = 1,
  VIDEO_BITSTREAM_DEFAULT           = VIDEO_BITSTREAM_SVC,
} VIDEO_BITSTREAM_TYPE;

typedef enum {
  NO_RECOVERY_REQUSET  = 0,
  LTR_RECOVERY_REQUEST = 1,
  IDR_RECOVERY_REQUEST = 2,
  NO_LTR_MARKING_FEEDBACK = 3,
  LTR_MARKING_SUCCESS = 4,
  LTR_MARKING_FAILED = 5,
} KEY_FRAME_REQUEST_TYPE;

typedef struct {
  unsigned int uiFeedbackType; //IDR request or LTR recovery request
  unsigned int uiIDRPicId; // distinguish request from different IDR
  int		  iLastCorrectFrameNum;
  int		  iCurrentFrameNum; //specify current decoder frame_num.
} SLTRRecoverRequest;

typedef struct {
  unsigned int  uiFeedbackType; //mark failed or successful
  unsigned int  uiIDRPicId; // distinguish request from different IDR
  int			  iLTRFrameNum; //specify current decoder frame_num
} SLTRMarkingFeedback;
#pragma pack(1)

typedef struct {

  //# 0 SM_SINGLE_SLICE			| SliceNum==1
  //# 1 SM_FIXEDSLCNUM_SLICE	| according to SliceNum			| Enabled dynamic slicing for multi-thread
  //# 2 SM_RASTER_SLICE			| according to SlicesAssign		| Need input of MB numbers each slice. In addition, if other constraint in SSliceArgument is presented, need to follow the constraints. Typically if MB num and slice size are both constrained, re-encoding may be involved.
  //# 3 SM_ROWMB_SLICE			| according to PictureMBHeight	|  Typical of single row of mbs each slice?+ slice size constraint which including re-encoding
  //# 4 SM_DYN_SLICE			| according to SliceSize		| Dynamic slicing (have no idea about slice_nums until encoding current frame)
  unsigned int uiSliceMode; //by default, uiSliceMode will be 0
  struct {
    unsigned int
    uiSliceMbNum[MAX_SLICES_NUM_TMP];  //here we use a tmp fixed value since MAX_SLICES_NUM is not defined here and its definition may be changed;
    unsigned int		uiSliceNum;
    unsigned int		uiSliceSizeConstraint;
  } sSliceArgument;//not all the elements in this argument will be used, how it will be used depends on uiSliceMode; see below
} SSliceConfig;

typedef struct {
  int	iVideoWidth;		// video size in cx specified for a layer
  int	iVideoHeight;		// video size in cy specified for a layer
  float	fFrameRate;		// frame rate specified for a layer
  int	iQualityLayerNum;	// layer number at quality level
  int	iSpatialBitrate;	// target bitrate for a spatial layer
  int	iCgsSnrRefined;	// 0: SNR layers all MGS; 1: SNR layers all CGS
  int	iInterSpatialLayerPredFlag;	// 0: diabled [independency spatial layer coding]; 1: enabled [base spatial layer dependency coding]

  int	iQualityBitrate[MAX_QUALITY_LAYER_NUM];	// target bitrate for a quality layer

  SSliceConfig sSliceCfg;
} SSpatialLayerConfig;

/* SVC Encoding Parameters */
typedef struct {
  int		iPicWidth;			// width of picture in samples
  int		iPicHeight;			// height of picture in samples
  int		iTargetBitrate;		// target bitrate desired
  int		iTemporalLayerNum;	// layer number at temporal level
  int		iSpatialLayerNum;	// layer number at spatial level

  float	fFrameRate;			// input maximal frame rate

  int		iInputCsp;			// color space of input sequence
  int		iKeyPicCodingMode;// mode of key picture coding
  int		iIntraPeriod;		// period of Intra frame
  bool    bEnableSpsPpsIdAddition;
  bool    bPrefixNalAddingCtrl;
  bool   	bEnableDenoise;	    // denoise control
  bool    bEnableBackgroundDetection; 	// background detection control //VAA_BACKGROUND_DETECTION //BGD cmd
  bool    bEnableAdaptiveQuant; // adaptive quantization control
  bool	bEnableCropPic;	// enable cropping source picture.  8/25/2010
  // FALSE: Streaming Video Sharing; TRUE: Video Conferencing Meeting;
  bool     bEnableLongTermReference; // 0: on, 1: off
  int     iLtrMarkPeriod;

  int iRCMode;                 // RC mode
  int	iTemporalBitrate[MAX_TEMPORAL_LAYER_NUM];	// target bitrate specified for a temporal level
  int iPaddingFlag;            // 0:disable padding;1:padding

  SSpatialLayerConfig sSpatialLayers[MAX_SPATIAL_LAYER_NUM];

} SVCEncodingParam, *PSVCEncodingParam;

//Define a new struct to show the property of video bitstream.
typedef struct {
  unsigned int          size; //size of the struct
  VIDEO_BITSTREAM_TYPE  eVideoBsType;
} SVideoProperty;

/* SVC Decoding Parameters, reserved here and potential applicable in the future */
typedef struct TagSVCDecodingParam {
  char*		pFileNameRestructed;	// File name of restructed frame used for PSNR calculation based debug

  int				iOutputColorFormat;	// color space format to be outputed, EVideoFormatType specified in codec_def.h
  unsigned int	uiCpuLoad;		// CPU load
  unsigned char	uiTargetDqLayer;	// Setting target dq layer id

  unsigned char	uiEcActiveFlag;		// Whether active error concealment feature in decoder

  SVideoProperty   sVideoProperty;
} SDecodingParam, *PDecodingParam;

/* Bitstream inforamtion of a layer being encoded */
typedef struct {
  unsigned char uiTemporalId;
  unsigned char uiSpatialId;
  unsigned char uiQualityId;

  unsigned char uiPriorityId; //ignore it currently

  unsigned char uiLayerType;

  int	iNalCount;					// Count number of NAL coded already
  int	iNalLengthInByte[MAX_NAL_UNITS_IN_LAYER];	// Length of NAL size in byte from 0 to iNalCount-1
  unsigned char*	pBsBuf;		// Buffer of bitstream contained
} SLayerBSInfo, *PLayerBSInfo;


typedef struct {
  int		iTemporalId;	// Temporal ID
  unsigned char	uiFrameType;

  int		iLayerNum;
  SLayerBSInfo	sLayerInfo[MAX_LAYER_NUM_OF_FRAME];

} SFrameBSInfo, *PFrameBSInfo;

typedef struct Source_Picture_s {
  int		    iColorFormat;	// color space type
  int  		iStride[4];		// stride for each plane pData
  unsigned char*  pData[4];		// plane pData
  int  		iPicWidth;				// luma picture width in x coordinate
  int 		iPicHeight;				// luma picture height in y coordinate
} SSourcePicture;


#pragma pack()
#endif//WELS_VIDEO_CODEC_APPLICATION_DEFINITION_H__
