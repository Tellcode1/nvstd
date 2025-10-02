#include "../../include/alloc.h"
#include "../../include/errorcodes.h"
#include "../../include/stdafx.h"
#include "../../include/string.h"
#include "../../include/types.h"

#include "../../include/containers/list.h"

#include <SDL3/SDL_mutex.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

nv_error
nv_list_init(size_t type_size, size_t init_capacity, nv_allocator_fn alloc, void* alloc_arg, nv_list_t* list)
{
  nv_assert_else_return(type_size > 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(alloc != NULL, NV_ERROR_INVALID_ARG);

  *list = nv_zero_init(nv_list_t);

  list->size      = 0;
  list->type_size = type_size;
  list->canary    = NOVA_CONT_CANARY;

  list->mutex = SDL_CreateMutex();
  if (!list->mutex)
  {
    return NV_ERROR_EXTERNAL;
  }

  list->alloc     = alloc;
  list->alloc_arg = alloc_arg;

  SDL_LockMutex(list->mutex);
  if (init_capacity > 0)
  {
    list->data     = NV_ALLOC(list->alloc, alloc_arg, type_size * init_capacity);
    list->capacity = init_capacity;
  }
  else
  {
    list->data = NULL;
  }
  SDL_UnlockMutex(list->mutex);

  return NV_ERROR_SUCCESS;
}

void
nv_list_destroy(nv_list_t* list)
{
  if (list)
  {
    SDL_LockMutex(list->mutex);
    nv_assert(NOVA_CONT_IS_VALID(list));
    if (list->data)
    {
      list->alloc(list->alloc_arg, list->data, list->type_size * list->capacity, NV_ALLOC_FREE);
      SDL_UnlockMutex(list->mutex);
      SDL_DestroyMutex(list->mutex);
    }
  }
}

void
nv_list_clear(nv_list_t* list)
{
  if (!list)
  {
    return;
  }

  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  list->size = 0;
  SDL_UnlockMutex(list->mutex);
}

size_t
nv_list_size(const nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }

  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  size_t sz = list->size;
  SDL_UnlockMutex(list->mutex);
  return sz;
}

size_t
nv_list_capacity(const nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }

  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  size_t cap = list->capacity;
  SDL_UnlockMutex(list->mutex);
  return cap;
}

size_t
nv_list_type_size(const nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }
  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  size_t tsize = list->type_size;
  SDL_UnlockMutex(list->mutex);
  return tsize;
}

void*
nv_list_data(const nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }
  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  void* ptr = list->data;
  SDL_UnlockMutex(list->mutex);
  return ptr;
}

void*
nv_list_front(nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }
  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  void* ptr = nv_list_get(list, 0);
  SDL_UnlockMutex(list->mutex);
  return ptr;
}

void*
nv_list_back(nv_list_t* list)
{
  if (!list)
  {
    return 0;
  }
  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  void* ptr = nv_list_get(list, NV_MAX(1ULL, list->size) - 1); // stupid but works
  // that's how I'd describe the entirety of this projetc
  SDL_UnlockMutex(list->mutex);
  return ptr;
}

void*
nv_list_get(const nv_list_t* list, size_t i)
{
  if (!list)
  {
    return NULL;
  }

  SDL_LockMutex(list->mutex);
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), NULL);
  nv_assert_else_return(i < list->capacity, NULL);
  uchar* data      = list->data;
  size_t type_size = list->type_size;
  SDL_UnlockMutex(list->mutex);
  if (!data)
  {
    return NULL;
  }
  return data + (type_size * i);
}

void
nv_list_set(nv_list_t* list, size_t i, void* elem)
{
  SDL_LockMutex(list->mutex);
  nv_assert(NOVA_CONT_IS_VALID(list));
  nv_memcpy((char*)list + (list->type_size * i), elem, list->type_size);
  SDL_UnlockMutex(list->mutex);
}

