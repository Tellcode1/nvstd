#include "../../include/containers/hashmap.h"

#include "../../include/alloc.h"
#include "../../include/error.h"
#include "../../include/stdafx.h"
#include "../../include/string.h"
#include "../../include/types.h"

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
  if (num == 0) { return 1; }
  if ((num & (num - 1)) == 0) // already power of two?
  {
    return num;
  }
  /* magic :> */
  num--;
  num |= num >> 1U;
  num |= num >> 2U;
  num |= num >> 4U;
  num |= num >> 8U;
  num |= num >> 16U;
  num++;
  return num;
}

#define NODE_OCCUPIED(node) ((node).key != NULL)

nv_error
nv_hashmap_init(size_t key_size, size_t value_size, nv_hash_fn hash_fn, nv_compare_fn comp_fn, size_t init_capacity, nv_hashmap_t* dst)

{
  nv_assert_else_return(dst != NULL, NV_ERROR_INVALID_ARG);

  *dst = nv_zinit(nv_hashmap_t);

  init_capacity = next_power_of_two(init_capacity);
  dst->nodes    = init_capacity == 0 ? NULL : (nv_hashmap_node_t*)nv_zmalloc(init_capacity * sizeof(nv_hashmap_node_t));
  nv_assert_else_return(dst->nodes != NULL, NV_ERROR_MALLOC_FAILED);

  if (key_size != 0) { dst->hash_fn = hash_fn ? hash_fn : nv_hash_fnv1a; }
  else
  {
    dst->hash_fn = hash_fn ? hash_fn : nv_hash_fnv1a_string;
  }

  dst->comp_fn = comp_fn ? comp_fn : nv_compare_default;

  dst->key_size   = key_size;
  dst->value_size = value_size;
  dst->capacity   = next_power_of_two(init_capacity);
  dst->size       = 0;
  dst->canary     = NOVA_CONT_CANARY;

  return NV_ERROR_SUCCESS;
}

void
nv_hashmap_destroy(nv_hashmap_t* map)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  if (map->nodes)
  {
    for (size_t idx = 0; idx < map->capacity; idx++)
    {
      nv_hashmap_node_t* node = &map->nodes[idx];

      if (NODE_OCCUPIED(*node))
      {
        nv_free(node->key);
        nv_free(node->value);
      }
    }
    nv_free(map->nodes);
    map->nodes = NULL;
  }
}

static inline void
nv_hashmap_resize_unsafe(nv_hashmap_t* map, size_t new_capacity)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  nv_hashmap_node_t* old_nodes   = map->nodes;
  const size_t       old_entries = map->capacity;

  if (new_capacity <= 0) { new_capacity = 1; }

  map->capacity = next_power_of_two(new_capacity);
  map->size     = 0;

  // we can't do realloc here because we need to rehash all the nodes
  map->nodes = (nv_hashmap_node_t*)nv_zmalloc(map->capacity * sizeof(nv_hashmap_node_t));
  nv_assert(map->nodes != NULL);

  if (old_nodes)
  {
    for (size_t i = 0; i < old_entries; i++)
    {
      nv_hashmap_node_t* old_node = &old_nodes[i];
      if (NODE_OCCUPIED(*old_node))
      {
        nv_hashmap_insert(map, old_node->key, old_node->value);

        // node copied, free old key and value
        nv_free(old_node->key);
        nv_free(old_node->value);
        // Safety!
        old_node->key   = NULL;
        old_node->value = NULL;
      }
    }
    nv_free(old_nodes);
  }
}

void
nv_hashmap_resize(nv_hashmap_t* map, size_t new_capacity)
{
  nv_hashmap_resize_unsafe(map, new_capacity);
}

void
nv_hashmap_clear(nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), );

  for (size_t i = 0; i < map->capacity; i++)
  {
    nv_hashmap_node_t* node = &map->nodes[i];
    if (NODE_OCCUPIED(*node))
    {
      if (map->key_size == 0) // key is string
      {
        nv_free(node->key);
      }
      if (map->value_size == 0) // value is string
      {
        nv_free(node->value);
      }
    }
  }

  map->nodes    = NULL;
  map->size     = 0;
  map->capacity = 0;
}

size_t
nv_hashmap_size(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), SIZE_MAX);
  return map->size;
}

size_t
nv_hashmap_capacity(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), SIZE_MAX);
  return map->capacity;
}

size_t
nv_hashmap_key_size(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), SIZE_MAX);
  return map->key_size;
}

