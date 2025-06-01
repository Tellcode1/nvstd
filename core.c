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

#include "format.h"
#include "rand.h"
#include "stdafx.h"

#include "alloc.h"
#include "chrclass.h"
#include "errorcodes.h"
#include "file.h"
#include "image.h"
#include "math/math.h"
#include "print.h"
#include "props.h"
#include "strconv.h"
#include "string.h"
#include "sysalloc.h"
#include "types.h"

#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>

#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>

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

#define ALLOC_ALLOC_CONDITION (old_size == NV_ALLOC_NEW_BLOCK)
#define ALLOC_FREE_CONDITION (new_size == NV_ALLOC_FREE)
#define ALLOC_REALLOC_CONDITION (!(ALLOC_ALLOC_CONDITION) && !(ALLOC_FREE_CONDITION))

void*
nv_allocator_c(void* user_data, void* old_ptr, size_t old_size, size_t new_size)
{
  (void)user_data;

  if (ALLOC_ALLOC_CONDITION)
  {
    return nv_calloc(new_size);
  }
  else if (ALLOC_FREE_CONDITION)
  {
    nv_free(old_ptr);
    return NULL;
  }
  else if (ALLOC_REALLOC_CONDITION)
  {
    nv_assert_else_return(new_size != 0, NULL);
    return nv_realloc(old_ptr, new_size);
  }
  else
  {
    nv_log_error("Invalid operation\n");
    return NULL;
  }

  return NULL;
}

void*
nv_allocator_estack(void* user_data, void* old_ptr, size_t old_size, size_t new_size)
{
  nv_alloc_estack_t* estack = (nv_alloc_estack_t*)user_data;

  if (ALLOC_ALLOC_CONDITION)
  {
    nv_assert_else_return(old_ptr == NULL, NULL);
    nv_assert_else_return(new_size > 0, NULL);
  }
  // free || realloc
  else if (ALLOC_REALLOC_CONDITION || ALLOC_FREE_CONDITION) {}
  else
  {
    nv_log_error("Invalid operation\n");
    return NULL;
  }

  /**
   * Stack integrity checks
   */

  nv_assert_else_return(estack->buffer_size != 0, NULL);
  nv_assert_else_return(estack->buffer_bumper < estack->buffer_size, NULL);

  // malloc or realloc && OOM
  if ((ALLOC_ALLOC_CONDITION || ALLOC_REALLOC_CONDITION) && (estack->buffer_bumper + new_size) > (estack->buffer_size))
  {
    /**
     * If we were already using a heap buffer, then we do need to free it
     */
    if (estack->using_heap_buffer)
    {
      nv_free(estack->buffer);
      estack->buffer = NULL;
    }

    const size_t new_buffer_size = NV_MAX(estack->buffer_size * 2, estack->buffer_bumper + new_size);
    nv_assert_else_return(new_buffer_size != 0, NULL);

    uchar* new_buffer = nv_calloc(new_buffer_size);

    /**
     * Copy all the memory from the old stack
     */
    nv_memmove(new_buffer, estack->buffer, estack->buffer_size);

    estack->buffer            = new_buffer;
    estack->buffer_size       = new_buffer_size;
    estack->using_heap_buffer = true;
  }

  if (ALLOC_ALLOC_CONDITION)
  {
    if ((estack->buffer_bumper + new_size) > (estack->buffer_size))
    {
      // OOM
      return NULL;
    }

    void* allocation = estack->buffer + estack->buffer_bumper;
    nv_memset(allocation, 0, new_size);

    // This can be checked later and reduce the bumper
    estack->last_allocation = (uchar*)allocation;

    /**
     * Move the bumper by the new size
     * buffer+bumper will now point to the new block
     */
    estack->buffer_bumper += new_size;

    return allocation;
  }
  else if (ALLOC_FREE_CONDITION)
  {
    if ((unsigned char*)old_ptr == estack->last_allocation)
    {
      estack->buffer_bumper -= old_size;
    }
    return NULL;
  }
  else if (ALLOC_REALLOC_CONDITION)
  {
    if ((unsigned char*)old_ptr != estack->last_allocation)
    {
      // cant realloc non-last allocation
      return NULL;
    }

    if (new_size > old_size)
    {
      if ((estack->buffer_bumper + (new_size - old_size)) > estack->buffer_size)
      {
        // OOM
        return NULL;
      }

      estack->buffer_bumper += (new_size - old_size);
      return old_ptr; // memory is contiguous, extended
    }
    else if (new_size < old_size)
    {
      estack->buffer_bumper -= (old_size - new_size);
      return old_ptr; // shrunk in place
    }

    // sizes are equal, nothing changes
    return old_ptr;
  }

  return NULL;
}

void
nv_print_time_as_string(FILE* stream)
{
  nv_assert(0);
  (void)stream;
}

void
_nv_core_log(const char* file, size_t line, const char* fn, const char* preceder, bool err, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  nv_log_va(file, line, fn, preceder, err, fmt, args);

  va_end(args);
}

void
nv_log_va(const char* file, size_t line, const char* fn, const char* preceder, bool err, const char* fmt, va_list args)
{
  FILE* out = (err) ? stderr : stdout;

  /* Two fprintf calls, good. */

  struct tm* time = _nv_get_time();
  nv_fprintf(out, "[%d:%d:%d] [%s:%zu]%s%s(): ", time->tm_hour % 12, time->tm_min, time->tm_sec, nv_basename(file), line, preceder, fn);

  nv_vfprintf(args, out, fmt);
}

// printf

