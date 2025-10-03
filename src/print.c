#include "../include/print.h"
#include "../include/attributes.h"
#include "../include/chrclass.h"
#include "../include/stdafx.h"
#include "../include/strconv.h"
#include "../include/string.h"
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PADBUF_SIZE 64

// Moral of the story? FU@# SIZE_MAX
// I spent an HOUR trying to figure out what's going wrong
// and I didn't even bat an eye towards it

size_t
nv_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  // size_t chars_written = nv_vnprintf(NOVA_WBUF_SIZE, args, fmt);
  size_t chars_written = nv_vsfnprintf(args, stdout, 1, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_fprintf(FILE* f, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = nv_vsfnprintf(args, f, 1, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_sprintf(char* dst, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = nv_vsfnprintf(args, dst, 0, SIZE_MAX, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vprintf(va_list args, const char* fmt)
{
  return nv_vsfnprintf(args, stdout, 1, SIZE_MAX, fmt);
}

size_t
nv_vfprintf(va_list args, FILE* f, const char* fmt)
{
  return nv_vsfnprintf(args, f, 1, SIZE_MAX, fmt);
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

  size_t chars_written = nv_vsfnprintf(args, stdout, 1, max_chars, fmt);

  va_end(args);

  return chars_written;
}

size_t
nv_vnprintf(va_list args, size_t max_chars, const char* fmt)
{
  return nv_vsfnprintf(args, stdout, 1, max_chars, fmt);
}

size_t
nv_vsnprintf(va_list src, char* dst, size_t max_chars, const char* fmt)
{
  return nv_vsfnprintf(src, dst, 0, max_chars, fmt);
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
} NOVA_ATTR_ALIGNED(128) nv_format_info_t;

static inline void
nv_printf_write(nv_format_info_t* info, const char* write_buffer, size_t written)
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
nv_printf_get_padding_and_precision_if_given(nv_format_info_t* info)
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
nv_printf_format_process_char(nv_format_info_t* info)
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
nv_printf_handle_long_type(nv_format_info_t* info)
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
nv_printf_handle_size_type(nv_format_info_t* info)
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
nv_printf_handle_hash_prefix_and_children(nv_format_info_t* info)
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
nv_printf_format_parse_string(nv_format_info_t* info)
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

  nv_printf_write(info, string, string_length);
  info->wbuffer_used = false;
}

static inline void
nv_printf_format_parse_format(nv_format_info_t* info)
{
  if (!NV_PRINTF_PEEK_FMT())
  {
    return;
  }

  size_t remaining = info->max_chars - info->chars_written;

  switch (nv_chr_tolower(NV_PRINTF_PEEK_FMT()))
  {
    /* By default, ftoa should not trim trailing zeroes. */
    case 'f': info->written = nv_ftoa2(va_arg(info->args, double), info->tmp_writebuffer, info->precision, remaining, false); break;
    case 'l': nv_printf_handle_long_type(info); break;
    case 'd':
    case 'i': info->written = nv_itoa2(va_arg(info->args, int), info->tmp_writebuffer, 10, remaining, NOVA_PRINTF_ADD_COMMAS); break;
    case 'u': info->written = nv_utoa2(va_arg(info->args, unsigned), info->tmp_writebuffer, 10, remaining, NOVA_PRINTF_ADD_COMMAS); break;

    /* size_t based formats. This is standard. */
    case 'z': nv_printf_handle_size_type(info); break;

    /* hash tells us that we need to have the 0x prefix (or the 0X prefix) */
    case '#': nv_printf_handle_hash_prefix_and_children(info); break;

    /* hex integer */
    case 'x': info->written = nv_utoa2(va_arg(info->args, unsigned), info->tmp_writebuffer, 16, info->max_chars - info->chars_written, false); break;

    /* pointer */
    case 'p': info->written = nv_ptoa2(va_arg(info->args, void*), info->tmp_writebuffer, remaining); break;

    /* bytes, custom */
    case 'b': info->written = nv_btoa2(va_arg(info->args, size_t), 1, info->tmp_writebuffer, remaining); break;

    case 's': nv_printf_format_parse_string(info); break;

    case 'c':
    /* If there are two % symbols in a row */
    case '%':
    default: nv_printf_format_process_char(info); break;
  }
}

static inline void
nv_printf_write_padding(nv_format_info_t* info)
{
  info->padding = info->padding_w - (int)info->written;
  char pad_char = info->pad_zero ? '0' : ' ';
  if (info->left_align && info->padding <= 0)
  {
    return;
  }

  /* No, we cannot run out of padding buffer space, because we write it in chunks. */

  nv_memset(info->pad_buf, pad_char, PADBUF_SIZE);
  while (info->padding > 0)
  {
    int chunk = (info->padding > PADBUF_SIZE) ? PADBUF_SIZE : info->padding;
    nv_printf_write(info, info->pad_buf, chunk);
    info->padding -= chunk;
  }
}

static inline void
nv_printf_format_upload_to_destination(nv_format_info_t* info)
{
  if (!info->wbuffer_used)
  {
    return;
  }

  nv_printf_write_padding(info);

  nv_printf_write(info, info->tmp_writebuffer, info->written);

  nv_printf_write_padding(info);
}

static inline void
nv_printf_format_parse_format_specifier(nv_format_info_t* info)
{
  info->wbuffer_used        = true;
  info->padding_w           = 0;
  info->precision           = 6;
  info->precision_specified = 0;

  nv_printf_get_padding_and_precision_if_given(info);

  nv_printf_format_parse_format(info);

  nv_printf_format_upload_to_destination(info);
}

static inline void
nv_printf_write_iterated_char(nv_format_info_t* info)
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
nv_printf_loop(nv_format_info_t* info)
{
  for (; NV_PRINTF_PEEK_FMT() && info->chars_written < info->max_chars; info->itr++)
  {
    if (NV_PRINTF_PEEK_FMT() == '%')
    {
      NV_PRINTF_ADVANCE_FMT();
      nv_printf_format_parse_format_specifier(info);
    }
    else
    {
      nv_printf_write_iterated_char(info);
    }
  }
}

size_t
nv_vsfnprintf(va_list args, void* dst, bool is_file, size_t max_chars, const char* fmt)
{
  if (max_chars == 0)
  {
    return 0;
  }

  nv_format_info_t info = nv_zero_init(nv_format_info_t);

  /* Aligned to 64 bytes for fast writing and reading access */
  NV_ALIGN_TO(64) char pad_buf[PADBUF_SIZE];

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

  nv_printf_loop(&info);

  if (!is_file && info.dst_string && max_chars > 0)
  {
    size_t width        = (info.chars_written < max_chars) ? info.chars_written : max_chars - 1;
    ((char*)dst)[width] = 0;
  }

  va_end(info.args);

  return info.chars_written;
}
