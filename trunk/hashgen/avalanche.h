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

#ifndef _AVALANCHE_H
#define _AVALANCHE_H

#include <stdint.h>
#include <stdio.h>

// score ratios
extern float volatile g_aval_r;
extern float volatile g_indep_r;

#define DEF_IND 10.0

class avalanche {
public:
	typedef uint64_t (* hash_func_t)(const void *, size_t);
	
	avalanche();

	static void
	sample(uint64_t diff, float m[]);
	
	// evaluate the quality of test hash function
	static float
	evaluate(const float mat[][64], int nbit);

	void
	measure(float mat[][64], hash_func_t f, int len, int times);

	float
	operator()(hash_func_t f, int len, int times);

private:
	// fill buf with random numbers
	void _rand_fill(void *buf, size_t len);

	uint64_t _u, _v, _w;  // RNG context
};

#endif
