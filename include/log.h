#ifndef NV_LOG_H
#define NV_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

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

#ifdef __cplusplus
}
#endif

#endif // NV_LOG_H