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
 * \brief	Tool kits for decoder
 *		( malloc, realloc, free, log output and PSNR calculation and so on )
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_UTILS_H__
#define WELS_UTILS_H__

#include <stdarg.h>
#include "typedefs.h"

namespace WelsSVCEnc {


/*
 *	Log output routines
 */

typedef int32_t	iWelsLogLevel;
enum {
WELS_LOG_QUIET		= 0x00,		// Quiet mode
WELS_LOG_ERROR		= 1 << 0,	// Error log iLevel
WELS_LOG_WARNING	= 1 << 1,	// Warning log iLevel
WELS_LOG_INFO		= 1 << 2,	// Information log iLevel
WELS_LOG_DEBUG		= 1 << 3,	// Debug log iLevel
WELS_LOG_RESV		= 1 << 4,	// Resversed log iLevel
WELS_LOG_LEVEL_COUNT = 5,
WELS_LOG_DEFAULT	= WELS_LOG_ERROR | WELS_LOG_WARNING | WELS_LOG_INFO | WELS_LOG_DEBUG	// Default log iLevel in Wels codec
};

/*
 *	Function pointer declaration for various tool sets
 */
// wels log output
typedef void (*PWelsLogCallbackFunc) (void* pCtx, const int32_t iLevel, const str_t* kpFmt, va_list argv);

// wels psnr calc
typedef real32_t (*PWelsPsnrFunc) (const void* kpTarPic,
                                   const int32_t kiTarStride,
                                   const void* kpRefPic,
                                   const int32_t kiRefStride,
                                   const int32_t kiWidth,
                                   const int32_t kiHeight);

extern PWelsLogCallbackFunc	wlog;

#ifdef __GNUC__
extern void WelsLog (void* pCtx, int32_t iLevel, const str_t* kpFmt, ...) __attribute__ ((__format__ (__printf__, 3,
    4)));
#else
extern void WelsLog (void* pCtx, int32_t iLevel, const str_t* kpFmt, ...);
#endif

extern const str_t* g_sWelsLogTags[];

/*!
 *************************************************************************************
 * \brief	System trace log output in Wels
 *
 * \param	pCtx	instance pointer
 * \param	kiLevel	log iLevel ( WELS_LOG_QUIET, ERROR, WARNING, INFO, DEBUG )
 * \param	kpFmtStr	formated string to mount
 * \param 	argv	pData string argument
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void WelsLogDefault (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv);
void WelsLogNil (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv);


/*!
 *************************************************************************************
 * \brief	set log iLevel from external call
 *
 * \param	iLevel	iLevel of log
 *
 * \return	NONE
 *
 * \note	can be able to control log iLevel dynamically
 *************************************************************************************
 */
void WelsSetLogLevel (const int32_t kiLevel);

/*!
 *************************************************************************************
 * \brief	get log iLevel from external call
 *
 * \param	N/A
 *
 * \return	current iLevel of log used in codec internal
 *
 * \note	can be able to get log iLevel of internal codec applicable
 *************************************************************************************
 */
int32_t WelsGetLogLevel (void);

/*!
 *************************************************************************************
 * \brief	set log callback from external call
 *
 * \param	_log	log function routine
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void WelsSetLogCallback (PWelsLogCallbackFunc _log);

/*!
*************************************************************************************
* \brief	reopen log file when finish setting current path
*
* \param	pCtx		context pCtx
* \param	pCurPath	current path string
*
* \return	NONE
*
* \note	N/A
*************************************************************************************
*/
void WelsReopenTraceFile (void* pCtx, str_t* pCurPath);

/*
 *	PSNR calculation routines
 */
/*!
 *************************************************************************************
 * \brief	PSNR calculation utilization in Wels
 *
 * \param	kpTarPic		target picture to be calculated in Picture pData format
 * \param	kiTarStride	stride of target picture pData pBuffer
 * \param 	kpRefPic		base referencing picture samples
 * \param	kiRefStride	stride of reference picture pData pBuffer
 * \param	kiWidth		picture iWidth in pixel
 * \param	kiHeight		picture iHeight in pixel
 *
 * \return	actual PSNR result;
 *
 * \note	N/A
 *************************************************************************************
 */
real32_t WelsCalcPsnr (const void* kpTarPic,
                       const int32_t kiTarStride,
                       const void* kpRefPic,
                       const int32_t kiRefStride,
                       const int32_t kiWidth,
                       const int32_t kiHeight);

}
#endif//WELS_UTILS_H__
