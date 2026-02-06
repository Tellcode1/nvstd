#include "../include/string.h"

#include "../include/alloc.h"
#include "../include/chrclass.h"
#include "../include/stdafx.h"
#include "../include/types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void*
nv_memset(void* dst, int to, size_t nbytes)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return memset(dst, to, nbytes);
#endif

  nv_assert_else_return(nbytes != 0, dst);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memset, dst, to, nbytes);

#if defined(__x86_64__) || defined(__i386__)
  //   // No matter what the foof I do, this instruction is always faster for some ungodly reason.
  //   // No, not even mixing stosq for quad word writes.
  //   // >:C
  __asm__ __volatile__("cld\n\t" // clear direction flag to move upwards
                       "rep stosb"
                       :
                       : "D"(dst), "a"((unsigned char)to), "c"(nbytes)
                       : "memory");
#else
  // Pick a god and pray this optimizes to anything good
  char* bytep = dst;
  for (size_t i = 0; i < nbytes; i++) { bytep[i] = to; }
#endif

  return dst;
}

void*
nv_memmove(void* dst, const void* src, size_t nbytes)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return memmove(dst, src, nbytes);
#endif
  nv_assert_else_return(dst != NULL, NULL);
  nv_assert_else_return(src != NULL, NULL);
  nv_assert_else_return(nbytes > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memmove, dst, src, nbytes);

  uchar*       dstp = (uchar*)dst;
  const uchar* srcp = (const uchar*)src;

  if (dstp > srcp && dstp < srcp + nbytes)
  {
    dstp += nbytes;
    srcp += nbytes;

#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__(
        // we first use the std instruction, which reverses direction of increment in d and s
        // then use cld instruction to restore increment state
        "std\n\t" // set direction
        "rep movsb\n\t"
        "cld\n\t" // clear direction
        :
        : "D"(dstp), "S"(srcp), "c"(nbytes)
        : "memory");
#else
    while ((nbytes--) > 0)
    {
      dstp--;
      srcp--;
      *dstp = *srcp;
    }
#endif
  }
  else
  {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("cld\n\t" // clear direction flag
                         "rep movsb;"
                         :
                         : "D"(dstp), "S"(srcp), "c"(nbytes)
                         : "memory");
#else
    for (size_t i = 0; i < nbytes; i++) { dstp[i] = srcp[i]; }
#endif
  }

  return dst;
}

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

    // size_t const new_aligned_size = align_up(size, alignment);
    // void*        new_block        = nv_realloc(absolute, new_aligned_size);

    // uchar* return_block = (uchar*)new_block + sizeof(void*) + sizeof(size_t);

    // *(size_t*)(new_block) = size;
    // void** store_ptr      = (void**)(return_block - sizeof(void*));
    // *store_ptr            = orig;

    // return return_block;
    void* new_block = nv_aligned_alloc(size, alignment);
    nv_memmove(new_block, orig, NV_MIN(prev_size, size));
    nv_free(absolute);
    return new_block;
  }

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
  // if (orig)
  // {
  // *orig_location = NULL;
  nv_free(orig);
  // }
  // else
  // {
  //   nv_log_error("double free %p\n", aligned_block);

  //   /**
  //    * TODO: should we?
  //    */
  //   abort();
  // }
}

void*
nv_memchr(const void* p, int chr, size_t psize)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return memchr(p, chr, psize);
#endif
  nv_assert_else_return(psize > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memchr, p, chr, psize);

  const uchar* read = (const uchar*)p;
  const uchar  chk  = chr;
  for (size_t i = 0; i < psize; i++)
  {
    if (read[i] == chk) { return (void*)(read + i); }
  }
  return NULL;
}

int
nv_memcmp(const void* _p1, const void* _p2, size_t max)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return memcmp(_p1, _p2, max);
#endif
  nv_assert_else_return(max > 0, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memcmp, _p1, _p2, max);

  const uchar* p1 = (const uchar*)_p1;
  const uchar* p2 = (const uchar*)_p2;

  while ((max--) != 0)
  {
    if (*p1 != *p2) { return *p1 - *p2; }
    p1++;
    p2++;
  }

  return 0;
}

