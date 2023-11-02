#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include "map_reduce.h"
#include "config.h"

char *file_name;
uint32_t nb_thread;
struct thread_info_s *tinfo;

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void print_usage(char **argv)
{
    printf("ERROR: Wrong arguments\n");
    printf("usage: %s FILE_NAME THREAD_NUMBER\n", argv[0]);
    exit(EXIT_FAILURE);
}

static void extract_arguments(int argc, char **argv)
{
    if (argc < 3)
        print_usage(argv);

    file_name = argv[1];
    if (!file_name)
        print_usage(argv);

    nb_thread = atoi(argv[2]);
    if (!nb_thread)
        print_usage(argv);

    dmsg("File %s, thread number %d\n", file_name, nb_thread);
}

static void start_threads(void)
{
    int i;

    tinfo = calloc(nb_thread, sizeof(struct thread_info_s));
    if (tinfo == NULL)
        handle_error("calloc");

    for (i = 0; i < nb_thread; i++) {
        tinfo[i].thread_num = i;
        if (pthread_create(&tinfo[i].thread_id, NULL, mr_map, &tinfo[i]))
            handle_error("thread create");

        dmsg("Thread number %d launched\n", i);
    }
}

static void wait_threads()
{
    int i;
    for (i = 0; i < nb_thread; i++) {
        dmsg("Waiting for thread %d\n", i);
        if (pthread_join(tinfo[i].thread_id, NULL))
            handle_error("thread join");
    }
    free(tinfo);
}

#if PRINT_TIME
static double time_so_far()
{
    struct timeval tp;

    if (gettimeofday(&tp, (struct timezone *)NULL) == -1)
        perror("gettimeofday");

    dmsg("%s: tp.tv_sec %g tp.tv_usec %g\n", __FUNCTION__, (double)tp.tv_sec, (double)tp.tv_usec);

    return ((double)(tp.tv_sec) * 1000.0) + (((double)tp.tv_usec / 1000.0));
}
#endif

int main(int argc, char **argv)
{
#if PRINT_TIME
    double start, end;
#endif

    extract_arguments(argc, argv);

#if PRINT_TIME
    start = time_so_far();
#endif
    if (mr_init())
        exit(EXIT_FAILURE);

    start_threads();
    wait_threads();

    if (mr_reduce())
        exit(EXIT_FAILURE);

#if PRINT_TIME
    /* Done here, to avoid counting the printing... */
    end = time_so_far();
#endif

    if (mr_print())
        exit(EXIT_FAILURE);
    if (mr_destroy())
        exit(EXIT_FAILURE);

#if PRINT_TIME
    printf("Done in %g msec\n", end - start);
#endif

    exit(EXIT_SUCCESS);
}
