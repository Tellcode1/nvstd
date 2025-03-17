#include <SDL2/SDL_mutex.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errqueue.h"
#include "print.h"
#include "props.h"
#include "stdafx.h"
#include "strconv.h"
#include "string.h"

static nv_error_queue_t g_error_queue = { .m_front = -1, .m_back = -1 };

void
_nv_push_error(const char* func, struct tm* time, const char* fmt, ...)
{
  char* write = nv_error_queue_push(&g_error_queue);

  va_list args;
  va_start(args, fmt);

  size_t written = nv_snprintf(write, NV_ERROR_LENGTH - 1, "[%d:%d:%d]%s%s(): ", time->tm_hour % 12, time->tm_min, time->tm_sec, " err: ", func);
  write += written;

  nv_vsnprintf(args, write, NV_ERROR_LENGTH - 1 - written, fmt);

  va_end(args);
}

const char*
nv_pop_error(void)
{
  return nv_error_queue_pop(&g_error_queue);
}

void
nv_flush_errors(void)
{
  const char* err = NULL;
  while ((err = nv_pop_error()) != NULL)
  {
    nv_printf("%s\n", err);
  }
}

static FILE*  g_stdstream   = NULL;
static char*  g_writebuf    = NULL;
static size_t g_writebufsiz = NOVA_WBUF_SIZE;

#if !defined(NVSM) && !defined(FONTC)

void
nv_print_time_as_string(FILE* stream)
{
  struct tm* time = _nv_get_time();

  nv_fprintf(stream, "[%d:%d:%d]", time->tm_hour % 12, time->tm_min, time->tm_sec);
}

void
_nv_log(va_list args, const char* fn, const char* succeeder, const char* preceder, const char* s, unsigned char err)
{
  FILE* out = (err) ? stderr : stdout;

  nv_print_time_as_string(out);
  nv_printf("%s%s(): ", preceder, fn);

  nv_vfprintf(args, out, s);
  nv_fprintf(out, "%s", succeeder);
}

// printf

void
nv_setwbuf(char* buf, size_t size)
{
  nv_assert(size > 0);
  if (g_writebuf)
  {
    nv_free(g_writebuf);
  }

  g_writebufsiz = size == 0 ? NOVA_WBUF_SIZE : size;
  g_writebuf    = buf ? buf : nv_malloc(size);
}

char*
nv_getwbuf(void)
{
  return g_writebuf;
}

void
nv_setstdout(FILE* stream)
{
  g_stdstream = stream;
}

