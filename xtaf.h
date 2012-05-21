#ifndef __XTAF_H__
#define __XTAF_H__

struct xtaf;
struct part;

struct xtaf *xtaf_init(struct part *part);
uint32_t print_root(struct xtaf *xtaf);

#endif /* __XTAF_H__ */
