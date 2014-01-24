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
 * \file	macros.h
 *
 * \brief	MACRO based tool utilization
 *
 * \date	3/13/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_MACRO_UTILIZATIONS_H__
#define WELS_MACRO_UTILIZATIONS_H__

#include <math.h>
#include <assert.h>
#include "typedefs.h"

/*
* FORCE_STACK_ALIGN_1D: force 1 dimension local data aligned in stack
* _tp: type
* _nm: var name
* _sz: size
* _al: align bytes
* auxiliary var: _nm ## _tEmP
*/
#define FORCE_STACK_ALIGN_1D(_tp, _nm, _sz, _al) \
	_tp _nm ## _tEmP[(_sz)+(_al)-1]; \
	_tp *_nm = _nm ## _tEmP + ((_al)-1) - (((uintptr_t)(_nm ## _tEmP + ((_al)-1)) & ((_al)-1))/sizeof(_tp))


#define ENFORCE_STACK_ALIGN_2D(_tp, _nm, _cx, _cy, _al) \
	assert( ((_al) && !((_al) & ((_al) - 1))) && ((_al) >= sizeof(_tp)) ); /*_al should be power-of-2 and >= sizeof(_tp)*/\
	_tp _nm ## _tEmP[(_cx)*(_cy)+(_al)/sizeof(_tp)-1]; \
	_tp *_nm ## _tEmP_al = _nm ## _tEmP + ((_al)/sizeof(_tp)-1); \
	_nm ## _tEmP_al -= (((uintptr_t)_nm ## _tEmP_al & ((_al)-1))/sizeof(_tp)); \
	_tp (*_nm)[(_cy)] = (_tp (*)[(_cy)])_nm ## _tEmP_al;


#if defined(_MSC_VER)

#if(_MSC_VER < 1700)
#define inline	__inline
#endif

#define ALIGNED_DECLARE( type, var, n ) __declspec(align(n)) type var
#define __align16(t,v) __declspec(align(16)) t v
#elif defined(__GNUC__)
#define ALIGNED_DECLARE( type, var, n ) type var __attribute__((aligned(n)))
#define __align16(t,v) t v __attribute__ ((aligned (16)))
#endif//_MSC_VER

#if defined(_MACH_PLATFORM) || defined(__GNUC__)
#define ALIGNED_DECLARE_MATRIX_2D(name,sizex,sizey,type,alignment) \
	type name[(sizex)*(sizey)] __attribute__((aligned(alignment)))
#else //_MSC_VER <= 1200
#define ALIGNED_DECLARE_MATRIX_2D(name,sizex,sizey,type,alignment) \
__declspec(align(alignment)) type name[(sizex)*(sizey)]
#endif//#if _MACH_PLATFORM

#if defined(_MACH_PLATFORM) || defined(__GNUC__)
#define ALIGNED_DECLARE_MATRIX_1D(name,size,type,alignment) \
	type name[size] __attribute__((aligned(alignment)))
#else //_MSC_VER <= 1200
#define ALIGNED_DECLARE_MATRIX_1D(name,size,type,alignment) \
	__declspec(align(alignment)) type name[(size)]
#endif//#if _MACH_PLATFORM

#if defined(APPLE_IOS)
#define inline  //For iOS platform
#endif


#if 1
#ifndef	WELS_ALIGN
#define WELS_ALIGN(x, n)	(((x)+(n)-1)&~((n)-1))
#endif//WELS_ALIGN

#ifndef WELS_MAX
#define WELS_MAX(x, y)	((x) > (y) ? (x) : (y))
#endif//WELS_MAX

#ifndef WELS_MIN
#define WELS_MIN(x, y)	((x) < (y) ? (x) : (y))
#endif//WELS_MIN
#else

#ifndef	WELS_ALIGN
#define WELS_ALIGN(x, n)	(((x)+(n)-1)&~((n)-1))
#endif//WELS_ALIGN

#ifndef WELS_MAX
#define WELS_MAX(x, y)	((x) - (((x)-(y))&(((x)-(y))>>31)))
#endif//WELS_MAX

#ifndef WELS_MIN
#define WELS_MIN(x, y)	((y) + (((x)-(y))&(((x)-(y))>>31)))
#endif//WELS_MIN

#endif

#ifndef WELS_CEIL
#define WELS_CEIL(x)	ceil(x)	// FIXME: low complexity instead of math library used
#endif//WELS_CEIL

#ifndef WELS_FLOOR
#define WELS_FLOOR(x)	floor(x)	// FIXME: low complexity instead of math library used
#endif//WELS_FLOOR

#ifndef WELS_ROUND
#define WELS_ROUND(x)	((int32_t)(0.5f+(x)))
#endif//WELS_ROUND

#define WELS_NON_ZERO_COUNT_AVERAGE(nC,nA,nB) {		\
    nC = nA + nB + 1;                      \
	nC >>= (uint8_t)( nA != -1 && nB != -1);        \
	nC += (uint8_t)(nA == -1 && nB == -1);           \
}

