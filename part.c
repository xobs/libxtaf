#include <stdio.h>
#include <stdint.h>

struct partition {
    char *name;
    uint64_t offset;
    uint64_t length;
    uint8_t format;
};

static struct partition partitions[] = {
    {
        .name   = "Security Sector",
        .offset = 0x2000,
        .length = 0x40,
        .format = 0,
    },
    {
        .name   = "System Cache",
        .offset = 0x80000,
        .length = 0x80000000,
        .format = 1,
    },
    {
        .name   = "Game Cache",
        .offset = 0x80080000,
        .length = 0xA0E30000,
        .format = 1,
    },
    {
        .name   = "SysExt",
        .offset = 0x10C080000,
        .length = 0xCE30000,
        .format = 2,
    },
    {
        .name   = "SysExt2",
        .offset = 0x118EB0000,
        .length = 0xCE30000,
        .format = 2,
    },
    {
        .name   = "Xbox 1",
        .offset = 0x120eb0000,
        .length = 0x10000000,
        .format = 2,
    },
    {
        .name   = "Data",
        .offset = 0x130eb0000,
        .length = 0,
        .format = 2,
    },
};

int partition_open(int fd) {
    printf("%s\n", partitions[fd].name);
    return 0;
}
