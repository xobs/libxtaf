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
    struct xtaf_dir *dir;
    char input_str[256];
    int part_num;

    if (argc != 2) {
        printf("Usage: %s [image-file]\n", argv[0]);
        return 1;
    }

    disk = disk_init(argv[1]);
    if (!disk)
        return 1;

    printf("Select a partition:\n");
    for (part_num=0; part_num<part_disk_count(disk); part_num++)
        printf("    %d. %s\n", part_num+1, part_disk_name(disk, part_num));
    fgets(input_str, sizeof(input_str)-1, stdin);
    part_num = strtoul(input_str, NULL, 0)-1;


    part = part_init(disk, part_num);
    if (!part) {
        perror("Unable to open partition");
        return 1;
    }

    xtaf = xtaf_init(part);
    if (!xtaf) {
        perror("Unable to open XTAF partition");
        return 1;
    }

    dir = xtaf_get_root(xtaf);
    xtaf_print_dir(dir);
    while (fgets(input_str, sizeof(input_str)-1, stdin)) {

    }

    return 0;
}
