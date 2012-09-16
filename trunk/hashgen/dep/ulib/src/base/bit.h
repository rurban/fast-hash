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

#ifndef __ULIB_BIT_H
#define __ULIB_BIT_H

#include <stdint.h>

#define BITS_PER_BYTE       8
#define BITS_PER_LONG       ((int)((sizeof(long) * BITS_PER_BYTE)))

#define DIV_ROUND_UP(n,d)   (((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)         ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ROR64(x, r)         ((x) >> (r) | (x) << (64 - (r)))

/* this isn't portable
 * #define SIGN(x)          ((v) >> (sizeof(v) * BITS_PER_BYTE - 1))
 */
#define SIGN(x)             (((v) > 0) - ((v) < 0))
#define OPPOSITE_SIGN(x,y)  ((x) ^ (y) < 0)
#define ABS(x) ({							\
			typeof(x) _t = (x) >> sizeof(x) * BITS_PER_BYTE - 1; \
			((x) ^ _t) - _t; })
#define XOR_MIN(x,y)        ((y) ^ (((x) ^ (y)) & -((x) < (y))))
#define XOR_MAX(x,y)        ((x) ^ (((x) ^ (y)) & -((x) < (y))))

#define BIN_TO_GRAYCODE(b)  ((b) ^ ((b) >> 1))
#define GRAYCODE_TO_BIN32(g)  ({		\
			(g) ^= (g) >> 1;	\
			(g) ^= (g) >> 2;	\
			(g) ^= (g) >> 4;	\
			(g) ^= (g) >> 8;	\
			(g) ^= (g) >> 16;	\
		})
#define GRAYCODE_TO_BIN64(g)  ({		\
			GRAYCODE_TO_BIN32(g);	\
			(g) ^= (g) >> 32;	\
		})

/* conditionally set or clear bits, where w is the word to modify, 
 * m is the bit mask and f is the condition flag */
#define BIT_ALTER(w,m,f)    ((w) ^ ((-(f) ^ (w)) & (m)))

#define HAS_ZERO32(x)       HAS_LESS32(x,1)
#define HAS_ZERO64(x)       HAS_LESS64(x,1)

#define HAS_VALUE32(x,v)    HAS_ZERO32((x) ^ (~(uint32_t)0/255 * (v)))
#define HAS_VALUE64(x,v)    HAS_ZERO64((x) ^ (~(uint64_t)0/255 * (v)))

/* Requirements: x>=0; 0<=v<=128 */
#define HAS_LESS32(x,v)     (((x) - ~(uint32_t)0/255 * (v)) & ~(x) & ~(uint32_t)0/255 * 128)
#define HAS_LESS64(x,v)     (((x) - ~(uint64_t)0/255 * (v)) & ~(x) & ~(uint64_t)0/255 * 128)
#define COUNT_LESS32(x,v)						\
	(((~(uint32_t)0/255 * (127 + (v)) - ((x) & ~(uint32_t)0/255 * 127)) & \
	  ~(x) & ~(uint32_t)0/255 * 128)/128 % 255)
#define COUNT_LESS64(x,v)						\
	(((~(uint64_t)0/255 * (127 + (v)) - ((x) & ~(uint64_t)0/255 * 127)) & \
	  ~(x) & ~(uint64_t)0/255 * 128)/128 % 255)

/* Requirements: x>=0; 0<=v<=127 */
#define HAS_MORE32(x,v)     (((x) + ~(uint32_t)0/255 * (127 - (v)) | (x)) &~(uint32_t)0/255 * 128)
#define HAS_MORE64(x,v)     (((x) + ~(uint64_t)0/255 * (127 - (v)) | (x)) &~(uint64_t)0/255 * 128)
#define COUNT_MORE32(x,v)						\
	(((((x) & ~(uint32_t)0/255 * 127) + ~(uint32_t)0/255 * (127 - (v)) | \
	   (x)) & ~(uint32_t)0/255 * 128)/128 % 255)
#define COUNT_MORE64(x,v)						\
	(((((x) & ~(uint64_t)0/255 * 127) + ~(uint64_t)0/255 * (127 - (v)) | \
	   (x)) & ~(uint64_t)0/255 * 128)/128 % 255)

#define ROUND_UP32(x) ({			\
			(x)--;			\
			(x) |= (x) >> 1;	\
			(x) |= (x) >> 2;	\
			(x) |= (x) >> 4;	\
			(x) |= (x) >> 8;	\
			(x) |= (x) >> 16;	\
			(x)++;			\
		})

