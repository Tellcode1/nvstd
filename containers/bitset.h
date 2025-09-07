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

#ifndef STD_CONTAINERS_BITSET_H
#define STD_CONTAINERS_BITSET_H

#include "../alloc.h"
#include "../attributes.h"
#include "../errorcodes.h"
#include "../stdafx.h"
#include "../types.h"
#include <SDL3/SDL_mutex.h>
#include <stddef.h>

NOVA_HEADER_START

typedef struct nv_bitset nv_bitset_t;
typedef unsigned char    nv_bitset_bit;

extern nv_error nv_bitset_init(int init_capacity, nv_allocator_fn allocator, nv_bitset_t* set);
extern void     nv_bitset_set_bit(nv_bitset_t* set, int bitindex);
extern void     nv_bitset_set_bit_to(nv_bitset_t* set, int bitindex, nv_bitset_bit to);
extern void     nv_bitset_clear_bit(nv_bitset_t* set, int bitindex);
extern void     nv_bitset_toggle_bit(nv_bitset_t* set, int bitindex);
extern void     nv_bitset_copy_from(nv_bitset_t* dst, const nv_bitset_t* src);
extern void     nv_bitset_destroy(nv_bitset_t* set);

nv_bitset_bit nv_bitset_access_bit(nv_bitset_t* set, int bitindex);

struct nv_bitset
{
  u8*             data;
  size_t          size;
  nv_allocator_fn alloc;
  void*           alloc_arg;
  SDL_Mutex*      mutex;
} NOVA_ATTR_ALIGNED(64);

NOVA_HEADER_END

#endif // STD_CONTAINERS_BITSET_H
