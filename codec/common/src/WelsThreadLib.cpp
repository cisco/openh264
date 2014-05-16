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
 * \file	WelsThreadLib.c
 *
 * \brief	Interfaces introduced in thread programming
 *
 * \date	11/17/2009 Created
 *
 *************************************************************************************
 */


#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#elif !defined(_WIN32)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <unistd.h>
#ifdef __APPLE__
#define HW_NCPU_NAME "hw.logicalcpu"
#else
#define HW_NCPU_NAME "hw.ncpu"
#endif
#endif
#ifdef ANDROID_NDK
#include <cpu-features.h>
#endif

#include "WelsThreadLib.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef  _WIN32

#ifdef WINAPI_FAMILY
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
#define USE_THREADPOOL

#define InitializeCriticalSection(x) InitializeCriticalSectionEx(x, 0, 0)
#define GetSystemInfo(x) GetNativeSystemInfo(x)
#define CreateEvent(attr, reset, init, name) CreateEventEx(attr, name, ((reset) ? CREATE_EVENT_MANUAL_RESET : 0) | ((init) ? CREATE_EVENT_INITIAL_SET : 0), EVENT_ALL_ACCESS)
#define WaitForSingleObject(a, b) WaitForSingleObjectEx(a, b, FALSE)
#define WaitForMultipleObjects(a, b, c, d) WaitForMultipleObjectsEx(a, b, c, d, FALSE)
#endif
#endif

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

#else /* _WIN32 */

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

#endif /* !_WIN32 */

#ifdef _WIN32

