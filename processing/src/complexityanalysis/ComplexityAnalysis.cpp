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

#include "ComplexityAnalysis.h"
#include "../common/cpu.h"

WELSVP_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CComplexityAnalysis::CComplexityAnalysis(int32_t iCpuFlag)
{
	m_eMethod   = METHOD_COMPLEXITY_ANALYSIS;
	m_pfGomSad   = NULL;
	WelsMemset( &m_sComplexityAnalysisParam, 0, sizeof(m_sComplexityAnalysisParam) );
}

CComplexityAnalysis::~CComplexityAnalysis()
{	
}

EResult CComplexityAnalysis::Process(int32_t iType, SPixMap *pSrcPixMap, SPixMap *pRefPixMap)
{
	EResult eReturn = RET_SUCCESS;	

	switch (m_sComplexityAnalysisParam.iComplexityAnalysisMode)
	{
	case FRAME_SAD:
		AnalyzeFrameComplexityViaSad( pSrcPixMap, pRefPixMap );
		break;
	case GOM_SAD:
		AnalyzeGomComplexityViaSad( pSrcPixMap, pRefPixMap );
		break;
	case GOM_VAR:
		AnalyzeGomComplexityViaVar( pSrcPixMap, pRefPixMap );
		break;
	default:
		eReturn = RET_INVALIDPARAM;
		break;
	}	

	return eReturn;
}


EResult CComplexityAnalysis::Set(int32_t iType, void *pParam)
{
	if (pParam == NULL)
	{
		return RET_INVALIDPARAM;
	}

	m_sComplexityAnalysisParam = *(SComplexityAnalysisParam *)pParam;

	return RET_SUCCESS;
}

