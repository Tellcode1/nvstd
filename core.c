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

#include <SDL2/SDL_mutex.h>
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

#include "chrclass.h"
#include "errqueue.h"
#include "print.h"
#include "props.h"
#include "stdafx.h"
#include "strconv.h"
#include "string.h"
#include "types.h"

/* https://graphics.stanford.edu/~seander/bithacks.html */
#define WORD_HAS_NULL_TERMINATOR(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)

#define WORD_HAS_CHARACTER(word, chr) (WORD_HAS_NULL_TERMINATOR((word) ^ (~0UL / 255 * (chr))))

#define IS_NOT_ALIGNED_TO_WORD_SIZE(ptr) (((uintptr_t)ptr & (sizeof(nv_word_t) - 1)) != 0)

static nv_error_queue_t g_error_queue = { .m_front = -1, .m_back = -1 };

void
_nv_push_error(const char* file, size_t line, const char* func, struct tm* time, const char* fmt, ...)
{
  char* write = nv_error_queue_push(&g_error_queue);

  va_list args;
  va_start(args, fmt);

  size_t written = nv_snprintf(write, NV_ERROR_LENGTH - 1, "[%d:%d:%d] [%s:%zu] err: %s(): ", time->tm_hour % 12, time->tm_min, time->tm_sec, nv_basename(file), line, func);
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
    puts(err);
  }
}

#if !defined(NVSM) && !defined(FONTC)

void
nv_print_time_as_string(FILE* stream)
{
  nv_assert(0);
  (void)stream;
}

void
_nv_log(va_list args, const char* file, size_t line, const char* fn, const char* preceder, const char* s, bool err)
{
  nv_assert_and_ret(args != NULL, );
  nv_assert_and_ret(file != NULL, );
  nv_assert_and_ret(fn != NULL, );
  nv_assert_and_ret(preceder != NULL, );
  nv_assert_and_ret(s != NULL, );

  FILE* out = (err) ? stderr : stdout;

  /* Two fprintf calls, good. */

  struct tm* time = _nv_get_time();
  nv_fprintf(out, "[%d:%d:%d] [%s:%zu]%s%s(): ", time->tm_hour % 12, time->tm_min, time->tm_sec, nv_basename(file), line, preceder, fn);

  nv_vfprintf(args, out, s);
}

// printf

