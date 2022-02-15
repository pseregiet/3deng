#ifndef __texloader_h
#define __texloader_h

#include "sokolgl.h"

int texloader_init();
void texloader_kill();
sg_image texloader_find(const char *name);

void make_sg_image_16f(float *ptr, int w, int h, sg_image *img);

#endif
