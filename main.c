#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "disk.h"
#include "part.h"
#include "xtaf.h"

int main (int argc, char **argv) {
    struct disk *disk;
    struct part *part;
    struct xtaf *xtaf;

    if (argc != 2) {
        printf("Usage: %s [image-file]\n", argv[0]);
        return 1;
    }

    disk = disk_init(argv[1]);
    if (!disk)
        return 1;


    part = part_open(disk, 6);
    if (!part) {
        perror("Unable to open partition");
        return 1;
    }

    xtaf = xtaf_open(part);
    if (!xtaf) {
        perror("Unable to open XTAF partition");
        return 1;
    }

    return 0;
}
