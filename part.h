#ifndef __PART_H__
#define __PART_H__

enum part_format {
    fmt_none,   // Unformatted (e.g. security sector)
    fmt_sfcx,   // Secure File Cache for Xbox
    fmt_xtaf,   // Little-endian FATX
};
struct disk;
struct part;

struct part *part_init(struct disk *disk, int part_id);
void part_free(struct part **part);

uint64_t part_length(struct part *part);
const uint8_t *part_data(struct part *part);
uint8_t part_byte(struct part *part, uint64_t offset);
enum part_format part_format(struct part *part);

#endif //__PART_H__
