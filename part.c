#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "part.h"
#include "disk.h"

struct partition_def {
    char             *name;
    uint64_t          offset;
    uint64_t          length;
    enum part_format  format;
};


struct part {
    uint32_t             id;
    struct partition_def *def;
    uint8_t              *data;
    struct disk          *disk;
};


/* Hardcoded into the Xbox 360 Phat's ROM */
static struct partition_def partitions[] = {
    {
        .name   = "Security Sector",
        .offset = 0x2000,
        .length = 0x40,
        .format = fmt_none,
    },
    {
        .name   = "System Cache",
        .offset = 0x80000,
        .length = 0x80000000,
        .format = fmt_sfcx,
    },
    {
        .name   = "Game Cache",
        .offset = 0x80080000,
        .length = 0xA0E30000,
        .format = fmt_sfcx,
    },
    {
        .name   = "SysExt",
        .offset = 0x10C080000,
        .length = 0xCE30000,
        .format = fmt_xtaf,
    },
    {
        .name   = "SysExt2",
        .offset = 0x118EB0000,
        .length = 0xCE30000,
        .format = fmt_xtaf,
    },
    {
        .name   = "Xbox 1",
        .offset = 0x120eb0000,
        .length = 0x10000000,
        .format = fmt_xtaf,
    },
    {
        .name   = "Data",
        .offset = 0x130eb0000,
        .length = 0,
        .format = fmt_xtaf,
    },
};


struct part *part_init(struct disk *disk, int part_id) {
    struct part *part;

    part = malloc(sizeof(struct part));
    if (!part)
        return NULL;

    part->id = part_id;
    if (part->id > (sizeof(partitions)/sizeof(*partitions))) {
        fprintf(stderr, "Partition %d out of range!\n", part_id);
        free(part);
        return NULL;
    }

    part->def = &partitions[part->id];
    part->disk = disk;

    fprintf(stderr, "Opening partition %s offset 0x%llx...\n",
            part->def->name, part->def->offset);
    if (!part->def->length)
        part->def->length = disk_length(part->disk) - part->def->offset;

    part->data = disk_mmap(part->disk, part->def->offset, part->def->length);

    if (!part->data) {
        fprintf(stderr, "Couldn't mmap partition: %s\n", strerror(errno));
        free(part);
        return NULL;
    }
    return part;
}

void part_free(struct part **part) {
    if (!*part)
        return;
    disk_munmap((*part)->disk);

    free(*part);
    *part = NULL;
    return;
}

uint64_t part_length(struct part *part) {
    return part->def->length;
}

uint8_t part_byte(struct part *part, uint64_t offset) {
    return part->data[offset];
}

const uint8_t *part_data(struct part *part) {
    return part->data;
}

enum part_format part_format(struct part *part) {
    return part->def->format;
}