EResult CComplexityAnalysis::Get(int32_t iType, void *pParam)
{
	if (pParam == NULL)
	{
		return RET_INVALIDPARAM;
	}

	SComplexityAnalysisParam * sComplexityAnalysisParam = (SComplexityAnalysisParam *)pParam;

	sComplexityAnalysisParam->iFrameComplexity = m_sComplexityAnalysisParam.iFrameComplexity;

	return RET_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////////////////
void CComplexityAnalysis::AnalyzeFrameComplexityViaSad( SPixMap *pSrcPixMap, SPixMap *pRefPixMap )
{
	SVAACalcResult     *pVaaCalcResults = NULL;
	pVaaCalcResults = m_sComplexityAnalysisParam.pCalcResult;

	m_sComplexityAnalysisParam.iFrameComplexity = pVaaCalcResults->iFrameSad;

	if ( m_sComplexityAnalysisParam.iCalcBgd ) //BGD control
	{
		m_sComplexityAnalysisParam.iFrameComplexity = (int32_t)GetFrameSadExcludeBackground( pSrcPixMap, pRefPixMap );
	}
}

int32_t CComplexityAnalysis::GetFrameSadExcludeBackground( SPixMap *pSrcPixMap, SPixMap *pRefPixMap )
{
	int32_t iWidth     = pSrcPixMap->sRect.iRectWidth;
	int32_t iHeight    = pSrcPixMap->sRect.iRectHeight;	
	int32_t iMbWidth  = iWidth  >> 4;
	int32_t iMbHeight = iHeight >> 4;
	int32_t iMbNum    = iMbWidth * iMbHeight;

	int32_t iMbNumInGom = m_sComplexityAnalysisParam.iMbNumInGom;
	int32_t iGomMbNum = (iMbNum + iMbNumInGom - 1 ) / iMbNumInGom;
	int32_t iGomMbStartIndex = 0, iGomMbEndIndex = 0;

	uint8_t *pBackgroundMbFlag = (uint8_t *)m_sComplexityAnalysisParam.pBackgroundMbFlag;
	uint32_t*uiRefMbType = (uint32_t *)m_sComplexityAnalysisParam.uiRefMbType;
	SVAACalcResult *pVaaCalcResults = m_sComplexityAnalysisParam.pCalcResult;
	int32_t  *pGomForegroundBlockNum = m_sComplexityAnalysisParam.pGomForegroundBlockNum;

	uint32_t uiFrameSad = 0;
	for ( int32_t j = 0; j < iGomMbNum; j ++ )
	{
		iGomMbStartIndex = j * iMbNumInGom;
		iGomMbEndIndex = WELS_MIN( (j + 1) * iMbNumInGom, iMbNum);

		for ( int32_t i = iGomMbStartIndex; i < iGomMbEndIndex; i ++)
		{	
			if ( pBackgroundMbFlag[i] == 0 || IS_INTRA(uiRefMbType[i]) )
			{
				pGomForegroundBlockNum[j]++;
				uiFrameSad += pVaaCalcResults->pSad8x8[i][0];
				uiFrameSad += pVaaCalcResults->pSad8x8[i][1];
				uiFrameSad += pVaaCalcResults->pSad8x8[i][2];
				uiFrameSad += pVaaCalcResults->pSad8x8[i][3];
			}
		}
	}

	return (uiFrameSad);
}


void InitGomSadFunc(PGOMSadFunc &pfGomSad, uint8_t iCalcBgd)
{
	pfGomSad = GomSampleSad;

	if ( iCalcBgd )
	{
		pfGomSad = GomSampleSadExceptBackground;
	}
}

void GomSampleSad(uint32_t *pGomSad, int32_t *pGomForegroundBlockNum, int32_t *pSad8x8, uint8_t pBackgroundMbFlag)
{
  (*pGomForegroundBlockNum) ++;
  *pGomSad += pSad8x8[0];
  *pGomSad += pSad8x8[1];
  *pGomSad += pSad8x8[2];
  *pGomSad += pSad8x8[3];
}

void GomSampleSadExceptBackground(uint32_t *pGomSad, int32_t *pGomForegroundBlockNum, int32_t *pSad8x8, uint8_t pBackgroundMbFlag)
{
  if ( pBackgroundMbFlag == 0 )
  {
    (*pGomForegroundBlockNum) ++;
    *pGomSad += pSad8x8[0];
    *pGomSad += pSad8x8[1];
    *pGomSad += pSad8x8[2];
    *pGomSad += pSad8x8[3];
  }
}

void CComplexityAnalysis::AnalyzeGomComplexityViaSad( SPixMap *pSrcPixMap, SPixMap *pRefPixMap )
{
	int32_t iWidth     = pSrcPixMap->sRect.iRectWidth;
	int32_t iHeight    = pSrcPixMap->sRect.iRectHeight;	
	int32_t iMbWidth  = iWidth  >> 4;
	int32_t iMbHeight = iHeight >> 4;
	int32_t iMbNum    = iMbWidth * iMbHeight;

	int32_t iMbNumInGom = m_sComplexityAnalysisParam.iMbNumInGom;
	int32_t iGomMbNum = (iMbNum + iMbNumInGom - 1 ) / iMbNumInGom;

	int32_t iGomMbStartIndex = 0, iGomMbEndIndex = 0, iGomMbRowNum = 0;
	int32_t iMbStartIndex = 0, iMbEndIndex = 0;
	int32_t iStartSampleIndex = 0;

	uint8_t *pBackgroundMbFlag = (uint8_t *)m_sComplexityAnalysisParam.pBackgroundMbFlag;
	uint32_t*uiRefMbType = (uint32_t *)m_sComplexityAnalysisParam.uiRefMbType;
	SVAACalcResult *pVaaCalcResults = m_sComplexityAnalysisParam.pCalcResult;
	int32_t  *pGomForegroundBlockNum = (int32_t *)m_sComplexityAnalysisParam.pGomForegroundBlockNum;
	int32_t  *pGomComplexity = (int32_t *)m_sComplexityAnalysisParam.pGomComplexity;

	uint8_t *pRefY = NULL, *pSrcY = NULL;
	int32_t iRefStride = 0, iCurStride = 0;

	uint8_t *pRefTmp = NULL, *pCurTmp = NULL;
	uint32_t uiGomSad = 0, uiFrameSad = 0;

	pRefY = (uint8_t *)pRefPixMap->pPixel[0];
	pSrcY = (uint8_t *)pSrcPixMap->pPixel[0];

	iRefStride  = pRefPixMap->iStride[0];
	iCurStride  = pSrcPixMap->iStride[0];

	InitGomSadFunc( m_pfGomSad, m_sComplexityAnalysisParam.iCalcBgd );

	for ( int32_t j = 0; j < iGomMbNum; j ++ )
	{
		uiGomSad = 0;

		iGomMbStartIndex = j * iMbNumInGom;
		iGomMbEndIndex = WELS_MIN( (j + 1) * iMbNumInGom, iMbNum);
		iGomMbRowNum = (iGomMbEndIndex + iMbWidth - 1 ) / iMbWidth  - iGomMbStartIndex / iMbWidth;

		iMbStartIndex = iGomMbStartIndex;
		iMbEndIndex = WELS_MIN( (iMbStartIndex / iMbWidth + 1) * iMbWidth, iGomMbEndIndex);

		iStartSampleIndex  = ( iMbStartIndex / iMbWidth ) * MB_WIDTH_LUMA * iRefStride + ( iMbStartIndex % iMbWidth ) * MB_WIDTH_LUMA;

		do 
		{   
			pRefTmp = pRefY + iStartSampleIndex;
			pCurTmp = pSrcY + iStartSampleIndex;

			for ( int32_t i = iMbStartIndex; i < iMbEndIndex; i ++)
			{
				m_pfGomSad(&uiGomSad, pGomForegroundBlockNum + j, pVaaCalcResults->pSad8x8[i], pBackgroundMbFlag[i] && !IS_INTRA(uiRefMbType[i]) );
			}

			iMbStartIndex = iMbEndIndex;
			iMbEndIndex = WELS_MIN( iMbEndIndex + iMbWidth , iGomMbEndIndex);

			iStartSampleIndex  = ( iMbStartIndex / iMbWidth ) * MB_WIDTH_LUMA * iRefStride + ( iMbStartIndex % iMbWidth ) * MB_WIDTH_LUMA;

		} while ( --iGomMbRowNum );

		pGomComplexity[j] = uiGomSad;
		uiFrameSad += pGomComplexity[j];
	}

	m_sComplexityAnalysisParam.iFrameComplexity = uiFrameSad;
}


void CComplexityAnalysis::AnalyzeGomComplexityViaVar( SPixMap *pSrcPixMap, SPixMap *pRefPixMap )
{
	int32_t iWidth     = pSrcPixMap->sRect.iRectWidth;
	int32_t iHeight    = pSrcPixMap->sRect.iRectHeight;	
	int32_t iMbWidth  = iWidth  >> 4;
	int32_t iMbHeight = iHeight >> 4;
	int32_t iMbNum    = iMbWidth * iMbHeight;

	int32_t iMbNumInGom = m_sComplexityAnalysisParam.iMbNumInGom;
	int32_t iGomMbNum = (iMbNum + iMbNumInGom - 1 ) / iMbNumInGom;
	int32_t iGomSampleNum = 0;

	int32_t iGomMbStartIndex = 0, iGomMbEndIndex = 0, iGomMbRowNum = 0;
	int32_t iMbStartIndex = 0, iMbEndIndex = 0;
	int32_t iStartSampleIndex = 0;

	SVAACalcResult *pVaaCalcResults = m_sComplexityAnalysisParam.pCalcResult;
	int32_t  *pGomComplexity = (int32_t *)m_sComplexityAnalysisParam.pGomComplexity;

	uint8_t *pSrcY = NULL;
	int32_t iCurStride = 0;

	uint8_t *pCurTmp = NULL;
	uint32_t uiSampleSum = 0, uiSquareSum = 0;

	pSrcY = (uint8_t *)pSrcPixMap->pPixel[0];
	iCurStride  = pSrcPixMap->iStride[0];

	for ( int32_t j = 0; j < iGomMbNum; j ++ )
	{
		uiSampleSum = 0;
		uiSquareSum = 0;

		iGomMbStartIndex = j * iMbNumInGom;
		iGomMbEndIndex = WELS_MIN( (j + 1) * iMbNumInGom, iMbNum);
		iGomMbRowNum = (iGomMbEndIndex + iMbWidth - 1 ) / iMbWidth  - iGomMbStartIndex / iMbWidth;

		iMbStartIndex = iGomMbStartIndex;
		iMbEndIndex = WELS_MIN( (iMbStartIndex / iMbWidth + 1) * iMbWidth, iGomMbEndIndex);

		iStartSampleIndex  = ( iMbStartIndex / iMbWidth ) * MB_WIDTH_LUMA * iCurStride + ( iMbStartIndex % iMbWidth ) * MB_WIDTH_LUMA;
		iGomSampleNum = (iMbEndIndex - iMbStartIndex) * MB_WIDTH_LUMA * MB_WIDTH_LUMA;

		do 
		{
			pCurTmp = pSrcY + iStartSampleIndex;

			for ( int32_t i = iMbStartIndex; i < iMbEndIndex; i ++ )
			{
				uiSampleSum += pVaaCalcResults->pSum16x16[i];
				uiSquareSum += pVaaCalcResults->pSumOfSquare16x16[i];
			}

			iMbStartIndex = iMbEndIndex;
			iMbEndIndex = WELS_MIN( iMbEndIndex + iMbWidth, iGomMbEndIndex);

			iStartSampleIndex  = ( iMbStartIndex / iMbWidth ) * MB_WIDTH_LUMA * iCurStride + ( iMbStartIndex % iMbWidth ) * MB_WIDTH_LUMA;
		} while ( --iGomMbRowNum );
	
		pGomComplexity[j] = uiSquareSum - (uiSampleSum * uiSampleSum / iGomSampleNum);
	}
}


WELSVP_NAMESPACE_END
