#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "xtaf.h"

int main (int argc, char **argv) {
    struct xtaf *xtaf;

    if (argc != 2) {
        printf("Usage: %s [image-file]\n", argv[0]);
        return 1;
    }

    xtaf = xtaf_init(argv[1]);
    if (!xtaf)
        return 1;

    return 0;
}