size_t
nv_hashmap_value_size(const nv_hashmap_t* map)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), SIZE_MAX);
  return map->value_size;
}

nv_hashmap_node_t*
nv_hashmap_iterate(const nv_hashmap_t* map, size_t* _i)
{
  nv_assert_else_return(NOVA_CONT_IS_VALID(map), NULL);

  /**
   * If map->capacity is 0, it simply jumps to returning NULL
   */
  for (; (*_i) < map->capacity;)
  {
    size_t i = (*_i)++;
    if (NODE_OCCUPIED(map->nodes[i])) { return &map->nodes[i]; }
  }
  return NULL;
}

static inline nv_hashmap_node_t*
find_node(const nv_hashmap_t* NV_RESTRICT map, const void* NV_RESTRICT key)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  if (!map->nodes) { return NULL; }

  /**
   * Handle strings. If key_size = 0, key is string so we compute its length.
   */
  size_t actual_key_size = map->key_size;
  if (map->key_size == 0) { actual_key_size = nv_strlen((const char*)key) + 1; }

  const u32 hash  = map->hash_fn(key, actual_key_size, map->user_data);
  const u32 begin = hash & (map->capacity - 1);

  u32 index = begin;
  u32 probe = 0;

  while (NODE_OCCUPIED(map->nodes[index]))
  {
    const void* node_key = map->nodes[index].key;
    // if (map->equal_fn(map->nodes[index].key, key, map->key_size))
    // key is string

    if (map->nodes[index].hash == hash && map->comp_fn(node_key, key, actual_key_size, map->user_data) == 0) { return &map->nodes[index]; }

    if (++probe >= map->capacity) break;
    index = (hash + probe + probe * probe) & (map->capacity - 1);
  }

  return NULL;
}

void*
nv_hashmap_find(const nv_hashmap_t* NV_RESTRICT map, const void* NV_RESTRICT key)
{
  void* found = NULL;

  nv_hashmap_node_t* node = find_node(map, key);
  if (node) found = node->value;

  return found;
}

static inline void*
nv_hashmap_insert_internal_unsafe(nv_hashmap_t* map, const void* NV_RESTRICT key, const void* NV_RESTRICT value, bool replace_if_exists)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  // the second check
  if (!map->nodes || (double)map->size >= ((double)map->capacity * NV_HASHMAP_LOAD_FACTOR))
  {
    // The check to whether map->entries is greater than 0 is already done in
    // resize();
    nv_hashmap_resize_unsafe(map, map->capacity * 2);
  }

  size_t actual_key_size   = map->key_size;
  size_t actual_value_size = map->value_size;
  if (map->key_size == 0) { actual_key_size = nv_strlen((const char*)key) + 1; }
  if (map->value_size == 0) { actual_value_size = nv_strlen((const char*)value) + 1; }

  u32 hash  = map->hash_fn(key, actual_key_size, map->user_data);
  u32 index = hash & (map->capacity - 1);
  u32 probe = 0;

  nv_hashmap_node_t* node = &map->nodes[index];
  while (NODE_OCCUPIED(*node))
  {
    if (node->hash == hash && map->comp_fn(node->key, key, actual_key_size, map->user_data) == 0)
    {
      if (replace_if_exists)
      {
        if (map->value_size == 0) // is the value a string?
        {
          if (node->value) { nv_free(node->value); }
          node->value = nv_strdup((const char*)value);
        }
        else
        {
          nv_memcpy(node->value, value, map->value_size);
        }
      }
      return node->value;
    }

    probe++;
    index = (hash + probe + probe * probe) & (map->capacity - 1);

    node = &map->nodes[index];
  }

  *node = nv_zinit(nv_hashmap_node_t);

  node->key   = nv_zmalloc(actual_key_size);
  node->value = nv_zmalloc(actual_value_size);
  node->hash  = hash;

  if (map->key_size != NV_HASHMAP_SIZE_STRING) { nv_memcpy(node->key, key, map->key_size); }
  else
  {
    if (node->key) { nv_free(node->key); }
    node->key = nv_strdup((const char*)key);
  }

  if (map->value_size != NV_HASHMAP_SIZE_STRING) { nv_memcpy(node->value, value, map->value_size); }
  else
  {
    if (node->value) { nv_free(node->value); }
    node->value = nv_strdup((const char*)value);
  }

  map->size++;

  return node->value;
}

