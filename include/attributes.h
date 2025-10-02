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

/* Utility file for cross compiler attribute support */

#ifndef STD_ATTRIBUTES_H
#define STD_ATTRIBUTES_H

#include "stdafx.h"

NOVA_HEADER_START

#ifndef NOVA_HAS_ATTR
#  if defined(__has_attribute)
#    define NOVA_HAS_ATTR(attr) __has_attribute(attr)
#  else
#    define NOVA_HAS_ATTR(attr)
#  endif
#endif

#if (defined(__GNUC__) || defined(__clang__)) && defined(__has_attribute)
#  if (defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11)
#    define NOVA_ATTR_MALLOC(free_fn, size_arg_idx) __attribute__((malloc(free_fn, size_arg_idx)))
#  elif (NOVA_HAS_ATTR(malloc))
// clang surprisingly does not support arguments to malloc attributes
#    define NOVA_ATTR_MALLOC(free_fn, size_arg_idx) __attribute__((malloc))
#  else
#    define NOVA_ATTR_MALLOC(free_fn, size_arg_idx)
#  endif
#  if NOVA_HAS_ATTR(pure)
#    define NOVA_ATTR_PURE __attribute__((pure))
#  endif
#  if NOVA_HAS_ATTR(const)
#    define NOVA_ATTR_CONST __attribute__((const))
#  endif
#  if NOVA_HAS_ATTR(noreturn)
#    define NOVA_ATTR_NORETURN __attribute__((noreturn))
#  endif
#  if NOVA_HAS_ATTR(deprecated)
#    define NOVA_ATTR_DEPRECATED __attribute__((deprecated))
#  endif
#  if NOVA_HAS_ATTR(unused)
#    define NOVA_ATTR_UNUSED __attribute__((unused))
#  endif
#  if NOVA_HAS_ATTR(aligned)
#    define NOVA_ATTR_ALIGNED(n) __attribute__((aligned(n)))
#  endif
#  if NOVA_HAS_ATTR(format)
#    define NOVA_ATTR_FORMAT(fmt_idx, args_idx) __attribute__((format(printf, fmt_idx, args_idx)))
#  endif
#  if NOVA_HAS_ATTR(hot)
#    define NOVA_ATTR_HOT __attribute__((hot))
#  endif
#  if NOVA_HAS_ATTR(cold)
#    define NOVA_ATTR_COLD __attribute__((cold))
#  endif
#  if NOVA_HAS_ATTR(constructor)
#    define NOVA_ATTR_CONSTRUCTOR __attribute__((constructor))
#  endif
#  if NOVA_HAS_ATTR(destructor)
#    define NOVA_ATTR_DESTRUCTOR __attribute__((destructor))
#  endif
#  if NOVA_HAS_ATTR(nonnull)
#    define NOVA_ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#  endif
#  if NOVA_HAS_ATTR(returns_nonnull)
#    define NOVA_ATTR_RETURNS_NONNULL __attribute__((returns_nonnull))
#  endif
// #  if NOVA_HAS_ATTR(access)
// #    define NOVA_ATTR_ACCESS(...) __attribute__((access(__VA_ARGS__)))
// #  endif

#elif defined(_MSC_VER)
#  define NOVA_ATTR_MALLOC __declspec(restrict)
#  define NOVA_ATTR_PURE
#  define NOVA_ATTR_CONST
#  define NOVA_ATTR_NORETURN __declspec(noreturn)
#  define NOVA_ATTR_DEPRECATED __declspec(deprecated)
#  define NOVA_ATTR_UNUSED
#  define NOVA_ATTR_ALIGNED(n) __declspec(align(n))
#  define NOVA_ATTR_FORMAT(fmt_idx, args_idx)
#  define NOVA_ATTR_HOT
#  define NOVA_ATTR_COLD
#  define NOVA_ATTR_CONSTRUCTOR __declspec(dllexport)
#  define NOVA_ATTR_DESTRUCTOR __declspec(dllexport)
#  define NOVA_ATTR_NONNULL(...)
#  define NOVA_ATTR_RETURNS_NONNULL
#  define NOVA_ATTR_ACCESS(...)
#else
#  define NOVA_ATTR_MALLOC
#  define NOVA_ATTR_PURE
#  define NOVA_ATTR_CONST
#  define NOVA_ATTR_NORETURN
#  define NOVA_ATTR_DEPRECATED
#  define NOVA_ATTR_UNUSED
#  define NOVA_ATTR_ALIGNED(n)
#  define NOVA_ATTR_FORMAT(fmt_idx, args_idx)
#  define NOVA_ATTR_HOT
#  define NOVA_ATTR_COLD
#  define NOVA_ATTR_CONSTRUCTOR
#  define NOVA_ATTR_DESTRUCTOR
#  define NOVA_ATTR_NONNULL(...)
#  define NOVA_ATTR_RETURNS_NONNULL
#  define NOVA_ATTR_ACCESS(...)
#endif

NOVA_HEADER_END

#endif // STD_ATTRIBUTES_H
