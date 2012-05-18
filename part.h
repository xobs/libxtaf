#ifndef __PART_H__
#define __PART_H__

struct disk;
struct part;

struct part *part_open(struct disk *disk, int part_id);

uint64_t part_length(struct part *part);
uint8_t part_byte(struct part *part, uint64_t offset);

#endif //__PART_H__
