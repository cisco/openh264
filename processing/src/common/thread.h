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
 * \file	thread.h
 *
 * \brief	Interfaces introduced in thread programming
 *
 * \date	11/17/2009 Created 
 *
 *************************************************************************************
 */

#ifndef _WELSVP_THREAD_H
#define _WELSVP_THREAD_H

#include "typedef.h"

#if defined(WIN32)

#include <windows.h>

#elif defined(__GNUC__) 

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#endif//WIN32

WELSVP_NAMESPACE_BEGIN

#if defined(WIN32)

typedef  HANDLE            WELS_THREAD_HANDLE;
typedef  CRITICAL_SECTION  WELS_MUTEX;

#elif defined(__GNUC__) 

typedef   pthread_t         WELS_THREAD_HANDLE;
typedef   pthread_mutex_t   WELS_MUTEX;

#endif

typedef long_t WELS_THREAD_ERROR_CODE;

#define   WELS_THREAD_ERROR_OK					0
#define   WELS_THREAD_ERROR_GENERIAL			((unsigned long)(-1))
#define   WELS_THREAD_ERROR_WAIT_OBJECT_0		0
#define	  WELS_THREAD_ERROR_WAIT_TIMEOUT		((unsigned long)0x00000102L)  
#define	  WELS_THREAD_ERROR_WAIT_FAILED		    WELS_THREAD_ERROR_GENERIAL

WELS_THREAD_ERROR_CODE   WelsMutexInit( WELS_MUTEX   * mutex );
WELS_THREAD_ERROR_CODE   WelsMutexLock( WELS_MUTEX   * mutex );
WELS_THREAD_ERROR_CODE   WelsMutexUnlock( WELS_MUTEX * mutex );
WELS_THREAD_ERROR_CODE   WelsMutexDestroy( WELS_MUTEX * mutex );

WELSVP_NAMESPACE_END

#endif
