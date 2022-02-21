#ifndef __material_h
#define __material_h

#include "sokolgl.h"

enum mattype {
    MAT_DIFF = 0,
    MAT_SPEC = 1,
    MAT_NORM = 2,
    MAT_HGHT = 3,

    MAT_COUNT,
};

enum matflags {
    MAT_ARRAY         = (1 << 0),
    MAT_MIPS          = (1 << 1),
    MAT_MIN_FILTER    = (1 << 2),
    MAT_MAG_FILTER    = (1 << 3),
    MAT_HAS_DIFF      = (1 << 4),
    MAT_HAS_SPEC      = (1 << 5),
    MAT_HAS_NORM      = (1 << 6),
    MAT_HAS_HGHT      = (1 << 7),
};

union matshine {
    float *array;
    float  value;
};

struct material {
    union matshine shine;
    sg_image imgs[MAT_COUNT];
    uint16_t count;
    uint16_t flags;
};

void material_init_dummy(struct material *mat);
int material_mngr_init();
void material_mngr_kill();
struct material *material_mngr_find(const char *name);

#endif
