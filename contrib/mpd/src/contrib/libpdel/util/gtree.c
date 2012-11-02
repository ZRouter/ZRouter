
/*
 * Copyright (c) 2001-2002 Packet Design, LLC.
 * All rights reserved.
 * 
 * Subject to the following obligations and disclaimer of warranty,
 * use and redistribution of this software, in source or object code
 * forms, with or without modifications are expressly permitted by
 * Packet Design; provided, however, that:
 * 
 *    (i)  Any and all reproductions of the source or object code
 *         must include the copyright notice above and the following
 *         disclaimer of warranties; and
 *    (ii) No rights are granted, in any manner or form, to use
 *         Packet Design trademarks, including the mark "PACKET DESIGN"
 *         on advertising, endorsements, or otherwise except as such
 *         appears in the above copyright notice or in the software.
 * 
 * THIS SOFTWARE IS BEING PROVIDED BY PACKET DESIGN "AS IS", AND
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, PACKET DESIGN MAKES NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, REGARDING
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY AND ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT.  PACKET DESIGN DOES NOT WARRANT, GUARANTEE,
 * OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF, OR THE RESULTS
 * OF THE USE OF THIS SOFTWARE IN TERMS OF ITS CORRECTNESS, ACCURACY,
 * RELIABILITY OR OTHERWISE.  IN NO EVENT SHALL PACKET DESIGN BE
 * LIABLE FOR ANY DAMAGES RESULTING FROM OR ARISING OUT OF ANY USE
 * OF THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE, OR CONSEQUENTIAL
 * DAMAGES, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, LOSS OF
 * USE, DATA OR PROFITS, HOWEVER CAUSED AND UNDER ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF PACKET DESIGN IS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Archie Cobbs <archie@freebsd.org>
 */

#include <sys/types.h>
#include <sys/param.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#endif	/* !_KERNEL */

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/gtree.h"
#include "util/typed_mem.h"

/* Enabled debug tracing: only use this when keys are strings */
#define GTREE_TRACE	0

enum node_color {
	BLACK=0,
	RED=1
};

/* One node in the tree */
struct node {
	enum node_color	color;			/* item color */
	struct node	*left;			/* left child */
	struct node	*right;			/* right child */
	struct node	*parent;		/* parent */
	const void	*item;			/* this item */
};

/* The tree itself */
struct gtree {
	void		*arg;
	gtree_cmp_t	*cmp;
	gtree_add_t	*add;
	gtree_del_t	*del;
	gtree_print_t	*print;
	u_int		mods;			/* modification counter */
	u_int		size;			/* number of items in table */
	struct node	*root;			/* root of tree */
	const char	*mtype;			/* memory type */
	char		mtbuf[TYPED_MEM_TYPELEN];
	u_char		locked;
};

/*
 * Internal functions
 */
static struct	node *gtree_find(struct gtree *g,
			const void *item, struct node **parentp);
static struct	node *gtree_get_next(struct gtree *g, struct node *node);
static struct	node *gtree_get_prev(struct gtree *g, struct node *node);
static void	gtree_insert_fixup(struct gtree *g, struct node *x);
static void	gtree_delete_fixup(struct gtree *g, struct node *x);
static void	gtree_rotate_left(struct gtree *g, struct node *x);
static void	gtree_rotate_right(struct gtree *g, struct node *x);
#ifndef _KERNEL
static void	gtree_print_node(struct gtree *g, FILE *fp,
			struct node *node, const char *which, int depth);
#endif

static gtree_cmp_t	gtree_default_cmp;
static gtree_add_t	gtree_default_add;
static gtree_del_t	gtree_default_del;

/*
 * Internal variables
 */

#define NIL	(&nil)

/* Note: nil->parent is modified during normal operation */
static struct	node nil = { BLACK, NIL, NIL, NULL };

/*
 * Create a new tree.
 */
struct gtree *
gtree_create(void *arg, const char *mtype, gtree_cmp_t *cmp,
	gtree_add_t *add, gtree_del_t *del, gtree_print_t *print)
{
	struct gtree *g;

	/* Create new tree object */
	if ((g = MALLOC(mtype, sizeof(*g))) == NULL)
		return (NULL);
	memset(g, 0, sizeof(*g));
	g->arg = arg;
	g->cmp = cmp != NULL ? cmp : gtree_default_cmp;
	g->add = add != NULL ? add : gtree_default_add;
	g->del = del != NULL ? del: gtree_default_del;
	g->print = print;
	g->root = NIL;

	/* Create memory subtypes */
	if (mtype != NULL) {
		strlcpy(g->mtbuf, mtype, sizeof(g->mtbuf));
		g->mtype = g->mtbuf;
	}

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL)
		fprintf(stderr, "%s[%p]: tree created\n", __FUNCTION__, g);
