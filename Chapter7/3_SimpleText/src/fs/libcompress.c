/* zlib 1.2.5, compressed to one file */

/* deflate.c -- compress data using the deflation algorithm
 * Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
 *  ALGORITHM
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many people for bug reports and testing.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"DEFLATE Compressed Data Format Specification".
 *      Available in http://www.ietf.org/rfc/rfc1951.txt
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 */

/* @(#) $Id$ */

/* #define GEN_TREES_H */

//#include "deflate.h"

/* deflate.h -- internal compression state
 * Copyright (C) 1995-2010 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

/* MSVC gives a lot of warnings, we ignore them */
#ifdef _MSC_VER
#pragma warning( disable : 4131 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#endif

#ifndef DEFLATE_H
#define DEFLATE_H

#include "libcompress.h"

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer creation by deflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip encoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GZIP
#endif

/* ===========================================================================
 * Internal compression state.
 */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define INIT_STATE    42
#define EXTRA_STATE   69
#define NAME_STATE    73
#define COMMENT_STATE 91
#define HCRC_STATE   103
#define BUSY_STATE   113
#define FINISH_STATE 666
/* Stream status */


/* Data structure describing a single value and its code string. */
typedef struct ct_data_s
{
	union
	{
		ush  freq;       /* frequency count */
		ush  code;       /* bit string */
	} fc;
	union
	{
		ush  dad;        /* father node in Huffman tree */
		ush  len;        /* length of bit string */
	} dl;
} FAR ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s
{
	ct_data* dyn_tree;           /* the dynamic tree */
	int     max_code;            /* largest code with non zero frequency */
	static_tree_desc* stat_desc; /* the corresponding static tree */
} FAR tree_desc;

typedef ush Pos;
typedef Pos FAR Posf;
typedef unsigned IPos;

/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

typedef struct internal_state
{
	z_streamp strm;      /* pointer back to this zlib stream */
	int   status;        /* as the name implies */
	Bytef* pending_buf;  /* output still pending */
	ulg   pending_buf_size; /* size of pending_buf */
	Bytef* pending_out;  /* next pending byte to output to the stream */
	uInt   pending;      /* nb of bytes in the pending buffer */
	int   wrap;          /* bit 0 true for zlib, bit 1 true for gzip */
	gz_headerp  gzhead;  /* gzip header information to write */
	uInt   gzindex;      /* where in extra, name, or comment */
	Byte  method;        /* STORED (for zip only) or DEFLATED */
	int   last_flush;    /* value of flush param for previous deflate call */

	/* used by deflate.c: */

	uInt  w_size;        /* LZ77 window size (32K by default) */
	uInt  w_bits;        /* log2(w_size)  (8..16) */
	uInt  w_mask;        /* w_size - 1 */

	Bytef* window;
	/* Sliding window. Input bytes are read into the second half of the window,
	 * and move to the first half later to keep a dictionary of at least wSize
	 * bytes. With this organization, matches are limited to a distance of
	 * wSize-MAX_MATCH bytes, but this ensures that IO is always
	 * performed with a length multiple of the block size. Also, it limits
	 * the window size to 64K, which is quite useful on MSDOS.
	 * To do: use the user input buffer as sliding window.
	 */

	ulg window_size;
	/* Actual size of window: 2*wSize, except when the user input buffer
	 * is directly used as sliding window.
	 */

	Posf* prev;
	/* Link to older string with same hash index. To limit the size of this
	 * array to 64K, this link is maintained only for the last 32K strings.
	 * An index in this array is thus a window index modulo 32K.
	 */

	Posf* head; /* Heads of the hash chains or NIL. */

	uInt  ins_h;          /* hash index of string to be inserted */
	uInt  hash_size;      /* number of elements in hash table */
	uInt  hash_bits;      /* log2(hash_size) */
	uInt  hash_mask;      /* hash_size-1 */

	uInt  hash_shift;
	/* Number of bits by which ins_h must be shifted at each input
	 * step. It must be such that after MIN_MATCH steps, the oldest
	 * byte no longer takes part in the hash key, that is:
	 *   hash_shift * MIN_MATCH >= hash_bits
	 */

	long block_start;
	/* Window position at the beginning of the current output block. Gets
	 * negative when the window is moved backwards.
	 */

	uInt match_length;           /* length of best match */
	IPos prev_match;             /* previous match */
	int match_available;         /* set if previous match exists */
	uInt strstart;               /* start of string to insert */
	uInt match_start;            /* start of matching string */
	uInt lookahead;              /* number of valid bytes ahead in window */

	uInt prev_length;
	/* Length of the best match at previous step. Matches not greater than this
	 * are discarded. This is used in the lazy match evaluation.
	 */

	uInt max_chain_length;
	/* To speed up deflation, hash chains are never searched beyond this
	 * length.  A higher limit improves compression ratio but degrades the
	 * speed.
	 */

	uInt max_lazy_match;
	/* Attempt to find a better match only when the current match is strictly
	 * smaller than this value. This mechanism is used only for compression
	 * levels >= 4.
	 */
#   define max_insert_length  max_lazy_match
	/* Insert new strings in the hash table only if the match length is not
	 * greater than this length. This saves time but degrades compression.
	 * max_insert_length is used only for compression levels <= 3.
	 */

	int level;    /* compression level (1..9) */
	int strategy; /* favor or force Huffman coding*/

	uInt good_match;
	/* Use a faster search when the previous match is longer than this */

	int nice_match; /* Stop searching when current match exceeds this */

	/* used by trees.c: */
	/* Didn't use ct_data typedef below to supress compiler warning */
	struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
	struct ct_data_s dyn_dtree[2 * D_CODES + 1]; /* distance tree */
	struct ct_data_s bl_tree[2 * BL_CODES + 1]; /* Huffman tree for bit lengths */

	struct tree_desc_s l_desc;               /* desc. for literal tree */
	struct tree_desc_s d_desc;               /* desc. for distance tree */
	struct tree_desc_s bl_desc;              /* desc. for bit length tree */

	ush bl_count[MAX_BITS + 1];
	/* number of codes at each bit length for an optimal tree */

	int heap[2 * L_CODES + 1];  /* heap used to build the Huffman trees */
	int heap_len;               /* number of elements in the heap */
	int heap_max;               /* element of largest frequency */
	/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
	 * The same heap array is used to build all trees.
	 */

	uch depth[2 * L_CODES + 1];
	/* Depth of each subtree used as tie breaker for trees of equal frequency
	 */

	uchf* l_buf;          /* buffer for literals or lengths */

	uInt  lit_bufsize;
	/* Size of match buffer for literals/lengths.  There are 4 reasons for
	 * limiting lit_bufsize to 64K:
	 *   - frequencies can be kept in 16 bit counters
	 *   - if compression is not successful for the first block, all input
	 *     data is still in the window so we can still emit a stored block even
	 *     when input comes from standard input.  (This can also be done for
	 *     all blocks if lit_bufsize is not greater than 32K.)
	 *   - if compression is not successful for a file smaller than 64K, we can
	 *     even emit a stored file instead of a stored block (saving 5 bytes).
	 *     This is applicable only for zip (not gzip or zlib).
	 *   - creating new Huffman trees less frequently may not provide fast
	 *     adaptation to changes in the input data statistics. (Take for
	 *     example a binary file with poorly compressible code followed by
	 *     a highly compressible string table.) Smaller buffer sizes give
	 *     fast adaptation but have of course the overhead of transmitting
	 *     trees more frequently.
	 *   - I can't count above 4
	 */

	uInt last_lit;      /* running index in l_buf */

	ushf* d_buf;
	/* Buffer for distances. To simplify the code, d_buf and l_buf have
	 * the same number of elements. To use different lengths, an extra flag
	 * array would be necessary.
	 */

	ulg opt_len;        /* bit length of current block with optimal trees */
	ulg static_len;     /* bit length of current block with static trees */
	uInt matches;       /* number of string matches in current block */
	int last_eob_len;   /* bit length of EOB code for last block */

#ifdef DEBUG
	ulg compressed_len; /* total bit length of compressed file mod 2^32 */
	ulg bits_sent;      /* bit length of compressed data sent mod 2^32 */
#endif

	ush bi_buf;
	/* Output buffer. bits are inserted starting at the bottom (least
	 * significant bits).
	 */
	int bi_valid;
	/* Number of valid bits in bi_buf.  All bits above the last valid bit
	 * are always zero.
	 */

	ulg high_water;
	/* High water mark offset in window for initialized bytes -- bytes above
	 * this are set to zero in order to avoid memory check warnings when
	 * longest match routines access bytes past the input.  This is then
	 * updated to the new high water mark.
	 */

} FAR deflate_state;

/* Output a byte on the stream.
 * IN assertion: there is enough room in pending_buf.
 */
#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}


#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define WIN_INIT MAX_MATCH
/* Number of bytes after end of data in window to initialize in order to avoid
   memory checker errors from longest match routines */

/* in trees.c */
void ZLIB_INTERNAL _tr_init OF( ( deflate_state* s ) );
int ZLIB_INTERNAL _tr_tally OF( ( deflate_state* s, unsigned dist, unsigned lc ) );
void ZLIB_INTERNAL _tr_flush_block OF( ( deflate_state* s, charf* buf,
                                         ulg stored_len, int last ) );
void ZLIB_INTERNAL _tr_align OF( ( deflate_state* s ) );
void ZLIB_INTERNAL _tr_stored_block OF( ( deflate_state* s, charf* buf,
                                          ulg stored_len, int last ) );

#define d_code(dist) \
   ((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _dist_code[256] and _dist_code[257] are never
 * used.
 */

#ifndef DEBUG
/* Inline versions of _tr_tally for speed: */

#if defined(GEN_TREES_H) || !defined(STDC)
extern uch ZLIB_INTERNAL _length_code[];
extern uch ZLIB_INTERNAL _dist_code[];
#else
extern const uch ZLIB_INTERNAL _length_code[];
extern const uch ZLIB_INTERNAL _dist_code[];
#endif

# define _tr_tally_lit(s, c, flush) \
  { uch cc = (c); \
    s->d_buf[s->last_lit] = 0; \
    s->l_buf[s->last_lit++] = cc; \
    s->dyn_ltree[cc].Freq++; \
    flush = (s->last_lit == s->lit_bufsize-1); \
   }
# define _tr_tally_dist(s, distance, length, flush) \
  { uch len = (length); \
    ush dist = (distance); \
    s->d_buf[s->last_lit] = dist; \
    s->l_buf[s->last_lit++] = len; \
    dist--; \
    s->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
    s->dyn_dtree[d_code(dist)].Freq++; \
    flush = (s->last_lit == s->lit_bufsize-1); \
  }
#else
# define _tr_tally_lit(s, c, flush) flush = _tr_tally(s, 0, c)
# define _tr_tally_dist(s, distance, length, flush) \
              flush = _tr_tally(s, distance, length)
#endif

#endif /* DEFLATE_H */


/*  //////  */

#ifdef DEBUG
#  include <ctype.h>
#endif

const char deflate_copyright[] =
   " deflate 1.2.5 Copyright 1995-2010 Jean-loup Gailly and Mark Adler ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/* ===========================================================================
 *  Function prototypes.
 */
typedef enum
{
   need_more,      /* block not completed, need more input or more output */
   block_done,     /* block flush performed */
   finish_started, /* finish started, need only more output at next deflate */
   finish_done     /* finish done, accept no more input or output */
} block_state;

typedef block_state ( *compress_func ) OF( ( deflate_state* s, int flush ) );
/* Compression function. Returns the block state after the call. */

local void fill_window    OF( ( deflate_state* s ) );
local block_state deflate_stored OF( ( deflate_state* s, int flush ) );
local block_state deflate_fast   OF( ( deflate_state* s, int flush ) );
#ifndef FASTEST
local block_state deflate_slow   OF( ( deflate_state* s, int flush ) );
#endif
local block_state deflate_rle    OF( ( deflate_state* s, int flush ) );
local block_state deflate_huff   OF( ( deflate_state* s, int flush ) );
local void lm_init        OF( ( deflate_state* s ) );
local void putShortMSB    OF( ( deflate_state* s, uInt b ) );
local void flush_pending  OF( ( z_streamp strm ) );
local int read_buf        OF( ( z_streamp strm, Bytef* buf, unsigned size ) );
#ifdef ASMV
void match_init OF( ( void ) ); /* asm code initialization */
uInt longest_match  OF( ( deflate_state* s, IPos cur_match ) );
#else
local uInt longest_match  OF( ( deflate_state* s, IPos cur_match ) );
#endif

#ifdef DEBUG
local  void check_match OF( ( deflate_state* s, IPos start, IPos match,
                              int length ) );
#endif

/* ===========================================================================
 * Local data
 */

#define NIL 0
/* Tail of hash chains */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */
typedef struct config_s
{
	ush good_length; /* reduce lazy search above this match length */
	ush max_lazy;    /* do not perform lazy search above this match length */
	ush nice_length; /* quit search above this match length */
	ush max_chain;
	compress_func func;
} config;

#ifdef FASTEST
local const config configuration_table[2] =
{
	/*      good lazy nice chain */
	/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
	/* 1 */ {4,    4,  8,    4, deflate_fast}
}; /* max speed, no lazy matches */
#else
local const config configuration_table[10] =
{
	/*      good lazy nice chain */
	/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
	/* 1 */ {4,    4,  8,    4, deflate_fast}, /* max speed, no lazy matches */
	/* 2 */ {4,    5, 16,    8, deflate_fast},
	/* 3 */ {4,    6, 32,   32, deflate_fast},

	/* 4 */ {4,    4, 16,   16, deflate_slow},  /* lazy matches */
	/* 5 */ {8,   16, 32,   32, deflate_slow},
	/* 6 */ {8,   16, 128, 128, deflate_slow},
	/* 7 */ {8,   32, 128, 256, deflate_slow},
	/* 8 */ {32, 128, 258, 1024, deflate_slow},
	/* 9 */ {32, 258, 258, 4096, deflate_slow}
}; /* max compression */
#endif

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

#define EQUAL 0
/* result of memcmp for equal strings */

#ifndef NO_DUMMY_DECL
/* LV: commented out
/// struct static_tree_desc_s {int dummy;}; / * for buggy compilers */
#endif

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)


/* ===========================================================================
 * Insert string str in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * If this file is compiled with -DFASTEST, the compression level is forced
 * to 1, and no hash chains are maintained.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of str are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#ifdef FASTEST
#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
    match_head = s->head[s->ins_h], \
    s->head[s->ins_h] = (Pos)(str))
#else
#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
    match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h], \
    s->head[s->ins_h] = (Pos)(str))
#endif

/* ===========================================================================
 * Initialize the hash table (avoiding 64K overflow for 16 bit systems).
 * prev[] will be initialized on the fly.
 */
#define CLEAR_HASH(s) \
    s->head[s->hash_size-1] = NIL; \
    zmemzero((Bytef *)s->head, (unsigned)(s->hash_size-1)*sizeof(*s->head));

/* ========================================================================= */
int ZEXPORT deflateInit_( strm, level, version, stream_size )
z_streamp strm;
int level;
const char* version;
int stream_size;
{
	return deflateInit2_( strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
	                      Z_DEFAULT_STRATEGY, version, stream_size );
	/* To do: ignore strm->next_in if we use it as window */
}

/* ========================================================================= */
int ZEXPORT deflateInit2_( strm, level, method, windowBits, memLevel, strategy,
                           version, stream_size )
z_streamp strm;
int  level;
int  method;
int  windowBits;
int  memLevel;
int  strategy;
const char* version;
int stream_size;
{
	deflate_state* s;
	int wrap = 1;
	static const char my_version[] = ZLIB_VERSION;

	ushf* overlay;
	/* We overlay pending_buf and d_buf+l_buf. This works since the average
	 * output size for (length,distance) codes is <= 24 bits.
	 */

	if ( version == Z_NULL || version[0] != my_version[0] ||
	     stream_size != sizeof( z_stream ) )
	{
		return Z_VERSION_ERROR;
	}

	if ( strm == Z_NULL ) { return Z_STREAM_ERROR; }

	strm->msg = Z_NULL;

	if ( strm->zalloc == ( alloc_func )0 )
	{
		strm->zalloc = zcalloc;
		strm->opaque = ( voidpf )0;
	}

	if ( strm->zfree == ( free_func )0 ) { strm->zfree = zcfree; }

#ifdef FASTEST

	if ( level != 0 ) { level = 1; }

#else

	if ( level == Z_DEFAULT_COMPRESSION ) { level = 6; }

#endif

	if ( windowBits < 0 ) /* suppress zlib wrapper */
	{
		wrap = 0;
		windowBits = -windowBits;
	}

#ifdef GZIP
	else if ( windowBits > 15 )
	{
		wrap = 2;       /* write gzip wrapper instead */
		windowBits -= 16;
	}

#endif

	if ( memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
	     windowBits < 8 || windowBits > 15 || level < 0 || level > 9 ||
	     strategy < 0 || strategy > Z_FIXED )
	{
		return Z_STREAM_ERROR;
	}

	if ( windowBits == 8 ) { windowBits = 9; } /* until 256-byte window bug fixed */

	s = ( deflate_state* ) ZALLOC( strm, 1, sizeof( deflate_state ) );

	if ( s == Z_NULL ) { return Z_MEM_ERROR; }

	strm->state = ( struct internal_state FAR* )s;
	s->strm = strm;

	s->wrap = wrap;
	s->gzhead = Z_NULL;
	s->w_bits = windowBits;
	s->w_size = 1 << s->w_bits;
	s->w_mask = s->w_size - 1;

	s->hash_bits = memLevel + 7;
	s->hash_size = 1 << s->hash_bits;
	s->hash_mask = s->hash_size - 1;
	s->hash_shift =  ( ( s->hash_bits + MIN_MATCH - 1 ) / MIN_MATCH );

	s->window = ( Bytef* ) ZALLOC( strm, s->w_size, 2 * sizeof( Byte ) );
	s->prev   = ( Posf* )  ZALLOC( strm, s->w_size, sizeof( Pos ) );
	s->head   = ( Posf* )  ZALLOC( strm, s->hash_size, sizeof( Pos ) );

	s->high_water = 0;      /* nothing written to s->window yet */

	s->lit_bufsize = 1 << ( memLevel + 6 ); /* 16K elements by default */

	overlay = ( ushf* ) ZALLOC( strm, s->lit_bufsize, sizeof( ush ) + 2 );
	s->pending_buf = ( uchf* ) overlay;
	s->pending_buf_size = ( ulg )s->lit_bufsize * ( sizeof( ush ) + 2L );

	if ( s->window == Z_NULL || s->prev == Z_NULL || s->head == Z_NULL ||
	     s->pending_buf == Z_NULL )
	{
		s->status = FINISH_STATE;
		strm->msg = ( char* )ERR_MSG( Z_MEM_ERROR );
		deflateEnd ( strm );
		return Z_MEM_ERROR;
	}

	s->d_buf = overlay + s->lit_bufsize / sizeof( ush );
	s->l_buf = s->pending_buf + ( 1 + sizeof( ush ) ) * s->lit_bufsize;

	s->level = level;
	s->strategy = strategy;
	s->method = ( Byte )method;

	return deflateReset( strm );
}

/* ========================================================================= */
int ZEXPORT deflateSetDictionary ( strm, dictionary, dictLength )
z_streamp strm;
const Bytef* dictionary;
uInt  dictLength;
{
	deflate_state* s;
	uInt length = dictLength;
	uInt n;
	IPos hash_head = 0;

	if ( strm == Z_NULL || strm->state == Z_NULL || dictionary == Z_NULL ||
	     strm->state->wrap == 2 ||
	     ( strm->state->wrap == 1 && strm->state->status != INIT_STATE ) )
	{
		return Z_STREAM_ERROR;
	}

	s = strm->state;

	if ( s->wrap )
	{
		strm->adler = adler32( strm->adler, dictionary, dictLength );
	}

	if ( length < MIN_MATCH ) { return Z_OK; }

	if ( length > s->w_size )
	{
		length = s->w_size;
		dictionary += dictLength - length; /* use the tail of the dictionary */
	}

	zmemcpy( s->window, dictionary, length );
	s->strstart = length;
	s->block_start = ( long )length;

	/* Insert all strings in the hash table (except for the last two bytes).
	 * s->lookahead stays null, so s->ins_h will be recomputed at the next
	 * call of fill_window.
	 */
	s->ins_h = s->window[0];
	UPDATE_HASH( s, s->ins_h, s->window[1] );

	for ( n = 0; n <= length - MIN_MATCH; n++ )
	{
		INSERT_STRING( s, n, hash_head );
	}

	if ( hash_head ) { hash_head = 0; } /* to make compiler happy */

	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateReset ( strm )
z_streamp strm;
{
	deflate_state* s;

	if ( strm == Z_NULL || strm->state == Z_NULL ||
	     strm->zalloc == ( alloc_func )0 || strm->zfree == ( free_func )0 )
	{
		return Z_STREAM_ERROR;
	}

	strm->total_in = strm->total_out = 0;
	strm->msg = Z_NULL; /* use zfree if we ever allocate msg dynamically */
	strm->data_type = Z_UNKNOWN;

	s = ( deflate_state* )strm->state;
	s->pending = 0;
	s->pending_out = s->pending_buf;

	if ( s->wrap < 0 )
	{
		s->wrap = -s->wrap; /* was made negative by deflate(..., Z_FINISH); */
	}

	s->status = s->wrap ? INIT_STATE : BUSY_STATE;
	strm->adler =
#ifdef GZIP
	   s->wrap == 2 ? crc32( 0L, Z_NULL, 0 ) :
#endif
	   adler32( 0L, Z_NULL, 0 );
	s->last_flush = Z_NO_FLUSH;

	_tr_init( s );
	lm_init( s );

	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateSetHeader ( strm, head )
z_streamp strm;
gz_headerp head;
{
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	if ( strm->state->wrap != 2 ) { return Z_STREAM_ERROR; }

	strm->state->gzhead = head;
	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflatePrime ( strm, bits, value )
z_streamp strm;
int bits;
int value;
{
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	strm->state->bi_valid = bits;
	strm->state->bi_buf = ( ush )( value & ( ( 1 << bits ) - 1 ) );
	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateParams( strm, level, strategy )
z_streamp strm;
int level;
int strategy;
{
	deflate_state* s;
	compress_func func;
	int err = Z_OK;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	s = strm->state;

#ifdef FASTEST

	if ( level != 0 ) { level = 1; }

#else

	if ( level == Z_DEFAULT_COMPRESSION ) { level = 6; }

#endif

	if ( level < 0 || level > 9 || strategy < 0 || strategy > Z_FIXED )
	{
		return Z_STREAM_ERROR;
	}

	func = configuration_table[s->level].func;

	if ( ( strategy != s->strategy || func != configuration_table[level].func ) &&
	     strm->total_in != 0 )
	{
		/* Flush the last buffer: */
		err = deflate( strm, Z_BLOCK );
	}

	if ( s->level != level )
	{
		s->level = level;
		s->max_lazy_match   = configuration_table[level].max_lazy;
		s->good_match       = configuration_table[level].good_length;
		s->nice_match       = configuration_table[level].nice_length;
		s->max_chain_length = configuration_table[level].max_chain;
	}

	s->strategy = strategy;
	return err;
}

/* ========================================================================= */
int ZEXPORT deflateTune( strm, good_length, max_lazy, nice_length, max_chain )
z_streamp strm;
int good_length;
int max_lazy;
int nice_length;
int max_chain;
{
	deflate_state* s;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	s = strm->state;
	s->good_match = good_length;
	s->max_lazy_match = max_lazy;
	s->nice_match = nice_length;
	s->max_chain_length = max_chain;
	return Z_OK;
}

/* =========================================================================
 * For the default windowBits of 15 and memLevel of 8, this function returns
 * a close to exact, as well as small, upper bound on the compressed size.
 * They are coded as constants here for a reason--if the #define's are
 * changed, then this function needs to be changed as well.  The return
 * value for 15 and 8 only works for those exact settings.
 *
 * For any setting other than those defaults for windowBits and memLevel,
 * the value returned is a conservative worst case for the maximum expansion
 * resulting from using fixed blocks instead of stored blocks, which deflate
 * can emit on compressed data for some combinations of the parameters.
 *
 * This function could be more sophisticated to provide closer upper bounds for
 * every combination of windowBits and memLevel.  But even the conservative
 * upper bound of about 14% expansion does not seem onerous for output buffer
 * allocation.
 */
uLong ZEXPORT deflateBound( strm, sourceLen )
z_streamp strm;
uLong sourceLen;
{
	deflate_state* s;
	uLong complen, wraplen;
	Bytef* str;

	/* conservative upper bound for compressed data */
	complen = sourceLen +
	          ( ( sourceLen + 7 ) >> 3 ) + ( ( sourceLen + 63 ) >> 6 ) + 5;

	/* if can't get parameters, return conservative bound plus zlib wrapper */
	if ( strm == Z_NULL || strm->state == Z_NULL )
	{
		return complen + 6;
	}

	/* compute wrapper length */
	s = strm->state;

	switch ( s->wrap )
	{
		case 0:                                 /* raw deflate */
			wraplen = 0;
			break;

		case 1:                                 /* zlib wrapper */
			wraplen = 6 + ( s->strstart ? 4 : 0 );
			break;

		case 2:                                 /* gzip wrapper */
			wraplen = 18;

			if ( s->gzhead != Z_NULL )          /* user-supplied gzip header */
			{
				if ( s->gzhead->extra != Z_NULL )
				{
					wraplen += 2 + s->gzhead->extra_len;
				}

				str = s->gzhead->name;

				if ( str != Z_NULL )
					do
					{
						wraplen++;
					}
					while ( *str++ );

				str = s->gzhead->comment;

				if ( str != Z_NULL )
					do
					{
						wraplen++;
					}
					while ( *str++ );

				if ( s->gzhead->hcrc )
				{
					wraplen += 2;
				}
			}

			break;

		default:                                /* for compiler happiness */
			wraplen = 6;
	}

	/* if not default parameters, return conservative bound */
	if ( s->w_bits != 15 || s->hash_bits != 8 + 7 )
	{
		return complen + wraplen;
	}

	/* default settings: return tight bound for that case */
	return sourceLen + ( sourceLen >> 12 ) + ( sourceLen >> 14 ) +
	       ( sourceLen >> 25 ) + 13 - 6 + wraplen;
}

/* =========================================================================
 * Put a short in the pending buffer. The 16-bit value is put in MSB order.
 * IN assertion: the stream state is correct and there is enough room in
 * pending_buf.
 */
local void putShortMSB ( s, b )
deflate_state* s;
uInt b;
{
	put_byte( s, ( Byte )( b >> 8 ) );
	put_byte( s, ( Byte )( b & 0xff ) );
}

/* =========================================================================
 * Flush as much pending output as possible. All deflate() output goes
 * through this function so some applications may wish to modify it
 * to avoid allocating a large strm->next_out buffer and copying into it.
 * (See also read_buf()).
 */
local void flush_pending( strm )
z_streamp strm;
{
	unsigned len = strm->state->pending;

	if ( len > strm->avail_out ) { len = strm->avail_out; }

	if ( len == 0 ) { return; }

	zmemcpy( strm->next_out, strm->state->pending_out, len );
	strm->next_out  += len;
	strm->state->pending_out  += len;
	strm->total_out += len;
	strm->avail_out  -= len;
	strm->state->pending -= len;

	if ( strm->state->pending == 0 )
	{
		strm->state->pending_out = strm->state->pending_buf;
	}
}

/* ========================================================================= */
int ZEXPORT deflate ( strm, flush )
z_streamp strm;
int flush;
{
	int old_flush; /* value of flush param for previous deflate call */
	deflate_state* s;

	if ( strm == Z_NULL || strm->state == Z_NULL ||
	     flush > Z_BLOCK || flush < 0 )
	{
		return Z_STREAM_ERROR;
	}

	s = strm->state;

	if ( strm->next_out == Z_NULL ||
	     ( strm->next_in == Z_NULL && strm->avail_in != 0 ) ||
	     ( s->status == FINISH_STATE && flush != Z_FINISH ) )
	{
		ERR_RETURN( strm, Z_STREAM_ERROR );
	}

	if ( strm->avail_out == 0 ) { ERR_RETURN( strm, Z_BUF_ERROR ); }

	s->strm = strm; /* just in case */
	old_flush = s->last_flush;
	s->last_flush = flush;

	/* Write the header */
	if ( s->status == INIT_STATE )
	{
#ifdef GZIP

		if ( s->wrap == 2 )
		{
			strm->adler = crc32( 0L, Z_NULL, 0 );
			put_byte( s, 31 );
			put_byte( s, 139 );
			put_byte( s, 8 );

			if ( s->gzhead == Z_NULL )
			{
				put_byte( s, 0 );
				put_byte( s, 0 );
				put_byte( s, 0 );
				put_byte( s, 0 );
				put_byte( s, 0 );
				put_byte( s, s->level == 9 ? 2 :
				          ( s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
				            4 : 0 ) );
				put_byte( s, OS_CODE );
				s->status = BUSY_STATE;
			}
			else
			{
				put_byte( s, ( s->gzhead->text ? 1 : 0 ) +
				          ( s->gzhead->hcrc ? 2 : 0 ) +
				          ( s->gzhead->extra == Z_NULL ? 0 : 4 ) +
				          ( s->gzhead->name == Z_NULL ? 0 : 8 ) +
				          ( s->gzhead->comment == Z_NULL ? 0 : 16 )
				        );
				put_byte( s, ( Byte )( s->gzhead->time & 0xff ) );
				put_byte( s, ( Byte )( ( s->gzhead->time >> 8 ) & 0xff ) );
				put_byte( s, ( Byte )( ( s->gzhead->time >> 16 ) & 0xff ) );
				put_byte( s, ( Byte )( ( s->gzhead->time >> 24 ) & 0xff ) );
				put_byte( s, s->level == 9 ? 2 :
				          ( s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
				            4 : 0 ) );
				put_byte( s, s->gzhead->os & 0xff );

				if ( s->gzhead->extra != Z_NULL )
				{
					put_byte( s, s->gzhead->extra_len & 0xff );
					put_byte( s, ( s->gzhead->extra_len >> 8 ) & 0xff );
				}

				if ( s->gzhead->hcrc )
					strm->adler = crc32( strm->adler, s->pending_buf,
					                     s->pending );

				s->gzindex = 0;
				s->status = EXTRA_STATE;
			}
		}
		else
#endif
		{
			uInt header = ( Z_DEFLATED + ( ( s->w_bits - 8 ) << 4 ) ) << 8;
			uInt level_flags;

			if ( s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 )
			{
				level_flags = 0;
			}
			else if ( s->level < 6 )
			{
				level_flags = 1;
			}
			else if ( s->level == 6 )
			{
				level_flags = 2;
			}
			else
			{
				level_flags = 3;
			}

			header |= ( level_flags << 6 );

			if ( s->strstart != 0 ) { header |= PRESET_DICT; }

			header += 31 - ( header % 31 );

			s->status = BUSY_STATE;
			putShortMSB( s, header );

			/* Save the adler32 of the preset dictionary: */
			if ( s->strstart != 0 )
			{
				putShortMSB( s, ( uInt )( strm->adler >> 16 ) );
				putShortMSB( s, ( uInt )( strm->adler & 0xffff ) );
			}

			strm->adler = adler32( 0L, Z_NULL, 0 );
		}
	}

#ifdef GZIP

	if ( s->status == EXTRA_STATE )
	{
		if ( s->gzhead->extra != Z_NULL )
		{
			uInt beg = s->pending;  /* start of bytes to update crc */

			while ( s->gzindex < ( s->gzhead->extra_len & 0xffff ) )
			{
				if ( s->pending == s->pending_buf_size )
				{
					if ( s->gzhead->hcrc && s->pending > beg )
						strm->adler = crc32( strm->adler, s->pending_buf + beg,
						                     s->pending - beg );

					flush_pending( strm );
					beg = s->pending;

					if ( s->pending == s->pending_buf_size )
					{
						break;
					}
				}

				put_byte( s, s->gzhead->extra[s->gzindex] );
				s->gzindex++;
			}

			if ( s->gzhead->hcrc && s->pending > beg )
				strm->adler = crc32( strm->adler, s->pending_buf + beg,
				                     s->pending - beg );

			if ( s->gzindex == s->gzhead->extra_len )
			{
				s->gzindex = 0;
				s->status = NAME_STATE;
			}
		}
		else
		{
			s->status = NAME_STATE;
		}
	}

	if ( s->status == NAME_STATE )
	{
		if ( s->gzhead->name != Z_NULL )
		{
			uInt beg = s->pending;  /* start of bytes to update crc */
			int val;

			do
			{
				if ( s->pending == s->pending_buf_size )
				{
					if ( s->gzhead->hcrc && s->pending > beg )
						strm->adler = crc32( strm->adler, s->pending_buf + beg,
						                     s->pending - beg );

					flush_pending( strm );
					beg = s->pending;

					if ( s->pending == s->pending_buf_size )
					{
						val = 1;
						break;
					}
				}

				val = s->gzhead->name[s->gzindex++];
				put_byte( s, val );
			}
			while ( val != 0 );

			if ( s->gzhead->hcrc && s->pending > beg )
				strm->adler = crc32( strm->adler, s->pending_buf + beg,
				                     s->pending - beg );

			if ( val == 0 )
			{
				s->gzindex = 0;
				s->status = COMMENT_STATE;
			}
		}
		else
		{
			s->status = COMMENT_STATE;
		}
	}

	if ( s->status == COMMENT_STATE )
	{
		if ( s->gzhead->comment != Z_NULL )
		{
			uInt beg = s->pending;  /* start of bytes to update crc */
			int val;

			do
			{
				if ( s->pending == s->pending_buf_size )
				{
					if ( s->gzhead->hcrc && s->pending > beg )
						strm->adler = crc32( strm->adler, s->pending_buf + beg,
						                     s->pending - beg );

					flush_pending( strm );
					beg = s->pending;

					if ( s->pending == s->pending_buf_size )
					{
						val = 1;
						break;
					}
				}

				val = s->gzhead->comment[s->gzindex++];
				put_byte( s, val );
			}
			while ( val != 0 );

			if ( s->gzhead->hcrc && s->pending > beg )
				strm->adler = crc32( strm->adler, s->pending_buf + beg,
				                     s->pending - beg );

			if ( val == 0 )
			{
				s->status = HCRC_STATE;
			}
		}
		else
		{
			s->status = HCRC_STATE;
		}
	}

	if ( s->status == HCRC_STATE )
	{
		if ( s->gzhead->hcrc )
		{
			if ( s->pending + 2 > s->pending_buf_size )
			{
				flush_pending( strm );
			}

			if ( s->pending + 2 <= s->pending_buf_size )
			{
				put_byte( s, ( Byte )( strm->adler & 0xff ) );
				put_byte( s, ( Byte )( ( strm->adler >> 8 ) & 0xff ) );
				strm->adler = crc32( 0L, Z_NULL, 0 );
				s->status = BUSY_STATE;
			}
		}
		else
		{
			s->status = BUSY_STATE;
		}
	}

#endif

	/* Flush as much pending output as possible */
	if ( s->pending != 0 )
	{
		flush_pending( strm );

		if ( strm->avail_out == 0 )
		{
			/* Since avail_out is 0, deflate will be called again with
			 * more output space, but possibly with both pending and
			 * avail_in equal to zero. There won't be anything to do,
			 * but this is not an error situation so make sure we
			 * return OK instead of BUF_ERROR at next call of deflate:
			 */
			s->last_flush = -1;
			return Z_OK;
		}

		/* Make sure there is something to do and avoid duplicate consecutive
		 * flushes. For repeated and useless calls with Z_FINISH, we keep
		 * returning Z_STREAM_END instead of Z_BUF_ERROR.
		 */
	}
	else if ( strm->avail_in == 0 && flush <= old_flush &&
	          flush != Z_FINISH )
	{
		ERR_RETURN( strm, Z_BUF_ERROR );
	}

	/* User must not provide more input after the first FINISH: */
	if ( s->status == FINISH_STATE && strm->avail_in != 0 )
	{
		ERR_RETURN( strm, Z_BUF_ERROR );
	}

	/* Start a new block or continue the current one.
	 */
	if ( strm->avail_in != 0 || s->lookahead != 0 ||
	     ( flush != Z_NO_FLUSH && s->status != FINISH_STATE ) )
	{
		block_state bstate;

		bstate = s->strategy == Z_HUFFMAN_ONLY ? deflate_huff( s, flush ) :
		         ( s->strategy == Z_RLE ? deflate_rle( s, flush ) :
		           ( *( configuration_table[s->level].func ) )( s, flush ) );

		if ( bstate == finish_started || bstate == finish_done )
		{
			s->status = FINISH_STATE;
		}

		if ( bstate == need_more || bstate == finish_started )
		{
			if ( strm->avail_out == 0 )
			{
				s->last_flush = -1; /* avoid BUF_ERROR next call, see above */
			}

			return Z_OK;
			/* If flush != Z_NO_FLUSH && avail_out == 0, the next call
			 * of deflate should use the same flush parameter to make sure
			 * that the flush is complete. So we don't have to output an
			 * empty block here, this will be done at next call. This also
			 * ensures that for a very small output buffer, we emit at most
			 * one empty block.
			 */
		}

		if ( bstate == block_done )
		{
			if ( flush == Z_PARTIAL_FLUSH )
			{
				_tr_align( s );
			}
			else if ( flush != Z_BLOCK )   /* FULL_FLUSH or SYNC_FLUSH */
			{
				_tr_stored_block( s, ( char* )0, 0L, 0 );

				/* For a full flush, this empty block will be recognized
				 * as a special marker by inflate_sync().
				 */
				if ( flush == Z_FULL_FLUSH )
				{
					CLEAR_HASH( s );           /* forget history */

					if ( s->lookahead == 0 )
					{
						s->strstart = 0;
						s->block_start = 0L;
					}
				}
			}

			flush_pending( strm );

			if ( strm->avail_out == 0 )
			{
				s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
				return Z_OK;
			}
		}
	}

	Assert( strm->avail_out > 0, "bug2" );

	if ( flush != Z_FINISH ) { return Z_OK; }

	if ( s->wrap <= 0 ) { return Z_STREAM_END; }

	/* Write the trailer */
#ifdef GZIP

	if ( s->wrap == 2 )
	{
		put_byte( s, ( Byte )( strm->adler & 0xff ) );
		put_byte( s, ( Byte )( ( strm->adler >> 8 ) & 0xff ) );
		put_byte( s, ( Byte )( ( strm->adler >> 16 ) & 0xff ) );
		put_byte( s, ( Byte )( ( strm->adler >> 24 ) & 0xff ) );
		put_byte( s, ( Byte )( strm->total_in & 0xff ) );
		put_byte( s, ( Byte )( ( strm->total_in >> 8 ) & 0xff ) );
		put_byte( s, ( Byte )( ( strm->total_in >> 16 ) & 0xff ) );
		put_byte( s, ( Byte )( ( strm->total_in >> 24 ) & 0xff ) );
	}
	else
#endif
	{
		putShortMSB( s, ( uInt )( strm->adler >> 16 ) );
		putShortMSB( s, ( uInt )( strm->adler & 0xffff ) );
	}

	flush_pending( strm );

	/* If avail_out is zero, the application will call deflate again
	 * to flush the rest.
	 */
	if ( s->wrap > 0 ) { s->wrap = -s->wrap; } /* write the trailer only once! */

	return s->pending != 0 ? Z_OK : Z_STREAM_END;
}

/* ========================================================================= */
int ZEXPORT deflateEnd ( strm )
z_streamp strm;
{
	int status;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	status = strm->state->status;

	if ( status != INIT_STATE &&
	     status != EXTRA_STATE &&
	     status != NAME_STATE &&
	     status != COMMENT_STATE &&
	     status != HCRC_STATE &&
	     status != BUSY_STATE &&
	     status != FINISH_STATE )
	{
		return Z_STREAM_ERROR;
	}

	/* Deallocate in reverse order of allocations: */
	TRY_FREE( strm, strm->state->pending_buf );
	TRY_FREE( strm, strm->state->head );
	TRY_FREE( strm, strm->state->prev );
	TRY_FREE( strm, strm->state->window );

	ZFREE( strm, strm->state );
	strm->state = Z_NULL;

	return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

/* =========================================================================
 * Copy the source state to the destination state.
 * To simplify the source, this is not supported for 16-bit MSDOS (which
 * doesn't have enough memory anyway to duplicate compression states).
 */
int ZEXPORT deflateCopy ( dest, source )
z_streamp dest;
z_streamp source;
{
#ifdef MAXSEG_64K
	return Z_STREAM_ERROR;
#else
	deflate_state* ds;
	deflate_state* ss;
	ushf* overlay;


	if ( source == Z_NULL || dest == Z_NULL || source->state == Z_NULL )
	{
		return Z_STREAM_ERROR;
	}

	ss = source->state;

	zmemcpy( dest, source, sizeof( z_stream ) );

	ds = ( deflate_state* ) ZALLOC( dest, 1, sizeof( deflate_state ) );

	if ( ds == Z_NULL ) { return Z_MEM_ERROR; }

	dest->state = ( struct internal_state FAR* ) ds;
	zmemcpy( ds, ss, sizeof( deflate_state ) );
	ds->strm = dest;

	ds->window = ( Bytef* ) ZALLOC( dest, ds->w_size, 2 * sizeof( Byte ) );
	ds->prev   = ( Posf* )  ZALLOC( dest, ds->w_size, sizeof( Pos ) );
	ds->head   = ( Posf* )  ZALLOC( dest, ds->hash_size, sizeof( Pos ) );
	overlay = ( ushf* ) ZALLOC( dest, ds->lit_bufsize, sizeof( ush ) + 2 );
	ds->pending_buf = ( uchf* ) overlay;

	if ( ds->window == Z_NULL || ds->prev == Z_NULL || ds->head == Z_NULL ||
	     ds->pending_buf == Z_NULL )
	{
		deflateEnd ( dest );
		return Z_MEM_ERROR;
	}

	/* following zmemcpy do not work for 16-bit MSDOS */
	zmemcpy( ds->window, ss->window, ds->w_size * 2 * sizeof( Byte ) );
	zmemcpy( ds->prev, ss->prev, ds->w_size * sizeof( Pos ) );
	zmemcpy( ds->head, ss->head, ds->hash_size * sizeof( Pos ) );
	zmemcpy( ds->pending_buf, ss->pending_buf, ( uInt )ds->pending_buf_size );

	ds->pending_out = ds->pending_buf + ( ss->pending_out - ss->pending_buf );
	ds->d_buf = overlay + ds->lit_bufsize / sizeof( ush );
	ds->l_buf = ds->pending_buf + ( 1 + sizeof( ush ) ) * ds->lit_bufsize;

	ds->l_desc.dyn_tree = ds->dyn_ltree;
	ds->d_desc.dyn_tree = ds->dyn_dtree;
	ds->bl_desc.dyn_tree = ds->bl_tree;

	return Z_OK;
#endif /* MAXSEG_64K */
}

/* ===========================================================================
 * Read a new buffer from the current input stream, update the adler32
 * and total number of bytes read.  All deflate() input goes through
 * this function so some applications may wish to modify it to avoid
 * allocating a large strm->next_in buffer and copying from it.
 * (See also flush_pending()).
 */
local int read_buf( strm, buf, size )
z_streamp strm;
Bytef* buf;
unsigned size;
{
	unsigned len = strm->avail_in;

	if ( len > size ) { len = size; }

	if ( len == 0 ) { return 0; }

	strm->avail_in  -= len;

	if ( strm->state->wrap == 1 )
	{
		strm->adler = adler32( strm->adler, strm->next_in, len );
	}

#ifdef GZIP
	else if ( strm->state->wrap == 2 )
	{
		strm->adler = crc32( strm->adler, strm->next_in, len );
	}

#endif
	zmemcpy( buf, strm->next_in, len );
	strm->next_in  += len;
	strm->total_in += len;

	return ( int )len;
}

/* ===========================================================================
 * Initialize the "longest match" routines for a new zlib stream
 */
local void lm_init ( s )
deflate_state* s;
{
	s->window_size = ( ulg )2L * s->w_size;

	CLEAR_HASH( s );

	/* Set the default configuration parameters:
	 */
	s->max_lazy_match   = configuration_table[s->level].max_lazy;
	s->good_match       = configuration_table[s->level].good_length;
	s->nice_match       = configuration_table[s->level].nice_length;
	s->max_chain_length = configuration_table[s->level].max_chain;

	s->strstart = 0;
	s->block_start = 0L;
	s->lookahead = 0;
	s->match_length = s->prev_length = MIN_MATCH - 1;
	s->match_available = 0;
	s->ins_h = 0;
#ifndef FASTEST
#ifdef ASMV
	match_init(); /* initialize the asm code */
#endif
#endif
}

#ifndef FASTEST
/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 * OUT assertion: the match length is not greater than s->lookahead.
 */
#ifndef ASMV
/* For 80x86 and 680x0, an optimized version will be provided in match.asm or
 * match.S. The code will be functionally equivalent.
 */
local uInt longest_match( s, cur_match )
deflate_state* s;
IPos cur_match;                             /* current match */
{
	unsigned chain_length = s->max_chain_length;/* max hash chain length */
	register Bytef* scan = s->window + s->strstart; /* current string */
	register Bytef* match;                       /* matched string */
	register int len;                           /* length of current match */
	int best_len = s->prev_length;              /* best match length so far */
	int nice_match = s->nice_match;             /* stop if match long enough */
	IPos limit = s->strstart > ( IPos )MAX_DIST( s ) ?
	             s->strstart - ( IPos )MAX_DIST( s ) : NIL;
	/* Stop when cur_match becomes <= limit. To simplify the code,
	 * we prevent matches with the string of window index 0.
	 */
	Posf* prev = s->prev;
	uInt wmask = s->w_mask;

#ifdef UNALIGNED_OK
	/* Compare two bytes at a time. Note: this is not always beneficial.
	 * Try with and without -DUNALIGNED_OK to check.
	 */
	register Bytef* strend = s->window + s->strstart + MAX_MATCH - 1;
	register ush scan_start = *( ushf* )scan;
	register ush scan_end   = *( ushf* )( scan + best_len - 1 );
#else
	register Bytef* strend = s->window + s->strstart + MAX_MATCH;
	register Byte scan_end1  = scan[best_len - 1];
	register Byte scan_end   = scan[best_len];
#endif

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert( s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever" );

	/* Do not waste too much time if we already have a good match: */
	if ( s->prev_length >= s->good_match )
	{
		chain_length >>= 2;
	}

	/* Do not look for matches beyond the end of the input. This is necessary
	 * to make deflate deterministic.
	 */
	if ( ( uInt )nice_match > s->lookahead ) { nice_match = s->lookahead; }

	Assert( ( ulg )s->strstart <= s->window_size - MIN_LOOKAHEAD, "need lookahead" );

	do
	{
		Assert( cur_match < s->strstart, "no future" );
		match = s->window + cur_match;

		/* Skip to next match if the match length cannot increase
		 * or if the match length is less than 2.  Note that the checks below
		 * for insufficient lookahead only occur occasionally for performance
		 * reasons.  Therefore uninitialized memory will be accessed, and
		 * conditional jumps will be made that depend on those values.
		 * However the length of the match is limited to the lookahead, so
		 * the output of deflate is not affected by the uninitialized values.
		 */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)

		/* This code assumes sizeof(unsigned short) == 2. Do not use
		 * UNALIGNED_OK if your compiler uses a different size.
		 */
		if ( *( ushf* )( match + best_len - 1 ) != scan_end ||
		     *( ushf* )match != scan_start ) { continue; }

		/* It is not necessary to compare scan[2] and match[2] since they are
		 * always equal when the other bytes match, given that the hash keys
		 * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
		 * strstart+3, +5, ... up to strstart+257. We check for insufficient
		 * lookahead only every 4th comparison; the 128th check will be made
		 * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
		 * necessary to put more guard bytes at the end of the window, or
		 * to check more often for insufficient lookahead.
		 */
		Assert( scan[2] == match[2], "scan[2]?" );
		scan++, match++;

		do
		{
		}
		while ( *( ushf* )( scan += 2 ) == *( ushf* )( match += 2 ) &&
		        *( ushf* )( scan += 2 ) == *( ushf* )( match += 2 ) &&
		        *( ushf* )( scan += 2 ) == *( ushf* )( match += 2 ) &&
		        *( ushf* )( scan += 2 ) == *( ushf* )( match += 2 ) &&
		        scan < strend );

		/* The funny "do {}" generates better code on most compilers */

		/* Here, scan <= window+strstart+257 */
		Assert( scan <= s->window + ( unsigned )( s->window_size - 1 ), "wild scan" );

		if ( *scan == *match ) { scan++; }

		len = ( MAX_MATCH - 1 ) - ( int )( strend - scan );
		scan = strend - ( MAX_MATCH - 1 );

#else /* UNALIGNED_OK */

		if ( match[best_len]   != scan_end  ||
		     match[best_len - 1] != scan_end1 ||
		     *match            != *scan     ||
		     *++match          != scan[1] ) { continue; }

		/* The check at best_len-1 can be removed because it will be made
		 * again later. (This heuristic is not always a win.)
		 * It is not necessary to compare scan[2] and match[2] since they
		 * are always equal when the other bytes match, given that
		 * the hash keys are equal and that HASH_BITS >= 8.
		 */
		scan += 2, match++;
		Assert( *scan == *match, "match[2]?" );

		/* We check for insufficient lookahead only every 8th comparison;
		 * the 256th check will be made at strstart+258.
		 */
		do
		{
		}
		while ( *++scan == *++match && *++scan == *++match &&
		        *++scan == *++match && *++scan == *++match &&
		        *++scan == *++match && *++scan == *++match &&
		        *++scan == *++match && *++scan == *++match &&
		        scan < strend );

		Assert( scan <= s->window + ( unsigned )( s->window_size - 1 ), "wild scan" );

		len = MAX_MATCH - ( int )( strend - scan );
		scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

		if ( len > best_len )
		{
			s->match_start = cur_match;
			best_len = len;

			if ( len >= nice_match ) { break; }

#ifdef UNALIGNED_OK
			scan_end = *( ushf* )( scan + best_len - 1 );
#else
			scan_end1  = scan[best_len - 1];
			scan_end   = scan[best_len];
#endif
		}
	}
	while ( ( cur_match = prev[cur_match & wmask] ) > limit
	        && --chain_length != 0 );

	if ( ( uInt )best_len <= s->lookahead ) { return ( uInt )best_len; }

	return s->lookahead;
}
#endif /* ASMV */

#else /* FASTEST */

/* ---------------------------------------------------------------------------
 * Optimized version for FASTEST only
 */
local uInt longest_match( s, cur_match )
deflate_state* s;
IPos cur_match;                             /* current match */
{
	register Bytef* scan = s->window + s->strstart; /* current string */
	register Bytef* match;                       /* matched string */
	register int len;                           /* length of current match */
	register Bytef* strend = s->window + s->strstart + MAX_MATCH;

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert( s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever" );

	Assert( ( ulg )s->strstart <= s->window_size - MIN_LOOKAHEAD, "need lookahead" );

	Assert( cur_match < s->strstart, "no future" );

	match = s->window + cur_match;

	/* Return failure if the match length is less than 2:
	 */
	if ( match[0] != scan[0] || match[1] != scan[1] ) { return MIN_MATCH - 1; }

	/* The check at best_len-1 can be removed because it will be made
	 * again later. (This heuristic is not always a win.)
	 * It is not necessary to compare scan[2] and match[2] since they
	 * are always equal when the other bytes match, given that
	 * the hash keys are equal and that HASH_BITS >= 8.
	 */
	scan += 2, match += 2;
	Assert( *scan == *match, "match[2]?" );

	/* We check for insufficient lookahead only every 8th comparison;
	 * the 256th check will be made at strstart+258.
	 */
	do
	{
	}
	while ( *++scan == *++match && *++scan == *++match &&
	        *++scan == *++match && *++scan == *++match &&
	        *++scan == *++match && *++scan == *++match &&
	        *++scan == *++match && *++scan == *++match &&
	        scan < strend );

	Assert( scan <= s->window + ( unsigned )( s->window_size - 1 ), "wild scan" );

	len = MAX_MATCH - ( int )( strend - scan );

	if ( len < MIN_MATCH ) { return MIN_MATCH - 1; }

	s->match_start = cur_match;
	return ( uInt )len <= s->lookahead ? ( uInt )len : s->lookahead;
}

#endif /* FASTEST */

#ifdef DEBUG
/* ===========================================================================
 * Check that the match at match_start is indeed a match.
 */
local void check_match( s, start, match, length )
deflate_state* s;
IPos start, match;
int length;
{
	/* check that the match is indeed a match */
	if ( zmemcmp( s->window + match,
	              s->window + start, length ) != EQUAL )
	{
		fprintf( stderr, " start %u, match %u, length %d\n",
		         start, match, length );

		do
		{
			fprintf( stderr, "%c%c", s->window[match++], s->window[start++] );
		}
		while ( --length != 0 );

		z_error( "invalid match" );
	}

	if ( z_verbose > 1 )
	{
		fprintf( stderr, "\\[%d,%d]", start - match, length );

		do { putc( s->window[start++], stderr ); }
		while ( --length != 0 );
	}
}
#else
#  define check_match(s, start, match, length)
#endif /* DEBUG */

/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead.
 *
 * IN assertion: lookahead < MIN_LOOKAHEAD
 * OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
 *    At least one byte has been read, or avail_in == 0; reads are
 *    performed for at least two bytes (required for the zip translate_eol
 *    option -- not supported here).
 */
local void fill_window( s )
deflate_state* s;
{
	register unsigned n, m;
	register Posf* p;
	unsigned more;    /* Amount of free space at the end of the window. */
	uInt wsize = s->w_size;

	do
	{
		more = ( unsigned )( s->window_size - ( ulg )s->lookahead - ( ulg )s->strstart );

		/* Deal with !@#$% 64K limit: */
		if ( sizeof( int ) <= 2 )
		{
			if ( more == 0 && s->strstart == 0 && s->lookahead == 0 )
			{
				more = wsize;

			}
			else if ( more == ( unsigned )( -1 ) )
			{
				/* Very unlikely, but possible on 16 bit machine if
				 * strstart == 0 && lookahead == 1 (input done a byte at time)
				 */
				more--;
			}
		}

		/* If the window is almost full and there is insufficient lookahead,
		 * move the upper half to the lower one to make room in the upper half.
		 */
		if ( s->strstart >= wsize + MAX_DIST( s ) )
		{

			zmemcpy( s->window, s->window + wsize, ( unsigned )wsize );
			s->match_start -= wsize;
			s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
			s->block_start -= ( long ) wsize;

			/* Slide the hash table (could be avoided with 32 bit values
			   at the expense of memory usage). We slide even when level == 0
			   to keep the hash table consistent if we switch back to level > 0
			   later. (Using level 0 permanently is not an optimal usage of
			   zlib, so we don't care about this pathological case.)
			 */
			n = s->hash_size;
			p = &s->head[n];

			do
			{
				m = *--p;
				*p = ( Pos )( m >= wsize ? m - wsize : NIL );
			}
			while ( --n );

			n = wsize;
#ifndef FASTEST
			p = &s->prev[n];

			do
			{
				m = *--p;
				*p = ( Pos )( m >= wsize ? m - wsize : NIL );
				/* If n is not on any hash chain, prev[n] is garbage but
				 * its value will never be used.
				 */
			}
			while ( --n );

#endif
			more += wsize;
		}

		if ( s->strm->avail_in == 0 ) { return; }

		/* If there was no sliding:
		 *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
		 *    more == window_size - lookahead - strstart
		 * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
		 * => more >= window_size - 2*WSIZE + 2
		 * In the BIG_MEM or MMAP case (not yet supported),
		 *   window_size == input_size + MIN_LOOKAHEAD  &&
		 *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
		 * Otherwise, window_size == 2*WSIZE so more >= 2.
		 * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
		 */
		Assert( more >= 2, "more < 2" );

		n = read_buf( s->strm, s->window + s->strstart + s->lookahead, more );
		s->lookahead += n;

		/* Initialize the hash value now that we have some input: */
		if ( s->lookahead >= MIN_MATCH )
		{
			s->ins_h = s->window[s->strstart];
			UPDATE_HASH( s, s->ins_h, s->window[s->strstart + 1] );
#if MIN_MATCH != 3
			Call UPDATE_HASH() MIN_MATCH - 3 more times
#endif
		}

		/* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
		 * but this is not important since only literal bytes will be emitted.
		 */

	}
	while ( s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0 );

	/* If the WIN_INIT bytes after the end of the current data have never been
	 * written, then zero those bytes in order to avoid memory check reports of
	 * the use of uninitialized (or uninitialised as Julian writes) bytes by
	 * the longest match routines.  Update the high water mark for the next
	 * time through here.  WIN_INIT is set to MAX_MATCH since the longest match
	 * routines allow scanning to strstart + MAX_MATCH, ignoring lookahead.
	 */
	if ( s->high_water < s->window_size )
	{
		ulg curr = s->strstart + ( ulg )( s->lookahead );
		ulg init;

		if ( s->high_water < curr )
		{
			/* Previous high water mark below current data -- zero WIN_INIT
			 * bytes or up to end of window, whichever is less.
			 */
			init = s->window_size - curr;

			if ( init > WIN_INIT )
			{
				init = WIN_INIT;
			}

			zmemzero( s->window + curr, ( unsigned )init );
			s->high_water = curr + init;
		}
		else if ( s->high_water < ( ulg )curr + WIN_INIT )
		{
			/* High water mark at or above current data, but below current data
			 * plus WIN_INIT -- zero out to current data plus WIN_INIT, or up
			 * to end of window, whichever is less.
			 */
			init = ( ulg )curr + WIN_INIT - s->high_water;

			if ( init > s->window_size - s->high_water )
			{
				init = s->window_size - s->high_water;
			}

			zmemzero( s->window + s->high_water, ( unsigned )init );
			s->high_water += init;
		}
	}
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */
#define FLUSH_BLOCK_ONLY(s, last) { \
   _tr_flush_block(s, (s->block_start >= 0L ? \
                   (charf *)&s->window[(unsigned)s->block_start] : \
                   (charf *)Z_NULL), \
                (ulg)((long)s->strstart - s->block_start), \
                (last)); \
   s->block_start = s->strstart; \
   flush_pending(s->strm); \
   Tracev((stderr,"[FLUSH]")); \
}

/* Same but force premature exit if necessary. */
#define FLUSH_BLOCK(s, last) { \
   FLUSH_BLOCK_ONLY(s, last); \
   if (s->strm->avail_out == 0) return (last) ? finish_started : need_more; \
}

/* ===========================================================================
 * Copy without compression as much as possible from the input stream, return
 * the current block state.
 * This function does not insert new strings in the dictionary since
 * uncompressible data is probably not useful. This function is used
 * only for the level=0 compression option.
 * NOTE: this function should be optimized to avoid extra copying from
 * window to pending_buf.
 */
local block_state deflate_stored( s, flush )
deflate_state* s;
int flush;
{
	/* Stored blocks are limited to 0xffff bytes, pending_buf is limited
	 * to pending_buf_size, and each stored block has a 5 byte header:
	 */
	ulg max_block_size = 0xffff;
	ulg max_start;

	if ( max_block_size > s->pending_buf_size - 5 )
	{
		max_block_size = s->pending_buf_size - 5;
	}

	/* Copy as much as possible from input to output: */
	for ( ;; )
	{
		/* Fill the window as much as possible: */
		if ( s->lookahead <= 1 )
		{

			Assert( s->strstart < s->w_size + MAX_DIST( s ) ||
			        s->block_start >= ( long )s->w_size, "slide too late" );

			fill_window( s );

			if ( s->lookahead == 0 && flush == Z_NO_FLUSH ) { return need_more; }

			if ( s->lookahead == 0 ) { break; } /* flush the current block */
		}

		Assert( s->block_start >= 0L, "block gone" );

		s->strstart += s->lookahead;
		s->lookahead = 0;

		/* Emit a stored block if pending_buf will be full: */
		max_start = s->block_start + max_block_size;

		if ( s->strstart == 0 || ( ulg )s->strstart >= max_start )
		{
			/* strstart == 0 is possible when wraparound on 16-bit machine */
			s->lookahead = ( uInt )( s->strstart - max_start );
			s->strstart = ( uInt )max_start;
			FLUSH_BLOCK( s, 0 );
		}

		/* Flush if we may have to slide, otherwise block_start may become
		 * negative and the data will be gone:
		 */
		if ( s->strstart - ( uInt )s->block_start >= MAX_DIST( s ) )
		{
			FLUSH_BLOCK( s, 0 );
		}
	}

	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

/* ===========================================================================
 * Compress as much as possible from the input stream, return the current
 * block state.
 * This function does not perform lazy evaluation of matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
local block_state deflate_fast( s, flush )
deflate_state* s;
int flush;
{
	IPos hash_head;       /* head of the hash chain */
	int bflush;           /* set if current block must be flushed */

	for ( ;; )
	{
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if ( s->lookahead < MIN_LOOKAHEAD )
		{
			fill_window( s );

			if ( s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH )
			{
				return need_more;
			}

			if ( s->lookahead == 0 ) { break; } /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		hash_head = NIL;

		if ( s->lookahead >= MIN_MATCH )
		{
			INSERT_STRING( s, s->strstart, hash_head );
		}

		/* Find the longest match, discarding those <= prev_length.
		 * At this point we have always match_length < MIN_MATCH
		 */
		if ( hash_head != NIL && s->strstart - hash_head <= MAX_DIST( s ) )
		{
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */
			s->match_length = longest_match ( s, hash_head );
			/* longest_match() sets match_start */
		}

		if ( s->match_length >= MIN_MATCH )
		{
			check_match( s, s->strstart, s->match_start, s->match_length );

			_tr_tally_dist( s, s->strstart - s->match_start,
			                s->match_length - MIN_MATCH, bflush );

			s->lookahead -= s->match_length;

			/* Insert new strings in the hash table only if the match length
			 * is not too large. This saves time but degrades compression.
			 */
#ifndef FASTEST

			if ( s->match_length <= s->max_insert_length &&
			     s->lookahead >= MIN_MATCH )
			{
				s->match_length--; /* string at strstart already in table */

				do
				{
					s->strstart++;
					INSERT_STRING( s, s->strstart, hash_head );
					/* strstart never exceeds WSIZE-MAX_MATCH, so there are
					 * always MIN_MATCH bytes ahead.
					 */
				}
				while ( --s->match_length != 0 );

				s->strstart++;
			}
			else
#endif
			{
				s->strstart += s->match_length;
				s->match_length = 0;
				s->ins_h = s->window[s->strstart];
				UPDATE_HASH( s, s->ins_h, s->window[s->strstart + 1] );
#if MIN_MATCH != 3
				Call UPDATE_HASH() MIN_MATCH - 3 more times
#endif
				/* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
				 * matter since it will be recomputed at next deflate call.
				 */
			}
		}
		else
		{
			/* No match, output a literal byte */
			Tracevv( ( stderr, "%c", s->window[s->strstart] ) );
			_tr_tally_lit ( s, s->window[s->strstart], bflush );
			s->lookahead--;
			s->strstart++;
		}

		if ( bflush ) { FLUSH_BLOCK( s, 0 ); }
	}

	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

#ifndef FASTEST
/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
local block_state deflate_slow( s, flush )
deflate_state* s;
int flush;
{
	IPos hash_head;          /* head of hash chain */
	int bflush;              /* set if current block must be flushed */

	/* Process the input block. */
	for ( ;; )
	{
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if ( s->lookahead < MIN_LOOKAHEAD )
		{
			fill_window( s );

			if ( s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH )
			{
				return need_more;
			}

			if ( s->lookahead == 0 ) { break; } /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		hash_head = NIL;

		if ( s->lookahead >= MIN_MATCH )
		{
			INSERT_STRING( s, s->strstart, hash_head );
		}

		/* Find the longest match, discarding those <= prev_length.
		 */
		s->prev_length = s->match_length, s->prev_match = s->match_start;
		s->match_length = MIN_MATCH - 1;

		if ( hash_head != NIL && s->prev_length < s->max_lazy_match &&
		     s->strstart - hash_head <= MAX_DIST( s ) )
		{
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */
			s->match_length = longest_match ( s, hash_head );
			/* longest_match() sets match_start */

			if ( s->match_length <= 5 && ( s->strategy == Z_FILTERED
#if TOO_FAR <= 32767
			                               || ( s->match_length == MIN_MATCH &&
			                                    s->strstart - s->match_start > TOO_FAR )
#endif
			                             ) )
			{

				/* If prev_match is also MIN_MATCH, match_start is garbage
				 * but we will ignore the current match anyway.
				 */
				s->match_length = MIN_MATCH - 1;
			}
		}

		/* If there was a match at the previous step and the current
		 * match is not better, output the previous match:
		 */
		if ( s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length )
		{
			uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
			/* Do not insert strings in hash table beyond this. */

			check_match( s, s->strstart - 1, s->prev_match, s->prev_length );

			_tr_tally_dist( s, s->strstart - 1 - s->prev_match,
			                s->prev_length - MIN_MATCH, bflush );

			/* Insert in hash table all strings up to the end of the match.
			 * strstart-1 and strstart are already inserted. If there is not
			 * enough lookahead, the last two strings are not inserted in
			 * the hash table.
			 */
			s->lookahead -= s->prev_length - 1;
			s->prev_length -= 2;

			do
			{
				if ( ++s->strstart <= max_insert )
				{
					INSERT_STRING( s, s->strstart, hash_head );
				}
			}
			while ( --s->prev_length != 0 );

			s->match_available = 0;
			s->match_length = MIN_MATCH - 1;
			s->strstart++;

			if ( bflush ) { FLUSH_BLOCK( s, 0 ); }

		}
		else if ( s->match_available )
		{
			/* If there was no match at the previous position, output a
			 * single literal. If there was a match but the current match
			 * is longer, truncate the previous match to a single literal.
			 */
			Tracevv( ( stderr, "%c", s->window[s->strstart - 1] ) );
			_tr_tally_lit( s, s->window[s->strstart - 1], bflush );

			if ( bflush )
			{
				FLUSH_BLOCK_ONLY( s, 0 );
			}

			s->strstart++;
			s->lookahead--;

			if ( s->strm->avail_out == 0 ) { return need_more; }
		}
		else
		{
			/* There is no previous match to compare with, wait for
			 * the next step to decide.
			 */
			s->match_available = 1;
			s->strstart++;
			s->lookahead--;
		}
	}

	Assert ( flush != Z_NO_FLUSH, "no flush?" );

	if ( s->match_available )
	{
		Tracevv( ( stderr, "%c", s->window[s->strstart - 1] ) );
		_tr_tally_lit( s, s->window[s->strstart - 1], bflush );
		s->match_available = 0;
	}

	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}
#endif /* FASTEST */

/* ===========================================================================
 * For Z_RLE, simply look for runs of bytes, generate matches only of distance
 * one.  Do not maintain a hash table.  (It will be regenerated if this run of
 * deflate switches away from Z_RLE.)
 */
local block_state deflate_rle( s, flush )
deflate_state* s;
int flush;
{
	int bflush;             /* set if current block must be flushed */
	uInt prev;              /* byte at distance one to match */
	Bytef* scan, *strend;   /* scan goes up to strend for length of run */

	for ( ;; )
	{
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the longest encodable run.
		 */
		if ( s->lookahead < MAX_MATCH )
		{
			fill_window( s );

			if ( s->lookahead < MAX_MATCH && flush == Z_NO_FLUSH )
			{
				return need_more;
			}

			if ( s->lookahead == 0 ) { break; } /* flush the current block */
		}

		/* See how many times the previous byte repeats */
		s->match_length = 0;

		if ( s->lookahead >= MIN_MATCH && s->strstart > 0 )
		{
			scan = s->window + s->strstart - 1;
			prev = *scan;

			if ( prev == *++scan && prev == *++scan && prev == *++scan )
			{
				strend = s->window + s->strstart + MAX_MATCH;

				do
				{
				}
				while ( prev == *++scan && prev == *++scan &&
				        prev == *++scan && prev == *++scan &&
				        prev == *++scan && prev == *++scan &&
				        prev == *++scan && prev == *++scan &&
				        scan < strend );

				s->match_length = MAX_MATCH - ( int )( strend - scan );

				if ( s->match_length > s->lookahead )
				{
					s->match_length = s->lookahead;
				}
			}
		}

		/* Emit match if have run of MIN_MATCH or longer, else emit literal */
		if ( s->match_length >= MIN_MATCH )
		{
			check_match( s, s->strstart, s->strstart - 1, s->match_length );

			_tr_tally_dist( s, 1, s->match_length - MIN_MATCH, bflush );

			s->lookahead -= s->match_length;
			s->strstart += s->match_length;
			s->match_length = 0;
		}
		else
		{
			/* No match, output a literal byte */
			Tracevv( ( stderr, "%c", s->window[s->strstart] ) );
			_tr_tally_lit ( s, s->window[s->strstart], bflush );
			s->lookahead--;
			s->strstart++;
		}

		if ( bflush ) { FLUSH_BLOCK( s, 0 ); }
	}

	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

/* ===========================================================================
 * For Z_HUFFMAN_ONLY, do not look for matches.  Do not maintain a hash table.
 * (It will be regenerated if this run of deflate switches away from Huffman.)
 */
local block_state deflate_huff( s, flush )
deflate_state* s;
int flush;
{
	int bflush;             /* set if current block must be flushed */

	for ( ;; )
	{
		/* Make sure that we have a literal to write. */
		if ( s->lookahead == 0 )
		{
			fill_window( s );

			if ( s->lookahead == 0 )
			{
				if ( flush == Z_NO_FLUSH )
				{
					return need_more;
				}

				break;      /* flush the current block */
			}
		}

		/* Output a literal byte */
		s->match_length = 0;
		Tracevv( ( stderr, "%c", s->window[s->strstart] ) );
		_tr_tally_lit ( s, s->window[s->strstart], bflush );
		s->lookahead--;
		s->strstart++;

		if ( bflush ) { FLUSH_BLOCK( s, 0 ); }
	}

	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

/* //// */

/* trees.c -- output deflated data using Huffman coding
 * Copyright (C) 1995-2010 Jean-loup Gailly
 * detect_data_type() function provided freely by Cosmin Truta, 2006
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
 *  ALGORITHM
 *
 *      The "deflation" process uses several Huffman trees. The more
 *      common source values are represented by shorter bit sequences.
 *
 *      Each code tree is stored in a compressed form which is itself
 * a Huffman encoding of the lengths of all the code strings (in
 * ascending order by source values).  The actual code strings are
 * reconstructed from the lengths in the inflate process, as described
 * in the deflate specification.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"'Deflate' Compressed Data Format Specification".
 *      Available in ftp.uu.net:/pub/archiving/zip/doc/deflate-1.1.doc
 *
 *      Storer, James A.
 *          Data Compression:  Methods and Theory, pp. 49-50.
 *          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
 *
 *      Sedgewick, R.
 *          Algorithms, p290.
 *          Addison-Wesley, 1983. ISBN 0-201-06672-6.
 */

/* @(#) $Id$ */

/* ===========================================================================
 * Constants
 */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define END_BLOCK 256
/* end of block literal code */

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

local const int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

local const int extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

local const int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7};

local const uch bl_order[BL_CODES]
   = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

#define Buf_size (8 * 2*sizeof(char))
/* Number of bits used within bi_buf. (bi_buf might be implemented on
 * more than 16 bits on some systems.)
 */

/* ===========================================================================
 * Local data. These are initialized only once.
 */

#define DIST_CODE_LEN  512 /* see definition of array dist_code below */

#if defined(GEN_TREES_H) || !defined(STDC)
/* non ANSI compilers may not accept trees.h */

local ct_data static_ltree[L_CODES + 2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see _tr_init
 * below).
 */

local ct_data static_dtree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */

uch _dist_code[DIST_CODE_LEN];
/* Distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

uch _length_code[MAX_MATCH - MIN_MATCH + 1];
/* length code for each normalized match length (0 == MIN_MATCH) */

local int base_length[LENGTH_CODES];
/* First normalized length for each code (0 = MIN_MATCH) */

local int base_dist[D_CODES];
/* First normalized distance for each code (0 = distance of 1) */

#else
// #  include "trees.h"


/* header created automatically with -DGEN_TREES_H */

local const ct_data static_ltree[L_CODES + 2] =
{
	{{ 12}, {  8}}, {{140}, {  8}}, {{ 76}, {  8}}, {{204}, {  8}}, {{ 44}, {  8}},
	{{172}, {  8}}, {{108}, {  8}}, {{236}, {  8}}, {{ 28}, {  8}}, {{156}, {  8}},
	{{ 92}, {  8}}, {{220}, {  8}}, {{ 60}, {  8}}, {{188}, {  8}}, {{124}, {  8}},
	{{252}, {  8}}, {{  2}, {  8}}, {{130}, {  8}}, {{ 66}, {  8}}, {{194}, {  8}},
	{{ 34}, {  8}}, {{162}, {  8}}, {{ 98}, {  8}}, {{226}, {  8}}, {{ 18}, {  8}},
	{{146}, {  8}}, {{ 82}, {  8}}, {{210}, {  8}}, {{ 50}, {  8}}, {{178}, {  8}},
	{{114}, {  8}}, {{242}, {  8}}, {{ 10}, {  8}}, {{138}, {  8}}, {{ 74}, {  8}},
	{{202}, {  8}}, {{ 42}, {  8}}, {{170}, {  8}}, {{106}, {  8}}, {{234}, {  8}},
	{{ 26}, {  8}}, {{154}, {  8}}, {{ 90}, {  8}}, {{218}, {  8}}, {{ 58}, {  8}},
	{{186}, {  8}}, {{122}, {  8}}, {{250}, {  8}}, {{  6}, {  8}}, {{134}, {  8}},
	{{ 70}, {  8}}, {{198}, {  8}}, {{ 38}, {  8}}, {{166}, {  8}}, {{102}, {  8}},
	{{230}, {  8}}, {{ 22}, {  8}}, {{150}, {  8}}, {{ 86}, {  8}}, {{214}, {  8}},
	{{ 54}, {  8}}, {{182}, {  8}}, {{118}, {  8}}, {{246}, {  8}}, {{ 14}, {  8}},
	{{142}, {  8}}, {{ 78}, {  8}}, {{206}, {  8}}, {{ 46}, {  8}}, {{174}, {  8}},
	{{110}, {  8}}, {{238}, {  8}}, {{ 30}, {  8}}, {{158}, {  8}}, {{ 94}, {  8}},
	{{222}, {  8}}, {{ 62}, {  8}}, {{190}, {  8}}, {{126}, {  8}}, {{254}, {  8}},
	{{  1}, {  8}}, {{129}, {  8}}, {{ 65}, {  8}}, {{193}, {  8}}, {{ 33}, {  8}},
	{{161}, {  8}}, {{ 97}, {  8}}, {{225}, {  8}}, {{ 17}, {  8}}, {{145}, {  8}},
	{{ 81}, {  8}}, {{209}, {  8}}, {{ 49}, {  8}}, {{177}, {  8}}, {{113}, {  8}},
	{{241}, {  8}}, {{  9}, {  8}}, {{137}, {  8}}, {{ 73}, {  8}}, {{201}, {  8}},
	{{ 41}, {  8}}, {{169}, {  8}}, {{105}, {  8}}, {{233}, {  8}}, {{ 25}, {  8}},
	{{153}, {  8}}, {{ 89}, {  8}}, {{217}, {  8}}, {{ 57}, {  8}}, {{185}, {  8}},
	{{121}, {  8}}, {{249}, {  8}}, {{  5}, {  8}}, {{133}, {  8}}, {{ 69}, {  8}},
	{{197}, {  8}}, {{ 37}, {  8}}, {{165}, {  8}}, {{101}, {  8}}, {{229}, {  8}},
	{{ 21}, {  8}}, {{149}, {  8}}, {{ 85}, {  8}}, {{213}, {  8}}, {{ 53}, {  8}},
	{{181}, {  8}}, {{117}, {  8}}, {{245}, {  8}}, {{ 13}, {  8}}, {{141}, {  8}},
	{{ 77}, {  8}}, {{205}, {  8}}, {{ 45}, {  8}}, {{173}, {  8}}, {{109}, {  8}},
	{{237}, {  8}}, {{ 29}, {  8}}, {{157}, {  8}}, {{ 93}, {  8}}, {{221}, {  8}},
	{{ 61}, {  8}}, {{189}, {  8}}, {{125}, {  8}}, {{253}, {  8}}, {{ 19}, {  9}},
	{{275}, {  9}}, {{147}, {  9}}, {{403}, {  9}}, {{ 83}, {  9}}, {{339}, {  9}},
	{{211}, {  9}}, {{467}, {  9}}, {{ 51}, {  9}}, {{307}, {  9}}, {{179}, {  9}},
	{{435}, {  9}}, {{115}, {  9}}, {{371}, {  9}}, {{243}, {  9}}, {{499}, {  9}},
	{{ 11}, {  9}}, {{267}, {  9}}, {{139}, {  9}}, {{395}, {  9}}, {{ 75}, {  9}},
	{{331}, {  9}}, {{203}, {  9}}, {{459}, {  9}}, {{ 43}, {  9}}, {{299}, {  9}},
	{{171}, {  9}}, {{427}, {  9}}, {{107}, {  9}}, {{363}, {  9}}, {{235}, {  9}},
	{{491}, {  9}}, {{ 27}, {  9}}, {{283}, {  9}}, {{155}, {  9}}, {{411}, {  9}},
	{{ 91}, {  9}}, {{347}, {  9}}, {{219}, {  9}}, {{475}, {  9}}, {{ 59}, {  9}},
	{{315}, {  9}}, {{187}, {  9}}, {{443}, {  9}}, {{123}, {  9}}, {{379}, {  9}},
	{{251}, {  9}}, {{507}, {  9}}, {{  7}, {  9}}, {{263}, {  9}}, {{135}, {  9}},
	{{391}, {  9}}, {{ 71}, {  9}}, {{327}, {  9}}, {{199}, {  9}}, {{455}, {  9}},
	{{ 39}, {  9}}, {{295}, {  9}}, {{167}, {  9}}, {{423}, {  9}}, {{103}, {  9}},
	{{359}, {  9}}, {{231}, {  9}}, {{487}, {  9}}, {{ 23}, {  9}}, {{279}, {  9}},
	{{151}, {  9}}, {{407}, {  9}}, {{ 87}, {  9}}, {{343}, {  9}}, {{215}, {  9}},
	{{471}, {  9}}, {{ 55}, {  9}}, {{311}, {  9}}, {{183}, {  9}}, {{439}, {  9}},
	{{119}, {  9}}, {{375}, {  9}}, {{247}, {  9}}, {{503}, {  9}}, {{ 15}, {  9}},
	{{271}, {  9}}, {{143}, {  9}}, {{399}, {  9}}, {{ 79}, {  9}}, {{335}, {  9}},
	{{207}, {  9}}, {{463}, {  9}}, {{ 47}, {  9}}, {{303}, {  9}}, {{175}, {  9}},
	{{431}, {  9}}, {{111}, {  9}}, {{367}, {  9}}, {{239}, {  9}}, {{495}, {  9}},
	{{ 31}, {  9}}, {{287}, {  9}}, {{159}, {  9}}, {{415}, {  9}}, {{ 95}, {  9}},
	{{351}, {  9}}, {{223}, {  9}}, {{479}, {  9}}, {{ 63}, {  9}}, {{319}, {  9}},
	{{191}, {  9}}, {{447}, {  9}}, {{127}, {  9}}, {{383}, {  9}}, {{255}, {  9}},
	{{511}, {  9}}, {{  0}, {  7}}, {{ 64}, {  7}}, {{ 32}, {  7}}, {{ 96}, {  7}},
	{{ 16}, {  7}}, {{ 80}, {  7}}, {{ 48}, {  7}}, {{112}, {  7}}, {{  8}, {  7}},
	{{ 72}, {  7}}, {{ 40}, {  7}}, {{104}, {  7}}, {{ 24}, {  7}}, {{ 88}, {  7}},
	{{ 56}, {  7}}, {{120}, {  7}}, {{  4}, {  7}}, {{ 68}, {  7}}, {{ 36}, {  7}},
	{{100}, {  7}}, {{ 20}, {  7}}, {{ 84}, {  7}}, {{ 52}, {  7}}, {{116}, {  7}},
	{{  3}, {  8}}, {{131}, {  8}}, {{ 67}, {  8}}, {{195}, {  8}}, {{ 35}, {  8}},
	{{163}, {  8}}, {{ 99}, {  8}}, {{227}, {  8}}
};

local const ct_data static_dtree[D_CODES] =
{
	{{ 0}, { 5}}, {{16}, { 5}}, {{ 8}, { 5}}, {{24}, { 5}}, {{ 4}, { 5}},
	{{20}, { 5}}, {{12}, { 5}}, {{28}, { 5}}, {{ 2}, { 5}}, {{18}, { 5}},
	{{10}, { 5}}, {{26}, { 5}}, {{ 6}, { 5}}, {{22}, { 5}}, {{14}, { 5}},
	{{30}, { 5}}, {{ 1}, { 5}}, {{17}, { 5}}, {{ 9}, { 5}}, {{25}, { 5}},
	{{ 5}, { 5}}, {{21}, { 5}}, {{13}, { 5}}, {{29}, { 5}}, {{ 3}, { 5}},
	{{19}, { 5}}, {{11}, { 5}}, {{27}, { 5}}, {{ 7}, { 5}}, {{23}, { 5}}
};

const uch ZLIB_INTERNAL _dist_code[DIST_CODE_LEN] =
{
	0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,
	8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,  0,  0, 16, 17,
	18, 18, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29
};

const uch ZLIB_INTERNAL _length_code[MAX_MATCH - MIN_MATCH + 1] =
{
	0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 12, 12,
	13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
	17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19,
	19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28
};

local const int base_length[LENGTH_CODES] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56,
	64, 80, 96, 112, 128, 160, 192, 224, 0
};

local const int base_dist[D_CODES] =
{
	0,     1,     2,     3,     4,     6,     8,    12,    16,    24,
	32,    48,    64,    96,   128,   192,   256,   384,   512,   768,
	1024,  1536,  2048,  3072,  4096,  6144,  8192, 12288, 16384, 24576
};

#endif /* GEN_TREES_H */

struct static_tree_desc_s
{
	const ct_data* static_tree;  /* static tree or NULL */
	const intf* extra_bits;      /* extra bits for each code or NULL */
	int     extra_base;          /* base index for extra_bits */
	int     elems;               /* max number of elements in the tree */
	int     max_length;          /* max bit length for the codes */
};

local static_tree_desc  static_l_desc =
{static_ltree, extra_lbits, LITERALS + 1, L_CODES, MAX_BITS};

local static_tree_desc  static_d_desc =
{static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS};

local static_tree_desc  static_bl_desc =
{( const ct_data* )0, extra_blbits, 0,   BL_CODES, MAX_BL_BITS};

/* ===========================================================================
 * Local (static) routines in this file.
 */

local void tr_static_init OF( ( void ) );
local void init_block     OF( ( deflate_state* s ) );
local void pqdownheap     OF( ( deflate_state* s, ct_data* tree, int k ) );
local void gen_bitlen     OF( ( deflate_state* s, tree_desc* desc ) );
local void gen_codes      OF( ( ct_data* tree, int max_code, ushf* bl_count ) );
local void build_tree     OF( ( deflate_state* s, tree_desc* desc ) );
local void scan_tree      OF( ( deflate_state* s, ct_data* tree, int max_code ) );
local void send_tree      OF( ( deflate_state* s, ct_data* tree, int max_code ) );
local int  build_bl_tree  OF( ( deflate_state* s ) );
local void send_all_trees OF( ( deflate_state* s, int lcodes, int dcodes,
                                int blcodes ) );
local void compress_block OF( ( deflate_state* s, ct_data* ltree,
                                ct_data* dtree ) );
local int  detect_data_type OF( ( deflate_state* s ) );
local unsigned bi_reverse OF( ( unsigned value, int length ) );
local void bi_windup      OF( ( deflate_state* s ) );
local void bi_flush       OF( ( deflate_state* s ) );
local void copy_block     OF( ( deflate_state* s, charf* buf, unsigned len,
                                int header ) );

#ifdef GEN_TREES_H
local void gen_trees_header OF( ( void ) );
#endif

#ifndef DEBUG
#  define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)
/* Send a code of the given tree. c and tree must not have side effects */

#else /* DEBUG */
#  define send_code(s, c, tree) \
     { if (z_verbose>2) fprintf(stderr,"\ncd %3d ",(c)); \
       send_bits(s, tree[c].Code, tree[c].Len); }
#endif

/* ===========================================================================
 * Output a short LSB first on the stream.
 * IN assertion: there is enough room in pendingBuf.
 */
#define put_short(s, w) { \
    put_byte(s, (uch)((w) & 0xff)); \
    put_byte(s, (uch)((ush)(w) >> 8)); \
}

/* ===========================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */
#ifdef DEBUG
local void send_bits      OF( ( deflate_state* s, int value, int length ) );

local void send_bits( s, value, length )
deflate_state* s;
int value;  /* value to send */
int length; /* number of bits */
{
	Tracevv( ( stderr, " l %2d v %4x ", length, value ) );
	Assert( length > 0 && length <= 15, "invalid length" );
	s->bits_sent += ( ulg )length;

	/* If not enough room in bi_buf, use (valid) bits from bi_buf and
	 * (16 - bi_valid) bits from value, leaving (width - (16-bi_valid))
	 * unused bits in value.
	 */
	if ( s->bi_valid > ( int )Buf_size - length )
	{
		s->bi_buf |= ( ush )value << s->bi_valid;
		put_short( s, s->bi_buf );
		s->bi_buf = ( ush )value >> ( Buf_size - s->bi_valid );
		s->bi_valid += length - Buf_size;
	}
	else
	{
		s->bi_buf |= ( ush )value << s->bi_valid;
		s->bi_valid += length;
	}
}
#else /* !DEBUG */

#define send_bits(s, value, length) \
{ int len = length;\
  if (s->bi_valid > (int)Buf_size - len) {\
    int val = value;\
    s->bi_buf |= (ush)val << s->bi_valid;\
    put_short(s, s->bi_buf);\
    s->bi_buf = (ush)val >> (Buf_size - s->bi_valid);\
    s->bi_valid += len - Buf_size;\
  } else {\
    s->bi_buf |= (ush)(value) << s->bi_valid;\
    s->bi_valid += len;\
  }\
}
#endif /* DEBUG */


/* the arguments must not have side effects */

/* ===========================================================================
 * Initialize the various 'constant' tables.
 */
local void tr_static_init()
{
#if defined(GEN_TREES_H) || !defined(STDC)
	static int static_init_done = 0;
	int n;        /* iterates over tree elements */
	int bits;     /* bit counter */
	int length;   /* length value */
	int code;     /* code value */
	int dist;     /* distance index */
	ush bl_count[MAX_BITS + 1];
	/* number of codes at each bit length for an optimal tree */

	if ( static_init_done ) { return; }

	/* For some embedded targets, global variables are not initialized: */
#ifdef NO_INIT_GLOBAL_POINTERS
	static_l_desc.static_tree = static_ltree;
	static_l_desc.extra_bits = extra_lbits;
	static_d_desc.static_tree = static_dtree;
	static_d_desc.extra_bits = extra_dbits;
	static_bl_desc.extra_bits = extra_blbits;
#endif

	/* Initialize the mapping length (0..255) -> length code (0..28) */
	length = 0;

	for ( code = 0; code < LENGTH_CODES - 1; code++ )
	{
		base_length[code] = length;

		for ( n = 0; n < ( 1 << extra_lbits[code] ); n++ )
		{
			_length_code[length++] = ( uch )code;
		}
	}

	Assert ( length == 256, "tr_static_init: length != 256" );
	/* Note that the length 255 (match length 258) can be represented
	 * in two different ways: code 284 + 5 bits or code 285, so we
	 * overwrite length_code[255] to use the best encoding:
	 */
	_length_code[length - 1] = ( uch )code;

	/* Initialize the mapping dist (0..32K) -> dist code (0..29) */
	dist = 0;

	for ( code = 0 ; code < 16; code++ )
	{
		base_dist[code] = dist;

		for ( n = 0; n < ( 1 << extra_dbits[code] ); n++ )
		{
			_dist_code[dist++] = ( uch )code;
		}
	}

	Assert ( dist == 256, "tr_static_init: dist != 256" );
	dist >>= 7; /* from now on, all distances are divided by 128 */

	for ( ; code < D_CODES; code++ )
	{
		base_dist[code] = dist << 7;

		for ( n = 0; n < ( 1 << ( extra_dbits[code] - 7 ) ); n++ )
		{
			_dist_code[256 + dist++] = ( uch )code;
		}
	}

	Assert ( dist == 256, "tr_static_init: 256+dist != 512" );

	/* Construct the codes of the static literal tree */
	for ( bits = 0; bits <= MAX_BITS; bits++ ) { bl_count[bits] = 0; }

	n = 0;

	while ( n <= 143 ) { static_ltree[n++].Len = 8, bl_count[8]++; }

	while ( n <= 255 ) { static_ltree[n++].Len = 9, bl_count[9]++; }

	while ( n <= 279 ) { static_ltree[n++].Len = 7, bl_count[7]++; }

	while ( n <= 287 ) { static_ltree[n++].Len = 8, bl_count[8]++; }

	/* Codes 286 and 287 do not exist, but we must include them in the
	 * tree construction to get a canonical Huffman tree (longest code
	 * all ones)
	 */
	gen_codes( ( ct_data* )static_ltree, L_CODES + 1, bl_count );

	/* The static distance tree is trivial: */
	for ( n = 0; n < D_CODES; n++ )
	{
		static_dtree[n].Len = 5;
		static_dtree[n].Code = bi_reverse( ( unsigned )n, 5 );
	}

	static_init_done = 1;

#  ifdef GEN_TREES_H
	gen_trees_header();
#  endif
#endif /* defined(GEN_TREES_H) || !defined(STDC) */
}

/* ===========================================================================
 * Genererate the file trees.h describing the static trees.
 */
#ifdef GEN_TREES_H
#  ifndef DEBUG
#    include <stdio.h>
#  endif

#  define SEPARATOR(i, last, width) \
      ((i) == (last)? "\n};\n\n" :    \
       ((i) % (width) == (width)-1 ? ",\n" : ", "))

void gen_trees_header()
{
	FILE* header = fopen( "trees.h", "w" );
	int i;

	Assert ( header != NULL, "Can't open trees.h" );
	fprintf( header,
	         "/* header created automatically with -DGEN_TREES_H */\n\n" );

	fprintf( header, "local const ct_data static_ltree[L_CODES+2] = {\n" );

	for ( i = 0; i < L_CODES + 2; i++ )
	{
		fprintf( header, "{{%3u},{%3u}}%s", static_ltree[i].Code,
		         static_ltree[i].Len, SEPARATOR( i, L_CODES + 1, 5 ) );
	}

	fprintf( header, "local const ct_data static_dtree[D_CODES] = {\n" );

	for ( i = 0; i < D_CODES; i++ )
	{
		fprintf( header, "{{%2u},{%2u}}%s", static_dtree[i].Code,
		         static_dtree[i].Len, SEPARATOR( i, D_CODES - 1, 5 ) );
	}

	fprintf( header, "const uch ZLIB_INTERNAL _dist_code[DIST_CODE_LEN] = {\n" );

	for ( i = 0; i < DIST_CODE_LEN; i++ )
	{
		fprintf( header, "%2u%s", _dist_code[i],
		         SEPARATOR( i, DIST_CODE_LEN - 1, 20 ) );
	}

	fprintf( header,
	         "const uch ZLIB_INTERNAL _length_code[MAX_MATCH-MIN_MATCH+1]= {\n" );

	for ( i = 0; i < MAX_MATCH - MIN_MATCH + 1; i++ )
	{
		fprintf( header, "%2u%s", _length_code[i],
		         SEPARATOR( i, MAX_MATCH - MIN_MATCH, 20 ) );
	}

	fprintf( header, "local const int base_length[LENGTH_CODES] = {\n" );

	for ( i = 0; i < LENGTH_CODES; i++ )
	{
		fprintf( header, "%1u%s", base_length[i],
		         SEPARATOR( i, LENGTH_CODES - 1, 20 ) );
	}

	fprintf( header, "local const int base_dist[D_CODES] = {\n" );

	for ( i = 0; i < D_CODES; i++ )
	{
		fprintf( header, "%5u%s", base_dist[i],
		         SEPARATOR( i, D_CODES - 1, 10 ) );
	}

	fclose( header );
}
#endif /* GEN_TREES_H */

/* ===========================================================================
 * Initialize the tree data structures for a new zlib stream.
 */
void ZLIB_INTERNAL _tr_init( s )
deflate_state* s;
{
	tr_static_init();

	s->l_desc.dyn_tree = s->dyn_ltree;
	s->l_desc.stat_desc = &static_l_desc;

	s->d_desc.dyn_tree = s->dyn_dtree;
	s->d_desc.stat_desc = &static_d_desc;

	s->bl_desc.dyn_tree = s->bl_tree;
	s->bl_desc.stat_desc = &static_bl_desc;

	s->bi_buf = 0;
	s->bi_valid = 0;
	s->last_eob_len = 8; /* enough lookahead for inflate */
#ifdef DEBUG
	s->compressed_len = 0L;
	s->bits_sent = 0L;
#endif

	/* Initialize the first block of the first file: */
	init_block( s );
}

/* ===========================================================================
 * Initialize a new block.
 */
local void init_block( s )
deflate_state* s;
{
	int n; /* iterates over tree elements */

	/* Initialize the trees. */
	for ( n = 0; n < L_CODES;  n++ ) { s->dyn_ltree[n].Freq = 0; }

	for ( n = 0; n < D_CODES;  n++ ) { s->dyn_dtree[n].Freq = 0; }

	for ( n = 0; n < BL_CODES; n++ ) { s->bl_tree[n].Freq = 0; }

	s->dyn_ltree[END_BLOCK].Freq = 1;
	s->opt_len = s->static_len = 0L;
	s->last_lit = s->matches = 0;
}

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */


/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(s, tree, top) \
{\
    top = s->heap[SMALLEST]; \
    s->heap[SMALLEST] = s->heap[s->heap_len--]; \
    pqdownheap(s, tree, SMALLEST); \
}

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m, depth) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
local void pqdownheap( s, tree, k )
deflate_state* s;
ct_data* tree;  /* the tree to restore */
int k;               /* node to move down */
{
	int v = s->heap[k];
	int j = k << 1;  /* left son of k */

	while ( j <= s->heap_len )
	{
		/* Set j to the smallest of the two sons: */
		if ( j < s->heap_len &&
		     smaller( tree, s->heap[j + 1], s->heap[j], s->depth ) )
		{
			j++;
		}

		/* Exit if v is smaller than both sons */
		if ( smaller( tree, v, s->heap[j], s->depth ) ) { break; }

		/* Exchange v with the smallest son */
		s->heap[k] = s->heap[j];
		k = j;

		/* And continue down the tree, setting j to the left son of k */
		j <<= 1;
	}

	s->heap[k] = v;
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
local void gen_bitlen( s, desc )
deflate_state* s;
tree_desc* desc;    /* the tree descriptor */
{
	ct_data* tree        = desc->dyn_tree;
	int max_code         = desc->max_code;
	const ct_data* stree = desc->stat_desc->static_tree;
	const intf* extra    = desc->stat_desc->extra_bits;
	int base             = desc->stat_desc->extra_base;
	int max_length       = desc->stat_desc->max_length;
	int h;              /* heap index */
	int n, m;           /* iterate over the tree elements */
	int bits;           /* bit length */
	int xbits;          /* extra bits */
	ush f;              /* frequency */
	int overflow = 0;   /* number of elements with bit length too large */

	for ( bits = 0; bits <= MAX_BITS; bits++ ) { s->bl_count[bits] = 0; }

	/* In a first pass, compute the optimal bit lengths (which may
	 * overflow in the case of the bit length tree).
	 */
	tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */

	for ( h = s->heap_max + 1; h < HEAP_SIZE; h++ )
	{
		n = s->heap[h];
		bits = tree[tree[n].Dad].Len + 1;

		if ( bits > max_length ) { bits = max_length, overflow++; }

		tree[n].Len = ( ush )bits;
		/* We overwrite tree[n].Dad which is no longer needed */

		if ( n > max_code ) { continue; } /* not a leaf node */

		s->bl_count[bits]++;
		xbits = 0;

		if ( n >= base ) { xbits = extra[n - base]; }

		f = tree[n].Freq;
		s->opt_len += ( ulg )f * ( bits + xbits );

		if ( stree ) { s->static_len += ( ulg )f * ( stree[n].Len + xbits ); }
	}

	if ( overflow == 0 ) { return; }

	Trace( ( stderr, "\nbit length overflow\n" ) );
	/* This happens for example on obj2 and pic of the Calgary corpus */

	/* Find the first bit length which could increase: */
	do
	{
		bits = max_length - 1;

		while ( s->bl_count[bits] == 0 ) { bits--; }

		s->bl_count[bits]--;      /* move one leaf down the tree */
		s->bl_count[bits + 1] += 2; /* move one overflow item as its brother */
		s->bl_count[max_length]--;
		/* The brother of the overflow item also moves one step up,
		 * but this does not affect bl_count[max_length]
		 */
		overflow -= 2;
	}
	while ( overflow > 0 );

	/* Now recompute all bit lengths, scanning in increasing frequency.
	 * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
	 * lengths instead of fixing only the wrong ones. This idea is taken
	 * from 'ar' written by Haruhiko Okumura.)
	 */
	for ( bits = max_length; bits != 0; bits-- )
	{
		n = s->bl_count[bits];

		while ( n != 0 )
		{
			m = s->heap[--h];

			if ( m > max_code ) { continue; }

			if ( ( unsigned ) tree[m].Len != ( unsigned ) bits )
			{
				Trace( ( stderr, "code %d bits %d->%d\n", m, tree[m].Len, bits ) );
				s->opt_len += ( ( long )bits - ( long )tree[m].Len )
				              * ( long )tree[m].Freq;
				tree[m].Len = ( ush )bits;
			}

			n--;
		}
	}
}

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
local void gen_codes ( tree, max_code, bl_count )
ct_data* tree;             /* the tree to decorate */
int max_code;              /* largest code with non zero frequency */
ushf* bl_count;            /* number of codes at each bit length */
{
	ush next_code[MAX_BITS + 1]; /* next code value for each bit length */
	ush code = 0;              /* running code value */
	int bits;                  /* bit index */
	int n;                     /* code index */

	/* The distribution counts are first used to generate the code values
	 * without bit reversal.
	 */
	for ( bits = 1; bits <= MAX_BITS; bits++ )
	{
		next_code[bits] = code = ( code + bl_count[bits - 1] ) << 1;
	}

	/* Check that the bit counts in bl_count are consistent. The last code
	 * must be all ones.
	 */
	Assert ( code + bl_count[MAX_BITS] - 1 == ( 1 << MAX_BITS ) - 1,
	         "inconsistent bit counts" );
	Tracev( ( stderr, "\ngen_codes: max_code %d ", max_code ) );

	for ( n = 0;  n <= max_code; n++ )
	{
		int len = tree[n].Len;

		if ( len == 0 ) { continue; }

		/* Now reverse the bits */
		tree[n].Code = bi_reverse( next_code[len]++, len );

		Tracecv( tree != static_ltree, ( stderr, "\nn %3d %c l %2d c %4x (%x) ",
		                                 n, ( isgraph( n ) ? n : ' ' ), len, tree[n].Code, next_code[len] - 1 ) );
	}
}

/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
local void build_tree( s, desc )
deflate_state* s;
tree_desc* desc; /* the tree descriptor */
{
	ct_data* tree         = desc->dyn_tree;
	const ct_data* stree  = desc->stat_desc->static_tree;
	int elems             = desc->stat_desc->elems;
	int n, m;          /* iterate over heap elements */
	int max_code = -1; /* largest code with non zero frequency */
	int node;          /* new node being created */

	/* Construct the initial heap, with least frequent element in
	 * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
	 * heap[0] is not used.
	 */
	s->heap_len = 0, s->heap_max = HEAP_SIZE;

	for ( n = 0; n < elems; n++ )
	{
		if ( tree[n].Freq != 0 )
		{
			s->heap[++( s->heap_len )] = max_code = n;
			s->depth[n] = 0;
		}
		else
		{
			tree[n].Len = 0;
		}
	}

	/* The pkzip format requires that at least one distance code exists,
	 * and that at least one bit should be sent even if there is only one
	 * possible code. So to avoid special checks later on we force at least
	 * two codes of non zero frequency.
	 */
	while ( s->heap_len < 2 )
	{
		node = s->heap[++( s->heap_len )] = ( max_code < 2 ? ++max_code : 0 );
		tree[node].Freq = 1;
		s->depth[node] = 0;
		s->opt_len--;

		if ( stree ) { s->static_len -= stree[node].Len; }

		/* node is 0 or 1 so it does not have extra bits */
	}

	desc->max_code = max_code;

	/* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
	 * establish sub-heaps of increasing lengths:
	 */
	for ( n = s->heap_len / 2; n >= 1; n-- ) { pqdownheap( s, tree, n ); }

	/* Construct the Huffman tree by repeatedly combining the least two
	 * frequent nodes.
	 */
	node = elems;              /* next internal node of the tree */

	do
	{
		pqremove( s, tree, n ); /* n = node of least frequency */
		m = s->heap[SMALLEST]; /* m = node of next least frequency */

		s->heap[--( s->heap_max )] = n; /* keep the nodes sorted by frequency */
		s->heap[--( s->heap_max )] = m;

		/* Create a new node father of n and m */
		tree[node].Freq = tree[n].Freq + tree[m].Freq;
		s->depth[node] = ( uch )( ( s->depth[n] >= s->depth[m] ?
		                            s->depth[n] : s->depth[m] ) + 1 );
		tree[n].Dad = tree[m].Dad = ( ush )node;
#ifdef DUMP_BL_TREE

		if ( tree == s->bl_tree )
		{
			fprintf( stderr, "\nnode %d(%d), sons %d(%d) %d(%d)",
			         node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq );
		}

#endif
		/* and insert the new node in the heap */
		s->heap[SMALLEST] = node++;
		pqdownheap( s, tree, SMALLEST );

	}
	while ( s->heap_len >= 2 );

	s->heap[--( s->heap_max )] = s->heap[SMALLEST];

	/* At this point, the fields freq and dad are set. We can now
	 * generate the bit lengths.
	 */
	gen_bitlen( s, ( tree_desc* )desc );

	/* The field len is now set, we can generate the bit codes */
	gen_codes ( ( ct_data* )tree, max_code, s->bl_count );
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree.
 */
local void scan_tree ( s, tree, max_code )
deflate_state* s;
ct_data* tree;   /* the tree to be scanned */
int max_code;    /* and its largest code of non zero frequency */
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	if ( nextlen == 0 ) { max_count = 138, min_count = 3; }

	tree[max_code + 1].Len = ( ush )0xffff; /* guard */

	for ( n = 0; n <= max_code; n++ )
	{
		curlen = nextlen;
		nextlen = tree[n + 1].Len;

		if ( ++count < max_count && curlen == nextlen )
		{
			continue;
		}
		else if ( count < min_count )
		{
			s->bl_tree[curlen].Freq += count;
		}
		else if ( curlen != 0 )
		{
			if ( curlen != prevlen ) { s->bl_tree[curlen].Freq++; }

			s->bl_tree[REP_3_6].Freq++;
		}
		else if ( count <= 10 )
		{
			s->bl_tree[REPZ_3_10].Freq++;
		}
		else
		{
			s->bl_tree[REPZ_11_138].Freq++;
		}

		count = 0;
		prevlen = curlen;

		if ( nextlen == 0 )
		{
			max_count = 138, min_count = 3;
		}
		else if ( curlen == nextlen )
		{
			max_count = 6, min_count = 3;
		}
		else
		{
			max_count = 7, min_count = 4;
		}
	}
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * bl_tree.
 */
local void send_tree ( s, tree, max_code )
deflate_state* s;
ct_data* tree; /* the tree to be scanned */
int max_code;       /* and its largest code of non zero frequency */
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	/* tree[max_code+1].Len = -1; */  /* guard already set */
	if ( nextlen == 0 ) { max_count = 138, min_count = 3; }

	for ( n = 0; n <= max_code; n++ )
	{
		curlen = nextlen;
		nextlen = tree[n + 1].Len;

		if ( ++count < max_count && curlen == nextlen )
		{
			continue;
		}
		else if ( count < min_count )
		{
			do { send_code( s, curlen, s->bl_tree ); }
			while ( --count != 0 );

		}
		else if ( curlen != 0 )
		{
			if ( curlen != prevlen )
			{
				send_code( s, curlen, s->bl_tree );
				count--;
			}

			Assert( count >= 3 && count <= 6, " 3_6?" );
			send_code( s, REP_3_6, s->bl_tree );
			send_bits( s, count - 3, 2 );

		}
		else if ( count <= 10 )
		{
			send_code( s, REPZ_3_10, s->bl_tree );
			send_bits( s, count - 3, 3 );

		}
		else
		{
			send_code( s, REPZ_11_138, s->bl_tree );
			send_bits( s, count - 11, 7 );
		}

		count = 0;
		prevlen = curlen;

		if ( nextlen == 0 )
		{
			max_count = 138, min_count = 3;
		}
		else if ( curlen == nextlen )
		{
			max_count = 6, min_count = 3;
		}
		else
		{
			max_count = 7, min_count = 4;
		}
	}
}

/* ===========================================================================
 * Construct the Huffman tree for the bit lengths and return the index in
 * bl_order of the last bit length code to send.
 */
local int build_bl_tree( s )
deflate_state* s;
{
	int max_blindex;  /* index of last bit length code of non zero freq */

	/* Determine the bit length frequencies for literal and distance trees */
	scan_tree( s, ( ct_data* )s->dyn_ltree, s->l_desc.max_code );
	scan_tree( s, ( ct_data* )s->dyn_dtree, s->d_desc.max_code );

	/* Build the bit length tree: */
	build_tree( s, ( tree_desc* )( &( s->bl_desc ) ) );
	/* opt_len now includes the length of the tree representations, except
	 * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
	 */

	/* Determine the number of bit length codes to send. The pkzip format
	 * requires that at least 4 bit length codes be sent. (appnote.txt says
	 * 3 but the actual value used is 4.)
	 */
	for ( max_blindex = BL_CODES - 1; max_blindex >= 3; max_blindex-- )
	{
		if ( s->bl_tree[bl_order[max_blindex]].Len != 0 ) { break; }
	}

	/* Update opt_len to include the bit length tree and counts */
	s->opt_len += 3 * ( max_blindex + 1 ) + 5 + 5 + 4;
	Tracev( ( stderr, "\ndyn trees: dyn %ld, stat %ld",
	          s->opt_len, s->static_len ) );

	return max_blindex;
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
 */
local void send_all_trees( s, lcodes, dcodes, blcodes )
deflate_state* s;
int lcodes, dcodes, blcodes; /* number of codes for each tree */
{
	int rank;                    /* index in bl_order */

	Assert ( lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes" );
	Assert ( lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
	         "too many codes" );
	Tracev( ( stderr, "\nbl counts: " ) );
	send_bits( s, lcodes - 257, 5 ); /* not +255 as stated in appnote.txt */
	send_bits( s, dcodes - 1,   5 );
	send_bits( s, blcodes - 4,  4 ); /* not -3 as stated in appnote.txt */

	for ( rank = 0; rank < blcodes; rank++ )
	{
		Tracev( ( stderr, "\nbl code %2d ", bl_order[rank] ) );
		send_bits( s, s->bl_tree[bl_order[rank]].Len, 3 );
	}

	Tracev( ( stderr, "\nbl tree: sent %ld", s->bits_sent ) );

	send_tree( s, ( ct_data* )s->dyn_ltree, lcodes - 1 ); /* literal tree */
	Tracev( ( stderr, "\nlit tree: sent %ld", s->bits_sent ) );

	send_tree( s, ( ct_data* )s->dyn_dtree, dcodes - 1 ); /* distance tree */
	Tracev( ( stderr, "\ndist tree: sent %ld", s->bits_sent ) );
}

/* ===========================================================================
 * Send a stored block
 */
void ZLIB_INTERNAL _tr_stored_block( s, buf, stored_len, last )
deflate_state* s;
charf* buf;       /* input block */
ulg stored_len;   /* length of input block */
int last;         /* one if this is the last block for a file */
{
	send_bits( s, ( STORED_BLOCK << 1 ) + last, 3 ); /* send block type */
#ifdef DEBUG
	s->compressed_len = ( s->compressed_len + 3 + 7 ) & ( ulg )~7L;
	s->compressed_len += ( stored_len + 4 ) << 3;
#endif
	copy_block( s, buf, ( unsigned )stored_len, 1 ); /* with header */
}

/* ===========================================================================
 * Send one empty static block to give enough lookahead for inflate.
 * This takes 10 bits, of which 7 may remain in the bit buffer.
 * The current inflate code requires 9 bits of lookahead. If the
 * last two codes for the previous block (real code plus EOB) were coded
 * on 5 bits or less, inflate may have only 5+3 bits of lookahead to decode
 * the last real code. In this case we send two empty static blocks instead
 * of one. (There are no problems if the previous block is stored or fixed.)
 * To simplify the code, we assume the worst case of last real code encoded
 * on one bit only.
 */
void ZLIB_INTERNAL _tr_align( s )
deflate_state* s;
{
	send_bits( s, STATIC_TREES << 1, 3 );
	send_code( s, END_BLOCK, static_ltree );
#ifdef DEBUG
	s->compressed_len += 10L; /* 3 for block type, 7 for EOB */
#endif
	bi_flush( s );

	/* Of the 10 bits for the empty block, we have already sent
	 * (10 - bi_valid) bits. The lookahead for the last real code (before
	 * the EOB of the previous block) was thus at least one plus the length
	 * of the EOB plus what we have just sent of the empty static block.
	 */
	if ( 1 + s->last_eob_len + 10 - s->bi_valid < 9 )
	{
		send_bits( s, STATIC_TREES << 1, 3 );
		send_code( s, END_BLOCK, static_ltree );
#ifdef DEBUG
		s->compressed_len += 10L;
#endif
		bi_flush( s );
	}

	s->last_eob_len = 7;
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file.
 */
void ZLIB_INTERNAL _tr_flush_block( s, buf, stored_len, last )
deflate_state* s;
charf* buf;       /* input block, or NULL if too old */
ulg stored_len;   /* length of input block */
int last;         /* one if this is the last block for a file */
{
	ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
	int max_blindex = 0;  /* index of last bit length code of non zero freq */

	/* Build the Huffman trees unless a stored block is forced */
	if ( s->level > 0 )
	{

		/* Check if the file is binary or text */
		if ( s->strm->data_type == Z_UNKNOWN )
		{
			s->strm->data_type = detect_data_type( s );
		}

		/* Construct the literal and distance trees */
		build_tree( s, ( tree_desc* )( &( s->l_desc ) ) );
		Tracev( ( stderr, "\nlit data: dyn %ld, stat %ld", s->opt_len,
		          s->static_len ) );

		build_tree( s, ( tree_desc* )( &( s->d_desc ) ) );
		Tracev( ( stderr, "\ndist data: dyn %ld, stat %ld", s->opt_len,
		          s->static_len ) );
		/* At this point, opt_len and static_len are the total bit lengths of
		 * the compressed block data, excluding the tree representations.
		 */

		/* Build the bit length tree for the above two trees, and get the index
		 * in bl_order of the last bit length code to send.
		 */
		max_blindex = build_bl_tree( s );

		/* Determine the best encoding. Compute the block lengths in bytes. */
		opt_lenb = ( s->opt_len + 3 + 7 ) >> 3;
		static_lenb = ( s->static_len + 3 + 7 ) >> 3;

		Tracev( ( stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u ",
		          opt_lenb, s->opt_len, static_lenb, s->static_len, stored_len,
		          s->last_lit ) );

		if ( static_lenb <= opt_lenb ) { opt_lenb = static_lenb; }

	}
	else
	{
		Assert( buf != ( char* )0, "lost buf" );
		opt_lenb = static_lenb = stored_len + 5; /* force a stored block */
	}

#ifdef FORCE_STORED

	if ( buf != ( char* )0 ) /* force stored block */
	{
#else

	if ( stored_len + 4 <= opt_lenb && buf != ( char* )0 )
	{
		/* 4: two words for the lengths */
#endif
		/* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
		 * Otherwise we can't have processed more than WSIZE input bytes since
		 * the last block flush, because compression would have been
		 * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
		 * transform a block into a stored block.
		 */
		_tr_stored_block( s, buf, stored_len, last );

#ifdef FORCE_STATIC
	}
	else if ( static_lenb >= 0 )   /* force static trees */
	{
#else
	}
	else if ( s->strategy == Z_FIXED || static_lenb == opt_lenb )
	{
#endif
		send_bits( s, ( STATIC_TREES << 1 ) + last, 3 );
		compress_block( s, ( ct_data* )static_ltree, ( ct_data* )static_dtree );
#ifdef DEBUG
		s->compressed_len += 3 + s->static_len;
#endif
	}
	else
	{
		send_bits( s, ( DYN_TREES << 1 ) + last, 3 );
		send_all_trees( s, s->l_desc.max_code + 1, s->d_desc.max_code + 1,
		                max_blindex + 1 );
		compress_block( s, ( ct_data* )s->dyn_ltree, ( ct_data* )s->dyn_dtree );
#ifdef DEBUG
		s->compressed_len += 3 + s->opt_len;
#endif
	}

	Assert ( s->compressed_len == s->bits_sent, "bad compressed size" );
	/* The above check is made mod 2^32, for files larger than 512 MB
	 * and uLong implemented on 32 bits.
	 */
	init_block( s );

	if ( last )
	{
		bi_windup( s );
#ifdef DEBUG
		s->compressed_len += 7;  /* align on byte boundary */
#endif
	}

	Tracev( ( stderr, "\ncomprlen %lu(%lu) ", s->compressed_len >> 3,
	          s->compressed_len - 7 * last ) );
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
int ZLIB_INTERNAL _tr_tally ( s, dist, lc )
deflate_state* s;
unsigned dist;  /* distance of matched string */
unsigned lc;    /* match length-MIN_MATCH or unmatched char (if dist==0) */
{
	s->d_buf[s->last_lit] = ( ush )dist;
	s->l_buf[s->last_lit++] = ( uch )lc;

	if ( dist == 0 )
	{
		/* lc is the unmatched char */
		s->dyn_ltree[lc].Freq++;
	}
	else
	{
		s->matches++;
		/* Here, lc is the match length - MIN_MATCH */
		dist--;             /* dist = match distance - 1 */
		Assert( ( ush )dist < ( ush )MAX_DIST( s ) &&
		        ( ush )lc <= ( ush )( MAX_MATCH - MIN_MATCH ) &&
		        ( ush )d_code( dist ) < ( ush )D_CODES,  "_tr_tally: bad match" );

		s->dyn_ltree[_length_code[lc] + LITERALS + 1].Freq++;
		s->dyn_dtree[d_code( dist )].Freq++;
	}

#ifdef TRUNCATE_BLOCK

	/* Try to guess if it is profitable to stop the current block here */
	if ( ( s->last_lit & 0x1fff ) == 0 && s->level > 2 )
	{
		/* Compute an upper bound for the compressed length */
		ulg out_length = ( ulg )s->last_lit * 8L;
		ulg in_length = ( ulg )( ( long )s->strstart - s->block_start );
		int dcode;

		for ( dcode = 0; dcode < D_CODES; dcode++ )
		{
			out_length += ( ulg )s->dyn_dtree[dcode].Freq *
			              ( 5L + extra_dbits[dcode] );
		}

		out_length >>= 3;
		Tracev( ( stderr, "\nlast_lit %u, in %ld, out ~%ld(%ld%%) ",
		          s->last_lit, in_length, out_length,
		          100L - out_length * 100L / in_length ) );

		if ( s->matches < s->last_lit / 2 && out_length < in_length / 2 ) { return 1; }
	}

#endif
	return ( s->last_lit == s->lit_bufsize - 1 );
	/* We avoid equality with lit_bufsize because of wraparound at 64K
	 * on 16 bit machines and because stored blocks are restricted to
	 * 64K-1 bytes.
	 */
}

/* ===========================================================================
 * Send the block data compressed using the given Huffman trees
 */
local void compress_block( s, ltree, dtree )
deflate_state* s;
ct_data* ltree; /* literal tree */
ct_data* dtree; /* distance tree */
{
	unsigned dist;      /* distance of matched string */
	int lc;             /* match length or unmatched char (if dist == 0) */
	unsigned lx = 0;    /* running index in l_buf */
	unsigned code;      /* the code to send */
	int extra;          /* number of extra bits to send */

	if ( s->last_lit != 0 ) do
		{
			dist = s->d_buf[lx];
			lc = s->l_buf[lx++];

			if ( dist == 0 )
			{
				send_code( s, lc, ltree ); /* send a literal byte */
				Tracecv( isgraph( lc ), ( stderr, " '%c' ", lc ) );
			}
			else
			{
				/* Here, lc is the match length - MIN_MATCH */
				code = _length_code[lc];
				send_code( s, code + LITERALS + 1, ltree ); /* send the length code */
				extra = extra_lbits[code];

				if ( extra != 0 )
				{
					lc -= base_length[code];
					send_bits( s, lc, extra );     /* send the extra length bits */
				}

				dist--; /* dist is now the match distance - 1 */
				code = d_code( dist );
				Assert ( code < D_CODES, "bad d_code" );

				send_code( s, code, dtree );     /* send the distance code */
				extra = extra_dbits[code];

				if ( extra != 0 )
				{
					dist -= base_dist[code];
					send_bits( s, dist, extra ); /* send the extra distance bits */
				}
			} /* literal or match pair ? */

			/* Check that the overlay between pending_buf and d_buf+l_buf is ok: */
			Assert( ( uInt )( s->pending ) < s->lit_bufsize + 2 * lx,
			        "pendingBuf overflow" );

		}
		while ( lx < s->last_lit );

	send_code( s, END_BLOCK, ltree );
	s->last_eob_len = ltree[END_BLOCK].Len;
}

/* ===========================================================================
 * Check if the data type is TEXT or BINARY, using the following algorithm:
 * - TEXT if the two conditions below are satisfied:
 *    a) There are no non-portable control characters belonging to the
 *       "black list" (0..6, 14..25, 28..31).
 *    b) There is at least one printable character belonging to the
 *       "white list" (9 {TAB}, 10 {LF}, 13 {CR}, 32..255).
 * - BINARY otherwise.
 * - The following partially-portable control characters form a
 *   "gray list" that is ignored in this detection algorithm:
 *   (7 {BEL}, 8 {BS}, 11 {VT}, 12 {FF}, 26 {SUB}, 27 {ESC}).
 * IN assertion: the fields Freq of dyn_ltree are set.
 */
local int detect_data_type( s )
deflate_state* s;
{
	/* black_mask is the bit mask of black-listed bytes
	 * set bits 0..6, 14..25, and 28..31
	 * 0xf3ffc07f = binary 11110011111111111100000001111111
	 */
	unsigned long black_mask = 0xf3ffc07fUL;
	int n;

	/* Check for non-textual ("black-listed") bytes. */
	for ( n = 0; n <= 31; n++, black_mask >>= 1 )
		if ( ( black_mask & 1 ) && ( s->dyn_ltree[n].Freq != 0 ) )
		{
			return Z_BINARY;
		}

	/* Check for textual ("white-listed") bytes. */
	if ( s->dyn_ltree[9].Freq != 0 || s->dyn_ltree[10].Freq != 0
	     || s->dyn_ltree[13].Freq != 0 )
	{
		return Z_TEXT;
	}

	for ( n = 32; n < LITERALS; n++ )
		if ( s->dyn_ltree[n].Freq != 0 )
		{
			return Z_TEXT;
		}

	/* There are no "black-listed" or "white-listed" bytes:
	 * this stream either is empty or has tolerated ("gray-listed") bytes only.
	 */
	return Z_BINARY;
}

/* ===========================================================================
 * Reverse the first len bits of a code, using straightforward code (a faster
 * method would use a table)
 * IN assertion: 1 <= len <= 15
 */
local unsigned bi_reverse( code, len )
unsigned code; /* the value to invert */
int len;       /* its bit length */
{
	register unsigned res = 0;

	do
	{
		res |= code & 1;
		code >>= 1, res <<= 1;
	}
	while ( --len > 0 );

	return res >> 1;
}

/* ===========================================================================
 * Flush the bit buffer, keeping at most 7 bits in it.
 */
local void bi_flush( s )
deflate_state* s;
{
	if ( s->bi_valid == 16 )
	{
		put_short( s, s->bi_buf );
		s->bi_buf = 0;
		s->bi_valid = 0;
	}
	else if ( s->bi_valid >= 8 )
	{
		put_byte( s, ( Byte )s->bi_buf );
		s->bi_buf >>= 8;
		s->bi_valid -= 8;
	}
}

/* ===========================================================================
 * Flush the bit buffer and align the output on a byte boundary
 */
local void bi_windup( s )
deflate_state* s;
{
	if ( s->bi_valid > 8 )
	{
		put_short( s, s->bi_buf );
	}
	else if ( s->bi_valid > 0 )
	{
		put_byte( s, ( Byte )s->bi_buf );
	}

	s->bi_buf = 0;
	s->bi_valid = 0;
#ifdef DEBUG
	s->bits_sent = ( s->bits_sent + 7 ) & ~7;
#endif
}

/* ===========================================================================
 * Copy a stored block, storing first the length and its
 * one's complement if requested.
 */
local void copy_block( s, buf, len, header )
deflate_state* s;
charf*    buf;    /* the input data */
unsigned len;     /* its length */
int      header;  /* true if block header must be written */
{
	bi_windup( s );      /* align on byte boundary */
	s->last_eob_len = 8; /* enough lookahead for inflate */

	if ( header )
	{
		put_short( s, ( ush )len );
		put_short( s, ( ush )~len );
#ifdef DEBUG
		s->bits_sent += 2 * 16;
#endif
	}

#ifdef DEBUG
	s->bits_sent += ( ulg )len << 3;
#endif

	while ( len-- )
	{
		put_byte( s, *buf++ );
	}
}

///// util.c

#ifndef NO_DUMMY_DECL
/// struct internal_state      {int dummy;}; /* for buggy compilers */
#endif

const char* const z_errmsg[10] =
{
	"need dictionary",     /* Z_NEED_DICT       2  */
	"stream end",          /* Z_STREAM_END      1  */
	"",                    /* Z_OK              0  */
	"file error",          /* Z_ERRNO         (-1) */
	"stream error",        /* Z_STREAM_ERROR  (-2) */
	"data error",          /* Z_DATA_ERROR    (-3) */
	"insufficient memory", /* Z_MEM_ERROR     (-4) */
	"buffer error",        /* Z_BUF_ERROR     (-5) */
	"incompatible version",/* Z_VERSION_ERROR (-6) */
	""
};


const char* ZEXPORT zlibVersion()
{
	return ZLIB_VERSION;
}

uLong ZEXPORT zlibCompileFlags()
{
	uLong flags;

	flags = 0;

	switch ( ( int )( sizeof( uInt ) ) )
	{
		case 2:
			break;

		case 4:
			flags += 1;
			break;

		case 8:
			flags += 2;
			break;

		default:
			flags += 3;
	}

	switch ( ( int )( sizeof( uLong ) ) )
	{
		case 2:
			break;

		case 4:
			flags += 1 << 2;
			break;

		case 8:
			flags += 2 << 2;
			break;

		default:
			flags += 3 << 2;
	}

	switch ( ( int )( sizeof( voidpf ) ) )
	{
		case 2:
			break;

		case 4:
			flags += 1 << 4;
			break;

		case 8:
			flags += 2 << 4;
			break;

		default:
			flags += 3 << 4;
	}

	switch ( ( int )( sizeof( z_off_t ) ) )
	{
		case 2:
			break;

		case 4:
			flags += 1 << 6;
			break;

		case 8:
			flags += 2 << 6;
			break;

		default:
			flags += 3 << 6;
	}

#ifdef DEBUG
	flags += 1 << 8;
#endif
#if defined(ASMV) || defined(ASMINF)
	flags += 1 << 9;
#endif
#ifdef ZLIB_WINAPI
	flags += 1 << 10;
#endif
#ifdef BUILDFIXED
	flags += 1 << 12;
#endif
#ifdef DYNAMIC_CRC_TABLE
	flags += 1 << 13;
#endif
#ifdef NO_GZCOMPRESS
	flags += 1L << 16;
#endif
#ifdef NO_GZIP
	flags += 1L << 17;
#endif
#ifdef PKZIP_BUG_WORKAROUND
	flags += 1L << 20;
#endif
#ifdef FASTEST
	flags += 1L << 21;
#endif
#ifdef STDC
#  ifdef NO_vsnprintf
	flags += 1L << 25;
#    ifdef HAS_vsprintf_void
	flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_vsnprintf_void
	flags += 1L << 26;
#    endif
#  endif
#else
	flags += 1L << 24;
#  ifdef NO_snprintf
	flags += 1L << 25;
#    ifdef HAS_sprintf_void
	flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_snprintf_void
	flags += 1L << 26;
#    endif
#  endif
#endif
	return flags;
}

#ifdef DEBUG

#  ifndef verbose
#    define verbose 0
#  endif
int ZLIB_INTERNAL z_verbose = verbose;

void ZLIB_INTERNAL z_error ( m )
char* m;
{
	fprintf( stderr, "%s\n", m );
	exit( 1 );
}
#endif

/* exported to allow conversion of error code to string for compress() and
 * uncompress()
 */
const char* ZEXPORT zError( err )
int err;
{
	return ERR_MSG( err );
}

#if defined(_WIN32_WCE)
/* The Microsoft C Run-Time Library for Windows CE doesn't have
 * errno.  We define it as a global variable to simplify porting.
 * Its value is always 0 and should not be used.
 */
int errno = 0;
#endif

#ifndef HAVE_MEMCPY

void ZLIB_INTERNAL zmemcpy( dest, source, len )
Bytef* dest;
const Bytef* source;
uInt  len;
{
	if ( len == 0 ) { return; }

	do
	{
		*dest++ = *source++; /* ??? to be unrolled */
	}
	while ( --len != 0 );
}

int ZLIB_INTERNAL zmemcmp( s1, s2, len )
const Bytef* s1;
const Bytef* s2;
uInt  len;
{
	uInt j;

	for ( j = 0; j < len; j++ )
	{
		if ( s1[j] != s2[j] ) { return 2 * ( s1[j] > s2[j] ) - 1; }
	}

	return 0;
}

void ZLIB_INTERNAL zmemzero( dest, len )
Bytef* dest;
uInt  len;
{
	if ( len == 0 ) { return; }

	do
	{
		*dest++ = 0;  /* ??? to be unrolled */
	}
	while ( --len != 0 );
}
#endif


#ifdef SYS16BIT

#ifdef __TURBOC__
/* Turbo C in 16-bit mode */

#  define MY_ZCALLOC

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

local int next_ptr = 0;

typedef struct ptr_table_s
{
	voidpf org_ptr;
	voidpf new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];
/* This table is used to remember the original form of pointers
 * to large buffers (64K). Such pointers are normalized with a zero offset.
 * Since MSDOS is not a preemptive multitasking OS, this table is not
 * protected from concurrent access. This hack doesn't work anyway on
 * a protected system like OS/2. Use Microsoft C instead.
 */

voidpf ZLIB_INTERNAL zcalloc ( voidpf opaque, unsigned items, unsigned size )
{
	voidpf buf = opaque; /* just to make some compilers happy */
	ulg bsize = ( ulg )items * size;

	/* If we allocate less than 65520 bytes, we assume that farmalloc
	 * will return a usable pointer which doesn't have to be normalized.
	 */
	if ( bsize < 65520L )
	{
		buf = farmalloc( bsize );

		if ( *( ush* )&buf != 0 ) { return buf; }
	}
	else
	{
		buf = farmalloc( bsize + 16L );
	}

	if ( buf == NULL || next_ptr >= MAX_PTR ) { return NULL; }

	table[next_ptr].org_ptr = buf;

	/* Normalize the pointer to seg:0 */
	*( ( ush* )&buf + 1 ) += ( ( ush )( ( uch* )buf - 0 ) + 15 ) >> 4;
	*( ush* )&buf = 0;
	table[next_ptr++].new_ptr = buf;
	return buf;
}

void ZLIB_INTERNAL zcfree ( voidpf opaque, voidpf ptr )
{
	int n;

	if ( *( ush* )&ptr != 0 ) /* object < 64K */
	{
		farfree( ptr );
		return;
	}

	/* Find the original pointer */
	for ( n = 0; n < next_ptr; n++ )
	{
		if ( ptr != table[n].new_ptr ) { continue; }

		farfree( table[n].org_ptr );

		while ( ++n < next_ptr )
		{
			table[n - 1] = table[n];
		}

		next_ptr--;
		return;
	}

	ptr = opaque; /* just to make some compilers happy */
	Assert( 0, "zcfree: ptr not found" );
}

#endif /* __TURBOC__ */


#ifdef M_I86
/* Microsoft C in 16-bit mode */

#  define MY_ZCALLOC

#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

voidpf ZLIB_INTERNAL zcalloc ( voidpf opaque, uInt items, uInt size )
{
	if ( opaque ) { opaque = 0; } /* to make compiler happy */

	return _halloc( ( long )items, size );
}

void ZLIB_INTERNAL zcfree ( voidpf opaque, voidpf ptr )
{
	if ( opaque ) { opaque = 0; } /* to make compiler happy */

	_hfree( ptr );
}

#endif /* M_I86 */

#endif /* SYS16BIT */


#ifndef MY_ZCALLOC /* Any system without a special alloc function */

#ifndef STDC
extern voidp  malloc OF( ( uInt size ) );
extern voidp  calloc OF( ( uInt items, uInt size ) );
extern void   free   OF( ( voidpf ptr ) );
#endif

voidpf ZLIB_INTERNAL zcalloc ( opaque, items, size )
voidpf opaque;
unsigned items;
unsigned size;
{
	if ( opaque ) { items += size - size; } /* make compiler happy */

	return sizeof( uInt ) > 2 ? ( voidpf )malloc( items * size ) :
	       ( voidpf )calloc( items, size );
}

void ZLIB_INTERNAL zcfree ( opaque, ptr )
voidpf opaque;
voidpf ptr;
{
	free( ptr );

	if ( opaque ) { return; } /* make compiler happy */
}

#endif /* MY_ZCALLOC */

/////////// adler32

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2007 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */


#define local static

local uLong adler32_combine_( uLong adler1, uLong adler2, z_off64_t len2 );

#define BASE 65521UL    /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware */
#ifdef NO_DIVIDE
#  define MOD(a) \
    do { \
        if (a >= (BASE << 16)) a -= (BASE << 16); \
        if (a >= (BASE << 15)) a -= (BASE << 15); \
        if (a >= (BASE << 14)) a -= (BASE << 14); \
        if (a >= (BASE << 13)) a -= (BASE << 13); \
        if (a >= (BASE << 12)) a -= (BASE << 12); \
        if (a >= (BASE << 11)) a -= (BASE << 11); \
        if (a >= (BASE << 10)) a -= (BASE << 10); \
        if (a >= (BASE << 9)) a -= (BASE << 9); \
        if (a >= (BASE << 8)) a -= (BASE << 8); \
        if (a >= (BASE << 7)) a -= (BASE << 7); \
        if (a >= (BASE << 6)) a -= (BASE << 6); \
        if (a >= (BASE << 5)) a -= (BASE << 5); \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#  define MOD4(a) \
    do { \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD4(a) a %= BASE
#endif

/* ========================================================================= */
uLong ZEXPORT adler32( adler, buf, len )
uLong adler;
const Bytef* buf;
uInt len;
{
	unsigned long sum2;
	unsigned n;

	/* split Adler-32 into component sums */
	sum2 = ( adler >> 16 ) & 0xffff;
	adler &= 0xffff;

	/* in case user likes doing a byte at a time, keep it fast */
	if ( len == 1 )
	{
		adler += buf[0];

		if ( adler >= BASE )
		{
			adler -= BASE;
		}

		sum2 += adler;

		if ( sum2 >= BASE )
		{
			sum2 -= BASE;
		}

		return adler | ( sum2 << 16 );
	}

	/* initial Adler-32 value (deferred check for len == 1 speed) */
	if ( buf == Z_NULL )
	{
		return 1L;
	}

	/* in case short lengths are provided, keep it somewhat fast */
	if ( len < 16 )
	{
		while ( len-- )
		{
			adler += *buf++;
			sum2 += adler;
		}

		if ( adler >= BASE )
		{
			adler -= BASE;
		}

		MOD4( sum2 );           /* only added so many BASE's */
		return adler | ( sum2 << 16 );
	}

	/* do length NMAX blocks -- requires just one modulo operation */
	while ( len >= NMAX )
	{
		len -= NMAX;
		n = NMAX / 16;          /* NMAX is divisible by 16 */

		do
		{
			DO16( buf );        /* 16 sums unrolled */
			buf += 16;
		}
		while ( --n );

		MOD( adler );
		MOD( sum2 );
	}

	/* do remaining bytes (less than NMAX, still just one modulo) */
	if ( len )                  /* avoid modulos if none remaining */
	{
		while ( len >= 16 )
		{
			len -= 16;
			DO16( buf );
			buf += 16;
		}

		while ( len-- )
		{
			adler += *buf++;
			sum2 += adler;
		}

		MOD( adler );
		MOD( sum2 );
	}

	/* return recombined sums */
	return adler | ( sum2 << 16 );
}

/* ========================================================================= */
local uLong adler32_combine_( adler1, adler2, len2 )
uLong adler1;
uLong adler2;
z_off64_t len2;
{
	unsigned long sum1;
	unsigned long sum2;
	unsigned rem;

	/* the derivation of this formula is left as an exercise for the reader */
	rem = ( unsigned )( len2 % BASE );
	sum1 = adler1 & 0xffff;
	sum2 = rem * sum1;
	MOD( sum2 );
	sum1 += ( adler2 & 0xffff ) + BASE - 1;
	sum2 += ( ( adler1 >> 16 ) & 0xffff ) + ( ( adler2 >> 16 ) & 0xffff ) + BASE - rem;

	if ( sum1 >= BASE ) { sum1 -= BASE; }

	if ( sum1 >= BASE ) { sum1 -= BASE; }

	if ( sum2 >= ( BASE << 1 ) ) { sum2 -= ( BASE << 1 ); }

	if ( sum2 >= BASE ) { sum2 -= BASE; }

	return sum1 | ( sum2 << 16 );
}

/* ========================================================================= */
uLong ZEXPORT adler32_combine( adler1, adler2, len2 )
uLong adler1;
uLong adler2;
z_off_t len2;
{
	return adler32_combine_( adler1, adler2, len2 );
}

uLong ZEXPORT adler32_combine64( adler1, adler2, len2 )
uLong adler1;
uLong adler2;
z_off64_t len2;
{
	return adler32_combine_( adler1, adler2, len2 );
}

#undef DO1
#undef DO2
#undef DO4
#undef DO8
#undef DO16

///// crc32

/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2006, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
 * CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
 * tables for updating the shift register in one step with three exclusive-ors
 * instead of four steps with four exclusive-ors.  This results in about a
 * factor of two increase in speed on a Power PC G4 (PPC7455) using gcc -O3.
 */

/* @(#) $Id$ */

/*
  Note on the use of DYNAMIC_CRC_TABLE: there is no mutex or semaphore
  protection on the static variables used to control the first-use generation
  of the crc tables.  Therefore, if you #define DYNAMIC_CRC_TABLE, you should
  first call get_crc_table() to initialize the tables before allowing more than
  one thread to use crc32().
 */

#ifdef MAKECRCH
#  include <stdio.h>
#  ifndef DYNAMIC_CRC_TABLE
#    define DYNAMIC_CRC_TABLE
#  endif /* !DYNAMIC_CRC_TABLE */
#endif /* MAKECRCH */

//#include "zutil.h"      /* for STDC and FAR definitions */

#define local static

/* Find a four-byte integer type for crc32_little() and crc32_big(). */
#ifndef NOBYFOUR
#  ifdef STDC           /* need ANSI C limits.h to determine sizes */
#    include <limits.h>
#    define BYFOUR
#    if (UINT_MAX == 0xffffffffUL)
typedef unsigned int u4;
#    else
#      if (ULONG_MAX == 0xffffffffUL)
typedef unsigned long u4;
#      else
#        if (USHRT_MAX == 0xffffffffUL)
typedef unsigned short u4;
#        else
#          undef BYFOUR     /* can't find a four-byte integer type! */
#        endif
#      endif
#    endif
#  endif /* STDC */
#endif /* !NOBYFOUR */

/* Definitions for doing the crc four data bytes at a time. */
#ifdef BYFOUR
#  define REV(w) ((((w)>>24)&0xff)+(((w)>>8)&0xff00)+ \
                (((w)&0xff00)<<8)+(((w)&0xff)<<24))
local unsigned long crc32_little OF( ( unsigned long,
                                       const unsigned char FAR*, unsigned ) );
local unsigned long crc32_big OF( ( unsigned long,
                                    const unsigned char FAR*, unsigned ) );
#  define TBLS 8
#else
#  define TBLS 1
#endif /* BYFOUR */

/* Local functions for crc concatenation */
local unsigned long gf2_matrix_times OF( ( unsigned long* mat,
                                           unsigned long vec ) );
local void gf2_matrix_square OF( ( unsigned long* square, unsigned long* mat ) );
local uLong crc32_combine_( uLong crc1, uLong crc2, z_off64_t len2 );


#ifdef DYNAMIC_CRC_TABLE

local volatile int crc_table_empty = 1;
local unsigned long FAR crc_table[TBLS][256];
local void make_crc_table OF( ( void ) );
#ifdef MAKECRCH
local void write_table OF( ( FILE*, const unsigned long FAR* ) );
#endif /* MAKECRCH */
/*
  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The first table is simply the CRC of all possible eight bit values.  This is
  all the information needed to generate CRCs on data a byte at a time for all
  combinations of CRC register values and incoming bytes.  The remaining tables
  allow for word-at-a-time CRC calculation for both big-endian and little-
  endian machines, where a word is four bytes.
*/
local void make_crc_table()
{
	unsigned long c;
	int n, k;
	unsigned long poly;                 /* polynomial exclusive-or pattern */
	/* terms of polynomial defining this crc (except x^32): */
	static volatile int first = 1;      /* flag to limit concurrent making */
	static const unsigned char p[] = {0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26};

	/* See if another task is already doing this (not thread-safe, but better
	   than nothing -- significantly reduces duration of vulnerability in
	   case the advice about DYNAMIC_CRC_TABLE is ignored) */
	if ( first )
	{
		first = 0;

		/* make exclusive-or pattern from polynomial (0xedb88320UL) */
		poly = 0UL;

		for ( n = 0; n < sizeof( p ) / sizeof( unsigned char ); n++ )
		{
			poly |= 1UL << ( 31 - p[n] );
		}

		/* generate a crc for every 8-bit value */
		for ( n = 0; n < 256; n++ )
		{
			c = ( unsigned long )n;

			for ( k = 0; k < 8; k++ )
			{
				c = c & 1 ? poly ^ ( c >> 1 ) : c >> 1;
			}

			crc_table[0][n] = c;
		}

#ifdef BYFOUR

		/* generate crc for each value followed by one, two, and three zeros,
		   and then the byte reversal of those as well as the first table */
		for ( n = 0; n < 256; n++ )
		{
			c = crc_table[0][n];
			crc_table[4][n] = REV( c );

			for ( k = 1; k < 4; k++ )
			{
				c = crc_table[0][c & 0xff] ^ ( c >> 8 );
				crc_table[k][n] = c;
				crc_table[k + 4][n] = REV( c );
			}
		}

#endif /* BYFOUR */

		crc_table_empty = 0;
	}
	else        /* not first */
	{
		/* wait for the other guy to finish (not efficient, but rare) */
		while ( crc_table_empty )
			;
	}

#ifdef MAKECRCH
	/* write out CRC tables to crc32.h */
	{
		FILE* out;

		out = fopen( "crc32.h", "w" );

		if ( out == NULL ) { return; }

		fprintf( out, "/* crc32.h -- tables for rapid CRC calculation\n" );
		fprintf( out, " * Generated automatically by crc32.c\n */\n\n" );
		fprintf( out, "local const unsigned long FAR " );
		fprintf( out, "crc_table[TBLS][256] =\n{\n  {\n" );
		write_table( out, crc_table[0] );
#  ifdef BYFOUR
		fprintf( out, "#ifdef BYFOUR\n" );

		for ( k = 1; k < 8; k++ )
		{
			fprintf( out, "  },\n  {\n" );
			write_table( out, crc_table[k] );
		}

		fprintf( out, "#endif\n" );
#  endif /* BYFOUR */
		fprintf( out, "  }\n};\n" );
		fclose( out );
	}
#endif /* MAKECRCH */
}

#ifdef MAKECRCH
local void write_table( out, table )
FILE* out;
const unsigned long FAR* table;
{
	int n;

	for ( n = 0; n < 256; n++ )
		fprintf( out, "%s0x%08lxUL%s", n % 5 ? "" : "    ", table[n],
		         n == 255 ? "\n" : ( n % 5 == 4 ? ",\n" : ", " ) );
}
#endif /* MAKECRCH */

#else /* !DYNAMIC_CRC_TABLE */
/* ========================================================================
 * Tables of CRC-32s of all single-byte values, made by make_crc_table().
 */

// #include "crc32.h"

/* crc32.h -- tables for rapid CRC calculation
 * Generated automatically by crc32.c
 */

local const unsigned long FAR crc_table[TBLS][256] =
{
	{
		0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
		0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
		0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
		0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
		0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
		0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
		0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
		0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
		0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
		0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
		0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
		0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
		0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
		0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
		0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
		0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
		0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
		0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
		0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
		0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
		0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
		0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
		0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
		0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
		0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
		0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
		0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
		0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
		0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
		0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
		0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
		0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
		0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
		0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
		0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
		0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
		0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
		0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
		0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
		0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
		0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
		0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
		0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
		0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
		0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
		0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
		0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
		0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
		0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
		0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
		0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
		0x2d02ef8dUL
#ifdef BYFOUR
	},
	{
		0x00000000UL, 0x191b3141UL, 0x32366282UL, 0x2b2d53c3UL, 0x646cc504UL,
		0x7d77f445UL, 0x565aa786UL, 0x4f4196c7UL, 0xc8d98a08UL, 0xd1c2bb49UL,
		0xfaefe88aUL, 0xe3f4d9cbUL, 0xacb54f0cUL, 0xb5ae7e4dUL, 0x9e832d8eUL,
		0x87981ccfUL, 0x4ac21251UL, 0x53d92310UL, 0x78f470d3UL, 0x61ef4192UL,
		0x2eaed755UL, 0x37b5e614UL, 0x1c98b5d7UL, 0x05838496UL, 0x821b9859UL,
		0x9b00a918UL, 0xb02dfadbUL, 0xa936cb9aUL, 0xe6775d5dUL, 0xff6c6c1cUL,
		0xd4413fdfUL, 0xcd5a0e9eUL, 0x958424a2UL, 0x8c9f15e3UL, 0xa7b24620UL,
		0xbea97761UL, 0xf1e8e1a6UL, 0xe8f3d0e7UL, 0xc3de8324UL, 0xdac5b265UL,
		0x5d5daeaaUL, 0x44469febUL, 0x6f6bcc28UL, 0x7670fd69UL, 0x39316baeUL,
		0x202a5aefUL, 0x0b07092cUL, 0x121c386dUL, 0xdf4636f3UL, 0xc65d07b2UL,
		0xed705471UL, 0xf46b6530UL, 0xbb2af3f7UL, 0xa231c2b6UL, 0x891c9175UL,
		0x9007a034UL, 0x179fbcfbUL, 0x0e848dbaUL, 0x25a9de79UL, 0x3cb2ef38UL,
		0x73f379ffUL, 0x6ae848beUL, 0x41c51b7dUL, 0x58de2a3cUL, 0xf0794f05UL,
		0xe9627e44UL, 0xc24f2d87UL, 0xdb541cc6UL, 0x94158a01UL, 0x8d0ebb40UL,
		0xa623e883UL, 0xbf38d9c2UL, 0x38a0c50dUL, 0x21bbf44cUL, 0x0a96a78fUL,
		0x138d96ceUL, 0x5ccc0009UL, 0x45d73148UL, 0x6efa628bUL, 0x77e153caUL,
		0xbabb5d54UL, 0xa3a06c15UL, 0x888d3fd6UL, 0x91960e97UL, 0xded79850UL,
		0xc7cca911UL, 0xece1fad2UL, 0xf5facb93UL, 0x7262d75cUL, 0x6b79e61dUL,
		0x4054b5deUL, 0x594f849fUL, 0x160e1258UL, 0x0f152319UL, 0x243870daUL,
		0x3d23419bUL, 0x65fd6ba7UL, 0x7ce65ae6UL, 0x57cb0925UL, 0x4ed03864UL,
		0x0191aea3UL, 0x188a9fe2UL, 0x33a7cc21UL, 0x2abcfd60UL, 0xad24e1afUL,
		0xb43fd0eeUL, 0x9f12832dUL, 0x8609b26cUL, 0xc94824abUL, 0xd05315eaUL,
		0xfb7e4629UL, 0xe2657768UL, 0x2f3f79f6UL, 0x362448b7UL, 0x1d091b74UL,
		0x04122a35UL, 0x4b53bcf2UL, 0x52488db3UL, 0x7965de70UL, 0x607eef31UL,
		0xe7e6f3feUL, 0xfefdc2bfUL, 0xd5d0917cUL, 0xcccba03dUL, 0x838a36faUL,
		0x9a9107bbUL, 0xb1bc5478UL, 0xa8a76539UL, 0x3b83984bUL, 0x2298a90aUL,
		0x09b5fac9UL, 0x10aecb88UL, 0x5fef5d4fUL, 0x46f46c0eUL, 0x6dd93fcdUL,
		0x74c20e8cUL, 0xf35a1243UL, 0xea412302UL, 0xc16c70c1UL, 0xd8774180UL,
		0x9736d747UL, 0x8e2de606UL, 0xa500b5c5UL, 0xbc1b8484UL, 0x71418a1aUL,
		0x685abb5bUL, 0x4377e898UL, 0x5a6cd9d9UL, 0x152d4f1eUL, 0x0c367e5fUL,
		0x271b2d9cUL, 0x3e001cddUL, 0xb9980012UL, 0xa0833153UL, 0x8bae6290UL,
		0x92b553d1UL, 0xddf4c516UL, 0xc4eff457UL, 0xefc2a794UL, 0xf6d996d5UL,
		0xae07bce9UL, 0xb71c8da8UL, 0x9c31de6bUL, 0x852aef2aUL, 0xca6b79edUL,
		0xd37048acUL, 0xf85d1b6fUL, 0xe1462a2eUL, 0x66de36e1UL, 0x7fc507a0UL,
		0x54e85463UL, 0x4df36522UL, 0x02b2f3e5UL, 0x1ba9c2a4UL, 0x30849167UL,
		0x299fa026UL, 0xe4c5aeb8UL, 0xfdde9ff9UL, 0xd6f3cc3aUL, 0xcfe8fd7bUL,
		0x80a96bbcUL, 0x99b25afdUL, 0xb29f093eUL, 0xab84387fUL, 0x2c1c24b0UL,
		0x350715f1UL, 0x1e2a4632UL, 0x07317773UL, 0x4870e1b4UL, 0x516bd0f5UL,
		0x7a468336UL, 0x635db277UL, 0xcbfad74eUL, 0xd2e1e60fUL, 0xf9ccb5ccUL,
		0xe0d7848dUL, 0xaf96124aUL, 0xb68d230bUL, 0x9da070c8UL, 0x84bb4189UL,
		0x03235d46UL, 0x1a386c07UL, 0x31153fc4UL, 0x280e0e85UL, 0x674f9842UL,
		0x7e54a903UL, 0x5579fac0UL, 0x4c62cb81UL, 0x8138c51fUL, 0x9823f45eUL,
		0xb30ea79dUL, 0xaa1596dcUL, 0xe554001bUL, 0xfc4f315aUL, 0xd7626299UL,
		0xce7953d8UL, 0x49e14f17UL, 0x50fa7e56UL, 0x7bd72d95UL, 0x62cc1cd4UL,
		0x2d8d8a13UL, 0x3496bb52UL, 0x1fbbe891UL, 0x06a0d9d0UL, 0x5e7ef3ecUL,
		0x4765c2adUL, 0x6c48916eUL, 0x7553a02fUL, 0x3a1236e8UL, 0x230907a9UL,
		0x0824546aUL, 0x113f652bUL, 0x96a779e4UL, 0x8fbc48a5UL, 0xa4911b66UL,
		0xbd8a2a27UL, 0xf2cbbce0UL, 0xebd08da1UL, 0xc0fdde62UL, 0xd9e6ef23UL,
		0x14bce1bdUL, 0x0da7d0fcUL, 0x268a833fUL, 0x3f91b27eUL, 0x70d024b9UL,
		0x69cb15f8UL, 0x42e6463bUL, 0x5bfd777aUL, 0xdc656bb5UL, 0xc57e5af4UL,
		0xee530937UL, 0xf7483876UL, 0xb809aeb1UL, 0xa1129ff0UL, 0x8a3fcc33UL,
		0x9324fd72UL
	},
	{
		0x00000000UL, 0x01c26a37UL, 0x0384d46eUL, 0x0246be59UL, 0x0709a8dcUL,
		0x06cbc2ebUL, 0x048d7cb2UL, 0x054f1685UL, 0x0e1351b8UL, 0x0fd13b8fUL,
		0x0d9785d6UL, 0x0c55efe1UL, 0x091af964UL, 0x08d89353UL, 0x0a9e2d0aUL,
		0x0b5c473dUL, 0x1c26a370UL, 0x1de4c947UL, 0x1fa2771eUL, 0x1e601d29UL,
		0x1b2f0bacUL, 0x1aed619bUL, 0x18abdfc2UL, 0x1969b5f5UL, 0x1235f2c8UL,
		0x13f798ffUL, 0x11b126a6UL, 0x10734c91UL, 0x153c5a14UL, 0x14fe3023UL,
		0x16b88e7aUL, 0x177ae44dUL, 0x384d46e0UL, 0x398f2cd7UL, 0x3bc9928eUL,
		0x3a0bf8b9UL, 0x3f44ee3cUL, 0x3e86840bUL, 0x3cc03a52UL, 0x3d025065UL,
		0x365e1758UL, 0x379c7d6fUL, 0x35dac336UL, 0x3418a901UL, 0x3157bf84UL,
		0x3095d5b3UL, 0x32d36beaUL, 0x331101ddUL, 0x246be590UL, 0x25a98fa7UL,
		0x27ef31feUL, 0x262d5bc9UL, 0x23624d4cUL, 0x22a0277bUL, 0x20e69922UL,
		0x2124f315UL, 0x2a78b428UL, 0x2bbade1fUL, 0x29fc6046UL, 0x283e0a71UL,
		0x2d711cf4UL, 0x2cb376c3UL, 0x2ef5c89aUL, 0x2f37a2adUL, 0x709a8dc0UL,
		0x7158e7f7UL, 0x731e59aeUL, 0x72dc3399UL, 0x7793251cUL, 0x76514f2bUL,
		0x7417f172UL, 0x75d59b45UL, 0x7e89dc78UL, 0x7f4bb64fUL, 0x7d0d0816UL,
		0x7ccf6221UL, 0x798074a4UL, 0x78421e93UL, 0x7a04a0caUL, 0x7bc6cafdUL,
		0x6cbc2eb0UL, 0x6d7e4487UL, 0x6f38fadeUL, 0x6efa90e9UL, 0x6bb5866cUL,
		0x6a77ec5bUL, 0x68315202UL, 0x69f33835UL, 0x62af7f08UL, 0x636d153fUL,
		0x612bab66UL, 0x60e9c151UL, 0x65a6d7d4UL, 0x6464bde3UL, 0x662203baUL,
		0x67e0698dUL, 0x48d7cb20UL, 0x4915a117UL, 0x4b531f4eUL, 0x4a917579UL,
		0x4fde63fcUL, 0x4e1c09cbUL, 0x4c5ab792UL, 0x4d98dda5UL, 0x46c49a98UL,
		0x4706f0afUL, 0x45404ef6UL, 0x448224c1UL, 0x41cd3244UL, 0x400f5873UL,
		0x4249e62aUL, 0x438b8c1dUL, 0x54f16850UL, 0x55330267UL, 0x5775bc3eUL,
		0x56b7d609UL, 0x53f8c08cUL, 0x523aaabbUL, 0x507c14e2UL, 0x51be7ed5UL,
		0x5ae239e8UL, 0x5b2053dfUL, 0x5966ed86UL, 0x58a487b1UL, 0x5deb9134UL,
		0x5c29fb03UL, 0x5e6f455aUL, 0x5fad2f6dUL, 0xe1351b80UL, 0xe0f771b7UL,
		0xe2b1cfeeUL, 0xe373a5d9UL, 0xe63cb35cUL, 0xe7fed96bUL, 0xe5b86732UL,
		0xe47a0d05UL, 0xef264a38UL, 0xeee4200fUL, 0xeca29e56UL, 0xed60f461UL,
		0xe82fe2e4UL, 0xe9ed88d3UL, 0xebab368aUL, 0xea695cbdUL, 0xfd13b8f0UL,
		0xfcd1d2c7UL, 0xfe976c9eUL, 0xff5506a9UL, 0xfa1a102cUL, 0xfbd87a1bUL,
		0xf99ec442UL, 0xf85cae75UL, 0xf300e948UL, 0xf2c2837fUL, 0xf0843d26UL,
		0xf1465711UL, 0xf4094194UL, 0xf5cb2ba3UL, 0xf78d95faUL, 0xf64fffcdUL,
		0xd9785d60UL, 0xd8ba3757UL, 0xdafc890eUL, 0xdb3ee339UL, 0xde71f5bcUL,
		0xdfb39f8bUL, 0xddf521d2UL, 0xdc374be5UL, 0xd76b0cd8UL, 0xd6a966efUL,
		0xd4efd8b6UL, 0xd52db281UL, 0xd062a404UL, 0xd1a0ce33UL, 0xd3e6706aUL,
		0xd2241a5dUL, 0xc55efe10UL, 0xc49c9427UL, 0xc6da2a7eUL, 0xc7184049UL,
		0xc25756ccUL, 0xc3953cfbUL, 0xc1d382a2UL, 0xc011e895UL, 0xcb4dafa8UL,
		0xca8fc59fUL, 0xc8c97bc6UL, 0xc90b11f1UL, 0xcc440774UL, 0xcd866d43UL,
		0xcfc0d31aUL, 0xce02b92dUL, 0x91af9640UL, 0x906dfc77UL, 0x922b422eUL,
		0x93e92819UL, 0x96a63e9cUL, 0x976454abUL, 0x9522eaf2UL, 0x94e080c5UL,
		0x9fbcc7f8UL, 0x9e7eadcfUL, 0x9c381396UL, 0x9dfa79a1UL, 0x98b56f24UL,
		0x99770513UL, 0x9b31bb4aUL, 0x9af3d17dUL, 0x8d893530UL, 0x8c4b5f07UL,
		0x8e0de15eUL, 0x8fcf8b69UL, 0x8a809decUL, 0x8b42f7dbUL, 0x89044982UL,
		0x88c623b5UL, 0x839a6488UL, 0x82580ebfUL, 0x801eb0e6UL, 0x81dcdad1UL,
		0x8493cc54UL, 0x8551a663UL, 0x8717183aUL, 0x86d5720dUL, 0xa9e2d0a0UL,
		0xa820ba97UL, 0xaa6604ceUL, 0xaba46ef9UL, 0xaeeb787cUL, 0xaf29124bUL,
		0xad6fac12UL, 0xacadc625UL, 0xa7f18118UL, 0xa633eb2fUL, 0xa4755576UL,
		0xa5b73f41UL, 0xa0f829c4UL, 0xa13a43f3UL, 0xa37cfdaaUL, 0xa2be979dUL,
		0xb5c473d0UL, 0xb40619e7UL, 0xb640a7beUL, 0xb782cd89UL, 0xb2cddb0cUL,
		0xb30fb13bUL, 0xb1490f62UL, 0xb08b6555UL, 0xbbd72268UL, 0xba15485fUL,
		0xb853f606UL, 0xb9919c31UL, 0xbcde8ab4UL, 0xbd1ce083UL, 0xbf5a5edaUL,
		0xbe9834edUL
	},
	{
		0x00000000UL, 0xb8bc6765UL, 0xaa09c88bUL, 0x12b5afeeUL, 0x8f629757UL,
		0x37def032UL, 0x256b5fdcUL, 0x9dd738b9UL, 0xc5b428efUL, 0x7d084f8aUL,
		0x6fbde064UL, 0xd7018701UL, 0x4ad6bfb8UL, 0xf26ad8ddUL, 0xe0df7733UL,
		0x58631056UL, 0x5019579fUL, 0xe8a530faUL, 0xfa109f14UL, 0x42acf871UL,
		0xdf7bc0c8UL, 0x67c7a7adUL, 0x75720843UL, 0xcdce6f26UL, 0x95ad7f70UL,
		0x2d111815UL, 0x3fa4b7fbUL, 0x8718d09eUL, 0x1acfe827UL, 0xa2738f42UL,
		0xb0c620acUL, 0x087a47c9UL, 0xa032af3eUL, 0x188ec85bUL, 0x0a3b67b5UL,
		0xb28700d0UL, 0x2f503869UL, 0x97ec5f0cUL, 0x8559f0e2UL, 0x3de59787UL,
		0x658687d1UL, 0xdd3ae0b4UL, 0xcf8f4f5aUL, 0x7733283fUL, 0xeae41086UL,
		0x525877e3UL, 0x40edd80dUL, 0xf851bf68UL, 0xf02bf8a1UL, 0x48979fc4UL,
		0x5a22302aUL, 0xe29e574fUL, 0x7f496ff6UL, 0xc7f50893UL, 0xd540a77dUL,
		0x6dfcc018UL, 0x359fd04eUL, 0x8d23b72bUL, 0x9f9618c5UL, 0x272a7fa0UL,
		0xbafd4719UL, 0x0241207cUL, 0x10f48f92UL, 0xa848e8f7UL, 0x9b14583dUL,
		0x23a83f58UL, 0x311d90b6UL, 0x89a1f7d3UL, 0x1476cf6aUL, 0xaccaa80fUL,
		0xbe7f07e1UL, 0x06c36084UL, 0x5ea070d2UL, 0xe61c17b7UL, 0xf4a9b859UL,
		0x4c15df3cUL, 0xd1c2e785UL, 0x697e80e0UL, 0x7bcb2f0eUL, 0xc377486bUL,
		0xcb0d0fa2UL, 0x73b168c7UL, 0x6104c729UL, 0xd9b8a04cUL, 0x446f98f5UL,
		0xfcd3ff90UL, 0xee66507eUL, 0x56da371bUL, 0x0eb9274dUL, 0xb6054028UL,
		0xa4b0efc6UL, 0x1c0c88a3UL, 0x81dbb01aUL, 0x3967d77fUL, 0x2bd27891UL,
		0x936e1ff4UL, 0x3b26f703UL, 0x839a9066UL, 0x912f3f88UL, 0x299358edUL,
		0xb4446054UL, 0x0cf80731UL, 0x1e4da8dfUL, 0xa6f1cfbaUL, 0xfe92dfecUL,
		0x462eb889UL, 0x549b1767UL, 0xec277002UL, 0x71f048bbUL, 0xc94c2fdeUL,
		0xdbf98030UL, 0x6345e755UL, 0x6b3fa09cUL, 0xd383c7f9UL, 0xc1366817UL,
		0x798a0f72UL, 0xe45d37cbUL, 0x5ce150aeUL, 0x4e54ff40UL, 0xf6e89825UL,
		0xae8b8873UL, 0x1637ef16UL, 0x048240f8UL, 0xbc3e279dUL, 0x21e91f24UL,
		0x99557841UL, 0x8be0d7afUL, 0x335cb0caUL, 0xed59b63bUL, 0x55e5d15eUL,
		0x47507eb0UL, 0xffec19d5UL, 0x623b216cUL, 0xda874609UL, 0xc832e9e7UL,
		0x708e8e82UL, 0x28ed9ed4UL, 0x9051f9b1UL, 0x82e4565fUL, 0x3a58313aUL,
		0xa78f0983UL, 0x1f336ee6UL, 0x0d86c108UL, 0xb53aa66dUL, 0xbd40e1a4UL,
		0x05fc86c1UL, 0x1749292fUL, 0xaff54e4aUL, 0x322276f3UL, 0x8a9e1196UL,
		0x982bbe78UL, 0x2097d91dUL, 0x78f4c94bUL, 0xc048ae2eUL, 0xd2fd01c0UL,
		0x6a4166a5UL, 0xf7965e1cUL, 0x4f2a3979UL, 0x5d9f9697UL, 0xe523f1f2UL,
		0x4d6b1905UL, 0xf5d77e60UL, 0xe762d18eUL, 0x5fdeb6ebUL, 0xc2098e52UL,
		0x7ab5e937UL, 0x680046d9UL, 0xd0bc21bcUL, 0x88df31eaUL, 0x3063568fUL,
		0x22d6f961UL, 0x9a6a9e04UL, 0x07bda6bdUL, 0xbf01c1d8UL, 0xadb46e36UL,
		0x15080953UL, 0x1d724e9aUL, 0xa5ce29ffUL, 0xb77b8611UL, 0x0fc7e174UL,
		0x9210d9cdUL, 0x2aacbea8UL, 0x38191146UL, 0x80a57623UL, 0xd8c66675UL,
		0x607a0110UL, 0x72cfaefeUL, 0xca73c99bUL, 0x57a4f122UL, 0xef189647UL,
		0xfdad39a9UL, 0x45115eccUL, 0x764dee06UL, 0xcef18963UL, 0xdc44268dUL,
		0x64f841e8UL, 0xf92f7951UL, 0x41931e34UL, 0x5326b1daUL, 0xeb9ad6bfUL,
		0xb3f9c6e9UL, 0x0b45a18cUL, 0x19f00e62UL, 0xa14c6907UL, 0x3c9b51beUL,
		0x842736dbUL, 0x96929935UL, 0x2e2efe50UL, 0x2654b999UL, 0x9ee8defcUL,
		0x8c5d7112UL, 0x34e11677UL, 0xa9362eceUL, 0x118a49abUL, 0x033fe645UL,
		0xbb838120UL, 0xe3e09176UL, 0x5b5cf613UL, 0x49e959fdUL, 0xf1553e98UL,
		0x6c820621UL, 0xd43e6144UL, 0xc68bceaaUL, 0x7e37a9cfUL, 0xd67f4138UL,
		0x6ec3265dUL, 0x7c7689b3UL, 0xc4caeed6UL, 0x591dd66fUL, 0xe1a1b10aUL,
		0xf3141ee4UL, 0x4ba87981UL, 0x13cb69d7UL, 0xab770eb2UL, 0xb9c2a15cUL,
		0x017ec639UL, 0x9ca9fe80UL, 0x241599e5UL, 0x36a0360bUL, 0x8e1c516eUL,
		0x866616a7UL, 0x3eda71c2UL, 0x2c6fde2cUL, 0x94d3b949UL, 0x090481f0UL,
		0xb1b8e695UL, 0xa30d497bUL, 0x1bb12e1eUL, 0x43d23e48UL, 0xfb6e592dUL,
		0xe9dbf6c3UL, 0x516791a6UL, 0xccb0a91fUL, 0x740cce7aUL, 0x66b96194UL,
		0xde0506f1UL
	},
	{
		0x00000000UL, 0x96300777UL, 0x2c610eeeUL, 0xba510999UL, 0x19c46d07UL,
		0x8ff46a70UL, 0x35a563e9UL, 0xa395649eUL, 0x3288db0eUL, 0xa4b8dc79UL,
		0x1ee9d5e0UL, 0x88d9d297UL, 0x2b4cb609UL, 0xbd7cb17eUL, 0x072db8e7UL,
		0x911dbf90UL, 0x6410b71dUL, 0xf220b06aUL, 0x4871b9f3UL, 0xde41be84UL,
		0x7dd4da1aUL, 0xebe4dd6dUL, 0x51b5d4f4UL, 0xc785d383UL, 0x56986c13UL,
		0xc0a86b64UL, 0x7af962fdUL, 0xecc9658aUL, 0x4f5c0114UL, 0xd96c0663UL,
		0x633d0ffaUL, 0xf50d088dUL, 0xc8206e3bUL, 0x5e10694cUL, 0xe44160d5UL,
		0x727167a2UL, 0xd1e4033cUL, 0x47d4044bUL, 0xfd850dd2UL, 0x6bb50aa5UL,
		0xfaa8b535UL, 0x6c98b242UL, 0xd6c9bbdbUL, 0x40f9bcacUL, 0xe36cd832UL,
		0x755cdf45UL, 0xcf0dd6dcUL, 0x593dd1abUL, 0xac30d926UL, 0x3a00de51UL,
		0x8051d7c8UL, 0x1661d0bfUL, 0xb5f4b421UL, 0x23c4b356UL, 0x9995bacfUL,
		0x0fa5bdb8UL, 0x9eb80228UL, 0x0888055fUL, 0xb2d90cc6UL, 0x24e90bb1UL,
		0x877c6f2fUL, 0x114c6858UL, 0xab1d61c1UL, 0x3d2d66b6UL, 0x9041dc76UL,
		0x0671db01UL, 0xbc20d298UL, 0x2a10d5efUL, 0x8985b171UL, 0x1fb5b606UL,
		0xa5e4bf9fUL, 0x33d4b8e8UL, 0xa2c90778UL, 0x34f9000fUL, 0x8ea80996UL,
		0x18980ee1UL, 0xbb0d6a7fUL, 0x2d3d6d08UL, 0x976c6491UL, 0x015c63e6UL,
		0xf4516b6bUL, 0x62616c1cUL, 0xd8306585UL, 0x4e0062f2UL, 0xed95066cUL,
		0x7ba5011bUL, 0xc1f40882UL, 0x57c40ff5UL, 0xc6d9b065UL, 0x50e9b712UL,
		0xeab8be8bUL, 0x7c88b9fcUL, 0xdf1ddd62UL, 0x492dda15UL, 0xf37cd38cUL,
		0x654cd4fbUL, 0x5861b24dUL, 0xce51b53aUL, 0x7400bca3UL, 0xe230bbd4UL,
		0x41a5df4aUL, 0xd795d83dUL, 0x6dc4d1a4UL, 0xfbf4d6d3UL, 0x6ae96943UL,
		0xfcd96e34UL, 0x468867adUL, 0xd0b860daUL, 0x732d0444UL, 0xe51d0333UL,
		0x5f4c0aaaUL, 0xc97c0dddUL, 0x3c710550UL, 0xaa410227UL, 0x10100bbeUL,
		0x86200cc9UL, 0x25b56857UL, 0xb3856f20UL, 0x09d466b9UL, 0x9fe461ceUL,
		0x0ef9de5eUL, 0x98c9d929UL, 0x2298d0b0UL, 0xb4a8d7c7UL, 0x173db359UL,
		0x810db42eUL, 0x3b5cbdb7UL, 0xad6cbac0UL, 0x2083b8edUL, 0xb6b3bf9aUL,
		0x0ce2b603UL, 0x9ad2b174UL, 0x3947d5eaUL, 0xaf77d29dUL, 0x1526db04UL,
		0x8316dc73UL, 0x120b63e3UL, 0x843b6494UL, 0x3e6a6d0dUL, 0xa85a6a7aUL,
		0x0bcf0ee4UL, 0x9dff0993UL, 0x27ae000aUL, 0xb19e077dUL, 0x44930ff0UL,
		0xd2a30887UL, 0x68f2011eUL, 0xfec20669UL, 0x5d5762f7UL, 0xcb676580UL,
		0x71366c19UL, 0xe7066b6eUL, 0x761bd4feUL, 0xe02bd389UL, 0x5a7ada10UL,
		0xcc4add67UL, 0x6fdfb9f9UL, 0xf9efbe8eUL, 0x43beb717UL, 0xd58eb060UL,
		0xe8a3d6d6UL, 0x7e93d1a1UL, 0xc4c2d838UL, 0x52f2df4fUL, 0xf167bbd1UL,
		0x6757bca6UL, 0xdd06b53fUL, 0x4b36b248UL, 0xda2b0dd8UL, 0x4c1b0aafUL,
		0xf64a0336UL, 0x607a0441UL, 0xc3ef60dfUL, 0x55df67a8UL, 0xef8e6e31UL,
		0x79be6946UL, 0x8cb361cbUL, 0x1a8366bcUL, 0xa0d26f25UL, 0x36e26852UL,
		0x95770cccUL, 0x03470bbbUL, 0xb9160222UL, 0x2f260555UL, 0xbe3bbac5UL,
		0x280bbdb2UL, 0x925ab42bUL, 0x046ab35cUL, 0xa7ffd7c2UL, 0x31cfd0b5UL,
		0x8b9ed92cUL, 0x1daede5bUL, 0xb0c2649bUL, 0x26f263ecUL, 0x9ca36a75UL,
		0x0a936d02UL, 0xa906099cUL, 0x3f360eebUL, 0x85670772UL, 0x13570005UL,
		0x824abf95UL, 0x147ab8e2UL, 0xae2bb17bUL, 0x381bb60cUL, 0x9b8ed292UL,
		0x0dbed5e5UL, 0xb7efdc7cUL, 0x21dfdb0bUL, 0xd4d2d386UL, 0x42e2d4f1UL,
		0xf8b3dd68UL, 0x6e83da1fUL, 0xcd16be81UL, 0x5b26b9f6UL, 0xe177b06fUL,
		0x7747b718UL, 0xe65a0888UL, 0x706a0fffUL, 0xca3b0666UL, 0x5c0b0111UL,
		0xff9e658fUL, 0x69ae62f8UL, 0xd3ff6b61UL, 0x45cf6c16UL, 0x78e20aa0UL,
		0xeed20dd7UL, 0x5483044eUL, 0xc2b30339UL, 0x612667a7UL, 0xf71660d0UL,
		0x4d476949UL, 0xdb776e3eUL, 0x4a6ad1aeUL, 0xdc5ad6d9UL, 0x660bdf40UL,
		0xf03bd837UL, 0x53aebca9UL, 0xc59ebbdeUL, 0x7fcfb247UL, 0xe9ffb530UL,
		0x1cf2bdbdUL, 0x8ac2bacaUL, 0x3093b353UL, 0xa6a3b424UL, 0x0536d0baUL,
		0x9306d7cdUL, 0x2957de54UL, 0xbf67d923UL, 0x2e7a66b3UL, 0xb84a61c4UL,
		0x021b685dUL, 0x942b6f2aUL, 0x37be0bb4UL, 0xa18e0cc3UL, 0x1bdf055aUL,
		0x8def022dUL
	},
	{
		0x00000000UL, 0x41311b19UL, 0x82623632UL, 0xc3532d2bUL, 0x04c56c64UL,
		0x45f4777dUL, 0x86a75a56UL, 0xc796414fUL, 0x088ad9c8UL, 0x49bbc2d1UL,
		0x8ae8effaUL, 0xcbd9f4e3UL, 0x0c4fb5acUL, 0x4d7eaeb5UL, 0x8e2d839eUL,
		0xcf1c9887UL, 0x5112c24aUL, 0x1023d953UL, 0xd370f478UL, 0x9241ef61UL,
		0x55d7ae2eUL, 0x14e6b537UL, 0xd7b5981cUL, 0x96848305UL, 0x59981b82UL,
		0x18a9009bUL, 0xdbfa2db0UL, 0x9acb36a9UL, 0x5d5d77e6UL, 0x1c6c6cffUL,
		0xdf3f41d4UL, 0x9e0e5acdUL, 0xa2248495UL, 0xe3159f8cUL, 0x2046b2a7UL,
		0x6177a9beUL, 0xa6e1e8f1UL, 0xe7d0f3e8UL, 0x2483dec3UL, 0x65b2c5daUL,
		0xaaae5d5dUL, 0xeb9f4644UL, 0x28cc6b6fUL, 0x69fd7076UL, 0xae6b3139UL,
		0xef5a2a20UL, 0x2c09070bUL, 0x6d381c12UL, 0xf33646dfUL, 0xb2075dc6UL,
		0x715470edUL, 0x30656bf4UL, 0xf7f32abbUL, 0xb6c231a2UL, 0x75911c89UL,
		0x34a00790UL, 0xfbbc9f17UL, 0xba8d840eUL, 0x79dea925UL, 0x38efb23cUL,
		0xff79f373UL, 0xbe48e86aUL, 0x7d1bc541UL, 0x3c2ade58UL, 0x054f79f0UL,
		0x447e62e9UL, 0x872d4fc2UL, 0xc61c54dbUL, 0x018a1594UL, 0x40bb0e8dUL,
		0x83e823a6UL, 0xc2d938bfUL, 0x0dc5a038UL, 0x4cf4bb21UL, 0x8fa7960aUL,
		0xce968d13UL, 0x0900cc5cUL, 0x4831d745UL, 0x8b62fa6eUL, 0xca53e177UL,
		0x545dbbbaUL, 0x156ca0a3UL, 0xd63f8d88UL, 0x970e9691UL, 0x5098d7deUL,
		0x11a9ccc7UL, 0xd2fae1ecUL, 0x93cbfaf5UL, 0x5cd76272UL, 0x1de6796bUL,
		0xdeb55440UL, 0x9f844f59UL, 0x58120e16UL, 0x1923150fUL, 0xda703824UL,
		0x9b41233dUL, 0xa76bfd65UL, 0xe65ae67cUL, 0x2509cb57UL, 0x6438d04eUL,
		0xa3ae9101UL, 0xe29f8a18UL, 0x21cca733UL, 0x60fdbc2aUL, 0xafe124adUL,
		0xeed03fb4UL, 0x2d83129fUL, 0x6cb20986UL, 0xab2448c9UL, 0xea1553d0UL,
		0x29467efbUL, 0x687765e2UL, 0xf6793f2fUL, 0xb7482436UL, 0x741b091dUL,
		0x352a1204UL, 0xf2bc534bUL, 0xb38d4852UL, 0x70de6579UL, 0x31ef7e60UL,
		0xfef3e6e7UL, 0xbfc2fdfeUL, 0x7c91d0d5UL, 0x3da0cbccUL, 0xfa368a83UL,
		0xbb07919aUL, 0x7854bcb1UL, 0x3965a7a8UL, 0x4b98833bUL, 0x0aa99822UL,
		0xc9fab509UL, 0x88cbae10UL, 0x4f5def5fUL, 0x0e6cf446UL, 0xcd3fd96dUL,
		0x8c0ec274UL, 0x43125af3UL, 0x022341eaUL, 0xc1706cc1UL, 0x804177d8UL,
		0x47d73697UL, 0x06e62d8eUL, 0xc5b500a5UL, 0x84841bbcUL, 0x1a8a4171UL,
		0x5bbb5a68UL, 0x98e87743UL, 0xd9d96c5aUL, 0x1e4f2d15UL, 0x5f7e360cUL,
		0x9c2d1b27UL, 0xdd1c003eUL, 0x120098b9UL, 0x533183a0UL, 0x9062ae8bUL,
		0xd153b592UL, 0x16c5f4ddUL, 0x57f4efc4UL, 0x94a7c2efUL, 0xd596d9f6UL,
		0xe9bc07aeUL, 0xa88d1cb7UL, 0x6bde319cUL, 0x2aef2a85UL, 0xed796bcaUL,
		0xac4870d3UL, 0x6f1b5df8UL, 0x2e2a46e1UL, 0xe136de66UL, 0xa007c57fUL,
		0x6354e854UL, 0x2265f34dUL, 0xe5f3b202UL, 0xa4c2a91bUL, 0x67918430UL,
		0x26a09f29UL, 0xb8aec5e4UL, 0xf99fdefdUL, 0x3accf3d6UL, 0x7bfde8cfUL,
		0xbc6ba980UL, 0xfd5ab299UL, 0x3e099fb2UL, 0x7f3884abUL, 0xb0241c2cUL,
		0xf1150735UL, 0x32462a1eUL, 0x73773107UL, 0xb4e17048UL, 0xf5d06b51UL,
		0x3683467aUL, 0x77b25d63UL, 0x4ed7facbUL, 0x0fe6e1d2UL, 0xccb5ccf9UL,
		0x8d84d7e0UL, 0x4a1296afUL, 0x0b238db6UL, 0xc870a09dUL, 0x8941bb84UL,
		0x465d2303UL, 0x076c381aUL, 0xc43f1531UL, 0x850e0e28UL, 0x42984f67UL,
		0x03a9547eUL, 0xc0fa7955UL, 0x81cb624cUL, 0x1fc53881UL, 0x5ef42398UL,
		0x9da70eb3UL, 0xdc9615aaUL, 0x1b0054e5UL, 0x5a314ffcUL, 0x996262d7UL,
		0xd85379ceUL, 0x174fe149UL, 0x567efa50UL, 0x952dd77bUL, 0xd41ccc62UL,
		0x138a8d2dUL, 0x52bb9634UL, 0x91e8bb1fUL, 0xd0d9a006UL, 0xecf37e5eUL,
		0xadc26547UL, 0x6e91486cUL, 0x2fa05375UL, 0xe836123aUL, 0xa9070923UL,
		0x6a542408UL, 0x2b653f11UL, 0xe479a796UL, 0xa548bc8fUL, 0x661b91a4UL,
		0x272a8abdUL, 0xe0bccbf2UL, 0xa18dd0ebUL, 0x62defdc0UL, 0x23efe6d9UL,
		0xbde1bc14UL, 0xfcd0a70dUL, 0x3f838a26UL, 0x7eb2913fUL, 0xb924d070UL,
		0xf815cb69UL, 0x3b46e642UL, 0x7a77fd5bUL, 0xb56b65dcUL, 0xf45a7ec5UL,
		0x370953eeUL, 0x763848f7UL, 0xb1ae09b8UL, 0xf09f12a1UL, 0x33cc3f8aUL,
		0x72fd2493UL
	},
	{
		0x00000000UL, 0x376ac201UL, 0x6ed48403UL, 0x59be4602UL, 0xdca80907UL,
		0xebc2cb06UL, 0xb27c8d04UL, 0x85164f05UL, 0xb851130eUL, 0x8f3bd10fUL,
		0xd685970dUL, 0xe1ef550cUL, 0x64f91a09UL, 0x5393d808UL, 0x0a2d9e0aUL,
		0x3d475c0bUL, 0x70a3261cUL, 0x47c9e41dUL, 0x1e77a21fUL, 0x291d601eUL,
		0xac0b2f1bUL, 0x9b61ed1aUL, 0xc2dfab18UL, 0xf5b56919UL, 0xc8f23512UL,
		0xff98f713UL, 0xa626b111UL, 0x914c7310UL, 0x145a3c15UL, 0x2330fe14UL,
		0x7a8eb816UL, 0x4de47a17UL, 0xe0464d38UL, 0xd72c8f39UL, 0x8e92c93bUL,
		0xb9f80b3aUL, 0x3cee443fUL, 0x0b84863eUL, 0x523ac03cUL, 0x6550023dUL,
		0x58175e36UL, 0x6f7d9c37UL, 0x36c3da35UL, 0x01a91834UL, 0x84bf5731UL,
		0xb3d59530UL, 0xea6bd332UL, 0xdd011133UL, 0x90e56b24UL, 0xa78fa925UL,
		0xfe31ef27UL, 0xc95b2d26UL, 0x4c4d6223UL, 0x7b27a022UL, 0x2299e620UL,
		0x15f32421UL, 0x28b4782aUL, 0x1fdeba2bUL, 0x4660fc29UL, 0x710a3e28UL,
		0xf41c712dUL, 0xc376b32cUL, 0x9ac8f52eUL, 0xada2372fUL, 0xc08d9a70UL,
		0xf7e75871UL, 0xae591e73UL, 0x9933dc72UL, 0x1c259377UL, 0x2b4f5176UL,
		0x72f11774UL, 0x459bd575UL, 0x78dc897eUL, 0x4fb64b7fUL, 0x16080d7dUL,
		0x2162cf7cUL, 0xa4748079UL, 0x931e4278UL, 0xcaa0047aUL, 0xfdcac67bUL,
		0xb02ebc6cUL, 0x87447e6dUL, 0xdefa386fUL, 0xe990fa6eUL, 0x6c86b56bUL,
		0x5bec776aUL, 0x02523168UL, 0x3538f369UL, 0x087faf62UL, 0x3f156d63UL,
		0x66ab2b61UL, 0x51c1e960UL, 0xd4d7a665UL, 0xe3bd6464UL, 0xba032266UL,
		0x8d69e067UL, 0x20cbd748UL, 0x17a11549UL, 0x4e1f534bUL, 0x7975914aUL,
		0xfc63de4fUL, 0xcb091c4eUL, 0x92b75a4cUL, 0xa5dd984dUL, 0x989ac446UL,
		0xaff00647UL, 0xf64e4045UL, 0xc1248244UL, 0x4432cd41UL, 0x73580f40UL,
		0x2ae64942UL, 0x1d8c8b43UL, 0x5068f154UL, 0x67023355UL, 0x3ebc7557UL,
		0x09d6b756UL, 0x8cc0f853UL, 0xbbaa3a52UL, 0xe2147c50UL, 0xd57ebe51UL,
		0xe839e25aUL, 0xdf53205bUL, 0x86ed6659UL, 0xb187a458UL, 0x3491eb5dUL,
		0x03fb295cUL, 0x5a456f5eUL, 0x6d2fad5fUL, 0x801b35e1UL, 0xb771f7e0UL,
		0xeecfb1e2UL, 0xd9a573e3UL, 0x5cb33ce6UL, 0x6bd9fee7UL, 0x3267b8e5UL,
		0x050d7ae4UL, 0x384a26efUL, 0x0f20e4eeUL, 0x569ea2ecUL, 0x61f460edUL,
		0xe4e22fe8UL, 0xd388ede9UL, 0x8a36abebUL, 0xbd5c69eaUL, 0xf0b813fdUL,
		0xc7d2d1fcUL, 0x9e6c97feUL, 0xa90655ffUL, 0x2c101afaUL, 0x1b7ad8fbUL,
		0x42c49ef9UL, 0x75ae5cf8UL, 0x48e900f3UL, 0x7f83c2f2UL, 0x263d84f0UL,
		0x115746f1UL, 0x944109f4UL, 0xa32bcbf5UL, 0xfa958df7UL, 0xcdff4ff6UL,
		0x605d78d9UL, 0x5737bad8UL, 0x0e89fcdaUL, 0x39e33edbUL, 0xbcf571deUL,
		0x8b9fb3dfUL, 0xd221f5ddUL, 0xe54b37dcUL, 0xd80c6bd7UL, 0xef66a9d6UL,
		0xb6d8efd4UL, 0x81b22dd5UL, 0x04a462d0UL, 0x33cea0d1UL, 0x6a70e6d3UL,
		0x5d1a24d2UL, 0x10fe5ec5UL, 0x27949cc4UL, 0x7e2adac6UL, 0x494018c7UL,
		0xcc5657c2UL, 0xfb3c95c3UL, 0xa282d3c1UL, 0x95e811c0UL, 0xa8af4dcbUL,
		0x9fc58fcaUL, 0xc67bc9c8UL, 0xf1110bc9UL, 0x740744ccUL, 0x436d86cdUL,
		0x1ad3c0cfUL, 0x2db902ceUL, 0x4096af91UL, 0x77fc6d90UL, 0x2e422b92UL,
		0x1928e993UL, 0x9c3ea696UL, 0xab546497UL, 0xf2ea2295UL, 0xc580e094UL,
		0xf8c7bc9fUL, 0xcfad7e9eUL, 0x9613389cUL, 0xa179fa9dUL, 0x246fb598UL,
		0x13057799UL, 0x4abb319bUL, 0x7dd1f39aUL, 0x3035898dUL, 0x075f4b8cUL,
		0x5ee10d8eUL, 0x698bcf8fUL, 0xec9d808aUL, 0xdbf7428bUL, 0x82490489UL,
		0xb523c688UL, 0x88649a83UL, 0xbf0e5882UL, 0xe6b01e80UL, 0xd1dadc81UL,
		0x54cc9384UL, 0x63a65185UL, 0x3a181787UL, 0x0d72d586UL, 0xa0d0e2a9UL,
		0x97ba20a8UL, 0xce0466aaUL, 0xf96ea4abUL, 0x7c78ebaeUL, 0x4b1229afUL,
		0x12ac6fadUL, 0x25c6adacUL, 0x1881f1a7UL, 0x2feb33a6UL, 0x765575a4UL,
		0x413fb7a5UL, 0xc429f8a0UL, 0xf3433aa1UL, 0xaafd7ca3UL, 0x9d97bea2UL,
		0xd073c4b5UL, 0xe71906b4UL, 0xbea740b6UL, 0x89cd82b7UL, 0x0cdbcdb2UL,
		0x3bb10fb3UL, 0x620f49b1UL, 0x55658bb0UL, 0x6822d7bbUL, 0x5f4815baUL,
		0x06f653b8UL, 0x319c91b9UL, 0xb48adebcUL, 0x83e01cbdUL, 0xda5e5abfUL,
		0xed3498beUL
	},
	{
		0x00000000UL, 0x6567bcb8UL, 0x8bc809aaUL, 0xeeafb512UL, 0x5797628fUL,
		0x32f0de37UL, 0xdc5f6b25UL, 0xb938d79dUL, 0xef28b4c5UL, 0x8a4f087dUL,
		0x64e0bd6fUL, 0x018701d7UL, 0xb8bfd64aUL, 0xddd86af2UL, 0x3377dfe0UL,
		0x56106358UL, 0x9f571950UL, 0xfa30a5e8UL, 0x149f10faUL, 0x71f8ac42UL,
		0xc8c07bdfUL, 0xada7c767UL, 0x43087275UL, 0x266fcecdUL, 0x707fad95UL,
		0x1518112dUL, 0xfbb7a43fUL, 0x9ed01887UL, 0x27e8cf1aUL, 0x428f73a2UL,
		0xac20c6b0UL, 0xc9477a08UL, 0x3eaf32a0UL, 0x5bc88e18UL, 0xb5673b0aUL,
		0xd00087b2UL, 0x6938502fUL, 0x0c5fec97UL, 0xe2f05985UL, 0x8797e53dUL,
		0xd1878665UL, 0xb4e03addUL, 0x5a4f8fcfUL, 0x3f283377UL, 0x8610e4eaUL,
		0xe3775852UL, 0x0dd8ed40UL, 0x68bf51f8UL, 0xa1f82bf0UL, 0xc49f9748UL,
		0x2a30225aUL, 0x4f579ee2UL, 0xf66f497fUL, 0x9308f5c7UL, 0x7da740d5UL,
		0x18c0fc6dUL, 0x4ed09f35UL, 0x2bb7238dUL, 0xc518969fUL, 0xa07f2a27UL,
		0x1947fdbaUL, 0x7c204102UL, 0x928ff410UL, 0xf7e848a8UL, 0x3d58149bUL,
		0x583fa823UL, 0xb6901d31UL, 0xd3f7a189UL, 0x6acf7614UL, 0x0fa8caacUL,
		0xe1077fbeUL, 0x8460c306UL, 0xd270a05eUL, 0xb7171ce6UL, 0x59b8a9f4UL,
		0x3cdf154cUL, 0x85e7c2d1UL, 0xe0807e69UL, 0x0e2fcb7bUL, 0x6b4877c3UL,
		0xa20f0dcbUL, 0xc768b173UL, 0x29c70461UL, 0x4ca0b8d9UL, 0xf5986f44UL,
		0x90ffd3fcUL, 0x7e5066eeUL, 0x1b37da56UL, 0x4d27b90eUL, 0x284005b6UL,
		0xc6efb0a4UL, 0xa3880c1cUL, 0x1ab0db81UL, 0x7fd76739UL, 0x9178d22bUL,
		0xf41f6e93UL, 0x03f7263bUL, 0x66909a83UL, 0x883f2f91UL, 0xed589329UL,
		0x546044b4UL, 0x3107f80cUL, 0xdfa84d1eUL, 0xbacff1a6UL, 0xecdf92feUL,
		0x89b82e46UL, 0x67179b54UL, 0x027027ecUL, 0xbb48f071UL, 0xde2f4cc9UL,
		0x3080f9dbUL, 0x55e74563UL, 0x9ca03f6bUL, 0xf9c783d3UL, 0x176836c1UL,
		0x720f8a79UL, 0xcb375de4UL, 0xae50e15cUL, 0x40ff544eUL, 0x2598e8f6UL,
		0x73888baeUL, 0x16ef3716UL, 0xf8408204UL, 0x9d273ebcUL, 0x241fe921UL,
		0x41785599UL, 0xafd7e08bUL, 0xcab05c33UL, 0x3bb659edUL, 0x5ed1e555UL,
		0xb07e5047UL, 0xd519ecffUL, 0x6c213b62UL, 0x094687daUL, 0xe7e932c8UL,
		0x828e8e70UL, 0xd49eed28UL, 0xb1f95190UL, 0x5f56e482UL, 0x3a31583aUL,
		0x83098fa7UL, 0xe66e331fUL, 0x08c1860dUL, 0x6da63ab5UL, 0xa4e140bdUL,
		0xc186fc05UL, 0x2f294917UL, 0x4a4ef5afUL, 0xf3762232UL, 0x96119e8aUL,
		0x78be2b98UL, 0x1dd99720UL, 0x4bc9f478UL, 0x2eae48c0UL, 0xc001fdd2UL,
		0xa566416aUL, 0x1c5e96f7UL, 0x79392a4fUL, 0x97969f5dUL, 0xf2f123e5UL,
		0x05196b4dUL, 0x607ed7f5UL, 0x8ed162e7UL, 0xebb6de5fUL, 0x528e09c2UL,
		0x37e9b57aUL, 0xd9460068UL, 0xbc21bcd0UL, 0xea31df88UL, 0x8f566330UL,
		0x61f9d622UL, 0x049e6a9aUL, 0xbda6bd07UL, 0xd8c101bfUL, 0x366eb4adUL,
		0x53090815UL, 0x9a4e721dUL, 0xff29cea5UL, 0x11867bb7UL, 0x74e1c70fUL,
		0xcdd91092UL, 0xa8beac2aUL, 0x46111938UL, 0x2376a580UL, 0x7566c6d8UL,
		0x10017a60UL, 0xfeaecf72UL, 0x9bc973caUL, 0x22f1a457UL, 0x479618efUL,
		0xa939adfdUL, 0xcc5e1145UL, 0x06ee4d76UL, 0x6389f1ceUL, 0x8d2644dcUL,
		0xe841f864UL, 0x51792ff9UL, 0x341e9341UL, 0xdab12653UL, 0xbfd69aebUL,
		0xe9c6f9b3UL, 0x8ca1450bUL, 0x620ef019UL, 0x07694ca1UL, 0xbe519b3cUL,
		0xdb362784UL, 0x35999296UL, 0x50fe2e2eUL, 0x99b95426UL, 0xfcdee89eUL,
		0x12715d8cUL, 0x7716e134UL, 0xce2e36a9UL, 0xab498a11UL, 0x45e63f03UL,
		0x208183bbUL, 0x7691e0e3UL, 0x13f65c5bUL, 0xfd59e949UL, 0x983e55f1UL,
		0x2106826cUL, 0x44613ed4UL, 0xaace8bc6UL, 0xcfa9377eUL, 0x38417fd6UL,
		0x5d26c36eUL, 0xb389767cUL, 0xd6eecac4UL, 0x6fd61d59UL, 0x0ab1a1e1UL,
		0xe41e14f3UL, 0x8179a84bUL, 0xd769cb13UL, 0xb20e77abUL, 0x5ca1c2b9UL,
		0x39c67e01UL, 0x80fea99cUL, 0xe5991524UL, 0x0b36a036UL, 0x6e511c8eUL,
		0xa7166686UL, 0xc271da3eUL, 0x2cde6f2cUL, 0x49b9d394UL, 0xf0810409UL,
		0x95e6b8b1UL, 0x7b490da3UL, 0x1e2eb11bUL, 0x483ed243UL, 0x2d596efbUL,
		0xc3f6dbe9UL, 0xa6916751UL, 0x1fa9b0ccUL, 0x7ace0c74UL, 0x9461b966UL,
		0xf10605deUL
#endif
	}
};

#endif /* DYNAMIC_CRC_TABLE */

/* =========================================================================
 * This function can be used by asm versions of crc32()
 */
const unsigned long FAR* ZEXPORT get_crc_table()
{
#ifdef DYNAMIC_CRC_TABLE

	if ( crc_table_empty )
	{
		make_crc_table();
	}

#endif /* DYNAMIC_CRC_TABLE */
	return ( const unsigned long FAR* )crc_table;
}

/* ========================================================================= */
#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1

/* ========================================================================= */
unsigned long ZEXPORT crc32( crc, buf, len )
unsigned long crc;
const unsigned char FAR* buf;
uInt len;
{
	if ( buf == Z_NULL ) { return 0UL; }

#ifdef DYNAMIC_CRC_TABLE

	if ( crc_table_empty )
	{
		make_crc_table();
	}

#endif /* DYNAMIC_CRC_TABLE */

#ifdef BYFOUR

	if ( sizeof( void* ) == sizeof( ptrdiff_t ) )
	{
		u4 endian;

		endian = 1;

		if ( *( ( unsigned char* )( &endian ) ) )
		{
			return crc32_little( crc, buf, len );
		}
		else
		{
			return crc32_big( crc, buf, len );
		}
	}

#endif /* BYFOUR */
	crc = crc ^ 0xffffffffUL;

	while ( len >= 8 )
	{
		DO8;
		len -= 8;
	}

	if ( len ) do
		{
			DO1;
		}
		while ( --len );

	return crc ^ 0xffffffffUL;
}

#ifdef BYFOUR

/* ========================================================================= */
#define DOLIT4 c ^= *buf4++; \
        c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
            crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4

/* ========================================================================= */
local unsigned long crc32_little( crc, buf, len )
unsigned long crc;
const unsigned char FAR* buf;
unsigned len;
{
	register u4 c;
	register const u4 FAR* buf4;

	c = ( u4 )crc;
	c = ~c;

	while ( len && ( ( ptrdiff_t )buf & 3 ) )
	{
		c = crc_table[0][( c ^ *buf++ ) & 0xff] ^ ( c >> 8 );
		len--;
	}

	buf4 = ( const u4 FAR* )( const void FAR* )buf;

	while ( len >= 32 )
	{
		DOLIT32;
		len -= 32;
	}

	while ( len >= 4 )
	{
		DOLIT4;
		len -= 4;
	}

	buf = ( const unsigned char FAR* )buf4;

	if ( len ) do
		{
			c = crc_table[0][( c ^ *buf++ ) & 0xff] ^ ( c >> 8 );
		}
		while ( --len );

	c = ~c;
	return ( unsigned long )c;
}

/* ========================================================================= */
#define DOBIG4 c ^= *++buf4; \
        c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ \
            crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
#define DOBIG32 DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4

/* ========================================================================= */
local unsigned long crc32_big( crc, buf, len )
unsigned long crc;
const unsigned char FAR* buf;
unsigned len;
{
	register u4 c;
	register const u4 FAR* buf4;

	c = REV( ( u4 )crc );
	c = ~c;

	while ( len && ( ( ptrdiff_t )buf & 3 ) )
	{
		c = crc_table[4][( c >> 24 ) ^ *buf++] ^ ( c << 8 );
		len--;
	}

	buf4 = ( const u4 FAR* )( const void FAR* )buf;
	buf4--;

	while ( len >= 32 )
	{
		DOBIG32;
		len -= 32;
	}

	while ( len >= 4 )
	{
		DOBIG4;
		len -= 4;
	}

	buf4++;
	buf = ( const unsigned char FAR* )buf4;

	if ( len ) do
		{
			c = crc_table[4][( c >> 24 ) ^ *buf++] ^ ( c << 8 );
		}
		while ( --len );

	c = ~c;
	return ( unsigned long )( REV( c ) );
}

#endif /* BYFOUR */

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */

/* ========================================================================= */
local unsigned long gf2_matrix_times( mat, vec )
unsigned long* mat;
unsigned long vec;
{
	unsigned long sum;

	sum = 0;

	while ( vec )
	{
		if ( vec & 1 )
		{
			sum ^= *mat;
		}

		vec >>= 1;
		mat++;
	}

	return sum;
}

/* ========================================================================= */
local void gf2_matrix_square( square, mat )
unsigned long* square;
unsigned long* mat;
{
	int n;

	for ( n = 0; n < GF2_DIM; n++ )
	{
		square[n] = gf2_matrix_times( mat, mat[n] );
	}
}

/* ========================================================================= */
local uLong crc32_combine_( crc1, crc2, len2 )
uLong crc1;
uLong crc2;
z_off64_t len2;
{
	int n;
	unsigned long row;
	unsigned long even[GF2_DIM];    /* even-power-of-two zeros operator */
	unsigned long odd[GF2_DIM];     /* odd-power-of-two zeros operator */

	/* degenerate case (also disallow negative lengths) */
	if ( len2 <= 0 )
	{
		return crc1;
	}

	/* put operator for one zero bit in odd */
	odd[0] = 0xedb88320UL;          /* CRC-32 polynomial */
	row = 1;

	for ( n = 1; n < GF2_DIM; n++ )
	{
		odd[n] = row;
		row <<= 1;
	}

	/* put operator for two zero bits in even */
	gf2_matrix_square( even, odd );

	/* put operator for four zero bits in odd */
	gf2_matrix_square( odd, even );

	/* apply len2 zeros to crc1 (first square will put the operator for one
	   zero byte, eight zero bits, in even) */
	do
	{
		/* apply zeros operator for this bit of len2 */
		gf2_matrix_square( even, odd );

		if ( len2 & 1 )
		{
			crc1 = gf2_matrix_times( even, crc1 );
		}

		len2 >>= 1;

		/* if no more bits set, then done */
		if ( len2 == 0 )
		{
			break;
		}

		/* another iteration of the loop with odd and even swapped */
		gf2_matrix_square( odd, even );

		if ( len2 & 1 )
		{
			crc1 = gf2_matrix_times( odd, crc1 );
		}

		len2 >>= 1;

		/* if no more bits set, then done */
	}
	while ( len2 != 0 );

	/* return combined crc */
	crc1 ^= crc2;
	return crc1;
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine( crc1, crc2, len2 )
uLong crc1;
uLong crc2;
z_off_t len2;
{
	return crc32_combine_( crc1, crc2, len2 );
}

uLong ZEXPORT crc32_combine64( crc1, crc2, len2 )
uLong crc1;
uLong crc2;
z_off64_t len2;
{
	return crc32_combine_( crc1, crc2, len2 );
}

//// inflate_all.c

/* infback.c -- inflate using a call-back interface
 * Copyright (C) 1995-2009 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
   This code is largely copied from inflate.c.  Normally either infback.o or
   inflate.o would be linked into an application--not both.  The interface
   with inffast.c is retained so that optimized assembler-coded versions of
   inflate_fast() can be used with either inflate.c or infback.c.
 */

//// inftrees.h

/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* Structure for decoding tables.  Each entry provides either the
   information needed to do the operation requested by the code that
   indexed that table entry, or it provides a pointer to another
   table that indexes more bits of the code.  op indicates whether
   the entry is a pointer to another table, a literal, a length or
   distance, an end-of-block, or an invalid code.  For a table
   pointer, the low four bits of op is the number of index bits of
   that table.  For a length or distance, the low four bits of op
   is the number of extra bits to get after the code.  bits is
   the number of bits in this code or part of the code to drop off
   of the bit buffer.  val is the actual byte to output in the case
   of a literal, the base length or distance, or the offset from
   the current table to the next table.  Each entry is four bytes. */
typedef struct
{
	unsigned char op;           /* operation, extra bits, table bits */
	unsigned char bits;         /* bits in this part of the code */
	unsigned short val;         /* offset in table or code value */
} code;

/* op values as set by inflate_table():
    00000000 - literal
    0000tttt - table link, tttt != 0 is the number of table index bits
    0001eeee - length or distance, eeee is the number of extra bits
    01100000 - end of block
    01000000 - invalid code
 */

/* Maximum size of the dynamic table.  The maximum number of code structures is
   1444, which is the sum of 852 for literal/length codes and 592 for distance
   codes.  These values were found by exhaustive searches using the program
   examples/enough.c found in the zlib distribtution.  The arguments to that
   program are the number of symbols, the initial root table size, and the
   maximum bit length of a code.  "enough 286 9 15" for literal/length codes
   returns returns 852, and "enough 30 6 15" for distance codes returns 592.
   The initial root table size (9 or 6) is found in the fifth argument of the
   inflate_table() calls in inflate.c and infback.c.  If the root table size is
   changed, then these maximum sizes would be need to be recalculated and
   updated. */
#define ENOUGH_LENS 852
#define ENOUGH_DISTS 592
#define ENOUGH (ENOUGH_LENS+ENOUGH_DISTS)

/* Type of code to build for inflate_table() */
typedef enum
{
   CODES,
   LENS,
   DISTS
} codetype;

int ZLIB_INTERNAL inflate_table OF( ( codetype type, unsigned short FAR* lens,
                                      unsigned codes, code FAR* FAR* table,
                                      unsigned FAR* bits, unsigned short FAR* work ) );

///////

/// inflate.h


/* inflate.h -- internal inflate state definition
 * Copyright (C) 1995-2009 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer decoding by inflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip decoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GUNZIP
#endif

/* Possible inflate modes between inflate() calls */
typedef enum
{
   HEAD,       /* i: waiting for magic header */
   FLAGS,      /* i: waiting for method and flags (gzip) */
   TIME,       /* i: waiting for modification time (gzip) */
   OS,         /* i: waiting for extra flags and operating system (gzip) */
   EXLEN,      /* i: waiting for extra length (gzip) */
   EXTRA,      /* i: waiting for extra bytes (gzip) */
   NAME,       /* i: waiting for end of file name (gzip) */
   COMMENT,    /* i: waiting for end of comment (gzip) */
   HCRC,       /* i: waiting for header crc (gzip) */
   DICTID,     /* i: waiting for dictionary check value */
   DICT,       /* waiting for inflateSetDictionary() call */
   TYPE,       /* i: waiting for type bits, including last-flag bit */
   TYPEDO,     /* i: same, but skip check to exit inflate on new block */
   STORED,     /* i: waiting for stored size (length and complement) */
   COPY_,      /* i/o: same as COPY below, but only first time in */
   COPY,       /* i/o: waiting for input or output to copy stored block */
   TABLE,      /* i: waiting for dynamic block table lengths */
   LENLENS,    /* i: waiting for code length code lengths */
   CODELENS,   /* i: waiting for length/lit and distance code lengths */
   LEN_,       /* i: same as LEN below, but only first time in */
   LEN,        /* i: waiting for length/lit/eob code */
   LENEXT,     /* i: waiting for length extra bits */
   DIST,       /* i: waiting for distance code */
   DISTEXT,    /* i: waiting for distance extra bits */
   MATCH,      /* o: waiting for output space to copy string */
   LIT,        /* o: waiting for output space to write literal */
   CHECK,      /* i: waiting for 32-bit check value */
   LENGTH,     /* i: waiting for 32-bit length (gzip) */
   DONE,       /* finished check, done -- remain here until reset */
   BAD,        /* got a data error -- remain here until reset */
   MEM,        /* got an inflate() memory error -- remain here until reset */
   SYNC        /* looking for synchronization bytes to restart inflate() */
} inflate_mode;

/*
    State transitions between above modes -

    (most modes can go to BAD or MEM on error -- not shown for clarity)

    Process header:
        HEAD -> (gzip) or (zlib) or (raw)
        (gzip) -> FLAGS -> TIME -> OS -> EXLEN -> EXTRA -> NAME -> COMMENT ->
                  HCRC -> TYPE
        (zlib) -> DICTID or TYPE
        DICTID -> DICT -> TYPE
        (raw) -> TYPEDO
    Read deflate blocks:
            TYPE -> TYPEDO -> STORED or TABLE or LEN_ or CHECK
            STORED -> COPY_ -> COPY -> TYPE
            TABLE -> LENLENS -> CODELENS -> LEN_
            LEN_ -> LEN
    Read deflate codes in fixed or dynamic block:
                LEN -> LENEXT or LIT or TYPE
                LENEXT -> DIST -> DISTEXT -> MATCH -> LEN
                LIT -> LEN
    Process trailer:
        CHECK -> LENGTH -> DONE
 */

/* state maintained between inflate() calls.  Approximately 10K bytes. */
struct inflate_state
{
	inflate_mode mode;          /* current inflate mode */
	int last;                   /* true if processing last block */
	int wrap;                   /* bit 0 true for zlib, bit 1 true for gzip */
	int havedict;               /* true if dictionary provided */
	int flags;                  /* gzip header method and flags (0 if zlib) */
	unsigned dmax;              /* zlib header max distance (INFLATE_STRICT) */
	unsigned long check;        /* protected copy of check value */
	unsigned long total;        /* protected copy of output count */
	gz_headerp head;            /* where to save gzip header information */
	/* sliding window */
	unsigned wbits;             /* log base 2 of requested window size */
	unsigned wsize;             /* window size or zero if not using window */
	unsigned whave;             /* valid bytes in the window */
	unsigned wnext;             /* window write index */
	unsigned char FAR* window;  /* allocated sliding window, if needed */
	/* bit accumulator */
	unsigned long hold;         /* input bit accumulator */
	unsigned bits;              /* number of bits in "in" */
	/* for string and stored block copying */
	unsigned length;            /* literal or length of data to copy */
	unsigned offset;            /* distance back to copy string from */
	/* for table and code decoding */
	unsigned extra;             /* extra bits needed */
	/* fixed and dynamic code tables */
	code const FAR* lencode;    /* starting table for length/literal codes */
	code const FAR* distcode;   /* starting table for distance codes */
	unsigned lenbits;           /* index bits for lencode */
	unsigned distbits;          /* index bits for distcode */
	/* dynamic table building */
	unsigned ncode;             /* number of code length code lengths */
	unsigned nlen;              /* number of length code lengths */
	unsigned ndist;             /* number of distance code lengths */
	unsigned have;              /* number of code lengths in lens[] */
	code FAR* next;             /* next available space in codes[] */
	unsigned short lens[320];   /* temporary storage for code lengths */
	unsigned short work[288];   /* work area for code table building */
	code codes[ENOUGH];         /* space for code tables */
	int sane;                   /* if false, allow invalid distance too far */
	int back;                   /* bits back of last unprocessed length/lit */
	unsigned was;               /* initial length of match */
};

///////

//#include "inffast.h"
// LV: from former inffast.h
void ZLIB_INTERNAL inflate_fast OF( ( z_streamp strm, unsigned start ) );

/* function prototypes */
local void fixedtables OF( ( struct inflate_state FAR* state ) );

/*
   strm provides memory allocation functions in zalloc and zfree, or
   Z_NULL to use the library memory allocation functions.

   windowBits is in the range 8..15, and window is a user-supplied
   window and output buffer that is 2**windowBits bytes.
 */
int ZEXPORT inflateBackInit_( strm, windowBits, window, version, stream_size )
z_streamp strm;
int windowBits;
unsigned char FAR* window;
const char* version;
int stream_size;
{
	struct inflate_state FAR* state;

	if ( version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
	     stream_size != ( int )( sizeof( z_stream ) ) )
	{
		return Z_VERSION_ERROR;
	}

	if ( strm == Z_NULL || window == Z_NULL ||
	     windowBits < 8 || windowBits > 15 )
	{
		return Z_STREAM_ERROR;
	}

	strm->msg = Z_NULL;                 /* in case we return an error */

	if ( strm->zalloc == ( alloc_func )0 )
	{
		strm->zalloc = zcalloc;
		strm->opaque = ( voidpf )0;
	}

	if ( strm->zfree == ( free_func )0 ) { strm->zfree = zcfree; }

	state = ( struct inflate_state FAR* )ZALLOC( strm, 1,
	                                             sizeof( struct inflate_state ) );

	if ( state == Z_NULL ) { return Z_MEM_ERROR; }

	Tracev( ( stderr, "inflate: allocated\n" ) );
	strm->state = ( struct internal_state FAR* )state;
	state->dmax = 32768U;
	state->wbits = windowBits;
	state->wsize = 1U << windowBits;
	state->window = window;
	state->wnext = 0;
	state->whave = 0;
	return Z_OK;
}

/*
   Return state with length and distance decoding tables and index sizes set to
   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
   If BUILDFIXED is defined, then instead this routine builds the tables the
   first time it's called, and returns those tables the first time and
   thereafter.  This reduces the size of the code by about 2K bytes, in
   exchange for a little execution time.  However, BUILDFIXED should not be
   used for threaded applications, since the rewriting of the tables and virgin
   may not be thread-safe.
 */
local void fixedtables( state )
struct inflate_state FAR* state;
{
#ifdef BUILDFIXED
	static int virgin = 1;
	static code* lenfix, *distfix;
	static code fixed[544];

	/* build fixed huffman tables if first call (may not be thread safe) */
	if ( virgin )
	{
		unsigned sym, bits;
		static code* next;

		/* literal/length table */
		sym = 0;

		while ( sym < 144 ) { state->lens[sym++] = 8; }

		while ( sym < 256 ) { state->lens[sym++] = 9; }

		while ( sym < 280 ) { state->lens[sym++] = 7; }

		while ( sym < 288 ) { state->lens[sym++] = 8; }

		next = fixed;
		lenfix = next;
		bits = 9;
		inflate_table( LENS, state->lens, 288, &( next ), &( bits ), state->work );

		/* distance table */
		sym = 0;

		while ( sym < 32 ) { state->lens[sym++] = 5; }

		distfix = next;
		bits = 5;
		inflate_table( DISTS, state->lens, 32, &( next ), &( bits ), state->work );

		/* do this just once */
		virgin = 0;
	}

#else /* !BUILDFIXED */






	/* inffixed.h -- table for decoding fixed codes
	 * Generated automatically by makefixed().
	 */

	/* WARNING: this file should *not* be used by applications. It
	   is part of the implementation of the compression library and
	   is subject to change. Applications should only use zlib.h.
	 */

	static const code lenfix[512] =
	{
		{96, 7, 0}, {0, 8, 80}, {0, 8, 16}, {20, 8, 115}, {18, 7, 31}, {0, 8, 112}, {0, 8, 48},
		{0, 9, 192}, {16, 7, 10}, {0, 8, 96}, {0, 8, 32}, {0, 9, 160}, {0, 8, 0}, {0, 8, 128},
		{0, 8, 64}, {0, 9, 224}, {16, 7, 6}, {0, 8, 88}, {0, 8, 24}, {0, 9, 144}, {19, 7, 59},
		{0, 8, 120}, {0, 8, 56}, {0, 9, 208}, {17, 7, 17}, {0, 8, 104}, {0, 8, 40}, {0, 9, 176},
		{0, 8, 8}, {0, 8, 136}, {0, 8, 72}, {0, 9, 240}, {16, 7, 4}, {0, 8, 84}, {0, 8, 20},
		{21, 8, 227}, {19, 7, 43}, {0, 8, 116}, {0, 8, 52}, {0, 9, 200}, {17, 7, 13}, {0, 8, 100},
		{0, 8, 36}, {0, 9, 168}, {0, 8, 4}, {0, 8, 132}, {0, 8, 68}, {0, 9, 232}, {16, 7, 8},
		{0, 8, 92}, {0, 8, 28}, {0, 9, 152}, {20, 7, 83}, {0, 8, 124}, {0, 8, 60}, {0, 9, 216},
		{18, 7, 23}, {0, 8, 108}, {0, 8, 44}, {0, 9, 184}, {0, 8, 12}, {0, 8, 140}, {0, 8, 76},
		{0, 9, 248}, {16, 7, 3}, {0, 8, 82}, {0, 8, 18}, {21, 8, 163}, {19, 7, 35}, {0, 8, 114},
		{0, 8, 50}, {0, 9, 196}, {17, 7, 11}, {0, 8, 98}, {0, 8, 34}, {0, 9, 164}, {0, 8, 2},
		{0, 8, 130}, {0, 8, 66}, {0, 9, 228}, {16, 7, 7}, {0, 8, 90}, {0, 8, 26}, {0, 9, 148},
		{20, 7, 67}, {0, 8, 122}, {0, 8, 58}, {0, 9, 212}, {18, 7, 19}, {0, 8, 106}, {0, 8, 42},
		{0, 9, 180}, {0, 8, 10}, {0, 8, 138}, {0, 8, 74}, {0, 9, 244}, {16, 7, 5}, {0, 8, 86},
		{0, 8, 22}, {64, 8, 0}, {19, 7, 51}, {0, 8, 118}, {0, 8, 54}, {0, 9, 204}, {17, 7, 15},
		{0, 8, 102}, {0, 8, 38}, {0, 9, 172}, {0, 8, 6}, {0, 8, 134}, {0, 8, 70}, {0, 9, 236},
		{16, 7, 9}, {0, 8, 94}, {0, 8, 30}, {0, 9, 156}, {20, 7, 99}, {0, 8, 126}, {0, 8, 62},
		{0, 9, 220}, {18, 7, 27}, {0, 8, 110}, {0, 8, 46}, {0, 9, 188}, {0, 8, 14}, {0, 8, 142},
		{0, 8, 78}, {0, 9, 252}, {96, 7, 0}, {0, 8, 81}, {0, 8, 17}, {21, 8, 131}, {18, 7, 31},
		{0, 8, 113}, {0, 8, 49}, {0, 9, 194}, {16, 7, 10}, {0, 8, 97}, {0, 8, 33}, {0, 9, 162},
		{0, 8, 1}, {0, 8, 129}, {0, 8, 65}, {0, 9, 226}, {16, 7, 6}, {0, 8, 89}, {0, 8, 25},
		{0, 9, 146}, {19, 7, 59}, {0, 8, 121}, {0, 8, 57}, {0, 9, 210}, {17, 7, 17}, {0, 8, 105},
		{0, 8, 41}, {0, 9, 178}, {0, 8, 9}, {0, 8, 137}, {0, 8, 73}, {0, 9, 242}, {16, 7, 4},
		{0, 8, 85}, {0, 8, 21}, {16, 8, 258}, {19, 7, 43}, {0, 8, 117}, {0, 8, 53}, {0, 9, 202},
		{17, 7, 13}, {0, 8, 101}, {0, 8, 37}, {0, 9, 170}, {0, 8, 5}, {0, 8, 133}, {0, 8, 69},
		{0, 9, 234}, {16, 7, 8}, {0, 8, 93}, {0, 8, 29}, {0, 9, 154}, {20, 7, 83}, {0, 8, 125},
		{0, 8, 61}, {0, 9, 218}, {18, 7, 23}, {0, 8, 109}, {0, 8, 45}, {0, 9, 186}, {0, 8, 13},
		{0, 8, 141}, {0, 8, 77}, {0, 9, 250}, {16, 7, 3}, {0, 8, 83}, {0, 8, 19}, {21, 8, 195},
		{19, 7, 35}, {0, 8, 115}, {0, 8, 51}, {0, 9, 198}, {17, 7, 11}, {0, 8, 99}, {0, 8, 35},
		{0, 9, 166}, {0, 8, 3}, {0, 8, 131}, {0, 8, 67}, {0, 9, 230}, {16, 7, 7}, {0, 8, 91},
		{0, 8, 27}, {0, 9, 150}, {20, 7, 67}, {0, 8, 123}, {0, 8, 59}, {0, 9, 214}, {18, 7, 19},
		{0, 8, 107}, {0, 8, 43}, {0, 9, 182}, {0, 8, 11}, {0, 8, 139}, {0, 8, 75}, {0, 9, 246},
		{16, 7, 5}, {0, 8, 87}, {0, 8, 23}, {64, 8, 0}, {19, 7, 51}, {0, 8, 119}, {0, 8, 55},
		{0, 9, 206}, {17, 7, 15}, {0, 8, 103}, {0, 8, 39}, {0, 9, 174}, {0, 8, 7}, {0, 8, 135},
		{0, 8, 71}, {0, 9, 238}, {16, 7, 9}, {0, 8, 95}, {0, 8, 31}, {0, 9, 158}, {20, 7, 99},
		{0, 8, 127}, {0, 8, 63}, {0, 9, 222}, {18, 7, 27}, {0, 8, 111}, {0, 8, 47}, {0, 9, 190},
		{0, 8, 15}, {0, 8, 143}, {0, 8, 79}, {0, 9, 254}, {96, 7, 0}, {0, 8, 80}, {0, 8, 16},
		{20, 8, 115}, {18, 7, 31}, {0, 8, 112}, {0, 8, 48}, {0, 9, 193}, {16, 7, 10}, {0, 8, 96},
		{0, 8, 32}, {0, 9, 161}, {0, 8, 0}, {0, 8, 128}, {0, 8, 64}, {0, 9, 225}, {16, 7, 6},
		{0, 8, 88}, {0, 8, 24}, {0, 9, 145}, {19, 7, 59}, {0, 8, 120}, {0, 8, 56}, {0, 9, 209},
		{17, 7, 17}, {0, 8, 104}, {0, 8, 40}, {0, 9, 177}, {0, 8, 8}, {0, 8, 136}, {0, 8, 72},
		{0, 9, 241}, {16, 7, 4}, {0, 8, 84}, {0, 8, 20}, {21, 8, 227}, {19, 7, 43}, {0, 8, 116},
		{0, 8, 52}, {0, 9, 201}, {17, 7, 13}, {0, 8, 100}, {0, 8, 36}, {0, 9, 169}, {0, 8, 4},
		{0, 8, 132}, {0, 8, 68}, {0, 9, 233}, {16, 7, 8}, {0, 8, 92}, {0, 8, 28}, {0, 9, 153},
		{20, 7, 83}, {0, 8, 124}, {0, 8, 60}, {0, 9, 217}, {18, 7, 23}, {0, 8, 108}, {0, 8, 44},
		{0, 9, 185}, {0, 8, 12}, {0, 8, 140}, {0, 8, 76}, {0, 9, 249}, {16, 7, 3}, {0, 8, 82},
		{0, 8, 18}, {21, 8, 163}, {19, 7, 35}, {0, 8, 114}, {0, 8, 50}, {0, 9, 197}, {17, 7, 11},
		{0, 8, 98}, {0, 8, 34}, {0, 9, 165}, {0, 8, 2}, {0, 8, 130}, {0, 8, 66}, {0, 9, 229},
		{16, 7, 7}, {0, 8, 90}, {0, 8, 26}, {0, 9, 149}, {20, 7, 67}, {0, 8, 122}, {0, 8, 58},
		{0, 9, 213}, {18, 7, 19}, {0, 8, 106}, {0, 8, 42}, {0, 9, 181}, {0, 8, 10}, {0, 8, 138},
		{0, 8, 74}, {0, 9, 245}, {16, 7, 5}, {0, 8, 86}, {0, 8, 22}, {64, 8, 0}, {19, 7, 51},
		{0, 8, 118}, {0, 8, 54}, {0, 9, 205}, {17, 7, 15}, {0, 8, 102}, {0, 8, 38}, {0, 9, 173},
		{0, 8, 6}, {0, 8, 134}, {0, 8, 70}, {0, 9, 237}, {16, 7, 9}, {0, 8, 94}, {0, 8, 30},
		{0, 9, 157}, {20, 7, 99}, {0, 8, 126}, {0, 8, 62}, {0, 9, 221}, {18, 7, 27}, {0, 8, 110},
		{0, 8, 46}, {0, 9, 189}, {0, 8, 14}, {0, 8, 142}, {0, 8, 78}, {0, 9, 253}, {96, 7, 0},
		{0, 8, 81}, {0, 8, 17}, {21, 8, 131}, {18, 7, 31}, {0, 8, 113}, {0, 8, 49}, {0, 9, 195},
		{16, 7, 10}, {0, 8, 97}, {0, 8, 33}, {0, 9, 163}, {0, 8, 1}, {0, 8, 129}, {0, 8, 65},
		{0, 9, 227}, {16, 7, 6}, {0, 8, 89}, {0, 8, 25}, {0, 9, 147}, {19, 7, 59}, {0, 8, 121},
		{0, 8, 57}, {0, 9, 211}, {17, 7, 17}, {0, 8, 105}, {0, 8, 41}, {0, 9, 179}, {0, 8, 9},
		{0, 8, 137}, {0, 8, 73}, {0, 9, 243}, {16, 7, 4}, {0, 8, 85}, {0, 8, 21}, {16, 8, 258},
		{19, 7, 43}, {0, 8, 117}, {0, 8, 53}, {0, 9, 203}, {17, 7, 13}, {0, 8, 101}, {0, 8, 37},
		{0, 9, 171}, {0, 8, 5}, {0, 8, 133}, {0, 8, 69}, {0, 9, 235}, {16, 7, 8}, {0, 8, 93},
		{0, 8, 29}, {0, 9, 155}, {20, 7, 83}, {0, 8, 125}, {0, 8, 61}, {0, 9, 219}, {18, 7, 23},
		{0, 8, 109}, {0, 8, 45}, {0, 9, 187}, {0, 8, 13}, {0, 8, 141}, {0, 8, 77}, {0, 9, 251},
		{16, 7, 3}, {0, 8, 83}, {0, 8, 19}, {21, 8, 195}, {19, 7, 35}, {0, 8, 115}, {0, 8, 51},
		{0, 9, 199}, {17, 7, 11}, {0, 8, 99}, {0, 8, 35}, {0, 9, 167}, {0, 8, 3}, {0, 8, 131},
		{0, 8, 67}, {0, 9, 231}, {16, 7, 7}, {0, 8, 91}, {0, 8, 27}, {0, 9, 151}, {20, 7, 67},
		{0, 8, 123}, {0, 8, 59}, {0, 9, 215}, {18, 7, 19}, {0, 8, 107}, {0, 8, 43}, {0, 9, 183},
		{0, 8, 11}, {0, 8, 139}, {0, 8, 75}, {0, 9, 247}, {16, 7, 5}, {0, 8, 87}, {0, 8, 23},
		{64, 8, 0}, {19, 7, 51}, {0, 8, 119}, {0, 8, 55}, {0, 9, 207}, {17, 7, 15}, {0, 8, 103},
		{0, 8, 39}, {0, 9, 175}, {0, 8, 7}, {0, 8, 135}, {0, 8, 71}, {0, 9, 239}, {16, 7, 9},
		{0, 8, 95}, {0, 8, 31}, {0, 9, 159}, {20, 7, 99}, {0, 8, 127}, {0, 8, 63}, {0, 9, 223},
		{18, 7, 27}, {0, 8, 111}, {0, 8, 47}, {0, 9, 191}, {0, 8, 15}, {0, 8, 143}, {0, 8, 79},
		{0, 9, 255}
	};

	static const code distfix[32] =
	{
		{16, 5, 1}, {23, 5, 257}, {19, 5, 17}, {27, 5, 4097}, {17, 5, 5}, {25, 5, 1025},
		{21, 5, 65}, {29, 5, 16385}, {16, 5, 3}, {24, 5, 513}, {20, 5, 33}, {28, 5, 8193},
		{18, 5, 9}, {26, 5, 2049}, {22, 5, 129}, {64, 5, 0}, {16, 5, 2}, {23, 5, 385},
		{19, 5, 25}, {27, 5, 6145}, {17, 5, 7}, {25, 5, 1537}, {21, 5, 97}, {29, 5, 24577},
		{16, 5, 4}, {24, 5, 769}, {20, 5, 49}, {28, 5, 12289}, {18, 5, 13}, {26, 5, 3073},
		{22, 5, 193}, {64, 5, 0}
	};


#endif /* BUILDFIXED */
	state->lencode = lenfix;
	state->lenbits = 9;
	state->distcode = distfix;
	state->distbits = 5;
}

/* Macros for inflateBack(): */

/* Load returned state from inflate_fast() */
#define LOAD() \
    do { \
        put = strm->next_out; \
        left = strm->avail_out; \
        next = strm->next_in; \
        have = strm->avail_in; \
        hold = state->hold; \
        bits = state->bits; \
    } while (0)

/* Set state from registers for inflate_fast() */
#define RESTORE() \
    do { \
        strm->next_out = put; \
        strm->avail_out = left; \
        strm->next_in = next; \
        strm->avail_in = have; \
        state->hold = hold; \
        state->bits = bits; \
    } while (0)

/* Clear the input bit accumulator */
#define INITBITS() \
    do { \
        hold = 0; \
        bits = 0; \
    } while (0)

/* Assure that some input is available.  If input is requested, but denied,
   then return a Z_BUF_ERROR from inflateBack(). */
#define PULL() \
    do { \
        if (have == 0) { \
            have = in(in_desc, &next); \
            if (have == 0) { \
                next = Z_NULL; \
                ret = Z_BUF_ERROR; \
                goto inf_leave; \
            } \
        } \
    } while (0)

/* Get a byte of input into the bit accumulator, or return from inflateBack()
   with an error if there is no input available. */
#define PULLBYTE() \
    do { \
        PULL(); \
        have--; \
        hold += (unsigned long)(*next++) << bits; \
        bits += 8; \
    } while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflateBack() with
   an error. */
#define NEEDBITS(n) \
    do { \
        while (bits < (unsigned)(n)) \
            PULLBYTE(); \
    } while (0)

/* Return the low n bits of the bit accumulator (n < 16) */
#define BITS(n) \
    ((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n) \
    do { \
        hold >>= (n); \
        bits -= (unsigned)(n); \
    } while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS() \
    do { \
        hold >>= bits & 7; \
        bits -= bits & 7; \
    } while (0)

/* Assure that some output space is available, by writing out the window
   if it's full.  If the write fails, return from inflateBack() with a
   Z_BUF_ERROR. */
#define ROOM() \
    do { \
        if (left == 0) { \
            put = state->window; \
            left = state->wsize; \
            state->whave = left; \
            if (out(out_desc, put, left)) { \
                ret = Z_BUF_ERROR; \
                goto inf_leave; \
            } \
        } \
    } while (0)

/*
   strm provides the memory allocation functions and window buffer on input,
   and provides information on the unused input on return.  For Z_DATA_ERROR
   returns, strm will also provide an error message.

   in() and out() are the call-back input and output functions.  When
   inflateBack() needs more input, it calls in().  When inflateBack() has
   filled the window with output, or when it completes with data in the
   window, it calls out() to write out the data.  The application must not
   change the provided input until in() is called again or inflateBack()
   returns.  The application must not change the window/output buffer until
   inflateBack() returns.

   in() and out() are called with a descriptor parameter provided in the
   inflateBack() call.  This parameter can be a structure that provides the
   information required to do the read or write, as well as accumulated
   information on the input and output such as totals and check values.

   in() should return zero on failure.  out() should return non-zero on
   failure.  If either in() or out() fails, than inflateBack() returns a
   Z_BUF_ERROR.  strm->next_in can be checked for Z_NULL to see whether it
   was in() or out() that caused in the error.  Otherwise,  inflateBack()
   returns Z_STREAM_END on success, Z_DATA_ERROR for an deflate format
   error, or Z_MEM_ERROR if it could not allocate memory for the state.
   inflateBack() can also return Z_STREAM_ERROR if the input parameters
   are not correct, i.e. strm is Z_NULL or the state was not initialized.
 */
int ZEXPORT inflateBack( strm, in, in_desc, out, out_desc )
z_streamp strm;
in_func in;
void FAR* in_desc;
out_func out;
void FAR* out_desc;
{
	struct inflate_state FAR* state;
	unsigned char FAR* next;    /* next input */
	unsigned char FAR* put;     /* next output */
	unsigned have, left;        /* available input and output */
	unsigned long hold;         /* bit buffer */
	unsigned bits;              /* bits in bit buffer */
	unsigned copy;              /* number of stored or match bytes to copy */
	unsigned char FAR* from;    /* where to copy match bytes from */
	code here;                  /* current decoding table entry */
	code last;                  /* parent table entry */
	unsigned len;               /* length to copy for repeats, bits to drop */
	int ret;                    /* return code */
	static const unsigned short order[19] = /* permutation of code lengths */
	{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

	/* Check that the strm exists and that the state was initialized */
	if ( strm == Z_NULL || strm->state == Z_NULL )
	{
		return Z_STREAM_ERROR;
	}

	state = ( struct inflate_state FAR* )strm->state;

	/* Reset the state */
	strm->msg = Z_NULL;
	state->mode = TYPE;
	state->last = 0;
	state->whave = 0;
	next = strm->next_in;
	have = next != Z_NULL ? strm->avail_in : 0;
	hold = 0;
	bits = 0;
	put = state->window;
	left = state->wsize;

	/* Inflate until end of block marked as last */
	for ( ;; )
		switch ( state->mode )
		{
			case TYPE:

				/* determine and dispatch block type */
				if ( state->last )
				{
					BYTEBITS();
					state->mode = DONE;
					break;
				}

				NEEDBITS( 3 );
				state->last = BITS( 1 );
				DROPBITS( 1 );

				switch ( BITS( 2 ) )
				{
					case 0:                             /* stored block */
						Tracev( ( stderr, "inflate:     stored block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = STORED;
						break;

					case 1:                             /* fixed block */
						fixedtables( state );
						Tracev( ( stderr, "inflate:     fixed codes block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = LEN;              /* decode codes */
						break;

					case 2:                             /* dynamic block */
						Tracev( ( stderr, "inflate:     dynamic codes block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = TABLE;
						break;

					case 3:
						strm->msg = ( char* )"invalid block type";
						state->mode = BAD;
				}

				DROPBITS( 2 );
				break;

			case STORED:
				/* get and verify stored block length */
				BYTEBITS();                         /* go to byte boundary */
				NEEDBITS( 32 );

				if ( ( hold & 0xffff ) != ( ( hold >> 16 ) ^ 0xffff ) )
				{
					strm->msg = ( char* )"invalid stored block lengths";
					state->mode = BAD;
					break;
				}

				state->length = ( unsigned )hold & 0xffff;
				Tracev( ( stderr, "inflate:       stored length %u\n",
				          state->length ) );
				INITBITS();

				/* copy stored block from input to output */
				while ( state->length != 0 )
				{
					copy = state->length;
					PULL();
					ROOM();

					if ( copy > have ) { copy = have; }

					if ( copy > left ) { copy = left; }

					zmemcpy( put, next, copy );
					have -= copy;
					next += copy;
					left -= copy;
					put += copy;
					state->length -= copy;
				}

				Tracev( ( stderr, "inflate:       stored end\n" ) );
				state->mode = TYPE;
				break;

			case TABLE:
				/* get dynamic table entries descriptor */
				NEEDBITS( 14 );
				state->nlen = BITS( 5 ) + 257;
				DROPBITS( 5 );
				state->ndist = BITS( 5 ) + 1;
				DROPBITS( 5 );
				state->ncode = BITS( 4 ) + 4;
				DROPBITS( 4 );
#ifndef PKZIP_BUG_WORKAROUND

				if ( state->nlen > 286 || state->ndist > 30 )
				{
					strm->msg = ( char* )"too many length or distance symbols";
					state->mode = BAD;
					break;
				}

#endif
				Tracev( ( stderr, "inflate:       table sizes ok\n" ) );

				/* get code length code lengths (not a typo) */
				state->have = 0;

				while ( state->have < state->ncode )
				{
					NEEDBITS( 3 );
					state->lens[order[state->have++]] = ( unsigned short )BITS( 3 );
					DROPBITS( 3 );
				}

				while ( state->have < 19 )
				{
					state->lens[order[state->have++]] = 0;
				}

				state->next = state->codes;
				state->lencode = ( code const FAR* )( state->next );
				state->lenbits = 7;
				ret = inflate_table( CODES, state->lens, 19, &( state->next ),
				                     &( state->lenbits ), state->work );

				if ( ret )
				{
					strm->msg = ( char* )"invalid code lengths set";
					state->mode = BAD;
					break;
				}

				Tracev( ( stderr, "inflate:       code lengths ok\n" ) );

				/* get length and distance code code lengths */
				state->have = 0;

				while ( state->have < state->nlen + state->ndist )
				{
					for ( ;; )
					{
						here = state->lencode[BITS( state->lenbits )];

						if ( ( unsigned )( here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					if ( here.val < 16 )
					{
						NEEDBITS( here.bits );
						DROPBITS( here.bits );
						state->lens[state->have++] = here.val;
					}
					else
					{
						if ( here.val == 16 )
						{
							NEEDBITS( here.bits + 2 );
							DROPBITS( here.bits );

							if ( state->have == 0 )
							{
								strm->msg = ( char* )"invalid bit length repeat";
								state->mode = BAD;
								break;
							}

							len = ( unsigned )( state->lens[state->have - 1] );
							copy = 3 + BITS( 2 );
							DROPBITS( 2 );
						}
						else if ( here.val == 17 )
						{
							NEEDBITS( here.bits + 3 );
							DROPBITS( here.bits );
							len = 0;
							copy = 3 + BITS( 3 );
							DROPBITS( 3 );
						}
						else
						{
							NEEDBITS( here.bits + 7 );
							DROPBITS( here.bits );
							len = 0;
							copy = 11 + BITS( 7 );
							DROPBITS( 7 );
						}

						if ( state->have + copy > state->nlen + state->ndist )
						{
							strm->msg = ( char* )"invalid bit length repeat";
							state->mode = BAD;
							break;
						}

						while ( copy-- )
						{
							state->lens[state->have++] = ( unsigned short )len;
						}
					}
				}

				/* handle error breaks in while */
				if ( state->mode == BAD ) { break; }

				/* check for end-of-block code (better have one) */
				if ( state->lens[256] == 0 )
				{
					strm->msg = ( char* )"invalid code -- missing end-of-block";
					state->mode = BAD;
					break;
				}

				/* build code tables -- note: do not change the lenbits or distbits
				   values here (9 and 6) without reading the comments in inftrees.h
				   concerning the ENOUGH constants, which depend on those values */
				state->next = state->codes;
				state->lencode = ( code const FAR* )( state->next );
				state->lenbits = 9;
				ret = inflate_table( LENS, state->lens, state->nlen, &( state->next ),
				                     &( state->lenbits ), state->work );

				if ( ret )
				{
					strm->msg = ( char* )"invalid literal/lengths set";
					state->mode = BAD;
					break;
				}

				state->distcode = ( code const FAR* )( state->next );
				state->distbits = 6;
				ret = inflate_table( DISTS, state->lens + state->nlen, state->ndist,
				                     &( state->next ), &( state->distbits ), state->work );

				if ( ret )
				{
					strm->msg = ( char* )"invalid distances set";
					state->mode = BAD;
					break;
				}

				Tracev( ( stderr, "inflate:       codes ok\n" ) );
				state->mode = LEN;

			case LEN:

				/* use inflate_fast() if we have enough input and output */
				if ( have >= 6 && left >= 258 )
				{
					RESTORE();

					if ( state->whave < state->wsize )
					{
						state->whave = state->wsize - left;
					}

					inflate_fast( strm, state->wsize );
					LOAD();
					break;
				}

				/* get a literal, length, or end-of-block code */
				for ( ;; )
				{
					here = state->lencode[BITS( state->lenbits )];

					if ( ( unsigned )( here.bits ) <= bits ) { break; }

					PULLBYTE();
				}

				if ( here.op && ( here.op & 0xf0 ) == 0 )
				{
					last = here;

					for ( ;; )
					{
						here = state->lencode[last.val +
						                      ( BITS( last.bits + last.op ) >> last.bits )];

						if ( ( unsigned )( last.bits + here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					DROPBITS( last.bits );
				}

				DROPBITS( here.bits );
				state->length = ( unsigned )here.val;

				/* process literal */
				if ( here.op == 0 )
				{
					Tracevv( ( stderr, here.val >= 0x20 && here.val < 0x7f ?
					           "inflate:         literal '%c'\n" :
					           "inflate:         literal 0x%02x\n", here.val ) );
					ROOM();
					*put++ = ( unsigned char )( state->length );
					left--;
					state->mode = LEN;
					break;
				}

				/* process end of block */
				if ( here.op & 32 )
				{
					Tracevv( ( stderr, "inflate:         end of block\n" ) );
					state->mode = TYPE;
					break;
				}

				/* invalid code */
				if ( here.op & 64 )
				{
					strm->msg = ( char* )"invalid literal/length code";
					state->mode = BAD;
					break;
				}

				/* length code -- get extra bits, if any */
				state->extra = ( unsigned )( here.op ) & 15;

				if ( state->extra != 0 )
				{
					NEEDBITS( state->extra );
					state->length += BITS( state->extra );
					DROPBITS( state->extra );
				}

				Tracevv( ( stderr, "inflate:         length %u\n", state->length ) );

				/* get distance code */
				for ( ;; )
				{
					here = state->distcode[BITS( state->distbits )];

					if ( ( unsigned )( here.bits ) <= bits ) { break; }

					PULLBYTE();
				}

				if ( ( here.op & 0xf0 ) == 0 )
				{
					last = here;

					for ( ;; )
					{
						here = state->distcode[last.val +
						                       ( BITS( last.bits + last.op ) >> last.bits )];

						if ( ( unsigned )( last.bits + here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					DROPBITS( last.bits );
				}

				DROPBITS( here.bits );

				if ( here.op & 64 )
				{
					strm->msg = ( char* )"invalid distance code";
					state->mode = BAD;
					break;
				}

				state->offset = ( unsigned )here.val;

				/* get distance extra bits, if any */
				state->extra = ( unsigned )( here.op ) & 15;

				if ( state->extra != 0 )
				{
					NEEDBITS( state->extra );
					state->offset += BITS( state->extra );
					DROPBITS( state->extra );
				}

				if ( state->offset > state->wsize - ( state->whave < state->wsize ?
				                                      left : 0 ) )
				{
					strm->msg = ( char* )"invalid distance too far back";
					state->mode = BAD;
					break;
				}

				Tracevv( ( stderr, "inflate:         distance %u\n", state->offset ) );

				/* copy match from window to output */
				do
				{
					ROOM();
					copy = state->wsize - state->offset;

					if ( copy < left )
					{
						from = put + copy;
						copy = left - copy;
					}
					else
					{
						from = put - state->offset;
						copy = left;
					}

					if ( copy > state->length ) { copy = state->length; }

					state->length -= copy;
					left -= copy;

					do
					{
						*put++ = *from++;
					}
					while ( --copy );
				}
				while ( state->length != 0 );

				break;

			case DONE:
				/* inflate stream terminated properly -- write leftover output */
				ret = Z_STREAM_END;

				if ( left < state->wsize )
				{
					if ( out( out_desc, state->window, state->wsize - left ) )
					{
						ret = Z_BUF_ERROR;
					}
				}

				goto inf_leave;

			case BAD:
				ret = Z_DATA_ERROR;
				goto inf_leave;

			default:                /* can't happen, but makes compilers happy */
				ret = Z_STREAM_ERROR;
				goto inf_leave;
		}

	/* Return unused input */
inf_leave:
	strm->next_in = next;
	strm->avail_in = have;
	return ret;
}

int ZEXPORT inflateBackEnd( strm )
z_streamp strm;
{
	if ( strm == Z_NULL || strm->state == Z_NULL || strm->zfree == ( free_func )0 )
	{
		return Z_STREAM_ERROR;
	}

	ZFREE( strm, strm->state );
	strm->state = Z_NULL;
	Tracev( ( stderr, "inflate: end\n" ) );
	return Z_OK;
}

#undef LOAD
#undef RESTORE
#undef INITBITS
#undef PULL
#undef PULLBYTE
#undef NEEDBITS
#undef BITS
#undef DROPBITS
#undef BYTEBITS
#undef ROOM

////////////// inflate.c

/* inflate.c -- zlib decompression
 * Copyright (C) 1995-2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/*
 * Change history:
 *
 * 1.2.beta0    24 Nov 2002
 * - First version -- complete rewrite of inflate to simplify code, avoid
 *   creation of window when not needed, minimize use of window when it is
 *   needed, make inffast.c even faster, implement gzip decoding, and to
 *   improve code readability and style over the previous zlib inflate code
 *
 * 1.2.beta1    25 Nov 2002
 * - Use pointers for available input and output checking in inffast.c
 * - Remove input and output counters in inffast.c
 * - Change inffast.c entry and loop from avail_in >= 7 to >= 6
 * - Remove unnecessary second byte pull from length extra in inffast.c
 * - Unroll direct copy to three copies per loop in inffast.c
 *
 * 1.2.beta2    4 Dec 2002
 * - Change external routine names to reduce potential conflicts
 * - Correct filename to inffixed.h for fixed tables in inflate.c
 * - Make hbuf[] unsigned char to match parameter type in inflate.c
 * - Change strm->next_out[-state->offset] to *(strm->next_out - state->offset)
 *   to avoid negation problem on Alphas (64 bit) in inflate.c
 *
 * 1.2.beta3    22 Dec 2002
 * - Add comments on state->bits assertion in inffast.c
 * - Add comments on op field in inftrees.h
 * - Fix bug in reuse of allocated window after inflateReset()
 * - Remove bit fields--back to byte structure for speed
 * - Remove distance extra == 0 check in inflate_fast()--only helps for lengths
 * - Change post-increments to pre-increments in inflate_fast(), PPC biased?
 * - Add compile time option, POSTINC, to use post-increments instead (Intel?)
 * - Make MATCH copy in inflate() much faster for when inflate_fast() not used
 * - Use local copies of stream next and avail values, as well as local bit
 *   buffer and bit count in inflate()--for speed when inflate_fast() not used
 *
 * 1.2.beta4    1 Jan 2003
 * - Split ptr - 257 statements in inflate_table() to avoid compiler warnings
 * - Move a comment on output buffer sizes from inffast.c to inflate.c
 * - Add comments in inffast.c to introduce the inflate_fast() routine
 * - Rearrange window copies in inflate_fast() for speed and simplification
 * - Unroll last copy for window match in inflate_fast()
 * - Use local copies of window variables in inflate_fast() for speed
 * - Pull out common wnext == 0 case for speed in inflate_fast()
 * - Make op and len in inflate_fast() unsigned for consistency
 * - Add FAR to lcode and dcode declarations in inflate_fast()
 * - Simplified bad distance check in inflate_fast()
 * - Added inflateBackInit(), inflateBack(), and inflateBackEnd() in new
 *   source file infback.c to provide a call-back interface to inflate for
 *   programs like gzip and unzip -- uses window as output buffer to avoid
 *   window copying
 *
 * 1.2.beta5    1 Jan 2003
 * - Improved inflateBack() interface to allow the caller to provide initial
 *   input in strm.
 * - Fixed stored blocks bug in inflateBack()
 *
 * 1.2.beta6    4 Jan 2003
 * - Added comments in inffast.c on effectiveness of POSTINC
 * - Typecasting all around to reduce compiler warnings
 * - Changed loops from while (1) or do {} while (1) to for (;;), again to
 *   make compilers happy
 * - Changed type of window in inflateBackInit() to unsigned char *
 *
 * 1.2.beta7    27 Jan 2003
 * - Changed many types to unsigned or unsigned short to avoid warnings
 * - Added inflateCopy() function
 *
 * 1.2.0        9 Mar 2003
 * - Changed inflateBack() interface to provide separate opaque descriptors
 *   for the in() and out() functions
 * - Changed inflateBack() argument and in_func typedef to swap the length
 *   and buffer address return values for the input function
 * - Check next_in and next_out for Z_NULL on entry to inflate()
 *
 * The history for versions after 1.2.0 are in ChangeLog in zlib distribution.
 */

#ifdef MAKEFIXED
#  ifndef BUILDFIXED
#    define BUILDFIXED
#  endif
#endif

/* function prototypes */
local void fixedtables OF( ( struct inflate_state FAR* state ) );
local int updatewindow OF( ( z_streamp strm, unsigned out ) );
#ifdef BUILDFIXED
void makefixed OF( ( void ) );
#endif
local unsigned syncsearch OF( ( unsigned FAR* have, unsigned char FAR* buf,
                                unsigned len ) );

int ZEXPORT inflateReset( strm )
z_streamp strm;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;
	strm->total_in = strm->total_out = state->total = 0;
	strm->msg = Z_NULL;
	strm->adler = 1;        /* to support ill-conceived Java test suite */
	state->mode = HEAD;
	state->last = 0;
	state->havedict = 0;
	state->dmax = 32768U;
	state->head = Z_NULL;
	state->wsize = 0;
	state->whave = 0;
	state->wnext = 0;
	state->hold = 0;
	state->bits = 0;
	state->lencode = state->distcode = state->next = state->codes;
	state->sane = 1;
	state->back = -1;
	Tracev( ( stderr, "inflate: reset\n" ) );
	return Z_OK;
}

int ZEXPORT inflateReset2( strm, windowBits )
z_streamp strm;
int windowBits;
{
	int wrap;
	struct inflate_state FAR* state;

	/* get the state */
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;

	/* extract wrap request from windowBits parameter */
	if ( windowBits < 0 )
	{
		wrap = 0;
		windowBits = -windowBits;
	}
	else
	{
		wrap = ( windowBits >> 4 ) + 1;
#ifdef GUNZIP

		if ( windowBits < 48 )
		{
			windowBits &= 15;
		}

#endif
	}

	/* set number of window bits, free window if different */
	if ( windowBits && ( windowBits < 8 || windowBits > 15 ) )
	{
		return Z_STREAM_ERROR;
	}

	if ( state->window != Z_NULL && state->wbits != ( unsigned )windowBits )
	{
		ZFREE( strm, state->window );
		state->window = Z_NULL;
	}

	/* update state and reset the rest of it */
	state->wrap = wrap;
	state->wbits = ( unsigned )windowBits;
	return inflateReset( strm );
}

int ZEXPORT inflateInit2_( strm, windowBits, version, stream_size )
z_streamp strm;
int windowBits;
const char* version;
int stream_size;
{
	int ret;
	struct inflate_state FAR* state;

	if ( version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
	     stream_size != ( int )( sizeof( z_stream ) ) )
	{
		return Z_VERSION_ERROR;
	}

	if ( strm == Z_NULL ) { return Z_STREAM_ERROR; }

	strm->msg = Z_NULL;                 /* in case we return an error */

	if ( strm->zalloc == ( alloc_func )0 )
	{
		strm->zalloc = zcalloc;
		strm->opaque = ( voidpf )0;
	}

	if ( strm->zfree == ( free_func )0 ) { strm->zfree = zcfree; }

	state = ( struct inflate_state FAR* )
	        ZALLOC( strm, 1, sizeof( struct inflate_state ) );

	if ( state == Z_NULL ) { return Z_MEM_ERROR; }

	Tracev( ( stderr, "inflate: allocated\n" ) );
	strm->state = ( struct internal_state FAR* )state;
	state->window = Z_NULL;
	ret = inflateReset2( strm, windowBits );

	if ( ret != Z_OK )
	{
		ZFREE( strm, state );
		strm->state = Z_NULL;
	}

	return ret;
}

int ZEXPORT inflateInit_( strm, version, stream_size )
z_streamp strm;
const char* version;
int stream_size;
{
	return inflateInit2_( strm, DEF_WBITS, version, stream_size );
}

int ZEXPORT inflatePrime( strm, bits, value )
z_streamp strm;
int bits;
int value;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;

	if ( bits < 0 )
	{
		state->hold = 0;
		state->bits = 0;
		return Z_OK;
	}

	if ( bits > 16 || state->bits + bits > 32 ) { return Z_STREAM_ERROR; }

	value &= ( 1L << bits ) - 1;
	state->hold += value << state->bits;
	state->bits += bits;
	return Z_OK;
}

#ifdef MAKEFIXED

/*
   Write out the inffixed.h that is #include'd above.  Defining MAKEFIXED also
   defines BUILDFIXED, so the tables are built on the fly.  makefixed() writes
   those tables to stdout, which would be piped to inffixed.h.  A small program
   can simply call makefixed to do this:

    void makefixed(void);

    int main(void)
    {
        makefixed();
        return 0;
    }

   Then that can be linked with zlib built with MAKEFIXED defined and run:

    a.out > inffixed.h
 */
void makefixed()
{
	unsigned low, size;
	struct inflate_state state;

	fixedtables( &state );
	puts( "    /* inffixed.h -- table for decoding fixed codes" );
	puts( "     * Generated automatically by makefixed()." );
	puts( "     */" );
	puts( "" );
	puts( "    /* WARNING: this file should *not* be used by applications." );
	puts( "       It is part of the implementation of this library and is" );
	puts( "       subject to change. Applications should only use zlib.h." );
	puts( "     */" );
	puts( "" );
	size = 1U << 9;
	printf( "    static const code lenfix[%u] = {", size );
	low = 0;

	for ( ;; )
	{
		if ( ( low % 7 ) == 0 ) { printf( "\n        " ); }

		printf( "{%u,%u,%d}", state.lencode[low].op, state.lencode[low].bits,
		        state.lencode[low].val );

		if ( ++low == size ) { break; }

		putchar( ',' );
	}

	puts( "\n    };" );
	size = 1U << 5;
	printf( "\n    static const code distfix[%u] = {", size );
	low = 0;

	for ( ;; )
	{
		if ( ( low % 6 ) == 0 ) { printf( "\n        " ); }

		printf( "{%u,%u,%d}", state.distcode[low].op, state.distcode[low].bits,
		        state.distcode[low].val );

		if ( ++low == size ) { break; }

		putchar( ',' );
	}

	puts( "\n    };" );
}
#endif /* MAKEFIXED */

/*
   Update the window with the last wsize (normally 32K) bytes written before
   returning.  If window does not exist yet, create it.  This is only called
   when a window is already in use, or when output has been written during this
   inflate call, but the end of the deflate stream has not been reached yet.
   It is also called to create a window for dictionary data when a dictionary
   is loaded.

   Providing output buffers larger than 32K to inflate() should provide a speed
   advantage, since only the last 32K of output is copied to the sliding window
   upon return from inflate(), and since all distances after the first 32K of
   output will fall in the output data, making match copies simpler and faster.
   The advantage may be dependent on the size of the processor's data caches.
 */
local int updatewindow( strm, out )
z_streamp strm;
unsigned out;
{
	struct inflate_state FAR* state;
	unsigned copy, dist;

	state = ( struct inflate_state FAR* )strm->state;

	/* if it hasn't been done already, allocate space for the window */
	if ( state->window == Z_NULL )
	{
		state->window = ( unsigned char FAR* )
		                ZALLOC( strm, 1U << state->wbits,
		                        sizeof( unsigned char ) );

		if ( state->window == Z_NULL ) { return 1; }
	}

	/* if window not in use yet, initialize */
	if ( state->wsize == 0 )
	{
		state->wsize = 1U << state->wbits;
		state->wnext = 0;
		state->whave = 0;
	}

	/* copy state->wsize or less output bytes into the circular window */
	copy = out - strm->avail_out;

	if ( copy >= state->wsize )
	{
		zmemcpy( state->window, strm->next_out - state->wsize, state->wsize );
		state->wnext = 0;
		state->whave = state->wsize;
	}
	else
	{
		dist = state->wsize - state->wnext;

		if ( dist > copy ) { dist = copy; }

		zmemcpy( state->window + state->wnext, strm->next_out - copy, dist );
		copy -= dist;

		if ( copy )
		{
			zmemcpy( state->window, strm->next_out - copy, copy );
			state->wnext = copy;
			state->whave = state->wsize;
		}
		else
		{
			state->wnext += dist;

			if ( state->wnext == state->wsize ) { state->wnext = 0; }

			if ( state->whave < state->wsize ) { state->whave += dist; }
		}
	}

	return 0;
}

/* Macros for inflate(): */

/* check function to use adler32() for zlib or crc32() for gzip */
#ifdef GUNZIP
#  define UPDATE(check, buf, len) \
    (state->flags ? crc32(check, buf, len) : adler32(check, buf, len))
#else
#  define UPDATE(check, buf, len) adler32(check, buf, len)
#endif

/* check macros for header crc */
#ifdef GUNZIP
#  define CRC2(check, word) \
    do { \
        hbuf[0] = (unsigned char)(word); \
        hbuf[1] = (unsigned char)((word) >> 8); \
        check = crc32(check, hbuf, 2); \
    } while (0)

#  define CRC4(check, word) \
    do { \
        hbuf[0] = (unsigned char)(word); \
        hbuf[1] = (unsigned char)((word) >> 8); \
        hbuf[2] = (unsigned char)((word) >> 16); \
        hbuf[3] = (unsigned char)((word) >> 24); \
        check = crc32(check, hbuf, 4); \
    } while (0)
#endif

/* Load registers with state in inflate() for speed */
#define LOAD() \
    do { \
        put = strm->next_out; \
        left = strm->avail_out; \
        next = strm->next_in; \
        have = strm->avail_in; \
        hold = state->hold; \
        bits = state->bits; \
    } while (0)

/* Restore state from registers in inflate() */
#define RESTORE() \
    do { \
        strm->next_out = put; \
        strm->avail_out = left; \
        strm->next_in = next; \
        strm->avail_in = have; \
        state->hold = hold; \
        state->bits = bits; \
    } while (0)

/* Clear the input bit accumulator */
#define INITBITS() \
    do { \
        hold = 0; \
        bits = 0; \
    } while (0)

/* Get a byte of input into the bit accumulator, or return from inflate()
   if there is no input available. */
#define PULLBYTE() \
    do { \
        if (have == 0) goto inf_leave; \
        have--; \
        hold += (unsigned long)(*next++) << bits; \
        bits += 8; \
    } while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflate(). */
#define NEEDBITS(n) \
    do { \
        while (bits < (unsigned)(n)) \
            PULLBYTE(); \
    } while (0)

/* Return the low n bits of the bit accumulator (n < 16) */
#define BITS(n) \
    ((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n) \
    do { \
        hold >>= (n); \
        bits -= (unsigned)(n); \
    } while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS() \
    do { \
        hold >>= bits & 7; \
        bits -= bits & 7; \
    } while (0)

/* Reverse the bytes in a 32-bit value */
#define REVERSE(q) \
    ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
     (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

/*
   inflate() uses a state machine to process as much input data and generate as
   much output data as possible before returning.  The state machine is
   structured roughly as follows:

    for (;;) switch (state) {
    ...
    case STATEn:
        if (not enough input data or output space to make progress)
            return;
        ... make progress ...
        state = STATEm;
        break;
    ...
    }

   so when inflate() is called again, the same case is attempted again, and
   if the appropriate resources are provided, the machine proceeds to the
   next state.  The NEEDBITS() macro is usually the way the state evaluates
   whether it can proceed or should return.  NEEDBITS() does the return if
   the requested bits are not available.  The typical use of the BITS macros
   is:

        NEEDBITS(n);
        ... do something with BITS(n) ...
        DROPBITS(n);

   where NEEDBITS(n) either returns from inflate() if there isn't enough
   input left to load n bits into the accumulator, or it continues.  BITS(n)
   gives the low n bits in the accumulator.  When done, DROPBITS(n) drops
   the low n bits off the accumulator.  INITBITS() clears the accumulator
   and sets the number of available bits to zero.  BYTEBITS() discards just
   enough bits to put the accumulator on a byte boundary.  After BYTEBITS()
   and a NEEDBITS(8), then BITS(8) would return the next byte in the stream.

   NEEDBITS(n) uses PULLBYTE() to get an available byte of input, or to return
   if there is no input available.  The decoding of variable length codes uses
   PULLBYTE() directly in order to pull just enough bytes to decode the next
   code, and no more.

   Some states loop until they get enough input, making sure that enough
   state information is maintained to continue the loop where it left off
   if NEEDBITS() returns in the loop.  For example, want, need, and keep
   would all have to actually be part of the saved state in case NEEDBITS()
   returns:

    case STATEw:
        while (want < need) {
            NEEDBITS(n);
            keep[want++] = BITS(n);
            DROPBITS(n);
        }
        state = STATEx;
    case STATEx:

   As shown above, if the next state is also the next case, then the break
   is omitted.

   A state may also return if there is not enough output space available to
   complete that state.  Those states are copying stored data, writing a
   literal byte, and copying a matching string.

   When returning, a "goto inf_leave" is used to update the total counters,
   update the check value, and determine whether any progress has been made
   during that inflate() call in order to return the proper return code.
   Progress is defined as a change in either strm->avail_in or strm->avail_out.
   When there is a window, goto inf_leave will update the window with the last
   output written.  If a goto inf_leave occurs in the middle of decompression
   and there is no window currently, goto inf_leave will create one and copy
   output to the window for the next call of inflate().

   In this implementation, the flush parameter of inflate() only affects the
   return code (per zlib.h).  inflate() always writes as much as possible to
   strm->next_out, given the space available and the provided input--the effect
   documented in zlib.h of Z_SYNC_FLUSH.  Furthermore, inflate() always defers
   the allocation of and copying into a sliding window until necessary, which
   provides the effect documented in zlib.h for Z_FINISH when the entire input
   stream available.  So the only thing the flush parameter actually does is:
   when flush is set to Z_FINISH, inflate() cannot return Z_OK.  Instead it
   will return Z_BUF_ERROR if it has not reached the end of the stream.
 */

int ZEXPORT inflate( strm, flush )
z_streamp strm;
int flush;
{
	struct inflate_state FAR* state;
	unsigned char FAR* next;    /* next input */
	unsigned char FAR* put;     /* next output */
	unsigned have, left;        /* available input and output */
	unsigned long hold;         /* bit buffer */
	unsigned bits;              /* bits in bit buffer */
	unsigned in, out;           /* save starting available input and output */
	unsigned copy;              /* number of stored or match bytes to copy */
	unsigned char FAR* from;    /* where to copy match bytes from */
	code here;                  /* current decoding table entry */
	code last;                  /* parent table entry */
	unsigned len;               /* length to copy for repeats, bits to drop */
	int ret;                    /* return code */
#ifdef GUNZIP
	unsigned char hbuf[4];      /* buffer for gzip header crc calculation */
#endif
	static const unsigned short order[19] = /* permutation of code lengths */
	{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

	if ( strm == Z_NULL || strm->state == Z_NULL || strm->next_out == Z_NULL ||
	     ( strm->next_in == Z_NULL && strm->avail_in != 0 ) )
	{
		return Z_STREAM_ERROR;
	}

	state = ( struct inflate_state FAR* )strm->state;

	if ( state->mode == TYPE ) { state->mode = TYPEDO; }    /* skip check */

	LOAD();
	in = have;
	out = left;
	ret = Z_OK;

	for ( ;; )
		switch ( state->mode )
		{
			case HEAD:
				if ( state->wrap == 0 )
				{
					state->mode = TYPEDO;
					break;
				}

				NEEDBITS( 16 );
#ifdef GUNZIP

				if ( ( state->wrap & 2 ) && hold == 0x8b1f ) /* gzip header */
				{
					state->check = crc32( 0L, Z_NULL, 0 );
					CRC2( state->check, hold );
					INITBITS();
					state->mode = FLAGS;
					break;
				}

				state->flags = 0;           /* expect zlib header */

				if ( state->head != Z_NULL )
				{
					state->head->done = -1;
				}

				if ( !( state->wrap & 1 ) || /* check if zlib header allowed */
#else
				if (
#endif
				     ( ( BITS( 8 ) << 8 ) + ( hold >> 8 ) ) % 31 )
				{
					strm->msg = ( char* )"incorrect header check";
					state->mode = BAD;
					break;
				}
				if ( BITS( 4 ) != Z_DEFLATED )
				{
					strm->msg = ( char* )"unknown compression method";
					state->mode = BAD;
					break;
				}
				DROPBITS( 4 );
				len = BITS( 4 ) + 8;

				if ( state->wbits == 0 )
					state->wbits = len;
				else if ( len > state->wbits )
				{
					strm->msg = ( char* )"invalid window size";
					state->mode = BAD;
					break;
				}
				state->dmax = 1U << len;
				Tracev( ( stderr, "inflate:   zlib header ok\n" ) );
				strm->adler = state->check = adler32( 0L, Z_NULL, 0 );
				state->mode = hold & 0x200 ? DICTID : TYPE;
				INITBITS();
				break;
#ifdef GUNZIP

			case FLAGS:
				NEEDBITS( 16 );
				state->flags = ( int )( hold );
				if ( ( state->flags & 0xff ) != Z_DEFLATED )
				{
					strm->msg = ( char* )"unknown compression method";
					state->mode = BAD;
					break;
				}
				if ( state->flags & 0xe000 )
				{
					strm->msg = ( char* )"unknown header flags set";
					state->mode = BAD;
					break;
				}
				if ( state->head != Z_NULL )
					state->head->text = ( int )( ( hold >> 8 ) & 1 );
				if ( state->flags & 0x0200 ) CRC2( state->check, hold );
				INITBITS();
				state->mode = TIME;
			case TIME:
				NEEDBITS( 32 );
				if ( state->head != Z_NULL )
					state->head->time = hold;
				if ( state->flags & 0x0200 ) CRC4( state->check, hold );
				INITBITS();
				state->mode = OS;
			case OS:
				NEEDBITS( 16 );
				if ( state->head != Z_NULL )
				{
					state->head->xflags = ( int )( hold & 0xff );
					state->head->os = ( int )( hold >> 8 );
				}
				if ( state->flags & 0x0200 ) CRC2( state->check, hold );
				INITBITS();
				state->mode = EXLEN;
			case EXLEN:
				if ( state->flags & 0x0400 )
				{
					NEEDBITS( 16 );
					state->length = ( unsigned )( hold );

					if ( state->head != Z_NULL )
					{
						state->head->extra_len = ( unsigned )hold;
					}

					if ( state->flags & 0x0200 ) { CRC2( state->check, hold ); }

					INITBITS();
				}
				else if ( state->head != Z_NULL )
					state->head->extra = Z_NULL;
				state->mode = EXTRA;
			case EXTRA:
				if ( state->flags & 0x0400 )
				{
					copy = state->length;

					if ( copy > have ) { copy = have; }

					if ( copy )
					{
						if ( state->head != Z_NULL &&
						     state->head->extra != Z_NULL )
						{
							len = state->head->extra_len - state->length;
							zmemcpy( state->head->extra + len, next,
							         len + copy > state->head->extra_max ?
							         state->head->extra_max - len : copy );
						}

						if ( state->flags & 0x0200 )
						{
							state->check = crc32( state->check, next, copy );
						}

						have -= copy;
						next += copy;
						state->length -= copy;
					}

					if ( state->length ) { goto inf_leave; }
				}
				state->length = 0;
				state->mode = NAME;

			case NAME:
				if ( state->flags & 0x0800 )
				{
					if ( have == 0 ) { goto inf_leave; }

					copy = 0;

					do
					{
						len = ( unsigned )( next[copy++] );

						if ( state->head != Z_NULL &&
						     state->head->name != Z_NULL &&
						     state->length < state->head->name_max )
						{
							state->head->name[state->length++] = len;
						}
					}
					while ( len && copy < have );

					if ( state->flags & 0x0200 )
					{
						state->check = crc32( state->check, next, copy );
					}

					have -= copy;
					next += copy;

					if ( len ) { goto inf_leave; }
				}
				else if ( state->head != Z_NULL )
					state->head->name = Z_NULL;
				state->length = 0;
				state->mode = COMMENT;
			case COMMENT:
				if ( state->flags & 0x1000 )
				{
					if ( have == 0 ) { goto inf_leave; }

					copy = 0;

					do
					{
						len = ( unsigned )( next[copy++] );

						if ( state->head != Z_NULL &&
						     state->head->comment != Z_NULL &&
						     state->length < state->head->comm_max )
						{
							state->head->comment[state->length++] = len;
						}
					}
					while ( len && copy < have );

					if ( state->flags & 0x0200 )
					{
						state->check = crc32( state->check, next, copy );
					}

					have -= copy;
					next += copy;

					if ( len ) { goto inf_leave; }
				}
				else if ( state->head != Z_NULL )
					state->head->comment = Z_NULL;
				state->mode = HCRC;
			case HCRC:
				if ( state->flags & 0x0200 )
				{
					NEEDBITS( 16 );

					if ( hold != ( state->check & 0xffff ) )
					{
						strm->msg = ( char* )"header crc mismatch";
						state->mode = BAD;
						break;
					}

					INITBITS();
				}
				if ( state->head != Z_NULL )
				{
					state->head->hcrc = ( int )( ( state->flags >> 9 ) & 1 );
					state->head->done = 1;
				}
				strm->adler = state->check = crc32( 0L, Z_NULL, 0 );
				state->mode = TYPE;
				break;
#endif

			case DICTID:
				NEEDBITS( 32 );
				strm->adler = state->check = REVERSE( hold );
				INITBITS();
				state->mode = DICT;
			case DICT:
				if ( state->havedict == 0 )
				{
					RESTORE();
					return Z_NEED_DICT;
				}
				strm->adler = state->check = adler32( 0L, Z_NULL, 0 );
				state->mode = TYPE;

			case TYPE:
				if ( flush == Z_BLOCK || flush == Z_TREES ) goto inf_leave;
			case TYPEDO:
				if ( state->last )
				{
					BYTEBITS();
					state->mode = CHECK;
					break;
				}
				NEEDBITS( 3 );
				state->last = BITS( 1 );
				DROPBITS( 1 );

				switch ( BITS( 2 ) )
				{
					case 0:                             /* stored block */
						Tracev( ( stderr, "inflate:     stored block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = STORED;
						break;

					case 1:                             /* fixed block */
						fixedtables( state );
						Tracev( ( stderr, "inflate:     fixed codes block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = LEN_;             /* decode codes */

						if ( flush == Z_TREES )
						{
							DROPBITS( 2 );
							goto inf_leave;
						}

						break;

					case 2:                             /* dynamic block */
						Tracev( ( stderr, "inflate:     dynamic codes block%s\n",
						          state->last ? " (last)" : "" ) );
						state->mode = TABLE;
						break;

					case 3:
						strm->msg = ( char* )"invalid block type";
						state->mode = BAD;
				}
				DROPBITS( 2 );
				break;

			case STORED:
				BYTEBITS();                         /* go to byte boundary */
				NEEDBITS( 32 );
				if ( ( hold & 0xffff ) != ( ( hold >> 16 ) ^ 0xffff ) )
				{
					strm->msg = ( char* )"invalid stored block lengths";
					state->mode = BAD;
					break;
				}
				state->length = ( unsigned )hold & 0xffff;
				Tracev( ( stderr, "inflate:       stored length %u\n",
				          state->length ) );
				INITBITS();
				state->mode = COPY_;

				if ( flush == Z_TREES ) goto inf_leave;
			case COPY_:
				state->mode = COPY;
			case COPY:
				copy = state->length;
				if ( copy )
				{
					if ( copy > have ) { copy = have; }

					if ( copy > left ) { copy = left; }

					if ( copy == 0 ) { goto inf_leave; }

					zmemcpy( put, next, copy );
					have -= copy;
					next += copy;
					left -= copy;
					put += copy;
					state->length -= copy;
					break;
				}
				Tracev( ( stderr, "inflate:       stored end\n" ) );
				state->mode = TYPE;
				break;

			case TABLE:
				NEEDBITS( 14 );
				state->nlen = BITS( 5 ) + 257;
				DROPBITS( 5 );
				state->ndist = BITS( 5 ) + 1;
				DROPBITS( 5 );
				state->ncode = BITS( 4 ) + 4;
				DROPBITS( 4 );
#ifndef PKZIP_BUG_WORKAROUND
				if ( state->nlen > 286 || state->ndist > 30 )
				{
					strm->msg = ( char* )"too many length or distance symbols";
					state->mode = BAD;
					break;
				}
#endif
				Tracev( ( stderr, "inflate:       table sizes ok\n" ) );
				state->have = 0;
				state->mode = LENLENS;

			case LENLENS:
				while ( state->have < state->ncode )
				{
					NEEDBITS( 3 );
					state->lens[order[state->have++]] = ( unsigned short )BITS( 3 );
					DROPBITS( 3 );
				}
				while ( state->have < 19 )
					state->lens[order[state->have++]] = 0;
				state->next = state->codes;
				state->lencode = ( code const FAR* )( state->next );
				state->lenbits = 7;
				ret = inflate_table( CODES, state->lens, 19, &( state->next ),
				                     &( state->lenbits ), state->work );
				if ( ret )
				{
					strm->msg = ( char* )"invalid code lengths set";
					state->mode = BAD;
					break;
				}
				Tracev( ( stderr, "inflate:       code lengths ok\n" ) );
				state->have = 0;
				state->mode = CODELENS;

			case CODELENS:
				while ( state->have < state->nlen + state->ndist )
				{
					for ( ;; )
					{
						here = state->lencode[BITS( state->lenbits )];

						if ( ( unsigned )( here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					if ( here.val < 16 )
					{
						NEEDBITS( here.bits );
						DROPBITS( here.bits );
						state->lens[state->have++] = here.val;
					}
					else
					{
						if ( here.val == 16 )
						{
							NEEDBITS( here.bits + 2 );
							DROPBITS( here.bits );

							if ( state->have == 0 )
							{
								strm->msg = ( char* )"invalid bit length repeat";
								state->mode = BAD;
								break;
							}

							len = state->lens[state->have - 1];
							copy = 3 + BITS( 2 );
							DROPBITS( 2 );
						}
						else if ( here.val == 17 )
						{
							NEEDBITS( here.bits + 3 );
							DROPBITS( here.bits );
							len = 0;
							copy = 3 + BITS( 3 );
							DROPBITS( 3 );
						}
						else
						{
							NEEDBITS( here.bits + 7 );
							DROPBITS( here.bits );
							len = 0;
							copy = 11 + BITS( 7 );
							DROPBITS( 7 );
						}

						if ( state->have + copy > state->nlen + state->ndist )
						{
							strm->msg = ( char* )"invalid bit length repeat";
							state->mode = BAD;
							break;
						}

						while ( copy-- )
						{
							state->lens[state->have++] = ( unsigned short )len;
						}
					}
				}

				/* handle error breaks in while */
				if ( state->mode == BAD ) break;

				/* check for end-of-block code (better have one) */
				if ( state->lens[256] == 0 )
				{
					strm->msg = ( char* )"invalid code -- missing end-of-block";
					state->mode = BAD;
					break;
				}

				/* build code tables -- note: do not change the lenbits or distbits
				   values here (9 and 6) without reading the comments in inftrees.h
				   concerning the ENOUGH constants, which depend on those values */
				state->next = state->codes;
				state->lencode = ( code const FAR* )( state->next );
				state->lenbits = 9;
				ret = inflate_table( LENS, state->lens, state->nlen, &( state->next ),
				                     &( state->lenbits ), state->work );

				if ( ret )
				{
					strm->msg = ( char* )"invalid literal/lengths set";
					state->mode = BAD;
					break;
				}
				state->distcode = ( code const FAR* )( state->next );
				state->distbits = 6;
				ret = inflate_table( DISTS, state->lens + state->nlen, state->ndist,
				                     &( state->next ), &( state->distbits ), state->work );

				if ( ret )
				{
					strm->msg = ( char* )"invalid distances set";
					state->mode = BAD;
					break;
				}
				Tracev( ( stderr, "inflate:       codes ok\n" ) );
				state->mode = LEN_;

				if ( flush == Z_TREES ) goto inf_leave;
			case LEN_:
				state->mode = LEN;
			case LEN:
				if ( have >= 6 && left >= 258 )
				{
					RESTORE();
					inflate_fast( strm, out );
					LOAD();

					if ( state->mode == TYPE )
					{
						state->back = -1;
					}

					break;
				}
				state->back = 0;

				for ( ;; )
				{
					here = state->lencode[BITS( state->lenbits )];

					if ( ( unsigned )( here.bits ) <= bits ) { break; }

					PULLBYTE();
				}
				if ( here.op && ( here.op & 0xf0 ) == 0 )
				{
					last = here;

					for ( ;; )
					{
						here = state->lencode[last.val +
						                      ( BITS( last.bits + last.op ) >> last.bits )];

						if ( ( unsigned )( last.bits + here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					DROPBITS( last.bits );
					state->back += last.bits;
				}
				DROPBITS( here.bits );
				state->back += here.bits;
				state->length = ( unsigned )here.val;

				if ( ( int )( here.op ) == 0 )
				{
					Tracevv( ( stderr, here.val >= 0x20 && here.val < 0x7f ?
					           "inflate:         literal '%c'\n" :
					           "inflate:         literal 0x%02x\n", here.val ) );
					state->mode = LIT;
					break;
				}
				if ( here.op & 32 )
				{
					Tracevv( ( stderr, "inflate:         end of block\n" ) );
					state->back = -1;
					state->mode = TYPE;
					break;
				}
				if ( here.op & 64 )
				{
					strm->msg = ( char* )"invalid literal/length code";
					state->mode = BAD;
					break;
				}
				state->extra = ( unsigned )( here.op ) & 15;
				state->mode = LENEXT;

			case LENEXT:
				if ( state->extra )
				{
					NEEDBITS( state->extra );
					state->length += BITS( state->extra );
					DROPBITS( state->extra );
					state->back += state->extra;
				}
				Tracevv( ( stderr, "inflate:         length %u\n", state->length ) );
				state->was = state->length;
				state->mode = DIST;

			case DIST:
				for ( ;; )
				{
					here = state->distcode[BITS( state->distbits )];

					if ( ( unsigned )( here.bits ) <= bits ) { break; }

					PULLBYTE();
				}
				if ( ( here.op & 0xf0 ) == 0 )
				{
					last = here;

					for ( ;; )
					{
						here = state->distcode[last.val +
						                       ( BITS( last.bits + last.op ) >> last.bits )];

						if ( ( unsigned )( last.bits + here.bits ) <= bits ) { break; }

						PULLBYTE();
					}

					DROPBITS( last.bits );
					state->back += last.bits;
				}
				DROPBITS( here.bits );
				state->back += here.bits;

				if ( here.op & 64 )
				{
					strm->msg = ( char* )"invalid distance code";
					state->mode = BAD;
					break;
				}
				state->offset = ( unsigned )here.val;
				state->extra = ( unsigned )( here.op ) & 15;
				state->mode = DISTEXT;

			case DISTEXT:
				if ( state->extra )
				{
					NEEDBITS( state->extra );
					state->offset += BITS( state->extra );
					DROPBITS( state->extra );
					state->back += state->extra;
				}
#ifdef INFLATE_STRICT

				if ( state->offset > state->dmax )
				{
					strm->msg = ( char* )"invalid distance too far back";
					state->mode = BAD;
					break;
				}
#endif
				Tracevv( ( stderr, "inflate:         distance %u\n", state->offset ) );
				state->mode = MATCH;

			case MATCH:
				if ( left == 0 ) goto inf_leave;
				copy = out - left;
				if ( state->offset > copy )         /* copy from window */
				{
					copy = state->offset - copy;

					if ( copy > state->whave )
					{
						if ( state->sane )
						{
							strm->msg = ( char* )"invalid distance too far back";
							state->mode = BAD;
							break;
						}

#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
						Trace( ( stderr, "inflate.c too far\n" ) );
						copy -= state->whave;

						if ( copy > state->length ) { copy = state->length; }

						if ( copy > left ) { copy = left; }

						left -= copy;
						state->length -= copy;

						do
						{
							*put++ = 0;
						}
						while ( --copy );

						if ( state->length == 0 ) { state->mode = LEN; }

						break;
#endif
					}

					if ( copy > state->wnext )
					{
						copy -= state->wnext;
						from = state->window + ( state->wsize - copy );
					}
					else
					{
						from = state->window + ( state->wnext - copy );
					}

					if ( copy > state->length ) { copy = state->length; }
				}
				else                                /* copy from output */
				{
					from = put - state->offset;
					copy = state->length;
				}
				if ( copy > left ) copy = left;
				left -= copy;
				state->length -= copy;
				do
				{
					*put++ = *from++;
				}
				while ( --copy );
				if ( state->length == 0 ) state->mode = LEN;
				break;
			case LIT:
				if ( left == 0 ) goto inf_leave;
				*put++ = ( unsigned char )( state->length );
				left--;
				state->mode = LEN;
				break;
			case CHECK:
				if ( state->wrap )
				{
					NEEDBITS( 32 );
					out -= left;
					strm->total_out += out;
					state->total += out;

					if ( out )
						strm->adler = state->check =
						                 UPDATE( state->check, put - out, out );

					out = left;

					if ( (
#ifdef GUNZIP
					        state->flags ? hold :
#endif
					        REVERSE( hold ) ) != state->check )
					{
						strm->msg = ( char* )"incorrect data check";
						state->mode = BAD;
						break;
					}

					INITBITS();
					Tracev( ( stderr, "inflate:   check matches trailer\n" ) );
				}
#ifdef GUNZIP
				state->mode = LENGTH;

			case LENGTH:
				if ( state->wrap && state->flags )
				{
					NEEDBITS( 32 );

					if ( hold != ( state->total & 0xffffffffUL ) )
					{
						strm->msg = ( char* )"incorrect length check";
						state->mode = BAD;
						break;
					}

					INITBITS();
					Tracev( ( stderr, "inflate:   length matches trailer\n" ) );
				}
#endif
				state->mode = DONE;

			case DONE:
				ret = Z_STREAM_END;
				goto inf_leave;
			case BAD:
				ret = Z_DATA_ERROR;
				goto inf_leave;
			case MEM:
				return Z_MEM_ERROR;
			case SYNC:
			default:
				return Z_STREAM_ERROR;
		}

	/*
	   Return from inflate(), updating the total counts and the check value.
	   If there was no progress during the inflate() call, return a buffer
	   error.  Call updatewindow() to create and/or update the window state.
	   Note: a memory error from inflate() is non-recoverable.
	 */
inf_leave:
	RESTORE();

	if ( state->wsize || ( state->mode < CHECK && out != strm->avail_out ) )
		if ( updatewindow( strm, out ) )
		{
			state->mode = MEM;
			return Z_MEM_ERROR;
		}

	in -= strm->avail_in;
	out -= strm->avail_out;
	strm->total_in += in;
	strm->total_out += out;
	state->total += out;

	if ( state->wrap && out )
		strm->adler = state->check =
		                 UPDATE( state->check, strm->next_out - out, out );

	strm->data_type = state->bits + ( state->last ? 64 : 0 ) +
	                  ( state->mode == TYPE ? 128 : 0 ) +
	                  ( state->mode == LEN_ || state->mode == COPY_ ? 256 : 0 );

	if ( ( ( in == 0 && out == 0 ) || flush == Z_FINISH ) && ret == Z_OK )
	{
		ret = Z_BUF_ERROR;
	}

	return ret;
}

int ZEXPORT inflateEnd( strm )
z_streamp strm;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL || strm->zfree == ( free_func )0 )
	{
		return Z_STREAM_ERROR;
	}

	state = ( struct inflate_state FAR* )strm->state;

	if ( state->window != Z_NULL ) { ZFREE( strm, state->window ); }

	ZFREE( strm, strm->state );
	strm->state = Z_NULL;
	Tracev( ( stderr, "inflate: end\n" ) );
	return Z_OK;
}

int ZEXPORT inflateSetDictionary( strm, dictionary, dictLength )
z_streamp strm;
const Bytef* dictionary;
uInt dictLength;
{
	struct inflate_state FAR* state;
	unsigned long id;

	/* check state */
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;

	if ( state->wrap != 0 && state->mode != DICT )
	{
		return Z_STREAM_ERROR;
	}

	/* check for correct dictionary id */
	if ( state->mode == DICT )
	{
		id = adler32( 0L, Z_NULL, 0 );
		id = adler32( id, dictionary, dictLength );

		if ( id != state->check )
		{
			return Z_DATA_ERROR;
		}
	}

	/* copy dictionary to window */
	if ( updatewindow( strm, strm->avail_out ) )
	{
		state->mode = MEM;
		return Z_MEM_ERROR;
	}

	if ( dictLength > state->wsize )
	{
		zmemcpy( state->window, dictionary + dictLength - state->wsize,
		         state->wsize );
		state->whave = state->wsize;
	}
	else
	{
		zmemcpy( state->window + state->wsize - dictLength, dictionary,
		         dictLength );
		state->whave = dictLength;
	}

	state->havedict = 1;
	Tracev( ( stderr, "inflate:   dictionary set\n" ) );
	return Z_OK;
}

int ZEXPORT inflateGetHeader( strm, head )
z_streamp strm;
gz_headerp head;
{
	struct inflate_state FAR* state;

	/* check state */
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;

	if ( ( state->wrap & 2 ) == 0 ) { return Z_STREAM_ERROR; }

	/* save header structure */
	state->head = head;
	head->done = 0;
	return Z_OK;
}

/*
   Search buf[0..len-1] for the pattern: 0, 0, 0xff, 0xff.  Return when found
   or when out of input.  When called, *have is the number of pattern bytes
   found in order so far, in 0..3.  On return *have is updated to the new
   state.  If on return *have equals four, then the pattern was found and the
   return value is how many bytes were read including the last byte of the
   pattern.  If *have is less than four, then the pattern has not been found
   yet and the return value is len.  In the latter case, syncsearch() can be
   called again with more data and the *have state.  *have is initialized to
   zero for the first call.
 */
local unsigned syncsearch( have, buf, len )
unsigned FAR* have;
unsigned char FAR* buf;
unsigned len;
{
	unsigned got;
	unsigned next;

	got = *have;
	next = 0;

	while ( next < len && got < 4 )
	{
		if ( ( int )( buf[next] ) == ( got < 2 ? 0 : 0xff ) )
		{
			got++;
		}
		else if ( buf[next] )
		{
			got = 0;
		}
		else
		{
			got = 4 - got;
		}

		next++;
	}

	*have = got;
	return next;
}

int ZEXPORT inflateSync( strm )
z_streamp strm;
{
	unsigned len;               /* number of bytes to look at or looked at */
	unsigned long in, out;      /* temporary to save total_in and total_out */
	unsigned char buf[4];       /* to restore bit buffer to byte string */
	struct inflate_state FAR* state;

	/* check parameters */
	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;

	if ( strm->avail_in == 0 && state->bits < 8 ) { return Z_BUF_ERROR; }

	/* if first time, start search in bit buffer */
	if ( state->mode != SYNC )
	{
		state->mode = SYNC;
		state->hold <<= state->bits & 7;
		state->bits -= state->bits & 7;
		len = 0;

		while ( state->bits >= 8 )
		{
			buf[len++] = ( unsigned char )( state->hold );
			state->hold >>= 8;
			state->bits -= 8;
		}

		state->have = 0;
		syncsearch( &( state->have ), buf, len );
	}

	/* search available input */
	len = syncsearch( &( state->have ), strm->next_in, strm->avail_in );
	strm->avail_in -= len;
	strm->next_in += len;
	strm->total_in += len;

	/* return no joy or set up to restart inflate() on a new block */
	if ( state->have != 4 ) { return Z_DATA_ERROR; }

	in = strm->total_in;
	out = strm->total_out;
	inflateReset( strm );
	strm->total_in = in;
	strm->total_out = out;
	state->mode = TYPE;
	return Z_OK;
}

/*
   Returns true if inflate is currently at the end of a block generated by
   Z_SYNC_FLUSH or Z_FULL_FLUSH. This function is used by one PPP
   implementation to provide an additional safety check. PPP uses
   Z_SYNC_FLUSH but removes the length bytes of the resulting empty stored
   block. When decompressing, PPP checks that at the end of input packet,
   inflate is waiting for these length bytes.
 */
int ZEXPORT inflateSyncPoint( strm )
z_streamp strm;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;
	return state->mode == STORED && state->bits == 0;
}

int ZEXPORT inflateCopy( dest, source )
z_streamp dest;
z_streamp source;
{
	struct inflate_state FAR* state;
	struct inflate_state FAR* copy;
	unsigned char FAR* window;
	unsigned wsize;

	/* check input */
	if ( dest == Z_NULL || source == Z_NULL || source->state == Z_NULL ||
	     source->zalloc == ( alloc_func )0 || source->zfree == ( free_func )0 )
	{
		return Z_STREAM_ERROR;
	}

	state = ( struct inflate_state FAR* )source->state;

	/* allocate space */
	copy = ( struct inflate_state FAR* )
	       ZALLOC( source, 1, sizeof( struct inflate_state ) );

	if ( copy == Z_NULL ) { return Z_MEM_ERROR; }

	window = Z_NULL;

	if ( state->window != Z_NULL )
	{
		window = ( unsigned char FAR* )
		         ZALLOC( source, 1U << state->wbits, sizeof( unsigned char ) );

		if ( window == Z_NULL )
		{
			ZFREE( source, copy );
			return Z_MEM_ERROR;
		}
	}

	/* copy state */
	zmemcpy( dest, source, sizeof( z_stream ) );
	zmemcpy( copy, state, sizeof( struct inflate_state ) );

	if ( state->lencode >= state->codes &&
	     state->lencode <= state->codes + ENOUGH - 1 )
	{
		copy->lencode = copy->codes + ( state->lencode - state->codes );
		copy->distcode = copy->codes + ( state->distcode - state->codes );
	}

	copy->next = copy->codes + ( state->next - state->codes );

	if ( window != Z_NULL )
	{
		wsize = 1U << state->wbits;
		zmemcpy( window, state->window, wsize );
	}

	copy->window = window;
	dest->state = ( struct internal_state FAR* )copy;
	return Z_OK;
}

int ZEXPORT inflateUndermine( strm, subvert )
z_streamp strm;
int subvert;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return Z_STREAM_ERROR; }

	state = ( struct inflate_state FAR* )strm->state;
	state->sane = !subvert;
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
	return Z_OK;
#else
	state->sane = 1;
	return Z_DATA_ERROR;
#endif
}

long ZEXPORT inflateMark( strm )
z_streamp strm;
{
	struct inflate_state FAR* state;

	if ( strm == Z_NULL || strm->state == Z_NULL ) { return -1L << 16; }

	state = ( struct inflate_state FAR* )strm->state;
	return ( ( long )( state->back ) << 16 ) +
	       ( state->mode == COPY ? state->length :
	         ( state->mode == MATCH ? state->was - state->length : 0 ) );
}

//////// end of inflate


/* inffast.c -- fast decoding
 * Copyright (C) 1995-2008, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef ASMINF

/* Allow machine dependent optimization for post-increment or pre-increment.
   Based on testing to date,
   Pre-increment preferred for:
   - PowerPC G3 (Adler)
   - MIPS R5000 (Randers-Pehrson)
   Post-increment preferred for:
   - none
   No measurable difference:
   - Pentium III (Anderson)
   - M68060 (Nikl)
 */
#ifdef POSTINC
#  define OFF 0
#  define PUP(a) *(a)++
#else
#  define OFF 1
#  define PUP(a) *++(a)
#endif

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

        state->mode == LEN
        strm->avail_in >= 6
        strm->avail_out >= 258
        start >= strm->avail_out
        state->bits < 8

   On return, state->mode is one of:

        LEN -- ran out of enough output space or enough available input
        TYPE -- reached end of block code, inflate() to interpret next block
        BAD -- error in block data

   Notes:

    - The maximum input bits used by a length/distance pair is 15 bits for the
      length code, 5 bits for the length extra, 15 bits for the distance code,
      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
      Therefore if strm->avail_in >= 6, then there is enough input to avoid
      checking for available input while decoding.

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires strm->avail_out >= 258 for each loop to avoid checking for
      output space.
 */
void ZLIB_INTERNAL inflate_fast( strm, start )
z_streamp strm;
unsigned start;         /* inflate()'s starting value for strm->avail_out */
{
	struct inflate_state FAR* state;
	unsigned char FAR* in;      /* local strm->next_in */
	unsigned char FAR* last;    /* while in < last, enough input available */
	unsigned char FAR* out;     /* local strm->next_out */
	unsigned char FAR* beg;     /* inflate()'s initial strm->next_out */
	unsigned char FAR* end;     /* while out < end, enough space available */
#ifdef INFLATE_STRICT
	unsigned dmax;              /* maximum distance from zlib header */
#endif
	unsigned wsize;             /* window size or zero if not using window */
	unsigned whave;             /* valid bytes in the window */
	unsigned wnext;             /* window write index */
	unsigned char FAR* window;  /* allocated sliding window, if wsize != 0 */
	unsigned long hold;         /* local strm->hold */
	unsigned bits;              /* local strm->bits */
	code const FAR* lcode;      /* local strm->lencode */
	code const FAR* dcode;      /* local strm->distcode */
	unsigned lmask;             /* mask for first level of length codes */
	unsigned dmask;             /* mask for first level of distance codes */
	code here;                  /* retrieved table entry */
	unsigned op;                /* code bits, operation, extra bits, or */
	/*  window position, window bytes to copy */
	unsigned len;               /* match length, unused bytes */
	unsigned dist;              /* match distance */
	unsigned char FAR* from;    /* where to copy match from */

	/* copy state to local variables */
	state = ( struct inflate_state FAR* )strm->state;
	in = strm->next_in - OFF;
	last = in + ( strm->avail_in - 5 );
	out = strm->next_out - OFF;
	beg = out - ( start - strm->avail_out );
	end = out + ( strm->avail_out - 257 );
#ifdef INFLATE_STRICT
	dmax = state->dmax;
#endif
	wsize = state->wsize;
	whave = state->whave;
	wnext = state->wnext;
	window = state->window;
	hold = state->hold;
	bits = state->bits;
	lcode = state->lencode;
	dcode = state->distcode;
	lmask = ( 1U << state->lenbits ) - 1;
	dmask = ( 1U << state->distbits ) - 1;

	/* decode literals and length/distances until end-of-block or not enough
	   input data or output space */
	do
	{
		if ( bits < 15 )
		{
			hold += ( unsigned long )( PUP( in ) ) << bits;
			bits += 8;
			hold += ( unsigned long )( PUP( in ) ) << bits;
			bits += 8;
		}

		here = lcode[hold & lmask];
dolen:
		op = ( unsigned )( here.bits );
		hold >>= op;
		bits -= op;
		op = ( unsigned )( here.op );

		if ( op == 0 )                          /* literal */
		{
			Tracevv( ( stderr, here.val >= 0x20 && here.val < 0x7f ?
			           "inflate:         literal '%c'\n" :
			           "inflate:         literal 0x%02x\n", here.val ) );
			PUP( out ) = ( unsigned char )( here.val );
		}
		else if ( op & 16 )                     /* length base */
		{
			len = ( unsigned )( here.val );
			op &= 15;                           /* number of extra bits */

			if ( op )
			{
				if ( bits < op )
				{
					hold += ( unsigned long )( PUP( in ) ) << bits;
					bits += 8;
				}

				len += ( unsigned )hold & ( ( 1U << op ) - 1 );
				hold >>= op;
				bits -= op;
			}

			Tracevv( ( stderr, "inflate:         length %u\n", len ) );

			if ( bits < 15 )
			{
				hold += ( unsigned long )( PUP( in ) ) << bits;
				bits += 8;
				hold += ( unsigned long )( PUP( in ) ) << bits;
				bits += 8;
			}

			here = dcode[hold & dmask];
dodist:
			op = ( unsigned )( here.bits );
			hold >>= op;
			bits -= op;
			op = ( unsigned )( here.op );

			if ( op & 16 )                      /* distance base */
			{
				dist = ( unsigned )( here.val );
				op &= 15;                       /* number of extra bits */

				if ( bits < op )
				{
					hold += ( unsigned long )( PUP( in ) ) << bits;
					bits += 8;

					if ( bits < op )
					{
						hold += ( unsigned long )( PUP( in ) ) << bits;
						bits += 8;
					}
				}

				dist += ( unsigned )hold & ( ( 1U << op ) - 1 );
#ifdef INFLATE_STRICT

				if ( dist > dmax )
				{
					strm->msg = ( char* )"invalid distance too far back";
					state->mode = BAD;
					break;
				}

#endif
				hold >>= op;
				bits -= op;
				Tracevv( ( stderr, "inflate:         distance %u\n", dist ) );
				op = ( unsigned )( out - beg ); /* max distance in output */

				if ( dist > op )                /* see if copy from window */
				{
					op = dist - op;             /* distance back in window */

					if ( op > whave )
					{
						if ( state->sane )
						{
							strm->msg =
							   ( char* )"invalid distance too far back";
							state->mode = BAD;
							break;
						}

#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR

						if ( len <= op - whave )
						{
							do
							{
								PUP( out ) = 0;
							}
							while ( --len );

							continue;
						}

						len -= op - whave;

						do
						{
							PUP( out ) = 0;
						}
						while ( --op > whave );

						if ( op == 0 )
						{
							from = out - dist;

							do
							{
								PUP( out ) = PUP( from );
							}
							while ( --len );

							continue;
						}

#endif
					}

					from = window - OFF;

					if ( wnext == 0 )           /* very common case */
					{
						from += wsize - op;

						if ( op < len )         /* some from window */
						{
							len -= op;

							do
							{
								PUP( out ) = PUP( from );
							}
							while ( --op );

							from = out - dist;  /* rest from output */
						}
					}
					else if ( wnext < op )      /* wrap around window */
					{
						from += wsize + wnext - op;
						op -= wnext;

						if ( op < len )         /* some from end of window */
						{
							len -= op;

							do
							{
								PUP( out ) = PUP( from );
							}
							while ( --op );

							from = window - OFF;

							if ( wnext < len )  /* some from start of window */
							{
								op = wnext;
								len -= op;

								do
								{
									PUP( out ) = PUP( from );
								}
								while ( --op );

								from = out - dist;      /* rest from output */
							}
						}
					}
					else                        /* contiguous in window */
					{
						from += wnext - op;

						if ( op < len )         /* some from window */
						{
							len -= op;

							do
							{
								PUP( out ) = PUP( from );
							}
							while ( --op );

							from = out - dist;  /* rest from output */
						}
					}

					while ( len > 2 )
					{
						PUP( out ) = PUP( from );
						PUP( out ) = PUP( from );
						PUP( out ) = PUP( from );
						len -= 3;
					}

					if ( len )
					{
						PUP( out ) = PUP( from );

						if ( len > 1 )
						{
							PUP( out ) = PUP( from );
						}
					}
				}
				else
				{
					from = out - dist;          /* copy direct from output */

					do                          /* minimum length is three */
					{
						PUP( out ) = PUP( from );
						PUP( out ) = PUP( from );
						PUP( out ) = PUP( from );
						len -= 3;
					}
					while ( len > 2 );

					if ( len )
					{
						PUP( out ) = PUP( from );

						if ( len > 1 )
						{
							PUP( out ) = PUP( from );
						}
					}
				}
			}
			else if ( ( op & 64 ) == 0 )        /* 2nd level distance code */
			{
				here = dcode[here.val + ( hold & ( ( 1U << op ) - 1 ) )];
				goto dodist;
			}
			else
			{
				strm->msg = ( char* )"invalid distance code";
				state->mode = BAD;
				break;
			}
		}
		else if ( ( op & 64 ) == 0 )            /* 2nd level length code */
		{
			here = lcode[here.val + ( hold & ( ( 1U << op ) - 1 ) )];
			goto dolen;
		}
		else if ( op & 32 )                     /* end-of-block */
		{
			Tracevv( ( stderr, "inflate:         end of block\n" ) );
			state->mode = TYPE;
			break;
		}
		else
		{
			strm->msg = ( char* )"invalid literal/length code";
			state->mode = BAD;
			break;
		}
	}
	while ( in < last && out < end );

	/* return unused bytes (on entry, bits < 8, so in won't go too far back) */
	len = bits >> 3;
	in -= len;
	bits -= len << 3;
	hold &= ( 1U << bits ) - 1;

	/* update state and return */
	strm->next_in = in + OFF;
	strm->next_out = out + OFF;
	strm->avail_in = ( unsigned )( in < last ? 5 + ( last - in ) : 5 - ( in - last ) );
	strm->avail_out = ( unsigned )( out < end ?
	                                257 + ( end - out ) : 257 - ( out - end ) );
	state->hold = hold;
	state->bits = bits;
	return;
}

/*
   inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
   - Using bit fields for code structure
   - Different op definition to avoid & for extra bits (do & for table bits)
   - Three separate decoding do-loops for direct, window, and wnext == 0
   - Special case for distance > 1 copies to do overlapped load and store copy
   - Explicit branch predictions (based on measured branch probabilities)
   - Deferring match copy and interspersed it with decoding subsequent codes
   - Swapping literal/length else
   - Swapping window/direct else
   - Larger unrolled copy loops (three is about right)
   - Moving len -= 3 statement into middle of loop
 */

#endif /* !ASMINF */

/* inftrees.c -- generate Huffman trees for efficient decoding
 * Copyright (C) 1995-2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define MAXBITS 15

const char inflate_copyright[] =
   " inflate 1.2.5 Copyright 1995-2010 Mark Adler ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/*
   Build a set of tables to decode the provided canonical Huffman code.
   The code lengths are lens[0..codes-1].  The result starts at *table,
   whose indices are 0..2^bits-1.  work is a writable array of at least
   lens shorts, which is used as a work area.  type is the type of code
   to be generated, CODES, LENS, or DISTS.  On return, zero is success,
   -1 is an invalid code, and +1 means that ENOUGH isn't enough.  table
   on return points to the next available entry's address.  bits is the
   requested root table index bits, and on return it is the actual root
   table index bits.  It will differ if the request is greater than the
   longest code or if it is less than the shortest code.
 */
int ZLIB_INTERNAL inflate_table( type, lens, codes, table, bits, work )
codetype type;
unsigned short FAR* lens;
unsigned codes;
code FAR* FAR* table;
unsigned FAR* bits;
unsigned short FAR* work;
{
	unsigned len;               /* a code's length in bits */
	unsigned sym;               /* index of code symbols */
	unsigned min, max;          /* minimum and maximum code lengths */
	unsigned root;              /* number of index bits for root table */
	unsigned curr;              /* number of index bits for current table */
	unsigned drop;              /* code bits to drop for sub-table */
	int left;                   /* number of prefix codes available */
	unsigned used;              /* code entries in table used */
	unsigned huff;              /* Huffman code */
	unsigned incr;              /* for incrementing code, index */
	unsigned fill;              /* index for replicating entries */
	unsigned low;               /* low bits for current root entry */
	unsigned mask;              /* mask for low root bits */
	code here;                  /* table entry for duplication */
	code FAR* next;             /* next available space in table */
	const unsigned short FAR* base;     /* base value table to use */
	const unsigned short FAR* extra;    /* extra bits table to use */
	int end;                    /* use base and extra for symbol > end */
	unsigned short count[MAXBITS + 1];  /* number of codes of each length */
	unsigned short offs[MAXBITS + 1];   /* offsets in table for each length */
	static const unsigned short lbase[31] =   /* Length codes 257..285 base */
	{
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
	};
	static const unsigned short lext[31] =   /* Length codes 257..285 extra */
	{
		16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
		19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 73, 195
	};
	static const unsigned short dbase[32] =   /* Distance codes 0..29 base */
	{
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577, 0, 0
	};
	static const unsigned short dext[32] =   /* Distance codes 0..29 extra */
	{
		16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
		23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
		28, 28, 29, 29, 64, 64
	};

	/*
	   Process a set of code lengths to create a canonical Huffman code.  The
	   code lengths are lens[0..codes-1].  Each length corresponds to the
	   symbols 0..codes-1.  The Huffman code is generated by first sorting the
	   symbols by length from short to long, and retaining the symbol order
	   for codes with equal lengths.  Then the code starts with all zero bits
	   for the first code of the shortest length, and the codes are integer
	   increments for the same length, and zeros are appended as the length
	   increases.  For the deflate format, these bits are stored backwards
	   from their more natural integer increment ordering, and so when the
	   decoding tables are built in the large loop below, the integer codes
	   are incremented backwards.

	   This routine assumes, but does not check, that all of the entries in
	   lens[] are in the range 0..MAXBITS.  The caller must assure this.
	   1..MAXBITS is interpreted as that code length.  zero means that that
	   symbol does not occur in this code.

	   The codes are sorted by computing a count of codes for each length,
	   creating from that a table of starting indices for each length in the
	   sorted table, and then entering the symbols in order in the sorted
	   table.  The sorted table is work[], with that space being provided by
	   the caller.

	   The length counts are used for other purposes as well, i.e. finding
	   the minimum and maximum length codes, determining if there are any
	   codes at all, checking for a valid set of lengths, and looking ahead
	   at length counts to determine sub-table sizes when building the
	   decoding tables.
	 */

	/* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
	for ( len = 0; len <= MAXBITS; len++ )
	{
		count[len] = 0;
	}

	for ( sym = 0; sym < codes; sym++ )
	{
		count[lens[sym]]++;
	}

	/* bound code lengths, force root to be within code lengths */
	root = *bits;

	for ( max = MAXBITS; max >= 1; max-- )
		if ( count[max] != 0 ) { break; }

	if ( root > max ) { root = max; }

	if ( max == 0 )                     /* no symbols to code at all */
	{
		here.op = ( unsigned char )64;  /* invalid code marker */
		here.bits = ( unsigned char )1;
		here.val = ( unsigned short )0;
		*( *table )++ = here;           /* make a table to force an error */
		*( *table )++ = here;
		*bits = 1;
		return 0;     /* no symbols, but wait for decoding to report error */
	}

	for ( min = 1; min < max; min++ )
		if ( count[min] != 0 ) { break; }

	if ( root < min ) { root = min; }

	/* check for an over-subscribed or incomplete set of lengths */
	left = 1;

	for ( len = 1; len <= MAXBITS; len++ )
	{
		left <<= 1;
		left -= count[len];

		if ( left < 0 ) { return -1; }      /* over-subscribed */
	}

	if ( left > 0 && ( type == CODES || max != 1 ) )
	{
		return -1;   /* incomplete set */
	}

	/* generate offsets into symbol table for each length for sorting */
	offs[1] = 0;

	for ( len = 1; len < MAXBITS; len++ )
	{
		offs[len + 1] = offs[len] + count[len];
	}

	/* sort symbols by length, by symbol order within each length */
	for ( sym = 0; sym < codes; sym++ )
		if ( lens[sym] != 0 ) { work[offs[lens[sym]]++] = ( unsigned short )sym; }

	/*
	   Create and fill in decoding tables.  In this loop, the table being
	   filled is at next and has curr index bits.  The code being used is huff
	   with length len.  That code is converted to an index by dropping drop
	   bits off of the bottom.  For codes where len is less than drop + curr,
	   those top drop + curr - len bits are incremented through all values to
	   fill the table with replicated entries.

	   root is the number of index bits for the root table.  When len exceeds
	   root, sub-tables are created pointed to by the root entry with an index
	   of the low root bits of huff.  This is saved in low to check for when a
	   new sub-table should be started.  drop is zero when the root table is
	   being filled, and drop is root when sub-tables are being filled.

	   When a new sub-table is needed, it is necessary to look ahead in the
	   code lengths to determine what size sub-table is needed.  The length
	   counts are used for this, and so count[] is decremented as codes are
	   entered in the tables.

	   used keeps track of how many table entries have been allocated from the
	   provided *table space.  It is checked for LENS and DIST tables against
	   the constants ENOUGH_LENS and ENOUGH_DISTS to guard against changes in
	   the initial root table size constants.  See the comments in inftrees.h
	   for more information.

	   sym increments through all symbols, and the loop terminates when
	   all codes of length max, i.e. all codes, have been processed.  This
	   routine permits incomplete codes, so another loop after this one fills
	   in the rest of the decoding tables with invalid code markers.
	 */

	/* set up for code type */
	switch ( type )
	{
		case CODES:
			base = extra = work;    /* dummy value--not used */
			end = 19;
			break;

		case LENS:
			base = lbase;
			base -= 257;
			extra = lext;
			extra -= 257;
			end = 256;
			break;

		default:            /* DISTS */
			base = dbase;
			extra = dext;
			end = -1;
	}

	/* initialize state for loop */
	huff = 0;                   /* starting code */
	sym = 0;                    /* starting code symbol */
	len = min;                  /* starting code length */
	next = *table;              /* current table to fill in */
	curr = root;                /* current table index bits */
	drop = 0;                   /* current bits to drop from code for index */
	low = ( unsigned )( -1 );   /* trigger new sub-table when len > root */
	used = 1U << root;          /* use root table entries */
	mask = used - 1;            /* mask for comparing low */

	/* check available table space */
	if ( ( type == LENS && used >= ENOUGH_LENS ) ||
	     ( type == DISTS && used >= ENOUGH_DISTS ) )
	{
		return 1;
	}

	/* process all codes and make table entries */
	for ( ;; )
	{
		/* create table entry */
		here.bits = ( unsigned char )( len - drop );

		if ( ( int )( work[sym] ) < end )
		{
			here.op = ( unsigned char )0;
			here.val = work[sym];
		}
		else if ( ( int )( work[sym] ) > end )
		{
			here.op = ( unsigned char )( extra[work[sym]] );
			here.val = base[work[sym]];
		}
		else
		{
			here.op = ( unsigned char )( 32 + 64 );     /* end of block */
			here.val = 0;
		}

		/* replicate for those indices with low len bits equal to huff */
		incr = 1U << ( len - drop );
		fill = 1U << curr;
		min = fill;                 /* save offset to next table */

		do
		{
			fill -= incr;
			next[( huff >> drop ) + fill] = here;
		}
		while ( fill != 0 );

		/* backwards increment the len-bit code huff */
		incr = 1U << ( len - 1 );

		while ( huff & incr )
		{
			incr >>= 1;
		}

		if ( incr != 0 )
		{
			huff &= incr - 1;
			huff += incr;
		}
		else
		{
			huff = 0;
		}

		/* go to next symbol, update count, len */
		sym++;

		if ( --( count[len] ) == 0 )
		{
			if ( len == max ) { break; }

			len = lens[work[sym]];
		}

		/* create new sub-table if needed */
		if ( len > root && ( huff & mask ) != low )
		{
			/* if first time, transition to sub-tables */
			if ( drop == 0 )
			{
				drop = root;
			}

			/* increment past last table */
			next += min;            /* here min is 1 << curr */

			/* determine length of next table */
			curr = len - drop;
			left = ( int )( 1 << curr );

			while ( curr + drop < max )
			{
				left -= count[curr + drop];

				if ( left <= 0 ) { break; }

				curr++;
				left <<= 1;
			}

			/* check for enough space */
			used += 1U << curr;

			if ( ( type == LENS && used >= ENOUGH_LENS ) ||
			     ( type == DISTS && used >= ENOUGH_DISTS ) )
			{
				return 1;
			}

			/* point entry in root table to sub-table */
			low = huff & mask;
			( *table )[low].op = ( unsigned char )curr;
			( *table )[low].bits = ( unsigned char )root;
			( *table )[low].val = ( unsigned short )( next - *table );
		}
	}

	/*
	   Fill in rest of table for incomplete codes.  This loop is similar to the
	   loop above in incrementing huff for table indices.  It is assumed that
	   len is equal to curr + drop, so there is no loop needed to increment
	   through high index bits.  When the current sub-table is filled, the loop
	   drops back to the root table to fill in any remaining entries there.
	 */
	here.op = ( unsigned char )64;              /* invalid code marker */
	here.bits = ( unsigned char )( len - drop );
	here.val = ( unsigned short )0;

	while ( huff != 0 )
	{
		/* when done with sub-table, drop back to root table */
		if ( drop != 0 && ( huff & mask ) != low )
		{
			drop = 0;
			len = root;
			next = *table;
			here.bits = ( unsigned char )len;
		}

		/* put invalid code marker in table */
		next[huff >> drop] = here;

		/* backwards increment the len-bit code huff */
		incr = 1U << ( len - 1 );

		while ( huff & incr )
		{
			incr >>= 1;
		}

		if ( incr != 0 )
		{
			huff &= incr - 1;
			huff += incr;
		}
		else
		{
			huff = 0;
		}
	}

	/* set return parameters */
	*table += used;
	*bits = root;
	return 0;
}

#undef GZIP

///////////////// Unuzed gzlib (.gz file read/write functions, tied to stdio)

/* gzguts.h -- zlib internal header definitions for gz* operations
 * Copyright (C) 2004, 2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifdef _LARGEFILE64_SOURCE
#  ifndef _LARGEFILE_SOURCE
#    define _LARGEFILE_SOURCE 1
#  endif
#  ifdef _FILE_OFFSET_BITS
#    undef _FILE_OFFSET_BITS
#  endif
#endif
/*
#if ((__GNUC__-0) * 10 + __GNUC_MINOR__-0 >= 33) && !defined(NO_VIZ)
#  define ZLIB_INTERNAL __attribute__((visibility ("hidden")))
#else
#  define ZLIB_INTERNAL
#endif
*/

#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#  include <limits.h>
#endif
#include <fcntl.h>

#ifdef NO_DEFLATE       /* for compatibility with old definition */
#  define NO_GZCOMPRESS
#endif

#ifdef _MSC_VER
#  include <io.h>
#  define vsnprintf _vsnprintf
#endif

#ifndef local
#  define local static
#endif
/* compile with -Dlocal if your debugger can't find static symbols */

/* gz* functions always use library allocation functions */
#ifndef STDC
extern voidp  malloc OF( ( uInt size ) );
extern void   free   OF( ( voidpf ptr ) );
#endif

/* get errno and strerror definition */
#if defined UNDER_CE
#  include <windows.h>
#  define zstrerror() gz_strwinerror((DWORD)GetLastError())
#else
#  ifdef STDC
#    include <errno.h>
#    define zstrerror() strerror(errno)
#  else
#    define zstrerror() "stdio error (consult errno)"
#  endif
#endif

/* provide prototypes for these when building zlib without LFS */
#if !defined(_LARGEFILE64_SOURCE) || _LFS64_LARGEFILE-0 == 0
ZEXTERN gzFile ZEXPORT gzopen64 OF( ( const char*, const char* ) );
ZEXTERN z_off64_t ZEXPORT gzseek64 OF( ( gzFile, z_off64_t, int ) );
ZEXTERN z_off64_t ZEXPORT gztell64 OF( ( gzFile ) );
ZEXTERN z_off64_t ZEXPORT gzoffset64 OF( ( gzFile ) );
#endif

/* default i/o buffer size -- double this for output when reading */
#define GZBUFSIZE 8192

/* gzip modes, also provide a little integrity check on the passed structure */
#define GZ_NONE 0
#define GZ_READ 7247
#define GZ_WRITE 31153
#define GZ_APPEND 1     /* mode set to GZ_WRITE after the file is opened */

/* values for gz_state how */
#define LOOK 0      /* look for a gzip header */
#define COPY 1      /* copy input directly */
#define GZIP 2      /* decompress a gzip stream */

/* internal gzip file state data structure */
typedef struct
{
	/* used for both reading and writing */
	int mode;               /* see gzip modes above */
	int fd;                 /* file descriptor */
	char* path;             /* path or fd for error messages */
	z_off64_t pos;          /* current position in uncompressed data */
	unsigned size;          /* buffer size, zero if not allocated yet */
	unsigned want;          /* requested buffer size, default is GZBUFSIZE */
	unsigned char* in;      /* input buffer */
	unsigned char* out;     /* output buffer (double-sized when reading) */
	unsigned char* next;    /* next output data to deliver or write */
	/* just for reading */
	unsigned have;          /* amount of output data unused at next */
	int eof;                /* true if end of input file reached */
	z_off64_t start;        /* where the gzip data started, for rewinding */
	z_off64_t raw;          /* where the raw data started, for seeking */
	int how;                /* 0: get header, 1: copy, 2: decompress */
	int direct;             /* true if last read direct, false if gzip */
	/* just for writing */
	int level;              /* compression level */
	int strategy;           /* compression strategy */
	/* seek request */
	z_off64_t skip;         /* amount to skip (already rewound if backwards) */
	int seek;               /* true if seek request pending */
	/* error information */
	int err;                /* error code */
	char* msg;              /* error message */
	/* zlib inflate or deflate stream */
	z_stream strm;          /* stream structure in-place (not a pointer) */
} gz_state;
typedef gz_state FAR* gz_statep;

/* shared functions */
void ZLIB_INTERNAL gz_error OF( ( gz_statep, int, const char* ) );
#if defined UNDER_CE
char ZLIB_INTERNAL* gz_strwinerror OF( ( DWORD error ) );
#endif

/* GT_OFF(x), where x is an unsigned value, is true if x > maximum z_off64_t
   value -- needed when comparing unsigned to z_off64_t, which is signed
   (possible z_off64_t types off_t, off64_t, and long are all signed) */
#ifdef INT_MAX
#  define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > INT_MAX)
#else
unsigned ZLIB_INTERNAL gz_intmax OF( ( void ) );
#  define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > gz_intmax())
#endif


/* gzclose.c -- zlib gzclose() function
 * Copyright (C) 2004, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* gzclose() is in a separate file so that it is linked in only if it is used.
   That way the other gzclose functions can be used instead to avoid linking in
   unneeded compression or decompression routines. */
int ZEXPORT gzclose( file )
gzFile file;
{
#ifndef NO_GZCOMPRESS
	gz_statep state;

	if ( file == NULL )
	{
		return Z_STREAM_ERROR;
	}

	state = ( gz_statep )file;

	return state->mode == GZ_READ ? gzclose_r( file ) : gzclose_w( file );
#else
	return gzclose_r( file );
#endif
}


/* gzlib.c -- zlib functions common to reading and writing gzip files
 * Copyright (C) 2004, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#if defined(_LARGEFILE64_SOURCE) && _LFS64_LARGEFILE-0
#  define LSEEK lseek64
#else
#  define LSEEK lseek
#endif

/* Local functions */
local void gz_reset OF( ( gz_statep ) );
local gzFile gz_open OF( ( const char*, int, const char* ) );

#if defined UNDER_CE

/* Map the Windows error number in ERROR to a locale-dependent error message
   string and return a pointer to it.  Typically, the values for ERROR come
   from GetLastError.

   The string pointed to shall not be modified by the application, but may be
   overwritten by a subsequent call to gz_strwinerror

   The gz_strwinerror function does not change the current setting of
   GetLastError. */
char ZLIB_INTERNAL* gz_strwinerror ( error )
DWORD error;
{
	static char buf[1024];

	wchar_t* msgbuf;
	DWORD lasterr = GetLastError();
	DWORD chars = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM
	                             | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	                             NULL,
	                             error,
	                             0, /* Default language */
	                             ( LPVOID )&msgbuf,
	                             0,
	                             NULL );

	if ( chars != 0 )
	{
		/* If there is an \r\n appended, zap it.  */
		if ( chars >= 2
		     && msgbuf[chars - 2] == '\r' && msgbuf[chars - 1] == '\n' )
		{
			chars -= 2;
			msgbuf[chars] = 0;
		}

		if ( chars > sizeof ( buf ) - 1 )
		{
			chars = sizeof ( buf ) - 1;
			msgbuf[chars] = 0;
		}

		wcstombs( buf, msgbuf, chars + 1 );
		LocalFree( msgbuf );
	}
	else
	{
		sprintf( buf, "unknown win32 error (%ld)", error );
	}

	SetLastError( lasterr );
	return buf;
}

#endif /* UNDER_CE */

/* Reset gzip file state */
local void gz_reset( state )
gz_statep state;
{
	if ( state->mode == GZ_READ )   /* for reading ... */
	{
		state->have = 0;            /* no output data available */
		state->eof = 0;             /* not at end of file */
		state->how = LOOK;          /* look for gzip header */
		state->direct = 1;          /* default for empty file */
	}

	state->seek = 0;                /* no seek request pending */
	gz_error( state, Z_OK, NULL );  /* clear error */
	state->pos = 0;                 /* no uncompressed data yet */
	state->strm.avail_in = 0;       /* no input data yet */
}

/* Open a gzip file either by name or file descriptor. */
local gzFile gz_open( path, fd, mode )
const char* path;
int fd;
const char* mode;
{
	gz_statep state;

	/* allocate gzFile structure to return */
	state = malloc( sizeof( gz_state ) );

	if ( state == NULL )
	{
		return NULL;
	}

	state->size = 0;            /* no buffers allocated yet */
	state->want = GZBUFSIZE;    /* requested buffer size */
	state->msg = NULL;          /* no error message yet */

	/* interpret mode */
	state->mode = GZ_NONE;
	state->level = Z_DEFAULT_COMPRESSION;
	state->strategy = Z_DEFAULT_STRATEGY;

	while ( *mode )
	{
		if ( *mode >= '0' && *mode <= '9' )
		{
			state->level = *mode - '0';
		}
		else
			switch ( *mode )
			{
				case 'r':
					state->mode = GZ_READ;
					break;
#ifndef NO_GZCOMPRESS

				case 'w':
					state->mode = GZ_WRITE;
					break;

				case 'a':
					state->mode = GZ_APPEND;
					break;
#endif

				case '+':       /* can't read and write at the same time */
					free( state );
					return NULL;

				case 'b':       /* ignore -- will request binary anyway */
					break;

				case 'f':
					state->strategy = Z_FILTERED;
					break;

				case 'h':
					state->strategy = Z_HUFFMAN_ONLY;
					break;

				case 'R':
					state->strategy = Z_RLE;
					break;

				case 'F':
					state->strategy = Z_FIXED;

				default:        /* could consider as an error, but just ignore */
					;
			}

		mode++;
	}

	/* must provide an "r", "w", or "a" */
	if ( state->mode == GZ_NONE )
	{
		free( state );
		return NULL;
	}

	/* save the path name for error messages */
	state->path = malloc( strlen( path ) + 1 );

	if ( state->path == NULL )
	{
		free( state );
		return NULL;
	}

	strcpy( state->path, path );

	/* open the file with the appropriate mode (or just use fd) */
	state->fd = fd != -1 ? fd :
	            open( path,
#ifdef O_LARGEFILE
	                  O_LARGEFILE |
#endif
#ifdef O_BINARY
	                  O_BINARY |
#endif
	                  ( state->mode == GZ_READ ?
	                    O_RDONLY :
	                    ( O_WRONLY | O_CREAT | (
	                         state->mode == GZ_WRITE ?
	                         O_TRUNC :
	                         O_APPEND ) ) ),
	                  0666 );

	if ( state->fd == -1 )
	{
		free( state->path );
		free( state );
		return NULL;
	}

	if ( state->mode == GZ_APPEND )
	{
		state->mode = GZ_WRITE;   /* simplify later checks */
	}

	/* save the current position for rewinding (only if reading) */
	if ( state->mode == GZ_READ )
	{
		state->start = LSEEK( state->fd, 0, SEEK_CUR );

		if ( state->start == -1 ) { state->start = 0; }
	}

	/* initialize stream */
	gz_reset( state );

	/* return stream */
	return ( gzFile )state;
}

/* -- see zlib.h -- */
gzFile ZEXPORT gzopen( path, mode )
const char* path;
const char* mode;
{
	return gz_open( path, -1, mode );
}

/* -- see zlib.h -- */
gzFile ZEXPORT gzopen64( path, mode )
const char* path;
const char* mode;
{
	return gz_open( path, -1, mode );
}

/* -- see zlib.h -- */
gzFile ZEXPORT gzdopen( fd, mode )
int fd;
const char* mode;
{
	char* path;         /* identifier for error messages */
	gzFile gz;

	if ( fd == -1 || ( path = malloc( 7 + 3 * sizeof( int ) ) ) == NULL )
	{
		return NULL;
	}

	sprintf( path, "<fd:%d>", fd ); /* for debugging */
	gz = gz_open( path, fd, mode );
	free( path );
	return gz;
}

/* -- see zlib.h -- */
int ZEXPORT gzbuffer( file, size )
gzFile file;
unsigned size;
{
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return -1;
	}

	/* make sure we haven't already allocated memory */
	if ( state->size != 0 )
	{
		return -1;
	}

	/* check and set requested size */
	if ( size == 0 )
	{
		return -1;
	}

	state->want = size;
	return 0;
}

/* -- see zlib.h -- */
int ZEXPORT gzrewind( file )
gzFile file;
{
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	/* check that we're reading and that there's no error */
	if ( state->mode != GZ_READ || state->err != Z_OK )
	{
		return -1;
	}

	/* back up and start over */
	if ( LSEEK( state->fd, state->start, SEEK_SET ) == -1 )
	{
		return -1;
	}

	gz_reset( state );
	return 0;
}

/* -- see zlib.h -- */
z_off64_t ZEXPORT gzseek64( file, offset, whence )
gzFile file;
z_off64_t offset;
int whence;
{
	unsigned n;
	z_off64_t ret;
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return -1;
	}

	/* check that there's no error */
	if ( state->err != Z_OK )
	{
		return -1;
	}

	/* can only seek from start or relative to current position */
	if ( whence != SEEK_SET && whence != SEEK_CUR )
	{
		return -1;
	}

	/* normalize offset to a SEEK_CUR specification */
	if ( whence == SEEK_SET )
	{
		offset -= state->pos;
	}
	else if ( state->seek )
	{
		offset += state->skip;
	}

	state->seek = 0;

	/* if within raw area while reading, just go there */
	if ( state->mode == GZ_READ && state->how == COPY &&
	     state->pos + offset >= state->raw )
	{
		ret = LSEEK( state->fd, offset - state->have, SEEK_CUR );

		if ( ret == -1 )
		{
			return -1;
		}

		state->have = 0;
		state->eof = 0;
		state->seek = 0;
		gz_error( state, Z_OK, NULL );
		state->strm.avail_in = 0;
		state->pos += offset;
		return state->pos;
	}

	/* calculate skip amount, rewinding if needed for back seek when reading */
	if ( offset < 0 )
	{
		if ( state->mode != GZ_READ )       /* writing -- can't go backwards */
		{
			return -1;
		}

		offset += state->pos;

		if ( offset < 0 )                   /* before start of file! */
		{
			return -1;
		}

		if ( gzrewind( file ) == -1 )       /* rewind, then skip to offset */
		{
			return -1;
		}
	}

	/* if reading, skip what's in output buffer (one less gzgetc() check) */
	if ( state->mode == GZ_READ )
	{
		n = GT_OFF( state->have ) || ( z_off64_t )state->have > offset ?
		    ( unsigned )offset : state->have;
		state->have -= n;
		state->next += n;
		state->pos += n;
		offset -= n;
	}

	/* request skip (if not zero) */
	if ( offset )
	{
		state->seek = 1;
		state->skip = offset;
	}

	return state->pos + offset;
}

/* -- see zlib.h -- */
z_off_t ZEXPORT gzseek( file, offset, whence )
gzFile file;
z_off_t offset;
int whence;
{
	z_off64_t ret;

	ret = gzseek64( file, ( z_off64_t )offset, whence );
	return ret == ( z_off_t )ret ? ( z_off_t )ret : -1;
}

/* -- see zlib.h -- */
z_off64_t ZEXPORT gztell64( file )
gzFile file;
{
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return -1;
	}

	/* return position */
	return state->pos + ( state->seek ? state->skip : 0 );
}

/* -- see zlib.h -- */
z_off_t ZEXPORT gztell( file )
gzFile file;
{
	z_off64_t ret;

	ret = gztell64( file );
	return ret == ( z_off_t )ret ? ( z_off_t )ret : -1;
}

/* -- see zlib.h -- */
z_off64_t ZEXPORT gzoffset64( file )
gzFile file;
{
	z_off64_t offset;
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return -1;
	}

	/* compute and return effective offset in file */
	offset = LSEEK( state->fd, 0, SEEK_CUR );

	if ( offset == -1 )
	{
		return -1;
	}

	if ( state->mode == GZ_READ )           /* reading */
	{
		offset -= state->strm.avail_in;   /* don't count buffered input */
	}

	return offset;
}

/* -- see zlib.h -- */
z_off_t ZEXPORT gzoffset( file )
gzFile file;
{
	z_off64_t ret;

	ret = gzoffset64( file );
	return ret == ( z_off_t )ret ? ( z_off_t )ret : -1;
}

/* -- see zlib.h -- */
int ZEXPORT gzeof( file )
gzFile file;
{
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return 0;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return 0;
	}

	/* return end-of-file state */
	return state->mode == GZ_READ ?
	       ( state->eof && state->strm.avail_in == 0 && state->have == 0 ) : 0;
}

/* -- see zlib.h -- */
const char* ZEXPORT gzerror( file, errnum )
gzFile file;
int* errnum;
{
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return NULL;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return NULL;
	}

	/* return error information */
	if ( errnum != NULL )
	{
		*errnum = state->err;
	}

	return state->msg == NULL ? "" : state->msg;
}

/* -- see zlib.h -- */
void ZEXPORT gzclearerr( file )
gzFile file;
{
	gz_statep state;

	/* get internal structure and check integrity */
	if ( file == NULL )
	{
		return;
	}

	state = ( gz_statep )file;

	if ( state->mode != GZ_READ && state->mode != GZ_WRITE )
	{
		return;
	}

	/* clear error and end-of-file */
	if ( state->mode == GZ_READ )
	{
		state->eof = 0;
	}

	gz_error( state, Z_OK, NULL );
}

/* Create an error message in allocated memory and set state->err and
   state->msg accordingly.  Free any previous error message already there.  Do
   not try to free or allocate space if the error is Z_MEM_ERROR (out of
   memory).  Simply save the error message as a static string.  If there is an
   allocation failure constructing the error message, then convert the error to
   out of memory. */
void ZLIB_INTERNAL gz_error( state, err, msg )
gz_statep state;
int err;
const char* msg;
{
	/* free previously allocated message and clear */
	if ( state->msg != NULL )
	{
		if ( state->err != Z_MEM_ERROR )
		{
			free( state->msg );
		}

		state->msg = NULL;
	}

	/* set error code, and if no message, then done */
	state->err = err;

	if ( msg == NULL )
	{
		return;
	}

	/* for an out of memory error, save as static string */
	if ( err == Z_MEM_ERROR )
	{
		state->msg = ( char* )msg;
		return;
	}

	/* construct error message with path */
	if ( ( state->msg = malloc( strlen( state->path ) + strlen( msg ) + 3 ) ) == NULL )
	{
		state->err = Z_MEM_ERROR;
		state->msg = ( char* )"out of memory";
		return;
	}

	strcpy( state->msg, state->path );
	strcat( state->msg, ": " );
	strcat( state->msg, msg );
	return;
}

#ifndef INT_MAX
/* portably return maximum value for an int (when limits.h presumed not
   available) -- we need to do this to cover cases where 2's complement not
   used, since C standard permits 1's complement and sign-bit representations,
   otherwise we could just use ((unsigned)-1) >> 1 */
unsigned ZLIB_INTERNAL gz_intmax()
{
	unsigned p, q;

	p = 1;

	do
	{
		q = p;
		p <<= 1;
		p++;
	}
	while ( p > q );

	return q >> 1;
}
#endif


/* gzread.c -- zlib functions for reading gzip files
 * Copyright (C) 2004, 2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* Local functions */
local int gz_load OF( ( gz_statep, unsigned char*, unsigned, unsigned* ) );
local int gz_avail OF( ( gz_statep ) );
local int gz_next4 OF( ( gz_statep, unsigned long* ) );
local int gz_head OF( ( gz_statep ) );
local int gz_decomp OF( ( gz_statep ) );
local int gz_make OF( ( gz_statep ) );
local int gz_skip OF( ( gz_statep, z_off64_t ) );

/* Use read() to load a buffer -- return -1 on error, otherwise 0.  Read from
   state->fd, and update state->eof, state->err, and state->msg as appropriate.
   This function needs to loop on read(), since read() is not guaranteed to
   read the number of bytes requested, depending on the type of descriptor. */
local int gz_load( state, buf, len, have )
gz_statep state;
unsigned char* buf;
unsigned len;
unsigned* have;
{
	int ret;

	*have = 0;

	do
	{
		ret = read( state->fd, buf + *have, len - *have );

		if ( ret <= 0 )
		{
			break;
		}

		*have += ret;
	}
	while ( *have < len );

	if ( ret < 0 )
	{
		gz_error( state, Z_ERRNO, zstrerror() );
		return -1;
	}

	if ( ret == 0 )
	{
		state->eof = 1;
	}

	return 0;
}

/* Load up input buffer and set eof flag if last data loaded -- return -1 on
   error, 0 otherwise.  Note that the eof flag is set when the end of the input
   file is reached, even though there may be unused data in the buffer.  Once
   that data has been used, no more attempts will be made to read the file.
   gz_avail() assumes that strm->avail_in == 0. */
local int gz_avail( state )
gz_statep state;
{
	z_streamp strm = &( state->strm );

	if ( state->err != Z_OK )
	{
		return -1;
	}

	if ( state->eof == 0 )
	{
		if ( gz_load( state, state->in, state->size,
		              ( unsigned* ) & ( strm->avail_in ) ) == -1 )
		{
			return -1;
		}

		strm->next_in = state->in;
	}

	return 0;
}

/* Get next byte from input, or -1 if end or error. */
#define NEXT() ((strm->avail_in == 0 && gz_avail(state) == -1) ? -1 : \
                (strm->avail_in == 0 ? -1 : \
                 (strm->avail_in--, *(strm->next_in)++)))

/* Get a four-byte little-endian integer and return 0 on success and the value
   in *ret.  Otherwise -1 is returned and *ret is not modified. */
local int gz_next4( state, ret )
gz_statep state;
unsigned long* ret;
{
	int ch;
	unsigned long val;
	z_streamp strm = &( state->strm );

	val = NEXT();
	val += ( unsigned )NEXT() << 8;
	val += ( unsigned long )NEXT() << 16;
	ch = NEXT();

	if ( ch == -1 )
	{
		return -1;
	}

	val += ( unsigned long )ch << 24;
	*ret = val;
	return 0;
}

/* Look for gzip header, set up for inflate or copy.  state->have must be zero.
   If this is the first time in, allocate required memory.  state->how will be
   left unchanged if there is no more input data available, will be set to COPY
   if there is no gzip header and direct copying will be performed, or it will
   be set to GZIP for decompression, and the gzip header will be skipped so
   that the next available input data is the raw deflate stream.  If direct
   copying, then leftover input data from the input buffer will be copied to
   the output buffer.  In that case, all further file reads will be directly to
   either the output buffer or a user buffer.  If decompressing, the inflate
   state and the check value will be initialized.  gz_head() will return 0 on
   success or -1 on failure.  Failures may include read errors or gzip header
   errors.  */
local int gz_head( state )
gz_statep state;
{
	z_streamp strm = &( state->strm );
	int flags;
	unsigned len;

	/* allocate read buffers and inflate memory */
	if ( state->size == 0 )
	{
		/* allocate buffers */
		state->in = malloc( state->want );
		state->out = malloc( state->want << 1 );

		if ( state->in == NULL || state->out == NULL )
		{
			if ( state->out != NULL )
			{
				free( state->out );
			}

			if ( state->in != NULL )
			{
				free( state->in );
			}

			gz_error( state, Z_MEM_ERROR, "out of memory" );
			return -1;
		}

		state->size = state->want;

		/* allocate inflate memory */
		state->strm.zalloc = Z_NULL;
		state->strm.zfree = Z_NULL;
		state->strm.opaque = Z_NULL;
		state->strm.avail_in = 0;
		state->strm.next_in = Z_NULL;

		if ( inflateInit2( &( state->strm ), -15 ) != Z_OK ) /* raw inflate */
		{
			free( state->out );
			free( state->in );
			state->size = 0;
			gz_error( state, Z_MEM_ERROR, "out of memory" );
			return -1;
		}
	}

	/* get some data in the input buffer */
	if ( strm->avail_in == 0 )
	{
		if ( gz_avail( state ) == -1 )
		{
			return -1;
		}

		if ( strm->avail_in == 0 )
		{
			return 0;
		}
	}

	/* look for the gzip magic header bytes 31 and 139 */
	if ( strm->next_in[0] == 31 )
	{
		strm->avail_in--;
		strm->next_in++;

		if ( strm->avail_in == 0 && gz_avail( state ) == -1 )
		{
			return -1;
		}

		if ( strm->avail_in && strm->next_in[0] == 139 )
		{
			/* we have a gzip header, woo hoo! */
			strm->avail_in--;
			strm->next_in++;

			/* skip rest of header */
			if ( NEXT() != 8 )      /* compression method */
			{
				gz_error( state, Z_DATA_ERROR, "unknown compression method" );
				return -1;
			}

			flags = NEXT();

			if ( flags & 0xe0 )     /* reserved flag bits */
			{
				gz_error( state, Z_DATA_ERROR, "unknown header flags set" );
				return -1;
			}

			NEXT();                 /* modification time */
			NEXT();
			NEXT();
			NEXT();
			NEXT();                 /* extra flags */
			NEXT();                 /* operating system */

			if ( flags & 4 )        /* extra field */
			{
				len = ( unsigned )NEXT();
				len += ( unsigned )NEXT() << 8;

				while ( len-- )
					if ( NEXT() < 0 )
					{
						break;
					}
			}

			if ( flags & 8 )        /* file name */
				while ( NEXT() > 0 )
					;

			if ( flags & 16 )       /* comment */
				while ( NEXT() > 0 )
					;

			if ( flags & 2 )        /* header crc */
			{
				NEXT();
				NEXT();
			}

			/* an unexpected end of file is not checked for here -- it will be
			   noticed on the first request for uncompressed data */

			/* set up for decompression */
			inflateReset( strm );
			strm->adler = crc32( 0L, Z_NULL, 0 );
			state->how = GZIP;
			state->direct = 0;
			return 0;
		}
		else
		{
			/* not a gzip file -- save first byte (31) and fall to raw i/o */
			state->out[0] = 31;
			state->have = 1;
		}
	}

	/* doing raw i/o, save start of raw data for seeking, copy any leftover
	   input to output -- this assumes that the output buffer is larger than
	   the input buffer, which also assures space for gzungetc() */
	state->raw = state->pos;
	state->next = state->out;

	if ( strm->avail_in )
	{
		memcpy( state->next + state->have, strm->next_in, strm->avail_in );
		state->have += strm->avail_in;
		strm->avail_in = 0;
	}

	state->how = COPY;
	state->direct = 1;
	return 0;
}

/* Decompress from input to the provided next_out and avail_out in the state.
   If the end of the compressed data is reached, then verify the gzip trailer
   check value and length (modulo 2^32).  state->have and state->next are set
   to point to the just decompressed data, and the crc is updated.  If the
   trailer is verified, state->how is reset to LOOK to look for the next gzip
   stream or raw data, once state->have is depleted.  Returns 0 on success, -1
   on failure.  Failures may include invalid compressed data or a failed gzip
   trailer verification. */
local int gz_decomp( state )
gz_statep state;
{
	int ret;
	unsigned had;
	unsigned long crc, len;
	z_streamp strm = &( state->strm );

	/* fill output buffer up to end of deflate stream */
	had = strm->avail_out;

	do
	{
		/* get more input for inflate() */
		if ( strm->avail_in == 0 && gz_avail( state ) == -1 )
		{
			return -1;
		}

		if ( strm->avail_in == 0 )
		{
			gz_error( state, Z_DATA_ERROR, "unexpected end of file" );
			return -1;
		}

		/* decompress and handle errors */
		ret = inflate( strm, Z_NO_FLUSH );

		if ( ret == Z_STREAM_ERROR || ret == Z_NEED_DICT )
		{
			gz_error( state, Z_STREAM_ERROR,
			          "internal error: inflate stream corrupt" );
			return -1;
		}

		if ( ret == Z_MEM_ERROR )
		{
			gz_error( state, Z_MEM_ERROR, "out of memory" );
			return -1;
		}

		if ( ret == Z_DATA_ERROR )              /* deflate stream invalid */
		{
			gz_error( state, Z_DATA_ERROR,
			          strm->msg == NULL ? "compressed data error" : strm->msg );
			return -1;
		}
	}
	while ( strm->avail_out && ret != Z_STREAM_END );

	/* update available output and crc check value */
	state->have = had - strm->avail_out;
	state->next = strm->next_out - state->have;
	strm->adler = crc32( strm->adler, state->next, state->have );

	/* check gzip trailer if at end of deflate stream */
	if ( ret == Z_STREAM_END )
	{
		if ( gz_next4( state, &crc ) == -1 || gz_next4( state, &len ) == -1 )
		{
			gz_error( state, Z_DATA_ERROR, "unexpected end of file" );
			return -1;
		}

		if ( crc != strm->adler )
		{
			gz_error( state, Z_DATA_ERROR, "incorrect data check" );
			return -1;
		}

		if ( len != ( strm->total_out & 0xffffffffL ) )
		{
			gz_error( state, Z_DATA_ERROR, "incorrect length check" );
			return -1;
		}

		state->how = LOOK;      /* ready for next stream, once have is 0 (leave
                                   state->direct unchanged to remember how) */
	}

	/* good decompression */
	return 0;
}

/* Make data and put in the output buffer.  Assumes that state->have == 0.
   Data is either copied from the input file or decompressed from the input
   file depending on state->how.  If state->how is LOOK, then a gzip header is
   looked for (and skipped if found) to determine wither to copy or decompress.
   Returns -1 on error, otherwise 0.  gz_make() will leave state->have as COPY
   or GZIP unless the end of the input file has been reached and all data has
   been processed.  */
local int gz_make( state )
gz_statep state;
{
	z_streamp strm = &( state->strm );

	if ( state->how == LOOK )           /* look for gzip header */
	{
		if ( gz_head( state ) == -1 )
		{
			return -1;
		}

		if ( state->have )              /* got some data from gz_head() */
		{
			return 0;
		}
	}

	if ( state->how == COPY )           /* straight copy */
	{
		if ( gz_load( state, state->out, state->size << 1, &( state->have ) ) == -1 )
		{
			return -1;
		}

		state->next = state->out;
	}
	else if ( state->how == GZIP )      /* decompress */
	{
		strm->avail_out = state->size << 1;
		strm->next_out = state->out;

		if ( gz_decomp( state ) == -1 )
		{
			return -1;
		}
	}

	return 0;
}

/* Skip len uncompressed bytes of output.  Return -1 on error, 0 on success. */
local int gz_skip( state, len )
gz_statep state;
z_off64_t len;
{
	unsigned n;

	/* skip over len bytes or reach end-of-file, whichever comes first */
	while ( len )

		/* skip over whatever is in output buffer */
		if ( state->have )
		{
			n = GT_OFF( state->have ) || ( z_off64_t )state->have > len ?
			    ( unsigned )len : state->have;
			state->have -= n;
			state->next += n;
			state->pos += n;
			len -= n;
		}

	/* output buffer empty -- return if we're at the end of the input */
		else if ( state->eof && state->strm.avail_in == 0 )
		{
			break;
		}

	/* need more data to skip -- load up output buffer */
		else
		{
			/* get more output, looking for header if required */
			if ( gz_make( state ) == -1 )
			{
				return -1;
			}
		}

	return 0;
}

/* -- see zlib.h -- */
int ZEXPORT gzread( file, buf, len )
gzFile file;
voidp buf;
unsigned len;
{
	unsigned got, n;
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're reading and that there's no error */
	if ( state->mode != GZ_READ || state->err != Z_OK )
	{
		return -1;
	}

	/* since an int is returned, make sure len fits in one, otherwise return
	   with an error (this avoids the flaw in the interface) */
	if ( ( int )len < 0 )
	{
		gz_error( state, Z_BUF_ERROR, "requested length does not fit in int" );
		return -1;
	}

	/* if len is zero, avoid unnecessary operations */
	if ( len == 0 )
	{
		return 0;
	}

	/* process a skip request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_skip( state, state->skip ) == -1 )
		{
			return -1;
		}
	}

	/* get len bytes to buf, or less than len if at the end */
	got = 0;

	do
	{
		/* first just try copying data from the output buffer */
		if ( state->have )
		{
			n = state->have > len ? len : state->have;
			memcpy( buf, state->next, n );
			state->next += n;
			state->have -= n;
		}

		/* output buffer empty -- return if we're at the end of the input */
		else if ( state->eof && strm->avail_in == 0 )
		{
			break;
		}

		/* need output data -- for small len or new stream load up our output
		   buffer */
		else if ( state->how == LOOK || len < ( state->size << 1 ) )
		{
			/* get more output, looking for header if required */
			if ( gz_make( state ) == -1 )
			{
				return -1;
			}

			continue;       /* no progress yet -- go back to memcpy() above */
			/* the copy above assures that we will leave with space in the
			   output buffer, allowing at least one gzungetc() to succeed */
		}

		/* large len -- read directly into user buffer */
		else if ( state->how == COPY )      /* read directly */
		{
			if ( gz_load( state, buf, len, &n ) == -1 )
			{
				return -1;
			}
		}

		/* large len -- decompress directly into user buffer */
		else    /* state->how == GZIP */
		{
			strm->avail_out = len;
			strm->next_out = buf;

			if ( gz_decomp( state ) == -1 )
			{
				return -1;
			}

			n = state->have;
			state->have = 0;
		}

		/* update progress */
		len -= n;
		buf = ( char* )buf + n;
		got += n;
		state->pos += n;
	}
	while ( len );

	/* return number of bytes read into user buffer (will fit in int) */
	return ( int )got;
}

/* -- see zlib.h -- */
int ZEXPORT gzgetc( file )
gzFile file;
{
	int ret;
	unsigned char buf[1];
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	/* check that we're reading and that there's no error */
	if ( state->mode != GZ_READ || state->err != Z_OK )
	{
		return -1;
	}

	/* try output buffer (no need to check for skip request) */
	if ( state->have )
	{
		state->have--;
		state->pos++;
		return *( state->next )++;
	}

	/* nothing there -- try gzread() */
	ret = gzread( file, buf, 1 );
	return ret < 1 ? -1 : buf[0];
}

/* -- see zlib.h -- */
int ZEXPORT gzungetc( c, file )
int c;
gzFile file;
{
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	/* check that we're reading and that there's no error */
	if ( state->mode != GZ_READ || state->err != Z_OK )
	{
		return -1;
	}

	/* process a skip request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_skip( state, state->skip ) == -1 )
		{
			return -1;
		}
	}

	/* can't push EOF */
	if ( c < 0 )
	{
		return -1;
	}

	/* if output buffer empty, put byte at end (allows more pushing) */
	if ( state->have == 0 )
	{
		state->have = 1;
		state->next = state->out + ( state->size << 1 ) - 1;
		state->next[0] = c;
		state->pos--;
		return c;
	}

	/* if no room, give up (must have already done a gzungetc()) */
	if ( state->have == ( state->size << 1 ) )
	{
		gz_error( state, Z_BUF_ERROR, "out of room to push characters" );
		return -1;
	}

	/* slide output data if needed and insert byte before existing data */
	if ( state->next == state->out )
	{
		unsigned char* src = state->out + state->have;
		unsigned char* dest = state->out + ( state->size << 1 );

		while ( src > state->out )
		{
			*--dest = *--src;
		}

		state->next = dest;
	}

	state->have++;
	state->next--;
	state->next[0] = c;
	state->pos--;
	return c;
}

/* -- see zlib.h -- */
char* ZEXPORT gzgets( file, buf, len )
gzFile file;
char* buf;
int len;
{
	unsigned left, n;
	char* str;
	unsigned char* eol;
	gz_statep state;

	/* check parameters and get internal structure */
	if ( file == NULL || buf == NULL || len < 1 )
	{
		return NULL;
	}

	state = ( gz_statep )file;

	/* check that we're reading and that there's no error */
	if ( state->mode != GZ_READ || state->err != Z_OK )
	{
		return NULL;
	}

	/* process a skip request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_skip( state, state->skip ) == -1 )
		{
			return NULL;
		}
	}

	/* copy output bytes up to new line or len - 1, whichever comes first --
	   append a terminating zero to the string (we don't check for a zero in
	   the contents, let the user worry about that) */
	str = buf;
	left = ( unsigned )len - 1;

	if ( left ) do
		{
			/* assure that something is in the output buffer */
			if ( state->have == 0 )
			{
				if ( gz_make( state ) == -1 )
				{
					return NULL;   /* error */
				}

				if ( state->have == 0 )     /* end of file */
				{
					if ( buf == str )       /* got bupkus */
					{
						return NULL;
					}

					break;                  /* got something -- return it */
				}
			}

			/* look for end-of-line in current output buffer */
			n = state->have > left ? left : state->have;
			eol = memchr( state->next, '\n', n );

			if ( eol != NULL )
			{
				n = ( unsigned )( eol - state->next ) + 1;
			}

			/* copy through end-of-line, or remainder if not found */
			memcpy( buf, state->next, n );
			state->have -= n;
			state->next += n;
			state->pos += n;
			left -= n;
			buf += n;
		}
		while ( left && eol == NULL );

	/* found end-of-line or out of space -- terminate string and return it */
	buf[0] = 0;
	return str;
}

/* -- see zlib.h -- */
int ZEXPORT gzdirect( file )
gzFile file;
{
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return 0;
	}

	state = ( gz_statep )file;

	/* check that we're reading */
	if ( state->mode != GZ_READ )
	{
		return 0;
	}

	/* if the state is not known, but we can find out, then do so (this is
	   mainly for right after a gzopen() or gzdopen()) */
	if ( state->how == LOOK && state->have == 0 )
	{
		( void )gz_head( state );
	}

	/* return 1 if reading direct, 0 if decompressing a gzip stream */
	return state->direct;
}

/* -- see zlib.h -- */
int ZEXPORT gzclose_r( file )
gzFile file;
{
	int ret;
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return Z_STREAM_ERROR;
	}

	state = ( gz_statep )file;

	/* check that we're reading */
	if ( state->mode != GZ_READ )
	{
		return Z_STREAM_ERROR;
	}

	/* free memory and close file */
	if ( state->size )
	{
		inflateEnd( &( state->strm ) );
		free( state->out );
		free( state->in );
	}

	gz_error( state, Z_OK, NULL );
	free( state->path );
	ret = close( state->fd );
	free( state );
	return ret ? Z_ERRNO : Z_OK;
}

/* gzwrite.c -- zlib functions for writing gzip files
 * Copyright (C) 2004, 2005, 2010 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* Local functions */
local int gz_init OF( ( gz_statep ) );
local int gz_comp OF( ( gz_statep, int ) );
local int gz_zero OF( ( gz_statep, z_off64_t ) );

/* Initialize state for writing a gzip file.  Mark initialization by setting
   state->size to non-zero.  Return -1 on failure or 0 on success. */
local int gz_init( state )
gz_statep state;
{
	int ret;
	z_streamp strm = &( state->strm );

	/* allocate input and output buffers */
	state->in = malloc( state->want );
	state->out = malloc( state->want );

	if ( state->in == NULL || state->out == NULL )
	{
		if ( state->out != NULL )
		{
			free( state->out );
		}

		if ( state->in != NULL )
		{
			free( state->in );
		}

		gz_error( state, Z_MEM_ERROR, "out of memory" );
		return -1;
	}

	/* allocate deflate memory, set up for gzip compression */
	strm->zalloc = Z_NULL;
	strm->zfree = Z_NULL;
	strm->opaque = Z_NULL;
	ret = deflateInit2( strm, state->level, Z_DEFLATED,
	                    15 + 16, 8, state->strategy );

	if ( ret != Z_OK )
	{
		free( state->in );
		gz_error( state, Z_MEM_ERROR, "out of memory" );
		return -1;
	}

	/* mark state as initialized */
	state->size = state->want;

	/* initialize write buffer */
	strm->avail_out = state->size;
	strm->next_out = state->out;
	state->next = strm->next_out;
	return 0;
}

/* Compress whatever is at avail_in and next_in and write to the output file.
   Return -1 if there is an error writing to the output file, otherwise 0.
   flush is assumed to be a valid deflate() flush value.  If flush is Z_FINISH,
   then the deflate() state is reset to start a new gzip stream. */
local int gz_comp( state, flush )
gz_statep state;
int flush;
{
	int ret, got;
	unsigned have;
	z_streamp strm = &( state->strm );

	/* allocate memory if this is the first time through */
	if ( state->size == 0 && gz_init( state ) == -1 )
	{
		return -1;
	}

	/* run deflate() on provided input until it produces no more output */
	ret = Z_OK;

	do
	{
		/* write out current buffer contents if full, or if flushing, but if
		   doing Z_FINISH then don't write until we get to Z_STREAM_END */
		if ( strm->avail_out == 0 || ( flush != Z_NO_FLUSH &&
		                               ( flush != Z_FINISH || ret == Z_STREAM_END ) ) )
		{
			have = ( unsigned )( strm->next_out - state->next );

			if ( have && ( ( got = write( state->fd, state->next, have ) ) < 0 ||
			               ( unsigned )got != have ) )
			{
				gz_error( state, Z_ERRNO, zstrerror() );
				return -1;
			}

			if ( strm->avail_out == 0 )
			{
				strm->avail_out = state->size;
				strm->next_out = state->out;
			}

			state->next = strm->next_out;
		}

		/* compress */
		have = strm->avail_out;
		ret = deflate( strm, flush );

		if ( ret == Z_STREAM_ERROR )
		{
			gz_error( state, Z_STREAM_ERROR,
			          "internal error: deflate stream corrupt" );
			return -1;
		}

		have -= strm->avail_out;
	}
	while ( have );

	/* if that completed a deflate stream, allow another to start */
	if ( flush == Z_FINISH )
	{
		deflateReset( strm );
	}

	/* all done, no errors */
	return 0;
}

/* Compress len zeros to output.  Return -1 on error, 0 on success. */
local int gz_zero( state, len )
gz_statep state;
z_off64_t len;
{
	int first;
	unsigned n;
	z_streamp strm = &( state->strm );

	/* consume whatever's left in the input buffer */
	if ( strm->avail_in && gz_comp( state, Z_NO_FLUSH ) == -1 )
	{
		return -1;
	}

	/* compress len zeros (len guaranteed > 0) */
	first = 1;

	while ( len )
	{
		n = GT_OFF( state->size ) || ( z_off64_t )state->size > len ?
		    ( unsigned )len : state->size;

		if ( first )
		{
			memset( state->in, 0, n );
			first = 0;
		}

		strm->avail_in = n;
		strm->next_in = state->in;
		state->pos += n;

		if ( gz_comp( state, Z_NO_FLUSH ) == -1 )
		{
			return -1;
		}

		len -= n;
	}

	return 0;
}

/* -- see zlib.h -- */
int ZEXPORT gzwrite( file, buf, len )
gzFile file;
voidpc buf;
unsigned len;
{
	unsigned put = len;
	unsigned n;
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if ( file == NULL )
	{
		return 0;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return 0;
	}

	/* since an int is returned, make sure len fits in one, otherwise return
	   with an error (this avoids the flaw in the interface) */
	if ( ( int )len < 0 )
	{
		gz_error( state, Z_BUF_ERROR, "requested length does not fit in int" );
		return 0;
	}

	/* if len is zero, avoid unnecessary operations */
	if ( len == 0 )
	{
		return 0;
	}

	/* allocate memory if this is the first time through */
	if ( state->size == 0 && gz_init( state ) == -1 )
	{
		return 0;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return 0;
		}
	}

	/* for small len, copy to input buffer, otherwise compress directly */
	if ( len < state->size )
	{
		/* copy to input buffer, compress when full */
		do
		{
			if ( strm->avail_in == 0 )
			{
				strm->next_in = state->in;
			}

			n = state->size - strm->avail_in;

			if ( n > len )
			{
				n = len;
			}

			memcpy( strm->next_in + strm->avail_in, buf, n );
			strm->avail_in += n;
			state->pos += n;
			buf = ( char* )buf + n;
			len -= n;

			if ( len && gz_comp( state, Z_NO_FLUSH ) == -1 )
			{
				return 0;
			}
		}
		while ( len );
	}
	else
	{
		/* consume whatever's left in the input buffer */
		if ( strm->avail_in && gz_comp( state, Z_NO_FLUSH ) == -1 )
		{
			return 0;
		}

		/* directly compress user buffer to file */
		strm->avail_in = len;
		strm->next_in = ( voidp )buf;
		state->pos += len;

		if ( gz_comp( state, Z_NO_FLUSH ) == -1 )
		{
			return 0;
		}
	}

	/* input was all buffered or compressed (put will fit in int) */
	return ( int )put;
}

/* -- see zlib.h -- */
int ZEXPORT gzputc( file, c )
gzFile file;
int c;
{
	unsigned char buf[1];
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return -1;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return -1;
		}
	}

	/* try writing to input buffer for speed (state->size == 0 if buffer not
	   initialized) */
	if ( strm->avail_in < state->size )
	{
		if ( strm->avail_in == 0 )
		{
			strm->next_in = state->in;
		}

		strm->next_in[strm->avail_in++] = c;
		state->pos++;
		return c;
	}

	/* no room in buffer or not initialized, use gz_write() */
	buf[0] = c;

	if ( gzwrite( file, buf, 1 ) != 1 )
	{
		return -1;
	}

	return c;
}

/* -- see zlib.h -- */
int ZEXPORT gzputs( file, str )
gzFile file;
const char* str;
{
	int ret;
	unsigned len;

	/* write string */
	len = ( unsigned )strlen( str );
	ret = gzwrite( file, str, len );
	return ret == 0 && len != 0 ? -1 : ret;
}

#ifdef STDC
#include <stdarg.h>

/* -- see zlib.h -- */
int ZEXPORTVA gzprintf ( gzFile file, const char* format, ... )
{
	int size, len;
	gz_statep state;
	z_streamp strm;
	va_list va;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return 0;
	}

	/* make sure we have some buffer space */
	if ( state->size == 0 && gz_init( state ) == -1 )
	{
		return 0;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return 0;
		}
	}

	/* consume whatever's left in the input buffer */
	if ( strm->avail_in && gz_comp( state, Z_NO_FLUSH ) == -1 )
	{
		return 0;
	}

	/* do the printf() into the input buffer, put length in len */
	size = ( int )( state->size );
	state->in[size - 1] = 0;
	va_start( va, format );
#ifdef NO_vsnprintf
#  ifdef HAS_vsprintf_void
	( void )vsprintf( state->in, format, va );
	va_end( va );

	for ( len = 0; len < size; len++ )
		if ( state->in[len] == 0 ) { break; }

#  else
	len = vsprintf( state->in, format, va );
	va_end( va );
#  endif
#else
#  ifdef HAS_vsnprintf_void
	( void )vsnprintf( state->in, size, format, va );
	va_end( va );
	len = strlen( state->in );
#  else
	len = vsnprintf( ( char* )( state->in ), size, format, va );
	va_end( va );
#  endif
#endif

	/* check that printf() results fit in buffer */
	if ( len <= 0 || len >= ( int )size || state->in[size - 1] != 0 )
	{
		return 0;
	}

	/* update buffer and position, defer compression until needed */
	strm->avail_in = ( unsigned )len;
	strm->next_in = state->in;
	state->pos += len;
	return len;
}

#else /* !STDC */

/* -- see zlib.h -- */
int ZEXPORTVA gzprintf ( file, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
                         a11, a12, a13, a14, a15, a16, a17, a18, a19, a20 )
gzFile file;
const char* format;
int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
    a11, a12, a13, a14, a15, a16, a17, a18, a19, a20;
{
	int size, len;
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return 0;
	}

	/* make sure we have some buffer space */
	if ( state->size == 0 && gz_init( state ) == -1 )
	{
		return 0;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return 0;
		}
	}

	/* consume whatever's left in the input buffer */
	if ( strm->avail_in && gz_comp( state, Z_NO_FLUSH ) == -1 )
	{
		return 0;
	}

	/* do the printf() into the input buffer, put length in len */
	size = ( int )( state->size );
	state->in[size - 1] = 0;
#ifdef NO_snprintf
#  ifdef HAS_sprintf_void
	sprintf( state->in, format, a1, a2, a3, a4, a5, a6, a7, a8,
	         a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20 );

	for ( len = 0; len < size; len++ )
		if ( state->in[len] == 0 ) { break; }

#  else
	len = sprintf( state->in, format, a1, a2, a3, a4, a5, a6, a7, a8,
	               a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20 );
#  endif
#else
#  ifdef HAS_snprintf_void
	snprintf( state->in, size, format, a1, a2, a3, a4, a5, a6, a7, a8,
	          a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20 );
	len = strlen( state->in );
#  else
	len = snprintf( state->in, size, format, a1, a2, a3, a4, a5, a6, a7, a8,
	                a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20 );
#  endif
#endif

	/* check that printf() results fit in buffer */
	if ( len <= 0 || len >= ( int )size || state->in[size - 1] != 0 )
	{
		return 0;
	}

	/* update buffer and position, defer compression until needed */
	strm->avail_in = ( unsigned )len;
	strm->next_in = state->in;
	state->pos += len;
	return len;
}

#endif

/* -- see zlib.h -- */
int ZEXPORT gzflush( file, flush )
gzFile file;
int flush;
{
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return -1;
	}

	state = ( gz_statep )file;

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return Z_STREAM_ERROR;
	}

	/* check flush parameter */
	if ( flush < 0 || flush > Z_FINISH )
	{
		return Z_STREAM_ERROR;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return -1;
		}
	}

	/* compress remaining data with requested flush */
	gz_comp( state, flush );
	return state->err;
}

/* -- see zlib.h -- */
int ZEXPORT gzsetparams( file, level, strategy )
gzFile file;
int level;
int strategy;
{
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if ( file == NULL )
	{
		return Z_STREAM_ERROR;
	}

	state = ( gz_statep )file;
	strm = &( state->strm );

	/* check that we're writing and that there's no error */
	if ( state->mode != GZ_WRITE || state->err != Z_OK )
	{
		return Z_STREAM_ERROR;
	}

	/* if no change is requested, then do nothing */
	if ( level == state->level && strategy == state->strategy )
	{
		return Z_OK;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;

		if ( gz_zero( state, state->skip ) == -1 )
		{
			return -1;
		}
	}

	/* change compression parameters for subsequent input */
	if ( state->size )
	{
		/* flush previous input with previous parameters before changing */
		if ( strm->avail_in && gz_comp( state, Z_PARTIAL_FLUSH ) == -1 )
		{
			return state->err;
		}

		deflateParams( strm, level, strategy );
	}

	state->level = level;
	state->strategy = strategy;
	return Z_OK;
}

/* -- see zlib.h -- */
int ZEXPORT gzclose_w( file )
gzFile file;
{
	int ret = 0;
	gz_statep state;

	/* get internal structure */
	if ( file == NULL )
	{
		return Z_STREAM_ERROR;
	}

	state = ( gz_statep )file;

	/* check that we're writing */
	if ( state->mode != GZ_WRITE )
	{
		return Z_STREAM_ERROR;
	}

	/* check for seek request */
	if ( state->seek )
	{
		state->seek = 0;
		ret += gz_zero( state, state->skip );
	}

	/* flush, free memory, and close file */
	ret += gz_comp( state, Z_FINISH );
	( void )deflateEnd( &( state->strm ) );
	free( state->out );
	free( state->in );
	gz_error( state, Z_OK, NULL );
	free( state->path );
	ret += close( state->fd );
	free( state );
	return ret ? Z_ERRNO : Z_OK;
}

//////// compress/decompress memory buffers
/* compress.c -- compress a memory buffer
 * Copyright (C) 1995-2005 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

//#define ZLIB_INTERNAL
//#include "zlib.h"

/* ===========================================================================
     Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/
int ZEXPORT compress2 ( dest, destLen, source, sourceLen, level )
Bytef* dest;
uLongf* destLen;
const Bytef* source;
uLong sourceLen;
int level;
{
	z_stream stream;
	int err;

	stream.next_in = ( Bytef* )source;
	stream.avail_in = ( uInt )sourceLen;
#ifdef MAXSEG_64K

	/* Check for source > 64K on 16-bit machine: */
	if ( ( uLong )stream.avail_in != sourceLen ) { return Z_BUF_ERROR; }

#endif
	stream.next_out = dest;
	stream.avail_out = ( uInt ) * destLen;

	if ( ( uLong )stream.avail_out != *destLen ) { return Z_BUF_ERROR; }

	stream.zalloc = ( alloc_func )0;
	stream.zfree = ( free_func )0;
	stream.opaque = ( voidpf )0;

	err = deflateInit( &stream, level );

	if ( err != Z_OK ) { return err; }

	err = deflate( &stream, Z_FINISH );

	if ( err != Z_STREAM_END )
	{
		deflateEnd( &stream );
		return err == Z_OK ? Z_BUF_ERROR : err;
	}

	*destLen = stream.total_out;

	err = deflateEnd( &stream );
	return err;
}

/* ===========================================================================
 */
int ZEXPORT compress ( dest, destLen, source, sourceLen )
Bytef* dest;
uLongf* destLen;
const Bytef* source;
uLong sourceLen;
{
	return compress2( dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION );
}

/* ===========================================================================
     If the default memLevel or windowBits for deflateInit() is changed, then
   this function needs to be updated.
 */
uLong ZEXPORT compressBound ( sourceLen )
uLong sourceLen;
{
	return sourceLen + ( sourceLen >> 12 ) + ( sourceLen >> 14 ) +
	       ( sourceLen >> 25 ) + 13;
}

/* uncompr.c -- decompress a memory buffer
 * Copyright (C) 1995-2003, 2010 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

/* ===========================================================================
     Decompresses the source buffer into the destination buffer.  sourceLen is
   the byte length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be large enough to hold the
   entire uncompressed data. (The size of the uncompressed data must have
   been saved previously by the compressor and transmitted to the decompressor
   by some mechanism outside the scope of this compression library.)
   Upon exit, destLen is the actual size of the compressed buffer.

     uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer, or Z_DATA_ERROR if the input data was corrupted.
*/
int ZEXPORT uncompress ( dest, destLen, source, sourceLen )
Bytef* dest;
uLongf* destLen;
const Bytef* source;
uLong sourceLen;
{
	z_stream stream;
	int err;

	stream.next_in = ( Bytef* )source;
	stream.avail_in = ( uInt )sourceLen;

	/* Check for source > 64K on 16-bit machine: */
	if ( ( uLong )stream.avail_in != sourceLen ) { return Z_BUF_ERROR; }

	stream.next_out = dest;
	stream.avail_out = ( uInt ) * destLen;

	if ( ( uLong )stream.avail_out != *destLen ) { return Z_BUF_ERROR; }

	stream.zalloc = ( alloc_func )0;
	stream.zfree = ( free_func )0;

	err = inflateInit( &stream );

	if ( err != Z_OK ) { return err; }

	err = inflate( &stream, Z_FINISH );

	if ( err != Z_STREAM_END )
	{
		inflateEnd( &stream );

		if ( err == Z_NEED_DICT || ( err == Z_BUF_ERROR && stream.avail_in == 0 ) )
		{
			return Z_DATA_ERROR;
		}

		return err;
	}

	*destLen = stream.total_out;

	err = inflateEnd( &stream );
	return err;
}

//////////// zip/unzip functions

/* zip.c -- IO on .zip files using zlib
   Version 1.1, February 14h, 2010
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

         Changes
   Oct-2009 - Mathias Svensson - Remove old C style function prototypes
   Oct-2009 - Mathias Svensson - Added Zip64 Support when creating new file archives
   Oct-2009 - Mathias Svensson - Did some code cleanup and refactoring to get better overview of some functions.
   Oct-2009 - Mathias Svensson - Added zipRemoveExtraInfoBlock to strip extra field data from its ZIP64 data
                                 It is used when recreting zip archive with RAW when deleting items from a zip.
                                 ZIP64 data is automaticly added to items that needs it, and existing ZIP64 data need to be removed.
   Oct-2009 - Mathias Svensson - Added support for BZIP2 as compression mode (bzip2 lib is required)
   Jan-2010 - back to unzip and minizip 1.0 name scheme, with compatibility layer

*/

#ifndef NOUNCRYPT
#define NOUNCRYPT
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include "z_.h"

#ifdef STDC
#  include <stddef.h>
#  include <string.h>
#  include <stdlib.h>
#endif
#ifdef NO_ERRNO_H
extern int errno;
#else
#   include <errno.h>
#endif


#ifndef local
#  define local static
#endif
/* compile with -Dlocal if your debugger can't find static symbols */

#ifndef VERSIONMADEBY
# define VERSIONMADEBY   (0x0) /* platform depedent */
#endif

#ifndef Z_BUFSIZE
#define Z_BUFSIZE (64*1024) //(16384)
#endif

#ifndef Z_MAXFILENAMEINZIP
#define Z_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
# define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) free(p);}
#endif

/*
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)
*/

/* I've found an old Unix (a SunOS 4.1.3_U1) without all SEEK_* defined.... */


// NOT sure that this work on ALL platform
#define MAKEULONG64(a, b) ((ZPOS64_T)(((unsigned long)(a)) | ((ZPOS64_T)((unsigned long)(b))) << 32))

#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif

#ifndef SEEK_END
#define SEEK_END    2
#endif

#ifndef SEEK_SET
#define SEEK_SET    0
#endif

#ifndef DEF_MEM_LEVEL
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
#endif
const char zip_copyright[] = " zip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";


#define SIZEDATA_INDATABLOCK (4096-(4*4))

#define LOCALHEADERMAGIC    (0x04034b50)
#define CENTRALHEADERMAGIC  (0x02014b50)
#define ENDHEADERMAGIC      (0x06054b50)
#define ZIP64ENDHEADERMAGIC      (0x6064b50)
#define ZIP64ENDLOCHEADERMAGIC   (0x7064b50)

#define FLAG_LOCALHEADER_OFFSET (0x06)
#define CRC_LOCALHEADER_OFFSET  (0x0e)

#define SIZECENTRALHEADER (0x2e) /* 46 */

typedef struct linkedlist_datablock_internal_s
{
	struct linkedlist_datablock_internal_s* next_datablock;
	uLong  avail_in_this_block;
	uLong  filled_in_this_block;
	uLong  unused; /* for future use and alignement */
	unsigned char data[SIZEDATA_INDATABLOCK];
} linkedlist_datablock_internal;

typedef struct linkedlist_data_s
{
	linkedlist_datablock_internal* first_block;
	linkedlist_datablock_internal* last_block;
} linkedlist_data;


typedef struct
{
	z_stream stream;            /* zLib stream structure for inflate */
#ifdef HAVE_BZIP2
	bz_stream bstream;          /* bzLib stream structure for bziped */
#endif

	int  stream_initialised;    /* 1 is stream is initialised */
	uInt pos_in_buffered_data;  /* last written byte in buffered_data */

	ZPOS64_T pos_local_header;     /* offset of the local header of the file
                                     currenty writing */
	char* central_header;       /* central header data for the current file */
	uLong size_centralExtra;
	uLong size_centralheader;   /* size of the central header for cur file */
	uLong size_centralExtraFree; /* Extra bytes allocated to the centralheader but that are not used */
	uLong flag;                 /* flag of the file currently writing */

	int  method;                /* compression method of file currenty wr.*/
	int  raw;                   /* 1 for directly writing raw data */
	Byte buffered_data[Z_BUFSIZE];/* buffer contain compressed data to be writ*/
	uLong dosDate;
	uLong crc32;
	int  encrypt;
	int  zip64;               /* Add ZIP64 extened information in the extra field */
	ZPOS64_T pos_zip64extrainfo;
	ZPOS64_T totalCompressedData;
	ZPOS64_T totalUncompressedData;
#ifndef NOCRYPT
	unsigned long keys[3];     /* keys defining the pseudo-random sequence */
	const unsigned long* pcrc_32_tab;
	int crypt_header_size;
#endif
} curfile64_info;

typedef struct
{
	zlib_filefunc64_32_def z_filefunc;
	voidpf filestream;        /* io structore of the zipfile */
	linkedlist_data central_dir;/* datablock with central dir in construction*/
	int  in_opened_file_inzip;  /* 1 if a file in the zip is currently writ.*/
	curfile64_info ci;            /* info on the file curretly writing */

	ZPOS64_T begin_pos;            /* position of the beginning of the zipfile */
	ZPOS64_T add_position_when_writting_offset;
	ZPOS64_T number_entry;

#ifndef NO_ADDFILEINEXISTINGZIP
	char* globalcomment;
#endif

} zip64_internal;


#ifndef NOCRYPT
#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED

//#include "crypt.h"

/* crypt.h -- base code for crypt/uncrypt ZIPfile


   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant

   This code is a modified version of crypting code in Infozip distribution

   The encryption/decryption parts of this source code (as opposed to the
   non-echoing password parts) were originally written in Europe.  The
   whole source package can be freely distributed, including from the USA.
   (Prior to January 2000, re-export from the US was a violation of US law.)

   This encryption code is a direct transcription of the algorithm from
   Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
   file (appnote.txt) is distributed with the PKZIP program (even in the
   version without encryption capabilities).

   If you don't need crypting in your application, just define symbols
   NOCRYPT and NOUNCRYPT.

   This code support the "Traditional PKWARE Encryption".

   The new AES encryption added on Zip format by Winzip (see the page
   http://www.winzip.com/aes_info.htm ) and PKWare PKZip 5.x Strong
   Encryption is not supported.
*/

#define CRC32(c, b) ((*(pcrc_32_tab+(((int)(c) ^ (b)) & 0xff))) ^ ((c) >> 8))

/***********************************************************************
 * Return the next byte in the pseudo-random sequence
 */
static int decrypt_byte( unsigned long* pkeys, const unsigned long* pcrc_32_tab )
{
	unsigned temp;  /* POTENTIAL BUG:  temp*(temp^1) may overflow in an
                     * unpredictable manner on 16-bit systems; not a problem
                     * with any known compiler so far, though */

	temp = ( ( unsigned )( *( pkeys + 2 ) ) & 0xffff ) | 2;
	return ( int )( ( ( temp * ( temp ^ 1 ) ) >> 8 ) & 0xff );
}

/***********************************************************************
 * Update the encryption keys with the next byte of plain text
 */
static int update_keys( unsigned long* pkeys, const unsigned long* pcrc_32_tab, int c )
{
	( *( pkeys + 0 ) ) = CRC32( ( *( pkeys + 0 ) ), c );
	( *( pkeys + 1 ) ) += ( *( pkeys + 0 ) ) & 0xff;
	( *( pkeys + 1 ) ) = ( *( pkeys + 1 ) ) * 134775813L + 1;
	{
		register int keyshift = ( int )( ( *( pkeys + 1 ) ) >> 24 );
		( *( pkeys + 2 ) ) = CRC32( ( *( pkeys + 2 ) ), keyshift );
	}
	return c;
}


/***********************************************************************
 * Initialize the encryption keys and the random header according to
 * the given password.
 */
static void init_keys( const char* passwd, unsigned long* pkeys, const unsigned long* pcrc_32_tab )
{
	*( pkeys + 0 ) = 305419896L;
	*( pkeys + 1 ) = 591751049L;
	*( pkeys + 2 ) = 878082192L;

	while ( *passwd != '\0' )
	{
		update_keys( pkeys, pcrc_32_tab, ( int )*passwd );
		passwd++;
	}
}

#define zdecode(pkeys,pcrc_32_tab,c) \
    (update_keys(pkeys,pcrc_32_tab,c ^= decrypt_byte(pkeys,pcrc_32_tab)))

#define zencode(pkeys,pcrc_32_tab,c,t) \
    (t=decrypt_byte(pkeys,pcrc_32_tab), update_keys(pkeys,pcrc_32_tab,c), t^(c))

#ifdef INCLUDECRYPTINGCODE_IFCRYPTALLOWED

#define RAND_HEAD_LEN  12
/* "last resort" source for second part of crypt seed pattern */
#  ifndef ZCR_SEED2
#    define ZCR_SEED2 3141592654UL     /* use PI as default pattern */
#  endif

static int crypthead( const char* passwd,     /* password string */
                      unsigned char* buf,      /* where to write header */
                      int bufSize,
                      unsigned long* pkeys,
                      const unsigned long* pcrc_32_tab,
                      unsigned long crcForCrypting )
{
	int n;                       /* index in random header */
	int t;                       /* temporary */
	int c;                       /* random byte */
	unsigned char header[RAND_HEAD_LEN - 2]; /* random header */
	static unsigned calls = 0;   /* ensure different random header each time */

	if ( bufSize < RAND_HEAD_LEN )
	{
		return 0;
	}

	/* First generate RAND_HEAD_LEN-2 random bytes. We encrypt the
	 * output of rand() to get less predictability, since rand() is
	 * often poorly implemented.
	 */
	if ( ++calls == 1 )
	{
		srand( ( unsigned )( time( NULL ) ^ ZCR_SEED2 ) );
	}

	init_keys( passwd, pkeys, pcrc_32_tab );

	for ( n = 0; n < RAND_HEAD_LEN - 2; n++ )
	{
		c = ( rand() >> 7 ) & 0xff;
		header[n] = ( unsigned char )zencode( pkeys, pcrc_32_tab, c, t );
	}

	/* Encrypt random header (last two bytes is high word of crc) */
	init_keys( passwd, pkeys, pcrc_32_tab );

	for ( n = 0; n < RAND_HEAD_LEN - 2; n++ )
	{
		buf[n] = ( unsigned char )zencode( pkeys, pcrc_32_tab, header[n], t );
	}

	buf[n++] = ( unsigned char )zencode( pkeys, pcrc_32_tab, ( int )( crcForCrypting >> 16 ) & 0xff, t );
	buf[n++] = ( unsigned char )zencode( pkeys, pcrc_32_tab, ( int )( crcForCrypting >> 24 ) & 0xff, t );
	return n;
}

#endif


#endif

local linkedlist_datablock_internal* allocate_new_datablock()
{
	linkedlist_datablock_internal* ldi;
	ldi = ( linkedlist_datablock_internal* )
	      ALLOC( sizeof( linkedlist_datablock_internal ) );

	if ( ldi != NULL )
	{
		ldi->next_datablock = NULL ;
		ldi->filled_in_this_block = 0 ;
		ldi->avail_in_this_block = SIZEDATA_INDATABLOCK ;
	}

	return ldi;
}

local void free_datablock( linkedlist_datablock_internal* ldi )
{
	while ( ldi != NULL )
	{
		linkedlist_datablock_internal* ldinext = ldi->next_datablock;
		TRYFREE( ldi );
		ldi = ldinext;
	}
}

local void init_linkedlist( linkedlist_data* ll )
{
	ll->first_block = ll->last_block = NULL;
}

local void free_linkedlist( linkedlist_data* ll )
{
	free_datablock( ll->first_block );
	ll->first_block = ll->last_block = NULL;
}


local int add_data_in_datablock( linkedlist_data* ll, const void* buf, uLong len )
{
	linkedlist_datablock_internal* ldi;
	const unsigned char* from_copy;

	if ( ll == NULL )
	{
		return ZIP_INTERNALERROR;
	}

	if ( ll->last_block == NULL )
	{
		ll->first_block = ll->last_block = allocate_new_datablock();

		if ( ll->first_block == NULL )
		{
			return ZIP_INTERNALERROR;
		}
	}

	ldi = ll->last_block;
	from_copy = ( unsigned char* )buf;

	while ( len > 0 )
	{
		uInt copy_this;
		uInt i;
		unsigned char* to_copy;

		if ( ldi->avail_in_this_block == 0 )
		{
			ldi->next_datablock = allocate_new_datablock();

			if ( ldi->next_datablock == NULL )
			{
				return ZIP_INTERNALERROR;
			}

			ldi = ldi->next_datablock ;
			ll->last_block = ldi;
		}

		if ( ldi->avail_in_this_block < len )
		{
			copy_this = ( uInt )ldi->avail_in_this_block;
		}
		else
		{
			copy_this = ( uInt )len;
		}

		to_copy = &( ldi->data[ldi->filled_in_this_block] );

		for ( i = 0; i < copy_this; i++ )
		{
			*( to_copy + i ) = *( from_copy + i );
		}

		ldi->filled_in_this_block += copy_this;
		ldi->avail_in_this_block -= copy_this;
		from_copy += copy_this ;
		len -= copy_this;
	}

	return ZIP_OK;
}



/****************************************************************************/

#ifndef NO_ADDFILEINEXISTINGZIP
/* ===========================================================================
   Inputs a long in LSB order to the given file
   nbByte == 1, 2 ,4 or 8 (byte, short or long, ZPOS64_T)
*/

local int zip64local_putValue OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, ZPOS64_T x, int nbByte ) );
local int zip64local_putValue ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, ZPOS64_T x, int nbByte )
{
	unsigned char buf[8];
	int n;

	for ( n = 0; n < nbByte; n++ )
	{
		buf[n] = ( unsigned char )( x & 0xff );
		x >>= 8;
	}

	if ( x != 0 )
	{
		/* data overflow - hack for ZIP64 (X Roche) */
		for ( n = 0; n < nbByte; n++ )
		{
			buf[n] = 0xff;
		}
	}

	if ( ZWRITE64( *pzlib_filefunc_def, filestream, buf, nbByte ) != ( uLong )nbByte )
	{
		return ZIP_ERRNO;
	}
	else
	{
		return ZIP_OK;
	}
}

local void zip64local_putValue_inmemory OF( ( void* dest, ZPOS64_T x, int nbByte ) );
local void zip64local_putValue_inmemory ( void* dest, ZPOS64_T x, int nbByte )
{
	unsigned char* buf = ( unsigned char* )dest;
	int n;

	for ( n = 0; n < nbByte; n++ )
	{
		buf[n] = ( unsigned char )( x & 0xff );
		x >>= 8;
	}

	if ( x != 0 )
	{
		/* data overflow - hack for ZIP64 */
		for ( n = 0; n < nbByte; n++ )
		{
			buf[n] = 0xff;
		}
	}
}

/****************************************************************************/


local uLong zip64local_TmzDateToDosDate( const tm_zip* ptm )
{
	uLong year = ( uLong )ptm->tm_year;

	if ( year >= 1980 )
	{
		year -= 1980;
	}
	else if ( year >= 80 )
	{
		year -= 80;
	}

	return
	   ( uLong ) ( ( ( ptm->tm_mday ) + ( 32 * ( ptm->tm_mon + 1 ) ) + ( 512 * year ) ) << 16 ) |
	   ( ( ptm->tm_sec / 2 ) + ( 32 * ptm->tm_min ) + ( 2048 * ( uLong )ptm->tm_hour ) );
}


/****************************************************************************/

local int zip64local_getByte OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, int* pi ) );

local int zip64local_getByte( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, int* pi )
{
	unsigned char c;
	int err = ( int )ZREAD64( *pzlib_filefunc_def, filestream, &c, 1 );

	if ( err == 1 )
	{
		*pi = ( int )c;
		return ZIP_OK;
	}
	else
	{
		if ( ZERROR64( *pzlib_filefunc_def, filestream ) )
		{
			return ZIP_ERRNO;
		}
		else
		{
			return ZIP_EOF;
		}
	}
}


/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets
*/
local int zip64local_getShort OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, uLong* pX ) );

local int zip64local_getShort ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, uLong* pX )
{
	uLong x ;
	int i = 0;
	int err;

	err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( uLong )i;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( uLong )i ) << 8;

	if ( err == ZIP_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

local int zip64local_getLong OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, uLong* pX ) );

local int zip64local_getLong ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, uLong* pX )
{
	uLong x ;
	int i = 0;
	int err;

	err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( uLong )i;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( uLong )i ) << 8;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( uLong )i ) << 16;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( uLong )i ) << 24;

	if ( err == ZIP_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

local int zip64local_getLong64 OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, ZPOS64_T* pX ) );


local int zip64local_getLong64 ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, ZPOS64_T* pX )
{
	ZPOS64_T x;
	int i = 0;
	int err;

	err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( ZPOS64_T )i;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 8;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 16;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 24;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 32;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 40;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 48;

	if ( err == ZIP_OK )
	{
		err = zip64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( ZPOS64_T )i ) << 56;

	if ( err == ZIP_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

#ifndef BUFREADCOMMENT
#define BUFREADCOMMENT (0x400)
#endif
/*
  Locate the Central directory of a zipfile (at the end, just before
    the global comment)
*/
local ZPOS64_T zip64local_SearchCentralDir OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream ) );

local ZPOS64_T zip64local_SearchCentralDir( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream )
{
	unsigned char* buf;
	ZPOS64_T uSizeFile;
	ZPOS64_T uBackRead;
	ZPOS64_T uMaxBack = 0xffff; /* maximum size of global comment */
	ZPOS64_T uPosFound = 0;

	if ( ZSEEK64( *pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END ) != 0 )
	{
		return 0;
	}


	uSizeFile = ZTELL64( *pzlib_filefunc_def, filestream );

	if ( uMaxBack > uSizeFile )
	{
		uMaxBack = uSizeFile;
	}

	buf = ( unsigned char* )ALLOC( BUFREADCOMMENT + 4 );

	if ( buf == NULL )
	{
		return 0;
	}

	uBackRead = 4;

	while ( uBackRead < uMaxBack )
	{
		uLong uReadSize;
		ZPOS64_T uReadPos ;
		int i;

		if ( uBackRead + BUFREADCOMMENT > uMaxBack )
		{
			uBackRead = uMaxBack;
		}
		else
		{
			uBackRead += BUFREADCOMMENT;
		}

		uReadPos = uSizeFile - uBackRead ;

		uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
		            ( BUFREADCOMMENT + 4 ) : ( uLong )( uSizeFile - uReadPos );

		if ( ZSEEK64( *pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			break;
		}

		if ( ZREAD64( *pzlib_filefunc_def, filestream, buf, uReadSize ) != uReadSize )
		{
			break;
		}

		for ( i = ( int )uReadSize - 3; ( i-- ) > 0; )
			if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) &&
			     ( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) )
			{
				uPosFound = uReadPos + i;
				break;
			}

		if ( uPosFound != 0 )
		{
			break;
		}
	}

	TRYFREE( buf );
	return uPosFound;
}

/*
Locate the End of Zip64 Central directory locator and from there find the CD of a zipfile (at the end, just before
the global comment)
*/
local ZPOS64_T zip64local_SearchCentralDir64 OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream ) );

local ZPOS64_T zip64local_SearchCentralDir64( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream )
{
	unsigned char* buf;
	ZPOS64_T uSizeFile;
	ZPOS64_T uBackRead;
	ZPOS64_T uMaxBack = 0xffff; /* maximum size of global comment */
	ZPOS64_T uPosFound = 0;
	uLong uL;
	ZPOS64_T relativeOffset;

	if ( ZSEEK64( *pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END ) != 0 )
	{
		return 0;
	}

	uSizeFile = ZTELL64( *pzlib_filefunc_def, filestream );

	if ( uMaxBack > uSizeFile )
	{
		uMaxBack = uSizeFile;
	}

	buf = ( unsigned char* )ALLOC( BUFREADCOMMENT + 4 );

	if ( buf == NULL )
	{
		return 0;
	}

	uBackRead = 4;

	while ( uBackRead < uMaxBack )
	{
		uLong uReadSize;
		ZPOS64_T uReadPos;
		int i;

		if ( uBackRead + BUFREADCOMMENT > uMaxBack )
		{
			uBackRead = uMaxBack;
		}
		else
		{
			uBackRead += BUFREADCOMMENT;
		}

		uReadPos = uSizeFile - uBackRead ;

		uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
		            ( BUFREADCOMMENT + 4 ) : ( uLong )( uSizeFile - uReadPos );

		if ( ZSEEK64( *pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			break;
		}

		if ( ZREAD64( *pzlib_filefunc_def, filestream, buf, uReadSize ) != uReadSize )
		{
			break;
		}

		for ( i = ( int )uReadSize - 3; ( i-- ) > 0; )
		{
			// Signature "0x07064b50" Zip64 end of central directory locater
			if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) && ( ( *( buf + i + 2 ) ) == 0x06 ) && ( ( *( buf + i + 3 ) ) == 0x07 ) )
			{
				uPosFound = uReadPos + i;
				break;
			}
		}

		if ( uPosFound != 0 )
		{
			break;
		}
	}

	TRYFREE( buf );

	if ( uPosFound == 0 )
	{
		return 0;
	}

	/* Zip64 end of central directory locator */
	if ( ZSEEK64( *pzlib_filefunc_def, filestream, uPosFound, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return 0;
	}

	/* the signature, already checked */
	if ( zip64local_getLong( pzlib_filefunc_def, filestream, &uL ) != ZIP_OK )
	{
		return 0;
	}

	/* number of the disk with the start of the zip64 end of  central directory */
	if ( zip64local_getLong( pzlib_filefunc_def, filestream, &uL ) != ZIP_OK )
	{
		return 0;
	}

	if ( uL != 0 )
	{
		return 0;
	}

	/* relative offset of the zip64 end of central directory record */
	if ( zip64local_getLong64( pzlib_filefunc_def, filestream, &relativeOffset ) != ZIP_OK )
	{
		return 0;
	}

	/* total number of disks */
	if ( zip64local_getLong( pzlib_filefunc_def, filestream, &uL ) != ZIP_OK )
	{
		return 0;
	}

	if ( uL != 1 )
	{
		return 0;
	}

	/* Goto Zip64 end of central directory record */
	if ( ZSEEK64( *pzlib_filefunc_def, filestream, relativeOffset, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return 0;
	}

	/* the signature */
	if ( zip64local_getLong( pzlib_filefunc_def, filestream, &uL ) != ZIP_OK )
	{
		return 0;
	}

	if ( uL != 0x06064b50 ) // signature of 'Zip64 end of central directory'
	{
		return 0;
	}

	return relativeOffset;
}

int LoadCentralDirectoryRecord( zip64_internal* pziinit )
{
	int err = ZIP_OK;
	ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/

	ZPOS64_T size_central_dir;     /* size of the central directory  */
	ZPOS64_T offset_central_dir;   /* offset of start of central directory */
	ZPOS64_T central_pos;
	uLong uL;

	uLong number_disk;          /* number of the current dist, used for
                              spaning ZIP, unsupported, always 0*/
	uLong number_disk_with_CD;  /* number the the disk with central dir, used
                              for spaning ZIP, unsupported, always 0*/
	ZPOS64_T number_entry;
	ZPOS64_T number_entry_CD;      /* total number of entries in
                                the central dir
                                (same than number_entry on nospan) */
	uLong VersionMadeBy;
	uLong VersionNeeded;
	uLong size_comment;

	int hasZIP64Record = 0;

	// check first if we find a ZIP64 record
	central_pos = zip64local_SearchCentralDir64( &pziinit->z_filefunc, pziinit->filestream );

	if ( central_pos > 0 )
	{
		hasZIP64Record = 1;
	}
	else if ( central_pos == 0 )
	{
		central_pos = zip64local_SearchCentralDir( &pziinit->z_filefunc, pziinit->filestream );
	}

	/* disable to allow appending to empty ZIP archive
	        if (central_pos==0)
	            err=ZIP_ERRNO;
	*/

	if ( hasZIP64Record )
	{
		ZPOS64_T sizeEndOfCentralDirectory;

		if ( ZSEEK64( pziinit->z_filefunc, pziinit->filestream, central_pos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = ZIP_ERRNO;
		}

		/* the signature, already checked */
		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* size of zip64 end of central directory record */
		if ( zip64local_getLong64( &pziinit->z_filefunc, pziinit->filestream, &sizeEndOfCentralDirectory ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* version made by */
		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &VersionMadeBy ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* version needed to extract */
		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &VersionNeeded ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* number of this disk */
		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &number_disk ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* number of the disk with the start of the central directory */
		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &number_disk_with_CD ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* total number of entries in the central directory on this disk */
		if ( zip64local_getLong64( &pziinit->z_filefunc, pziinit->filestream, &number_entry ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* total number of entries in the central directory */
		if ( zip64local_getLong64( &pziinit->z_filefunc, pziinit->filestream, &number_entry_CD ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		if ( ( number_entry_CD != number_entry ) || ( number_disk_with_CD != 0 ) || ( number_disk != 0 ) )
		{
			err = ZIP_BADZIPFILE;
		}

		/* size of the central directory */
		if ( zip64local_getLong64( &pziinit->z_filefunc, pziinit->filestream, &size_central_dir ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* offset of start of central directory with respect to the
		starting disk number */
		if ( zip64local_getLong64( &pziinit->z_filefunc, pziinit->filestream, &offset_central_dir ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		// TODO..
		// read the comment from the standard central header.
		size_comment = 0;
	}
	else
	{
		// Read End of central Directory info
		if ( ZSEEK64( pziinit->z_filefunc, pziinit->filestream, central_pos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = ZIP_ERRNO;
		}

		/* the signature, already checked */
		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* number of this disk */
		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &number_disk ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* number of the disk with the start of the central directory */
		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &number_disk_with_CD ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

		/* total number of entries in the central dir on this disk */
		number_entry = 0;

		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}
		else
		{
			number_entry = uL;
		}

		/* total number of entries in the central dir */
		number_entry_CD = 0;

		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}
		else
		{
			number_entry_CD = uL;
		}

		if ( ( number_entry_CD != number_entry ) || ( number_disk_with_CD != 0 ) || ( number_disk != 0 ) )
		{
			err = ZIP_BADZIPFILE;
		}

		/* size of the central directory */
		size_central_dir = 0;

		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}
		else
		{
			size_central_dir = uL;
		}

		/* offset of start of central directory with respect to the starting disk number */
		offset_central_dir = 0;

		if ( zip64local_getLong( &pziinit->z_filefunc, pziinit->filestream, &uL ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}
		else
		{
			offset_central_dir = uL;
		}


		/* zipfile global comment length */
		if ( zip64local_getShort( &pziinit->z_filefunc, pziinit->filestream, &size_comment ) != ZIP_OK )
		{
			err = ZIP_ERRNO;
		}
	}

	if ( ( central_pos < offset_central_dir + size_central_dir ) &&
	     ( err == ZIP_OK ) )
	{
		err = ZIP_BADZIPFILE;
	}

	if ( err != ZIP_OK )
	{
		ZCLOSE64( pziinit->z_filefunc, pziinit->filestream );
		return ZIP_ERRNO;
	}

	if ( size_comment > 0 )
	{
		pziinit->globalcomment = ( char* )ALLOC( size_comment + 1 );

		if ( pziinit->globalcomment )
		{
			size_comment = ZREAD64( pziinit->z_filefunc, pziinit->filestream, pziinit->globalcomment, size_comment );
			pziinit->globalcomment[size_comment] = 0;
		}
	}

	byte_before_the_zipfile = central_pos - ( offset_central_dir + size_central_dir );
	pziinit->add_position_when_writting_offset = byte_before_the_zipfile;

	{
		ZPOS64_T size_central_dir_to_read = size_central_dir;
		size_t buf_size = SIZEDATA_INDATABLOCK;
		void* buf_read = ( void* )ALLOC( buf_size );

		if ( ZSEEK64( pziinit->z_filefunc, pziinit->filestream, offset_central_dir + byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = ZIP_ERRNO;
		}

		while ( ( size_central_dir_to_read > 0 ) && ( err == ZIP_OK ) )
		{
			ZPOS64_T read_this = SIZEDATA_INDATABLOCK;

			if ( read_this > size_central_dir_to_read )
			{
				read_this = size_central_dir_to_read;
			}

			if ( ZREAD64( pziinit->z_filefunc, pziinit->filestream, buf_read, ( uLong )read_this ) != read_this )
			{
				err = ZIP_ERRNO;
			}

			if ( err == ZIP_OK )
			{
				err = add_data_in_datablock( &pziinit->central_dir, buf_read, ( uLong )read_this );
			}

			size_central_dir_to_read -= read_this;
		}

		TRYFREE( buf_read );
	}
	pziinit->begin_pos = byte_before_the_zipfile;
	pziinit->number_entry = number_entry_CD;

	if ( ZSEEK64( pziinit->z_filefunc, pziinit->filestream, offset_central_dir + byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		err = ZIP_ERRNO;
	}

	return err;
}


#endif /* !NO_ADDFILEINEXISTINGZIP*/


/************************************************************/
extern zipFile ZEXPORT zipOpen3 ( const void* pathname, int append, zipcharpc* globalcomment, zlib_filefunc64_32_def* pzlib_filefunc64_32_def )
{
	zip64_internal ziinit;
	zip64_internal* zi;
	int err = ZIP_OK;

	ziinit.z_filefunc.zseek32_file = NULL;
	ziinit.z_filefunc.ztell32_file = NULL;

	if ( pzlib_filefunc64_32_def == NULL )
		/*fill_fopen64_filefunc(&ziinit.z_filefunc.zfile_func64)*/ { }
	else
	{
		ziinit.z_filefunc = *pzlib_filefunc64_32_def;
	}

	ziinit.filestream = ZOPEN64( ziinit.z_filefunc,
	                             pathname,
	                             ( append == APPEND_STATUS_CREATE ) ?
	                             ( ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE ) :
	                             ( ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING ) );

	if ( ziinit.filestream == NULL )
	{
		return NULL;
	}

	if ( append == APPEND_STATUS_CREATEAFTER )
	{
		ZSEEK64( ziinit.z_filefunc, ziinit.filestream, 0, SEEK_END );
	}

	ziinit.begin_pos = ZTELL64( ziinit.z_filefunc, ziinit.filestream );
	ziinit.in_opened_file_inzip = 0;
	ziinit.ci.stream_initialised = 0;
	ziinit.number_entry = 0;
	ziinit.add_position_when_writting_offset = 0;
	init_linkedlist( &( ziinit.central_dir ) );



	zi = ( zip64_internal* )ALLOC( sizeof( zip64_internal ) );

	if ( zi == NULL )
	{
		ZCLOSE64( ziinit.z_filefunc, ziinit.filestream );
		return NULL;
	}

	/* now we add file in a zipfile */
#    ifndef NO_ADDFILEINEXISTINGZIP
	ziinit.globalcomment = NULL;

	if ( append == APPEND_STATUS_ADDINZIP )
	{
		// Read and Cache Central Directory Records
		err = LoadCentralDirectoryRecord( &ziinit );
	}

	if ( globalcomment )
	{
		*globalcomment = ziinit.globalcomment;
	}

#    endif /* !NO_ADDFILEINEXISTINGZIP*/

	if ( err != ZIP_OK )
	{
#    ifndef NO_ADDFILEINEXISTINGZIP
		TRYFREE( ziinit.globalcomment );
#    endif /* !NO_ADDFILEINEXISTINGZIP*/
		TRYFREE( zi );
		return NULL;
	}
	else
	{
		*zi = ziinit;
		return ( zipFile )zi;
	}
}

extern zipFile ZEXPORT zipOpen2 ( const char* pathname, int append, zipcharpc* globalcomment, zlib_filefunc_def* pzlib_filefunc32_def )
{
	if ( pzlib_filefunc32_def != NULL )
	{
		zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
		fill_zlib_filefunc64_32_def_from_filefunc32( &zlib_filefunc64_32_def_fill, pzlib_filefunc32_def );
		return zipOpen3( pathname, append, globalcomment, &zlib_filefunc64_32_def_fill );
	}
	else
	{
		return zipOpen3( pathname, append, globalcomment, NULL );
	}
}

extern zipFile ZEXPORT zipOpen2_64 ( const void* pathname, int append, zipcharpc* globalcomment, zlib_filefunc64_def* pzlib_filefunc_def )
{
	if ( pzlib_filefunc_def != NULL )
	{
		zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
		zlib_filefunc64_32_def_fill.zfile_func64 = *pzlib_filefunc_def;
		zlib_filefunc64_32_def_fill.ztell32_file = NULL;
		zlib_filefunc64_32_def_fill.zseek32_file = NULL;
		return zipOpen3( pathname, append, globalcomment, &zlib_filefunc64_32_def_fill );
	}
	else
	{
		return zipOpen3( pathname, append, globalcomment, NULL );
	}
}



extern zipFile ZEXPORT zipOpen ( const char* pathname, int append )
{
	return zipOpen3( ( const void* )pathname, append, NULL, NULL );
}

extern zipFile ZEXPORT zipOpen64 ( const void* pathname, int append )
{
	return zipOpen3( pathname, append, NULL, NULL );
}

int Write_LocalFileHeader( zip64_internal* zi, const char* filename, uInt size_extrafield_local, const void* extrafield_local )
{
	/* write the local header */
	int err;
	uInt size_filename = ( uInt )strlen( filename );
	uInt size_extrafield = size_extrafield_local;

	err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )LOCALHEADERMAGIC, 4 );

	if ( err == ZIP_OK )
	{
		if ( zi->ci.zip64 )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )45, 2 );   /* version needed to extract */
		}
		else
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )20, 2 );   /* version needed to extract */
		}
	}

	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )zi->ci.flag, 2 );
	}

	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )zi->ci.method, 2 );
	}

	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )zi->ci.dosDate, 4 );
	}

	// CRC / Compressed size / Uncompressed size will be filled in later and rewritten later
	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );   /* crc 32, unknown */
	}

	if ( err == ZIP_OK )
	{
		if ( zi->ci.zip64 )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0xFFFFFFFF, 4 );   /* compressed size, unknown */
		}
		else
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );   /* compressed size, unknown */
		}
	}

	if ( err == ZIP_OK )
	{
		if ( zi->ci.zip64 )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0xFFFFFFFF, 4 );   /* uncompressed size, unknown */
		}
		else
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );   /* uncompressed size, unknown */
		}
	}

	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )size_filename, 2 );
	}

	if ( zi->ci.zip64 )
	{
		size_extrafield += 20;
	}

	if ( err == ZIP_OK )
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )size_extrafield, 2 );
	}

	if ( ( err == ZIP_OK ) && ( size_filename > 0 ) )
	{
		if ( ZWRITE64( zi->z_filefunc, zi->filestream, filename, size_filename ) != size_filename )
		{
			err = ZIP_ERRNO;
		}
	}

	if ( ( err == ZIP_OK ) && ( size_extrafield_local > 0 ) )
	{
		if ( ZWRITE64( zi->z_filefunc, zi->filestream, extrafield_local, size_extrafield_local ) != size_extrafield_local )
		{
			err = ZIP_ERRNO;
		}
	}


	if ( ( err == ZIP_OK ) && ( zi->ci.zip64 ) )
	{
		// write the Zip64 extended info
		short HeaderID = 1;
		short DataSize = 16;
		ZPOS64_T CompressedSize = 0;
		ZPOS64_T UncompressedSize = 0;

		// Remember position of Zip64 extended info for the local file header. (needed when we update size after done with file)
		zi->ci.pos_zip64extrainfo = ZTELL64( zi->z_filefunc, zi->filestream );

		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( short )HeaderID, 2 );
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( short )DataSize, 2 );

		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( ZPOS64_T )UncompressedSize, 8 );
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( ZPOS64_T )CompressedSize, 8 );
	}

	return err;
}

/*
 NOTE.
 When writing RAW the ZIP64 extended information in extrafield_local and extrafield_global needs to be stripped
 before calling this function it can be done with zipRemoveExtraInfoBlock

 It is not done here because then we need to realloc a new buffer since parameters are 'const' and I want to minimize
 unnecessary allocations.
 */
extern int ZEXPORT zipOpenNewFileInZip4_64 ( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                             const void* extrafield_local, uInt size_extrafield_local,
                                             const void* extrafield_global, uInt size_extrafield_global,
                                             const char* comment, int method, int level, int raw,
                                             int windowBits, int memLevel, int strategy,
                                             const char* password, uLong crcForCrypting,
                                             uLong versionMadeBy, uLong flagBase, int zip64 )
{
	zip64_internal* zi;
	uInt size_filename;
	uInt size_comment;
	uInt i;
	int err = ZIP_OK;

#    ifdef NOCRYPT

	if ( password != NULL )
	{
		return ZIP_PARAMERROR;
	}

#    endif

	if ( file == NULL )
	{
		return ZIP_PARAMERROR;
	}

#ifdef HAVE_BZIP2

	if ( ( method != 0 ) && ( method != Z_DEFLATED ) && ( method != Z_BZIP2ED ) )
	{
		return ZIP_PARAMERROR;
	}

#else

	if ( ( method != 0 ) && ( method != Z_DEFLATED ) )
	{
		return ZIP_PARAMERROR;
	}

#endif

	zi = ( zip64_internal* )file;

	if ( zi->in_opened_file_inzip == 1 )
	{
		err = zipCloseFileInZip ( file );

		if ( err != ZIP_OK )
		{
			return err;
		}
	}

	if ( filename == NULL )
	{
		filename = "-";
	}

	if ( comment == NULL )
	{
		size_comment = 0;
	}
	else
	{
		size_comment = ( uInt )strlen( comment );
	}

	size_filename = ( uInt )strlen( filename );

	if ( zipfi == NULL )
	{
		zi->ci.dosDate = 0;
	}
	else
	{
		if ( zipfi->dosDate != 0 )
		{
			zi->ci.dosDate = zipfi->dosDate;
		}
		else
		{
			zi->ci.dosDate = zip64local_TmzDateToDosDate( &zipfi->tmz_date );
		}
	}

	zi->ci.flag = flagBase;

	if ( ( level == 8 ) || ( level == 9 ) )
	{
		zi->ci.flag |= 2;
	}

	if ( level == 2 )
	{
		zi->ci.flag |= 4;
	}

	if ( level == 1 )
	{
		zi->ci.flag |= 6;
	}

	if ( password != NULL )
	{
		zi->ci.flag |= 1;
	}

	zi->ci.crc32 = 0;
	zi->ci.method = method;
	zi->ci.encrypt = 0;
	zi->ci.stream_initialised = 0;
	zi->ci.pos_in_buffered_data = 0;
	zi->ci.raw = raw;
	zi->ci.pos_local_header = ZTELL64( zi->z_filefunc, zi->filestream );

	zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename + size_extrafield_global + size_comment;
	zi->ci.size_centralExtraFree = 32; // Extra space we have reserved in case we need to add ZIP64 extra info data

	zi->ci.central_header = ( char* )ALLOC( ( uInt )zi->ci.size_centralheader + zi->ci.size_centralExtraFree );

	zi->ci.size_centralExtra = size_extrafield_global;
	zip64local_putValue_inmemory( zi->ci.central_header, ( uLong )CENTRALHEADERMAGIC, 4 );
	/* version info */
	zip64local_putValue_inmemory( zi->ci.central_header + 4, ( uLong )versionMadeBy, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 6, ( uLong )20, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 8, ( uLong )zi->ci.flag, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 10, ( uLong )zi->ci.method, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 12, ( uLong )zi->ci.dosDate, 4 );
	zip64local_putValue_inmemory( zi->ci.central_header + 16, ( uLong )0, 4 ); /*crc*/
	zip64local_putValue_inmemory( zi->ci.central_header + 20, ( uLong )0, 4 ); /*compr size*/
	zip64local_putValue_inmemory( zi->ci.central_header + 24, ( uLong )0, 4 ); /*uncompr size*/
	zip64local_putValue_inmemory( zi->ci.central_header + 28, ( uLong )size_filename, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 30, ( uLong )size_extrafield_global, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 32, ( uLong )size_comment, 2 );
	zip64local_putValue_inmemory( zi->ci.central_header + 34, ( uLong )0, 2 ); /*disk nm start*/

	if ( zipfi == NULL )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 36, ( uLong )0, 2 );
	}
	else
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 36, ( uLong )zipfi->internal_fa, 2 );
	}

	if ( zipfi == NULL )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 38, ( uLong )0, 4 );
	}
	else
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 38, ( uLong )zipfi->external_fa, 4 );
	}

	if ( zi->ci.pos_local_header >= 0xffffffff )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 42, ( uLong )0xffffffff, 4 );
	}
	else
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 42, ( uLong )zi->ci.pos_local_header - zi->add_position_when_writting_offset, 4 );
	}

	for ( i = 0; i < size_filename; i++ )
	{
		*( zi->ci.central_header + SIZECENTRALHEADER + i ) = *( filename + i );
	}

	for ( i = 0; i < size_extrafield_global; i++ )
		*( zi->ci.central_header + SIZECENTRALHEADER + size_filename + i ) =
		   *( ( ( const char* )extrafield_global ) + i );

	for ( i = 0; i < size_comment; i++ )
		*( zi->ci.central_header + SIZECENTRALHEADER + size_filename +
		   size_extrafield_global + i ) = *( comment + i );

	if ( zi->ci.central_header == NULL )
	{
		return ZIP_INTERNALERROR;
	}

	zi->ci.zip64 = zip64;
	zi->ci.totalCompressedData = 0;
	zi->ci.totalUncompressedData = 0;
	zi->ci.pos_zip64extrainfo = 0;

	err = Write_LocalFileHeader( zi, filename, size_extrafield_local, extrafield_local );

#ifdef HAVE_BZIP2
	zi->ci.bstream.avail_in = ( uInt )0;
	zi->ci.bstream.avail_out = ( uInt )Z_BUFSIZE;
	zi->ci.bstream.next_out = ( char* )zi->ci.buffered_data;
	zi->ci.bstream.total_in_hi32 = 0;
	zi->ci.bstream.total_in_lo32 = 0;
	zi->ci.bstream.total_out_hi32 = 0;
	zi->ci.bstream.total_out_lo32 = 0;
#endif

	zi->ci.stream.avail_in = ( uInt )0;
	zi->ci.stream.avail_out = ( uInt )Z_BUFSIZE;
	zi->ci.stream.next_out = zi->ci.buffered_data;
	zi->ci.stream.total_in = 0;
	zi->ci.stream.total_out = 0;
	zi->ci.stream.data_type = Z_BINARY;

#ifdef HAVE_BZIP2

	if ( ( err == ZIP_OK ) && ( zi->ci.method == Z_DEFLATED || zi->ci.method == Z_BZIP2ED ) && ( !zi->ci.raw ) )
#else
	if ( ( err == ZIP_OK ) && ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) )
#endif
	{
		if ( zi->ci.method == Z_DEFLATED )
		{
			zi->ci.stream.zalloc = ( alloc_func )0;
			zi->ci.stream.zfree = ( free_func )0;
			zi->ci.stream.opaque = ( voidpf )0;

			if ( windowBits > 0 )
			{
				windowBits = -windowBits;
			}

			err = deflateInit2( &zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy );

			if ( err == Z_OK )
			{
				zi->ci.stream_initialised = Z_DEFLATED;
			}
		}
		else if ( zi->ci.method == Z_BZIP2ED )
		{
#ifdef HAVE_BZIP2
			// Init BZip stuff here
			zi->ci.bstream.bzalloc = 0;
			zi->ci.bstream.bzfree = 0;
			zi->ci.bstream.opaque = ( voidpf )0;

			err = BZ2_bzCompressInit( &zi->ci.bstream, level, 0, 35 );

			if ( err == BZ_OK )
			{
				zi->ci.stream_initialised = Z_BZIP2ED;
			}

#endif
		}

	}

#    ifndef NOCRYPT
	zi->ci.crypt_header_size = 0;

	if ( ( err == Z_OK ) && ( password != NULL ) )
	{
		unsigned char bufHead[RAND_HEAD_LEN];
		unsigned int sizeHead;
		zi->ci.encrypt = 1;
		zi->ci.pcrc_32_tab = get_crc_table();
		/*init_keys(password,zi->ci.keys,zi->ci.pcrc_32_tab);*/

		sizeHead = crypthead( password, bufHead, RAND_HEAD_LEN, zi->ci.keys, zi->ci.pcrc_32_tab, crcForCrypting );
		zi->ci.crypt_header_size = sizeHead;

		if ( ZWRITE64( zi->z_filefunc, zi->filestream, bufHead, sizeHead ) != sizeHead )
		{
			err = ZIP_ERRNO;
		}
	}

#    endif

	if ( err == Z_OK )
	{
		zi->in_opened_file_inzip = 1;
	}

	return err;
}

extern int ZEXPORT zipOpenNewFileInZip4 ( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                          const void* extrafield_local, uInt size_extrafield_local,
                                          const void* extrafield_global, uInt size_extrafield_global,
                                          const char* comment, int method, int level, int raw,
                                          int windowBits, int memLevel, int strategy,
                                          const char* password, uLong crcForCrypting,
                                          uLong versionMadeBy, uLong flagBase )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, raw,
	                                 windowBits, memLevel, strategy,
	                                 password, crcForCrypting, versionMadeBy, flagBase, 0 );
}

extern int ZEXPORT zipOpenNewFileInZip3 ( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                          const void* extrafield_local, uInt size_extrafield_local,
                                          const void* extrafield_global, uInt size_extrafield_global,
                                          const char* comment, int method, int level, int raw,
                                          int windowBits, int memLevel, int strategy,
                                          const char* password, uLong crcForCrypting )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, raw,
	                                 windowBits, memLevel, strategy,
	                                 password, crcForCrypting, VERSIONMADEBY, 0, 0 );
}

extern int ZEXPORT zipOpenNewFileInZip3_64( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                            const void* extrafield_local, uInt size_extrafield_local,
                                            const void* extrafield_global, uInt size_extrafield_global,
                                            const char* comment, int method, int level, int raw,
                                            int windowBits, int memLevel, int strategy,
                                            const char* password, uLong crcForCrypting, int zip64 )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, raw,
	                                 windowBits, memLevel, strategy,
	                                 password, crcForCrypting, VERSIONMADEBY, 0, zip64 );
}

extern int ZEXPORT zipOpenNewFileInZip2( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                         const void* extrafield_local, uInt size_extrafield_local,
                                         const void* extrafield_global, uInt size_extrafield_global,
                                         const char* comment, int method, int level, int raw )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, raw,
	                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
	                                 NULL, 0, VERSIONMADEBY, 0, 0 );
}

extern int ZEXPORT zipOpenNewFileInZip2_64( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                            const void* extrafield_local, uInt size_extrafield_local,
                                            const void* extrafield_global, uInt size_extrafield_global,
                                            const char* comment, int method, int level, int raw, int zip64 )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, raw,
	                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
	                                 NULL, 0, VERSIONMADEBY, 0, zip64 );
}

extern int ZEXPORT zipOpenNewFileInZip64 ( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                           const void* extrafield_local, uInt size_extrafield_local,
                                           const void* extrafield_global, uInt size_extrafield_global,
                                           const char* comment, int method, int level, int zip64 )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, 0,
	                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
	                                 NULL, 0, VERSIONMADEBY, 0, zip64 );
}

extern int ZEXPORT zipOpenNewFileInZip ( zipFile file, const char* filename, const zip_fileinfo* zipfi,
                                         const void* extrafield_local, uInt size_extrafield_local,
                                         const void* extrafield_global, uInt size_extrafield_global,
                                         const char* comment, int method, int level )
{
	return zipOpenNewFileInZip4_64 ( file, filename, zipfi,
	                                 extrafield_local, size_extrafield_local,
	                                 extrafield_global, size_extrafield_global,
	                                 comment, method, level, 0,
	                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
	                                 NULL, 0, VERSIONMADEBY, 0, 0 );
}

local int zip64FlushWriteBuffer( zip64_internal* zi )
{
	int err = ZIP_OK;

	if ( zi->ci.encrypt != 0 )
	{
#ifndef NOCRYPT
		uInt i;
		int t;

		for ( i = 0; i < zi->ci.pos_in_buffered_data; i++ )
		{
			zi->ci.buffered_data[i] = zencode( zi->ci.keys, zi->ci.pcrc_32_tab, zi->ci.buffered_data[i], t );
		}

#endif
	}

	if ( ZWRITE64( zi->z_filefunc, zi->filestream, zi->ci.buffered_data, zi->ci.pos_in_buffered_data ) != zi->ci.pos_in_buffered_data )
	{
		err = ZIP_ERRNO;
	}

	zi->ci.totalCompressedData += zi->ci.pos_in_buffered_data;

#ifdef HAVE_BZIP2

	if ( zi->ci.method == Z_BZIP2ED )
	{
		zi->ci.totalUncompressedData += zi->ci.bstream.total_in_lo32;
		zi->ci.bstream.total_in_lo32 = 0;
		zi->ci.bstream.total_in_hi32 = 0;
	}
	else
#endif
	{
		zi->ci.totalUncompressedData += zi->ci.stream.total_in;
		zi->ci.stream.total_in = 0;
	}


	zi->ci.pos_in_buffered_data = 0;

	return err;
}

extern int ZEXPORT zipWriteInFileInZip ( zipFile file, const void* buf, unsigned int len )
{
	zip64_internal* zi;
	int err = ZIP_OK;

	if ( file == NULL )
	{
		return ZIP_PARAMERROR;
	}

	zi = ( zip64_internal* )file;

	if ( zi->in_opened_file_inzip == 0 )
	{
		return ZIP_PARAMERROR;
	}

	zi->ci.crc32 = crc32( zi->ci.crc32, buf, ( uInt )len );

#ifdef HAVE_BZIP2

	if ( zi->ci.method == Z_BZIP2ED && ( !zi->ci.raw ) )
	{
		zi->ci.bstream.next_in = ( void* )buf;
		zi->ci.bstream.avail_in = len;
		err = BZ_RUN_OK;

		while ( ( err == BZ_RUN_OK ) && ( zi->ci.bstream.avail_in > 0 ) )
		{
			if ( zi->ci.bstream.avail_out == 0 )
			{
				if ( zip64FlushWriteBuffer( zi ) == ZIP_ERRNO )
				{
					err = ZIP_ERRNO;
				}

				zi->ci.bstream.avail_out = ( uInt )Z_BUFSIZE;
				zi->ci.bstream.next_out = ( char* )zi->ci.buffered_data;
			}


			if ( err != BZ_RUN_OK )
			{
				break;
			}

			if ( ( zi->ci.method == Z_BZIP2ED ) && ( !zi->ci.raw ) )
			{
				uLong uTotalOutBefore_lo = zi->ci.bstream.total_out_lo32;
//          uLong uTotalOutBefore_hi = zi->ci.bstream.total_out_hi32;
				err = BZ2_bzCompress( &zi->ci.bstream,  BZ_RUN );

				zi->ci.pos_in_buffered_data += ( uInt )( zi->ci.bstream.total_out_lo32 - uTotalOutBefore_lo ) ;
			}
		}

		if ( err == BZ_RUN_OK )
		{
			err = ZIP_OK;
		}
	}
	else
#endif
	{
		zi->ci.stream.next_in = ( Bytef* )buf;
		zi->ci.stream.avail_in = len;

		while ( ( err == ZIP_OK ) && ( zi->ci.stream.avail_in > 0 ) )
		{
			if ( zi->ci.stream.avail_out == 0 )
			{
				if ( zip64FlushWriteBuffer( zi ) == ZIP_ERRNO )
				{
					err = ZIP_ERRNO;
				}

				zi->ci.stream.avail_out = ( uInt )Z_BUFSIZE;
				zi->ci.stream.next_out = zi->ci.buffered_data;
			}


			if ( err != ZIP_OK )
			{
				break;
			}

			if ( ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) )
			{
				uLong uTotalOutBefore = zi->ci.stream.total_out;
				err = deflate( &zi->ci.stream,  Z_NO_FLUSH );

				if ( uTotalOutBefore > zi->ci.stream.total_out )
				{
					int bBreak = 0;
					bBreak++;
				}

				zi->ci.pos_in_buffered_data += ( uInt )( zi->ci.stream.total_out - uTotalOutBefore ) ;
			}
			else
			{
				uInt copy_this, i;

				if ( zi->ci.stream.avail_in < zi->ci.stream.avail_out )
				{
					copy_this = zi->ci.stream.avail_in;
				}
				else
				{
					copy_this = zi->ci.stream.avail_out;
				}

				for ( i = 0; i < copy_this; i++ )
					*( ( ( char* )zi->ci.stream.next_out ) + i ) =
					   *( ( ( const char* )zi->ci.stream.next_in ) + i );

				{
					zi->ci.stream.avail_in -= copy_this;
					zi->ci.stream.avail_out -= copy_this;
					zi->ci.stream.next_in += copy_this;
					zi->ci.stream.next_out += copy_this;
					zi->ci.stream.total_in += copy_this;
					zi->ci.stream.total_out += copy_this;
					zi->ci.pos_in_buffered_data += copy_this;
				}
			}
		}// while(...)
	}

	return err;
}

extern int ZEXPORT zipCloseFileInZipRaw ( zipFile file, uLong uncompressed_size, uLong crc32 )
{
	return zipCloseFileInZipRaw64 ( file, uncompressed_size, crc32 );
}

extern int ZEXPORT zipCloseFileInZipRaw64 ( zipFile file, ZPOS64_T uncompressed_size, uLong crc32 )
{
	zip64_internal* zi;
	ZPOS64_T compressed_size;
	uLong invalidValue = 0xffffffff;
	short datasize = 0;
	int err = ZIP_OK;

	if ( file == NULL )
	{
		return ZIP_PARAMERROR;
	}

	zi = ( zip64_internal* )file;

	if ( zi->in_opened_file_inzip == 0 )
	{
		return ZIP_PARAMERROR;
	}

	zi->ci.stream.avail_in = 0;

	if ( ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) )
	{
		while ( err == ZIP_OK )
		{
			uLong uTotalOutBefore;

			if ( zi->ci.stream.avail_out == 0 )
			{
				if ( zip64FlushWriteBuffer( zi ) == ZIP_ERRNO )
				{
					err = ZIP_ERRNO;
				}

				zi->ci.stream.avail_out = ( uInt )Z_BUFSIZE;
				zi->ci.stream.next_out = zi->ci.buffered_data;
			}

			uTotalOutBefore = zi->ci.stream.total_out;
			err = deflate( &zi->ci.stream,  Z_FINISH );
			zi->ci.pos_in_buffered_data += ( uInt )( zi->ci.stream.total_out - uTotalOutBefore ) ;
		}
	}
	else if ( ( zi->ci.method == Z_BZIP2ED ) && ( !zi->ci.raw ) )
	{
#ifdef HAVE_BZIP2
		err = BZ_FINISH_OK;

		while ( err == BZ_FINISH_OK )
		{
			uLong uTotalOutBefore;

			if ( zi->ci.bstream.avail_out == 0 )
			{
				if ( zip64FlushWriteBuffer( zi ) == ZIP_ERRNO )
				{
					err = ZIP_ERRNO;
				}

				zi->ci.bstream.avail_out = ( uInt )Z_BUFSIZE;
				zi->ci.bstream.next_out = ( char* )zi->ci.buffered_data;
			}

			uTotalOutBefore = zi->ci.bstream.total_out_lo32;
			err = BZ2_bzCompress( &zi->ci.bstream,  BZ_FINISH );

			if ( err == BZ_STREAM_END )
			{
				err = Z_STREAM_END;
			}

			zi->ci.pos_in_buffered_data += ( uInt )( zi->ci.bstream.total_out_lo32 - uTotalOutBefore );
		}

		if ( err == BZ_FINISH_OK )
		{
			err = ZIP_OK;
		}

#endif
	}

	if ( err == Z_STREAM_END )
	{
		err = ZIP_OK;   /* this is normal */
	}

	if ( ( zi->ci.pos_in_buffered_data > 0 ) && ( err == ZIP_OK ) )
	{
		if ( zip64FlushWriteBuffer( zi ) == ZIP_ERRNO )
		{
			err = ZIP_ERRNO;
		}
	}

	if ( ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) )
	{
		int tmp_err = deflateEnd( &zi->ci.stream );

		if ( err == ZIP_OK )
		{
			err = tmp_err;
		}

		zi->ci.stream_initialised = 0;
	}

#ifdef HAVE_BZIP2
	else if ( ( zi->ci.method == Z_BZIP2ED ) && ( !zi->ci.raw ) )
	{
		int tmperr = BZ2_bzCompressEnd( &zi->ci.bstream );

		if ( err == ZIP_OK )
		{
			err = tmperr;
		}

		zi->ci.stream_initialised = 0;
	}

#endif

	if ( !zi->ci.raw )
	{
		crc32 = ( uLong )zi->ci.crc32;
		uncompressed_size = zi->ci.totalUncompressedData;
	}

	compressed_size = zi->ci.totalCompressedData;

#    ifndef NOCRYPT
	compressed_size += zi->ci.crypt_header_size;
#    endif

	// update Current Item crc and sizes,
	if ( compressed_size >= 0xffffffff || uncompressed_size >= 0xffffffff || zi->ci.pos_local_header >= 0xffffffff )
	{
		/*version Made by*/
		zip64local_putValue_inmemory( zi->ci.central_header + 4, ( uLong )45, 2 );
		/*version needed*/
		zip64local_putValue_inmemory( zi->ci.central_header + 6, ( uLong )45, 2 );

	}

	zip64local_putValue_inmemory( zi->ci.central_header + 16, crc32, 4 ); /*crc*/


	if ( compressed_size >= 0xffffffff )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 20, invalidValue, 4 );   /*compr size*/
	}
	else
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 20, compressed_size, 4 );   /*compr size*/
	}

	/// set internal file attributes field
	if ( zi->ci.stream.data_type == Z_ASCII )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 36, ( uLong )Z_ASCII, 2 );
	}

	if ( uncompressed_size >= 0xffffffff )
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 24, invalidValue, 4 );   /*uncompr size*/
	}
	else
	{
		zip64local_putValue_inmemory( zi->ci.central_header + 24, uncompressed_size, 4 );   /*uncompr size*/
	}

	// Add ZIP64 extra info field for uncompressed size
	if ( uncompressed_size >= 0xffffffff )
	{
		datasize += 8;
	}

	// Add ZIP64 extra info field for compressed size
	if ( compressed_size >= 0xffffffff )
	{
		datasize += 8;
	}

	// Add ZIP64 extra info field for relative offset to local file header of current file
	if ( zi->ci.pos_local_header >= 0xffffffff )
	{
		datasize += 8;
	}

	if ( datasize > 0 )
	{
		char* p = NULL;

		if ( ( uLong )( datasize + 4 ) > zi->ci.size_centralExtraFree )
		{
			// we can not write more data to the buffer that we have room for.
			return ZIP_BADZIPFILE;
		}

		p = zi->ci.central_header + zi->ci.size_centralheader;

		// Add Extra Information Header for 'ZIP64 information'
		zip64local_putValue_inmemory( p, 0x0001, 2 ); // HeaderID
		p += 2;
		zip64local_putValue_inmemory( p, datasize, 2 ); // DataSize
		p += 2;

		if ( uncompressed_size >= 0xffffffff )
		{
			zip64local_putValue_inmemory( p, uncompressed_size, 8 );
			p += 8;
		}

		if ( compressed_size >= 0xffffffff )
		{
			zip64local_putValue_inmemory( p, compressed_size, 8 );
			p += 8;
		}

		if ( zi->ci.pos_local_header >= 0xffffffff )
		{
			zip64local_putValue_inmemory( p, zi->ci.pos_local_header, 8 );
			p += 8;
		}

		// Update how much extra free space we got in the memory buffer
		// and increase the centralheader size so the new ZIP64 fields are included
		// ( 4 below is the size of HeaderID and DataSize field )
		zi->ci.size_centralExtraFree -= datasize + 4;
		zi->ci.size_centralheader += datasize + 4;

		// Update the extra info size field
		zi->ci.size_centralExtra += datasize + 4;
		zip64local_putValue_inmemory( zi->ci.central_header + 30, ( uLong )zi->ci.size_centralExtra, 2 );
	}

	if ( err == ZIP_OK )
	{
		err = add_data_in_datablock( &zi->central_dir, zi->ci.central_header, ( uLong )zi->ci.size_centralheader );
	}

	free( zi->ci.central_header );

	if ( err == ZIP_OK )
	{
		// Update the LocalFileHeader with the new values.

		ZPOS64_T cur_pos_inzip = ZTELL64( zi->z_filefunc, zi->filestream );

		if ( ZSEEK64( zi->z_filefunc, zi->filestream, zi->ci.pos_local_header + 14, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = ZIP_ERRNO;
		}

		if ( err == ZIP_OK )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, crc32, 4 );   /* crc 32, unknown */
		}

		if ( uncompressed_size >= 0xffffffff )
		{
			if ( zi->ci.pos_zip64extrainfo > 0 )
			{
				// Update the size in the ZIP64 extended field.
				if ( ZSEEK64( zi->z_filefunc, zi->filestream, zi->ci.pos_zip64extrainfo + 4, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
				{
					err = ZIP_ERRNO;
				}

				if ( err == ZIP_OK ) /* compressed size, unknown */
				{
					err = zip64local_putValue( &zi->z_filefunc, zi->filestream, uncompressed_size, 8 );
				}

				if ( err == ZIP_OK ) /* uncompressed size, unknown */
				{
					err = zip64local_putValue( &zi->z_filefunc, zi->filestream, compressed_size, 8 );
				}
			}
		}
		else
		{
			if ( err == ZIP_OK ) /* compressed size, unknown */
			{
				err = zip64local_putValue( &zi->z_filefunc, zi->filestream, compressed_size, 4 );
			}

			if ( err == ZIP_OK ) /* uncompressed size, unknown */
			{
				err = zip64local_putValue( &zi->z_filefunc, zi->filestream, uncompressed_size, 4 );
			}
		}

		if ( ZSEEK64( zi->z_filefunc, zi->filestream, cur_pos_inzip, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = ZIP_ERRNO;
		}
	}

	zi->number_entry ++;
	zi->in_opened_file_inzip = 0;

	return err;
}

extern int ZEXPORT zipCloseFileInZip ( zipFile file )
{
	return zipCloseFileInZipRaw ( file, 0, 0 );
}

int Write_Zip64EndOfCentralDirectoryLocator( zip64_internal* zi, ZPOS64_T zip64eocd_pos_inzip )
{
	int err = ZIP_OK;
	ZPOS64_T pos = zip64eocd_pos_inzip - zi->add_position_when_writting_offset;

	err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )ZIP64ENDLOCHEADERMAGIC, 4 );

	/*num disks*/
	if ( err == ZIP_OK ) /* number of the disk with the start of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );
	}

	/*relative offset*/
	if ( err == ZIP_OK ) /* Relative offset to the Zip64EndOfCentralDirectory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, pos, 8 );
	}

	/*total disks*/ /* Do not support spawning of disk so always say 1 here*/
	if ( err == ZIP_OK ) /* number of the disk with the start of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )1, 4 );
	}

	return err;
}

int Write_Zip64EndOfCentralDirectoryRecord( zip64_internal* zi, uLong size_centraldir, ZPOS64_T centraldir_pos_inzip )
{
	int err = ZIP_OK;

	uLong Zip64DataSize = 44;

	err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )ZIP64ENDHEADERMAGIC, 4 );

	if ( err == ZIP_OK ) /* size of this 'zip64 end of central directory' */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( ZPOS64_T )Zip64DataSize, 8 );   // why ZPOS64_T of this ?
	}

	if ( err == ZIP_OK ) /* version made by */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )45, 2 );
	}

	if ( err == ZIP_OK ) /* version needed */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )45, 2 );
	}

	if ( err == ZIP_OK ) /* number of this disk */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );
	}

	if ( err == ZIP_OK ) /* number of the disk with the start of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 4 );
	}

	if ( err == ZIP_OK ) /* total number of entries in the central dir on this disk */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, zi->number_entry, 8 );
	}

	if ( err == ZIP_OK ) /* total number of entries in the central dir */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, zi->number_entry, 8 );
	}

	if ( err == ZIP_OK ) /* size of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( ZPOS64_T )size_centraldir, 8 );
	}

	if ( err == ZIP_OK ) /* offset of start of central directory with respect to the starting disk number */
	{
		ZPOS64_T pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( ZPOS64_T )pos, 8 );
	}

	return err;
}
int Write_EndOfCentralDirectoryRecord( zip64_internal* zi, uLong size_centraldir, ZPOS64_T centraldir_pos_inzip )
{
	int err = ZIP_OK;

	/*signature*/
	err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )ENDHEADERMAGIC, 4 );

	if ( err == ZIP_OK ) /* number of this disk */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 2 );
	}

	if ( err == ZIP_OK ) /* number of the disk with the start of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0, 2 );
	}

	if ( err == ZIP_OK ) /* total number of entries in the central dir on this disk */
	{
		{
			if ( zi->number_entry >= 0xFFFF )
			{
				err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0xffff, 2 );   // use value in ZIP64 record
			}
			else
			{
				err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )zi->number_entry, 2 );
			}
		}
	}

	if ( err == ZIP_OK ) /* total number of entries in the central dir */
	{
		if ( zi->number_entry >= 0xFFFF )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0xffff, 2 );   // use value in ZIP64 record
		}
		else
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )zi->number_entry, 2 );
		}
	}

	if ( err == ZIP_OK ) /* size of the central directory */
	{
		err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )size_centraldir, 4 );
	}

	if ( err == ZIP_OK ) /* offset of start of central directory with respect to the starting disk number */
	{
		ZPOS64_T pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;

		if ( pos >= 0xffffffff )
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )0xffffffff, 4 );
		}
		else
		{
			err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )( centraldir_pos_inzip - zi->add_position_when_writting_offset ), 4 );
		}
	}

	return err;
}

int Write_GlobalComment( zip64_internal* zi, const char* global_comment )
{
	int err = ZIP_OK;
	uInt size_global_comment = 0;

	if ( global_comment != NULL )
	{
		size_global_comment = ( uInt )strlen( global_comment );
	}

	err = zip64local_putValue( &zi->z_filefunc, zi->filestream, ( uLong )size_global_comment, 2 );

	if ( err == ZIP_OK && size_global_comment > 0 )
	{
		if ( ZWRITE64( zi->z_filefunc, zi->filestream, global_comment, size_global_comment ) != size_global_comment )
		{
			err = ZIP_ERRNO;
		}
	}

	return err;
}

extern int ZEXPORT zipClose ( zipFile file, const char* global_comment )
{
	zip64_internal* zi;
	int err = 0;
	uLong size_centraldir = 0;
	ZPOS64_T centraldir_pos_inzip;
	ZPOS64_T pos;

	if ( file == NULL )
	{
		return ZIP_PARAMERROR;
	}

	zi = ( zip64_internal* )file;

	if ( zi->in_opened_file_inzip == 1 )
	{
		err = zipCloseFileInZip ( file );
	}

#ifndef NO_ADDFILEINEXISTINGZIP

	if ( global_comment == NULL )
	{
		global_comment = zi->globalcomment;
	}

#endif

	centraldir_pos_inzip = ZTELL64( zi->z_filefunc, zi->filestream );

	if ( err == ZIP_OK )
	{
		linkedlist_datablock_internal* ldi = zi->central_dir.first_block;

		while ( ldi != NULL )
		{
			if ( ( err == ZIP_OK ) && ( ldi->filled_in_this_block > 0 ) )
			{
				if ( ZWRITE64( zi->z_filefunc, zi->filestream, ldi->data, ldi->filled_in_this_block ) != ldi->filled_in_this_block )
				{
					err = ZIP_ERRNO;
				}
			}

			size_centraldir += ldi->filled_in_this_block;
			ldi = ldi->next_datablock;
		}
	}

	free_linkedlist( &( zi->central_dir ) );

	pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;

	if ( pos >= 0xffffffff )
	{
		ZPOS64_T Zip64EOCDpos = ZTELL64( zi->z_filefunc, zi->filestream );
		Write_Zip64EndOfCentralDirectoryRecord( zi, size_centraldir, centraldir_pos_inzip );

		Write_Zip64EndOfCentralDirectoryLocator( zi, Zip64EOCDpos );
	}

	if ( err == ZIP_OK )
	{
		err = Write_EndOfCentralDirectoryRecord( zi, size_centraldir, centraldir_pos_inzip );
	}

	if ( err == ZIP_OK )
	{
		err = Write_GlobalComment( zi, global_comment );
	}

	if ( ZCLOSE64( zi->z_filefunc, zi->filestream ) != 0 )
		if ( err == ZIP_OK )
		{
			err = ZIP_ERRNO;
		}

#ifndef NO_ADDFILEINEXISTINGZIP
	TRYFREE( zi->globalcomment );
#endif
	TRYFREE( zi );

	return err;
}

extern int ZEXPORT zipRemoveExtraInfoBlock ( char* pData, int* dataLen, short sHeader )
{
	char* p = pData;
	int size = 0;
	char* pNewHeader;
	char* pTmp;
	short header;
	short dataSize;

	int retVal = ZIP_OK;

	if ( pData == NULL || *dataLen < 4 )
	{
		return ZIP_PARAMERROR;
	}

	pNewHeader = ( char* )ALLOC( *dataLen );
	pTmp = pNewHeader;

	while ( p < ( pData + *dataLen ) )
	{
		header = *( short* )p;
		dataSize = *( ( ( short* )p ) + 1 );

		if ( header == sHeader ) // Header found.
		{
			p += dataSize + 4; // skip it. do not copy to temp buffer
		}
		else
		{
			// Extra Info block should not be removed, So copy it to the temp buffer.
			memcpy( pTmp, p, dataSize + 4 );
			p += dataSize + 4;
			size += dataSize + 4;
		}

	}

	if ( size < *dataLen )
	{
		// clean old extra info block.
		memset( pData, 0, *dataLen );

		// copy the new extra info block over the old
		if ( size > 0 )
		{
			memcpy( pData, pNewHeader, size );
		}

		// set the new extra info size
		*dataLen = size;

		retVal = ZIP_OK;
	}
	else
	{
		retVal = ZIP_ERRNO;
	}

	TRYFREE( pNewHeader );

	return retVal;
}

/////// unzip.c

/* unzip.c -- IO for uncompress .zip files using zlib
   Version 1.1, February 14h, 2010
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications of Unzip for Zip64
         Copyright (C) 2007-2008 Even Rouault

         Modifications for Zip64 support on both zip and unzip
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt


  ------------------------------------------------------------------------------------
  Decryption code comes from crypt.c by Info-ZIP but has been greatly reduced in terms of
  compatibility with older software. The following is from the original crypt.c.
  Code woven in by Terry Thorsen 1/2003.

  Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html

        crypt.c (full version) by Info-ZIP.      Last revised:  [see crypt.h]

  The encryption/decryption parts of this source code (as opposed to the
  non-echoing password parts) were originally written in Europe.  The
  whole source package can be freely distributed, including from the USA.
  (Prior to January 2000, re-export from the US was a violation of US law.)

        This encryption code is a direct transcription of the algorithm from
  Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
  file (appnote.txt) is distributed with the PKZIP program (even in the
  version without encryption capabilities).

        ------------------------------------------------------------------------------------

        Changes in unzip.c

        2007-2008 - Even Rouault - Addition of cpl_unzGetCurrentFileZStreamPos
  2007-2008 - Even Rouault - Decoration of symbol names unz* -> cpl_unz*
  2007-2008 - Even Rouault - Remove old C style function prototypes
  2007-2008 - Even Rouault - Add unzip support for ZIP64

        Copyright (C) 2007-2008 Even Rouault


        Oct-2009 - Mathias Svensson - Removed cpl_* from symbol names (Even Rouault added them but since this is now moved to a new project (minizip64) I renamed them again).
  Oct-2009 - Mathias Svensson - Fixed problem if uncompressed size was > 4G and compressed size was <4G
                                should only read the compressed/uncompressed size from the Zip64 format if
                                the size from normal header was 0xFFFFFFFF
  Oct-2009 - Mathias Svensson - Applied some bug fixes from paches recived from Gilles Vollant
        Oct-2009 - Mathias Svensson - Applied support to unzip files with compression mathod BZIP2 (bzip2 lib is required)
                                Patch created by Daniel Borca

  Jan-2010 - back to unzip and minizip 1.0 name scheme, with compatibility layer

  Copyright (C) 1998 - 2010 Gilles Vollant, Even Rouault, Mathias Svensson

*/

#ifndef local
#  define local static
#endif
/* compile with -Dlocal if your debugger can't find static symbols */


#ifndef CASESENSITIVITYDEFAULT_NO
#  if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES)
#    define CASESENSITIVITYDEFAULT_NO
#  endif
#endif


#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (16384)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
# define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) free(p);}
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)


const char unz_copyright[] =
   " unzip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";

/* unz_file_info_interntal contain internal info about a file in zipfile*/
typedef struct unz_file_info64_internal_s
{
	ZPOS64_T offset_curfile;/* relative offset of local header 8 bytes */
} unz_file_info64_internal;


/* file_in_zip_read_info_s contain internal information about a file in zipfile,
    when reading and decompress it */
typedef struct
{
	char*  read_buffer;         /* internal buffer for compressed data */
	z_stream stream;            /* zLib stream structure for inflate */

#ifdef HAVE_BZIP2
	bz_stream bstream;          /* bzLib stream structure for bziped */
#endif

	ZPOS64_T pos_in_zipfile;       /* position in byte on the zipfile, for fseek*/
	uLong stream_initialised;   /* flag set if stream structure is initialised*/

	ZPOS64_T offset_local_extrafield;/* offset of the local extra field */
	uInt  size_local_extrafield;/* size of the local extra field */
	ZPOS64_T pos_local_extrafield;   /* position in the local extra field in read*/
	ZPOS64_T total_out_64;

	uLong crc32;                /* crc32 of all data uncompressed */
	uLong crc32_wait;           /* crc32 we must obtain after decompress all */
	ZPOS64_T rest_read_compressed; /* number of byte to be decompressed */
	ZPOS64_T rest_read_uncompressed;/*number of byte to be obtained after decomp*/
	zlib_filefunc64_32_def z_filefunc;
	voidpf filestream;        /* io structore of the zipfile */
	uLong compression_method;   /* compression method (0==store) */
	ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
	int   raw;
} file_in_zip64_read_info_s;


/* unz64_s contain internal information about the zipfile
*/
typedef struct
{
	zlib_filefunc64_32_def z_filefunc;
	int is64bitOpenFunction;
	voidpf filestream;        /* io structore of the zipfile */
	unz_global_info64 gi;       /* public global information */
	ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
	ZPOS64_T num_file;             /* number of the current file in the zipfile*/
	ZPOS64_T pos_in_central_dir;   /* pos of the current file in the central dir*/
	ZPOS64_T current_file_ok;      /* flag about the usability of the current file*/
	ZPOS64_T central_pos;          /* position of the beginning of the central dir*/

	ZPOS64_T size_central_dir;     /* size of the central directory  */
	ZPOS64_T offset_central_dir;   /* offset of start of central directory with
                                   respect to the starting disk number */

	unz_file_info64 cur_file_info; /* public info about the current file in zip*/
	unz_file_info64_internal cur_file_info_internal; /* private info about it*/
	file_in_zip64_read_info_s* pfile_in_zip_read; /* structure about the current
                                        file if we are decompressing it */
	int encrypted;

	int isZip64;

#    ifndef NOUNCRYPT
	unsigned long keys[3];     /* keys defining the pseudo-random sequence */
	const unsigned long* pcrc_32_tab;
#    endif
} unz64_s;


#ifndef NOUNCRYPT
//#include "crypt.h"
#endif

/* ===========================================================================
     Read a byte from a gz_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/


local int unz64local_getByte OF( (
                                    const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                    voidpf filestream,
                                    int* pi ) );

local int unz64local_getByte( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream, int* pi )
{
	unsigned char c;
	int err = ( int )ZREAD64( *pzlib_filefunc_def, filestream, &c, 1 );

	if ( err == 1 )
	{
		*pi = ( int )c;
		return UNZ_OK;
	}
	else
	{
		if ( ZERROR64( *pzlib_filefunc_def, filestream ) )
		{
			return UNZ_ERRNO;
		}
		else
		{
			return UNZ_EOF;
		}
	}
}


/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets
*/
local int unz64local_getShort OF( (
                                     const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                     voidpf filestream,
                                     uLong* pX ) );

local int unz64local_getShort ( const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                voidpf filestream,
                                uLong* pX )
{
	uLong x ;
	int i = 0;
	int err;

	err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( uLong )i;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( uLong )i ) << 8;

	if ( err == UNZ_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

local int unz64local_getLong OF( (
                                    const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                    voidpf filestream,
                                    uLong* pX ) );

local int unz64local_getLong ( const zlib_filefunc64_32_def* pzlib_filefunc_def,
                               voidpf filestream,
                               uLong* pX )
{
	uLong x ;
	int i = 0;
	int err;

	err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( uLong )i;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( uLong )i ) << 8;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( uLong )i ) << 16;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x += ( ( uLong )i ) << 24;

	if ( err == UNZ_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

local int unz64local_getLong64 OF( (
                                      const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                      voidpf filestream,
                                      ZPOS64_T* pX ) );


local int unz64local_getLong64 ( const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                 voidpf filestream,
                                 ZPOS64_T* pX )
{
	ZPOS64_T x ;
	int i = 0;
	int err;

	err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	x = ( ZPOS64_T )i;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 8;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 16;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 24;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 32;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 40;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 48;

	if ( err == UNZ_OK )
	{
		err = unz64local_getByte( pzlib_filefunc_def, filestream, &i );
	}

	x |= ( ( ZPOS64_T )i ) << 56;

	if ( err == UNZ_OK )
	{
		*pX = x;
	}
	else
	{
		*pX = 0;
	}

	return err;
}

/* My own strcmpi / strcasecmp */
local int strcmpcasenosensitive_internal ( const char* fileName1, const char* fileName2 )
{
	for ( ;; )
	{
		char c1 = *( fileName1++ );
		char c2 = *( fileName2++ );

		if ( ( c1 >= 'a' ) && ( c1 <= 'z' ) )
		{
			c1 -= 0x20;
		}

		if ( ( c2 >= 'a' ) && ( c2 <= 'z' ) )
		{
			c2 -= 0x20;
		}

		if ( c1 == '\0' )
		{
			return ( ( c2 == '\0' ) ? 0 : -1 );
		}

		if ( c2 == '\0' )
		{
			return 1;
		}

		if ( c1 < c2 )
		{
			return -1;
		}

		if ( c1 > c2 )
		{
			return 1;
		}
	}
}


#ifdef  CASESENSITIVITYDEFAULT_NO
#define CASESENSITIVITYDEFAULTVALUE 2
#else
#define CASESENSITIVITYDEFAULTVALUE 1
#endif

#ifndef STRCMPCASENOSENTIVEFUNCTION
#define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
#endif

/*
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
                                                                or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
        (like 1 on Unix, 2 on Windows)

*/
extern int ZEXPORT unzStringFileNameCompare ( const char*  fileName1,
                                              const char*  fileName2,
                                              int iCaseSensitivity )

{
	if ( iCaseSensitivity == 0 )
	{
		iCaseSensitivity = CASESENSITIVITYDEFAULTVALUE;
	}

	if ( iCaseSensitivity == 1 )
	{
		return strcmp( fileName1, fileName2 );
	}

	return STRCMPCASENOSENTIVEFUNCTION( fileName1, fileName2 );
}

#ifndef BUFREADCOMMENT
#define BUFREADCOMMENT (0x400)
#endif

/*
  Locate the Central directory of a zipfile (at the end, just before
    the global comment)
*/
local ZPOS64_T unz64local_SearchCentralDir OF( ( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream ) );
local ZPOS64_T unz64local_SearchCentralDir( const zlib_filefunc64_32_def* pzlib_filefunc_def, voidpf filestream )
{
	unsigned char* buf;
	ZPOS64_T uSizeFile;
	ZPOS64_T uBackRead;
	ZPOS64_T uMaxBack = 0xffff; /* maximum size of global comment */
	ZPOS64_T uPosFound = 0;

	if ( ZSEEK64( *pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END ) != 0 )
	{
		return 0;
	}


	uSizeFile = ZTELL64( *pzlib_filefunc_def, filestream );

	if ( uMaxBack > uSizeFile )
	{
		uMaxBack = uSizeFile;
	}

	buf = ( unsigned char* )ALLOC( BUFREADCOMMENT + 4 );

	if ( buf == NULL )
	{
		return 0;
	}

	uBackRead = 4;

	while ( uBackRead < uMaxBack )
	{
		uLong uReadSize;
		ZPOS64_T uReadPos ;
		int i;

		if ( uBackRead + BUFREADCOMMENT > uMaxBack )
		{
			uBackRead = uMaxBack;
		}
		else
		{
			uBackRead += BUFREADCOMMENT;
		}

		uReadPos = uSizeFile - uBackRead ;

		uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
		            ( BUFREADCOMMENT + 4 ) : ( uLong )( uSizeFile - uReadPos );

		if ( ZSEEK64( *pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			break;
		}

		if ( ZREAD64( *pzlib_filefunc_def, filestream, buf, uReadSize ) != uReadSize )
		{
			break;
		}

		for ( i = ( int )uReadSize - 3; ( i-- ) > 0; )
			if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) &&
			     ( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) )
			{
				uPosFound = uReadPos + i;
				break;
			}

		if ( uPosFound != 0 )
		{
			break;
		}
	}

	TRYFREE( buf );
	return uPosFound;
}


/*
  Locate the Central directory 64 of a zipfile (at the end, just before
    the global comment)
*/
local ZPOS64_T unz64local_SearchCentralDir64 OF( (
                                                    const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                                    voidpf filestream ) );

local ZPOS64_T unz64local_SearchCentralDir64( const zlib_filefunc64_32_def* pzlib_filefunc_def,
                                              voidpf filestream )
{
	unsigned char* buf;
	ZPOS64_T uSizeFile;
	ZPOS64_T uBackRead;
	ZPOS64_T uMaxBack = 0xffff; /* maximum size of global comment */
	ZPOS64_T uPosFound = 0;
	uLong uL;
	ZPOS64_T relativeOffset;

	if ( ZSEEK64( *pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END ) != 0 )
	{
		return 0;
	}


	uSizeFile = ZTELL64( *pzlib_filefunc_def, filestream );

	if ( uMaxBack > uSizeFile )
	{
		uMaxBack = uSizeFile;
	}

	buf = ( unsigned char* )ALLOC( BUFREADCOMMENT + 4 );

	if ( buf == NULL )
	{
		return 0;
	}

	uBackRead = 4;

	while ( uBackRead < uMaxBack )
	{
		uLong uReadSize;
		ZPOS64_T uReadPos;
		int i;

		if ( uBackRead + BUFREADCOMMENT > uMaxBack )
		{
			uBackRead = uMaxBack;
		}
		else
		{
			uBackRead += BUFREADCOMMENT;
		}

		uReadPos = uSizeFile - uBackRead ;

		uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
		            ( BUFREADCOMMENT + 4 ) : ( uLong )( uSizeFile - uReadPos );

		if ( ZSEEK64( *pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			break;
		}

		if ( ZREAD64( *pzlib_filefunc_def, filestream, buf, uReadSize ) != uReadSize )
		{
			break;
		}

		for ( i = ( int )uReadSize - 3; ( i-- ) > 0; )
			if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) &&
			     ( ( *( buf + i + 2 ) ) == 0x06 ) && ( ( *( buf + i + 3 ) ) == 0x07 ) )
			{
				uPosFound = uReadPos + i;
				break;
			}

		if ( uPosFound != 0 )
		{
			break;
		}
	}

	TRYFREE( buf );

	if ( uPosFound == 0 )
	{
		return 0;
	}

	/* Zip64 end of central directory locator */
	if ( ZSEEK64( *pzlib_filefunc_def, filestream, uPosFound, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return 0;
	}

	/* the signature, already checked */
	if ( unz64local_getLong( pzlib_filefunc_def, filestream, &uL ) != UNZ_OK )
	{
		return 0;
	}

	/* number of the disk with the start of the zip64 end of  central directory */
	if ( unz64local_getLong( pzlib_filefunc_def, filestream, &uL ) != UNZ_OK )
	{
		return 0;
	}

	if ( uL != 0 )
	{
		return 0;
	}

	/* relative offset of the zip64 end of central directory record */
	if ( unz64local_getLong64( pzlib_filefunc_def, filestream, &relativeOffset ) != UNZ_OK )
	{
		return 0;
	}

	/* total number of disks */
	if ( unz64local_getLong( pzlib_filefunc_def, filestream, &uL ) != UNZ_OK )
	{
		return 0;
	}

	if ( uL != 1 )
	{
		return 0;
	}

	/* Goto end of central directory record */
	if ( ZSEEK64( *pzlib_filefunc_def, filestream, relativeOffset, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return 0;
	}

	/* the signature */
	if ( unz64local_getLong( pzlib_filefunc_def, filestream, &uL ) != UNZ_OK )
	{
		return 0;
	}

	if ( uL != 0x06064b50 )
	{
		return 0;
	}

	return relativeOffset;
}

/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\test\\zlib114.zip" or on an Unix computer
     "zlib/zlib114.zip".
     If the zipfile cannot be opened (file doesn't exist or in not valid), the
       return value is NULL.
     Else, the return value is a unzFile Handle, usable with other function
       of this unzip package.
*/
local unzFile unzOpenInternal ( const void* path,
                                zlib_filefunc64_32_def* pzlib_filefunc64_32_def,
                                int is64bitOpenFunction )
{
	unz64_s us;
	unz64_s* s;
	ZPOS64_T central_pos;
	uLong   uL;

	uLong number_disk;          /* number of the current dist, used for
                                   spaning ZIP, unsupported, always 0*/
	uLong number_disk_with_CD;  /* number the the disk with central dir, used
                                   for spaning ZIP, unsupported, always 0*/
	ZPOS64_T number_entry_CD;      /* total number of entries in
                                   the central dir
                                   (same than number_entry on nospan) */

	int err = UNZ_OK;

	if ( unz_copyright[0] != ' ' )
	{
		return NULL;
	}

	us.z_filefunc.zseek32_file = NULL;
	us.z_filefunc.ztell32_file = NULL;

	if ( pzlib_filefunc64_32_def == NULL )
		/*fill_fopen64_filefunc(&us.z_filefunc.zfile_func64)*/ {}
	else
	{
		us.z_filefunc = *pzlib_filefunc64_32_def;
	}

	us.is64bitOpenFunction = is64bitOpenFunction;



	us.filestream = ZOPEN64( us.z_filefunc,
	                         path,
	                         ZLIB_FILEFUNC_MODE_READ |
	                         ZLIB_FILEFUNC_MODE_EXISTING );

	if ( us.filestream == NULL )
	{
		return NULL;
	}

	central_pos = unz64local_SearchCentralDir64( &us.z_filefunc, us.filestream );

	if ( central_pos )
	{
		uLong uS;
		ZPOS64_T uL64;

		us.isZip64 = 1;

		if ( ZSEEK64( us.z_filefunc, us.filestream,
		              central_pos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = UNZ_ERRNO;
		}

		/* the signature, already checked */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* size of zip64 end of central directory record */
		if ( unz64local_getLong64( &us.z_filefunc, us.filestream, &uL64 ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* version made by */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &uS ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* version needed to extract */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &uS ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* number of this disk */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &number_disk ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* number of the disk with the start of the central directory */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &number_disk_with_CD ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* total number of entries in the central directory on this disk */
		if ( unz64local_getLong64( &us.z_filefunc, us.filestream, &us.gi.number_entry ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* total number of entries in the central directory */
		if ( unz64local_getLong64( &us.z_filefunc, us.filestream, &number_entry_CD ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		if ( ( number_entry_CD != us.gi.number_entry ) ||
		     ( number_disk_with_CD != 0 ) ||
		     ( number_disk != 0 ) )
		{
			err = UNZ_BADZIPFILE;
		}

		/* size of the central directory */
		if ( unz64local_getLong64( &us.z_filefunc, us.filestream, &us.size_central_dir ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* offset of start of central directory with respect to the
		  starting disk number */
		if ( unz64local_getLong64( &us.z_filefunc, us.filestream, &us.offset_central_dir ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		us.gi.size_comment = 0;
	}
	else
	{
		central_pos = unz64local_SearchCentralDir( &us.z_filefunc, us.filestream );

		if ( central_pos == 0 )
		{
			err = UNZ_ERRNO;
		}

		us.isZip64 = 0;

		if ( ZSEEK64( us.z_filefunc, us.filestream,
		              central_pos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
		{
			err = UNZ_ERRNO;
		}

		/* the signature, already checked */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* number of this disk */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &number_disk ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* number of the disk with the start of the central directory */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &number_disk_with_CD ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		/* total number of entries in the central dir on this disk */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		us.gi.number_entry = uL;

		/* total number of entries in the central dir */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		number_entry_CD = uL;

		if ( ( number_entry_CD != us.gi.number_entry ) ||
		     ( number_disk_with_CD != 0 ) ||
		     ( number_disk != 0 ) )
		{
			err = UNZ_BADZIPFILE;
		}

		/* size of the central directory */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		us.size_central_dir = uL;

		/* offset of start of central directory with respect to the
		    starting disk number */
		if ( unz64local_getLong( &us.z_filefunc, us.filestream, &uL ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}

		us.offset_central_dir = uL;

		/* zipfile comment length */
		if ( unz64local_getShort( &us.z_filefunc, us.filestream, &us.gi.size_comment ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}
	}

	if ( ( central_pos < us.offset_central_dir + us.size_central_dir ) &&
	     ( err == UNZ_OK ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( err != UNZ_OK )
	{
		ZCLOSE64( us.z_filefunc, us.filestream );
		return NULL;
	}

	us.byte_before_the_zipfile = central_pos -
	                             ( us.offset_central_dir + us.size_central_dir );
	us.central_pos = central_pos;
	us.pfile_in_zip_read = NULL;
	us.encrypted = 0;


	s = ( unz64_s* )ALLOC( sizeof( unz64_s ) );

	if ( s != NULL )
	{
		*s = us;
		unzGoToFirstFile( ( unzFile )s );
	}

	return ( unzFile )s;
}


extern unzFile ZEXPORT unzOpen2 ( const char* path,
                                  zlib_filefunc_def* pzlib_filefunc32_def )
{
	if ( pzlib_filefunc32_def != NULL )
	{
		zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
		fill_zlib_filefunc64_32_def_from_filefunc32( &zlib_filefunc64_32_def_fill, pzlib_filefunc32_def );
		return unzOpenInternal( path, &zlib_filefunc64_32_def_fill, 0 );
	}
	else
	{
		return unzOpenInternal( path, NULL, 0 );
	}
}

extern unzFile ZEXPORT unzOpen2_64 ( const void* path,
                                     zlib_filefunc64_def* pzlib_filefunc_def )
{
	if ( pzlib_filefunc_def != NULL )
	{
		zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
		zlib_filefunc64_32_def_fill.zfile_func64 = *pzlib_filefunc_def;
		zlib_filefunc64_32_def_fill.ztell32_file = NULL;
		zlib_filefunc64_32_def_fill.zseek32_file = NULL;
		return unzOpenInternal( path, &zlib_filefunc64_32_def_fill, 1 );
	}
	else
	{
		return unzOpenInternal( path, NULL, 1 );
	}
}

extern unzFile ZEXPORT unzOpen ( const char* path )
{
	return unzOpenInternal( path, NULL, 0 );
}

extern unzFile ZEXPORT unzOpen64 ( const void* path )
{
	return unzOpenInternal( path, NULL, 1 );
}

/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with unzipOpenCurrentFile (see later),
    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */
extern int ZEXPORT unzClose ( unzFile file )
{
	unz64_s* s;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( s->pfile_in_zip_read != NULL )
	{
		unzCloseCurrentFile( file );
	}

	ZCLOSE64( s->z_filefunc, s->filestream );
	TRYFREE( s );
	return UNZ_OK;
}


/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */
extern int ZEXPORT unzGetGlobalInfo64 ( unzFile file, unz_global_info64* pglobal_info )
{
	unz64_s* s;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	*pglobal_info = s->gi;
	return UNZ_OK;
}

extern int ZEXPORT unzGetGlobalInfo ( unzFile file, unz_global_info* pglobal_info32 )
{
	unz64_s* s;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	/* to do : check if number_entry is not truncated */
	pglobal_info32->number_entry = ( uLong )s->gi.number_entry;
	pglobal_info32->size_comment = s->gi.size_comment;
	return UNZ_OK;
}
/*
   Translate date/time from Dos format to tm_unz (readable more easilty)
*/
local void unz64local_DosDateToTmuDate ( ZPOS64_T ulDosDate, tm_unz* ptm )
{
	ZPOS64_T uDate;
	uDate = ( ZPOS64_T )( ulDosDate >> 16 );
	ptm->tm_mday = ( uInt )( uDate & 0x1f ) ;
	ptm->tm_mon =  ( uInt )( ( ( ( uDate ) & 0x1E0 ) / 0x20 ) - 1 ) ;
	ptm->tm_year = ( uInt )( ( ( uDate & 0x0FE00 ) / 0x0200 ) + 1980 ) ;

	ptm->tm_hour = ( uInt ) ( ( ulDosDate & 0xF800 ) / 0x800 );
	ptm->tm_min =  ( uInt ) ( ( ulDosDate & 0x7E0 ) / 0x20 ) ;
	ptm->tm_sec =  ( uInt ) ( 2 * ( ulDosDate & 0x1f ) ) ;
}

/*
  Get Info about the current file in the zipfile, with internal only info
*/
local int unz64local_GetCurrentFileInfoInternal OF( ( unzFile file,
                                                      unz_file_info64* pfile_info,
                                                      unz_file_info64_internal
                                                      *pfile_info_internal,
                                                      char* szFileName,
                                                      uLong fileNameBufferSize,
                                                      void* extraField,
                                                      uLong extraFieldBufferSize,
                                                      char* szComment,
                                                      uLong commentBufferSize ) );

local int unz64local_GetCurrentFileInfoInternal ( unzFile file,
                                                  unz_file_info64* pfile_info,
                                                  unz_file_info64_internal
                                                  *pfile_info_internal,
                                                  char* szFileName,
                                                  uLong fileNameBufferSize,
                                                  void* extraField,
                                                  uLong extraFieldBufferSize,
                                                  char* szComment,
                                                  uLong commentBufferSize )
{
	unz64_s* s;
	unz_file_info64 file_info;
	unz_file_info64_internal file_info_internal;
	int err = UNZ_OK;
	uLong uMagic;
	long lSeek = 0;
	uLong uL;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( ZSEEK64( s->z_filefunc, s->filestream,
	              s->pos_in_central_dir + s->byte_before_the_zipfile,
	              ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		err = UNZ_ERRNO;
	}


	/* we check the magic */
	if ( err == UNZ_OK )
	{
		if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uMagic ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}
		else if ( uMagic != 0x02014b50 )
		{
			err = UNZ_BADZIPFILE;
		}
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.version ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.version_needed ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.flag ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.compression_method ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &file_info.dosDate ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	unz64local_DosDateToTmuDate( file_info.dosDate, &file_info.tmu_date );

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &file_info.crc ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uL ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	file_info.compressed_size = uL;

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uL ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	file_info.uncompressed_size = uL;

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.size_filename ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.size_file_extra ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.size_file_comment ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.disk_num_start ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &file_info.internal_fa ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &file_info.external_fa ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	// relative offset of local header
	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uL ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	file_info_internal.offset_curfile = uL;

	lSeek += file_info.size_filename;

	if ( ( err == UNZ_OK ) && ( szFileName != NULL ) )
	{
		uLong uSizeRead ;

		if ( file_info.size_filename < fileNameBufferSize )
		{
			*( szFileName + file_info.size_filename ) = '\0';
			uSizeRead = file_info.size_filename;
		}
		else
		{
			uSizeRead = fileNameBufferSize;
		}

		if ( ( file_info.size_filename > 0 ) && ( fileNameBufferSize > 0 ) )
			if ( ZREAD64( s->z_filefunc, s->filestream, szFileName, uSizeRead ) != uSizeRead )
			{
				err = UNZ_ERRNO;
			}

		lSeek -= uSizeRead;
	}

	// Read extrafield
	if ( ( err == UNZ_OK ) && ( extraField != NULL ) )
	{
		ZPOS64_T uSizeRead ;

		if ( file_info.size_file_extra < extraFieldBufferSize )
		{
			uSizeRead = file_info.size_file_extra;
		}
		else
		{
			uSizeRead = extraFieldBufferSize;
		}

		if ( lSeek != 0 )
		{
			if ( ZSEEK64( s->z_filefunc, s->filestream, lSeek, ZLIB_FILEFUNC_SEEK_CUR ) == 0 )
			{
				lSeek = 0;
			}
			else
			{
				err = UNZ_ERRNO;
			}
		}

		if ( ( file_info.size_file_extra > 0 ) && ( extraFieldBufferSize > 0 ) )
			if ( ZREAD64( s->z_filefunc, s->filestream, extraField, ( uLong )uSizeRead ) != uSizeRead )
			{
				err = UNZ_ERRNO;
			}

		lSeek += file_info.size_file_extra - ( uLong )uSizeRead;
	}
	else
	{
		lSeek += file_info.size_file_extra;
	}


	if ( ( err == UNZ_OK ) && ( file_info.size_file_extra != 0 ) )
	{
		uLong acc = 0;

		// since lSeek now points to after the extra field we need to move back
		lSeek -= file_info.size_file_extra;

		if ( lSeek != 0 )
		{
			if ( ZSEEK64( s->z_filefunc, s->filestream, lSeek, ZLIB_FILEFUNC_SEEK_CUR ) == 0 )
			{
				lSeek = 0;
			}
			else
			{
				err = UNZ_ERRNO;
			}
		}

		while ( acc < file_info.size_file_extra )
		{
			uLong headerId;
			uLong dataSize;

			if ( unz64local_getShort( &s->z_filefunc, s->filestream, &headerId ) != UNZ_OK )
			{
				err = UNZ_ERRNO;
			}

			if ( unz64local_getShort( &s->z_filefunc, s->filestream, &dataSize ) != UNZ_OK )
			{
				err = UNZ_ERRNO;
			}

			/* ZIP64 extra fields */
			if ( headerId == 0x0001 )
			{
				uLong uL;

				if ( file_info.uncompressed_size == ( ZPOS64_T )( unsigned long ) - 1 )
				{
					if ( unz64local_getLong64( &s->z_filefunc, s->filestream, &file_info.uncompressed_size ) != UNZ_OK )
					{
						err = UNZ_ERRNO;
					}
				}

				if ( file_info.compressed_size == ( ZPOS64_T )( unsigned long ) - 1 )
				{
					if ( unz64local_getLong64( &s->z_filefunc, s->filestream, &file_info.compressed_size ) != UNZ_OK )
					{
						err = UNZ_ERRNO;
					}
				}

				if ( file_info_internal.offset_curfile == ( ZPOS64_T )( unsigned long ) - 1 )
				{
					/* Relative Header offset */
					if ( unz64local_getLong64( &s->z_filefunc, s->filestream, &file_info_internal.offset_curfile ) != UNZ_OK )
					{
						err = UNZ_ERRNO;
					}
				}

				if ( file_info.disk_num_start == ( unsigned long ) - 1 )
				{
					/* Disk Start Number */
					if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uL ) != UNZ_OK )
					{
						err = UNZ_ERRNO;
					}
				}

			}
			else
			{
				if ( ZSEEK64( s->z_filefunc, s->filestream, dataSize, ZLIB_FILEFUNC_SEEK_CUR ) != 0 )
				{
					err = UNZ_ERRNO;
				}
			}

			acc += 2 + 2 + dataSize;
		}
	}

	if ( ( err == UNZ_OK ) && ( szComment != NULL ) )
	{
		uLong uSizeRead ;

		if ( file_info.size_file_comment < commentBufferSize )
		{
			*( szComment + file_info.size_file_comment ) = '\0';
			uSizeRead = file_info.size_file_comment;
		}
		else
		{
			uSizeRead = commentBufferSize;
		}

		if ( lSeek != 0 )
		{
			if ( ZSEEK64( s->z_filefunc, s->filestream, lSeek, ZLIB_FILEFUNC_SEEK_CUR ) == 0 )
			{
				lSeek = 0;
			}
			else
			{
				err = UNZ_ERRNO;
			}
		}

		if ( ( file_info.size_file_comment > 0 ) && ( commentBufferSize > 0 ) )
			if ( ZREAD64( s->z_filefunc, s->filestream, szComment, uSizeRead ) != uSizeRead )
			{
				err = UNZ_ERRNO;
			}

		lSeek += file_info.size_file_comment - uSizeRead;
	}
	else
	{
		lSeek += file_info.size_file_comment;
	}


	if ( ( err == UNZ_OK ) && ( pfile_info != NULL ) )
	{
		*pfile_info = file_info;
	}

	if ( ( err == UNZ_OK ) && ( pfile_info_internal != NULL ) )
	{
		*pfile_info_internal = file_info_internal;
	}

	return err;
}



/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem.
*/
extern int ZEXPORT unzGetCurrentFileInfo64 ( unzFile file,
                                             unz_file_info64* pfile_info,
                                             char* szFileName, uLong fileNameBufferSize,
                                             void* extraField, uLong extraFieldBufferSize,
                                             char* szComment,  uLong commentBufferSize )
{
	return unz64local_GetCurrentFileInfoInternal( file, pfile_info, NULL,
	                                              szFileName, fileNameBufferSize,
	                                              extraField, extraFieldBufferSize,
	                                              szComment, commentBufferSize );
}

extern int ZEXPORT unzGetCurrentFileInfo ( unzFile file,
                                           unz_file_info* pfile_info,
                                           char* szFileName, uLong fileNameBufferSize,
                                           void* extraField, uLong extraFieldBufferSize,
                                           char* szComment,  uLong commentBufferSize )
{
	int err;
	unz_file_info64 file_info64;
	err = unz64local_GetCurrentFileInfoInternal( file, &file_info64, NULL,
	                                             szFileName, fileNameBufferSize,
	                                             extraField, extraFieldBufferSize,
	                                             szComment, commentBufferSize );

	if ( err == UNZ_OK )
	{
		pfile_info->version = file_info64.version;
		pfile_info->version_needed = file_info64.version_needed;
		pfile_info->flag = file_info64.flag;
		pfile_info->compression_method = file_info64.compression_method;
		pfile_info->dosDate = file_info64.dosDate;
		pfile_info->crc = file_info64.crc;

		pfile_info->size_filename = file_info64.size_filename;
		pfile_info->size_file_extra = file_info64.size_file_extra;
		pfile_info->size_file_comment = file_info64.size_file_comment;

		pfile_info->disk_num_start = file_info64.disk_num_start;
		pfile_info->internal_fa = file_info64.internal_fa;
		pfile_info->external_fa = file_info64.external_fa;

		pfile_info->tmu_date = file_info64.tmu_date,


		            pfile_info->compressed_size = ( uLong )file_info64.compressed_size;
		pfile_info->uncompressed_size = ( uLong )file_info64.uncompressed_size;

	}

	return err;
}
/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/
extern int ZEXPORT unzGoToFirstFile ( unzFile file )
{
	int err = UNZ_OK;
	unz64_s* s;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	s->pos_in_central_dir = s->offset_central_dir;
	s->num_file = 0;
	err = unz64local_GetCurrentFileInfoInternal( file, &s->cur_file_info,
	                                             &s->cur_file_info_internal,
	                                             NULL, 0, NULL, 0, NULL, 0 );
	s->current_file_ok = ( err == UNZ_OK );
	return err;
}

/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/
extern int ZEXPORT unzGoToNextFile ( unzFile  file )
{
	unz64_s* s;
	int err;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( !s->current_file_ok )
	{
		return UNZ_END_OF_LIST_OF_FILE;
	}

	if ( s->gi.number_entry != 0xffff )  /* 2^16 files overflow hack */
		if ( s->num_file + 1 == s->gi.number_entry )
		{
			return UNZ_END_OF_LIST_OF_FILE;
		}

	s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
	                         s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment ;
	s->num_file++;
	err = unz64local_GetCurrentFileInfoInternal( file, &s->cur_file_info,
	                                             &s->cur_file_info_internal,
	                                             NULL, 0, NULL, 0, NULL, 0 );
	s->current_file_ok = ( err == UNZ_OK );
	return err;
}


/*
  Try locate the file szFileName in the zipfile.
  For the iCaseSensitivity signification, see unzipStringFileNameCompare

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/
extern int ZEXPORT unzLocateFile ( unzFile file, const char* szFileName, int iCaseSensitivity )
{
	unz64_s* s;
	int err;

	/* We remember the 'current' position in the file so that we can jump
	 * back there if we fail.
	 */
	unz_file_info64 cur_file_infoSaved;
	unz_file_info64_internal cur_file_info_internalSaved;
	ZPOS64_T num_fileSaved;
	ZPOS64_T pos_in_central_dirSaved;


	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	if ( strlen( szFileName ) >= UNZ_MAXFILENAMEINZIP )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( !s->current_file_ok )
	{
		return UNZ_END_OF_LIST_OF_FILE;
	}

	/* Save the current state */
	num_fileSaved = s->num_file;
	pos_in_central_dirSaved = s->pos_in_central_dir;
	cur_file_infoSaved = s->cur_file_info;
	cur_file_info_internalSaved = s->cur_file_info_internal;

	err = unzGoToFirstFile( file );

	while ( err == UNZ_OK )
	{
		char szCurrentFileName[UNZ_MAXFILENAMEINZIP + 1];
		err = unzGetCurrentFileInfo64( file, NULL,
		                               szCurrentFileName, sizeof( szCurrentFileName ) - 1,
		                               NULL, 0, NULL, 0 );

		if ( err == UNZ_OK )
		{
			if ( unzStringFileNameCompare( szCurrentFileName,
			                               szFileName, iCaseSensitivity ) == 0 )
			{
				return UNZ_OK;
			}

			err = unzGoToNextFile( file );
		}
	}

	/* We failed, so restore the state of the 'current file' to where we
	 * were.
	 */
	s->num_file = num_fileSaved ;
	s->pos_in_central_dir = pos_in_central_dirSaved ;
	s->cur_file_info = cur_file_infoSaved;
	s->cur_file_info_internal = cur_file_info_internalSaved;
	return err;
}


/*
///////////////////////////////////////////
// Contributed by Ryan Haksi (mailto://cryogen@infoserve.net)
// I need random access
//
// Further optimization could be realized by adding an ability
// to cache the directory in memory. The goal being a single
// comprehensive file read to put the file I need in a memory.
*/

/*
typedef struct unz_file_pos_s
{
    ZPOS64_T pos_in_zip_directory;   // offset in file
    ZPOS64_T num_of_file;            // # of file
} unz_file_pos;
*/

extern int ZEXPORT unzGetFilePos64( unzFile file, unz64_file_pos*  file_pos )
{
	unz64_s* s;

	if ( file == NULL || file_pos == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( !s->current_file_ok )
	{
		return UNZ_END_OF_LIST_OF_FILE;
	}

	file_pos->pos_in_zip_directory  = s->pos_in_central_dir;
	file_pos->num_of_file           = s->num_file;

	return UNZ_OK;
}

extern int ZEXPORT unzGetFilePos(
   unzFile file,
   unz_file_pos* file_pos )
{
	unz64_file_pos file_pos64;
	int err = unzGetFilePos64( file, &file_pos64 );

	if ( err == UNZ_OK )
	{
		file_pos->pos_in_zip_directory = ( uLong )file_pos64.pos_in_zip_directory;
		file_pos->num_of_file = ( uLong )file_pos64.num_of_file;
	}

	return err;
}

extern int ZEXPORT unzGoToFilePos64( unzFile file, const unz64_file_pos* file_pos )
{
	unz64_s* s;
	int err;

	if ( file == NULL || file_pos == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	/* jump to the right spot */
	s->pos_in_central_dir = file_pos->pos_in_zip_directory;
	s->num_file           = file_pos->num_of_file;

	/* set the current file */
	err = unz64local_GetCurrentFileInfoInternal( file, &s->cur_file_info,
	                                             &s->cur_file_info_internal,
	                                             NULL, 0, NULL, 0, NULL, 0 );
	/* return results */
	s->current_file_ok = ( err == UNZ_OK );
	return err;
}

extern int ZEXPORT unzGoToFilePos(
   unzFile file,
   unz_file_pos* file_pos )
{
	unz64_file_pos file_pos64;

	if ( file_pos == NULL )
	{
		return UNZ_PARAMERROR;
	}

	file_pos64.pos_in_zip_directory = file_pos->pos_in_zip_directory;
	file_pos64.num_of_file = file_pos->num_of_file;
	return unzGoToFilePos64( file, &file_pos64 );
}

/*
// Unzip Helper Functions - should be here?
///////////////////////////////////////////
*/

/*
  Read the local header of the current zipfile
  Check the coherency of the local header and info in the end of central
        directory about this file
  store in *piSizeVar the size of extra info in local header
        (filename and size of extra field data)
*/
local int unz64local_CheckCurrentFileCoherencyHeader ( unz64_s* s, uInt* piSizeVar,
                                                       ZPOS64_T* poffset_local_extrafield,
                                                       uInt*   psize_local_extrafield )
{
	uLong uMagic, uData, uFlags;
	uLong size_filename;
	uLong size_extra_field;
	int err = UNZ_OK;

	*piSizeVar = 0;
	*poffset_local_extrafield = 0;
	*psize_local_extrafield = 0;

	if ( ZSEEK64( s->z_filefunc, s->filestream, s->cur_file_info_internal.offset_curfile +
	              s->byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return UNZ_ERRNO;
	}


	if ( err == UNZ_OK )
	{
		if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uMagic ) != UNZ_OK )
		{
			err = UNZ_ERRNO;
		}
		else if ( uMagic != 0x04034b50 )
		{
			err = UNZ_BADZIPFILE;
		}
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	/*
	    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
	        err=UNZ_BADZIPFILE;
	*/
	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &uFlags ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}
	else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.compression_method ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( ( err == UNZ_OK ) && ( s->cur_file_info.compression_method != 0 ) &&
	     /* #ifdef HAVE_BZIP2 */
	     ( s->cur_file_info.compression_method != Z_BZIP2ED ) &&
	     /* #endif */
	     ( s->cur_file_info.compression_method != Z_DEFLATED ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK ) /* date/time */
	{
		err = UNZ_ERRNO;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK ) /* crc */
	{
		err = UNZ_ERRNO;
	}
	else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.crc ) && ( ( uFlags & 8 ) == 0 ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK ) /* size compr */
	{
		err = UNZ_ERRNO;
	}
	else if ( uData != 0xFFFFFFFF && ( err == UNZ_OK ) && ( uData != s->cur_file_info.compressed_size ) && ( ( uFlags & 8 ) == 0 ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( unz64local_getLong( &s->z_filefunc, s->filestream, &uData ) != UNZ_OK ) /* size uncompr */
	{
		err = UNZ_ERRNO;
	}
	else if ( uData != 0xFFFFFFFF && ( err == UNZ_OK ) && ( uData != s->cur_file_info.uncompressed_size ) && ( ( uFlags & 8 ) == 0 ) )
	{
		err = UNZ_BADZIPFILE;
	}

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &size_filename ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}
	else if ( ( err == UNZ_OK ) && ( size_filename != s->cur_file_info.size_filename ) )
	{
		err = UNZ_BADZIPFILE;
	}

	*piSizeVar += ( uInt )size_filename;

	if ( unz64local_getShort( &s->z_filefunc, s->filestream, &size_extra_field ) != UNZ_OK )
	{
		err = UNZ_ERRNO;
	}

	*poffset_local_extrafield = s->cur_file_info_internal.offset_curfile +
	                            SIZEZIPLOCALHEADER + size_filename;
	*psize_local_extrafield = ( uInt )size_extra_field;

	*piSizeVar += ( uInt )size_extra_field;

	return err;
}

/*
  Open for reading data the current file in the zipfile.
  If there is no error and the file is opened, the return value is UNZ_OK.
*/
extern int ZEXPORT unzOpenCurrentFile3 ( unzFile file, int* method,
                                         int* level, int raw, const char* password )
{
	int err = UNZ_OK;
	uInt iSizeVar;
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;
	ZPOS64_T offset_local_extrafield;  /* offset of the local extra field */
	uInt  size_local_extrafield;    /* size of the local extra field */
#    ifndef NOUNCRYPT
	char source[12];
#    else

	if ( password != NULL )
	{
		return UNZ_PARAMERROR;
	}

#    endif

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( !s->current_file_ok )
	{
		return UNZ_PARAMERROR;
	}

	if ( s->pfile_in_zip_read != NULL )
	{
		unzCloseCurrentFile( file );
	}

	if ( unz64local_CheckCurrentFileCoherencyHeader( s, &iSizeVar, &offset_local_extrafield, &size_local_extrafield ) != UNZ_OK )
	{
		return UNZ_BADZIPFILE;
	}

	pfile_in_zip_read_info = ( file_in_zip64_read_info_s* )ALLOC( sizeof( file_in_zip64_read_info_s ) );

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_INTERNALERROR;
	}

	pfile_in_zip_read_info->read_buffer = ( char* )ALLOC( UNZ_BUFSIZE );
	pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
	pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
	pfile_in_zip_read_info->pos_local_extrafield = 0;
	pfile_in_zip_read_info->raw = raw;

	if ( pfile_in_zip_read_info->read_buffer == NULL )
	{
		TRYFREE( pfile_in_zip_read_info );
		return UNZ_INTERNALERROR;
	}

	pfile_in_zip_read_info->stream_initialised = 0;

	if ( method != NULL )
	{
		*method = ( int )s->cur_file_info.compression_method;
	}

	if ( level != NULL )
	{
		*level = 6;

		switch ( s->cur_file_info.flag & 0x06 )
		{
			case 6 :
				*level = 1;
				break;

			case 4 :
				*level = 2;
				break;

			case 2 :
				*level = 9;
				break;
		}
	}

	if ( ( s->cur_file_info.compression_method != 0 ) &&
	     /* #ifdef HAVE_BZIP2 */
	     ( s->cur_file_info.compression_method != Z_BZIP2ED ) &&
	     /* #endif */
	     ( s->cur_file_info.compression_method != Z_DEFLATED ) )

	{
		err = UNZ_BADZIPFILE;
	}

	pfile_in_zip_read_info->crc32_wait = s->cur_file_info.crc;
	pfile_in_zip_read_info->crc32 = 0;
	pfile_in_zip_read_info->total_out_64 = 0;
	pfile_in_zip_read_info->compression_method = s->cur_file_info.compression_method;
	pfile_in_zip_read_info->filestream = s->filestream;
	pfile_in_zip_read_info->z_filefunc = s->z_filefunc;
	pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;

	pfile_in_zip_read_info->stream.total_out = 0;

	if ( ( s->cur_file_info.compression_method == Z_BZIP2ED ) && ( !raw ) )
	{
#ifdef HAVE_BZIP2
		pfile_in_zip_read_info->bstream.bzalloc = ( void * (* ) ( void*, int, int ) )0;
		pfile_in_zip_read_info->bstream.bzfree = ( free_func )0;
		pfile_in_zip_read_info->bstream.opaque = ( voidpf )0;
		pfile_in_zip_read_info->bstream.state = ( voidpf )0;

		pfile_in_zip_read_info->stream.zalloc = ( alloc_func )0;
		pfile_in_zip_read_info->stream.zfree = ( free_func )0;
		pfile_in_zip_read_info->stream.opaque = ( voidpf )0;
		pfile_in_zip_read_info->stream.next_in = ( voidpf )0;
		pfile_in_zip_read_info->stream.avail_in = 0;

		err = BZ2_bzDecompressInit( &pfile_in_zip_read_info->bstream, 0, 0 );

		if ( err == Z_OK )
		{
			pfile_in_zip_read_info->stream_initialised = Z_BZIP2ED;
		}
		else
		{
			TRYFREE( pfile_in_zip_read_info );
			return err;
		}

#else
		pfile_in_zip_read_info->raw = 1;
#endif
	}
	else if ( ( s->cur_file_info.compression_method == Z_DEFLATED ) && ( !raw ) )
	{
		pfile_in_zip_read_info->stream.zalloc = ( alloc_func )0;
		pfile_in_zip_read_info->stream.zfree = ( free_func )0;
		pfile_in_zip_read_info->stream.opaque = ( voidpf )0;
		pfile_in_zip_read_info->stream.next_in = 0;
		pfile_in_zip_read_info->stream.avail_in = 0;

		err = inflateInit2( &pfile_in_zip_read_info->stream, -MAX_WBITS );

		if ( err == Z_OK )
		{
			pfile_in_zip_read_info->stream_initialised = Z_DEFLATED;
		}
		else
		{
			TRYFREE( pfile_in_zip_read_info );
			return err;
		}

		/* windowBits is passed < 0 to tell that there is no zlib header.
		 * Note that in this case inflate *requires* an extra "dummy" byte
		 * after the compressed stream in order to complete decompression and
		 * return Z_STREAM_END.
		 * In unzip, i don't wait absolutely Z_STREAM_END because I known the
		 * size of both compressed and uncompressed data
		 */
	}

	pfile_in_zip_read_info->rest_read_compressed =
	   s->cur_file_info.compressed_size ;
	pfile_in_zip_read_info->rest_read_uncompressed =
	   s->cur_file_info.uncompressed_size ;


	pfile_in_zip_read_info->pos_in_zipfile =
	   s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER +
	   iSizeVar;

	pfile_in_zip_read_info->stream.avail_in = ( uInt )0;

	s->pfile_in_zip_read = pfile_in_zip_read_info;
	s->encrypted = 0;

#    ifndef NOUNCRYPT

	if ( password != NULL )
	{
		int i;
		s->pcrc_32_tab = get_crc_table();
		init_keys( password, s->keys, s->pcrc_32_tab );

		if ( ZSEEK64( s->z_filefunc, s->filestream,
		              s->pfile_in_zip_read->pos_in_zipfile +
		              s->pfile_in_zip_read->byte_before_the_zipfile,
		              SEEK_SET ) != 0 )
		{
			return UNZ_INTERNALERROR;
		}

		if ( ZREAD64( s->z_filefunc, s->filestream, source, 12 ) < 12 )
		{
			return UNZ_INTERNALERROR;
		}

		for ( i = 0; i < 12; i++ )
		{
			zdecode( s->keys, s->pcrc_32_tab, source[i] );
		}

		s->pfile_in_zip_read->pos_in_zipfile += 12;
		s->encrypted = 1;
	}

#    endif


	return UNZ_OK;
}

extern int ZEXPORT unzOpenCurrentFile ( unzFile file )
{
	return unzOpenCurrentFile3( file, NULL, NULL, 0, NULL );
}

extern int ZEXPORT unzOpenCurrentFilePassword ( unzFile file, const char*  password )
{
	return unzOpenCurrentFile3( file, NULL, NULL, 0, password );
}

extern int ZEXPORT unzOpenCurrentFile2 ( unzFile file, int* method, int* level, int raw )
{
	return unzOpenCurrentFile3( file, method, level, raw, NULL );
}

/** Addition for GDAL : START */

extern ZPOS64_T ZEXPORT unzGetCurrentFileZStreamPos64( unzFile file )
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;
	s = ( unz64_s* )file;

	if ( file == NULL )
	{
		return 0;   //UNZ_PARAMERROR;
	}

	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return 0;   //UNZ_PARAMERROR;
	}

	return pfile_in_zip_read_info->pos_in_zipfile +
	       pfile_in_zip_read_info->byte_before_the_zipfile;
}

/** Addition for GDAL : END */

/*
  Read bytes from the current file.
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of byte copied if somes bytes are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/
extern int ZEXPORT unzReadCurrentFile  ( unzFile file, voidp buf, unsigned len )
{
	int err = UNZ_OK;
	uInt iRead = 0;
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_PARAMERROR;
	}


	if ( pfile_in_zip_read_info->read_buffer == NULL )
	{
		return UNZ_END_OF_LIST_OF_FILE;
	}

	if ( len == 0 )
	{
		return 0;
	}

	pfile_in_zip_read_info->stream.next_out = ( Bytef* )buf;

	pfile_in_zip_read_info->stream.avail_out = ( uInt )len;

	if ( ( len > pfile_in_zip_read_info->rest_read_uncompressed ) &&
	     ( !( pfile_in_zip_read_info->raw ) ) )
		pfile_in_zip_read_info->stream.avail_out =
		   ( uInt )pfile_in_zip_read_info->rest_read_uncompressed;

	if ( ( len > pfile_in_zip_read_info->rest_read_compressed +
	       pfile_in_zip_read_info->stream.avail_in ) &&
	     ( pfile_in_zip_read_info->raw ) )
		pfile_in_zip_read_info->stream.avail_out =
		   ( uInt )pfile_in_zip_read_info->rest_read_compressed +
		   pfile_in_zip_read_info->stream.avail_in;

	while ( pfile_in_zip_read_info->stream.avail_out > 0 )
	{
		if ( ( pfile_in_zip_read_info->stream.avail_in == 0 ) &&
		     ( pfile_in_zip_read_info->rest_read_compressed > 0 ) )
		{
			uInt uReadThis = UNZ_BUFSIZE;

			if ( pfile_in_zip_read_info->rest_read_compressed < uReadThis )
			{
				uReadThis = ( uInt )pfile_in_zip_read_info->rest_read_compressed;
			}

			if ( uReadThis == 0 )
			{
				return UNZ_EOF;
			}

			if ( ZSEEK64( pfile_in_zip_read_info->z_filefunc,
			              pfile_in_zip_read_info->filestream,
			              pfile_in_zip_read_info->pos_in_zipfile +
			              pfile_in_zip_read_info->byte_before_the_zipfile,
			              ZLIB_FILEFUNC_SEEK_SET ) != 0 )
			{
				return UNZ_ERRNO;
			}

			if ( ZREAD64( pfile_in_zip_read_info->z_filefunc,
			              pfile_in_zip_read_info->filestream,
			              pfile_in_zip_read_info->read_buffer,
			              uReadThis ) != uReadThis )
			{
				return UNZ_ERRNO;
			}


#            ifndef NOUNCRYPT

			if ( s->encrypted )
			{
				uInt i;

				for ( i = 0; i < uReadThis; i++ )
					pfile_in_zip_read_info->read_buffer[i] =
					   zdecode( s->keys, s->pcrc_32_tab,
					            pfile_in_zip_read_info->read_buffer[i] );
			}

#            endif


			pfile_in_zip_read_info->pos_in_zipfile += uReadThis;

			pfile_in_zip_read_info->rest_read_compressed -= uReadThis;

			pfile_in_zip_read_info->stream.next_in =
			   ( Bytef* )pfile_in_zip_read_info->read_buffer;
			pfile_in_zip_read_info->stream.avail_in = ( uInt )uReadThis;
		}

		if ( ( pfile_in_zip_read_info->compression_method == 0 ) || ( pfile_in_zip_read_info->raw ) )
		{
			uInt uDoCopy, i ;

			if ( ( pfile_in_zip_read_info->stream.avail_in == 0 ) &&
			     ( pfile_in_zip_read_info->rest_read_compressed == 0 ) )
			{
				return ( iRead == 0 ) ? UNZ_EOF : iRead;
			}

			if ( pfile_in_zip_read_info->stream.avail_out <
			     pfile_in_zip_read_info->stream.avail_in )
			{
				uDoCopy = pfile_in_zip_read_info->stream.avail_out ;
			}
			else
			{
				uDoCopy = pfile_in_zip_read_info->stream.avail_in ;
			}

			for ( i = 0; i < uDoCopy; i++ )
				*( pfile_in_zip_read_info->stream.next_out + i ) =
				   *( pfile_in_zip_read_info->stream.next_in + i );

			pfile_in_zip_read_info->total_out_64 = pfile_in_zip_read_info->total_out_64 + uDoCopy;

			pfile_in_zip_read_info->crc32 = crc32( pfile_in_zip_read_info->crc32,
			                                       pfile_in_zip_read_info->stream.next_out,
			                                       uDoCopy );
			pfile_in_zip_read_info->rest_read_uncompressed -= uDoCopy;
			pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
			pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
			pfile_in_zip_read_info->stream.next_out += uDoCopy;
			pfile_in_zip_read_info->stream.next_in += uDoCopy;
			pfile_in_zip_read_info->stream.total_out += uDoCopy;
			iRead += uDoCopy;
		}
		else if ( pfile_in_zip_read_info->compression_method == Z_BZIP2ED )
		{
#ifdef HAVE_BZIP2
			uLong uTotalOutBefore, uTotalOutAfter;
			const Bytef* bufBefore;
			uLong uOutThis;

			pfile_in_zip_read_info->bstream.next_in        = ( char* )pfile_in_zip_read_info->stream.next_in;
			pfile_in_zip_read_info->bstream.avail_in       = pfile_in_zip_read_info->stream.avail_in;
			pfile_in_zip_read_info->bstream.total_in_lo32  = pfile_in_zip_read_info->stream.total_in;
			pfile_in_zip_read_info->bstream.total_in_hi32  = 0;
			pfile_in_zip_read_info->bstream.next_out       = ( char* )pfile_in_zip_read_info->stream.next_out;
			pfile_in_zip_read_info->bstream.avail_out      = pfile_in_zip_read_info->stream.avail_out;
			pfile_in_zip_read_info->bstream.total_out_lo32 = pfile_in_zip_read_info->stream.total_out;
			pfile_in_zip_read_info->bstream.total_out_hi32 = 0;

			uTotalOutBefore = pfile_in_zip_read_info->bstream.total_out_lo32;
			bufBefore = ( const Bytef* )pfile_in_zip_read_info->bstream.next_out;

			err = BZ2_bzDecompress( &pfile_in_zip_read_info->bstream );

			uTotalOutAfter = pfile_in_zip_read_info->bstream.total_out_lo32;
			uOutThis = uTotalOutAfter - uTotalOutBefore;

			pfile_in_zip_read_info->total_out_64 = pfile_in_zip_read_info->total_out_64 + uOutThis;

			pfile_in_zip_read_info->crc32 = crc32( pfile_in_zip_read_info->crc32, bufBefore, ( uInt )( uOutThis ) );
			pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;
			iRead += ( uInt )( uTotalOutAfter - uTotalOutBefore );

			pfile_in_zip_read_info->stream.next_in   = ( Bytef* )pfile_in_zip_read_info->bstream.next_in;
			pfile_in_zip_read_info->stream.avail_in  = pfile_in_zip_read_info->bstream.avail_in;
			pfile_in_zip_read_info->stream.total_in  = pfile_in_zip_read_info->bstream.total_in_lo32;
			pfile_in_zip_read_info->stream.next_out  = ( Bytef* )pfile_in_zip_read_info->bstream.next_out;
			pfile_in_zip_read_info->stream.avail_out = pfile_in_zip_read_info->bstream.avail_out;
			pfile_in_zip_read_info->stream.total_out = pfile_in_zip_read_info->bstream.total_out_lo32;

			if ( err == BZ_STREAM_END )
			{
				return ( iRead == 0 ) ? UNZ_EOF : iRead;
			}

			if ( err != BZ_OK )
			{
				break;
			}

#endif
		} // end Z_BZIP2ED
		else
		{
			ZPOS64_T uTotalOutBefore, uTotalOutAfter;
			const Bytef* bufBefore;
			ZPOS64_T uOutThis;
			int flush = Z_SYNC_FLUSH;

			uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
			bufBefore = pfile_in_zip_read_info->stream.next_out;

			/*
			if ((pfile_in_zip_read_info->rest_read_uncompressed ==
			         pfile_in_zip_read_info->stream.avail_out) &&
			    (pfile_in_zip_read_info->rest_read_compressed == 0))
			    flush = Z_FINISH;
			*/
			err = inflate( &pfile_in_zip_read_info->stream, flush );

			if ( ( err >= 0 ) && ( pfile_in_zip_read_info->stream.msg != NULL ) )
			{
				err = Z_DATA_ERROR;
			}

			uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
			uOutThis = uTotalOutAfter - uTotalOutBefore;

			pfile_in_zip_read_info->total_out_64 = pfile_in_zip_read_info->total_out_64 + uOutThis;

			pfile_in_zip_read_info->crc32 =
			   crc32( pfile_in_zip_read_info->crc32, bufBefore,
			          ( uInt )( uOutThis ) );

			pfile_in_zip_read_info->rest_read_uncompressed -=
			   uOutThis;

			iRead += ( uInt )( uTotalOutAfter - uTotalOutBefore );

			if ( err == Z_STREAM_END )
			{
				return ( iRead == 0 ) ? UNZ_EOF : iRead;
			}

			if ( err != Z_OK )
			{
				break;
			}
		}
	}

	if ( err == Z_OK )
	{
		return iRead;
	}

	return err;
}


/*
  Give the current position in uncompressed data
*/
extern z_off_t ZEXPORT unztell ( unzFile file )
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_PARAMERROR;
	}

	return ( z_off_t )pfile_in_zip_read_info->stream.total_out;
}

extern ZPOS64_T ZEXPORT unztell64 ( unzFile file )
{

	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;

	if ( file == NULL )
	{
		return ( ZPOS64_T ) - 1;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return ( ZPOS64_T ) - 1;
	}

	return pfile_in_zip_read_info->total_out_64;
}


/*
  return 1 if the end of file was reached, 0 elsewhere
*/
extern int ZEXPORT unzeof ( unzFile file )
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_PARAMERROR;
	}

	if ( pfile_in_zip_read_info->rest_read_uncompressed == 0 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}



/*
Read extra field from the current file (opened by unzOpenCurrentFile)
This is the local-header version of the extra field (sometimes, there is
more info in the local-header version than in the central-header)

  if buf==NULL, it return the size of the local extra field that can be read

  if buf!=NULL, len is the size of the buffer, the extra header is copied in
    buf.
  the return value is the number of bytes copied in buf, or (if <0)
    the error code
*/
extern int ZEXPORT unzGetLocalExtrafield ( unzFile file, voidp buf, unsigned len )
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;
	uInt read_now;
	ZPOS64_T size_to_read;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_PARAMERROR;
	}

	size_to_read = ( pfile_in_zip_read_info->size_local_extrafield -
	                 pfile_in_zip_read_info->pos_local_extrafield );

	if ( buf == NULL )
	{
		return ( int )size_to_read;
	}

	if ( len > size_to_read )
	{
		read_now = ( uInt )size_to_read;
	}
	else
	{
		read_now = ( uInt )len ;
	}

	if ( read_now == 0 )
	{
		return 0;
	}

	if ( ZSEEK64( pfile_in_zip_read_info->z_filefunc,
	              pfile_in_zip_read_info->filestream,
	              pfile_in_zip_read_info->offset_local_extrafield +
	              pfile_in_zip_read_info->pos_local_extrafield,
	              ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return UNZ_ERRNO;
	}

	if ( ZREAD64( pfile_in_zip_read_info->z_filefunc,
	              pfile_in_zip_read_info->filestream,
	              buf, read_now ) != read_now )
	{
		return UNZ_ERRNO;
	}

	return ( int )read_now;
}

/*
  Close the file in zip opened with unzipOpenCurrentFile
  Return UNZ_CRCERROR if all the file was read but the CRC is not good
*/
extern int ZEXPORT unzCloseCurrentFile ( unzFile file )
{
	int err = UNZ_OK;

	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if ( pfile_in_zip_read_info == NULL )
	{
		return UNZ_PARAMERROR;
	}


	if ( ( pfile_in_zip_read_info->rest_read_uncompressed == 0 ) &&
	     ( !pfile_in_zip_read_info->raw ) )
	{
		if ( pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait )
		{
			err = UNZ_CRCERROR;
		}
	}


	TRYFREE( pfile_in_zip_read_info->read_buffer );
	pfile_in_zip_read_info->read_buffer = NULL;

	if ( pfile_in_zip_read_info->stream_initialised == Z_DEFLATED )
	{
		inflateEnd( &pfile_in_zip_read_info->stream );
	}

#ifdef HAVE_BZIP2
	else if ( pfile_in_zip_read_info->stream_initialised == Z_BZIP2ED )
	{
		BZ2_bzDecompressEnd( &pfile_in_zip_read_info->bstream );
	}

#endif


	pfile_in_zip_read_info->stream_initialised = 0;
	TRYFREE( pfile_in_zip_read_info );

	s->pfile_in_zip_read = NULL;

	return err;
}


/*
  Get the global comment string of the ZipFile, in the szComment buffer.
  uSizeBuf is the size of the szComment buffer.
  return the number of byte copied or an error code <0
*/
extern int ZEXPORT unzGetGlobalComment ( unzFile file, char* szComment, uLong uSizeBuf )
{
	unz64_s* s;
	uLong uReadThis ;

	if ( file == NULL )
	{
		return ( int )UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	uReadThis = uSizeBuf;

	if ( uReadThis > s->gi.size_comment )
	{
		uReadThis = s->gi.size_comment;
	}

	if ( ZSEEK64( s->z_filefunc, s->filestream, s->central_pos + 22, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
	{
		return UNZ_ERRNO;
	}

	if ( uReadThis > 0 )
	{
		*szComment = '\0';

		if ( ZREAD64( s->z_filefunc, s->filestream, szComment, uReadThis ) != uReadThis )
		{
			return UNZ_ERRNO;
		}
	}

	if ( ( szComment != NULL ) && ( uSizeBuf > s->gi.size_comment ) )
	{
		*( szComment + s->gi.size_comment ) = '\0';
	}

	return ( int )uReadThis;
}

/* Additions by RX '2004 */
extern ZPOS64_T ZEXPORT unzGetOffset64( unzFile file )
{
	unz64_s* s;

	if ( file == NULL )
	{
		return 0;   //UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	if ( !s->current_file_ok )
	{
		return 0;
	}

	if ( s->gi.number_entry != 0 && s->gi.number_entry != 0xffff )
		if ( s->num_file == s->gi.number_entry )
		{
			return 0;
		}

	return s->pos_in_central_dir;
}

extern uLong ZEXPORT unzGetOffset ( unzFile file )
{
	ZPOS64_T offset64;

	if ( file == NULL )
	{
		return 0;   //UNZ_PARAMERROR;
	}

	offset64 = unzGetOffset64( file );
	return ( uLong )offset64;
}

extern int ZEXPORT unzSetOffset64( unzFile file, ZPOS64_T pos )
{
	unz64_s* s;
	int err;

	if ( file == NULL )
	{
		return UNZ_PARAMERROR;
	}

	s = ( unz64_s* )file;

	s->pos_in_central_dir = pos;
	s->num_file = s->gi.number_entry;      /* hack */
	err = unz64local_GetCurrentFileInfoInternal( file, &s->cur_file_info,
	                                             &s->cur_file_info_internal,
	                                             NULL, 0, NULL, 0, NULL, 0 );
	s->current_file_ok = ( err == UNZ_OK );
	return err;
}

extern int ZEXPORT unzSetOffset ( unzFile file, uLong pos )
{
	return unzSetOffset64( file, pos );
}

//// ioapi.c

/* ioapi.h -- IO base function header for compress/uncompress .zip
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

*/

#if (defined(_WIN32))
#define _CRT_SECURE_NO_WARNINGS
#endif

// #include "ioapi.h"

voidpf call_zopen64 ( const zlib_filefunc64_32_def* pfilefunc, const void* filename, int mode )
{
	if ( pfilefunc->zfile_func64.zopen64_file != NULL )
	{
		return ( *( pfilefunc->zfile_func64.zopen64_file ) ) ( pfilefunc->zfile_func64.opaque, filename, mode );
	}
	else
	{
		return ( *( pfilefunc->zopen32_file ) )( pfilefunc->zfile_func64.opaque, ( const char* )filename, mode );
	}
}

long call_zseek64 ( const zlib_filefunc64_32_def* pfilefunc, voidpf filestream, ZPOS64_T offset, int origin )
{
	if ( pfilefunc->zfile_func64.zseek64_file != NULL )
	{
		return ( *( pfilefunc->zfile_func64.zseek64_file ) ) ( pfilefunc->zfile_func64.opaque, filestream, offset, origin );
	}
	else
	{
		uLong offsetTruncated = ( uLong )offset;

		if ( offsetTruncated != offset )
		{
			return -1;
		}
		else
		{
			return ( *( pfilefunc->zseek32_file ) )( pfilefunc->zfile_func64.opaque, filestream, offsetTruncated, origin );
		}
	}
}

ZPOS64_T call_ztell64 ( const zlib_filefunc64_32_def* pfilefunc, voidpf filestream )
{
	if ( pfilefunc->zfile_func64.zseek64_file != NULL )
	{
		return ( *( pfilefunc->zfile_func64.ztell64_file ) ) ( pfilefunc->zfile_func64.opaque, filestream );
	}
	else
	{
		uLong tell_uLong = ( *( pfilefunc->ztell32_file ) )( pfilefunc->zfile_func64.opaque, filestream );

		if ( ( tell_uLong ) == ( ( uLong ) - 1 ) )
		{
			return ( ZPOS64_T ) - 1;
		}
		else
		{
			return tell_uLong;
		}
	}
}

void fill_zlib_filefunc64_32_def_from_filefunc32( zlib_filefunc64_32_def* p_filefunc64_32, const zlib_filefunc_def* p_filefunc32 )
{
	p_filefunc64_32->zfile_func64.zopen64_file = NULL;
	p_filefunc64_32->zopen32_file = p_filefunc32->zopen_file;
	p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
	p_filefunc64_32->zfile_func64.zread_file = p_filefunc32->zread_file;
	p_filefunc64_32->zfile_func64.zwrite_file = p_filefunc32->zwrite_file;
	p_filefunc64_32->zfile_func64.ztell64_file = NULL;
	p_filefunc64_32->zfile_func64.zseek64_file = NULL;
	p_filefunc64_32->zfile_func64.zclose_file = p_filefunc32->zclose_file;
	p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
	p_filefunc64_32->zfile_func64.opaque = p_filefunc32->opaque;
	p_filefunc64_32->zseek32_file = p_filefunc32->zseek_file;
	p_filefunc64_32->ztell32_file = p_filefunc32->ztell_file;
}

/*

static voidpf  ZCALLBACK fopen_file_func OF((voidpf opaque, const char* filename, int mode));
static uLong   ZCALLBACK fread_file_func OF((voidpf opaque, voidpf stream, void* buf, uLong size));
static uLong   ZCALLBACK fwrite_file_func OF((voidpf opaque, voidpf stream, const void* buf,uLong size));
static ZPOS64_T ZCALLBACK ftell64_file_func OF((voidpf opaque, voidpf stream));
static long    ZCALLBACK fseek64_file_func OF((voidpf opaque, voidpf stream, ZPOS64_T offset, int origin));
static int     ZCALLBACK fclose_file_func OF((voidpf opaque, voidpf stream));
static int     ZCALLBACK ferror_file_func OF((voidpf opaque, voidpf stream));

static voidpf ZCALLBACK fopen_file_func (voidpf opaque, const char* filename, int mode)
{
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = "rb";
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = "r+b";
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = "wb";

    if ((filename!=NULL) && (mode_fopen != NULL))
        file = fopen(filename, mode_fopen);
    return file;
}

static voidpf ZCALLBACK fopen64_file_func (voidpf opaque, const void* filename, int mode)
{
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = "rb";
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = "r+b";
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = "wb";

    if ((filename!=NULL) && (mode_fopen != NULL))
#if defined(ANDROID)
        file = fopen((const char*)filename, mode_fopen);
#else
        file = fopen64((const char*)filename, mode_fopen);
#endif
    return file;
}


static uLong ZCALLBACK fread_file_func (voidpf opaque, voidpf stream, void* buf, uLong size)
{
    uLong ret;
    ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}

static uLong ZCALLBACK fwrite_file_func (voidpf opaque, voidpf stream, const void* buf, uLong size)
{
    uLong ret;
    ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}

static long ZCALLBACK ftell_file_func (voidpf opaque, voidpf stream)
{
    long ret;
    ret = ftell((FILE *)stream);
    return ret;
}


static ZPOS64_T ZCALLBACK ftell64_file_func (voidpf opaque, voidpf stream)
{
    ZPOS64_T ret;
    ret = ftello64((FILE *)stream);
    return ret;
}

static long ZCALLBACK fseek_file_func (voidpf  opaque, voidpf stream, uLong offset, int origin)
{
    int fseek_origin=0;
    long ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;
    if (fseek((FILE *)stream, offset, fseek_origin) != 0)
        ret = -1;
    return ret;
}

static long ZCALLBACK fseek64_file_func (voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
    int fseek_origin=0;
    long ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;

    if(fseeko64((FILE *)stream, offset, fseek_origin) != 0)
                        ret = -1;

    return ret;
}


static int ZCALLBACK fclose_file_func (voidpf opaque, voidpf stream)
{
    int ret;
    ret = fclose((FILE *)stream);
    return ret;
}

static int ZCALLBACK ferror_file_func (voidpf opaque, voidpf stream)
{
    int ret;
    ret = ferror((FILE *)stream);
    return ret;
}

void fill_fopen_filefunc (pzlib_filefunc_def)
  zlib_filefunc_def* pzlib_filefunc_def;
{
    pzlib_filefunc_def->zopen_file = fopen_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell_file = ftell_file_func;
    pzlib_filefunc_def->zseek_file = fseek_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

void fill_fopen64_filefunc (zlib_filefunc64_def*  pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen64_file = fopen64_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell64_file = ftell64_file_func;
    pzlib_filefunc_def->zseek64_file = fseek64_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}
*/

/* libzip2 1.0.6, compressed to one file */


/*-------------------------------------------------------------*/
/*--- Private header file for the library.                  ---*/
/*---                                       bzlib_private.h ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */


#ifndef _BZLIB_PRIVATE_H
#define _BZLIB_PRIVATE_H

#define BZ_NO_STDIO

#include <stdlib.h>

#ifndef BZ_NO_STDIO
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#endif

#ifdef _MSC_VER

// LV: I have no time to mess with this code and make it warning-clean

// conditional expression is constant
#pragma warning( disable : 4127 )
// UChar to Int32 cast
#pragma warning( disable : 4244 )
// unreferenced formal parameter
#pragma warning( disable : 4100 )

#endif


/*-- General stuff. --*/

#define BZ_VERSION  "1.0.6, 6-Sept-2010"

typedef char            Char;
typedef unsigned char   Bool;
typedef unsigned char   UChar;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef short           Int16;
typedef unsigned short  UInt16;

#define True  ((Bool)1)
#define False ((Bool)0)

#ifndef __GNUC__
#define __inline__  /* */
#endif

#ifndef BZ_NO_STDIO

extern void BZ2_bz__AssertH__fail ( int errcode );
#define AssertH(cond,errcode) \
   { if (!(cond)) BZ2_bz__AssertH__fail ( errcode ); }

#if BZ_DEBUG
#define AssertD(cond,msg) \
   { if (!(cond)) {       \
      fprintf ( stderr,   \
        "\n\nlibbzip2(debug build): internal error\n\t%s\n", msg );\
      exit(1); \
   }}
#else
#define AssertD(cond,msg) /* */
#endif

#define VPrintf0(zf) \
   fprintf(stderr,zf)
#define VPrintf1(zf,za1) \
   fprintf(stderr,zf,za1)
#define VPrintf2(zf,za1,za2) \
   fprintf(stderr,zf,za1,za2)
#define VPrintf3(zf,za1,za2,za3) \
   fprintf(stderr,zf,za1,za2,za3)
#define VPrintf4(zf,za1,za2,za3,za4) \
   fprintf(stderr,zf,za1,za2,za3,za4)
#define VPrintf5(zf,za1,za2,za3,za4,za5) \
   fprintf(stderr,zf,za1,za2,za3,za4,za5)

#else

extern void bz_internal_error ( int errcode );
#define AssertH(cond,errcode) \
   { if (!(cond)) bz_internal_error ( errcode ); }
#define AssertD(cond,msg)                do { } while (0)
#define VPrintf0(zf)                     do { } while (0)
#define VPrintf1(zf,za1)                 do { } while (0)
#define VPrintf2(zf,za1,za2)             do { } while (0)
#define VPrintf3(zf,za1,za2,za3)         do { } while (0)
#define VPrintf4(zf,za1,za2,za3,za4)     do { } while (0)
#define VPrintf5(zf,za1,za2,za3,za4,za5) do { } while (0)

#endif


#define BZALLOC(nnn) (strm->bzalloc)(strm->opaque,(nnn),1)
#define BZFREE(ppp)  (strm->bzfree)(strm->opaque,(ppp))


/*-- Header bytes. --*/

#define BZ_HDR_B 0x42   /* 'B' */
#define BZ_HDR_Z 0x5a   /* 'Z' */
#define BZ_HDR_h 0x68   /* 'h' */
#define BZ_HDR_0 0x30   /* '0' */

/*-- Constants for the back end. --*/

#define BZ_MAX_ALPHA_SIZE 258
#define BZ_MAX_CODE_LEN    23

#define BZ_RUNA 0
#define BZ_RUNB 1

#define BZ_N_GROUPS 6
#define BZ_G_SIZE   50
#define BZ_N_ITERS  4

#define BZ_MAX_SELECTORS (2 + (900000 / BZ_G_SIZE))



/*-- Stuff for randomising repetitive blocks. --*/

extern Int32 BZ2_rNums[512];

#define BZ_RAND_DECLS                          \
   Int32 rNToGo;                               \
   Int32 rTPos                                 \
 
#define BZ_RAND_INIT_MASK                      \
   s->rNToGo = 0;                              \
   s->rTPos  = 0                               \
 
#define BZ_RAND_MASK ((s->rNToGo == 1) ? 1 : 0)

#define BZ_RAND_UPD_MASK                       \
   if (s->rNToGo == 0) {                       \
      s->rNToGo = BZ2_rNums[s->rTPos];         \
      s->rTPos++;                              \
      if (s->rTPos == 512) s->rTPos = 0;       \
   }                                           \
   s->rNToGo--;



/*-- Stuff for doing CRCs. --*/

extern UInt32 BZ2_crc32Table[256];

#define BZ_INITIALISE_CRC(crcVar)              \
{                                              \
   crcVar = 0xffffffffL;                       \
}

#define BZ_FINALISE_CRC(crcVar)                \
{                                              \
   crcVar = ~(crcVar);                         \
}

#define BZ_UPDATE_CRC(crcVar,cha)              \
{                                              \
   crcVar = (crcVar << 8) ^                    \
            BZ2_crc32Table[(crcVar >> 24) ^    \
                           ((UChar)cha)];      \
}



/*-- States and modes for compression. --*/

#define BZ_M_IDLE      1
#define BZ_M_RUNNING   2
#define BZ_M_FLUSHING  3
#define BZ_M_FINISHING 4

#define BZ_S_OUTPUT    1
#define BZ_S_INPUT     2

#define BZ_N_RADIX 2
#define BZ_N_QSORT 12
#define BZ_N_SHELL 18
#define BZ_N_OVERSHOOT (BZ_N_RADIX + BZ_N_QSORT + BZ_N_SHELL + 2)




/*-- Structure holding all the compression-side stuff. --*/

typedef
struct
{
	/* pointer back to the struct bz_stream */
	bz_stream* strm;

	/* mode this stream is in, and whether inputting */
	/* or outputting data */
	Int32    mode;
	Int32    state;

	/* remembers avail_in when flush/finish requested */
	UInt32   avail_in_expect;

	/* for doing the block sorting */
	UInt32*  arr1;
	UInt32*  arr2;
	UInt32*  ftab;
	Int32    origPtr;

	/* aliases for arr1 and arr2 */
	UInt32*  ptr;
	UChar*   block;
	UInt16*  mtfv;
	UChar*   zbits;

	/* for deciding when to use the fallback sorting algorithm */
	Int32    workFactor;

	/* run-length-encoding of the input */
	UInt32   state_in_ch;
	Int32    state_in_len;
	BZ_RAND_DECLS;

	/* input and output limits and current posns */
	Int32    nblock;
	Int32    nblockMAX;
	Int32    numZ;
	Int32    state_out_pos;

	/* map of bytes used in block */
	Int32    nInUse;
	Bool     inUse[256];
	UChar    unseqToSeq[256];

	/* the buffer for bit stream creation */
	UInt32   bsBuff;
	Int32    bsLive;

	/* block and combined CRCs */
	UInt32   blockCRC;
	UInt32   combinedCRC;

	/* misc administratium */
	Int32    verbosity;
	Int32    blockNo;
	Int32    blockSize100k;

	/* stuff for coding the MTF values */
	Int32    nMTF;
	Int32    mtfFreq    [BZ_MAX_ALPHA_SIZE];
	UChar    selector   [BZ_MAX_SELECTORS];
	UChar    selectorMtf[BZ_MAX_SELECTORS];

	UChar    len     [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32    code    [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32    rfreq   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	/* second dimension: only 3 needed; 4 makes index calculations faster */
	UInt32   len_pack[BZ_MAX_ALPHA_SIZE][4];

}
EState;



/*-- externs for compression. --*/

extern void
BZ2_blockSort ( EState* );

extern void
BZ2_compressBlock ( EState*, Bool );

extern void
BZ2_bsInitWrite ( EState* );

extern void
BZ2_hbAssignCodes ( Int32*, UChar*, Int32, Int32, Int32 );

extern void
BZ2_hbMakeCodeLengths ( UChar*, Int32*, Int32, Int32 );



/*-- states for decompression. --*/

#define BZ_X_IDLE        1
#define BZ_X_OUTPUT      2

#define BZ_X_MAGIC_1     10
#define BZ_X_MAGIC_2     11
#define BZ_X_MAGIC_3     12
#define BZ_X_MAGIC_4     13
#define BZ_X_BLKHDR_1    14
#define BZ_X_BLKHDR_2    15
#define BZ_X_BLKHDR_3    16
#define BZ_X_BLKHDR_4    17
#define BZ_X_BLKHDR_5    18
#define BZ_X_BLKHDR_6    19
#define BZ_X_BCRC_1      20
#define BZ_X_BCRC_2      21
#define BZ_X_BCRC_3      22
#define BZ_X_BCRC_4      23
#define BZ_X_RANDBIT     24
#define BZ_X_ORIGPTR_1   25
#define BZ_X_ORIGPTR_2   26
#define BZ_X_ORIGPTR_3   27
#define BZ_X_MAPPING_1   28
#define BZ_X_MAPPING_2   29
#define BZ_X_SELECTOR_1  30
#define BZ_X_SELECTOR_2  31
#define BZ_X_SELECTOR_3  32
#define BZ_X_CODING_1    33
#define BZ_X_CODING_2    34
#define BZ_X_CODING_3    35
#define BZ_X_MTF_1       36
#define BZ_X_MTF_2       37
#define BZ_X_MTF_3       38
#define BZ_X_MTF_4       39
#define BZ_X_MTF_5       40
#define BZ_X_MTF_6       41
#define BZ_X_ENDHDR_2    42
#define BZ_X_ENDHDR_3    43
#define BZ_X_ENDHDR_4    44
#define BZ_X_ENDHDR_5    45
#define BZ_X_ENDHDR_6    46
#define BZ_X_CCRC_1      47
#define BZ_X_CCRC_2      48
#define BZ_X_CCRC_3      49
#define BZ_X_CCRC_4      50



/*-- Constants for the fast MTF decoder. --*/

#define MTFA_SIZE 4096
#define MTFL_SIZE 16



/*-- Structure holding all the decompression-side stuff. --*/

typedef
struct
{
	/* pointer back to the struct bz_stream */
	bz_stream* strm;

	/* state indicator for this stream */
	Int32    state;

	/* for doing the final run-length decoding */
	UChar    state_out_ch;
	Int32    state_out_len;
	Bool     blockRandomised;
	BZ_RAND_DECLS;

	/* the buffer for bit stream reading */
	UInt32   bsBuff;
	Int32    bsLive;

	/* misc administratium */
	Int32    blockSize100k;
	Bool     smallDecompress;
	Int32    currBlockNo;
	Int32    verbosity;

	/* for undoing the Burrows-Wheeler transform */
	Int32    origPtr;
	UInt32   tPos;
	Int32    k0;
	Int32    unzftab[256];
	Int32    nblock_used;
	Int32    cftab[257];
	Int32    cftabCopy[257];

	/* for undoing the Burrows-Wheeler transform (FAST) */
	UInt32*   tt;

	/* for undoing the Burrows-Wheeler transform (SMALL) */
	UInt16*   ll16;
	UChar*    ll4;

	/* stored and calculated CRCs */
	UInt32   storedBlockCRC;
	UInt32   storedCombinedCRC;
	UInt32   calculatedBlockCRC;
	UInt32   calculatedCombinedCRC;

	/* map of bytes used in block */
	Int32    nInUse;
	Bool     inUse[256];
	Bool     inUse16[16];
	UChar    seqToUnseq[256];

	/* for decoding the MTF values */
	UChar    mtfa   [MTFA_SIZE];
	Int32    mtfbase[256 / MTFL_SIZE];
	UChar    selector   [BZ_MAX_SELECTORS];
	UChar    selectorMtf[BZ_MAX_SELECTORS];
	UChar    len  [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];

	Int32    limit  [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32    base   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32    perm   [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32    minLens[BZ_N_GROUPS];

	/* save area for scalars in the main decompress code */
	Int32    save_i;
	Int32    save_j;
	Int32    save_t;
	Int32    save_alphaSize;
	Int32    save_nGroups;
	Int32    save_nSelectors;
	Int32    save_EOB;
	Int32    save_groupNo;
	Int32    save_groupPos;
	Int32    save_nextSym;
	Int32    save_nblockMAX;
	Int32    save_nblock;
	Int32    save_es;
	Int32    save_N;
	Int32    save_curr;
	Int32    save_zt;
	Int32    save_zn;
	Int32    save_zvec;
	Int32    save_zj;
	Int32    save_gSel;
	Int32    save_gMinlen;
	Int32*   save_gLimit;
	Int32*   save_gBase;
	Int32*   save_gPerm;

}
DState;



/*-- Macros for decompression. --*/

#define BZ_GET_FAST(cccc)                     \
    /* c_tPos is unsigned, hence test < 0 is pointless. */ \
    if (s->tPos >= (UInt32)100000 * (UInt32)s->blockSize100k) return True; \
    s->tPos = s->tt[s->tPos];                 \
    cccc = (UChar)(s->tPos & 0xff);           \
    s->tPos >>= 8;

#define BZ_GET_FAST_C(cccc)                   \
    /* c_tPos is unsigned, hence test < 0 is pointless. */ \
    if (c_tPos >= (UInt32)100000 * (UInt32)ro_blockSize100k) return True; \
    c_tPos = c_tt[c_tPos];                    \
    cccc = (UChar)(c_tPos & 0xff);            \
    c_tPos >>= 8;

#define SET_LL4(i,n)                                          \
   { if (((i) & 0x1) == 0)                                    \
        s->ll4[(i) >> 1] = (s->ll4[(i) >> 1] & 0xf0) | (n); else    \
        s->ll4[(i) >> 1] = (s->ll4[(i) >> 1] & 0x0f) | ((n) << 4);  \
   }

#define GET_LL4(i)                             \
   ((((UInt32)(s->ll4[(i) >> 1])) >> (((i) << 2) & 0x4)) & 0xF)

#define SET_LL(i,n)                          \
   { s->ll16[i] = (UInt16)(n & 0x0000ffff);  \
     SET_LL4(i, n >> 16);                    \
   }

#define GET_LL(i) \
   (((UInt32)s->ll16[i]) | (GET_LL4(i) << 16))

#define BZ_GET_SMALL(cccc)                            \
    /* c_tPos is unsigned, hence test < 0 is pointless. */ \
    if (s->tPos >= (UInt32)100000 * (UInt32)s->blockSize100k) return True; \
    cccc = BZ2_indexIntoF ( s->tPos, s->cftab );    \
    s->tPos = GET_LL(s->tPos);


/*-- externs for decompression. --*/

extern Int32
BZ2_indexIntoF ( Int32, Int32* );

extern Int32
BZ2_decompress ( DState* );

extern void
BZ2_hbCreateDecodeTables ( Int32*, Int32*, Int32*, UChar*,
                           Int32,  Int32, Int32 );


#endif


/*-- BZ_NO_STDIO seems to make NULL disappear on some platforms. --*/

#ifdef BZ_NO_STDIO
#ifndef NULL
#define NULL 0
#endif
#endif


/*-------------------------------------------------------------*/
/*--- end                                   bzlib_private.h ---*/
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--- Library top-level functions.                          ---*/
/*---                                               bzlib.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */

/* CHANGES
   0.9.0    -- original version.
   0.9.0a/b -- no changes in this file.
   0.9.0c   -- made zero-length BZ_FLUSH work correctly in bzCompress().
     fixed bzWrite/bzRead to ignore zero-length requests.
     fixed bzread to correctly handle read requests after EOF.
     wrong parameter order in call to bzDecompressInit in
     bzBuffToBuffDecompress.  Fixed.
*/

// #include "bzlib_private.h"


/*---------------------------------------------------*/
/*--- Compression stuff                           ---*/
/*---------------------------------------------------*/


/*---------------------------------------------------*/

/*---------------------------------------------------*/
static
int bz_config_ok ( void )
{
	if ( sizeof( int )   != 4 ) { return 0; }

	if ( sizeof( short ) != 2 ) { return 0; }

	if ( sizeof( char )  != 1 ) { return 0; }

	return 1;
}


/*---------------------------------------------------*/
static
void* default_bzalloc ( void* opaque, Int32 items, Int32 size )
{
	void* v = malloc ( items * size );
	return v;
}

static
void default_bzfree ( void* opaque, void* addr )
{
	if ( addr != NULL ) { free ( addr ); }
}


/*---------------------------------------------------*/
static
void prepare_new_block ( EState* s )
{
	Int32 i;
	s->nblock = 0;
	s->numZ = 0;
	s->state_out_pos = 0;
	BZ_INITIALISE_CRC ( s->blockCRC );

	for ( i = 0; i < 256; i++ ) { s->inUse[i] = False; }

	s->blockNo++;
}


/*---------------------------------------------------*/
static
void init_RL ( EState* s )
{
	s->state_in_ch  = 256;
	s->state_in_len = 0;
}


static
Bool isempty_RL ( EState* s )
{
	if ( s->state_in_ch < 256 && s->state_in_len > 0 )
	{
		return False;
	}
	else
	{
		return True;
	}
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzCompressInit )
( bz_stream* strm,
  int        blockSize100k,
  int        verbosity,
  int        workFactor )
{
	Int32   n;
	EState* s;

	if ( !bz_config_ok() ) { return BZ_CONFIG_ERROR; }

	if ( strm == NULL ||
	     blockSize100k < 1 || blockSize100k > 9 ||
	     workFactor < 0 || workFactor > 250 )
	{
		return BZ_PARAM_ERROR;
	}

	if ( workFactor == 0 ) { workFactor = 30; }

	if ( strm->bzalloc == NULL ) { strm->bzalloc = default_bzalloc; }

	if ( strm->bzfree == NULL ) { strm->bzfree = default_bzfree; }

	s = BZALLOC( sizeof( EState ) );

	if ( s == NULL ) { return BZ_MEM_ERROR; }

	s->strm = strm;

	s->arr1 = NULL;
	s->arr2 = NULL;
	s->ftab = NULL;

	n       = 100000 * blockSize100k;
	s->arr1 = BZALLOC( n                  * sizeof( UInt32 ) );
	s->arr2 = BZALLOC( ( n + BZ_N_OVERSHOOT ) * sizeof( UInt32 ) );
	s->ftab = BZALLOC( 65537              * sizeof( UInt32 ) );

	if ( s->arr1 == NULL || s->arr2 == NULL || s->ftab == NULL )
	{
		if ( s->arr1 != NULL ) { BZFREE( s->arr1 ); }

		if ( s->arr2 != NULL ) { BZFREE( s->arr2 ); }

		if ( s->ftab != NULL ) { BZFREE( s->ftab ); }

		if ( s       != NULL ) { BZFREE( s ); }

		return BZ_MEM_ERROR;
	}

	s->blockNo           = 0;
	s->state             = BZ_S_INPUT;
	s->mode              = BZ_M_RUNNING;
	s->combinedCRC       = 0;
	s->blockSize100k     = blockSize100k;
	s->nblockMAX         = 100000 * blockSize100k - 19;
	s->verbosity         = verbosity;
	s->workFactor        = workFactor;

	s->block             = ( UChar* )s->arr2;
	s->mtfv              = ( UInt16* )s->arr1;
	s->zbits             = NULL;
	s->ptr               = ( UInt32* )s->arr1;

	strm->state          = s;
	strm->total_in_lo32  = 0;
	strm->total_in_hi32  = 0;
	strm->total_out_lo32 = 0;
	strm->total_out_hi32 = 0;
	init_RL ( s );
	prepare_new_block ( s );
	return BZ_OK;
}


/*---------------------------------------------------*/
static
void add_pair_to_block ( EState* s )
{
	Int32 i;
	UChar ch = ( UChar )( s->state_in_ch );

	for ( i = 0; i < s->state_in_len; i++ )
	{
		BZ_UPDATE_CRC( s->blockCRC, ch );
	}

	s->inUse[s->state_in_ch] = True;

	switch ( s->state_in_len )
	{
		case 1:
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			break;

		case 2:
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			break;

		case 3:
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			break;

		default:
			s->inUse[s->state_in_len - 4] = True;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( UChar )ch;
			s->nblock++;
			s->block[s->nblock] = ( ( UChar )( s->state_in_len - 4 ) );
			s->nblock++;
			break;
	}
}


/*---------------------------------------------------*/
static
void flush_RL ( EState* s )
{
	if ( s->state_in_ch < 256 ) { add_pair_to_block ( s ); }

	init_RL ( s );
}


/*---------------------------------------------------*/
#define ADD_CHAR_TO_BLOCK(zs,zchh0)               \
{                                                 \
   UInt32 zchh = (UInt32)(zchh0);                 \
   /*-- fast track the common case --*/           \
   if (zchh != zs->state_in_ch &&                 \
       zs->state_in_len == 1) {                   \
      UChar ch = (UChar)(zs->state_in_ch);        \
      BZ_UPDATE_CRC( zs->blockCRC, ch );          \
      zs->inUse[zs->state_in_ch] = True;          \
      zs->block[zs->nblock] = (UChar)ch;          \
      zs->nblock++;                               \
      zs->state_in_ch = zchh;                     \
   }                                              \
   else                                           \
   /*-- general, uncommon cases --*/              \
   if (zchh != zs->state_in_ch ||                 \
      zs->state_in_len == 255) {                  \
      if (zs->state_in_ch < 256)                  \
         add_pair_to_block ( zs );                \
      zs->state_in_ch = zchh;                     \
      zs->state_in_len = 1;                       \
   } else {                                       \
      zs->state_in_len++;                         \
   }                                              \
}


/*---------------------------------------------------*/
static
Bool copy_input_until_stop ( EState* s )
{
	Bool progress_in = False;

	if ( s->mode == BZ_M_RUNNING )
	{

		/*-- fast track the common case --*/
		while ( True )
		{
			/*-- block full? --*/
			if ( s->nblock >= s->nblockMAX ) { break; }

			/*-- no input? --*/
			if ( s->strm->avail_in == 0 ) { break; }

			progress_in = True;
			ADD_CHAR_TO_BLOCK ( s, ( UInt32 )( *( ( UChar* )( s->strm->next_in ) ) ) );
			s->strm->next_in++;
			s->strm->avail_in--;
			s->strm->total_in_lo32++;

			if ( s->strm->total_in_lo32 == 0 ) { s->strm->total_in_hi32++; }
		}

	}
	else
	{

		/*-- general, uncommon case --*/
		while ( True )
		{
			/*-- block full? --*/
			if ( s->nblock >= s->nblockMAX ) { break; }

			/*-- no input? --*/
			if ( s->strm->avail_in == 0 ) { break; }

			/*-- flush/finish end? --*/
			if ( s->avail_in_expect == 0 ) { break; }

			progress_in = True;
			ADD_CHAR_TO_BLOCK ( s, ( UInt32 )( *( ( UChar* )( s->strm->next_in ) ) ) );
			s->strm->next_in++;
			s->strm->avail_in--;
			s->strm->total_in_lo32++;

			if ( s->strm->total_in_lo32 == 0 ) { s->strm->total_in_hi32++; }

			s->avail_in_expect--;
		}
	}

	return progress_in;
}


/*---------------------------------------------------*/
static
Bool copy_output_until_stop ( EState* s )
{
	Bool progress_out = False;

	while ( True )
	{

		/*-- no output space? --*/
		if ( s->strm->avail_out == 0 ) { break; }

		/*-- block done? --*/
		if ( s->state_out_pos >= s->numZ ) { break; }

		progress_out = True;
		*( s->strm->next_out ) = s->zbits[s->state_out_pos];
		s->state_out_pos++;
		s->strm->avail_out--;
		s->strm->next_out++;
		s->strm->total_out_lo32++;

		if ( s->strm->total_out_lo32 == 0 ) { s->strm->total_out_hi32++; }
	}

	return progress_out;
}


/*---------------------------------------------------*/
static
Bool handle_compress ( bz_stream* strm )
{
	Bool progress_in  = False;
	Bool progress_out = False;
	EState* s = strm->state;

	while ( True )
	{

		if ( s->state == BZ_S_OUTPUT )
		{
			progress_out |= copy_output_until_stop ( s );

			if ( s->state_out_pos < s->numZ ) { break; }

			if ( s->mode == BZ_M_FINISHING &&
			     s->avail_in_expect == 0 &&
			     isempty_RL( s ) ) { break; }

			prepare_new_block ( s );
			s->state = BZ_S_INPUT;

			if ( s->mode == BZ_M_FLUSHING &&
			     s->avail_in_expect == 0 &&
			     isempty_RL( s ) ) { break; }
		}

		if ( s->state == BZ_S_INPUT )
		{
			progress_in |= copy_input_until_stop ( s );

			if ( s->mode != BZ_M_RUNNING && s->avail_in_expect == 0 )
			{
				flush_RL ( s );
				BZ2_compressBlock ( s, ( Bool )( s->mode == BZ_M_FINISHING ) );
				s->state = BZ_S_OUTPUT;
			}
			else if ( s->nblock >= s->nblockMAX )
			{
				BZ2_compressBlock ( s, False );
				s->state = BZ_S_OUTPUT;
			}
			else if ( s->strm->avail_in == 0 )
			{
				break;
			}
		}

	}

	return progress_in || progress_out;
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzCompress ) ( bz_stream* strm, int action )
{
	Bool progress;
	EState* s;

	if ( strm == NULL ) { return BZ_PARAM_ERROR; }

	s = strm->state;

	if ( s == NULL ) { return BZ_PARAM_ERROR; }

	if ( s->strm != strm ) { return BZ_PARAM_ERROR; }

preswitch:

	switch ( s->mode )
	{

		case BZ_M_IDLE:
			return BZ_SEQUENCE_ERROR;

		case BZ_M_RUNNING:
			if ( action == BZ_RUN )
			{
				progress = handle_compress ( strm );
				return progress ? BZ_RUN_OK : BZ_PARAM_ERROR;
			}
			else if ( action == BZ_FLUSH )
			{
				s->avail_in_expect = strm->avail_in;
				s->mode = BZ_M_FLUSHING;
				goto preswitch;
			}
			else if ( action == BZ_FINISH )
			{
				s->avail_in_expect = strm->avail_in;
				s->mode = BZ_M_FINISHING;
				goto preswitch;
			}
			else
			{
				return BZ_PARAM_ERROR;
			}

		case BZ_M_FLUSHING:
			if ( action != BZ_FLUSH ) { return BZ_SEQUENCE_ERROR; }

			if ( s->avail_in_expect != s->strm->avail_in )
			{
				return BZ_SEQUENCE_ERROR;
			}

			progress = handle_compress ( strm );

			if ( s->avail_in_expect > 0 || !isempty_RL( s ) ||
			     s->state_out_pos < s->numZ ) { return BZ_FLUSH_OK; }

			s->mode = BZ_M_RUNNING;
			return BZ_RUN_OK;

		case BZ_M_FINISHING:
			if ( action != BZ_FINISH ) { return BZ_SEQUENCE_ERROR; }

			if ( s->avail_in_expect != s->strm->avail_in )
			{
				return BZ_SEQUENCE_ERROR;
			}

			progress = handle_compress ( strm );

			if ( !progress ) { return BZ_SEQUENCE_ERROR; }

			if ( s->avail_in_expect > 0 || !isempty_RL( s ) ||
			     s->state_out_pos < s->numZ ) { return BZ_FINISH_OK; }

			s->mode = BZ_M_IDLE;
			return BZ_STREAM_END;
	}

	return BZ_OK; /*--not reached--*/
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzCompressEnd )  ( bz_stream* strm )
{
	EState* s;

	if ( strm == NULL ) { return BZ_PARAM_ERROR; }

	s = strm->state;

	if ( s == NULL ) { return BZ_PARAM_ERROR; }

	if ( s->strm != strm ) { return BZ_PARAM_ERROR; }

	if ( s->arr1 != NULL ) { BZFREE( s->arr1 ); }

	if ( s->arr2 != NULL ) { BZFREE( s->arr2 ); }

	if ( s->ftab != NULL ) { BZFREE( s->ftab ); }

	BZFREE( strm->state );

	strm->state = NULL;

	return BZ_OK;
}


/*---------------------------------------------------*/
/*--- Decompression stuff                         ---*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
int BZ_API( BZ2_bzDecompressInit )
( bz_stream* strm,
  int        verbosity,
  int        small )
{
	DState* s;

	if ( !bz_config_ok() ) { return BZ_CONFIG_ERROR; }

	if ( strm == NULL ) { return BZ_PARAM_ERROR; }

	if ( small != 0 && small != 1 ) { return BZ_PARAM_ERROR; }

	if ( verbosity < 0 || verbosity > 4 ) { return BZ_PARAM_ERROR; }

	if ( strm->bzalloc == NULL ) { strm->bzalloc = default_bzalloc; }

	if ( strm->bzfree == NULL ) { strm->bzfree = default_bzfree; }

	s = BZALLOC( sizeof( DState ) );

	if ( s == NULL ) { return BZ_MEM_ERROR; }

	s->strm                  = strm;
	strm->state              = s;
	s->state                 = BZ_X_MAGIC_1;
	s->bsLive                = 0;
	s->bsBuff                = 0;
	s->calculatedCombinedCRC = 0;
	strm->total_in_lo32      = 0;
	strm->total_in_hi32      = 0;
	strm->total_out_lo32     = 0;
	strm->total_out_hi32     = 0;
	s->smallDecompress       = ( Bool )small;
	s->ll4                   = NULL;
	s->ll16                  = NULL;
	s->tt                    = NULL;
	s->currBlockNo           = 0;
	s->verbosity             = verbosity;

	return BZ_OK;
}


/*---------------------------------------------------*/
/* Return  True iff data corruption is discovered.
   Returns False if there is no problem.
*/
static
Bool unRLE_obuf_to_output_FAST ( DState* s )
{
	UChar k1;

	if ( s->blockRandomised )
	{

		while ( True )
		{
			/* try to finish existing run */
			while ( True )
			{
				if ( s->strm->avail_out == 0 ) { return False; }

				if ( s->state_out_len == 0 ) { break; }

				*( ( UChar* )( s->strm->next_out ) ) = s->state_out_ch;
				BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->total_out_lo32++;

				if ( s->strm->total_out_lo32 == 0 ) { s->strm->total_out_hi32++; }
			}

			/* can a new run be started? */
			if ( s->nblock_used == s->save_nblock + 1 ) { return False; }

			/* Only caused by corrupt data stream? */
			if ( s->nblock_used > s->save_nblock + 1 )
			{
				return True;
			}

			s->state_out_len = 1;
			s->state_out_ch = s->k0;
			BZ_GET_FAST( k1 );
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK;
			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 2;

			BZ_GET_FAST( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 3;

			BZ_GET_FAST( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			BZ_GET_FAST( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			s->state_out_len = ( ( Int32 )k1 ) + 4;

			BZ_GET_FAST( s->k0 );

			BZ_RAND_UPD_MASK;

			s->k0 ^= BZ_RAND_MASK;

			s->nblock_used++;
		}

	}
	else
	{

		/* restore */
		UInt32        c_calculatedBlockCRC = s->calculatedBlockCRC;
		UChar         c_state_out_ch       = s->state_out_ch;
		Int32         c_state_out_len      = s->state_out_len;
		Int32         c_nblock_used        = s->nblock_used;
		Int32         c_k0                 = s->k0;
		UInt32*       c_tt                 = s->tt;
		UInt32        c_tPos               = s->tPos;
		char*         cs_next_out          = s->strm->next_out;
		unsigned int  cs_avail_out         = s->strm->avail_out;
		Int32         ro_blockSize100k     = s->blockSize100k;
		/* end restore */

		UInt32       avail_out_INIT = cs_avail_out;
		Int32        s_save_nblockPP = s->save_nblock + 1;
		unsigned int total_out_lo32_old;

		while ( True )
		{

			/* try to finish existing run */
			if ( c_state_out_len > 0 )
			{
				while ( True )
				{
					if ( cs_avail_out == 0 ) { goto return_notr; }

					if ( c_state_out_len == 1 ) { break; }

					*( ( UChar* )( cs_next_out ) ) = c_state_out_ch;
					BZ_UPDATE_CRC ( c_calculatedBlockCRC, c_state_out_ch );
					c_state_out_len--;
					cs_next_out++;
					cs_avail_out--;
				}

s_state_out_len_eq_one:
				{
					if ( cs_avail_out == 0 )
					{
						c_state_out_len = 1;
						goto return_notr;
					};

					*( ( UChar* )( cs_next_out ) ) = c_state_out_ch;

					BZ_UPDATE_CRC ( c_calculatedBlockCRC, c_state_out_ch );

					cs_next_out++;

					cs_avail_out--;
				}
			}

			/* Only caused by corrupt data stream? */
			if ( c_nblock_used > s_save_nblockPP )
			{
				return True;
			}

			/* can a new run be started? */
			if ( c_nblock_used == s_save_nblockPP )
			{
				c_state_out_len = 0;
				goto return_notr;
			};

			c_state_out_ch = c_k0;

			BZ_GET_FAST_C( k1 );

			c_nblock_used++;

			if ( k1 != c_k0 )
			{
				c_k0 = k1;
				goto s_state_out_len_eq_one;
			};

			if ( c_nblock_used == s_save_nblockPP )
			{
				goto s_state_out_len_eq_one;
			}

			c_state_out_len = 2;
			BZ_GET_FAST_C( k1 );
			c_nblock_used++;

			if ( c_nblock_used == s_save_nblockPP ) { continue; }

			if ( k1 != c_k0 ) { c_k0 = k1; continue; };

			c_state_out_len = 3;

			BZ_GET_FAST_C( k1 );

			c_nblock_used++;

			if ( c_nblock_used == s_save_nblockPP ) { continue; }

			if ( k1 != c_k0 ) { c_k0 = k1; continue; };

			BZ_GET_FAST_C( k1 );

			c_nblock_used++;

			c_state_out_len = ( ( Int32 )k1 ) + 4;

			BZ_GET_FAST_C( c_k0 );

			c_nblock_used++;
		}

return_notr:
		total_out_lo32_old = s->strm->total_out_lo32;
		s->strm->total_out_lo32 += ( avail_out_INIT - cs_avail_out );

		if ( s->strm->total_out_lo32 < total_out_lo32_old )
		{
			s->strm->total_out_hi32++;
		}

		/* save */
		s->calculatedBlockCRC = c_calculatedBlockCRC;
		s->state_out_ch       = c_state_out_ch;
		s->state_out_len      = c_state_out_len;
		s->nblock_used        = c_nblock_used;
		s->k0                 = c_k0;
		s->tt                 = c_tt;
		s->tPos               = c_tPos;
		s->strm->next_out     = cs_next_out;
		s->strm->avail_out    = cs_avail_out;
		/* end save */
	}

	return False;
}



/*---------------------------------------------------*/
__inline__ Int32 BZ2_indexIntoF ( Int32 indx, Int32* cftab )
{
	Int32 nb, na, mid;
	nb = 0;
	na = 256;

	do
	{
		mid = ( nb + na ) >> 1;

		if ( indx >= cftab[mid] ) { nb = mid; }
		else { na = mid; }
	}
	while ( na - nb != 1 );

	return nb;
}


/*---------------------------------------------------*/
/* Return  True iff data corruption is discovered.
   Returns False if there is no problem.
*/
static
Bool unRLE_obuf_to_output_SMALL ( DState* s )
{
	UChar k1;

	if ( s->blockRandomised )
	{

		while ( True )
		{
			/* try to finish existing run */
			while ( True )
			{
				if ( s->strm->avail_out == 0 ) { return False; }

				if ( s->state_out_len == 0 ) { break; }

				*( ( UChar* )( s->strm->next_out ) ) = s->state_out_ch;
				BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->total_out_lo32++;

				if ( s->strm->total_out_lo32 == 0 ) { s->strm->total_out_hi32++; }
			}

			/* can a new run be started? */
			if ( s->nblock_used == s->save_nblock + 1 ) { return False; }

			/* Only caused by corrupt data stream? */
			if ( s->nblock_used > s->save_nblock + 1 )
			{
				return True;
			}

			s->state_out_len = 1;
			s->state_out_ch = s->k0;
			BZ_GET_SMALL( k1 );
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK;
			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 2;

			BZ_GET_SMALL( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 3;

			BZ_GET_SMALL( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			BZ_GET_SMALL( k1 );

			BZ_RAND_UPD_MASK;

			k1 ^= BZ_RAND_MASK;

			s->nblock_used++;

			s->state_out_len = ( ( Int32 )k1 ) + 4;

			BZ_GET_SMALL( s->k0 );

			BZ_RAND_UPD_MASK;

			s->k0 ^= BZ_RAND_MASK;

			s->nblock_used++;
		}

	}
	else
	{

		while ( True )
		{
			/* try to finish existing run */
			while ( True )
			{
				if ( s->strm->avail_out == 0 ) { return False; }

				if ( s->state_out_len == 0 ) { break; }

				*( ( UChar* )( s->strm->next_out ) ) = s->state_out_ch;
				BZ_UPDATE_CRC ( s->calculatedBlockCRC, s->state_out_ch );
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->total_out_lo32++;

				if ( s->strm->total_out_lo32 == 0 ) { s->strm->total_out_hi32++; }
			}

			/* can a new run be started? */
			if ( s->nblock_used == s->save_nblock + 1 ) { return False; }

			/* Only caused by corrupt data stream? */
			if ( s->nblock_used > s->save_nblock + 1 )
			{
				return True;
			}

			s->state_out_len = 1;
			s->state_out_ch = s->k0;
			BZ_GET_SMALL( k1 );
			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 2;

			BZ_GET_SMALL( k1 );

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			s->state_out_len = 3;

			BZ_GET_SMALL( k1 );

			s->nblock_used++;

			if ( s->nblock_used == s->save_nblock + 1 ) { continue; }

			if ( k1 != s->k0 ) { s->k0 = k1; continue; };

			BZ_GET_SMALL( k1 );

			s->nblock_used++;

			s->state_out_len = ( ( Int32 )k1 ) + 4;

			BZ_GET_SMALL( s->k0 );

			s->nblock_used++;
		}

	}
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzDecompress ) ( bz_stream* strm )
{
	Bool    corrupt;
	DState* s;

	if ( strm == NULL ) { return BZ_PARAM_ERROR; }

	s = strm->state;

	if ( s == NULL ) { return BZ_PARAM_ERROR; }

	if ( s->strm != strm ) { return BZ_PARAM_ERROR; }

	while ( True )
	{
		if ( s->state == BZ_X_IDLE ) { return BZ_SEQUENCE_ERROR; }

		if ( s->state == BZ_X_OUTPUT )
		{
			if ( s->smallDecompress )
			{
				corrupt = unRLE_obuf_to_output_SMALL ( s );
			}
			else
			{
				corrupt = unRLE_obuf_to_output_FAST  ( s );
			}

			if ( corrupt ) { return BZ_DATA_ERROR; }

			if ( s->nblock_used == s->save_nblock + 1 && s->state_out_len == 0 )
			{
				BZ_FINALISE_CRC ( s->calculatedBlockCRC );

				if ( s->verbosity >= 3 )
					VPrintf2 ( " {0x%08x, 0x%08x}", s->storedBlockCRC,
					           s->calculatedBlockCRC );

				if ( s->verbosity >= 2 ) { VPrintf0 ( "]" ); }

				if ( s->calculatedBlockCRC != s->storedBlockCRC )
				{
					return BZ_DATA_ERROR;
				}

				s->calculatedCombinedCRC
				   = ( s->calculatedCombinedCRC << 1 ) |
				     ( s->calculatedCombinedCRC >> 31 );
				s->calculatedCombinedCRC ^= s->calculatedBlockCRC;
				s->state = BZ_X_BLKHDR_1;
			}
			else
			{
				return BZ_OK;
			}
		}

		if ( s->state >= BZ_X_MAGIC_1 )
		{
			Int32 r = BZ2_decompress ( s );

			if ( r == BZ_STREAM_END )
			{
				if ( s->verbosity >= 3 )
					VPrintf2 ( "\n    combined CRCs: stored = 0x%08x, computed = 0x%08x",
					           s->storedCombinedCRC, s->calculatedCombinedCRC );

				if ( s->calculatedCombinedCRC != s->storedCombinedCRC )
				{
					return BZ_DATA_ERROR;
				}

				return r;
			}

			if ( s->state != BZ_X_OUTPUT ) { return r; }
		}
	}

	AssertH ( 0, 6001 );

	return 0;  /*NOTREACHED*/
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzDecompressEnd )  ( bz_stream* strm )
{
	DState* s;

	if ( strm == NULL ) { return BZ_PARAM_ERROR; }

	s = strm->state;

	if ( s == NULL ) { return BZ_PARAM_ERROR; }

	if ( s->strm != strm ) { return BZ_PARAM_ERROR; }

	if ( s->tt   != NULL ) { BZFREE( s->tt ); }

	if ( s->ll16 != NULL ) { BZFREE( s->ll16 ); }

	if ( s->ll4  != NULL ) { BZFREE( s->ll4 ); }

	BZFREE( strm->state );
	strm->state = NULL;

	return BZ_OK;
}

/*---------------------------------------------------*/
/*--- Misc convenience stuff                      ---*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
int BZ_API( BZ2_bzBuffToBuffCompress )
( char*         dest,
  unsigned int* destLen,
  char*         source,
  unsigned int  sourceLen,
  int           blockSize100k,
  int           verbosity,
  int           workFactor )
{
	bz_stream strm;
	int ret;

	if ( dest == NULL || destLen == NULL ||
	     source == NULL ||
	     blockSize100k < 1 || blockSize100k > 9 ||
	     verbosity < 0 || verbosity > 4 ||
	     workFactor < 0 || workFactor > 250 )
	{
		return BZ_PARAM_ERROR;
	}

	if ( workFactor == 0 ) { workFactor = 30; }

	strm.bzalloc = NULL;
	strm.bzfree = NULL;
	strm.opaque = NULL;
	ret = BZ2_bzCompressInit ( &strm, blockSize100k,
	                           verbosity, workFactor );

	if ( ret != BZ_OK ) { return ret; }

	strm.next_in = source;
	strm.next_out = dest;
	strm.avail_in = sourceLen;
	strm.avail_out = *destLen;

	ret = BZ2_bzCompress ( &strm, BZ_FINISH );

	if ( ret == BZ_FINISH_OK ) { goto output_overflow; }

	if ( ret != BZ_STREAM_END ) { goto errhandler; }

	/* normal termination */
	*destLen -= strm.avail_out;
	BZ2_bzCompressEnd ( &strm );
	return BZ_OK;

output_overflow:
	BZ2_bzCompressEnd ( &strm );
	return BZ_OUTBUFF_FULL;

errhandler:
	BZ2_bzCompressEnd ( &strm );
	return ret;
}


/*---------------------------------------------------*/
int BZ_API( BZ2_bzBuffToBuffDecompress )
( char*         dest,
  unsigned int* destLen,
  char*         source,
  unsigned int  sourceLen,
  int           small,
  int           verbosity )
{
	bz_stream strm;
	int ret;

	if ( dest == NULL || destLen == NULL ||
	     source == NULL ||
	     ( small != 0 && small != 1 ) ||
	     verbosity < 0 || verbosity > 4 )
	{
		return BZ_PARAM_ERROR;
	}

	strm.bzalloc = NULL;
	strm.bzfree = NULL;
	strm.opaque = NULL;
	ret = BZ2_bzDecompressInit ( &strm, verbosity, small );

	if ( ret != BZ_OK ) { return ret; }

	strm.next_in = source;
	strm.next_out = dest;
	strm.avail_in = sourceLen;
	strm.avail_out = *destLen;

	ret = BZ2_bzDecompress ( &strm );

	if ( ret == BZ_OK ) { goto output_overflow_or_eof; }

	if ( ret != BZ_STREAM_END ) { goto errhandler; }

	/* normal termination */
	*destLen -= strm.avail_out;
	BZ2_bzDecompressEnd ( &strm );
	return BZ_OK;

output_overflow_or_eof:

	if ( strm.avail_out > 0 )
	{
		BZ2_bzDecompressEnd ( &strm );
		return BZ_UNEXPECTED_EOF;
	}
	else
	{
		BZ2_bzDecompressEnd ( &strm );
		return BZ_OUTBUFF_FULL;
	};

errhandler:
	BZ2_bzDecompressEnd ( &strm );

	return ret;
}


/*---------------------------------------------------*/
/*--
   Code contributed by Yoshioka Tsuneo (tsuneo@rr.iij4u.or.jp)
   to support better zlib compatibility.
   This code is not _officially_ part of libbzip2 (yet);
   I haven't tested it, documented it, or considered the
   threading-safeness of it.
   If this code breaks, please contact both Yoshioka and me.
--*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
/*--
   return version like "0.9.5d, 4-Sept-1999".
--*/
const char* BZ_API( BZ2_bzlibVersion )( void )
{
	return BZ_VERSION;
}

/*-------------------------------------------------------------*/
/*--- end                                           bzlib.c ---*/
/*-------------------------------------------------------------*/



/*-------------------------------------------------------------*/
/*--- Compression machinery (not incl block sorting)        ---*/
/*---                                            compress.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */


/* CHANGES
    0.9.0    -- original version.
    0.9.0a/b -- no changes in this file.
    0.9.0c   -- changed setting of nGroups in sendMTFValues()
                so as to do a bit better on small files
*/


/*---------------------------------------------------*/
/*--- Bit stream I/O                              ---*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
void BZ2_bsInitWrite ( EState* s )
{
	s->bsLive = 0;
	s->bsBuff = 0;
}


/*---------------------------------------------------*/
static
void bsFinishWrite ( EState* s )
{
	while ( s->bsLive > 0 )
	{
		s->zbits[s->numZ] = ( UChar )( s->bsBuff >> 24 );
		s->numZ++;
		s->bsBuff <<= 8;
		s->bsLive -= 8;
	}
}


/*---------------------------------------------------*/
#define bsNEEDW(nz)                           \
{                                             \
   while (s->bsLive >= 8) {                   \
      s->zbits[s->numZ]                       \
         = (UChar)(s->bsBuff >> 24);          \
      s->numZ++;                              \
      s->bsBuff <<= 8;                        \
      s->bsLive -= 8;                         \
   }                                          \
}


/*---------------------------------------------------*/
static
__inline__
void bsW ( EState* s, Int32 n, UInt32 v )
{
	bsNEEDW ( n );
	s->bsBuff |= ( v << ( 32 - s->bsLive - n ) );
	s->bsLive += n;
}


/*---------------------------------------------------*/
static
void bsPutUInt32 ( EState* s, UInt32 u )
{
	bsW ( s, 8, ( u >> 24 ) & 0xffL );
	bsW ( s, 8, ( u >> 16 ) & 0xffL );
	bsW ( s, 8, ( u >>  8 ) & 0xffL );
	bsW ( s, 8,  u        & 0xffL );
}


/*---------------------------------------------------*/
static
void bsPutUChar ( EState* s, UChar c )
{
	bsW( s, 8, ( UInt32 )c );
}


/*---------------------------------------------------*/
/*--- The back end proper                         ---*/
/*---------------------------------------------------*/

/*---------------------------------------------------*/
static
void makeMaps_e ( EState* s )
{
	Int32 i;
	s->nInUse = 0;

	for ( i = 0; i < 256; i++ )
		if ( s->inUse[i] )
		{
			s->unseqToSeq[i] = s->nInUse;
			s->nInUse++;
		}
}


/*---------------------------------------------------*/
static
void generateMTFValues ( EState* s )
{
	UChar   yy[256];
	Int32   i, j;
	Int32   zPend;
	Int32   wr;
	Int32   EOB;

	/*
	   After sorting (eg, here),
	      s->arr1 [ 0 .. s->nblock-1 ] holds sorted order,
	      and
	      ((UChar*)s->arr2) [ 0 .. s->nblock-1 ]
	      holds the original block data.

	   The first thing to do is generate the MTF values,
	   and put them in
	      ((UInt16*)s->arr1) [ 0 .. s->nblock-1 ].
	   Because there are strictly fewer or equal MTF values
	   than block values, ptr values in this area are overwritten
	   with MTF values only when they are no longer needed.

	   The final compressed bitstream is generated into the
	   area starting at
	      (UChar*) (&((UChar*)s->arr2)[s->nblock])

	   These storage aliases are set up in bzCompressInit(),
	   except for the last one, which is arranged in
	   compressBlock().
	*/
	UInt32* ptr   = s->ptr;
	UChar* block  = s->block;
	UInt16* mtfv  = s->mtfv;

	makeMaps_e ( s );
	EOB = s->nInUse + 1;

	for ( i = 0; i <= EOB; i++ ) { s->mtfFreq[i] = 0; }

	wr = 0;
	zPend = 0;

	for ( i = 0; i < s->nInUse; i++ ) { yy[i] = ( UChar ) i; }

	for ( i = 0; i < s->nblock; i++ )
	{
		UChar ll_i;
		AssertD ( wr <= i, "generateMTFValues(1)" );
		j = ptr[i] - 1;

		if ( j < 0 ) { j += s->nblock; }

		ll_i = s->unseqToSeq[block[j]];
		AssertD ( ll_i < s->nInUse, "generateMTFValues(2a)" );

		if ( yy[0] == ll_i )
		{
			zPend++;
		}
		else
		{

			if ( zPend > 0 )
			{
				zPend--;

				while ( True )
				{
					if ( zPend & 1 )
					{
						mtfv[wr] = BZ_RUNB;
						wr++;
						s->mtfFreq[BZ_RUNB]++;
					}
					else
					{
						mtfv[wr] = BZ_RUNA;
						wr++;
						s->mtfFreq[BZ_RUNA]++;
					}

					if ( zPend < 2 ) { break; }

					zPend = ( zPend - 2 ) / 2;
				};

				zPend = 0;
			}

			{
				register UChar  rtmp;
				register UChar* ryy_j;
				register UChar  rll_i;
				rtmp  = yy[1];
				yy[1] = yy[0];
				ryy_j = &( yy[1] );
				rll_i = ll_i;

				while ( rll_i != rtmp )
				{
					register UChar rtmp2;
					ryy_j++;
					rtmp2  = rtmp;
					rtmp   = *ryy_j;
					*ryy_j = rtmp2;
				};

				yy[0] = rtmp;

				j = ryy_j - &( yy[0] );

				mtfv[wr] = j + 1;

				wr++;

				s->mtfFreq[j + 1]++;
			}

		}
	}

	if ( zPend > 0 )
	{
		zPend--;

		while ( True )
		{
			if ( zPend & 1 )
			{
				mtfv[wr] = BZ_RUNB;
				wr++;
				s->mtfFreq[BZ_RUNB]++;
			}
			else
			{
				mtfv[wr] = BZ_RUNA;
				wr++;
				s->mtfFreq[BZ_RUNA]++;
			}

			if ( zPend < 2 ) { break; }

			zPend = ( zPend - 2 ) / 2;
		};

		zPend = 0;
	}

	mtfv[wr] = EOB;
	wr++;
	s->mtfFreq[EOB]++;

	s->nMTF = wr;
}


/*---------------------------------------------------*/
#define BZ_LESSER_ICOST  0
#define BZ_GREATER_ICOST 15

static
void sendMTFValues ( EState* s )
{
	Int32 v, t, i, j, gs, ge, totc, bt, bc, iter;
	Int32 nSelectors, alphaSize, minLen, maxLen, selCtr;
	Int32 nGroups, nBytes;

	/*--
	UChar  len [BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	is a global since the decoder also needs it.

	Int32  code[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	Int32  rfreq[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];
	are also globals only used in this proc.
	Made global to keep stack frame size small.
	--*/


	UInt16 cost[BZ_N_GROUPS];
	Int32  fave[BZ_N_GROUPS];

	UInt16* mtfv = s->mtfv;

	if ( s->verbosity >= 3 )
		VPrintf3( "      %d in block, %d after MTF & 1-2 coding, "
		          "%d+2 syms in use\n",
		          s->nblock, s->nMTF, s->nInUse );

	alphaSize = s->nInUse + 2;

	for ( t = 0; t < BZ_N_GROUPS; t++ )
		for ( v = 0; v < alphaSize; v++ )
		{
			s->len[t][v] = BZ_GREATER_ICOST;
		}

	/*--- Decide how many coding tables to use ---*/
	AssertH ( s->nMTF > 0, 3001 );

	if ( s->nMTF < 200 ) { nGroups = 2; }
	else if ( s->nMTF < 600 ) { nGroups = 3; }
	else if ( s->nMTF < 1200 ) { nGroups = 4; }
	else if ( s->nMTF < 2400 ) { nGroups = 5; }
	else
	{
		nGroups = 6;
	}

	/*--- Generate an initial set of coding tables ---*/
	{
		Int32 nPart, remF, tFreq, aFreq;

		nPart = nGroups;
		remF  = s->nMTF;
		gs = 0;

		while ( nPart > 0 )
		{
			tFreq = remF / nPart;
			ge = gs - 1;
			aFreq = 0;

			while ( aFreq < tFreq && ge < alphaSize - 1 )
			{
				ge++;
				aFreq += s->mtfFreq[ge];
			}

			if ( ge > gs
			     && nPart != nGroups && nPart != 1
			     && ( ( nGroups - nPart ) % 2 == 1 ) )
			{
				aFreq -= s->mtfFreq[ge];
				ge--;
			}

			if ( s->verbosity >= 3 )
				VPrintf5( "      initial group %d, [%d .. %d], "
				          "has %d syms (%4.1f%%)\n",
				          nPart, gs, ge, aFreq,
				          ( 100.0 * ( float )aFreq ) / ( float )( s->nMTF ) );

			for ( v = 0; v < alphaSize; v++ )
				if ( v >= gs && v <= ge )
				{
					s->len[nPart - 1][v] = BZ_LESSER_ICOST;
				}
				else
				{
					s->len[nPart - 1][v] = BZ_GREATER_ICOST;
				}

			nPart--;
			gs = ge + 1;
			remF -= aFreq;
		}
	}

	/*---
	   Iterate up to BZ_N_ITERS times to improve the tables.
	---*/
	for ( iter = 0; iter < BZ_N_ITERS; iter++ )
	{

		for ( t = 0; t < nGroups; t++ ) { fave[t] = 0; }

		for ( t = 0; t < nGroups; t++ )
			for ( v = 0; v < alphaSize; v++ )
			{
				s->rfreq[t][v] = 0;
			}

		/*---
		  Set up an auxiliary length table which is used to fast-track
		the common case (nGroups == 6).
		     ---*/
		if ( nGroups == 6 )
		{
			for ( v = 0; v < alphaSize; v++ )
			{
				s->len_pack[v][0] = ( s->len[1][v] << 16 ) | s->len[0][v];
				s->len_pack[v][1] = ( s->len[3][v] << 16 ) | s->len[2][v];
				s->len_pack[v][2] = ( s->len[5][v] << 16 ) | s->len[4][v];
			}
		}

		nSelectors = 0;
		totc = 0;
		gs = 0;

		while ( True )
		{

			/*--- Set group start & end marks. --*/
			if ( gs >= s->nMTF ) { break; }

			ge = gs + BZ_G_SIZE - 1;

			if ( ge >= s->nMTF ) { ge = s->nMTF - 1; }

			/*--
			   Calculate the cost of this group as coded
			   by each of the coding tables.
			--*/
			for ( t = 0; t < nGroups; t++ ) { cost[t] = 0; }

			if ( nGroups == 6 && 50 == ge - gs + 1 )
			{
				/*--- fast track the common case ---*/
				register UInt32 cost01, cost23, cost45;
				register UInt16 icv;
				cost01 = cost23 = cost45 = 0;

#           define BZ_ITER(nn)                \
               icv = mtfv[gs+(nn)];           \
               cost01 += s->len_pack[icv][0]; \
               cost23 += s->len_pack[icv][1]; \
               cost45 += s->len_pack[icv][2]; \
 
				BZ_ITER( 0 );
				BZ_ITER( 1 );
				BZ_ITER( 2 );
				BZ_ITER( 3 );
				BZ_ITER( 4 );
				BZ_ITER( 5 );
				BZ_ITER( 6 );
				BZ_ITER( 7 );
				BZ_ITER( 8 );
				BZ_ITER( 9 );
				BZ_ITER( 10 );
				BZ_ITER( 11 );
				BZ_ITER( 12 );
				BZ_ITER( 13 );
				BZ_ITER( 14 );
				BZ_ITER( 15 );
				BZ_ITER( 16 );
				BZ_ITER( 17 );
				BZ_ITER( 18 );
				BZ_ITER( 19 );
				BZ_ITER( 20 );
				BZ_ITER( 21 );
				BZ_ITER( 22 );
				BZ_ITER( 23 );
				BZ_ITER( 24 );
				BZ_ITER( 25 );
				BZ_ITER( 26 );
				BZ_ITER( 27 );
				BZ_ITER( 28 );
				BZ_ITER( 29 );
				BZ_ITER( 30 );
				BZ_ITER( 31 );
				BZ_ITER( 32 );
				BZ_ITER( 33 );
				BZ_ITER( 34 );
				BZ_ITER( 35 );
				BZ_ITER( 36 );
				BZ_ITER( 37 );
				BZ_ITER( 38 );
				BZ_ITER( 39 );
				BZ_ITER( 40 );
				BZ_ITER( 41 );
				BZ_ITER( 42 );
				BZ_ITER( 43 );
				BZ_ITER( 44 );
				BZ_ITER( 45 );
				BZ_ITER( 46 );
				BZ_ITER( 47 );
				BZ_ITER( 48 );
				BZ_ITER( 49 );

#           undef BZ_ITER

				cost[0] = cost01 & 0xffff;
				cost[1] = cost01 >> 16;
				cost[2] = cost23 & 0xffff;
				cost[3] = cost23 >> 16;
				cost[4] = cost45 & 0xffff;
				cost[5] = cost45 >> 16;

			}
			else
			{
				/*--- slow version which correctly handles all situations ---*/
				for ( i = gs; i <= ge; i++ )
				{
					UInt16 icv = mtfv[i];

					for ( t = 0; t < nGroups; t++ ) { cost[t] += s->len[t][icv]; }
				}
			}

			/*--
			   Find the coding table which is best for this group,
			   and record its identity in the selector table.
			--*/
			bc = 999999999;
			bt = -1;

			for ( t = 0; t < nGroups; t++ )
				if ( cost[t] < bc ) { bc = cost[t]; bt = t; };

			totc += bc;

			fave[bt]++;

			s->selector[nSelectors] = bt;

			nSelectors++;

			/*--
			   Increment the symbol frequencies for the selected table.
			 --*/
			if ( nGroups == 6 && 50 == ge - gs + 1 )
			{
				/*--- fast track the common case ---*/

#           define BZ_ITUR(nn) s->rfreq[bt][ mtfv[gs+(nn)] ]++

				BZ_ITUR( 0 );
				BZ_ITUR( 1 );
				BZ_ITUR( 2 );
				BZ_ITUR( 3 );
				BZ_ITUR( 4 );
				BZ_ITUR( 5 );
				BZ_ITUR( 6 );
				BZ_ITUR( 7 );
				BZ_ITUR( 8 );
				BZ_ITUR( 9 );
				BZ_ITUR( 10 );
				BZ_ITUR( 11 );
				BZ_ITUR( 12 );
				BZ_ITUR( 13 );
				BZ_ITUR( 14 );
				BZ_ITUR( 15 );
				BZ_ITUR( 16 );
				BZ_ITUR( 17 );
				BZ_ITUR( 18 );
				BZ_ITUR( 19 );
				BZ_ITUR( 20 );
				BZ_ITUR( 21 );
				BZ_ITUR( 22 );
				BZ_ITUR( 23 );
				BZ_ITUR( 24 );
				BZ_ITUR( 25 );
				BZ_ITUR( 26 );
				BZ_ITUR( 27 );
				BZ_ITUR( 28 );
				BZ_ITUR( 29 );
				BZ_ITUR( 30 );
				BZ_ITUR( 31 );
				BZ_ITUR( 32 );
				BZ_ITUR( 33 );
				BZ_ITUR( 34 );
				BZ_ITUR( 35 );
				BZ_ITUR( 36 );
				BZ_ITUR( 37 );
				BZ_ITUR( 38 );
				BZ_ITUR( 39 );
				BZ_ITUR( 40 );
				BZ_ITUR( 41 );
				BZ_ITUR( 42 );
				BZ_ITUR( 43 );
				BZ_ITUR( 44 );
				BZ_ITUR( 45 );
				BZ_ITUR( 46 );
				BZ_ITUR( 47 );
				BZ_ITUR( 48 );
				BZ_ITUR( 49 );

#           undef BZ_ITUR

			}
			else
			{
				/*--- slow version which correctly handles all situations ---*/
				for ( i = gs; i <= ge; i++ )
				{
					s->rfreq[bt][ mtfv[i] ]++;
				}
			}

			gs = ge + 1;
		}

		if ( s->verbosity >= 3 )
		{
			VPrintf2 ( "      pass %d: size is %d, grp uses are ",
			           iter + 1, totc / 8 );

			for ( t = 0; t < nGroups; t++ )
			{
				VPrintf1 ( "%d ", fave[t] );
			}

			VPrintf0 ( "\n" );
		}

		/*--
		  Recompute the tables based on the accumulated frequencies.
		--*/
		/* maxLen was changed from 20 to 17 in bzip2-1.0.3.  See
		   comment in huffman.c for details. */
		for ( t = 0; t < nGroups; t++ )
			BZ2_hbMakeCodeLengths ( &( s->len[t][0] ), &( s->rfreq[t][0] ),
			                        alphaSize, 17 /*20*/ );
	}


	AssertH( nGroups < 8, 3002 );
	AssertH( nSelectors < 32768 &&
	         nSelectors <= ( 2 + ( 900000 / BZ_G_SIZE ) ),
	         3003 );


	/*--- Compute MTF values for the selectors. ---*/
	{
		UChar pos[BZ_N_GROUPS], ll_i, tmp2, tmp;

		for ( i = 0; i < nGroups; i++ ) { pos[i] = i; }

		for ( i = 0; i < nSelectors; i++ )
		{
			ll_i = s->selector[i];
			j = 0;
			tmp = pos[j];

			while ( ll_i != tmp )
			{
				j++;
				tmp2 = tmp;
				tmp = pos[j];
				pos[j] = tmp2;
			};

			pos[0] = tmp;

			s->selectorMtf[i] = j;
		}
	};

	/*--- Assign actual codes for the tables. --*/
	for ( t = 0; t < nGroups; t++ )
	{
		minLen = 32;
		maxLen = 0;

		for ( i = 0; i < alphaSize; i++ )
		{
			if ( s->len[t][i] > maxLen ) { maxLen = s->len[t][i]; }

			if ( s->len[t][i] < minLen ) { minLen = s->len[t][i]; }
		}

		AssertH ( !( maxLen > 17 /*20*/ ), 3004 );
		AssertH ( !( minLen < 1 ),  3005 );
		BZ2_hbAssignCodes ( &( s->code[t][0] ), &( s->len[t][0] ),
		                    minLen, maxLen, alphaSize );
	}

	/*--- Transmit the mapping table. ---*/
	{
		Bool inUse16[16];

		for ( i = 0; i < 16; i++ )
		{
			inUse16[i] = False;

			for ( j = 0; j < 16; j++ )
				if ( s->inUse[i * 16 + j] ) { inUse16[i] = True; }
		}

		nBytes = s->numZ;

		for ( i = 0; i < 16; i++ )
			if ( inUse16[i] ) { bsW( s, 1, 1 ); }
			else { bsW( s, 1, 0 ); }

		for ( i = 0; i < 16; i++ )
			if ( inUse16[i] )
				for ( j = 0; j < 16; j++ )
				{
					if ( s->inUse[i * 16 + j] ) { bsW( s, 1, 1 ); }
					else { bsW( s, 1, 0 ); }
				}

		if ( s->verbosity >= 3 )
		{
			VPrintf1( "      bytes: mapping %d, ", s->numZ - nBytes );
		}
	}

	/*--- Now the selectors. ---*/
	nBytes = s->numZ;
	bsW ( s, 3, nGroups );
	bsW ( s, 15, nSelectors );

	for ( i = 0; i < nSelectors; i++ )
	{
		for ( j = 0; j < s->selectorMtf[i]; j++ ) { bsW( s, 1, 1 ); }

		bsW( s, 1, 0 );
	}

	if ( s->verbosity >= 3 )
	{
		VPrintf1( "selectors %d, ", s->numZ - nBytes );
	}

	/*--- Now the coding tables. ---*/
	nBytes = s->numZ;

	for ( t = 0; t < nGroups; t++ )
	{
		Int32 curr = s->len[t][0];
		bsW ( s, 5, curr );

		for ( i = 0; i < alphaSize; i++ )
		{
			while ( curr < s->len[t][i] ) { bsW( s, 2, 2 ); curr++; /* 10 */ };

			while ( curr > s->len[t][i] ) { bsW( s, 2, 3 ); curr--; /* 11 */ };

			bsW ( s, 1, 0 );
		}
	}

	if ( s->verbosity >= 3 )
	{
		VPrintf1 ( "code lengths %d, ", s->numZ - nBytes );
	}

	/*--- And finally, the block data proper ---*/
	nBytes = s->numZ;
	selCtr = 0;
	gs = 0;

	while ( True )
	{
		if ( gs >= s->nMTF ) { break; }

		ge = gs + BZ_G_SIZE - 1;

		if ( ge >= s->nMTF ) { ge = s->nMTF - 1; }

		AssertH ( s->selector[selCtr] < nGroups, 3006 );

		if ( nGroups == 6 && 50 == ge - gs + 1 )
		{
			/*--- fast track the common case ---*/
			UInt16 mtfv_i;
			UChar* s_len_sel_selCtr
			   = &( s->len[s->selector[selCtr]][0] );
			Int32* s_code_sel_selCtr
			   = &( s->code[s->selector[selCtr]][0] );

#           define BZ_ITAH(nn)                      \
               mtfv_i = mtfv[gs+(nn)];              \
               bsW ( s,                             \
                     s_len_sel_selCtr[mtfv_i],      \
                     s_code_sel_selCtr[mtfv_i] )

			BZ_ITAH( 0 );
			BZ_ITAH( 1 );
			BZ_ITAH( 2 );
			BZ_ITAH( 3 );
			BZ_ITAH( 4 );
			BZ_ITAH( 5 );
			BZ_ITAH( 6 );
			BZ_ITAH( 7 );
			BZ_ITAH( 8 );
			BZ_ITAH( 9 );
			BZ_ITAH( 10 );
			BZ_ITAH( 11 );
			BZ_ITAH( 12 );
			BZ_ITAH( 13 );
			BZ_ITAH( 14 );
			BZ_ITAH( 15 );
			BZ_ITAH( 16 );
			BZ_ITAH( 17 );
			BZ_ITAH( 18 );
			BZ_ITAH( 19 );
			BZ_ITAH( 20 );
			BZ_ITAH( 21 );
			BZ_ITAH( 22 );
			BZ_ITAH( 23 );
			BZ_ITAH( 24 );
			BZ_ITAH( 25 );
			BZ_ITAH( 26 );
			BZ_ITAH( 27 );
			BZ_ITAH( 28 );
			BZ_ITAH( 29 );
			BZ_ITAH( 30 );
			BZ_ITAH( 31 );
			BZ_ITAH( 32 );
			BZ_ITAH( 33 );
			BZ_ITAH( 34 );
			BZ_ITAH( 35 );
			BZ_ITAH( 36 );
			BZ_ITAH( 37 );
			BZ_ITAH( 38 );
			BZ_ITAH( 39 );
			BZ_ITAH( 40 );
			BZ_ITAH( 41 );
			BZ_ITAH( 42 );
			BZ_ITAH( 43 );
			BZ_ITAH( 44 );
			BZ_ITAH( 45 );
			BZ_ITAH( 46 );
			BZ_ITAH( 47 );
			BZ_ITAH( 48 );
			BZ_ITAH( 49 );

#           undef BZ_ITAH

		}
		else
		{
			/*--- slow version which correctly handles all situations ---*/
			for ( i = gs; i <= ge; i++ )
			{
				bsW ( s,
				      s->len  [s->selector[selCtr]] [mtfv[i]],
				      s->code [s->selector[selCtr]] [mtfv[i]] );
			}
		}


		gs = ge + 1;
		selCtr++;
	}

	AssertH( selCtr == nSelectors, 3007 );

	if ( s->verbosity >= 3 )
	{
		VPrintf1( "codes %d\n", s->numZ - nBytes );
	}
}


/*---------------------------------------------------*/
void BZ2_compressBlock ( EState* s, Bool is_last_block )
{
	if ( s->nblock > 0 )
	{

		BZ_FINALISE_CRC ( s->blockCRC );
		s->combinedCRC = ( s->combinedCRC << 1 ) | ( s->combinedCRC >> 31 );
		s->combinedCRC ^= s->blockCRC;

		if ( s->blockNo > 1 ) { s->numZ = 0; }

		if ( s->verbosity >= 2 )
			VPrintf4( "    block %d: crc = 0x%08x, "
			          "combined CRC = 0x%08x, size = %d\n",
			          s->blockNo, s->blockCRC, s->combinedCRC, s->nblock );

		BZ2_blockSort ( s );
	}

	s->zbits = ( UChar* ) ( &( ( UChar* )s->arr2 )[s->nblock] );

	/*-- If this is the first block, create the stream header. --*/
	if ( s->blockNo == 1 )
	{
		BZ2_bsInitWrite ( s );
		bsPutUChar ( s, BZ_HDR_B );
		bsPutUChar ( s, BZ_HDR_Z );
		bsPutUChar ( s, BZ_HDR_h );
		bsPutUChar ( s, ( UChar )( BZ_HDR_0 + s->blockSize100k ) );
	}

	if ( s->nblock > 0 )
	{

		bsPutUChar ( s, 0x31 );
		bsPutUChar ( s, 0x41 );
		bsPutUChar ( s, 0x59 );
		bsPutUChar ( s, 0x26 );
		bsPutUChar ( s, 0x53 );
		bsPutUChar ( s, 0x59 );

		/*-- Now the block's CRC, so it is in a known place. --*/
		bsPutUInt32 ( s, s->blockCRC );

		/*--
		   Now a single bit indicating (non-)randomisation.
		   As of version 0.9.5, we use a better sorting algorithm
		   which makes randomisation unnecessary.  So always set
		   the randomised bit to 'no'.  Of course, the decoder
		   still needs to be able to handle randomised blocks
		   so as to maintain backwards compatibility with
		   older versions of bzip2.
		--*/
		bsW( s, 1, 0 );

		bsW ( s, 24, s->origPtr );
		generateMTFValues ( s );
		sendMTFValues ( s );
	}


	/*-- If this is the last block, add the stream trailer. --*/
	if ( is_last_block )
	{

		bsPutUChar ( s, 0x17 );
		bsPutUChar ( s, 0x72 );
		bsPutUChar ( s, 0x45 );
		bsPutUChar ( s, 0x38 );
		bsPutUChar ( s, 0x50 );
		bsPutUChar ( s, 0x90 );
		bsPutUInt32 ( s, s->combinedCRC );

		if ( s->verbosity >= 2 )
		{
			VPrintf1( "    final combined CRC = 0x%08x\n   ", s->combinedCRC );
		}

		bsFinishWrite ( s );
	}
}


/*-------------------------------------------------------------*/
/*--- end                                        compress.c ---*/
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--- Decompression machinery                               ---*/
/*---                                          decompress.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */


/*---------------------------------------------------*/
static
void makeMaps_d ( DState* s )
{
	Int32 i;
	s->nInUse = 0;

	for ( i = 0; i < 256; i++ )
		if ( s->inUse[i] )
		{
			s->seqToUnseq[s->nInUse] = i;
			s->nInUse++;
		}
}


/*---------------------------------------------------*/
#define RETURN(rrr)                               \
   { retVal = rrr; goto save_state_and_return; };

#define GET_BITS(lll,vvv,nnn)                     \
   case lll: s->state = lll;                      \
   while (True) {                                 \
      if (s->bsLive >= nnn) {                     \
         UInt32 v;                                \
         v = (s->bsBuff >>                        \
             (s->bsLive-nnn)) & ((1 << nnn)-1);   \
         s->bsLive -= nnn;                        \
         vvv = v;                                 \
         break;                                   \
      }                                           \
      if (s->strm->avail_in == 0) RETURN(BZ_OK);  \
      s->bsBuff                                   \
         = (s->bsBuff << 8) |                     \
           ((UInt32)                              \
              (*((UChar*)(s->strm->next_in))));   \
      s->bsLive += 8;                             \
      s->strm->next_in++;                         \
      s->strm->avail_in--;                        \
      s->strm->total_in_lo32++;                   \
      if (s->strm->total_in_lo32 == 0)            \
         s->strm->total_in_hi32++;                \
   }

#define GET_UCHAR(lll,uuu)                        \
   GET_BITS(lll,uuu,8)

#define GET_BIT(lll,uuu)                          \
   GET_BITS(lll,uuu,1)

/*---------------------------------------------------*/
#define GET_MTF_VAL(label1,label2,lval)           \
{                                                 \
   if (groupPos == 0) {                           \
      groupNo++;                                  \
      if (groupNo >= nSelectors)                  \
         RETURN(BZ_DATA_ERROR);                   \
      groupPos = BZ_G_SIZE;                       \
      gSel = s->selector[groupNo];                \
      gMinlen = s->minLens[gSel];                 \
      gLimit = &(s->limit[gSel][0]);              \
      gPerm = &(s->perm[gSel][0]);                \
      gBase = &(s->base[gSel][0]);                \
   }                                              \
   groupPos--;                                    \
   zn = gMinlen;                                  \
   GET_BITS(label1, zvec, zn);                    \
   while (1) {                                    \
      if (zn > 20 /* the longest code */)         \
         RETURN(BZ_DATA_ERROR);                   \
      if (zvec <= gLimit[zn]) break;              \
      zn++;                                       \
      GET_BIT(label2, zj);                        \
      zvec = (zvec << 1) | zj;                    \
   };                                             \
   if (zvec - gBase[zn] < 0                       \
       || zvec - gBase[zn] >= BZ_MAX_ALPHA_SIZE)  \
      RETURN(BZ_DATA_ERROR);                      \
   lval = gPerm[zvec - gBase[zn]];                \
}


/*---------------------------------------------------*/
Int32 BZ2_decompress ( DState* s )
{
	UChar      uc;
	Int32      retVal;
	Int32      minLen, maxLen;
	bz_stream* strm = s->strm;

	/* stuff that needs to be saved/restored */
	Int32  i;
	Int32  j;
	Int32  t;
	Int32  alphaSize;
	Int32  nGroups;
	Int32  nSelectors;
	Int32  EOB;
	Int32  groupNo;
	Int32  groupPos;
	Int32  nextSym;
	Int32  nblockMAX;
	Int32  nblock;
	Int32  es;
	Int32  N;
	Int32  curr;
	Int32  zt;
	Int32  zn;
	Int32  zvec;
	Int32  zj;
	Int32  gSel;
	Int32  gMinlen;
	Int32* gLimit;
	Int32* gBase;
	Int32* gPerm;

	if ( s->state == BZ_X_MAGIC_1 )
	{
		/*initialise the save area*/
		s->save_i           = 0;
		s->save_j           = 0;
		s->save_t           = 0;
		s->save_alphaSize   = 0;
		s->save_nGroups     = 0;
		s->save_nSelectors  = 0;
		s->save_EOB         = 0;
		s->save_groupNo     = 0;
		s->save_groupPos    = 0;
		s->save_nextSym     = 0;
		s->save_nblockMAX   = 0;
		s->save_nblock      = 0;
		s->save_es          = 0;
		s->save_N           = 0;
		s->save_curr        = 0;
		s->save_zt          = 0;
		s->save_zn          = 0;
		s->save_zvec        = 0;
		s->save_zj          = 0;
		s->save_gSel        = 0;
		s->save_gMinlen     = 0;
		s->save_gLimit      = NULL;
		s->save_gBase       = NULL;
		s->save_gPerm       = NULL;
	}

	/*restore from the save area*/
	i           = s->save_i;
	j           = s->save_j;
	t           = s->save_t;
	alphaSize   = s->save_alphaSize;
	nGroups     = s->save_nGroups;
	nSelectors  = s->save_nSelectors;
	EOB         = s->save_EOB;
	groupNo     = s->save_groupNo;
	groupPos    = s->save_groupPos;
	nextSym     = s->save_nextSym;
	nblockMAX   = s->save_nblockMAX;
	nblock      = s->save_nblock;
	es          = s->save_es;
	N           = s->save_N;
	curr        = s->save_curr;
	zt          = s->save_zt;
	zn          = s->save_zn;
	zvec        = s->save_zvec;
	zj          = s->save_zj;
	gSel        = s->save_gSel;
	gMinlen     = s->save_gMinlen;
	gLimit      = s->save_gLimit;
	gBase       = s->save_gBase;
	gPerm       = s->save_gPerm;

	retVal = BZ_OK;

	switch ( s->state )
	{

			GET_UCHAR( BZ_X_MAGIC_1, uc );

			if ( uc != BZ_HDR_B ) { RETURN( BZ_DATA_ERROR_MAGIC ); }

			GET_UCHAR( BZ_X_MAGIC_2, uc );

			if ( uc != BZ_HDR_Z ) { RETURN( BZ_DATA_ERROR_MAGIC ); }

			GET_UCHAR( BZ_X_MAGIC_3, uc )

			if ( uc != BZ_HDR_h ) { RETURN( BZ_DATA_ERROR_MAGIC ); }

			GET_BITS( BZ_X_MAGIC_4, s->blockSize100k, 8 )

			if ( s->blockSize100k < ( BZ_HDR_0 + 1 ) ||
			     s->blockSize100k > ( BZ_HDR_0 + 9 ) ) { RETURN( BZ_DATA_ERROR_MAGIC ); }

			s->blockSize100k -= BZ_HDR_0;

			if ( s->smallDecompress )
			{
				s->ll16 = BZALLOC( s->blockSize100k * 100000 * sizeof( UInt16 ) );
				s->ll4  = BZALLOC(
				             ( ( 1 + s->blockSize100k * 100000 ) >> 1 ) * sizeof( UChar )
				          );

				if ( s->ll16 == NULL || s->ll4 == NULL ) { RETURN( BZ_MEM_ERROR ); }
			}
			else
			{
				s->tt  = BZALLOC( s->blockSize100k * 100000 * sizeof( Int32 ) );

				if ( s->tt == NULL ) { RETURN( BZ_MEM_ERROR ); }
			}

			GET_UCHAR( BZ_X_BLKHDR_1, uc );

			if ( uc == 0x17 ) { goto endhdr_2; }

			if ( uc != 0x31 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_BLKHDR_2, uc );

			if ( uc != 0x41 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_BLKHDR_3, uc );

			if ( uc != 0x59 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_BLKHDR_4, uc );

			if ( uc != 0x26 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_BLKHDR_5, uc );

			if ( uc != 0x53 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_BLKHDR_6, uc );

			if ( uc != 0x59 ) { RETURN( BZ_DATA_ERROR ); }

			s->currBlockNo++;

			if ( s->verbosity >= 2 )
			{
				VPrintf1 ( "\n    [%d: huff+mtf ", s->currBlockNo );
			}

			s->storedBlockCRC = 0;
			GET_UCHAR( BZ_X_BCRC_1, uc );
			s->storedBlockCRC = ( s->storedBlockCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_BCRC_2, uc );
			s->storedBlockCRC = ( s->storedBlockCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_BCRC_3, uc );
			s->storedBlockCRC = ( s->storedBlockCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_BCRC_4, uc );
			s->storedBlockCRC = ( s->storedBlockCRC << 8 ) | ( ( UInt32 )uc );

			GET_BITS( BZ_X_RANDBIT, s->blockRandomised, 1 );

			s->origPtr = 0;
			GET_UCHAR( BZ_X_ORIGPTR_1, uc );
			s->origPtr = ( s->origPtr << 8 ) | ( ( Int32 )uc );
			GET_UCHAR( BZ_X_ORIGPTR_2, uc );
			s->origPtr = ( s->origPtr << 8 ) | ( ( Int32 )uc );
			GET_UCHAR( BZ_X_ORIGPTR_3, uc );
			s->origPtr = ( s->origPtr << 8 ) | ( ( Int32 )uc );

			if ( s->origPtr < 0 )
			{
				RETURN( BZ_DATA_ERROR );
			}

			if ( s->origPtr > 10 + 100000 * s->blockSize100k )
			{
				RETURN( BZ_DATA_ERROR );
			}

			/*--- Receive the mapping table ---*/
			for ( i = 0; i < 16; i++ )
			{
				GET_BIT( BZ_X_MAPPING_1, uc );

				if ( uc == 1 )
				{
					s->inUse16[i] = True;
				}
				else
				{
					s->inUse16[i] = False;
				}
			}

			for ( i = 0; i < 256; i++ ) { s->inUse[i] = False; }

			for ( i = 0; i < 16; i++ )
				if ( s->inUse16[i] )
					for ( j = 0; j < 16; j++ )
					{
						GET_BIT( BZ_X_MAPPING_2, uc );

						if ( uc == 1 ) { s->inUse[i * 16 + j] = True; }
					}

			makeMaps_d ( s );

			if ( s->nInUse == 0 ) { RETURN( BZ_DATA_ERROR ); }

			alphaSize = s->nInUse + 2;

			/*--- Now the selectors ---*/
			GET_BITS( BZ_X_SELECTOR_1, nGroups, 3 );

			if ( nGroups < 2 || nGroups > 6 ) { RETURN( BZ_DATA_ERROR ); }

			GET_BITS( BZ_X_SELECTOR_2, nSelectors, 15 );

			if ( nSelectors < 1 ) { RETURN( BZ_DATA_ERROR ); }

			for ( i = 0; i < nSelectors; i++ )
			{
				j = 0;

				while ( True )
				{
					GET_BIT( BZ_X_SELECTOR_3, uc );

					if ( uc == 0 ) { break; }

					j++;

					if ( j >= nGroups ) { RETURN( BZ_DATA_ERROR ); }
				}

				s->selectorMtf[i] = j;
			}

			/*--- Undo the MTF values for the selectors. ---*/
			{
				UChar pos[BZ_N_GROUPS], tmp, v;

				for ( v = 0; v < nGroups; v++ ) { pos[v] = v; }

				for ( i = 0; i < nSelectors; i++ )
				{
					v = s->selectorMtf[i];
					tmp = pos[v];

					while ( v > 0 ) { pos[v] = pos[v - 1]; v--; }

					pos[0] = tmp;
					s->selector[i] = tmp;
				}
			}

			/*--- Now the coding tables ---*/
			for ( t = 0; t < nGroups; t++ )
			{
				GET_BITS( BZ_X_CODING_1, curr, 5 );

				for ( i = 0; i < alphaSize; i++ )
				{
					while ( True )
					{
						if ( curr < 1 || curr > 20 ) { RETURN( BZ_DATA_ERROR ); }

						GET_BIT( BZ_X_CODING_2, uc );

						if ( uc == 0 ) { break; }

						GET_BIT( BZ_X_CODING_3, uc );

						if ( uc == 0 ) { curr++; }
						else { curr--; }
					}

					s->len[t][i] = curr;
				}
			}

			/*--- Create the Huffman decoding tables ---*/
			for ( t = 0; t < nGroups; t++ )
			{
				minLen = 32;
				maxLen = 0;

				for ( i = 0; i < alphaSize; i++ )
				{
					if ( s->len[t][i] > maxLen ) { maxLen = s->len[t][i]; }

					if ( s->len[t][i] < minLen ) { minLen = s->len[t][i]; }
				}

				BZ2_hbCreateDecodeTables (
				   &( s->limit[t][0] ),
				   &( s->base[t][0] ),
				   &( s->perm[t][0] ),
				   &( s->len[t][0] ),
				   minLen, maxLen, alphaSize
				);
				s->minLens[t] = minLen;
			}

			/*--- Now the MTF values ---*/

			EOB      = s->nInUse + 1;
			nblockMAX = 100000 * s->blockSize100k;
			groupNo  = -1;
			groupPos = 0;

			for ( i = 0; i <= 255; i++ ) { s->unzftab[i] = 0; }

			/*-- MTF init --*/
			{
				Int32 ii, jj, kk;
				kk = MTFA_SIZE - 1;

				for ( ii = 256 / MTFL_SIZE - 1; ii >= 0; ii-- )
				{
					for ( jj = MTFL_SIZE - 1; jj >= 0; jj-- )
					{
						s->mtfa[kk] = ( UChar )( ii * MTFL_SIZE + jj );
						kk--;
					}

					s->mtfbase[ii] = kk + 1;
				}
			}
			/*-- end MTF init --*/

			nblock = 0;
			GET_MTF_VAL( BZ_X_MTF_1, BZ_X_MTF_2, nextSym );

			while ( True )
			{

				if ( nextSym == EOB ) { break; }

				if ( nextSym == BZ_RUNA || nextSym == BZ_RUNB )
				{

					es = -1;
					N = 1;

					do
					{
						/* Check that N doesn't get too big, so that es doesn't
						   go negative.  The maximum value that can be
						   RUNA/RUNB encoded is equal to the block size (post
						   the initial RLE), viz, 900k, so bounding N at 2
						   million should guard against overflow without
						   rejecting any legitimate inputs. */
						if ( N >= 2 * 1024 * 1024 ) { RETURN( BZ_DATA_ERROR ); }

						if ( nextSym == BZ_RUNA ) { es = es + ( 0 + 1 ) * N; }
						else if ( nextSym == BZ_RUNB ) { es = es + ( 1 + 1 ) * N; }

						N = N * 2;
						GET_MTF_VAL( BZ_X_MTF_3, BZ_X_MTF_4, nextSym );
					}
					while ( nextSym == BZ_RUNA || nextSym == BZ_RUNB );

					es++;
					uc = s->seqToUnseq[ s->mtfa[s->mtfbase[0]] ];
					s->unzftab[uc] += es;

					if ( s->smallDecompress )
						while ( es > 0 )
						{
							if ( nblock >= nblockMAX ) { RETURN( BZ_DATA_ERROR ); }

							s->ll16[nblock] = ( UInt16 )uc;
							nblock++;
							es--;
						}
					else
						while ( es > 0 )
						{
							if ( nblock >= nblockMAX ) { RETURN( BZ_DATA_ERROR ); }

							s->tt[nblock] = ( UInt32 )uc;
							nblock++;
							es--;
						};

					continue;

				}
				else
				{

					if ( nblock >= nblockMAX ) { RETURN( BZ_DATA_ERROR ); }

					/*-- uc = MTF ( nextSym-1 ) --*/
					{
						Int32 ii, jj, kk, pp, lno, off;
						UInt32 nn;
						nn = ( UInt32 )( nextSym - 1 );

						if ( nn < MTFL_SIZE )
						{
							/* avoid general-case expense */
							pp = s->mtfbase[0];
							uc = s->mtfa[pp + nn];

							while ( nn > 3 )
							{
								Int32 z = pp + nn;
								s->mtfa[( z )  ] = s->mtfa[( z ) - 1];
								s->mtfa[( z ) - 1] = s->mtfa[( z ) - 2];
								s->mtfa[( z ) - 2] = s->mtfa[( z ) - 3];
								s->mtfa[( z ) - 3] = s->mtfa[( z ) - 4];
								nn -= 4;
							}

							while ( nn > 0 )
							{
								s->mtfa[( pp + nn )] = s->mtfa[( pp + nn ) - 1];
								nn--;
							};

							s->mtfa[pp] = uc;
						}
						else
						{
							/* general case */
							lno = nn / MTFL_SIZE;
							off = nn % MTFL_SIZE;
							pp = s->mtfbase[lno] + off;
							uc = s->mtfa[pp];

							while ( pp > s->mtfbase[lno] )
							{
								s->mtfa[pp] = s->mtfa[pp - 1];
								pp--;
							};

							s->mtfbase[lno]++;

							while ( lno > 0 )
							{
								s->mtfbase[lno]--;
								s->mtfa[s->mtfbase[lno]]
								   = s->mtfa[s->mtfbase[lno - 1] + MTFL_SIZE - 1];
								lno--;
							}

							s->mtfbase[0]--;
							s->mtfa[s->mtfbase[0]] = uc;

							if ( s->mtfbase[0] == 0 )
							{
								kk = MTFA_SIZE - 1;

								for ( ii = 256 / MTFL_SIZE - 1; ii >= 0; ii-- )
								{
									for ( jj = MTFL_SIZE - 1; jj >= 0; jj-- )
									{
										s->mtfa[kk] = s->mtfa[s->mtfbase[ii] + jj];
										kk--;
									}

									s->mtfbase[ii] = kk + 1;
								}
							}
						}
					}
					/*-- end uc = MTF ( nextSym-1 ) --*/

					s->unzftab[s->seqToUnseq[uc]]++;

					if ( s->smallDecompress )
					{
						s->ll16[nblock] = ( UInt16 )( s->seqToUnseq[uc] );
					}
					else
					{
						s->tt[nblock]   = ( UInt32 )( s->seqToUnseq[uc] );
					}

					nblock++;

					GET_MTF_VAL( BZ_X_MTF_5, BZ_X_MTF_6, nextSym );
					continue;
				}
			}

			/* Now we know what nblock is, we can do a better sanity
			   check on s->origPtr.
			*/
			if ( s->origPtr < 0 || s->origPtr >= nblock )
			{
				RETURN( BZ_DATA_ERROR );
			}

			/*-- Set up cftab to facilitate generation of T^(-1) --*/
			/* Check: unzftab entries in range. */
			for ( i = 0; i <= 255; i++ )
			{
				if ( s->unzftab[i] < 0 || s->unzftab[i] > nblock )
				{
					RETURN( BZ_DATA_ERROR );
				}
			}

			/* Actually generate cftab. */
			s->cftab[0] = 0;

			for ( i = 1; i <= 256; i++ ) { s->cftab[i] = s->unzftab[i - 1]; }

			for ( i = 1; i <= 256; i++ ) { s->cftab[i] += s->cftab[i - 1]; }

			/* Check: cftab entries in range. */
			for ( i = 0; i <= 256; i++ )
			{
				if ( s->cftab[i] < 0 || s->cftab[i] > nblock )
				{
					/* s->cftab[i] can legitimately be == nblock */
					RETURN( BZ_DATA_ERROR );
				}
			}

			/* Check: cftab entries non-descending. */
			for ( i = 1; i <= 256; i++ )
			{
				if ( s->cftab[i - 1] > s->cftab[i] )
				{
					RETURN( BZ_DATA_ERROR );
				}
			}

			s->state_out_len = 0;
			s->state_out_ch  = 0;
			BZ_INITIALISE_CRC ( s->calculatedBlockCRC );
			s->state = BZ_X_OUTPUT;

			if ( s->verbosity >= 2 ) { VPrintf0 ( "rt+rld" ); }

			if ( s->smallDecompress )
			{

				/*-- Make a copy of cftab, used in generation of T --*/
				for ( i = 0; i <= 256; i++ ) { s->cftabCopy[i] = s->cftab[i]; }

				/*-- compute the T vector --*/
				for ( i = 0; i < nblock; i++ )
				{
					uc = ( UChar )( s->ll16[i] );
					SET_LL( i, s->cftabCopy[uc] );
					s->cftabCopy[uc]++;
				}

				/*-- Compute T^(-1) by pointer reversal on T --*/
				i = s->origPtr;
				j = GET_LL( i );

				do
				{
					Int32 tmp = GET_LL( j );
					SET_LL( j, i );
					i = j;
					j = tmp;
				}
				while ( i != s->origPtr );

				s->tPos = s->origPtr;
				s->nblock_used = 0;

				if ( s->blockRandomised )
				{
					BZ_RAND_INIT_MASK;
					BZ_GET_SMALL( s->k0 );
					s->nblock_used++;
					BZ_RAND_UPD_MASK;
					s->k0 ^= BZ_RAND_MASK;
				}
				else
				{
					BZ_GET_SMALL( s->k0 );
					s->nblock_used++;
				}

			}
			else
			{

				/*-- compute the T^(-1) vector --*/
				for ( i = 0; i < nblock; i++ )
				{
					uc = ( UChar )( s->tt[i] & 0xff );
					s->tt[s->cftab[uc]] |= ( i << 8 );
					s->cftab[uc]++;
				}

				s->tPos = s->tt[s->origPtr] >> 8;
				s->nblock_used = 0;

				if ( s->blockRandomised )
				{
					BZ_RAND_INIT_MASK;
					BZ_GET_FAST( s->k0 );
					s->nblock_used++;
					BZ_RAND_UPD_MASK;
					s->k0 ^= BZ_RAND_MASK;
				}
				else
				{
					BZ_GET_FAST( s->k0 );
					s->nblock_used++;
				}

			}

			RETURN( BZ_OK );



endhdr_2:

			GET_UCHAR( BZ_X_ENDHDR_2, uc );

			if ( uc != 0x72 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_ENDHDR_3, uc );

			if ( uc != 0x45 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_ENDHDR_4, uc );

			if ( uc != 0x38 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_ENDHDR_5, uc );

			if ( uc != 0x50 ) { RETURN( BZ_DATA_ERROR ); }

			GET_UCHAR( BZ_X_ENDHDR_6, uc );

			if ( uc != 0x90 ) { RETURN( BZ_DATA_ERROR ); }

			s->storedCombinedCRC = 0;
			GET_UCHAR( BZ_X_CCRC_1, uc );
			s->storedCombinedCRC = ( s->storedCombinedCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_CCRC_2, uc );
			s->storedCombinedCRC = ( s->storedCombinedCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_CCRC_3, uc );
			s->storedCombinedCRC = ( s->storedCombinedCRC << 8 ) | ( ( UInt32 )uc );
			GET_UCHAR( BZ_X_CCRC_4, uc );
			s->storedCombinedCRC = ( s->storedCombinedCRC << 8 ) | ( ( UInt32 )uc );

			s->state = BZ_X_IDLE;
			RETURN( BZ_STREAM_END );

		default:
			AssertH ( False, 4001 );
	}

	AssertH ( False, 4002 );

save_state_and_return:

	s->save_i           = i;
	s->save_j           = j;
	s->save_t           = t;
	s->save_alphaSize   = alphaSize;
	s->save_nGroups     = nGroups;
	s->save_nSelectors  = nSelectors;
	s->save_EOB         = EOB;
	s->save_groupNo     = groupNo;
	s->save_groupPos    = groupPos;
	s->save_nextSym     = nextSym;
	s->save_nblockMAX   = nblockMAX;
	s->save_nblock      = nblock;
	s->save_es          = es;
	s->save_N           = N;
	s->save_curr        = curr;
	s->save_zt          = zt;
	s->save_zn          = zn;
	s->save_zvec        = zvec;
	s->save_zj          = zj;
	s->save_gSel        = gSel;
	s->save_gMinlen     = gMinlen;
	s->save_gLimit      = gLimit;
	s->save_gBase       = gBase;
	s->save_gPerm       = gPerm;

	return retVal;
}


/*-------------------------------------------------------------*/
/*--- end                                      decompress.c ---*/
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--- Huffman coding low-level stuff                        ---*/
/*---                                             huffman.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */

/*---------------------------------------------------*/
#define WEIGHTOF(zz0)  ((zz0) & 0xffffff00)
#define DEPTHOF(zz1)   ((zz1) & 0x000000ff)
#define MYMAX(zz2,zz3) ((zz2) > (zz3) ? (zz2) : (zz3))

#define ADDWEIGHTS(zw1,zw2)                           \
   (WEIGHTOF(zw1)+WEIGHTOF(zw2)) |                    \
   (1 + MYMAX(DEPTHOF(zw1),DEPTHOF(zw2)))

#define UPHEAP(z)                                     \
{                                                     \
   Int32 zz, tmp;                                     \
   zz = z; tmp = heap[zz];                            \
   while (weight[tmp] < weight[heap[zz >> 1]]) {      \
      heap[zz] = heap[zz >> 1];                       \
      zz >>= 1;                                       \
   }                                                  \
   heap[zz] = tmp;                                    \
}

#define DOWNHEAP(z)                                   \
{                                                     \
   Int32 zz, yy, tmp;                                 \
   zz = z; tmp = heap[zz];                            \
   while (True) {                                     \
      yy = zz << 1;                                   \
      if (yy > nHeap) break;                          \
      if (yy < nHeap &&                               \
          weight[heap[yy+1]] < weight[heap[yy]])      \
         yy++;                                        \
      if (weight[tmp] < weight[heap[yy]]) break;      \
      heap[zz] = heap[yy];                            \
      zz = yy;                                        \
   }                                                  \
   heap[zz] = tmp;                                    \
}


/*---------------------------------------------------*/
void BZ2_hbMakeCodeLengths ( UChar* len,
                             Int32* freq,
                             Int32 alphaSize,
                             Int32 maxLen )
{
	/*--
	   Nodes and heap entries run from 1.  Entry 0
	   for both the heap and nodes is a sentinel.
	--*/
	Int32 nNodes, nHeap, n1, n2, i, j, k;
	Bool  tooLong;

	Int32 heap   [ BZ_MAX_ALPHA_SIZE + 2 ];
	Int32 weight [ BZ_MAX_ALPHA_SIZE * 2 ];
	Int32 parent [ BZ_MAX_ALPHA_SIZE * 2 ];

	for ( i = 0; i < alphaSize; i++ )
	{
		weight[i + 1] = ( freq[i] == 0 ? 1 : freq[i] ) << 8;
	}

	while ( True )
	{

		nNodes = alphaSize;
		nHeap = 0;

		heap[0] = 0;
		weight[0] = 0;
		parent[0] = -2;

		for ( i = 1; i <= alphaSize; i++ )
		{
			parent[i] = -1;
			nHeap++;
			heap[nHeap] = i;
			UPHEAP( nHeap );
		}

		AssertH( nHeap < ( BZ_MAX_ALPHA_SIZE + 2 ), 2001 );

		while ( nHeap > 1 )
		{
			n1 = heap[1];
			heap[1] = heap[nHeap];
			nHeap--;
			DOWNHEAP( 1 );
			n2 = heap[1];
			heap[1] = heap[nHeap];
			nHeap--;
			DOWNHEAP( 1 );
			nNodes++;
			parent[n1] = parent[n2] = nNodes;
			weight[nNodes] = ADDWEIGHTS( weight[n1], weight[n2] );
			parent[nNodes] = -1;
			nHeap++;
			heap[nHeap] = nNodes;
			UPHEAP( nHeap );
		}

		AssertH( nNodes < ( BZ_MAX_ALPHA_SIZE * 2 ), 2002 );

		tooLong = False;

		for ( i = 1; i <= alphaSize; i++ )
		{
			j = 0;
			k = i;

			while ( parent[k] >= 0 ) { k = parent[k]; j++; }

			len[i - 1] = j;

			if ( j > maxLen ) { tooLong = True; }
		}

		if ( ! tooLong ) { break; }

		/* 17 Oct 04: keep-going condition for the following loop used
		   to be 'i < alphaSize', which missed the last element,
		   theoretically leading to the possibility of the compressor
		   looping.  However, this count-scaling step is only needed if
		   one of the generated Huffman code words is longer than
		   maxLen, which up to and including version 1.0.2 was 20 bits,
		   which is extremely unlikely.  In version 1.0.3 maxLen was
		   changed to 17 bits, which has minimal effect on compression
		   ratio, but does mean this scaling step is used from time to
		   time, enough to verify that it works.

		   This means that bzip2-1.0.3 and later will only produce
		   Huffman codes with a maximum length of 17 bits.  However, in
		   order to preserve backwards compatibility with bitstreams
		   produced by versions pre-1.0.3, the decompressor must still
		   handle lengths of up to 20. */

		for ( i = 1; i <= alphaSize; i++ )
		{
			j = weight[i] >> 8;
			j = 1 + ( j / 2 );
			weight[i] = j << 8;
		}
	}
}


/*---------------------------------------------------*/
void BZ2_hbAssignCodes ( Int32* code,
                         UChar* length,
                         Int32 minLen,
                         Int32 maxLen,
                         Int32 alphaSize )
{
	Int32 n, vec, i;

	vec = 0;

	for ( n = minLen; n <= maxLen; n++ )
	{
		for ( i = 0; i < alphaSize; i++ )
			if ( length[i] == n ) { code[i] = vec; vec++; };

		vec <<= 1;
	}
}


/*---------------------------------------------------*/
void BZ2_hbCreateDecodeTables ( Int32* limit,
                                Int32* base,
                                Int32* perm,
                                UChar* length,
                                Int32 minLen,
                                Int32 maxLen,
                                Int32 alphaSize )
{
	Int32 pp, i, j, vec;

	pp = 0;

	for ( i = minLen; i <= maxLen; i++ )
		for ( j = 0; j < alphaSize; j++ )
			if ( length[j] == i ) { perm[pp] = j; pp++; };

	for ( i = 0; i < BZ_MAX_CODE_LEN; i++ ) { base[i] = 0; }

	for ( i = 0; i < alphaSize; i++ ) { base[length[i] + 1]++; }

	for ( i = 1; i < BZ_MAX_CODE_LEN; i++ ) { base[i] += base[i - 1]; }

	for ( i = 0; i < BZ_MAX_CODE_LEN; i++ ) { limit[i] = 0; }

	vec = 0;

	for ( i = minLen; i <= maxLen; i++ )
	{
		vec += ( base[i + 1] - base[i] );
		limit[i] = vec - 1;
		vec <<= 1;
	}

	for ( i = minLen + 1; i <= maxLen; i++ )
	{
		base[i] = ( ( limit[i - 1] + 1 ) << 1 ) - base[i];
	}
}


/*-------------------------------------------------------------*/
/*--- end                                         huffman.c ---*/
/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/*--- Table for doing CRCs                                  ---*/
/*---                                            crctable.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */


/*--
  I think this is an implementation of the AUTODIN-II,
  Ethernet & FDDI 32-bit CRC standard.  Vaguely derived
  from code by Rob Warnock, in Section 51 of the
  comp.compression FAQ.
--*/

UInt32 BZ2_crc32Table[256] =
{

	/*-- Ugly, innit? --*/

	0x00000000L, 0x04c11db7L, 0x09823b6eL, 0x0d4326d9L,
	0x130476dcL, 0x17c56b6bL, 0x1a864db2L, 0x1e475005L,
	0x2608edb8L, 0x22c9f00fL, 0x2f8ad6d6L, 0x2b4bcb61L,
	0x350c9b64L, 0x31cd86d3L, 0x3c8ea00aL, 0x384fbdbdL,
	0x4c11db70L, 0x48d0c6c7L, 0x4593e01eL, 0x4152fda9L,
	0x5f15adacL, 0x5bd4b01bL, 0x569796c2L, 0x52568b75L,
	0x6a1936c8L, 0x6ed82b7fL, 0x639b0da6L, 0x675a1011L,
	0x791d4014L, 0x7ddc5da3L, 0x709f7b7aL, 0x745e66cdL,
	0x9823b6e0L, 0x9ce2ab57L, 0x91a18d8eL, 0x95609039L,
	0x8b27c03cL, 0x8fe6dd8bL, 0x82a5fb52L, 0x8664e6e5L,
	0xbe2b5b58L, 0xbaea46efL, 0xb7a96036L, 0xb3687d81L,
	0xad2f2d84L, 0xa9ee3033L, 0xa4ad16eaL, 0xa06c0b5dL,
	0xd4326d90L, 0xd0f37027L, 0xddb056feL, 0xd9714b49L,
	0xc7361b4cL, 0xc3f706fbL, 0xceb42022L, 0xca753d95L,
	0xf23a8028L, 0xf6fb9d9fL, 0xfbb8bb46L, 0xff79a6f1L,
	0xe13ef6f4L, 0xe5ffeb43L, 0xe8bccd9aL, 0xec7dd02dL,
	0x34867077L, 0x30476dc0L, 0x3d044b19L, 0x39c556aeL,
	0x278206abL, 0x23431b1cL, 0x2e003dc5L, 0x2ac12072L,
	0x128e9dcfL, 0x164f8078L, 0x1b0ca6a1L, 0x1fcdbb16L,
	0x018aeb13L, 0x054bf6a4L, 0x0808d07dL, 0x0cc9cdcaL,
	0x7897ab07L, 0x7c56b6b0L, 0x71159069L, 0x75d48ddeL,
	0x6b93dddbL, 0x6f52c06cL, 0x6211e6b5L, 0x66d0fb02L,
	0x5e9f46bfL, 0x5a5e5b08L, 0x571d7dd1L, 0x53dc6066L,
	0x4d9b3063L, 0x495a2dd4L, 0x44190b0dL, 0x40d816baL,
	0xaca5c697L, 0xa864db20L, 0xa527fdf9L, 0xa1e6e04eL,
	0xbfa1b04bL, 0xbb60adfcL, 0xb6238b25L, 0xb2e29692L,
	0x8aad2b2fL, 0x8e6c3698L, 0x832f1041L, 0x87ee0df6L,
	0x99a95df3L, 0x9d684044L, 0x902b669dL, 0x94ea7b2aL,
	0xe0b41de7L, 0xe4750050L, 0xe9362689L, 0xedf73b3eL,
	0xf3b06b3bL, 0xf771768cL, 0xfa325055L, 0xfef34de2L,
	0xc6bcf05fL, 0xc27dede8L, 0xcf3ecb31L, 0xcbffd686L,
	0xd5b88683L, 0xd1799b34L, 0xdc3abdedL, 0xd8fba05aL,
	0x690ce0eeL, 0x6dcdfd59L, 0x608edb80L, 0x644fc637L,
	0x7a089632L, 0x7ec98b85L, 0x738aad5cL, 0x774bb0ebL,
	0x4f040d56L, 0x4bc510e1L, 0x46863638L, 0x42472b8fL,
	0x5c007b8aL, 0x58c1663dL, 0x558240e4L, 0x51435d53L,
	0x251d3b9eL, 0x21dc2629L, 0x2c9f00f0L, 0x285e1d47L,
	0x36194d42L, 0x32d850f5L, 0x3f9b762cL, 0x3b5a6b9bL,
	0x0315d626L, 0x07d4cb91L, 0x0a97ed48L, 0x0e56f0ffL,
	0x1011a0faL, 0x14d0bd4dL, 0x19939b94L, 0x1d528623L,
	0xf12f560eL, 0xf5ee4bb9L, 0xf8ad6d60L, 0xfc6c70d7L,
	0xe22b20d2L, 0xe6ea3d65L, 0xeba91bbcL, 0xef68060bL,
	0xd727bbb6L, 0xd3e6a601L, 0xdea580d8L, 0xda649d6fL,
	0xc423cd6aL, 0xc0e2d0ddL, 0xcda1f604L, 0xc960ebb3L,
	0xbd3e8d7eL, 0xb9ff90c9L, 0xb4bcb610L, 0xb07daba7L,
	0xae3afba2L, 0xaafbe615L, 0xa7b8c0ccL, 0xa379dd7bL,
	0x9b3660c6L, 0x9ff77d71L, 0x92b45ba8L, 0x9675461fL,
	0x8832161aL, 0x8cf30badL, 0x81b02d74L, 0x857130c3L,
	0x5d8a9099L, 0x594b8d2eL, 0x5408abf7L, 0x50c9b640L,
	0x4e8ee645L, 0x4a4ffbf2L, 0x470cdd2bL, 0x43cdc09cL,
	0x7b827d21L, 0x7f436096L, 0x7200464fL, 0x76c15bf8L,
	0x68860bfdL, 0x6c47164aL, 0x61043093L, 0x65c52d24L,
	0x119b4be9L, 0x155a565eL, 0x18197087L, 0x1cd86d30L,
	0x029f3d35L, 0x065e2082L, 0x0b1d065bL, 0x0fdc1becL,
	0x3793a651L, 0x3352bbe6L, 0x3e119d3fL, 0x3ad08088L,
	0x2497d08dL, 0x2056cd3aL, 0x2d15ebe3L, 0x29d4f654L,
	0xc5a92679L, 0xc1683bceL, 0xcc2b1d17L, 0xc8ea00a0L,
	0xd6ad50a5L, 0xd26c4d12L, 0xdf2f6bcbL, 0xdbee767cL,
	0xe3a1cbc1L, 0xe760d676L, 0xea23f0afL, 0xeee2ed18L,
	0xf0a5bd1dL, 0xf464a0aaL, 0xf9278673L, 0xfde69bc4L,
	0x89b8fd09L, 0x8d79e0beL, 0x803ac667L, 0x84fbdbd0L,
	0x9abc8bd5L, 0x9e7d9662L, 0x933eb0bbL, 0x97ffad0cL,
	0xafb010b1L, 0xab710d06L, 0xa6322bdfL, 0xa2f33668L,
	0xbcb4666dL, 0xb8757bdaL, 0xb5365d03L, 0xb1f740b4L
};


/*-------------------------------------------------------------*/
/*--- end                                        crctable.c ---*/
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--- Table for randomising repetitive blocks               ---*/
/*---                                           randtable.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */

/*---------------------------------------------*/
Int32 BZ2_rNums[512] =
{
	619, 720, 127, 481, 931, 816, 813, 233, 566, 247,
	985, 724, 205, 454, 863, 491, 741, 242, 949, 214,
	733, 859, 335, 708, 621, 574, 73, 654, 730, 472,
	419, 436, 278, 496, 867, 210, 399, 680, 480, 51,
	878, 465, 811, 169, 869, 675, 611, 697, 867, 561,
	862, 687, 507, 283, 482, 129, 807, 591, 733, 623,
	150, 238, 59, 379, 684, 877, 625, 169, 643, 105,
	170, 607, 520, 932, 727, 476, 693, 425, 174, 647,
	73, 122, 335, 530, 442, 853, 695, 249, 445, 515,
	909, 545, 703, 919, 874, 474, 882, 500, 594, 612,
	641, 801, 220, 162, 819, 984, 589, 513, 495, 799,
	161, 604, 958, 533, 221, 400, 386, 867, 600, 782,
	382, 596, 414, 171, 516, 375, 682, 485, 911, 276,
	98, 553, 163, 354, 666, 933, 424, 341, 533, 870,
	227, 730, 475, 186, 263, 647, 537, 686, 600, 224,
	469, 68, 770, 919, 190, 373, 294, 822, 808, 206,
	184, 943, 795, 384, 383, 461, 404, 758, 839, 887,
	715, 67, 618, 276, 204, 918, 873, 777, 604, 560,
	951, 160, 578, 722, 79, 804, 96, 409, 713, 940,
	652, 934, 970, 447, 318, 353, 859, 672, 112, 785,
	645, 863, 803, 350, 139, 93, 354, 99, 820, 908,
	609, 772, 154, 274, 580, 184, 79, 626, 630, 742,
	653, 282, 762, 623, 680, 81, 927, 626, 789, 125,
	411, 521, 938, 300, 821, 78, 343, 175, 128, 250,
	170, 774, 972, 275, 999, 639, 495, 78, 352, 126,
	857, 956, 358, 619, 580, 124, 737, 594, 701, 612,
	669, 112, 134, 694, 363, 992, 809, 743, 168, 974,
	944, 375, 748, 52, 600, 747, 642, 182, 862, 81,
	344, 805, 988, 739, 511, 655, 814, 334, 249, 515,
	897, 955, 664, 981, 649, 113, 974, 459, 893, 228,
	433, 837, 553, 268, 926, 240, 102, 654, 459, 51,
	686, 754, 806, 760, 493, 403, 415, 394, 687, 700,
	946, 670, 656, 610, 738, 392, 760, 799, 887, 653,
	978, 321, 576, 617, 626, 502, 894, 679, 243, 440,
	680, 879, 194, 572, 640, 724, 926, 56, 204, 700,
	707, 151, 457, 449, 797, 195, 791, 558, 945, 679,
	297, 59, 87, 824, 713, 663, 412, 693, 342, 606,
	134, 108, 571, 364, 631, 212, 174, 643, 304, 329,
	343, 97, 430, 751, 497, 314, 983, 374, 822, 928,
	140, 206, 73, 263, 980, 736, 876, 478, 430, 305,
	170, 514, 364, 692, 829, 82, 855, 953, 676, 246,
	369, 970, 294, 750, 807, 827, 150, 790, 288, 923,
	804, 378, 215, 828, 592, 281, 565, 555, 710, 82,
	896, 831, 547, 261, 524, 462, 293, 465, 502, 56,
	661, 821, 976, 991, 658, 869, 905, 758, 745, 193,
	768, 550, 608, 933, 378, 286, 215, 979, 792, 961,
	61, 688, 793, 644, 986, 403, 106, 366, 905, 644,
	372, 567, 466, 434, 645, 210, 389, 550, 919, 135,
	780, 773, 635, 389, 707, 100, 626, 958, 165, 504,
	920, 176, 193, 713, 857, 265, 203, 50, 668, 108,
	645, 990, 626, 197, 510, 357, 358, 850, 858, 364,
	936, 638
};


/*-------------------------------------------------------------*/
/*--- end                                       randtable.c ---*/
/*-------------------------------------------------------------*/


/*-------------------------------------------------------------*/
/*--- Block sorting machinery                               ---*/
/*---                                           blocksort.c ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the
   README file.

   This program is released under the terms of the license contained
   in the file LICENSE.
   ------------------------------------------------------------------ */


/*---------------------------------------------*/
/*--- Fallback O(N log(N)^2) sorting        ---*/
/*--- algorithm, for repetitive blocks      ---*/
/*---------------------------------------------*/

/*---------------------------------------------*/
static
__inline__
void fallbackSimpleSort ( UInt32* fmap,
                          UInt32* eclass,
                          Int32   lo,
                          Int32   hi )
{
	Int32 i, j, tmp;
	UInt32 ec_tmp;

	if ( lo == hi ) { return; }

	if ( hi - lo > 3 )
	{
		for ( i = hi - 4; i >= lo; i-- )
		{
			tmp = fmap[i];
			ec_tmp = eclass[tmp];

			for ( j = i + 4; j <= hi && ec_tmp > eclass[fmap[j]]; j += 4 )
			{
				fmap[j - 4] = fmap[j];
			}

			fmap[j - 4] = tmp;
		}
	}

	for ( i = hi - 1; i >= lo; i-- )
	{
		tmp = fmap[i];
		ec_tmp = eclass[tmp];

		for ( j = i + 1; j <= hi && ec_tmp > eclass[fmap[j]]; j++ )
		{
			fmap[j - 1] = fmap[j];
		}

		fmap[j - 1] = tmp;
	}
}


/*---------------------------------------------*/
#define fswap(zz1, zz2) \
   { Int32 zztmp = zz1; zz1 = zz2; zz2 = zztmp; }

#define fvswap(zzp1, zzp2, zzn)       \
{                                     \
   Int32 yyp1 = (zzp1);               \
   Int32 yyp2 = (zzp2);               \
   Int32 yyn  = (zzn);                \
   while (yyn > 0) {                  \
      fswap(fmap[yyp1], fmap[yyp2]);  \
      yyp1++; yyp2++; yyn--;          \
   }                                  \
}


#define fmin(a,b) ((a) < (b)) ? (a) : (b)

#define fpush(lz,hz) { stackLo[sp] = lz; \
                       stackHi[sp] = hz; \
                       sp++; }

#define fpop(lz,hz) { sp--;              \
                      lz = stackLo[sp];  \
                      hz = stackHi[sp]; }

#define FALLBACK_QSORT_SMALL_THRESH 10
#define FALLBACK_QSORT_STACK_SIZE   100


static
void fallbackQSort3 ( UInt32* fmap,
                      UInt32* eclass,
                      Int32   loSt,
                      Int32   hiSt )
{
	Int32 unLo, unHi, ltLo, gtHi, n, m;
	Int32 sp, lo, hi;
	UInt32 med, r, r3;
	Int32 stackLo[FALLBACK_QSORT_STACK_SIZE];
	Int32 stackHi[FALLBACK_QSORT_STACK_SIZE];

	r = 0;

	sp = 0;
	fpush ( loSt, hiSt );

	while ( sp > 0 )
	{

		AssertH ( sp < FALLBACK_QSORT_STACK_SIZE - 1, 1004 );

		fpop ( lo, hi );

		if ( hi - lo < FALLBACK_QSORT_SMALL_THRESH )
		{
			fallbackSimpleSort ( fmap, eclass, lo, hi );
			continue;
		}

		/* Random partitioning.  Median of 3 sometimes fails to
		   avoid bad cases.  Median of 9 seems to help but
		   looks rather expensive.  This too seems to work but
		   is cheaper.  Guidance for the magic constants
		   7621 and 32768 is taken from Sedgewick's algorithms
		   book, chapter 35.
		*/
		r = ( ( r * 7621 ) + 1 ) % 32768;
		r3 = r % 3;

		if ( r3 == 0 ) { med = eclass[fmap[lo]]; }
		else if ( r3 == 1 ) { med = eclass[fmap[( lo + hi ) >> 1]]; }
		else
		{
			med = eclass[fmap[hi]];
		}

		unLo = ltLo = lo;
		unHi = gtHi = hi;

		while ( 1 )
		{
			while ( 1 )
			{
				if ( unLo > unHi ) { break; }

				n = ( Int32 )eclass[fmap[unLo]] - ( Int32 )med;

				if ( n == 0 )
				{
					fswap( fmap[unLo], fmap[ltLo] );
					ltLo++;
					unLo++;
					continue;
				};

				if ( n > 0 ) { break; }

				unLo++;
			}

			while ( 1 )
			{
				if ( unLo > unHi ) { break; }

				n = ( Int32 )eclass[fmap[unHi]] - ( Int32 )med;

				if ( n == 0 )
				{
					fswap( fmap[unHi], fmap[gtHi] );
					gtHi--;
					unHi--;
					continue;
				};

				if ( n < 0 ) { break; }

				unHi--;
			}

			if ( unLo > unHi ) { break; }

			fswap( fmap[unLo], fmap[unHi] );
			unLo++;
			unHi--;
		}

		AssertD ( unHi == unLo - 1, "fallbackQSort3(2)" );

		if ( gtHi < ltLo ) { continue; }

		n = fmin( ltLo - lo, unLo - ltLo );
		fvswap( lo, unLo - n, n );
		m = fmin( hi - gtHi, gtHi - unHi );
		fvswap( unLo, hi - m + 1, m );

		n = lo + unLo - ltLo - 1;
		m = hi - ( gtHi - unHi ) + 1;

		if ( n - lo > hi - m )
		{
			fpush ( lo, n );
			fpush ( m, hi );
		}
		else
		{
			fpush ( m, hi );
			fpush ( lo, n );
		}
	}
}

#undef fmin
#undef fpush
#undef fpop
#undef fswap
#undef fvswap
#undef FALLBACK_QSORT_SMALL_THRESH
#undef FALLBACK_QSORT_STACK_SIZE


/*---------------------------------------------*/
/* Pre:
      nblock > 0
      eclass exists for [0 .. nblock-1]
      ((UChar*)eclass) [0 .. nblock-1] holds block
      ptr exists for [0 .. nblock-1]

   Post:
      ((UChar*)eclass) [0 .. nblock-1] holds block
      All other areas of eclass destroyed
      fmap [0 .. nblock-1] holds sorted order
      bhtab [ 0 .. 2+(nblock/32) ] destroyed
*/

#define       SET_BH(zz)  bhtab[(zz) >> 5] |= (1 << ((zz) & 31))
#define     CLEAR_BH(zz)  bhtab[(zz) >> 5] &= ~(1 << ((zz) & 31))
#define     ISSET_BH(zz)  (bhtab[(zz) >> 5] & (1 << ((zz) & 31)))
#define      WORD_BH(zz)  bhtab[(zz) >> 5]
#define UNALIGNED_BH(zz)  ((zz) & 0x01f)

static
void fallbackSort ( UInt32* fmap,
                    UInt32* eclass,
                    UInt32* bhtab,
                    Int32   nblock,
                    Int32   verb )
{
	Int32 ftab[257];
	Int32 ftabCopy[256];
	Int32 H, i, j, k, l, r, cc, cc1;
	Int32 nNotDone;
	Int32 nBhtab;
	UChar* eclass8 = ( UChar* )eclass;

	/*--
	   Initial 1-char radix sort to generate
	   initial fmap and initial BH bits.
	--*/
	if ( verb >= 4 )
	{
		VPrintf0 ( "        bucket sorting ...\n" );
	}

	for ( i = 0; i < 257;    i++ ) { ftab[i] = 0; }

	for ( i = 0; i < nblock; i++ ) { ftab[eclass8[i]]++; }

	for ( i = 0; i < 256;    i++ ) { ftabCopy[i] = ftab[i]; }

	for ( i = 1; i < 257;    i++ ) { ftab[i] += ftab[i - 1]; }

	for ( i = 0; i < nblock; i++ )
	{
		j = eclass8[i];
		k = ftab[j] - 1;
		ftab[j] = k;
		fmap[k] = i;
	}

	nBhtab = 2 + ( nblock / 32 );

	for ( i = 0; i < nBhtab; i++ ) { bhtab[i] = 0; }

	for ( i = 0; i < 256; i++ ) { SET_BH( ftab[i] ); }

	/*--
	   Inductively refine the buckets.  Kind-of an
	   "exponential radix sort" (!), inspired by the
	   Manber-Myers suffix array construction algorithm.
	--*/

	/*-- set sentinel bits for block-end detection --*/
	for ( i = 0; i < 32; i++ )
	{
		SET_BH( nblock + 2 * i );
		CLEAR_BH( nblock + 2 * i + 1 );
	}

	/*-- the log(N) loop --*/
	H = 1;

	while ( 1 )
	{

		if ( verb >= 4 )
		{
			VPrintf1 ( "        depth %6d has ", H );
		}

		j = 0;

		for ( i = 0; i < nblock; i++ )
		{
			if ( ISSET_BH( i ) ) { j = i; }

			k = fmap[i] - H;

			if ( k < 0 ) { k += nblock; }

			eclass[k] = j;
		}

		nNotDone = 0;
		r = -1;

		while ( 1 )
		{

			/*-- find the next non-singleton bucket --*/
			k = r + 1;

			while ( ISSET_BH( k ) && UNALIGNED_BH( k ) ) { k++; }

			if ( ISSET_BH( k ) )
			{
				while ( WORD_BH( k ) == 0xffffffff ) { k += 32; }

				while ( ISSET_BH( k ) ) { k++; }
			}

			l = k - 1;

			if ( l >= nblock ) { break; }

			while ( !ISSET_BH( k ) && UNALIGNED_BH( k ) ) { k++; }

			if ( !ISSET_BH( k ) )
			{
				while ( WORD_BH( k ) == 0x00000000 ) { k += 32; }

				while ( !ISSET_BH( k ) ) { k++; }
			}

			r = k - 1;

			if ( r >= nblock ) { break; }

			/*-- now [l, r] bracket current bucket --*/
			if ( r > l )
			{
				nNotDone += ( r - l + 1 );
				fallbackQSort3 ( fmap, eclass, l, r );

				/*-- scan bucket and generate header bits-- */
				cc = -1;

				for ( i = l; i <= r; i++ )
				{
					cc1 = eclass[fmap[i]];

					if ( cc != cc1 ) { SET_BH( i ); cc = cc1; };
				}
			}
		}

		if ( verb >= 4 )
		{
			VPrintf1 ( "%6d unresolved strings\n", nNotDone );
		}

		H *= 2;

		if ( H > nblock || nNotDone == 0 ) { break; }
	}

	/*--
	   Reconstruct the original block in
	   eclass8 [0 .. nblock-1], since the
	   previous phase destroyed it.
	--*/
	if ( verb >= 4 )
	{
		VPrintf0 ( "        reconstructing block ...\n" );
	}

	j = 0;

	for ( i = 0; i < nblock; i++ )
	{
		while ( ftabCopy[j] == 0 ) { j++; }

		ftabCopy[j]--;
		eclass8[fmap[i]] = ( UChar )j;
	}

	AssertH ( j < 256, 1005 );
}

#undef       SET_BH
#undef     CLEAR_BH
#undef     ISSET_BH
#undef      WORD_BH
#undef UNALIGNED_BH


/*---------------------------------------------*/
/*--- The main, O(N^2 log(N)) sorting       ---*/
/*--- algorithm.  Faster for "normal"       ---*/
/*--- non-repetitive blocks.                ---*/
/*---------------------------------------------*/

/*---------------------------------------------*/
static
__inline__
Bool mainGtU ( UInt32  i1,
               UInt32  i2,
               UChar*  block,
               UInt16* quadrant,
               UInt32  nblock,
               Int32*  budget )
{
	Int32  k;
	UChar  c1, c2;
	UInt16 s1, s2;

	AssertD ( i1 != i2, "mainGtU" );
	/* 1 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 2 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 3 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 4 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 5 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 6 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 7 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 8 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 9 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 10 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 11 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;
	/* 12 */
	c1 = block[i1];
	c2 = block[i2];

	if ( c1 != c2 ) { return ( c1 > c2 ); }

	i1++;
	i2++;

	k = nblock + 8;

	do
	{
		/* 1 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 2 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 3 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 4 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 5 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 6 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 7 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;
		/* 8 */
		c1 = block[i1];
		c2 = block[i2];

		if ( c1 != c2 ) { return ( c1 > c2 ); }

		s1 = quadrant[i1];
		s2 = quadrant[i2];

		if ( s1 != s2 ) { return ( s1 > s2 ); }

		i1++;
		i2++;

		if ( i1 >= nblock ) { i1 -= nblock; }

		if ( i2 >= nblock ) { i2 -= nblock; }

		k -= 8;
		( *budget )--;
	}
	while ( k >= 0 );

	return False;
}


/*---------------------------------------------*/
/*--
   Knuth's increments seem to work better
   than Incerpi-Sedgewick here.  Possibly
   because the number of elems to sort is
   usually small, typically <= 20.
--*/
static
Int32 incs[14] = { 1, 4, 13, 40, 121, 364, 1093, 3280,
                   9841, 29524, 88573, 265720,
                   797161, 2391484
                 };

static
void mainSimpleSort ( UInt32* ptr,
                      UChar*  block,
                      UInt16* quadrant,
                      Int32   nblock,
                      Int32   lo,
                      Int32   hi,
                      Int32   d,
                      Int32*  budget )
{
	Int32 i, j, h, bigN, hp;
	UInt32 v;

	bigN = hi - lo + 1;

	if ( bigN < 2 ) { return; }

	hp = 0;

	while ( incs[hp] < bigN ) { hp++; }

	hp--;

	for ( ; hp >= 0; hp-- )
	{
		h = incs[hp];

		i = lo + h;

		while ( True )
		{

			/*-- copy 1 --*/
			if ( i > hi ) { break; }

			v = ptr[i];
			j = i;

			while ( mainGtU (
			           ptr[j - h] + d, v + d, block, quadrant, nblock, budget
			        ) )
			{
				ptr[j] = ptr[j - h];
				j = j - h;

				if ( j <= ( lo + h - 1 ) ) { break; }
			}

			ptr[j] = v;
			i++;

			/*-- copy 2 --*/
			if ( i > hi ) { break; }

			v = ptr[i];
			j = i;

			while ( mainGtU (
			           ptr[j - h] + d, v + d, block, quadrant, nblock, budget
			        ) )
			{
				ptr[j] = ptr[j - h];
				j = j - h;

				if ( j <= ( lo + h - 1 ) ) { break; }
			}

			ptr[j] = v;
			i++;

			/*-- copy 3 --*/
			if ( i > hi ) { break; }

			v = ptr[i];
			j = i;

			while ( mainGtU (
			           ptr[j - h] + d, v + d, block, quadrant, nblock, budget
			        ) )
			{
				ptr[j] = ptr[j - h];
				j = j - h;

				if ( j <= ( lo + h - 1 ) ) { break; }
			}

			ptr[j] = v;
			i++;

			if ( *budget < 0 ) { return; }
		}
	}
}


/*---------------------------------------------*/
/*--
   The following is an implementation of
   an elegant 3-way quicksort for strings,
   described in a paper "Fast Algorithms for
   Sorting and Searching Strings", by Robert
   Sedgewick and Jon L. Bentley.
--*/

#define mswap(zz1, zz2) \
   { Int32 zztmp = zz1; zz1 = zz2; zz2 = zztmp; }

#define mvswap(zzp1, zzp2, zzn)       \
{                                     \
   Int32 yyp1 = (zzp1);               \
   Int32 yyp2 = (zzp2);               \
   Int32 yyn  = (zzn);                \
   while (yyn > 0) {                  \
      mswap(ptr[yyp1], ptr[yyp2]);    \
      yyp1++; yyp2++; yyn--;          \
   }                                  \
}

static
__inline__
UChar mmed3 ( UChar a, UChar b, UChar c )
{
	UChar t;

	if ( a > b ) { t = a; a = b; b = t; };

	if ( b > c )
	{
		b = c;

		if ( a > b ) { b = a; }
	}

	return b;
}

#define mmin(a,b) ((a) < (b)) ? (a) : (b)

#define mpush(lz,hz,dz) { stackLo[sp] = lz; \
                          stackHi[sp] = hz; \
                          stackD [sp] = dz; \
                          sp++; }

#define mpop(lz,hz,dz) { sp--;             \
                         lz = stackLo[sp]; \
                         hz = stackHi[sp]; \
                         dz = stackD [sp]; }


#define mnextsize(az) (nextHi[az]-nextLo[az])

#define mnextswap(az,bz)                                        \
   { Int32 tz;                                                  \
     tz = nextLo[az]; nextLo[az] = nextLo[bz]; nextLo[bz] = tz; \
     tz = nextHi[az]; nextHi[az] = nextHi[bz]; nextHi[bz] = tz; \
     tz = nextD [az]; nextD [az] = nextD [bz]; nextD [bz] = tz; }


#define MAIN_QSORT_SMALL_THRESH 20
#define MAIN_QSORT_DEPTH_THRESH (BZ_N_RADIX + BZ_N_QSORT)
#define MAIN_QSORT_STACK_SIZE 100

static
void mainQSort3 ( UInt32* ptr,
                  UChar*  block,
                  UInt16* quadrant,
                  Int32   nblock,
                  Int32   loSt,
                  Int32   hiSt,
                  Int32   dSt,
                  Int32*  budget )
{
	Int32 unLo, unHi, ltLo, gtHi, n, m, med;
	Int32 sp, lo, hi, d;

	Int32 stackLo[MAIN_QSORT_STACK_SIZE];
	Int32 stackHi[MAIN_QSORT_STACK_SIZE];
	Int32 stackD [MAIN_QSORT_STACK_SIZE];

	Int32 nextLo[3];
	Int32 nextHi[3];
	Int32 nextD [3];

	sp = 0;
	mpush ( loSt, hiSt, dSt );

	while ( sp > 0 )
	{

		AssertH ( sp < MAIN_QSORT_STACK_SIZE - 2, 1001 );

		mpop ( lo, hi, d );

		if ( hi - lo < MAIN_QSORT_SMALL_THRESH ||
		     d > MAIN_QSORT_DEPTH_THRESH )
		{
			mainSimpleSort ( ptr, block, quadrant, nblock, lo, hi, d, budget );

			if ( *budget < 0 ) { return; }

			continue;
		}

		med = ( Int32 )
		      mmed3 ( block[ptr[ lo         ] + d],
		              block[ptr[ hi         ] + d],
		              block[ptr[ ( lo + hi ) >> 1 ] + d] );

		unLo = ltLo = lo;
		unHi = gtHi = hi;

		while ( True )
		{
			while ( True )
			{
				if ( unLo > unHi ) { break; }

				n = ( ( Int32 )block[ptr[unLo] + d] ) - med;

				if ( n == 0 )
				{
					mswap( ptr[unLo], ptr[ltLo] );
					ltLo++;
					unLo++;
					continue;
				};

				if ( n >  0 ) { break; }

				unLo++;
			}

			while ( True )
			{
				if ( unLo > unHi ) { break; }

				n = ( ( Int32 )block[ptr[unHi] + d] ) - med;

				if ( n == 0 )
				{
					mswap( ptr[unHi], ptr[gtHi] );
					gtHi--;
					unHi--;
					continue;
				};

				if ( n <  0 ) { break; }

				unHi--;
			}

			if ( unLo > unHi ) { break; }

			mswap( ptr[unLo], ptr[unHi] );
			unLo++;
			unHi--;
		}

		AssertD ( unHi == unLo - 1, "mainQSort3(2)" );

		if ( gtHi < ltLo )
		{
			mpush( lo, hi, d + 1 );
			continue;
		}

		n = mmin( ltLo - lo, unLo - ltLo );
		mvswap( lo, unLo - n, n );
		m = mmin( hi - gtHi, gtHi - unHi );
		mvswap( unLo, hi - m + 1, m );

		n = lo + unLo - ltLo - 1;
		m = hi - ( gtHi - unHi ) + 1;

		nextLo[0] = lo;
		nextHi[0] = n;
		nextD[0] = d;
		nextLo[1] = m;
		nextHi[1] = hi;
		nextD[1] = d;
		nextLo[2] = n + 1;
		nextHi[2] = m - 1;
		nextD[2] = d + 1;

		if ( mnextsize( 0 ) < mnextsize( 1 ) ) { mnextswap( 0, 1 ); }

		if ( mnextsize( 1 ) < mnextsize( 2 ) ) { mnextswap( 1, 2 ); }

		if ( mnextsize( 0 ) < mnextsize( 1 ) ) { mnextswap( 0, 1 ); }

		AssertD ( mnextsize( 0 ) >= mnextsize( 1 ), "mainQSort3(8)" );
		AssertD ( mnextsize( 1 ) >= mnextsize( 2 ), "mainQSort3(9)" );

		mpush ( nextLo[0], nextHi[0], nextD[0] );
		mpush ( nextLo[1], nextHi[1], nextD[1] );
		mpush ( nextLo[2], nextHi[2], nextD[2] );
	}
}

#undef mswap
#undef mvswap
#undef mpush
#undef mpop
#undef mmin
#undef mnextsize
#undef mnextswap
#undef MAIN_QSORT_SMALL_THRESH
#undef MAIN_QSORT_DEPTH_THRESH
#undef MAIN_QSORT_STACK_SIZE


/*---------------------------------------------*/
/* Pre:
      nblock > N_OVERSHOOT
      block32 exists for [0 .. nblock-1 +N_OVERSHOOT]
      ((UChar*)block32) [0 .. nblock-1] holds block
      ptr exists for [0 .. nblock-1]

   Post:
      ((UChar*)block32) [0 .. nblock-1] holds block
      All other areas of block32 destroyed
      ftab [0 .. 65536 ] destroyed
      ptr [0 .. nblock-1] holds sorted order
      if (*budget < 0), sorting was abandoned
*/

#define BIGFREQ(b) (ftab[((b)+1) << 8] - ftab[(b) << 8])
#define SETMASK (1 << 21)
#define CLEARMASK (~(SETMASK))

static
void mainSort ( UInt32* ptr,
                UChar*  block,
                UInt16* quadrant,
                UInt32* ftab,
                Int32   nblock,
                Int32   verb,
                Int32*  budget )
{
	Int32  i, j, k, ss, sb;
	Int32  runningOrder[256];
	Bool   bigDone[256];
	Int32  copyStart[256];
	Int32  copyEnd  [256];
	UChar  c1;
	Int32  numQSorted;
	UInt16 s;

	if ( verb >= 4 ) { VPrintf0 ( "        main sort initialise ...\n" ); }

	/*-- set up the 2-byte frequency table --*/
	for ( i = 65536; i >= 0; i-- ) { ftab[i] = 0; }

	j = block[0] << 8;
	i = nblock - 1;

	for ( ; i >= 3; i -= 4 )
	{
		quadrant[i] = 0;
		j = ( j >> 8 ) | ( ( ( UInt16 )block[i] ) << 8 );
		ftab[j]++;
		quadrant[i - 1] = 0;
		j = ( j >> 8 ) | ( ( ( UInt16 )block[i - 1] ) << 8 );
		ftab[j]++;
		quadrant[i - 2] = 0;
		j = ( j >> 8 ) | ( ( ( UInt16 )block[i - 2] ) << 8 );
		ftab[j]++;
		quadrant[i - 3] = 0;
		j = ( j >> 8 ) | ( ( ( UInt16 )block[i - 3] ) << 8 );
		ftab[j]++;
	}

	for ( ; i >= 0; i-- )
	{
		quadrant[i] = 0;
		j = ( j >> 8 ) | ( ( ( UInt16 )block[i] ) << 8 );
		ftab[j]++;
	}

	/*-- (emphasises close relationship of block & quadrant) --*/
	for ( i = 0; i < BZ_N_OVERSHOOT; i++ )
	{
		block   [nblock + i] = block[i];
		quadrant[nblock + i] = 0;
	}

	if ( verb >= 4 ) { VPrintf0 ( "        bucket sorting ...\n" ); }

	/*-- Complete the initial radix sort --*/
	for ( i = 1; i <= 65536; i++ ) { ftab[i] += ftab[i - 1]; }

	s = block[0] << 8;
	i = nblock - 1;

	for ( ; i >= 3; i -= 4 )
	{
		s = ( s >> 8 ) | ( block[i] << 8 );
		j = ftab[s] - 1;
		ftab[s] = j;
		ptr[j] = i;
		s = ( s >> 8 ) | ( block[i - 1] << 8 );
		j = ftab[s] - 1;
		ftab[s] = j;
		ptr[j] = i - 1;
		s = ( s >> 8 ) | ( block[i - 2] << 8 );
		j = ftab[s] - 1;
		ftab[s] = j;
		ptr[j] = i - 2;
		s = ( s >> 8 ) | ( block[i - 3] << 8 );
		j = ftab[s] - 1;
		ftab[s] = j;
		ptr[j] = i - 3;
	}

	for ( ; i >= 0; i-- )
	{
		s = ( s >> 8 ) | ( block[i] << 8 );
		j = ftab[s] - 1;
		ftab[s] = j;
		ptr[j] = i;
	}

	/*--
	   Now ftab contains the first loc of every small bucket.
	   Calculate the running order, from smallest to largest
	   big bucket.
	--*/
	for ( i = 0; i <= 255; i++ )
	{
		bigDone     [i] = False;
		runningOrder[i] = i;
	}

	{
		Int32 vv;
		Int32 h = 1;

		do { h = 3 * h + 1; }
		while ( h <= 256 );

		do
		{
			h = h / 3;

			for ( i = h; i <= 255; i++ )
			{
				vv = runningOrder[i];
				j = i;

				while ( BIGFREQ( runningOrder[j - h] ) > BIGFREQ( vv ) )
				{
					runningOrder[j] = runningOrder[j - h];
					j = j - h;

					if ( j <= ( h - 1 ) ) { goto zero; }
				}

zero:
				runningOrder[j] = vv;
			}
		}
		while ( h != 1 );
	}

	/*--
	   The main sorting loop.
	--*/

	numQSorted = 0;

	for ( i = 0; i <= 255; i++ )
	{

		/*--
		   Process big buckets, starting with the least full.
		   Basically this is a 3-step process in which we call
		   mainQSort3 to sort the small buckets [ss, j], but
		   also make a big effort to avoid the calls if we can.
		--*/
		ss = runningOrder[i];

		/*--
		   Step 1:
		   Complete the big bucket [ss] by quicksorting
		   any unsorted small buckets [ss, j], for j != ss.
		   Hopefully previous pointer-scanning phases have already
		   completed many of the small buckets [ss, j], so
		   we don't have to sort them at all.
		--*/
		for ( j = 0; j <= 255; j++ )
		{
			if ( j != ss )
			{
				sb = ( ss << 8 ) + j;

				if ( ! ( ftab[sb] & SETMASK ) )
				{
					Int32 lo = ftab[sb]   & CLEARMASK;
					Int32 hi = ( ftab[sb + 1] & CLEARMASK ) - 1;

					if ( hi > lo )
					{
						if ( verb >= 4 )
							VPrintf4 ( "        qsort [0x%x, 0x%x]   "
							           "done %d   this %d\n",
							           ss, j, numQSorted, hi - lo + 1 );

						mainQSort3 (
						   ptr, block, quadrant, nblock,
						   lo, hi, BZ_N_RADIX, budget
						);
						numQSorted += ( hi - lo + 1 );

						if ( *budget < 0 ) { return; }
					}
				}

				ftab[sb] |= SETMASK;
			}
		}

		AssertH ( !bigDone[ss], 1006 );

		/*--
		   Step 2:
		   Now scan this big bucket [ss] so as to synthesise the
		   sorted order for small buckets [t, ss] for all t,
		   including, magically, the bucket [ss,ss] too.
		   This will avoid doing Real Work in subsequent Step 1's.
		--*/
		{
			for ( j = 0; j <= 255; j++ )
			{
				copyStart[j] =  ftab[( j << 8 ) + ss]     & CLEARMASK;
				copyEnd  [j] = ( ftab[( j << 8 ) + ss + 1] & CLEARMASK ) - 1;
			}

			for ( j = ftab[ss << 8] & CLEARMASK; j < copyStart[ss]; j++ )
			{
				k = ptr[j] - 1;

				if ( k < 0 ) { k += nblock; }

				c1 = block[k];

				if ( !bigDone[c1] )
				{
					ptr[ copyStart[c1]++ ] = k;
				}
			}

			for ( j = ( ftab[( ss + 1 ) << 8] & CLEARMASK ) - 1; j > copyEnd[ss]; j-- )
			{
				k = ptr[j] - 1;

				if ( k < 0 ) { k += nblock; }

				c1 = block[k];

				if ( !bigDone[c1] )
				{
					ptr[ copyEnd[c1]-- ] = k;
				}
			}
		}

		AssertH ( ( copyStart[ss] - 1 == copyEnd[ss] )
		          ||
		          /* Extremely rare case missing in bzip2-1.0.0 and 1.0.1.
		             Necessity for this case is demonstrated by compressing
		             a sequence of approximately 48.5 million of character
		             251; 1.0.0/1.0.1 will then die here. */
		          ( copyStart[ss] == 0 && copyEnd[ss] == nblock - 1 ),
		          1007 )

		for ( j = 0; j <= 255; j++ ) { ftab[( j << 8 ) + ss] |= SETMASK; }

		/*--
		   Step 3:
		   The [ss] big bucket is now done.  Record this fact,
		   and update the quadrant descriptors.  Remember to
		   update quadrants in the overshoot area too, if
		   necessary.  The "if (i < 255)" test merely skips
		   this updating for the last bucket processed, since
		   updating for the last bucket is pointless.

		   The quadrant array provides a way to incrementally
		   cache sort orderings, as they appear, so as to
		   make subsequent comparisons in fullGtU() complete
		   faster.  For repetitive blocks this makes a big
		   difference (but not big enough to be able to avoid
		   the fallback sorting mechanism, exponential radix sort).

		   The precise meaning is: at all times:

		      for 0 <= i < nblock and 0 <= j <= nblock

		      if block[i] != block[j],

		         then the relative values of quadrant[i] and
		              quadrant[j] are meaningless.

		         else {
		            if quadrant[i] < quadrant[j]
		               then the string starting at i lexicographically
		               precedes the string starting at j

		            else if quadrant[i] > quadrant[j]
		               then the string starting at j lexicographically
		               precedes the string starting at i

		            else
		               the relative ordering of the strings starting
		               at i and j has not yet been determined.
		         }
		--*/
		bigDone[ss] = True;

		if ( i < 255 )
		{
			Int32 bbStart  = ftab[ss << 8] & CLEARMASK;
			Int32 bbSize   = ( ftab[( ss + 1 ) << 8] & CLEARMASK ) - bbStart;
			Int32 shifts   = 0;

			while ( ( bbSize >> shifts ) > 65534 ) { shifts++; }

			for ( j = bbSize - 1; j >= 0; j-- )
			{
				Int32 a2update     = ptr[bbStart + j];
				UInt16 qVal        = ( UInt16 )( j >> shifts );
				quadrant[a2update] = qVal;

				if ( a2update < BZ_N_OVERSHOOT )
				{
					quadrant[a2update + nblock] = qVal;
				}
			}

			AssertH ( ( ( bbSize - 1 ) >> shifts ) <= 65535, 1002 );
		}

	}

	if ( verb >= 4 )
		VPrintf3 ( "        %d pointers, %d sorted, %d scanned\n",
		           nblock, numQSorted, nblock - numQSorted );
}

#undef BIGFREQ
#undef SETMASK
#undef CLEARMASK


/*---------------------------------------------*/
/* Pre:
      nblock > 0
      arr2 exists for [0 .. nblock-1 +N_OVERSHOOT]
      ((UChar*)arr2)  [0 .. nblock-1] holds block
      arr1 exists for [0 .. nblock-1]

   Post:
      ((UChar*)arr2) [0 .. nblock-1] holds block
      All other areas of block destroyed
      ftab [ 0 .. 65536 ] destroyed
      arr1 [0 .. nblock-1] holds sorted order
*/
void BZ2_blockSort ( EState* s )
{
	UInt32* ptr    = s->ptr;
	UChar*  block  = s->block;
	UInt32* ftab   = s->ftab;
	Int32   nblock = s->nblock;
	Int32   verb   = s->verbosity;
	Int32   wfact  = s->workFactor;
	UInt16* quadrant;
	Int32   budget;
	Int32   budgetInit;
	Int32   i;

	if ( nblock < 10000 )
	{
		fallbackSort ( s->arr1, s->arr2, ftab, nblock, verb );
	}
	else
	{
		/* Calculate the location for quadrant, remembering to get
		   the alignment right.  Assumes that &(block[0]) is at least
		   2-byte aligned -- this should be ok since block is really
		   the first section of arr2.
		*/
		i = nblock + BZ_N_OVERSHOOT;

		if ( i & 1 ) { i++; }

		quadrant = ( UInt16* )( &( block[i] ) );

		/* (wfact-1) / 3 puts the default-factor-30
		   transition point at very roughly the same place as
		   with v0.1 and v0.9.0.
		   Not that it particularly matters any more, since the
		   resulting compressed stream is now the same regardless
		   of whether or not we use the main sort or fallback sort.
		*/
		if ( wfact < 1  ) { wfact = 1; }

		if ( wfact > 100 ) { wfact = 100; }

		budgetInit = nblock * ( ( wfact - 1 ) / 3 );
		budget = budgetInit;

		mainSort ( ptr, block, quadrant, ftab, nblock, verb, &budget );

		if ( verb >= 3 )
			VPrintf3 ( "      %d work, %d block, ratio %5.2f\n",
			           budgetInit - budget,
			           nblock,
			           ( float )( budgetInit - budget ) /
			           ( float )( nblock == 0 ? 1 : nblock ) );

		if ( budget < 0 )
		{
			if ( verb >= 2 )
				VPrintf0 ( "    too repetitive; using fallback"
				           " sorting algorithm\n" );

			fallbackSort ( s->arr1, s->arr2, ftab, nblock, verb );
		}
	}

	s->origPtr = -1;

	for ( i = 0; i < s->nblock; i++ )
		if ( ptr[i] == 0 )
		{ s->origPtr = i; break; };

	AssertH( s->origPtr != -1, 1003 );
}


/*-------------------------------------------------------------*/
/*--- end                                       blocksort.c ---*/
/*-------------------------------------------------------------*/
