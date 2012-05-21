#ifndef __PART_H__
#define __PART_H__

enum part_format {
    fmt_none,   // Unformatted (e.g. security sector)
    fmt_sfcx,   // Secure File Cache for Xbox
    fmt_xtaf,   // Little-endian FATX
};
struct disk;
struct part;


/* Returns the number of partitions available */
uint32_t part_disk_count(struct disk *disk);

/* Returns the name of the specified partition */
char *part_disk_name(struct disk *disk, uint8_t part_id);

/* Returns the format of the specified partition */
enum part_format part_disk_format(struct disk *disk, uint8_t part_id);




struct part *part_init(struct disk *disk, uint8_t part_id);
void part_free(struct part **part);

/* Returns the length of the selected partition */
uint64_t part_length(struct part *part);

/* Returns a pointer to the partition's data */
const uint8_t *part_data(struct part *part);

/* Returns a byte from the specified offset */
uint8_t part_byte(struct part *part, uint64_t offset);

/* Returns the format of the selected partition */
enum part_format part_format(struct part *part);

#endif //__PART_H__
