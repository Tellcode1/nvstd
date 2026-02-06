#include "../../include/containers/rectpack.h"

#include "../../include/alloc.h"
#include "../../include/errorcodes.h"
#include "../../include/stdafx.h"

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

nv_error
nv_skyline_bin_init(size_t width, size_t height, nv_skyline_bin_t* dst)
{
  nv_assert_else_return(dst != NULL, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(width != 0, NV_ERROR_INVALID_ARG);
  nv_assert_else_return(height != 0, NV_ERROR_INVALID_ARG);

  *dst = nv_zinit(nv_skyline_bin_t);

  dst->canary = NOVA_CONT_CANARY;
  dst->width  = width;
  dst->height = height;

  dst->rects                = NULL;
  dst->num_rects            = 0;
  dst->allocated_rect_count = 0;

  dst->skyline = (size_t*)nv_zmalloc(width * sizeof(size_t));
  nv_assert_else_return(dst->skyline != NULL, NV_ERROR_MALLOC_FAILED);

  nv_assert_else_return(NOVA_CONT_IS_VALID(dst), NV_ERROR_BROKEN_STATE);

  return NV_ERROR_SUCCESS;
}

void
nv_skyline_bin_destroy(nv_skyline_bin_t* bin)
{
  if (!bin) { return; }
  nv_assert(NOVA_CONT_IS_VALID(bin));
  if (bin->rects) { nv_free(bin->rects); }
  if (bin->skyline) { nv_free(bin->skyline); }
}

size_t
nv_skyline_bin_max_height_nolock(const nv_skyline_bin_t* bin, size_t x, size_t w)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  size_t max_h = 0;
  for (size_t i = x; i < x + w && i < bin->width; i++)
  {
    if (bin->skyline[i] > max_h) { max_h = bin->skyline[i]; }
  }

  return max_h;
}

size_t
nv_skyline_bin_max_height(const nv_skyline_bin_t* bin, size_t x, size_t w)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  size_t ret = nv_skyline_bin_max_height_nolock(bin, x, w);

  return ret;
}

int
nv_skyline_bin_find_best_placement(const nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t* best_x, size_t* best_y)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  /**
   * SIZE_MAX is used as a no find value.
   * Obviously, no rect in a bin should have a position of SIZE_MAX
   * That implies that its width is SIZE_MAX + sizeofrect which is impossible
   */
  size_t min_y = SIZE_MAX;
  *best_x      = SIZE_MAX;
  *best_y      = SIZE_MAX;

  if (rect->width > bin->width) { return -1; }

  size_t max_x = bin->width - rect->width;
  for (size_t x = 0; x <= max_x; x++)
  {
    size_t y = nv_skyline_bin_max_height_nolock(bin, x, rect->width);
    if (y + rect->height <= bin->height && y < min_y)
    {
      min_y   = y;
      *best_x = x;
      *best_y = y;
      break;
    }
  }

  return (*best_x != SIZE_MAX);
}

void
nv_skyline_bin_place_rect(nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t x, size_t y)
{
  nv_assert(NOVA_CONT_IS_VALID(bin));

  if (bin->num_rects >= bin->allocated_rect_count)
  {
    size_t new_alloc = (bin->allocated_rect_count == 0) ? 2 : bin->allocated_rect_count * 2;

    if (bin->rects) { bin->rects = (nv_skyline_rect_t*)nv_realloc(bin->rects, new_alloc * sizeof(nv_skyline_rect_t)); }
    else
    {
      bin->rects = nv_zmalloc(new_alloc * sizeof(nv_skyline_rect_t));
    }
    bin->allocated_rect_count = new_alloc;
  }

  bin->rects[bin->num_rects++] = (nv_skyline_rect_t){ rect->width, rect->height, x, y };

  for (size_t i = x; i < x + rect->width && i < bin->width; i++) { bin->skyline[i] = y + rect->height; }
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

  for (size_t i = 0; i < bin->num_rects; i++)
  {
    nv_skyline_rect_t rect = bin->rects[i];
    if (rect.posx + rect.width > new_w || rect.posy + rect.height > new_h)
    {
      nv_skyline_rect_t* tmp = NULL;
      if (!invalid_rects) { tmp = (nv_skyline_rect_t*)nv_zmalloc(sizeof(nv_skyline_rect_t)); }
      else
      {
        tmp = (nv_skyline_rect_t*)nv_realloc(invalid_rects, (num_invalid + 1) * sizeof(nv_skyline_rect_t));
      }
      if (!tmp)
      {
        nv_free(valid_rects);
        nv_free(invalid_rects);
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
      return;
    }
    // if it's bigger horizontally, clear the new entries
    if (new_w > bin->width)
    {
      for (size_t i = bin->width; i < new_w; i++) { new_skyline[i] = 0; }
    }
    bin->skyline = new_skyline;
  }

  for (size_t i = 0; i < new_w; i++)
  {
    if (bin->skyline[i] > new_h) { bin->skyline[i] = new_h; }
  }

  for (size_t i = 0; i < num_valid; i++)
  {
    nv_skyline_rect_t rect = valid_rects[i];
    for (size_t x = rect.posx; x < rect.posx + rect.width && x < new_w; x++)
    {
      if (bin->skyline[x] < rect.posy + rect.height) { bin->skyline[x] = rect.posy + rect.height; }
    }
  }

  if (bin->rects) { nv_free(bin->rects); }
  bin->rects                = valid_rects;
  bin->num_rects            = num_valid;
  bin->allocated_rect_count = num_valid;

  for (size_t i = 0; i < num_invalid; i++)
  {
    size_t x = 0;
    size_t y = 0;
    if (nv_skyline_bin_find_best_placement(bin, &invalid_rects[i], &x, &y)) { nv_skyline_bin_place_rect(bin, &invalid_rects[i], x, y); }
    else
    {
      nv_log_error("failed to repack rect %lu after resize\n", i);
    }
  }

  if (invalid_rects) { nv_free(invalid_rects); }

  bin->width  = new_w;
  bin->height = new_h;
}