size_t
nv_itoa2(intmax_t x, char out[], int base, size_t max)
{
  nv_assert(base >= 2 && base <= 36);
  nv_assert(out != NULL);

  if (max == 0)
  {
    return 0; // this shouldn't be an error
  }
  if (max == 1)
  {
    out[0] = 0;
    return 0;
  }

  /* The second parameter should always evaluate to true, left for brevity */
  if (x == 0 && max >= 2)
  {
    out[0] = '0';
    out[1] = 0;
    return 1;
  }

  size_t i = 0;

  // we need 1 space for NULL terminator!!
  // max will be greater or equal to 1 due to past checks
  max--;
  if (i >= max)
  {
    out[i] = 0;
    return i;
  }

  /* TODO: If max is exactly two, and x is negative, the function will only output a - */
  if (x < 0 && base == 10)
  {
    out[i] = '-';
    i++;
    x = -x;
  }
  if (i >= max)
  {
    out[i] = 0;
    return i;
  }

  /* highest power of base that is <= x */
  intmax_t highest_power_of_base = 1;
  while (highest_power_of_base <= x / base)
  {
    highest_power_of_base *= base;
  }

  do
  {
    if (i >= max)
    {
      break;
    }

    intmax_t dig = x / highest_power_of_base;

    if (dig < 10)
    {
      out[i] = (char)('0' + dig);
    }
    else
    {
      out[i] = (char)('a' + (dig - 10));
    }
    i++;

    x %= highest_power_of_base;
    highest_power_of_base /= base;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

size_t
nv_utoa2(uintmax_t x, char out[], int base, size_t max)
{
  nv_assert(base >= 2 && base <= 36);
  nv_assert(out != NULL);

  if (max == 0)
  {
    return 0; // this shouldn't be an error
  }
  if (max == 1)
  {
    out[0] = 0;
    return 0;
  }

  /* The second parameter should always evaluate to true, left for brevity */
  if (x == 0 && max >= 2)
  {
    out[0] = '0';
    out[1] = 0;
    return 1;
  }

  size_t i = 0;

  // we need 1 space for NULL terminator!!
  // max will be greater or equal to 1 due to past checks
  max--;
  if (i >= max)
  {
    out[i] = 0;
    return i;
  }

  /* highest power of base that is <= x */
  uintmax_t highest_power_of_base = 1;
  while (highest_power_of_base <= x / (uintmax_t)base)
  {
    highest_power_of_base *= base;
  }

  do
  {
    if (i >= max)
    {
      break;
    }

    uintmax_t dig = x / highest_power_of_base;

    if (dig < 10)
    {
      out[i] = (char)('0' + dig);
    }
    else
    {
      out[i] = (char)('a' + (dig - 10));
    }
    i++;

    x %= highest_power_of_base;
    highest_power_of_base /= base;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

#  define NOVA_FTOA_HANDLE_CASE(fn, n, str)                                                                                                                                   \
    if (fn(n))                                                                                                                                                                \
    {                                                                                                                                                                         \
      if (signbit(n) == 0)                                                                                                                                                    \
        return nv_strncpy2(s, str, max);                                                                                                                                      \
      else                                                                                                                                                                    \
        return nv_strncpy2(s, "-" str, max);                                                                                                                                  \
    }

// WARNING::: I didn't write most of this, stole it from stack overflow.
// if it explodes your computer its your fault!!!
size_t
nv_ftoa2(real_t n, char s[], int precision, size_t max, bool remove_zeros)
{
  if (max == 0)
  {
    return 0;
  }
  if (max == 1)
  {
    s[0] = 0;
    return 0;
  }

  NOVA_FTOA_HANDLE_CASE(isnan, n, "nan");
  NOVA_FTOA_HANDLE_CASE(isinf, n, "inf");
  NOVA_FTOA_HANDLE_CASE(0.0 ==, n, "0.0");

  char* c   = s;
  int   neg = (n < 0);
  if (neg)
  {
    n      = -n;
    *(c++) = '-';
  }

  int exp    = (n == 0.0) ? 0 : (int)log10(n);
  int useExp = (exp >= 14 || (neg && exp >= 9) || exp <= -9);
  if (useExp)
  {
    n /= pow(10.0, exp);
  }

  real_t rounding = pow(10.0, -precision) * 0.5;
  n += rounding;

  uint64_t int_part = (uint64_t)n;
  // int part is now floored
  real_t frac_part = n - (real_t)int_part;

  char* start = c;
  do
  {
    *(c++) = (char)('0' + (int_part % 10U));
    int_part /= 10;
  } while (int_part && (size_t)(c - s) < max - 1);

  char* end = c - 1;
  while (start < end)
  {
    char tmp = *start;
    *start++ = *end;
    *end--   = tmp;
  }

  if (precision > 0 && (size_t)(c - s) < max - 2)
  {
    *(c++) = '.';
    for (int i = 0; i < precision && (size_t)(c - s) < max - 1; i++)
    {
      frac_part *= 10;
      int digit = (int)frac_part;
      *(c++)    = (char)('0' + digit);
      frac_part -= digit;
    }
  }

  if (remove_zeros && precision > 0)
  {
    while (*(c - 1) == '0')
    {
      c--;
    }
    if (*(c - 1) == '.')
    {
      c--;
    }
  }

  if (useExp && (size_t)(c - s) < max - 4)
  {
    *(c++) = 'e';
    *(c++) = (exp >= 0) ? '+' : '-';
    exp    = (exp >= 0) ? exp : -exp;

    if (exp >= 100)
    {
      *(c++) = (char)('0' + (exp / 100U));
    }
    if (exp >= 10)
    {
      *(c++) = (char)('0' + ((exp / 10U) % 10U));
    }
    *(c++) = (char)('0' + (exp % 10U));
  }

  *c = 0;
  return c - s;
}

#  define NV_SKIP_WHITSPACE(s)                                                                                                                                                \
    while (*(s) && isspace(*(s)))                                                                                                                                             \
    (s)++

intmax_t
nv_atoi(const char s[])
{
  if (!s)
  {
    return __INTMAX_MAX__;
  }

  const char* i   = s;
  intmax_t    ret = 0;

  NV_SKIP_WHITSPACE(i);

  bool neg = 0;
  if (*i == '-')
  {
    neg = 1;
    i++;
  }
  else if (*i == '+')
  {
    i++;
  }

  while (*i)
  {
    if (!isdigit(*i))
    {
      break;
    }

    int digit = *i - '0';
    ret       = ret * 10 + digit;

    i++;
  }

  if (neg)
  {
    ret *= -1;
  }

  return ret;
}

real_t
nv_atof(const char s[])
{
  if (!s)
  {
    return INFINITY;
  }

  real_t      result = 0.0, fraction = 0.0;
  int         divisor = 1;
  bool        neg     = 0;
  const char* i       = s;

  NV_SKIP_WHITSPACE(i);

  if (*i == '-')
  {
    neg = 1;
    i++;
  }
  else if (*i == '+')
  {
    i++;
  }

  while (isdigit(*i))
  {
    result = result * 10 + (*i - '0');
    i++;
  }

  if (*i == '.')
  {
    i++;
    while (isdigit(*i))
    {
      fraction = fraction * 10 + (*i - '0');
      divisor *= 10;
      i++;
    }
    result += fraction / divisor;
  }

  if (*s == 'e' || *i == 'E')
  {
    i++;
    int exp_sign = 1;
    int exponent = 0;

    if (*i == '-')
    {
      exp_sign = -1;
      i++;
    }
    else if (*s == '+')
    {
      i++;
    }

    while (isdigit(*i))
    {
      exponent = exponent * 10 + (*i - '0');
      i++;
    }

    result = ldexp(result, exp_sign * exponent);
  }

  if (neg)
  {
    result *= -1.0;
  }

  return result;
}

bool
nv_atobool(const char s[])
{
  NV_SKIP_WHITSPACE(s);
  if (nv_strcasecmp(s, "false") == 0 || nv_strcmp(s, "0") == 0)
  {
    return false;
  }
  return true;
}

size_t
nv_ptoa2(void* p, char* buf, size_t max)
{
  if (p == NULL)
  {
    return nv_strncpy2(buf, "NULL", max);
  }

  unsigned long addr   = (unsigned long)p;
  const char    digs[] = "0123456789abcdef";

  size_t w = 0;

  w += nv_strncpy2(buf, "0x", max);

  // stolen from stack overflow
  for (int i = (sizeof(addr) * 2) - 1; i >= 0 && w < max - 1; i--)
  {
    int dig = (int)((addr >> (i * 4)) & 0xF);
    buf[w]  = digs[dig];
    w++;
  }
  buf[w] = 0;
  return w;
}

size_t
nv_btoa2(size_t x, bool upgrade, char* buf, size_t max)
{
  size_t written = 0;
  if (upgrade)
  {
    const char* stages[] = { " B", " KB", " MB", " GB", " TB", " PB", " Comically large number of bytes" };
    real_t      b        = (real_t)x;
    u32         stagei   = 0;

    const size_t num_stages = nv_arrlen(stages) - 1;
    while (b >= 1000.0 && stagei < num_stages)
    {
      stagei++;
      b /= 1000.0;
    }

    written = nv_ftoa2(b, buf, 3, max, 1);
    nv_strcat_max(buf, stages[stagei], max);
    written += nv_strlen(stages[stagei]);
    written = NV_MIN(written, max);
  }
  else
  {
    written = nv_utoa2(x, buf, 10, max);
  }
  return written;
}

// Moral of the story? FU@# SIZE_MAX
// I spent an HOUR trying to figure out what's going wrong
// and I didn't even bat an eye towards it

size_t
nv_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  // size_t chars_written = nv_vnprintf(g_writebufsiz, args, fmt);
  size_t chars_written = _nv_vsfnprintf(args, g_stdstream, 1, g_writebufsiz, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_fprintf(FILE* f, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = _nv_vsfnprintf(args, f, 1, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_sprintf(char* dest, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = _nv_vsfnprintf(args, dest, 0, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vprintf(va_list args, const char* fmt)
{
  return _nv_vsfnprintf(args, g_stdstream, 1, SIZE_MAX, fmt);
}

size_t
nv_vfprintf(va_list args, FILE* f, const char* fmt)
{
  return _nv_vsfnprintf(args, f, 1, SIZE_MAX, fmt);
}

size_t
nv_snprintf(char* dest, size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = nv_vsnprintf(args, dest, max_chars, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_nprintf(size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = _nv_vsfnprintf(args, g_stdstream, 1, max_chars, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vnprintf(va_list args, size_t max_chars, const char* fmt)
{
  return _nv_vsfnprintf(args, g_stdstream, 1, max_chars, fmt);
}

void
_nv_free_write_buffer(void)
{
  if (g_writebuf)
  {
    free(g_writebuf);
  }
}

size_t
nv_vsnprintf(va_list src, char* dest, size_t max_chars, const char* fmt)
{
  return _nv_vsfnprintf(src, dest, 0, max_chars, fmt);
}

typedef struct nv_format_info_t
{
  va_list*    m_args;
  void*       m_writeptr;
  char*       m_writep;
  const char* m_fmt_str_end;
  const char* m_iter;
  size_t      m_chars_written;
  size_t      m_written;
  size_t      m_max_chars;
  int         m_padding;
  int         m_padding_w;
  int         m_precision;
  bool        m_pad_zero;
  bool        m_left_align;
  bool        m_file;
  bool        m_wbuffer_used;
  bool        m_precision_specified;
  // ADD a precision given bool to allow for strings of length to be read
  char* m_pad_buf;
} nv_format_info_t;

static inline void
nv_printf_write(void* _write, nv_format_info_t* info, const char* write_buffer, size_t written)
{
  if (info->m_chars_written >= info->m_max_chars)
  {
    return;
  }

  size_t remaining = info->m_max_chars - info->m_chars_written;
  size_t to_write  = (written > remaining) ? remaining : written;
  info->m_chars_written += to_write;
  if (!write_buffer)
  {
    return;
  }

  if (info->m_file)
  {
    FILE* file = (FILE*)_write;
    fwrite(write_buffer, 1, to_write, file);
  }
  else
  {
    char** write = (char**)_write;
    if (*write && to_write > 0)
    {
      nv_memcpy(*write, write_buffer, to_write);
      (*write) += to_write;
    }
  }
}

static inline void
nv_printf_get_padding_and_precision_if_given(nv_format_info_t* info)
{
  if (*info->m_iter == '-')
  {
    info->m_left_align = true;
    info->m_iter++;
  }
  if (*info->m_iter == '0')
  {
    info->m_pad_zero = true;
    info->m_iter++;
  }

  if (*info->m_iter == '*')
  {
    info->m_padding_w = va_arg(*info->m_args, int);
    if (info->m_padding_w < 0)
    { // negative width means left align
      info->m_left_align = true;
      info->m_padding_w  = -info->m_padding_w;
    }
    info->m_iter++;
  }
  else
  {
    while (isdigit((unsigned char)*info->m_iter))
    {
      info->m_padding_w = info->m_padding_w * 10 + (*info->m_iter - '0');
      info->m_iter++;
    }
  }

  if (*info->m_iter == '.')
  {
    info->m_iter++;
    info->m_precision_specified = 1;
    if (*info->m_iter == '*')
    {
      info->m_precision = va_arg(*info->m_args, int);
      if (info->m_precision < 0)
      {
        info->m_precision = 6;
      }
      info->m_iter++;
    }
    else
    {
      info->m_precision = 0;
      while (isdigit((unsigned char)*info->m_iter))
      {
        info->m_precision = info->m_precision * 10 + (*info->m_iter - '0');
        info->m_iter++;
      }
    }
  }
}

static inline void
nv_printf_format_process_char(nv_format_info_t* info)
{
  if (info->m_chars_written >= info->m_max_chars - 1)
  {
    return;
  }

  // if user is asking for literal % sign, *iter will be the percent sign!!
  int chr = (*info->m_iter == 'c') ? va_arg(*info->m_args, int) : *info->m_iter;
  if (info->m_file)
  {
    fputc(chr, (FILE*)info->m_writeptr);
  }
  else if (info->m_writep)
  {
    *info->m_writep++ = (char)chr;
  }
  info->m_wbuffer_used = false;
  info->m_chars_written++;
}

static inline void
nv_printf_format_parse_string(nv_format_info_t* info)
{
  const char* string        = va_arg(*info->m_args, const char*);
  size_t      string_length = nv_strlen(string);
  if (!string)
  {
    string = "(null)";
  }
  if (info->m_precision_specified)
  {
    string_length = NV_MIN(string_length, info->m_precision);
  }
  nv_printf_write(info->m_writeptr, info, string, string_length);
  info->m_wbuffer_used = false;
}

static inline void
nv_printf_format_parse_format(nv_format_info_t* info)
{
  switch (*info->m_iter)
  {
    case 'F':
    case 'f': info->m_written = nv_ftoa2(va_arg(*info->m_args, real_t), g_writebuf, info->m_precision, info->m_max_chars - info->m_chars_written, 0); break;

    case 'l':
      if ((info->m_iter + 1) < info->m_fmt_str_end)
      {
        info->m_iter++;
      }

      if (*info->m_iter == 'd' || *info->m_iter == 'i')
      {
        info->m_written = nv_itoa2(va_arg(*info->m_args, long int), g_writebuf, 10, info->m_max_chars - info->m_chars_written);
      }
      else if (*info->m_iter == 'u')
      {
        info->m_written = nv_utoa2(va_arg(*info->m_args, long unsigned), g_writebuf, 10, info->m_max_chars - info->m_chars_written);
      }
      else if (*info->m_iter == 'f' || *info->m_iter == 'F')
      {
        info->m_written = nv_ftoa2(va_arg(*info->m_args, real_t), g_writebuf, info->m_precision, info->m_max_chars - info->m_chars_written, 0);
      }
      break;
    case 'd':
    case 'i': info->m_written = nv_itoa2(va_arg(*info->m_args, int), g_writebuf, 10, info->m_max_chars - info->m_chars_written); break;
    case 'z':
      if ((info->m_iter + 1) < info->m_fmt_str_end)
      {
        info->m_iter++;
      }
      if (*info->m_iter == 'i')
      {
        info->m_written = nv_itoa2(va_arg(*info->m_args, ssize_t), g_writebuf, 10, info->m_max_chars - info->m_chars_written);
      }
      else
      {
        info->m_written = nv_utoa2(va_arg(*info->m_args, size_t), g_writebuf, 10, info->m_max_chars - info->m_chars_written);
      }
      break;
    case 'u': info->m_written = nv_utoa2(va_arg(*info->m_args, unsigned), g_writebuf, 10, info->m_max_chars - info->m_chars_written); break;
    case '#':
      if ((info->m_iter + 1) < info->m_fmt_str_end && (*(info->m_iter + 1) == 'x'))
      {
        info->m_iter++;
        g_writebuf[0]   = '0';
        g_writebuf[1]   = 'x';
        info->m_written = 2 + nv_utoa2(va_arg(*info->m_args, unsigned), g_writebuf + 2, 16, info->m_max_chars - info->m_chars_written);
      }
      break;
    case 'x': info->m_written = nv_utoa2(va_arg(*info->m_args, unsigned), g_writebuf, 16, info->m_max_chars - info->m_chars_written); break;
    case 'p': info->m_written = nv_ptoa2(va_arg(*info->m_args, void*), g_writebuf, info->m_max_chars - info->m_chars_written); break;
    /* bytes, custom */
    case 'b': info->m_written = nv_btoa2(va_arg(*info->m_args, size_t), 1, g_writebuf, info->m_max_chars - info->m_chars_written); break;
    case 's': nv_printf_format_parse_string(info); break;
    case 'c':
    case '%':
    default: nv_printf_format_process_char(info); break;
  }
}

static inline void
nv_printf_format_upload_to_destination(nv_format_info_t* info)
{
  if (!info->m_wbuffer_used)
  {
    return;
  }

  info->m_padding = info->m_padding_w - (int)info->m_written;
  char pad_char   = info->m_pad_zero ? '0' : ' ';
  if (info->m_padding < 0)
  {
    info->m_padding = 0;
  }

  if (!info->m_left_align && info->m_padding > 0)
  {
    nv_memset(info->m_pad_buf, pad_char, sizeof(info->m_pad_buf));
    while (info->m_padding)
    {
      int chunk = (info->m_padding > (int)sizeof(info->m_pad_buf)) ? (int)sizeof(info->m_pad_buf) : info->m_padding;
      nv_printf_write(info->m_writeptr, info, info->m_pad_buf, chunk);
      info->m_padding -= chunk;
    }
  }

  nv_printf_write(info->m_writeptr, info, g_writebuf, info->m_written);

  if (info->m_left_align && info->m_padding > 0)
  {
    nv_memset(info->m_pad_buf, pad_char, sizeof(info->m_pad_buf));
    while (info->m_padding)
    {
      int chunk = (info->m_padding > (int)sizeof(info->m_pad_buf)) ? (int)sizeof(info->m_pad_buf) : info->m_padding;
      nv_printf_write(info->m_writeptr, info, info->m_pad_buf, chunk);
      info->m_padding -= chunk;
    }
  }
}

static inline void
nv_printf_format_parse_format_specifier(nv_format_info_t* info)
{
  info->m_wbuffer_used        = true;
  info->m_padding_w           = 0;
  info->m_precision           = 6;
  info->m_precision_specified = 0;

  nv_printf_get_padding_and_precision_if_given(info);

  nv_printf_format_parse_format(info);

  nv_printf_format_upload_to_destination(info);
}

static inline void
nv_printf_write_iterated_char(nv_format_info_t* info)
{
  if (info->m_chars_written >= info->m_max_chars - 1)
  {
    return;
  }

  if (*info->m_iter == '\b')
  {
    if (info->m_chars_written > 0)
    {
      if (info->m_file)
      {
        fputc('\b', (FILE*)info->m_writeptr);
      }
      else if (info->m_writep)
      {
        info->m_writep--;
        info->m_chars_written--;
      }
    }
  }
  else if (info->m_file)
  {
    if (fputc(*info->m_iter, (FILE*)info->m_writeptr) != *info->m_iter)
    {
      /* we can't use nv_printf here to avoid recursion */
      puts(strerror(errno));
    }
  }
  else if (info->m_writep)
  {
    *info->m_writep = *info->m_iter;
    info->m_writep++;
  }
  info->m_chars_written++;
}

static inline void
nv_printf_loop(nv_format_info_t* info)
{
  for (; *info->m_iter && info->m_chars_written < info->m_max_chars; info->m_iter++)
  {
    if (*info->m_iter == '%')
    {
      info->m_iter++;
      nv_printf_format_parse_format_specifier(info);
    }
    else
    {
      nv_printf_write_iterated_char(info);
    }
  }
}

size_t
_nv_vsfnprintf(va_list src, void* dest, bool is_file, size_t max_chars, const char* fmt)
{
  if (!g_writebuf)
  {
    g_writebuf = nv_malloc(g_writebufsiz);
    if (g_writebuf == NULL)
    {
      exit(-1);
    }
    if (atexit(_nv_free_write_buffer) != 0)
    {
      exit(-1);
    }
  }
  if (!g_stdstream)
  {
    g_stdstream = stdout;
  }
  if (is_file && !dest)
  {
    dest = g_stdstream;
  }

  if (fmt == NULL)
  {
    return 0;
  }

  nv_format_info_t info = nv_zero_init(nv_format_info_t);

  va_list args;
  va_copy(args, src);

  NV_ALIGN_TO(64) char pad_buf[64];

  char* writep = (char*)dest;

  info.m_args        = &args;
  info.m_writeptr    = is_file ? dest : (void*)&writep;
  info.m_max_chars   = max_chars;
  info.m_precision   = 6;
  info.m_writep      = (char*)dest;
  info.m_fmt_str_end = fmt + nv_strlen(fmt);
  info.m_iter        = fmt;
  info.m_file        = is_file;
  info.m_pad_buf     = pad_buf;

  if (is_file)
  {
    info.m_writeptr = dest; // attach file
  }
  else
  {
    // attach POINTER to the write so we can iterate over it
    info.m_writeptr = (void*)&info.m_writep;
  }

  nv_printf_loop(&info);

  if (!is_file && info.m_writep && max_chars > 0)
  {
    size_t width         = (info.m_chars_written < max_chars) ? info.m_chars_written : max_chars - 1;
    ((char*)dest)[width] = 0;
  }

  va_end(args);

  return info.m_chars_written;
}

// printf

void
_nv_log_error(const char* func, const char* fmt, ...)
{
  // it was funny while it lasted.
  const char* preceder  = " err: ";
  const char* succeeder = "";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
}

void
_nv_log_and_abort(const char* func, const char* fmt, ...)
{
  const char* preceder  = " fatal error: ";
  const char* succeeder = "\nabort.";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
  exit(-1);
}

void
_nv_log_warning(const char* func, const char* fmt, ...)
{
  const char* preceder  = " warning: ";
  const char* succeeder = "";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_info(const char* func, const char* fmt, ...)
{
  const char* preceder  = " info: ";
  const char* succeeder = "";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_debug(const char* func, const char* fmt, ...)
{
  const char* preceder  = " debug: ";
  const char* succeeder = "";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_custom(const char* func, const char* preceder, const char* fmt, ...)
{
  const char* succeeder = "";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

#  include <zlib.h>

int
nv_bufcompress(const void* NV_RESTRICT input, size_t input_size, void* NV_RESTRICT output, size_t* NV_RESTRICT output_size)
{
  z_stream stream = nv_zero_init(z_stream);

  if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
  {
    return -1;
  }

  stream.next_in  = (unsigned char*)input;
  stream.avail_in = input_size;

  stream.next_out  = output;
  stream.avail_out = *output_size;

  if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
  {
    deflateEnd(&stream);
    return -1;
  }

  *output_size = stream.total_out;

  deflateEnd(&stream);
  return 0;
}

unsigned long
nv_bufdecompress(const void* NV_RESTRICT compressed_data, size_t compressed_size, void* NV_RESTRICT o_buf, size_t o_buf_sz)
{
  z_stream strm  = { 0 };
  strm.next_in   = (unsigned char*)compressed_data;
  strm.avail_in  = compressed_size;
  strm.next_out  = o_buf;
  strm.avail_out = o_buf_sz;

  if (inflateInit(&strm) != Z_OK)
  {
    return 0;
  }

  int ret = inflate(&strm, Z_FINISH);
  if (ret != Z_STREAM_END)
  {
    inflateEnd(&strm);
    return 0;
  }

  inflateEnd(&strm);
  return strm.total_out;
}

void*
nv_memcpy(void* NV_RESTRICT dst, const void* NV_RESTRICT src, size_t sz)
{
  if (dst == NULL || src == NULL || sz == 0)
  {
    return NULL;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_memcpy(dst, src, sz);
#  endif

  // I saw this optimization trick a long time ago in some big codebase
  // and it has sticken to me
  // do you know how much I just get an ITCH to write memcpy myself?
  if (((uintptr_t)src & 0x3) == 0 && ((uintptr_t)dst & 0x3) == 0)
  {
    const int* read   = (const int*)src;
    int*       writep = (int*)dst;

    const size_t int_count = sz / sizeof(int);
    for (size_t i = 0; i < int_count; i++)
    {
      writep[i] = read[i];
    }

    const uchar* byte_read  = (uchar*)(read + int_count);
    uchar*       byte_write = (uchar*)(writep + int_count);

    sz %= sizeof(int);
    for (size_t i = 0; i < sz; i++)
    {
      byte_write[i] = byte_read[i];
    }
  }
  else
  {
    const uchar* read   = (const uchar*)src;
    uchar*       writep = (uchar*)dst;
    for (size_t i = 0; i < sz; i++)
    {
      writep[i] = read[i];
    }
  }

  return dst;
}

// rewritten memcpy
void*
nv_memset(void* dst, char to, size_t sz)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_memset(dst, to, sz);
#  endif

  nv_assert(dst != NULL);
  nv_assert(sz != 0);

  uintptr_t d            = (uintptr_t)dst;
  size_t    align_offset = d & (sizeof(size_t) - 1);

  // do not ask what the fuck this is.
  // basically, it's used to project a character to a word
  size_t word_to = 0x0101010101010101ULL * (unsigned char)to;

  unsigned char* byte_write = (unsigned char*)dst;
  while (align_offset && sz)
  {
    *byte_write++ = to;
    sz--;
    align_offset = (uintptr_t)byte_write & (sizeof(size_t) - 1);
  }

  size_t* word_write = (size_t*)byte_write;
  while (sz >= sizeof(size_t) * 4)
  {
    word_write[0] = word_to;
    word_write[1] = word_to;
    word_write[2] = word_to;
    word_write[3] = word_to;
    word_write += 4;
    sz -= sizeof(size_t) * 4;
  }

  while (sz >= sizeof(size_t))
  {
    *word_write++ = word_to;
    sz -= sizeof(size_t);
  }

  byte_write = (unsigned char*)word_write;
  while (sz--)
  {
    *byte_write++ = to;
  }

  return dst;
}

void*
nv_memmove(void* dst, const void* src, size_t sz)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_memmove(dst, src, sz);
#  endif

  if (!dst || !src || sz == 0)
  {
    return NULL;
  }

  unsigned char*       d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;

  if (d > s && d < s + sz)
  {
    d += sz;
    s += sz;
    while (sz--)
    {
      *(--d) = *(--s);
    }
  }
  else
  {
    while (sz--)
    {
      *(d++) = *(s++);
    }
  }

  return dst;
}

void*
nv_malloc(size_t sz)
{
  void* ptr = malloc(sz);
  nv_assert(ptr != NULL);
  return ptr;
}

void*
nv_calloc(size_t sz)
{
  void* ptr = calloc(1, sz);
  nv_assert(ptr != NULL);
  return ptr;
}

void*
nv_realloc(void* prevblock, size_t new_sz)
{
  void* ptr = realloc(prevblock, new_sz);
  nv_assert(ptr != NULL);
  return ptr;
}

void
nv_free(void* block)
{
  // fuck you
  nv_assert(block != NULL);
  free(block);
}

void*
nv_memchr(const void* p, int chr, size_t psize)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_memchr(p, chr, psize);
#  endif

  if (!p || !psize)
  {
    return NULL;
  }
  const unsigned char* read = (const unsigned char*)p;
  const unsigned char  chk  = chr;
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
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_memcmp(_p1, _p2, max);
#  endif

  if (!_p1 || !_p2 || max == 0)
  {
    return -1;
  }

  const uchar* p1 = (const uchar*)_p1;
  const uchar* p2 = (const uchar*)_p2;

  // move && compare the pointer p1 until we reach alignment
  while (max > 0 && ((uintptr_t)p1 & (sizeof(size_t) - 1)) != 0)
  {
    if (*p1 != *p2)
    {
      return *p1 - *p2;
    }
    p1++;
    p2++;
    max--;
  }

  const size_t* w1         = (const size_t*)p1;
  const size_t* w2         = (const size_t*)p2;
  size_t        word_count = max / sizeof(size_t);

  for (size_t i = 0; i < word_count; i++)
  {
    if (w1[i] != w2[i])
    {
      p1 = (const uchar*)&w1[i];
      p2 = (const uchar*)&w2[i];
      break;
    }
  }

  max %= sizeof(size_t);
  p1 += word_count * sizeof(size_t);
  p2 += word_count * sizeof(size_t);

  while (max--)
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
nv_strncpy2(char* dest, const char* src, size_t max)
{
  size_t slen = nv_strlen(src);

  if (!dest)
  {
    return NV_MIN(slen, max);
  }
  if (!src)
  {
    return (size_t)-1;
  }
  if (max == 0)
  {
    return 0;
  } // we have to have this condition because max is subtracted just after which may cause it to underflow

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  __builtin_strncpy(dest, src, max);
  return NV_MIN(slen, max);
#  endif

  max--;

  while (*src && ((uintptr_t)dest & (sizeof(size_t) - 1)) != 0)
  {
    *dest++ = *src++;
    max--;
  }

  size_t*       destp = (size_t*)dest;
  const size_t* srcp  = (const size_t*)src;
  size_t        words = max / sizeof(size_t);

  for (size_t i = 0; i < words; i++)
  {
    destp[i] = srcp[i];
  }

  dest += words * sizeof(size_t);
  src += words * sizeof(size_t);
  max %= sizeof(size_t);

  while (*src && max)
  {
    *dest++ = *src++;
    max--;
  }

  *dest = 0;

  return NV_MIN(slen, max);
}

char*
nv_strcpy(char* dest, const char* src)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strcpy(dest, src); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
#  endif

  if (!dest || !src)
  {
    return NULL;
  }
  size_t i = 0;
  // clang-format off
  while (src[i])
  {
    dest[i] = src[i]; i++;
  }
  // clang-format on
  dest[i] = 0;
}

char*
nv_strncpy(char* dest, const char* src, size_t max)
{
  if (!dest || !src)
  {
    return NULL;
  }
  if (max == 0)
  {
    return dest;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strncpy(dest, src, max);
#  endif

  if (max == 0 || !dest || !src)
  {
    return NULL;
  }
  max--; // -1 so we can fit the NULL terminator
  size_t i = 0;
  // clang-format off
  while (i < max && src[i])
  {
    dest[i] = src[i]; i++;
  }
  // clang-format on
  dest[i] = 0;
}

char*
nv_strcat(char* dest, const char* src)
{
  char* original_dest = dest;
  while (*dest)
  {
    dest++; // move to end of dest
  }

  while (*src)
  {
    *dest = *src;
    src++;
    dest++;
  }
  *dest = 0;
  return original_dest;
}

char*
nv_strncat(char* dest, const char* src, size_t max)
{
  char* original_dest = dest;
  while (*dest)
  {
    dest++; // move to end of dest
  }

  size_t i = 0;
  while (*src && i < max)
  {
    *dest = *src;
    i++;
    src++;
    dest++;
  }
  *dest = 0;
  return original_dest;
}

char*
nv_strcat_max(char* dest, const char* src, size_t dest_size)
{
  char* original_dest = dest;
  while (*dest)
  {
    dest++;
    dest_size--;
    if (dest_size == 1)
    { // null terminator
      return dest;
    }
  }

  size_t i = 0;

  while (*src && i < dest_size - 1)
  {
    *dest = *src;
    i++;
    src++;
    dest++;
  }

  *dest = 0;
  return original_dest;
}

int
nv_strcmp(const char* s1, const char* s2)
{
  if (!s1 || !s2)
  {
    return -1;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strcmp(s1, s2);
#  endif

  while (*s1 && *s2 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *s2 - *s1;
}

char*
nv_strchr(const char* s, int chr)
{
  if (!s)
  {
    return NULL;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strchr(s, chr);
#  endif

  while (*s)
  {
    if (*s == chr)
    {
      return (char*)s;
    }
    s++;
  }
  return NULL;
}

char*
nv_strrchr(const char* s, int chr)
{
  if (!s)
  {
    return NULL;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strrchr(s, chr);
#  endif

  const char* beg = s;
  s += nv_strlen(s) - 1;
  while (s >= beg)
  {
    if (*s == chr)
    {
      return (char*)s;
    }
    s--;
  }
  return NULL;
}

int
nv_strncmp(const char* s1, const char* s2, size_t max)
{
  if (!s1 || !s2 || max == 0)
  {
    return -1;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strncmp(s1, s2, max);
#  endif
  size_t i = 0;
  while (*s1 && *s2 && (*s1 == *s2) && i < max)
  {
    s1++;
    s2++;
    i++;
  }
  return (i == max) ? 0 : (*(const unsigned char*)s1 - *(const unsigned char*)s2);
}

int
nv_strcasencmp(const char* s1, const char* s2, size_t max)
{
  if (!s1 || !s2)
  {
    return -1;
  }

  size_t i = 0;
  while (*s1 && *s2 && i < max)
  {
    unsigned char c1 = tolower(*(unsigned char*)s1);
    unsigned char c2 = tolower(*(unsigned char*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
    i++;
  }
  return tolower(*(unsigned char*)s1) - tolower(*(unsigned char*)s2);
}

int
nv_strcasecmp(const char* s1, const char* s2)
{
  if (!s1 || !s2)
  {
    return -1;
  }

  while ((uintptr_t)*s1 & (sizeof(size_t) - 1))
  {
    unsigned char c1 = tolower(*(unsigned char*)s1);
    unsigned char c2 = tolower(*(unsigned char*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
  }

  // FIXME: Implement
  while (*s1 && *s2)
  {
    unsigned char c1 = tolower(*(unsigned char*)s1);
    unsigned char c2 = tolower(*(unsigned char*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return tolower(*(unsigned char*)s1) - tolower(*(unsigned char*)s2);
}

size_t
nv_strlen(const char* s)
{
  if (!s)
  {
    return 0;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strlen(s);
#  endif

  const char* start = s;

  while ((uintptr_t)s & (sizeof(size_t) - 1)) // align s to 8 byte boundary so we can check sizeof(size_t) bytes at once
  {
    if (!*s)
    {
      return s - start;
    }
    s++;
  }

  const uint64_t mask = 0x0101010101010101ULL;
  while (1)
  {
    uint64_t word = *(uint64_t*)s;
    if (((word - mask) & ~word) & (mask << 7))
    {
      break;
    }
    s += 8;
  }

  while (*s)
  {
    s++;
  }

  return s - start;
}

char*
nv_strstr(const char* s, const char* sub)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strstr(s, sub);
#  endif

  if (!s || !sub)
  {
    return NULL;
  }

  for (; *s; s++)
  {
    const char* s = s;
    const char* p = sub;

    while (*s && *p && *s == *p)
    {
      s++;
      p++;
    }

    if (!*p)
    {
      return (char*)s;
    }
  }

  return NULL;
}

size_t
nv_strcpy2(char* dest, const char* src)
{
  if (!src)
  {
    return 0;
  }

  size_t slen = nv_strlen(src);
  if (!dest)
  {
    return slen;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strlen(__builtin_strcpy(dest, src)); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
#  endif
  const char* original_dest = dest;
  while (*src)
  {
    *dest = *src;
    src++;
    dest++;
  }
  *dest = 0;
  return dest - original_dest;
}

size_t
nv_strspn(const char* s, const char* accept)
{
  if (!s || !accept)
  {
    return 0;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strspn(s, accept);
#  endif
  size_t i = 0;
  while (*s && *accept && *s == *accept)
  {
    i++;
  }
  return i;
}

size_t
nv_strcspn(const char* s, const char* reject)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strcspn(s, reject);
#  endif

  if (!s || !reject)
  {
    return 0;
  }

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
  if (!s1 || !s2)
  {
    return NULL;
  }

#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strpbrk(s1, s2);
#  endif

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

char* strtoks = NULL;
char*
nv_strtok(char* s, const char* delim)
{
  if (!s)
  {
    s = strtoks;
  }
  char* p;

  s += nv_strspn(s, delim);
  if (!s || *s == 0)
  {
    strtoks = s;
    return NULL;
  }

  p = s;
  s = nv_strpbrk(s, delim);

  if (!s)
  {
    strtoks = nv_strchr(s, 0); // get pointer to last char
    return p;
  }
  *s      = 0;
  strtoks = s + 1;
  return p;
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
nv_strdup(const char* s)
{
#  if defined(__GNUC__) && (NOVA_STR_USE_BUILTIN)
  return __builtin_strdup(s);
#  endif

  size_t slen  = nv_strlen(s);
  char*  new_s = nv_malloc(slen);
  nv_strncpy(new_s, s, slen);
  new_s[slen] = 0;
  return new_s;
}

char*
nv_substr(const char* s, size_t start, size_t len)
{
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

static inline const nv_option_t*
nv_option_find(const nv_option_t* options, int noptions, const char* short_name, const char* long_name)
{
  nv_assert(noptions >= 0);

  size_t i = 0;
  for (const nv_option_t* opt = options; i < (size_t)noptions; opt++, i++)
  {
    if (short_name && opt->m_short_name && nv_strcmp(opt->m_short_name, short_name) == 0)
    {
      return opt;
    }
    if (long_name && opt->m_long_name && nv_strcmp(opt->m_long_name, long_name) == 0)
    {
      return opt;
    }
  }
  return NULL;
}

static inline const char*
nv_props_get_tp_name(nv_option_type tp)
{
  switch (tp)
  {
    case NV_OP_TYPE_BOOL: return "bool";
    case NV_OP_TYPE_STRING: return "string";
    case NV_OP_TYPE_INT: return "int";
    case NV_OP_TYPE_FLOAT: return "float";
    case NV_OP_TYPE_DOUBLE: return "double";
    default: return "unknown";
  }
}

void
nv_props_gen_help(const nv_option_t* options, int noptions, char* buf, size_t buf_size)
{
  nv_assert(options != NULL);
  nv_assert(noptions > 0);
  nv_assert(buf != NULL);
  nv_assert(buf_size > 0);

  size_t available = buf_size;
  size_t written   = nv_snprintf(buf, 256, "Options: %i\n", noptions);
  nv_assert(available > written);

  available -= written;
  buf += written;

  for (int i = 0; i < noptions; i++)
  {
    const nv_option_t* opt = &options[i];

    const char* short_name = opt->m_short_name ? opt->m_short_name : "<empty>";
    const char* long_name  = opt->m_long_name ? opt->m_long_name : "<empty>";

    written = nv_snprintf(buf, available, "\t-%s, --%s <%s>\n", short_name, long_name, nv_props_get_tp_name(opt->m_type));
    buf += written;
    if (written > available)
    {
      break;
    }
    available -= written;
  }
}

static inline int
_nv_props_parse_arg(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size, int* i)
{
  nv_assert(argc != 0);
  nv_assert(argv != NULL);
  nv_assert(options != NULL);
  nv_assert(noptions > 0);
  nv_assert(error != NULL);
  nv_assert(error_size > 0);
  nv_assert(i != NULL);

  char* arg     = argv[*i];
  bool  is_long = false;
  char* name    = NULL;
  char* value   = NULL;

  // not option?
  if (arg[0] != '-')
  {
    return 0;
  }

  if (arg[1] == '-')
  {
    is_long  = 1;
    name     = arg + 2;
    char* eq = nv_strchr(name, '=');
    if (eq)
    {
      *eq   = '\0';
      value = eq + 1;
    }
  }
  else
  {
    name = arg + 1;
  }

  const nv_option_t* opt = is_long ? nv_option_find(options, noptions, NULL, name) : nv_option_find(options, noptions, name, NULL);
  if (!opt)
  {
    nv_snprintf(error, error_size, "unknown option: %s%s", is_long ? "--" : "-", name);
    (*i)++;
    return -1;
  }

  if (opt->m_type == NV_OP_TYPE_BOOL)
  {
    bool flag_value = true;
    if (is_long && value)
    {
      flag_value = nv_atobool(value);
    }
    if (opt->m_type)
    {
      *(bool*)opt->m_type = flag_value;
    }
    (*i)++;
    return 0;
  }

  if (!value)
  {
    if (!is_long)
    {
      size_t opt_name_len = nv_strlen(opt->m_short_name);
      size_t arg_name_len = nv_strlen(name);
      if (arg_name_len > opt_name_len)
      {
        value = name + opt_name_len;
      }
      else if ((*i) + 1 < argc && argv[*i + 1][0] != '-')
      {
        value = argv[++(*i)];
      }
    }
    else
    {
      (*i)++;
      if ((*i) >= argc || argv[*i][0] == '-')
      {
        nv_snprintf(error, error_size, "option --%s requires a value", name);
        return -1;
      }
      value = argv[*i];
    }
  }

  if (opt->m_value)
  {
    switch (opt->m_type)
    {
      case NV_OP_TYPE_STRING:
        nv_strncpy((char*)opt->m_value, value, opt->m_buffer_size);
        ((char*)opt->m_value)[opt->m_buffer_size - 1] = '\0';
        break;
      case NV_OP_TYPE_INT: *(int*)opt->m_value = (int)nv_atoi(value); break;
      case NV_OP_TYPE_FLOAT: *(flt_t*)opt->m_value = (flt_t)nv_atof(value); break;
      case NV_OP_TYPE_DOUBLE: *(real_t*)opt->m_value = nv_atof(value); break;
      default: break;
    }
  }

  (*i)++;
  return 0;
}

int
nv_props_parse(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size)
{
  nv_assert(argc != 0);
  nv_assert(argv != NULL);
  nv_assert(options != NULL);
  nv_assert(noptions > 0);
  nv_assert(error != NULL);
  nv_assert(error_size > 0);

  int  i       = 1; // program name is argv[0]
  bool success = true;

  while (i < argc)
  {
    char* arg = argv[i];
    if (arg[0] == '-')
    {
      int result = _nv_props_parse_arg(argc, argv, options, noptions, error, error_size, &i);
      if (result != 0)
      {
        success = false;
      }
    }
    else
    {
      break;
    }
  }
  return success ? 0 : -1;
}

#endif
