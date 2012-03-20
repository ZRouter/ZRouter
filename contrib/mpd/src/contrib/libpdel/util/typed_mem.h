
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

#ifndef _PDEL_UTIL_TYPED_MEM_H_
#define _PDEL_UTIL_TYPED_MEM_H_

/*
 * Memory type to use for temporary memory that is to be free'd
 * before the calling function returns. Note: the type string
 * includes the calling function name, so the allocation type
 * and free type MUST be both specified within the same function.
 */
#ifdef __GNUC__
#define TYPED_MEM_TEMP		__FUNCTION__
#else
#define TYPED_MEM_TEMP		"TEMPORARY"
#endif

/*
 * Size of saved type buffer (including '\0' byte). Any characters in
 * the type name that don't fit in a buffer this size are ignored.
 */
#define TYPED_MEM_TYPELEN	24

/*
 * Define this to print all (de)allocations to stderr. Doing
 * so requires rebuilding the entire library and all user code.
 */
#define TYPED_MEM_TRACE		0

/* Statistics reporting structure */
struct typed_mem_typestats {
	char		type[TYPED_MEM_TYPELEN];	/* type string */
	u_int		allocs;				/* # blocks alloc'd */
	u_int		bytes;				/* # bytes alloc'd */
};

DEFINE_STRUCTS_ARRAY(typed_mem_stats, struct typed_mem_typestats);

/*
 * Typed memory wrapper macros
 */
#if TYPED_MEM_TRACE
#define MALLOC(type, size)					\
	typed_mem_realloc(__FILE__, __LINE__, type, NULL, size)
#define CALLOC(type, num, size)					\
	typed_mem_calloc(__FILE__, __LINE__, type, num, size)
#define REALLOC(type, mem, size)				\
	typed_mem_realloc(__FILE__, __LINE__, type, mem, size)
#define REALLOCF(type, mem, size)				\
	typed_mem_reallocf(__FILE__, __LINE__, type, mem, size)
#define STRDUP(type, string)					\
	typed_mem_strdup(__FILE__, __LINE__, type, string)
#define FREE(type, mem)						\
	typed_mem_free(__FILE__, __LINE__, type, mem)
#define ASPRINTF(type, r, f, args...)				\
	typed_mem_asprintf(__FILE__, __LINE__, type, r, f , ## args)
#define VASPRINTF(type, r, f, va)				\
	typed_mem_vasprintf(__FILE__, __LINE__, type, r, f, va)
#else
#define MALLOC(type, size)		typed_mem_realloc(type, NULL, size)
#define CALLOC(type, num, size)		typed_mem_calloc(type, num, size)
#define REALLOC(type, mem, size)	typed_mem_realloc(type, mem, size)
#define REALLOCF(type, mem, size)	typed_mem_reallocf(type, mem, size)
#define STRDUP(type, string)		typed_mem_strdup(type, string)
#define FREE(type, mem)			typed_mem_free(type, mem)
#define ASPRINTF(type, r, f, args...)	typed_mem_asprintf(type, r, f , ## args)
#define VASPRINTF(type, r, f, va)	typed_mem_vasprintf(type, r, f, va)
#endif

/*
 * If TYPED_MEM_UNDEFINE_ORIGINALS is defined, then the including file
 * wants us to prevent it from using any of the original libc functions.
 */
#ifdef TYPED_MEM_UNDEFINE_ORIGINALS
#undef malloc
#define malloc		%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef calloc
#define calloc		%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef realloc
#define realloc		%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef reallocf
#define reallocf	%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef free
#define free		%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef strdup
#define strdup		%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef asprintf
#define asprintf	%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#undef vasprintf
#define vasprintf	%%%_USE_TYPED_MEM_MACROS_INSTEAD_%%%
#endif

__BEGIN_DECLS

/*
 * Typed memory allocation and freeing routines
 */
#if TYPED_MEM_TRACE
extern void	*typed_mem_realloc(const char *file, u_int line,
			const char *type, void *mem, size_t size);
extern void	*typed_mem_reallocf(const char *file, u_int line,
			const char *type, void *mem, size_t size);
extern void	*typed_mem_calloc(const char *file, u_int line,
			const char *type, size_t num, size_t size);
extern char	*typed_mem_strdup(const char *file, u_int line,
			const char *type, const char *string);
extern void	typed_mem_free(const char *file, u_int line,
			const char *type, void *mem);
extern int	typed_mem_asprintf(const char *file, u_int line,
			const char *type, char **ret, const char *format, ...);
extern int	typed_mem_vasprintf(const char *file, u_int line,
			const char *type, char **ret,
			const char *format, va_list va);
#else
extern void	*typed_mem_realloc(const char *type, void *mem, size_t size);
extern void	*typed_mem_reallocf(const char *type, void *mem, size_t size);
extern void	*typed_mem_calloc(const char *type, size_t num, size_t size);
extern char	*typed_mem_strdup(const char *type, const char *string);
extern void	typed_mem_free(const char *type, void *mem);
extern int	typed_mem_asprintf(const char *type,
			char **ret, const char *format, ...);
extern int	typed_mem_vasprintf(const char *type, char **ret,
			const char *format, va_list va);
#endif

/* Typed memory must be enabled by calling this function before any ops */
extern int	typed_mem_enable(void);

/* Typed statistics routines */
extern char	*typed_mem_type(const void *mem, char *typebuf);
extern int	typed_mem_usage(struct typed_mem_stats *stats);
#ifndef _KERNEL
extern void	typed_mem_dump(FILE *fp);
#endif

/* Structs type for 'struct typed_mem_stats' */
extern const	struct structs_type typed_mem_stats_type;

__END_DECLS

#endif	/* _PDEL_UTIL_TYPED_MEM_H_ */

