#include <stdio.h>
#include <assert.h>
#include <ulib/rdtsc.h>

int main()
{
	uint64_t s = rdtsc();
	for (int i = 0; i < 1000; ++i)
		;
	uint64_t t = rdtsc();
	assert(s != t);

	printf("1000 cycle costs %lu\n", t - s);

	printf("passed\n");

	return 0;
}
