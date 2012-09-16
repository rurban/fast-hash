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

#include "tree.h"

struct tree_root *
tree_search(struct tree_root *entry,
	    int (*compare) (const void *, const void *),
	    struct tree_root *root)
{
	int retval;

	while (root != NIL) {
		retval = compare(entry, root);
		if (retval == 0)
			return root;
		if (retval < 0)
			root = root->left;
		else
			root = root->right;
	}
	return root;
}

struct tree_root *
tree_min(struct tree_root *root)
{
	if (root != NIL) {
		while (root->left != NIL)
			root = root->left;
	}
	return root;
}

struct tree_root *
tree_max(struct tree_root *root)
{
	if (root != NIL) {
		while (root->right != NIL)
			root = root->right;
	}
	return root;
}

struct tree_root *
tree_successor(struct tree_root *root)
{
	struct tree_root *p = NIL;

	if (root != NIL) {
		if (root->right != NIL)
			return tree_min(root->right);

		p = root->parent;
		while (p != NIL && root == p->right) {
			root = p;
			p = p->parent;
		}
	}
	return p;
}

struct tree_root *
tree_predecessor(struct tree_root *root)
{
	struct tree_root *p = NIL;

	if (root != NIL) {
		if (root->left != NIL)
			return tree_max(root->left);

		p = root->parent;
		while (p != NIL && root == p->left) {
			root = p;
			p = p->parent;
		}
	}
	return p;
}

/*
 * Left rotate entry
 *
 * This is only for internal tree manipulation.
 */
static inline void
__rotate_left(struct tree_root *entry,
	      struct tree_root **root)
{
	struct tree_root *n;

	n = entry->right;

	entry->right = n->left;
	if (n->left != NIL)
		n->left->parent = entry;

	n->parent = entry->parent;

	if (entry->parent == NIL)
		*root = n;
	else if (entry == entry->parent->left)
		entry->parent->left = n;
	else
		entry->parent->right = n;

	n->left = entry;
	entry->parent = n;
}

/*
 * Right rotate entry
 *
 * This is only for internal tree manipulation.
 */
static inline void
__rotate_right(struct tree_root *entry,
	       struct tree_root **root)
{
	struct tree_root *n;

	n = entry->left;

	entry->left = n->right;
	if (n->right != NIL)
		n->right->parent = entry;

	n->parent = entry->parent;

	if (entry->parent == NIL)
		*root = n;
	else if (entry == entry->parent->left)
		entry->parent->left = n;
	else
		entry->parent->right = n;

	n->right = entry;
	entry->parent = n;
}

void
tree_add(struct tree_root *new,
	 int (*compare) (const void *, const void *),
	 struct tree_root **root)
{
	int retval = 0;
	struct tree_root *r = *root;
	struct tree_root *n = NIL;

	INIT_TREE_ROOT(new);

	/*
	 * searches the new entry
	 */
	while (r != NIL) {
		n = r;
		retval = compare(new, r);
		if (retval < 0)
			r = r->left;
		else
			r = r->right;
	}

	new->parent = n;

	if (n == NIL)
		*root = new;
	else if (retval < 0)
		n->left = new;
	else
		n->right = new;
}

struct tree_root *
tree_map(struct tree_root *new,
	 int (*compare) (const void *, const void *),
	 struct tree_root **root)
{
	int retval = 0;
	struct tree_root *r = *root;
	struct tree_root *n = NIL;

	INIT_TREE_ROOT(new);

	/*
	 * searches the new entry
	 */
	while (r != NIL) {
		n = r;
		retval = compare(new, r);
		if (retval == 0)
			return r;
		else if (retval < 0)
			r = r->left;
		else
			r = r->right;
	}

	new->parent = n;

	if (n == NIL)
		*root = new;
	else if (retval < 0)
		n->left = new;
	else
		n->right = new;
	return new;
}

