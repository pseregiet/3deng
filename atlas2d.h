#ifndef __image2d_h
#define __image2d_h

#include "sokolgl.h"
#include <stdint.h>

struct rect {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int voffset;
};

struct atlas2d {
    sg_image img;
    sg_buffer vbuf;
    uint16_t width;
    uint16_t height;
    int spritecount;
    struct rect *sprites;
};

int atlas2d_mngr_init();
void atlas2d_mngr_kill();
struct atlas2d *atlas2d_mngr_find(const char *name);

#endif
