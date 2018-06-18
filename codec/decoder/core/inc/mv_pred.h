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
 * \file    mv_pred.h
 *
 * \brief   Get MV predictor and update motion vector of mb cache
 *
 * \date    05/22/2009 Created
 *
 *************************************************************************************
 */

#ifndef WELS_MV_PRED_H__
#define WELS_MV_PRED_H__

#include "dec_frame.h"
#include "decoder_context.h"

namespace WelsDec {

/*!
* \brief     update mv and ref_index cache for current MB, only for P_16x16 (SKIP inclusive)
* \param
* \param
*/
void UpdateP16x16MotionInfo (PDqLayer pCurDqLayer, int32_t listIdx, int8_t iRef, int16_t iMVs[2]);

/*!
* \brief   update mv and ref_index cache for current MB, only for P_16x8
* \param
* \param
*/
void UpdateP16x8MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A],
                            int8_t iRefIndex[LIST_A][30],
														int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]);


/*!
 * \brief    update mv and ref_index cache for current MB, only for P_8x16
 * \param
 * \param
 */
void UpdateP8x16MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A],
                            int8_t iRefIndex[LIST_A][30],
														int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]);

/*!
 * \brief   get the motion predictor for skip mode
 * \param
 * \param   output iMvp[]
 */
void PredPSkipMvFromNeighbor (PDqLayer pCurLayer, int16_t iMvp[2]);

/*!
* \brief   get the motion predictor and reference for B-slice direct mode version 2
* \param
* \param   output iMvp[] and ref
*/
SubMbType  PredMvBDirectSpatial(PWelsDecoderContext pCtx, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]);

/*!
* \brief   get Colocated MB for both Spatial and Temporal Direct Mode
* \param
* \param   output MbType and SubMbType
*/
int32_t GetColocatedMb(PWelsDecoderContext pCtx, MbType& mbType, SubMbType& subMbType);

/*!
* \brief   get the motion predictor for B-slice temporal direct mode 16x16
*/
void PredBDirectTemporal(PWelsDecoderContext pCtx, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]);

/*!
* \brief   get the motion params for B-slice spatial direct mode
* \param
* \param   output iMvp[]
*/

/*!
 * \brief   get the motion predictor for 4*4 or 8*8 or 16*16 block
 * \param
 * \param   output iMvp[]
 */
void PredMv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
	 int32_t listIdx, int32_t iPartIdx, int32_t iPartWidth, int8_t iRef, int16_t iMVP[2]);

/*!
 * \brief   get the motion predictor for inter16x8 MB
 * \param
 * \param   output mvp_x and mvp_y
 */
void PredInter16x8Mv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
											int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]);

/*!
 * \brief   get the motion predictor for inter8x16 MB
 * \param
 * \param   output mvp_x and mvp_y
 */
void PredInter8x16Mv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
											int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]);

} // namespace WelsDec

#endif//WELS_MV_PRED_H__
