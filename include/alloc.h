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

#ifndef NV_STD_ALLOC_H
#define NV_STD_ALLOC_H

#include "attributes.h"
#include "stdafx.h"
#include "string.h"

#include <stddef.h>
#include <stdlib.h>

NOVA_HEADER_START

#define nv_stalloc(p) (nv_zmalloc(sizeof(*p)))

typedef struct nv_allocator nv_allocator_t;

// allocate memory initialized to zero
typedef void* (*nv_zalloc_fn)(nv_allocator_t* self, size_t size);

// realloc memory. the new section will NOT be initialized to zero
typedef void* (*nv_realloc_fn)(nv_allocator_t* self, void* oldptr, size_t size);

// Free an allocated block of memory
typedef void (*nv_free_fn)(nv_allocator_t* self, void* ptr);

/* Start with libc allocator. Change this to change default allocator */
#define NV_ALLOC_DEFAULT &nv_alloc_libc

struct nv_allocator
{
  nv_zalloc_fn  alloc;
  nv_realloc_fn realloc;
  nv_free_fn    free;
  // set to the stack context
  void* ctx; // context ptr ; used by the allocator
  void* ud;  // user data
};

typedef struct nv_stack_ctx
{
  uint8_t* buffer;
  size_t   size; // total size
  size_t   offset;
} nv_stack_ctx_t;

inline void*
nv_stack_zmalloc(nv_allocator_t* self, size_t size)
{
  nv_stack_ctx_t* ctx        = (nv_stack_ctx_t*)self->ctx;
  size_t          new_offset = ctx->offset + size;
  if (new_offset > ctx->size)
  {
    // OOM
    return NULL;
  }

  void* ptr   = ctx->buffer + ctx->offset;
  ctx->offset = new_offset;

  ///
  /// Zero out new memory block.
  /// We can't rely on the memory being clean from the start
  /// because if the user frees memory once (we don't want to zero out memory every free)
  /// Then the state of the memory is left undefined.
  ///
  nv_memset(ptr, 0, size);

  return ptr;
}

inline void*
nv_stack_realloc(nv_allocator_t* self, void* oldptr, size_t size)
{
  // only support last ptr realloc
  nv_stack_ctx_t* ctx     = (nv_stack_ctx_t*)self->ctx;
  uint8_t*        top_ptr = ctx->buffer + ctx->offset;
  if ((uint8_t*)oldptr + size == top_ptr) // oldptr == last ptr allocated
  {
    ctx->offset = ((uint8_t*)oldptr - ctx->buffer) + size;
    return oldptr;
  }

  // failed
  return NULL;
}

inline void
nv_stack_free(nv_allocator_t* self, void* ptr)
{
  nv_stack_ctx_t* ctx = (nv_stack_ctx_t*)self->ctx;
  // only allow freeing the last allocation
  if ((uint8_t*)ptr == ctx->buffer + ctx->offset - 1) { ctx->offset = (uint8_t*)ptr - ctx->buffer; }
}

static inline void*
nv_czmalloc(nv_allocator_t* self, size_t size)
{
  (void)self;
  return calloc(1, size);
}

static inline void*
nv_crealloc(nv_allocator_t* self, void* oldptr, size_t size)
{
  (void)self;
  return realloc(oldptr, size);
}

static inline void
nv_cfree(nv_allocator_t* self, void* ptr)
{
  (void)self;
  free(ptr);
}

static nv_allocator_t nv_alloc_libc = { .alloc = nv_czmalloc, .realloc = nv_crealloc, .free = nv_cfree };

static nv_allocator_t* nv_alloc_current = NV_ALLOC_DEFAULT;

// push a new allocator and get the previous one
static inline nv_allocator_t*
nv_push_allocator(nv_allocator_t* allocator)
{
  nv_allocator_t* old = nv_alloc_current;
  nv_alloc_current    = allocator;
  return old;
}

static inline void
nv_pop_allocator(nv_allocator_t* old)
{
  nv_alloc_current = old;
}

static inline void*
nv_zmalloc(size_t size)
{
  return nv_alloc_current->alloc(nv_alloc_current, size);
}
static inline void*
nv_memdup(void* ptr, size_t size)
{
  void* allocd = nv_alloc_current->alloc(nv_alloc_current, size);
  if (!allocd) return allocd;
  return nv_memmove(allocd, ptr, size);
}
static inline void*
nv_realloc(void* ptr, size_t size)
{
  return nv_alloc_current->realloc(nv_alloc_current, ptr, size);
}
static inline void
nv_free(void* ptr)
{
  nv_alloc_current->free(nv_alloc_current, ptr);
}

NOVA_HEADER_END

#endif // NV_STD_ALLOC_H
