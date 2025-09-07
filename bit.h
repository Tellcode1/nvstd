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

/* Utilities to accessing bits in bitmaps and making them. */

#ifndef STD_BIT_H
#define STD_BIT_H

#include "stdafx.h"
#include "types.h"
#include <stdbool.h>
#include <stddef.h>

NOVA_HEADER_START

typedef uchar* nv_bitmap_t;
typedef uchar  nv_bit_t;

static inline size_t
nv_bytes_to_bits(size_t bytes)
{
  return bytes * 8;
}

static inline size_t
nv_bytes_to_kib(size_t bytes)
{
  return bytes * 1024;
}

static inline size_t
nv_bytes_to_mib(size_t bytes)
{
  return bytes * 1024 * 1024;
}

static inline size_t
nv_bytes_to_gib(size_t bytes)
{
  return bytes * 1024 * 1024 * 1024;
}

/* Get the size of the buffer needed, if it is to be interpreted as a bitmap */
static inline size_t
nv_get_equivalent_bitmap_size(size_t buffer_size)
{
  return (buffer_size + 7) / 8;
}

static inline nv_bit_t
nv_bitmap_check_bit(const nv_bitmap_t bitmap, size_t index)
{
  return (nv_bit_t)((bitmap[index / 8] & (1 << (index % 8))) != 0);
}

static inline void
nv_bitmap_set_bit(nv_bitmap_t bitmap, size_t index)
{
  bitmap[index / 8] |= 1 << (index % 8);
}

static inline void
nv_bitmap_clear_bit(nv_bitmap_t bitmap, size_t index)
{
  bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline void
nv_bitmap_set_bit_to(nv_bitmap_t bitmap, size_t index, nv_bit_t to)
{
  if (to == 1)
  {
    nv_bitmap_set_bit(bitmap, index);
  }
  else
  {
    nv_bitmap_clear_bit(bitmap, index);
  }
}

NOVA_HEADER_END

#endif // STD_BIT_H
