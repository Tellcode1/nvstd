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

#ifndef NV_STD_MATH_VEC3_H
#define NV_STD_MATH_VEC3_H

#include "../stdafx.h"

#include <math.h>

NOVA_HEADER_START

#define NV_DECL_VEC3(TYPE, NAME, FUNC, SQRT_FUNC)                                                                                                                             \
  typedef struct NAME                                                                                                                                                         \
  {                                                                                                                                                                           \
    TYPE x, y, z;                                                                                                                                                             \
  }(NAME);                                                                                                                                                                    \
  static const NAME  FUNC##zero = { 0, 0, 0 };                                                                                                                                \
  static const NAME  FUNC##one  = { 1, 1, 1 };                                                                                                                                \
  static inline NAME FUNC##init(const TYPE x, const TYPE y, const TYPE z) { return (NAME){ x, y, z }; }                                                                       \
  static inline NAME FUNC##add(const NAME v1, const NAME v2) { return (NAME){ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z }; }                                                      \
  static inline NAME FUNC##sub(const NAME v1, const NAME v2) { return (NAME){ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z }; }                                                      \
  static inline NAME FUNC##mulv(const NAME v1, const NAME v2) { return (NAME){ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z }; }                                                     \
  static inline NAME FUNC##muls(const NAME v1, const TYPE s) { return (NAME){ v1.x * s, v1.y * s, v1.z * s }; }                                                               \
  static inline NAME FUNC##divs(const NAME v1, const TYPE s) { return (NAME){ v1.x / s, v1.y / s, v1.z / s }; }                                                               \
  static inline TYPE FUNC##magcheap(const NAME v) { return v.x * v.x + v.y * v.y + v.z * v.z; }                                                                               \
  static inline TYPE FUNC##mag(const NAME v) { return SQRT_FUNC(v.x * v.x + v.y * v.y + v.z * v.z); }                                                                         \
  static inline TYPE FUNC##dot(const NAME v1, const NAME v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }                                                            \
  static inline NAME FUNC##cross(const NAME v1, const NAME v2) { return (NAME){ v1.y * v2.z - v2.y * v1.z, v1.z * v2.x - v2.z * v1.x, v1.x * v2.y - v2.x * v1.y }; }          \
  static inline bool FUNC##areeq(const NAME v1, const NAME v2) { return (bool)(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z); }                                               \
  static inline NAME FUNC##normalize(const NAME v)                                                                                                                            \
  {                                                                                                                                                                           \
    TYPE magnitude = FUNC##mag(v);                                                                                                                                            \
    if (magnitude == 0) return nv_zinit(NAME);                                                                                                                                \
    return FUNC##divs(v, magnitude);                                                                                                                                          \
  }

NV_DECL_VEC3(int, vec3i, v3i, (int)sqrtf)
NV_DECL_VEC3(unsigned, vec3u, v3u, (unsigned)sqrtf)
NV_DECL_VEC3(float, vec3f, v3f, sqrtf)
NV_DECL_VEC3(double, vec3d, v3d, sqrt)
NV_DECL_VEC3(double, vec3, v3, sqrt)

NOVA_HEADER_END

#endif // NV_STD_MATH_VEC3_H
