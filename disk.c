#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "disk.h"

struct disk {
    char *filename;
    int fd;
};

struct disk *disk_init(char *filename) {
    struct disk *disk;

    disk = malloc(sizeof(struct disk));
    if (!disk)
        return NULL;

    disk->filename = strdup(filename);
    disk->fd = open(disk->filename, O_RDONLY);
    if (-1 == disk->fd) {
        free(disk);
        return NULL;
    }


    return disk;
}

uint64_t disk_length(struct disk *disk) {
    return lseek(disk->fd, 0, SEEK_END);
}

uint8_t *disk_mmap(struct disk *disk, uint64_t offset, uint64_t length) {
    void *result;

    /* Allow us to have a "rest-of-the-disk" length */
    if (!length) {
        length = disk_length(disk) - offset;
    }

    result = mmap(NULL, length, PROT_READ, MAP_SHARED, disk->fd, offset);
    if (result == MAP_FAILED)
        return 0;
    return result;
}
