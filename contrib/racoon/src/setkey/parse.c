
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 34 "parse.y"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/pfkeyv2.h>
#include PATH_IPSEC_H
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "libpfkey.h"
#include "vchar.h"
#include "extern.h"

#ifndef IPPROTO_MH
#define IPPROTO_MH		135
#endif

#define DEFAULT_NATT_PORT	4500

#ifndef UDP_ENCAP_ESPINUDP
#define UDP_ENCAP_ESPINUDP	2
#endif

#define ATOX(c) \
  (isdigit((int)c) ? (c - '0') : \
    (isupper((int)c) ? (c - 'A' + 10) : (c - 'a' + 10)))

u_int32_t p_spi;
u_int p_ext, p_alg_enc, p_alg_auth, p_replay, p_mode;
u_int32_t p_reqid;
u_int p_key_enc_len, p_key_auth_len;
const char *p_key_enc;
const char *p_key_auth;
time_t p_lt_hard, p_lt_soft;
size_t p_lb_hard, p_lb_soft;

struct security_ctx {
	u_int8_t doi;
	u_int8_t alg;
	u_int16_t len;
	char *buf;
};

struct security_ctx sec_ctx;

static u_int p_natt_type;
static struct addrinfo * p_natt_oa = NULL;

static int p_aiflags = 0, p_aifamily = PF_UNSPEC;

static struct addrinfo *parse_addr __P((char *, char *));
static int fix_portstr __P((vchar_t *, vchar_t *, vchar_t *));
static int setvarbuf __P((char *, int *, struct sadb_ext *, int, 
    const void *, int));
void parse_init __P((void));
void free_buffer __P((void));

int setkeymsg0 __P((struct sadb_msg *, unsigned int, unsigned int, size_t));
static int setkeymsg_spdaddr __P((unsigned int, unsigned int, vchar_t *,
	struct addrinfo *, int, struct addrinfo *, int));
static int setkeymsg_spdaddr_tag __P((unsigned int, char *, vchar_t *));
static int setkeymsg_addr __P((unsigned int, unsigned int,
	struct addrinfo *, struct addrinfo *, int));
static int setkeymsg_add __P((unsigned int, unsigned int,
	struct addrinfo *, struct addrinfo *));


/* Line 189 of yacc.c  */
#line 154 "parse.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EOT = 258,
     SLASH = 259,
     BLCL = 260,
     ELCL = 261,
     ADD = 262,
     GET = 263,
     DELETE = 264,
     DELETEALL = 265,
     FLUSH = 266,
     DUMP = 267,
     EXIT = 268,
     PR_ESP = 269,
     PR_AH = 270,
     PR_IPCOMP = 271,
     PR_ESPUDP = 272,
     PR_TCP = 273,
     F_PROTOCOL = 274,
     F_AUTH = 275,
     F_ENC = 276,
     F_REPLAY = 277,
     F_COMP = 278,
     F_RAWCPI = 279,
     F_MODE = 280,
     MODE = 281,
     F_REQID = 282,
     F_EXT = 283,
     EXTENSION = 284,
     NOCYCLICSEQ = 285,
     ALG_AUTH = 286,
     ALG_AUTH_NOKEY = 287,
     ALG_ENC = 288,
     ALG_ENC_NOKEY = 289,
     ALG_ENC_DESDERIV = 290,
     ALG_ENC_DES32IV = 291,
     ALG_ENC_OLD = 292,
     ALG_COMP = 293,
     F_LIFETIME_HARD = 294,
     F_LIFETIME_SOFT = 295,
     F_LIFEBYTE_HARD = 296,
     F_LIFEBYTE_SOFT = 297,
     DECSTRING = 298,
     QUOTEDSTRING = 299,
     HEXSTRING = 300,
     STRING = 301,
     ANY = 302,
     SPDADD = 303,
     SPDDELETE = 304,
     SPDDUMP = 305,
     SPDFLUSH = 306,
     F_POLICY = 307,
     PL_REQUESTS = 308,
     F_AIFLAGS = 309,
     TAGGED = 310,
     SECURITY_CTX = 311
   };
#endif
/* Tokens.  */
#define EOT 258
#define SLASH 259
#define BLCL 260
#define ELCL 261
#define ADD 262
#define GET 263
#define DELETE 264
#define DELETEALL 265
#define FLUSH 266
#define DUMP 267
#define EXIT 268
#define PR_ESP 269
#define PR_AH 270
#define PR_IPCOMP 271
#define PR_ESPUDP 272
#define PR_TCP 273
#define F_PROTOCOL 274
#define F_AUTH 275
#define F_ENC 276
#define F_REPLAY 277
#define F_COMP 278
#define F_RAWCPI 279
#define F_MODE 280
#define MODE 281
#define F_REQID 282
#define F_EXT 283
#define EXTENSION 284
#define NOCYCLICSEQ 285
#define ALG_AUTH 286
#define ALG_AUTH_NOKEY 287
#define ALG_ENC 288
#define ALG_ENC_NOKEY 289
#define ALG_ENC_DESDERIV 290
#define ALG_ENC_DES32IV 291
#define ALG_ENC_OLD 292
#define ALG_COMP 293
#define F_LIFETIME_HARD 294
#define F_LIFETIME_SOFT 295
#define F_LIFEBYTE_HARD 296
#define F_LIFEBYTE_SOFT 297
#define DECSTRING 298
#define QUOTEDSTRING 299
#define HEXSTRING 300
#define STRING 301
#define ANY 302
#define SPDADD 303
#define SPDDELETE 304
#define SPDDUMP 305
#define SPDFLUSH 306
#define F_POLICY 307
#define PL_REQUESTS 308
#define F_AIFLAGS 309
#define TAGGED 310
#define SECURITY_CTX 311




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 114 "parse.y"

	int num;
	unsigned long ulnum;
	vchar_t val;
	struct addrinfo *res;