static __inline int32_t CeilLog2 (int32_t i) {
int32_t s = 0;
i--;
while (i > 0) {
  s++;
  i >>= 1;
}
return s;
}
/*
the second path will degrades the performance
*/
#if 1
static inline int32_t WelsMedian (int32_t iX,  int32_t iY, int32_t iZ) {
int32_t iMin = iX, iMax = iX;

if (iY < iMin)
  iMin	= iY;
else
  iMax = iY;

if (iZ < iMin)
  iMin	= iZ;
else if (iZ > iMax)
  iMax	= iZ;

return (iX + iY + iZ) - (iMin + iMax);
}
#else
static inline int32_t WelsMedian (int32_t iX,  int32_t iY, int32_t iZ) {
int32_t iTmp = (iX - iY) & ((iX - iY) >> 31);
iX -= iTmp;
iY += iTmp;
iY -= (iY - iZ) & ((iY - iZ) >> 31);
iY += (iX - iY) & ((iX - iY) >> 31);
return iY;
}

#endif

#ifndef NEG_NUM
//#define NEG_NUM( num ) (-num)
#define NEG_NUM(iX) (1+(~(iX)))
#endif// NEG_NUM

#ifndef WELS_CLIP1
//#define WELS_CLIP1(x) (x & ~255) ? (-x >> 31) : x
#define WELS_CLIP1(iX) (((iX) & ~255) ? (-(iX) >> 31) : (iX)) //iX not only a value but also can be an expression
#endif//WELS_CLIP1


#ifndef WELS_SIGN
#define WELS_SIGN(iX) ((int32_t)(iX) >> 31)
#endif //WELS_SIGN
#ifndef WELS_ABS
#define WELS_ABS(iX) ((WELS_SIGN(iX) ^ (int32_t)(iX)) - WELS_SIGN(iX))
#endif //WELS_ABS

// WELS_CLIP3
#ifndef WELS_CLIP3
#define WELS_CLIP3(iX, iY, iZ) ((iX) < (iY) ? (iY) : ((iX) > (iZ) ? (iZ) : (iX)))
#endif //WELS_CLIP3

/*
 * Description: to check variable validation and return the specified result
 *	iResult:	value to be return
 *	bCaseIf:	negative condition to be verified
 */
#ifndef WELS_VERIFY_RETURN_IF
#define WELS_VERIFY_RETURN_IF(iResult, bCaseIf) \
	if ( bCaseIf ){ \
		return iResult; \
	}
#endif//#if WELS_VERIFY_RETURN_IF

/*
 *	Description: to check variable validation and return the specified result
 *		with correspoinding process advance.
 *	 result:	value to be return
 *	 case_if:	negative condition to be verified
 *	 proc:		process need perform
 */
#ifndef WELS_VERIFY_RETURN_PROC_IF
#define WELS_VERIFY_RETURN_PROC_IF(iResult, bCaseIf, fProc) \
	if ( bCaseIf ){ \
		fProc;	\
		return iResult;	\
	}
#endif//#if WELS_VERIFY_RETURN_PROC_IF

static inline int32_t WELS_LOG2 (uint32_t v) {
int32_t r = 0;
while (v >>= 1) {
  ++r;
}
return r;

}

#define CLIP3_QP_0_51(q)		WELS_CLIP3(q, 0, 51)	// ((q) < (0) ? (0) : ((q) > (51) ? (51) : (q)))
#define   CALC_BI_STRIDE(width,bitcount)  ((((width * bitcount) + 31) & ~31) >> 3)
#ifdef    WORDS_BIGENDIAN

static inline uint32_t ENDIAN_FIX (uint32_t x) {
return x;
}

#else


#if defined(_MSC_VER) && defined(_M_IX86)
static inline uint32_t ENDIAN_FIX (uint32_t x) {
__asm {
  mov   eax,  x
  bswap   eax
  mov   x,    eax
}
return x;
}
#else  // GCC
static inline uint32_t ENDIAN_FIX (uint32_t x) {
#ifdef X86_ARCH
__asm__ __volatile__ ("bswap %0":"+r" (x));
#else
x = ((x & 0xff000000) >> 24) | ((x & 0xff0000) >> 8) |
    ((x & 0xff00) << 8) | ((x & 0xff) << 24);
#endif
return x;
}


#endif

#endif
#ifndef BUTTERFLY1x2
#define BUTTERFLY1x2(b) (((b)<<8) | (b))
#endif//BUTTERFLY1x2

#ifndef BUTTERFLY2x4
#define BUTTERFLY2x4(wd) (((uint32_t)(wd)<<16) |(wd))
#endif//BUTTERFLY2x4

#ifndef BUTTERFLY4x8
#define BUTTERFLY4x8(dw) (((uint64_t)(dw)<<32) | (dw))
#endif//BUTTERFLY4x8

static inline int32_t WELS_MEDIAN (int32_t x,  int32_t y, int32_t z) {
int32_t t = (x - y) & ((x - y) >> 31);
x -= t;
y += t;
y -= (y - z) & ((y - z) >> 31);
y += (x - y) & ((x - y) >> 31);
return y;
}
static inline BOOL_T WELS_POWER2_IF (uint32_t v) {
return (v && ! (v & (v - 1)));
}


#endif//WELS_MACRO_UTILIZATIONS_H__
