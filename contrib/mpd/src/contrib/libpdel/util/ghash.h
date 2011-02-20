
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

#ifndef _PDEL_UTIL_GHASH_H_
#define _PDEL_UTIL_GHASH_H_

/*
 * General purpose hash table stuff.
 */

/**********************************************************************
			HASH TABLE FUNCTION TYPES
**********************************************************************/

struct ghash;
struct ghash_walk;

/*
 * How to compare two items for equality.
 *
 * If this function is not specified, then "item1 == item2" is used.
 */
typedef int	ghash_equal_t(struct ghash *g,
			const void *item1, const void *item2);

/*
 * How to compute the hash value for an item.
 *
 * If this function is not specified, then "(u_int32_t)item" is used.
 */
typedef u_int32_t ghash_hash_t(struct ghash *g, const void *item);

/*
 * Notification that an item is being added to the table.
 *
 * Supplying this function is optional.
 */
typedef void	ghash_add_t(struct ghash *g, void *item);

/*
 * Notification that an item is being removed from the table.
 *
 * Supplying this function is optional.
 */
typedef void	ghash_del_t(struct ghash *g, void *item);

/**********************************************************************
			HASH TABLE METHODS
**********************************************************************/

__BEGIN_DECLS

/*
 * Create a new hash table.
 *
 * "isize" is the initial table size, or zero for the default.
 *
 * "maxload" is the maximum hash table load in percent, or zero
 * for the default which is 75 (i.e., 75%).
 *
 * The "hash", "equal", "add", and "del" methods are optional (see above).
 */
extern struct	ghash *ghash_create(void *arg, u_int isize, u_int maxload,
			const char *mtype, ghash_hash_t *hash,
			ghash_equal_t *equal, ghash_add_t *add,
			ghash_del_t *del);

/*
 * Destroy a hash table.
 *
 * Any items remaining in the table will be removed first.
 */
extern void	ghash_destroy(struct ghash **gp);

/*
 * Get the argument supplied to ghash_create().
 */
extern void	*ghash_arg(struct ghash *g);

/*
 * Get an item.
 *
 * Returns the item, or NULL if the item does not exist.
 */
extern void	*ghash_get(struct ghash *g, const void *item);

/*
 * Put an item.
 *
 * Returns 0 if the item is new, 1 if it replaces an existing
 * item, and -1 if there was an error.
 */
extern int	ghash_put(struct ghash *g, const void *item);

/*
 * Remove an item.
 *
 * Returns 1 if the item was found and removed, 0 if not found.
 */
extern int	ghash_remove(struct ghash *g, const void *item);

/*
 * Get the size of the table.
 */
extern u_int	ghash_size(struct ghash *g);

/*
 * Get an array of all items in the table.
 *
 * Returns number of items in the list, or -1 if error.
 * Caller must free the list.
 */
extern int	ghash_dump(struct ghash *g, void ***listp, const char *mtype);

/*
 * Start a hash table walk. Caller must supply a pointer to a
 * 'struct ghash_walk' (see below) which is used to store the
 * position in the table for ghash_walk_next().
 */
extern void	ghash_walk_init(struct ghash *g, struct ghash_walk *walk);

/*
 * Get the next item in the hash table walk, or NULL if the walk
 * is finished (ENOENT) or the table has been modified (EINVAL).
 *
 * This is only valid if the hash table has not been modified
 * since the previous call to ghash_walk_init() or ghash_walk_next().
 */
extern void	*ghash_walk_next(struct ghash *g, struct ghash_walk *walk);

__END_DECLS

struct ghash_walk {
	u_int		mods;
	u_int		bucket;
	struct gent	*e;
};

/**********************************************************************
			ITERATOR METHODS
**********************************************************************/

struct ghash_iter;

__BEGIN_DECLS

extern int	ghash_iter_has_next(struct ghash_iter *iter);
extern void	*ghash_iter_next(struct ghash_iter *iter);
extern int	ghash_iter_remove(struct ghash_iter *iter);

extern struct	ghash_iter *ghash_iter_create(struct ghash *g);
extern void	ghash_iter_destroy(struct ghash_iter **iterp);

__END_DECLS

#endif	/* _PDEL_UTIL_GHASH_H_ */

