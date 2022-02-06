#ifndef __heightmap_h
#define __heightmap_h

#include "sokolgl.h"
#include "texloader.h"
#include "hmm.h"
#include <stdint.h>

#define WORLDMAP_MAXW 2
#define WORLDMAP_MAXH 2
struct worldmap {
    int w;
    int h;
    float scale;
    sg_buffer vbuffers[WORLDMAP_MAXW * WORLDMAP_MAXH];
    sg_buffer ibuffers[WORLDMAP_MAXW * WORLDMAP_MAXH];
    sg_image blendmap;

    uint16_t *hmap[WORLDMAP_MAXH][WORLDMAP_MAXW];
};

int worldmap_init(struct worldmap *map, const char *fn);
bool worldmap_isunder(struct worldmap *map, hmm_vec3 pos);

#endif
