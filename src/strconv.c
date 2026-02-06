#include "../include/strconv.h"

#include "../include/chrclass.h"
#include "../include/stdafx.h"
#include "../include/string.h"
#include "../include/types.h"

#include <math.h>
#include <stdint.h>

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
  while (highest_power_of_base <= num / base) { highest_power_of_base *= base; }

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
    if (i >= max) { break; }

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

    if (dig < 10) { out[i] = (char)('0' + dig); }
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
  while (highest_power_of_base <= num / (uintmax_t)base) { highest_power_of_base *= base; }

  size_t loop_digits_written = 0;
  do
  {
    if (i >= max) { break; }

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

    if (dig < 10) { out[i] = (char)('0' + dig); }
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
    if (signbit(n) == 0) return nv_strncpy2(out, str, max);                                                                                                                   \
    else return nv_strncpy2(out, "-" str, max);                                                                                                                               \
  }

size_t
nv_fltoa2(long double num, char out[], int precision, size_t max, bool remove_zeros)
{
  if (max == 0) { return 0; }
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

  int exponent        = (num == 0.0) ? 0 : (int)log10l(num);
  int to_use_exponent = (exponent >= 14 || (neg && exponent >= 9) || exponent <= -9);
  if (to_use_exponent) { num /= pow(10.0, exponent); }

  long double rounding = powl(10.0, -precision) * 0.5;
  num += rounding;

  /* n has been absoluted before so we can expect that it won't be negative */
  uintmax_t int_part = (uintmax_t)num;

  // int part is now floored
  long double frac_part = num - (long double)int_part;

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
    while (*(itr - 1) == '0') { itr--; }
    if (*(itr - 1) == '.') { itr--; }
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

// WARNING::: I didn't write most of this, stole it from stack overflow.
// if it explodes your computer its your fault!!!
// Well.. its safe :3 you have my word
size_t
nv_ftoa2(double num, char out[], int precision, size_t max, bool remove_zeros)
{
  return nv_fltoa2((long double)num, out, precision, max, remove_zeros);
}

#define NV_SKIP_WHITSPACE(s) nv_strtrim_c(s, &(s), NULL);

intmax_t
nv_atoi2(const char in_string[], size_t max, char** endptr)
{
  if (!in_string)
  {
    if (endptr) { *endptr = NULL; }
    return INTMAX_MAX;
  }

  intmax_t base = 10;

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

  if (*c == '0')
  {
    // num = 0600
    base = 8;

    c++;
    i++;

    if (i < max && nv_tolower(*c) == 'x')
    {
      // num = 0x600
      base = 16;
      c++;
      i++;
    }
  }

  while (i < max && *c)
  {
    bool is_hex_dig = (nv_tolower(*c) >= 'a') && (nv_tolower(*c) <= 'f');
    if (!nv_isdigit(*c) && !is_hex_dig) // *c is not digit or hexadecimal letter
    {
      break;
    }

    int digit = *c - '0';
#if defined(DEBUG) && DEBUG
    if (base == 8) { nv_assert(digit < 8); }
    else if (base == 10) { nv_assert(is_hex_dig == false); }
#endif

    if (is_hex_dig)
    {
      /**
       *c - a maps it from 0 - 5, +10 gives us the actual value
       */
      digit = (nv_tolower(*c) - 'a') + 10;
    }
    ret = ret * base + digit;

    c++;
    i++;
  }

  if (neg) { ret *= -1; }

  if (endptr) { *endptr = (char*)c; }
  return ret;
}

double
nv_atof2(const char in_string[], size_t max, char** endptr)
{
  if (!in_string)
  {
    if (endptr) { *endptr = NULL; }
    return 0.0;
  }

  double      result   = 0.0;
  double      fraction = 0.0;
  int         divisor  = 1;
  bool        neg      = 0;
  const char* c        = in_string;
  size_t      i        = 0;

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

  while (i < max && nv_isdigit(*c))
  {
    result = result * 10 + (*c - '0');
    c++;
  }

  if (*c == '.')
  {
    c++;
    while (nv_isdigit(*c))
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

    while (i < max && nv_isdigit(*c))
    {
      exponent = exponent * 10 + (*c - '0');
      c++;
      i++;
    }

    result = ldexp(result, exp_sign * exponent);
  }

  if (neg) { result *= -1.0; }

  if (endptr) { *endptr = (char*)c; }
  return result;
}

bool
nv_atobool(const char in_string[], size_t max)
{
  size_t i = 0;
  NV_SKIP_WHITSPACE(in_string);
  if (i > max) { return true; }
  if (nv_strcasencmp(in_string, "false", max - i) == 0 || nv_strncmp(in_string, "0", max - i) == 0) { return false; }
  return true;
}

size_t
nv_ptoa2(void* ptr, char out[], size_t max)
{
  if (ptr == NULL) { return nv_strncpy2(out, "NULL", max); }

  u64        addr   = (u64)ptr;
  const char digs[] = "0123456789abcdef";

  size_t w = 0;

  w += nv_strncpy2(out, "0x", max);

  // stolen from stack overflow
  for (int i = (sizeof(addr) * 2) - 1; i >= 0 && w < max - 1; i--)
  {
    int dig = (int)((addr >> (i * 4)) & 0xF);
    out[w]  = digs[dig];
    w++;
  }
  out[w] = 0;
  return w;
}

size_t
nv_btoa2(size_t num_bytes, bool upgrade, char out[], size_t max)
{
  size_t written = 0;
  if (upgrade)
  {
    const char* stages[] = { " B", " KB", " MB", " GB", " TB", " PB", " Comically large number of bytes" };
    double      b        = (double)num_bytes;
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
