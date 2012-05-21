#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "part.h"
#include "xtaf.h"


struct xtaf_record;



struct xtaf {
    const uint8_t *data;
    const uint8_t *chainmap;
    const uint8_t *clusters;
    uint64_t length;

    uint8_t  magic[4];
    uint32_t id;
    uint32_t spc; // Sectors per cluster
    uint32_t rdc; // Root directory cluster
    uint32_t cluster_count;
    uint32_t cluster_size;
    uint32_t entry_size;
    uint32_t chainmap_size;
};

struct xtaf_dir {
    struct xtaf *xtaf;
    struct xtaf_dir *parent;
    struct xtaf_record *rec;
    uint32_t cluster;   /* Starting cluster */
};

struct xtaf_record {
    uint8_t  name_len;
    uint8_t  file_flags;
    uint8_t  filename[0x2a];
    uint32_t start_cluster;
    uint32_t file_size;
    uint16_t creation_date;
    uint16_t creation_time;
    uint16_t access_date;
    uint16_t access_time;
    uint16_t update_date;
    uint16_t update_time;
} __attribute__ ((__packed__));




static inline const uint8_t *xtaf_cluster(struct xtaf *xtaf, uint32_t cluster) {
    return xtaf->clusters+(xtaf->cluster_size*(cluster-1));
}

static char *xtaf_time_str(uint16_t t) {
    static char str[64];
    t = htons(t);
    uint8_t hours = (t>>11)&0x1f;
    uint8_t minutes = (t>>5)&0x3f;
    uint8_t seconds = 2*((t>>0)&0x1f);
    snprintf(str, sizeof(str)-1, "%u:%02u:%02u", hours, minutes, seconds);
    return str;
}


static char *xtaf_date_str(uint16_t t) {
    static char str[64];
    t = htons(t);
    uint32_t year = ((t>>9)&0x7f)+1980;
    uint8_t month = (t>>5)&0x0f;
    uint8_t day = (t>>0)&0x1f;
    snprintf(str, sizeof(str)-1, "%u/%u/%u", day, month, year);
    return str;
}


struct xtaf *xtaf_init(struct part *part) {
    struct xtaf *xtaf;

    if (part_format(part) != fmt_xtaf) {
        fprintf(stderr, "Partition isn't formatted XTAF\n");
        errno = EINVAL;
        return NULL;
    }

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
    xtaf->cluster_count = (xtaf->length) / xtaf->cluster_size;
    xtaf->entry_size = (xtaf->cluster_count>=0xfff0?4:2);
    xtaf->chainmap_size = (xtaf->cluster_count*xtaf->entry_size);
    xtaf->chainmap_size =  (xtaf->chainmap_size / 4096 + 1) * 4096;

    fprintf(stderr, "Partition size: %lld\n", xtaf->length);
    fprintf(stderr, "Sectors per cluster: %d\n", xtaf->spc);
    fprintf(stderr, "Root directory cluster: %d\n", xtaf->rdc);
    fprintf(stderr, "Cluster size: %d\n", xtaf->cluster_size);
    fprintf(stderr, "Number of clusters: %d (0x%x)\n", xtaf->cluster_count,
            xtaf->cluster_count);
    fprintf(stderr, "Chainmap entries are %d bytes\n", xtaf->entry_size);
    fprintf(stderr, "Chainmap size: %d bytes\n", xtaf->chainmap_size);

    xtaf->chainmap = xtaf->data + 0x1000;
    xtaf->clusters = xtaf->chainmap + xtaf->chainmap_size;

    return xtaf;
}



struct xtaf_dir *xtaf_dir_get(struct xtaf *xtaf, uint32_t cluster) {
    struct xtaf_dir *dir;

    dir = malloc(sizeof(struct xtaf_dir));
    if (!dir)
        return NULL;


    dir->rec = (struct xtaf_record *)xtaf_cluster(xtaf, cluster);
    dir->parent = NULL;
    dir->cluster = xtaf->rdc;
    dir->xtaf = xtaf;

    return dir;
}


struct xtaf_dir *xtaf_get_root(struct xtaf *xtaf) {
    return xtaf_dir_get(xtaf, xtaf->rdc);
}


void xtaf_dir_free(struct xtaf_dir **xtaf_dir) {
    free(*xtaf_dir);
    *xtaf_dir = NULL;
    return;
}



uint32_t xtaf_print_root(struct xtaf *xtaf) {
    struct xtaf_dir *dir;
    int i;
    struct xtaf_record *rec;
    dir = xtaf_get_root(xtaf);
    fprintf(stderr, "Root directory:\n");
    fprintf(stderr, "First few entries:  \n");
    rec = dir->rec;
    for (i=0; i<64; i++) {
        uint8_t filename[0x2b];

        if (rec->access_date == 0xffff
         && rec->access_time == 0xffff
         && rec->update_date == 0xffff
         && rec->update_time == 0xffff
         && rec->creation_date == 0xffff
         && rec->creation_time == 0xffff
         && rec->file_size == -1
         && rec->start_cluster == -1
         && rec->file_flags == 0xff
         && rec->name_len == 0xff
         )
            break;

        bzero(filename, sizeof(filename));
        memcpy(filename, rec->filename, sizeof(rec->filename));
        int j;
        for(j=0; j<sizeof(filename); j++)
            if (filename[j] == 0xff)
                filename[j] = 0x00;
        fprintf(stderr, "New file\n");
        fprintf(stderr, "    Name length: 0x%02x\n", rec->name_len);
        fprintf(stderr, "    Flags: 0x%02x\n", rec->file_flags);
        fprintf(stderr, "    Filename: %s\n", filename);
        fprintf(stderr, "    Start cluster: %d\n", htonl(rec->start_cluster));
        fprintf(stderr, "    File size: %d\n", htonl(rec->file_size));
        fprintf(stderr, "    Creation: %s %s\n", xtaf_date_str(rec->creation_date),
                xtaf_time_str(rec->creation_time));
        fprintf(stderr, "    Access: %s %s\n", xtaf_date_str(rec->access_date),
                xtaf_time_str(rec->access_time));
        fprintf(stderr, "    Update: %s %s\n", xtaf_date_str(rec->update_date),
                xtaf_time_str(rec->update_time));
        rec++;
    }
    printf("\n");

    xtaf_dir_free(&dir);

    return 0;
}
