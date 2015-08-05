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
 * \file    property.h
 *
 * \brief   CODE name, library module and corresponding version are included
 *
 * \date    03/10/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_DECODER_PROPERTY_H__
#define WELS_DECODER_PROPERTY_H__

#include "typedefs.h"

namespace WelsEnc {


/*!
 * \brief   get code name
 * \param   pBuf    pBuffer to restore code name
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetCodeName (char* pBuf, int32_t iSize);

/*!
 * \brief   get library/module name
 * \param   pBuf    pBuffer to restore module name
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetLibName (char* pBuf, int32_t iSize);

/*!
 * \brief   get version number
 * \param   pBuf    pBuffer to restore version number
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetVerNum (char* pBuf, int32_t iSize);

/*!
 * \brief   get identify information
 * \param   pBuf    pBuffer to restore indentify information
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetIdentInfo (char* pBuf, int32_t iSize);
}
#endif//WELS_DECODER_PROPERTY_H__
