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
 * \file	mt_defs.h
 *
 * \brief	Main macros for multiple threading implementation
 *
 * \date	2/26/2010 Created
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
 *	Dynamic Slicing Assignment (DSA)
 */
#define DYNAMIC_SLICE_ASSIGN
/*
 *	Try to do dynamic slicing for multiple threads sync based on history slicing complexity result,
 *	valid in case DYNAMIC_SLICE_ASSIGN enabled. In case it is disabled using step interval slicing map for DSA
 */
#define TRY_SLICING_BALANCE
/*
 *	not absolute balancing, tolerant conditions for dynamic adjustment
 */
#define NOT_ABSOLUTE_BALANCING
/*
 *  using root mean square error of slice complexity ratios for balancing
 */
#define USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING

/*
 *  REQUIREMENT FROM NOT BEING ABLE TO SUPPORT ASO ON GPU BASED DECODER
 */
#define RASTER_SCAN_ORDER_PACKING	// Arbitary SSlice Ordering (ASO) exclusive

/*
 *	Parallel slice bs output without memcpy used
 *  NOTE: might be not applicable for SVC 2.0/2.1 client application layer implementation 
 *	due bs of various slices need be continuous within a layer packing
 */
//#define PACKING_ONE_SLICE_PER_LAYER	// MEAN packing only slice for a pLayerBs, disabled at SVC 2.0/2.1 in case Multi-Threading (MT) & Multi-SSlice (MS)

//#define FIXED_PARTITION_ASSIGN	// for dynamic slicing parallelization, mean same partition number used in P or I slices

/*
 * Need disable PACKING_ONE_SLICE_PER_LAYER if RASTER_SCAN_ORDER_PACKING enabled
 * PACKING_ONE_SLICE_PER_LAYER might potentially introduce disordering slice packing into layer info for application layer
 */
#if defined(RASTER_SCAN_ORDER_PACKING)
#if defined(PACKING_ONE_SLICE_PER_LAYER)
#undef PACKING_ONE_SLICE_PER_LAYER
#endif//PACKING_ONE_SLICE_PER_LAYER
#endif//RASTER_SCAN_ORDER_PACKING

/*
 *	MT_DEBUG: output trace MT related into log file
 */
//#define MT_DEBUG
//#define ENABLE_TRACE_MT

#ifdef MT_ENABLED

#define DYNAMIC_DETECT_CPU_CORES

//#if defined(WIN32)
//#define BIND_CPU_CORES_TO_THREADS	// if it is not defined here mean cross cpu cores load balance automatically
//#endif//WIN32

#else

#endif//MT_ENABLED

/*
 * TO Check macros dependencies MT related
 */

#if !defined(DYNAMIC_SLICE_ASSIGN)

#if defined(TRY_SLICING_BALANCE)
#undef TRY_SLICING_BALANCE
#endif//TRY_SLICING_BALANCE

#endif//!DYNAMIC_SLICE_ASSIGN

#if !defined(DYNAMIC_SLICE_ASSIGN) || !defined(TRY_SLICING_BALANCE)

#if defined(NOT_ABSOLUTE_BALANCING)
#undef NOT_ABSOLUTE_BALANCING
#endif//NOT_ABSOLUTE_BALANCING

#if defined(USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING)
#undef USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING
#endif//USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING

#endif//!DYNAMIC_SLICE_ASSIGN || !TRY_SLICING_BALANCE

#if !defined(MT_ENABLED)

#if defined(DYNAMIC_SLICE_ASSIGN)
#undef DYNAMIC_SLICE_ASSIGN
#endif//DYNAMIC_SLICE_ASSIGN
#if defined(TRY_SLICING_BALANCE)
#undef TRY_SLICING_BALANCE
#endif//TRY_SLICING_BALANCE
#if defined(MT_DEBUG)
#undef MT_DEBUG
#endif//MT_DEBUG
#if defined(ENABLE_TRACE_MT)
#undef ENABLE_TRACE_MT
#endif//ENABLE_TRACE_MT
#if defined(PACKING_ONE_SLICE_PER_LAYER)
#undef PACKING_ONE_SLICE_PER_LAYER
#endif//PACKING_ONE_SLICE_PER_LAYER
#ifdef NOT_ABSOLUTE_BALANCING
#undef NOT_ABSOLUTE_BALANCING
#endif//NOT_ABSOLUTE_BALANCING
#ifdef USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING
#undef USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING
#endif//USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING

#endif//!MT_ENABLED


