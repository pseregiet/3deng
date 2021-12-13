#ifndef __heightmap_h
#define __heightmap_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "texloader.h"

#define WORLDMAP_MAXW 2
#define WORLDMAP_MAXH 2
struct worldmap {
    int w;
    int h;
    sg_buffer vbuffers[WORLDMAP_MAXW * WORLDMAP_MAXH];
    sg_buffer ibuffers[WORLDMAP_MAXW * WORLDMAP_MAXH];
    sg_image blendmap;
};

int worldmap_init(struct worldmap *map, const char *fn);

#endif