size_t
nv_itoa2(intmax_t num, char out[], int base, size_t max, bool add_commas)
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

  size_t   dig_count = 0;
  intmax_t temp      = num;
  while (temp > 0)
  {
    dig_count++;
    temp /= base;
  }

  size_t loop_digits_written = 0;

  do
  {
    if (i >= max)
    {
      break;
    }

    if (add_commas && NOVA_SEPERATOR_CHAR && dig_count > 3 && loop_digits_written > 0 && (dig_count - loop_digits_written) % 3 == 0)
    {
      if (i < max)
      {
        out[i] = NOVA_SEPERATOR_CHAR;
        i++;
      }
      else
      {
        break;
      }
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
    loop_digits_written++;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

size_t
nv_utoa2(uintmax_t num, char out[], int base, size_t max, bool add_commas)
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

  size_t    dig_count = 0;
  uintmax_t temp      = num;
  while (temp > 0)
  {
    dig_count++;
    temp /= base;
  }

  /* highest power of base that is <= x */
  uintmax_t highest_power_of_base = 1;
  while (highest_power_of_base <= num / (uintmax_t)base)
  {
    highest_power_of_base *= base;
  }

  size_t loop_digits_written = 0;
  do
  {
    if (i >= max)
    {
      break;
    }

    if (add_commas && NOVA_SEPERATOR_CHAR && dig_count > 3 && loop_digits_written > 0 && (dig_count - loop_digits_written) % 3 == 0)
    {
      if (i < max)
      {
        out[i] = NOVA_SEPERATOR_CHAR;
        i++;
      }
      else
      {
        break;
      }
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
    loop_digits_written++;
  } while (highest_power_of_base > 0);

  out[i] = 0;
  return i;
}

#define NOVA_FTOA_HANDLE_CASE(fn, n, str)                                                                                                                                     \
  if (fn(n))                                                                                                                                                                  \
  {                                                                                                                                                                           \
    if (signbit(n) == 0)                                                                                                                                                      \
      return nv_strncpy2(out, str, max);                                                                                                                                      \
    else                                                                                                                                                                      \
      return nv_strncpy2(out, "-" str, max);                                                                                                                                  \
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

  itr += nv_utoa2(int_part, itr, 10, max - 1, false);

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

#define NV_SKIP_WHITSPACE(s) nv_strtrim_c(s, &s, NULL);

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

  u64        addr   = (u64)ptr;
  const char digs[] = "0123456789abcdef";

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
    written = nv_utoa2(num_bytes, out, 10, max, true);
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

#define NV_PRINTF_PEEK_FMT() ((info->itr < info->fmt_str_end) ? *info->itr : 0)
#define NV_PRINTF_PEEK_NEXT_FMT() (((info->itr + 1) < info->fmt_str_end) ? *(info->itr + 1) : 0)
#define NV_PRINTF_ADVANCE_FMT()                                                                                                                                               \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    if ((info->itr + 1) < info->fmt_str_end)                                                                                                                                  \
    {                                                                                                                                                                         \
      info->itr++;                                                                                                                                                            \
    }                                                                                                                                                                         \
    else                                                                                                                                                                      \
    {                                                                                                                                                                         \
      info->itr = info->fmt_str_end;                                                                                                                                          \
    }                                                                                                                                                                         \
  } while (0);
#define NV_PRINTF_ADVANCE_NUM_CHARACTERS_FMT(num_chars)                                                                                                                       \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    if ((info->itr + (num_chars)) < info->fmt_str_end)                                                                                                                        \
    {                                                                                                                                                                         \
      info->itr += (num_chars);                                                                                                                                               \
    }                                                                                                                                                                         \
    else                                                                                                                                                                      \
    {                                                                                                                                                                         \
      info->itr = info->fmt_str_end;                                                                                                                                          \
    }                                                                                                                                                                         \
  } while (0);

typedef struct nv_format_info_t
{
  va_list     args;
  void*       dst_file;
  char*       dst_string;
  char*       tmp_writebuffer;
  const char* fmt_str_end;
  const char* itr;
  size_t      chars_written;
  size_t      written;
  size_t      max_chars;
  int         padding;
  int         padding_w;
  int         precision;
  bool        pad_zero;
  bool        left_align;
  bool        file;
  bool        wbuffer_used;
  bool        precision_specified;
  char*       pad_buf;
} nv_format_info_t;

static inline void
_nv_printf_write(nv_format_info_t* info, const char* write_buffer, size_t written)
{
  if (info->chars_written >= info->max_chars)
  {
    return;
  }

  size_t remaining = info->max_chars - info->chars_written;
  size_t to_write  = (written > remaining) ? remaining : written;
  info->chars_written += to_write;

  /* As we need to return the number of characters printf would have returned, we can't exit before here. */
  if (!write_buffer)
  {
    return;
  }

  if (info->file)
  {
    FILE* file = (FILE*)info->dst_file;
    fwrite(write_buffer, 1, to_write, file);
  }
  else if (info->dst_string)
  {
    if (to_write > 0)
    {
      nv_memcpy(info->dst_string, write_buffer, to_write);
      info->dst_string += to_write;
    }
  }
}

static inline void
_nv_printf_get_padding_and_precision_if_given(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_FMT())
  {
    return;
  }

  if (NV_PRINTF_PEEK_FMT() == '-')
  {
    info->left_align = true;
    NV_PRINTF_ADVANCE_FMT();
  }
  if (NV_PRINTF_PEEK_FMT() == '0')
  {
    info->pad_zero = true;
    NV_PRINTF_ADVANCE_FMT();
  }

  if (NV_PRINTF_PEEK_FMT() == '*')
  {
    info->padding_w = va_arg(info->args, int);
    if (info->padding_w < 0)
    {
      // negative width means left align
      info->left_align = true;
      info->padding_w  = -info->padding_w;
    }
    else
    {
      /* Positive padding width means right padded */
      info->left_align = false;
    }
    NV_PRINTF_ADVANCE_FMT();
  }
  else
  {
    while (nv_chr_isdigit(NV_PRINTF_PEEK_FMT()))
    {
      info->padding_w = info->padding_w * 10 + (NV_PRINTF_PEEK_FMT() - '0');
      NV_PRINTF_ADVANCE_FMT();
    }
  }

  if (NV_PRINTF_PEEK_FMT() == '.')
  {
    NV_PRINTF_ADVANCE_FMT();
    info->precision_specified = 1;
    if (NV_PRINTF_PEEK_FMT() == '*')
    {
      info->precision = va_arg(info->args, int);
      if (info->precision < 0)
      {
        info->precision = 6;
      }
      NV_PRINTF_ADVANCE_FMT();
    }
    else
    {
      info->precision = 0;
      while (nv_chr_isdigit(NV_PRINTF_PEEK_FMT()))
      {
        info->precision = info->precision * 10 + (NV_PRINTF_PEEK_FMT() - '0');
        NV_PRINTF_ADVANCE_FMT();
      }
    }
  }
}

static inline void
_nv_printf_format_process_char(nv_format_info_t* info)
{
  if (info->chars_written >= info->max_chars - 1)
  {
    return;
  }

  // if user is asking for literal % sign, *iter will be the percent sign!!
  int chr = (nv_chr_tolower(NV_PRINTF_PEEK_FMT()) == 'c') ? va_arg(info->args, int) : NV_PRINTF_PEEK_FMT();
  if (info->file)
  {
    fputc(chr, (FILE*)info->dst_file);
  }
  else if (info->dst_string)
  {
    *info->dst_string = (char)chr;
    info->dst_string++;
  }
  info->wbuffer_used = false;
  info->chars_written++;
}

static inline void
_nv_printf_handle_long_type(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_NEXT_FMT() || NV_PRINTF_PEEK_FMT() != 'l')
  {
    return;
  }

  const bool type_is_long_long_integer = nv_chr_tolower(NV_PRINTF_PEEK_NEXT_FMT()) == 'l';
  if (type_is_long_long_integer)
  {
    /* Move past the extra l */
    NV_PRINTF_ADVANCE_FMT();
    NV_PRINTF_ADVANCE_FMT();

    /* long long integers */
    switch (nv_chr_tolower(NV_PRINTF_PEEK_FMT()))
    {
      case 'd':
      case 'i': info->written = nv_itoa2(va_arg(info->args, long long), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS); break;
      case 'u':
        info->written = nv_utoa2(va_arg(info->args, unsigned long long), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS);
        break;
    }
  }
  else
  {
    NV_PRINTF_ADVANCE_FMT();
    /* standard long integers */
    /* %lF\f is non standard behaviour */
    switch (nv_chr_tolower(NV_PRINTF_PEEK_FMT()))
    {
      case 'd':
      case 'i': info->written = nv_itoa2(va_arg(info->args, long), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS); break;
      case 'u': info->written = nv_utoa2(va_arg(info->args, unsigned long), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS); break;
    }
  }
}

static inline void
_nv_printf_handle_size_type(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_NEXT_FMT() || NV_PRINTF_PEEK_FMT() != 'z')
  {
    return;
  }

  if ((nv_chr_tolower(NV_PRINTF_PEEK_NEXT_FMT()) == 'i' || nv_chr_tolower(NV_PRINTF_PEEK_NEXT_FMT()) == 'u'))
  {
    NV_PRINTF_ADVANCE_FMT();
  }

  if (nv_chr_tolower(NV_PRINTF_PEEK_FMT()) == 'i')
  {
    info->written = nv_itoa2(va_arg(info->args, ssize_t), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS);
  }
  else
  {
    info->written = nv_utoa2(va_arg(info->args, size_t), info->tmp_writebuffer, 10, info->max_chars - info->chars_written, NOVA_PRINTF_ADD_COMMAS);
  }
}

static inline void
_nv_printf_handle_hash_prefix_and_children(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_NEXT_FMT() || NV_PRINTF_PEEK_FMT() != '#')
  {
    return;
  }

  info->tmp_writebuffer[0] = '0';

  /* If the character is X, then the prefix needs to have X. If it is x, then teh prefix needs to have x */
  info->tmp_writebuffer[1] = NV_PRINTF_PEEK_NEXT_FMT();

  /* Move past x\X */
  NV_PRINTF_ADVANCE_FMT();

  /* The 0x\0X prefix */
  info->written = 2;
  info->written += nv_utoa2(va_arg(info->args, unsigned), info->tmp_writebuffer + 2, 16, info->max_chars - info->chars_written, false);
}