#ifdef NOT_ABSOLUTE_BALANCING
#ifdef USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING
#define THRESHOLD_RMSE_CORE8	0.0320f	// v1.1: 0.0320f; v1.0: 0.02f
#define THRESHOLD_RMSE_CORE4	0.0215f	// v1.1: 0.0215f; v1.0: 0.03f
#define THRESHOLD_RMSE_CORE2	0.0200f	// v1.1: 0.0200f; v1.0: 0.04f
#else
#define TOLERANT_BALANCING_RATIO_LOSS	0.08f
#define TOLERANT_BALANCING_RATIO_LOWER(n)	((1.0f-TOLERANT_BALANCING_RATIO_LOSS)/(n))
#define TOLERANT_BALANCING_RATIO_UPPER(n)	((1.0f+TOLERANT_BALANCING_RATIO_LOSS)/(n))
#endif//USE_RMSE_SLICE_COMPLEXITY_RATIO_FOR_BALANCING
#endif//NOT_ABSOLUTE_BALANCING

typedef struct TagSliceThreadPrivateData {
	void		*pWelsPEncCtx;
	SLayerBSInfo	*pLayerBs;
	int32_t		iSliceIndex;	// slice index, zero based								
	int32_t		iThreadIndex;	// thread index, zero based

	// for dynamic slicing mode
	int32_t		iStartMbIndex;	// inclusive
	int32_t		iEndMbIndex;	// exclusive
} SSliceThreadPrivateData;

typedef struct TagSliceThreading 
{
	SSliceThreadPrivateData	*pThreadPEncCtx;// thread context, [iThreadIdx]
	WELS_THREAD_HANDLE			*pThreadHandles;// thread handles, [iThreadIdx]
#ifdef WIN32
	WELS_EVENT					*pSliceCodedEvent;// events for slice coded state, [iThreadIdx]
	WELS_EVENT					*pReadySliceCodingEvent;	// events for slice coding ready, [iThreadIdx]
	WELS_EVENT					*pFinSliceCodingEvent;	// notify slice coding thread is done
	WELS_EVENT					*pExitEncodeEvent;			// event for exit encoding event
#else
	WELS_EVENT*					pSliceCodedEvent[MAX_THREADS_NUM];// events for slice coded state, [iThreadIdx]
	WELS_EVENT*					pReadySliceCodingEvent[MAX_THREADS_NUM];	// events for slice coding ready, [iThreadIdx]
#endif//WIN32

#if defined(DYNAMIC_SLICE_ASSIGN) && defined(TRY_SLICING_BALANCE)
#if defined(__GNUC__)
	WELS_THREAD_HANDLE			*pUpdateMbListThrdHandles;	// thread handles for update mb list thread, [iThreadIdx]
#endif//__GNUC__
#ifdef WIN32
	WELS_EVENT					*pUpdateMbListEvent;		// signal to update mb list neighbor for various slices
	WELS_EVENT					*pFinUpdateMbListEvent;	// signal to indicate finish updating mb list
#else
	WELS_EVENT*					pUpdateMbListEvent[MAX_THREADS_NUM];		// signal to update mb list neighbor for various slices
	WELS_EVENT*					pFinUpdateMbListEvent[MAX_THREADS_NUM];	// signal to indicate finish updating mb list	
#endif//WIN32
#endif//#if defined(DYNAMIC_SLICE_ASSIGN) && defined(TRY_SLICING_BALANCE)

	WELS_MUTEX					mutexSliceNumUpdate;	// for dynamic slicing mode MT

#if defined(DYNAMIC_SLICE_ASSIGN) || defined(MT_DEBUG)
	uint32_t					*pSliceConsumeTime[MAX_DEPENDENCY_LAYER];	// consuming time for each slice, [iSpatialIdx][uiSliceIdx]
#endif//DYNAMIC_SLICE_ASSIGN || MT_DEBUG
#if defined(DYNAMIC_SLICE_ASSIGN) && defined(TRY_SLICING_BALANCE)
	float						*pSliceComplexRatio[MAX_DEPENDENCY_LAYER];
#endif//DYNAMIC_SLICE_ASSIGN && TRY_SLICING_BALANCE

#ifdef MT_DEBUG
	FILE						*pFSliceDiff;	// file handle for debug
#endif//MT_DEBUG

#ifdef PACKING_ONE_SLICE_PER_LAYER
	uint32_t					*pCountBsSizeInPartition;
#endif//PACKING_ONE_SLICE_PER_LAYER
} SSliceThreading;

#endif//MULTIPLE_THREADING_DEFINES_H__
