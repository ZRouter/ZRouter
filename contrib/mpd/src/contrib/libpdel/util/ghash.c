
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
#include <netinet/in.h>

#ifndef _KERNEL
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <err.h>
#else
#include <sys/systm.h>
#include <sys/syslog.h>
#endif

#include "structs/structs.h"
#include "structs/type/array.h"

#include "util/ghash.h"
#include "util/typed_mem.h"

#define MIN_BUCKETS			31
#define DEFAULT_MAX_LOAD		75	/* 75% full */
#define MIN_LOAD_FACTOR			20	/* 1/20 of current size */

struct gent {
	const void	*item;			/* this item */
	struct gent	*next;			/* next item in bucket */
};

struct ghash {
	void		*arg;
	ghash_hash_t	*hash;
	ghash_equal_t	*equal;
	ghash_add_t	*add;
	ghash_del_t	*del;
	u_int		maxload;		/* maximum load factor in % */
	u_int		mods;			/* modification counter */
	u_int		size;			/* number of items in table */
	u_int		maxsize;		/* when we need to increase */
	u_int		minsize;		/* when we need to decrease */
	u_int		nbuckets;		/* number of buckets in table */
	u_int		iter_count;		/* number outstanding iter's */
	struct gent	**buckets;		/* hash bucket array */
	struct {				/* typed memory alloc types */
	    const char	*g;
	    char	g_buf[TYPED_MEM_TYPELEN];
	    const char	*gent;
	    char	gent_buf[TYPED_MEM_TYPELEN];
	    const char	*iter;
	    char	iter_buf[TYPED_MEM_TYPELEN];
	    const char	*buckets;
	    char	buckets_buf[TYPED_MEM_TYPELEN];
	}		mtype;
	u_char		locked;
};

/*
 * Internal functions
 */

static ghash_equal_t	ghash_default_equal;
static ghash_hash_t	ghash_default_hash;
static ghash_add_t	ghash_default_add;
static ghash_del_t	ghash_default_del;

static void	ghash_rehash(struct ghash *g, u_int new_nbuckets);

/* Update maxsize and minsize after nbuckets changes */
#define UPDATE_SIZES(g)							\
	do {								\
		(g)->maxsize = ((g)->nbuckets * (g)->maxload) / 100;	\
		if ((g)->maxsize == 0)					\
			(g)->maxsize = 1;				\
		(g)->minsize = (g)->nbuckets / MIN_LOAD_FACTOR;		\
	} while (0)

/*
 * Create a new hash table.
 */
struct ghash *
ghash_create(void *arg, u_int isize, u_int maxload, const char *mtype,
	ghash_hash_t *hash, ghash_equal_t *equal, ghash_add_t *add,
	ghash_del_t *del)
{
	struct ghash *g;

	/* Apply defaults and sanity check */
	if (isize < MIN_BUCKETS)
		isize = MIN_BUCKETS;
	if (maxload == 0)
		maxload = DEFAULT_MAX_LOAD;

	/* Create new hash table object */
	if ((g = MALLOC(mtype, sizeof(*g))) == NULL)
		return (NULL);
	memset(g, 0, sizeof(*g));
	g->arg = arg;
	g->hash = hash != NULL ? hash : ghash_default_hash;
	g->equal = equal != NULL ? equal : ghash_default_equal;
	g->add = add != NULL ? add : ghash_default_add;
	g->del = del != NULL ? del: ghash_default_del;
	g->nbuckets = isize;
	g->maxload = maxload;
	UPDATE_SIZES(g);

	/* Create memory subtypes */
	if (mtype != NULL) {
		snprintf(g->mtype.g_buf, sizeof(g->mtype.g_buf), "%s", mtype);
		g->mtype.g = g->mtype.g_buf;
		snprintf(g->mtype.gent_buf, sizeof(g->mtype.gent_buf),
		    "%s.gent", mtype);
		g->mtype.gent = g->mtype.gent_buf;
		snprintf(g->mtype.iter_buf, sizeof(g->mtype.iter_buf),
		    "%s.iter", mtype);
		g->mtype.iter = g->mtype.iter_buf;
		snprintf(g->mtype.buckets_buf, sizeof(g->mtype.buckets_buf),
		    "%s.buckets", mtype);
		g->mtype.buckets = g->mtype.buckets_buf;
	}

	/* Allocate initial bucket array */
	if ((g->buckets = MALLOC(g->mtype.buckets,
	    g->nbuckets * sizeof(*g->buckets))) == NULL) {
		FREE(g->mtype.g, g);
		return (NULL);
	}
	memset(g->buckets, 0, g->nbuckets * sizeof(*g->buckets));

	/* Done */
	return (g);
}

