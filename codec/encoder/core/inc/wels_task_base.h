/*!
 * \copy
 *     Copyright (c)  2009-2015, Cisco Systems
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
 * \file    wels_task_base.h
 *
 * \brief   interface for base task
 *
 * \date    5/09/2012 Created
 *
 *************************************************************************************
 */



#ifndef  _WELS_BASE_TASK_H_
#define  _WELS_BASE_TASK_H_

#include "typedefs.h"
#include "WelsTask.h"

namespace WelsEnc {

class CWelsBaseTask : public WelsCommon::IWelsTask {
 public:
  enum ETaskType {
    WELS_ENC_TASK_ENCODING = 0,
    WELS_ENC_TASK_ENCODE_FIXED_SLICE = WELS_ENC_TASK_ENCODING,
    WELS_ENC_TASK_ENCODE_SLICE_LOADBALANCING = WELS_ENC_TASK_ENCODING,
    WELS_ENC_TASK_ENCODE_SLICE_SIZECONSTRAINED = WELS_ENC_TASK_ENCODING,
    WELS_ENC_TASK_PREENCODING = 1,
    WELS_ENC_TASK_PREPROCESS = 2,
    WELS_ENC_TASK_ALL = 3,
  };

  CWelsBaseTask();
  virtual ~CWelsBaseTask();

  virtual uint32_t GetTaskType() const = 0;

 private:

};

}


#endif




