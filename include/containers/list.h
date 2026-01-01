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

#ifndef NV_STD_CONTAINERS_LIST_H
#define NV_STD_CONTAINERS_LIST_H

#include "../alloc.h"
#include "../attributes.h"
#include "../errorcodes.h"
#include "../hash.h"
#include "../stdafx.h"
#include "../types.h"

#include <stddef.h>

NOVA_HEADER_START

typedef struct nv_list
{
  u32    canary;
  size_t size;
  size_t capacity;
  size_t type_size;
  void*  data;
} nv_list_t;

/**
 * init_capacity may be 0
 */
extern nv_error nv_list_init(size_t type_size, size_t init_capacity, nv_list_t* list);
extern void     nv_list_destroy(nv_list_t* list);

/**
 * Duplicate the list's data and copy over its contents.
 * The returned pointer will have the size capacity * type_size.
 * The returned pointer is owned by the callee, and must be freed through nv_free().
 * WARNING: This function does not care about the lists capacity, only the number of elements in the list will be accomodated in the returned pointer.
 */
extern void* nv_list_duplicate_data(const nv_list_t* list);

static inline bool
nv_list_is_valid(const nv_list_t* arr)
{
  if (arr == NULL) { return false; }
  if (arr->canary != NOVA_CONT_CANARY) { return false; }
  if (arr->capacity > 0 && arr->data == NULL) { return false; }
  if (arr->type_size <= 0) { return false; }
  return true;
}

/**
 * Set the number of elements in the list to 'new_size'.
 * If you do not want to change the number of elements, only ensure that they can be accomodated, use nv_list_reserve().
 * Useful if you are manually copying the data inot the lists memory through nv_list_data()
 * The list will automatically be resized if need be.
 * \sa nv_list_data, nv_list_resize
 */
extern void nv_list_resize(nv_list_t* list, size_t new_size);

/**
 * Ensure atleast 'new_capacity' of elements can be contained in the list.
 * This does not change the actual list of the size.
 * \sa nv_list_resize
 */
extern void nv_list_reserve(nv_list_t* list, size_t new_capacity);

/**
 * Set the number of elements in the list to 0. This does not reduce the lists capacity.
 * Use nv_list_reserve(list, 0) to ensure the list has 0 capacity, for whatever reason you may have.
 */
extern void nv_list_clear(nv_list_t* list);

/**
 * Get the number of elements in the list.
 */
extern size_t nv_list_size(const nv_list_t* list);

/**
 * Get the number of elements that can be accomodated in the list before a resize.
 */
extern size_t nv_list_capacity(const nv_list_t* list);

/**
 * Get the element size passed through nv_list_init()
 */
extern size_t nv_list_type_size(const nv_list_t* list);

/**
 * Get the contiguous memory block where the list is storing its data.
 * This will always have the byte size = nv_list_capacity(list) * nv_list_type_size(list)
 */
extern void* nv_list_data(const nv_list_t* list);

/**
 * Get the first element in the list, NULL if 0 capacity
 */
extern void* nv_list_front(const nv_list_t* list);

/**
 * Get the last element in the list, NULL if 0 capacity
 */
extern void* nv_list_back(const nv_list_t* list);

/**
 * Get the element at index i, similar to how you'd index C++ vectors through vec[i]
 */
extern void* nv_list_get(const nv_list_t* list, size_t i);

/**
 * Copy the contents of 'src' to 'dst', replacing 'dst's contents entirely.
 */
extern void nv_list_copy_from(const nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst);

/**
 * Move the contents of 'src' to 'dst', where 'src' will have 0 elements after this call.
 * 'src' will still retain its capacity, but all its data will be memset to 0
 */
extern void nv_list_move_from(nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst);

/**
 * Is the list empty? i.e. 0 size
 */
extern bool nv_list_empty(const nv_list_t* list);

/**
 * Doesn't mean that the two vectors ptrs are pointing to the same vector
 * This'll check the size and the data only, not the capacity (why would you?)
 */
extern bool nv_list_equal(const nv_list_t* list1, const nv_list_t* list2);

/**
 * WARNING: sizeof(*elem) != vec->type_size is UNDEFINED!
 */
extern void nv_list_push_back(nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT elem);

/**
 * @brief Push a zero initialized member to the vec
 * @return rw pointer to the newly added element
 */
extern void* nv_list_push_empty(nv_list_t* NV_RESTRICT vec);

/**
 * Push an array of elements to the list. Ensure each element in the arr has the same typesize that was provided to the list.
 */
extern void nv_list_push_set(nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT arr, size_t count);

/**
 * Remove the last element of the list. This is very inexpensive.
 */
extern void nv_list_pop_back(nv_list_t* list);

/**
 * Remove the first element of the list. This is relatively expensive, as all elements after must be moved to replace the popped element.
 * Try restructuring your code to ensure this function need not be called. But for small lists, its okay.
 */
extern void nv_list_pop_front(nv_list_t* list);

/**
 * Insert an element into a specified index. The list will be resized automatically to accomodate the index.
 */
extern void nv_list_insert(nv_list_t* NV_RESTRICT vec, size_t index, const void* NV_RESTRICT elem);

/**
 * Remove an element from the list. The list will not be downscaled (capacity will not be reduced).
 */
extern void nv_list_remove(nv_list_t* list, size_t index);

/**
 * Find the index of an element in the list. SIZE_MAX if not found.
 */
extern size_t nv_list_find(const nv_list_t* NV_RESTRICT vec, const void* NV_RESTRICT elem);

/**
 * Sort all elements in the list, using the sort function 'compare' through qsort().
 */
extern void nv_list_sort(nv_list_t* list, nv_compare_fn compare);

NOVA_HEADER_END

#endif // NV_STD_CONTAINERS_LIST_H
