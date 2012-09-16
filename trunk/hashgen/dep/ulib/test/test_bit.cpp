#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <ulib/bit.h>
#include <ulib/rand_tpl.h>

int main()
{
	uint64_t u, v, w;
	uint64_t seed = time(0);

	RAND_NR_INIT(u, v, w, seed);

	for (int i = 0; i < 100; i++)
		printf("rand number = %llx\n", (unsigned long long)RAND_NR_NEXT(u, v, w));

	uint64_t r = RAND_NR_NEXT(u, v, w);
	uint64_t s = BIN_TO_GRAYCODE(r);
	uint64_t t = BIN_TO_GRAYCODE(r + 1);
	assert(hweight64(t ^ s) == 1);
	GRAYCODE_TO_BIN64(s);

	if (s != r)
		fprintf(stderr, "expected %016llx, acutal %016llx\n", r, s);
	else
		printf("passed\n");

	return 0;
}
