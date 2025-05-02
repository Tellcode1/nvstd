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

/**
  * This allocator is blatantly stolen from lua!
  * You'd learn more about the allocator from lua than from me.
  * https://www.lua.org/ to learn more about lua

  The following is lua's license. I don't think I need to add it but still:

  Copyright © 1994–2024 Lua.org, PUC-Rio.

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
  persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
  OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
 * A standardized allocator interface for all of nova.
 * Based off of lua's allocator.
 * For more info: https://nullprogram.com/blog/2023/12/17/
 */

#ifndef NOVA_ALLOC_H_INCLUDED_
#define NOVA_ALLOC_H_INCLUDED_

#include "stdafx.h"

NOVA_HEADER_START

/**
 * The following defines, though techincally not needed and can be replaced by 0
 * are essential for the readability of the allocate function. I find lua's allocator to be very unreadable
 * wihtout those defines
 *
 * If a custom allocator is to be used, then it should be passed to user_data
 */

/**
 * When passed as new_size to allocator(), frees the pointer
 */
#define NV_ALLOC_FREE ((size_t)0)

/**
 * When passed as old_size to allocator(), allocates a new block of memory (malloc)
 */
#define NV_ALLOC_NEW_BLOCK ((size_t)-1)

#define NV_ALLOC(allocator, user_data, new_size) (allocator)(user_data, NULL, NV_ALLOC_NEW_BLOCK, new_size)
#define NV_REALLOC(allocator, user_data, old_ptr, old_size, new_size) (allocator)(user_data, old_ptr, old_size, new_size)
#define NV_FREE(allocator, user_data, old_ptr, old_size) (allocator)(user_data, old_ptr, old_size, NV_ALLOC_FREE)

/**
 * A simple yet effective allocator, blatantly stole from lua
 * - Allocates new memory if old_ptr is NULL and old_size is 0.
 * - Reallocates if old_ptr is non-NULL.
 * - Frees memory if new_size is 0.
 * old_ptr may not be NULL if old_size > 0 and vice versa
 * If a free is successful, returns old_ptr
 * If NV_ALLOC_FREE or NV_ALLOC_NEW_BLOCK aren't specified, then realloc is assumed
 */
typedef void* (*nv_allocator_fn)(void* user_data, void* old_ptr, size_t old_size, size_t new_size);

typedef struct nv_alloc_estack_s nv_alloc_estack_t;

/**
 * The C standard's allocator
 */
extern void* nv_allocator_c(void* user_data, void* old_ptr, size_t old_size, size_t new_size);

struct nv_alloc_estack_s
{
  unsigned char* buffer;      // << USER MUST WRITE >>
  size_t         buffer_size; // << USER MUST WRITE >>
  /**
   * Used for free'ing purposes
   */
  unsigned char* last_allocation; // << MUST BE ZERO INITIALIZED >>
  size_t         buffer_bumper;   // << MUST BE ZERO INITIALIZED >>

  bool using_heap_buffer; // << DO NOT MODIFY >>
};

/**
 * A custom, expandable stack allocator
 * It is legal to call this function for free'ing a memory block that is not the last
 * the function will simply exit and do nothing, but return old_ptr nonetheless
 * It is not legal to call new_size>old_size(realloc)
 */
extern void* nv_allocator_estack(void* user_data, void* old_ptr, size_t old_size, size_t new_size);

NOVA_HEADER_END

#endif //__NOVA_ALLOC_H__