size_t
nv_strncpy2(char* dst, const char* src, size_t max)
{
  nv_assert_else_return(max > 0, 0);

  size_t slen         = nv_strlen(src);
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

char*
nv_strcpy(char* dst, const char* src)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strcpy(dst, src);
#endif
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcpy, dst, src); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)

  char* const dst_orig = dst;

  while (*src)
  {
    *dst = *src;
    dst++;
    src++;
  }

  *dst = 0;

  return dst_orig;
}

char*
nv_stpcpy(char* dst, char* src)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(stpcpy, dst, src);

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
nv_strncpy(char* dst, const char* src, size_t max)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strncpy(dst, src, max);
#endif
  nv_assert_else_return(max > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strncpy, dst, src, max);

  (void)nv_strncpy2(dst, src, max);
  return dst;
}

char*
nv_strcat(char* dst, const char* src)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strcat(dst, src);
#endif
  char* end = dst + nv_strlen(dst);

  while (*src)
  {
    *end = *src;
    end++;
    src++;
  }
  *end = 0;

  return dst;
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
nv_strncat(char* dst, const char* src, size_t max)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strncat(dst, src, max);
#endif
  nv_assert_else_return(max > 0, NULL);

  char* original_dest = dst;

  dst = nv_strchr(dst, '\0');
  if (!dst) { return NULL; }

  size_t i = 0;
  while (*src && i < max)
  {
    *dst = *src;
    i++;
    src++;
    dst++;
  }
  *dst = 0;
  return original_dest;
}

size_t
nv_strcat_max(char* dst, const char* src, size_t dest_size)
{
  /* Optionally, memset the remaining part of dst to 0? */

  if (dest_size == 0) { return nv_strlen(src); }

  size_t dst_len = 0;
  while (dst_len < dest_size && dst[dst_len]) { dst_len++; }

  if (dst_len == dest_size) { return dst_len + nv_strlen(src); }

  size_t copy_len = dest_size - dst_len - 1;
  size_t i        = 0;
  while (i < copy_len && src[i])
  {
    dst[dst_len + i] = src[i];
    i++;
  }

  dst[dst_len + i] = 0;

  return dst_len + nv_strlen(src);
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

  s += nv_strlen(s);

  while (s > begin_copy && nv_isspace((uchar) * (s - 1))) { s--; }

  if (end) { *end = (char*)s; }
  return begin_copy;
}

int
nv_strcmp(const char* s1, const char* s2)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcmp, s1, s2);

  while (*s1 && *s2 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }

  return (uchar)*s1 - (uchar)*s2;
}

char*
nv_strchr(const char* s, int chr)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strchr(s, chr);
#endif
  uchar c = (uchar)chr;

  while (*s)
  {
    if (*s == c) { return (char*)s; }
    s++;
  }

  return (c == 0) ? (char*)s : NULL;
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

char*
nv_strrchr(const char* s, int chr)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strrchr(s, chr);
#endif
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strrchr, s, chr);

  const char* beg = s;
  char*       end = (char*)s + nv_strlen(s) - 1;

  if (chr == 0) { return (char*)s + 1; }

  while (end > beg)
  {
    end--;
    if (*end == (char)chr) { return (char*)end; }
  }
  return NULL;
}

int
nv_strncmp(const char* s1, const char* s2, size_t max)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strncmp(s1, s2, max);
#endif
  nv_assert_else_return(max > 0, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strncmp, s1, s2, max);
  size_t i = 0;
  while (*s1 && *s2 && (*s1 == *s2) && i < max)
  {
    s1++;
    s2++;
    i++;
  }
  return (i == max) ? 0 : (*(const uchar*)s1 - *(const uchar*)s2);
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

int
nv_strcasecmp(const char* s1, const char* s2)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strcasecmp(s1, s2);
#endif
  while (*s1 && *s2)
  {
    uchar c1 = nv_tolower(*(uchar*)s1);
    uchar c2 = nv_tolower(*(uchar*)s2);
    if (c1 != c2) { return c1 - c2; }
    s1++;
    s2++;
  }
  return nv_tolower(*(uchar*)s1) - nv_tolower(*(uchar*)s2);
}

size_t
nv_strlen(const char* s)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strlen(s);
#endif
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strlen, s);

  const char* start = s;
  while (*s) { s++; }
  return s - start;
}

size_t
nv_strnlen(const char* s, size_t max)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strnlen(s, max);
#endif
  nv_assert_else_return(max > 0, 0);

  const char* s_orig = s;
  while (*s && max > 0)
  {
    s++;
    max--;
  }

  return s - s_orig;
}

