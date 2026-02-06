#include "../include/print.h"

#include "../include/chrclass.h"
#include "../include/stdafx.h"
#include "../include/strconv.h"
#include "../include/stream.h"
#include "../include/string.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PADBUF_SIZE 64
#define DEFAULTPREC 6

// Moral of the story? FU@# SIZE_MAX
// I spent an HOUR trying to figure out what's going wrong
// and I didn't even bat an eye towards it

size_t
nv_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  // size_t written = nv_vnprintf(NOVA_WBUF_SIZE, args, fmt);
  size_t written = nv_vprintf(args, fmt);

  va_end(args);

  return written;
}

size_t
nv_fprintf(FILE* f, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t written = 0;
  if (NV_LIKELY(f)) { written = nv_vfprintf(args, f, fmt); }
  else
  {
    written = nv_sinkprintf(args, fmt);
  }

  va_end(args);

  return written;
}

size_t
nv_sprintf(char* dst, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t written = 0;
  if (NV_LIKELY(dst)) { written = nv_vsnprintf(args, dst, SIZE_MAX, fmt); }
  else
  {
    written = nv_sinkprintf(args, fmt);
  }

  va_end(args);

  return written;
}

size_t
nv_vprintf(va_list args, const char* fmt)
{
  nv_stream_t* stm = NULL;
  nv_error     e   = nv_open_pipestream(stdout, &stm);
  if (e) return 0;

  size_t wrote = nv_vssnprintf(stm, SIZE_MAX, fmt, args);

  nv_close_stream(stm);

  return wrote;
}

size_t
nv_vfprintf(va_list args, FILE* f, const char* fmt)
{
  nv_stream_t* stm = NULL;
  nv_error     e   = nv_open_pipestream(f, &stm);
  if (e) return 0;

  size_t written = 0;
  if (NV_LIKELY(f)) { written = nv_vssnprintf(stm, SIZE_MAX, fmt, args); }
  else
  {
    written = nv_sinkprintf(args, fmt);
  }

  nv_close_stream(stm);

  return written;
}

size_t
nv_snprintf(char* dst, size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t written = 0;
  if (NV_LIKELY(dst)) { written = nv_vsnprintf(args, dst, max_chars, fmt); }
  else
  {
    written = nv_sinkprintf(args, fmt);
  }

  va_end(args);

  return written;
}

size_t
nv_nprintf(size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t written = nv_vnprintf(args, max_chars, fmt);

  va_end(args);

  return written;
}

size_t
nv_vnprintf(va_list args, size_t max_chars, const char* fmt)
{
  nv_stream_t* stm = NULL;
  nv_error     e   = nv_open_pipestream(stdout, &stm);
  if (e) return 0;

  size_t wrote = nv_vssnprintf(stm, max_chars, fmt, args);

  nv_close_stream(stm);

  return wrote;
}

size_t
nv_vsnprintf(va_list src, char* dst, size_t max_chars, const char* fmt)
{
  nv_stream_t* stm = NULL;
  nv_error     e   = nv_open_memstream(dst, max_chars, &stm);
  if (e) return 0;

  size_t written = 0;
  if (NV_LIKELY(dst)) { written = nv_vssnprintf(stm, max_chars, fmt, src); }
  else
  {
    written = nv_sinkprintf(src, fmt);
  }

  nv_close_stream(stm);

  return written;
}

size_t
nv_sinkprintf(va_list args, const char* fmt)
{
  nv_stream_t* sink;

  nv_error e = nv_open_sinkstream(&sink);
  if (e) return 0;

  size_t wrote = nv_vssnprintf(sink, SIZE_MAX, fmt, args);

  nv_close_stream(sink);

  return wrote;
}

enum p_length
{
  LENGTH_UNSPEC,
  LENGTH_HH,
  LENGTH_H,
  LENGTH_L,
  LENGTH_LL,
  LENGTH_Z, // size_t length
};

struct printf_info
{
  char scratch[256];

  nv_stream_t* s;
  va_list*     argp;

  size_t written; /* characters written until now. */
  // size_t      remaining; /* max - written */
  size_t      max;  /* max number of characters we're allowed to write. */
  const char* head; /* Format string iterator */

  /* width.prec */
  int           width;          /* width, 0 if unspecified */
  int           prec;           /* precision, 6 if unspecified (general default) */
  enum p_length length;         /* length, 0 if unspecified */
  bool          prec_specified; /* Precision specified? */
};

static inline int
peek(const struct printf_info* inf)
{
  return *inf->head;
}

static inline int
next(struct printf_info* inf)
{
  /* If already at null term then don't continue.  */
  if (!*inf->head) return 0;
  return *inf->head++;
}

/**
 * Conversion function is called with the head at the format specifier.
 * See the dispatch table for each function and it's invoking character.
 * Notably, precision, width and length (l, ll, h, z) are parsed before the invoking character.
 *  Those values are readily available in info struct.
 */
typedef size_t (*p_conv_fn)(struct printf_info* inf);
struct p_conv_entry
{
  char      ch;
  p_conv_fn fn;
};

