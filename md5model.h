#ifndef __md5model_h
#define __md5model_h
#include "extrahmm.h"
#include "sokolgl.h"

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
    int woffset;
    uint16_t icount;
    uint16_t ioffset;
    int16_t wcount;
    int16_t pad1;
};

struct md5_anim {
    struct md5_joint *frames;
    struct md5_bbox *bboxes;
    int jcount;
    int fcount;
    int fps;
};

struct md5_model {
    sg_buffer bigvbuf;
    sg_buffer bigibuf;
    struct md5_mesh meshes[MD5_MAX_MESHES];
    struct md5_anim **anims;
    hmm_mat4 *invmatrices;
    sg_image weightmap;
    int16_t mcount;
    int16_t jcount;
    int16_t acount;
    int16_t weightw;
    int16_t weighth;
};

int md5model_open(const char *fn, struct md5_model *mdl);
void md5model_kill(struct md5_model *model);
int md5anim_open(const char *fn, struct md5_anim *anim);
void md5anim_kill(struct md5_anim *anim);
void md5anim_interp(const struct md5_anim *anim, struct md5_joint *out,
                    int fa, int fb, float interp);
void md5anim_plain(const struct md5_anim *anim, struct md5_joint *out, int fa);

#endif

