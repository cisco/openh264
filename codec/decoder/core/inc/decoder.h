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
 * \file	decoder.h
 *
 * \brief	Interfaces introduced in decoder system architecture
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_DECODER_SYSTEM_ARCHITECTURE_H__
#define WELS_DECODER_SYSTEM_ARCHITECTURE_H__

#include "typedefs.h"
#include "decoder_context.h"

namespace WelsDec {

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/*!
 * \brief	configure decoder parameters
 */
int32_t DecoderConfigParam (PWelsDecoderContext pCtx, const void_t* kpParam);

/*!
 *************************************************************************************
 * \brief	Initialize Wels decoder parameters and memory
 *
 * \param 	pCtx	        input context to be initialized at first stage
 * \param   pTraceHandle    handle for trace
 * \param   pLo             log info pointer
 *
 * \return	0 - successed
 * \return	1 - failed
 *
 * \note	N/A
 *************************************************************************************
 */
int32_t WelsInitDecoder (PWelsDecoderContext pCtx,  void_t* pTraceHandle, PWelsLogCallbackFunc pLog);

/*!
 *************************************************************************************
 * \brief	Uninitialize Wels decoder parameters and memory
 *
 * \param 	pCtx	input context to be uninitialized at release stage
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void_t WelsEndDecoder (PWelsDecoderContext pCtx);

/*!
 *************************************************************************************
 * \brief	First entrance to decoding core interface.
 *
 * \param 	pCtx	        decoder context
 * \param	pBufBs	        bit streaming buffer
 * \param	kBsLen	        size in bytes length of bit streaming buffer input
 * \param	ppDst	        picture payload data to be output
 * \param	pDstBufInfo	    buf information of ouput data
 *
 * \return	0 - successed
 * \return	1 - failed
 *
 * \note	N/A
 *************************************************************************************
 */

int32_t WelsDecodeBs (PWelsDecoderContext pCtx, const uint8_t* kpBsBuf, const int32_t kiBsLen,
                      uint8_t** ppDst, SBufferInfo* pDstBufInfo);

/*
 *	request memory blocks for decoder avc part
 */
int32_t WelsRequestMem (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight);


/*
 *	free memory blocks in avc
 */
void_t WelsFreeMem (PWelsDecoderContext pCtx);

/*
 * set colorspace format in decoder
 */
int32_t DecoderSetCsp (PWelsDecoderContext pCtx, const int32_t kiColorFormat);

/*!
 * \brief	make sure synchonozization picture resolution (get from slice header) among different parts (i.e, memory related and so on)
 *			over decoder internal
 * ( MB coordinate and parts of data within decoder context structure )
 * \param	pCtx		Wels decoder context
 * \param	iMbWidth	MB width
 * \pram	iMbHeight	MB height
 * \return	0 - successful; none 0 - something wrong
 */
int32_t SyncPictureResolutionExt (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight);

/*!
 * \brief	update maximal picture width and height if applicable when receiving a SPS NAL
 */
void_t UpdateMaxPictureResolution (PWelsDecoderContext pCtx, const int32_t kiCurWidth, const int32_t kiCurHeight);

void_t AssignFuncPointerForRec (PWelsDecoderContext pCtx);

void_t ResetParameterSetsState (PWelsDecoderContext pCtx);

void_t GetVclNalTemporalId (PWelsDecoderContext pCtx); //get the info that whether or not have VCL NAL in current AU,
//and if YES, get the temporal ID

#ifdef __cplusplus
}
#endif//__cplusplus

} // namespace WelsDec

#endif//WELS_DECODER_SYSTEM_ARCHITECTURE_H__