static inline int
p_readint(struct printf_info* inf)
{
  int neg = 0;
  int c   = 0;

  if (*inf->head == '-')
  {
    next(inf);
    neg = true;
  }
  else if (*inf->head == '+') { next(inf); }

  while (nv_isdigit(*inf->head))
  {
    c = c * 10 + (*inf->head - '0');
    next(inf);
  }

  return neg ? -c : c;
}

/**
 * Parse width and length info from format and deposit them in the inf struct.
 * values for width and length will be reset, even if unspecified.
 * Must be called immediately after the % format specifier
 */
static inline void
p_parsewidlen(struct printf_info* inf)
{
  inf->prec_specified = false;

  int width = 0;
  if (NV_UNLIKELY(*inf->head == '*')) { width = va_arg(*inf->argp, int); }
  else
  {
    width = p_readint(inf);
  }

  int prec = DEFAULTPREC;
  if (*inf->head == '.')
  {
    next(inf);
    if (NV_UNLIKELY(*inf->head == '*')) { prec = va_arg(*inf->argp, int); }
    else
    {
      prec = p_readint(inf);
    }
    inf->prec_specified = true;
  }
  inf->prec  = prec;
  inf->width = width;
}

/**
 * Parse the length if specified. Otherwise do nothing.
 * Must be called after precision and width collection.
 */
static inline void
p_parseintlen(struct printf_info* inf)
{
  if (peek(inf) == 'h' && inf->head[1] == 'h')
  {
    inf->length = LENGTH_HH; // OHHH yes the bytes I'll save from using HH!!!!!!!!111!!!!!!!!JLK@#JL!Q:KEJ:LKSFS:DJ
    next(inf);
    next(inf);
  }
  else if (peek(inf) == 'h')
  {
    inf->length = LENGTH_H;
    next(inf);
  }
  else if (peek(inf) == 'l' && inf->head[1] == 'l')
  {
    inf->length = LENGTH_LL;
    next(inf);
    next(inf);
  }
  else if (peek(inf) == 'l')
  {
    inf->length = LENGTH_L;
    next(inf);
  }
  else if (peek(inf) == 'z' || peek(inf) == 'Z')
  {
    inf->length = LENGTH_Z;
    next(inf);
  }
  else
  {
    inf->length = LENGTH_UNSPEC;
    // inf->head++; why the fuck would you move head if no length specifier
  }
}

/**
 * Write no more than n characters of s to inf->s.
 * Limits of inf will be respected.
 */
static inline size_t
p_lwrite(struct printf_info* restrict inf, const char* restrict s, size_t n)
{
  if (NV_UNLIKELY(inf->written >= inf->max)) return 0;

  size_t len = nv_strlen(s);
  size_t rem = inf->max - inf->written;
  size_t cpy = NV_MIN(len, NV_MIN(rem, n)); // The minimum of string length, n and remaining characters

  return (inf->written += nv_stream_write(s, cpy, inf->s));
}

static inline size_t
p_lputc(struct printf_info* inf, int c)
{
  if (NV_UNLIKELY(inf->written >= inf->max)) return 0;
  return (inf->written += nv_stream_putc(c, inf->s));
}

static inline size_t
p_chr_dispatch(struct printf_info* inf)
{
  next(inf);
  int c = (char)va_arg(*inf->argp, int);
  return p_lputc(inf, c);
}

static inline size_t
p_int_dispatch(struct printf_info* inf)
{
  inf->head++; // skip i/I/d/D

  intmax_t val = 0;
  switch (inf->length)
  {
    case LENGTH_UNSPEC: val = (int)va_arg(*inf->argp, int); break;
    case LENGTH_H: val = (short)va_arg(*inf->argp, int); break;
    case LENGTH_HH: val = (signed char)va_arg(*inf->argp, int); break;
    case LENGTH_L: val = (long)va_arg(*inf->argp, long); break;
    case LENGTH_LL: val = (long long)va_arg(*inf->argp, long long int); break;
    case LENGTH_Z: val = (ssize_t)va_arg(*inf->argp, ssize_t); break;
  }

  size_t write = nv_itoa2(val, inf->scratch, 10, sizeof(inf->scratch), false);
  return p_lwrite(inf, inf->scratch, write);
}

static inline size_t
p_unt_dispatch(struct printf_info* inf)
{
  inf->head++; // skip u/U

  uintmax_t val = 0;
  switch (inf->length)
  {
    case LENGTH_UNSPEC: val = (unsigned)va_arg(*inf->argp, unsigned); break;
    case LENGTH_H: val = (unsigned short)va_arg(*inf->argp, unsigned); break;
    case LENGTH_HH: val = (unsigned char)va_arg(*inf->argp, unsigned); break;
    case LENGTH_L: val = (unsigned long)va_arg(*inf->argp, unsigned long int); break;
    case LENGTH_LL: val = (unsigned long long)va_arg(*inf->argp, unsigned long long int); break;
    case LENGTH_Z: val = (size_t)va_arg(*inf->argp, size_t); break;
  }

  size_t write = nv_utoa2(val, inf->scratch, 10, sizeof(inf->scratch), false);
  return p_lwrite(inf, inf->scratch, write);
}