/* Line 214 of yacc.c  */
#line 311 "parse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 323 "parse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   157

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNRULES -- Number of states.  */
#define YYNSTATES  170

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   311

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    39,    48,    55,    64,
      68,    72,    73,    75,    77,    79,    81,    84,    86,    88,
      90,    92,    94,    96,   101,   104,   107,   110,   114,   116,
     119,   121,   124,   127,   130,   132,   134,   136,   137,   140,
     143,   146,   149,   152,   155,   158,   161,   164,   167,   170,
     175,   189,   195,   209,   212,   215,   216,   219,   221,   223,
     225,   228,   229,   232,   233,   237,   241,   245,   247,   249,
     251,   253,   254,   256,   257,   262,   265,   267
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      58,     0,    -1,    -1,    58,    59,    -1,    60,    -1,    63,
      -1,    61,    -1,    62,    -1,    64,    -1,    65,    -1,    92,
      -1,    77,    -1,    78,    -1,    79,    -1,    80,    -1,     7,
      81,    84,    84,    66,    67,    75,    68,     3,    -1,     9,
      81,    84,    84,    66,    67,    75,     3,    -1,    10,    81,
      83,    83,    66,     3,    -1,     8,    81,    84,    84,    66,
      67,    75,     3,    -1,    11,    66,     3,    -1,    12,    66,
       3,    -1,    -1,    14,    -1,    15,    -1,    16,    -1,    17,
      -1,    17,    83,    -1,    18,    -1,    43,    -1,    45,    -1,
      69,    -1,    70,    -1,    71,    -1,    21,    72,    20,    73,
      -1,    21,    72,    -1,    20,    73,    -1,    23,    38,    -1,
      23,    38,    24,    -1,    34,    -1,    33,    74,    -1,    37,
      -1,    35,    74,    -1,    36,    74,    -1,    31,    74,    -1,
      32,    -1,    44,    -1,    45,    -1,    -1,    75,    76,    -1,
      28,    29,    -1,    28,    30,    -1,    25,    26,    -1,    25,
      47,    -1,    27,    43,    -1,    22,    43,    -1,    39,    43,
      -1,    40,    43,    -1,    41,    43,    -1,    42,    43,    -1,
      56,    43,    43,    44,    -1,    48,    81,    46,    85,    86,
      46,    85,    86,    87,    88,    89,    90,     3,    -1,    48,
      55,    44,    90,     3,    -1,    49,    81,    46,    85,    86,
      46,    85,    86,    87,    88,    89,    90,     3,    -1,    50,
       3,    -1,    51,     3,    -1,    -1,    81,    82,    -1,    54,
      -1,    46,    -1,    46,    -1,    46,    86,    -1,    -1,     4,
      43,    -1,    -1,     5,    47,     6,    -1,     5,    43,     6,
      -1,     5,    46,     6,    -1,    43,    -1,    47,    -1,    18,
      -1,    46,    -1,    -1,    46,    -1,    -1,    56,    43,    43,
      44,    -1,    52,    91,    -1,    53,    -1,    13,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   155,   155,   157,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   181,   193,   212,   224,   239,
     249,   259,   262,   270,   278,   282,   289,   296,   305,   306,
     327,   328,   329,   333,   334,   338,   342,   350,   362,   377,
     392,   408,   429,   453,   478,   491,   495,   524,   526,   530,
     531,   532,   533,   534,   535,   544,   545,   546,   547,   548,
     559,   599,   611,   650,   661,   670,   672,   676,   701,   712,
     720,   731,   732,   737,   745,   754,   765,   772,   773,   774,
     777,   800,   804,   815,   817,   826,   850,   855
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EOT", "SLASH", "BLCL", "ELCL", "ADD",
  "GET", "DELETE", "DELETEALL", "FLUSH", "DUMP", "EXIT", "PR_ESP", "PR_AH",
  "PR_IPCOMP", "PR_ESPUDP", "PR_TCP", "F_PROTOCOL", "F_AUTH", "F_ENC",
  "F_REPLAY", "F_COMP", "F_RAWCPI", "F_MODE", "MODE", "F_REQID", "F_EXT",
  "EXTENSION", "NOCYCLICSEQ", "ALG_AUTH", "ALG_AUTH_NOKEY", "ALG_ENC",
  "ALG_ENC_NOKEY", "ALG_ENC_DESDERIV", "ALG_ENC_DES32IV", "ALG_ENC_OLD",
  "ALG_COMP", "F_LIFETIME_HARD", "F_LIFETIME_SOFT", "F_LIFEBYTE_HARD",
  "F_LIFEBYTE_SOFT", "DECSTRING", "QUOTEDSTRING", "HEXSTRING", "STRING",
  "ANY", "SPDADD", "SPDDELETE", "SPDDUMP", "SPDFLUSH", "F_POLICY",
  "PL_REQUESTS", "F_AIFLAGS", "TAGGED", "SECURITY_CTX", "$accept",
  "commands", "command", "add_command", "delete_command",
  "deleteall_command", "get_command", "flush_command", "dump_command",
  "protocol_spec", "spi", "algorithm_spec", "esp_spec", "ah_spec",
  "ipcomp_spec", "enc_alg", "auth_alg", "key_string", "extension_spec",
  "extension", "spdadd_command", "spddelete_command", "spddump_command",
  "spdflush_command", "ipaddropts", "ipaddropt", "ipaddr", "ipandport",
  "prefix", "portstr", "upper_spec", "upper_misc_spec", "context_spec",
  "policy_spec", "policy_requests", "exit_command", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    58,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    60,    61,    62,    63,    64,
      65,    66,    66,    66,    66,    66,    66,    66,    67,    67,
      68,    68,    68,    69,    69,    70,    71,    71,    72,    72,
      72,    72,    72,    73,    73,    74,    74,    75,    75,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      77,    77,    78,    79,    80,    81,    81,    82,    83,    84,
      84,    85,    85,    86,    86,    86,    86,    87,    87,    87,
      87,    88,    88,    89,    89,    90,    91,    92
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     9,     8,     6,     8,     3,
       3,     0,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     4,     2,     2,     2,     3,     1,     2,
       1,     2,     2,     2,     1,     1,     1,     0,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     4,
      13,     5,    13,     2,     2,     0,     2,     1,     1,     1,
       2,     0,     2,     0,     3,     3,     3,     1,     1,     1,
       1,     0,     1,     0,     4,     2,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    65,    65,    65,    65,    21,    21,     0,
      65,    65,     0,     0,     3,     4,     6,     7,     5,     8,
       9,    11,    12,    13,    14,    10,     0,     0,     0,     0,
      22,    23,    24,    25,    27,     0,     0,    87,     0,     0,
       0,    63,    64,    69,    67,    66,     0,     0,     0,    68,
       0,    26,    19,    20,     0,    71,    71,     0,    70,    21,
      21,    21,    21,     0,     0,     0,    73,    73,     0,     0,
       0,     0,     0,     0,     0,    86,    85,    61,    72,     0,
       0,    75,    76,    74,    28,    29,    47,    47,    47,    17,
      71,    71,     0,     0,     0,    73,    73,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      30,    31,    32,    48,    18,    16,     0,     0,     0,    44,
      35,     0,    38,     0,     0,    40,    34,    54,    36,    51,
      52,    53,    49,    50,    55,    56,    57,    58,     0,    15,
      79,    77,    80,    78,    81,    81,    45,    46,    43,    39,
      41,    42,     0,    37,     0,    82,    83,    83,    33,    59,
       0,     0,     0,     0,     0,     0,     0,    60,    62,    84
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    35,
      86,   109,   110,   111,   112,   126,   120,   148,    92,   113,
      21,    22,    23,    24,    26,    45,    50,    46,    66,    58,
     144,   156,   161,    64,    76,    25
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -64
static const yytype_int16 yypact[] =
{
     -64,    56,   -64,   -64,   -64,   -64,   -64,    32,    32,    10,
     -32,   -64,    13,    17,   -64,   -64,   -64,   -64,   -64,   -64,
     -64,   -64,   -64,   -64,   -64,   -64,   -39,   -39,   -39,   -37,
     -64,   -64,   -64,   -12,   -64,    34,    73,   -64,    16,   -36,
     -35,   -64,   -64,    82,   -64,   -64,    44,    44,    44,   -64,
     -12,   -64,   -64,   -64,    26,   115,   115,    15,   -64,    32,
      32,    32,    32,    69,   122,    83,    82,    82,   121,   123,
     124,   -31,   -31,   -31,   125,   -64,   -64,   -64,   -64,    85,
      86,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,
     115,   115,    52,    -1,     3,    82,    82,    57,    64,    90,
      96,   -18,    92,    41,    93,    94,    95,    97,    98,   136,
     -64,   -64,   -64,   -64,   -64,   -64,    39,    39,    58,   -64,
     -64,    58,   -64,    58,    58,   -64,   126,   -64,   118,   -64,
     -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   100,   -64,
     -64,   -64,   -64,   -64,    99,    99,   -64,   -64,   -64,   -64,
     -64,   -64,    57,   -64,   103,   -64,    88,    88,   -64,   -64,
     105,    26,    26,   106,   147,   148,   108,   -64,   -64,   -64
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,   -64,    -8,
      37,   -64,   -64,   -64,   -64,   -64,     1,   -40,    33,   -64,
     -64,   -64,   -64,   -64,   107,   -64,   -28,    68,   -55,   -63,
      38,     9,     0,   -38,   -64,   -64
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      36,    67,   114,    79,    80,    51,   115,    43,   129,    49,
      55,    56,    84,    37,    85,    44,    41,    44,    44,    44,
      42,    99,    62,    38,   101,    99,   102,   103,   101,   130,
     102,   103,   116,   117,    49,    95,    96,    52,   104,   105,
     106,   107,   104,   105,   106,   107,    30,    31,    32,    33,
      34,    71,    72,    73,    74,   108,     2,   140,    68,   108,
      54,    69,    70,     3,     4,     5,     6,     7,     8,     9,
     132,   133,    97,    98,    99,   100,    53,   101,    63,   102,
     103,   149,   141,   150,   151,   142,   143,    57,   118,   119,
      43,   104,   105,   106,   107,    47,    48,   121,   122,   123,
     124,   125,   146,   147,    10,    11,    12,    13,   108,    87,
      88,    27,    28,    29,    59,    60,    61,    39,    40,    65,
      93,    94,    75,   164,   165,    77,    78,    81,    89,    82,
      83,    90,    91,   127,   128,   131,   134,   135,   136,   139,
     137,   138,   153,   154,   160,   155,   152,   159,   163,   166,
     167,   168,   169,   158,   157,   145,     0,   162
};