#endif

	/* Done */
	return (g);
}

/*
 * Destroy a tree.
 */
void
gtree_destroy(struct gtree **gp)
{
	struct gtree *const g = *gp;

	/* Sanity check */
	if (g == NULL)
		return;
	assert(!g->locked);

	/* Remove all remaining nodes */
	while (g->root != NIL)
		gtree_remove(g, g->root->item);

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL)
		fprintf(stderr, "%s[%p]: tree destroyed\n", __FUNCTION__, g);
#endif

	/* Free object */
	FREE(g->mtype, g);
	*gp = NULL;
}

/*
 * Get the argument supplied to gtree_create().
 */
void *
gtree_arg(struct gtree *g)
{
	return (g->arg);
}

/*
 * Return number of items in the table.
 */
u_int
gtree_size(struct gtree *g)
{
	return (g->size);
}

/*
 * Get an item.
 *
 * Returns the item, or NULL if the item does not exist.
 */
void *
gtree_get(struct gtree *g, const void *item)
{
	struct node *dummy;
	struct node *node;

	/* Find node */
	node = gtree_find(g, item, &dummy);

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL) {
		fprintf(stderr, "%s[%p]: key=\"%s\" found=%d size=%u\n",
		    __FUNCTION__, g, (*g->print)(g, (void *)item),
		    node != NULL, g->size);
	}
#endif

	/* Return result */
	if (node != NULL)
		return ((void *)node->item);
	return (NULL);
}

/*
 * Put an item.
 *
 * Returns 0 if the item is new, 1 if it replaces an existing
 * item, and -1 if there was an error.
 */
int
gtree_put(struct gtree *g, const void *item)
{
	return (gtree_put_prealloc(g, item, NULL));
}

/*
 * Put an item -- common internal code.
 */
int
gtree_put_prealloc(struct gtree *g, const void *item, void *new_node)
{
	struct node *parent;
	struct node *node;

	/* NULL item is invalid */
	if (item == NULL) {
		errno = EINVAL;
		return (-1);
	}

	/* Lock tree */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* See if item already exists */
	node = gtree_find(g, item, &parent);

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL) {
		fprintf(stderr, "%s[%p]: key=\"%s\" exists=%d newsize=%u\n",
		    __FUNCTION__, g, (*g->print)(g, (void *)item),
		    node != NULL, g->size + (node == NULL));
	}
#endif

	/* If item already exists, just replace */
	if (node != NULL) {
		if (new_node != NULL)
			FREE(g->mtype, new_node);	/* didn't need it */
		g->mods++;
		(*g->del)(g, (void *)node->item);
		node->item = item;
		(*g->add)(g, (void *)node->item);
		g->locked = 0;
		return (1);
	}

	/* Allocate or use caller-supplied memory for new node */
	if (new_node != NULL)
		node = new_node;
	else if ((node = MALLOC(g->mtype, sizeof(*node))) == NULL) {
		g->locked = 0;
		return (-1);
	}
	memset(node, 0, sizeof(*node));
	node->color = RED;
	node->left = NIL;
	node->right = NIL;
	node->parent = parent;
	node->item = item;
	g->mods++;
	g->size++;

	/* Add to tree */
	if (parent != NULL) {
		if ((*g->cmp)(g, node->item, parent->item) < 0)
			parent->left = node;
		else
			parent->right = node;
	} else
		g->root = node;

	/* Rebalance */
	gtree_insert_fixup(g, node);

	/* Notify caller of new item */
	(*g->add)(g, (void *)node->item);

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL)
		gtree_print(g, stderr);
#endif

	/* Unlock tree and return */
	g->locked = 0;
	return (0);
}

/*
 * Remove an item.
 *
 * Returns 1 if the item was found and removed, 0 if not found.
 */
