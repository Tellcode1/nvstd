#ifndef STD_MATH_VEC4_H
#define STD_MATH_VEC4_H

#include "../stdafx.h"
#include <math.h>

NOVA_HEADER_START

#define NV_DECL_VEC4(TYPE, NAME, FUNC, SQRT_FUNC)                                                                                                                             \
  typedef struct NAME                                                                                                                                                         \
  {                                                                                                                                                                           \
    TYPE x, y, z, w;                                                                                                                                                          \
  }(NAME);                                                                                                                                                                    \
  static const NAME  FUNC##zero = { 0, 0, 0, 0 };                                                                                                                             \
  static const NAME  FUNC##one  = { 1, 1, 1, 1 };                                                                                                                             \
  static inline NAME FUNC##init(const TYPE x, const TYPE y, const TYPE z, const TYPE w) { return (NAME){ x, y, z, w }; }                                                      \
  static inline NAME FUNC##add(const NAME v1, const NAME v2) { return (NAME){ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w }; }                                         \
  static inline NAME FUNC##sub(const NAME v1, const NAME v2) { return (NAME){ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w }; }                                         \
  static inline NAME FUNC##mulv(const NAME v1, const NAME v2) { return (NAME){ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w }; }                                        \
  static inline NAME FUNC##muls(const NAME v1, const TYPE s) { return (NAME){ v1.x * s, v1.y * s, v1.z * s, v1.w * s }; }                                                     \
  static inline NAME FUNC##divs(const NAME v1, const TYPE s) { return (NAME){ v1.x / s, v1.y / s, v1.z / s, v1.w / s }; }                                                     \
  static inline TYPE FUNC##magcheap(const NAME v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }                                                                   \
  static inline TYPE FUNC##mag(const NAME v) { return SQRT_FUNC(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w); }                                                             \
  static inline TYPE FUNC##dot(const NAME v1, const NAME v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w); }                                              \
  static inline bool FUNC##areeq(const NAME v1, const NAME v2) { return (bool)(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w); }                               \
  static inline NAME FUNC##normalize(const NAME v)                                                                                                                            \
  {                                                                                                                                                                           \
    TYPE magnitude = FUNC##mag(v);                                                                                                                                            \
    if (magnitude == 0)                                                                                                                                                       \
      return nv_zero_init(NAME);                                                                                                                                              \
    return FUNC##divs(v, magnitude);                                                                                                                                          \
  }

NV_DECL_VEC4(int, vec4i, v4i, (int)sqrtf)
NV_DECL_VEC4(unsigned, vec4u, v4u, (unsigned)sqrtf)
NV_DECL_VEC4(float, vec4f, v4f, sqrtf)
NV_DECL_VEC4(double, vec4d, v4d, sqrt)
NV_DECL_VEC4(double, vec4, v4, sqrt)

NOVA_HEADER_END

#endif // STD_MATH_VEC4_H
