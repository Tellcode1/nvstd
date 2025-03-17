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

#ifndef __NOVA_STRING_CONV_H__
#define __NOVA_STRING_CONV_H__

#include "stdafx.h"

NOVA_HEADER_START

/**
 * @brief Converts an integer to ASCII.
 *
 * @param max Maximum number of characters to write.
 * @return The number of characters written (excluding null terminator).
 */
extern size_t nv_itoa2(intmax_t x, char out[], int base, size_t max);

/**
 * @brief Converts an unsigned integer to ASCII.
 */
extern size_t nv_utoa2(uintmax_t x, char out[], int base, size_t max);

/**
 * @brief Converts a real_t to ASCII.
 *
 * @param precision Number of digits after the decimal point.
 * @param remove_zeroes If true, trailing zeroes are removed.
 * @return number of characters written. (excluding null terminator)
 */
extern size_t nv_ftoa2(real_t x, char out[], int precision, size_t max, bool remove_zeroes);

/**
 * @brief Converts a pointer to ASCII.
 */
extern size_t nv_ptoa2(void* p, char* buf, size_t max);

/**
 * @brief Converts a byte count to ASCII.
 *
 * Supports upgrade modes (e.g. converting 1000 bytes to "1KB", etc.).
 * Up to 1 petabyte is supported. It can go farther but it is undefined behaviour
 * Writes the bytes (using itoa_u) and writes the suffix ( B/KB/MB/GB/PB )
 * @return The number of characters written.
 */
extern size_t nv_btoa2(size_t x, bool upgrade, char* buf, size_t max);

/**
 * @brief Converts a string to an integer.
 */
extern intmax_t nv_atoi(const char s[]);

/**
 * @brief Converts a string to a real_t.
 */
extern real_t nv_atof(const char s[]);

/**
 * @brief Converts a string to a boolean.
 */
extern bool nv_atobool(const char s[]);

static inline char*
nv_itoa(intmax_t x, char out[], int base, size_t max)
{
  nv_itoa2(x, out, base, max);
  return out;
}

static inline char*
nv_itoa_u(uintmax_t x, char out[], int base, size_t max)
{
  nv_utoa2(x, out, base, max);
  return out;
}

static inline char*
nv_ftoa(real_t x, char out[], int precision, size_t max, bool remove_zeroes)
{
  nv_ftoa2(x, out, precision, max, remove_zeroes);
  return out;
}

static inline char*
nv_ptoa(void* p, char* buf, size_t max)
{
  nv_ptoa2(p, buf, max);
  return buf;
}

static inline char*
nv_btoa(size_t x, bool upgrade, char* buf, size_t max)
{
  nv_btoa2(x, upgrade, buf, max);
  return buf;
}

NOVA_HEADER_END

#endif //__NOVA_STRING_CONV_H__
