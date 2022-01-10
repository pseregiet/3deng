#ifndef __shadow_h
#define __shadow_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "hmm.h"

struct frameshadow {
    sg_shader shd;
    sg_bindings tbind;
    sg_bindings mbind;
    sg_pipeline tpip;
    sg_pipeline pip;
    sg_pass pass;
    sg_pass_action act;
    sg_image colormap;
    sg_image depthmap;
    hmm_mat4 lightspace;
};

void shadowmap_draw();
int  shadowmap_init();
void shadowmap_kill();
sg_pipeline shadow_create_pipeline(int stride, bool cullfront, bool ibuf);

#endif