void
tree_del(struct tree_root *entry, struct tree_root **root)
{
	struct tree_root *r;
	struct tree_root *n;

	if (entry->left == NIL || entry->right == NIL)
		n = entry;
	else
		n = tree_successor(entry);

	if (n->left != NIL)
		r = n->left;
	else
		r = n->right;

	if (r != NIL)
		r->parent = n->parent;

	if (n->parent == NIL)
		*root = r;
	else if (n == n->parent->left)
		n->parent->left = r;
	else
		n->parent->right = r;

	if (n != entry) {
		/*
		 * takes entry's position with n
		 */
		n->left = entry->left;
		if (entry->left != NIL)
			entry->left->parent = n;
		n->right = entry->right;
		if (entry->right != NIL)
			entry->right->parent = n;
		n->parent = entry->parent;
		if (entry->parent == NIL)
			*root = n;
		else if (entry == entry->parent->left)
			entry->parent->left = n;
		else
			entry->parent->right = n;
	}
}

/*
 * Right rotate entry.
 *
 * @entry: node on which to perform rotation
 * @tmp:   left child of entry
 * Note:   Entry and tmp shall not be NIL.
 */
#define SPLAY_ROTATE_RIGHT(entry, tmp) do {				\
		(entry)->left = (tmp)->right;				\
		if ((tmp)->right) (tmp)->right->parent = (entry);	\
		(tmp)->right = (entry);					\
		(tmp)->parent = (entry)->parent;			\
		(entry)->parent = (tmp);				\
		(entry) = (tmp);					\
	} while (0)

/*
 * Left rotate entry.
 *
 * @entry: node on which to perform rotation
 * @tmp:   right child of entry
 * Note:   Entry and tmp shall not be NIL.
 */
#define SPLAY_ROTATE_LEFT(entry, tmp) do {			\
		(entry)->right = (tmp)->left;			\
		if ((tmp)->left) (tmp)->left->parent = (entry);	\
		(tmp)->left = (entry);				\
		(tmp)->parent = (entry)->parent;		\
		(entry)->parent = (tmp);			\
		(entry) = (tmp);				\
	} while (0)

/*
 * Splay tree link right operation.
 *
 * Entry and large shall not be NIL.
 */
#define SPLAY_LINK_RIGHT(entry, large) do {	\
		(large)->left = (entry);	\
		(entry)->parent = (large);	\
		(large) = (entry);		\
		(entry) = (entry)->left;	\
	} while (0)

/*
 * Splay tree link left operation.
 *
 * Entry and small shall not be NIL.
 */
#define SPLAY_LINK_LEFT(entry, small) do {	\
		(small)->right = (entry);	\
		(entry)->parent = (small);	\
		(small) = (entry);		\
		(entry) = (entry)->right;	\
	} while (0)

/*
 * Assemble splay tree from subtrees.
 *
 * Head, node, small and large shall not be NIL.
 */
#define SPLAY_ASSEMBLE(head, node, small, large) do {		\
		(small)->right = (head)->left;			\
		if ((head)->left)				\
			(head)->left->parent = (small);		\
		(large)->left = (head)->right;			\
		if ((head)->right)				\
			(head)->right->parent = (large);	\
		(head)->left = (node)->right;			\
		if ((node)->right)				\
			(node)->right->parent = (head);		\
		(head)->right = (node)->left;			\
		if ((node)->left)				\
			(node)->left->parent = (head);		\
	} while (0)

/*
 * Right rotate entry.
 *
 * @entry: node on which to perform rotation
 * @tmp:   left child of entry
 * Note:   Entry and tmp shall not be NIL.
 */
#define SPLAY_ROTATE_RIGHT_NPARENT(entry, tmp) do {	\
		(entry)->left = (tmp)->right;		\
		(tmp)->right = (entry);			\
		(entry) = (tmp);			\
	} while (0)

/*
 * Left rotate entry.
 *
 * @entry: node on which to perform rotation
 * @tmp:   right child of entry
 * Note:   Entry and tmp shall not be NIL.
 */
#define SPLAY_ROTATE_LEFT_NPARENT(entry, tmp) do {	\
		(entry)->right = (tmp)->left;		\
		(tmp)->left = (entry);			\
		(entry) = (tmp);			\
	} while (0)

/*
 * Splay tree link right operation.
 *
 * Entry and large shall not be NIL.
 */
#define SPLAY_LINK_RIGHT_NPARENT(entry, large) do {	\
		(large)->left = (entry);		\
		(large) = (entry);			\
		(entry) = (entry)->left;		\
	} while (0)

/*
 * Splay tree link left operation.
 *
 * Entry and small shall not be NIL.
 */
#define SPLAY_LINK_LEFT_NPARENT(entry, small) do {	\
		(small)->right = (entry);		\
		(small) = (entry);			\
		(entry) = (entry)->right;		\
	} while (0)

