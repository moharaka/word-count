#ifndef __MAP_REDUCE_H__
#define __MAP_REDUCE_H__

#include <stdint.h>
#include <pthread.h>


extern char* file_name;
extern uint32_t nb_thread;

struct thread_info_s
{
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       thread_num;       /* Application-defined thread # */
};

/* Executed by the main thread */
int mr_init(void);

/* Executed by worker threads */
void* mr_map(void* id);

/* Executed by the main thread */
int mr_reduce(void);

/* Executed by the main thread */
int mr_print(void);

/* Executed by the main thread */
int mr_destroy(void);

#endif /* __MAP_REDUCE_H__ */
