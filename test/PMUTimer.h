/*!
 * \copy
 *     Copyright (c)  2026, OpenH264 Contributors
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
 * \file    PMUTimer.h
 *
 * \brief   PMU performance measurement utility
 *
 *************************************************************************************
 */

#ifndef PMU_TIMER_H
#define PMU_TIMER_H

#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

class PMUTimer {
public:
    enum Mode {
        INSTRUCTIONS,
        CPU_CYCLES
    };

    PMUTimer(Mode mode = INSTRUCTIONS) : fd_(-1), mode_(mode) {
        init();
    }

    ~PMUTimer() {
        if (fd_ >= 0)
            close(fd_);
    }

    bool is_available() const { return fd_ >= 0; }

    void start() {
        if (fd_ >= 0) {
            ioctl(fd_, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
        }
    }

    void stop() {
        if (fd_ >= 0) {
            ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
        }
    }

    uint64_t read() {
        uint64_t value = 0;
        if (fd_ >= 0) {
            ::read(fd_, &value, sizeof(value));
        } else {
            // Fallback to cntvct_el0
            __asm__ volatile("isb\nmrs %0, cntvct_el0" : "=r"(value) :: "memory");
        }
        return value;
    }

    const char* mode_name() const {
        return mode_ == INSTRUCTIONS ? "instructions" : "cycles";
    }

private:
    void init() {
        struct perf_event_attr attr = {
            .type = PERF_TYPE_HARDWARE,
            .config = mode_ == INSTRUCTIONS ? PERF_COUNT_HW_INSTRUCTIONS : PERF_COUNT_HW_CPU_CYCLES,
            .disabled = 1,
            .exclude_kernel = 1,
            .exclude_hv = 1,
        };
        fd_ = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    }

    int fd_;
    Mode mode_;
};

#endif /* PMU_TIMER_H */