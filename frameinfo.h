#ifndef __frameinfo_h
#define __frameinfo_h

#include "sokolgl.h"
#include "heightmap.h"
#include "pipelines.h"
#include "camera.h"
#include <stdbool.h>

#define DEFAULT_POINTLIGHT_COUNT 4

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

struct shadowmap {
    hmm_mat4 lightspace_dir;
    hmm_mat4 lightspace_spot;
    hmm_mat4 lightspace_point[DEFAULT_POINTLIGHT_COUNT];

    sg_image colormap;
    sg_image colormap_cube;
    sg_image depthmap_dir;
    sg_image depthmap_spot;
    sg_image depthmap_point[DEFAULT_POINTLIGHT_COUNT];
    sg_pass  pass_dir;
    sg_pass  pass_spot;
    sg_pass  pass_point[6 * DEFAULT_POINTLIGHT_COUNT];
};

struct frameinfo {
    struct worldmap map;
    struct pipelines pipes;
    struct camera cam;
    struct shadowmap shadowmap;
    struct pointlight pointlight[DEFAULT_POINTLIGHT_COUNT];
    struct dirlight dirlight;
    struct spotlight spotlight;

    hmm_mat4 projection;
    hmm_mat4 view;
    hmm_mat4 vp;

    int shadowmap_resolution;

    sg_bindings lightbind;
    sg_bindings terrainbind[4];

    sg_pass_action pass_action;

    bool lightsenable[32];
    enum videodriver vd;
    bool draw_bbox;
};

#endif
