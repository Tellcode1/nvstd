#ifndef STD_MATH_VEC3_H
#define STD_MATH_VEC3_H

#include "../../std/stdafx.h"
#include <math.h>

NOVA_HEADER_START

#define NV_DECL_VEC3(TYPE, NAME, FUNC, SQRT_FUNC)                                                                                                                             \
  typedef struct NAME                                                                                                                                                         \
  {                                                                                                                                                                           \
    TYPE x, y, z;                                                                                                                                                             \
  }(NAME);                                                                                                                                                                    \
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
    if (magnitude == 0)                                                                                                                                                       \
      return nv_zero_init(NAME);                                                                                                                                              \
    return FUNC##divs(v, magnitude);                                                                                                                                          \
  }

NV_DECL_VEC3(int, vec3i, v3i, sqrt)
NV_DECL_VEC3(float, vec3f, v3f, sqrtf)
NV_DECL_VEC3(double, vec3d, v3d, sqrt)
NV_DECL_VEC3(flt_t, vec3, v3, sqrtf)

NOVA_HEADER_END

#endif // STD_MATH_VEC3_H
