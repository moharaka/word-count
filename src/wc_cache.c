#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "wc_cache.h"
#include "htable.h"

/* TODO: handle '-' character */
/* TODO: add case sensitive support */
/* TODO: add number support: see alpahnum */
/* FIXME: remove assumptions on ascii encoding *
 * or place such assumptions in a separate file */
/* Note: The cache table can be improved (h func) */

static uint32_t nb_buckets;
static struct wc_cache_s main_wc_cache;
static struct wc_cache_s *thread_wc_caches;

#define FIRST_LOWER_LETTER 'a'
#define NB_LETTERS (('z' - 'a') + 1)
#define MIN_NB_BCKT NB_LETTERS

#define WC_GET_WORD(_wd)		\
(_wd->full_word ? _wd->full_word : _wd->word)

#define MIN_MAX(a,b,op)({ 		\
	__typeof__ (a) _a = (a); 	\
	__typeof__ (b) _b = (b); 	\
	_a op _b ? _a : _b; })

#define MAX(a,b) MIN_MAX(a,b,>)
#define MIN(a,b) MIN_MAX(a,b,<)

static int wcc_hash_bkt(void* key, hash_t *hash, uint32_t *bkt);
static int wcc_compare_main(struct hnode_s *hn, void* key);
static int wcc_compare(struct hnode_s *hn, void* key);
#if ALPHA_ORDER
static int scale_range_init();
#endif

int wcc_init(uint32_t nb_thread, uint32_t nb_word)
{
	int i;

	/* FIXME: a resizable htable would be better */
	nb_buckets = MAX(MIN(nb_word, MAX_NB_WORD), MIN_NB_BCKT);
	//printf("number of bucket %d\n", nb_buckets);
	thread_wc_caches = malloc(sizeof(struct wc_cache_s)*nb_thread);

#if ALPHA_ORDER
	scale_range_init();
#endif

	for(i=0; i < nb_thread; i++)
	{
		if(ht_init(&thread_wc_caches[i].htable, 
				wcc_hash_bkt, wcc_compare, nb_buckets))
			return -1;
	}
		
	if(ht_init(&main_wc_cache.htable, wcc_hash_bkt, 
					wcc_compare_main, nb_buckets))
		return -1;

	dmsg("htable initialised\n");
	return 0;
}


/****** The next functions are for the hash table **********/

#if CASE_SENSITIVE
static int humcmp(const char *s1, const char *s2)
{
	int case_diff;
	unsigned char c1, lc1;
	unsigned char c2, lc2;

	c1 = *s1;
	c2 = *s2;
	lc1 = tolower(c1);
	lc2 = tolower(c2);

	case_diff = 0;

	while (c1 != '\0') {
		if (c2 == '\0') return 1;
		if (lc2 > lc1)   return -1;
		if (lc1 > lc2)   return 1;
		
		//lc1 == lc2
		if((c1 != c2) && !case_diff)
		{
			case_diff=c2-c1;
			goto NEXT_ITER;
		}

NEXT_ITER:
		c1 = *++s1;
		c2 = *++s2;
		lc1 = tolower(c1);
		lc2 = tolower(c2);
	}

	if (c2 != '\0') return -1;

	if(case_diff) 
	{
		/* Pb: Order between letters depend on the text */
		return case_diff;
	}

	return 0;
}
#endif

/* Called to compare word if their hash value is similar */
static int __wcc_compare(struct hnode_s *hn, void* key, char main)
{
	int res;
	char* word;
	char* hn_word;
	struct wc_word_s *wcw;

	word = (char*) key;

	if(main)
		wcw = container_of(hn, struct wc_word_s, hn_main);
	else
		wcw = container_of(hn, struct wc_word_s, hn);

	hn_word = WC_GET_WORD(wcw);

#if CASE_SENSITIVE
	res = humcmp(hn_word, word);
#else
	res = strcasecmp(hn_word, word);
#endif

	dmsg3("Comparing %s and %s = %d\n", hn_word, word, res);

	return res;
}

static int wcc_compare(struct hnode_s *hn, void* key)
{
	return __wcc_compare(hn, key, 0);

}

static int wcc_compare_main(struct hnode_s *hn, void* key)
{
	return __wcc_compare(hn, key, 1);
}


#if ALPHA_ORDER
uint32_t code_min;
uint32_t code_max;
uint32_t code_range;

static uint32_t get_code(char* word)
{
	int i;
	char c;
	char shift;
	uint32_t code;

	code = 0;
	assert(strlen(word) >= 1);
        /* The hash value is a concatenation of the letters */

	assert(NB_LETTERS <= UCHAR_MAX);
	shift = (char)(sizeof(unsigned int)*CHAR_BIT -
				__builtin_clz(NB_LETTERS));

	i=((sizeof(code)*8)/shift)-1;
	dmsg3("shif %d, start %d\n",shift,i);
	for(; i>=0 && *word; i--)
	{
		c = tolower(*(word));
		c =  c - FIRST_LOWER_LETTER;
		code |= ((uint32_t) c) << (i*shift);
		word++;
	}

	return code;
}

