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

/* An overlayer of standard string.h */
/* This library though, contains many differences than the standard. Like memcpy has been abolished in favour of memmove. */

#ifndef __NOVA_STRING_H__
#define __NOVA_STRING_H__

// implementation: core.c

#include "stdafx.h"
#include <stddef.h>
#include <stdint.h>

NOVA_HEADER_START

// Whether to use the __builtin functions provided by GNU C
// They are generally faster, so no reason not to?
// Can only work if the program is compiling with GNU C
#ifndef NOVA_STRING_USE_BUILTIN
#  define NOVA_STRING_USE_BUILTIN false
#endif

#if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin)
#  define NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(fn, ...)                                                                                                               \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (__has_builtin(__builtin_##fn))                                                                                                                                      \
      {                                                                                                                                                                       \
        return __builtin_##fn(__VA_ARGS__);                                                                                                                                   \
      }                                                                                                                                                                       \
    } while (0);
#else
#  define NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(fn, ...)
#endif

/**
 *  @brief sets 'sz' bytes of 'dst' to 'to'
 *  @return null on error and dst for success
 */
extern void* nv_memset(void* dst, char to, size_t dst_size);

#define nv_bzero(dst, sz) nv_memset((dst), 0, (sz))

/**
 *  @brief copy memory from src to dst
 */
extern void* nv_memmove(void* dst, const void* src, size_t sz);

/**
 * memcpy is marginally faster than memmove, but also has issues on overlapping regions of memory
 * So, we ditch memcpy for memmove for the sake of safety.
 */
#define nv_memcpy(...) nv_memmove(__VA_ARGS__)

/**
 *  @return a pointer to the first occurance of chr in p
 *  searches at most psize bytes of p
 */
extern void* nv_memchr(const void* ptr, int chr, size_t psize);

/**
 *  @return non zero if p1 is not equal to p2
 */
extern int nv_memcmp(const void* ptr1, const void* ptr2, size_t max);

// god is dead and i killed him

/* Dude, if using calloc is really slowing down your program, your program is the fault. */
#define nv_malloc(...) (nv_calloc(__VA_ARGS__))

extern void* nv_calloc(size_t sz);

extern void* nv_realloc(void* prevblock, size_t new_sz);

extern void nv_free(void* block);

/**
 *  uses zlib to compress and decompress the buffer
 *  this works just as you'd expect on images
 *  @param output should be an allocation of output_size (or bigger)
 */
extern int nv_bufcompress(const void* NV_RESTRICT input, size_t input_size, void* NV_RESTRICT output, size_t* NV_RESTRICT output_size);

/**
 *  @return The number of bytes decompressed
 *  @param o_buf must be allocated with atleast o_buz_sz bytes of memory
 */
extern size_t nv_bufdecompress(const void* NV_RESTRICT compressed_data, size_t compressed_size, void* NV_RESTRICT o_buf, size_t o_buf_sz);

/**
 *  get the size of the string
 *  the size is determined by the position of the null terminator.
 */
extern size_t nv_strlen(const char* s);

/**
 * @brief Get the length of the string, reading no more than max characters
 * @return size_t The length of the string
 */
extern size_t nv_strnlen(const char* s, size_t max);

/**
 * Copy from src to dst ensuring NULL termination using the length of the dst buffer.
 *  Will copy not more than dst_size - 1 characters from source.
 */
extern char* nv_strlcpy(char* dst, const char* src, size_t dst_size);

/**
 *  copy from src to dst, stopping once it hits the null terminator in src
 */
extern char* nv_strcpy(char* dst, const char* src);

/**
 *  copies min(strlen(dst), min(strlen(src), max)) chars.
 *  ie. the least of the lengths and the max chars
 */
extern char* nv_strncpy(char* dst, const char* src, size_t max);

/* https://manpages.debian.org/testing/linux-manual-4.8/strscpy.9.en.html */
#define nv_strscpy(...) (nv_strncpy(__VA_ARGS__))

/**
 *  concatenate src to dst
 */
extern char* nv_strcat(char* dst, const char* src);

/**
 *  concatenate src to dst while copying no more than max characters.
 */
extern char* nv_strncat(char* dst, const char* src, size_t max);

/**
 *  copy from src to dst while ensuring that there are no more than dest_size characters in dst
 *  if nv_strlen(dst) > dest_size, this function will return and do nothing.
 *  @return the length of the string it TRIED to create.
 */
extern size_t nv_strcat_max(char* dst, const char* src, size_t dest_size);

/* nv_strcat_max is functionally equivalent to strlcat */
#define nv_strlcat(...) (nv_strcat_max(__VA_ARGS__))

/**
 *  @return the number of characters copied.
 */
extern size_t nv_strncpy2(char* dst, const char* src, size_t max);

/**
 * @brief Copy characters from src to dst
 * @return The pointer to the NULL terminator of dst
 */
extern char* nv_stpcpy(char* dst, char* src);

/**
 * @brief Remove leading and trailing whitspaces from string
 * @return s if success, NULL if not.
 */
