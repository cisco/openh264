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

//picture.h	-	reconstruction picture/ reference picture/ residual picture are declared here
#ifndef WELS_PICTURE_H__
#define WELS_PICTURE_H__

#include "typedefs.h"
#include "as264_common.h"
#include "wels_common_basis.h"

namespace WelsSVCEnc {
#define LIST_SIZE			0x10000		//(256*256)
typedef struct TagScreenBlockFeatureStorage
{
	uint16_t*	pFeatureOfBlock;		// Feature of every block (8x8), begin with the point
	uint32_t*	pTimesOfFeatureValue;		// times of every value in Feature
	uint16_t**	pLocationOfFeature;			// uint16_t *pLocationOfFeature[LIST_SIZE], pLocationOfFeature[i] saves all the location(x,y) whose Feature = i;
	uint16_t*	pLocationPointer;	// buffer of position array
	int32_t		iActualListSize;			// actual list size (8x8 based)
} SScreenBlockFeatureStorage;

typedef struct TagScreenContentStorage{
  SScreenBlockFeatureStorage	sRefBlockFeature[MAX_MULTI_REF_PIC_COUNT];
	bool						bRefBlockFeatureCalculated; // flag of whether pre-process is done
	uint8_t				uiFeatureStrategyIndex;// index of hash strategy

	/* for FME frame-level switch */
	bool bFMESwitchFlag;
	uint8_t uiFMEGoodFrameCount;
	int32_t iHighFreMbCount;
}SScreenContentStorage;


/*
 *	Reconstructed Picture definition
 *	It is used to express reference picture, also consequent reconstruction picture for output
 */
typedef struct TagPicture {
  /************************************payload pData*********************************/
  uint8_t*		pBuffer;		// pointer to the first allocated byte, basical offset of pBuffer, dimension:
  uint8_t*		pData[3];		// pointer to picture planes respectively
  int32_t		iLineSize[3];	// iLineSize of picture planes respectively

  // picture information
  /*******************************from other standard syntax****************************/
  /*from pSps*/
  int32_t		iWidthInPixel;	// picture width in pixel
  int32_t		iHeightInPixel;// picture height in pixel
  int32_t		iPictureType;	// got from sSliceHeader(): eSliceType
  int32_t		iFramePoc;		// frame POC

  float			fFrameRate;   // MOVE
  int32_t		iFrameNum;		// frame number			//for pRef pic management

  uint32_t*	uiRefMbType;	// for iMbWidth*iMbHeight
  uint8_t*		pRefMbQp;		// for iMbWidth*iMbHeight

  int32_t*     pMbSkipSad;   //for iMbWidth*iMbHeight

  SMVUnitXY*	sMvList;

  /*******************************sef_definition for misc use****************************/
  int32_t		iMarkFrameNum;
  int32_t		iLongTermPicNum;

  bool		bUsedAsRef;						//for pRef pic management
  bool		bIsLongRef;	// long term reference frame flag	//for pRef pic management
  uint8_t		uiRecieveConfirmed;
  uint8_t		uiTemporalId;
  uint8_t		uiSpatialId;
} SPicture;

/*
 *	Residual Picture
 */
//typedef struct Rs_Picture_s{
//	int16_t		*pBuffer[4];		// base pBuffer
//	int16_t		*pData[4];		// pData pBuffer
//	int32_t		real_linesize[4];// actual iLineSize of picture planes respectively
//	int32_t		used_linesize[4];// iLineSize of picture planes respectively used currently
//	int32_t		planes;			// planes of YUV
//}Rs_Picture_t;

}	// end of namespace WelsSVCEnc {

#endif//WELS_PICTURE_H__