size_t
nv_itoa2(intmax_t num, char out[], int base, size_t max)
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
  if (num == 0 && max >= 2)
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
  if (num < 0 && base == 10)
  {
    out[i] = '-';
    i++;
    num = -num;
  }
  if (i >= max)
  {
    out[i] = 0;
    return i;
  }

  /* highest power of base that is <= x */
  intmax_t highest_power_of_base = 1;
  while (highest_power_of_base <= num / base)
  {
    highest_power_of_base *= base;
  }

  do
  {
    if (i >= max)
    {
      break;
    }

    intmax_t dig = num / highest_power_of_base;

    if (dig < 10)
    {
      out[i] = (char)('0' + dig);
    }
    else
    {
      out[i] = (char)('a' + (dig - 10));
    }
    i++;

    num %= highest_power_of_base;
    highest_power_of_base /= base;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

size_t
nv_utoa2(uintmax_t num, char out[], int base, size_t max)
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
  if (num == 0 && max >= 2)
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
  while (highest_power_of_base <= num / (uintmax_t)base)
  {
    highest_power_of_base *= base;
  }

  do
  {
    if (i >= max)
    {
      break;
    }

    uintmax_t dig = num / highest_power_of_base;

    if (dig < 10)
    {
      out[i] = (char)('0' + dig);
    }
    else
    {
      out[i] = (char)('a' + (dig - 10));
    }
    i++;

    num %= highest_power_of_base;
    highest_power_of_base /= base;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

#  define NOVA_FTOA_HANDLE_CASE(fn, n, str)                                                                                                                                   \
    if (fn(n))                                                                                                                                                                \
    {                                                                                                                                                                         \
      if (signbit(n) == 0)                                                                                                                                                    \
        return nv_strncpy2(out, str, max);                                                                                                                                    \
      else                                                                                                                                                                    \
        return nv_strncpy2(out, "-" str, max);                                                                                                                                \
    }

// WARNING::: I didn't write most of this, stole it from stack overflow.
// if it explodes your computer its your fault!!!
size_t
nv_ftoa2(real_t num, char out[], int precision, size_t max, bool remove_zeros)
{
  if (max == 0)
  {
    return 0;
  }
  if (max == 1)
  {
    out[0] = 0;
    return 0;
  }

  NOVA_FTOA_HANDLE_CASE(isnan, num, "nan");
  NOVA_FTOA_HANDLE_CASE(isinf, num, "inf");
  NOVA_FTOA_HANDLE_CASE(0.0 ==, num, "0.0");

  char* itr = out;

  const int neg = (num < 0);
  if (neg)
  {
    num  = -num;
    *itr = '-';
    itr++;
  }

  int exponent        = (num == 0.0) ? 0 : (int)log10(num);
  int to_use_exponent = (exponent >= 14 || (neg && exponent >= 9) || exponent <= -9);
  if (to_use_exponent)
  {
    num /= pow(10.0, exponent);
  }

  real_t rounding = pow(10.0, -precision) * 0.5;
  num += rounding;

  /* n has been absoluted before so we can expect that it won't be negative */
  uintmax_t int_part = (uintmax_t)num;

  // int part is now floored
  real_t frac_part = num - (real_t)int_part;

  itr += nv_utoa2(int_part, itr, 10, max - 1);

  if (precision > 0 && (size_t)(itr - out) < max - 2)
  {
    *itr = '.';
    itr++;

    for (int i = 0; i < precision && (size_t)(itr - out) < max - 1; i++)
    {
      frac_part *= 10;
      int digit = (int)frac_part;
      *itr      = (char)('0' + digit);
      itr++;
      frac_part -= digit;
    }
  }

  if (remove_zeros && precision > 0)
  {
    while (*(itr - 1) == '0')
    {
      itr--;
    }
    if (*(itr - 1) == '.')
    {
      itr--;
    }
  }

  if (to_use_exponent && (size_t)(itr - out) < max - 4)
  {
    *itr = 'e';
    itr++;
    *itr = (exponent >= 0) ? '+' : '-';
    itr++;

    exponent = (exponent >= 0) ? exponent : -exponent;

    if (exponent >= 100)
    {
      *itr = (char)('0' + (exponent / 100U));
      itr++;
    }
    if (exponent >= 10)
    {
      *itr = (char)('0' + ((exponent / 10U) % 10U));
      itr++;
    }
    *itr = (char)('0' + (exponent % 10U));
    itr++;
  }

  *itr = 0;
  return itr - out;
}

#  define NV_SKIP_WHITSPACE(s) nv_strtrim_c(s, &s, NULL);

intmax_t
nv_atoi(const char in_string[], size_t max)
{
  if (!in_string)
  {
    return __INTMAX_MAX__;
  }

  const char* c   = in_string;
  intmax_t    ret = 0;
  size_t      i   = 0;

  NV_SKIP_WHITSPACE(c);

  bool neg = 0;
  if (i < max && *c == '-')
  {
    neg = 1;
    c++;
    i++;
  }
  else if (i < max && *c == '+')
  {
    c++;
    i++;
  }

  while (i < max && *c)
  {
    if (!nv_chr_isdigit(*c))
    {
      break;
    }

    int digit = *c - '0';
    ret       = ret * 10 + digit;

    c++;
    i++;
  }

  if (neg)
  {
    ret *= -1;
  }

  return ret;
}

real_t
nv_atof(const char in_string[], size_t max)
{
  if (!in_string)
  {
    return 0.0;
  }

  real_t      result = 0.0, fraction = 0.0;
  int         divisor = 1;
  bool        neg     = 0;
  const char* c       = in_string;
  size_t      i       = 0;

  NV_SKIP_WHITSPACE(c);

  if (i < max && *c == '-')
  {
    neg = 1;
    c++;
    i++;
  }
  else if (i < max && *c == '+')
  {
    c++;
    i++;
  }

  while (i < max && nv_chr_isdigit(*c))
  {
    result = result * 10 + (*c - '0');
    c++;
  }

  if (*c == '.')
  {
    c++;
    while (nv_chr_isdigit(*c))
    {
      fraction = fraction * 10 + (*c - '0');
      divisor *= 10;
      c++;
      i++;
    }
    result += fraction / divisor;
  }

  if (i < max && (*c == 'e' || *c == 'E'))
  {
    c++;
    i++;
    int exp_sign = 1;
    int exponent = 0;

    if (*c == '-')
    {
      exp_sign = -1;
      c++;
      i++;
    }
    else if (*in_string == '+')
    {
      c++;
      i++;
    }

    while (i < max && nv_chr_isdigit(*c))
    {
      exponent = exponent * 10 + (*c - '0');
      c++;
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
nv_atobool(const char in_string[], size_t max)
{
  size_t i = 0;
  NV_SKIP_WHITSPACE(in_string);
  if (i > max)
  {
    return true;
  }
  if (nv_strcasencmp(in_string, "false", max - i) == 0 || nv_strncmp(in_string, "0", max - i) == 0)
  {
    return false;
  }
  return true;
}

size_t
nv_ptoa2(void* ptr, char out[], size_t max)
{
  if (ptr == NULL)
  {
    return nv_strncpy2(out, "NULL", max);
  }

  unsigned long addr   = (unsigned long)ptr;
  const char    digs[] = "0123456789abcdef";

  size_t w = 0;

  w += nv_strncpy2(out, "0x", max);

  // stolen from stack overflow
  /* forgot where I stole it from, god. */
  for (int i = (sizeof(addr) * 2) - 1; i >= 0 && w < max - 1; i--)
  {
    int dig = (int)((addr >> (i * 4)) & 0xF);
    out[w]  = digs[dig];
    w++;
  }
  out[w] = 0;
  return w;
}

/* Written by yours truly. */
size_t
nv_btoa2(size_t num_bytes, bool upgrade, char out[], size_t max)
{
  size_t written = 0;
  if (upgrade)
  {
    const char* stages[] = { " B", " KB", " MB", " GB", " TB", " PB", " Comically large number of bytes" };
    real_t      b        = (real_t)num_bytes;
    u32         stagei   = 0;

    const size_t num_stages = nv_arrlen(stages) - 1;
    while (b >= 1000.0 && stagei < num_stages)
    {
      stagei++;
      b /= 1000.0;
    }

    written = nv_ftoa2(b, out, 3, max, 1);
    nv_strcat_max(out, stages[stagei], max);
    written += nv_strlen(stages[stagei]);
    written = NV_MIN(written, max);
  }
  else
  {
    written = nv_utoa2(num_bytes, out, 10, max);
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

  // size_t chars_written = nv_vnprintf(NOVA_WBUF_SIZE, args, fmt);
  size_t chars_written = _nv_vsfnprintf(args, stdout, 1, SIZE_MAX, fmt);

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
nv_sprintf(char* dst, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = _nv_vsfnprintf(args, dst, 0, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vprintf(va_list args, const char* fmt)
{
  return _nv_vsfnprintf(args, stdout, 1, SIZE_MAX, fmt);
}

size_t
nv_vfprintf(va_list args, FILE* f, const char* fmt)
{
  return _nv_vsfnprintf(args, f, 1, SIZE_MAX, fmt);
}

size_t
nv_snprintf(char* dst, size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = nv_vsnprintf(args, dst, max_chars, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_nprintf(size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = _nv_vsfnprintf(args, stdout, 1, max_chars, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vnprintf(va_list args, size_t max_chars, const char* fmt)
{
  return _nv_vsfnprintf(args, stdout, 1, max_chars, fmt);
}

size_t
nv_vsnprintf(va_list src, char* dst, size_t max_chars, const char* fmt)
{
  return _nv_vsfnprintf(src, dst, 0, max_chars, fmt);
}

typedef struct nv_format_info_t
{
  va_list*    m_args;
  void*       m_raw_writeptr;
  char*       m_write_string;
  char*       m_wbuf;
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
  char*       m_pad_buf;
} nv_format_info_t;

static inline void
nv_printf_write(nv_format_info_t* info, const char* write_buffer, size_t written)
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
    FILE* file = (FILE*)info->m_raw_writeptr;
    fwrite(write_buffer, 1, to_write, file);
  }
  else
  {
    char** write = (char**)info->m_raw_writeptr;
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
    while (nv_chr_isdigit((unsigned char)*info->m_iter))
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
      while (nv_chr_isdigit((unsigned char)*info->m_iter))
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
    fputc(chr, (FILE*)info->m_raw_writeptr);
  }
  else if (info->m_write_string)
  {
    *info->m_write_string = (char)chr;
    info->m_write_string++;
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
  nv_printf_write(info, string, string_length);
  info->m_wbuffer_used = false;
}

static inline void
nv_printf_format_parse_format(nv_format_info_t* info)
{
  switch (*info->m_iter)
  {
    case 'F':
    case 'f': info->m_written = nv_ftoa2(va_arg(*info->m_args, real_t), info->m_wbuf, info->m_precision, info->m_max_chars - info->m_chars_written, 0); break;

    case 'l':
      if ((info->m_iter + 1) < info->m_fmt_str_end)
      {
        info->m_iter++;
      }

      /* %lF\f is non standard behaviour */
      switch (*info->m_iter)
      {
        case 'd':
        case 'i': info->m_written = nv_itoa2(va_arg(*info->m_args, long), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written); break;
        case 'u': info->m_written = nv_utoa2(va_arg(*info->m_args, unsigned long), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written); break;
      }
      break;
    case 'd':
    case 'i': info->m_written = nv_itoa2(va_arg(*info->m_args, int), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written); break;
    case 'z':
      if ((info->m_iter + 1) < info->m_fmt_str_end)
      {
        info->m_iter++;
      }
      if (*info->m_iter == 'i')
      {
        info->m_written = nv_itoa2(va_arg(*info->m_args, ssize_t), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written);
      }
      else
      {
        info->m_written = nv_utoa2(va_arg(*info->m_args, size_t), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written);
      }
      break;
    case 'u': info->m_written = nv_utoa2(va_arg(*info->m_args, unsigned), info->m_wbuf, 10, info->m_max_chars - info->m_chars_written); break;
    case '#':
      if ((info->m_iter + 1) < info->m_fmt_str_end && (*(info->m_iter + 1) == 'x'))
      {
        info->m_iter++;
        info->m_wbuf[0] = '0';
        info->m_wbuf[1] = 'x';
        info->m_written = 2 + nv_utoa2(va_arg(*info->m_args, unsigned), info->m_wbuf + 2, 16, info->m_max_chars - info->m_chars_written);
      }
      break;
    case 'x': info->m_written = nv_utoa2(va_arg(*info->m_args, unsigned), info->m_wbuf, 16, info->m_max_chars - info->m_chars_written); break;
    case 'p': info->m_written = nv_ptoa2(va_arg(*info->m_args, void*), info->m_wbuf, info->m_max_chars - info->m_chars_written); break;
    /* bytes, custom */
    case 'b': info->m_written = nv_btoa2(va_arg(*info->m_args, size_t), 1, info->m_wbuf, info->m_max_chars - info->m_chars_written); break;
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
      nv_printf_write(info, info->m_pad_buf, chunk);
      info->m_padding -= chunk;
    }
  }

  nv_printf_write(info, info->m_wbuf, info->m_written);

  if (info->m_left_align && info->m_padding > 0)
  {
    nv_memset(info->m_pad_buf, pad_char, sizeof(info->m_pad_buf));
    while (info->m_padding)
    {
      int chunk = (info->m_padding > (int)sizeof(info->m_pad_buf)) ? (int)sizeof(info->m_pad_buf) : info->m_padding;
      nv_printf_write(info, info->m_pad_buf, chunk);
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
    if (info->m_chars_written == 0)
    {
      return;
    }

    if (info->m_file)
    {
      fputc('\b', (FILE*)info->m_raw_writeptr);
    }
    else if (info->m_write_string)
    {
      info->m_write_string--;
      info->m_chars_written--;
    }
  }
  else if (info->m_file)
  {
    if (fputc(*info->m_iter, (FILE*)info->m_raw_writeptr) != *info->m_iter)
    {
      /* we can't use nv_printf here to avoid recursion */
      puts(strerror(errno));
    }
  }
  else if (info->m_write_string)
  {
    *info->m_write_string = *info->m_iter;
    info->m_write_string++;
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
_nv_vsfnprintf(va_list src, void* dst, bool is_file, size_t max_chars, const char* fmt)
{
  nv_assert_and_ret(src != NULL, 0);
  nv_assert_and_ret(fmt != NULL, 0);

  if (max_chars == 0)
  {
    return 0;
  }

  nv_format_info_t info = nv_zero_init(nv_format_info_t);

  va_list args;
  va_copy(args, src);

  /* Aligned to 64 bytes for fast writing and reading access */
  NV_ALIGN_TO(64) char pad_buf[64];

  char wbuf[NOVA_WBUF_SIZE];

  char* writep = (char*)dst;

  info.m_args         = &args;
  info.m_raw_writeptr = is_file ? dst : (void*)&writep;
  info.m_max_chars    = max_chars;
  info.m_precision    = 6;
  info.m_write_string = (char*)dst;
  info.m_fmt_str_end  = fmt + nv_strlen(fmt);
  info.m_iter         = fmt;
  info.m_file         = is_file;
  info.m_pad_buf      = pad_buf;
  info.m_wbuf         = wbuf;

  if (is_file)
  {
    info.m_raw_writeptr = dst; // attach file
  }
  else
  {
    // attach POINTER to the write string so we can iterate over it
    info.m_raw_writeptr = (void*)&info.m_write_string;
  }

  nv_printf_loop(&info);

  if (!is_file && info.m_write_string && max_chars > 0)
  {
    size_t width        = (info.m_chars_written < max_chars) ? info.m_chars_written : max_chars - 1;
    ((char*)dst)[width] = 0;
  }

  va_end(args);

  return info.m_chars_written;
}

// printf

void
_nv_log_error(const char* file, size_t line, const char* func, const char* fmt, ...)
{
  // it was funny while it lasted.
  const char* preceder = " err: ";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 1);
  va_end(args);
}

void
_nv_log_and_abort(const char* file, size_t line, const char* func, const char* fmt, ...)
{
  const char* preceder = " fatal error: ";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 1);
  va_end(args);
  exit(-1);
}

void
_nv_log_warning(const char* file, size_t line, const char* func, const char* fmt, ...)
{
  const char* preceder = " warning: ";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_info(const char* file, size_t line, const char* func, const char* fmt, ...)
{
  const char* preceder = " info: ";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_debug(const char* file, size_t line, const char* func, const char* fmt, ...)
{
  const char* preceder = " debug: ";
  va_list     args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 0);
  va_end(args);
}

void
_nv_log_custom(const char* file, size_t line, const char* func, const char* preceder, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  _nv_log(args, file, line, func, preceder, fmt, 0);
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

size_t
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

// rewritten memcpy
void*
nv_memset(void* dst, char to, size_t sz)
{
  nv_assert(dst != NULL);
  nv_assert(sz != 0);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memset, dst, to, sz);

  uintptr_t d            = (uintptr_t)dst;
  nv_word_t align_offset = d & (sizeof(nv_word_t) - 1);

  // do not ask what the fuck this is.
  // basically, it's used to project a character to a word
  nv_word_t word_to = 0x0101010101010101ULL * (unsigned char)to;

  unsigned char* byte_write = (unsigned char*)dst;
  while (align_offset && sz)
  {
    *byte_write++ = to;
    sz--;
    align_offset = (uintptr_t)byte_write & (sizeof(nv_word_t) - 1);
  }

  nv_word_t* word_write = (nv_word_t*)byte_write;

  // quad word copy
  while (sz >= sizeof(nv_word_t) * 4)
  {
    word_write[0] = word_to;
    word_write[1] = word_to;
    word_write[2] = word_to;
    word_write[3] = word_to;
    word_write += 4;
    sz -= sizeof(nv_word_t) * 4;
  }

  while (sz >= sizeof(nv_word_t))
  {
    *word_write++ = word_to;
    sz -= sizeof(nv_word_t);
  }

  byte_write = (unsigned char*)word_write;
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
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);
  nv_assert_and_ret(sz > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memmove, dst, src, sz);

  unsigned char*       d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;

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
nv_malloc(size_t sz)
{
  nv_assert_and_ret(sz > 0, NULL);

  void* ptr = malloc(sz);
  nv_assert(ptr != NULL);
  return ptr;
}

void*
nv_calloc(size_t sz)
{
  nv_assert_and_ret(sz > 0, NULL);

  void* ptr = calloc(1, sz);
  nv_assert(ptr != NULL);
  return ptr;
}

void*
nv_realloc(void* prevblock, size_t new_sz)
{
  nv_assert_and_ret(new_sz > 0, NULL);

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
  nv_assert_and_ret(p != NULL, NULL);
  nv_assert_and_ret(psize > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memchr, p, chr, psize);

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
  nv_assert_and_ret(_p1 != NULL, -1);
  nv_assert_and_ret(_p2 != NULL, -1);
  nv_assert_and_ret(max > 0, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(memcmp, _p1, _p2, max);

  const uchar* p1 = (const uchar*)_p1;
  const uchar* p2 = (const uchar*)_p2;

  // move && compare the pointer p1 until we reach alignment
  while (max > 0 && IS_NOT_ALIGNED_TO_WORD_SIZE(p1) && IS_NOT_ALIGNED_TO_WORD_SIZE(p2))
  {
    if (*p1 != *p2)
    {
      return *p1 - *p2;
    }
    p1++;
    p2++;
    max--;
  }

  const nv_word_t* w1         = (const nv_word_t*)p1;
  const nv_word_t* w2         = (const nv_word_t*)p2;
  nv_word_t        word_count = max / sizeof(nv_word_t);

  for (nv_word_t i = 0; i < word_count; i++)
  {
    if (w1[i] != w2[i])
    {
      p1 = (const uchar*)&w1[i];
      p2 = (const uchar*)&w2[i];
      break;
    }
  }

  max %= sizeof(nv_word_t);
  p1 += word_count * sizeof(nv_word_t);
  p2 += word_count * sizeof(nv_word_t);

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
  nv_assert_and_ret(src != NULL, 0);
  nv_assert_and_ret(max > 0, 0);

  size_t slen         = nv_strlen(src);
  size_t original_max = max;

  if (!dst)
  {
    return NV_MIN(slen, max);
  }

#  if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_strncpy)
  __builtin_strncpy(dst, src, max);
  return NV_MIN(slen, max);
#  endif

  while (*src && max > 0 && IS_NOT_ALIGNED_TO_WORD_SIZE(src))
  {
    *dst = *src;
    dst++;
    src++;
    max--;
  }

  nv_word_t*       destp = (nv_word_t*)dst;
  const nv_word_t* srcp  = (const nv_word_t*)src;
  size_t           words = max / sizeof(nv_word_t);

  for (size_t i = 0; i < words; i++)
  {
    destp[i] = srcp[i];
  }

  dst += words * sizeof(nv_word_t);
  src += words * sizeof(nv_word_t);
  max %= sizeof(nv_word_t);

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
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcpy, dst, src); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)

  char* const dst_orig = dst;

  while (*src && IS_NOT_ALIGNED_TO_WORD_SIZE(src) && IS_NOT_ALIGNED_TO_WORD_SIZE(dst))
  {
    *dst = *src;
    dst++;
    src++;
  }

  while (1)
  {
    const nv_word_t word = *(const nv_word_t*)src;
    if (WORD_HAS_NULL_TERMINATOR(word))
    {
      break;
    }

    *(nv_word_t*)dst = word;
    dst += sizeof(nv_word_t);
    src += sizeof(nv_word_t);
  }

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
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);

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
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);
  nv_assert_and_ret(max > 0, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strncpy, dst, src, max);

  (void)nv_strncpy2(dst, src, max);
  return dst;
}

char*
nv_strcat(char* dst, const char* src)
{
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);

  char* end = dst + nv_strlen(dst);

  while (*src)
  {
    *end++ = *src++;
  }
  *end = 0;

  return dst;
}

char*
nv_strncat(char* dst, const char* src, size_t max)
{
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);
  nv_assert_and_ret(max > 0, NULL);

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
  nv_assert_and_ret(dst != NULL, 0);
  nv_assert_and_ret(src != NULL, 0);

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
  nv_assert_and_ret(s != NULL, NULL);

  char *begin, *end;
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
  nv_assert_and_ret(s != NULL, NULL);
  nv_assert_and_ret(begin != NULL, NULL);
  nv_assert_and_ret(end != NULL, NULL);

  while (*s && nv_chr_isspace((unsigned char)*s))
  {
    s++;
  }

  *begin = (const char*)s;
  s += nv_strlen(s);

  while (s > *begin && nv_chr_isspace((unsigned char)*(s - 1)))
  {
    s--;
  }

  *end = (char*)s;
  return *begin;
}

int
nv_strcmp(const char* s1, const char* s2)
{
  nv_assert_and_ret(s1 != NULL, -1);
  nv_assert_and_ret(s2 != NULL, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strcmp, s1, s2);

  while (*s1 && *s2 && IS_NOT_ALIGNED_TO_WORD_SIZE(s1) && IS_NOT_ALIGNED_TO_WORD_SIZE(s2))
  {
    if (*s1 != *s2)
    {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    s1++;
    s2++;
  }

  while (1)
  {
    const nv_word_t word_s1 = *(const nv_word_t*)s1;
    const nv_word_t word_s2 = *(const nv_word_t*)s2;

    if (word_s1 != word_s2 || WORD_HAS_NULL_TERMINATOR(word_s1) || WORD_HAS_NULL_TERMINATOR(word_s2))
    {
      break;
    }

    s1 += sizeof(nv_word_t);
    s2 += sizeof(nv_word_t);
  }

  while (*s1 && *s2 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }

  return (unsigned char)*s1 - (unsigned char)*s2;
}

char*
nv_strchr(const char* s, int chr)
{
  nv_assert_and_ret(s != NULL, NULL);

  unsigned char c = (unsigned char)chr;

  while (*s && IS_NOT_ALIGNED_TO_WORD_SIZE(s))
  {
    if (*s == c)
    {
      return (char*)s;
    }
    s++;
  }

  while (1)
  {
    const nv_word_t word = *(const nv_word_t*)s;
    if (WORD_HAS_NULL_TERMINATOR(word) || WORD_HAS_CHARACTER(word, c))
    {
      break;
    }
    s += sizeof(nv_word_t);
  }

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
  nv_assert_and_ret(s != NULL, NULL);

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
  nv_assert_and_ret(s != NULL, NULL);

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
  nv_assert_and_ret(s1 != NULL, -1);
  nv_assert_and_ret(s2 != NULL, -1);
  nv_assert_and_ret(max > 0, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strncmp, s1, s2, max);
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
  nv_assert_and_ret(s1 != NULL, -1);
  nv_assert_and_ret(s2 != NULL, -1);
  nv_assert_and_ret(max > 0, -1);

  size_t i = 0;
  while (*s1 && *s2 && i < max)
  {
    unsigned char c1 = nv_chr_tolower(*(unsigned char*)s1);
    unsigned char c2 = nv_chr_tolower(*(unsigned char*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
    i++;
  }
  return nv_chr_tolower(*(unsigned char*)s1) - nv_chr_tolower(*(unsigned char*)s2);
}

int
nv_strcasecmp(const char* s1, const char* s2)
{
  nv_assert_and_ret(s1 != NULL, -1);
  nv_assert_and_ret(s2 != NULL, -1);

  while (*s1 && *s2)
  {
    unsigned char c1 = nv_chr_tolower(*(unsigned char*)s1);
    unsigned char c2 = nv_chr_tolower(*(unsigned char*)s2);
    if (c1 != c2)
    {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return nv_chr_tolower(*(unsigned char*)s1) - nv_chr_tolower(*(unsigned char*)s2);
}

size_t
nv_strlen(const char* s)
{
  nv_assert_and_ret(s != NULL, -1);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strlen, s);

  const char* start = s;

  while ((uintptr_t)s & (sizeof(nv_word_t) - 1)) // align s to 8 byte boundary so we can check sizeof(size_t) bytes at once
  {
    if (!*s)
    {
      return s - start;
    }
    s++;
  }

  const nv_word_t mask = 0x0101010101010101ULL;
  while (1)
  {
    nv_word_t word = *(nv_word_t*)s;
    if (((word - mask) & ~word) & (mask << 7))
    {
      break;
    }
    s += sizeof(nv_word_t);
  }

  while (*s)
  {
    s++;
  }

  return s - start;
}

size_t
nv_strnlen(const char* s, size_t max)
{
  nv_assert_and_ret(s != NULL, 0);
  nv_assert_and_ret(max > 0, 0);

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
  nv_assert_and_ret(s != NULL, NULL);
  nv_assert_and_ret(sub != NULL, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strstr, s, sub);

  for (; *s; s++)
  {
    const char* _s = s;
    const char* p  = sub;

    while (*_s && *p && *_s == *p)
    {
      _s++;
      p++;
    }

    if (!*p)
    {
      return (char*)s;
    }
  }

  return NULL;
}

char*
nv_strlcpy(char* dst, const char* src, size_t dst_size)
{
  nv_assert_and_ret(dst != NULL, NULL);
  nv_assert_and_ret(src != NULL, NULL);

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
  nv_assert_and_ret(src != NULL, (size_t)-1);

  size_t slen = nv_strlen(src);
  if (!dst)
  {
    return slen;
  }

#  if NOVA_STRING_USE_BUILTIN && defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_strcpy)
  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strlen, __builtin_strcpy(dst, src)); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
#  endif

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
  nv_assert_and_ret(s != NULL, (size_t)-1);
  nv_assert_and_ret(accept != NULL, (size_t)-1);

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
  nv_assert_and_ret(s != NULL, (size_t)-1);
  nv_assert_and_ret(reject != NULL, (size_t)-1);

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
  nv_assert_and_ret(s1 != NULL, NULL);
  nv_assert_and_ret(s2 != NULL, NULL);

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
  nv_assert_and_ret(context != NULL, NULL);

  if (!s)
  {
    s = *context;
  }
  char* p;

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
    *context = nv_strchr(s, 0); // get pointer to last char
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
nv_strdup(const char* s)
{
  nv_assert_and_ret(s != NULL, NULL);

  NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE(strdup, s);

  size_t slen  = nv_strlen(s);
  char*  new_s = nv_malloc(slen + 1);
  nv_strlcpy(new_s, s, slen + 1);
  return new_s;
}

char*
nv_substr(const char* s, size_t start, size_t len)
{
  nv_assert_and_ret(s != NULL, NULL);
  nv_assert_and_ret(len > 0, NULL);

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
  nv_assert_and_ret(str != NULL, NULL);

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
  nv_assert_and_ret(str != NULL, NULL);
  nv_assert_and_ret(max != 0, NULL);

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
      *eq   = 0;
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
      flag_value = nv_atobool(value, NOVA_MAX_IGNORE);
    }
    if (opt->m_value)
    {
      *(bool*)opt->m_value = flag_value;
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
      case NV_OP_TYPE_STRING: nv_strlcpy((char*)opt->m_value, value, opt->m_buffer_size); break;
      case NV_OP_TYPE_INT: *(int*)opt->m_value = (int)nv_atoi(value, NOVA_MAX_IGNORE); break;
      case NV_OP_TYPE_FLOAT: *(flt_t*)opt->m_value = (flt_t)nv_atof(value, NOVA_MAX_IGNORE); break;
      case NV_OP_TYPE_DOUBLE: *(real_t*)opt->m_value = nv_atof(value, NOVA_MAX_IGNORE); break;
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

void
nv_error_queue_init(nv_error_queue_t* dst)
{
  dst->m_front = -1;
  dst->m_back  = -1;
}

void
nv_error_queue_destroy(nv_error_queue_t* dst)
{
  (void)dst;
}

char*
nv_error_queue_push(nv_error_queue_t* queue)
{
  if ((queue->m_back + 1) % NV_MAX_ERRORS == queue->m_front)
  {
    const char* overwritten_error = nv_error_queue_pop(queue);
    if (overwritten_error)
    {
      nv_printf("queue full: poppd error '%s'\n", overwritten_error);
    }
  }

  if (queue->m_front == -1)
  {
    queue->m_front = 0;
  }
  queue->m_back = (queue->m_back + 1) % NV_MAX_ERRORS;

  char* ret = queue->m_errors[queue->m_back];
  nv_memset(ret, 0, NV_ERROR_LENGTH);
  return ret;
}

const char*
nv_error_queue_pop(nv_error_queue_t* queue)
{
  if (queue->m_front == -1)
  {
    return NULL;
  }

  const char* value = queue->m_errors[queue->m_front];

  if (queue->m_front == queue->m_back)
  {
    queue->m_front = queue->m_back = -1;
  }
  else
  {
    queue->m_front = (queue->m_front + 1) % NV_MAX_ERRORS;
  }

  return value; // return the popped value
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
    return chr + 'a';
  }
  return chr;
}

int
nv_chr_toupper(int chr)
{
  if (nv_chr_islower(chr))
  {
    return chr - ('a' - 'A'); /* chr - 32 */
  }
  return chr;
}

#endif
