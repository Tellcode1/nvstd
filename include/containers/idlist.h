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

#ifndef NV_MAPPED_LIST_H
#define NV_MAPPED_LIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../error.h"
#include "../stdafx.h"
#include "../types.h"

  /**
   * An ID mapped list.
   * Each element is stored contiguously in memory, however:
   * A seperate array of actual indices are used to map to the objects.
   * This allows for fast deletion (swap && pop) and also near list like insertion.
   * HOWEVER: The order in which the objects were inserted is NOT preserved.
   *    If an element is deleted, the last element is always swapped with it, breaking the order.
   * Iteration is also as fast as lists/vectors, where we just iterate over the data pointer.
   * ID lists are effectively lists that store an additional table to map to the values.
   */
  typedef struct nv_id_list nv_id_list_t;

  /**
   * Initialize an ID list with 'init_capacity' elements of size 'type_size'.
   */
  nv_error nv_id_list_init(size_t type_size, size_t init_capacity, nv_id_list_t* idlist);
  void     nv_id_list_destroy(nv_id_list_t* idlist);

  /**
   * Resize an ID list, reserving space for the elements and ID table.
   */
  void nv_id_list_resize(size_t new_capacity, nv_id_list_t* idlist);

  /**
   * Iterate over an ID list.
   * ctx must be initialized with 0.
   * ctx must be a pointer to a size_t on the stack (or wherever).
   */
  void* nv_id_list_iter(size_t* ctx, nv_id_list_t* idlist);

  void* nv_id_list_get(size_t id, const nv_id_list_t* idlist);

  /**
   * Push a copy of the element in to the ID list.
   * Returns its ID.
   * Maintain the ID if you want to access that specific element.
   */
  size_t nv_id_list_push(const void* elem, nv_id_list_t* idlist);

  /**
   * Remove the last element in the ID list.
   */
  void nv_id_list_pop(nv_id_list_t* idlist);

  /**
   * Delete the element with the specified ID.
   * Swap the element with the last element (if it is not) and pop it.
   */
  bool nv_id_list_delete(size_t id, nv_id_list_t* idlist);

  struct nv_id_list
  {
    u32     canary; // == 0xFEF6324
    size_t  size;
    size_t* id_to_index;
    size_t* index_to_id;
    size_t  type_size;
    size_t  capacity;
    void*   data;
  };

#ifdef __cplusplus
}
#endif

#endif // NV_MAPPED_LIST_H