static inline size_t
p_flt_dispatch(struct printf_info* inf)
{
  inf->head++;

  char scratch[256];

  /* only process in long doubles */
  long double val   = inf->length == LENGTH_L ? va_arg(*inf->argp, long double) : (long double)va_arg(*inf->argp, double);
  size_t      write = nv_fltoa2(val, scratch, inf->prec, sizeof(scratch), true);

  return p_lwrite(inf, scratch, write);
}

/* https://stackoverflow.com/a/75644188 */
static inline char
hex_digit(unsigned v)
{
  return "0123456789abcdef"[v & 0xF];
}

static inline char
hex_digit_upper(unsigned v)
{
  return "0123456789ABCDEF"[v & 0xF];
}

size_t
__hex_conv_helper(uint64_t v, char* out, bool cap)
{
  char   buf[16];
  size_t n = 0;

  if (cap)
  {
    do
    {
      buf[n++] = hex_digit_upper(v);
      v >>= 4;
    } while (v);
  }
  else
  {
    do
    {
      buf[n++] = hex_digit(v);
      v >>= 4;
    } while (v);
  }

  // reverse
  for (size_t i = 0; i < n; i++) out[i] = buf[n - 1 - i];

  return n;
}

static inline size_t
p_hex_dispatch(struct printf_info* inf)
{
  bool        lower = *inf->head == 'x';
  const char* tbl   = lower ? "0123456789abcdef" : "0123456789ABCDEF";
  inf->head++;
  __hex_conv_helper(va_arg(*inf->argp, unsigned), inf->scratch, tbl);
  return p_lwrite(inf, lower ? "0x" : "0X", 2) + p_lwrite(inf, inf->scratch, sizeof(inf->scratch)); /* on success, p_lwrite would just return nv_strlen(scratch) */
}

static inline size_t
p_bin_dispatch(struct printf_info* inf)
{
  inf->head++;

  size_t write = nv_utoa2(va_arg(*inf->argp, unsigned), inf->scratch, 2, sizeof(inf->scratch), false);
  return p_lwrite(inf, inf->scratch, write);
}

static inline size_t
p_ptr_dispatch(struct printf_info* inf)
{
  inf->head++;

  size_t write = nv_ptoa2(va_arg(*inf->argp, void*), inf->scratch, sizeof(inf->scratch));
  return p_lwrite(inf, inf->scratch, write);
}

static inline size_t
p_str_dispatch(struct printf_info* inf)
{
  inf->head++;

  const char* s = va_arg(*inf->argp, const char*);
  if (NV_UNLIKELY(!s)) s = "(null)";

  size_t cpy = nv_strlen(s);
  if (inf->prec_specified) cpy = NV_MIN(cpy, inf->prec); /* If precision specified, min it to the lower of cpy or prec */

  return p_lwrite(inf, s, cpy);
}

static inline size_t
p_pct_dispatch(struct printf_info* inf)
{
  inf->head++;
  return p_lputc(inf, '%');
}

size_t
nv_vssnprintf(nv_stream_t* s, size_t max_chars, const char* fmt, va_list args)
{
  struct printf_info inf = {
    .s       = s,
    .argp    = NULL,
    .written = 0,
    .max     = max_chars,
    .head    = fmt,
    .prec    = 6,
    .width   = 0,
    .length  = LENGTH_UNSPEC,
  };

  va_list argsc;
  va_copy(argsc, args);

  inf.argp = &argsc;

  while (peek(&inf))
  {
    // Out of space?
    // if (inf.written >= inf.max) break;

    /* oo yummy optimizations oo */
    if (NV_LIKELY(*inf.head != '%')) { p_lputc(&inf, *inf.head); }
    else // *inf.head == '%
    {
      // character after %
      next(&inf);

      inf.prec   = DEFAULTPREC;
      inf.width  = 0;
      inf.length = LENGTH_UNSPEC;

      p_parsewidlen(&inf);
      p_parseintlen(&inf);

      // search for format specifier
      // It's the functions job to move the head forwards!
      switch (*inf.head)
      {
        // call the conversion function.
        case 'i':
        case 'd': p_int_dispatch(&inf); break;
        case 'u': p_unt_dispatch(&inf); break;
        case 'x': p_hex_dispatch(&inf); break;
        case 'b': p_bin_dispatch(&inf); break;
        case 'p': p_ptr_dispatch(&inf); break;
        case 's': p_str_dispatch(&inf); break;
        case 'f': p_flt_dispatch(&inf); break;
        case 'c': p_chr_dispatch(&inf); break;
        case '%': p_pct_dispatch(&inf); break;
        default: next(&inf); continue;
      }
      // It's the stream write function's job to increment inf.write, don't do it here.
    }

    next(&inf);
  }

  va_end(argsc);
  return inf.written;
}
