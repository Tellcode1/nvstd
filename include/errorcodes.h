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

#ifndef NV_STD_ERRORCODES_H
#define NV_STD_ERRORCODES_H

#include "stdafx.h"

#include <stdarg.h>
#include <stddef.h>

NOVA_HEADER_START

#define nv_raise_error(code, ...) _nv_raise_error(code, __func__, __FILE__, __LINE__, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#define nv_raise_and_return(code, ...)                                                                                                                                        \
  _nv_raise_error(code, __func__, __FILE__, __LINE__, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__));                                                      \
  return code

/* Helper macro for asserting that a pointer arguement is not NULL */
#define nv_assert_ptr(ptr)                                                                                                                                                    \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    if (ptr == NULL) { nv_raise_error(NV_ERROR_INVALID_ARG, "Pointer %s NULL!\n", #ptr); }                                                                                    \
  } while (0)

/**
 * Assert that an expression must return NV_SUCCESS. If it does not, return the 2nd parameter
 */
#define nv_return_error_if_fail(expr)                                                                                                                                         \
  do                                                                                                                                                                          \
  {                                                                                                                                                                           \
    nv_error __tmp_error_code_eval = (expr);                                                                                                                                  \
    if (__tmp_error_code_eval != NV_SUCCESS) { return __tmp_error_code_eval; }                                                                                                \
  } while (0);

typedef enum nv_error
{
  /* A shorthand alternative to NV_ERROR_SUCCESS */
  NV_SUCCESS       = 0,
  NV_ERROR_SUCCESS = NV_SUCCESS,

  /**
   * Invalid argument to function, possibly indirectly.
   */
  NV_ERROR_INVALID_ARG,

  /**
   * A memory allocation has failed. May even be from a stack allocator.
   */
  NV_ERROR_MALLOC_FAILED,

  /**
   * A miscellaneous IO error. These should not typically happen, and may indicate errors in something external.
   */
  NV_ERROR_IO_ERROR,

  /**
   * We are sure the error isn't from our side.
   */
  NV_ERROR_EXTERNAL,

  /**
   * Generally used as file or directory not found.
   * But a more general no exist error code.
   * Just make sure to pass the reason in the supplementary string.
   */
  NV_ERROR_NO_EXIST,

  /**
   * Entry or object already exists.
   */
  NV_ERROR_EXIST,

  /**
   * Permissions either out of scope, or you tried to
   * execute a non executable file.
   */
  NV_ERROR_INSUFFICIENT_PERMISSIONS,

  /**
   * The stored cache has been invalidated. This typically should induce a cache rebuild.
   */
  NV_ERROR_INVALID_CACHE,

  /**
   * Our stored state is now broken. This typically is the result of a buffer overflow or memory corruption.
   */
  NV_ERROR_BROKEN_STATE,

  /**
   * Input to the program is invalid, invalid argument by the user.
   */
  NV_ERROR_INVALID_INPUT,

  /**
   * A function returned an invalid value or failed. We do not know why and it was most defenitely not our fault.
   */
  NV_ERROR_INVALID_RETVAL,

  /**
   * The operation requested (in/directly) is illegal.
   * The user asked for something they should or could not have.
   */
  NV_ERROR_INVALID_OPERATION,

  /**
   * An unknown error.
   * This typically is the result of something that is out of hand of the returning function.
   * You should avoid returning this, as it is really opaque as to what went wrong.
   */
  NV_ERROR_UNKNOWN,
} nv_error;

/**
 * The supplementary string is specified by the error raising function.
 * It shall contain an extra string for the callee to specify what exactly went wrong.
 * The supplementary string shall not be NULL even if enough information is given by the code
 * or it is irrelevant. It shall be "" in that case.
 * The error should be propogated throught each call.
 */
typedef nv_error (*nv_error_handler_fn)(nv_error error, const char* fn, const char* file, size_t line, const char* supplementary, va_list args);

extern nv_error nv_default_error_handler(nv_error error, const char* fn, const char* file, size_t line, const char* supplementary, va_list args);

static nv_error_handler_fn error_handler = nv_default_error_handler;

static inline nv_error
_nv_raise_error(nv_error error, const char* fn, const char* file, size_t line, const char* supplementary, ...)
{
  va_list args;
  va_start(args, supplementary);

  // What the fuck???
  // if (error_handler != NULL != NULL != NULL != NULL != NULL)
  if (error_handler != NULL)
  {
    nv_error passthrough = error_handler(error, fn, file, line, supplementary, args);
    va_end(args);
    return passthrough;
  }
  return error;
}

static inline void
nv_set_error_handler(nv_error_handler_fn fn)
{
  error_handler = fn;
}

static inline const char*
nv_error_str(nv_error code)
{
  switch (code)
  {
    case NV_ERROR_SUCCESS: return "success";
    case NV_ERROR_INVALID_ARG: return "invalid arg";
    case NV_ERROR_MALLOC_FAILED: return "allocation failed";
    case NV_ERROR_IO_ERROR: return "io error";
    case NV_ERROR_EXTERNAL: return "external";
    case NV_ERROR_NO_EXIST: return "object inexistent";
    case NV_ERROR_INVALID_CACHE: return "invalid cache";
    case NV_ERROR_BROKEN_STATE: return "broken state";
    case NV_ERROR_INVALID_INPUT: return "invalid input";
    case NV_ERROR_INVALID_RETVAL: return "invalid retval";
    case NV_ERROR_INVALID_OPERATION: return "invalid operation";
    default:
    case NV_ERROR_UNKNOWN: return "unknown error";
  }
}

NV_STATIC_ASSERT(sizeof(nv_error) == sizeof(int), sizeof_erroc_must_be_sizeof_int);

NOVA_HEADER_END

#endif // NV_STD_ERRORCODES_H
