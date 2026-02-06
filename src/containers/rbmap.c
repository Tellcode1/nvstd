#include "../../include/containers/rbmap.h"

#include "../../include/error.h"
#include "../../include/stdafx.h"
#include "../../include/string.h"

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

nv_error
nv_rbmap_init(size_t key_size, size_t val_size, nv_compare_fn compare_fn, nv_rbmap_t* dst)
{
  nv_assert_else_return(key_size != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(val_size != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(compare_fn != NULL, NV_ERROR_INVALID_ARG);

  *dst = nv_zinit(nv_rbmap_t);

  dst->canary     = NOVA_CONT_CANARY;
  dst->key_size   = key_size;
  dst->val_size   = val_size;
  dst->compare_fn = compare_fn;
  dst->root       = NULL;

  return NV_ERROR_SUCCESS;
}

void
nv_rbmap_left_rotate(nv_rbmap_t* map, nv_rbmap_node_t* x)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  nv_rbmap_node_t* y = x->children[1];
  x->children[1]     = y->children[0];
  if (y->children[0]) { y->children[0]->parent = x; }

  y->parent = x->parent;
  if (!x->parent) { map->root = y; }
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
  if (x->children[1]) { x->children[1]->parent = y; }

  x->parent = y->parent;
  if (!y->parent) { map->root = x; }
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
  while (node && node->children[0] != NULL) { node = node->children[0]; }
  return node;
}

void
nv_rbmap_transplant(nv_rbmap_t* map, nv_rbmap_node_t* u, nv_rbmap_node_t* v)
{
  nv_assert(NOVA_CONT_IS_VALID(map));
  if (u->parent == NULL) { map->root = v; }
  else if (u == u->parent->children[0]) { u->parent->children[0] = v; }
  else
  {
    u->parent->children[1] = v;
  }

  if (v != NULL) { v->parent = u->parent; }
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
      if (!w) { continue; }
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
          if (w->children[0]) { w->children[0]->color = NOVA_RBNODE_COLOR_BLK; }
          w->color = NOVA_RBNODE_COLOR_RED;
          nv_rbmap_right_rotate(map, w);
          w = x->parent->children[1];
        }
        w->color         = x->parent->color;
        x->parent->color = NOVA_RBNODE_COLOR_BLK;
        if (w->children[1]) { w->children[1]->color = NOVA_RBNODE_COLOR_BLK; }
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
          if (w->children[1]) { w->children[1]->color = NOVA_RBNODE_COLOR_BLK; }
          w->color = NOVA_RBNODE_COLOR_RED;
          nv_rbmap_left_rotate(map, w);
          w = x->parent->children[0];
        }
        w->color         = x->parent->color;
        x->parent->color = NOVA_RBNODE_COLOR_BLK;
        if (w->children[0]) { w->children[0]->color = NOVA_RBNODE_COLOR_BLK; }
        nv_rbmap_right_rotate(map, x->parent);
        x = map->root;
      }
    }
  }
  if (x) { x->color = NOVA_RBNODE_COLOR_BLK; }
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
      if (x) { x->parent = y; }
    }
    else
    {
      nv_rbmap_transplant(map, y, y->children[1]);
      y->children[1] = z->children[1];
      if (y->children[1]) { y->children[1]->parent = y; }
    }
    nv_rbmap_transplant(map, z, y);
    y->children[0] = z->children[0];
    if (y->children[0]) { y->children[0]->parent = y; }
    y->color = z->color;
  }

  if (y_original_color == NOVA_RBNODE_COLOR_BLK && x != NULL) { nv_rbmap_delete_fixup(map, x); }

  nv_free(z);
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

  void* allocation = nv_zmalloc(sizeof(nv_rbmap_node_t));

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
    if (cmp < 0) { x = x->children[0]; }
    else
    {
      x = x->children[1];
    }
  }
  z->parent = y;
  if (y == NULL) { map->root = z; }
  else if (map->compare_fn(key, y->key, map->key_size, user_hash_data) < 0) { y->children[0] = z; }
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

  *dst          = nv_zinit(nv_rbmap_iterator_t);
  dst->current  = map->root;
  dst->stack    = NULL;
  dst->capacity = 0;
  dst->top      = 0;
}

void
nv_rbmap_iterator_reserve(nv_rbmap_iterator_t* itr, size_t num_elems)
{
  nv_assert(itr != NULL);
  if (itr->stack) { itr->stack = (nv_rbmap_node_t**)nv_realloc(itr->stack, num_elems * sizeof(nv_rbmap_node_t*)); }
  else
  {
    itr->stack = (nv_rbmap_node_t**)nv_zmalloc(num_elems * sizeof(nv_rbmap_node_t*));
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
  if (!itr) { return; }
  if (itr->stack) { nv_free(itr->stack); }
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
    if (cmp < 0) { node = node->children[0]; }
    else if (cmp > 0) { node = node->children[1]; }
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
  nodes                      = (nv_rbmap_node_t**)nv_zmalloc(capacity * sizeof(nv_rbmap_node_t*));

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
    nv_free(node);
  }
  nv_free(nodes);
  nv_rbmap_iterator_destroy(&itr);
  map->root = NULL;
}