void*
nv_hashmap_insert(nv_hashmap_t* map, const void* NV_RESTRICT key, const void* NV_RESTRICT value)
{
  // TODO(bird): This should not be structured like this????
  void* inserted = nv_hashmap_insert_internal_unsafe(map, key, value, 0);

  return inserted;
}

void*
nv_hashmap_insert_or_replace(nv_hashmap_t* map, const void* NV_RESTRICT key, void* NV_RESTRICT value)
{
  void* inserted = nv_hashmap_insert_internal_unsafe(map, key, value, 1);
  return inserted;
}

void
nv_hashmap_serialize(const nv_hashmap_t* map, FILE* f)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  const size_t key_size = map->key_size;
  const size_t val_size = map->value_size;
  const size_t size     = map->size;

  // The NOVA_CONT_VALID ensures the canary is valid here.
  fwrite(&map->canary, sizeof(u32), 1, f);
  fwrite(&key_size, sizeof(key_size), 1, f);
  fwrite(&val_size, sizeof(val_size), 1, f);
  fwrite(&size, sizeof(size), 1, f);

  for (size_t i = 0; i < map->capacity; i++)
  {
    if (NODE_OCCUPIED(map->nodes[i]))
    {
      void* node_key   = map->nodes[i].key;
      void* node_value = map->nodes[i].value;

      if (key_size == NV_HASHMAP_SIZE_STRING)
      {
        size_t len = nv_strlen((const char*)node_key);
        fwrite(&len, sizeof(len), 1, f);
        fwrite(node_key, sizeof(char), len, f);
      }
      else
      {
        fwrite(node_key, key_size, 1, f);
      }

      if (val_size == NV_HASHMAP_SIZE_STRING)
      {
        size_t len = nv_strlen((const char*)node_value);
        fwrite(&len, sizeof(len), 1, f);
        fwrite(node_value, sizeof(char), len, f);
      }
      else
      {
        fwrite(node_value, val_size, 1, f);
      }
    }
  }
}

void
nv_hashmap_deserialize(nv_hashmap_t* map, FILE* f)
{
  // Container must have already been initialized.
  nv_assert(NOVA_CONT_IS_VALID(map));

  u32    file_canary   = 0;
  size_t file_key_size = 0;
  size_t file_val_size = 0;
  size_t num_nodes     = 0;

  fread(&file_canary, sizeof(u32), 1, f);
  fread(&file_key_size, sizeof(size_t), 1, f);
  fread(&file_val_size, sizeof(size_t), 1, f);
  fread(&num_nodes, sizeof(size_t), 1, f);

  nv_assert(file_canary == NOVA_CONT_CANARY);
  nv_assert(map->key_size == file_key_size);
  nv_assert(map->value_size == file_val_size);

  const bool key_is_string = map->key_size == NV_HASHMAP_SIZE_STRING;
  const bool val_is_string = map->value_size == NV_HASHMAP_SIZE_STRING;

  for (size_t i = 0; i < num_nodes; i++)
  {
    void* key = NULL;
    if (key_is_string)
    {
      size_t len = 0;
      fread(&len, sizeof(len), 1, f);

      char* s = nv_zmalloc(len + 1);
      fread(s, sizeof(char), len, f);
      s[len] = 0;

      key = (void*)s;
    }
    else
    {
      key = nv_zmalloc(map->key_size);
      fread(key, map->key_size, 1, f);
    }

    void* value = NULL;
    if (val_is_string)
    {
      size_t len = 0;
      fread(&len, sizeof(len), 1, f);

      char* s = nv_zmalloc(len + 1);
      fread(s, sizeof(char), len, f);
      s[len] = 0;

      value = (void*)s;
    }
    else
    {
      value = nv_zmalloc(map->value_size);
      fread(value, map->value_size, 1, f);
    }

    nv_hashmap_insert_internal_unsafe(map, key, value, true);

    nv_free(key);
    nv_free(value);
  }
}

bool
nv_hashmap_delete(nv_hashmap_t* map, const void* key)
{
  nv_assert(NOVA_CONT_IS_VALID(map));

  bool deleted = false;

  // Find the node, free its key and value and then bzero it.
  nv_hashmap_node_t* node = find_node(map, key);
  if (node)
  {
    if (map->key_size == 0 && node->key != NULL) nv_free(node->key);       // key is string, free key
    if (map->value_size == 0 && node->value != NULL) nv_free(node->value); // value is string free value
    *node = nv_zinit(nv_hashmap_node_t);

    deleted = true;
  }

  return deleted;
}
