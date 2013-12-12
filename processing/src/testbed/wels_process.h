/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	wels_process.h
 *
 * \brief	interface of video pre-process plugins
 *
 * \date	03/21/2011
 *
 * \description : this class is designed as an interface to unify video pre-processing
 *                class implement sets such as denoise,colorspace conversion etc...
 *
 *************************************************************************************
 */

#ifndef WELS_PREPROCESS_H
#define WELS_PREPROCESS_H

#include "../../interface/IWelsVP.h"

class IWelsVpPlugin
{
public:
	IWelsVpPlugin(int &ret);
	~IWelsVpPlugin();

	enum
	{
		STATE_BEFOREENC = 0, /* before picture encoding */
		STATE_AFTERENC     , /* after picture encoded */
	};

public:
	vResult Init    (int nType, void *pCfg);
	vResult Uninit  (int nType);
	vResult Flush   (int nType);
	vResult Process (int nType, vPixMap *src, vPixMap *dst);
	vResult Get     (int nType, void *pParam);
	vResult Set     (int nType, void *pParam);
	vResult SpecialFeature (int nType, void *pIn, void *pOut);

	void SetFlag(int a)   { flag = a; }
	void GetFlag(int &a)  { a = flag; }

private:
	int      flag;
	IWelsVP  *ivp;
	void     *hlib;
	void     *iface[2];
};

#endif