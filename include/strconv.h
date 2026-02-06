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

#ifndef NV_STD_STRCONV_H
#define NV_STD_STRCONV_H

#include "stdafx.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

NOVA_HEADER_START

#ifndef NOVA_MAX_IGNORE
#  define NOVA_MAX_IGNORE SIZE_MAX
#endif

#ifndef NOVA_SEPERATOR_CHAR
#  define NOVA_SEPERATOR_CHAR ','
#endif

/**
 * @brief Converts an integer to ASCII.
 *
 * @param max Maximum number of characters to write.
 * @return The number of characters written (excluding null terminator).
 */
size_t nv_itoa2(intmax_t num, char out[], int base, size_t max, bool add_commas);

/**
 * @brief Converts an unsigned integer to ASCII.
 */
size_t nv_utoa2(uintmax_t num, char out[], int base, size_t max, bool add_commas);

/**
 * @brief Converts a double to ASCII.
 *
 * @param precision Number of digits after the decimal point.
 * @param remove_zeroes If true, trailing zeroes are removed.
 * @return number of characters written. (excluding null terminator)
 */
size_t nv_ftoa2(double num, char out[], int precision, size_t max, bool remove_zeros);

/**
 * @brief Convert a long double to ASCII.
 *
 * @param precision Number of digits after the decimal point.
 * @param remove_zeroes If true, trailing zeroes are removed.
 * @return number of characters written. (excluding null terminator)
 */
size_t nv_fltoa2(long double num, char out[], int precision, size_t max, bool remove_zeros);

/**
 * @brief Converts a pointer to ASCII.
 */
size_t nv_ptoa2(void* ptr, char out[], size_t max);

/**
 * @brief Converts a byte count to ASCII.
 *
 * Supports upgrade modes (e.g. converting 1000 bytes to "1KB", etc.).
 * Up to 1 petabyte is supported. It can go farther but it is undefined behaviour
 * Writes the bytes (using utoa) and writes the suffix ( B/KB/MB/GB/PB )
 * @return The number of characters written.
 */
size_t nv_btoa2(size_t num_bytes, bool upgrade, char out[], size_t max);

/**
 * @brief Converts a string to an integer and if not NULL, store a pointer
 * to the last successfully converted character in endptr
 * If an error occurs, endptr will be set to NULL and INTMAX_MAX will be returned
 */
intmax_t nv_atoi2(const char in_string[], size_t max, char** endptr);

/**
 * @brief Converts a string to a double and if not NULL, store a pointer
 * to the last successfully converted character in endptr
 * If an error occurs, endptr will be set to NULL and DBL_MAX will be returned
 */
double nv_atof2(const char in_string[], size_t max, char** endptr);

/**
 * @brief Converts a string to an integer.
 */
static inline intmax_t
nv_atoi(const char in_string[])
{
  return nv_atoi2(in_string, SIZE_MAX, NULL);
}
/**
 * @brief Converts a string to a double.
 */
static inline double
nv_atof(const char in_string[])
{
  return nv_atof2(in_string, SIZE_MAX, NULL);
}

/**
 * @brief Converts a string to a boolean.
 */
bool nv_atobool(const char in_string[], size_t max);

static inline char*
nv_itoa(int x, char out[], int base)
{
  nv_itoa2((intmax_t)x, out, base, SIZE_MAX, false);
  return out;
}

static inline char*
nv_utoa(unsigned x, char out[], int base)
{
  nv_utoa2((uintmax_t)x, out, base, SIZE_MAX, false);
  return out;
}

static inline char*
nv_ftoa(double x, char out[], int precision)
{
  nv_ftoa2(x, out, precision, SIZE_MAX, false);
  return out;
}

static inline char*
nv_ptoa(void* p, char* buf)
{
  nv_ptoa2(p, buf, SIZE_MAX);
  return buf;
}

static inline char*
nv_btoa(size_t x, char* buf)
{
  nv_btoa2(x, true, buf, SIZE_MAX);
  return buf;
}

NOVA_HEADER_END

#endif // NV_STD_STRCONV_H
