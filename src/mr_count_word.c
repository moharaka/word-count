#include "map_reduce.h"
#include "file_access.h"
#include "wc_cache.h"
#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>

#define IS_A_LETTER(c)		\
(BETWEEN(*buff, 'A', 'Z') ||	\
BETWEEN(*buff, 'a', 'z'))

#define BETWEEN(_wd, _min, _max) \
((_wd >= _min) && (_wd <= _max))

static off_t file_size;
static pthread_barrier_t barrier;

/*********** Main thread functions **************/
int mr_init(void)
{
    if (pthread_barrier_init(&barrier, NULL, nb_thread)) {
        perror("bar init");
        return -1;
    }

    if (fa_init(file_name, nb_thread, &file_size))
        return -1;

    /* TODO: better heuristique for nb_buckets */
    if (wcc_init(nb_thread, file_size / AMAX_WORD_SIZE))
        return -1;

    return 0;
}

int mr_destroy(void)
{
    if (pthread_barrier_destroy(&barrier)) {
        perror("bar init");
        return -1;
    }

    if (fa_init(file_name, nb_thread, &file_size))
        return -1;

    if (wcc_destroy(nb_thread))
        return -1;

    return 0;
}

int mr_reduce(void)
{
    /* The merge is done by worker threads */
    return 0;
}

int mr_print(void)
{
    return wcc_print(-1);
}

/********* Worker thread variables/functions **********/
static __thread off_t worker_slice;
static __thread off_t worker_current;

static __thread uint32_t count = 0;
static __thread uint32_t wsize = 0;
static __thread char *word = NULL;

/* The next three funcitons handle a buffer of the file.	*
 * Note that a buffer may end in the middle of word.		*
 * The word will be completed on the next call to the func.	*/
static int add_letter(char ltr)
{
    if ((count > wsize - 1) || (!wsize)) {
        wsize += AMAX_WORD_SIZE;
        if ((word = realloc(word, wsize)) == NULL)
            return -1;
    }

    word[count] = ltr;
    count++;
    return 0;
}

static int add_sep(uint32_t tid)
{
    if (count) {
        /* Add current word */
        word[count] = '\0';
        if (wcc_add_word(tid, word, count))
            return -1;
        count = 0;
    }
    return 0;
}

static int buff_hdl(uint32_t tid, char *buff, size_t size, char last)
{
    dmsg2("start %s, %d: word %s, count %d, *buff %c\n", __FUNCTION__, tid, word, count, *buff);

    for (; size; size--, buff++) {
        if (!IS_A_LETTER(*buff)) {
            /*Not a letter */
            if (add_sep(tid))
                return -1;
            continue;
        }
        /* A letter */
        if (add_letter(*buff))
            return -1;
    }

    /* If this is the last buffer, end the word (if any) */
    if (last) {
        add_sep(tid);
    }

    dmsg3("end %s, %d: word %s, count %d, *buff %c\n", __FUNCTION__, tid, word, count, *buff);

    return 0;
}

/* Set for each worker the buffer slice to handle */
static int buff_init(uint32_t tid)
{
    char *buff;
    off_t worker_end;

    if (fa_read_init())
        return -1;

    worker_slice = file_size / nb_thread;
    worker_current = worker_slice * tid;

    /* Last thread handle remaining bytes */
    if (tid == (nb_thread - 1))
        worker_slice += file_size % nb_thread;

    worker_end = worker_current + worker_slice;

    /* Balance worker_slice to include words at the ends */
    /* skip first letters if we are not the first thread  */
    do {
        if (tid == 0)
            break;
        if (fa_read(tid, &buff, 1, worker_current) != 1)
            return -1;
        if (!IS_A_LETTER(*buff))
            break;
        dmsg3("%d skipping letter %c\n", tid, *buff);
        worker_current++;
        worker_slice--;
    } while (*buff);

    /* add letters of the last word if we are not the last thread */
    do {
        if (tid == (nb_thread - 1))
            break;
        if (fa_read(tid, &buff, 1, worker_end) != 1)
            return -1;
        if (!IS_A_LETTER(*buff))
            break;
        dmsg3("%d adding letter %c\n", tid, *buff);
        worker_end++;
        worker_slice++;
    } while (*buff);

    dmsg2("%d: slice start %d, slice size %d, slice end %d\n", tid, worker_current, worker_slice, worker_end);

    return 0;
}

static int buff_destroy()
{
    free(word);

    if (fa_read_destroy())
        return -1;
    return 0;
}

/* Read a buffer from the file. */
static int buff_read(uint32_t tid, char **buff, off_t * size, char *last)
{
    off_t size_read;

    if (!worker_slice)
        return 0;

    dmsg2("Worker %d, worker current %d, slice size %d\n", tid, worker_current, worker_slice);

    size_read = fa_read(tid, buff, worker_slice, worker_current);

    if (size_read == -1)
        return -1;

    *size = size_read;
    worker_current += size_read;
    worker_slice -= size_read;

    if (!worker_slice)
        *last = 1;

    dmsg2("Worker %d, size read %d, slice start %d, slice size %d\n", tid, size_read, worker_current, worker_slice);

    return 0;
}

void *mr_map(void *id_arg)
{
    struct wc_cache;
    uint32_t tid;
    off_t size;
    char *buff;
    char last;
    int ret;

    last = 0;
    ret = -1;
    size = 0;
    tid = ((struct thread_info_s *)id_arg)->thread_num;

    if ((ret = buff_init(tid)))
        goto _END;

    while (!(ret = buff_read(tid, &buff, &size, &last))) {
        /* Nothing to do */
        if (!size)
            break;

        if (buff_hdl(tid, buff, size, last))
            goto _END;

        /* If this was the last buffer */
        if (last)
            break;
    }

    if (buff_destroy())
        goto _END;

    /* wait for other worker before merging */
    if (pthread_barrier_wait(&barrier) > 0) {
        perror("bar wait");
        goto _END;
    }

    /* merge results (done in parrallel) */
    ret = wcc_merge_results(tid, nb_thread);

    //wcc_print(tid);
 _END:
    return ((void *)(long)ret);
}
