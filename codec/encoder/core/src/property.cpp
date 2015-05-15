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
 * \file    property.c
 *
 * \brief   CODE name, library module and corresponding version are included
 *
 * \date    03/10/2009 Created
 *
 *************************************************************************************
 */
#include "property.h"
#include "crt_util_safe_x.h" // Safe CRT routines like utils for cross_platforms
namespace WelsEnc {
#define WELS_CODE_NAME  "Wels"
#define WELS_LIB_NAME   "Encoder"

#define WELS_VERSION_INT        0x000001        // v 0.0.1
#define WELS_VERSION_STR        "0.0.1"

#define WELS_BUILD_NUM          "090420"        // yymmdd

//////////////summary information//////////////

#define WELS_IDENT              WELS_CODE_NAME WELS_LIB_NAME "v" WELS_VERSION_STR "b" WELS_BUILD_NUM

/*!
 * \brief   get code name
 * \param   pBuf    pBuffer to restore code name
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetCodeName (char* pBuf, int32_t iSize) {
  int32_t iLen = 0;

  if (NULL == pBuf)
    return 0;

  iLen = (int32_t)strlen (WELS_CODE_NAME); // confirmed_safe_unsafe_usage
  if (iSize <= iLen)
    return 0;

  WelsStrncpy (pBuf, iSize, WELS_CODE_NAME); // confirmed_safe_unsafe_usage

  return iLen;
}

/*!
 * \brief   get library/module name
 * \param   pBuf    pBuffer to restore module name
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetLibName (char* pBuf, int32_t iSize) {
  int32_t iLen = 0;

  if (NULL == pBuf)
    return 0;

  iLen = (int32_t)strlen (WELS_LIB_NAME); // confirmed_safe_unsafe_usage
  if (iSize <= iLen)
    return 0;

  WelsStrncpy (pBuf, iSize, WELS_LIB_NAME); // confirmed_safe_unsafe_usage

  return iLen;
}

/*!
 * \brief   get version number
 * \param   pBuf    pBuffer to restore version number
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetVerNum (char* pBuf, int32_t iSize) {
  int32_t iLen = 0;

  if (NULL == pBuf)
    return 0;

  iLen = (int32_t)strlen (WELS_VERSION_STR); // confirmed_safe_unsafe_usage
  if (iSize <= iLen)
    return 0;

  WelsStrncpy (pBuf, iSize, WELS_VERSION_STR); // confirmed_safe_unsafe_usage

  return iLen;
}

/*!
 * \brief   get identify information
 * \param   pBuf    pBuffer to restore indentify information
 * \param   iSize   size of pBuffer overall
 * \return  actual size of pBuffer used; 0 returned in failure
 */
int32_t GetIdentInfo (char* pBuf, int32_t iSize) {
  int32_t iLen = 0;

  if (NULL == pBuf)
    return 0;

  iLen = (int32_t)strlen (WELS_IDENT); // confirmed_safe_unsafe_usage
  if (iSize <= iLen)
    return 0;

  WelsStrncpy (pBuf, iSize, WELS_IDENT); // confirmed_safe_unsafe_usage

  return iLen;
}

} // namespace WelsEnc