int
gtree_remove(struct gtree *g, const void *item)
{
	struct node *dummy;
	struct node *node;
	struct node *x;
	struct node *y;

	/* Lock tree */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* Find node */
	node = gtree_find(g, item, &dummy);

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL) {
		fprintf(stderr, "%s[%p]: key=\"%s\" exists=%d newsize=%u\n",
		    __FUNCTION__, g, (*g->print)(g, (void *)item),
		    node != NULL, g->size - (node != NULL));
	}
#endif

	/* If not found, nothing to do */
	if (node == NULL) {
		g->locked = 0;
		return (0);
	}

	/* Notify caller of deletion */
	(*g->del)(g, (void *)node->item);

	/* Set y to node or first successor with a NIL child */
	if (node->left == NIL || node->right == NIL)
		y = node;
	else
		for (y = node->right; y->left != NIL; y = y->left);

	/* Set x to y's only child, or NIL if no children */
	if (y->left != NIL)
		x = y->left;
	else
		x = y->right;

	/* Remove y from the parent chain */
	x->parent = y->parent;
	if (y->parent != NULL) {
		if (y == y->parent->left)
			y->parent->left = x;
		else
			y->parent->right = x;
	} else
		g->root = x;

	/* Move y's item to node */
	if (y != node)
		node->item = y->item;

	/* Do fixup if necessary */
	if (y->color == BLACK)
		gtree_delete_fixup(g, x);

	/* Free node y */
	FREE(g->mtype, y);

	/* Update stats */
	g->mods++;
	g->size--;

#if GTREE_TRACE
	/* Tracing */
	if (g->print != NULL)
		gtree_print(g, stderr);
#endif

	/* Unlock tree and return */
	g->locked = 0;
	return (1);
}

/*
 * Get the size of a node.
 */
u_int
gtree_node_size(void)
{
	return (sizeof(struct node));
}

/*
 * Get the first item.
 */
void *
gtree_first(struct gtree *g)
{
	struct node *node;

	if ((node = gtree_get_next(g, NULL)) == NULL)
		return (NULL);
	return ((void *)node->item);
}

/*
 * Get the next item.
 */
void *
gtree_next(struct gtree *g, const void *item)
{
	struct node *dummy;
	struct node *node;

	if ((node = gtree_find(g, item, &dummy)) == NULL)
		return (NULL);
	if ((node = gtree_get_next(g, node)) == NULL)
		return (NULL);
	return ((void *)node->item);
}

/*
 * Get the last item.
 */
void *
gtree_last(struct gtree *g)
{
	struct node *node;

	if ((node = gtree_get_prev(g, NULL)) == NULL)
		return (NULL);
	return ((void *)node->item);
}

/*
 * Get the previous item.
 */
void *
gtree_prev(struct gtree *g, const void *item)
{
	struct node *dummy;
	struct node *node;

	if ((node = gtree_find(g, item, &dummy)) == NULL)
		return (NULL);
	if ((node = gtree_get_prev(g, node)) == NULL)
		return (NULL);
	return ((void *)node->item);
}

/*
 * Traverse the tree.
 */
int
gtree_traverse(struct gtree *g, int (*handler)(struct gtree *g, void *item))
{
	struct node *node;
	int r = 0;

	/* Lock tree */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* Get items in a list */
	for (node = gtree_get_next(g, NULL);
	    node != NULL; node = gtree_get_next(g, node)) {
		if ((*handler)(g, (void *)node->item) == -1) {
			r = -1;
			break;
		}
	}

	/* Done */
	g->locked = 0;
	return (r);
}

/*
 * Get a sorted array of tree contents.
 */
int
gtree_dump(struct gtree *g, void ***listp, const char *mtype)
{
	struct node *node;
	void **list;
	int i;

	/* Get items in a list */
	if ((list = MALLOC(mtype, g->size * sizeof(*list))) == NULL)
		return (-1);
	for (i = 0, node = gtree_get_next(g, NULL);
	    node != NULL; node = gtree_get_next(g, node))
		list[i++] = (void *)node->item;
	assert(i == g->size);

	/* Done */
	*listp = list;
	return (i);
}

#ifndef _KERNEL
/*
 * Print out a tree
 */
void
gtree_print(struct gtree *g, FILE *fp)
{
	gtree_print_node(g, fp, g->root, "ROOT", 0);
}
#endif

/**********************************************************************
			DEFAULT CALLBACKS
**********************************************************************/

static int
gtree_default_cmp(struct gtree *g, const void *item1, const void *item2)
{
	if (item1 < item2)
		return (-1);
	if (item1 > item2)
		return (-1);
	return (0);
}

