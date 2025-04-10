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

#ifndef __NOVA_ERROR_CODES_H__
#define __NOVA_ERROR_CODES_H__

#include "stdafx.h"

NOVA_HEADER_START

typedef enum nv_errorc
{
  /* A shorthand alternative to NOVA_ERROR_CODE_SUCCESS */
  NOVA_SUCCESS            = 0,
  NOVA_ERROR_CODE_SUCCESS = NOVA_SUCCESS,

  /**
   * Invalid argument to function, possibly indirectly.
   */
  NOVA_ERROR_CODE_INVALID_ARG = 1,

  /**
   * A memory allocation has failed. May even be from a stack allocator.
   */
  NOVA_ERROR_CODE_MALLOC_FAILED = -1,

  /**
   * A miscellaneous IO error. These should not typically happen, and may indicate errors in something external.
   */
  NOVA_ERROR_CODE_IO_ERROR = 3,

  /**
   * We are sure the error isn't from our side.
   */
  NOVA_ERROR_CODE_EXTERNAL = 4,

  NOVA_ERROR_CODE_FILE_NOT_FOUND = 5,

  /**
   * The stored cache has been invalidated. This typically should induce a cache rebuild.
   */
  NOVA_ERROR_CODE_INVALID_CACHE = 6,

  /**
   * Our stored state is now broken. This typically is the result of a buffer overflow.
   */
  NOVA_ERROR_CODE_BROKEN_STATE = 7,

  /**
   * Input to the program is invalid, invalid argument by the user.
   */
  NOVA_ERROR_CODE_INVALID_INPUT = 8,

  /**
   * A function returned an invalid value
   */
  NOVA_ERROR_CODE_INVALID_RETVAL = 9,

  /**
   * An unknown error.
   * This typically is the result of something that is out of hand of the returning function.
   * You should avoid returning this, as it is really opaque as to what went wrong.
   */
  NOVA_ERROR_CODE_UNKNOWN = 10
} nv_errorc;

static inline const char*
nv_error_str(nv_errorc code)
{
  switch (code)
  {
    case NOVA_ERROR_CODE_SUCCESS: return "Success";
    case NOVA_ERROR_CODE_INVALID_ARG: return "Invalid arg";
    case NOVA_ERROR_CODE_MALLOC_FAILED: return "malloc failed";
    case NOVA_ERROR_CODE_IO_ERROR: return "IO error";
    case NOVA_ERROR_CODE_EXTERNAL: return "External";
    case NOVA_ERROR_CODE_FILE_NOT_FOUND: return "File not found";
    case NOVA_ERROR_CODE_INVALID_CACHE: return "Invalid cache";
    case NOVA_ERROR_CODE_BROKEN_STATE: return "Broken state";
    case NOVA_ERROR_CODE_INVALID_INPUT: return "Invalid input";
    case NOVA_ERROR_CODE_INVALID_RETVAL: return "Invalid retval";
    case NOVA_ERROR_CODE_UNKNOWN: return "Unknown";
    default: return "(NotAnErrorCode)";
  }
}

NV_STATIC_ASSERT(sizeof(nv_errorc) == sizeof(int), sizeof_erroc_must_be_sizeof_int);

NOVA_HEADER_END

#endif //__NOVA_ERROR_CODES_H__
