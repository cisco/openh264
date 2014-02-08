/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
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
 *
 * \file	slice_segment.c
 *
 * \brief	SSlice segment routine (Single slice/multiple slice/fmo arrangement exclusive)
 *
 * \date	2/4/2009 Created
 *
 *************************************************************************************
 */
#include <string.h>
#include "rc.h"

namespace WelsSVCEnc {
/*!
 * \brief	Assign MB map for single slice segment
 *
 * \param	pMbMap			overall MB map
 * \param	iCountMbNum	count number of MB
 *
 * \return	0 - successful; none 0 - failed
 */
int32_t AssignMbMapSingleSlice (void* pMbMap, const int32_t kiCountMbNum, const int32_t kiMapUnitSize) {
  if (NULL == pMbMap || kiCountMbNum <= 0)
    return 1;

  memset (pMbMap, 0, kiCountMbNum * kiMapUnitSize);

  return 0;
}

/*!
 * \brief	Assign MB map for multiple slice(s) segment
 *
 * \param	pMbMap			overall MB map
 * \param	iCountMbNum	count number of MB
 *
 * \return	0 - successful; none 0 - failed
 */
int32_t AssignMbMapMultipleSlices (SSliceCtx* pSliceSeg, const SMulSliceOption* kpMso) {
  if (NULL == pSliceSeg || SM_SINGLE_SLICE == pSliceSeg->uiSliceMode)
    return 1;

  if (SM_ROWMB_SLICE == pSliceSeg->uiSliceMode) {
    const int32_t kiMbWidth	= pSliceSeg->iMbWidth;
    int32_t iSliceNum = pSliceSeg->iSliceNumInFrame, uiSliceIdx = 0;

    while (uiSliceIdx < iSliceNum) {
      const int16_t kiFirstMb = uiSliceIdx * kiMbWidth;
      pSliceSeg->pCountMbNumInSlice[uiSliceIdx]	= kiMbWidth;
      pSliceSeg->pFirstMbInSlice[uiSliceIdx]		= kiFirstMb;
      memset (pSliceSeg->pOverallMbMap + kiFirstMb, (uint8_t)uiSliceIdx, kiMbWidth * sizeof (uint8_t));
      ++ uiSliceIdx;
    }

    return 0;
  } else if (SM_RASTER_SLICE  == pSliceSeg->uiSliceMode ||
             SM_FIXEDSLCNUM_SLICE == pSliceSeg->uiSliceMode) {
    const int32_t* kpSlicesAssignList				= (int32_t*) & (kpMso->sSliceArgument.uiSliceMbNum[0]);
    const int32_t kiCountNumMbInFrame		= pSliceSeg->iMbNumInFrame;
    const int32_t kiCountSliceNumInFrame	= pSliceSeg->iSliceNumInFrame;
    int32_t iSliceIdx						= 0;
    int16_t iMbIdx							= 0;

    do {
      const int32_t kiCurRunLength	= kpSlicesAssignList[iSliceIdx];
      int32_t iRunIdx					= 0;

      pSliceSeg->pFirstMbInSlice[iSliceIdx]			= iMbIdx;
      pSliceSeg->pCountMbNumInSlice[iSliceIdx]		= kiCurRunLength;

      // due here need check validate mb_assign_map for input pData, can not use memset
      do {
        pSliceSeg->pOverallMbMap[iMbIdx + iRunIdx]	= iSliceIdx;
        ++ iRunIdx;
      } while (iRunIdx < kiCurRunLength && iMbIdx + iRunIdx < kiCountNumMbInFrame);

      iMbIdx += kiCurRunLength;
      ++ iSliceIdx;
    } while (iSliceIdx < kiCountSliceNumInFrame && iMbIdx < kiCountNumMbInFrame);
  } else if (SM_DYN_SLICE == pSliceSeg->uiSliceMode) {
  } else {	// any else uiSliceMode?
    assert (0);
  }

  // extention for other multiple slice type in the future
  return 1;
}

/*!
 *  Check slices assignment setttings on MST_INTERLEAVE type
 */

//slice parameter check for SM_FIXEDSLCNUM_SLICE
bool_t CheckFixedSliceNumMultiSliceSetting (const int32_t kiMbNumInFrame, SSliceArgument* pSliceArg) {
  int32_t* pSlicesAssignList		= (int32_t*) & (pSliceArg->uiSliceMbNum[0]);
  const uint32_t kuiSliceNum			= pSliceArg->iSliceNum;
  uint32_t uiSliceIdx				= 0;
  const int32_t kiMbNumPerSlice	= kiMbNumInFrame / kuiSliceNum;
  int32_t iNumMbLeft				= kiMbNumInFrame;

  if (NULL == pSlicesAssignList)
    return false;

  for (; uiSliceIdx + 1 < kuiSliceNum; ++ uiSliceIdx) {
    pSlicesAssignList[uiSliceIdx] = kiMbNumPerSlice;
    iNumMbLeft	-= kiMbNumPerSlice;
  }
  pSlicesAssignList[uiSliceIdx] = iNumMbLeft;

  return true;
}

//slice parameter check for SM_ROWMB_SLICE
bool_t CheckRowMbMultiSliceSetting (const int32_t kiMbWidth, SSliceArgument* pSliceArg) {
  int32_t* pSlicesAssignList = (int32_t*) & (pSliceArg->uiSliceMbNum[0]);
  const uint32_t kuiSliceNum		= pSliceArg->iSliceNum;
  uint32_t uiSliceIdx			= 0;

  if (NULL == pSlicesAssignList)
    return false;

  while (uiSliceIdx < kuiSliceNum) {
    pSlicesAssignList[uiSliceIdx]	= kiMbWidth;
    ++ uiSliceIdx;
  }
  return true;
}

//slice parameter check for SM_RASTER_SLICE
bool_t CheckRasterMultiSliceSetting (const int32_t kiMbNumInFrame, SSliceArgument* pSliceArg) {
  int32_t*			pSlicesAssignList = (int32_t*) & (pSliceArg->uiSliceMbNum[0]);
  int32_t			iActualSliceCount	= 0;

  //check mb_num setting
  uint32_t uiSliceIdx			= 0;
  int32_t iCountMb			= 0;

  if (NULL == pSlicesAssignList)
    return false;

  while ((uiSliceIdx < MAX_SLICES_NUM) && (0 < pSlicesAssignList[uiSliceIdx])) {
    iCountMb			+= pSlicesAssignList[uiSliceIdx];
    iActualSliceCount	=  uiSliceIdx + 1;

    if (iCountMb >= kiMbNumInFrame) {
      break;
    }

    ++ uiSliceIdx;
  }
  //break condition above makes, after the while
  // here must have (iActualSliceCount <= MAX_SLICES_NUM)

  //correction if needed
  if (iCountMb == kiMbNumInFrame) {
    ;
  } else if (iCountMb > kiMbNumInFrame) {
    //need correction:
    //setting is more than iMbNumInFrame,
    //cut the last uiSliceMbNum; adjust iCountMb
    pSlicesAssignList[iActualSliceCount - 1]	-=	(iCountMb - kiMbNumInFrame);
    iCountMb								=	kiMbNumInFrame;
  } else if (iActualSliceCount < MAX_SLICES_NUM) {
    //where ( iCountMb < iMbNumInFrame )
    //can do correction:
    //	make the last uiSliceMbNum the left num
    pSlicesAssignList[iActualSliceCount] = kiMbNumInFrame - iCountMb;
    iActualSliceCount += 1;
  } else {
    //here ( iCountMb < iMbNumInFrame ) && ( iActualSliceCount == MAX_SLICES_NUM )
    //no more slice can be added
    return false;
  }

  pSliceArg->iSliceNum = iActualSliceCount;
  return true;

}


// GOM based RC related for uiSliceNum decision, only used at SM_FIXEDSLCNUM_SLICE
void GomValidCheckSliceNum (const int32_t kiMbWidth, const int32_t kiMbHeight, int32_t* pSliceNum) {
  const int32_t kiCountNumMb	= kiMbWidth * kiMbHeight;
  int32_t iSliceNum			= *pSliceNum;
  int32_t iGomSize;

  //The default RC is Bit-rate mode[Yi], but need consider as below:
  // Tuned to use max of mode0 and mode1 due can not refresh on this from rc mode changed outside, 8/16/2011
  // NOTE: GOM_ROW_MODE0_?P is integer multipler of GOM_ROW_MODE1_?P, which predefined at rc.h there, so GOM_ROM take MODE0 as the initial
  if (kiMbWidth <= MB_WIDTH_THRESHOLD_90P)
    iGomSize = kiMbWidth * GOM_ROW_MODE0_90P;
  else if (kiMbWidth <= MB_WIDTH_THRESHOLD_180P)
    iGomSize = kiMbWidth *  GOM_ROW_MODE0_180P;
  else if (kiMbWidth <= MB_WIDTH_THRESHOLD_360P)
    iGomSize = kiMbWidth * GOM_ROW_MODE0_360P;
  else
    iGomSize = kiMbWidth * GOM_ROW_MODE0_720P;

  while (true) {
    if (kiCountNumMb < iGomSize * iSliceNum) {
      -- iSliceNum;
      iSliceNum = iSliceNum - (iSliceNum & 0x01);	// verfiy even num for multiple slices case
      if (iSliceNum < 2)	// for safe
        break;
      continue;
    }
    break;
  }

  if (0 == iSliceNum)
    iSliceNum = 1;

  *pSliceNum	= iSliceNum;
}


// GOM based RC related for uiSliceMbNum decision, only used at SM_FIXEDSLCNUM_SLICE
void GomValidCheckSliceMbNum (const int32_t kiMbWidth, const int32_t kiMbHeight, SSliceArgument* pSliceArg) {
  uint32_t* pSlicesAssignList		= & (pSliceArg->uiSliceMbNum[0]);
  const uint32_t kuiSliceNum			= pSliceArg->iSliceNum;
  const int32_t kiMbNumInFrame	= kiMbWidth * kiMbHeight;
  const int32_t kiMbNumPerSlice	= kiMbNumInFrame / kuiSliceNum;
  int32_t iNumMbLeft				= kiMbNumInFrame;

  int32_t iMinimalMbNum			= kiMbWidth;	// in theory we need only 1 SMB, here let it as one SMB row required
  int32_t iMaximalMbNum			= 0;	// dynamically assign later
  int32_t iGomSize;

  uint32_t uiSliceIdx	= 0;	// for test

  // The default RC is Bit-rate mode [Yi], but need consider as below:
  // Tuned to use max of mode0 and mode1 due can not refresh on this from rc mode changed outside, 8/16/2011
  // NOTE: GOM_ROW_MODE0_?P is integer multipler of GOM_ROW_MODE1_?P, which predefined at rc.h there, so GOM_ROM take MODE0 as the initial
  if (kiMbWidth <= MB_WIDTH_THRESHOLD_90P)
    iGomSize = kiMbWidth * GOM_ROW_MODE0_90P;
  else if (kiMbWidth <= MB_WIDTH_THRESHOLD_180P)
    iGomSize = kiMbWidth * GOM_ROW_MODE0_180P;
  else if (kiMbWidth <= MB_WIDTH_THRESHOLD_360P)
    iGomSize = kiMbWidth * GOM_ROW_MODE0_360P;
  else
    iGomSize = kiMbWidth * GOM_ROW_MODE0_720P;

  iMinimalMbNum	= iGomSize;
  iMaximalMbNum	= kiMbNumInFrame - (kuiSliceNum - 1) * iMinimalMbNum;

  while (uiSliceIdx + 1 < kuiSliceNum) {
    // GOM boundary aligned
    int32_t iNumMbAssigning = (int32_t) (1.0f * kiMbNumPerSlice / iGomSize + 0.5f + EPSN) * iGomSize;

    // make sure one GOM at least in each slice for safe
    if (iNumMbAssigning < iMinimalMbNum)
      iNumMbAssigning	= iMinimalMbNum;
    else if (iNumMbAssigning > iMaximalMbNum)
      iNumMbAssigning	= iMaximalMbNum;

    assert (iNumMbAssigning > 0);

    iNumMbLeft -= iNumMbAssigning;
    assert (iNumMbLeft > 0);
    pSlicesAssignList[uiSliceIdx]	= iNumMbAssigning;

    ++ uiSliceIdx;
    iMaximalMbNum	= iNumMbLeft - (kuiSliceNum - uiSliceIdx - 1) * iMinimalMbNum;	// get maximal num_mb in left parts
  }
  pSlicesAssignList[uiSliceIdx] = iNumMbLeft;
}


/*!
 *	Get slice count for multiple slice segment
 *
 */
int32_t GetInitialSliceNum (const int32_t kiMbWidth, const int32_t kiMbHeight, SMulSliceOption* pMso) {
  if (NULL == pMso)
    return -1;

  switch (pMso->uiSliceMode) {
  case SM_SINGLE_SLICE:
  case SM_FIXEDSLCNUM_SLICE:
  case SM_RASTER_SLICE:
  case SM_ROWMB_SLICE: {
    return pMso->sSliceArgument.iSliceNum;
  }
  case SM_DYN_SLICE: {
    return AVERSLICENUM_CONSTRAINT;//at the beginning of dynamic slicing, set the uiSliceNum to be 1
  }
  case SM_RESERVED:
  default: {
    return -1;
  }
  }

  return -1;
}

/*!
 * \brief	Initialize slice segment (Single/multiple slices)
 *
 * \param	pSliceSeg			SSlice segment to be initialized
 * \param	uiSliceMode			SSlice mode
 * \param	multi_slice_argv	Multiple slices argument
 * \param	iMbWidth			MB width
 * \param	iMbHeight			MB height
 *
 * \return	0 - successful; none 0 - failed;
 */
int32_t InitSliceSegment (SSliceCtx* pSliceSeg,
                          CMemoryAlign* pMa,
                          SMulSliceOption* pMso,
                          const int32_t kiMbWidth,
                          const int32_t kiMbHeight) {
  const int32_t kiCountMbNum = kiMbWidth * kiMbHeight;
  SliceMode uiSliceMode = SM_SINGLE_SLICE;

  if (NULL == pSliceSeg || NULL == pMso || kiMbWidth == 0 || kiMbHeight == 0)
    return 1;

  uiSliceMode = pMso->uiSliceMode;
  if (pSliceSeg->iMbNumInFrame == kiCountMbNum && pSliceSeg->iMbWidth == kiMbWidth
      && pSliceSeg->iMbHeight == kiMbHeight && pSliceSeg->uiSliceMode == uiSliceMode && pSliceSeg->pOverallMbMap != NULL)
    return 0;
  else if (pSliceSeg->iMbNumInFrame != kiCountMbNum) {
    if (NULL != pSliceSeg->pOverallMbMap) {
      pMa->WelsFree (pSliceSeg->pOverallMbMap, "pSliceSeg->pOverallMbMap");

      pSliceSeg->pOverallMbMap = NULL;
    }
    if (NULL != pSliceSeg->pFirstMbInSlice) {
      pMa->WelsFree (pSliceSeg->pFirstMbInSlice, "pSliceSeg->pFirstMbInSlice");

      pSliceSeg->pFirstMbInSlice = NULL;
    }
    if (NULL != pSliceSeg->pCountMbNumInSlice) {
      pMa->WelsFree (pSliceSeg->pCountMbNumInSlice, "pSliceSeg->pCountMbNumInSlice");

      pSliceSeg->pCountMbNumInSlice	= NULL;
    }
    // just for safe
    pSliceSeg->iSliceNumInFrame	= 0;
    pSliceSeg->iMbNumInFrame		= 0;
    pSliceSeg->iMbWidth				= 0;
    pSliceSeg->iMbHeight			= 0;
    pSliceSeg->uiSliceMode			= SM_SINGLE_SLICE;	// sigle in default
  }

  if (SM_SINGLE_SLICE == uiSliceMode) {
    pSliceSeg->pOverallMbMap	= (uint8_t*)pMa->WelsMalloc (kiCountMbNum * sizeof (uint8_t), "pSliceSeg->pOverallMbMap");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pOverallMbMap)
    pSliceSeg->iSliceNumInFrame	= 1;

    pSliceSeg->pFirstMbInSlice	= (int16_t*)pMa->WelsMalloc (pSliceSeg->iSliceNumInFrame * sizeof (int16_t),
                                  "pSliceSeg->pFirstMbInSlice");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pFirstMbInSlice)

