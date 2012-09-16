#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ulib/tree.h>
#include <ulib/common.h>
#include <ulib/rand_tpl.h>

uint64_t u, v, w;
#define myrand()  RAND_NR_NEXT(u, v, w)

const char *usage = 
	"%s [ins] [get]\n";

volatile long counter = 0;

struct avl_node {
	struct avl_root link;
	uint64_t value;

	avl_node(uint64_t val = 0)
	: value(val) { }

	bool
	operator<(const avl_node &other) const
	{ return value < other.value; }

	bool
	operator>(const avl_node &other) const
	{ return value > other.value; }
};

int avl_node_cmp(const void *a, const void *b)
{
	return generic_compare(*(avl_node *)a, *(avl_node *)b);
}

static void sig_alarm_handler(int)
{
	printf("%ld per sec\n", counter);
	counter = 0;
	alarm(1);
}

void register_sig_handler()
{
	struct sigaction sigact;

	sigact.sa_handler = sig_alarm_handler;
	sigact.sa_flags = 0;
	if (sigaction(SIGALRM, &sigact, NULL)) {
		perror("sigaction");
		exit(-1);
	}
	alarm(1);
}

void constant_insert(long ins, long get)
{
	long t;
	avl_node *avl, *tmp;
	avl_root *root = NULL;

	for (t = 0; t < ins; t++) {
		avl = new avl_node(myrand());
		avl_add(&avl->link, avl_node_cmp, &root);
		counter++;
	}

	printf("insertion done\n");

	for (t = 0; t < get; t++) {
		avl_node n(myrand());
		tree_search((tree_root *)&n.link, avl_node_cmp, (tree_root *)root);
		counter++;
	}

	avl_for_each_entry_safe(avl, tmp, root, link) {
		avl_del(&avl->link, &root);
		delete avl;
	}
    
	printf("all done\n");
}

int main(int argc, char *argv[])
{
	long ins = 2000000;
	long get = 5000000;
	uint64_t seed = time(NULL);

	if (argc > 1)
		ins = atol(argv[1]);
	if (argc > 2)
		get = atol(argv[2]);

	RAND_NR_INIT(u, v, w, seed);

	register_sig_handler();

	constant_insert(ins, get);

	printf("passed\n");
	
	return 0;
}
