
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

#ifndef _KERNEL
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#else
#include <sys/systm.h>
#include <machine/stdarg.h>
#include <malloc.h>
#include "util/kernelglue.h"
#endif

#include "structs/structs.h"
#include "structs/type/array.h"
#include "structs/type/struct.h"
#include "structs/type/string.h"
#include "structs/type/int.h"
#include "util/gtree.h"
#include "util/typed_mem.h"

/* Alignment, with a minimum of 4 bytes */
#if defined(__FreeBSD__)
#include <machine/param.h>
#define ALIGNMENT	(_ALIGNBYTES + 1)
#else
#define ALIGNMENT	8			/* conservative guess */
#endif

/* Summary information about a single type of memory */
struct mem_type {
	char		name[TYPED_MEM_TYPELEN];/* string for this type */
	u_int32_t	count;			/* number of allocated blocks */
	size_t		total;			/* total allocated bytes */
};

/* Information about a single allocated block of memory */
struct mem_info {
	void		*mem;			/* actual memory block */
	size_t		size;			/* size of memory block */
	struct mem_type	*type;			/* pointer to type info */
};

/*
 * Structs type for 'struct typed_mem_stats'
 */
#define TYPED_MEM_STATS_MTYPE		"typed_mem_stats"

static const struct structs_type typebuf_type
	= STRUCTS_FIXEDSTRING_TYPE(TYPED_MEM_TYPELEN);
static const struct structs_field typed_mem_typestats_fields[] = {
	STRUCTS_STRUCT_FIELD(typed_mem_typestats, type, &typebuf_type),
	STRUCTS_STRUCT_FIELD(typed_mem_typestats, allocs, &structs_type_uint),
	STRUCTS_STRUCT_FIELD(typed_mem_typestats, bytes, &structs_type_uint),
	STRUCTS_STRUCT_FIELD_END
};
static const struct structs_type typed_mem_typestats_type
	= STRUCTS_STRUCT_TYPE(typed_mem_typestats, typed_mem_typestats_fields);
const struct structs_type typed_mem_stats_type
	= STRUCTS_ARRAY_TYPE(&typed_mem_typestats_type,
	  TYPED_MEM_STATS_MTYPE, "type");

/* We need to use the real functions in here */
#undef malloc
#undef calloc
#undef realloc
#undef reallocf
#undef free
#undef asprintf
#undef vasprintf

/* How to print out problems */
#define WHINE(fmt, args...)	(void)fprintf(stderr, fmt "\n" , ## args)

/*
 * Kernel glue
 */
#ifdef _KERNEL

#if TYPED_MEM_TRACE
#error TYPED_MEM_TRACE not supported in kernel mode
#endif

#define malloc(x)       kern_malloc(x)
#define realloc(x, y)   kern_realloc(x, y)
#define free(x)         kern_free(x)

#define pthread_mutex_lock()	0
#define pthread_mutex_unlock()	0

#undef WHINE
#define WHINE(fmt, args...)	log(LOG_CRIT, fmt "\n" , ## args)

#endif	/* _KERNEL */

/*
 * Internal variables
 */
static u_char	typed_mem_started;
static u_char	typed_mem_enabled;
static struct	gtree *mem_tree;
static struct	gtree *type_tree;
static u_int	tree_node_size;

#ifndef _KERNEL
static pthread_mutex_t	typed_mem_mutex;
#endif

/* Guard bytes. Should have an "aligned" length. */
static const u_char	typed_mem_guard_data[] = { 0x34, 0x8e, 0x71, 0x9f };
static u_char		typed_mem_guard[ALIGNMENT];

/*
 * Internal functions
 */
static gtree_cmp_t	type_cmp;
static gtree_print_t	type_print;
static gtree_cmp_t	mem_cmp;

static struct		mem_info *typed_mem_find(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	void *mem, const char *typename, const char *func);

#ifndef _KERNEL
static gtree_print_t	mem_print;
#else
#define mem_print	NULL
#endif

/*
 * Enable typed memory.
 */
