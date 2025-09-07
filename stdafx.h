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

#ifndef STD_STDAFX_H
#define STD_STDAFX_H

// implementation: core.c

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define NOVA_STD_VERSION_MAJOR_ 0
#define NOVA_STD_VERSION_MINOR_ 2
#define NOVA_STD_VERSION_PATCH_ 0

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
#  elif (defined(__GNUC__) || defined(__clang__)) && __STDC_VERSION__ >= 199901L
#    define NV_RESTRICT __restrict__
#  else
#    define NV_RESTRICT
#  endif
#endif

#ifndef NV_TYPEOF
#  if defined(__GNUC__) || defined(__clang__)
#    define NV_TYPEOF(x) __typeof__(x)
#  elif defined(_MSC_VER)
#    define NV_TYPEOF(x) decltype(x)
#  else
#    pragma message("NV_TYPEOF not available")
#    define NV_TYPEOF(x)
#  endif
#endif

#ifndef NV_ALIGN_TO
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L // C11+
#    define NV_ALIGN_TO(N) _Alignas(N)
#  elif defined(__GNUC__) || defined(__clang__)
#    define NV_ALIGN_TO(N) __attribute__((aligned(N)))
#  elif defined(_MSC_VER) // MSVC
#    define NV_ALIGN_TO(N) __declspec(align(N))
#  elif defined(__INTEL_COMPILER)
#    define NV_ALIGN_TO(N) __declspec(align(N))
#  else
#    pragma message("NV_ALIGN_TO() not available")
#    define NV_ALIGN_TO(N)
#  endif
#endif

#ifndef NV_USED
#  if defined(__GNUC__) || defined(__clang__)
#    define NV_USED __attribute__((__used__))
#  else
#    define NV_USED
#  endif
#endif

/* [fallthrough] is a C23 extension. I get it now. Shut up please. */
#if defined(__GNUC__) && __GNUC__ >= 7 || defined(__clang__) && __clang_major__ >= 12
#  define NV_FALLTHROUGH __attribute__((fallthrough))
#else
/**
 * @WARNING: This does not work. It expands to nothing
 * This is becuase the comment is interpreted out of the define
 */
#  define NV_FALLTHROUGH /* fallthrough */
#endif

#if defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_expect)
#  define NV_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#  define NV_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
/* Note that equals must be a long (or convertible to a long) */
#  define NV_EXPECT_EQUALS(expr, equals) (__builtin_expect(!!(expr), equals))
#else
#  define NV_LIKELY(expr) (expr)
#  define NV_UNLIKELY(expr) (expr)
#  define NV_EXPECT_EQUALS(expr, equals) (expr)
#endif

#define NV_CONCAT(x, y) x##y

#ifndef NV_STATIC_ASSERT
#  define NV_STATIC_ASSERT(expr, errmsg) static volatile char static_assert_failed__##errmsg[!(expr) ? -1 : 1]
#endif

// deadbeef is for losers
#define NOVA_CONT_CANARY 0xFEEF
#define NOVA_CONT_IS_VALID(cont) ((cont) && ((cont)->canary == NOVA_CONT_CANARY))

/**
 * Note that you may change these and everything should still work
 */
#ifndef real_t
#  define real_t double
#endif

#ifndef flt_t
#  define flt_t float
#endif

#define DEBUG 1
#define JUST_LOG_EVERYTHING_MAN 0

#if defined NV_TYPEOF
#  define NV_MAX(a, b) ((a) > (NV_TYPEOF(a))(b) ? (a) : (NV_TYPEOF(a))(b))
#  define NV_MIN(a, b) ((a) < (NV_TYPEOF(a))(b) ? (a) : (NV_TYPEOF(a))(b))
#else
#  define NV_MAX(a, b) ((a) > (b) ? (a) : (b))
#  define NV_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/*
 * GNUC and builtin have protection from accidentally passing in pointers instead of stack arrays
 */
#if defined(__GNUC__) && (__STDC_VERSION__ >= 201112L) && defined(NV_TYPEOF)
#  define nv_arrlen(arr) _Generic(&(arr), NV_TYPEOF(*(arr))(*): 0, default: (sizeof(arr) / sizeof((arr)[0])))
#elif defined(__has_builtin) && __has_builtin(__builtin_choose_expr) && __has_builtin(__builtin_types_compatible_p) && defined(NV_TYPEOF)
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

/**
 * https://stackoverflow.com/a/11172679
 * Stupid fix to , ##__VA_ARGS__ being a GNU extension
 * Only supports up to 10 arguments! However, increasing the limit is easy
 * Go to __GNUC_HELP_ME_PLEASE_SELECT_10TH and just add more variables and set the define to the last one
 */
#define NV_COMMA_ARGS_FIRST(...) __GNUC_HELP_ME_PLEASE_FIRST_HELPER(__VA_ARGS__, throwaway)

/*
 * if there's only one argument, expands to nothing.  if there is more
 * than one argument, expands to a comma followed by everything but
 * the first argument.  only supports up to 10 arguments but can be
 * trivially expanded.
 */
#define NV_COMMA_ARGS_REST(...) __GNUC_HELP_ME_PLEASE_REST_HELPER(__GNUC_HELP_ME_PLEASE_NUM(__VA_ARGS__), __VA_ARGS__)

