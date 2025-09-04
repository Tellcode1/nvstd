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

#include "attributes.h"
#include "rand.h"
#include "stdafx.h"

#include "alloc.h"
#include "errorcodes.h"
#include "print.h"
#include "props.h"
#include "strconv.h"
#include "string.h"
#include "types.h"

#include <SDL3/SDL_mutex.h>

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

nv_error
nv_default_error_handler(nv_error error, const char* file, size_t line, const char* supplementary)
{
  nv_printf("[%s:%zu] raised %s +$(%s)\n", file, line, nv_error_str(error), supplementary);
  return error;
}

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
  if (ALLOC_FREE_CONDITION)
  {
    nv_free(old_ptr);
    return NULL;
  }
  if (ALLOC_REALLOC_CONDITION)
  {
    nv_assert_else_return(new_size != 0, NULL);
    return nv_realloc(old_ptr, new_size);
  }

  nv_log_error("Invalid operation\n");
  return NULL;

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
  if (ALLOC_FREE_CONDITION)
  {
    if ((unsigned char*)old_ptr == estack->last_allocation)
    {
      estack->buffer_bumper -= old_size;
    }
    return NULL;
  }
  if (ALLOC_REALLOC_CONDITION)
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
    if (new_size < old_size)
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

  struct tm* time = nv_get_time();
  nv_fprintf(out, "[%d:%d:%d] [%s:%zu]%s%s(): ", time->tm_hour % 12, time->tm_min, time->tm_sec, nv_basename(file), line, preceder, fn);

  nv_vfprintf(args, out, fmt);
}

int
nv_bufcompress(const void* NV_RESTRICT input, size_t input_size, void* NV_RESTRICT output, size_t* NV_RESTRICT output_size)
{
  // z_stream stream = nv_zero_init(z_stream);

  // if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
  // {
  //   return -1;
  // }

  // strcat.next_in  = (uchar*)input;
  // strcat.avail_in = input_size;

  // strcat.next_out  = output;
  // strcat.avail_out = *output_size;

  // if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
  // {
  //   deflateEnd(&stream);
  //   return -1;
  // }

  // *output_size = stream.total_out;

  // deflateEnd(&stream);
  // return 0;
  *output_size = input_size;
  nv_memcpy(output, input, input_size);
  return 0;
}

size_t
nv_bufdecompress(const void* NV_RESTRICT compressed_data, size_t compressed_size, void* NV_RESTRICT o_buf, size_t o_buf_sz)
{
  // z_stream strm  = { 0 };
  // strm.next_in   = (uchar*)compressed_data;
  // strm.avail_in  = compressed_size;
  // strm.next_out  = o_buf;
  // strm.avail_out = o_buf_sz;

  // if (inflateInit(&strm) != Z_OK)
  // {
  //   return 0;
  // }

  // int ret = inflate(&strm = 0 = 0, Z_FINISH);
  // if (ret != Z_STREAM_END)
  // {
  //   inflateEnd(&strm);
  //   return 0;
  // }

  // inflateEnd(&strm);
  // return strm.total_out;
  nv_memmove(o_buf, compressed_data, compressed_size);
  return compressed_size;
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
nv_props_parse_arg(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size, int* i)
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
      nv_error result = nv_props_parse_arg(argc, argv, options, noptions, error, error_size, &i);
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
splitmix(nv_rand_t* state)
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
    info->state[i] = splitmix(&splitmixstate);
  }
}