int
typed_mem_enable(void)
{
	pthread_mutexattr_t mattr;
	int i;

	/* Already enabled? */
	if (typed_mem_enabled)
		return (0);

	/* Already started allocating memory? */
	if (typed_mem_started) {
		errno = EALREADY;
		return (-1);
	}

	/* Initialize recursive mutex */
	if ((errno = pthread_mutexattr_init(&mattr)) != 0)
		return (-1);
	if ((errno = pthread_mutexattr_settype(&mattr,
	    PTHREAD_MUTEX_RECURSIVE)) != 0) {
		pthread_mutexattr_destroy(&mattr);
		return (-1);
	}
	if ((errno = pthread_mutex_init(&typed_mem_mutex, &mattr)) != 0) {
		pthread_mutexattr_destroy(&mattr);
		return (-1);
	}
	pthread_mutexattr_destroy(&mattr);

	/* Fill in guard bytes */
	for (i = 0; i < ALIGNMENT; i++)
		typed_mem_guard[i] = typed_mem_guard_data[i % ALIGNMENT];

	/* Done */
	typed_mem_enabled = 1;
	return (0);
}

/*
 * realloc(3) replacement
 */
void *
typed_mem_realloc(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *typename, void *mem, size_t size)
{
#if TYPED_MEM_TRACE
	void *const omem = mem;
#endif
	struct mem_info *info;
	void *rtn = NULL;
	int errno_save;
	int removed;
	int r;

	/* Check if typed memory is active */
	typed_mem_started = 1;
	if (!typed_mem_enabled || typename == NULL)
		return (realloc(mem, size));

	/* Lock info */
	r = pthread_mutex_lock(&typed_mem_mutex);
	assert(r == 0);

	/* Initialize first */
	if (mem_tree == NULL) {
		if ((mem_tree = gtree_create(NULL, NULL,
		    mem_cmp, NULL, NULL, mem_print)) == NULL)
			goto done;
		if ((type_tree = gtree_create(NULL, NULL,
		    type_cmp, NULL, NULL, type_print)) == NULL) {
			gtree_destroy(&mem_tree);
			goto done;
		}
		tree_node_size = gtree_node_size();
	}

	/* Find/create the memory descriptor block */
	if (mem == NULL) {
		struct mem_type *type;
		struct mem_type tkey;

		/* Get memory and new descriptor block */
		if ((info = malloc(sizeof(*info))) == NULL)
			goto done;
		if ((info->mem = malloc(size
		    + 2 * sizeof(typed_mem_guard))) == NULL) {
			errno_save = errno;
			free(info);
			errno = errno_save;
			goto done;
		}
		info->size = size;

		/* Find/create type descriptor for this memory type */
		strncpy(tkey.name, typename, sizeof(tkey.name) - 1);
		tkey.name[sizeof(tkey.name) - 1] = '\0';
		if ((type = gtree_get(type_tree, &tkey)) == NULL) {
			if ((type = malloc(sizeof(*type))) == NULL) {
				errno_save = errno;
				free(info->mem);
				free(info);
				errno = errno_save;
				goto done;
			}
			strcpy(type->name, tkey.name);
			type->count = 0;
			type->total = 0;
			if (gtree_put(type_tree, type) == -1) {
				errno_save = errno;
				free(type);
				free(info->mem);
				free(info);
				errno = errno_save;
				goto done;
			}
		}
		info->type = type;

		/* Add block descriptor to mem tree */
		if (gtree_put(mem_tree, info) == -1) {
			errno_save = errno;
			if (type->count == 0) {		/* was a new type */
				removed = gtree_remove(type_tree, type);
				assert(removed);
				free(type);
			}
			free(info->mem);
			free(info);
			errno = errno_save;
			goto done;
		}
	} else {
		void *node;

		/* Find memory descriptor */
#if TYPED_MEM_TRACE
		info = typed_mem_find(file, line, mem, typename, "REALLOC");
#else
		info = typed_mem_find(mem, typename, "REALLOC");
#endif

		/* Pre-allocate mem tree node */
		if ((node = malloc(tree_node_size)) == NULL)
			goto done;

		/* Get reallocated memory block */
		if ((mem = realloc(info->mem,
		    size + 2 * sizeof(typed_mem_guard))) == NULL) {
			errno_save = errno;
			free(node);
			errno = errno_save;
			goto done;
		}

		/* Subtract old block from type stats */
		info->type->total -= info->size;
		info->type->count--;

		/* Update block descriptor */
		info->size = size;
		if (info->mem != mem) {
			removed = gtree_remove(mem_tree, info);
			assert(removed);
			info->mem = mem;
			gtree_put_prealloc(mem_tree, info, node);
		} else
			free(node);		/* we didn't need it */
	}

	/* Add new block to type stats */
	info->type->total += info->size;
	info->type->count++;

	/* Install guard bytes */
	memcpy(info->mem, typed_mem_guard, sizeof(typed_mem_guard));
	memcpy((u_char *)info->mem + sizeof(typed_mem_guard) + info->size,
	    typed_mem_guard, sizeof(typed_mem_guard));

#if TYPED_MEM_TRACE
	/* Tracing */
	{
		const char *basename;

		if ((basename = strrchr(file, '/')) != NULL)
			basename++;
		else
			basename = file;
		fprintf(stderr, "%s:%u:ALLOC %p -> %p \"%s\" %u [%u]\n",
		    basename, line, omem, (u_char *)info->mem
		    + sizeof(typed_mem_guard), typename, size,
		    info->type->count);
	}
#endif

	/* Return memory block */
	rtn = (u_char *)info->mem + sizeof(typed_mem_guard);

done:
	/* Done */
	r = pthread_mutex_unlock(&typed_mem_mutex);
	assert(r == 0);
	return (rtn);
}