static inline void
_nv_printf_format_parse_string(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_FMT() || NV_PRINTF_PEEK_FMT() != 's')
  {
    return;
  }

  const char* string = va_arg(info->args, const char*);
  /* strlen(NULL) is not ok */
  if (!string)
  {
    string = "(null)";
  }

  size_t string_length = 0;
  if (info->precision_specified)
  {
    string_length = info->precision;
  }
  else
  {
    string_length = nv_strlen(string);
  }

  _nv_printf_write(info, string, string_length);
  info->wbuffer_used = false;
}

static inline void
_nv_printf_format_parse_format(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_FMT())
  {
    return;
  }

  size_t remaining = info->max_chars - info->chars_written;

  switch (nv_chr_tolower(NV_PRINTF_PEEK_FMT()))
  {
    /* By default, ftoa should not trim trailing zeroes. */
    case 'f': info->written = nv_ftoa2(va_arg(info->args, real_t), info->tmp_writebuffer, info->precision, remaining, false); break;
    case 'l': _nv_printf_handle_long_type(info); break;
    case 'd':
    case 'i': info->written = nv_itoa2(va_arg(info->args, int), info->tmp_writebuffer, 10, remaining, NOVA_PRINTF_ADD_COMMAS); break;
    case 'u': info->written = nv_utoa2(va_arg(info->args, unsigned), info->tmp_writebuffer, 10, remaining, NOVA_PRINTF_ADD_COMMAS); break;

    /* size_t based formats. This is standard. */
    case 'z': _nv_printf_handle_size_type(info); break;

    /* hash tells us that we need to have the 0x prefix (or the 0X prefix) */
    case '#': _nv_printf_handle_hash_prefix_and_children(info); break;

    /* hex integer */
    case 'x': info->written = nv_utoa2(va_arg(info->args, unsigned), info->tmp_writebuffer, 16, info->max_chars - info->chars_written, false); break;

    /* pointer */
    case 'p': info->written = nv_ptoa2(va_arg(info->args, void*), info->tmp_writebuffer, remaining); break;

    /* bytes, custom */
    case 'b': info->written = nv_btoa2(va_arg(info->args, size_t), 1, info->tmp_writebuffer, remaining); break;

    case 's': _nv_printf_format_parse_string(info); break;

    case 'c':
    /* If there are two % symbols in a row */
    case '%':
    default: _nv_printf_format_process_char(info); break;
  }
}

static inline void
_nv_printf_write_padding(nv_format_info_t* info)
{
  info->padding = info->padding_w - (int)info->written;
  char pad_char = info->pad_zero ? '0' : ' ';
  if (info->left_align && info->padding <= 0)
  {
    return;
  }

  /* No, we cannot run out of padding buffer space, because we write it in chunks. */

  nv_memset(info->pad_buf, pad_char, sizeof(info->pad_buf));
  while (info->padding > 0)
  {
    int chunk = (info->padding > (int)sizeof(info->pad_buf)) ? (int)sizeof(info->pad_buf) : info->padding;
    _nv_printf_write(info, info->pad_buf, chunk);
    info->padding -= chunk;
  }
}

static inline void
_nv_printf_format_upload_to_destination(nv_format_info_t* info)
{
  if (!info->wbuffer_used)
  {
    return;
  }

  _nv_printf_write_padding(info);

  _nv_printf_write(info, info->tmp_writebuffer, info->written);

  _nv_printf_write_padding(info);
}

static inline void
_nv_printf_format_parse_format_specifier(nv_format_info_t* info)
{
  info->wbuffer_used        = true;
  info->padding_w           = 0;
  info->precision           = 6;
  info->precision_specified = 0;

  _nv_printf_get_padding_and_precision_if_given(info);

  _nv_printf_format_parse_format(info);

  _nv_printf_format_upload_to_destination(info);
}

static inline void
_nv_printf_write_iterated_char(nv_format_info_t* info)
{
  if (info->chars_written >= info->max_chars - 1)
  {
    return;
  }

  if (NV_PRINTF_PEEK_FMT() == '\b')
  {
    if (info->chars_written == 0)
    {
      return;
    }

    if (info->file)
    {
      fputc('\b', (FILE*)info->dst_file);
    }
    else if (info->dst_string)
    {
      info->dst_string--;
      info->chars_written--;
    }
  }
  else if (info->file)
  {
    if (fputc(NV_PRINTF_PEEK_FMT(), (FILE*)info->dst_file) != NV_PRINTF_PEEK_FMT())
    {
      /* we can't use nv_printf here to avoid recursion */
      puts(strerror(errno));
    }
  }
  else if (info->dst_string)
  {
    *info->dst_string = NV_PRINTF_PEEK_FMT();
    info->dst_string++;
  }
  info->chars_written++;
}

static inline void
_nv_printf_loop(nv_format_info_t* info)
{
  for (; NV_PRINTF_PEEK_FMT() && info->chars_written < info->max_chars; info->itr++)
  {
    if (NV_PRINTF_PEEK_FMT() == '%')
    {
      NV_PRINTF_ADVANCE_FMT();
      _nv_printf_format_parse_format_specifier(info);
    }
    else
    {
      _nv_printf_write_iterated_char(info);
    }
  }
}

size_t
_nv_vsfnprintf(va_list args, void* dst, bool is_file, size_t max_chars, const char* fmt)
{
  if (max_chars == 0)
  {
    return 0;
  }

  nv_format_info_t info = nv_zero_init(nv_format_info_t);

  /* Aligned to 64 bytes for fast writing and reading access */
  NV_ALIGN_TO(64) char pad_buf[64];

  char wbuf[NOVA_WBUF_SIZE];

  info.dst_file        = is_file ? (FILE*)dst : NULL;
  info.dst_string      = is_file ? NULL : (char*)dst;
  info.max_chars       = max_chars;
  info.precision       = 6;
  info.fmt_str_end     = fmt + nv_strlen(fmt);
  info.itr             = fmt;
  info.file            = is_file;
  info.pad_buf         = pad_buf;
  info.tmp_writebuffer = wbuf;

  va_copy(info.args, args);

  _nv_printf_loop(&info);

  if (!is_file && info.dst_string && max_chars > 0)
  {
    size_t width        = (info.chars_written < max_chars) ? info.chars_written : max_chars - 1;
    ((char*)dst)[width] = 0;
  }

  va_end(info.args);

  return info.chars_written;
}

// printf

#include <zlib.h>

