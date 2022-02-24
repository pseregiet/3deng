#ifndef __particle_h
#define __particle_h

#include "atlas2d.h"
#include "hmm.h"

struct particle_base {
    struct atlas2d *atlas;
    int idx;
    int count;
    float fps;
};

struct particle {
    hmm_vec3 pos;
    hmm_vec3 speed;
    float camdist;
    float life;
    float angle;
};

int particle_base_init();
void particle_base_kill();
struct particle_base *particle_base_find(const char *name);
#endif