extern char* nv_strtrim(char* s);

/**
 * @brief A constant version of strtrim. begin will contain the first non whitespace character and end will contain the last non space char.
 * @return s if success, NULL if not.
 */
extern const char* nv_strtrim_c(const char* s, const char** begin, const char** end);

/**
 *  @brief compare two strings, stopping at either s1 or s2's null terminator or at max.
 *  it will stop when it reaches the null terminator, no segv
 */
extern int nv_strncmp(const char* s1, const char* s2, size_t max);

/**
 *  @brief compare two strings case insensitively
 *  does not care whether s1 or s2 has 'a' or 'A', they're the same thing
 */
extern int nv_strcasencmp(const char* s1, const char* s2, size_t max);

/**
 *  @brief case insensitively compare no more than max chars
 */
extern int nv_strcasecmp(const char* s1, const char* s2);

/**
 *  @brief find the first occurence of a character in a string
 *  @return NULL if chr is not in s or there is no n-th occurence of chr.
 */
extern char* nv_strchr(const char* s, int chr);

/**
 *  @brief Find the n-th occurence of a character in a string.
 *  @return NULL if chr is not in s or there is no n-th occurence of chr.
 */
extern char* nv_strchr_n(const char* s, int chr, int n);

/**
 *  @brief find the last occurence of a character in a string
 *  @sa use nv_strstr if you want to find earliest occurence of a string in a string
 */
extern char* nv_strrchr(const char* s, int chr);

/**
 *  strchr() but string
 *  @brief find the earlier occurence of a (sub)string in a string.
 *  eg. for s baller and sub ll
 *  @return a pointer to the first l
 */
extern char* nv_strstr(const char* s, const char* sub);

/**
 *  @brief copy two strings, ensuring null termination
 *  WARNING: use strlcpy, noob.
 */
extern size_t nv_strcpy2(char* dst, const char* src);

/**
 *  @return 0 when they are equal,
 *  positive number if the lc (last character) of s1 is greater than lc of s2
 *  negative number if the lc (last character) of s1 is less than lc of s2
 */
extern int nv_strcmp(const char* s1, const char* s2);

/**
 *  @brief return the number of characters after which 's' contains a character in 'reject'
 *  for example,
 *  s is balling, reject is hello
 *  so, strcspn will return 2 because after b and a,
 *  l is in both s and reject.
 */
extern size_t nv_strcspn(const char* s, const char* reject);

/**
 *  @brief return the number of characters after which s does not contain a character in accept
 *  ie. the number of similar characters they have.
 *  for example,
 *  s is balling and accept is ball
 *  strspn will return 4 because ball is found in both s and accept and it is of 4 characters.
 */
extern size_t nv_strspn(const char* s, const char* accept);

/**
 * @brief return a pointer ot the first character in s1 that is also in s2.
 *
 */
extern char* nv_strpbrk(const char* s1, const char* s2);

/**
 *  warning: modifies s directly.
 *  @brief splits 's' by a delimiter
 *  so if you have obama-is-good-gamer,
 *  it'll first return to you 'obama', then 'is' them 'good' then 'gamer'
 *  you should pass null instead of the string for chaining calls
 *  like:
 *  char buf[] = "obama-care-gaming";
 *  char *ptr = strtok(buf, '-'); * this will return 'obama'
 *  while (ptr != null) {
 *    ptr = strtok(null, '-'); * this will first return care, and in second iteration, return gaming
 *  }
 *  context must be declared typically on the stack that the string is declared, as a char *
 *  You must pass the address of the char * to this function.
 *  INFO: This is more in line with strtok_r. We use it because it comes at literally no cost to the user.
 */
extern char* nv_strtok(char* s, const char* delim, char** context);

/* As nv_strtok is functionally equivalent to strtok_r, we just define to allow for idiots who don't want to open this header to use it. */
#define nv_strtok_r(...) (nv_strtok(__VA_ARGS__))

/**
 * @brief Replace every occurence of to_replace in string with replace_with
 * @return Pointer to NULL terminator of s
 */
extern char* nv_strreplace(char* s, char to_replace, char replace_with);

/**
 *  @brief @return the name of the file
 *  basically, ../../pdf/nuclearlaunchcodes.pdf would give you nuclearlaunchcodes.pdf in return.
 */
extern char* nv_basename(const char* path);

/**
 *  @brief duplicate a string (using nv_malloc)
 *  and return it
 */
extern char* nv_strdup(const char* s);

/**
 *  @brief make a substring of the string s
 *  the returned string is malloc'd and must be freed by the caller.
 */
extern char* nv_substr(const char* s, size_t start, size_t len);

/**
 * @brief Reverse a string, in place, no copying to a buffer or whatever.
 */
extern char* nv_strrev(char* str);

/**
 * @brief Reverse a string, in place, copying no more than max chars.
 *  Note that the NULL terminator is not touched, only the characters before it.
 */
extern char* nv_strnrev(char* str, size_t max);

NOVA_HEADER_END

#endif //__NOVA_STRING_H__