/*
 * reallocf(3) replacement
 */
void *
typed_mem_reallocf(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *typename, void *mem, size_t size)
{
	void *p;

#if TYPED_MEM_TRACE
	if ((p = typed_mem_realloc(file, line, typename, mem, size)) == NULL)
		typed_mem_free(file, line, typename, mem);
#else
	if ((p = typed_mem_realloc(typename, mem, size)) == NULL)
		typed_mem_free(typename, mem);
#endif
	return (p);
}

/*
 * free(3) replacement
 *
 * Note: it is OK if "typedname" points to memory within the region
 * pointed to by "mem".
 */
void
typed_mem_free(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *typename, void *mem)
{
	const int errno_save = errno;
	struct mem_info *info;
	int removed;
	int r;

	/* free(NULL) does nothing */
	if (mem == NULL)
		return;

	/* Check if typed memory is active */
	typed_mem_started = 1;
	if (!typed_mem_enabled || typename == NULL) {
		free(mem);
		return;
	}

	/* Lock info */
	r = pthread_mutex_lock(&typed_mem_mutex);
	assert(r == 0);

	/* Find memory descriptor */
#if TYPED_MEM_TRACE
	info = typed_mem_find(file, line, mem, typename, "FREE");
#else
	info = typed_mem_find(mem, typename, "FREE");
#endif

#if TYPED_MEM_TRACE
	/* Tracing */
	{
		const char *basename;

		if ((basename = strrchr(file, '/')) != NULL)
			basename++;
		else
			basename = file;
		fprintf(stderr, "%s:%u:FREE %p -> %p \"%s\" [%u]\n",
		    basename, line, mem, (void *)0,
		    typename, info->type->count);
	}
#endif

	/* Subtract block from descriptor info */
	info->type->total -= info->size;
	if (--info->type->count == 0) {
		assert(info->type->total == 0);
		removed = gtree_remove(type_tree, info->type);
		assert(removed);
		free(info->type);
	}

	/* Free memory and descriptor block */
	removed = gtree_remove(mem_tree, info);
	assert(removed);
	free(info->mem);
	free(info);

	/* Done */
	r = pthread_mutex_unlock(&typed_mem_mutex);
	assert(r == 0);
	errno = errno_save;
}

/*
 * calloc(3) replacement
 */
void *
typed_mem_calloc(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *type, size_t num, size_t size)
{
	void *mem;

	size *= num;
#if TYPED_MEM_TRACE
	if ((mem = typed_mem_realloc(file, line, type, NULL, size)) == NULL)
		return (NULL);
#else
	if ((mem = typed_mem_realloc(type, NULL, size)) == NULL)
		return (NULL);
#endif
	memset(mem, 0, size);
	return (mem);
}

/*
 * strdup(3) replacement
 */
char *
typed_mem_strdup(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *type, const char *string)
{
	const int slen = strlen(string) + 1;
	char *result;

#if TYPED_MEM_TRACE
	if ((result = typed_mem_realloc(file, line, type, NULL, slen)) == NULL)
		return (NULL);
#else
	if ((result = typed_mem_realloc(type, NULL, slen)) == NULL)
		return (NULL);
#endif
	memcpy(result, string, slen);
	return (result);
}

/*
 * asprintf(3) replacement
 */