    pSliceSeg->pCountMbNumInSlice = (int32_t*)pMa->WelsMalloc (pSliceSeg->iSliceNumInFrame * sizeof (int32_t),
                                    "pSliceSeg->pCountMbNumInSlice");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pCountMbNumInSlice)
    pSliceSeg->uiSliceMode			= uiSliceMode;
    pSliceSeg->iMbWidth				= kiMbWidth;
    pSliceSeg->iMbHeight			= kiMbHeight;
    pSliceSeg->iMbNumInFrame		= kiCountMbNum;
    pSliceSeg->pCountMbNumInSlice[0]	= kiCountMbNum;
    pSliceSeg->pFirstMbInSlice[0]		= 0;

    return AssignMbMapSingleSlice (pSliceSeg->pOverallMbMap, kiCountMbNum, sizeof (pSliceSeg->pOverallMbMap[0]));
  } else { //if ( SM_MULTIPLE_SLICE == uiSliceMode )
    if (uiSliceMode != SM_FIXEDSLCNUM_SLICE && uiSliceMode != SM_ROWMB_SLICE && uiSliceMode != SM_RASTER_SLICE
        && uiSliceMode != SM_DYN_SLICE)
      return 1;

    pSliceSeg->pOverallMbMap	= (uint8_t*)pMa->WelsMalloc (kiCountMbNum * sizeof (uint8_t), "pSliceSeg->pOverallMbMap");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pOverallMbMap)

    //SM_DYN_SLICE: init, set pSliceSeg->iSliceNumInFrame	= 1;
    pSliceSeg->iSliceNumInFrame = GetInitialSliceNum (kiMbWidth, kiMbHeight, pMso);

    if (-1 == pSliceSeg->iSliceNumInFrame)
      return 1;

    pSliceSeg->pCountMbNumInSlice	= (int32_t*)pMa->WelsMalloc (pSliceSeg->iSliceNumInFrame * sizeof (int32_t),
                                    "pSliceSeg->pCountMbNumInSlice");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pCountMbNumInSlice)

    pSliceSeg->pFirstMbInSlice		= (int16_t*)pMa->WelsMalloc (pSliceSeg->iSliceNumInFrame * sizeof (int16_t),
                                    "pSliceSeg->pFirstMbInSlice");

    WELS_VERIFY_RETURN_IF (1, NULL == pSliceSeg->pFirstMbInSlice)
    pSliceSeg->uiSliceMode			= pMso->uiSliceMode;
    pSliceSeg->iMbWidth				= kiMbWidth;
    pSliceSeg->iMbHeight			= kiMbHeight;
    pSliceSeg->iMbNumInFrame		= kiCountMbNum;
    if (SM_DYN_SLICE == pMso->uiSliceMode) {
      if (0 < pMso->sSliceArgument.uiSliceSizeConstraint) {
        pSliceSeg->uiSliceSizeConstraint = pMso->sSliceArgument.uiSliceSizeConstraint;
      } else {
        return 1;
      }
    } else {
      pSliceSeg->uiSliceSizeConstraint = DEFAULT_MAXPACKETSIZE_CONSTRAINT;
    }
    // about "iMaxSliceNumConstraint"
    //only used in SM_DYN_SLICE mode so far,
    //now follows NAL_UNIT_CONSTRAINT, (see definition)
    //will be adjusted under MT if there is limitation on iLayerNum
    pSliceSeg->iMaxSliceNumConstraint = MAX_SLICES_NUM;


    return AssignMbMapMultipleSlices (pSliceSeg, pMso);
  }
  return 0;
}

