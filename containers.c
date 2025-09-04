#include "containers/rectpack.h"
#include "hash.h"
#include "stdafx.h"

#include "alloc.h"
#include "errorcodes.h"
#include "string.h"

#include "containers/bitset.h"
#include "containers/hashmap.h"
#include "containers/list.h"
#include "containers/rbmap.h"
#include "types.h"

#include <SDL3/SDL.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shutup_compiler_errno_warning_(void);

void
shutup_compiler_errno_warning_(void)
{
  (void)(errno);
}

/* find last quote in a line: "([^"]+)"(?!.*") */

// ==============================
// listTOR
// ==============================

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

// ==============================
// HASHMAP
// ==============================

static inline u32
next_power_of_two(u32 num)
{
  if (num == 0)
  {
    return 1;
  }
  num--;
  num |= num >> 1U;
  num |= num >> 2U;
  num |= num >> 4U;
  num |= num >> 8U;
  num |= num >> 16U;
  num++;
  return num;
}

static inline u32
power_of_two_mod(u32 num, u32 mod_by)
{
  return num & (mod_by - 1);
}

#define NV_NODE_OCCUPIED(node) ((node).key != NULL && (node).value != NULL)

nv_error
nv_hashmap_init(size_t init_size, size_t key_size, size_t value_size, nv_hash_fn hash_fn, nv_allocator_fn allocator, void* alloc_arg, nv_hashmap_t* dst)
{
  nv_assert_else_return(dst != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(key_size > 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(value_size > 0, NV_ERROR_INVALID_ARG);

  *dst = nv_zero_init(nv_hashmap_t);

  dst->mutex = SDL_CreateMutex();
  if (!dst->mutex)
  {
    return NV_ERROR_EXTERNAL;
  }

  // TODO(bird): Is this needed?
  SDL_LockMutex(dst->mutex);

  dst->alloc     = allocator;
  dst->alloc_arg = alloc_arg;
  dst->nodes     = init_size == 0 ? NULL : (nv_hashmap_node_t*)NV_ALLOC(allocator, alloc_arg, init_size * sizeof(nv_hashmap_node_t));
  nv_assert_else_return(dst->nodes != NULL, NV_ERROR_MALLOC_FAILED);

  dst->hash_fn    = hash_fn ? hash_fn : nv_hash_murmur3;
  dst->key_size   = key_size;
  dst->value_size = value_size;
  dst->capacity   = next_power_of_two(init_size);
  dst->size       = 0;
  dst->canary     = NOVA_CONT_CANARY;

  SDL_UnlockMutex(dst->mutex);

  return NV_ERROR_SUCCESS;
}

void
nv_hashmap_destroy(nv_hashmap_t* map)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  const size_t old_capacity = map->capacity;

  SDL_LockMutex(map->mutex);
  if (map->nodes)
  {
    for (size_t idx = 0; idx < map->capacity; idx++)
    {
      nv_hashmap_node_t* node = &map->nodes[idx];

      if (NV_NODE_OCCUPIED(*node))
      {
        void* node_key_and_value_allocation = (void*)node->key;
        NV_FREE(map->alloc, map->alloc_arg, node_key_and_value_allocation, map->key_size + map->value_size);
      }
    }
    NV_FREE(map->alloc, map->alloc_arg, (void*)map->nodes, old_capacity * sizeof(nv_hashmap_node_t));
    map->nodes = NULL;
  }
  SDL_UnlockMutex(map->mutex);
  SDL_DestroyMutex(map->mutex);
}

static inline void
nv_hashmap_resize_unsafe(nv_hashmap_t* map, size_t new_capacity, void* hash_fn_arg)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  nv_hashmap_node_t* old_nodes   = map->nodes;
  const size_t       old_entries = map->capacity;

  if (new_capacity <= 0)
  {
    new_capacity = 1;
  }

  const size_t old_capacity = map->capacity;

  map->capacity = next_power_of_two(new_capacity);
  map->size     = 0;

  // we can't do realloc here because we need to rehash all the nodes
  map->nodes = (nv_hashmap_node_t*)NV_ALLOC(map->alloc, map->alloc_arg, new_capacity * sizeof(nv_hashmap_node_t));
  nv_assert(map->nodes != NULL);

  if (old_nodes)
  {
    for (size_t i = 0; i < old_entries; i++)
    {
      nv_hashmap_node_t* old_node = &old_nodes[i];
      if (NV_NODE_OCCUPIED(*old_node))
      {
        nv_hashmap_insert(map, old_node->key, old_node->value, hash_fn_arg);
        NV_FREE(map->alloc, map->alloc_arg, old_node->key, map->key_size + map->value_size);
      }
    }
    NV_FREE(map->alloc, map->alloc_arg, (void*)old_nodes, old_capacity * sizeof(nv_hashmap_node_t));
    old_nodes = NULL;
  }
}

