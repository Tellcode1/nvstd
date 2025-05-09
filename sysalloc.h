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

NOVA_HEADER_START

/**
 * 64 KiB on 64 bit, 32 KiB on 32 bit
 */
#ifndef NOVA_SYSALLOC_PAGE_SIZE
#  define NOVA_SYSALLOC_PAGE_SIZE (sizeof(void*) * 8192)
#endif // NOVA_SYSALLOC_PAGE_SIZE

/**
 * A structure containing all of the data of the system allocator.
 */
typedef struct nv_sysalloc      nv_sysalloc_t;
typedef struct nv_sysalloc_page nv_sysalloc_page_t;

struct nv_sysalloc
{
  nv_sysalloc_page_t* root_page;
};

struct nv_sysalloc_page
{

};

NOVA_HEADER_END

#endif // NOVA_SYSTEM_ALLOCATOR_H_INCLUDED_