WELS_THREAD_ERROR_CODE    WelsEventOpen (WELS_EVENT* event, const char* event_name) {
  WELS_EVENT   h = CreateEvent (NULL, FALSE, FALSE, NULL);

  if (h == NULL) {
    return WELS_THREAD_ERROR_GENERAL;
  }
  *event = h;
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsEventSignal (WELS_EVENT* event) {
  if (SetEvent (*event)) {
    return WELS_THREAD_ERROR_OK;
  }
  return WELS_THREAD_ERROR_GENERAL;
}

WELS_THREAD_ERROR_CODE    WelsEventWait (WELS_EVENT* event) {
  return WaitForSingleObject (*event, INFINITE);
}

WELS_THREAD_ERROR_CODE    WelsEventWaitWithTimeOut (WELS_EVENT* event, uint32_t dwMilliseconds) {
  return WaitForSingleObject (*event, dwMilliseconds);
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitSingleBlocking (uint32_t nCount,
    WELS_EVENT* event_list, WELS_EVENT* master_event) {
  // Don't need/use the master event for anything, since windows has got WaitForMultipleObjects
  return WaitForMultipleObjects (nCount, event_list, FALSE, INFINITE);
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitAllBlocking (uint32_t nCount,
    WELS_EVENT* event_list, WELS_EVENT* master_event) {
  // Don't need/use the master event for anything, since windows has got WaitForMultipleObjects
  return WaitForMultipleObjects (nCount, event_list, TRUE, INFINITE);
}

WELS_THREAD_ERROR_CODE    WelsEventClose (WELS_EVENT* event, const char* event_name) {
  CloseHandle (*event);

  *event = NULL;
  return WELS_THREAD_ERROR_OK;
}


WELS_THREAD_ERROR_CODE    WelsThreadCreate (WELS_THREAD_HANDLE* thread,  LPWELS_THREAD_ROUTINE  routine,
    void* arg, WELS_THREAD_ATTR attr) {
#ifdef USE_THREADPOOL
  HANDLE h = CreateEvent (NULL, FALSE, FALSE, NULL);
  HANDLE h2;
  DuplicateHandle (GetCurrentProcess(), h, GetCurrentProcess(), &h2, 0, FALSE, DUPLICATE_SAME_ACCESS);
  ThreadPool::RunAsync (ref new WorkItemHandler ([ = ] (IAsyncAction^) {
    routine (arg);
    SetEvent (h2);
    CloseHandle (h2);
  }, CallbackContext::Any), WorkItemPriority::Normal, WorkItemOptions::TimeSliced);
#else
  WELS_THREAD_HANDLE   h = CreateThread (NULL, 0, routine, arg, 0, NULL);
#endif

  if (h == NULL) {
    return WELS_THREAD_ERROR_GENERAL;
  }
  * thread = h;

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE    WelsThreadJoin (WELS_THREAD_HANDLE  thread) {
  WaitForSingleObject (thread, INFINITE);
  CloseHandle (thread);

  return WELS_THREAD_ERROR_OK;
}


WELS_THREAD_HANDLE        WelsThreadSelf() {
  return GetCurrentThread();
}

WELS_THREAD_ERROR_CODE    WelsQueryLogicalProcessInfo (WelsLogicalProcessInfo* pInfo) {
  SYSTEM_INFO  si;

  GetSystemInfo (&si);

  pInfo->ProcessorCount = si.dwNumberOfProcessors;

  return WELS_THREAD_ERROR_OK;
}

#else

WELS_THREAD_ERROR_CODE    WelsThreadCreate (WELS_THREAD_HANDLE* thread,  LPWELS_THREAD_ROUTINE  routine,
    void* arg, WELS_THREAD_ATTR attr) {
  WELS_THREAD_ERROR_CODE err = 0;

  pthread_attr_t at;
  err = pthread_attr_init (&at);
  if (err)
    return err;
#ifndef __ANDROID__
  err = pthread_attr_setscope (&at, PTHREAD_SCOPE_SYSTEM);
  if (err)
    return err;
  err = pthread_attr_setschedpolicy (&at, SCHED_FIFO);
  if (err)
    return err;
#endif
  err = pthread_create (thread, &at, routine, arg);

  pthread_attr_destroy (&at);

  return err;
}

WELS_THREAD_ERROR_CODE    WelsThreadJoin (WELS_THREAD_HANDLE  thread) {
  return pthread_join (thread, NULL);
}

WELS_THREAD_HANDLE        WelsThreadSelf() {
  return pthread_self();
}

// unnamed semaphores aren't supported on OS X

WELS_THREAD_ERROR_CODE    WelsEventOpen (WELS_EVENT* p_event, const char* event_name) {
#ifdef __APPLE__
  if (p_event == NULL || event_name == NULL)
    return WELS_THREAD_ERROR_GENERAL;
  *p_event = sem_open (event_name, O_CREAT, (S_IRUSR | S_IWUSR)/*0600*/, 0);
  if (*p_event == (sem_t*)SEM_FAILED) {
    sem_unlink (event_name);
    *p_event = NULL;
    return WELS_THREAD_ERROR_GENERAL;
  } else {
    return WELS_THREAD_ERROR_OK;
  }
#else
  WELS_EVENT event = (WELS_EVENT) malloc (sizeof (*event));
  if (event == NULL)
    return WELS_THREAD_ERROR_GENERAL;
  WELS_THREAD_ERROR_CODE err = sem_init (event, 0, 0);
  if (!err) {
    *p_event = event;
    return err;
  }
  free (event);
  return err;
#endif
}
WELS_THREAD_ERROR_CODE    WelsEventClose (WELS_EVENT* event, const char* event_name) {
#ifdef __APPLE__
  WELS_THREAD_ERROR_CODE err = sem_close (*event);	// match with sem_open
  if (event_name)
    sem_unlink (event_name);
  return err;
#else
  WELS_THREAD_ERROR_CODE err = sem_destroy (*event);	// match with sem_init
  free (*event);
  return err;
#endif
}

WELS_THREAD_ERROR_CODE   WelsEventSignal (WELS_EVENT* event) {
  WELS_THREAD_ERROR_CODE err = 0;
//	int32_t val = 0;
//	sem_getvalue(event, &val);
//	fprintf( stderr, "before signal it, val= %d..\n",val );
  err = sem_post (*event);
//	sem_getvalue(event, &val);
//	fprintf( stderr, "after signal it, val= %d..\n",val );
  return err;
}

WELS_THREAD_ERROR_CODE   WelsEventWait (WELS_EVENT* event) {
  return sem_wait (*event);	// blocking until signaled
}

WELS_THREAD_ERROR_CODE    WelsEventWaitWithTimeOut (WELS_EVENT* event, uint32_t dwMilliseconds) {
  if (dwMilliseconds != (uint32_t) - 1) {
    return sem_wait (*event);
  } else {
#if defined(__APPLE__)
    int32_t err = 0;
    int32_t wait_count = 0;
    do {
      err = sem_trywait (*event);
      if (WELS_THREAD_ERROR_OK == err)
        break;// WELS_THREAD_ERROR_OK;
      else if (wait_count > 0)
        break;
      usleep (dwMilliseconds * 1000);
      ++ wait_count;
    } while (1);
    return err;
#else
    struct timespec ts;
    struct timeval tv;

    gettimeofday (&tv, 0);

    ts.tv_nsec = tv.tv_usec * 1000 + dwMilliseconds * 1000000;
    ts.tv_sec = tv.tv_sec + ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    return sem_timedwait (*event, &ts);
#endif//__APPLE__
  }
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitSingleBlocking (uint32_t nCount,
    WELS_EVENT* event_list, WELS_EVENT* master_event) {
  uint32_t nIdx = 0;
  uint32_t uiAccessTime = 2;	// 2 us once

  if (nCount == 0)
    return WELS_THREAD_ERROR_WAIT_FAILED;

  if (master_event != NULL) {
    // This design relies on the events actually being semaphores;
    // if multiple events in the list have been signalled, the master
    // event should have a similar count (events in windows can't keep
    // track of the actual count, but the master event isn't needed there
    // since it uses WaitForMultipleObjects).
    int32_t err = sem_wait (*master_event);
    if (err != WELS_THREAD_ERROR_OK)
      return err;
    uiAccessTime = 0; // no blocking, just quickly loop through all to find the one that was signalled
  }

  while (1) {
    nIdx = 0;	// access each event by order
    while (nIdx < nCount) {
      int32_t err = 0;
      int32_t wait_count = 0;

      /*
       * although such interface is not used in __GNUC__ like platform, to use
       * pthread_cond_timedwait() might be better choice if need
       */
      do {
        err = sem_trywait (event_list[nIdx]);
        if (WELS_THREAD_ERROR_OK == err)
          return WELS_THREAD_ERROR_WAIT_OBJECT_0 + nIdx;
        else if (wait_count > 0 || uiAccessTime == 0)
          break;
        usleep (uiAccessTime);
        ++ wait_count;
      } while (1);
      // we do need access next event next time
      ++ nIdx;
    }
    usleep (1);	// switch to working threads
    if (master_event != NULL) {
      // A master event was used and was signalled, but none of the events in the
      // list was found to be signalled, thus wait a little more when rechecking
      // the list to avoid busylooping here.
      // If we ever hit this codepath it's mostly a bug in the code that signals
      // the events.
      uiAccessTime = 2;
    }
  }

  return WELS_THREAD_ERROR_WAIT_FAILED;
}

WELS_THREAD_ERROR_CODE    WelsMultipleEventsWaitAllBlocking (uint32_t nCount,
    WELS_EVENT* event_list, WELS_EVENT* master_event) {
  uint32_t nIdx = 0;
  uint32_t uiCountSignals = 0;
  uint32_t uiSignalFlag	= 0;	// UGLY: suppose maximal event number up to 32

  if (nCount == 0 || nCount > (sizeof (uint32_t) << 3))
    return WELS_THREAD_ERROR_WAIT_FAILED;

  while (1) {
    nIdx = 0;	// access each event by order
    while (nIdx < nCount) {
      const uint32_t kuiBitwiseFlag = (1 << nIdx);

      if ((uiSignalFlag & kuiBitwiseFlag) != kuiBitwiseFlag) { // non-blocking mode
        int32_t err = 0;
//				fprintf( stderr, "sem_wait(): start to wait event %d..\n", nIdx );
        if (master_event == NULL) {
          err = sem_wait (event_list[nIdx]);
        } else {
          err = sem_wait (*master_event);
          if (err == WELS_THREAD_ERROR_OK) {
            err = sem_wait (event_list[nIdx]);
            if (err != WELS_THREAD_ERROR_OK) {
              // We successfully waited for the master event,
              // but waiting for the individual event failed (e.g. EINTR?).
              // Increase the master event count so that the next retry will
              // work as intended.
              sem_post (*master_event);
            }
          }
        }
//				fprintf( stderr, "sem_wait(): wait event %d result %d errno %d..\n", nIdx, err, errno );
        if (WELS_THREAD_ERROR_OK == err) {
//					int32_t val = 0;
//					sem_getvalue(&event_list[nIdx], &val);
//					fprintf( stderr, "after sem_timedwait(), event_list[%d] semaphore value= %d..\n", nIdx, val);

          uiSignalFlag |= kuiBitwiseFlag;
          ++ uiCountSignals;
          if (uiCountSignals >= nCount) {
            return WELS_THREAD_ERROR_OK;
          }
        }
      }
      // we do need access next event next time
      ++ nIdx;
    }
  }

  return WELS_THREAD_ERROR_WAIT_FAILED;
}

WELS_THREAD_ERROR_CODE    WelsQueryLogicalProcessInfo (WelsLogicalProcessInfo* pInfo) {
#ifdef ANDROID_NDK
  pInfo->ProcessorCount = android_getCpuCount();
  return WELS_THREAD_ERROR_OK;
#elif defined(__linux__)

  cpu_set_t cpuset;

  CPU_ZERO (&cpuset);

  if (!sched_getaffinity (0, sizeof (cpuset), &cpuset))
    pInfo->ProcessorCount = CPU_COUNT (&cpuset);
  else
    pInfo->ProcessorCount = 1;

  return WELS_THREAD_ERROR_OK;

#else

  size_t len = sizeof (pInfo->ProcessorCount);

  if (sysctlbyname (HW_NCPU_NAME, &pInfo->ProcessorCount, &len, NULL, 0) == -1)
    pInfo->ProcessorCount = 1;

  return WELS_THREAD_ERROR_OK;

#endif//__linux__
}

#endif


