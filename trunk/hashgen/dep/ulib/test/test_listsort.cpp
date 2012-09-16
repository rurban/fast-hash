#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <ulib/common.h>
#include <ulib/listsort.h>

struct list_node {
	struct list_head link;
	int data;
};

int comp_list_node(void *, const void *x, const void *y)
{
	return generic_compare((const list_node *)x, (const list_node *)y);
}

int main()
{
	LIST_HEAD(head);
	struct list_node *node;
	srand((int)time(NULL));

	// insert some nodes
	for (int i = 0; i < 100; i++) {
		node = new list_node;
		node->data = rand();
		list_add_tail(&node->link, &head);
	}

	list_sort(NULL, &head, comp_list_node);

	// verify and sorted list
	struct list_node *tmp;
	list_for_each_entry_safe(node, tmp, &head, link) {
		if (&tmp->link != &head)
			assert(comp_list_node(NULL, node, tmp) <= 0);
		delete node;
	}

	printf("passed\n");

	return 0;
}
