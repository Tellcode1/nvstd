#include "../include/rand.h"

/**
 * Must be larger than nv_rand_t
 */
#ifndef NOVA_RAND_TMP_CONVERT_TYPE
#  define NOVA_RAND_TMP_CONVERT_TYPE __uint128_t
#endif

static inline nv_rand_t
rotl(nv_rand_t x, int k)
{
#if defined(__GNUC__) && defined(__has_builtin) && __has_builtin(__builtin_rotateleft64) && (__SIZEOF_LONG_LONG__ == 8)
  return __builtin_rotateleft64(x, k);
#else
  return (x << k) | (x >> ((sizeof(nv_rand_t) * 8 - k) & ((sizeof(nv_rand_t) * 8) - 1)));
#endif
}

nv_error
nv_random_bulk_range(nv_rand_info_t* info, nv_rand_t* outbuf, size_t outbuf_size, size_t min, size_t max)
{
  nv_assert_else_return(outbuf_size != 0, NV_ERROR_INVALID_ARG);

  if (min >= max) { return min; }

  for (size_t i = 0; i < outbuf_size; i++)
  {
    /**
     * xoshiro 256 random number generator
     * https://prng.di.unimi.it/xoshiro256plusplus.c
     */
    const NOVA_RAND_TMP_CONVERT_TYPE bound = max - min + 1;

    const nv_rand_t result = rotl(info->state[0] + info->state[3], 23) + info->state[0];

    NOVA_RAND_TMP_CONVERT_TYPE tmp = ((NOVA_RAND_TMP_CONVERT_TYPE)result * bound);
    outbuf[i]                      = min + (tmp >> (sizeof(nv_rand_t) * 8));

    const nv_rand_t t = info->state[1] << 17;

    info->state[2] ^= info->state[0];
    info->state[3] ^= info->state[1];
    info->state[1] ^= info->state[2];
    info->state[0] ^= info->state[3];

    info->state[2] ^= t;
    info->state[3] = rotl(info->state[3], 45);
  }

  return NV_SUCCESS;
}

static inline nv_rand_t
splitmix(nv_rand_t* state)
{
  nv_rand_t tmp = (*state += 0x9E3779B97f4A7C15);
  tmp           = (tmp ^ (tmp >> 30)) * 0xBF58476D1CE4E5B9;
  tmp           = (tmp ^ (tmp >> 27)) * 0x94D049BB133111EB;
  return tmp ^ (tmp >> 31);
}

void
nv_random_seed(nv_rand_info_t* info, nv_rand_t seed)
{
  nv_rand_t splitmixstate = seed;
  for (size_t i = 0; i < 4; i++) { info->state[i] = splitmix(&splitmixstate); }
}
