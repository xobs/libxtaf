#ifndef __DISK_H__
#define __DISK_H__
#include <stdint.h>

struct disk;

struct disk *disk_init(char *filename);

/* XXX WARNING XXX NON-REENTRANT */
uint8_t *disk_mmap(struct disk *disk, uint64_t offset, uint64_t length);
void disk_munmap(struct disk *disk);

uint64_t disk_length(struct disk *disk);

#endif //__DISK_H__
