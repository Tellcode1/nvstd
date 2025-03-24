/*
  MIT License

  Copyright (c) 2025 Tellcode

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

#ifndef __NOVA_ROVER_H__
#define __NOVA_ROVER_H__

#include "bit.h"
#include "stdafx.h"
#include "string.h"

typedef struct nv_rover_t            nv_rover_t;
typedef struct nv_rover_mem_header_t nv_rover_mem_header_t;

NOVA_HEADER_START

struct nv_rover_t
{
  size_t      m_buffer_size;
  uchar*      m_data;
  nv_bitmap_t m_dict;
  size_t      m_dict_size;
  uchar*      m_bumper;
};

struct nv_rover_mem_header_t
{
  size_t m_index;
  size_t m_block_size;
};

static inline bool
_nv_ptr_is_within_range(uchar* ptr, uchar* range, size_t size)
{
  return ptr >= range && ptr < (range + size);
}

static inline int
nv_rover_init(size_t buffer_size, uchar* buffer, nv_rover_t* dst)
{
  nv_assert_and_ret(dst != NULL, -1);
  nv_assert_and_ret(buffer != NULL, -1);
  nv_assert_and_ret(buffer_size > sizeof(size_t), -1);

  nv_bzero(dst, sizeof(nv_rover_t));

  dst->m_buffer_size = buffer_size;
  dst->m_dict_size   = buffer_size;
  dst->m_dict        = (uchar*)nv_calloc(nv_get_equivalent_bitmap_size(buffer_size));
  dst->m_data        = buffer;
  dst->m_bumper      = buffer;

  return 0;
}

static inline void
nv_rover_destroy(nv_rover_t* rover)
{
  if (!rover)
  {
    return;
  }

  if (rover->m_dict != NULL)
  {
    nv_free(rover->m_dict);
  }
  nv_bzero(rover, sizeof(nv_rover_t));
}

static inline void*
nv_rover_alloc(nv_rover_t* rover, size_t size)
{
  size_t total_size = size + sizeof(nv_rover_mem_header_t);

  for (size_t i = 0; i < rover->m_dict_size; i++)
  {
    if (rover->m_dict[i] == 0)
    {
      size_t num_free_bytes = 0;
      for (size_t j = i; j < rover->m_dict_size; j++)
      {
        if (rover->m_dict[j] != 0)
        {
          break;
        }
        num_free_bytes++;
      }

      if (num_free_bytes >= total_size)
      {
        for (size_t it = i; it < i + total_size; it++)
        {
          nv_bitmap_set_bit(rover->m_dict, it);
        }

        uchar* alloc = rover->m_bumper + sizeof(nv_rover_mem_header_t);

        size_t bumper_index = alloc - rover->m_data;
        for (size_t it = bumper_index; it < size; it++)
        {
          nv_bitmap_clear_bit(rover->m_dict, it);
        }

        *(nv_rover_mem_header_t*)rover->m_bumper = (nv_rover_mem_header_t){
          .m_index      = i,
          .m_block_size = total_size,
        };

        rover->m_bumper += total_size;
        return alloc;
      }
    }
  }

  return NULL;
}

static inline void
nv_rover_free(nv_rover_t* rover, void* alloc)
{
  nv_assert_and_ret(rover != NULL, );
  nv_assert_and_ret(alloc != NULL, );

  uchar* ptr = (uchar*)alloc - sizeof(nv_rover_mem_header_t);

  nv_assert_and_ret(_nv_ptr_is_within_range(ptr, rover->m_data, rover->m_buffer_size), );

  nv_rover_mem_header_t* header = (nv_rover_mem_header_t*)ptr;

  for (size_t it = 0; it < header->m_block_size; it++)
  {
    nv_bitmap_clear_bit(rover->m_dict, header->m_index + it);
  }
}

NOVA_HEADER_END

#endif //__NOVA_ROVER_H__