/*!
 * \brief	Uninitialize slice segment (Single/multiple slices)
 *
 * \param	pSliceSeg			SSlice segment to be uninitialized
 *
 * \return	none;
 */
void UninitSliceSegment (SSliceCtx* pSliceSeg, CMemoryAlign* pMa) {
  if (NULL != pSliceSeg) {
    if (NULL != pSliceSeg->pOverallMbMap) {
      pMa->WelsFree (pSliceSeg->pOverallMbMap, "pSliceSeg->pOverallMbMap");

      pSliceSeg->pOverallMbMap = NULL;
    }
    if (NULL != pSliceSeg->pFirstMbInSlice) {
      pMa->WelsFree (pSliceSeg->pFirstMbInSlice, "pSliceSeg->pFirstMbInSlice");

      pSliceSeg->pFirstMbInSlice = NULL;
    }
    if (NULL != pSliceSeg->pCountMbNumInSlice) {
      pMa->WelsFree (pSliceSeg->pCountMbNumInSlice, "pSliceSeg->pCountMbNumInSlice");

      pSliceSeg->pCountMbNumInSlice = NULL;
    }

    pSliceSeg->iMbNumInFrame		= 0;
    pSliceSeg->iMbWidth				= 0;
    pSliceSeg->iMbHeight			= 0;
    pSliceSeg->uiSliceMode			= SM_SINGLE_SLICE;	// single in default
    pSliceSeg->iSliceNumInFrame	= 0;
  }
}


