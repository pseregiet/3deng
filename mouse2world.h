#ifndef __mouse2world_h
#define __mouse2world_h

#include "hmm.h"
#include "stdbool.h"
#include "staticmapobj.h"

struct m2world {
    hmm_mat4 projection;
    hmm_mat4 view;
    hmm_vec3 cam;
    struct worldmap *map;
    struct staticmapobj obj;
    int mx;
    int my;
    int ww;
    int wh;
};

hmm_vec3 mouse2ray(struct m2world *m2);

#endif
