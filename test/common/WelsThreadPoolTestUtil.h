/*!
 * \copy
 *     Copyright (c)  2009-2026, Cisco Systems
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
 * \file    WelsThreadPoolTestUtil.h
 *
 * \brief   OS-agnostic utilities for thread pool testing (thread limits and signal handling)
 *
 */

#ifndef _WELS_THREAD_POOL_TEST_UTIL_H_
#define _WELS_THREAD_POOL_TEST_UTIL_H_

#include "typedefs.h"
#include <stdio.h>

#if defined(__linux__)
#include <sys/resource.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace WelsThreadPoolTestUtil {

struct SThreadLimitResult {
  bool bSupported;
  size_t uiLimit;
};

#if defined(__linux__)

static struct rlimit g_sOriginalRlimit;
static bool g_bOriginalRlimitSaved = false;
static struct sigaction g_sOriginalSigaction;
static bool g_bOriginalSigactionSaved = false;

inline void* DummyEmptyThreadRoutine (void*) {
  return NULL;
}

inline void DummySignalHandler (int) {
  // Do nothing, just interrupt system calls like sem_wait
}

inline SThreadLimitResult FindSingleThreadLimit() {
  SThreadLimitResult sRes = { false, 0 };
  if (0 != getrlimit (RLIMIT_NPROC, &g_sOriginalRlimit)) {
    return sRes;
  }
  g_bOriginalRlimitSaved = true;

  struct rlimit rl = g_sOriginalRlimit;
  int low = 1, high = (int)g_sOriginalRlimit.rlim_cur;
  while (low < high) {
    int mid = (low + high) / 2;
    rl.rlim_cur = mid;
    if (setrlimit (RLIMIT_NPROC, &rl) != 0) {
      low = mid + 1;
      continue;
    }
    pthread_t th;
    int res = pthread_create (&th, NULL, DummyEmptyThreadRoutine, NULL);
    if (res == 0) {
      pthread_join (th, NULL);
      high = mid;
    } else {
      low = mid + 1;
    }
  }

  sRes.bSupported = true;
  sRes.uiLimit = (size_t)low;
  return sRes;
}

inline bool SetThreadLimit (size_t uiLimit) {
  if (!g_bOriginalRlimitSaved) {
    if (0 != getrlimit (RLIMIT_NPROC, &g_sOriginalRlimit)) {
      return false;
    }
    g_bOriginalRlimitSaved = true;
  }
  struct rlimit rl = g_sOriginalRlimit;
  rl.rlim_cur = uiLimit;
  return (0 == setrlimit (RLIMIT_NPROC, &rl));
}

inline bool RemoveThreadLimit() {
  if (!g_bOriginalRlimitSaved) {
    return true;
  }
  return (0 == setrlimit (RLIMIT_NPROC, &g_sOriginalRlimit));
}

inline void SetupSignalHandler() {
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = DummySignalHandler;
  sa.sa_flags = 0; // Ensure SA_RESTART is NOT set so sem_wait returns EINTR
  sigemptyset (&sa.sa_mask);
  if (!g_bOriginalSigactionSaved) {
    sigaction (SIGUSR1, &sa, &g_sOriginalSigaction);
    g_bOriginalSigactionSaved = true;
  } else {
    sigaction (SIGUSR1, &sa, NULL);
  }
}

inline void RestoreSignalHandler() {
  if (g_bOriginalSigactionSaved) {
    sigaction (SIGUSR1, &g_sOriginalSigaction, NULL);
    g_bOriginalSigactionSaved = false;
  }
}

inline void SendSignalToOtherThreads() {
  for (int i = 0; i < 10; i++) {
    usleep (10000);
    DIR* dir = opendir ("/proc/self/task");
    if (dir) {
      struct dirent* entry;
      pid_t my_tid = syscall (SYS_gettid);
      pid_t my_pid = getpid();
      while ((entry = readdir (dir)) != NULL) {
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
          pid_t tid = atoi (entry->d_name);
          if (tid != my_tid) {
            syscall (SYS_tgkill, my_pid, tid, SIGUSR1);
          }
        }
      }
      closedir (dir);
    }
  }
}

#else

inline SThreadLimitResult FindSingleThreadLimit() {
  SThreadLimitResult sRes = { false, 0 };
  return sRes;
}

inline bool SetThreadLimit (size_t uiLimit) {
  return false;
}

inline bool RemoveThreadLimit() {
  return false;
}

inline void SetupSignalHandler() {
}

inline void RestoreSignalHandler() {
}

inline void SendSignalToOtherThreads() {
}

#endif

class CTestStateGuard {
 public:
  CTestStateGuard() {}
  ~CTestStateGuard() {
    RemoveThreadLimit();
    RestoreSignalHandler();
  }
};

typedef CTestStateGuard CThreadLimitGuard;

} // namespace WelsThreadPoolTestUtil

#endif