/*
 * Assemble splay tree from subtrees.
 *
 * Head, node, small and large shall not be NIL.
 */
#define SPLAY_ASSEMBLE_NPARENT(head, node, small, large) do {	\
		(small)->right = (head)->left;			\
		(large)->left = (head)->right;			\
		(head)->left = (node)->right;			\
		(head)->right = (node)->left;			\
	} while (0)

struct tree_root *
splay_search(struct tree_root *entry,
	     int (*compare) (const void *, const void *),
	     struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	head = *root;
	small = large = &node;

	while ((cmp = compare(entry, head)) != 0) {
		if (cmp < 0) {
			tmp = head->left;
			if (tmp == NIL)
				break;
			if (compare(entry, tmp) < 0) {
				SPLAY_ROTATE_RIGHT(head, tmp);
				if (head->left == NIL)
					break;
			}
			SPLAY_LINK_RIGHT(head, large);
		} else {
			tmp = head->right;
			if (tmp == NIL)
				break;
			if (compare(entry, tmp) > 0) {
				SPLAY_ROTATE_LEFT(head, tmp);
				if (head->right == NIL)
					break;
			}
			SPLAY_LINK_LEFT(head, small);
		}
	}
	head->parent = NIL;
	SPLAY_ASSEMBLE(head, &node, small, large);
	*root = head;

	if (cmp != 0)
		return NIL;

	return head;
}

struct tree_root *
splay_map(struct tree_root *new,
	  int (*compare) (const void *, const void *),
	  struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	INIT_TREE_ROOT(new);
	small = large = &node;
	head = *root;

	while (head && (cmp = compare(new, head)) != 0) {
		if (cmp < 0) {
			tmp = head->left;
			if (tmp == NIL) {
				/* zig */
				SPLAY_LINK_RIGHT(head, large);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp < 0) {
				/* zig-zig */
				SPLAY_ROTATE_RIGHT(head, tmp);
				SPLAY_LINK_RIGHT(head, large);
			} else if (cmp > 0) {
				/* zig-zag */
				SPLAY_LINK_RIGHT(head, large);
				SPLAY_LINK_LEFT(head, small);
			} else {
				/* zig */
				SPLAY_LINK_RIGHT(head, large);
				break;
			}
		} else {
			tmp = head->right;
			if (tmp == NIL) {
				/* zag */
				SPLAY_LINK_LEFT(head, small);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp > 0) {
				/* zag-zag */
				SPLAY_ROTATE_LEFT(head, tmp);
				SPLAY_LINK_LEFT(head, small);
			} else if (cmp < 0) {
				/* zag-zig */
				SPLAY_LINK_LEFT(head, small);
				SPLAY_LINK_RIGHT(head, large);
			} else {
				/* zag */
				SPLAY_LINK_LEFT(head, small);
				break;
			}
		}
	}

	if (head == NIL)
		head = new;

	head->parent = NIL;

	SPLAY_ASSEMBLE(head, &node, small, large);

	*root = head;

	return head;
}

struct tree_root *
splay_search_nparent(struct tree_root *entry,
		     int (*compare) (const void *, const void *),
		     struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	head = *root;
	small = large = &node;

	while ((cmp = compare(entry, head)) != 0) {
		if (cmp < 0) {
			tmp = head->left;
			if (tmp == NIL)
				break;
			if (compare(entry, tmp) < 0) {
				SPLAY_ROTATE_RIGHT_NPARENT(head, tmp);
				if (head->left == NIL)
					break;
			}
			SPLAY_LINK_RIGHT_NPARENT(head, large);
		} else {
			tmp = head->right;
			if (tmp == NIL)
				break;
			if (compare(entry, tmp) > 0) {
				SPLAY_ROTATE_LEFT_NPARENT(head, tmp);
				if (head->right == NIL)
					break;
			}
			SPLAY_LINK_LEFT_NPARENT(head, small);
		}
	}

	SPLAY_ASSEMBLE_NPARENT(head, &node, small, large);
	*root = head;

	if (cmp != 0)
		return NIL;

	return head;
}

struct tree_root *
splay_map_nparent(struct tree_root *new,
		  int (*compare) (const void *, const void *),
		  struct tree_root **root)
{
	int cmp;
	TREE_ROOT(node);  /* node for assembly use */
	struct tree_root *small, *large, *head, *tmp;

