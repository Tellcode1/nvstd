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

#ifndef __NOVA_STDAFX_H__
#define __NOVA_STDAFX_H__

// implementation: core.c

#define __NOVA_STD_VERSION_MAJOR__ 0
#define __NOVA_STD_VERSION_MINOR__ 1
#define __NOVA_STD_VERSION_PATCH__ 1

#include <SDL2/SDL_mutex.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "types.h"

#ifdef __cplusplus
#  define NOVA_HEADER_START                                                                                                                                                   \
    extern "C"                                                                                                                                                                \
    {
#  define NOVA_HEADER_END }
#else
#  define NOVA_HEADER_START
#  define NOVA_HEADER_END
#endif

NOVA_HEADER_START

#if !defined(NV_RESTRICT)
#  if defined(_MSC_VER)
#    define NV_RESTRICT __restrict
#  elif defined(___GNUC__) || defined(__clang__)
#    define NV_RESTRICT __restrict__
#  else
#    define NV_RESTRICT
#  endif
#endif

#ifndef NV_TYPEOF
#  if defined(___GNUC__) || defined(__clang__)
#    define NV_TYPEOF(x) __typeof__(x)
#  elif defined(_MSC_VER)
#    define NV_TYPEOF(x) decltype(x)
#  else
/* As Typeof is typically used in casts, we can not compile without typeof */
#    error "no typeof"
#  endif
#endif

#ifndef NV_ALIGN_TO
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L // C11+
#    define NV_ALIGN_TO(N) _Alignas(N)
#  elif defined(___GNUC__) || defined(__clang__)
#    define NV_ALIGN_TO(N) __attribute__((aligned(N)))
#  elif defined(_MSC_VER) // MSVC
#    define NV_ALIGN_TO(N) __declspec(align(N))
#  elif defined(__INTEL_COMPILER)
#    define NV_ALIGN_TO(N) __declspec(align(N))
#  else
#    error "no align"
#  endif
#endif

#ifndef NV_USED
#  if defined(___GNUC__) || defined(__clang__)
#    define NV_USED __attribute__((__used__))
#  else
#    define NV_USED
#  endif
#endif

/* [fallthrough] is a C23 extension. I get it now. Shut up please. */
#if defined(___GNUC__) && ___GNUC__ >= 7 || defined(__clang__) && __clang_major__ >= 12
#  define NV_FALLTHROUGH __attribute__((fallthrough))
#else
#  define NV_FALLTHROUGH /* fallthrough */
#endif

#if defined(___GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_expect)
#  define NV_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#  define NV_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
/* Note that equals must be a long (or convertible to a long) */
#  define NV_EXPECT_EQUALS(expr, equals) (__builtin_expect(!!(expr), equals))
#else
#  define NV_LIKELY(expr) (expr)
#  define NV_UNLIKELY(expr) (expr)
#  define NV_EXPECT_EQUALS(expr, equals) (expr)
#endif

/* If enabled, error messages are instantaneously printed, otherwise, they are added to a queue */
#ifndef NV_UNBUFFERED_ERRORS
#  define NV_UNBUFFERED_ERRORS true
#endif

#ifndef real_t
#  define real_t double
#endif

#ifndef flt_t
#  define flt_t float
#endif

#define DEBUG

#define NV_MAX(a, b) ((a) > (NV_TYPEOF(a))(b) ? (a) : (NV_TYPEOF(a))(b))
#define NV_MIN(a, b) ((a) < (NV_TYPEOF(a))(b) ? (a) : (NV_TYPEOF(a))(b))
#define NV_CONCAT(x, y) x##y

/*
 * GNUC and builtin have protection from accidentally passing in pointers instead of stack arrays
 */
#if defined(___GNUC__) && (__STDC_VERSION__ >= 201112L)
#  define nv_arrlen(arr) _Generic(&(arr), NV_TYPEOF(*(arr))(*): 0, default: (sizeof(arr) / sizeof((arr)[0])))
#elif defined(__has_builtin) && __has_builtin(__builtin_choose_expr) && __has_builtin(__builtin_types_compatible_p)
#  define nv_arrlen(arr) __builtin_choose_expr(__builtin_types_compatible_p(NV_TYPEOF(arr), NV_TYPEOF(&(arr)[0])), 0, (sizeof(arr) / sizeof((arr)[0])))
#else
#  define nv_arrlen(arr) ((size_t)(sizeof(arr) / sizeof((arr)[0])))
#endif

#ifndef nv_zero_init
#  ifndef __cplusplus
#    define nv_zero_init(TYPE) (TYPE){ 0 }
#  else
#    define nv_zero_init(TYPE)                                                                                                                                                \
      (TYPE) {}
#  endif
#endif

/* https://stackoverflow.com/a/11172679 */
/* Stupid fix to , ##__VA_ARGS__ being a GNU extension */
/* Only supports up to 10 arguments! However, increasing the limit is easy */
/* Go to __GNUC_HELP_ME_PLEASE_SELECT_10TH and just add more variables and set the define to the last one */

#define __GNUC_HELP_ME_PLEASE_FIRST(...) __GNUC_HELP_ME_PLEASE_FIRST_HELPER(__VA_ARGS__, throwaway)
#define __GNUC_HELP_ME_PLEASE_FIRST_HELPER(first, ...) first

/*
 * if there's only one argument, expands to nothing.  if there is more
 * than one argument, expands to a comma followed by everything but
 * the first argument.  only supports up to 10 arguments but can be
 * trivially expanded.
 */
#define __GNUC_HELP_ME_PLEASE_REST(...) __GNUC_HELP_ME_PLEASE_REST_HELPER(__GNUC_HELP_ME_PLEASE_NUM(__VA_ARGS__), __VA_ARGS__)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER(qty, ...) __GNUC_HELP_ME_PLEASE_REST_HELPER2(qty, __VA_ARGS__)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER2(qty, ...) __GNUC_HELP_ME_PLEASE_REST_HELPER_##qty(__VA_ARGS__)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER_ONE(first)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define __GNUC_HELP_ME_PLEASE_NUM(...)                                                                                                                                        \
  __GNUC_HELP_ME_PLEASE_SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define __GNUC_HELP_ME_PLEASE_SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10

#define nv_push_error(...) _nv_push_error(__FILE__, __LINE__, __func__, (_nv_get_time()), __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))

extern const char* nv_pop_error(void);

/* Print all error messages in the queue and clean it */
extern void nv_flush_errors(void);

#ifndef NDEBUG
#  define nv_assert_and_ret(expr, retval)                                                                                                                                     \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                       \
      {                                                                                                                                                                       \
        nv_push_error("Assertion failed -> %s", #expr);                                                                                                                       \
        return retval;                                                                                                                                                        \
      }                                                                                                                                                                       \
    } while (0);
#  define nv_assert(expr)                                                                                                                                                     \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                       \
      {                                                                                                                                                                       \
        nv_push_error("Assertion failed -> %s", #expr);                                                                                                                       \
      }                                                                                                                                                                       \
    } while (0);
#else
// These are typecasted to void because they give warnings because result (its
// like expr != NULL) is not used
#  define nv_assert_and_ret(expr, retval) (void)(expr)
#  define nv_assert(expr) (void)(expr)
#  pragma message "Assertions disabled"
#endif

// puts but with formatting and with the preceder "error". does not stop execution of program if you want that, use nv_log_and_abort instead.
#define nv_log_error(...) _nv_log_error(__FILE__, __LINE__, __func__, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))
// formats the string, puts() it with the preceder "fatal error" and then aborts the program
#define nv_log_and_abort(...) _nv_log_and_abort(__FILE__, __LINE__, __func__, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))
// puts but with formatting and with the preceder "warning"
#define nv_log_warning(...) _nv_log_warning(__FILE__, __LINE__, __func__, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))
// puts but with formatting and with the preceder "info"
#define nv_log_info(...) _nv_log_info(__FILE__, __LINE__, __func__, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))
// puts but with formatting and with the preceder "debug"
#define nv_log_debug(...) _nv_log_debug(__FILE__, __LINE__, __func__, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))
// puts but with formatting and with a custom preceder
#define nv_log_custom(preceder, ...) _nv_log_custom(__FILE__, __LINE__, __func__, preceder, __GNUC_HELP_ME_PLEASE_FIRST(__VA_ARGS__) __GNUC_HELP_ME_PLEASE_REST(__VA_ARGS__))

extern void _nv_log_error(const char* file, size_t line, const char* func, const char* fmt, ...);
extern void _nv_log_and_abort(const char* file, size_t line, const char* func, const char* fmt, ...);
extern void _nv_log_warning(const char* file, size_t line, const char* func, const char* fmt, ...);
extern void _nv_log_info(const char* file, size_t line, const char* func, const char* fmt, ...);
extern void _nv_log_debug(const char* file, size_t line, const char* func, const char* fmt, ...);
extern void _nv_log_custom(const char* file, size_t line, const char* func, const char* preceder, const char* fmt, ...);

extern void _nv_log(va_list args, const char* file, size_t line, const char* fn, const char* preceder, const char* str, bool err);

/* printf's the time to stdout. yep. [hour:minute:second]*/
extern void nv_print_time_as_string(FILE* stream);

extern void _nv_push_error(const char* file, size_t line, const char* func, struct tm* time, const char* fmt, ...);

static inline struct tm*
_nv_get_time(void)
{
  time_t     now;
  struct tm* tm;

  now = time(0);
  if ((tm = localtime(&now)) == NULL)
  {
    nv_push_error("Error extracting time stuff");
    return NULL;
  }

  return tm;
}

#define NOVA_CALL_FILE_FN(fn)                                                                                                                                                 \
  if (NV_UNLIKELY((fn) != 0))                                                                                                                                                 \
  {                                                                                                                                                                           \
    nv_push_error("%s() => %i", #fn, errno);                                                                                                                                  \
  }

NOVA_HEADER_END

#endif