static void
gtree_default_add(struct gtree *g, void *item)
{
	return;
}

static void
gtree_default_del(struct gtree *g, void *item)
{
	return;
}

/**********************************************************************
			HELPER METHODS
**********************************************************************/

/*
 * Find an item. If not found, set *parentp to the would-be parent
 * (NULL if the tree is empty).
 */
static struct node *
gtree_find(struct gtree *g, const void *item, struct node **parentp)
{
	struct node *node;
	int diff;

	*parentp = NULL;
	for (node = g->root; node != NIL; ) {
		diff = (*g->cmp)(g, item, node->item);
		if (diff == 0)
			return (node);
		*parentp = node;
		node = (diff < 0) ? node->left : node->right;
	}
	return (NULL);
}

/*
 * Get the next item in the tree after "node", or the first
 * item if "node" is NULL.
 */
static struct node *
gtree_get_next(struct gtree *g, struct node *node)
{
	if (node == NULL) {
		if (g->root == NIL)
			return (NULL);
		node = g->root;
	} else if (node->right != NIL)
		node = node->right;
	else {
		while (1) {
			if (node->parent == NULL
			    || node == node->parent->left)
				return (node->parent);
			node = node->parent;
		}
	}
	while (node->left != NIL)
		node = node->left;
	return (node);
}

/*
 * Get the previous item in the tree before "node", or the last
 * item if "node" is NULL.
 */
static struct node *
gtree_get_prev(struct gtree *g, struct node *node)
{
	if (node == NULL) {
		if (g->root == NIL)
			return (NULL);
		node = g->root;
	} else if (node->left != NIL)
		node = node->left;
	else {
		while (1) {
			if (node->parent == NULL
			    || node == node->parent->right)
				return (node->parent);
			node = node->parent;
		}
	}
	while (node->right != NIL)
		node = node->right;
	return (node);
}

/*
 * Rotate node x to the left
 */
static void
gtree_rotate_left(struct gtree *g, struct node *x)
{
	struct node *y = x->right;

	x->right = y->left;
	if (y->left != NIL)
		y->left->parent = x;
	if (y != NIL)
		y->parent = x->parent;
	if (x->parent != NULL) {
		if (x == x->parent->left)
			x->parent->left = y;
		else
			x->parent->right = y;
	} else
		g->root = y;
	y->left = x;
	if (x != NIL)
		x->parent = y;
}

/*
 * Rotate node x to the right
 */
static void
gtree_rotate_right(struct gtree *g, struct node *x)
{
	struct node *y = x->left;

	x->left = y->right;
	if (y->right != NIL)
		y->right->parent = x;
	if (y != NIL)
		y->parent = x->parent;
	if (x->parent != NULL) {
		if (x == x->parent->right)
			x->parent->right = y;
		else
			x->parent->left = y;
	} else
		g->root = y;
	y->right = x;
	if (x != NIL)
		x->parent = y;
}

/*
 * Reestablish red/black balance after inserting "node"
 */