	INIT_TREE_ROOT(new);
	small = large = &node;
	head = *root;

	while (head && (cmp = compare(new, head)) != 0) {
		if (cmp < 0) {
			tmp = head->left;
			if (tmp == NIL) {
				/* zig */
				SPLAY_LINK_RIGHT_NPARENT(head, large);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp < 0) {
				/* zig-zig */
				SPLAY_ROTATE_RIGHT_NPARENT(head, tmp);
				SPLAY_LINK_RIGHT_NPARENT(head, large);
			} else if (cmp > 0) {
				/* zig-zag */
				SPLAY_LINK_RIGHT_NPARENT(head, large);
				SPLAY_LINK_LEFT_NPARENT(head, small);
			} else {
				/* zig */
				SPLAY_LINK_RIGHT_NPARENT(head, large);
				break;
			}
		} else {
			tmp = head->right;
			if (tmp == NIL) {
				/* zag */
				SPLAY_LINK_LEFT_NPARENT(head, small);
				break;
			}
			cmp = compare(new, tmp);
			if (cmp > 0) {
				/* zag-zag */
				SPLAY_ROTATE_LEFT_NPARENT(head, tmp);
				SPLAY_LINK_LEFT_NPARENT(head, small);
			} else if (cmp < 0) {
				/* zag-zig */
				SPLAY_LINK_LEFT_NPARENT(head, small);
				SPLAY_LINK_RIGHT_NPARENT(head, large);
			} else {
				/* zag */
				SPLAY_LINK_LEFT_NPARENT(head, small);
				break;
			}
		}
	}

	if (head == NIL)
		head = new;

	SPLAY_ASSEMBLE_NPARENT(head, &node, small, large);

	*root = head;

	return head;
}


/*
 * Post-Insert balance factor update and entry rebalance
 *
 * This is only for internal AVL tree manipulation after adding new entry
 */
static inline void
__avl_balance(struct avl_root *new, struct avl_root **root)
{
	int balance = 0;
	struct avl_root *n;
	struct avl_root *r;

	while (new->parent != NIL && balance == 0) {
		/*
		 * updates balance factor
		 */
		balance = new->parent->balance;

		if (new == new->parent->left)
			new->parent->balance--;
		else
			new->parent->balance++;

		new = new->parent;
	}

	/*
	 * rebalances AVL tree
	 */
	if (new->balance == -2) {
		n = new->left;
		if (n->balance == -1) {
			__rotate_right((struct tree_root *)new,
				       (struct tree_root **)root);
			n->balance = 0;
			new->balance = 0;
		} else {
			r = n->right;
			__rotate_left((struct tree_root *)n,
				      (struct tree_root **)root);
			__rotate_right((struct tree_root *)new,
				       (struct tree_root **)root);
			if (r->balance == -1) {
				n->balance = 0;
				new->balance = 1;
			} else if (r->balance == 0) {
				n->balance = 0;
				new->balance = 0;
			} else {
				n->balance = -1;
				new->balance = 0;
			}
			r->balance = 0;
		}
	} else if (new->balance == 2) {
		n = new->right;
		if (n->balance == 1) {
			__rotate_left((struct tree_root *)new,
				      (struct tree_root **)root);
			n->balance = 0;
			new->balance = 0;
		} else {
			r = n->left;
			__rotate_right((struct tree_root *)n,
				       (struct tree_root **)root);
			__rotate_left((struct tree_root *)new,
				      (struct tree_root **)root);
			if (r->balance == -1) {
				n->balance = 1;
				new->balance = 0;
			} else if (r->balance == 0) {
				n->balance = 0;
				new->balance = 0;
			} else {
				n->balance = 0;
				new->balance = -1;
			}
			r->balance = 0;
		}
	}
}

void
avl_add(struct avl_root *new,
	int (*compare) (const void *, const void *),
	struct avl_root **root)
{
	new->balance = 0;

	/*
	 * inserts entry
	 */
	tree_add((struct tree_root *)new, compare, (struct tree_root **)root);
	__avl_balance(new, root);
}

struct avl_root *
avl_map(struct avl_root *new,
	int (*compare) (const void *, const void *),
	struct avl_root **root)
{
	struct avl_root *n;

