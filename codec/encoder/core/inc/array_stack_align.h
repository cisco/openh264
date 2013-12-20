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
 *
 * \file		array_stack_align.h
 *
 * \brief	promised alignment of array pData declaration on stack
 *			multidimensional array can be extended if applicable need
 *
 * \date		8/8/2011 Created
 *			8/12/2011 functionality implementation for multidimensional array
 *			8/26/2011 better solution with reducing extra memory used,
 *						stack size is adaptively reduced by _tp & _al
 *
 *************************************************************************************
 */
#ifndef ARRAY_STACK_ALIGN_H__
#define ARRAY_STACK_ALIGN_H__

#include <assert.h>
#include "typedefs.h"

/*
 * ENFORCE_STACK_ALIGN_1D: force 1 dimension local pData aligned in stack
 * _tp: type
 * _nm: var name
 * _sz: size
 * _al: align bytes
 * auxiliary var: _nm ## _tEmP
 * NOTE: _al should be power-of-2 and >= sizeof(_tp), before considering to use such macro
 */

//#define ENFORCE_STACK_ALIGN_1D(_tp, _nm, _sz, _al) \
//_tp _nm ## _tEmP[(_sz)+(_al)-1]; \
//_tp *_nm = _nm ## _tEmP + ((_al)-1); \
//_nm -= (((int32_t)_nm & ((_al)-1))/sizeof(_tp));

/* Another better solution with reducing extra memory used */
#define ENFORCE_STACK_ALIGN_1D(_tp, _nm, _sz, _al) \
assert( ((_al) && !((_al) & ((_al) - 1))) && ((_al) >= sizeof(_tp)) ); /*_al should be power-of-2 and >= sizeof(_tp)*/\
_tp _nm ## _tEmP[(_sz)+(_al)/sizeof(_tp)-1]; \
_tp *_nm = _nm ## _tEmP + ((_al)/sizeof(_tp)-1); \
_nm -= (((uintptr_t)_nm & ((_al)-1))/sizeof(_tp));

/*
 * ENFORCE_STACK_ALIGN_2D: force 2 dimension local pData aligned in stack
 * _tp: type
 * _nm: var name
 * _cx, _cy: size in x, y dimension
 * _al: align bytes
 * auxiliary var: _nm ## _tEmP, _nm ## _tEmP_al
 * NOTE: _al should be power-of-2 and >= sizeof(_tp), before considering to use such macro
 */

//#define ENFORCE_STACK_ALIGN_2D(_tp, _nm, _cx, _cy, _al) \
//_tp _nm ## _tEmP[(_cx)*(_cy)+(_al)-1]; \
//_tp *_nm ## _tEmP_al = _nm ## _tEmP + ((_al)-1); \
//_nm ## _tEmP_al -= (((int32_t)_nm ## _tEmP_al & ((_al)-1))/sizeof(_tp)); \
//_tp (*_nm)[(_cy)] = (_tp (*)[(_cy)])_nm ## _tEmP_al;

/* Another better solution with reducing extra memory used */
#define ENFORCE_STACK_ALIGN_2D(_tp, _nm, _cx, _cy, _al) \
assert( ((_al) && !((_al) & ((_al) - 1))) && ((_al) >= sizeof(_tp)) ); /*_al should be power-of-2 and >= sizeof(_tp)*/\
_tp _nm ## _tEmP[(_cx)*(_cy)+(_al)/sizeof(_tp)-1]; \
_tp *_nm ## _tEmP_al = _nm ## _tEmP + ((_al)/sizeof(_tp)-1); \
_nm ## _tEmP_al -= (((uintptr_t)_nm ## _tEmP_al & ((_al)-1))/sizeof(_tp)); \
_tp (*_nm)[(_cy)] = (_tp (*)[(_cy)])_nm ## _tEmP_al;

/*
 * ENFORCE_STACK_ALIGN_3D: force 3 dimension local pData aligned in stack
 * _tp: type
 * _nm: var name
 * _cx, _cy, _cz: size in x, y, z dimension
 * _al: align bytes
 * auxiliary var: _nm ## _tEmP, _nm ## _tEmP_al
 * NOTE: _al should be power-of-2 and >= sizeof(_tp), before considering to use such macro
 */

//#define ENFORCE_STACK_ALIGN_3D(_tp, _nm, _cx, _cy, _cz, _al) \
//_tp _nm ## _tEmP[(_cx)*(_cy)*(_cz)+(_al)-1]; \
//_tp *_nm ## _tEmP_al = _nm ## _tEmP + ((_al)-1); \
//_nm ## _tEmP_al -= (((int32_t)_nm ## _tEmP_al & ((_al)-1))/sizeof(_tp)); \
//_tp (*_nm)[(_cy)][(_cz)] = (_tp (*)[(_cy)][(_cz)])_nm ## _tEmP_al;

/* Another better solution with reducing extra memory used */
#define ENFORCE_STACK_ALIGN_3D(_tp, _nm, _cx, _cy, _cz, _al) \
assert( ((_al) && !((_al) & ((_al) - 1))) && ((_al) >= sizeof(_tp)) ); /*_al should be power-of-2 and >= sizeof(_tp)*/\
_tp _nm ## _tEmP[(_cx)*(_cy)*(_cz)+(_al)/sizeof(_tp)-1]; \
_tp *_nm ## _tEmP_al = _nm ## _tEmP + ((_al)/sizeof(_tp)-1); \
_nm ## _tEmP_al -= (((int32_t)_nm ## _tEmP_al & ((_al)-1))/sizeof(_tp)); \
_tp (*_nm)[(_cy)][(_cz)] = (_tp (*)[(_cy)][(_cz)])_nm ## _tEmP_al;

#endif//ARRAY_STACK_ALIGN_H__

