## \[TODO\]
*   Optimize every string function.
*   Check for unhandled edge cases in strconv's functions.
*   Add more hash functions and features to hash.h, it's pretty empty rn.

## \[VERSION 0.1.1\]
### Changes
*   Broke off type definitions of stdafx.h into types.h
*   Add nv_word_t, with the default type definition to u64, may be increased by the user (for example to __uint128_t). It is used for the optimized string functions.
*   Implemented probably every string function, left to optimize all of them.
*   Add NOVA_STRING_RETURN_WITH_BUILTIN_IF_AVAILABLE to easily check for builtins and return using them if available.
*   Added functions like strlcpy and other linux only functions with custom implementations, currently unoptimized.
*   Add max parameter to ato* functions for improved safety
*   New define for above change : NOVA_MAX_IGNORE to ignore the max limits
*   Add file and line to logs and errors to allow for quick jumping to files
*   Fixed using s in ftoa loop instead of c
*   Renamed or deleted some stdafx.h defines.
*   Replaced gettimeofday() with a more friendly and cross platform SDL timer alternative.
*   Added error checking to all string.h functions.
*   Removed 'Optimized' verisons of string.h functions. The compiler would be far better at optimization than us.
*   Added basic checking in nv_printf, and cleaned the functiosn quite a lot.
*   Added new rover.h for a rover style allocator.