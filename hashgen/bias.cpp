/* Public Domain
   Written 2024 by Reini Urban
   Taken from Chris Wellons 2018 hash-prospector
*/

// for expf
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include "bias.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <ulib/bit.h>
#include <ulib/log.h>
#include <ulib/rand_tpl.h>
#include <unistd.h>

// exported symbols
float volatile g_bias_r = 0.1;

//#define POOL      40
#define THRESHOLD 2.0  // Use exact_bias when estimate is below this
#define DONTCARE  0.3  // Only accept bias below this threshold
#define QUALITY   18   // 2^N iterations of estimate samples


// fill full random numbers
void bias::_rand_fill(void *buf, size_t len) {
  uint64_t n;

  while (len >= 8) {
    n = RAND_NR_NEXT(_u, _v, _w);
    RAND_INT4_MIX64(n);
    *(uint64_t *)buf = n;
    buf = (void *)((char *)buf + 8);
    len -= 8;
  }
  if (len) {
    n = RAND_NR_NEXT(_u, _v, _w);
    RAND_INT4_MIX64(n);
    memcpy(buf, &n, len);
  }
}

static float
estimate_bias32(hash_func_t hash, int len, int times)
{
    long n = 1L << QUALITY;
    long bins[32][32] = {{0}};
    for (long i = 0; i < n; i++) {
        uint64_t x = RAND_NR_NEXT(_u, _v, _w);
        uint32_t h0 = hash(x);
        for (int j = 0; j < 32; j++) {
            uint32_t bit = UINT32_C(1) << j;
            uint32_t h1 = hash(x ^ bit);
            uint32_t set = h0 ^ h1;
            for (int k = 0; k < 32; k++)
                bins[j][k] += (set >> k) & 1;
        }
    }
    double mean = 0;
    for (int j = 0; j < 32; j++) {
        for (int k = 0; k < 32; k++) {
            double diff = (bins[j][k] - n / 2) / (n / 2.0);
            mean += (diff * diff) / (32 * 32);
        }
    }
    return sqrt(mean) * 1000.0;
}

#define EXACT_SPLIT 32  // must be power of two
static float
exact_bias32(hash_func_t hash, int len, int times)
{
  unsigned char buf[len];
  _rand_fill(buf, len);
  long long bins[32][32] = {{0}};
  static const uint64_t range = (UINT64_C(1) << 32) / EXACT_SPLIT;
  #pragma omp parallel for
  for (int i = 0; i < EXACT_SPLIT; i++) {
    long long b[32][32] = {{0}};
    for (uint64_t x = i * range; x < (i + 1) * range; x++) {
      uint32_t h0 = hash(x);
      for (int j = 0; j < 32; j++) {
        uint32_t bit = UINT32_C(1) << j;
        uint32_t h1 = hash(x ^ bit);
        uint32_t set = h0 ^ h1;
        for (int k = 0; k < 32; k++)
          b[j][k] += (set >> k) & 1;
      }
    }
    #pragma omp critical
    for (int j = 0; j < 32; j++)
      for (int k = 0; k < 32; k++)
        bins[j][k] += b[j][k];
  }
  double mean = 0.0;
  for (int j = 0; j < 32; j++) {
    for (int k = 0; k < 32; k++) {
      double diff = (bins[j][k] - 2147483648L) / 2147483648.0;
      mean += (diff * diff) / (32 * 32);
    }
  }
  return (float)(sqrt(mean) * 1000.0);
}

bias::bias() {
  uint64_t seed = (uint64_t)time(NULL);
  RAND_NR_INIT(_u, _v, _w, seed);
}

float bias::operator()(hash_func_t f, int len, int times) {
  return exact_bias32(f, len, times);
}
