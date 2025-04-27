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

#ifndef __NOVA_ATLAS_H__
#define __NOVA_ATLAS_H__

#include "../alloc.h"
#include "../errorcodes.h"
#include "../format.h"
#include "../image.h"
#include "../stdafx.h"
#include "rectpack.h"
#include <SDL2/SDL_mutex.h>

NOVA_HEADER_START

// Possibly add features for removing textures?
// Possibly start compressing the texture in async when finish is called?

typedef struct nv_texture_atlas_s nv_texture_atlas_t;

extern nv_error nv_texture_atlas_init(size_t width, size_t height, nv_format fmt, u32 padding, nv_texture_atlas_t* dst);

/**
 * @return Returns false(0) if the image was not packed and anything else if it was
 */
extern int nv_texture_atlas_add(nv_texture_atlas_t* atlas, const nv_image_t* img, size_t* out_x, size_t* out_y);

extern void nv_texture_atlas_resize(nv_texture_atlas_t* atlas, int scale);

/**
 * @brief Resizes the atlas to only fit the current amount of glyphs
 * @return 1 (true) if the atlas was truncated. this need not be checked.
 */
extern int nv_texture_atlas_finish(nv_texture_atlas_t* atlas);

extern void nv_texture_atlas_destroy(nv_texture_atlas_t* atlas);

struct nv_texture_atlas_s
{
  unsigned  canary; // = 0xDEADBEEF
  u32       padding;
  nv_format format;

  nv_allocator_fn alloc;

  SDL_mutex* mutex;

  unsigned char* data;

  size_t width;
  size_t height;

  nv_skyline_bin_t bin;
};

NOVA_HEADER_END

#endif //__NOVA_ATLAS_H__
