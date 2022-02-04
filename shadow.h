#ifndef __shadow_h
#define __shadow_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "hmm.h"

void shadowmap_draw();
int  shadowmap_init();
void shadowmap_kill();
//sg_pipeline shadow_create_pipeline(int stride, bool cullfront, bool ibuf);

#endif

