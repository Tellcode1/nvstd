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

#ifndef NOVA_TYPES_H_INCLUDED_
#define NOVA_TYPES_H_INCLUDED_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

  typedef uint64_t nv_word_t;

  typedef uint64_t u64;
  typedef uint32_t u32;
  typedef uint16_t u16;
  typedef uint8_t  u8;
  typedef int8_t   sbyte;
  typedef uint8_t  ubyte;

  typedef unsigned char uchar;

  typedef int64_t i64;
  typedef int32_t i32;
  typedef int16_t i16;
  typedef int8_t  i8;

  // They ARE 32 and 64 bits by IEEE-754 but aren't set by the standard
  // But there is a 99.9% chance that they will be
  typedef float  f32;
  typedef double f64;

  /**
   * little easier to type
   */
#define null (NULL)

#ifdef __cplusplus
}
#endif

#endif // NOVA_TYPES_H_INCLUDED_
