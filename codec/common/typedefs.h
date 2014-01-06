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

// typedef.h
#ifndef WELS_TYPE_DEFINES_H__
#define WELS_TYPE_DEFINES_H__

#include <limits.h>

////////////////////////////////////////////////////////////////////////////
// NOTICE : ALL internal implement MUST use the data type defined as below
//          ONLY except with the interface file !!!!!
////////////////////////////////////////////////////////////////////////////

#ifndef  _MSC_VER

#include <stdint.h>

#else

// FIXME:     all singed type should be declared explicit,  for example,  int8_t should be declared as signed char.
typedef signed char      int8_t  ;
typedef unsigned char    uint8_t ;
typedef short            int16_t ;
typedef unsigned short   uint16_t;
typedef int              int32_t ;
typedef unsigned int     uint32_t;
typedef __int64          int64_t ;
typedef unsigned __int64 uint64_t;

#endif // _MSC_VER defined

// FIXME:     all string type should be declared explicit as char.
typedef char      str_t;
typedef float     real32_t;

#ifdef PESN
#undef PESN
#endif//PESN
#define PESN	  (0.000001f) // (1e-6)	// desired float precision

#ifndef NULL
#define NULL 0
#endif

typedef bool bool_t;
typedef int32_t BOOL_T;

#ifndef FALSE
#define FALSE   ((int32_t)0)
#endif//FALSE

#ifndef TRUE
#define TRUE    ((int32_t)1)
#endif//TRUE

#ifndef void_t
#define void_t void
#endif

#endif //WELS_TYPE_DEFINES_H__

