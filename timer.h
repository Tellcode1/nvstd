/*
  MIT License

  Copyright (c) 2025 Fouzan MD Ishaque (fouzanmdishaque@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef STD_TIMER_H
#define STD_TIMER_H

// implementation: none

#include "attributes.h"
#include "stdafx.h"
#include <SDL3/SDL_timer.h>
#include <bits/types.h>
#include <stdbool.h>

NOVA_HEADER_START

#ifndef NV_TIMER_TIME_TYPE
#  define NV_TIMER_TIME_TYPE real_t
#endif

typedef struct nv_timer_s
{
  NV_TIMER_TIME_TYPE start;
  NV_TIMER_TIME_TYPE end;
} NOVA_ATTR_ALIGNED(16) nv_timer_t;

static inline NV_TIMER_TIME_TYPE
nv_timer_get_currtime(void)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return (float)tp.tv_nsec * 1.000000e-09;
}

// the timer will finish after 's' seconds
// pass __FLT_MAX__ for duration for it to just not end
static inline nv_timer_t
nv_timer_begin(NV_TIMER_TIME_TYPE duration_seconds)
{
  if (duration_seconds <= 0.0F)
  {
    nv_log_error("Timer duration passed as negative or zero. What do you even want the timer to do????\n");
    return (nv_timer_t){ -1, -1 };
  }
  nv_timer_t tmr = nv_zero_init(nv_timer_t);
  tmr.start      = nv_timer_get_currtime();
  tmr.end        = tmr.start + duration_seconds;
  return tmr;
}

// sets the end of the timer to -1
// resetting a timer is not required if it is going to be started again by
// nv_timer_begin()
static inline void
nv_timer_reset(nv_timer_t* tmr)
{
  tmr->start = -1;
  tmr->end   = -1;
}

static inline bool
nv_timer_is_done(const nv_timer_t* tmr)
{
  if (tmr->end == -1 || tmr->start == -1)
  {
    nv_log_warning("Timer end is invalid (it has been reset or was not created correctly). The timer needs to be restarted using nv_timer_begin()");
    return false;
  }

  return nv_timer_get_currtime() >= tmr->end;
}

static inline NV_TIMER_TIME_TYPE
nv_timer_time_since_start(const nv_timer_t* tmr)
{
  return (nv_timer_get_currtime() - tmr->start);
}

static inline NV_TIMER_TIME_TYPE
nv_timer_time_since_done(const nv_timer_t* tmr)
{
  return (nv_timer_get_currtime() - tmr->end);
}

NOVA_HEADER_END

#endif // STD_TIMER_H
