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

#ifdef __cplusplus
}
#endif

#endif // NV_ASSERT_H
