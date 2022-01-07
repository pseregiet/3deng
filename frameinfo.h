#ifndef __frameinfo_h
#define __frameinfo_h

#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "objloader.h"
#include "heightmap.h"
#include <stdbool.h>

struct frameinfo {
    sg_pipeline mainpip;
    sg_bindings mainbind;

    sg_pipeline lightpip;
    sg_bindings lightbind;

    sg_pipeline terrainpip;
    sg_bindings terrainbind[4];

    sg_pass_action pass_action;
    struct worldmap map;

    bool lightsenable[32];
};

#endif