static const yytype_int16 yycheck[] =
{
       8,    56,     3,    66,    67,    33,     3,    46,    26,    46,
      46,    46,    43,     3,    45,    54,     3,    54,    54,    54,
       3,    22,    50,    55,    25,    22,    27,    28,    25,    47,
      27,    28,    95,    96,    46,    90,    91,     3,    39,    40,
      41,    42,    39,    40,    41,    42,    14,    15,    16,    17,
      18,    59,    60,    61,    62,    56,     0,    18,    43,    56,
      44,    46,    47,     7,     8,     9,    10,    11,    12,    13,
      29,    30,    20,    21,    22,    23,     3,    25,    52,    27,
      28,   121,    43,   123,   124,    46,    47,     5,    31,    32,
      46,    39,    40,    41,    42,    27,    28,    33,    34,    35,
      36,    37,    44,    45,    48,    49,    50,    51,    56,    72,
      73,     4,     5,     6,    46,    47,    48,    10,    11,     4,
      87,    88,    53,   161,   162,     3,    43,     6,     3,     6,
       6,    46,    46,    43,    38,    43,    43,    43,    43,     3,
      43,    43,    24,    43,    56,    46,    20,    44,    43,    43,
       3,     3,    44,   152,   145,   117,    -1,   157
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    58,     0,     7,     8,     9,    10,    11,    12,    13,
      48,    49,    50,    51,    59,    60,    61,    62,    63,    64,
      65,    77,    78,    79,    80,    92,    81,    81,    81,    81,
      14,    15,    16,    17,    18,    66,    66,     3,    55,    81,
      81,     3,     3,    46,    54,    82,    84,    84,    84,    46,
      83,    83,     3,     3,    44,    46,    46,     5,    86,    84,
      84,    84,    83,    52,    90,     4,    85,    85,    43,    46,
      47,    66,    66,    66,    66,    53,    91,     3,    43,    86,
      86,     6,     6,     6,    43,    45,    67,    67,    67,     3,
      46,    46,    75,    75,    75,    85,    85,    20,    21,    22,
      23,    25,    27,    28,    39,    40,    41,    42,    56,    68,
      69,    70,    71,    76,     3,     3,    86,    86,    31,    32,
      73,    33,    34,    35,    36,    37,    72,    43,    38,    26,
      47,    43,    29,    30,    43,    43,    43,    43,    43,     3,
      18,    43,    46,    47,    87,    87,    44,    45,    74,    74,
      74,    74,    20,    24,    43,    46,    88,    88,    73,    44,
      56,    89,    89,    43,    90,    90,    43,     3,     3,    44
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 158 "parse.y"
    {
			free_buffer();
			parse_init();
		}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 182 "parse.y"
    {
			int status;

			status = setkeymsg_add(SADB_ADD, (yyvsp[(5) - (9)].num), (yyvsp[(3) - (9)].res), (yyvsp[(4) - (9)].res));
			if (status < 0)
				return -1;
		}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 194 "parse.y"
    {
			int status;

			if ((yyvsp[(3) - (8)].res)->ai_next || (yyvsp[(4) - (8)].res)->ai_next) {
				yyerror("multiple address specified");
				return -1;
			}
			if (p_mode != IPSEC_MODE_ANY)
				yyerror("WARNING: mode is obsolete");

			status = setkeymsg_addr(SADB_DELETE, (yyvsp[(5) - (8)].num), (yyvsp[(3) - (8)].res), (yyvsp[(4) - (8)].res), 0);
			if (status < 0)
				return -1;
		}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 213 "parse.y"
    {
			int status;

			status = setkeymsg_addr(SADB_DELETE, (yyvsp[(5) - (6)].num), (yyvsp[(3) - (6)].res), (yyvsp[(4) - (6)].res), 1);
			if (status < 0)
				return -1;
		}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 225 "parse.y"
    {
			int status;

			if (p_mode != IPSEC_MODE_ANY)
				yyerror("WARNING: mode is obsolete");

			status = setkeymsg_addr(SADB_GET, (yyvsp[(5) - (8)].num), (yyvsp[(3) - (8)].res), (yyvsp[(4) - (8)].res), 0);
			if (status < 0)
				return -1;
		}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 240 "parse.y"
    {
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_FLUSH, (yyvsp[(2) - (3)].num), sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 250 "parse.y"
    {
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_DUMP, (yyvsp[(2) - (3)].num), sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 259 "parse.y"
    {
			(yyval.num) = SADB_SATYPE_UNSPEC;
		}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 263 "parse.y"
    {
			(yyval.num) = SADB_SATYPE_ESP;
			if ((yyvsp[(1) - (1)].num) == 1)
				p_ext |= SADB_X_EXT_OLD;
			else
				p_ext &= ~SADB_X_EXT_OLD;
		}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 271 "parse.y"
    {
			(yyval.num) = SADB_SATYPE_AH;
			if ((yyvsp[(1) - (1)].num) == 1)
				p_ext |= SADB_X_EXT_OLD;
			else
				p_ext &= ~SADB_X_EXT_OLD;
		}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 279 "parse.y"
    {
			(yyval.num) = SADB_X_SATYPE_IPCOMP;
		}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 283 "parse.y"
    {
			(yyval.num) = SADB_SATYPE_ESP;
			p_ext &= ~SADB_X_EXT_OLD;
			p_natt_oa = 0;
			p_natt_type = UDP_ENCAP_ESPINUDP;
		}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 290 "parse.y"
    {
			(yyval.num) = SADB_SATYPE_ESP;
			p_ext &= ~SADB_X_EXT_OLD;
			p_natt_oa = (yyvsp[(2) - (2)].res);
			p_natt_type = UDP_ENCAP_ESPINUDP;
		}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 297 "parse.y"
    {
#ifdef SADB_X_SATYPE_TCPSIGNATURE
			(yyval.num) = SADB_X_SATYPE_TCPSIGNATURE;
#endif
		}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 305 "parse.y"
    { p_spi = (yyvsp[(1) - (1)].ulnum); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 307 "parse.y"
    {
			char *ep;
			unsigned long v;

			ep = NULL;
			v = strtoul((yyvsp[(1) - (1)].val).buf, &ep, 16);
			if (!ep || *ep) {
				yyerror("invalid SPI");
				return -1;
			}
			if (v & ~0xffffffff) {
				yyerror("SPI too big.");
				return -1;
			}

			p_spi = v;
		}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 343 "parse.y"
    {
			if ((yyvsp[(2) - (2)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(2) - (2)].num);
		}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 351 "parse.y"
    {
			if ((yyvsp[(2) - (3)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(2) - (3)].num);
			p_ext |= SADB_X_EXT_RAWCPI;
		}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 362 "parse.y"
    {
			if ((yyvsp[(1) - (1)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(1) - (1)].num);

			p_key_enc_len = 0;
			p_key_enc = "";
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 377 "parse.y"
    {
			if ((yyvsp[(1) - (2)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(1) - (2)].num);

			p_key_enc_len = (yyvsp[(2) - (2)].val).len;
			p_key_enc = (yyvsp[(2) - (2)].val).buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 392 "parse.y"
    {
			if ((yyvsp[(1) - (1)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			yyerror("WARNING: obsolete algorithm");
			p_alg_enc = (yyvsp[(1) - (1)].num);

			p_key_enc_len = 0;
			p_key_enc = "";
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 409 "parse.y"
    {
			if ((yyvsp[(1) - (2)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(1) - (2)].num);
			if (p_ext & SADB_X_EXT_OLD) {
				yyerror("algorithm mismatched");
				return -1;
			}
			p_ext |= SADB_X_EXT_DERIV;

			p_key_enc_len = (yyvsp[(2) - (2)].val).len;
			p_key_enc = (yyvsp[(2) - (2)].val).buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 430 "parse.y"
    {
			if ((yyvsp[(1) - (2)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_enc = (yyvsp[(1) - (2)].num);
			if (!(p_ext & SADB_X_EXT_OLD)) {
				yyerror("algorithm mismatched");
				return -1;
			}
			p_ext |= SADB_X_EXT_IV4B;

			p_key_enc_len = (yyvsp[(2) - (2)].val).len;
			p_key_enc = (yyvsp[(2) - (2)].val).buf;
			if (ipsec_check_keylen(SADB_EXT_SUPPORTED_ENCRYPT,
			    p_alg_enc, PFKEY_UNUNIT64(p_key_enc_len)) < 0) {
				yyerror(ipsec_strerror());
				return -1;
			}
		}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 453 "parse.y"
    {
			if ((yyvsp[(1) - (2)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_auth = (yyvsp[(1) - (2)].num);

			p_key_auth_len = (yyvsp[(2) - (2)].val).len;
			p_key_auth = (yyvsp[(2) - (2)].val).buf;
#ifdef SADB_X_AALG_TCP_MD5
			if (p_alg_auth == SADB_X_AALG_TCP_MD5) {
				if ((p_key_auth_len < 1) || 
				    (p_key_auth_len > 80))
					return -1;
			} else 
#endif
			{
				if (ipsec_check_keylen(SADB_EXT_SUPPORTED_AUTH,
				    p_alg_auth, 
				    PFKEY_UNUNIT64(p_key_auth_len)) < 0) {
					yyerror(ipsec_strerror());
					return -1;
				}
			}
		}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 478 "parse.y"
    {
			if ((yyvsp[(1) - (1)].num) < 0) {
				yyerror("unsupported algorithm");
				return -1;
			}
			p_alg_auth = (yyvsp[(1) - (1)].num);

			p_key_auth_len = 0;
			p_key_auth = NULL;
		}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 492 "parse.y"
    {
			(yyval.val) = (yyvsp[(1) - (1)].val);
		}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 496 "parse.y"
    {
			caddr_t pp_key;
			caddr_t bp;
			caddr_t yp = (yyvsp[(1) - (1)].val).buf;
			int l;

			l = strlen(yp) % 2 + strlen(yp) / 2;
			if ((pp_key = malloc(l)) == 0) {
				yyerror("not enough core");
				return -1;
			}
			memset(pp_key, 0, l);

			bp = pp_key;
			if (strlen(yp) % 2) {
				*bp = ATOX(yp[0]);
				yp++, bp++;
			}
			while (*yp) {
				*bp = (ATOX(yp[0]) << 4) | ATOX(yp[1]);
				yp += 2, bp++;
			}

			(yyval.val).len = l;
			(yyval.val).buf = pp_key;
		}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 530 "parse.y"
    { p_ext |= (yyvsp[(2) - (2)].num); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 531 "parse.y"
    { p_ext &= ~SADB_X_EXT_CYCSEQ; }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 532 "parse.y"
    { p_mode = (yyvsp[(2) - (2)].num); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 533 "parse.y"
    { p_mode = IPSEC_MODE_ANY; }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 534 "parse.y"
    { p_reqid = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 536 "parse.y"
    {
			if ((p_ext & SADB_X_EXT_OLD) != 0) {
				yyerror("replay prevention cannot be used with "
				    "ah/esp-old");
				return -1;
			}
			p_replay = (yyvsp[(2) - (2)].ulnum);
		}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 544 "parse.y"
    { p_lt_hard = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 545 "parse.y"
    { p_lt_soft = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 546 "parse.y"
    { p_lb_hard = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 547 "parse.y"
    { p_lb_soft = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 548 "parse.y"
    {
		sec_ctx.doi = (yyvsp[(2) - (4)].ulnum);
		sec_ctx.alg = (yyvsp[(3) - (4)].ulnum);
		sec_ctx.len = (yyvsp[(4) - (4)].val).len+1;
		sec_ctx.buf = (yyvsp[(4) - (4)].val).buf;
	}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 560 "parse.y"
    {
			int status;
			struct addrinfo *src, *dst;

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			last_msg_type = SADB_X_SPDADD;
#endif

			/* fixed port fields if ulp is icmpv6 */
			if ((yyvsp[(10) - (13)].val).buf != NULL) {
				if ( ((yyvsp[(9) - (13)].num) != IPPROTO_ICMPV6) &&
					 ((yyvsp[(9) - (13)].num) != IPPROTO_MH))
					return -1;
				free((yyvsp[(5) - (13)].val).buf);
				free((yyvsp[(8) - (13)].val).buf);
				if (fix_portstr(&(yyvsp[(10) - (13)].val), &(yyvsp[(5) - (13)].val), &(yyvsp[(8) - (13)].val)))
					return -1;
			}

			src = parse_addr((yyvsp[(3) - (13)].val).buf, (yyvsp[(5) - (13)].val).buf);
			dst = parse_addr((yyvsp[(6) - (13)].val).buf, (yyvsp[(8) - (13)].val).buf);
			if (!src || !dst) {
				/* yyerror is already called */
				return -1;
			}
			if (src->ai_next || dst->ai_next) {
				yyerror("multiple address specified");
				freeaddrinfo(src);
				freeaddrinfo(dst);
				return -1;
			}

			status = setkeymsg_spdaddr(SADB_X_SPDADD, (yyvsp[(9) - (13)].num), &(yyvsp[(12) - (13)].val),
			    src, (yyvsp[(4) - (13)].num), dst, (yyvsp[(7) - (13)].num));
			freeaddrinfo(src);
			freeaddrinfo(dst);
			if (status < 0)
				return -1;
		}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 600 "parse.y"
    {
			int status;

			status = setkeymsg_spdaddr_tag(SADB_X_SPDADD,
			    (yyvsp[(3) - (5)].val).buf, &(yyvsp[(4) - (5)].val));
			if (status < 0)
				return -1;
		}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 612 "parse.y"
    {
			int status;
			struct addrinfo *src, *dst;

			/* fixed port fields if ulp is icmpv6 */
			if ((yyvsp[(10) - (13)].val).buf != NULL) {
				if (((yyvsp[(9) - (13)].num) != IPPROTO_ICMPV6) &&
					((yyvsp[(9) - (13)].num) != IPPROTO_MH))
					return -1;
				free((yyvsp[(5) - (13)].val).buf);
				free((yyvsp[(8) - (13)].val).buf);
				if (fix_portstr(&(yyvsp[(10) - (13)].val), &(yyvsp[(5) - (13)].val), &(yyvsp[(8) - (13)].val)))
					return -1;
			}

			src = parse_addr((yyvsp[(3) - (13)].val).buf, (yyvsp[(5) - (13)].val).buf);
			dst = parse_addr((yyvsp[(6) - (13)].val).buf, (yyvsp[(8) - (13)].val).buf);
			if (!src || !dst) {
				/* yyerror is already called */
				return -1;
			}
			if (src->ai_next || dst->ai_next) {
				yyerror("multiple address specified");
				freeaddrinfo(src);
				freeaddrinfo(dst);
				return -1;
			}

			status = setkeymsg_spdaddr(SADB_X_SPDDELETE, (yyvsp[(9) - (13)].num), &(yyvsp[(12) - (13)].val),
			    src, (yyvsp[(4) - (13)].num), dst, (yyvsp[(7) - (13)].num));
			freeaddrinfo(src);
			freeaddrinfo(dst);
			if (status < 0)
				return -1;
		}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 651 "parse.y"
    {
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_X_SPDDUMP, SADB_SATYPE_UNSPEC,
			    sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 662 "parse.y"
    {
			struct sadb_msg msg;
			setkeymsg0(&msg, SADB_X_SPDFLUSH, SADB_SATYPE_UNSPEC,
			    sizeof(msg));
			sendkeymsg((char *)&msg, sizeof(msg));
		}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 677 "parse.y"
    {
			char *p;

			for (p = (yyvsp[(1) - (1)].val).buf + 1; *p; p++)
				switch (*p) {
				case '4':
					p_aifamily = AF_INET;
					break;
#ifdef INET6
				case '6':
					p_aifamily = AF_INET6;
					break;
#endif
				case 'n':
					p_aiflags = AI_NUMERICHOST;
					break;
				default:
					yyerror("invalid flag");
					return -1;
				}
		}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 702 "parse.y"
    {
			(yyval.res) = parse_addr((yyvsp[(1) - (1)].val).buf, NULL);
			if ((yyval.res) == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 713 "parse.y"
    {
			(yyval.res) = parse_addr((yyvsp[(1) - (1)].val).buf, NULL);
			if ((yyval.res) == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 721 "parse.y"
    {
			(yyval.res) = parse_addr((yyvsp[(1) - (2)].val).buf, (yyvsp[(2) - (2)].val).buf);
			if ((yyval.res) == NULL) {
				/* yyerror already called by parse_addr */
				return -1;
			}
		}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 731 "parse.y"
    { (yyval.num) = -1; }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 732 "parse.y"
    { (yyval.num) = (yyvsp[(2) - (2)].ulnum); }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 737 "parse.y"
    {
			(yyval.val).buf = strdup("0");
			if (!(yyval.val).buf) {
				yyerror("insufficient memory");
				return -1;
			}
			(yyval.val).len = strlen((yyval.val).buf);
		}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 746 "parse.y"
    {
			(yyval.val).buf = strdup("0");
			if (!(yyval.val).buf) {
				yyerror("insufficient memory");
				return -1;
			}
			(yyval.val).len = strlen((yyval.val).buf);
		}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 755 "parse.y"
    {
			char buf[20];
			snprintf(buf, sizeof(buf), "%lu", (yyvsp[(2) - (3)].ulnum));
			(yyval.val).buf = strdup(buf);
			if (!(yyval.val).buf) {
				yyerror("insufficient memory");
				return -1;
			}
			(yyval.val).len = strlen((yyval.val).buf);
		}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 766 "parse.y"
    {
			(yyval.val) = (yyvsp[(2) - (3)].val);
		}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 772 "parse.y"
    { (yyval.num) = (yyvsp[(1) - (1)].ulnum); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 773 "parse.y"
    { (yyval.num) = IPSEC_ULPROTO_ANY; }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 774 "parse.y"
    { 
				(yyval.num) = IPPROTO_TCP; 
			}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 778 "parse.y"
    {
			struct protoent *ent;

			ent = getprotobyname((yyvsp[(1) - (1)].val).buf);
			if (ent)
				(yyval.num) = ent->p_proto;
			else {
				if (strcmp("icmp6", (yyvsp[(1) - (1)].val).buf) == 0) {
					(yyval.num) = IPPROTO_ICMPV6;
				} else if(strcmp("ip4", (yyvsp[(1) - (1)].val).buf) == 0) {
					(yyval.num) = IPPROTO_IPV4;
				} else {
					yyerror("invalid upper layer protocol");
					return -1;
				}
			}
			endprotoent();
		}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 800 "parse.y"
    {
			(yyval.val).buf = NULL;
			(yyval.val).len = 0;
		}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 805 "parse.y"
    {
			(yyval.val).buf = strdup((yyvsp[(1) - (1)].val).buf);
			if (!(yyval.val).buf) {
				yyerror("insufficient memory");
				return -1;
			}
			(yyval.val).len = strlen((yyval.val).buf);
		}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 817 "parse.y"
    {
			sec_ctx.doi = (yyvsp[(2) - (4)].ulnum);
			sec_ctx.alg = (yyvsp[(3) - (4)].ulnum);
			sec_ctx.len = (yyvsp[(4) - (4)].val).len+1;
			sec_ctx.buf = (yyvsp[(4) - (4)].val).buf;
		}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 827 "parse.y"
    {
			char *policy;
#ifdef HAVE_PFKEY_POLICY_PRIORITY
			struct sadb_x_policy *xpl;
#endif

			policy = ipsec_set_policy((yyvsp[(2) - (2)].val).buf, (yyvsp[(2) - (2)].val).len);
			if (policy == NULL) {
				yyerror(ipsec_strerror());
				return -1;
			}

			(yyval.val).buf = policy;
			(yyval.val).len = ipsec_get_policylen(policy);

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			xpl = (struct sadb_x_policy *) (yyval.val).buf;
			last_priority = xpl->sadb_x_policy_priority;
#endif
		}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 850 "parse.y"
    { (yyval.val) = (yyvsp[(1) - (1)].val); }
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 856 "parse.y"
    {
			exit_now = 1;
			YYACCEPT;
		}
    break;



/* Line 1455 of yacc.c  */
#line 2575 "parse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 861 "parse.y"


int
setkeymsg0(msg, type, satype, l)
	struct sadb_msg *msg;
	unsigned int type;
	unsigned int satype;
	size_t l;
{

	msg->sadb_msg_version = PF_KEY_V2;
	msg->sadb_msg_type = type;
	msg->sadb_msg_errno = 0;
	msg->sadb_msg_satype = satype;
	msg->sadb_msg_reserved = 0;
	msg->sadb_msg_seq = 0;
	msg->sadb_msg_pid = getpid();
	msg->sadb_msg_len = PFKEY_UNIT64(l);
	return 0;
}

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_spdaddr(type, upper, policy, srcs, splen, dsts, dplen)
	unsigned int type;
	unsigned int upper;
	vchar_t *policy;
	struct addrinfo *srcs;
	int splen;
	struct addrinfo *dsts;
	int dplen;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;
	struct sadb_x_policy *sp;
#ifdef HAVE_POLICY_FWD
	struct sadb_x_ipsecrequest *ps = NULL;
	int saved_level, saved_id = 0;
#endif

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, SADB_SATYPE_UNSPEC, 0);
	l = sizeof(struct sadb_msg);

	sp = (struct sadb_x_policy*) (buf + l);
	memcpy(buf + l, policy->buf, policy->len);
	l += policy->len;

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = upper;
			m_addr.sadb_address_prefixlen =
			    (splen >= 0 ? splen : plen);
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), (caddr_t)sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = upper;
			m_addr.sadb_address_prefixlen =
			    (dplen >= 0 ? dplen : plen);
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);
#ifdef SADB_X_EXT_SEC_CTX
			/* Add security context label */
			if (sec_ctx.doi) {
				struct sadb_x_sec_ctx m_sec_ctx;
				u_int slen = sizeof(struct sadb_x_sec_ctx);

				memset(&m_sec_ctx, 0, slen);

				m_sec_ctx.sadb_x_sec_len =
				PFKEY_UNIT64(slen + PFKEY_ALIGN8(sec_ctx.len));

				m_sec_ctx.sadb_x_sec_exttype = 
					SADB_X_EXT_SEC_CTX;
				m_sec_ctx.sadb_x_ctx_len = sec_ctx.len;/*bytes*/
				m_sec_ctx.sadb_x_ctx_doi = sec_ctx.doi;
				m_sec_ctx.sadb_x_ctx_alg = sec_ctx.alg;
				setvarbuf(buf, &l, 
					  (struct sadb_ext *)&m_sec_ctx, slen, 
					  (caddr_t)sec_ctx.buf, sec_ctx.len);
			}
#endif
			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

#ifdef HAVE_POLICY_FWD
			/* create extra call for FWD policy */
			if (f_rfcmode && sp->sadb_x_policy_dir == IPSEC_DIR_INBOUND) {
				sp->sadb_x_policy_dir = IPSEC_DIR_FWD;
				ps = (struct sadb_x_ipsecrequest*) (sp+1);

				/* if request level is unique, change it to
				 * require for fwd policy */
				/* XXX: currently, only first policy is updated
				 * only. Update following too... */
				saved_level = ps->sadb_x_ipsecrequest_level;
				if (saved_level == IPSEC_LEVEL_UNIQUE) {
					saved_id = ps->sadb_x_ipsecrequest_reqid;
					ps->sadb_x_ipsecrequest_reqid=0;
					ps->sadb_x_ipsecrequest_level=IPSEC_LEVEL_REQUIRE;
				}

				sendkeymsg(buf, l);
				/* restoring for next message */
				sp->sadb_x_policy_dir = IPSEC_DIR_INBOUND;
				if (saved_level == IPSEC_LEVEL_UNIQUE) {
					ps->sadb_x_ipsecrequest_reqid = saved_id;
					ps->sadb_x_ipsecrequest_level = saved_level;
				}
			}
#endif

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

static int
setkeymsg_spdaddr_tag(type, tag, policy)
	unsigned int type;
	char *tag;
	vchar_t *policy;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0;
#ifdef SADB_X_EXT_TAG
	struct sadb_x_tag m_tag;
#endif
	int n;

	msg = (struct sadb_msg *)buf;

	/* fix up length afterwards */
	setkeymsg0(msg, type, SADB_SATYPE_UNSPEC, 0);
	l = sizeof(struct sadb_msg);

	memcpy(buf + l, policy->buf, policy->len);
	l += policy->len;

	l0 = l;
	n = 0;

#ifdef SADB_X_EXT_TAG
	memset(&m_tag, 0, sizeof(m_tag));
	m_tag.sadb_x_tag_len = PFKEY_UNIT64(sizeof(m_tag));
	m_tag.sadb_x_tag_exttype = SADB_X_EXT_TAG;
	if (strlcpy(m_tag.sadb_x_tag_name, tag,
	    sizeof(m_tag.sadb_x_tag_name)) >= sizeof(m_tag.sadb_x_tag_name))
		return -1;
	memcpy(buf + l, &m_tag, sizeof(m_tag));
	l += sizeof(m_tag);
#endif

	msg->sadb_msg_len = PFKEY_UNIT64(l);

	sendkeymsg(buf, l);

	return 0;
}

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_addr(type, satype, srcs, dsts, no_spi)
	unsigned int type;
	unsigned int satype;
	struct addrinfo *srcs;
	struct addrinfo *dsts;
	int no_spi;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0, len;
	struct sadb_sa m_sa;
	struct sadb_x_sa2 m_sa2;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, satype, 0);
	l = sizeof(struct sadb_msg);

	if (!no_spi) {
		len = sizeof(struct sadb_sa);
		m_sa.sadb_sa_len = PFKEY_UNIT64(len);
		m_sa.sadb_sa_exttype = SADB_EXT_SA;
		m_sa.sadb_sa_spi = htonl(p_spi);
		m_sa.sadb_sa_replay = p_replay;
		m_sa.sadb_sa_state = 0;
		m_sa.sadb_sa_auth = p_alg_auth;
		m_sa.sadb_sa_encrypt = p_alg_enc;
		m_sa.sadb_sa_flags = p_ext;

		memcpy(buf + l, &m_sa, len);
		l += len;

		len = sizeof(struct sadb_x_sa2);
		m_sa2.sadb_x_sa2_len = PFKEY_UNIT64(len);
		m_sa2.sadb_x_sa2_exttype = SADB_X_EXT_SA2;
		m_sa2.sadb_x_sa2_mode = p_mode;
		m_sa2.sadb_x_sa2_reqid = p_reqid;

		memcpy(buf + l, &m_sa2, len);
		l += len;
	}

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

#ifdef SADB_X_EXT_NAT_T_TYPE
static u_int16_t get_port (struct addrinfo *addr)
{
	struct sockaddr *s = addr->ai_addr;
	u_int16_t port = 0;

	switch (s->sa_family) {
	case AF_INET:
	  {
		struct sockaddr_in *sin4 = (struct sockaddr_in *)s;
		port = ntohs(sin4->sin_port);
		break;
	  }
	case AF_INET6:
	  {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)s;
		port = ntohs(sin6->sin6_port);
		break;
	  }
	}

	if (port == 0)
		port = DEFAULT_NATT_PORT;

	return port;
}
#endif

/* XXX NO BUFFER OVERRUN CHECK! BAD BAD! */
static int
setkeymsg_add(type, satype, srcs, dsts)
	unsigned int type;
	unsigned int satype;
	struct addrinfo *srcs;
	struct addrinfo *dsts;
{
	struct sadb_msg *msg;
	char buf[BUFSIZ];
	int l, l0, len;
	struct sadb_sa m_sa;
	struct sadb_x_sa2 m_sa2;
	struct sadb_address m_addr;
	struct addrinfo *s, *d;
	int n;
	int plen;
	struct sockaddr *sa;
	int salen;

	msg = (struct sadb_msg *)buf;

	if (!srcs || !dsts)
		return -1;

	/* fix up length afterwards */
	setkeymsg0(msg, type, satype, 0);
	l = sizeof(struct sadb_msg);

	/* set encryption algorithm, if present. */
	if (satype != SADB_X_SATYPE_IPCOMP && p_key_enc) {
		union {
			struct sadb_key key;
			struct sadb_ext ext;
		} m;

		m.key.sadb_key_len =
			PFKEY_UNIT64(sizeof(m.key)
				   + PFKEY_ALIGN8(p_key_enc_len));
		m.key.sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
		m.key.sadb_key_bits = p_key_enc_len * 8;
		m.key.sadb_key_reserved = 0;

		setvarbuf(buf, &l, &m.ext, sizeof(m.key),
			p_key_enc, p_key_enc_len);
	}

	/* set authentication algorithm, if present. */
	if (p_key_auth) {
		union {
			struct sadb_key key;
			struct sadb_ext ext;
		} m;

		m.key.sadb_key_len =
			PFKEY_UNIT64(sizeof(m.key)
				   + PFKEY_ALIGN8(p_key_auth_len));
		m.key.sadb_key_exttype = SADB_EXT_KEY_AUTH;
		m.key.sadb_key_bits = p_key_auth_len * 8;
		m.key.sadb_key_reserved = 0;

		setvarbuf(buf, &l, &m.ext, sizeof(m.key),
			p_key_auth, p_key_auth_len);
	}

	/* set lifetime for HARD */
	if (p_lt_hard != 0 || p_lb_hard != 0) {
		struct sadb_lifetime m_lt;
		u_int slen = sizeof(struct sadb_lifetime);

		m_lt.sadb_lifetime_len = PFKEY_UNIT64(slen);
		m_lt.sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
		m_lt.sadb_lifetime_allocations = 0;
		m_lt.sadb_lifetime_bytes = p_lb_hard;
		m_lt.sadb_lifetime_addtime = p_lt_hard;
		m_lt.sadb_lifetime_usetime = 0;

		memcpy(buf + l, &m_lt, slen);
		l += slen;
	}

	/* set lifetime for SOFT */
	if (p_lt_soft != 0 || p_lb_soft != 0) {
		struct sadb_lifetime m_lt;
		u_int slen = sizeof(struct sadb_lifetime);

		m_lt.sadb_lifetime_len = PFKEY_UNIT64(slen);
		m_lt.sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
		m_lt.sadb_lifetime_allocations = 0;
		m_lt.sadb_lifetime_bytes = p_lb_soft;
		m_lt.sadb_lifetime_addtime = p_lt_soft;
		m_lt.sadb_lifetime_usetime = 0;

		memcpy(buf + l, &m_lt, slen);
		l += slen;
	}

#ifdef SADB_X_EXT_SEC_CTX
	/* Add security context label */
	if (sec_ctx.doi) {
		struct sadb_x_sec_ctx m_sec_ctx;
		u_int slen = sizeof(struct sadb_x_sec_ctx);

		memset(&m_sec_ctx, 0, slen);

		m_sec_ctx.sadb_x_sec_len = PFKEY_UNIT64(slen +
					PFKEY_ALIGN8(sec_ctx.len));
		m_sec_ctx.sadb_x_sec_exttype = SADB_X_EXT_SEC_CTX;
		m_sec_ctx.sadb_x_ctx_len = sec_ctx.len; /* bytes */
		m_sec_ctx.sadb_x_ctx_doi = sec_ctx.doi;
		m_sec_ctx.sadb_x_ctx_alg = sec_ctx.alg;
		setvarbuf(buf, &l, (struct sadb_ext *)&m_sec_ctx, slen,
			  (caddr_t)sec_ctx.buf, sec_ctx.len); 
	}
#endif

	len = sizeof(struct sadb_sa);
	m_sa.sadb_sa_len = PFKEY_UNIT64(len);
	m_sa.sadb_sa_exttype = SADB_EXT_SA;
	m_sa.sadb_sa_spi = htonl(p_spi);
	m_sa.sadb_sa_replay = p_replay;
	m_sa.sadb_sa_state = 0;
	m_sa.sadb_sa_auth = p_alg_auth;
	m_sa.sadb_sa_encrypt = p_alg_enc;
	m_sa.sadb_sa_flags = p_ext;

	memcpy(buf + l, &m_sa, len);
	l += len;

	len = sizeof(struct sadb_x_sa2);
	m_sa2.sadb_x_sa2_len = PFKEY_UNIT64(len);
	m_sa2.sadb_x_sa2_exttype = SADB_X_EXT_SA2;
	m_sa2.sadb_x_sa2_mode = p_mode;
	m_sa2.sadb_x_sa2_reqid = p_reqid;

	memcpy(buf + l, &m_sa2, len);
	l += len;

#ifdef SADB_X_EXT_NAT_T_TYPE
	if (p_natt_type) {
		struct sadb_x_nat_t_type natt_type;

		len = sizeof(struct sadb_x_nat_t_type);
		memset(&natt_type, 0, len);
		natt_type.sadb_x_nat_t_type_len = PFKEY_UNIT64(len);
		natt_type.sadb_x_nat_t_type_exttype = SADB_X_EXT_NAT_T_TYPE;
		natt_type.sadb_x_nat_t_type_type = p_natt_type;

		memcpy(buf + l, &natt_type, len);
		l += len;

		if (p_natt_oa) {
			sa = p_natt_oa->ai_addr;
			switch (sa->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				return -1;
			}
			salen = sysdep_sa_len(sa);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_X_EXT_NAT_T_OA;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);
		}
	}
#endif

	l0 = l;
	n = 0;

	/* do it for all src/dst pairs */
	for (s = srcs; s; s = s->ai_next) {
		for (d = dsts; d; d = d->ai_next) {
			/* rewind pointer */
			l = l0;

			if (s->ai_addr->sa_family != d->ai_addr->sa_family)
				continue;
			switch (s->ai_addr->sa_family) {
			case AF_INET:
				plen = sizeof(struct in_addr) << 3;
				break;
#ifdef INET6
			case AF_INET6:
				plen = sizeof(struct in6_addr) << 3;
				break;
#endif
			default:
				continue;
			}

			/* set src */
			sa = s->ai_addr;
			salen = sysdep_sa_len(s->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

			/* set dst */
			sa = d->ai_addr;
			salen = sysdep_sa_len(d->ai_addr);
			m_addr.sadb_address_len = PFKEY_UNIT64(sizeof(m_addr) +
			    PFKEY_ALIGN8(salen));
			m_addr.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			m_addr.sadb_address_proto = IPSEC_ULPROTO_ANY;
			m_addr.sadb_address_prefixlen = plen;
			m_addr.sadb_address_reserved = 0;

			setvarbuf(buf, &l, (struct sadb_ext *)&m_addr,
			    sizeof(m_addr), sa, salen);

#ifdef SADB_X_EXT_NAT_T_TYPE
			if (p_natt_type) {
				struct sadb_x_nat_t_port natt_port;

				/* NATT_SPORT */
				len = sizeof(struct sadb_x_nat_t_port);
				memset(&natt_port, 0, len);
				natt_port.sadb_x_nat_t_port_len = PFKEY_UNIT64(len);
				natt_port.sadb_x_nat_t_port_exttype =
					SADB_X_EXT_NAT_T_SPORT;
				natt_port.sadb_x_nat_t_port_port = htons(get_port(s));
				
				memcpy(buf + l, &natt_port, len);
				l += len;

				/* NATT_DPORT */
				natt_port.sadb_x_nat_t_port_exttype =
					SADB_X_EXT_NAT_T_DPORT;
				natt_port.sadb_x_nat_t_port_port = htons(get_port(d));
				
				memcpy(buf + l, &natt_port, len);
				l += len;
			}
#endif
			msg->sadb_msg_len = PFKEY_UNIT64(l);

			sendkeymsg(buf, l);

			n++;
		}
	}

	if (n == 0)
		return -1;
	else
		return 0;
}

static struct addrinfo *
parse_addr(host, port)
	char *host;
	char *port;
{
	struct addrinfo hints, *res = NULL;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = p_aifamily;
	hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
	hints.ai_protocol = IPPROTO_UDP;	/*dummy*/
	hints.ai_flags = p_aiflags;
	error = getaddrinfo(host, port, &hints, &res);
	if (error != 0) {
		yyerror(gai_strerror(error));
		return NULL;
	}
	return res;
}

static int
fix_portstr(spec, sport, dport)
	vchar_t *spec, *sport, *dport;
{
	const char *p, *p2 = "0";
	char *q;
	u_int l;

	l = 0;
	for (q = spec->buf; *q != ',' && *q != '\0' && l < spec->len; q++, l++)
		;
	if (*q != '\0') {
		if (*q == ',') {
			*q = '\0';
			p2 = ++q;
		}
		for (p = p2; *p != '\0' && l < spec->len; p++, l++)
			;
		if (*p != '\0' || *p2 == '\0') {
			yyerror("invalid an upper layer protocol spec");
			return -1;
		}
	}

	sport->buf = strdup(spec->buf);
	if (!sport->buf) {
		yyerror("insufficient memory");
		return -1;
	}
	sport->len = strlen(sport->buf);
	dport->buf = strdup(p2);
	if (!dport->buf) {
		yyerror("insufficient memory");
		return -1;
	}
	dport->len = strlen(dport->buf);

	return 0;
}

static int
setvarbuf(buf, off, ebuf, elen, vbuf, vlen)
	char *buf;
	int *off;
	struct sadb_ext *ebuf;
	int elen;
	const void *vbuf;
	int vlen;
{
	memset(buf + *off, 0, PFKEY_UNUNIT64(ebuf->sadb_ext_len));
	memcpy(buf + *off, (caddr_t)ebuf, elen);
	memcpy(buf + *off + elen, vbuf, vlen);
	(*off) += PFKEY_ALIGN8(elen + vlen);

	return 0;
}

void
parse_init()
{
	p_spi = 0;

	p_ext = SADB_X_EXT_CYCSEQ;
	p_alg_enc = SADB_EALG_NONE;
	p_alg_auth = SADB_AALG_NONE;
	p_mode = IPSEC_MODE_ANY;
	p_reqid = 0;
	p_replay = 0;
	p_key_enc_len = p_key_auth_len = 0;
	p_key_enc = p_key_auth = 0;
	p_lt_hard = p_lt_soft = 0;
	p_lb_hard = p_lb_soft = 0;

	memset(&sec_ctx, 0, sizeof(struct security_ctx));

	p_aiflags = 0;
	p_aifamily = PF_UNSPEC;

	/* Clear out any natt OA information */
	if (p_natt_oa)
		freeaddrinfo (p_natt_oa);
	p_natt_oa = NULL;
	p_natt_type = 0;

	return;
}

void
free_buffer()
{
	/* we got tons of memory leaks in the parser anyways, leave them */

	return;
}

