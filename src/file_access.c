#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "file_access.h"
#include "wc_cache.h"

int fd;
off_t file_size;
void *file_content;

__thread char *worker_buffer;

#if POPULATE_MMAP
#define MMAP_FLAGS (MAP_POPULATE | MAP_PRIVATE)
#else
#define MMAP_FLAGS (MAP_PRIVATE)
#endif

int fa_init(char *file, uint32_t nb_thread, off_t * fsz)
{

    /* Opening file */
    if ((fd = open(file, O_RDONLY)) < 0) {
        perror("open");
        return -1;
    }

    /* Get file size */
    if ((file_size = lseek(fd, 0, SEEK_END)) < 0) {
        perror("open");
        return -1;
    }
    dmsg("File %s has size %d\n", file, file_size);

#if USE_PREAD
    file_content = NULL;
    dmsg("Accessing file using pread\n");
#else
    file_content = mmap(NULL, file_size, PROT_READ, MMAP_FLAGS, fd, 0);
    /* Note for 32-bit machine: errors of type EVOERFLOW are 
     * not detected on some Linux versions. */
    if (file_content == MAP_FAILED) {
        dmsg("Mmap failed, using pread interface\n");
        file_content = NULL;
    }
#endif

    *fsz = file_size;
    return 0;
}

int fa_destroy()
{
    if (close(fd))
        perror("fa close");

    return 0;
}

int fa_read_init()
{

    if (!file_content)
        if (!(worker_buffer = malloc(BUFFER_SIZE)))
            return -1;
    return 0;
}

int fa_read_destroy()
{
    free(worker_buffer);
    return 0;
}

off_t fa_read(uint32_t id, char **buff, off_t size, off_t pos)
{
    off_t size_to_read, size_read, end;

    if (file_content) {
        if (pos >= file_size)   //EOF
            return 0;

        *buff = (char *)file_content + pos;

        end = pos + size;
        size_read = (end > file_size) ? (end - file_size) : size;
        return size_read;
    }

    size_to_read = BUFFER_SIZE < size ? BUFFER_SIZE : size;
    if ((size_read = pread(fd, worker_buffer, size_to_read, pos)) == -1) {
        perror("pread");
        return -1;
    }

    *buff = worker_buffer;

    return size_read;

}