static int scale_range_init()
{
	code_min = get_code("a");
	code_max = get_code("zzzzzzzzzz");
	code_range = (code_max-code_min);

	return 0;
}

static inline 
uint32_t scale_range(uint32_t code)
{
	uint32_t res;

	assert(code_range);
	res = (uint32_t) ((((double)code-code_min)*(double)nb_buckets)/code_range);
	dmsg3("nb_bucket %d, code %x min: %x max: %x range %x; res %x\n", 
			nb_buckets, code, code_min, code_max, code_range, res);
	return res;

}


/* Must keep an an alphabetic order when assiging buckets. */
static int wcc_hash_bkt(void* key, hash_t *hash, uint32_t *bkt)
{
	uint32_t code;

	code = get_code((char*)key);

	assert(sizeof(hash) >= sizeof(uint32_t));
	*hash = (hash_t) code;

	*bkt = scale_range(code);

	dmsg3("%s hashed to %x bucket is %d\n", (char*)key, *hash, *bkt);

	return 0;
}
#else

static uint32_t
__hash(unsigned char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int wcc_hash_bkt(void* key, hash_t *hash, uint32_t *bkt)
{
	*hash = (hash_t) __hash((unsigned char*)key);
	*bkt = *hash%nb_buckets;
	return 0;
}

#endif

/*** htable functions end *******/
/*******************************/

/* Copy while setting to lower case */
/* from man page */
static char *wcc_strncpy(char *dest, char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; i++)
#if CASE_SENSITIVE
		dest[i] = src[i];
#else
		dest[i] = (char)tolower((int)(src[i]));
#endif
	return dest;
}


/* Add a word to the cache of the thread tid. If the word	*
 * already exist, increment its counter. Else add a new word. 	*/
int wcc_add_word(uint32_t tid, char* word, uint32_t count)
{
	struct hnode_s *hn;
	struct wc_word_s *wcw;
	struct wc_cache_s *wcc;
	int ret;

	ret = 0;
	wcc = &thread_wc_caches[tid];
	assert(count);
	
	dmsg3("%d: adding %s\n", tid, word);

	if(!(hn = ht_find(&wcc->htable, word)))
	{
		/* word absent. Allocate a new wc_word_s */
		if(!(wcw = calloc(1, sizeof(struct wc_word_s))))
			return -1;

		/* cpy char (malloc if to big) */
		/* -1 to accomodate the null caracter */
		if(count > (AMAX_WORD_SIZE-1))
		{
			wcw->full_word = calloc(1, count+1);
		}
		
		wcc_strncpy(WC_GET_WORD(wcw), word, count);

		/* Add the new world to the table */
		ret = ht_insert(&wcc->htable, &wcw->hn, word);
#ifndef NDEBUG
		assert(!ret);
#endif
	}else
	{
		wcw = container_of(hn, struct wc_word_s, hn);
	}

	wcw->counter++;

	dmsg3("%d: word added %s\n", tid, WC_GET_WORD(wcw));

	return 0;
}

/* Merge the results of all threads to the main cache.		*
 * This Merge is done in parralel by all threads.		*
 * Each thread handle at least nb_buckets/nb_thread buckets.	*/
static int __wcc_merge_results(uint32_t tid, uint32_t bckt, 
					struct wc_cache_s *wcc);
int wcc_merge_results(uint32_t tid, uint32_t nb_thread)
{
	uint32_t i,j;
	uint32_t wk_bend;
	uint32_t wk_bstart;
	uint32_t nb_worker;
	uint32_t wk_buckets;
	struct wc_cache_s *wcc;

	/* Keep the number of workers <= nbthread */
	if(nb_thread > nb_buckets)
	{
		if(tid > nb_buckets-1)
			return 0;
		nb_worker = nb_buckets;
	}else
		nb_worker = nb_thread;

	
	/* Each thread will treat at least wk_buckets*/
	wk_buckets = nb_buckets / nb_worker;

	/* The range that this thread will treat */
	wk_bstart = wk_buckets*tid;
	wk_bend = wk_bstart+wk_buckets;
	/* last thread must also do last buckets */
	if((tid == (nb_worker-1)))
		wk_bend += nb_buckets % nb_worker;
	
	//printf("thread %d : bucket %d to %d \n", tid, wk_bstart, wk_bend);
	for(i=0; i < nb_thread; i++)
	{
		wcc = &thread_wc_caches[i];
		for(j=wk_bstart; j < wk_bend; j++)
		{
			/* Walk the buckets of all threads  from wk_bstart   *	
			 * to wk_bend.	Agregate the nodes of theses buckets *
			 * in the main_wc_cache				     */
			__wcc_merge_results(tid, j, wcc);
		}
	}
	return 0;
}

