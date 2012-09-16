/* The MIT License

   Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

// for expf
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ulib/log.h>
#include <ulib/bit.h>
#include <ulib/rand_tpl.h>
#include "avalanche.h"

// exported symbols
float volatile g_aval_r  = 0.1;
float volatile g_indep_r = 2.0;

// independence test
static inline float
indep_score(unsigned char *s, int num)
{
	double n = 0;
	double m = 0;
	double r = 0;

	if (num < 2)
		return DEF_IND;

	for (int i = 0; i < num; ++i) {
		r += s[i] & (1 - s[i + 1]);
		n += s[i];
		m += 1 - s[i];
	}
	r += r - 1 + !s[0] + !s[num - 1];
	double mean = 2 * n * m / (n + m) + 1;
	double var = 2 * n * m / (n + m) * (2 * n * m - n - m) / (n + m) / (n + m - 1);
	if (var == 0)
		return DEF_IND;
	if (r > mean)
		return fabs(r - mean - 0.5) / sqrt(var);
	return fabs(r - mean + 0.5) / sqrt(var);
}

static void
binary_classify(const float mat[][64], int max, unsigned char *sample)
{
	int i;

	for (i = 0; i < max; ++i)
		sample[i] = mat[i >> 6][i & 0x3f] > 0.5;
}

avalanche::avalanche()
{
	uint64_t seed = (uint64_t) time(NULL);
	RAND_NR_INIT(_u, _v, _w, seed);
}

void avalanche::sample(uint64_t diff, float m[])
{
	while (diff) {
		++m[ffs64(diff) - 1];
		diff &= diff - 1;
	}
}

float avalanche::evaluate(const float mat[][64], int nbit)
{
	float r = 0;
	float m, s;
	int i, j;
	int bin_max = 64 * nbit;

	unsigned char bin[bin_max + 1];
	bin[bin_max] = 0;  // reserved for special use
	binary_classify(mat, bin_max, bin);
	s = indep_score(bin, bin_max);
	ULIB_DEBUG("independence score = %f", s * g_indep_r);

	for (i = 0; i < nbit; ++i) {
		for (j = 0; j < 64; ++j) {
			m  = mat[i][j] - 0.5;
			r += expf((m < 0? -m: m) + 8.0) - 2980.95798704172827474359;
		}
	}
	
	ULIB_DEBUG("avalanche score    = %f", r / (nbit * 64) * g_aval_r);

	return (r / (nbit * 64)) * g_aval_r + s * g_indep_r;
}

// fill full random numbers
void avalanche::_rand_fill(void *buf, size_t len)
{
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

/*
// fill the first half with random numbers and the other half with
// zeros this may shed light upon certain weeknesses in hash function
// when buf is sparse(low hamming weight)
void avalanche::_rand_fill(void *buf, size_t len)
{
	uint64_t n;
	size_t k = len;

	len /= 2;
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
	memset((char *)buf + k/2, 0, k - k/2);
}

void avalanche::_rand_fill(void *buf, size_t len)
{
	char *data = (char *)buf;

	while (len-- > 0) {
		uint64_t r = RAND_NR_NEXT(_u, _v, _w);
		r %= 52;
		if (r >= 26)
			*data++ = 0x61 + r - 26;
		else
			*data++ = 0x41 + r;
	}
}
*/

void avalanche::measure(float mat[][64], hash_func_t f, int len, int times)
{
	int i, n;
	int nbit = len << 3;

	for (i = 0; i < nbit; ++i) {
		for (n = 0; n < times; ++n) {
			unsigned char buf[len];
			uint64_t hash, newhash;
			_rand_fill(buf, len);
			hash = f(buf, len);
			buf[i >> 3] ^= 1 << (i & 7);
			newhash = f(buf, len);
			newhash ^= hash;
			sample(newhash, mat[i]);
		}
		for (n = 0; n < 64; ++n)
			mat[i][n] /= times;
	}
}

float avalanche::operator()(hash_func_t f, int len, int times)
{
	int i;
	int nbit = len << 3;
	float mat[nbit][64];	

	for (i = 0; i < nbit; ++i)
		memset(mat[i], 0, sizeof(float) * 64);
	measure(mat, f, len, times);
	return evaluate(mat, nbit);
}
