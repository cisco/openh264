/*!
 * \copy
 *     Copyright (c)  2010-2013, Cisco Systems
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
 * \file    mt_defs.h
 *
 * \brief   Main macros for multiple threading implementation
 *
 * \date    2/26/2010 Created
 *
 *************************************************************************************
 */
#if !defined(MULTIPLE_THREADING_DEFINES_H__)
#define MULTIPLE_THREADING_DEFINES_H__

#include "typedefs.h"
#include "codec_app_def.h"
#include "wels_const.h"
#include "WelsThreadLib.h"

/*
 *  MT_DEBUG: output trace MT related into log file
 */
//#define MT_DEBUG
//#define ENABLE_TRACE_MT

#define THRESHOLD_RMSE_CORE8    0.0320f // v1.1: 0.0320f; v1.0: 0.02f
#define THRESHOLD_RMSE_CORE4    0.0215f // v1.1: 0.0215f; v1.0: 0.03f
#define THRESHOLD_RMSE_CORE2    0.0200f // v1.1: 0.0200f; v1.0: 0.04f

typedef struct TagSliceThreadPrivateData {
void*           pWelsPEncCtx;
SLayerBSInfo*   pLayerBs;
int32_t         iSliceIndex;    // slice index, zero based
int32_t         iThreadIndex;   // thread index, zero based

// for dynamic slicing mode
int32_t         iStartMbIndex;  // inclusive
int32_t         iEndMbIndex;    // exclusive
} SSliceThreadPrivateData;

typedef struct TagSliceThreading {
SSliceThreadPrivateData*        pThreadPEncCtx;// thread context, [iThreadIdx]
char eventNamespace[100];
WELS_THREAD_HANDLE              pThreadHandles[MAX_THREADS_NUM];// thread handles, [iThreadIdx]
WELS_EVENT                      pSliceCodedEvent[MAX_THREADS_NUM];// events for slice coded state, [iThreadIdx]
WELS_EVENT                      pSliceCodedMasterEvent; // events for signalling that some event in pSliceCodedEvent has been signalled
WELS_EVENT                      pReadySliceCodingEvent[MAX_THREADS_NUM];        // events for slice coding ready, [iThreadIdx]
WELS_EVENT                      pUpdateMbListEvent[MAX_THREADS_NUM];            // signal to update mb list neighbor for various slices
WELS_EVENT                      pFinUpdateMbListEvent[MAX_THREADS_NUM]; // signal to indicate finish updating mb list
WELS_EVENT                      pExitEncodeEvent[MAX_THREADS_NUM];                      // event for exit encoding event
WELS_EVENT                      pThreadMasterEvent[MAX_THREADS_NUM];    // event for indicating that some event has been signalled to the thread

WELS_MUTEX                      mutexSliceNumUpdate;    // for dynamic slicing mode MT

uint32_t*                       pSliceConsumeTime[MAX_DEPENDENCY_LAYER];        // consuming time for each slice, [iSpatialIdx][uiSliceIdx]
int32_t*                        pSliceComplexRatio[MAX_DEPENDENCY_LAYER]; // *INT_MULTIPLY

#ifdef MT_DEBUG
FILE*                           pFSliceDiff;    // file handle for debug
#endif//MT_DEBUG

} SSliceThreading;

#endif//MULTIPLE_THREADING_DEFINES_H__
