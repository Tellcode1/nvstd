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

#ifndef STD_PROPS_H
#define STD_PROPS_H

// implementation: core.c

#include "attributes.h"
#include "errorcodes.h"
#include "stdafx.h"
#include <stddef.h>

NOVA_HEADER_START

typedef struct nv_option nv_option_t;

typedef enum nv_option_type
{
  NV_OP_TYPE_BOOL, // bool
  NV_OP_TYPE_STRING,
  NV_OP_TYPE_INT,
  NV_OP_TYPE_FLOAT,
  NV_OP_TYPE_DOUBLE
} nv_option_type;

/* (nv_option_t){ .type = , .short_name = , .long_name = , .value = , .buffer_size = , } */
struct nv_option
{
  nv_option_type type;
  const char*    short_name;
  const char*    long_name;

  /*
    Pointer to where the value will be stored
    For strings, pass the buffer instead.
    This CAN be NULL.
  */
  void* value;

  // The size of the char buffer when option type is string
  size_t buffer_size;
} NOVA_ATTR_ALIGNED(64);

/**
 * @brief parse command-line options.
 *
 * @param error buffer for error messages.
 * @param error_size size of the error buffer.
 */
extern nv_error nv_props_parse(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size);

/**
 * @brief generate a help message and write it into buf
 *
 * @param buf_size size of buf for writing the help message to
 */
extern void nv_props_gen_help(const nv_option_t* options, int noptions, char* buf, size_t buf_size);

NOVA_HEADER_END

#endif // STD_PROPS_H
