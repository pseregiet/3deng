#ifndef __md5model_h
#define __md5model_h
#include "extrahmm.h"
#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

#define MD5_MAX_MESHES 4

struct md5_joint {
    hmm_vec3 pos;
    float bias;
    hmm_quat orient;
    int parent;
};

struct md5_basejoint {
    char name[64];
    struct md5_joint joint;
};

struct md5_weight {
    hmm_vec3 pos;
    float bias;
    int joint;
};

struct md5_bbox {
    hmm_vec3 min;
    hmm_vec3 max;
};

struct md5_mesh {
    sg_buffer vbuf;
    sg_buffer ibuf;
    sg_image  imgd;
    sg_image  imgs;
    sg_image  imgn;
    int vcount;
    int icount;
    int wcount;
    int woffset;
};

struct md5_anim {
    struct md5_joint *frames;
    struct md5_bbox *bboxes;
    int jcount;
    int fcount;
    int fps;
};

struct md5_model {
    struct md5_mesh meshes[MD5_MAX_MESHES];
    struct md5_anim **anims;
    hmm_mat4 *invmatrices;
    sg_image weightmap;
    int mcount;
    int jcount;
    int acount;
    int weightw;
    int weighth;
};

int md5model_open(const char *fn, struct md5_model *mdl);
int md5anim_open(const char *fn, struct md5_anim *anim);
void md5anim_kill(struct md5_anim *anim);
void md5anim_interp(const struct md5_anim *anim, struct md5_joint *out,
                    int fa, int fb, float interp);

#endif