char*
nv_strstr(const char* s, const char* sub)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strstr(s, sub);
#endif
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strstr, s, sub);

  for (; *s; s++)
  {
    const char* str_it = s;
    const char* sub_it = sub;

    while (*str_it && *sub_it && *str_it == *sub_it)
    {
      str_it++;
      sub_it++;
    }

    /* If we reach the NULL terminator of the substring, it exists in str. */
    if (!*sub_it) { return (char*)s; }
  }

  return NULL;
}

char*
nv_strlcpy(char* dst, const char* src, size_t dst_size)
{
  if (dst_size == 0) { return dst; }

  char* dst_orig = dst;

  size_t i = 0;
  while (*src && i < dst_size - 1)
  {
    *dst = *src;
    dst++;
    src++;
    i++;
  }

  *dst = 0;

  return dst_orig;
}

size_t
nv_strcpy2(char* dst, const char* src)
{
#if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_strcpy)
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strlen, __builtin_strcpy(dst, src)); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
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

size_t
nv_strspn(const char* s, const char* accept)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strspn(s, accept);
#endif

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strspn, s, accept);
  size_t i = 0;
  while (*s && *accept && *s == *accept)
  {
    i++;
    s++;
    accept++;
  }
  return i;
}

size_t
nv_strcspn(const char* s, const char* reject)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strcspn(s, reject);
#endif

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcspn, s, reject);

  const char* base = reject;
  size_t      i    = 0;

  while (*s)
  {
    const char* j = base;
    while (*j && *j != *s) { j++; }
    if (*j) { break; }
    i++;
    s++;
  }
  return i;
}

char*
nv_strpbrk(const char* s1, const char* s2)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strpbrk(s1, s2);
#endif

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strpbrk, s1, s2);

  while (*s1)
  {
    const char* j = s2;
    while (*j)
    {
      if (*j == *s1) { return (char*)s1; }
      j++;
    }
    s1++;
  }
  return NULL;
}

char*
nv_strtok_r(char* s, const char* delim, char** context)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strtok_r(s, delim, context);
#endif

  if (!s) { s = *context; }
  char* p = NULL;

  s += nv_strspn(s, delim);
  if (!s || *s == 0)
  {
    *context = s;
    return NULL;
  }

  p = s;
  s = nv_strpbrk(s, delim);

  if (!s)
  {
    *context = p + nv_strlen(p); // get pointer to last char
    return p;
  }
  *s       = 0;
  *context = s + 1;
  return p;
}

char*
nv_strtok(char* s, const char* delim)
{
  static char* __nv_strtok_ctx = NULL;
  if (s) __nv_strtok_ctx = NULL; // first call? set context to NULL.
  return nv_strtok_r(s, delim, &__nv_strtok_ctx);
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
  char* p         = (char*)path; // shut up C compiler
  char* backslash = nv_strrchr(path, '/');
  if (backslash != NULL) { return backslash + 1; }
  return p;
}

char*
nv_strdup(const char* s)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strdup(s);
#endif
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strdup, s);

  size_t slen  = nv_strlen(s);
  char*  new_s = nv_zmalloc(slen + 1);
  nv_strlcpy(new_s, s, slen + 1);

  return new_s;
}

char*
nv_strndup(const char* s, size_t n)
{
#if !NV_NO_STDLIB_FUNCTIONS
  return strndup(s, n);
#endif
  size_t slen    = nv_strlen(s);
  size_t dup_len = NV_MIN(slen, n);

  char* new_s = nv_zmalloc(dup_len + 1);
  nv_strlcpy(new_s, s, dup_len + 1);

  return new_s;
}

char*
nv_strexdup(const char* s, size_t size)
{
  char* new_s = nv_zmalloc(size + 1);

  nv_strlcpy(new_s, s, size + 1);

  return new_s;
}

char*
nv_substr(const char* s, size_t start, size_t len)
{
  nv_assert_else_return(len > 0, NULL);

  size_t slen = nv_strlen(s);
  if (start + len > slen) { return NULL; }

  char* sub = nv_zmalloc(len + 1);
  nv_strncpy(sub, s + start, len);
  sub[len] = 0;
  return sub;
}

char*
nv_strrev(char* str)
{
  size_t len = nv_strlen(str);
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

  size_t len = nv_strnlen(str, max);
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