/*!
 * \brief	Initialize Wels SSlice context (Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context to be initialized
 * \param	bFmoUseFlag	flag of using fmo
 * \param	iMbWidth		MB width
 * \param	iMbHeight		MB height
 * \param	uiSliceMode		slice mode
 * \param	mul_slice_arg	argument for multiple slice if it is applicable
 * \param	pPpsArg			argument for pPps parameter
 *
 * \return	0 - successful; none 0 - failed;
 */
int32_t InitSlicePEncCtx (SSliceCtx* pSliceCtx,
                          CMemoryAlign* pMa,
                          bool_t bFmoUseFlag,
                          int32_t iMbWidth,
                          int32_t iMbHeight,
                          SMulSliceOption* pMso,
                          void* pPpsArg) {
  if (NULL == pSliceCtx)
    return 1;

  InitSliceSegment (pSliceCtx,
                    pMa,
                    pMso,
                    iMbWidth,
                    iMbHeight);
  return 0;
}

/*!
 * \brief	Uninitialize Wels SSlice context (Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context to be initialized
 *
 * \return	NONE;
 */
void UninitSlicePEncCtx (SSliceCtx* pSliceCtx, CMemoryAlign* pMa) {
  if (NULL != pSliceCtx) {
    UninitSliceSegment (pSliceCtx, pMa);
  }
}

