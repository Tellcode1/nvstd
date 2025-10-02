#include "../include/string.h"
#include "../include/alloc.h"
#include "../include/chrclass.h"
#include "../include/stdafx.h"
#include "../include/types.h"
#include <stdint.h>
#include <stdlib.h>

void*
nv_memset(void* dst, char to, size_t sz)
{
  nv_assert(dst != NULL);
  nv_assert(sz != 0);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memset, dst, to, sz);

  uchar* byte_write = (uchar*)dst;
  while ((sz--) > 0)
  {
    *byte_write = to;
    byte_write++;
  }

  return dst;
}

void*
nv_memmove(void* dst, const void* src, size_t sz)
{
  nv_assert_else_return(sz > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memmove, dst, src, sz);

  uchar*       d = (uchar*)dst;
  const uchar* s = (const uchar*)src;

  if (d > s && d < s + sz)
  {
    d += sz;
    s += sz;
    while ((sz--) > 0)
    {
      d--;
      s--;
      *d = *s;
    }
  }
  else
  {
    while ((sz--) > 0)
    {
      *d = *s;
      d++;
      s++;
    }
  }

  return dst;
}

void*
nv_calloc(size_t sz)
{
  nv_assert_else_return(sz > 0, NULL);

  // void* ptr = calloc(1, sz);
  void* ptr = calloc(1, sz);
  nv_memset(ptr, 0, sz);

  return ptr;
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

  void* const orig = nv_malloc(total_size);
  if (!orig)
  {
    return NULL;
  }

  /**
   * We need to store the original pointer and the previous size just behind the
   * aligned block of memory.
   */
  uchar* ptr     = (uchar*)orig + sizeof(void*) + sizeof(size_t);
  uchar* aligned = (uchar*)((uintptr_t)(ptr + alignment - 1) & ~(alignment - 1));

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

  if (orig == NULL)
  {
    return nv_aligned_alloc(size, alignment);
  }

  void* absolute = nv_aligned_get_absolute_ptr(orig);
  if (NV_LIKELY(absolute))
  {
    size_t prev_size = *(size_t*)((uchar*)orig - sizeof(void*) - sizeof(size_t));

    if (prev_size == size)
    {
      return orig;
    }

    void* new_block = nv_aligned_alloc(size, alignment);
    nv_memmove(new_block, orig, NV_MIN(prev_size, size));
    nv_aligned_free(orig);

    return new_block;
  }

  nv_log_error("double free %p\n", orig);
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

void*
nv_realloc(void* prevblock, size_t new_sz)
{
  nv_assert_else_return(new_sz > 0, NULL);

  void* ptr = realloc(prevblock, new_sz);

  return ptr;
}

void
nv_free(void* block)
{
  // fuck you

  free(block);
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
  nv_assert_else_return(psize > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memchr, p, chr, psize);

  const uchar* read = (const uchar*)p;
  const uchar  chk  = chr;
  for (size_t i = 0; i < psize; i++)
  {
    if (read[i] == chk)
    {
      return (void*)(read + i);
    }
  }
  return NULL;
}

int
nv_memcmp(const void* _p1, const void* _p2, size_t max)
{
  nv_assert_else_return(max > 0, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memcmp, _p1, _p2, max);

  const uchar* p1 = (const uchar*)_p1;
  const uchar* p2 = (const uchar*)_p2;

  while ((max--) != 0)
  {
    if (*p1 != *p2)
    {
      return *p1 - *p2;
    }
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

  if (!dst)
  {
    return NV_MIN(slen, max);
  }

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
  nv_assert_else_return(max > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strncpy, dst, src, max);

  (void)nv_strncpy2(dst, src, max);
  return dst;
}

char*
nv_strcat(char* dst, const char* src)
{
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
nv_strncat(char* dst, const char* src, size_t max)
{
  nv_assert_else_return(max > 0, NULL);

  char* original_dest = dst;

  dst = nv_strchr(dst, '\0');
  if (!dst)
  {
    nv_assert(0);
    return NULL;
  }

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

  if (dest_size == 0)
  {
    return nv_strlen(src);
  }

  size_t dst_len = 0;
  while (dst_len < dest_size && dst[dst_len])
  {
    dst_len++;
  }

  if (dst_len == dest_size)
  {
    return dst_len + nv_strlen(src);
  }

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
  if (nv_strtrim_c(s, (const char**)&begin, (const char**)&end) == NULL)
  {
    return NULL;
  }

  /* end *may* be a pointer to the NULL terminator but yeah, still works */
  *end = 0;

  return begin;
}

const char*
nv_strtrim_c(const char* s, const char** begin, const char** end)
{
  while (*s && nv_chr_isspace((uchar)*s))
  {
    s++;
  }

  if (begin)
  {
    *begin = (const char*)s;
  }

  const char* begin_copy = s;

  s += nv_strlen(s);

  while (s > begin_copy && nv_chr_isspace((uchar) * (s - 1)))
  {
    s--;
  }

  if (end)
  {
    *end = (char*)s;
  }
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
  uchar c = (uchar)chr;

  while (*s)
  {
    if (*s == c)
    {
      return (char*)s;
    }
    s++;
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
      if (n <= 0)
      {
        return (char*)s;
      }
    }
  }
  return NULL;
}

char*
nv_strrchr(const char* s, int chr)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strrchr, s, chr);

  const char* beg = s;
  char*       end = (char*)s + nv_strlen(s) - 1;

  if (chr == 0)
  {
    return (char*)s + 1;
  }

  while (end > beg)
  {
    end--;
    if (*end == (char)chr)
    {
      return (char*)end;
    }
  }
  return NULL;
}