int
nv_bufcompress(const void* NV_RESTRICT input, size_t input_size, void* NV_RESTRICT output, size_t* NV_RESTRICT output_size)
{
  z_stream stream = nv_zero_init(z_stream);

  if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
  {
    return -1;
  }

  stream.next_in  = (uchar*)input;
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
  strm.next_in   = (uchar*)compressed_data;
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
_align_up(size_t sz, size_t align)
{
  return (sz + (align - 1)) & ~(align - 1);
}

void*
nv_aligned_alloc(size_t size, size_t alignment)
{
  nv_assert_else_return((alignment & (alignment - 1)) == 0, NULL);
  nv_assert_else_return(size > 0, NULL);

  const size_t total_size = _align_up(size + sizeof(void*) + sizeof(size_t), alignment);

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
  else
  {
    nv_log_error("double free %p\n", orig);
    abort();
  }

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

static inline const nv_option_t*
nv_option_find(const nv_option_t* options, int noptions, const char* short_name, const char* long_name)
{
  nv_assert(noptions >= 0);

  size_t i = 0;
  for (const nv_option_t* opt = options; i < (size_t)noptions; opt++, i++)
  {
    if (short_name && opt->short_name && nv_strcmp(opt->short_name, short_name) == 0)
    {
      return opt;
    }
    if (long_name && opt->long_name && nv_strcmp(opt->long_name, long_name) == 0)
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

    const char* short_name = opt->short_name ? opt->short_name : "<empty>";
    const char* long_name  = opt->long_name ? opt->long_name : "<empty>";

    written = nv_snprintf(buf, available, "\t-%s, --%s <%s>\n", short_name, long_name, nv_props_get_tp_name(opt->type));
    buf += written;
    if (written > available)
    {
      break;
    }
    available -= written;
  }
}

static inline nv_error
_nv_props_parse_arg(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size, int* i)
{
  nv_assert_else_return(argc != 0, NV_ERROR_INVALID_ARG);

  nv_assert_else_return(noptions > 0, NV_ERROR_INVALID_ARG);

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
    if (error && error_size > 0)
    {
      nv_snprintf(error, error_size, "unknown option: %s%s", is_long ? "--" : "-", name);
    }
    (*i)++;
    return -1;
  }

  if (opt->type == NV_OP_TYPE_BOOL)
  {
    bool flag_value = true;
    if (is_long && value)
    {
      flag_value = nv_atobool(value, NOVA_MAX_IGNORE);
    }
    if (opt->value)
    {
      *(bool*)opt->value = flag_value;
    }
    (*i)++;
    return 0;
  }

  if (!value)
  {
    if (!is_long)
    {
      size_t opt_name_len = nv_strlen(opt->short_name);
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
        if (error && error_size > 0)
        {
          nv_snprintf(error, error_size, "option --%s requires a value", name);
        }
        return NV_ERROR_INVALID_INPUT;
      }
      value = argv[*i];
    }
  }

  if (opt->value)
  {
    switch (opt->type)
    {
      case NV_OP_TYPE_STRING: nv_strlcpy((char*)opt->value, value, opt->buffer_size); break;
      case NV_OP_TYPE_INT: *(int*)opt->value = (int)nv_atoi(value, NOVA_MAX_IGNORE); break;
      case NV_OP_TYPE_FLOAT: *(flt_t*)opt->value = (flt_t)nv_atof(value, NOVA_MAX_IGNORE); break;
      case NV_OP_TYPE_DOUBLE: *(real_t*)opt->value = nv_atof(value, NOVA_MAX_IGNORE); break;
      default: break;
    }
  }

  (*i)++;
  return NV_ERROR_SUCCESS;
}

