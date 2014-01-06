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
 * \file	cpu_core.h
 *
 * \brief	cpu core feature detection
 *
 * \date	4/24/2009 Created
 *
 *************************************************************************************
 */
#if !defined(WELS_CPU_CORE_FEATURE_DETECTION_H__)
#define WELS_CPU_CORE_FEATURE_DETECTION_H__

/*
 *	WELS CPU feature flags
 */
#define WELS_CPU_MMX        0x00000001    /* mmx */
#define WELS_CPU_MMXEXT     0x00000002    /* mmx-ext*/
#define WELS_CPU_SSE        0x00000004    /* sse */
#define WELS_CPU_SSE2       0x00000008    /* sse 2 */
#define WELS_CPU_SSE3       0x00000010    /* sse 3 */
#define WELS_CPU_SSE41      0x00000020    /* sse 4.1 */
#define WELS_CPU_3DNOW      0x00000040    /* 3dnow! */
#define WELS_CPU_3DNOWEXT   0x00000080    /* 3dnow! ext */
#define WELS_CPU_ALTIVEC    0x00000100    /* altivec */
#define WELS_CPU_SSSE3      0x00000200    /* ssse3 */
#define WELS_CPU_SSE42      0x00000400    /* sse 4.2 */

/* CPU features application extensive */
#define WELS_CPU_AVX		0x00000800	/* Advanced Vector eXtentions */
#define WELS_CPU_FPU		0x00001000	/* x87-FPU on chip */
#define WELS_CPU_HTT		0x00002000	/* Hyper-Threading Technology (HTT), Multi-threading enabled feature: 
										   physical processor package is capable of supporting more than one logic processor
										*/
#define WELS_CPU_CMOV		0x00004000	/* Conditional Move Instructions,
										   also if x87-FPU is present at indicated by the CPUID.FPU feature bit, then FCOMI and FCMOV are supported
										*/
#define WELS_CPU_MOVBE		0x00008000	/* MOVBE instruction */
#define WELS_CPU_AES		0x00010000	/* AES instruction extensions */
#define WELS_CPU_FMA		0x00020000	/* AVX VEX FMA instruction sets */

#define WELS_CPU_CACHELINE_16    0x10000000    /* CacheLine Size 16 */
#define WELS_CPU_CACHELINE_32    0x20000000    /* CacheLine Size 32 */
#define WELS_CPU_CACHELINE_64    0x40000000    /* CacheLine Size 64 */
#define WELS_CPU_CACHELINE_128   0x80000000    /* CacheLine Size 128 */

/*
 *	Interfaces for CPU core feature detection as below
 */

#endif//WELS_CPU_CORE_FEATURE_DETECTION_H__
