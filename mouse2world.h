#ifndef __mouse2world_h
#define __mouse2world_h

#include "hmm.h"
#include "stdbool.h"
#include "static_object.h"

struct m2world {
    hmm_mat4 projection;
    hmm_mat4 view;
    hmm_vec3 cam;
    struct worldmap *map;
    struct static_object obj;
    int mx;
    int my;
    int ww;
    int wh;
};

hmm_vec3 mouse2ray(struct m2world *m2);

#endif
