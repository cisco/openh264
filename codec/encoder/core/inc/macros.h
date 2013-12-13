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

//#include <math.h>
#include "typedefs.h"
#if defined(WIN64) && defined(WIN32)
#undef WIN32
#endif
namespace WelsSVCEnc {
#if defined(_MSC_VER)
#if _MSC_VER <= 1200
#define ALLOC_ALLIGN_MEM(name,size,type,alignment) \
			type name##_storage[size+(alignment)-1]; \
			type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))

#define ALLOC_ALLIGN_MEM_2(name,sizex,sizey,type,alignment) \
			type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
			type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))
#else //_MSC_VER <= 1200
#define ALLOC_ALLIGN_MEM(name,size,type,alignment) \
			__declspec(align(alignment)) type name[size]

#define ALLOC_ALLIGN_MEM_2(name,sizex,sizey,type,alignment) \
		__declspec(align(alignment)) type name[(sizex)*(sizey)]
#endif//_MSC_VER <= 1200

#elif defined(__GNUC__)

#define ALLOC_ALLIGN_MEM(name,size,type,alignment) \
		type name[size] __attribute__((aligned(alignment)))
#define ALLOC_ALLIGN_MEM_2(name,sizex,sizey,type,alignment) \
		type name[(sizex)*(sizey)] __attribute__((aligned(alignment)))

#endif//_MSC_VER


#if defined(_MSC_VER)

#if(_MSC_VER < 1700)
#define inline	__inline
#endif

#define __FASTCALL   __fastcall
#define ALIGNED_DECLARE( type, var, n ) __declspec(align(n)) type var
#define __align8(t,v) __declspec(align(8)) t v
#define __align16(t,v) __declspec(align(16)) t v
#elif defined(__GNUC__)
#if !defined(MAC_POWERPC)
#define __FASTCALL    __attribute__ ((fastcall))
#else
#define __FASTCALL	// mean NULL for mac ppc
#endif//MAC_POWERPC    
#define ALIGNED_DECLARE( type, var, n ) type var __attribute__((aligned(n)))
#define __align8(t,v) t v __attribute__ ((aligned (8)))
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

//#if !defined(SIZEOFRGB24)
//#define SIZEOFRGB24(cx, cy)	(3 * (cx) * (cy))
//#endif//SIZEOFRGB24

//#if !defined(SIZEOFRGB32)
//#define SIZEOFRGB32(cx, cy)	(4 * (cx) * (cy))
//#endif//SIZEOFRGB32

#ifndef	WELS_ALIGN
#define WELS_ALIGN(x, n)	(((x)+(n)-1)&~((n)-1))
#endif//WELS_ALIGN

#ifndef WELS_MAX
//#define WELS_MAX(x, y)	((x) > (y) ? (x) : (y))
//#define WELS_MAX(x, y)	((x) - (((x)-(y))&(((x)-(y))>>31)))
#define WELS_MAX(x, y)	((x) ^ (((x)^(y))& -((x)<(y))))		// WELS_MAX(x, y)
#endif//WELS_MAX

#ifndef WELS_MIN
//#define WELS_MIN(x, y)	((x) < (y) ? (x) : (y))
//#define WELS_MIN(x, y)	((y) + (((x)-(y))&(((x)-(y))>>31)))
#define WELS_MIN(x, y)	((y) ^ (((x)^(y))& -((x)<(y))))		// WELS_MIN(x, y)
#endif//WELS_MIN

#ifndef WELS_ROUND
#define WELS_ROUND(x)	((int32_t)((x)+0.5f+EPSN))
#endif//WELS_ROUND

static inline int32_t WELS_CEIL (float v) {
const int32_t n = (int32_t)v;	// floor value
return ((v > EPSN + n) ? (1 + n) : n);	// (int32_t)ceil(v);
}

static inline int32_t WELS_FLOOR (float v) {
return (int32_t)v;
}


#define WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB) {	\
    iC = iA + iB + 1;                           \
	iC >>= (int32_t)( iA != -1 && iB != -1);    \
	iC += (iA == -1 && iB == -1);               \
}

/*
 * log base 2 of v and ceil/floor extension
 */

static inline int32_t WELS_CEILLOG2 (uint32_t v) {
int32_t r = 0;
--v;
while (v > 0) {
  ++r;
  v >>= 1;
}
return r;
}

static inline int32_t WELS_FLOORLOG2 (uint32_t v) {
int32_t r = 0;
while (v > 1) {
  ++r;
  v >>= 1;
}
return r;
}

static inline int32_t WELS_LOG2 (uint32_t v) {
int32_t r = 0;
while (v >>= 1) {
  ++r;
}
return r;

}

static inline BOOL_T WELS_POWER2_IF (uint32_t v) {
return (v && ! (v & (v - 1)));
}

static inline int32_t WELS_MEDIAN (int32_t x,  int32_t y, int32_t z) {
int32_t t = (x - y) & ((x - y) >> 31);
x -= t;
y += t;
y -= (y - z) & ((y - z) >> 31);
y += (x - y) & ((x - y) >> 31);
return y;
}

#ifndef BUTTERFLY1x2
#define BUTTERFLY1x2(b) (((b)<<8) | (b))
#endif//BUTTERFLY1x2

#ifndef BUTTERFLY2x4
#define BUTTERFLY2x4(wd) (((uint32_t)(wd)<<16) |(wd))
#endif//BUTTERFLY2x4