static void
gtree_insert_fixup(struct gtree *g, struct node *x)
{
	while (x != g->root && x->parent->color == RED) {
		if (x->parent == x->parent->parent->left) {
			struct node *y = x->parent->parent->right;

			if (y->color == RED) {
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {
				if (x == x->parent->right) {
					x = x->parent;
					gtree_rotate_left(g, x);
				}
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				gtree_rotate_right(g, x->parent->parent);
			}
		} else {
			struct node *y = x->parent->parent->left;

			if (y->color == RED) {
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {
				if (x == x->parent->left) {
					x = x->parent;
					gtree_rotate_right(g, x);
				}
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				gtree_rotate_left(g, x->parent->parent);
			}
		}
	}
	g->root->color = BLACK;
}

/*
 * Reestablish red/black balance after deleting node x
 */
static void
gtree_delete_fixup(struct gtree *g, struct node *x)
{
	while (x != g->root && x->color == BLACK) {
		if (x == x->parent->left) {
			struct node *w = x->parent->right;

			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				gtree_rotate_left(g, x->parent);
				w = x->parent->right;
			}
			if (w->left->color == BLACK
			    && w->right->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					gtree_rotate_right(g, w);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				gtree_rotate_left(g, x->parent);
				x = g->root;
			}
		} else {
			struct node *w = x->parent->left;

			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				gtree_rotate_right(g, x->parent);
				w = x->parent->left;
			}
			if (w->right->color == BLACK
			    && w->left->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					gtree_rotate_left(g, w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				gtree_rotate_right(g, x->parent);
				x = g->root;
			}
		}
	}
	x->color = BLACK;
}

#ifndef _KERNEL
/*
 * Recursively print out one node and all its descendants.
 */
static void
gtree_print_node(struct gtree *g, FILE *fp, struct node *node,
	const char *which, int depth)
{
	int i;

	for (i = 0; i < depth; i++)
		fprintf(fp, "  ");
	fprintf(fp, "%s: ", which);
	if (node == NIL) {
		fprintf(fp, "NIL\n");
		return;
	}
	if (g->print != NULL)
		fprintf(fp, "%s", (*g->print)(g, (void *)node->item));
	else
		fprintf(fp, "%p", node);
	fprintf(fp, " %s\n", node->color == RED ? "RED" : "BLACK");
	if (node->left != NIL)
		gtree_print_node(g, fp, node->left, "LEFT", depth + 1);
	if (node->right != NIL)
		gtree_print_node(g, fp, node->right, "RIGHT", depth + 1);
}
#endif	/* !_KERNEL */

#ifdef GTREE_TEST

/**********************************************************************
			TEST ROUTINE
**********************************************************************/

#include <err.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

/* Debugging */
#define DBG(fmt, args...)				\
	do {						\
		if (debug)				\
			printf(fmt "\n" , ## args);	\
	} while (0)

static gtree_cmp_t	gtree_test_cmp;
static gtree_add_t	gtree_test_add;
static gtree_del_t	gtree_test_del;
static gtree_print_t	gtree_test_print;

static int	*trin;	/* trin[i] iff i in the tree, according to add, del */
static int	trsz;	/* size of tree according to add, del */
static int	*nums;	/* fixed array where nums[i] == i */
static u_long	num;	/* size of nums[] array */
static int	debug;
static struct	gtree *g;

static void	gtree_assert(int i, const char *s, const char *func, int line);
static int	chknode(struct node *node);

#define CHECK(x)	gtree_assert(x, #x, __FUNCTION__, __LINE__)

int
main(int ac, char **av)
{
	u_long count;
	u_long seed;
	int size = 0;		/* my idea of what the tree size should be */
	int *myin;		/* our idea of what values are in the tree */
	int ch;
	int i;

	/* Default seed and count */
	seed = htonl(getpid()) ^ time(NULL);
	count = 10000;

	/* Parse command line */
	while ((ch = getopt(ac, av, "dc:s:n:")) != -1) {
		switch (ch) {
		case 'c':
			count = strtoul(optarg, NULL, 0);
			break;
		case 'd':
			debug++;
			break;
		case 'n':
			num = strtoul(optarg, NULL, 0);
			break;
		case 's':
			seed = strtoul(optarg, NULL, 0);
			break;
		default:
		usage:
			errx(1, "usage: gtree [-s seed] [-c count] [-n num]");
		}
	}
	ac -= optind;
	av += optind;
	if (ac != 0)
		goto usage;

	/* Seed RNG */
	srandom(seed);
	if (num == 0)
		num = (random() % 500) + 1;
	printf("seed = %lu num = %lu count = %lu\n", seed, num, count);

	/* Initialize number array */
	if ((nums = MALLOC(TYPED_MEM_TEMP, num * sizeof(*nums))) == NULL)
		err(1, "malloc");
	if ((myin = MALLOC(TYPED_MEM_TEMP, num * sizeof(*myin))) == NULL)
		err(1, "malloc");
	if ((trin = MALLOC(TYPED_MEM_TEMP, num * sizeof(*trin))) == NULL)
		err(1, "malloc");
	for (i = 0; i < num; i++)
		nums[i] = i;
	memset(myin, 0, num * sizeof(*myin));
	memset(trin, 0, num * sizeof(*trin));

	/* Get tree */
	if ((g = gtree_create(NULL, "gtree", gtree_test_cmp,
	    gtree_test_add, gtree_test_del, gtree_test_print)) == NULL)
		err(1, "gtree_create");

	/* Iterate */
	for (i = 0; i < count; i++) {
		void **list;
		int first;
		int last;
		int add;
		int *p;
		int i;
		int r;

		/* Pick a random number and action */
		i = random() % num;
		add = (random() & (1 << (i % 17))) != 0;
		DBG("%s %u siz=%u in=%d",
		    add ? "add" : "del", i, size, myin[i]);

		/* Do an operation (add or remove) */
		if (add) {
			if (gtree_put(g, &nums[i]) == -1) {
				warn("gtree_put");
				CHECK(0);
			}
			if (!myin[i])
				size++;
			myin[i] = 1;
		} else {
			r = gtree_remove(g, &nums[i]);
			if (r == -1) {
				warn("gtree_remove");
				CHECK(0);
			}
			CHECK(r == myin[i]);
			if (myin[i])
				size--;
			myin[i] = 0;
		}

		/* Dump tree */
		if (debug >= 2)
			gtree_print(g, stdout);

		/* Check sizes */
		CHECK(trsz == size);
		CHECK(gtree_size(g) == size);

		/* Check presence of items */
		if (memcmp(trin, myin, num * sizeof(*trin)) != 0)
			CHECK(0);

		/* Check first and last */
		p = (int *)gtree_first(g);
		first = p ? *p : -1;
		p = (int *)gtree_last(g);
		last = p ? *p : -1;
		for (i = 0; i < num && myin[i] == 0; i++);
		if (i == num)
			i = -1;
		CHECK(first == i);

		/* Check last */
		for (i = num - 1; i >= 0 && myin[i] == 0; i--);
		CHECK(last == i);

		/* Check sorting */
		if ((r = gtree_dump(g, &list, TYPED_MEM_TEMP)) == -1)
			err(1, "gtree_dump");
		CHECK(r == size);
		for (i = 1; i < r; i++) {
			const int this = *((int *)list[i]);
			const int prev = *((int *)list[i - 1]);

			CHECK(prev < this);
		}
		FREE(TYPED_MEM_TEMP, list);

		/* Check red-black property */
		chknode(g->root);
	}

	/* Done */
	gtree_destroy(&g);
	FREE(TYPED_MEM_TEMP, nums);
	FREE(TYPED_MEM_TEMP, myin);
	FREE(TYPED_MEM_TEMP, trin);
	if (debug)
		typed_mem_dump(stdout);
	return (0);
}

static int
gtree_test_cmp(struct gtree *g, const void *item1, const void *item2)
{
	const int *const p1 = item1;
	const int *const p2 = item2;

	CHECK(p1 >= &nums[0] && p1 < &nums[num]);
	CHECK(p2 >= &nums[0] && p2 < &nums[num]);
	return (*((int *)item1) - *((int *)item2));
}

static void
gtree_test_add(struct gtree *g, void *item)
{
	const int *const p = item;
	const int i = *((int *)item);

	DBG("  add->%u", i);
	CHECK(p >= &nums[0] && p < &nums[num]);
	if (gtree_remove(g, item) != -1 || errno != EBUSY)
		CHECK(0);
	CHECK(!trin[i]);
	trin[i] = 1;
	trsz++;
}

static void
gtree_test_del(struct gtree *g, void *item)
{
	const int *const p = item;
	const int i = *((int *)item);

	DBG("  del->%u", i);
	CHECK(p >= &nums[0] && p < &nums[num]);
	if (gtree_put(g, item) != -1 || errno != EBUSY)
		CHECK(0);
	CHECK(trin[i]);
	trin[i] = 0;
	trsz--;
}

static const char *
gtree_test_print(struct gtree *g, const void *item)
{
	static char buf[16];

	snprintf(buf, sizeof(buf), "%03u", *((u_int *)item));
	return (buf);
}

static void
gtree_assert(int pred, const char *s, const char *func, int line)
{
	if (pred)
		return;
	printf("FAILURE: %s:%u: %s\n", func, line, s);
	gtree_print(g, stdout);
	kill(getpid(), SIGABRT);
}

static int
chknode(struct node *node)
{
	int l, r;

	CHECK(node->color == RED || node->color == BLACK);
	if (node == NIL) {
		CHECK(node->left == NIL && node->right == NIL);
		CHECK(node->color == BLACK);
		return (1);
	}
	if (node->color == RED) {
		CHECK(node->left->color == BLACK);
		CHECK(node->right->color == BLACK);
	}
	l = chknode(node->left);
	r = chknode(node->right);
	CHECK(l == r);
	return (l + (node->color == BLACK));
}

#endif	/* GTREE_TEST */
