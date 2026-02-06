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

/* TODO: Add documentation; Clean up code!!! */

/**
 * A red black map. Pretty similar to the hashmap, though hashmaps generally are more stable (i.e. scale well).
 * If your data is small (< 1KB) then you can use this, if not, go with hashmaps. They're more supported by me anyway as they're far more simpler.
 */

#ifndef NV_STD_CONTAINERS_RBMAP_H
#define NV_STD_CONTAINERS_RBMAP_H

#include "../alloc.h"
#include "../attributes.h"
#include "../errorcodes.h"
#include "../hash.h"
#include "../stdafx.h"

#include <stddef.h>

NOVA_HEADER_START

typedef struct nv_rbmap_node     nv_rbmap_node_t;
typedef struct nv_rbmap_iterator nv_rbmap_iterator_t;
typedef struct nv_rbmap          nv_rbmap_t;

/**
 * Red black trees are a threat to god and must not be implemented by hand
 * WARNING: A lot of the code here is stolen from soruces like wiki, and shoved in by me
 * How this works I do not know. Neither do I want to know.
 */

typedef enum nv_rbnode_color
{
  NOVA_RBNODE_COLOR_RED = 0,
  NOVA_RBNODE_COLOR_BLK = 1
} nv_rbnode_color;

struct nv_rbmap_node
{
  nv_rbmap_node_t* parent;
  nv_rbmap_node_t* children[2];
  nv_rbnode_color  color;
  void*            key;
  void*            val;
};

struct nv_rbmap_iterator
{
  nv_rbmap_node_t*  current;
  size_t            top;
  size_t            capacity;
  nv_rbmap_node_t** stack;
};

struct nv_rbmap
{
  unsigned          canary;
  size_t            key_size;
  size_t            val_size;
  nv_compare_fn     compare_fn;
  nv_rbmap_node_t*  root;
  nv_rbmap_node_t** nodes;
};

extern nv_error nv_rbmap_init(size_t key_size, size_t val_size, nv_compare_fn compare_fn, nv_rbmap_t* dst);

extern void nv_rbmap_destroy(nv_rbmap_t* map);

extern void nv_rbmap_iterator_init(nv_rbmap_t* map, nv_rbmap_iterator_t* dst);

extern void nv_rbmap_iterator_reserve(nv_rbmap_iterator_t* itr, size_t num_elems);

/**
 * Get the next node pointer in an iteration
 * Returns NULL when there are no more nodes to iterate left
 */
extern nv_rbmap_node_t* nv_rbmap_iterator_next(nv_rbmap_iterator_t* itr);

extern void nv_rbmap_iterator_destroy(nv_rbmap_iterator_t* itr);

extern void nv_rbmap_left_rotate(nv_rbmap_t* map, nv_rbmap_node_t* x);
extern void nv_rbmap_right_rotate(nv_rbmap_t* map, nv_rbmap_node_t* y);

/**
 * After insertion/deletion operations, these functions will
 * Fix all violations of the color rules in the map
 * Use insert_fixup() if there was an insertion, delete fixup otherwise, simple.
 */
extern void nv_rbmap_insert_fixup(nv_rbmap_t* map, nv_rbmap_node_t* z);
extern void nv_rbmap_delete_fixup(nv_rbmap_t* map, nv_rbmap_node_t* x);

/* Find the minimum node in a subtree */
extern nv_rbmap_node_t* nv_rbmap_minimum(nv_rbmap_node_t* node);

/* Replaces one subtree as a child of its parent with another subtree */
/* Do not ask me what that means. I will get agressive. */
extern void nv_rbmap_transplant(nv_rbmap_t* map, nv_rbmap_node_t* u, nv_rbmap_node_t* v);

/**
 * @param value value to be inserted. Must not be NULL.
 * @param user_hash_data An argument passed to the comparision function. May be NULL.
 */
extern void nv_rbmap_insert(nv_rbmap_t* map, const void* key, const void* value, void* user_hash_data);

/**
 * Warning: big.
 * OK that doesn't mean you can just not delete a node, just be careful!
 */
extern void nv_rbmap_delete(nv_rbmap_t* map, nv_rbmap_node_t* z);

/**
 * The root node. Yes, this is functionally equivalent to acessing the member directly.
 */
extern nv_rbmap_node_t* nv_rbmap_root(nv_rbmap_t* map);

/**
 * @param user_hash_data An argument passed to the comparision function. May be NULL.
 */
extern void* nv_rbmap_find(const nv_rbmap_t* NV_RESTRICT map, const void* NV_RESTRICT key, void* user_hash_data);

NOVA_HEADER_END

#endif // NV_STD_CONTAINERS_RBMAP_H
