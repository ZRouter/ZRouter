
/*
 * mbuf.c
 *
 * Written by Toshiharu OHNO <tony-o@iij.ad.jp>
 * Copyright (c) 1993, Internet Initiative Japan, Inc. All rights reserved.
 * See ``COPYRIGHT.iij''
 * 
 * Rewritten by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"

/*
 * Malloc()
 *
 * Replacement for the usual malloc()
 */

void *
Malloc(const char *type, size_t size)
{
    const char	**memory;

    if ((memory = MALLOC(type, sizeof(char *) + size)) == NULL) {
	Perror("Malloc: malloc");
	DoExit(EX_ERRDEAD);
    }

    memory[0] = type;
    bzero(memory + 1, size);
    return (memory + 1);
}

/*
 * Mdup()
 *
 * Malloc() + memcpy()
 */

void *
Mdup(const char *type, const void *src, size_t size)
{
    const char	**memory;
    if ((memory = MALLOC(type, sizeof(char *) + size)) == NULL) {
	Perror("Mdup: malloc");
	DoExit(EX_ERRDEAD);
    }

    memory[0] = type;
    memcpy(memory + 1, src, size);
    return(memory + 1);
}

void *
Mstrdup(const char *type, const void *src)
{
    return (Mdup(type, src, strlen(src) + 1));
}

/*
 * Freee()
 *
 * Replacement for the ususal free()
 */

void
Freee(void *ptr)
{
    if (ptr) {
	char	**memory = ptr;
	memory--;
	FREE(memory[0], memory);
    }
}

/*
 * mballoc()
 *
 * Allocate an mbuf with memory
 */

Mbuf
mballoc(int size)
{
    u_char	*memory;
    int		amount, osize;
    Mbuf	bp;

    assert(size >= 0);

    if (size == 0) {
	osize = 64 - sizeof(*bp);
    } else if (size < 512)
	osize = ((size - 1) / 32 + 1) * 64 - sizeof(*bp);
    else
	osize = ((size - 1) / 64 + 1) * 64 + 512 - sizeof(*bp);
    amount = sizeof(*bp) + osize;

    if ((memory = MALLOC(MB_MBUF, amount)) == NULL) {
	Perror("mballoc: malloc");
	DoExit(EX_ERRDEAD);
    }

    /* Put mbuf at front of memory region */
    bp = (Mbuf)(void *)memory;
    bp->size = osize;
    bp->offset = (osize - size) / 2;
    bp->cnt = 0;

    return (bp);
}

/*
 * mbfree()
 *
 * Free head of chain, return next
 */

void
mbfree(Mbuf bp)
{
    if (bp)
	FREE(MB_MBUF, bp);
}

/*
 * mbread()
 *
 * Read contents of an mbuf chain into buffer, consuming len bytes.
 * If all of the chain is consumed, return NULL.
 *
 * This should ALWAYS be called like this:
 *	bp = mbread(bp, ... );
 */

Mbuf
mbread(Mbuf bp, void *buf, int cnt)
{
    int nread;

    assert(cnt >= 0);

    if (!bp)
	return (NULL);
    if (cnt > bp->cnt)
	nread = bp->cnt;
    else
        nread = cnt;
    memcpy(buf, MBDATAU(bp), nread);
    bp->offset += nread;
    bp->cnt -= nread;
    if (bp->cnt == 0) {
    	mbfree(bp);
	return (NULL);
    }
    return(bp);
}

/*
 * mbcopy()
 *
 * Copy contents of an mbuf chain into buffer, up to "cnt" bytes.
 * This does not consume any of the mbuf chain. Returns number copied.
 */

int
mbcopy(Mbuf bp, int offset, void *buf, int cnt)
{
    int nread;

    assert(offset >= 0);
    assert(cnt >= 0);

    if (!bp)
	return (0);
    if (offset >= bp->cnt)
	return (0);

    if (cnt > bp->cnt - offset)
	nread = bp->cnt - offset;
    else
        nread = cnt;
    memcpy(buf, MBDATAU(bp) + offset, nread);
    return (nread);
}

/*
 * mbcopyback()
 *
 * Write bytes from buffer into an mbuf chain. Returns first argument.
 */

