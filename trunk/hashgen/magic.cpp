#include <stdint.h>
#include <stdio.h>
#include <ulib/bn.h>
#include <ulib/rdtsc.h>
#include <ulib/rand_tpl.h>

int main()
{
	uint64_t u, v, w, s;

	s = rdtsc();
	RAND_NR_INIT(u, v, w, s);

	do 
		s = (uint32_t)(RAND_NR_NEXT(u, v, w) | 3);  
	while (mpower(s, 1 << 29, 1ull << 32) == 1);

	printf("generator of 2^32: %08lx\n", s);

	return 0;
}
