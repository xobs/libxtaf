#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "part.h"
#include "xtaf.h"


struct xtaf_dir_entry;
struct xtaf_dir;


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

    uint8_t *page_cache; // Temporary cache for reading clusters.
    struct xtaf_dir *current_dir;
};

struct xtaf_dir {
    struct xtaf *xtaf;
    struct xtaf_dir_entry *entries;
    uint32_t entry_count;
    uint32_t cluster;   /* Starting cluster */
    uint32_t parent;
};

struct xtaf_dir_entry {
    uint8_t  name_len;
    uint8_t  file_flags;
    uint8_t  filename[0x2a];
    uint32_t start_cluster;
    uint32_t file_size;
    uint16_t create_date;
    uint16_t create_time;
    uint16_t access_date;
    uint16_t access_time;
    uint16_t update_date;
    uint16_t update_time;
} __attribute__ ((__packed__));




static inline const uint8_t *xtaf_cluster(struct xtaf *xtaf, uint32_t cluster) {
    if (xtaf->entry_size == 2)
        return xtaf->clusters+ntohs(xtaf->entry_size*(cluster-1));
    else if (xtaf->entry_size == 4)
        return xtaf->clusters+ntohl(xtaf->entry_size*(cluster-1));
    fprintf(stderr, "Severe error: Unknown entry size %d\n", xtaf->entry_size);
    return 0;
}

static inline uint32_t xtaf_next_cluster_num(struct xtaf *xtaf, uint32_t cluster) {
    if (xtaf->entry_size == 2) {
        uint16_t *chainmap = (uint16_t *)xtaf->chainmap;
        return (uint32_t)chainmap[cluster];
    }
    else if (xtaf->entry_size == 4) {
        uint32_t *chainmap = (uint32_t *)xtaf->chainmap;
        return (uint32_t)chainmap[cluster];
    }
    fprintf(stderr, "Severe error: Unknown entry size %d\n", xtaf->entry_size);
    return 0xffffffff;
}

static char *xtaf_time_str(uint16_t t) {
    static char str[64];
    uint8_t hours = (t>>11)&0x1f;
    uint8_t minutes = (t>>5)&0x3f;
    uint8_t seconds = 2*((t>>0)&0x1f);
    snprintf(str, sizeof(str)-1, "%u:%02u:%02u", hours, minutes, seconds);
    return str;
}


static char *xtaf_date_str(uint16_t t) {
    static char str[64];
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
    xtaf->current_dir = NULL;

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

void xtaf_free(struct xtaf **xtaf) {
    free((*xtaf)->page_cache);
    free(*xtaf);
    *xtaf = NULL;
}




struct xtaf_dir *xtaf_dir_get(struct xtaf *xtaf, uint32_t cluster) {
    struct xtaf_dir *dir = xtaf->current_dir;
    const uint8_t *current_cluster;
    uint32_t start_cluster = cluster;

    if (!dir) {
        xtaf->current_dir = malloc(sizeof(struct xtaf_dir));
        if (!xtaf->current_dir)
            return NULL;
        dir = xtaf->current_dir;
        dir->entries = NULL;
    }

    /* Count up the number of directory entries */
    dir->entry_count = 0;
    dir->parent = 0;
    dir->cluster = start_cluster;
    dir->xtaf = xtaf;
    free(dir->entries);

    cluster = start_cluster;
    do {
        const struct xtaf_dir_entry *rec;
        current_cluster = xtaf_cluster(xtaf, cluster);
        rec = (const struct xtaf_dir_entry *)current_cluster;

        while ((uint8_t *)rec-(uint8_t *)current_cluster < xtaf->cluster_size) {
            if (rec->access_date == 0xffff
            && rec->access_time == 0xffff
            && rec->update_date == 0xffff
            && rec->update_time == 0xffff
            && rec->create_date == 0xffff
            && rec->create_time == 0xffff
            && rec->file_size == -1
            && rec->start_cluster == -1
            && rec->file_flags == 0xff
            && rec->name_len == 0xff
            )
                break;
            dir->entry_count++;
            rec++;
        }
        cluster = xtaf_next_cluster_num(xtaf, cluster);
    } while (cluster && cluster != 0xffffffff);

    free(dir->entries);
    dir->entries = malloc(dir->entry_count * sizeof(struct xtaf_dir_entry));

    /* Now read all entries */
    cluster = start_cluster;
    uint32_t entry = 0;
    do {
        const struct xtaf_dir_entry *rec;
        current_cluster = xtaf_cluster(xtaf, cluster);
        rec = (const struct xtaf_dir_entry *)current_cluster;

        while ((uint8_t *)rec-(uint8_t *)current_cluster < xtaf->cluster_size) {
            if (rec->access_date == 0xffff
            && rec->access_time == 0xffff
            && rec->update_date == 0xffff
            && rec->update_time == 0xffff
            && rec->create_date == 0xffff
            && rec->create_time == 0xffff
            && rec->file_size == -1
            && rec->start_cluster == -1
            && rec->file_flags == 0xff
            && rec->name_len == 0xff
            )
                break;

            dir->entries[entry] = *rec;
            dir->entries[entry].access_date = ntohs(dir->entries[entry].access_date);
            dir->entries[entry].access_time = ntohs(dir->entries[entry].access_time);
            dir->entries[entry].update_date = ntohs(dir->entries[entry].update_date);
            dir->entries[entry].update_time = ntohs(dir->entries[entry].update_time);
            dir->entries[entry].create_date = ntohs(dir->entries[entry].create_date);
            dir->entries[entry].create_time = ntohs(dir->entries[entry].create_time);
            dir->entries[entry].file_size = ntohl(dir->entries[entry].file_size);
            dir->entries[entry].start_cluster = ntohl(dir->entries[entry].start_cluster);
            int j;
            /* Replace 0xff with 0x00, which shows up sometimes in filenames */
            for(j=0; j<sizeof(dir->entries[entry].filename); j++)
                if (dir->entries[entry].filename[j] == 0xff)
                    dir->entries[entry].filename[j] = 0x00;
            entry++;
            rec++;
        }
        cluster = xtaf_next_cluster_num(xtaf, cluster);
    } while (cluster && cluster != 0xffffffff);

    return dir;
}


struct xtaf_dir *xtaf_get_root(struct xtaf *xtaf) {
    return xtaf_dir_get(xtaf, xtaf->rdc);
}


void xtaf_dir_free(struct xtaf_dir **xtaf_dir) {
    (*xtaf_dir)->xtaf->current_dir = NULL;
    free(*xtaf_dir);
    *xtaf_dir = NULL;
    return;
}


uint32_t xtaf_print_dir(struct xtaf_dir *dir) {
    int i;
    printf(" -1.      [Parent Directory]\n");
    for (i=0; i<dir->entry_count; i++) {
        struct xtaf_dir_entry *rec = &dir->entries[i];
        printf("%3d. %02x %10d  0x%02x %10d %10s %8s  %10s %8s  %10s %8s %s\n",
                i,
                rec->name_len,
                rec->file_size,
                rec->file_flags, rec->start_cluster,
                xtaf_date_str(rec->create_date),
                xtaf_time_str(rec->create_time),
                xtaf_date_str(rec->access_date),
                xtaf_time_str(rec->access_time),
                xtaf_date_str(rec->update_date),
                xtaf_time_str(rec->update_time),
                rec->filename);
    }
    return 0;
}

uint32_t xtaf_print_root(struct xtaf *xtaf) {
    struct xtaf_dir *dir;

    dir = xtaf_get_root(xtaf);
    fprintf(stderr, "Root directory:\n");
    return xtaf_print_dir(dir);

    return 0;
}
