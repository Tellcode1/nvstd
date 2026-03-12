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
  if (strcmp(supplementary, "") != 0) { printf("\n"); }
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
