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

#ifndef NV_STD_RAND_H
#define NV_STD_RAND_H

#include "attributes.h"
#include "errorcodes.h"
#include "stdafx.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

NOVA_HEADER_START

/**
 * The type that this library works with
 * Also the type of the seed
 * Note that this isn't a changeable constant.
 * Most random functions are specialized to bit width, and
 * so would require specialization which I do not have the time to do.
 */
typedef u64                 nv_rand_t;
typedef struct nv_rand_info nv_rand_info_t;

struct nv_rand_info
{
  nv_rand_t state[4];
};

/**
 * For 1000000000 iterations, normal rand() takes 14.829721 seconds while this takes 3.230303 seconds at -O2.
 * Yeah, the results are skewed but shut up, blazingly fast.
 */
extern nv_error nv_random_bulk_range(nv_rand_info_t* info, nv_rand_t* outbuf, size_t buf_num_elements, size_t min, size_t max) NOVA_ATTR_NONNULL(1, 2);

static inline NOVA_ATTR_NONNULL(1) nv_rand_t nv_random(nv_rand_info_t* info)
{
  nv_rand_t buf[1];
  nv_random_bulk_range(info, buf, 1, 0, SIZE_MAX);
  return buf[0];
}

static inline NOVA_ATTR_NONNULL(1) nv_rand_t nv_random_range(nv_rand_info_t* info, nv_rand_t min, nv_rand_t max)
{
  nv_rand_t buf[1];
  nv_random_bulk_range(info, buf, 1, min, max);
  return buf[0];
}

static inline NOVA_ATTR_NONNULL(1, 2) nv_error nv_random_bulk(nv_rand_info_t* info, nv_rand_t* outbuf, size_t buf_num_elements)
{
  return nv_random_bulk_range(info, outbuf, buf_num_elements, 0, SIZE_MAX);
}

/**
 * Seed the random number generator.
 * Do not use constants, use something like the current time
 * or the process id or something.
 */
extern void nv_random_seed(nv_rand_info_t* info, nv_rand_t seed) NOVA_ATTR_NONNULL(1);

NOVA_HEADER_END

#endif // NV_STD_RAND_H
