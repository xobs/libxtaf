#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "xtaf.h"

struct xtaf {
    char *filename;
    int fd;
};

struct xtaf *xtaf_init(char *filename) {
    struct xtaf *xtaf;

    xtaf = malloc(sizeof(struct xtaf));
    if (!xtaf)
        return NULL;

    xtaf->filename = strdup(filename);
    xtaf->fd = open(xtaf->filename, O_RDONLY);
    if (-1 == xtaf->fd) {
        free(xtaf);
        return NULL;
    }


    return xtaf;
}
