#pragma once

#include <assert.h>

/* General config */
#define CASE_SENSITIVE 1
#define ALPHA_ORDER 0

/* Word cache configs */
#define AMAX_WORD_SIZE 32
#define MAX_NB_WORD 8192

/* Output configs */
#define PRINT_TIME 0
#define PRINT_TOTAL 1

/* I/O operation configs */
#define BUFFER_SIZE 4096
#define USE_PREAD 1
#define POPULATE_MMAP 1

/* Debug config */
#if 0
#define dmsg printf
#define dmsg2 printf
#define dmsg3 printf
#else
#define dmsg(...) /**/
#define dmsg2(...) /**/
#define dmsg3(...) /**/
#endif