/*
 * Destroy a hash table.
 */
void
ghash_destroy(struct ghash **gp)
{
	struct ghash *const g = *gp;
	u_int i;

	if (g == NULL)
		return;
	assert(!g->locked);
	assert(g->iter_count == 0);
	g->locked = 1;
	for (i = 0; i < g->nbuckets; i++) {
		while (g->buckets[i] != NULL) {
			struct gent *const e = g->buckets[i];
			const void *const item = e->item;

			g->buckets[i] = e->next;
			FREE(g->mtype.gent, e);
			g->size--;
			(*g->del)(g, (void *)item);
		}
	}
	FREE(g->mtype.buckets, g->buckets);
	FREE(g->mtype.g, g);
	*gp = NULL;
}

/*
 * Get the argument supplied to ghash_create().
 */
void *
ghash_arg(struct ghash *g)
{
	return (g->arg);
}

/*
 * Return number of items in the table.
 */
u_int
ghash_size(struct ghash *g)
{
	return (g->size);
}

/*
 * Get an item.
 *
 * Returns the item, or NULL if the item does not exist.
 */
void *
ghash_get(struct ghash *g, const void *item)
{
	struct gent *e;

	if (item == NULL)
		return (NULL);
	for (e = g->buckets[(*g->hash)(g, item) % g->nbuckets];
	    e != NULL; e = e->next) {
		if (item == e->item || (*g->equal)(g, item, e->item))
			return ((void *)e->item);
	}
	return (NULL);
}

/*
 * Put an item.
 *
 * Returns 0 if the item is new, 1 if it replaces an existing
 * item, and -1 if there was an error.
 */
