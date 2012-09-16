#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <ulib/stirling.h>

#define FLOAT_EQUAL(x,y) (fabs((x) - (y)) < 0.01)

int main()
{
	assert(st_perm_ln(0) == 0);
	assert(FLOAT_EQUAL(1.7917595, st_perm_ln(3)));
	assert(FLOAT_EQUAL(8.5251614, st_perm_ln(7)));
	assert(FLOAT_EQUAL(42.335616, st_perm_ln(20)));
	assert(FLOAT_EQUAL(863.23199, st_perm_ln(200)));

	assert(FLOAT_EQUAL(1, st_perm(0)));
	assert(FLOAT_EQUAL(1, st_perm(1)));
	assert(FLOAT_EQUAL(24, st_perm(4)));
	assert(FLOAT_EQUAL(120, st_perm(5)));

	assert(FLOAT_EQUAL(0, st_comb_ln(0, 0)));
	assert(FLOAT_EQUAL(0, st_comb_ln(1, 1)));
	assert(FLOAT_EQUAL(3.555348, st_comb_ln(7, 3)));
	assert(FLOAT_EQUAL(1.609438, st_comb_ln(5, 4)));
	assert(FLOAT_EQUAL(14.48910, st_comb_ln(24, 10)));

	assert(FLOAT_EQUAL(1, st_comb(0, 0)));
	assert(FLOAT_EQUAL(1, st_comb(1, 1)));
	assert(FLOAT_EQUAL(1, st_comb(1, 0)));
	assert(FLOAT_EQUAL(0, st_comb(1, 2)));
	assert(FLOAT_EQUAL(35, st_comb(7, 3)));
	assert(FLOAT_EQUAL(6, st_comb(4, 2)));

	printf("passed\n");
	return 0;
}
