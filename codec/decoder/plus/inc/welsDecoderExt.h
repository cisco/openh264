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
 *  WelsDecoderExt.h
 *
 *  Abstract
 *      Cisco OpenH264 decoder extension utilization interface
 *
 *  History
 *      3/12/2009 Created
 *
 *
 *************************************************************************/
#if !defined(AFX_WELSH264DECODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_)
#define AFX_WELSH264DECODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_

#include "codec_api.h"
#include "codec_app_def.h"
#include "decoder_context.h"
#include "welsCodecTrace.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class ISVCDecoder;

namespace WelsDec {

//#define OUTPUT_BIT_STREAM  ////for test to output bitstream

class CWelsDecoder : public ISVCDecoder  
{
public:
	CWelsDecoder(void_t);
	virtual ~CWelsDecoder();

	virtual long Initialize(void_t* pParam, const INIT_TYPE keInitType);
	virtual long Uninitialize();
	
	/***************************************************************************
	*	Description:
	*		Decompress one frame, and output RGB24 or YV12 decoded stream and its length.
	*	Input parameters:
	*       Parameter		TYPE			       Description
	*       pSrc             unsigned char*         the h264 stream to decode
	*       srcLength       int                    the length of h264 steam
	*       pDst             unsigned char*         buffer pointer of decoded data
	*       pDstInfo        SBufferInfo&           information provided to API including width, height, SW/HW option, etc
	*
	*	return: if decode frame success return 0, otherwise corresponding error returned.
	/***************************************************************************/
	virtual DECODING_STATE DecodeFrame(	const unsigned char* kpSrc,
		                                const int kiSrcLen,	
		                                unsigned char** ppDst,
		                                int* pStride,
		                                int& iWidth,
		                                int& iHeight	);

	virtual DECODING_STATE DecodeFrame(	const unsigned char* kpSrc,
											const int kiSrcLen,	
											void_t ** ppDst,
											SBufferInfo* pDstInfo);
	virtual DECODING_STATE DecodeFrameEx( const unsigned char * kpSrc,
		                                  const int kiSrcLen,
		                                  unsigned char * pDst,
										  int iDstStride,
		                                  int & iDstLen,
		                                  int & iWidth,
		                                  int & iHeight,
		                                  int & color_format);

    virtual long SetOption(DECODER_OPTION eOptID, void_t* pOption);
	virtual long GetOption(DECODER_OPTION eOptID, void_t* pOption);

private:	
	PWelsDecoderContext 				m_pDecContext;
	IWelsTrace							*m_pTrace;
	
	void_t InitDecoder( void_t );
	void_t UninitDecoder( void_t );
	
#ifdef OUTPUT_BIT_STREAM
	WelsFileHandle* m_pFBS;
	WelsFileHandle* m_pFBSSize;
#endif//OUTPUT_BIT_STREAM
	
};

} // namespace WelsDec

#endif // !defined(AFX_WELSH264DECODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_)