int
typed_mem_asprintf(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *type, char **ret, const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
#if TYPED_MEM_TRACE
	r = typed_mem_vasprintf(file, line, type, ret, fmt, args);
#else
	r = typed_mem_vasprintf(type, ret, fmt, args);
#endif
	va_end(args);
	return (r);
}

/*
 * vasprintf(3) replacement
 */
int
typed_mem_vasprintf(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	const char *type, char **ret, const char *fmt, va_list args)
{
	int errno_save;
	char *s;
	int r;

	if ((r = vasprintf(ret, fmt, args)) == -1)
		return (-1);
	s = *ret;
#if TYPED_MEM_TRACE
	*ret = typed_mem_strdup(file, line, type, s);
#else
	*ret = typed_mem_strdup(type, s);
#endif
	errno_save = errno;
	free(s);
	if (*ret == NULL) {
		errno = errno_save;
		return (-1);
	}
	return (r);
}

/*
 * Get type for a memory block.
 */
char *
typed_mem_type(const void *mem, char *typebuf)
{
	struct mem_info *info = NULL;
	struct mem_info ikey;
	int r;

	/* Are we enabled? */
	if (!typed_mem_enabled) {
		errno = ENXIO;
		return (NULL);
	}

	/* Lock info */
	r = pthread_mutex_lock(&typed_mem_mutex);
	assert(r == 0);

	/* Find memory descriptor */
	ikey.mem = (u_char *)mem - sizeof(typed_mem_guard);
	if (mem_tree == NULL || (info = gtree_get(mem_tree, &ikey)) == NULL) {
		errno = ENOENT;
		typebuf = NULL;
		goto done;
	}

	/* Copy type */
	strlcpy(typebuf, info->type->name, TYPED_MEM_TYPELEN);

done:
	/* Unlock info */
	r = pthread_mutex_unlock(&typed_mem_mutex);
	assert(r == 0);

	/* Done */
	return (typebuf);
}

/*
 * Return typed memory usage statistics. The caller must free the
 * array by calling "structs_free(&typed_mem_stats_type, NULL, stats)".
 *
 * Returns zero if successful, -1 (and sets errno) if not.
 * If typed memory is disabled, errno = ENXIO.
 */
int
typed_mem_usage(struct typed_mem_stats *stats)
{
	struct mem_type *type;
	int i;
	int r;

	/* Check if enabled */
	if (!typed_mem_enabled) {
		errno = ENXIO;
		return (-1);
	}

	/* Lock info */
	r = pthread_mutex_lock(&typed_mem_mutex);
	assert(r == 0);

	/* Allocate array */
	memset(stats, 0, sizeof(*stats));
	if ((stats->elems = typed_mem_realloc(
#if TYPED_MEM_TRACE
	    __FILE__, __LINE__,
#endif
	    TYPED_MEM_STATS_MTYPE, NULL, (gtree_size(type_tree) + 1)
	    * sizeof(*stats->elems))) == NULL) {
		r = pthread_mutex_unlock(&typed_mem_mutex);
		assert(r == 0);
		return (-1);
	}

	/* Copy type stats */
	for (i = 0, type = gtree_first(type_tree);
	    type != NULL; i++, type = gtree_next(type_tree, type)) {
		struct typed_mem_typestats *const elem = &stats->elems[i];

		strlcpy(elem->type, type->name, sizeof(elem->type));
		elem->allocs = type->count;
		elem->bytes = type->total;
	}
	stats->length = i;

	/* Unlock info */
	r = pthread_mutex_unlock(&typed_mem_mutex);
	assert(r == 0);

	/* Done */
	return (0);
}

#ifndef _KERNEL

/*
 * Return usage statistics in a malloc'd string of the specified type.
 */
void
typed_mem_dump(FILE *fp)
{
	u_long total_blocks = 0;
	u_long total_alloc = 0;
	struct mem_type *type;
	int r;

	/* Check if enabled */
	if (!typed_mem_enabled) {
		fprintf(fp, "Typed memory is not enabled.\n");
		return;
	}

	/* Print header */
	fprintf(fp, "   %-28s %10s %10s\n", "Type", "Count", "Total");
	fprintf(fp, "   %-28s %10s %10s\n", "----", "-----", "-----");

	/* Lock info */
	r = pthread_mutex_lock(&typed_mem_mutex);
	assert(r == 0);

	/* Print allocation types */
	for (type = gtree_first(type_tree);
	    type != NULL; type = gtree_next(type_tree, type)) {
		fprintf(fp, "   %-28s %10u %10lu\n",
		    type->name, (int)type->count, (u_long)type->total);
		total_blocks += type->count;
		total_alloc += type->total;
	}

	/* Print totals */
	fprintf(fp, "   %-28s %10s %10s\n", "", "-----", "-----");
	fprintf(fp, "   %-28s %10lu %10lu\n",
	    "Totals", total_blocks, total_alloc);

	/* Unlock info */
	r = pthread_mutex_unlock(&typed_mem_mutex);
	assert(r == 0);
}

