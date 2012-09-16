/* The MIT License

   Copyright (C) 2011 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

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

#ifndef __ULIB_HASH_H
#define __ULIB_HASH_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * hash_fast64 - 64-bit implementation of fasthash, 
 * @buf:  data buffer
 * @len:  data size
 * @seed: the seed
 */
	uint64_t hash_fast64(const void *buf, size_t len, uint64_t seed);

/**
 * hash_jenkins - implementation of Jenkins hash
 * @buf: data buffer
 * @len: data size
 */
	uint32_t hash_jenkins(const void *buf, size_t len, uint32_t seed);

/**
 * hash_jenkins2 - 2-hash version of Jenkins hash, outputs two hash
 * values to pc and pb. Both pc and pb must be non-null and should be
 * initialized with seeds.
 */
	void hash_jenkins2(const void *buf, size_t len, uint32_t * pc, uint32_t * pb);

#ifdef __cplusplus
}
#endif

#endif  /* __ULIB_HASH_H */