nv_error
nv_props_parse(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size)
{
  nv_assert(argc != 0);
  nv_assert(argv != NULL);
  nv_assert(options != NULL);
  nv_assert(noptions > 0);

  int      i       = 1; // program name is argv[0]
  nv_error errcode = NV_ERROR_SUCCESS;

  while (i < argc)
  {
    char* arg = argv[i];
    if (arg[0] == '-')
    {
      nv_error result = _nv_props_parse_arg(argc, argv, options, noptions, error, error_size, &i);
      if (result != 0)
      {
        errcode = false;
      }
    }
    else
    {
      break;
    }
  }
  return errcode;
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

nv_error
nv_image_load(const char* path, nv_image_t* dst)
{
  nv_bzero(dst, sizeof(nv_image_t));

  SDL_Surface* surface = IMG_Load(path);
  nv_assert_and_exec(surface != NULL, { nv_log_error("load failed $?(%s)\n", SDL_GetError()); });

  *dst = _nv_sdl_surface_to_image(surface);

  return NV_SUCCESS;
}

unsigned char*
nv_image_pad_channels(const nv_image_t* src, size_t dst_channels)
{
  const size_t src_channels = nv_format_get_num_channels(src->format);
  nv_assert(src_channels < dst_channels);

  uint8_t* dst = nv_calloc(src->width * src->height * dst_channels * sizeof(uchar));

  for (size_t y = 0; y < src->height; y++)
  {
    for (size_t x = 0; x < src->width; x++)
    {
      for (size_t c = 0; c < dst_channels; c++)
      {
        if (c < src_channels)
        {
          dst[(((y * src->width) + x) * dst_channels) + c] = src->data[(((y * src->width) + x) * src_channels) + c];
        }
        else
        {
          if (c == 3)
          { // alpha channel
            dst[(((y * src->width) + x) * dst_channels) + c] = __UINT8_MAX__;
          }
          else
          {
            dst[(((y * src->width) + x) * dst_channels) + c] = 0;
          }
        }
      }
    }
  }

  return dst;
}

bool
nv_image_overlay(nv_image_t* dst, const nv_image_t* src, int dst_x_offset, int dst_y_offset, int src_x_offset, int src_y_offset)
{
  nv_assert(dst != NULL);
  nv_assert(src != NULL);

  const int src_channels = nv_format_get_num_channels(src->format);

  for (ssize_t y = src_y_offset; y < (ssize_t)src->height; y++)
  {
    for (ssize_t x = src_x_offset; x < (ssize_t)src->width; x++)
    {
      ssize_t dst_x = dst_x_offset + (x - src_x_offset);
      ssize_t dst_y = dst_y_offset + (y - src_y_offset);

      if (dst_x >= 0 && dst_x < (ssize_t)dst->width && dst_y >= 0 && dst_y < (ssize_t)dst->height)
      {
        size_t src_i = (y * src->width + x) * src_channels;
        size_t dst_i = (dst_y * dst->width + dst_x) * src_channels;

        for (int c = 0; c < src_channels; c++)
        {
          dst->data[dst_i + c] = src->data[src_i + c];
        }
      }
    }
  }

  return 0;
}

void
nv_image_enlarge(nv_image_t* dst, const nv_image_t* src, size_t scale)
{
  size_t new_w = src->width * scale;

  nv_assert(dst->data != NULL);

  uchar*       write = dst->data;
  const uchar* read  = src->data;

  size_t bpp = nv_format_get_bytes_per_pixel(src->format); // bytes per pixel
  for (size_t y = 0; y < src->height; y++)
  {
    for (size_t x = 0; x < src->width; x++)
    {
      size_t src_i = (y * src->width + x) * bpp;
      for (size_t i = 0; i < scale; i++)
      {
        for (size_t j = 0; j < scale; j++)
        {
          size_t dst_i = ((y * scale + i) * new_w + (x * scale + j)) * bpp;
          for (size_t c = 0; c < bpp; c++)
          {
            write[dst_i + c] = read[src_i + c];
          }
        }
      }
    }
  }
}

void
nv_image_bilinear_filter(nv_image_t* dst, const nv_image_t* src, flt_t scale)
{
  const int nchannels = nv_format_get_num_channels(src->format);

  dst->width  = (size_t)((flt_t)src->width / scale);
  dst->height = (size_t)((flt_t)src->width / scale);
  dst->format = src->format;
  dst->data   = nv_calloc(dst->width * dst->height * nv_format_get_bytes_per_pixel(dst->format));

  // Calculate the ratios for x and y coordinates
  flt_t x_ratio, y_ratio;
  if (dst->width > 1)
  {
    x_ratio = ((flt_t)src->width - 1.0F) / ((flt_t)dst->width - 1.0F);
  }
  else
  {
    x_ratio = 0;
  }

  if (dst->height > 1)
  {
    y_ratio = ((flt_t)src->height - 1.0F) / ((flt_t)dst->height - 1.0F);
  }
  else
  {
    y_ratio = 0;
  }

  for (size_t y = 0; y < dst->height; y++)
  {
    const flt_t ratiod_y = y_ratio * (flt_t)y;
    flt_t       y_l      = floorf(ratiod_y);
    flt_t       y_h      = ceilf(ratiod_y);
    flt_t       y_weight = (ratiod_y)-y_l;

    const size_t y_l_offset = (size_t)y_l * src->width * nchannels;
    const size_t y_h_offset = (size_t)y_h * src->width * nchannels;

    for (size_t x = 0; x < dst->width; x++)
    {
      const flt_t ratiod_x = x_ratio * (flt_t)x;

      flt_t x_l      = floorf(ratiod_x);
      flt_t x_h      = ceilf(ratiod_x);
      flt_t x_weight = (ratiod_x)-x_l;

      const size_t x_l_offset = (size_t)x_l * nchannels;
      const size_t x_h_offset = (size_t)x_h * nchannels;

      uchar* top_left_pixel     = &src->data[y_l_offset + x_l_offset];
      uchar* top_right_pixel    = &src->data[y_l_offset + x_h_offset];
      uchar* bottom_left_pixel  = &src->data[y_h_offset + x_l_offset];
      uchar* bottom_right_pixel = &src->data[y_h_offset + x_h_offset];
      for (int c = 0; c < nchannels; c++)
      {
        flt_t pixel = (flt_t)top_left_pixel[c] * (1.0F - x_weight) * (1.0F - y_weight) + (flt_t)top_right_pixel[c] * x_weight * (1.0F - y_weight)
            + (flt_t)bottom_left_pixel[c] * y_weight * (1.0F - x_weight) + (flt_t)bottom_right_pixel[c] * x_weight * y_weight;

        dst->data[(y * dst->width + x) * nchannels + c] = (unsigned char)NVM_CLAMP(pixel, 0.0f, 255.0f);
      }
    }
  }
}

SDL_Surface*
_nv_image_to_sdl_surface(const nv_image_t* tex)
{
  nv_assert_else_return(tex->width != 0, NULL);
  nv_assert_else_return(tex->height != 0, NULL);
  nv_assert_else_return(tex->format != NOVA_FORMAT_UNDEFINED, NULL);

  SDL_Surface* surface = SDL_CreateSurfaceFrom(
      (int)tex->width, (int)tex->height, nv_format_to_sdl_format(tex->format), tex->data, (int)(tex->width * nv_format_get_bytes_per_pixel(tex->format)));

  return surface;
}

nv_image_t
_nv_sdl_surface_to_image(SDL_Surface* surface)
{
  nv_assert_else_return(surface->w != 0, nv_zero_init(nv_image_t));
  nv_assert_else_return(surface->h != 0, nv_zero_init(nv_image_t));

  SDL_LockSurface(surface);

  const size_t surface_size_bytes = surface->w * surface->h * SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel;

  nv_image_t image;
  image.width  = (size_t)surface->w;
  image.height = (size_t)surface->h;
  nv_assert_else_return(image.width != NOVA_FORMAT_UNDEFINED, nv_zero_init(nv_image_t));
  nv_assert_else_return(image.height != NOVA_FORMAT_UNDEFINED, nv_zero_init(nv_image_t));

  image.format = nv_sdl_format_to_nv_format((SDL_Format_)surface->format);
  nv_assert_else_return(image.format != NOVA_FORMAT_UNDEFINED, nv_zero_init(nv_image_t));

  image.data = nv_calloc(surface_size_bytes);

  nv_memcpy(image.data, surface->pixels, surface_size_bytes);

  SDL_UnlockSurface(surface);
  SDL_DestroySurface(surface);

  return image;
}

void
nv_image_write_png(const nv_image_t* tex, const char* path)
{
  if (tex == NULL || path == NULL || tex->data == NULL)
  {
    return;
  }

  SDL_Surface* surface = _nv_image_to_sdl_surface(tex);

  if (IMG_SavePNG(surface, path) != 0)
  {
    nv_log_error("Failed in writing image %s. Perhaps its parent directories do not exist?. SDL reports: %s\n", path, SDL_GetError());
  }

  SDL_DestroySurface(surface);
}

void
nv_image_write_jpeg(const nv_image_t* tex, const char* path, int quality)
{
  if (tex == NULL || path == NULL || tex->data == NULL)
  {
    return;
  }

  SDL_Surface* surface = _nv_image_to_sdl_surface(tex);

  if (IMG_SaveJPG(surface, path, quality) != 0)
  {
    nv_log_error("Failed in writing image %s. Perhaps its parent directories do not exist?. SDL reports: %s\n", path, SDL_GetError());
  }

  SDL_DestroySurface(surface);
}

// nv_image_t

const char*
nv_format_to_string(nv_format format)
{
  switch (format)
  {
    case NOVA_FORMAT_UNDEFINED: return "NOVA_FORMAT_UNDEFINED";
    case NOVA_FORMAT_R8: return "NOVA_FORMAT_R8";
    case NOVA_FORMAT_RG8: return "NOVA_FORMAT_RG8";
    case NOVA_FORMAT_RGB8: return "NOVA_FORMAT_RGB8";
    case NOVA_FORMAT_RGBA8: return "NOVA_FORMAT_RGBA8";
    case NOVA_FORMAT_BGR8: return "NOVA_FORMAT_BGR8";
    case NOVA_FORMAT_BGRA8: return "NOVA_FORMAT_BGRA8";
    case NOVA_FORMAT_RGB16: return "NOVA_FORMAT_RGB16";
    case NOVA_FORMAT_RGBA16: return "NOVA_FORMAT_RGBA16";
    case NOVA_FORMAT_RG32: return "NOVA_FORMAT_RG32";
    case NOVA_FORMAT_RGB32: return "NOVA_FORMAT_RGB32";
    case NOVA_FORMAT_RGBA32: return "NOVA_FORMAT_RGBA32";
    case NOVA_FORMAT_R8_SINT: return "NOVA_FORMAT_R8_SINT";
    case NOVA_FORMAT_RG8_SINT: return "NOVA_FORMAT_RG8_SINT";
    case NOVA_FORMAT_RGB8_SINT: return "NOVA_FORMAT_RGB8_SINT";
    case NOVA_FORMAT_RGBA8_SINT: return "NOVA_FORMAT_RGBA8_SINT";
    case NOVA_FORMAT_R8_UINT: return "NOVA_FORMAT_R8_UINT";
    case NOVA_FORMAT_RG8_UINT: return "NOVA_FORMAT_RG8_UINT";
    case NOVA_FORMAT_RGB8_UINT: return "NOVA_FORMAT_RGB8_UINT";
    case NOVA_FORMAT_RGBA8_UINT: return "NOVA_FORMAT_RGBA8_UINT";
    case NOVA_FORMAT_R8_SRGB: return "NOVA_FORMAT_R8_SRGB";
    case NOVA_FORMAT_RG8_SRGB: return "NOVA_FORMAT_RG8_SRGB";
    case NOVA_FORMAT_RGB8_SRGB: return "NOVA_FORMAT_RGB8_SRGB";
    case NOVA_FORMAT_RGBA8_SRGB: return "NOVA_FORMAT_RGBA8_SRGB";
    case NOVA_FORMAT_BGR8_SRGB: return "NOVA_FORMAT_BGR8_SRGB";
    case NOVA_FORMAT_BGRA8_SRGB: return "NOVA_FORMAT_BGRA8_SRGB";
    case NOVA_FORMAT_D16: return "NOVA_FORMAT_D16";
    case NOVA_FORMAT_D24: return "NOVA_FORMAT_D24";
    case NOVA_FORMAT_D32: return "NOVA_FORMAT_D32";
    case NOVA_FORMAT_D24_S8: return "NOVA_FORMAT_D24_S8";
    case NOVA_FORMAT_D32_S8: return "NOVA_FORMAT_D32_S8";
    case NOVA_FORMAT_BC1: return "NOVA_FORMAT_BC1";
    case NOVA_FORMAT_BC3: return "NOVA_FORMAT_BC3";
    case NOVA_FORMAT_BC7: return "NOVA_FORMAT_BC7";
    default: return "(NotAFormat)";
  }
  return "(NotAFormat)";
}

bool
nv_format_has_color_channel(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8:
    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
    case NOVA_FORMAT_UNDEFINED: return 0;
    default: return 1;
  }
}

// Returns false even for stencil/depth and undefined format
bool
nv_format_has_alpha_channel(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB: return 1;
    default: return 0;
  }
}

