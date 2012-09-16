/* The MIT License

   Copyright (C) 2011 Zilong Tan (eric.zltan@gmail.com)
                 2002 Christopher Clark <Christopher.Clark@cl.cam.ac.uk>

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "chainhash.h"

/*
 * for consistency, the caller is responsible for freeing the keys
 */
#define freekey(X)

/*
 * prime table used in STL
 */
static const uint32_t primes[] =
{
	53ul,         97ul,         193ul,       389ul,       769ul,
	1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
	49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
	1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
	50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul, 
	1610612741ul, 3221225473ul, 4294967291ul
};

static const uint32_t prime_table_length =
	sizeof(primes) / sizeof(primes[0]);

static const float max_load_factor = 0.65;

static inline uint32_t indexFor(uint32_t tablelength, uint32_t hashvalue)
{
	return (hashvalue % tablelength);
};

/* Only works if tablelength == 2^N */
/*static inline uint32_t
  indexFor(uint32_t tablelength, uint32_t hashvalue)
  {
  return (hashvalue & (tablelength - 1u));
  }
*/

struct chainhash *
chainhash_create(uint32_t minsize,
		 uint32_t (*hashf) (void *),
		 int (*eqfn) (void *, void *))
{
	struct chainhash *h;
	uint32_t pindex, size = primes[0];
	/* Check requested chainhash isn't too large */
	if (minsize > (1u << 31))
		return NULL;
	/* Enforce size as prime */
	for (pindex = 0; pindex < prime_table_length; pindex++) {
		if (primes[pindex] > minsize) {
			size = primes[pindex];
			break;
		}
	}
	h = (struct chainhash *)malloc(sizeof(struct chainhash));
	if (NULL == h)
		return NULL;	/*oom */
	h->table = (struct entry **)malloc(sizeof(struct entry *) * size);
	if (NULL == h->table) {
		free(h);
		return NULL;
	}			/*oom */
	memset(h->table, 0, size * sizeof(struct entry *));
	h->tablelength = size;
	h->primeindex = pindex;
	h->entrycount = 0;
	h->hashfn = hashf;
	h->eqfn = eqfn;
	h->loadlimit = (uint32_t) ceil(size * max_load_factor);
	return h;
}

int chainhash_expand(struct chainhash *h)
{
	/* Double the size of the table to accomodate more entries */
	struct entry **newtable;
	struct entry *e;
	struct entry **pE;
	uint32_t newsize, i, index;
	/* Check we're not hitting max capacity */
	if (h->primeindex == (prime_table_length - 1))
		return -1;
	newsize = primes[++(h->primeindex)];

	newtable = (struct entry **)malloc(sizeof(struct entry *) * newsize);
	if (NULL != newtable) {
		memset(newtable, 0, newsize * sizeof(struct entry *));
		/* This algorithm is not 'stable'. ie. it reverses the list
		 * when it transfers entries between the tables */
		for (i = 0; i < h->tablelength; i++) {
			while (NULL != (e = h->table[i])) {
				h->table[i] = e->next;
				index = indexFor(newsize, e->h);
				e->next = newtable[index];
				newtable[index] = e;
			}
		}
		free(h->table);
		h->table = newtable;
	}
	/* Plan B: realloc instead */
	else {
		newtable = (struct entry **)
			realloc(h->table, newsize * sizeof(struct entry *));
		if (NULL == newtable) {
			(h->primeindex)--;
			return -1;
		}
		h->table = newtable;
		memset(newtable[h->tablelength], 0, newsize - h->tablelength);
		for (i = 0; i < h->tablelength; i++) {
			for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
				index = indexFor(newsize, e->h);
				if (index == i) {
					pE = &(e->next);
				} else {
					*pE = e->next;
					e->next = newtable[index];
					newtable[index] = e;
				}
			}
		}
	}
	h->tablelength = newsize;
	h->loadlimit = (uint32_t) ceil(newsize * max_load_factor);
	return 0;
}

uint32_t chainhash_size(struct chainhash * h)
{
	return h->entrycount;
}

int chainhash_insert_only(struct chainhash *h, void *k, void *v)
{
	/* This method allows duplicate keys - but they shouldn't be used */
	uint32_t index;
	struct entry *e;
	e = (struct entry *)malloc(sizeof(struct entry));
	if (NULL == e)
		return -1;
	++(h->entrycount);
	e->h = chainhash_hash(h, k);
	index = indexFor(h->tablelength, e->h);
	e->k = k;
	e->v = v;
	e->next = h->table[index];
	h->table[index] = e;
	return 0;
}

int chainhash_insert(struct chainhash *h, void *k, void *v)
{
	/* This method allows duplicate keys - but they shouldn't be used */
	uint32_t index;
	struct entry *e;
	if (++(h->entrycount) > h->loadlimit) {
		/* Ignore the return value. If expand fails, we should
		 * still try cramming just this value into the existing table
		 * -- we may not have memory for a larger table, but one more
		 * element may be ok. Next time we insert, we'll try expanding again.*/
		chainhash_expand(h);
	}
	e = (struct entry *)malloc(sizeof(struct entry));
	if (NULL == e) {
		--(h->entrycount);
		return -1;
	}			/*oom */
	e->h = chainhash_hash(h, k);
	index = indexFor(h->tablelength, e->h);
	e->k = k;
	e->v = v;
	e->next = h->table[index];
	h->table[index] = e;
	return 0;
}

