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

#ifndef NV_STD_MATH_MATH_H
#define NV_STD_MATH_MATH_H

#include "../attributes.h"
#include "../stdafx.h"
#include "vec2.h"

#include <stdbool.h>

NOVA_HEADER_START

#define NVM_VEC_INDEX(v, i) (&((v).x))[i]

#define nvm_vec_copy(dst, src)                                                                                                                                                \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    nv_memset(&dst, 0, sizeof(dst));                                                                                                                                          \
    size_t _nvm_min = NV_MIN(sizeof(dst) / sizeof((dst).x), sizeof(src) / sizeof((src).x));                                                                                   \
    for (size_t _i = 0; _i < _nvm_min; ++_i) { NVM_VEC_INDEX(dst, _i) = NVM_VEC_INDEX(src, _i); }                                                                             \
  } while (0)

#define nvm_mat_copy(dst, src)                                                                                                                                                \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    const int size = nv_arrlen((dst).data);                                                                                                                                   \
    for (int __nv_matrix_copy_i = 0; __nv_matrix_copy_i < (size); __nv_matrix_copy_i++)                                                                                       \
      for (int __nv_matrix_copy_j = 0; __nv_matrix_copy_j < (size); __nv_matrix_copy_j++)                                                                                     \
        ((&(dst).data[__nv_matrix_copy_i].x)[__nv_matrix_copy_j] = (&(src).data[__nv_matrix_copy_i].x)[__nv_matrix_copy_j]);                                                  \
  } while (0)

static const double NVM_PI  = 3.1415926535897932385;
static const double NVM_2PI = 6.283185307179586;

// Multiply these to convert degrees to radians or vice versa
static const double NVM_DEG2RAD_CONSTANT = 0.017453292519943295; // 2Pi / 360.0
static const double NVM_RAD2DEG_CONSTANT = 57.29577951308232;    // 1.0 / DEG2RAD_CONSTANT -> rad / DEG2RAD_CONSTANT

#define NVM_LERP(a, b, t) ((a) + (((b) - (a)) * (t)))

/* Unneeded, defined in stdafx.h as NV_MIN,NV_MAX */
// #define NVM_MAX(a, b) ((a) > ((NV_TYPEOF(a))(b)) ? (a) : ((NV_TYPEOF(a))(b)))
// #define NVM_MIN(a, b) ((a) < ((NV_TYPEOF(a))(b)) ? (a) : ((NV_TYPEOF(a))(b)))

#define NVM_CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define NVM_CLAMP01(x) NVM_CLAMP((x), 0, 1)

// Round x to the nearest multiple of y
#define NVM_ROUND(x, y) (round((x) / (y)) * (y))

// The constants aren't used because for static const variables, they can cause minor issues like not compiling at all.
#define NVM_DEG2RAD(x) ((x) * 0.017453292519943295)
#define NVM_RAD2DEG(x) ((x) * 57.29577951308232)

typedef struct nvm_rect2d
{
  // The CENTER of the rect
  vec2 position;
  vec2 size;
} nvm_rect2d;

static inline bool
nvm_aabb(const struct nvm_rect2d* r1, const struct nvm_rect2d* r2)
{
  const vec2 r1_half_size = v2muls(r1->size, 0.5F);
  const vec2 r2_half_size = v2muls(r2->size, 0.5F);

  const vec2 r1_min = v2sub(r1->position, r1_half_size);
  const vec2 r1_max = v2add(r1->position, r1_half_size);

  const vec2 r2_min = v2sub(r2->position, r2_half_size);
  const vec2 r2_max = v2add(r2->position, r2_half_size);

  const bool overlaps_x = r1_max.x > r2_min.x && r1_min.x < r2_max.x;
  const bool overlaps_y = r1_max.y > r2_min.y && r1_min.y < r2_max.y;

  return overlaps_x && overlaps_y;
}

// Checks if a point is inside the rect
static inline bool
nvm_is_point_inside_rect(const vec2* point, const struct nvm_rect2d* r)
{
  const vec2 r_half_size = v2muls(r->size, 0.5F);

  const vec2 r_min = v2sub(r->position, r_half_size);
  const vec2 r_max = v2add(r->position, r_half_size);

  const bool overlaps_x = r_max.x > point->x && r_min.x < point->x;
  const bool overlaps_y = r_max.y > point->y && r_min.y < point->y;

  return overlaps_x && overlaps_y;
}

NOVA_HEADER_END

#endif // NV_STD_MATH_MATH_H