void
nv_list_copy_from(const nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst)
{
  SDL_LockMutex(src->mutex);
  SDL_LockMutex(dst->mutex);

  nv_assert(NOVA_CONT_IS_VALID(src));
  nv_assert(NOVA_CONT_IS_VALID(dst));

  nv_assert(src->type_size == dst->type_size);
  if (src->size >= dst->capacity)
  {
    nv_list_reserve(dst, src->size);
  }
  dst->size = src->size;
  nv_memcpy(dst->data, src->data, src->size * src->type_size);

  SDL_UnlockMutex(src->mutex);
  SDL_UnlockMutex(dst->mutex);
}

void
nv_list_move_from(nv_list_t* NV_RESTRICT src, nv_list_t* NV_RESTRICT dst)
{
  SDL_LockMutex(src->mutex);
  SDL_LockMutex(dst->mutex);

  nv_assert(NOVA_CONT_IS_VALID(src));
  nv_assert(NOVA_CONT_IS_VALID(dst));

  dst->size     = src->size;
  dst->capacity = src->capacity;
  dst->data     = src->data;

  src->size     = 0;
  src->capacity = 0;
  src->data     = NULL;

  SDL_UnlockMutex(src->mutex);
  SDL_UnlockMutex(dst->mutex);
}

bool
nv_list_empty(const nv_list_t* list)
{
  nv_assert(NOVA_CONT_IS_VALID(list));
  return (list->size == 0);
}

bool
nv_list_equal(const nv_list_t* list1, const nv_list_t* list2)
{
  nv_assert(NOVA_CONT_IS_VALID(list1));
  nv_assert(NOVA_CONT_IS_VALID(list2));

  SDL_LockMutex(list1->mutex);
  SDL_LockMutex(list2->mutex);

  bool equal = 1;
  if ((list1->size != list2->size || list1->type_size != list2->type_size) || (nv_memcmp(list1->data, list2->data, list1->size * list1->type_size) != 0))
  {
    equal = 0;
  }

  SDL_UnlockMutex(list1->mutex);
  SDL_UnlockMutex(list2->mutex);

  return equal;
}

void
nv_list_resize(nv_list_t* list, size_t new_size)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );

  if (NV_UNLIKELY(new_size == 0))
  {
    nv_list_clear(list);
    return;
  }
  else if (new_size > list->capacity)
  {
    nv_list_reserve(list, NV_MAX(new_size, list->capacity * 2));
  }

  list->size = new_size;
}

void
nv_list_reserve(nv_list_t* list, size_t new_capacity)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );
  if (NV_UNLIKELY(new_capacity == 0))
  {
    nv_list_clear(list);
    return;
  }

  if (list->data)
  {
    list->data = NV_REALLOC(list->alloc, list->alloc_arg, list->data, list->type_size * list->capacity, list->type_size * new_capacity);
  }
  else
  {
    list->data = NV_ALLOC(list->alloc, list->alloc_arg, list->type_size * new_capacity);
  }
  nv_assert(list->data != NULL);

  list->capacity = new_capacity;
}

void
nv_list_push_back(nv_list_t* NV_RESTRICT list, const void* NV_RESTRICT elem)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );

  SDL_LockMutex(list->mutex);

  if (list->size >= list->capacity)
  {
    nv_list_reserve(list, NV_MAX(1, list->capacity * 2));
  }

  nv_assert(list->data != NULL);
  nv_assert(!(elem >= list->data && (unsigned char*)elem <= ((unsigned char*)list->data + list->size))); // breaks restriction rules
  nv_memcpy((uchar*)list->data + (list->size * list->type_size), elem, list->type_size);
  list->size++;

  SDL_UnlockMutex(list->mutex);
}

void*
nv_list_push_empty(nv_list_t* __restrict list)
{
  nv_assert(NOVA_CONT_IS_VALID(list));

  SDL_LockMutex(list->mutex);

  if (list->size >= list->capacity)
  {
    nv_list_reserve(list, NV_MAX(1, list->capacity * 2));
  }

  nv_assert(list->data != NULL);
  void* p = (uchar*)list->data + (list->size * list->type_size);
  nv_memset(p, 0, list->type_size);
  list->size++;

  SDL_UnlockMutex(list->mutex);

  return p;
}