Mbuf
mbcopyback(Mbuf bp, int offset, const void *buf, int cnt)
{
    int		b, e;

    if (!bp) {
	if (offset < 0)
	    offset = 0;
	bp = mballoc(offset + cnt);
	memcpy(MBDATAU(bp) + offset, buf, cnt);
	bp->cnt = offset + cnt;
	return (bp);
    }

    b = (offset > 0) ? 0 : -offset;
    e = (offset + cnt > bp->cnt) ? offset + cnt - bp->cnt : 0;
    
    if (b + bp->cnt + e > bp->size) {
	Mbuf	nbp = mballoc(b + bp->cnt + e);
	memcpy(MBDATAU(nbp) + b, MBDATAU(bp), bp->cnt);
	nbp->cnt = bp->cnt;
	mbfree(bp);
	bp = nbp;
    } else if ((b > bp->offset) || (bp->offset + bp->cnt + e > bp->size)) {
	int	noff = (bp->size - (b + bp->cnt + e)) / 2;
	memmove(MBDATAU(bp) - bp->offset + noff + b, MBDATAU(bp), bp->cnt);
	bp->offset = noff;
    } else {
	bp->offset -= b;
    }
    bp->cnt = b + bp->cnt + e;
    memcpy(MBDATAU(bp) + offset + b, buf, cnt);
    return(bp);
}

/*
 * mbtrunc()
 *
 * Truncate mbuf to total of "max" bytes. If max is zero
 * then a zero length mbuf is returned (rather than a NULL mbuf).
 */

Mbuf
mbtrunc(Mbuf bp, int max)
{
    assert(max >= 0);

    if (!bp)
	return (NULL);

    if (bp->cnt > max)
	bp->cnt = max;

    return (bp);
}

/*
 * mbadj()
 *
 * Truncate mbuf cutting "cnt" bytes from begin or end.
 */

Mbuf
mbadj(Mbuf bp, int cnt)
{
    if (!bp)
	return (NULL);

    if (cnt >= 0) {
	if (bp->cnt > cnt) {
	    bp->cnt -= cnt;
	    bp->offset += cnt;
	} else {
	    bp->cnt = 0;
	}
    } else {
	if (bp->cnt > -cnt) {
	    bp->cnt -= -cnt;
	} else {
	    bp->cnt = 0;
	}
    }

    return (bp);
}

/*
 * mbsplit()
 *
 * Break an mbuf chain after "cnt" bytes.
 * Return the newly created mbuf chain that
 * starts after "cnt" bytes. If MBLEN(bp) <= cnt,
 * then returns NULL.  The first part of
 * the chain remains pointed to by "bp".
 */

Mbuf
mbsplit(Mbuf bp, int cnt)
{
    Mbuf	nbp;
    
    assert(cnt >= 0);

    if (!bp)
	return (NULL);

    if (MBLEN(bp) <= cnt)
	return (NULL);

    nbp = mballoc(bp->cnt - cnt);
    memcpy(MBDATAU(nbp), MBDATAU(bp) + cnt, bp->cnt - cnt);
    nbp->cnt = bp->cnt - cnt;
    bp->cnt = cnt;

    return(nbp);
}

/*
 * MemStat()
 */

int
MemStat(Context ctx, int ac, char *av[], void *arg)
{
    struct typed_mem_stats stats;
    u_int	i;
    u_int	total_allocs = 0;
    u_int	total_bytes = 0;

    if (typed_mem_usage(&stats))
	Error("typed_mem_usage() error");
    
    /* Print header */
    Printf("   %-28s %10s %10s\r\n", "Type", "Count", "Total");
    Printf("   %-28s %10s %10s\r\n", "----", "-----", "-----");

    for (i = 0; i < stats.length; i++) {
	struct typed_mem_typestats *type = &stats.elems[i];

	Printf("   %-28s %10u %10lu\r\n",
	    type->type, (int)type->allocs, (u_long)type->bytes);
	total_allocs += type->allocs;
	total_bytes += type->bytes;
    }
    /* Print totals */
    Printf("   %-28s %10s %10s\r\n", "", "-----", "-----");
    Printf("   %-28s %10lu %10lu\r\n",
        "Totals", total_allocs, total_bytes);

    structs_free(&typed_mem_stats_type, NULL, &stats);
    return(0);
}

