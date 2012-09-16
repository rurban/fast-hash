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

/* Example of use:
 *
 *      struct chainhash  *h;
 *      struct some_key   *k;
 *      struct some_value *v;
 *
 *      static uint32_t		hash_from_key_fn( void *k );
 *      static int		keys_equal_fn ( void *key1, void *key2 );
 *
 *      h = chainhash_create(16, hash_from_key_fn, keys_equal_fn);
 *      k = (struct some_key *)     malloc(sizeof(struct some_key));
 *      v = (struct some_value *)   malloc(sizeof(struct some_value));
 *
 *      (initialise k and v to suitable values)
 * 
 *      if ( chainhash_insert(h,k,v) )
 *      {     exit(-1);              }
 *
 *      if (NULL == (found = chainhash_search(h,k) ))
 *      {    printf("not found!");                  }
 *
 *      if (NULL == (found = chainhash_remove(h,k) ))
 *      {    printf("Not found\n");                 }
 *
 */

/* Macros may be used to define type-safe(r) chainhash access functions, with
 * methods specialized to take known key and value types as parameters.
 * 
 * Example:
 *
 * Insert this at the start of your file:
 *
 * DEFINE_CHAINHASH_INSERT(insert_some, struct some_key, struct some_value);
 * DEFINE_CHAINHASH_SEARCH(search_some, struct some_key, struct some_value);
 * DEFINE_CHAINHASH_REMOVE(remove_some, struct some_key, struct some_value);
 *
 * This defines the functions 'insert_some', 'search_some' and 'remove_some'.
 * These operate just like chainhash_insert etc., with the same parameters,
 * but their function signatures have 'struct some_key *' rather than
 * 'void *', and hence can generate compile time errors if your program is
 * supplying incorrect data as a key (and similarly for value).
 *
 * Note that the hash and key equality functions passed to chainhash_create
 * still take 'void *' parameters instead of 'some key *'. This shouldn't be
 * a difficult issue as they're only defined and passed once, and the other
 * functions will ensure that only valid keys are supplied to them.
 *
 * The cost for this checking is increased code size and runtime overhead
 * - if performance is important, it may be worth switching back to the
 * unsafe methods once your program has been debugged with the safe methods.
 * This just requires switching to some simple alternative defines - eg:
 * #define insert_some chainhash_insert
 *
 */

#ifndef __ULIB_CHAINHASH_H
#define __ULIB_CHAINHASH_H

#include <stdint.h>

struct entry
{
	void *k, *v;
	uint32_t h;
	struct entry *next;
};

struct chainhash {
	uint32_t tablelength;
	struct entry **table;
	uint32_t entrycount;
	uint32_t loadlimit;
	uint32_t primeindex;
	uint32_t (*hashfn) (void *k);
	int (*eqfn) (void *k1, void *k2);
};

/* This struct is only concrete here to allow the inlining of two of the
 * accessor functions. */
struct chainhash_itr
{
	struct chainhash *h;
	struct entry *e;
	struct entry *parent;
	uint32_t index;
};

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * chainhash_hash - calculates hash value of key
	 * @h: chainhash
	 * @k: key
	 */
	static inline uint32_t chainhash_hash(struct chainhash * h, void *k)
	{
		/* Aim to protect against poor hash functions by adding logic here
		 * - logic taken from java 1.4 chainhash source */
		uint32_t i = h->hashfn(k);
		/*
		  i += ~(i << 9);
		  i ^=  ((i >> 14) | (i << 18));
		  i +=  (i << 4);
		  i ^=  ((i >> 10) | (i << 22));
		*/
		return i;
	}

	/* chainhash_create   
	 * @name                    chainhash_create
	 * @param   minsize         minimum initial size of chainhash
	 * @param   hashfn          function for hashing keys
	 * @param   key_eq_fn       function for determining key equality
	 * @return                  newly created chainhash or NULL on failure
	 */
	struct chainhash *
	chainhash_create(uint32_t minsize,
			 uint32_t (*hashfn) (void*),
			      int (*eqfn) (void*, void*));
	
	/* chainhash_insert
	 * @name        chainhash_insert
	 * @param   h   the chainhash to insert into
	 * @param   k   the key - chainhash does not claim ownership
	 * @param   v   the value - does not claim ownership
	 * @return      zero for successful insertion
	 *
	 * This function will cause the table to expand if the insertion would take
	 * the ratio of entries to table size over the maximum load factor.
	 *
	 * This function does not check for repeated insertions with a duplicate key.
	 * The value returned when using a duplicate key is undefined -- when
	 * the chainhash changes size, the order of retrieval of duplicate key
	 * entries is reversed.
	 * If in doubt, remove before insert.
	 */
	int chainhash_insert(struct chainhash *h, void *k, void *v);