void
nv_list_push_set(nv_list_t* NV_RESTRICT list, const void* NV_RESTRICT arr, size_t count)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );

  SDL_LockMutex(list->mutex);

  size_t required_capacity = list->size + count;
  if (required_capacity >= list->capacity)
  {
    nv_list_reserve(list, required_capacity);
  }
  nv_memcpy((uchar*)list->data + (list->size * list->type_size), arr, count * list->type_size);
  list->size += count;

  SDL_UnlockMutex(list->mutex);
}

void
nv_list_pop_back(nv_list_t* list)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );

  SDL_LockMutex(list->mutex);

  if (list->size > 0)
  {
    list->size--;
  }

  SDL_UnlockMutex(list->mutex);
}

void
nv_list_pop_front(nv_list_t* list)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(list), );

  SDL_LockMutex(list->mutex);

  if (list->size > 0)
  {
    list->size--;
    nv_memcpy(list->data, (uchar*)list->data + list->type_size, list->size * list->type_size);
  }

  SDL_UnlockMutex(list->mutex);
}

void
nv_list_insert(nv_list_t* NV_RESTRICT list, size_t index, const void* NV_RESTRICT elem)
{
  nv_assert(NOVA_CONT_IS_VALID(list));

  SDL_LockMutex(list->mutex);

  if (index >= list->capacity)
  {
    nv_list_reserve(list, NV_MAX(1, index * 2));
  }
  if (index >= list->size)
  {
    list->size = index + 1;
  }
  nv_memcpy((uchar*)list->data + (list->type_size * index), elem, list->type_size);

  SDL_UnlockMutex(list->mutex);
}

void
nv_list_remove(nv_list_t* list, size_t index)
{
  nv_assert(NOVA_CONT_IS_VALID(list));

  SDL_LockMutex(list->mutex);

  if (index >= list->size)
  {
    return;
  }

  size_t elements_to_move = list->size - index - 1;
  if (elements_to_move > 0)
  {
    uchar* dest          = (uchar*)list->data + (index * list->type_size);
    uchar* src           = (uchar*)list->data + ((index + 1) * list->type_size);
    size_t bytes_to_move = elements_to_move * list->type_size;

    nv_memcpy(dest, src, bytes_to_move);
  }

  list->size--;

  SDL_UnlockMutex(list->mutex);
}

size_t
nv_list_find(const nv_list_t* NV_RESTRICT list, const void* NV_RESTRICT elem)
{
  nv_assert(NOVA_CONT_IS_VALID(list));

  SDL_LockMutex(list->mutex);

  if (list->size == 0)
  {
    return SIZE_MAX;
  }
  if (list->size == 1)
  {
    if (nv_memcmp(list->data, elem, list->type_size) == 0)
    {
      return 0;
    }

    return SIZE_MAX;
  }

  /* start two probes, one from the front and one from behind */
  for (size_t i = 0; i < list->size; i++)
  {
    const size_t fwd_index = i;
    const size_t bck_index = (list->size - 1) - i;

    const void* fwd = (unsigned char*)list->data + (fwd_index * list->type_size);
    const void* bck = (unsigned char*)list->data + (bck_index * list->type_size);

    if (nv_memcmp(fwd, elem, list->type_size) == 0)
    {
      SDL_UnlockMutex(list->mutex);
      return fwd_index;
    }
    if (nv_memcmp(bck, elem, list->type_size) == 0)
    {
      SDL_UnlockMutex(list->mutex);
      return bck_index;
    }
  }

  SDL_UnlockMutex(list->mutex);
  return (size_t)-1;
}

void
nv_list_sort(nv_list_t* list, nv_list_compare_fn compare)
{
  nv_assert(NOVA_CONT_IS_VALID(list));

  SDL_LockMutex(list->mutex);

  qsort(list->data, list->size, list->type_size, compare);

  SDL_UnlockMutex(list->mutex);
}
