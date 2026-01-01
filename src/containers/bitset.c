#include "../../include/containers/bitset.h"

#include "../../include/errorcodes.h"
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
nv_bitset_init(size_t init_capacity, nv_bitset_t* set)
{
  nv_assert_else_return(set != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(init_capacity > 0, NV_ERROR_INVALID_ARG);

  *set = nv_zinit(nv_bitset_t);

  if (init_capacity > 0)
  {
    init_capacity = (init_capacity + 7) / 8;
    set->size     = init_capacity;

    set->data = nv_zmalloc(init_capacity * sizeof(uint8_t));
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
  set->data[bitindex / 8] |= (1U << (bitindex % 8U));
}

void
nv_bitset_set_bit_to(nv_bitset_t* set, size_t bitindex, nv_bitset_bit to)
{
  to ? nv_bitset_set_bit(set, bitindex) : nv_bitset_clear_bit(set, bitindex);
}

void
nv_bitset_clear_bit(nv_bitset_t* set, size_t bitindex)
{
  set->data[bitindex / 8] &= ~(1U << (bitindex % 8U));
}

void
nv_bitset_toggle_bit(nv_bitset_t* set, size_t bitindex)
{
  set->data[bitindex / 8] ^= (1U << (bitindex % 8U));
}

nv_bitset_bit
nv_bitset_access_bit(const nv_bitset_t* set, size_t bitindex)
{
  nv_bitset_bit bit = (set->data[bitindex / 8] & (1U << (bitindex % 8U))) != 0;
  return bit;
}

void
nv_bitset_copy_from(nv_bitset_t* dst, const nv_bitset_t* src)
{
  if (!src->data) { return; }
  if (src->size != dst->size && dst->data)
  {
    nv_free(dst->data);
    dst->data = nv_zmalloc(src->size * sizeof(u8));
    dst->size = src->size;
  }
  if (dst->data && src->data) { nv_memcpy(dst->data, src->data, src->size); }
}

void
nv_bitset_destroy(nv_bitset_t* set)
{
  if (!set) { return; }

  nv_free(set->data);

  nv_bzero(set, sizeof(nv_bitset_t));
}

void
nv_bitset_copy_from_bool_array(nv_bitset_t* set, const bool* array, size_t array_size)
{
  if (!set) { return; }

  if (set->size != array_size)
  {
    nv_free(set->data);
    set->size = (array_size + 7) / 8;
    set->data = nv_zmalloc(set->size * sizeof(uint8_t));
  }

  for (size_t i = 0; i < array_size; i++) { (array[i]) ? nv_bitset_set_bit(set, i) : nv_bitset_clear_bit(set, i); }
}
