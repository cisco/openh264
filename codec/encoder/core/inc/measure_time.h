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
 * \file	measure_time.h
 *
 * \brief	time cost measure utilization
 *
 * \date	04/28/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_TIME_COST_MEASURE_UTIL_H__
#define WELS_TIME_COST_MEASURE_UTIL_H__

#include <stdlib.h>

#if !(defined(_MSC_VER) || defined(__MINGW32__))
#include <sys/time.h>
#else
#include "typedefs.h"
//#include <sys/types.h>
#include <sys/timeb.h>
#endif
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
//#include <mmsystem.h>	// need static lib winmm.lib for link for such windows 95/98 mm timer
#endif//#if _WIN32

/*!
 * \brief	time cost measure utilization
 * \param	void
 * \return	time elapsed since run (unit: microsecond)
 */

static inline int64_t WelsTime() {
#if !(defined(_MSC_VER) || defined(__MINGW32__))
struct timeval tv_date;

gettimeofday (&tv_date, NULL);
return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
#else
#if defined (_WIN32)
static int64_t iMeasureTimeFreq = 0;
//	static BOOL_T support_high_resolution_perf_flag = TRUE;
int64_t iMeasureTimeCur = 0;
int64_t iResult = 0;
if (0 == iMeasureTimeFreq) {
  // Per MSDN minimum supported OS is Windows 2000 Professional/Server above for high-resolution performance counter
  /*BOOL_T ret = */QueryPerformanceFrequency ((LARGE_INTEGER*)&iMeasureTimeFreq);
//		if ( !ret )	// the installed hardware can not support a high-resolution performance counter, we have to use others instead for well feature
//		{
//			support_high_resolution_perf_flag	= FALSE;
//		}
  if (!iMeasureTimeFreq)
    iMeasureTimeFreq = 1;
}
//	if ( support_high_resolution_perf_flag )
//	{
QueryPerformanceCounter ((LARGE_INTEGER*)&iMeasureTimeCur);
iResult = (int64_t) ((double)iMeasureTimeCur * 1e6 / (double)iMeasureTimeFreq + 0.5);
//	}
//	else
//	{
//		iResult = timeGetTime() * 1000;	// 10 ms precision
//	}
return iResult;

#else
struct _timeb tb;

_ftime (&tb);
return ((int64_t)tb.time * (1000) + (int64_t)tb.millitm) * (1000);
#endif//#if _WIN32
#endif//!(defined(_MSC_VER) || defined(__MINGW32__))
}

#endif//WELS_TIME_COST_MEASURE_UTIL_H__
