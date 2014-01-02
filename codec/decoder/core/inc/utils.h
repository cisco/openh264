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
 * \file	utils.h
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "typedefs.h"

namespace WelsDec {


// cache line size
extern uint32_t g_uiCacheLineSize;

/*
 *	Function pointer declaration for various tool sets
 */
// wels log output
typedef void_t (*PWelsLogCallbackFunc) (void_t* pPtr, const int32_t kiLevel, const char* kpFmt, va_list pArgv);

extern PWelsLogCallbackFunc	g_pLog;

#ifdef __GNUC__
extern void_t WelsLog (void_t* pPtr, int32_t iLevel, const char* kpFmt, ...) __attribute__ ((__format__ (__printf__, 3,
    4)));
#else
extern void_t WelsLog (void_t* pPtr, int32_t iLevel, const char* kpFmt, ...);
#endif

#define DECODER_MODE_NAME(a) ((a == SW_MODE)?"SW_MODE":((a == GPU_MODE)?"GPU_MODE":((a == AUTO_MODE)?"AUTO_MODE":"SWITCH_MODE")))
#define OUTPUT_PROPERTY_NAME(a) ((a == 0)?"system_memory":"video_memory")
#define BUFFER_STATUS_NAME(a) ((a == 0)?"unvalid":"valid")


/*
 *	Log output routines
 */

typedef int32_t	WelsLogLevel;
enum {
  WELS_LOG_QUIET		= 0x00,		// Quiet mode
  WELS_LOG_ERROR		= 1 << 0,	// Error log level
  WELS_LOG_WARNING	= 1 << 1,	// Warning log level
  WELS_LOG_INFO		= 1 << 2,	// Information log level
  WELS_LOG_DEBUG		= 1 << 3,	// Debug log level
  WELS_LOG_RESV		= 1 << 4,	// Resversed log level
  WELS_LOG_LEVEL_COUNT = 5,
  WELS_LOG_DEFAULT	= WELS_LOG_ERROR | WELS_LOG_WARNING | WELS_LOG_INFO | WELS_LOG_DEBUG	// Default log level in Wels codec
};



} // namespace WelsDec

#endif//WELS_UTILS_H__