bool
nv_format_has_depth_channel(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8: return 1;

    default: return 0;
  }
}

bool
nv_format_has_stencil_channel(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32_S8: return 1;

    default: return 0;
  }
}

int
nv_format_get_bytes_per_channel(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_R8:
    case NOVA_FORMAT_RG8:
    case NOVA_FORMAT_RGB8:
    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGR8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_R8_SINT:
    case NOVA_FORMAT_RG8_SINT:
    case NOVA_FORMAT_RGB8_SINT:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_R8_UINT:
    case NOVA_FORMAT_RG8_UINT:
    case NOVA_FORMAT_RGB8_UINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_R8_SRGB:
    case NOVA_FORMAT_RG8_SRGB:
    case NOVA_FORMAT_RGB8_SRGB:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGR8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB: return 1;

    case NOVA_FORMAT_RGB16:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_D16: return 2;

    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8: return 3;

    case NOVA_FORMAT_RG32:
    case NOVA_FORMAT_RGB32:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8: return 4;

    default:
    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
    case NOVA_FORMAT_UNDEFINED: return -1;
  }
}

int
nv_format_get_bytes_per_pixel(nv_format fmt)
{
  return nv_format_get_bytes_per_channel(fmt) * nv_format_get_num_channels(fmt);
}

int
nv_format_get_num_channels(nv_format fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_R8:
    case NOVA_FORMAT_R8_SINT:
    case NOVA_FORMAT_R8_UINT:
    case NOVA_FORMAT_R8_SRGB:
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D32: return 1;

    case NOVA_FORMAT_RG8:
    case NOVA_FORMAT_RG32:
    case NOVA_FORMAT_RG8_SINT:
    case NOVA_FORMAT_RG8_UINT:
    case NOVA_FORMAT_RG8_SRGB:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32_S8: return 2;

    case NOVA_FORMAT_RGB8:
    case NOVA_FORMAT_BGR8:
    case NOVA_FORMAT_RGB16:
    case NOVA_FORMAT_RGB32:
    case NOVA_FORMAT_RGB8_SINT:
    case NOVA_FORMAT_RGB8_UINT:
    case NOVA_FORMAT_RGB8_SRGB:
    case NOVA_FORMAT_BGR8_SRGB: return 3;

    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB: return 4;

    // FIXME: Implement
    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
    case NOVA_FORMAT_UNDEFINED:
    default: return 0;
  }
}

nv_format
nv_sdl_format_to_nv_format(SDL_Format_ format)
{
  switch (format)
  {
    /* TODO: Add more? I wasn't able to find any more though */
    case SDL_PIXELFORMAT_RGB24: return NOVA_FORMAT_RGB8;
    case SDL_PIXELFORMAT_RGBA32: return NOVA_FORMAT_RGBA8;
    case SDL_PIXELFORMAT_ABGR32: return NOVA_FORMAT_BGRA8;

    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
    default: return NOVA_FORMAT_UNDEFINED;
  }
}

SDL_Format_
nv_format_to_sdl_format(nv_format format)
{
  switch (format)
  {
    case NOVA_FORMAT_RGB8: return SDL_PIXELFORMAT_RGB24;
    case NOVA_FORMAT_RGBA8: return SDL_PIXELFORMAT_RGBA32;
    case NOVA_FORMAT_BGRA8: return SDL_PIXELFORMAT_ABGR32;
    default: return SDL_PIXELFORMAT_UNKNOWN;
  }
}

static inline nv_rand_t
rotl(nv_rand_t x, int k)
{
#if defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_rotateleft64) && (__SIZEOF_LONG_LONG__ == 8)
  return __builtin_rotateleft64(x, k);
#else
  return (x << k) | (x >> ((sizeof(nv_rand_t) * 8 - k) & ((sizeof(nv_rand_t) * 8) - 1)));
#endif
}

nv_error
nv_random_bulk_range(nv_rand_info_t* info, nv_rand_t* outbuf, size_t outbuf_size, size_t min, size_t max)
{
  nv_assert_else_return(outbuf_size != 0, NV_ERROR_INVALID_ARG);

  if (min >= max)
  {
    return min;
  }

  for (size_t i = 0; i < outbuf_size; i++)
  {
    /**
     * xoshiro 256 random number generator
     * https://prng.di.unimi.it/xoshiro256plusplus.c
     */
    const _NOVA_RAND_TMP_CONVERT_TYPE bound = max - min + 1;

    const nv_rand_t result = rotl(info->state[0] + info->state[3], 23) + info->state[0];

    _NOVA_RAND_TMP_CONVERT_TYPE tmp = ((_NOVA_RAND_TMP_CONVERT_TYPE)result * bound);
    outbuf[i]                       = min + (tmp >> (sizeof(nv_rand_t) * 8));

    const nv_rand_t t = info->state[1] << 17;

    info->state[2] ^= info->state[0];
    info->state[3] ^= info->state[1];
    info->state[1] ^= info->state[2];
    info->state[0] ^= info->state[3];

    info->state[2] ^= t;
    info->state[3] = rotl(info->state[3], 45);
  }

  return NV_SUCCESS;
}

static inline nv_rand_t
_splitmix(nv_rand_t* state)
{
  nv_rand_t tmp = (*state += 0x9E3779B97f4A7C15);
  tmp           = (tmp ^ (tmp >> 30)) * 0xBF58476D1CE4E5B9;
  tmp           = (tmp ^ (tmp >> 27)) * 0x94D049BB133111EB;
  return tmp ^ (tmp >> 31);
}

void
nv_random_seed(nv_rand_info_t* info, nv_rand_t seed)
{
  nv_rand_t splitmixstate = seed;
  for (size_t i = 0; i < 4; i++)
  {
    info->state[i] = _splitmix(&splitmixstate);
  }
}

#if (NOVA_FILE_API_AVAILABLE)

#  ifdef WIN32
#    include <io.h>
#    include <sys/stat.h>
#    include <windows.h>
#    define access _access
#    define FSTAT _fstat
#    define STAT stat
#    define FILENO _filen
#  else
#    include <dirent.h>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    define FSTAT fstat
#    define STAT stat
#    define FILENO fileno
#  endif

int
nv_fs_perms_to_win_perms(nv_fs_permission perms, bool for_directory)
{
  (void)for_directory;
  switch (perms)
  {
    case NV_FS_PERMISSION_EXISTS: return 0;
    case NV_FS_PERMISSION_READ_ONLY: return 4;
    case NV_FS_PERMISSION_WRITE_ONLY: return 2;
    case NV_FS_PERMISSION_READ_WRITE: return 6;
    default: return -1;
  }
}

int
nv_fs_perms_to_unix_perms(nv_fs_permission perms, bool for_directory)
{
  int mode = -1;

#  ifndef _WIN32
  switch (perms)
  {
    case NV_FS_PERMISSION_EXISTS: mode = S_IRUSR | S_IWUSR; break;
    case NV_FS_PERMISSION_READ_ONLY: mode = S_IRUSR | S_IRGRP | S_IROTH; break;
    case NV_FS_PERMISSION_WRITE_ONLY: mode = S_IWUSR | S_IWGRP | S_IWOTH; break;
    case NV_FS_PERMISSION_READ_WRITE: mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; break;
    default: mode = S_IRUSR | S_IWUSR; break;
  }

  if (for_directory)
  {
    if (mode & S_IRUSR)
    {
      mode |= S_IXUSR;
    }
    if (mode & S_IRGRP)
    {
      mode |= S_IXGRP;
    }
    if (mode & S_IROTH)
    {
      mode |= S_IXOTH;
    }
  }
#  endif

  return mode;
}

static inline int
_nv_fs_perms_to_sys_perms(nv_fs_permission perms, bool for_directory)
{
#  ifdef _WIN32
  return nv_fs_perms_to_win_perms(perms, for_directory);
#  else
  return nv_fs_perms_to_unix_perms(perms, for_directory);
#  endif
}