/*!
 * \brief	Get slice idc for given iMbXY (apply in Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context
 * \param	kiMbXY			MB xy index
 *
 * \return	uiSliceIdc - successful; -1 - failed;
 */
uint8_t WelsMbToSliceIdc (SSliceCtx* pSliceCtx, const int16_t kiMbXY) {
  if (NULL != pSliceCtx && kiMbXY < pSliceCtx->iMbNumInFrame && kiMbXY >= 0)
    return pSliceCtx->pOverallMbMap[ kiMbXY ];
  return (uint8_t) (-1);
}

/*!
 * \brief	Get first mb in slice/slice_group: uiSliceIdc (apply in Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context
 * \param	kuiSliceIdc		slice idc
 *
 * \return	iFirstMb - successful; -1 - failed;
 */
int32_t WelsGetFirstMbOfSlice (SSliceCtx* pSliceCtx, const int32_t kuiSliceIdc) {
  return pSliceCtx->pFirstMbInSlice[ kuiSliceIdc ];
}

/*!
 * \brief	Get successive mb to be processed in slice/slice_group: uiSliceIdc (apply in Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context
 * \param	kiMbXY			MB xy index
 *
 * \return	next_mb - successful; -1 - failed;
 */
int32_t WelsGetNextMbOfSlice (SSliceCtx* pSliceCtx, const int16_t kiMbXY) {
  if (NULL != pSliceCtx) {
    SSliceCtx* pSliceSeg = pSliceCtx;
    if (NULL == pSliceSeg || kiMbXY < 0 || kiMbXY >= pSliceSeg->iMbNumInFrame)
      return -1;
    if (SM_SINGLE_SLICE == pSliceSeg->uiSliceMode) {
      int32_t iNextMbIdx = kiMbXY;
      ++ iNextMbIdx;
      if (iNextMbIdx >= pSliceSeg->iMbNumInFrame)
        iNextMbIdx	= -1;
      return iNextMbIdx;
    } else { /*if ( SM_MULTIPLE_SLICE == pSliceSeg->uiSliceMode )*/
      if (SM_RESERVED != pSliceSeg->uiSliceMode) {
        int32_t iNextMbIdx = kiMbXY;
        ++ iNextMbIdx;
        if (iNextMbIdx < pSliceSeg->iMbNumInFrame && pSliceSeg->pOverallMbMap != NULL
            && pSliceSeg->pOverallMbMap[iNextMbIdx] == pSliceSeg->pOverallMbMap[ kiMbXY ])
          return iNextMbIdx;
        return -1;
      } else
        return -1;	// reserved here for other multiple slice type
    }
  } else
    return -1;
}

