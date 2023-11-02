#ifndef __WC_CACHE__
#define __WC_CACHE__

#include "htable.h"
#include "config.h"

struct wc_cache_s {
    struct htable_s htable;
};

struct wc_word_s {
    char word[AMAX_WORD_SIZE];
    char *full_word;
    uint32_t counter;
    struct hnode_s hn;
    struct hnode_s hn_main;
};

/* Initialise each (worker+main) a cache */
int wcc_init(uint32_t nb_threads, uint32_t nb_word);

/* destroy ressource allocated by wcc_init */
int wcc_destroy(uint32_t nb_threads);

/* Add a word to the cache of the thread tid. If the word	*
 * already exist, increment its counter. Else add a new word. 	*/
int wcc_add_word(uint32_t tid, char *word, uint32_t count);

/* Merge the results of all threads to the main cache.		*
 * This Merge is done in parralel by all threads.		*
 * Each thread handle at least nb_buckets/nb_thread buckets.	*/
int wcc_merge_results(uint32_t tid, uint32_t nb_threads);

/* Print the merged result */
int wcc_print(int id);

#endif /*__WC_CACHE__*/
