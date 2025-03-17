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

#ifndef __NOVA_PROGRAM_OPTIONS_H__
#define __NOVA_PROGRAM_OPTIONS_H__

// implementation: core.c

#include "stdafx.h"

NOVA_HEADER_START

typedef struct nv_option_t nv_option_t;

typedef enum nv_option_type
{
  NV_OP_TYPE_BOOL, // bool
  NV_OP_TYPE_STRING,
  NV_OP_TYPE_INT,
  NV_OP_TYPE_FLOAT,
  NV_OP_TYPE_DOUBLE
} nv_option_type;

struct nv_option_t
{
  nv_option_type m_type;
  const char*    m_short_name;
  const char*    m_long_name;

  /*
    Pointer to where the value will be stored
    For strings, pass the buffer instead.
    This CAN be NULL.
  */
  void* m_value;

  // The size of the char buffer when option type is string
  size_t m_buffer_size;
};

/**
 * @brief parse command-line options.
 *
 * @param error buffer for error messages.
 * @param error_size size of the error buffer.
 */
extern int nv_props_parse(int argc, char* argv[], const nv_option_t* options, int noptions, char* error, size_t error_size);

/**
 * @brief generate a help message and write it into buf
 *
 * @param buf_size size of buf for writing the help message to
 */
extern void nv_props_gen_help(const nv_option_t* options, int noptions, char* buf, size_t buf_size);

NOVA_HEADER_END

#endif //__NOVA_PROGRAM_OPTIONS_H__
