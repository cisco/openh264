/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	    :  typedef.h
 *
 * \brief	    :  basic type definition
 *
 * \date        :  2011/01/04
 *
 * \description :  1. Define basic type with platform-independent;
 *                 2. Define specific namespace to avoid name pollution;
 *                 3. C++ ONLY;
 *
 *************************************************************************************
 */

#ifndef WELSVP_TYPEDEF_H
#define WELSVP_TYPEDEF_H

#define WELSVP_EXTERN_C_BEGIN       extern "C" {
#define WELSVP_EXTERN_C_END         }

#define WELSVP_NAMESPACE_BEGIN      namespace nsWelsVP {
#define WELSVP_NAMESPACE_END        }

#ifdef _MSC_VER
#include <stddef.h>
#else
#include <stdint.h>
#endif

WELSVP_NAMESPACE_BEGIN

#if defined(_MSC_VER)

typedef char               int8_t   ;
typedef unsigned char      uint8_t  ;
typedef short              int16_t  ;
typedef unsigned short     uint16_t ;
typedef int                int32_t  ;
typedef unsigned int       uint32_t ;
typedef __int64            int64_t  ;
typedef unsigned __int64   uint64_t ;
#if _MSC_VER < 1700
#define inline            __inline
#endif

#endif

typedef char    str_t    ; // [comment]: specific use plain char only for character parameters
typedef bool    bool_t   ;

#if defined(_WIN32) || defined(_MACH_PLATFORM) || defined(__GNUC__)
typedef float   float_t  ;
typedef double  double_t ;
#endif

#ifndef NULL
#define NULL    0
#endif

enum {
  FALSE = 0,
  TRUE  = !FALSE
};

WELSVP_NAMESPACE_END

#endif