/*!
 * \brief	Get previous mb to be processed in slice/slice_group: uiSliceIdc (apply in Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context
 * \param	kiMbXY			MB xy index
 *
 * \return	prev_mb - successful; -1 - failed;
 */
int32_t WelsGetPrevMbOfSlice (SSliceCtx* pSliceCtx, const int16_t kiMbXY) {
  if (NULL != pSliceCtx) {
    SSliceCtx* pSliceSeg = pSliceCtx;
    if (NULL == pSliceSeg || kiMbXY < 0 || kiMbXY >= pSliceSeg->iMbNumInFrame)
      return -1;
    if (pSliceSeg->uiSliceMode == SM_SINGLE_SLICE)
      return (-1 + kiMbXY);
    else { /* if ( pSliceSeg->uiSliceMode == SM_MULTIPLE_SLICE )*/
      if (SM_RESERVED == pSliceSeg->uiSliceMode) {
        int32_t iPrevMbIdx = kiMbXY;
        -- iPrevMbIdx;
        if (iPrevMbIdx >= 0 && iPrevMbIdx < pSliceSeg->iMbNumInFrame && NULL != pSliceSeg->pOverallMbMap
            && pSliceSeg->pOverallMbMap[ kiMbXY ] == pSliceSeg->pOverallMbMap[ iPrevMbIdx ])
          return iPrevMbIdx;
        return -1;
      } else
        return -1;
    }
  } else
    return -1;
}