#endif	/* !_KERNEL */

/*
 * Find memory block info descriptor and verify guard bytes
 *
 * This assumes the mutex is locked.
 */
static struct mem_info *
typed_mem_find(
#if TYPED_MEM_TRACE
	const char *file, u_int line,
#endif
	void *mem, const char *typename, const char *func)
{
	struct mem_info *info = NULL;
	struct mem_info ikey;

	/* Find memory descriptor which must already exist */
	ikey.mem = (u_char *)mem - sizeof(typed_mem_guard);
	if (mem_tree == NULL || (info = gtree_get(mem_tree, &ikey)) == NULL) {
#if TYPED_MEM_TRACE
		WHINE("%s:%u: %s() of unallocated block:"
		    " ptr=%p type=\"%s\"", file, line, func, mem, typename);
#else
		WHINE("%s() of unallocated block:"
		    " ptr=%p type=\"%s\"", func, mem, typename);
#endif
		assert(0);
	}

	/* Check type is the same */
	if (strncmp(info->type->name, typename, TYPED_MEM_TYPELEN - 1) != 0) {
#if TYPED_MEM_TRACE
		WHINE("%s:%u: %s() with wrong type:"
		    " ptr=%p \"%s\" != \"%s\"", file, line,
		    func, mem, typename, info->type->name);
#else
		WHINE("%s() with wrong type:"
		    " ptr=%p \"%s\" != \"%s\"", 
		    func, mem, typename, info->type->name);
#endif
		assert(0);
	}

	/* Check ref count */
	assert(info->type->count > 0);

	/* Check guard bytes */
	if (memcmp(info->mem, typed_mem_guard, sizeof(typed_mem_guard)) != 0) {
#if TYPED_MEM_TRACE
		WHINE("%s:%u: %s(): %s guard violated ptr=%p type=\"%s\"",
		    file, line, func, "head", mem, typename);
#else
		WHINE("%s(): %s guard violated ptr=%p type=\"%s\"",
		    func, "head", mem, typename);
#endif
		assert(0);
	}
	if (memcmp((u_char *)info->mem + sizeof(typed_mem_guard) + info->size,
	    typed_mem_guard, sizeof(typed_mem_guard)) != 0) {
#if TYPED_MEM_TRACE
		WHINE("%s:%u: %s(): %s guard violated ptr=%p type=\"%s\"",
		    file, line, func, "tail", mem, typename);
#else
		WHINE("%s(): %s guard violated ptr=%p type=\"%s\"",
		    func, "tail", mem, typename);
#endif
		assert(0);
	}

	/* Done */
	return (info);
}

/*
 * Sort type descriptors by type string.
 */
static int
type_cmp(struct gtree *g, const void *item1, const void *item2)
{
	const struct mem_type *const type1 = item1;
	const struct mem_type *const type2 = item2;

	return (strcmp(type1->name, type2->name));
}

/*
 * Print a type
 */
static const char *
type_print(struct gtree *g, const void *item)
{
	const struct mem_type *const type = item;

	return (type->name);
}

/*
 * Sort memory block descriptors by memory address.
 */
static int
mem_cmp(struct gtree *g, const void *item1, const void *item2)
{
	const struct mem_info *const info1 = item1;
	const struct mem_info *const info2 = item2;

	if (info1->mem < info2->mem)
		return (-1);
	if (info1->mem > info2->mem)
		return (1);
	return (0);
}

#ifndef _KERNEL

/*
 * Print a type
 */
static const char *
mem_print(struct gtree *g, const void *item)
{
	const struct mem_info *const info = item;
	static char buf[32];

	snprintf(buf, sizeof(buf), "%p", info->mem);
	return (buf);
}

#endif	/* !_KERNEL */