#define __GNUC_HELP_ME_PLEASE_FIRST_HELPER(first, ...) first
#define __GNUC_HELP_ME_PLEASE_REST_HELPER(qty, ...) __GNUC_HELP_ME_PLEASE_REST_HELPER2(qty, __VA_ARGS__)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER2(qty, ...) __GNUC_HELP_ME_PLEASE_REST_HELPER_##qty(__VA_ARGS__)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER_ONE(first)
#define __GNUC_HELP_ME_PLEASE_REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define __GNUC_HELP_ME_PLEASE_NUM(...)                                                                                                                                        \
  __GNUC_HELP_ME_PLEASE_SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define __GNUC_HELP_ME_PLEASE_SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10

#ifndef NDEBUG
#  define nv_assert_else_return(expr, retval)                                                                                                                                 \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                       \
      {                                                                                                                                                                       \
        nv_log_error("Assertion failed -> %s\n", #expr);                                                                                                                      \
        return retval;                                                                                                                                                        \
      }                                                                                                                                                                       \
    } while (0);
#  define nv_assert_and_exec(expr, code)                                                                                                                                      \
    if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                         \
    {                                                                                                                                                                         \
      nv_log_error("Assertion failed -> %s\n", #expr);                                                                                                                        \
      code                                                                                                                                                                    \
    }
#  define nv_assert(expr)                                                                                                                                                     \
    do                                                                                                                                                                        \
    {                                                                                                                                                                         \
      if (NV_UNLIKELY(!((bool)(expr))))                                                                                                                                       \
      {                                                                                                                                                                       \
        nv_log_error("Assertion failed -> %s\n", #expr);                                                                                                                      \
      }                                                                                                                                                                       \
    } while (0);
#else
// These are typecasted to void because they give warnings because result (its
// like expr != NULL) is not used
#  define nv_assert_else_return(expr, retval) (void)(expr)
#  define nv_assert_and_exec(expr, code) (void)(expr)
#  define nv_assert(expr) (void)(expr)
#  pragma message("Assertions disabled")
#endif

#define _NV_LOG_EXPAND_PARAMETERS(preceder, is_error, ...)                                                                                                                    \
  _nv_core_log(__FILE__, __LINE__, __func__, preceder, is_error, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))

#define nv_log_error(...) _NV_LOG_EXPAND_PARAMETERS(" err: ", true, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#define nv_log_and_abort(...)                                                                                                                                                 \
  {                                                                                                                                                                           \
    _NV_LOG_EXPAND_PARAMETERS(" _fatal_: ", true, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__));                                                          \
    abort();                                                                                                                                                                  \
  }
#define nv_log_warning(...) _NV_LOG_EXPAND_PARAMETERS(" warning: ", false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#define nv_log_info(...) _NV_LOG_EXPAND_PARAMETERS(" info: ", false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#define nv_log_debug(...) _NV_LOG_EXPAND_PARAMETERS(" debug: ", false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#define nv_log_verbose(...) _NV_LOG_EXPAND_PARAMETERS(" verbose: ", false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))

#if JUST_LOG_EVERYTHING_MAN
#  define nv_log_too_much_info(...) _NV_LOG_EXPAND_PARAMETERS(" +info: ", false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))
#else
#  define nv_log_too_much_info(...) ((void)(__VA_ARGS__))
#endif

#define nv_log_custom(...) _NV_LOG_EXPAND_PARAMETERS(preceder, false, NV_COMMA_ARGS_FIRST(__VA_ARGS__) NV_COMMA_ARGS_REST(__VA_ARGS__))

#if defined(__has_attribute) && __has_attribute(format)
#  define NV_FORMAT_ATTR(...) __attribute__((format(__VA_ARGS__)))
#else
#  define NV_FORMAT_ATTR(...)
#endif

extern void _nv_core_log(const char* file, size_t line, const char* fn, const char* preceder, bool err, const char* fmt, ...) NV_FORMAT_ATTR(printf, 6, 7);
extern void nv_log_va(const char* file, size_t line, const char* fn, const char* preceder, bool err, const char* fmt, va_list args);

/* printf's the time to stdout. yep. [hour:minute:second]*/
extern void nv_print_time_as_string(FILE* stream);

static inline struct tm*
nv_get_time(void)
{
  time_t     now = 0;
  struct tm* tm  = NULL;

  now = time(NULL);
  if ((tm = localtime(&now)) == NULL)
  {
    nv_log_error("Error extracting time stuff\n");
    return NULL;
  }

  return tm;
}

#define NOVA_CALL_FILE_FN(fn)                                                                                                                                                 \
  if (NV_UNLIKELY((fn) != 0))                                                                                                                                                 \
  {                                                                                                                                                                           \
    nv_log_error("%s() => %i\n", #fn, errno);                                                                                                                                 \
  }

NV_STATIC_ASSERT(sizeof(float) == 4, sizeof_float_must_be_32_bits);
NV_STATIC_ASSERT(sizeof(double) == 8, sizeof_float_must_be_64_bits);

NOVA_HEADER_END

#endif