void
nv_hashmap_resize(nv_hashmap_t* map, size_t new_capacity, void* hash_fn_arg)
{
  SDL_LockMutex(map->mutex);
  nv_hashmap_resize_unsafe(map, new_capacity, hash_fn_arg);
  SDL_UnlockMutex(map->mutex);
}

void
nv_hashmap_clear(nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), );

  nv_hashmap_destroy(map);

  SDL_LockMutex(map->mutex);
  map->nodes    = NULL;
  map->size     = 0;
  map->capacity = 0;
  SDL_UnlockMutex(map->mutex);
}

size_t
nv_hashmap_size(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), 0);

  SDL_LockMutex(map->mutex);
  size_t size = map->size;
  SDL_UnlockMutex(map->mutex);

  return size;
}

size_t
nv_hashmap_capacity(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), 0);

  SDL_LockMutex(map->mutex);
  size_t capacity = map->capacity;
  SDL_UnlockMutex(map->mutex);

  return capacity;
}

size_t
nv_hashmap_keysize(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), 0);

  SDL_LockMutex(map->mutex);
  size_t key_size = map->key_size;
  SDL_UnlockMutex(map->mutex);

  return key_size;
}

size_t
nv_hashmap_valuesize(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), 0);

  SDL_LockMutex(map->mutex);
  size_t value_size = map->value_size;
  SDL_UnlockMutex(map->mutex);

  return value_size;
}

nv_hashmap_node_t*
nv_hashmap_iterate_unsafe(const nv_hashmap_t* map, size_t* _i)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), NULL);

  /**
   * If map->capacity is 0, it simply jumps to returning NULL
   */
  for (; (*_i) < map->capacity; (*_i)++)
  {
    size_t i = *_i;
    if (NV_NODE_OCCUPIED(map->nodes[i]))
    {
      (*_i)++;
      return &map->nodes[i];
    }
  }
  return NULL;
}

static inline void*
nv_hashmap_find_unsafe(const nv_hashmap_t* NV_RESTRICT map, const void* NV_RESTRICT key, void* hash_fn_arg)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  if (!map->nodes)
  {
    return NULL;
  }

  const u32 hash  = map->hash_fn(key, map->key_size, hash_fn_arg);
  const u32 begin = power_of_two_mod(hash, map->capacity);

  u32 index = begin;
  u32 probe = 1;

  while (NV_NODE_OCCUPIED(map->nodes[index]))
  {
    // if (map->equal_fn(map->nodes[index].key, key, map->key_size))
    if (map->nodes[index].hash == hash)
    {
      void* value = map->nodes[index].value;
      return value;
    }
    index = power_of_two_mod((hash + probe + (probe * probe)), map->capacity);
    if (index == begin)
    {
      break;
    }
    probe++;
  }

  return NULL;
}

void*
nv_hashmap_find(const nv_hashmap_t* NV_RESTRICT map, const void* NV_RESTRICT key, void* hash_fn_arg)
{
  SDL_LockMutex(map->mutex);
  void* found = nv_hashmap_find_unsafe(map, key, hash_fn_arg);
  SDL_UnlockMutex(map->mutex);
  return found;
}

static inline void
nv_hashmap_insert_internal_unsafe(nv_hashmap_t* map, const void* NV_RESTRICT key, const void* NV_RESTRICT value, bool replace_if_exists, void* hash_fn_arg)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  // the second check
  if (!map->nodes || (flt_t)map->size >= ((flt_t)map->capacity * NV_HASHMAP_LOAD_FACTOR))
  {
    // The check to whether map->entries is greater than 0 is already done in
    // resize();
    nv_hashmap_resize(map, map->capacity * 2, hash_fn_arg);
  }

  const u32 hash  = map->hash_fn(key, map->key_size, hash_fn_arg);
  const u32 begin = power_of_two_mod(hash, map->capacity);

  u32 index = begin;
  u32 probe = 1;

  while (NV_NODE_OCCUPIED(map->nodes[index]))
  {
    index = power_of_two_mod((hash + probe + (probe * probe)), map->capacity);
    if (hash == map->nodes[index].hash && nv_memcmp(map->nodes[index].key, key, map->key_size) == 0 && replace_if_exists)
    {
      nv_memcpy(map->nodes[index].value, value, map->value_size);
      return;
    }
    if (index == begin)
    {
      return;
    }
    probe++;
  }

  map->nodes[index].key   = NV_ALLOC(map->alloc, map->alloc_arg, map->key_size + map->value_size);
  map->nodes[index].value = (char*)map->nodes[index].key + map->key_size;
  map->nodes[index].hash  = hash;

  nv_memcpy(map->nodes[index].key, key, map->key_size);
  nv_memcpy(map->nodes[index].value, value, map->value_size);
  map->size++;
}

