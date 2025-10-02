#include "../../include/alloc.h"
#include "../../include/errorcodes.h"
#include "../../include/stdafx.h"
#include "../../include/string.h"
#include "../../include/types.h"

#include "../../include/containers/hashmap.h"

#include <SDL3/SDL_mutex.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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