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
 * \file	thread.cpp
 *
 * \brief	Interfaces introduced in thread programming
 *
 * \date	11/17/2009 Created
 *
 *************************************************************************************
 */

#include "thread.h"

WELSVP_NAMESPACE_BEGIN

#if defined(WIN32)

WELS_THREAD_ERROR_CODE    WelsMutexInit (WELS_MUTEX*    mutex) {
  InitializeCriticalSection (mutex);

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexLock (WELS_MUTEX*    mutex) {
  EnterCriticalSection (mutex);

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexUnlock (WELS_MUTEX* mutex) {
  LeaveCriticalSection (mutex);

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsMutexDestroy (WELS_MUTEX* mutex) {
  DeleteCriticalSection (mutex);

  return WELS_THREAD_ERROR_OK;
}

#elif  defined(__GNUC__)

WELS_THREAD_ERROR_CODE    WelsMutexInit (WELS_MUTEX*    mutex) {
  return pthread_mutex_init (mutex, NULL);
}

WELS_THREAD_ERROR_CODE    WelsMutexLock (WELS_MUTEX*    mutex) {
  return pthread_mutex_lock (mutex);
}

WELS_THREAD_ERROR_CODE    WelsMutexUnlock (WELS_MUTEX* mutex) {
  return pthread_mutex_unlock (mutex);
}

WELS_THREAD_ERROR_CODE    WelsMutexDestroy (WELS_MUTEX* mutex) {
  return pthread_mutex_destroy (mutex);
}

#endif

WELSVP_NAMESPACE_END