static int __wcc_merge_results(uint32_t tid, uint32_t j, struct wc_cache_s *wcc)
{
#ifndef NDEBUG
	int res;
#endif
	char *word;
	struct wc_cache_s *mcc;
	struct hnode_s *hn, *ihn;
	struct wc_word_s *wcw, *iwcw;

	mcc = &main_wc_cache;

	ihn = ht_get_bfirst(&wcc->htable, j);	
	for(; ihn; ihn = ht_get_next(&wcc->htable, j, ihn))
	{
		iwcw = container_of(ihn, struct wc_word_s, hn);

		/* check if word alredy exist in main_cache */
		word = WC_GET_WORD(iwcw);
		if((hn = ht_find(&mcc->htable, word)))
		{
			/*word alread exist. Get the word and increment it */
			dmsg3("'%s' exist adding count by thread %d\n", 
								word, tid);
			wcw = container_of(hn, struct wc_word_s, hn_main);
			wcw->counter+= iwcw->counter;
		}else{
			/*word does not exist. insert the new word usin hn_main */
			dmsg3("'%s' added to main table by thread %d\n", 
							word, tid);
#ifndef NDEBUG
			res = 
#endif
			ht_insert(&mcc->htable, &iwcw->hn_main, word);
#ifndef NDEBUG
			assert(!res);
#endif
		}
	}

	return 0;

}

int wcc_print(int id)
{
	uint32_t j;
	struct hnode_s *ihn;
	struct wc_word_s *iwcw;
	struct wc_cache_s *wcc;
	uint32_t total, count_total;
	uint32_t bkt_total, empty_bkt;

	total = 0;
	count_total = 0;
	bkt_total = 0;
	empty_bkt = 0;

	if(id == -1)
		wcc = &main_wc_cache;
	else
		wcc = &thread_wc_caches[id];

	dmsg("---------------- Thread %d word cache -----------\n", id);
	for(j=0; j < nb_buckets; j++)
	{
		/* Walk all the buckets of wcc */
		ihn = ht_get_bfirst(&wcc->htable, j);	
		for(; ihn; ihn = ht_get_next(&wcc->htable, j, ihn))
		{
			if(id == -1)
				iwcw = container_of(ihn, struct wc_word_s, hn_main);
			else
				iwcw = container_of(ihn, struct wc_word_s, hn);
			printf("%s = %d\n", WC_GET_WORD(iwcw), iwcw->counter);
			bkt_total++;
			total++;
			count_total += iwcw->counter;
		}
		if(!bkt_total) empty_bkt++;
		//if(id>=0) printf("bucket %d, total %d\n", j, bkt_total); 
		bkt_total=0;
	}
#if PRINT_TOTAL
//#if 1
	printf("Total word %d, total words counter %d, full bkt %d (ideal %d)\n", 
				total, count_total, nb_buckets-empty_bkt, 
				MIN(total, nb_buckets));
#endif
	return 0;
}

static int __wcc_destroy(struct wc_cache_s *wcc, int id)
{
	uint32_t j;
	struct hnode_s *ihn;
	struct wc_word_s *iwcw;

	/* Walk all the buckets of wcc */
	for(j=0; j < nb_buckets; j++)
	{
		ihn = ht_get_bfirst(&wcc->htable, j);	
		for(; ihn; ihn = ht_get_next(&wcc->htable, j, ihn))
		{
			if(id == -1)
				iwcw = container_of(ihn, struct wc_word_s, hn_main);
			else
				iwcw = container_of(ihn, struct wc_word_s, hn);
			hn_destroy(&iwcw->hn_main);
			hn_destroy(&iwcw->hn);
			if(iwcw->full_word)
				free(iwcw->full_word);
			free(iwcw);
		}
	}
	return 0;
}

/* Free nodes and htable */
int wcc_destroy(uint32_t nb_thread)
{
	uint32_t i;
	struct wc_cache_s *wcc;

	/* destroy thread table nodes */
	/* no need to destroy main table nodes *
	 * (since it reuse thread nodes) */
	for(i=0; i < nb_thread; i++)
	{
		wcc = &thread_wc_caches[i];
		if(__wcc_destroy(wcc, i))
			return -1;
	}

	/* destroy tables */
	for(i=0; i < nb_thread; i++)
	{
		if(ht_destroy(&thread_wc_caches[i].htable))
			return -1;
	}

	free(thread_wc_caches); 
		
	if(ht_destroy(&main_wc_cache.htable))
		return -1;


	return 0;
}
