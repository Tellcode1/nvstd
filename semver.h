/*
  MIT License

  Copyright (c) 2025 Tellcode

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
 * Utils for packing and unpacking version numbers as per semantic versioning
 * https://semver.org/
 */

#ifndef NOVA_SEMANTIC_VERSIONING_H_INCLUDED_
#define NOVA_SEMANTIC_VERSIONING_H_INCLUDED_

#include "stdafx.h"

NOVA_HEADER_START

#define NV_SEMVER_MAJOR_VERSION_NUMBER_MAX 1023
#define NV_SEMVER_MINOR_VERSION_NUMBER_MAX 1023
#define NV_SEMVER_PATCH_VERSION_NUMBER_MAX 4095

#ifndef __NOVA_SEMVER_MAJOR_SHIFT
#  define __NOVA_SEMVER_MAJOR_SHIFT (22)
#  define __NOVA_SEMVER_MINOR_SHIFT (12)
#  define __NOVA_SEMVER_PATCH_SHIFT (0)
#endif

#define NV_SEMVER_PACK_VERSION_DEF(major, minor, patch) ((major << __NOVA_SEMVER_MAJOR_SHIFT) | (minor << __NOVA_SEMVER_MINOR_SHIFT) | (patch << __NOVA_SEMVER_PATCH_SHIFT))
#define NV_SEMVER_UNPACK_VERSION_MAJOR_DEF(packed) ((version >> __NOVA_SEMVER_MAJOR_SHIFT) & 0x3FF)
#define NV_SEMVER_UNPACK_VERSION_MINOR_DEF(packed) ((version >> __NOVA_SEMVER_MINOR_SHIFT) & 0x3FF)
#define NV_SEMVER_UNPACK_VERSION_PATCH_DEF(packed) ((version >> __NOVA_SEMVER_PATCH_SHIFT) & 0xFFF)

typedef uint32_t version_t;

static inline version_t
nv_semver_pack_version(version_t major, version_t minor, version_t patch)
{
  nv_assert(major <= NV_SEMVER_MAJOR_VERSION_NUMBER_MAX);
  nv_assert(minor <= NV_SEMVER_MINOR_VERSION_NUMBER_MAX);
  nv_assert(patch <= NV_SEMVER_PATCH_VERSION_NUMBER_MAX);

  return (major << __NOVA_SEMVER_MAJOR_SHIFT) | (minor << __NOVA_SEMVER_MINOR_SHIFT) | (patch << __NOVA_SEMVER_PATCH_SHIFT);
}

static inline void
nv_semver_unpack_version(version_t version, version_t* major, version_t* minor, version_t* patch)
{
  *major = (version >> __NOVA_SEMVER_MAJOR_SHIFT) & 0x3FF;
  *minor = (version >> __NOVA_SEMVER_MINOR_SHIFT) & 0x3FF;
  *patch = (version >> __NOVA_SEMVER_PATCH_SHIFT) & 0xFFF;
}

NOVA_HEADER_END

#endif //__NOVA_SEMVER_H__
