#include "../include/string.h"

#include "../include/alloc.h"
#include "../include/ctype.h"
#include "../include/stdafx.h"
#include "../include/types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

size_t
nv_memswp(void* ptr1, void* ptr2, size_t nbyte)
{
  /* Compilers love this function. It's always optimized up to SSE/AVX instructions at -O2. */
  /* Trying to work ahead of the compiler just makes it more difficult and slower. */
  uint8_t* ba = ptr1;
  uint8_t* bb = ptr2;

  for (size_t i = 0; i < nbyte; i++)
  {
    uint8_t t = ba[i];
    ba[i]     = bb[i];
    bb[i]     = t;
  }

  return nbyte;
}

static inline size_t
align_up(size_t sz, size_t align)
{
  return (sz + (align - 1)) & ~(align - 1);
}

void*
nv_aligned_alloc(size_t size, size_t alignment)
{
  nv_assert_else_return((alignment & (alignment - 1)) == 0, NULL);
  nv_assert_else_return(size > 0, NULL);

  const size_t total_size = align_up(size + sizeof(void*) + sizeof(size_t), alignment);

  void* const orig = nv_zmalloc(total_size);
  if (!orig) { return NULL; }

  /**
   * We need to store the original pointer and the previous size just behind the
   * aligned block of memory.
   */
  uchar* ptr     = (uchar*)orig + sizeof(void*) + sizeof(size_t);
  uchar* aligned = (uchar*)align_up((size_t)ptr, alignment);

  size_t* store_size = (size_t*)(aligned - sizeof(void*) - sizeof(size_t));
  *store_size        = size;

  void** store_ptr = (void**)(aligned - sizeof(void*));
  *store_ptr       = orig;

  nv_assert_else_return(((uintptr_t)aligned % alignment) == 0, NULL);

  return aligned;
}

void*
nv_aligned_realloc(void* orig, size_t size, size_t alignment)
{
  nv_assert_else_return(size != 0, NULL);
  nv_assert_else_return(alignment != 0, NULL);
  nv_assert_else_return((alignment & (alignment - 1)) == 0, NULL);

  if (orig == NULL) { return nv_aligned_alloc(size, alignment); }

  void* absolute = nv_aligned_get_absolute_ptr(orig);
  if (NV_LIKELY(absolute))
  {
    size_t prev_size = nv_aligned_ptr_get_size(orig);

    if (prev_size == size) { return orig; }

    // return return_block;
    void* new_block = nv_aligned_alloc(size, alignment);
    memmove(new_block, orig, NV_MIN(prev_size, size));
    nv_free(absolute);
    return new_block;
  }

#if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
  __builtin_unreachable();
#endif

  nv_log_error("=== double free %p ===\n", orig);
  abort();

  return NULL;
}

void*
nv_aligned_get_absolute_ptr(void* aligned_ptr)
{
  void** orig_location = (void**)((uchar*)aligned_ptr - sizeof(void*));
  return *orig_location;
}

size_t
nv_aligned_ptr_get_size(void* aligned_ptr)
{
  size_t prev_size = *(size_t*)((uchar*)aligned_ptr - sizeof(void*) - sizeof(size_t));
  return prev_size;
}

void
nv_aligned_free(void* aligned_block)
{
  void** orig_location = (void**)((uchar*)aligned_block - sizeof(void*));

  void* orig = *orig_location;
  nv_free(orig);
}

size_t
nv_strncpy2(char* dst, const char* src, size_t max)
{
  nv_assert_else_return(max > 0, 0);

  size_t slen         = strlen(src);
  size_t original_max = max;

#if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_strncpy)
  __builtin_strncpy(dst, src, max);
  return NV_MIN(slen, max);
#endif

  while (*src && max > 0)
  {
    *dst = *src;
    dst++;
    src++;
    max--;
  }

  *dst = 0;

  return NV_MIN(slen, original_max);
}

size_t
nv_strlcat(char* dst, const char* src, size_t size)
{
  const size_t d = strlen(dst);
  const size_t s = strlen(src);

  char* const odst = dst;

  size_t i = 0;
  while (*src && i < size)
  {
    *dst = *src;
    dst++;
    src++;
  }
  *dst       = 0;
  odst[size] = 0;

  return d + s;
}

size_t
nv_strlcpy(char* dst, const char* src, size_t size)
{
  size_t      o    = strlen(src);
  char* const odst = dst;

  size_t i = 0;
  while (*src && i < size)
  {
    *dst = *src;
    dst++;
    src++;
  }
  *dst       = 0;
  odst[size] = 0;

  return o;
}