/*!
 * \brief	Get number of mb in slice/slice_group: uiSliceIdc (apply in Single/multiple slices and FMO)
 *
 * \param	pSliceCtx		SSlice context
 * \param	kuiSliceIdc		slice/slice_group idc
 *
 * \return	count_num_of_mb - successful; -1 - failed;
 */
int32_t WelsGetNumMbInSlice (SSliceCtx* pSliceCtx, const int32_t kuiSliceIdc) {
  if (NULL == pSliceCtx || kuiSliceIdc < 0)
    return -1;
  {
    SSliceCtx* pSliceSeg = pSliceCtx;
    if (SM_SINGLE_SLICE != pSliceSeg->uiSliceMode) {
      if (NULL == pSliceSeg->pCountMbNumInSlice || kuiSliceIdc >= pSliceSeg->iSliceNumInFrame)
        return -1;
      return pSliceSeg->pCountMbNumInSlice[ kuiSliceIdc ];
    } else { /*if ( pSliceSeg->uiSliceMode == SM_SINGLE_SLICE )*/
      if (kuiSliceIdc > 0 || NULL == pSliceSeg->pCountMbNumInSlice)
        return -1;
      return pSliceSeg->pCountMbNumInSlice[ kuiSliceIdc ];
    }
  }
}

int32_t GetCurrentSliceNum (const SSliceCtx* kpSliceCtx) {
  return (kpSliceCtx != NULL) ? (kpSliceCtx->iSliceNumInFrame) : (-1);
}
int32_t DynamicAdjustSlicePEncCtxAll (SSliceCtx* pSliceCtx,
                                      int32_t* pRunLength) {
  const int32_t iCountNumMbInFrame		= pSliceCtx->iMbNumInFrame;
  const int32_t iCountSliceNumInFrame	= pSliceCtx->iSliceNumInFrame;
  int32_t iSameRunLenFlag				= 1;
  int32_t iFirstMbIdx					= 0;
  int32_t iSliceIdx						= 0;

  assert (iCountSliceNumInFrame <= MAX_THREADS_NUM);

  while (iSliceIdx < iCountSliceNumInFrame) {
    if (pRunLength[iSliceIdx] != pSliceCtx->pCountMbNumInSlice[iSliceIdx]) {
      iSameRunLenFlag = 0;
      break;
    }
    ++ iSliceIdx;
  }
  if (iSameRunLenFlag) {
    return 1;	// do not need adjust it due to same running length as before to save complexity
  }

  iSliceIdx = 0;
  do {
    const int32_t kiSliceRun	= pRunLength[iSliceIdx];

    pSliceCtx->pFirstMbInSlice[iSliceIdx]			= iFirstMbIdx;
    pSliceCtx->pCountMbNumInSlice[iSliceIdx]		= kiSliceRun;

    memset (pSliceCtx->pOverallMbMap + iFirstMbIdx, (uint8_t)iSliceIdx, kiSliceRun * sizeof (uint8_t));

    iFirstMbIdx += kiSliceRun;

    ++ iSliceIdx;
  } while (iSliceIdx < iCountSliceNumInFrame && iFirstMbIdx < iCountNumMbInFrame);

  return 0;
}

int32_t DynamicMaxSliceNumConstraint (uint32_t uiMaximumNum, int32_t iConsumedNum, uint32_t iDulplicateTimes) {
  return ((uiMaximumNum - iConsumedNum - 1) / iDulplicateTimes);
}

} // namespace WelsSVCEnc
