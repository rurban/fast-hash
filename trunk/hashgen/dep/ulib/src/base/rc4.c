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

#include "common.h"
#include "rc4.h"

void rc4_set_key(const unsigned char *buf, size_t len, rc4_key_t * key)
{
	unsigned char j = 0;
	unsigned char *state = key->state;
	int i;

	for (i = 0;  i < 256; ++i)
		state[i] = i;

	key->x = 0;
	key->y = 0;

	for (i = 0; i < 256; ++i) {
		j = j + state[i] + buf[i % len];
		swap(state[i], state[j]);
	}
}

void rc4_crypt(unsigned char *buf, size_t len, rc4_key_t * key)
{
	unsigned char x;
	unsigned char y;
	unsigned char *state = key->state;
	unsigned int  i;

	x = key->x;
	y = key->y;

	for (i = 0; i < len; i++) {
		y = y + state[++x];
		swap(state[x], state[y]);
		buf[i] ^= state[(state[x] + state[y]) & 0xff];
	}

	key->x = x;
	key->y = y;
}