#define DEFINE_CHAINHASH_INSERT(fnname, keytype, valuetype)		\
	int fnname (struct chainhash *h, keytype *k, valuetype *v)	\
	{								\
		return chainhash_insert(h,k,v);				\
	}

	/* chainhash_insert_only
	 * @name        chainhash_insert
	 * @param   h   the chainhash to insert into
	 * @param   k   the key - chainhash does not claim ownership
	 * @param   v   the value - does not claim ownership
	 * @return      zero for successful insertion
	 *
	 * This function will NOT cause the table to expand even if
	 * the insertion would take the ratio of entries to table size
	 * over the maximum load factor.
	 *
	 * This function does not check for repeated insertions with a duplicate key.
	 * The value returned when using a duplicate key is undefined -- when
	 * the chainhash changes size, the order of retrieval of duplicate key
	 * entries is reversed.
	 * If in doubt, remove before insert.
	 */
	int chainhash_insert_only(struct chainhash *h, void *k, void *v);

#define DEFINE_CHAINHASH_INSERT_ONLY(fnname, keytype, valuetype)	\
	int fnname (struct chainhash *h, keytype *k, valuetype *v)	\
	{								\
		return chainhash_insert_only(h,k,v);			\
	}

	/* chainhash_search
	 * @name        chainhash_search
	 * @param   h   the chainhash to search
	 * @param   k   the key to search for  - does not claim ownership
	 * @return      the value associated with the key, or NULL if none found
	 */
	void *chainhash_search(struct chainhash *h, void *k);

#define DEFINE_CHAINHASH_SEARCH(fnname, keytype, valuetype)	\
	valuetype * fnname (struct chainhash *h, keytype *k)	\
	{							\
		return (valuetype *) (chainhash_search(h,k));	\
	}

	/* chainhash_remove
	 * @name        chainhash_remove
	 * @param   h   the chainhash to remove the item from
	 * @param   k   the key to search for  - does not claim ownership
	 * @return      the value associated with the key, or NULL if none found
	 */
	void *chainhash_remove(struct chainhash *h, void *k);

#define DEFINE_CHAINHASH_REMOVE(fnname, keytype, valuetype)	\
	valuetype * fnname (struct chainhash *h, keytype *k)	\
	{							\
		return (valuetype *) (chainhash_remove(h,k));	\
	}
	
	/* chainhash_expand
	 * @name        chainhash_expand
	 * @param   h   the chainhash to remove the item from
	 * @return      zero for successful insertion
	 *
	 * Normally, this method should NOT be called unless the
	 * implementation requires additional features, such as
	 * concurrency control.
	 *
	 */
	int chainhash_expand(struct chainhash *h);

        /*
	 * chainhash_size
	 * @name        chainhash_size
	 * @param   h   the chainhash
	 * @return      the number of items stored in the chainhash
	 */
	uint32_t chainhash_size(struct chainhash *h);

        /*
	 * chainhash_destroy
	 * @name        chainhash_destroy
	 * @param   h   the chainhash
	 * @param       free_values     whether to call 'free' on the remaining values
	 */
	void chainhash_destroy(struct chainhash *h, int free_values);

	/**
	 * chainhash_iterator - allocates an iterator
	 * @h:   chainhash
	 * Note: the caller should free the iterator to prevent memory leaks
	 */
	struct chainhash_itr *chainhash_iterator(struct chainhash *h);

	/**
	 * chainhash_iterator_key - gets the key associated with the iterator
	 * @i: iterator
	 */
	void *chainhash_iterator_key(struct chainhash_itr *i);

	/**
	 * chainhash_iterator_value - gets the value associated with the iterator
	 * @i: iterator
	 */
	void *chainhash_iterator_value(struct chainhash_itr *i);

        /**
	 * chainhash_iterator_advance - advances the iterator to the next element
	 * @itr: iterator
	 * Note: returns -1 if advanced to end of table
	 */
	int chainhash_iterator_advance(struct chainhash_itr *itr);

        /**
	 * chainhash_iterator_remove - removes current element and advance the
	 * iterator to the next element
	 * Note: if you need the value to free it, read it before
	 *       removing. ie: beware memory leaks!
	 *       returns -1 if advanced to end of table
	 */
	 int chainhash_iterator_remove(struct chainhash_itr *itr);

        /**
	 * chainhash_iterator_search - overwrites the supplied iterator, to point to the entry
	 *          matching the supplied key.
	 * @itr:    iterator
	 * @h:      points to the chainhash to be searched.
	 * Note:    returns -1 if not found.
	 */
	int chainhash_iterator_search(struct chainhash_itr *itr,
				      struct chainhash *h, void *k);

#define DEFINE_CHAINHASH_ITERATOR_SEARCH(fnname, keytype)		\
	int fnname (struct chainhash_itr *i, struct chainhash *h, keytype *k) \
	{								\
		return (chainhash_iterator_search(i,h,k));		\
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
	int chainhash_change(struct chainhash *h, void *k, void *v, int free_old);

#ifdef __cplusplus
}
#endif

#endif /* __ULIB_CHAINHASH_H */
