#ifndef __particle_emiter_h
#define __particle_emiter_h

#include "particle.h"
#include "frameinfo.h"
#include "hmm.h"
#include "sokolgl.h"

struct particle_vbuf {
    hmm_mat4 matrix;
};

struct particle_emiter {
    struct particle *particles;
    struct particle_vbuf *cpubuf;
    struct particle_base base;
    hmm_vec3 pos;
    float animtime;
    int currentframe;
    int count;
    int countalive;
    int instaoffset;
    int lastfree;
    sg_buffer gpubuf;
};


int particle_emiter_init(struct particle_emiter *pe, hmm_vec3 pos, const char *name);
void particle_emiter_kill(struct particle_emiter *pe);
void particle_emiter_vertuniforms_slow(struct frameinfo *fi);
void particle_emiter_update(struct particle_emiter *pe,
        struct frameinfo *fi, double delta);
void particle_emiter_render(struct particle_emiter *pe);
#endif

