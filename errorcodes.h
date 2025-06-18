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

#ifndef NOVA_ERROR_CODES_H_INCLUDED_
#define NOVA_ERROR_CODES_H_INCLUDED_

#include "stdafx.h"

NOVA_HEADER_START

typedef enum nv_error
{
  /* A shorthand alternative to NV_ERROR_SUCCESS */
  NV_SUCCESS       = 0,
  NV_ERROR_SUCCESS = NV_SUCCESS,

  /**
   * Invalid argument to function, possibly indirectly.
   */
  NV_ERROR_INVALID_ARG = 1,

  /**
   * A memory allocation has failed. May even be from a stack allocator.
   */
  NV_ERROR_MALLOC_FAILED = -1,

  /**
   * A miscellaneous IO error. These should not typically happen, and may indicate errors in something external.
   */
  NV_ERROR_IO_ERROR = 3,

  /**
   * We are sure the error isn't from our side.
   */
  NV_ERROR_EXTERNAL = 4,

  NV_ERROR_FILE_NOT_FOUND = 5,

  /**
   * The stored cache has been invalidated. This typically should induce a cache rebuild.
   */
  NV_ERROR_INVALID_CACHE = 6,

  /**
   * Our stored state is now broken. This typically is the result of a buffer overflow or memory corruption.
   */
  NV_ERROR_BROKEN_STATE = 7,

  /**
   * Input to the program is invalid, invalid argument by the user.
   */
  NV_ERROR_INVALID_INPUT = 8,

  /**
   * A function returned an invalid value
   */
  NV_ERROR_INVALID_RETVAL = 9,

  /**
   * An unknown error.
   * This typically is the result of something that is out of hand of the returning function.
   * You should avoid returning this, as it is really opaque as to what went wrong.
   */
  NV_ERROR_UNKNOWN = 10,

  /**
   * The operation requested (in/directly) is illegal.
   * The user asked for something they should or could not have.
   */
  NV_ERROR_INVALID_OPERATION = 11,
} nv_error;

/**
 * The supplementary string is specified by the error raising function.
 * It shall contain an extra string for the callee to specify what exactly went wrong.
 * The supplementary string shall not be NULL even if enough information is given by the code
 * or it is irrelevant. It shall be "" in that case.
 * The error should be propogated throught each call.
 */
typedef nv_error (*nv_error_handler_fn)(nv_error error, const char* file, size_t line, const char* supplementary);

extern nv_error _nv_default_error_handler(nv_error error, const char* file, size_t line, const char* supplementary);

static nv_error_handler_fn _error_handler = _nv_default_error_handler;

static inline nv_error
_nv_raise_error(nv_error error, const char* file, size_t line, const char* supplementary)
{
  if (_error_handler)
  {
    return _error_handler(error, file, line, supplementary);
  }
  return error;
}

#define NV_RAISE_ERROR(code, suppl) _nv_raise_error(code, __FILE__, __LINE__, suppl)

static inline void
nv_set_error_handler(nv_error_handler_fn fn)
{
  _error_handler = fn;
}

static inline const char*
nv_error_str(nv_error code)
{
  switch (code)
  {
    case NV_ERROR_SUCCESS: return "Success";
    case NV_ERROR_INVALID_ARG: return "Invalid arg";
    case NV_ERROR_MALLOC_FAILED: return "Allocation failed";
    case NV_ERROR_IO_ERROR: return "IO error";
    case NV_ERROR_EXTERNAL: return "External";
    case NV_ERROR_FILE_NOT_FOUND: return "File not found";
    case NV_ERROR_INVALID_CACHE: return "Invalid cache";
    case NV_ERROR_BROKEN_STATE: return "Broken state";
    case NV_ERROR_INVALID_INPUT: return "Invalid input";
    case NV_ERROR_INVALID_RETVAL: return "Invalid retval";
    case NV_ERROR_UNKNOWN: return "Unknown error";
    case NV_ERROR_INVALID_OPERATION: return "Invalid operation";
    default: return "(NotAnErrorCode)";
  }
}

NV_STATIC_ASSERT(sizeof(nv_error) == sizeof(int), sizeof_erroc_must_be_sizeof_int);

NOVA_HEADER_END

#endif //__NOVA_ERROR_CODES_H__