void *chainhash_search(struct chainhash *h, void *k)
{
	struct entry *e;
	uint32_t hashvalue, index;
	hashvalue = chainhash_hash(h, k);
	index = indexFor(h->tablelength, hashvalue);
	e = h->table[index];
	while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
			return e->v;
		e = e->next;
	}
	return NULL;
}

void *chainhash_remove(struct chainhash *h, void *k)
{
	/* TODO: consider compacting the table when the load factor drops enough,
	 *       or provide a 'compact' method. */

	struct entry *e;
	struct entry **pE;
	void *v;
	uint32_t hashvalue, index;

	hashvalue = chainhash_hash(h, k);
	index = indexFor(h->tablelength, chainhash_hash(h, k));
	pE = &(h->table[index]);
	e = *pE;
	while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k))) {
			*pE = e->next;
			h->entrycount--;
			v = e->v;
			freekey(e->k);
			free(e);
			return v;
		}
		pE = &(e->next);
		e = e->next;
	}
	return NULL;
}

void chainhash_destroy(struct chainhash *h, int free_values)
{
	uint32_t i;
	struct entry *e, *f;
	struct entry **table = h->table;
	if (free_values) {
		for (i = 0; i < h->tablelength; i++) {
			e = table[i];
			while (NULL != e) {
				f = e;
				e = e->next;
				freekey(f->k);
				free(f->v);
				free(f);
			}
		}
	} else {
		for (i = 0; i < h->tablelength; i++) {
			e = table[i];
			while (NULL != e) {
				f = e;
				e = e->next;
				freekey(f->k);
				free(f);
			}
		}
	}
	free(h->table);
	free(h);
}

struct chainhash_itr *chainhash_iterator(struct chainhash *h)
{
	uint32_t i, tablelength;
	struct chainhash_itr *itr = (struct chainhash_itr *)
		malloc(sizeof(struct chainhash_itr));
	if (NULL == itr)
		return NULL;
	itr->h = h;
	itr->e = NULL;
	itr->parent = NULL;
	tablelength = h->tablelength;
	itr->index = tablelength;
	if (0 == h->entrycount)
		return itr;

	for (i = 0; i < tablelength; i++) {
		if (NULL != h->table[i]) {
			itr->e = h->table[i];
			itr->index = i;
			break;
		}
	}
	return itr;
}

void *chainhash_iterator_key(struct chainhash_itr *i)
{
	if (NULL == i->e) {
		return NULL;
	}

	return i->e->k;
}

void *chainhash_iterator_value(struct chainhash_itr *i)
{
	if (NULL == i->e) {
		return NULL;
	}

	return i->e->v;
}

int chainhash_iterator_advance(struct chainhash_itr *itr)
{
	uint32_t j, tablelength;
	struct entry **table;
	struct entry *next;
	if (NULL == itr->e)
		return -1;	/* stupidity check */

	next = itr->e->next;
	if (NULL != next) {
		itr->parent = itr->e;
		itr->e = next;
		return 0;
	}
	tablelength = itr->h->tablelength;
	itr->parent = NULL;
	if (tablelength <= (j = ++(itr->index))) {
		itr->e = NULL;
		return -1;
	}
	table = itr->h->table;
	while (NULL == (next = table[j])) {
		if (++j >= tablelength) {
			itr->index = tablelength;
			itr->e = NULL;
			return -1;
		}
	}
	itr->index = j;
	itr->e = next;
	return 0;
}

/*****************************************************************************/
/* remove - remove the entry at the current iterator position
 *          and advance the iterator, if there is a successive
 *          element.
 *          If you want the value, read it before you remove:
 *          beware memory leaks if you don't.
 *          Returns -1 if end of iteration. */

int chainhash_iterator_remove(struct chainhash_itr *itr)
{
	struct entry *remember_e, *remember_parent;
	int ret;

	/* Do the removal */
	if (NULL == (itr->parent)) {
		/* element is head of a chain */
		itr->h->table[itr->index] = itr->e->next;
	} else {
		/* element is mid-chain */
		itr->parent->next = itr->e->next;
	}
	/* itr->e is now outside the chainhash */
	remember_e = itr->e;
	itr->h->entrycount--;
	freekey(remember_e->k);

	/* Advance the iterator, correcting the parent */
	remember_parent = itr->parent;
	ret = chainhash_iterator_advance(itr);
	if (itr->parent == remember_e) {
		itr->parent = remember_parent;
	}
	free(remember_e);
	return ret;
}

int chainhash_iterator_search(struct chainhash_itr *itr,
			      struct chainhash *h, void *k)
{
	struct entry *e, *parent;
	uint32_t hashvalue, index;

	hashvalue = chainhash_hash(h, k);
	index = indexFor(h->tablelength, hashvalue);

	e = h->table[index];
	parent = NULL;
	while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k))) {
			itr->index = index;
			itr->e = e;
			itr->parent = parent;
			itr->h = h;
			return 0;
		}
		parent = e;
		e = e->next;
	}
	return -1;
}

/**
 * chainhash_change
 *
 * function to change the value associated with a key, where there already
 * exists a value bound to the key in the chainhash.
 * Source due to Holger Schemel.
 * @h:        chainhash
 * @k:        key for the value
 * @v:        new value to use
 * @free_old: free old value if exists
 */
int chainhash_change(struct chainhash * h, void *k, void *v, int free_old)
{
	struct entry *e;
	uint32_t hashvalue, index;
	hashvalue = chainhash_hash(h, k);
	index = indexFor(h->tablelength, hashvalue);
	e = h->table[index];
	while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k))) {
			if (free_old) {
				free(e->v);
			}
			e->v = v;
			return 0;
		}
		e = e->next;
	}
	return -1;
}
