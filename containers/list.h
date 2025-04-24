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

#ifndef __NOVA_LIST_H__
#define __NOVA_LIST_H__

#include "../alloc.h"
#include "../errorcodes.h"
#include <SDL2/SDL_mutex.h>

NOVA_HEADER_START

#define CONT_CANARY 0xFEEF

typedef struct nv_list_t
{
  u32             canary;
  size_t          size;
  size_t          capacity;
  size_t          type_size;
  void*           data;
  SDL_mutex*      mutex;
  nv_allocator_fn alloc;
  void*           alloc_arg;
} nv_list_t;

/**
 * A comparison function for a sort.
 * return a positive value if obj_1 is greater than obj_2
 * return a negative value if obj_1 is lesser than obj_2
 */
typedef int (*nv_list_compare_fn)(const void* obj1, const void* obj2);

/**
 * init_capacity may be 0
 */
extern nv_error nv_list_init(size_t type_size, size_t init_capacity, nv_allocator_fn alloc, void* alloc_arg, nv_list_t* vec);
extern void     nv_list_destroy(nv_list_t* vec);

/*
  Returns 0 if the list is valid and anything else if it's not
*/
static inline bool
nv_list_is_valid(const nv_list_t* arr)
{
  if (arr == NULL)
  {
    return false;
  }
  if (arr->canary != CONT_CANARY)
  {
    return false;
  }
  if (arr->capacity > 0 && arr->data == NULL)
  {
    return false;
  }
  if (arr->type_size <= 0)
  {
    return false;
  }
  return true;
}

extern void nv_list_resize(nv_list_t* vec, size_t new_size);
extern void nv_list_clear(nv_list_t* vec);

extern size_t nv_list_size(const nv_list_t* vec);
extern size_t nv_list_capacity(const nv_list_t* vec);
extern size_t nv_list_type_size(const nv_list_t* vec);
extern void*  nv_list_data(const nv_list_t* vec);

extern void* nv_list_front(nv_list_t* vec);

extern void* nv_list_back(nv_list_t* vec);

extern void* nv_list_get(const nv_list_t* vec, size_t i);
extern void  nv_list_set(nv_list_t* vec, size_t i, void* elem);

// Overrides contents
extern void nv_list_copy_from(const nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst);

// src is destroyed and unusable after this call!
extern void nv_list_move_from(nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst);

extern bool nv_list_empty(const nv_list_t* vec);

/**
 * Doesn't mean that the two vectors ptrs are pointing to the same vector
 * This'll check the size and the data only, not the capacity (why would you?)
 */
extern bool nv_list_equal(const nv_list_t* vec1, const nv_list_t* vec2);

/**
 * WARNING: sizeof(*elem) != vec->type_size is UNDEFINED!
 */
extern void nv_list_push_back(nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT elem);

/**
 * @brief Push a zero initialized member to the vec
 * @return pointer to the newly added element
 */
extern void* nv_list_push_empty(nv_list_t* NV_RESTRICT vec);

extern void nv_list_push_set(nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT arr, size_t count);

extern void nv_list_pop_back(nv_list_t* vec);
extern void nv_list_pop_front(nv_list_t* vec); // expensive

extern void nv_list_insert(nv_list_t* NV_RESTRICT vec, size_t index, const void* NV_RESTRICT elem);
extern void nv_list_remove(nv_list_t* vec, size_t index);

extern int nv_list_find(const nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT elem);

/**
 * Internally calls qsort.
 */
extern void nv_list_sort(nv_list_t* vec, nv_list_compare_fn compare);

NOVA_HEADER_END

#endif //__NOVA_LIST_H__
