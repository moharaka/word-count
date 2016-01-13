#ifndef _FILE_ACCESS_H
#define _FILE_ACCESS_H

#include "config.h"
#include <stdint.h>
#include <sys/types.h>

/* Initialise file access for worker threads.	*
 * Should be called by main thread. 		*
 * return 0 success; else error.		*/
int fa_init(char *file, uint32_t nb_thread, off_t *fsz);

/* destroy fa_init allocated ressource */
int fa_destroy();

/* Initialise file read access.  	*
 * Should be called by worker threads. 	*
 * return 0 success; else error.	*/
int fa_read_init();

/* destroy fa_read_init allocated ressource */
int fa_read_destroy();

/* Read worker part of the file. 	*
 * Should be called by worker threads. 	*/
off_t fa_read(uint32_t id, char **buff, off_t size, off_t pos);

#endif