void
nv_hashmap_insert(nv_hashmap_t* map, const void* NV_RESTRICT key, const void* NV_RESTRICT value, void* hash_fn_arg)
{
  SDL_LockMutex(map->mutex);
  // TODO(bird): This should not be structured like this????
  nv_hashmap_insert_internal_unsafe(map, key, value, 0, hash_fn_arg);
  SDL_UnlockMutex(map->mutex);
}

void
nv_hashmap_insert_or_replace(nv_hashmap_t* map, const void* NV_RESTRICT key, void* NV_RESTRICT value, void* hash_fn_arg)
{
  SDL_LockMutex(map->mutex);
  nv_hashmap_insert_internal_unsafe(map, key, value, 1, hash_fn_arg);
  SDL_UnlockMutex(map->mutex);
}

void
nv_hashmap_serialize(nv_hashmap_t* map, FILE* f)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  SDL_LockMutex(map->mutex);

  const size_t key_size = map->key_size;
  const size_t val_size = map->value_size;

  for (size_t i = 0; i < map->capacity; i++)
  {
    if (NV_NODE_OCCUPIED(map->nodes[i]))
    {
      void* node_key   = map->nodes[i].key;
      void* node_value = map->nodes[i].value;

      fwrite(node_value, val_size, 1, f);
      fwrite(node_key, key_size, 1, f);
    }
  }

  SDL_UnlockMutex(map->mutex);
}

void
nv_hashmap_deserialize(nv_hashmap_t* map, FILE* f, void* hash_fn_arg)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  SDL_LockMutex(map->mutex);

  void* key   = nv_malloc(map->key_size);
  void* value = nv_malloc(map->value_size);

  while (fread(value, map->value_size, 1, f) == 1 && fread(key, map->key_size, 1, f) == 1)
  {
    SDL_UnlockMutex(map->mutex);
    nv_hashmap_insert(map, key, value, hash_fn_arg);
    SDL_LockMutex(map->mutex);
  }

  nv_free(key);
  nv_free(value);

  SDL_UnlockMutex(map->mutex);
}

// ==============================
// RECTPACK
// ==============================

nv_error
nv_skyline_bin_init(size_t width, size_t height, nv_skyline_bin_t* dst)
{
  nv_assert_else_return(dst != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(width != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(height != 0, NV_ERROR_INVALID_ARG);

  *dst = nv_zero_init(nv_skyline_bin_t);

  dst->canary = NOVA_CONT_CANARY;
  dst->width  = width;
  dst->height = height;

  dst->rects                = NULL;
  dst->num_rects            = 0;
  dst->allocated_rect_count = 0;

  dst->skyline = (size_t*)nv_calloc(width * sizeof(size_t));
  nv_assert_else_return(dst->skyline != NULL, NV_ERROR_MALLOC_FAILED);

  dst->mutex = SDL_CreateMutex();
  nv_assert_else_return(dst->mutex != NULL, NV_ERROR_EXTERNAL);

  nv_assert_else_return(NOVA_CONT_IS_VALID(dst), NV_ERROR_BROKEN_STATE);

  return NV_ERROR_SUCCESS;
}

void
nv_skyline_bin_destroy(nv_skyline_bin_t* bin)
{
  if (!bin)
  {
    return;
  }
  nv_assert(NOVA_CONT_IS_VALID(bin));
  SDL_LockMutex(bin->mutex);
  if (bin->rects)
  {
    nv_free(bin->rects);
  }
  if (bin->skyline)
  {
    nv_free(bin->skyline);
  }
  SDL_UnlockMutex(bin->mutex);
  SDL_DestroyMutex(bin->mutex);
}

size_t
nv_skyline_bin_max_height(const nv_skyline_bin_t* bin, size_t x, size_t w)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  SDL_LockMutex(bin->mutex);

  size_t max_h = 0;
  for (size_t i = x; i < x + w && i < bin->width; i++)
  {
    if (bin->skyline[i] > max_h)
    {
      max_h = bin->skyline[i];
    }
  }

  SDL_UnlockMutex(bin->mutex);
  return max_h;
}

int
nv_skyline_bin_find_best_placement(const nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t* best_x, size_t* best_y)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  SDL_LockMutex(bin->mutex);

  /**
   * SIZE_MAX is used as a no find value.
   * Obviously, no rect in a bin should have a position of SIZE_MAX
   * That implies that its width is SIZE_MAX + sizeofrect which is impossible
   */
  size_t min_y = SIZE_MAX;
  *best_x      = SIZE_MAX;
  *best_y      = SIZE_MAX;

  if (rect->width > bin->width)
  {
    return -1;
  }

  size_t max_x = bin->width - rect->width;
  for (size_t x = 0; x <= max_x; x++)
  {
    SDL_UnlockMutex(bin->mutex);
    size_t y = nv_skyline_bin_max_height(bin, x, rect->width);
    if (y + rect->height <= bin->height && y < min_y)
    {
      min_y   = y;
      *best_x = x;
      *best_y = y;
    }
    SDL_LockMutex(bin->mutex);
  }

  SDL_UnlockMutex(bin->mutex);
  return (*best_x != SIZE_MAX);
}