int
ghash_put(struct ghash *g, const void *item)
{
	struct gent **start;
	struct gent *e;

	/* Sanity check */
	if (item == NULL) {
		errno = EINVAL;
		return (-1);
	}

	/* Lock hash table */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* Find item's bucket */
	start = &g->buckets[(*g->hash)(g, item) % g->nbuckets];

	/* See if item already exists, and replace it if so */
	for (e = *start; e != NULL; e = e->next) {
		if ((*g->equal)(g, item, e->item)) {
			(*g->del)(g, (void *)e->item);
			e->item = item;
			(*g->add)(g, (void *)e->item);
			g->mods++;
			g->locked = 0;
			return (1);
		}
	}

	/* Expand table if necessary */
	if (g->size + 1 > g->maxsize) {
		ghash_rehash(g, (g->nbuckets * 2) - 1);
		start = &g->buckets[(*g->hash)(g, item) % g->nbuckets];
	}
	g->mods++;

	/* Add new item */
	if ((e = MALLOC(g->mtype.gent, sizeof(*e))) == NULL) {
		g->locked = 0;
		return (-1);
	}
	(*g->add)(g, (void *)item);
	e->item = item;
	e->next = *start;
	*start = e;
	g->size++;

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
ghash_remove(struct ghash *g, const void *item)
{
	struct gent **ep;
	int found = 0;

	/* Sanity check */
	if (item == NULL)
		return (0);

	/* Lock hash table */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* Find item */
	for (ep = &g->buckets[(*g->hash)(g, item) % g->nbuckets];
	    *ep != NULL; ep = &(*ep)->next) {
		struct gent *const e = *ep;

		if ((*g->equal)(g, item, e->item)) {
			const void *const oitem = e->item;

			*ep = e->next;
			FREE(g->mtype.gent, e);
			g->size--;
			(*g->del)(g, (void *)oitem);
			found = 1;
			break;
		}
	}
	if (!found) {
		g->locked = 0;
		return (0);
	}
	g->mods++;

	/* Shrink table if desired */
	if (g->size < g->minsize && g->size > MIN_BUCKETS) {
		u_int new_size;

		new_size = (g->size * g->maxload) / 200;
		if (new_size > MIN_BUCKETS)
			new_size = MIN_BUCKETS;
		ghash_rehash(g, new_size);
	}

	/* Unlock tree and return */
	g->locked = 0;
	return (1);
}

/**********************************************************************
			ITERATOR METHODS
**********************************************************************/

struct ghash_iter {
	struct ghash	*g;		/* hash table we're iterating over */
	u_int		mods;		/* guard against changes to table */
	u_int		bucket;		/* current bucket we're traversing */
	u_int		count;		/* number of items returned so far */
	struct gent	**ep;		/* points to previous rtn'd by next() */
};

struct ghash_iter *
ghash_iter_create(struct ghash *g)
{
	struct ghash_iter *iter;

	if (g == NULL) {
		errno = EINVAL;
		return (NULL);
	}
	if ((iter = MALLOC(g->mtype.iter, sizeof(*iter))) == NULL)
		return (NULL);
	memset(iter, 0, sizeof(*iter));
	iter->g = g;
	iter->mods = g->mods;
	g->iter_count++;
	return (iter);
}

void
ghash_iter_destroy(struct ghash_iter **iterp)
{
	struct ghash_iter *const iter = *iterp;

	if (iter == NULL)
		return;
	iter->g->iter_count--;
	FREE(iter->g->mtype.iter, iter);
	*iterp = NULL;
}

int
ghash_iter_has_next(struct ghash_iter *iter)
{
	struct ghash *const g = iter->g;

	if (iter->mods != g->mods)
		return (1);			/* force a call to next() */
	return (iter->count < g->size);
}

/*
 * Return next element in an iteration.
 */
void *
ghash_iter_next(struct ghash_iter *iter)
{
	struct ghash *const g = iter->g;

	/* Sanity checks */
	if (iter->mods != g->mods || iter->count >= g->size) {
		errno = EINVAL;
		return (NULL);
	}

	/* Advance pointer to next element */
	if (iter->count++ == 0)
		iter->ep = &g->buckets[0];		/* initialize pointer */
	else {
		assert(*iter->ep != NULL);
		iter->ep = &(*iter->ep)->next;
	}
	while (*iter->ep == NULL) {
		iter->ep = &g->buckets[++iter->bucket];
		assert(iter->bucket < g->nbuckets);
	}

	/* Update item count and return next item */
	return ((void *)(*iter->ep)->item);
}

/*
 * Remove previously returned element in an iteration.
 */
int
ghash_iter_remove(struct ghash_iter *iter)
{
	struct ghash *const g = iter->g;
	const void *item;
	struct gent *e;

	/* Sanity checks */
	if (iter->count == 0 || iter->mods != g->mods) {
		errno = EINVAL;
		return (-1);
	}

	/* Lock hash table */
	if (g->locked) {
		errno = EBUSY;
		return (-1);
	}
	g->locked = 1;

	/* Remove element */
	e = *iter->ep;
	item = e->item;
	*iter->ep = e->next;
	FREE(g->mtype.gent, e);
	g->size--;
	iter->count--;
	g->mods++;
	iter->mods++;
	(*g->del)(g, (void *)item);

	/* Shrink table if desired */
	if (g->size < g->minsize && g->size > MIN_BUCKETS) {
		u_int new_size;

		new_size = (g->size * g->maxload) / 200;
		if (new_size > MIN_BUCKETS)
			new_size = MIN_BUCKETS;
		ghash_rehash(g, new_size);
	}
	g->locked = 0;
	return (0);
}

/*
 * Get an array of hash table contents, optionally sorted.
 */
int
ghash_dump(struct ghash *g, void ***listp, const char *mtype)
{
	struct gent *e;
	void **list;
	u_int num;
	u_int i;

	/* Get items in a list */
	if ((list = MALLOC(mtype, g->size * sizeof(*list))) == NULL)
		return (-1);
	for (num = i = 0; i < g->nbuckets; i++) {
		for (e = g->buckets[i]; e != NULL; e = e->next)
			list[num++] = (void *)e->item;
	}
	assert(num == g->size);

	/* Done */
	*listp = list;
	return (num);
}

/*
 * Start a hash table walk.
 */
void
ghash_walk_init(struct ghash *g, struct ghash_walk *walk)
{
	memset(walk, 0, sizeof(*walk));
	walk->mods = g->mods;
}

/*
 * Get the next item in the hash table walk.
 */
void *
ghash_walk_next(struct ghash *g, struct ghash_walk *walk)
{
	void *item;

	/* Check for modifications */
	if (g->mods != walk->mods) {
		errno = EINVAL;
		return (NULL);
	}

	/* Go to next bucket if needed */
	if (walk->e == NULL) {
		while (walk->bucket < g->nbuckets
		    && g->buckets[walk->bucket] == NULL)
			walk->bucket++;
		if (walk->bucket == g->nbuckets) {
			errno = ENOENT;
			return (NULL);
		}
		walk->e = g->buckets[walk->bucket++];
	}

	/* Get item to return */
	item = (void *)walk->e->item;

	/* Point at next item for next time */
	walk->e = walk->e->next;

	/* Return item */
	return (item);
}

/**********************************************************************
			DEFAULT CALLBACKS
**********************************************************************/

static int
ghash_default_equal(struct ghash *g, const void *item1, const void *item2)
{
	return (item1 == item2);
}

static u_int32_t
ghash_default_hash(struct ghash *g, const void *item)
{
	return ((u_int32_t)(u_long)item);
}

static void
ghash_default_add(struct ghash *g, void *item)
{
	return;
}

static void
ghash_default_del(struct ghash *g, void *item)
{
	return;
}

/**********************************************************************
			HELPER METHODS
**********************************************************************/

/*
 * Resize the hash bucket array.
 */
static void
ghash_rehash(struct ghash *g, u_int new_nbuckets)
{
	struct gent **new_buckets;
	u_int i;

	/* Get new bucket array */
	assert(new_nbuckets > 0);
	if ((new_buckets = MALLOC(g->mtype.buckets,
	    new_nbuckets * sizeof(*new_buckets))) == NULL)
		return;					/* can't do it */
	memset(new_buckets, 0, new_nbuckets * sizeof(*new_buckets));

	/* Move all entries over to new array */
	for (i = 0; i < g->nbuckets; i++) {
		while (g->buckets[i] != NULL) {
			struct gent *const e = g->buckets[i];
			const u_int new_bucket
			    = (*g->hash)(g, e->item) % new_nbuckets;

			g->buckets[i] = e->next;
			e->next = new_buckets[new_bucket];
			new_buckets[new_bucket] = e;
		}
	}
	g->nbuckets = new_nbuckets;
	FREE(g->mtype.buckets, g->buckets);
	g->buckets = new_buckets;

	/* Update new upper and lower size limits */
	UPDATE_SIZES(g);
}

#ifdef GHASH_TEST

/**********************************************************************
			TEST ROUTINE
**********************************************************************/

#define MAX_ARGS		32
#define WS			" \t\r\n\v\f"

static ghash_equal_t	ghash_test_equal;
static ghash_hash_t	ghash_test_hash;
static ghash_add_t	ghash_test_add;
static ghash_del_t	ghash_test_del;

static int	ghash_test_sort(const void *p1, const void *p2);

int
main(int ac, char **av)
{
	char buf[1024];
	struct ghash *g;

	if ((g = ghash_create(NULL, 3, 0, "ghash", ghash_test_hash,
	    ghash_test_equal, ghash_test_add, ghash_test_del)) == NULL)
		err(1, "ghash_create");

	while (1) {
		char *args[MAX_ARGS];
		char *tokctx;
		char *s;

		/* Prompt */
		printf("Current size is %d items (%d buckets)\n",
		    ghash_size(g), g->nbuckets);
getline:
		printf("> ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;

		/* Parse line */
		for (ac = 0, s = strtok_r(buf, WS, &tokctx);
		    ac < MAX_ARGS && s != NULL;
		    ac++, s = strtok_r(NULL, WS, &tokctx)) {
			if ((args[ac] = STRDUP(NULL, s)) == NULL)
				err(1, "strdup");
		}
		if (ac == 0)
			goto getline;

		/* Do cmd */
		if (strcmp(args[0], "put") == 0) {
			if (ac != 2)
				err(1, "usage: put <token>");
			printf("Putting \"%s\"...\n", args[1]);
			if (ghash_put(g, args[1]) == -1)
				err(1, "ghash_put");
			printf("Done\n");
		} else if (strcmp(args[0], "get") == 0) {
			const char *s;

			if (ac != 2)
				err(1, "usage: get <token>");
			if ((s = ghash_get(g, args[1])) == NULL)
				printf("\"%s\" was not found\n", args[1]);
			else
				printf("Found \"%s\"\n", s);
		} else if (strcmp(args[0], "del") == 0) {
			int rtn;

			if (ac != 2)
				err(1, "usage: del <token>");
			if ((rtn = ghash_remove(g, args[1])) == -1)
				err(1, "ghash_remove");
			if (rtn)
				printf("Removed \"%s\"\n", args[1]);
			else
				printf("\"%s\" was not found\n", args[1]);
		} else if (strcmp(args[0], "dump") == 0) {
			struct ghash_iter *iter;

			if ((iter = ghash_iter_create(g)) == NULL)
				err(1, "ghash_iter_create");
			printf("Iterating contents...\n");
			while (ghash_iter_has_next(iter)) {
				const char *s = ghash_iter_next(iter);

				if (s == NULL)
					err(1, "ghash_iter_next");
				printf("\t\"%s\"\n", s);
			}
			ghash_iter_destroy(&iter);
			printf("Done\n");
		} else if (strcmp(args[0], "sort") == 0) {
			void **list;
			int i, num;

			if ((num = ghash_dump(g, &list, TYPED_MEM_TEMP)) == -1)
				err(1, "ghash_get_sorted");
			printf("Sorting contents...\n");
			qsort(list, num, sizeof(*list), ghash_test_sort);
			for (i = 0; i < num; i++)
				printf("\t\"%s\"\n", (const char *)list[i]);
			FREE(TYPED_MEM_TEMP, list);
			printf("Done\n");
		} else {
			printf("Commands:\n"
			"\tget <token>\n"
			"\tput <token>\n"
			"\tdel <token>\n"
			"\tdump\n"
			"\tsort\n");
		}
	}
	if (ferror(stdin))
		err(1, "stdin");
	printf("\n");
	ghash_destroy(&g);
	typed_mem_dump(stdout);
	return (0);
}

static int
ghash_test_equal(struct ghash *g, const void *item1, const void *item2)
{
	return (strcmp((const char *)item1, (const char *)item2) == 0);
}

static int
ghash_test_sort(const void *p1, const void *p2)
{
	const char *s1 = *((const char **)p1);
	const char *s2 = *((const char **)p2);

	return (strcmp(s1, s2));
}

static u_int32_t
ghash_test_hash(struct ghash *g, const void *item)
{
	const char *s = item;
	u_int32_t hash;

	for (hash = 0; *s != '\0'; s++)
		hash = (hash * 31) + (u_char)*s;
	return (hash);
}

static void
ghash_test_add(struct ghash *g, void *item)
{
	printf("-> adding \"%s\"\n", (char *)item);
}

static void
ghash_test_del(struct ghash *g, void *item)
{
	printf("-> deleting \"%s\"\n", (char *)item);
}

#endif	/* GHASH_TEST */
