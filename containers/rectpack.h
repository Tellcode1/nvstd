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

/**
 * An implementation of the skyline algorithm for packing rects.
 *
 * Nice sources:
 * https://jvernay.fr/en/blog/skyline-2d-packer/implementation/
 * https://github.com/juj/RectangleBinPack/blob/master/SkylineBinPack.h
 */

#ifndef __NOVA_RECT_PACK_H__
#define __NOVA_RECT_PACK_H__

#include "../errorcodes.h"
#include "../stdafx.h"
#include <SDL3/SDL_mutex.h>

NOVA_HEADER_START

typedef struct nv_skyline_bin  nv_skyline_bin_t;
typedef struct nv_skyline_rect nv_skyline_rect_t;

struct nv_skyline_rect
{
  size_t width, height, posx, posy;
};

struct nv_skyline_bin
{
  unsigned           canary;
  size_t*            skyline;
  nv_skyline_rect_t* rects;
  size_t             width, height;
  size_t             allocated_rect_count;
  size_t             num_rects;
  SDL_Mutex*         mutex;
};

extern nv_error nv_skyline_bin_init(size_t width, size_t height, nv_skyline_bin_t* bin);
extern void     nv_skyline_bin_destroy(nv_skyline_bin_t* bin);

/**
 * @brief The maximum height of a skyline in the bin
 */
extern size_t nv_skyline_bin_max_height(const nv_skyline_bin_t* bin, size_t x, size_t w);

/**
 * @return Returns true if a placement was found, anything else if not.
 * WARNING: This may not trigger a resize!
 */
extern int nv_skyline_bin_find_best_placement(const nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t* best_x, size_t* best_y);

/**
 * why are x and y needed as seperate arguements? Can't they be included in the rect
 */
extern void nv_skyline_bin_place_rect(nv_skyline_bin_t* bin, const nv_skyline_rect_t* rect, size_t x, size_t y);

/**
 * @brief Pack all rects into the bin. This involves copying over the rects into their positions, as specified from posx and posy
 * The function logs and error if it could not pack a rect
 * Say, if the bin got full.
 * WARNING: This function should be called only once per bin!
 */
extern void nv_skyline_bin_pack_rects(nv_skyline_bin_t* bin, nv_skyline_rect_t* rects, size_t nrects);

/**
 * WARNING: new_w < current_width or new_h < current_height is currently UNDEFINED
 */
extern void nv_skyline_bin_resize(nv_skyline_bin_t* bin, size_t new_w, size_t new_h);

NOVA_HEADER_END

#endif //__NOVA_RECT_PACK_H__
