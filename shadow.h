#ifndef __shadow_h
#define __shadow_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "hmm.h"

struct frameshadow {
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

void shadow_draw();
void init_shadow();
void calc_lightmatrix();

#endif