#define ROUND_UP64(x) ({			\
			(x)--;			\
			(x) |= (x) >> 1;	\
			(x) |= (x) >> 2;	\
			(x) |= (x) >> 4;	\
			(x) |= (x) >> 8;	\
			(x) |= (x) >> 16;	\
			(x) |= (x) >> 32;	\
			(x)++;			\
		})

static inline void set_bit(int nr, volatile unsigned long *addr)
{
	*(addr + BIT_WORD(nr)) |= BIT_MASK(nr);
}

static inline void clear_bit(int nr, volatile unsigned long *addr)
{
	*(addr + BIT_WORD(nr)) &= ~BIT_MASK(nr);
}

static inline void change_bit(int nr, volatile unsigned long *addr)
{
	*(addr + BIT_WORD(nr)) ^= BIT_MASK(nr);
}

static inline int test_bit(int nr, const volatile unsigned long *addr)
{
        return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

/**
 * hweight15 - calculates the hamming weight of a natural number less than 2^15 - 1
 * @a: the natural number
 */
static inline int hweight15(uint16_t a)
{
	return ((a * 35185445863425ULL) & 76861433640456465ULL) % 15;
}

/**
 * hweight32_fast - calculates hamming weight of a 32-bit integer
 * @a: the integer
 */
static inline int hweight32(uint32_t a)
{
	register uint32_t t;
	
	t = a - ((a >> 1) & 033333333333)
		- ((a >> 2) & 011111111111);
	return ((t + (t >> 3)) & 030707070707) % 63;
}

/**
 * hweight64 - calculates hamming weight of a 64-bit integer
 * @a: the integer
 */
static inline int hweight64(uint64_t a)
{
	a = (a & 0x5555555555555555ULL) + ((a >> 1) & 0x5555555555555555ULL);
	a = (a & 0x3333333333333333ULL) + ((a >> 2) & 0x3333333333333333ULL);
	a = (a + (a >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	a = (a + (a >> 8)); 
	a = (a + (a >> 16));
	a = (a + (a >> 32));
	return a & 0xFF;
}

/**
 * hweight64 - calculates hamming weight of a word
 * @a: the integer
 */
static inline int hweight_long(unsigned long a)
{
	return sizeof(a) == 4? hweight32(a): hweight64(a);
}

/**
 * rev8 - reverses the order of bits in a byte
 * @n: the byte
 * returns the revd byte
 */
static inline unsigned char rev8(unsigned char n)
{
	return ((n * 0x000202020202ULL) & 0x010884422010ULL) % 1023;
}

/**
 * rev32 - reverses the order of bits in a 32-bit integer
 * @n: the integer
 * returns the revd integer
 */
static inline uint32_t rev32(uint32_t n)
{
	n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
	n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
	return (n >> 16) | (n << 16);
}

/**
 * rev64 - reverses the order of bits in a 64-bit integer
 * @n: the integer
 * returns the revd integer
 */
static inline uint64_t rev64(uint64_t n)
{
	n = ((n & 0xAAAAAAAAAAAAAAAAULL) >> 1) | ((n & 0x5555555555555555ULL) << 1);
	n = ((n & 0xCCCCCCCCCCCCCCCCULL) >> 2) | ((n & 0x3333333333333333ULL) << 2);
	n = ((n & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((n & 0x0F0F0F0F0F0F0F0FULL) << 4);
	n = ((n & 0xFF00FF00FF00FF00ULL) >> 8) | ((n & 0x00FF00FF00FF00FFULL) << 8);
	n = ((n & 0xFFFF0000FFFF0000ULL) >> 16)| ((n & 0x0000FFFF0000FFFFULL) << 16);
	return (n >> 32) | (n << 32);
}

/**
 * ispow2_32 - tests if the 32-bit integer is a power of 2
 * @n: the integer
 */
static inline int ispow2_32(uint32_t n)
{
	return (n & (n - 1)) == 0;
}

/**
 * ispow2_64 - tests if the 64-bit integer is a power of 2
 * @n: the integer
 */
static inline int ispow2_64(uint64_t n)
{
	return (n & (n - 1)) == 0;
}

/**
 * fls32 - finds last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls32(uint32_t x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/**
 * fls64 - finds last set bit in a 64-bit word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffsll, but returns the position of the most significant set bit.
 *
 * fls64(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 64.
 */
static inline int fls64(uint64_t x)
{
	uint32_t h = x >> 32;
	if (h)
		return fls32(h) + 32;
	return fls32(x);
}

/**
 * ffs32 - finds first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int ffs32(uint32_t x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/**
 * ffs64 - finds first bit in word.
 * @word: The word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int ffs64(uint64_t word)
{
	uint32_t h = word & 0xffffffff;
	if (!h)
		return ffs32(word >> 32) + 32;
	return ffs32(h);
}

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * This is the intuitive version of ffs, which is different from ffs64.
 * Undefined if no bit exists, so code should check against 0 first.
 */
/* use gcc builtin instead if possible */
#define __ffs(w) (__builtin_ffsl(w) - 1)

/*
static inline unsigned long __ffs(unsigned long word)
{
	int num = 0;

#if __WORDSIZE == 64
	if ((word & 0xffffffff) == 0) {
		num += 32;
		word >>= 32;
	}
#endif
	if ((word & 0xffff) == 0) {
		num += 16;
		word >>= 16;
	}
	if ((word & 0xff) == 0) {
		num += 8;
		word >>= 8;
	}
	if ((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if ((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if ((word & 0x1) == 0)
		num += 1;
	return num;
}
*/

/*
 * ffz - find first zero in word.
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
#define ffz(x)  __ffs(~(x))

/**
 * hweight_next32 - calculates the next higher integer
 * with the same hamming weight
 * @a: the integer
 */
static inline uint32_t hweight_next32(uint32_t a)
{
	uint32_t c = a & -a;
	uint32_t r = a + c;
	return (((r ^ a) >> 2) / c) | r;
}

/**
 * hweight_next64 - calculates the next higher integer
 * with the same hamming weight
 * @a: the integer
 */
static inline uint64_t hweight_next64(uint64_t a)
{
	uint64_t c = a & -a;
	uint64_t r = a + c;
	return (((r ^ a) >> 2) / c) | r;
}

/*
 * Find the next set bit in a memory region.
 */
static inline unsigned long
find_next_bit(const unsigned long *addr, unsigned long size, unsigned long offset)
{
	const unsigned long *p = addr + BIT_WORD(offset);
	unsigned long result = offset & ~(BITS_PER_LONG-1);
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset %= BITS_PER_LONG;
	if (offset) {
		tmp = *(p++);
		tmp &= (~0UL << offset);
		if (size < BITS_PER_LONG)
			goto found_first;
		if (tmp)
			goto found_middle;
		size -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}
	while (size & ~(BITS_PER_LONG-1)) {
		if ((tmp = *(p++)))
			goto found_middle;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp &= (~0UL >> (BITS_PER_LONG - size));
	if (tmp == 0UL)		/* Are any bits set? */
		return result + size;	/* Nope. */
found_middle:
	return result + __ffs(tmp);
}

/*
 * This implementation of find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h.
 */
static inline unsigned long
find_next_zero_bit(const unsigned long *addr, unsigned long size, unsigned long offset)
{
	const unsigned long *p = addr + BIT_WORD(offset);
	unsigned long result = offset & ~(BITS_PER_LONG-1);
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset %= BITS_PER_LONG;
	if (offset) {
		tmp = *(p++);
		tmp |= ~0UL >> (BITS_PER_LONG - offset);
		if (size < BITS_PER_LONG)
			goto found_first;
		if (~tmp)
			goto found_middle;
		size -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}
	while (size & ~(BITS_PER_LONG-1)) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp |= ~0UL << size;
	if (tmp == ~0UL)	/* Are any bits zero? */
		return result + size;	/* Nope. */
found_middle:
	return result + ffz(tmp);
}

/*
 * Find the first set bit in a memory region.
 */
static inline unsigned long
find_first_bit(const unsigned long *addr, unsigned long size)
{
	const unsigned long *p = addr;
	unsigned long result = 0;
	unsigned long tmp;

	while (size & ~(BITS_PER_LONG-1)) {
		if ((tmp = *(p++)))
			goto found;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;

	tmp = (*p) & (~0UL >> (BITS_PER_LONG - size));
	if (tmp == 0UL)		/* Are any bits set? */
		return result + size;	/* Nope. */
found:
	return result + __ffs(tmp);
}

/*
 * Find the first cleared bit in a memory region.
 */
static inline unsigned long
find_first_zero_bit(const unsigned long *addr, unsigned long size)
{
	const unsigned long *p = addr;
	unsigned long result = 0;
	unsigned long tmp;

	while (size & ~(BITS_PER_LONG-1)) {
		if (~(tmp = *(p++)))
			goto found;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;

	tmp = (*p) | (~0UL << size);
	if (tmp == ~0UL)	/* Are any bits zero? */
		return result + size;	/* Nope. */
found:
	return result + ffz(tmp);
}

#define for_each_set_bit(bit, addr, size)			\
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

#endif  /* __ULIB_BIT_H */
