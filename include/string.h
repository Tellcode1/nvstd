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

// Whether to use the __builtin functions provided by GNU C
// They are generally faster, so no reason not to?
// Can only work if the program is compiling with GNU C
#ifndef NOVA_STRING_USE_BUILTIN
#  define NOVA_STRING_USE_BUILTIN false
#endif

/**
 * Do not let nvstd serve as passthrough to Cstd.
 */
#ifndef NV_NO_STDLIB_FUNCTIONS
#  define NV_NO_STDLIB_FUNCTIONS false
#endif

#define nv_alloc_struct(struc) ((struc*)nv_calloc(sizeof(struc)))
#define nv_zero_structp(struc) (nv_memset(struc, 0, sizeof(*(struc))))

#if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin)
#  define NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(fn, ...)                                                                                                               \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (__has_builtin(__builtin_##fn)) { return __builtin_##fn(__VA_ARGS__); }                                                                                              \
    } while (0);
#else
#  define NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(fn, ...)
#endif

/**
 * @brief sets 'sz' bytes of 'dst' to 'to'
 * Can not fail.
 */
void* nv_memset(void* dst, int to, size_t dst_size) NOVA_ATTR_RETURNS_NONNULL NOVA_ATTR_NONNULL(1) NOVA_ATTR_WRITE_ONLY(1);

#define nv_bzero(dst, sz) nv_memset((dst), 0, (sz))

/**
 *  @brief copy memory from src to dst
 */
void* nv_memmove(void* dst, const void* src, size_t sz) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1) NOVA_ATTR_READ_ONLY(2);

/**
 * memcpy is marginally faster than memmove, but also has issues on overlapping regions of memory
 * So, we ditch memcpy for memmove for the sake of safety.
 */
#define nv_memcpy nv_memmove

/**
 *  @return a pointer to the first occurance of chr in p
 *  searches at most psize bytes of p
 */
void* nv_memchr(const void* ptr, int chr, size_t psize) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @return non zero if p1 is not equal to p2
 */
int nv_memcmp(const void* ptr1, const void* ptr2, size_t max) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

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

void nv_aligned_free(void* aligned_block);

/**
 *  get the size of the string
 *  the size is determined by the position of the null terminator.
 */
size_t nv_strlen(const char* s) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 * @brief Get the length of the string, reading no more than max characters
 * @return size_t The length of the string
 */
size_t nv_strnlen(const char* s, size_t max) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 * Copy from src to dst ensuring NULL termination using the length of the dst buffer.
 *  Will copy not more than dst_size - 1 characters from source.
 */
char* nv_strlcpy(char* dst, const char* src, size_t dst_size) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

/**
 *  copy from src to dst, stopping once it hits the null terminator in src
 */
char* nv_strcpy(char* dst, const char* src) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

/**
 *  copies min(strlen(dst), min(strlen(src), max)) chars.
 *  ie. the least of the lengths and the max chars
 */
char* nv_strncpy(char* dst, const char* src, size_t max) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

/* https://manpages.debian.org/testing/linux-manual-4.8/strscpy.9.en.html */
#define nv_strscpy nv_strncpy

/**
 *  concatenate src to dst
 */
char* nv_strcat(char* dst, const char* src) NOVA_ATTR_NONNULL(1, 2);

/**
 *  concatenate src to dst while copying no more than max characters.
 */
char* nv_strncat(char* dst, const char* src, size_t max) NOVA_ATTR_NONNULL(1, 2);

/**
 *  copy from src to dst while ensuring that there are no more than dest_size characters in dst
 *  if nv_strlen(dst) > dest_size, this function will return and do nothing.
 *  @return the length of the string it TRIED to create.
 */
size_t nv_strcat_max(char* dst, const char* src, size_t dest_size) NOVA_ATTR_NONNULL(1, 2);

/* nv_strcat_max is functionally equivalent to strlcat */
#define nv_strlcat nv_strcat_max

char* nv_strlpcat(char* dst, char* dst_absolute, const char* src, size_t dst_size) NOVA_ATTR_NONNULL(1, 2, 3);

/**
 *  @return the number of characters copied.
 */
size_t nv_strncpy2(char* dst, const char* src, size_t max) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

/**
 * @brief Copy characters from src to dst
 * @return The pointer to the NULL terminator of dst
 */
char* nv_stpcpy(char* dst, char* src) NOVA_ATTR_NONNULL(1, 2) NOVA_ATTR_WRITE_ONLY(1);

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
 *  @brief compare two strings, stopping at either s1 or s2's null terminator or at max.
 *  it will stop when it reaches the null terminator, no segv
 */
int nv_strncmp(const char* s1, const char* s2, size_t max) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 *  @brief compare two strings case insensitively
 *  does not care whether s1 or s2 has 'a' or 'A', they're the same thing
 */
int nv_strcasencmp(const char* s1, const char* s2, size_t max) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 *  @brief case insensitively compare no more than max chars
 */
int nv_strcasecmp(const char* s1, const char* s2) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 *  @brief find the first occurence of a character in a string
 *  @return NULL if chr is not in s
 */
char* nv_strchr(const char* s, int chr) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @brief find the first occurence of a character in a string within n characters
 *  @return NULL if chr is not in s within n characters
 */
char* nv_strnchr(const char* s, size_t n, int chr) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @brief Find the n-th occurence of a character in a string.
 *  @return NULL if chr is not in s or there is no n-th occurence of chr.
 */
char* nv_strchr_n(const char* s, int chr, int n) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @brief find the last occurence of a character in a string
 *  @sa use nv_strstr if you want to find earliest occurence of a string in a string
 */
char* nv_strrchr(const char* s, int chr) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  strchr() but string
 *  @brief find the earlier occurence of a (sub)string in a string.
 *  eg. for s baller and sub ll
 *  @return a pointer to the first l
 */
char* nv_strstr(const char* s, const char* sub) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1);

/**
 *  @brief copy two strings, ensuring null termination
 *  WARNING: use strlcpy, noob.
 */
size_t nv_strcpy2(char* dst, const char* src) NOVA_ATTR_NONNULL(1, 2);

/**
 *  @return 0 when they are equal,
 *  positive number if the first non equal character of s1 is greater than lc of s2
 *  negative number if the first non equal character of s1 is less than lc of s2
 */
int nv_strcmp(const char* s1, const char* s2) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 *  @brief return the number of characters after which 's' contains a character in 'reject'
 *  for example,
 *  s is balling, reject is hello
 *  so, strcspn will return 2 because after b and a,
 *  l is in both s and reject.
 */
size_t nv_strcspn(const char* s, const char* reject) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 *  @brief return the number of characters after which s does not contain a character in accept
 *  ie. the number of similar characters they have.
 *  for example,
 *  s is balling and accept is ball
 *  strspn will return 4 because ball is found in both s and accept and it is of 4 characters.
 */
size_t nv_strspn(const char* s, const char* accept) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

/**
 * @brief return a pointer ot the first character in s1 that is also in s2.
 *
 */
char* nv_strpbrk(const char* s1, const char* s2) NOVA_ATTR_PURE NOVA_ATTR_NONNULL(1, 2);

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
char* nv_strtok(char* s, const char* delim) NOVA_ATTR_NONNULL(2);

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
 *  @brief duplicate a string (using nv_zmalloc)
 *  and return it
 */
char* nv_strdup(const char* s) NOVA_ATTR_NONNULL(1);

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
