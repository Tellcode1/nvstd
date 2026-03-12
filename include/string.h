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

/* An overlayer of standard string.h */
/* This library though, contains many differences than the standard. Like memcpy has been abolished in favour of memmove. */

#ifndef NV_STD_STRING_H
#define NV_STD_STRING_H

// implementation: core.c

#include "attributes.h"
#include "stdafx.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

NOVA_HEADER_START

/**
 * Do not let nvstd serve as passthrough to Cstd.
 */
#ifndef NV_NO_STDLIB_FUNCTIONS
#  define NV_NO_STDLIB_FUNCTIONS false
#endif

#define nv_alloc_struct(struc) ((struc*)nv_zmalloc(sizeof(struc)))
#define nv_zero_structp(struc) (memset(struc, 0, sizeof(*(struc))))

#define nv_bzero(dst, sz) memset((dst), 0, (sz))

/**
 * Swap nbyte of memory between ptr1 and ptr2.
 * Returns the number of bytes swapped. Always nbyte unless you did something catostrophically wrong.
 */
size_t nv_memswp(void* ptr1, void* ptr2, size_t nbyte) NOVA_ATTR_NONNULL(1, 2);

/**
 * Free aligned block of memory.
 */
void nv_aligned_free(void* aligned_block);

/**
 * Get an aligned block of memory
 * The memory must be freed by aligned_free
 * WARNING: Only supports power of two alignments
 * https://tabreztalks.medium.com/memory-aligned-malloc-6c7b562d58d0
 */
void* nv_aligned_alloc(size_t sz, size_t alignment) NOVA_ATTR_MALLOC(nv_aligned_free, 1) NOVA_ATTR_ALLOC_ALIGN(2);

void* nv_aligned_realloc(void* orig, size_t size, size_t alignment);

/**
 * Get the actual pointer allocated by nv_aligned_alloc()\*realloc()
 * WARNING: If the block had been freed, this function returns NULL
 */
void* nv_aligned_get_absolute_ptr(void* aligned_ptr);

/**
 * Get the size allocated by nv_aligned_alloc()\*realloc
 */
size_t nv_aligned_ptr_get_size(void* aligned_ptr);

char* nv_strlpcat(char* dst, char* dst_absolute, const char* src, size_t dst_size) NOVA_ATTR_NONNULL(1, 2, 3);

/**
 *  @return the number of characters copied.
 */
size_t nv_strncpy2(char* dst, const char* src, size_t max) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

/**
 * @brief Remove leading and trailing whitspaces from string
 * @return s if success, NULL if not.
 */
char* nv_strtrim(char* s) NOVA_ATTR_NONNULL(1);

/**
 * @brief A constant version of strtrim. begin will contain the first non whitespace character and end will contain the last non space char.
 * @param begin may be NULL
 * @param end may be NULL
 * @return s if success, NULL if not.
 */
const char* nv_strtrim_c(const char* s, const char** begin, const char** end) NOVA_ATTR_NONNULL(1);

/**
 *  @brief compare two strings case insensitively
 *  does not care whether s1 or s2 has 'a' or 'A', they're the same thing
 */
int nv_strcasencmp(const char* s1, const char* s2, size_t max) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 * @brief Search for chr in n characters of s.
 */
char* nv_strnchr(const char* s, size_t n, int chr);

/**
 *  @brief Find the n-th occurence of a character in a string.
 *  @return NULL if chr is not in s or there is no n-th occurence of chr.
 */
char* nv_strchr_n(const char* s, int chr, int n) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @brief copy two strings, ensuring null termination
 *  WARNING: use strlcpy, noob.
 */
size_t nv_strcpy2(char* dst, const char* src) NOVA_ATTR_NONNULL(1, 2);

/**
 * Concatenate src to dst, ensuring it does not go over size bytes.
 */
size_t nv_strlcat(char* dst, const char* src, size_t size);

/**
 * Copy src to dst, ensuring it does not run over size bytes.
 * Ensures NULL termination.
 */
size_t nv_strlcpy(char* dst, const char* src, size_t size);

/**
 *  warning: modifies s directly.
 *  @brief splits 's' by a delimiter
 *  @brief you should pass null instead of the string for chaining calls
 *  so if you have obama-is-good-gamer,
 *  it'll first return to you 'obama', then 'is' them 'good' then 'gamer'
 *  like:
 *  char buf[] = "obama-care-gaming";
 *  char *ptr = strtok(buf, '-'); * this will return 'obama'
 *  while (ptr != null) {
 *    ptr = strtok(null, '-'); * this will first return care, and in second iteration, return gaming
 *  }
 *  context must be declared typically on the stack that the string is declared, as a char *
 *  You must pass the address of the char * to this function.
 */
char* nv_strtok_r(char* s, const char* delim, char** context) NOVA_ATTR_NONNULL(2, 3);

/**
 * @brief Replace every occurence of to_replace in string with replace_with
 * @return Pointer to NULL terminator of s
 */
char* nv_strreplace(char* s, char to_replace, char replace_with) NOVA_ATTR_NONNULL(1);

/**
 *  @return the name of the file
 *  basically, ../../pdf/nuclearlaunchcodes.pdf would give you nuclearlaunchcodes.pdf in return.
 */
char* nv_basename(const char* path) NOVA_ATTR_NONNULL(1);

/**
 * @brief Duplicate no more than n characters of a string 's'
 * Returned string will be nv_zmalloc()'d and must be freed.
 */
char* nv_strndup(const char* s, size_t n) NOVA_ATTR_NONNULL(1);

/**
 *  @brief duplicate a string (using nv_zmalloc) with extra space equal to 'len'. If 'len' is less than required, the function fails and will return NULL.
 *  Note that the NULL terminator is excluded from 'len', i.e. one extra byte will be allocated at the end for the terminator.
 */
char* nv_strexdup(const char* s, size_t len) NOVA_ATTR_NONNULL(1);

/**
 *  @brief make a substring of the string s
 *  the returned string is malloc'd and must be freed by the caller.
 */
char* nv_substr(const char* s, size_t start, size_t len) NOVA_ATTR_NONNULL(1);

/**
 * @brief Reverse a string, in place, no copying to a buffer or whatever.
 */
char* nv_strrev(char* str) NOVA_ATTR_NONNULL(1);

/**
 * @brief Reverse a string, in place, copying no more than max chars.
 *  Note that the NULL terminator is not touched, only the characters before it.
 */
char* nv_strnrev(char* str, size_t max) NOVA_ATTR_NONNULL(1);

NOVA_HEADER_END

#endif // NV_STD_STRING_H
