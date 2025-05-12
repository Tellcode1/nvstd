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

#ifndef NOVA_SYSTEM_ALLOCATOR_H_INCLUDED_
#define NOVA_SYSTEM_ALLOCATOR_H_INCLUDED_

#include "stdafx.h"
#include "string.h"

#include <sys/mman.h>

NOVA_HEADER_START

#ifndef NOVA_SYSALLOC_PAGE_CAPACITY
#  define NOVA_SYSALLOC_PAGE_CAPACITY (4096 - sizeof(void*) - sizeof(void*))
#endif // NOVA_SYSALLOC_PAGE_CAPACITY

/**
 * 64 KiB on 64 bit, 32 KiB on 32 bit
 */
#ifndef NOVA_SYSALLOC_SUPER_BLOCK_CAPACITY
#  define NOVA_SYSALLOC_SUPER_BLOCK_CAPACITY (sizeof(void*) * 8192)
#endif // NOVA_SYSALLOC_SUPER_BLOCK_CAPACITY

#define NOVA_SYSALLOC_PAGE_HEADER_SIZE (sizeof(void*) * 2 + sizeof(size_t))

#define NOVA_SYSALLOC_PAGES_PER_BLOCK (NOVA_SYSALLOC_SUPER_BLOCK_CAPACITY / NOVA_SYSALLOC_PAGE_CAPACITY)

/**
 * A structure containing all of the data of the system allocator.
 */
typedef struct nv_sa_page      nv_sa_page_t;
typedef struct nv_sa_block     nv_sa_block_t;
typedef struct nv_sa_freeblock nv_sa_freeblock_t;

void* nv_sa_alloc(size_t size);
void  _sort_and_merge_free_blocks(struct nv_sa_page* page);
void* nv_sa_realloc(void* old_ptr, size_t new_size);
void  nv_sa_free(void* ptr);

struct nv_sa_block
{
  size_t             size;
  size_t             offset;
  struct nv_sa_page* page;
  size_t             alignment_exponent : 6;
};

struct nv_sa_freeblock
{
  size_t                  size;
  size_t                  offset;
  struct nv_sa_freeblock* next;
};

struct nv_sa_page
{
  struct nv_sa_freeblock* root_free_block;
  struct nv_sa_page*      next;
  size_t                  page_size;
  uchar                   capacity[NOVA_SYSALLOC_PAGE_CAPACITY];
};

NOVA_HEADER_END

#endif // NOVA_SYSTEM_ALLOCATOR_H_INCLUDED_