void
nv_skyline_bin_place_rect(nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t x, size_t y)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  SDL_LockMutex(bin->mutex);

  if (bin->num_rects >= bin->allocated_rect_count)
  {
    size_t new_alloc = (bin->allocated_rect_count == 0) ? 2 : bin->allocated_rect_count * 2;

    if (bin->rects)
    {
      bin->rects = (nv_skyline_rect_t*)nv_realloc(bin->rects, new_alloc * sizeof(nv_skyline_rect_t));
    }
    else
    {
      bin->rects = nv_calloc(new_alloc * sizeof(nv_skyline_rect_t));
    }
    bin->allocated_rect_count = new_alloc;
  }

  bin->rects[bin->num_rects++] = (nv_skyline_rect_t){ rect->width, rect->height, x, y };

  for (size_t i = x; i < x + rect->width && i < bin->width; i++)
  {
    bin->skyline[i] = y + rect->height;
  }

  SDL_UnlockMutex(bin->mutex);
}

static int
nv_skyline_compare_rect(const void* rect1, const void* rect2)
{
  size_t rect2_height = ((const nv_skyline_rect_t*)rect2)->height;
  size_t rect1_height = ((const nv_skyline_rect_t*)rect1)->height;
  return (int)rect2_height - (int)rect1_height;
}

void
nv_skyline_bin_pack_rects(nv_skyline_bin_t* bin, nv_skyline_rect_t* rects, size_t nrects)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  qsort(rects, nrects, sizeof(nv_skyline_rect_t), nv_skyline_compare_rect);

  for (size_t i = 0; i < nrects; i++)
  {
    size_t x = 0;
    size_t y = 0;
    if (nv_skyline_bin_find_best_placement(bin, &rects[i], &x, &y))
    {
      nv_skyline_bin_place_rect(bin, &rects[i], x, y);
      rects[i].posx = x;
      rects[i].posy = y;
    }
    else
    {
      nv_log_error("Failed to pack rect %d\n", (int)i);
    }
  }
}

// Please do not look at this.
// Please
// This is stupid and I can't (just don't) want to find a work around
void
nv_skyline_bin_resize(nv_skyline_bin_t* bin, size_t new_w, size_t new_h)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  nv_skyline_rect_t* valid_rects   = NULL;
  size_t             num_valid     = 0;
  nv_skyline_rect_t* invalid_rects = NULL;
  size_t             num_invalid   = 0;

  SDL_LockMutex(bin->mutex);

  for (size_t i = 0; i < bin->num_rects; i++)
  {
    nv_skyline_rect_t rect = bin->rects[i];
    if (rect.posx + rect.width > new_w || rect.posy + rect.height > new_h)
    {
      nv_skyline_rect_t* tmp = NULL;
      if (!invalid_rects)
      {
        tmp = (nv_skyline_rect_t*)nv_calloc(sizeof(nv_skyline_rect_t));
      }
      else
      {
        tmp = (nv_skyline_rect_t*)nv_realloc(invalid_rects, (num_invalid + 1) * sizeof(nv_skyline_rect_t));
      }
      if (!tmp)
      {
        nv_free(valid_rects);
        nv_free(invalid_rects);
        SDL_UnlockMutex(bin->mutex);
        return;
      }
      invalid_rects                = tmp;
      invalid_rects[num_invalid++] = rect;
    }
    else
    {
      nv_skyline_rect_t* tmp = (nv_skyline_rect_t*)nv_realloc(valid_rects, (num_valid + 1) * sizeof(nv_skyline_rect_t));
      if (!tmp)
      {
        nv_free(valid_rects);
        nv_free(invalid_rects);
        SDL_UnlockMutex(bin->mutex);
        return;
      }
      valid_rects              = tmp;
      valid_rects[num_valid++] = rect;
    }
  }

  if (new_w != bin->width)
  {
    size_t* new_skyline = (size_t*)nv_realloc(bin->skyline, new_w * sizeof(size_t));
    if (!new_skyline)
    {
      nv_log_error("Memory allocation failed for bin->skyline in nv_skyline_bin_resize\n");
      nv_free(valid_rects);
      nv_free(invalid_rects);
      SDL_UnlockMutex(bin->mutex);
      return;
    }
    // if it's bigger horizontally, clear the new entries
    if (new_w > bin->width)
    {
      for (size_t i = bin->width; i < new_w; i++)
      {
        new_skyline[i] = 0;
      }
    }
    bin->skyline = new_skyline;
  }

  for (size_t i = 0; i < new_w; i++)
  {
    if (bin->skyline[i] > new_h)
    {
      bin->skyline[i] = new_h;
    }
  }

  for (size_t i = 0; i < num_valid; i++)
  {
    nv_skyline_rect_t rect = valid_rects[i];
    for (size_t x = rect.posx; x < rect.posx + rect.width && x < new_w; x++)
    {
      if (bin->skyline[x] < rect.posy + rect.height)
      {
        bin->skyline[x] = rect.posy + rect.height;
      }
    }
  }

  if (bin->rects)
  {
    nv_free(bin->rects);
  }
  bin->rects                = valid_rects;
  bin->num_rects            = num_valid;
  bin->allocated_rect_count = num_valid;

  for (size_t i = 0; i < num_invalid; i++)
  {
    size_t x = 0;
    size_t y = 0;
    if (nv_skyline_bin_find_best_placement(bin, &invalid_rects[i], &x, &y))
    {
      nv_skyline_bin_place_rect(bin, &invalid_rects[i], x, y);
    }
    else
    {
      nv_log_error("failed to repack rect %lu after resize\n", i);
    }
  }

  if (invalid_rects)
  {
    nv_free(invalid_rects);
  }

  bin->width  = new_w;
  bin->height = new_h;
  SDL_UnlockMutex(bin->mutex);
}