bool
nv_fs_file_exists(const char* fpath)
{
  return (access(fpath, _nv_fs_perms_to_sys_perms(NV_FS_PERMISSION_EXISTS, false)) == 0);
}

nv_error
nv_fs_file_size(const char* fpath, size_t* out_size)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_size = (size_t)st.st_size;
    return NV_SUCCESS;
  }
  *out_size = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_get_access_time(const char* fpath, size_t* out_atime)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_atime = (size_t)st.st_atime;
    return NV_SUCCESS;
  }
  *out_atime = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_get_modified_time(const char* fpath, size_t* out_atime)
{
  struct STAT st;
  if (STAT(fpath, &st) == 0)
  {
    *out_atime = (size_t)st.st_mtime;
    return NV_SUCCESS;
  }
  *out_atime = 0;
  return NV_ERROR_IO_ERROR;
}

nv_error
nv_fs_file_readall(const char* fpath, nv_allocator_fn alloc, void* alloc_arg, char** buffer, size_t* buffer_size)
{
  nv_error code = NV_SUCCESS;
  size_t   size = 0;

  *buffer = NULL;
  if (buffer_size)
  {
    *buffer_size = 0;
  }

  FILE* fp = fopen(fpath, "rb");
  if (!fp)
  {
    return NV_ERROR_IO_ERROR;
  }

  if ((code = nv_fs_file_size(fpath, &size)) != NV_SUCCESS)
  {
    fclose(fp);
    return code;
  }

  *buffer = alloc(alloc_arg, NULL, NV_ALLOC_NEW_BLOCK, size + 1);
  if (!*buffer)
  {
    fclose(fp);
    return NV_ERROR_MALLOC_FAILED;
  }

  if (fread(*buffer, 1, size, fp) != size)
  {
    alloc(alloc_arg, *buffer, size + 1, NV_ALLOC_FREE);
    *buffer = NULL;
    fclose(fp);
    return NV_ERROR_IO_ERROR;
  }

  (*buffer)[size] = '\0';

  if (buffer_size)
  {
    *buffer_size = size;
  }

  fclose(fp);
  return NV_SUCCESS;
}

nv_error
nv_fs_file_writeall(const char* fpath, const void* data, size_t data_size)
{
  FILE* fp = fopen(fpath, "wb");
  if (!fp)
  {
    return NV_ERROR_IO_ERROR;
  }

  if (fwrite(data, 1, data_size, fp) != data_size)
  {
    fclose(fp);
    return NV_ERROR_IO_ERROR;
  }

  fclose(fp);
  return NV_SUCCESS;
}