int
nv_strncmp(const char* s1, const char* s2, size_t max)
{
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
    uchar c1 = nv_chr_tolower(*(uchar*)s1);
    uchar c2 = nv_chr_tolower(*(uchar*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }

    s1++;
    s2++;
    i++;
  }
  if (i == max)
  {
    return 0;
  }
  return nv_chr_tolower(*(const uchar*)s1) - nv_chr_tolower(*(const uchar*)s2);
}

int
nv_strcasecmp(const char* s1, const char* s2)
{
  while (*s1 && *s2)
  {
    uchar c1 = nv_chr_tolower(*(uchar*)s1);
    uchar c2 = nv_chr_tolower(*(uchar*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return nv_chr_tolower(*(uchar*)s1) - nv_chr_tolower(*(uchar*)s2);
}

size_t
nv_strlen(const char* s)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strlen, s);

  const char* start = s;
  while (*s)
  {
    s++;
  }

  return s - start;
}

size_t
nv_strnlen(const char* s, size_t max)
{
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
    if (!*sub_it)
    {
      return (char*)s;
    }
  }

  return NULL;
}

char*
nv_strlcpy(char* dst, const char* src, size_t dst_size)
{
  if (dst_size == 0)
  {
    return dst;
  }

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
  size_t slen = nv_strlen(src);
  if (!dst)
  {
    return slen;
  }

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
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcspn, s, reject);

  const char* base = reject;
  size_t      i    = 0;

  while (*s)
  {
    const char* j = base;
    while (*j && *j != *s)
    {
      j++;
    }
    if (*j)
    {
      break;
    }
    i++;
    s++;
  }
  return i;
}

char*
nv_strpbrk(const char* s1, const char* s2)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strpbrk, s1, s2);

  while (*s1)
  {
    const char* j = s2;
    while (*j)
    {
      if (*j == *s1)
      {
        return (char*)s1;
      }
      j++;
    }
    s1++;
  }
  return NULL;
}

char*
nv_strtok(char* s, const char* delim, char** context)
{
  if (!s)
  {
    s = *context;
  }
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
nv_strreplace(char* s, char to_replace, char replace_with)
{
  while (*s)
  {
    if (*s == to_replace)
    {
      *s = replace_with;
    }
  }
  return s;
}

char*
nv_basename(const char* path)
{
  char* p         = (char*)path; // shut up C compiler
  char* backslash = nv_strrchr(path, '/');
  if (backslash != NULL)
  {
    return backslash + 1;
  }
  return p;
}

char*
nv_strdup(nv_allocator_fn alloc, void* alloc_user_data, const char* s)
{
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strdup, s);

  size_t slen = nv_strlen(s);

  char* new_s = alloc(alloc_user_data, NULL, NV_ALLOC_NEW_BLOCK, slen + 1);

  nv_strlcpy(new_s, s, slen + 1);

  return new_s;
}

char*
nv_strexdup(nv_allocator_fn alloc, void* alloc_user_data, const char* s, size_t size)
{
  char* new_s = alloc(alloc_user_data, NULL, NV_ALLOC_NEW_BLOCK, size + 1);

  nv_strlcpy(new_s, s, size + 1);

  return new_s;
}

char*
nv_substr(const char* s, size_t start, size_t len)
{
  nv_assert_else_return(len > 0, NULL);

  size_t slen = nv_strlen(s);
  if (start + len > slen)
  {
    return NULL;
  }

  char* sub = nv_malloc(len + 1);
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
  if (len == 0)
  {
    return str;
  }

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

bool
nv_chr_isalpha(int chr)
{
  return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
}

bool
nv_chr_isdigit(int chr)
{
  return (chr >= '0' && chr <= '9');
}

bool
nv_chr_isalnum(int chr)
{
  return nv_chr_isalpha(chr) || nv_chr_isdigit(chr);
}

bool
nv_chr_isblank(int chr)
{
  return (chr == ' ') || (chr == '\t');
}

/* https://en.wikipedia.org/wiki/Control_character */
bool
nv_chr_iscntrl(int chr)
{
  switch (chr)
  {
    /* \e also exists. non standard. yep. */
    case '\0':
    case '\a':
    case '\b':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r': return true;
    default: return false;
  }
  return false;
}

bool
nv_chr_islower(int chr)
{
  return (chr >= 'a' && chr <= 'z');
}

bool
nv_chr_isupper(int chr)
{
  return (chr >= 'A' && chr <= 'Z');
}

bool
nv_chr_isspace(int chr)
{
  return (chr == ' ' || chr == '\n' || chr == '\t');
}

bool
nv_chr_ispunct(int chr)
{
  /* Generated with
    for (i = 0; i < 256; i++)
    {
      if (ispunct(i))
      {
        printf("%c ", i);
      }
    }
  */
  switch (chr)
  {
    case '!':
    case '\"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '?':
    case '@':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '_':
    case '`':
    case '{':
    case '|':
    case '}':
    case '~': return true;
    default: return false;
  }
}

int
nv_chr_tolower(int chr)
{
  if (nv_chr_isupper(chr))
  {
    return chr + 32;
  }
  return chr;
}

int
nv_chr_toupper(int chr)
{
  if (nv_chr_islower(chr))
  {
    return chr - 32; /* chr - 32 */
  }
  return chr;
}