#ifndef BUTTERFLY4x8
#define BUTTERFLY4x8(dw) (((uint64_t)(dw)<<32) | (dw))
#endif//BUTTERFLY4x8

//when RS accumulation, should clip rs among range of [-255, 255]
#ifndef CLIP_RS
#define CLIP_RS( value ) ( WELS_MAX( WELS_MIN( value, 255 ), -255 ) )
#endif //CLIP_RS

//#ifndef NEG_NUM
//#define NEG_NUM( num ) (1+(~(num)))
//#endif// NEG_NUM

#ifndef WELS_CLIP1
#define WELS_CLIP1(x) (((x) & ~255) ? (-(x) >> 31) : (x))
#endif//WELS_CLIP1

#ifndef WELS_SIGN
#define WELS_SIGN(a) ((int32_t)(a) >> 31)	// General: (a)>>(sizeof(int)*CHAR_BIT-1), CHAR_BIT= the number of bits per byte (normally 8)
#endif //WELS_SIGN

static inline int32_t WELS_ABS (int32_t a) {
const int32_t sign = WELS_SIGN (a);
return ((a + sign) ^ sign);
}

// wels_tostring
//#ifndef wels_tostring
//#define wels_tostring(s)	#s
//#endif //wels_tostring

// WELS_CLIP3
#ifndef WELS_CLIP3
#define WELS_CLIP3(x, y, z)		((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))
#endif //WELS_CLIP3

#define CLIP3_QP_0_51(q)		WELS_CLIP3(q, 0, 51)	// ((q) < (0) ? (0) : ((q) > (51) ? (51) : (q)))

// Bitwise routines
// n: ulong
// b: bit order
static inline bool_t BITWISE_ENABLED (const uint32_t n, const uint8_t b) {
const uint8_t bit = (b & 0x1f);	// maximal bit position 31 for uint32_t 4 bytes
#if defined(WORDS_BIGENDIAN)
/*
 * 31 .. 24, 23 .. 16, 15 .. 8, 7 .. 0
 * 7 .. 0, 15 .. 8, 23 .. 16, 31 .. 24
 */
const uint8_t map = 24 + ((bit & 7) << 1) - bit;	// BIG_ENDIAN map
return (bool_t) ((n & (1 << map)) >> map);	// BIG_ENDIAN
#else
return ((n & (1 << bit)) >> bit) ? true : false;	// LITTLE_ENDIAN
#endif//WORDS_BIGENDIAN
}

#define   CALC_BI_STRIDE(width,bitcount)  ((((width * bitcount) + 31) & ~31) >> 3)

//////////////////////////////////////////////////////////

#ifdef    WORDS_BIGENDIAN

static inline uint32_t ENDIAN_FIX (uint32_t x) {
return x;
}

#else


#ifdef    WIN32
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

// wels_swap16

// wels_swap32

// sad, satd, avg might being in other header

/*
 * Description: to check variable validation and return the specified result
 *	result:		value to be return
 *	case_if:	negative condition to be verified
 */
#ifndef WELS_VERIFY_RETURN_IF
#define WELS_VERIFY_RETURN_IF(result, case_if) \
	if ( case_if ){ \
		return result; \
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
#define WELS_VERIFY_RETURN_PROC_IF(result, case_if, proc) \
	if ( case_if ){ \
		proc;	\
		return result;	\
	}
#endif//#if WELS_VERIFY_RETURN_PROC_IF

/*
 * Description:	to check variable validation and return
 *	case_if:	negtive condition to be verified
 *	return:		NONE
 */
#ifndef WELS_VERIFY_IF
#define WELS_VERIFY_IF(case_if) \
	if ( case_if ){ \
		return; \
	}
#endif//#if WELS_VERIFY_IF

/*
 * Description:	to check variable validation and return with correspoinding process advance.
 *	case_if:	negtive condition to be verified
 *	proc:		process need preform
 *	return:		NONE
 */
#ifndef WELS_VERIFY_PROC_IF
#define WELS_VERIFY_PROC_IF(case_if, proc) \
	if ( case_if ){ \
		proc; \
		return; \
	}
#endif//#if WELS_VERIFY_IF

/*
 * Description: to safe free a ptr with free function pointer
 *  p:			pointer to be destroyed
 *	free_fn:	free function pointer used
 */
#ifndef WELS_SAFE_FREE_P
#define WELS_SAFE_FREE_P(p, free_fn) \
	do{ \
		if ( NULL != (p) ){ \
			free_fn( (p) ); \
			(p) = NULL; \
		} \
	}while( 0 );
#endif//#if WELS_SAFE_FREE_P

/*
 * Description: to safe free an array ptr with free function pointer
 *	arr:		pointer to an array, something like "**p";
 *	num:		number of elements in array
 *  free_fn:	free function pointer
 */
#ifndef WELS_SAFE_FREE_ARR
#define WELS_SAFE_FREE_ARR(arr, num, free_fn) \
	do{ \
		if ( NULL != (arr) ){ \
			int32_t iidx = 0; \
			while( iidx < num ){ \
				if ( NULL != (arr)[iidx] ){ \
					free_fn( (arr)[iidx] ); \
					(arr)[iidx] = NULL; \
				} \
				++ iidx; \
			} \
			free_fn((arr)); \
			(arr) = NULL; \
		} \
	}while( 0 );
#endif//#if WELS_SAFE_FREE_ARR

}

#endif//WELS_MACRO_UTILIZATIONS_H__
