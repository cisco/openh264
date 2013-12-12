/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
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
 */

#ifndef ___LD_ST_MACROS___
#define ___LD_ST_MACROS___

#include "typedefs.h"

#ifdef __GNUC__

struct tagUnaligned_64 {
  uint64_t l;
} __attribute__ ((packed));
struct tagUnaligned_32 {
  uint32_t l;
} __attribute__ ((packed));
struct tagUnaligned_16 {
  uint16_t l;
} __attribute__ ((packed));

#define LD16(a) (((struct tagUnaligned_16 *) (a))->l)
#define LD32(a) (((struct tagUnaligned_32 *) (a))->l)
#define LD64(a) (((struct tagUnaligned_64 *) (a))->l)
//#define _USE_STRUCT_INT_CVT
//	#ifdef _USE_STRUCT_INT_CVT
#define ST16(a, b) (((struct tagUnaligned_16 *) (a))->l) = (b)
#define ST32(a, b) (((struct tagUnaligned_32 *) (a))->l) = (b)
#define ST64(a, b) (((struct tagUnaligned_64 *) (a))->l) = (b)
//	#else
//		inline void_t __ST16(void_t *dst, uint16_t v) { memcpy(dst, &v, 2); }
//		inline void_t __ST32(void_t *dst, uint32_t v) { memcpy(dst, &v, 4); }
//inline void_t __ST64(void_t *dst, uint64_t v) { memcpy(dst, &v, 8); }
//	#endif

#else

//#define INTD16(a) (*((int16_t*)(a)))
//#define INTD32(a) (*((int32_t*)(a)))
//#define INTD64(a) (*((int64_t*)(a)))

#define LD16(a) (*((uint16_t*)(a)))
#define LD32(a) (*((uint32_t*)(a)))
#define LD64(a) (*((uint64_t*)(a)))

#define ST16(a, b) *((uint16_t*)(a)) = (b)
#define ST32(a, b) *((uint32_t*)(a)) = (b)
#define ST64(a, b) *((uint64_t*)(a)) = (b)

#endif /* !__GNUC__ */

#ifndef INTD16
#define INTD16	LD16
#endif//INTD16

#ifndef INTD32
#define INTD32	LD32
#endif//INTD32

#ifndef INTD64
#define INTD64	LD64
#endif//INTD64

#endif//___LD_ST_MACROS___