char*
nv_strlpcat(char* dst, char* dst_absolute, const char* src, size_t dst_size)
{
  size_t consumed = dst_absolute - dst;
  dst_size -= consumed;

  while (*src)
  {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = 0;

  return dst;
}

char*
nv_strtrim(char* s)
{
  char* begin = NULL;
  char* end   = NULL;
  if (nv_strtrim_c(s, (const char**)&begin, (const char**)&end) == NULL) { return NULL; }

  /* end *may* be a pointer to the NULL terminator but yeah, still works */
  *end = 0;

  return begin;
}

const char*
nv_strtrim_c(const char* s, const char** begin, const char** end)
{
  while (*s && nv_isspace((uchar)*s)) { s++; }

  if (begin) { *begin = (const char*)s; }

  const char* begin_copy = s;

  s += strlen(s);

  while (s > begin_copy && nv_isspace((uchar) * (s - 1))) { s--; }

  if (end) { *end = (char*)s; }
  return begin_copy;
}

char*
nv_strnchr(const char* s, size_t n, int chr)
{
  uchar  c = (uchar)chr;
  size_t i = 0;

  while (*s && i < n)
  {
    if (*s == c) { return (char*)s; }
    s++;
    i++;
  }

  return (c == 0) ? (char*)s : NULL;
}

char*
nv_strchr_n(const char* s, int chr, int n)
{
  while (*s)
  {
    if (*s == chr)
    {
      n--;
      if (n <= 0) { return (char*)s; }
    }
  }
  return NULL;
}

int
nv_strcasencmp(const char* s1, const char* s2, size_t max)
{
  nv_assert_else_return(max > 0, -1);

  size_t i = 0;
  while (*s1 && *s2 && i < max)
  {
    uchar c1 = nv_tolower(*(uchar*)s1);
    uchar c2 = nv_tolower(*(uchar*)s2);
    if (c1 != c2) { return c1 - c2; }

    s1++;
    s2++;
    i++;
  }
  if (i == max) { return 0; }
  return nv_tolower(*(const uchar*)s1) - nv_tolower(*(const uchar*)s2);
}

size_t
nv_strcpy2(char* dst, const char* src)
{
#if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_strcpy)
#endif

  const char* original_dest = dst;
  while (*src)
  {
    *dst = *src;
    src++;
    dst++;
  }
  *dst = 0;
  return dst - original_dest;
}

char*
nv_strtok_r(char* s, const char* delim, char** context)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strtok_r(s, delim, context);
#endif

  if (!s) { s = *context; }
  char* p = NULL;

  s += strspn(s, delim);
  if (!s || *s == 0)
  {
    *context = s;
    return NULL;
  }

  p = s;
  s = strpbrk(s, delim);

  if (!s)
  {
    *context = p + strlen(p); // get pointer to last char
    return p;
  }
  *s       = 0;
  *context = s + 1;
  return p;
}

char*
nv_strreplace(char* s, char to_replace, char replace_with)
{
  while (*s)
  {
    if (*s == to_replace) { *s = replace_with; }
  }
  return s;
}

char*
nv_basename(const char* path)
{
  char*       p         = (char*)path; // shut up C compiler
  const char* backslash = strrchr(path, '/');
  if (backslash != NULL) { return (char*)backslash + 1; }
  return p;
}

char*
nv_strndup(const char* s, size_t n)
{
  // strndup is POSIX only
  // #if !NV_NO_STDLIB_FUNCTIONS
  //   return strndup(s, n);
  // #endif
  size_t slen    = strlen(s);
  size_t dup_len = NV_MIN(slen, n);

  char* new_s = nv_zmalloc(dup_len + 1);
  strlcpy(new_s, s, dup_len + 1);

  return new_s;
}

char*
nv_strexdup(const char* s, size_t size)
{
  char* new_s = nv_zmalloc(size + 1);

  strlcpy(new_s, s, size + 1);

  return new_s;
}

char*
nv_substr(const char* s, size_t start, size_t len)
{
  nv_assert_else_return(len > 0, NULL);

  size_t slen = strlen(s);
  if (start + len > slen) { return NULL; }

  char* sub = nv_zmalloc(len + 1);
  strncpy(sub, s + start, len);
  sub[len] = 0;
  return sub;
}

char*
nv_strrev(char* str)
{
  size_t len = strlen(str);
  for (size_t i = 0; i < len / 2; i++)
  {
    char temp        = str[i];
    str[i]           = str[len - i - 1];
    str[len - i - 1] = temp;
  }
  return str;
}

char*
nv_strnrev(char* str, size_t max)
{
  nv_assert_else_return(max != 0, NULL);

  size_t len = strnlen(str, max);
  if (len == 0) { return str; }

  char* fwrd = str;
  char* back = str + len - 1;

  while (fwrd < back)
  {
    char temp = *fwrd;
    *fwrd     = *back;
    *back     = temp;

    fwrd++;
    back--;
  }

  return str;
}