	new->balance = 0;

	/*
	 * inserts entry
	 */
	n = (struct avl_root *)tree_map((struct tree_root *)new,
					compare, (struct tree_root **)root);

	if (n != new)
		return n;

	__avl_balance(new, root);

	return new;
}

void
avl_del(struct avl_root *entry, struct avl_root **root)
{
	int dir = 0;
	int dir_next = 0;
	struct avl_root *new;
	struct avl_root *n;
	struct avl_root *r;
	struct avl_root *p;

	/*
	 * deletes entry
	 */
	if (entry->right == NIL) {
		/*
		 * Case 1: entry has no right child
		 */
		if (entry->left != NIL)
			entry->left->parent = entry->parent;
		if (entry->parent == NIL) {
			*root = entry->left;
			return;
		} else if (entry == entry->parent->left) {
			entry->parent->left = entry->left;
			dir = 0;
		} else {
			entry->parent->right = entry->left;
			dir = 1;
		}
		new = entry->parent;
	} else if (entry->right->left == NIL) {
		/*
		 * Case 2: entry's right child has no left child
		 */
		entry->right->left = entry->left;
		if (entry->left != NIL)
			entry->left->parent = entry->right;
		entry->right->parent = entry->parent;
		if (entry->parent == NIL)
			*root = entry->right;
		else if (entry == entry->parent->left)
			entry->parent->left = entry->right;
		else
			entry->parent->right = entry->right;
		entry->right->balance = entry->balance;
		dir = 1;
		new = entry->right;
	} else {
		/*
		 * Case 3: entry's right child has a left child
		 */

		/* splices successor entry's child and parent */
		r = (struct avl_root *)tree_successor((struct tree_root *)
						      entry);
		if (r->right != NIL)
			r->right->parent = r->parent;
		r->parent->left = r->right;

		new = r->parent;

		r->left = entry->left;
		entry->left->parent = r;
		r->right = entry->right;
		entry->right->parent = r;
		r->parent = entry->parent;
		if (entry->parent == NIL)
			*root = r;
		else if (entry == entry->parent->left)
			entry->parent->left = r;
		else
			entry->parent->right = r;
		r->balance = entry->balance;
		dir = 0;
	}

	for (;;) {
		/*
		 * updates balance factor and rebalances if necessary
		 */
		p = new->parent;
		if (p != NIL)
			dir_next = (new == p->right);

		if (dir == 0) {
			new->balance++;
			if (new->balance == 1)
				break;
			if (new->balance == 2) {
				n = new->right;
				if (n->balance == -1) {
					r = n->left;
					__rotate_right((struct tree_root *)n,
						       (struct tree_root **)
						       root);
					__rotate_left((struct tree_root *)new,
						      (struct tree_root **)
						      root);
					if (r->balance == -1) {
						n->balance = 1;
						new->balance = 0;
					} else if (r->balance == 0) {
						n->balance = 0;
						new->balance = 0;
					} else {
						n->balance = 0;
						new->balance = -1;
					}
					r->balance = 0;
				} else {
					__rotate_left((struct tree_root *)new,
						      (struct tree_root **)
						      root);
					if (n->balance == 0) {
						n->balance = -1;
						new->balance = 1;
						break;
					} else {
						n->balance = 0;
						new->balance = 0;
					}
				}
			}
		} else {
			new->balance--;
			if (new->balance == -1)
				break;
			if (new->balance == -2) {
				n = new->left;
				if (n->balance == 1) {
					r = n->right;
					__rotate_left((struct tree_root *)n,
						      (struct tree_root **)
						      root);
					__rotate_right((struct tree_root *)new,
						       (struct tree_root **)
						       root);
					if (r->balance == -1) {
						n->balance = 0;
						new->balance = 1;
					} else if (r->balance == 0) {
						n->balance = 0;
						new->balance = 0;
					} else {
						n->balance = -1;
						new->balance = 0;
					}
					r->balance = 0;
				} else {
					__rotate_right((struct tree_root *)new,
						       (struct tree_root **)
						       root);
					if (n->balance == 0) {
						n->balance = 1;
						new->balance = -1;
						break;
					} else {
						n->balance = 0;
						new->balance = 0;
					}
				}
			}
		}

		if (p == NIL)
			break;
		dir = dir_next;
		new = p;
	}
}
