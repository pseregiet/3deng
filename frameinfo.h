#ifndef __frameinfo_h
#define __frameinfo_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "heightmap.h"
#include "pipelines.h"
#include "camera.h"
#include <stdbool.h>

struct frameinfo {
    struct worldmap map;
    struct pipelines pipes;
    struct camera cam;
    struct shadow {
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
    } shadow;
    sg_bindings lightbind;
    sg_bindings terrainbind[4];

    sg_pass_action pass_action;

    bool lightsenable[32];
    hmm_vec3 dlight_dir;

    hmm_vec3 dlight_diff;
    hmm_vec3 dlight_ambi;
    hmm_vec3 dlight_spec;
    hmm_mat4 vp;
};

#endif