// ==============================
// BITSET
// ==============================

nv_error
nv_bitset_init(int init_capacity, nv_allocator_fn allocator, nv_bitset_t* set)
{
  nv_assert_else_return(set != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(allocator != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(init_capacity >= 0, NV_ERROR_INVALID_ARG);

  *set = nv_zero_init(nv_bitset_t);

  set->mutex = SDL_CreateMutex();
  nv_assert_else_return(set->mutex != NULL, NV_ERROR_EXTERNAL);

  if (init_capacity > 0)
  {
    init_capacity = (init_capacity + 7) / 8;
    set->size     = init_capacity;
    set->alloc    = allocator;

    set->data = NV_ALLOC(set->alloc, set->alloc_arg, init_capacity * sizeof(uint8_t));
    nv_assert_else_return(set->data != NULL, NV_ERROR_MALLOC_FAILED);
  }
  else
  {
    set->size = 0;
  }

  return NV_ERROR_SUCCESS;
}

void
nv_bitset_set_bit(nv_bitset_t* set, int bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] |= (1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

void
nv_bitset_set_bit_to(nv_bitset_t* set, int bitindex, nv_bitset_bit to)
{
  to ? nv_bitset_set_bit(set, bitindex) : nv_bitset_clear_bit(set, bitindex);
}

void
nv_bitset_clear_bit(nv_bitset_t* set, int bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] &= ~(1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

void
nv_bitset_toggle_bit(nv_bitset_t* set, int bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] ^= (1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

nv_bitset_bit
nv_bitset_access_bit(nv_bitset_t* set, int bitindex)
{
  SDL_LockMutex(set->mutex);
  nv_bitset_bit bit = (set->data[bitindex / 8] & (1U << (bitindex % 8U))) != 0;
  SDL_UnlockMutex(set->mutex);
  return bit;
}

void
nv_bitset_copy_from(nv_bitset_t* dst, const nv_bitset_t* src)
{
  if (!src->data)
  {
    return;
  }
  SDL_LockMutex(dst->mutex);
  SDL_LockMutex(src->mutex);
  if (src->size != dst->size && dst->data)
  {
    NV_FREE(dst->alloc, dst->alloc_arg, dst->data, dst->size * sizeof(uint8_t));
    dst->data = NV_ALLOC(dst->alloc, dst->alloc_arg, src->size * sizeof(uint8_t));
    dst->size = src->size;
  }
  if (dst->data && src->data)
  {
    nv_memcpy(dst->data, src->data, src->size);
  }
  SDL_UnlockMutex(dst->mutex);
  SDL_UnlockMutex(src->mutex);
}

void
nv_bitset_destroy(nv_bitset_t* set)
{
  if (!set)
  {
    return;
  }

  SDL_LockMutex(set->mutex);
  NV_FREE(set->alloc, set->alloc_arg, set->data, set->size * sizeof(uint8_t));
  SDL_UnlockMutex(set->mutex);

  SDL_DestroyMutex(set->mutex);

  nv_bzero(set, sizeof(nv_bitset_t));
}

// RBMAP

nv_error
nv_rbmap_init(size_t key_size, size_t val_size, nv_compare_fn compare_fn, nv_allocator_fn alloc, void* alloc_arg, nv_rbmap_t* dst)
{
  nv_assert_else_return(key_size != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(val_size != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(compare_fn != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(alloc != NULL, NV_ERROR_INVALID_ARG);

  *dst = nv_zero_init(nv_rbmap_t);

  dst->canary     = NOVA_CONT_CANARY;
  dst->key_size   = key_size;
  dst->val_size   = val_size;
  dst->compare_fn = compare_fn;
  dst->root       = NULL;
  dst->alloc      = alloc;
  dst->alloc_arg  = alloc_arg;

  return NV_ERROR_SUCCESS;
}

void
nv_rbmap_left_rotate(nv_rbmap_t* map, nv_rbmap_node_t* x)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_rbmap_node_t* y = x->children[1];
  x->children[1]     = y->children[0];
  if (y->children[0])
  {
    y->children[0]->parent = x;
  }

  y->parent = x->parent;
  if (!x->parent)
  {
    map->root = y;
  }
  else
  {
    x->parent->children[x == x->parent->children[1]] = y;
  }

  y->children[0] = x;
  x->parent      = y;
}

void
nv_rbmap_right_rotate(nv_rbmap_t* map, nv_rbmap_node_t* y)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_rbmap_node_t* x = y->children[0];
  y->children[0]     = x->children[1];
  if (x->children[1])
  {
    x->children[1]->parent = y;
  }

  x->parent = y->parent;
  if (!y->parent)
  {
    map->root = x;
  }
  else
  {
    y->parent->children[y == y->parent->children[1]] = x;
  }

  x->children[1] = y;
  y->parent      = x;
}

nv_rbmap_node_t*
nv_rbmap_minimum(nv_rbmap_node_t* node)
{
  while (node && node->children[0] != NULL)
  {
    node = node->children[0];
  }
  return node;
}

void
nv_rbmap_transplant(nv_rbmap_t* map, nv_rbmap_node_t* u, nv_rbmap_node_t* v)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  if (u->parent == NULL)
  {
    map->root = v;
  }
  else if (u == u->parent->children[0])
  {
    u->parent->children[0] = v;
  }
  else
  {
    u->parent->children[1] = v;
  }

  if (v != NULL)
  {
    v->parent = u->parent;
  }
}

void
nv_rbmap_delete_fixup(nv_rbmap_t* map, nv_rbmap_node_t* x)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  while (x != map->root && (x == NULL || x->color == NOVA_RBNODE_COLOR_BLK))
  {
    if (x == x->parent->children[0])
    {
      nv_rbmap_node_t* w = x->parent->children[1];
      if (!w)
      {
        continue;
      }
      if (w && w->color == NOVA_RBNODE_COLOR_RED)
      {
        w->color         = NOVA_RBNODE_COLOR_BLK;
        x->parent->color = NOVA_RBNODE_COLOR_RED;
        nv_rbmap_left_rotate(map, x->parent);
        w = x->parent->children[1];
      }
      if (w && (w->children[0] == NULL || (w->children[0] && w->children[0]->color == NOVA_RBNODE_COLOR_BLK))
          && (w->children[1] == NULL || (w->children[1] && w->children[1]->color == NOVA_RBNODE_COLOR_BLK)))
      {
        w->color = NOVA_RBNODE_COLOR_RED;
        x        = x->parent;
      }
      else if (w)
      {
        if (w->children[1] == NULL || w->children[1]->color == NOVA_RBNODE_COLOR_BLK)
        {
          if (w->children[0])
          {
            w->children[0]->color = NOVA_RBNODE_COLOR_BLK;
          }
          w->color = NOVA_RBNODE_COLOR_RED;
          nv_rbmap_right_rotate(map, w);
          w = x->parent->children[1];
        }
        w->color         = x->parent->color;
        x->parent->color = NOVA_RBNODE_COLOR_BLK;
        if (w->children[1])
        {
          w->children[1]->color = NOVA_RBNODE_COLOR_BLK;
        }
        nv_rbmap_left_rotate(map, x->parent);
        x = map->root;
      }
    }
    else
    {
      nv_rbmap_node_t* w = x->parent->children[0];
      if (w && w->color == NOVA_RBNODE_COLOR_RED)
      {
        w->color         = NOVA_RBNODE_COLOR_BLK;
        x->parent->color = NOVA_RBNODE_COLOR_RED;
        nv_rbmap_right_rotate(map, x->parent);
        w = x->parent->children[0];
      }
      if ((w && (w->children[0] == NULL || w->children[0]->color == NOVA_RBNODE_COLOR_BLK) && (w->children[1] == NULL || w->children[1]->color == NOVA_RBNODE_COLOR_BLK)))
      {
        w->color = NOVA_RBNODE_COLOR_RED;
        x        = x->parent;
      }
      else if (w)
      {
        if (w->children[0] == NULL || w->children[0]->color == NOVA_RBNODE_COLOR_BLK)
        {
          if (w->children[1])
          {
            w->children[1]->color = NOVA_RBNODE_COLOR_BLK;
          }
          w->color = NOVA_RBNODE_COLOR_RED;
          nv_rbmap_left_rotate(map, w);
          w = x->parent->children[0];
        }
        w->color         = x->parent->color;
        x->parent->color = NOVA_RBNODE_COLOR_BLK;
        if (w->children[0])
        {
          w->children[0]->color = NOVA_RBNODE_COLOR_BLK;
        }
        nv_rbmap_right_rotate(map, x->parent);
        x = map->root;
      }
    }
  }
  if (x)
  {
    x->color = NOVA_RBNODE_COLOR_BLK;
  }
}

void
nv_rbmap_delete(nv_rbmap_t* map, nv_rbmap_node_t* z)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_assert(z != NULL);

  nv_rbmap_node_t* y                = z;
  int              y_original_color = y->color;
  nv_rbmap_node_t* x                = NULL;

  if (z->children[0] == NULL)
  {
    x = z->children[1];
    nv_rbmap_transplant(map, z, z->children[1]);
  }
  else if (z->children[1] == NULL)
  {
    x = z->children[0];
    nv_rbmap_transplant(map, z, z->children[0]);
  }
  else
  {
    y                = nv_rbmap_minimum(z->children[1]);
    y_original_color = y->color;
    x                = y->children[1];
    if (y->parent == z)
    {
      if (x)
      {
        x->parent = y;
      }
    }
    else
    {
      nv_rbmap_transplant(map, y, y->children[1]);
      y->children[1] = z->children[1];
      if (y->children[1])
      {
        y->children[1]->parent = y;
      }
    }
    nv_rbmap_transplant(map, z, y);
    y->children[0] = z->children[0];
    if (y->children[0])
    {
      y->children[0]->parent = y;
    }
    y->color = z->color;
  }

  if (y_original_color == NOVA_RBNODE_COLOR_BLK && x != NULL)
  {
    nv_rbmap_delete_fixup(map, x);
  }

  NV_FREE(map->alloc, map->alloc_arg, z, sizeof(nv_rbmap_node_t));
}

void
nv_rbmap_insert_fixup(nv_rbmap_t* map, nv_rbmap_node_t* z)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_assert(z != NULL);

  while (z->parent && z->parent->color == NOVA_RBNODE_COLOR_RED)
  {
    if (z->parent == z->parent->parent->children[0])
    {
      nv_rbmap_node_t* uncle = z->parent->parent->children[1];
      if (uncle && uncle->color == NOVA_RBNODE_COLOR_RED)
      {
        z->parent->color         = NOVA_RBNODE_COLOR_BLK;
        uncle->color             = NOVA_RBNODE_COLOR_BLK;
        z->parent->parent->color = NOVA_RBNODE_COLOR_RED;
        z                        = z->parent->parent;
      }
      else
      {
        if (z == z->parent->children[1])
        {
          z = z->parent;
          nv_rbmap_left_rotate(map, z);
        }
        z->parent->color         = NOVA_RBNODE_COLOR_BLK;
        z->parent->parent->color = NOVA_RBNODE_COLOR_RED;
        nv_rbmap_right_rotate(map, z->parent->parent);
      }
    }
    else
    {
      nv_rbmap_node_t* uncle = z->parent->parent->children[0];
      if (uncle && uncle->color == NOVA_RBNODE_COLOR_RED)
      {
        z->parent->color         = NOVA_RBNODE_COLOR_BLK;
        uncle->color             = NOVA_RBNODE_COLOR_BLK;
        z->parent->parent->color = NOVA_RBNODE_COLOR_RED;
        z                        = z->parent->parent;
      }
      else
      {
        if (z == z->parent->children[0])
        {
          z = z->parent;
          nv_rbmap_right_rotate(map, z);
        }
        z->parent->color         = NOVA_RBNODE_COLOR_BLK;
        z->parent->parent->color = NOVA_RBNODE_COLOR_RED;
        nv_rbmap_left_rotate(map, z->parent->parent);
      }
    }
  }
  map->root->color = NOVA_RBNODE_COLOR_BLK;
}

static inline nv_rbmap_node_t*
nv_rbmap_allocate_node(nv_rbmap_t* map)
{
  return (nv_rbmap_node_t*)map;
}

void
nv_rbmap_insert(nv_rbmap_t* map, const void* key, const void* value, void* user_hash_data)
{
  (void)nv_rbmap_allocate_node;
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_assert(map != NULL);
  nv_assert(key != NULL);
  nv_assert(value != NULL);

  void* allocation = NV_ALLOC(map->alloc, map->alloc_arg, sizeof(nv_rbmap_node_t));

  nv_rbmap_node_t* z = (nv_rbmap_node_t*)allocation;

  z->key = (char*)allocation + sizeof(nv_rbmap_node_t);
  z->val = (char*)allocation + sizeof(nv_rbmap_node_t) + map->key_size;
  nv_assert(z->val != NULL);

  nv_memcpy(z->key, key, map->key_size);
  nv_memcpy(z->val, value, map->val_size);

  z->color       = NOVA_RBNODE_COLOR_RED;
  z->parent      = NULL;
  z->children[0] = z->children[1] = NULL;

  nv_rbmap_node_t* y = NULL;
  nv_rbmap_node_t* x = map->root;
  while (x != NULL)
  {
    y       = x;
    int cmp = map->compare_fn(key, x->key, map->key_size, user_hash_data);
    if (cmp < 0)
    {
      x = x->children[0];
    }
    else
    {
      x = x->children[1];
    }
  }
  z->parent = y;
  if (y == NULL)
  {
    map->root = z;
  }
  else if (map->compare_fn(key, y->key, map->key_size, user_hash_data) < 0)
  {
    y->children[0] = z;
  }
  else
  {
    y->children[1] = z;
  }

  nv_rbmap_insert_fixup(map, z);
}

void
nv_rbmap_iterator_init(nv_rbmap_t* map, nv_rbmap_iterator_t* dst)
{
  nv_assert(dst != NULL);
  nv_assert(map != NULL);

  *dst          = nv_zero_init(nv_rbmap_iterator_t);
  dst->current  = map->root;
  dst->stack    = NULL;
  dst->capacity = 0;
  dst->top      = 0;
}

void
nv_rbmap_iterator_reserve(nv_rbmap_iterator_t* itr, size_t num_elems)
{
  nv_assert(itr != NULL);
  if (itr->stack)
  {
    itr->stack = (nv_rbmap_node_t**)nv_realloc(itr->stack, num_elems * sizeof(nv_rbmap_node_t*));
  }
  else
  {
    itr->stack = (nv_rbmap_node_t**)nv_calloc(num_elems * sizeof(nv_rbmap_node_t*));
  }
  itr->capacity = num_elems;
}

nv_rbmap_node_t*
nv_rbmap_iterator_next(nv_rbmap_iterator_t* itr)
{
  while (itr->current || itr->top > 0)
  {
    if (itr->current)
    {
      if (!itr->stack || itr->top >= itr->capacity)
      {
        itr->capacity = itr->capacity ? itr->capacity * 2 : 4;
        itr->stack    = (nv_rbmap_node_t**)nv_realloc(itr->stack, itr->capacity * sizeof(nv_rbmap_node_t*));
      }
      itr->stack[itr->top++] = itr->current;
      itr->current           = itr->current->children[0];
    }
    else
    {
      itr->current          = itr->stack[--itr->top];
      nv_rbmap_node_t* node = itr->current;
      itr->current          = itr->current->children[1];
      return node;
    }
  }
  return NULL;
}

void
nv_rbmap_iterator_destroy(nv_rbmap_iterator_t* itr)
{
  if (!itr)
  {
    return;
  }
  if (itr->stack)
  {
    nv_free(itr->stack);
  }
  itr->stack    = NULL;
  itr->capacity = 0;
}

void*
nv_rbmap_find(const nv_rbmap_t* map, const void* key, void* user_hash_data)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  nv_rbmap_node_t* node = map->root;
  while (node != NULL)
  {
    int cmp = map->compare_fn(key, node->key, map->key_size, user_hash_data);
    if (cmp < 0)
    {
      node = node->children[0];
    }
    else if (cmp > 0)
    {
      node = node->children[1];
    }
    else
    {
      return node->val;
    }
  }
  return NULL;
}

void
nv_rbmap_destroy(nv_rbmap_t* map)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  nv_rbmap_iterator_t itr;
  nv_rbmap_iterator_init(map, &itr);

  nv_rbmap_node_t** nodes    = NULL;
  size_t            count    = 0;
  size_t            capacity = 16;
  nodes                      = (nv_rbmap_node_t**)nv_calloc(capacity * sizeof(nv_rbmap_node_t*));

  nv_rbmap_node_t* node = NULL;
  while ((node = nv_rbmap_iterator_next(&itr)) != NULL)
  {
    if (count >= capacity)
    {
      capacity *= 2;
      nodes = (nv_rbmap_node_t**)nv_realloc(nodes, capacity * sizeof(nv_rbmap_node_t*));
    }
    nodes[count] = node;
    count++;
  }

  for (size_t i = 0; i < count; i++)
  {
    node = nodes[i];
    NV_FREE(map->alloc, map->alloc_arg, node, sizeof(nv_rbmap_node_t));
  }
  nv_free(nodes);
  nv_rbmap_iterator_destroy(&itr);
  map->root = NULL;
}
