#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ulib/chainhash.h>

typedef struct {
	uint32_t key;
	uint32_t value;
} pair;

static uint32_t hash_from_key_fn(void *k)
{
	return ((pair *)k)->key;
}

static int  keys_equal_fn(void *a, void *b)
{
	return ((pair *)a)->key == ((pair *)b)->key;
}

int main()
{
	struct chainhash  *h;
	pair p1 = { 1, 2 };
	pair p2 = { 3, 5 };
	void *found;

	h = chainhash_create(0, hash_from_key_fn, keys_equal_fn);

	assert((found = chainhash_search(h, &p1)) == NULL);
	assert(chainhash_size(h) == 0);

	if (chainhash_insert(h, &p1, &p1))
	{
		printf("insertion failed\n");
		return -1;
	}

	assert((found = chainhash_search(h, &p1)) != NULL);
	assert(keys_equal_fn(&p1, found));
	assert((found = chainhash_search(h, &p2)) == NULL);
	assert(chainhash_size(h) == 1);

	if (chainhash_insert(h, &p2, &p2))
	{
		printf("insertion failed\n");
		return -1;
	}

	assert((found = chainhash_search(h, &p2)) != NULL);
	assert(keys_equal_fn(&p2, found));
	assert(chainhash_size(h) == 2);

	assert((found = chainhash_remove(h, &p2)) != NULL);
	assert(keys_equal_fn(&p2, found));
	assert(chainhash_size(h) == 1);

	chainhash_destroy(h, 0);

	printf("passed\n");

	return 0;
}
