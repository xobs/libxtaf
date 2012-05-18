#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "part.h"
#include "xtaf.h"
struct xtaf {
    const uint8_t *data;
    const uint8_t *chainmap;
    const uint8_t *entries;
    uint64_t length;

    uint8_t magic[4];
    uint32_t id;
    uint32_t spc; // Sectors per cluster
    uint32_t rdc; // Root directory cluster
    uint32_t cluster_count;
    uint32_t cluster_size;
    uint32_t entry_size;
    uint32_t chainmap_size;
};

struct xtaf *xtaf_open(struct part *part) {
    struct xtaf *xtaf;

    xtaf = malloc(sizeof(struct xtaf));
    if (!xtaf) {
        perror("Couldn't allocate XTAF");
        return NULL;
    }

    xtaf->data = part_data(part);
    xtaf->length = part_length(part);

    memcpy(xtaf->magic, xtaf->data, sizeof(xtaf->magic));
    if (memcmp(xtaf->magic, "XTAF", sizeof(xtaf->magic))) {
        fprintf(stderr, "Magic doesn't match XTAF!\n");
        free(xtaf);
        return NULL;
    }

    memcpy(&(xtaf->id), xtaf->data+4, 4);
    memcpy(&(xtaf->spc), xtaf->data+8, 4);
    memcpy(&(xtaf->rdc), xtaf->data+12, 4);
    xtaf->id = ntohl(xtaf->id);
    xtaf->spc = ntohl(xtaf->spc);
    xtaf->rdc = ntohl(xtaf->rdc);

    xtaf->cluster_size = xtaf->spc * 512;
    xtaf->cluster_count = xtaf->length / xtaf->cluster_size;
    xtaf->entry_size = (xtaf->cluster_count>=0xfff0?4:2);
    xtaf->chainmap_size = xtaf->cluster_count*xtaf->entry_size;

    printf("Sectors per cluster: %d\n", xtaf->spc);
    printf("Root directory cluster: %d\n", xtaf->rdc);
    printf("Cluster size: %d\n", xtaf->cluster_size);
    printf("Number of clusters: %d\n", xtaf->cluster_count);
    printf("Chainmap entries are %d bytes\n", xtaf->entry_size);
    printf("Chainmap size: %d bytes\n", xtaf->chainmap_size);

    xtaf->chainmap = xtaf->data + 0x1000;
    xtaf->entries = xtaf->chainmap + xtaf->chainmap_size;

    int i;
    printf("First few entries:\n");
    for (i=0; i<16384; i++) {
        printf("%c", xtaf->entries[i]);
    }
    printf("\n");

    return xtaf;
}
