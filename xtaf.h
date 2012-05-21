#ifndef __XTAF_H__
#define __XTAF_H__

struct part;
struct xtaf;
struct xtaf_dir;

struct xtaf *xtaf_init(struct part *part);
struct xtaf_dir *xtaf_get_root(struct xtaf *xtaf);
uint32_t xtaf_print_root(struct xtaf *xtaf);

#endif /* __XTAF_H__ */