nv_error
nv_fs_file_create(const char* fpath)
{
#  ifndef _WIN32

  int fd = open(fpath, O_RDWR | O_CREAT | O_EXCL, 0755);
  if (fd < 0 && errno != EEXIST)
  {
    return NV_ERROR_IO_ERROR;
  }
  close(fd);

#  else

  HANDLE bimbows_file_handle = CreateFileA(fpath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

  if (bimbows_file_handle == INVALID_HANDLE_VALUE)
  {
    DWORD error = GetLastError();
    if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
    {
      return NV_ERROR_IO_ERROR;
    }
  }
  else
  {
    CloseHandle(bimbows_file_handle);
  }

#  endif

  return NV_SUCCESS;
}

nv_error
nv_fs_file_delete(const char* fpath)
{
  if (remove(fpath) != 0)
  {
    return NV_ERROR_IO_ERROR;
  }
  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create(const char* dpath, nv_fs_permission perms)
{
#  ifndef _WIN32
  if (mkdir(dpath, _nv_fs_perms_to_sys_perms(perms, true)) != 0 && errno != EEXIST)
  {
    return NV_ERROR_IO_ERROR;
  }
#  else
  (void)perms;

  if (!CreateDirectoryA(dpath, NULL))
  {
    DWORD err = GetLastError();
    if (err != ERROR_ALREADY_EXISTS)
    {
      return NV_ERROR_IO_ERROR;
    }
  }
#  endif
  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create_recursive(const char* dpath, nv_fs_permission perms)
{
  const size_t dirlen    = nv_strlen(dpath);
  char*        path_copy = nv_calloc(dirlen + 1);
  char*        buffer    = nv_calloc(dirlen + 1);

  nv_strlcpy(path_copy, dpath, dirlen + 1);

  char* last_separator = nv_strrchr(path_copy, '/');
  if (last_separator != NULL)
  {
    *last_separator = '\0';

    nv_strlcpy(buffer, path_copy, dirlen + 1);
    size_t len = nv_strlen(buffer);

    if (buffer[len - 1] == '/')
    {
      buffer[len - 1] = 0;
    }

    for (char* p = buffer + 1; *p; p++)
    {
      if (*p == '/')
      {
        *p = 0;
        nv_fs_dir_create(buffer, perms);
        *p = '/';
      }
    }

    nv_fs_dir_create(buffer, perms);
  }

  nv_fs_dir_create(buffer, perms);
  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nv_fs_dir_create_recursive_for_file(const char* fpath, nv_fs_permission perms)
{
  const size_t pathlen   = nv_strlen(fpath);
  char*        path_copy = nv_calloc(pathlen + 1);
  char*        buffer    = nv_calloc(pathlen + 1);

  nv_strlcpy(path_copy, fpath, pathlen + 1);

  char* last_separator = nv_strrchr(path_copy, '/');
  if (last_separator != NULL)
  {
    *last_separator = '\0';

    nv_strlcpy(buffer, path_copy, pathlen + 1);
    size_t len = nv_strlen(buffer);

    if (buffer[len - 1] == '/')
    {
      buffer[len - 1] = 0;
    }

    char* start = buffer;
    if (buffer[0] == '/')
    {
      start = buffer + 1;
    }

    for (char* p = start; *p; p++)
    {
      if (*p == '/')
      {
        *p = 0;

        if (nv_fs_dir_create(buffer, perms) != NV_SUCCESS)
        {
          nv_free(path_copy);
          nv_free(buffer);
          return NV_ERROR_IO_ERROR;
        }

        *p = '/';
      }
    }

    if (nv_fs_dir_create(buffer, perms) != NV_SUCCESS)
    {
      nv_free(path_copy);
      nv_free(buffer);
      return NV_ERROR_IO_ERROR;
    }
  }

  nv_free(path_copy);
  nv_free(buffer);

  return NV_SUCCESS;
}

nv_error
nv_fs_dir_delete_recursive(const char* dpath)
{
#  ifndef _WIN32
  DIR* dir = opendir(dpath);
  if (!dir)
  {
    return NV_ERROR_IO_ERROR;
  }

  char*    fullpath = nv_calloc(NV_MAX(PATH_MAX, nv_strlen(dpath)));
  nv_error code     = NV_SUCCESS;

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (nv_strcmp(entry->d_name, ".") == 0 || nv_strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    nv_snprintf(fullpath, sizeof(fullpath), "%s/%s", dpath, entry->d_name);

    struct stat st;
    if (stat(fullpath, &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
      {
        code = nv_fs_dir_delete_recursive(fullpath);
      }
      else
      {
        code = nv_fs_file_delete(fullpath);
      }

      if (NV_UNLIKELY(code != NV_SUCCESS))
      {
        closedir(dir);
        nv_free(fullpath);
        return code;
      }
    }
  }
  closedir(dir);
  remove(dpath);

  nv_free(fullpath);

#  else

  char pattern[MAX_PATH];
  nv_snprintf(pattern, MAX_PATH, "%s\\*", dpath);

  WIN32_FIND_DATAA fd;
  HANDLE           bimbows_find_file = FindFirstFileA(pattern, &fd);
  if (bimbows_find_file == INVALID_HANDLE_VALUE)
  {
    return NV_ERROR_IO_ERROR;
  }

  nv_error code = NV_SUCCESS;

  do
  {
    if (nv_strcmp(fd.cFileName, ".") == 0 || nv_strcmp(fd.cFileName, "..") == 0)
    {
      continue;
    }

    char fullpath[MAX_PATH];
    nv_snprintf(fullpath, MAX_PATH, "%s\\%s", dpath, fd.cFileName);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      code = nv_fs_dir_delete_recursive(fullpath);
    }
    else
    {
      DeleteFileA(fullpath);
    }

    if (code != NV_SUCCESS)
    {
      FindClose(bimbows_find_file);
      return code;
    }
  } while (FindNextFileA(bimbows_find_file, &fd));
  FindClose(bimbows_find_file);

  RemoveDirectoryA(dpath);

#  endif

  return NV_SUCCESS;
}

#endif

// static struct nv_sa_page* root = null;

// static inline nv_error
// nv_alloc_and_link_page(struct nv_sa_page** dstptr, size_t min_size)
// {
//   min_size += NOVA_SYSALLOC_PAGE_HEADER_SIZE;
//   const size_t map_size = NV_MAX(min_size, NOVA_SYSALLOC_PAGE_HEADER_SIZE + NOVA_SYSALLOC_PAGE_CAPACITY);

//   void* ptr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
//   if (ptr == MAP_FAILED)
//   {
//     return NV_ERROR_EXTERNAL;
//   }

//   *dstptr = (struct nv_sa_page*)ptr;

//   struct nv_sa_freeblock* new_node = malloc(sizeof(struct nv_sa_freeblock));
//   nv_zero_structp(new_node);

//   new_node->offset = NOVA_SYSALLOC_PAGE_HEADER_SIZE;
//   new_node->size   = map_size - NOVA_SYSALLOC_PAGE_HEADER_SIZE;
//   new_node->next   = NULL;

//   (*dstptr)->root_free_block = new_node;
//   (*dstptr)->page_size       = map_size;

//   if (root != null)
//   {
//     struct nv_sa_page* last_page = root;
//     while (last_page->next)
//     {
//       if (last_page->next == last_page)
//       {
//         nv_log_and_abort("Circular reference detected: page->next == page\n");
//       }
//       last_page = last_page->next;
//     }

//     if (last_page == *dstptr)
//     {
//       nv_log_and_abort("BUG: Attempt to link page to itself!\n");
//     }

//     last_page->next = *dstptr;
//   }
//   else
//   {
//     root = (*dstptr);
//   }

//   return NV_SUCCESS;
// }

// static inline void
// nv_free_page(struct nv_sa_page* page)
// {
//   if (!page)
//   {
//     return;
//   }

//   struct nv_sa_freeblock* node = page->root_free_block;
//   while (node)
//   {
//     struct nv_sa_freeblock* next = node->next;
//     free(next);
//     node = next;
//   }

//   // dislodge the page from the linked list
//   struct nv_sa_page* page_it = root;
//   while (page_it)
//   {
//     if (page_it->next == page)
//     {
//       page_it->next = page->next;
//       break;
//     }
//     page_it = page_it->next;
//   }

//   if (munmap((void*)page, page->page_size) != 0)
//   {
//     nv_log_and_abort("munmap: %s\n", strerror(errno));
//   }
// }

// static inline void
// _trim_free_block(struct nv_sa_page* page, struct nv_sa_freeblock* block, size_t trim_size_trailing)
// {
//   if (block->size <= trim_size_trailing)
//   {
//     return;
//   }

//   nv_assert_else_return(trim_size_trailing > 0, );

//   struct nv_sa_freeblock* new_node = malloc(sizeof(struct nv_sa_freeblock));
//   nv_zero_structp(new_node);

//   new_node->size   = trim_size_trailing;
//   new_node->offset = block->offset + (block->size - trim_size_trailing);
//   new_node->next   = NULL;

//   // append a node
//   new_node->next = page->root_free_block;
//   if (page->root_free_block)
//   {
//     page->root_free_block = new_node;
//   }

//   block->size -= trim_size_trailing;
// }

// static inline size_t
// _get_block_size(void* block)
// {
//   struct nv_sa_block* header = (struct nv_sa_block*)((uchar*)block - sizeof(struct nv_sa_block));
//   return header->size;
// }

// void*
// nv_sa_alloc(size_t size)
// {
//   if (!root)
//   {
//     struct nv_sa_page* tmp = NULL;
//     nv_alloc_and_link_page(&tmp, size);
//   }

//   const size_t total_size = size + sizeof(struct nv_sa_block);

//   struct nv_sa_page* page = root;
//   while (page)
//   {
//     struct nv_sa_freeblock* block = page->root_free_block;
//     while (block)
//     {
//       if (block->size >= total_size)
//       {
//         if (block->size > total_size)
//         {
//           _trim_free_block(page, block, block->size - total_size);
//         }

//         struct nv_sa_block* header = (struct nv_sa_block*)((uchar*)page->capacity + block->offset);
//         void*               ptr    = (void*)(header + 1);

//         // insert bookkeeping info
//         header->page               = page;
//         header->size               = size;
//         header->offset             = block->offset;
//         header->alignment_exponent = 0; // 2 ^ 0 = 1

//         return ptr;
//       }
//       block = block->next;
//     }

//     page = page->next;
//   }

//   if (!page)
//   {
//     struct nv_sa_page* new_page = NULL;
//     nv_alloc_and_link_page(&new_page, size);
//     return nv_sa_alloc(size);
//   }
//   return null;
// }

// void
// _sort_and_merge_free_blocks(struct nv_sa_page* page)
// {
//   // struct nv_sa_freeblock* sorted = NULL;
//   // struct nv_sa_freeblock* curr   = page->root_free_block;

//   // while (curr)
//   // {
//   //   struct nv_sa_freeblock* next = curr->next;

//   //   if (!sorted || curr->offset < sorted->offset)
//   //   {
//   //     curr->next = sorted;
//   //     sorted     = curr;
//   //   }
//   //   else
//   //   {
//   //     struct nv_sa_freeblock* s = sorted;
//   //     while (s->next && s->next->offset < curr->offset)
//   //     {
//   //       s = s->next;
//   //     }
//   //     curr->next = s->next;
//   //     s->next    = curr;
//   //   }

//   //   curr = next;
//   // }

//   // page->root_free_block = sorted;

//   // struct nv_sa_freeblock* block = sorted;
//   // while (block && block->next)
//   // {
//   //   if (block->offset + block->size == block->next->offset)
//   //   {
//   //     block->size += block->next->size;

//   //     struct nv_sa_freeblock* to_free = block->next;
//   //     block->next                     = to_free->next;

//   //     free(to_free);
//   //   }
//   //   else
//   //   {
//   //     block = block->next;
//   //   }
//   // }
// }

// void*
// nv_sa_realloc(void* old_ptr, size_t new_size)
// {
//   void* new_ptr = nv_sa_alloc(new_size);
//   nv_memmove(new_ptr, old_ptr, _get_block_size(old_ptr));
//   nv_sa_free(old_ptr);
//   return new_ptr;
// }

// void
// nv_sa_free(void* ptr)
// {
//   struct nv_sa_block* header = (struct nv_sa_block*)((uchar*)ptr - sizeof(struct nv_sa_block));

//   struct nv_sa_freeblock* last_block = header->page->root_free_block;
//   while (last_block->next)
//   {
//     last_block = last_block->next;
//   }

//   struct nv_sa_freeblock* new_node = malloc(sizeof(struct nv_sa_freeblock));
//   nv_zero_structp(new_node);

//   new_node->size   = header->size + sizeof(struct nv_sa_freeblock);
//   new_node->offset = header->offset;
//   new_node->next   = NULL;

//   new_node->next                = header->page->root_free_block;
//   header->page->root_free_block = new_node;

//   _sort_and_merge_free_blocks(header->page);

//   if (header->page->root_free_block == NULL)
//   {
//     nv_free_page(header->page);
//   }
// }
