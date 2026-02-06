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

#include "../include/error.h"
#include "../include/print.h"
#include "../include/props.h"
#include "../include/strconv.h"
#include "../include/string.h"

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
nv_default_error_handler(nv_error error, const char* fn, const char* file, size_t line, const char* supplementary, va_list args)
{
  printf("[%s:%zu] function %s raised %s", nv_basename(file), line, fn, nv_error_str(error));
  if (nv_strcmp(supplementary, "") != 0) { printf("\n"); }
  else
  {
    printf(": ");
  }
  vprintf(supplementary, args);
  fputc('\n', stdout);

  return error;
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

  fprintf(out, "[%s:%zu]%s%s(): ", nv_basename(file), line, preceder, fn);
  vfprintf(out, fmt, args);
}

static inline const nv_option_desc_t*
nv_option_find(const nv_option_desc_t* options, int noptions, const char* short_name, const char* long_name)
{
  nv_assert(noptions >= 0);

  size_t i = 0;
  for (const nv_option_desc_t* opt = options; i < (size_t)noptions; opt++, i++)
  {
    if (short_name && opt->short_name && nv_strcmp(opt->short_name, short_name) == 0) { return opt; }
    if (long_name && opt->long_name && nv_strcmp(opt->long_name, long_name) == 0) { return opt; }
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
nv_props_generate_help_message(const nv_option_desc_t* options, int noptions, char* buf, size_t buf_size)
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
    const nv_option_desc_t* opt = &options[i];

    const char* short_name = opt->short_name ? opt->short_name : "<empty>";
    const char* long_name  = opt->long_name ? opt->long_name : "<empty>";

    written = nv_snprintf(buf, available, "\t-%s, --%s <%s>\n", short_name, long_name, nv_props_get_tp_name(opt->type));
    buf += written;
    if (written > available) { break; }
    available -= written;
  }
}

static inline nv_error
nv_props_parse_arg(int argc, char* argv[], const nv_option_desc_t* options, int noptions, char* error, size_t error_size, int* i)
{
  nv_assert_else_return(argc != 0, NV_ERROR_INVALID_ARG);

  nv_assert_else_return(noptions > 0, NV_ERROR_INVALID_ARG);

  char* arg     = argv[*i];
  bool  is_long = false;
  char* name    = NULL;
  char* value   = NULL;

  // not option?
  if (arg[0] != '-') { return 0; }

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

  const nv_option_desc_t* opt = is_long ? nv_option_find(options, noptions, NULL, name) : nv_option_find(options, noptions, name, NULL);
  if (!opt)
  {
    if (error && error_size > 0) { nv_snprintf(error, error_size, "unknown option: %s%s", is_long ? "--" : "-", name); }
    (*i)++;
    return -1;
  }

  if (opt->type == NV_OP_TYPE_BOOL)
  {
    bool flag_value = true;
    if (is_long && value) { flag_value = nv_atobool(value, NOVA_MAX_IGNORE); }
    if (opt->value) { *(bool*)opt->value = flag_value; }
    (*i)++;
    return 0;
  }

  if (!value)
  {
    if (!is_long)
    {
      size_t opt_name_len = nv_strlen(opt->short_name);
      size_t arg_name_len = nv_strlen(name);
      if (arg_name_len > opt_name_len) { value = name + opt_name_len; }
      else if ((*i) + 1 < argc && argv[*i + 1][0] != '-') { value = argv[++(*i)]; }
    }
    else
    {
      (*i)++;
      if ((*i) >= argc || argv[*i][0] == '-')
      {
        if (error && error_size > 0) { nv_snprintf(error, error_size, "option --%s requires a value", name); }
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
      case NV_OP_TYPE_INT: *(int*)opt->value = (int)nv_atoi(value); break;
      case NV_OP_TYPE_FLOAT: *(float*)opt->value = (float)nv_atof(value); break;
      case NV_OP_TYPE_DOUBLE: *(double*)opt->value = nv_atof(value); break;
      default: break;
    }
  }

  (*i)++;
  return NV_ERROR_SUCCESS;
}

nv_error
nv_props_parse(int argc, char* argv[], const nv_option_desc_t* options, int noptions, char* error, size_t error_size)
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
      if (result != 0) { errcode = false; }
    }
    else
    {
      break;
    }
  }
  return errcode;
}
