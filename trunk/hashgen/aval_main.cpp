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

// avalanche driver

#include <ulib/hash.h>
#include "xxhash.h"
#include "avalanche.h"
#include "../fasthash.h"

static uint64_t fasthash64_noseed(const void *buf, size_t len)
{
	return fasthash64(buf, len, 0);
}

static uint64_t hash_jenkins_noseed(const void *buf, size_t len)
{
	uint64_t hash = 0x0000000100000001ULL;
	uint32_t *ph  = (uint32_t *)&hash;
	hash_jenkins2(buf, len, ph, ph + 1);
	return hash;
}

static uint64_t hash_xxhash_noseed(const void *buf, size_t len)
{
	uint64_t low, high;
	low  = XXH_fast32(buf, len, 0);
	high = XXH_fast32(buf, len, 1);
	return low | (high << 32);
}

int main()
{
	avalanche aval;

	printf("Overall quality of jenkinshash : %f\n", aval(hash_jenkins_noseed, 49, 5000));
	printf("Overall quality of fasthash    : %f\n", aval(fasthash64_noseed,   49, 5000));
	printf("Overall quality of xxhash      : %f\n", aval(hash_xxhash_noseed,  49, 5000));

	return 0;
}
