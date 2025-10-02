#include "../../include/errorcodes.h"
#include "../../include/stdafx.h"
#include "../../include/string.h"

#include "../../include/containers/bitset.h"

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
nv_bitset_init(size_t init_capacity, nv_allocator_fn allocator, nv_bitset_t* set)
{
  nv_assert_else_return(set != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(allocator != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(init_capacity > 0, NV_ERROR_INVALID_ARG);

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
nv_bitset_set_bit(nv_bitset_t* set, size_t bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] |= (1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

void
nv_bitset_set_bit_to(nv_bitset_t* set, size_t bitindex, nv_bitset_bit to)
{
  to ? nv_bitset_set_bit(set, bitindex) : nv_bitset_clear_bit(set, bitindex);
}

void
nv_bitset_clear_bit(nv_bitset_t* set, size_t bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] &= ~(1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

void
nv_bitset_toggle_bit(nv_bitset_t* set, size_t bitindex)
{
  SDL_LockMutex(set->mutex);
  set->data[bitindex / 8] ^= (1U << (bitindex % 8U));
  SDL_UnlockMutex(set->mutex);
}

nv_bitset_bit
nv_bitset_access_bit(const nv_bitset_t* set, size_t bitindex)
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
  NV_FREE(set->alloc, set->alloc_arg, set->data, set->size);
  SDL_UnlockMutex(set->mutex);

  SDL_DestroyMutex(set->mutex);

  nv_bzero(set, sizeof(nv_bitset_t));
}

void
nv_bitset_copy_from_bool_array(nv_bitset_t* set, const bool* array, size_t array_size)
{
  if (!set)
  {
    return;
  }

  SDL_LockMutex(set->mutex);

  if (set->size != array_size)
  {
    NV_FREE(set->alloc, set->alloc_arg, set->data, set->size);
    set->size = (array_size + 7) / 8;
    set->data = NV_ALLOC(set->alloc, set->alloc_arg, set->size * sizeof(uint8_t));
  }

  for (size_t i = 0; i < array_size; i++)
  {
    (array[i]) ? nv_bitset_set_bit(set, i) : nv_bitset_clear_bit(set, i);
  }

  SDL_UnlockMutex(set->mutex);
}