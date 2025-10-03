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

#ifndef STD_CONTAINERS_HASHMAP_H
#define STD_CONTAINERS_HASHMAP_H

#include "../alloc.h"
#include "../attributes.h"
#include "../errorcodes.h"
#include "../hash.h"
#include "../stdafx.h"
#include "../types.h"
#include <SDL3/SDL_mutex.h>
#include <stddef.h>
#include <stdio.h>

NOVA_HEADER_START

#ifndef NV_HASHMAP_LOAD_FACTOR
/* If the size of the hashmap grows to more than this, it will resize */
#  define NV_HASHMAP_LOAD_FACTOR (3.0 / 4.0)
#endif

typedef struct nv_hashmap      nv_hashmap_t;
typedef struct nv_hashmap_node nv_hashmap_node_t;

/**
  @note hash_fn may be NULL for the standard FNV-1A function.
  @note equal_fn may also be NULL for standard memcmp == 0
*/
extern nv_error nv_hashmap_init(size_t init_size, size_t keysize, size_t valuesize, nv_hash_fn hash_fn, nv_allocator_fn allocator, void* alloc_arg, nv_hashmap_t* dst);

extern void nv_hashmap_destroy(nv_hashmap_t* map);

extern void nv_hashmap_resize(nv_hashmap_t* map, size_t new_capacity, void* hash_fn_arg);

extern void nv_hashmap_clear(nv_hashmap_t* map);

extern size_t nv_hashmap_size(const nv_hashmap_t* map);

extern size_t nv_hashmap_capacity(const nv_hashmap_t* map);

extern size_t nv_hashmap_keysize(const nv_hashmap_t* map);

extern size_t nv_hashmap_valuesize(const nv_hashmap_t* map);

/**
 *  __i needs to point to an integer initialized to 0
 * Note that this function is MT-Unsafe and the caller must use SDL_LockMutex to ensure MT safeness.
 */
extern nv_hashmap_node_t* nv_hashmap_iterate_unsafe(const nv_hashmap_t* NV_RESTRICT map, size_t* NV_RESTRICT _i);

extern nv_hashmap_node_t* nv_hashmap_root_node(const nv_hashmap_t* map);

/**
  WARNING: Doesn't replace the value if a key already exists!! Use nv_hashmap_insert_or_replace()
    also, if key or value is a string (const char *, not a nv_string_t or something),
    just pass in the const char *, not a pointer to it!!!
  @param hash_fn_arg The argument provided to the hash function as "user_data"
    The hash function argument will be passed on to resize() too if it needs to be
*/
extern void nv_hashmap_insert(nv_hashmap_t* map, const void* NV_RESTRICT key, const void* NV_RESTRICT value, void* hash_fn_arg);

/**
 * @param hash_fn_arg The argument provided to the hash function as "user_data"
 */
extern void nv_hashmap_insert_or_replace(nv_hashmap_t* map, const void* NV_RESTRICT key, void* NV_RESTRICT value, void* hash_fn_arg);

/**
 * @return NULL on no find
 * @param hash_fn_arg The argument provided to the hash function as "user_data"
 */
extern void* nv_hashmap_find(const nv_hashmap_t* NV_RESTRICT map, const void* NV_RESTRICT key, void* hash_fn_arg);

/**
 * @brief Write to the file containing each key-value pair
 * @note Does not close or open the file
 */
extern void nv_hashmap_serialize(nv_hashmap_t* NV_RESTRICT map, FILE* NV_RESTRICT f);

/**
 * map must have been initialized
 * @note Does not close or open the file
 */
extern void nv_hashmap_deserialize(nv_hashmap_t* NV_RESTRICT map, FILE* NV_RESTRICT f, void* hash_fn_arg);

struct nv_hashmap_node
{
  void* key;
  void* value;
  u32   hash;
} NOVA_ATTR_ALIGNED(32);

struct nv_hashmap
{
  unsigned           canary;
  SDL_Mutex*         mutex;
  nv_hashmap_node_t* nodes;
  nv_hash_fn         hash_fn;
  size_t             capacity;
  size_t             size;
  size_t             key_size;
  size_t             value_size;
  nv_allocator_fn    alloc;
  void*              alloc_arg;
} NOVA_ATTR_ALIGNED(128);

NOVA_HEADER_END

#endif // STD_CONTAINERS_HASHMAP_H
