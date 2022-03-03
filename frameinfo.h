#ifndef __frameinfo_h
#define __frameinfo_h

#include "sokolgl.h"
#include "heightmap.h"
#include "pipelines.h"
#include "camera.h"
#include <stdbool.h>

enum videodriver {
    VD_NONE,
    VD_X11,
    VD_WAYLAND,
    VD_WINDOWS,
    VD_APPLE,
};


struct pointlight {
    hmm_vec3 pos;
    hmm_vec3 ambi;
    hmm_vec3 diff;
    hmm_vec3 spec;
    hmm_vec3 atte;
};
struct dirlight {
    hmm_vec3 dir;
    hmm_vec3 ambi;
    hmm_vec3 diff;
    hmm_vec3 spec;
};
struct spotlight {
    hmm_vec3 pos;
    hmm_vec3 dir;
    hmm_vec3 ambi;
    hmm_vec3 diff;
    hmm_vec3 spec;
    hmm_vec3 atte;
    float cutoff;
    float outcutoff;
};

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
    struct pointlight pointlight[4];
    struct dirlight dirlight;
    struct spotlight spotlight;

    hmm_mat4 projection;
    hmm_mat4 view;
    hmm_mat4 vp;

    sg_bindings lightbind;
    sg_bindings terrainbind[4];

    sg_pass_action pass_action;

    bool lightsenable[32];
    enum videodriver vd;
    bool draw_bbox;
};

#endif
