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

#ifndef NV_ASSERT_H
#define NV_ASSERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NDEBUG
#  define nv_assert_else_return(expr, retval)                                                                                                                                 \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                       \
      {                                                                                                                                                                       \
        nv_log_error("Assertion failed -> %s\n", #expr);                                                                                                                      \
        return retval;                                                                                                                                                        \
      }                                                                                                                                                                       \
    } while (0)
#  define nv_assert_and_exec(expr, code)                                                                                                                                      \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr)))) { nv_log_error("Assertion failed -> %s\n", #expr); }                                                                                  \
      code                                                                                                                                                                    \
    } while (0)
#  define nv_assert(expr)                                                                                                                                                     \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr)))) { nv_log_and_abort("Assertion failed -> %s\n", #expr); }                                                                              \
    } while (0)
#else
// These are typecasted to void because they give warnings because result (its
// like expr != NULL) is not used
#  define nv_assert_else_return(expr, retval) (void)(expr)
#  define nv_assert_and_exec(expr, code) (void)(expr)
#  define nv_assert(expr) (void)(expr)
#  pragma message("Assertions disabled")
#endif

#ifdef __cplusplus
}
#endif

#endif // NV_ASSERT_H
