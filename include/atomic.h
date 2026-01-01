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

#ifndef NV_STD_ATOMIC_H
#define NV_STD_ATOMIC_H

#include "stdafx.h"

#include <stdbool.h>
#include <stdint.h>

NOVA_HEADER_START

#if defined(__STDC_NO_ATOMICS__) || (defined(_MSC_VER) && _MSC_VER < 1900)
#  define NV_NO_STD_ATOMICS
#endif

#if !defined(NV_NO_STD_ATOMICS)
#  include <stdatomic.h>

typedef _Atomic(int)       nv_atomic_int;
typedef _Atomic(unsigned)  nv_atomic_uint;
typedef _Atomic(long)      nv_atomic_long;
typedef _Atomic(uintptr_t) nv_atomic_ptr;
typedef _Atomic(bool)      nv_atomic_bool;

#  define nv_atomic_load(ptr) atomic_load(ptr)
#  define nv_atomic_store(ptr, val) atomic_store(ptr, val)
#  define nv_atomic_add(ptr, val) atomic_fetch_add(ptr, val)
#  define nv_atomic_sub(ptr, val) atomic_fetch_sub(ptr, val)
#  define nv_atomic_exchange(ptr, val) atomic_exchange(ptr, val)
#  define nv_atomic_cas(ptr, oldval, newval) atomic_compare_exchange_strong(ptr, &(oldval), newval)

#else // generally only for old compilers

#  if defined(_MSC_VER)
#    include <Windows.h>
typedef volatile long          nv_atomic_int;
typedef volatile unsigned long nv_atomic_uint;
typedef volatile long          nv_atomic_long;
typedef volatile bool          nv_atomic_bool;
typedef void*                  nv_atomic_ptr;

#    define nv_atomic_load(ptr) InterlockedCompareExchange(ptr, 0, 0)
#    define nv_atomic_store(ptr, val) InterlockedExchange(ptr, val)
#    define nv_atomic_add(ptr, val) InterlockedAdd(ptr, val)
#    define nv_atomic_sub(ptr, val) InterlockedAdd(ptr, -(val))
#    define nv_atomic_exchange(ptr, val) InterlockedExchange(ptr, val)
#    define nv_atomic_cas(ptr, oldval, newval) (InterlockedCompareExchange(ptr, newval, oldval) == (oldval))

#  elif defined(__GNUC__) || defined(__clang__)
typedef volatile int      nv_atomic_int;
typedef volatile unsigned nv_atomic_uint;
typedef volatile long     nv_atomic_long;
typedef volatile void*    nv_atomic_ptr;
typedef volatile bool     nv_atomic_bool;

#    define nv_atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#    define nv_atomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#    define nv_atomic_add(ptr, val) __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
#    define nv_atomic_sub(ptr, val) __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)
#    define nv_atomic_exchange(ptr, val) __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST)
#    define nv_atomic_cas(ptr, oldval, newval) __atomic_compare_exchange_n(ptr, &(oldval), 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#  else
#    error "No atomic support on this platform."
#  endif

#endif

NOVA_HEADER_END

#endif // NV_STD_ATOMIC_H
