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
 *  welsCodecTrace.h
 *
 *  Abstract
 *      Cisco OpenH264 encoder extension utilization interface for T26
 *
 *  History
 *      4/24/2009 Created
 *
 *
 *************************************************************************/
#if !defined(AFX_WELSH264ENCODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_)
#define AFX_WELSH264ENCODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_

#include "codec_api.h"
#include "codec_def.h"
#include "codec_app_def.h"
#include "welsCodecTrace.h"
#include "encoder_context.h"
#include "param_svc.h"
#include "extern.h"
#include "cpu.h"

//#define OUTPUT_BIT_STREAM
//#define DUMP_SRC_PICTURE
//#define REC_FRAME_COUNT

class ISVCEncoder;
namespace WelsSVCEnc {
class CWelsH264SVCEncoder : public ISVCEncoder {
 public:
  CWelsH264SVCEncoder();
  virtual ~CWelsH264SVCEncoder();

  /* Interfaces override from ISVCEncoder */
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI Initialize (const SEncParamBase* argv);
  virtual int EXTAPI InitializeExt (const SEncParamExt* argv);

  virtual int EXTAPI GetDefaultParams (SEncParamExt* argv);

  virtual int EXTAPI Uninitialize();

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo);
  virtual int        EncodeFrameInternal (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo);

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI EncodeParameterSets (SFrameBSInfo* pBsInfo);

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI PauseFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo);

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI ForceIntraFrame (bool bIDR);

  /************************************************************************
   * InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
   ************************************************************************/
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI SetOption (ENCODER_OPTION opt_id, void* option);
  virtual int EXTAPI GetOption (ENCODER_OPTION opt_id, void* option);

 private:
  int InitializeInternal (SWelsSvcCodingParam* argv);

  sWelsEncCtx*	m_pEncContext;

  welsCodecTrace*			m_pWelsTrace;
  SSourcePicture**			m_pSrcPicList;
  int32_t						m_iSrcListSize;

  int32_t						m_iMaxPicWidth;
  int32_t						m_iMaxPicHeight;

  int32_t						m_iCspInternal;
  bool					m_bInitialFlag;

#ifdef OUTPUT_BIT_STREAM
  FILE*				m_pFileBs;
  FILE*               m_pFileBsSize;
  bool				m_bSwitch;
  int32_t					m_iSwitchTimes;
#endif//OUTPUT_BIT_STREAM

#ifdef REC_FRAME_COUNT
  int32_t		m_uiCountFrameNum;
#endif//REC_FRAME_COUNT

  void    InitEncoder (void);
  void    DumpSrcPicture (const uint8_t* pSrc);
};
}
#endif // !defined(AFX_WELSH264ENCODER_H__D9FAA1D1_5403_47E1_8E27_78F11EE65F02__INCLUDED_)
