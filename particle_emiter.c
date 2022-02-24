#include "particle_emiter.h"
#include "genshd_particle.h"
#define SORT_NAME sort
#define SORT_TYPE struct particle
#define SORT_CMP(y, x)  ((x).camdist < (y).camdist ? -1 : ((y).camdist < (x).camdist ? 1 : 0))
//#define SORT_CMP(y, x) ((x).camdist - (y).camdist)
#include "sort.h"
/*
#define SORT_CMP(x, y) ((x).camdist - (y).camdist)
#define MAX(x, y) ((x).camdist > (y).camdist ? (x) : (y))
#define MIN(x, y) ((x).camdist < (y).camdist ? (x) : (y))
*/

int particle_emiter_init(struct particle_emiter *pe, hmm_vec3 pos, const char *name)
{
    struct particle_base *pb = particle_base_find(name);
    if (!pb) {
        printf("No %s sprite base found\n", name);
        return -1;
    }

    pe->base = *pb;
    pe->count = 1000;
    pe->particles = calloc(sizeof(*pe->particles), 1000);
    pe->pos = pos;
    pe->cpubuf = 0;
    pe->currentframe = pe->base.idx;
    pe->animtime = 0.0f;
    pe->lastfree = 0;
    pe->instaoffset = 0;

    return 0;
}

void particle_emiter_kill(struct particle_emiter *pe)
{
    if (pe->particles)
        free(pe->particles);
}

void particle_emiter_vertuniforms_slow(struct frameinfo *fi)
{
    vs_particle_slow_t vs = {
        //.uproject = fi->projection,
        .uvp = fi->vp,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_particle_slow, &SG_RANGE(vs));
}

inline static void sort_particles(struct particle_emiter *pe)
{
    sort_quick_sort(pe->particles, pe->count);
    float lastbig = pe->particles[0].camdist;
    for (int i = 1; i < pe->countalive; ++i) {
        if (lastbig < pe->particles[i].camdist)
            printf("[%d] = %f vs lastbig: %f\n",
                    i, pe->particles[i].camdist, lastbig);
        lastbig = pe->particles[i].camdist;
    }
}

static int find_free_particle(struct particle_emiter *pe)
{
    for (int i = pe->lastfree; i < pe->count; ++i) {
        if (pe->particles[i].life <= 0.0f) {
            pe->lastfree = i;
            return i;
        }
    }

    for (int i = 0; i < pe->count; ++i) {
        if (pe->particles[i].life <= 0.0f) {
            pe->lastfree = i;
            return i;
        }
    }

    return 0;
}

//TODO: Different starting parameters (script/Lua ?)
static void make_new_particle(struct particle_emiter *pe)
{
    const int scale = 20;

    for (int i = 0; i < 2; ++i) {
        int fp = find_free_particle(pe);
        struct particle *p = &pe->particles[fp];
        p->pos = pe->pos;
        p->pos.Z += rand() % 5;
        p->speed = HMM_Vec3(
                rand() % scale - (scale/2),
                (scale/10) + 2.0f *i,
                rand() % scale - (scale/2)
        );
        p->camdist = 0.0f;
        p->life = 3.0f;
    }
}

inline static hmm_mat4 face_camera(hmm_mat4 view)
{
    hmm_mat4 out = HMM_Mat4d(1.0f);
    out.Elements[0][0] = view.Elements[0][0];
    out.Elements[0][1] = view.Elements[1][0];
    out.Elements[0][2] = view.Elements[2][0];
    out.Elements[1][0] = view.Elements[0][1];
    out.Elements[1][1] = view.Elements[1][1];
    out.Elements[1][2] = view.Elements[2][1];
    out.Elements[2][0] = view.Elements[0][2];
    out.Elements[2][1] = view.Elements[1][2];
    out.Elements[2][2] = view.Elements[2][2];

    return out;
}

static void write_to_buffer(struct particle_emiter *pe, struct frameinfo *fi)
{
    if (!pe->cpubuf)
        return;

    const float skale = 5.0f;
    const hmm_mat4 skalemat = HMM_Scale(HMM_Vec3(skale,skale,skale));
    const hmm_mat4 faceme = face_camera(fi->view);
    //TODO: Save only alive particles and store how many of them are alive
    for (int i = 0; i < pe->countalive; ++i) {
        struct particle *p = &pe->particles[i];
        struct particle_vbuf *pv = &pe->cpubuf[i];
    
        hmm_mat4 model = faceme;
        model.Elements[3][0] = p->pos.X;
        model.Elements[3][1] = p->pos.Y;
        model.Elements[3][2] = p->pos.Z;
        //rotate
        pv->matrix = HMM_MultiplyMat4(model, skalemat);
        pv->matrix.Elements[3][3] = p->life;
        //pv->transparency = p->life;
    }
}

static void update_animation(struct particle_emiter *pe, double delta)
{
    int beg = pe->base.idx;
    int end = beg + pe->base.count;
    pe->animtime += delta;
    if (pe->animtime >= pe->base.fps) {
        pe->animtime = 0.0f;
        pe->currentframe++;
        if (pe->currentframe >= end)
            pe->currentframe = beg;
    }
}

//TODO: different effects, done via script/Lua ?
void particle_emiter_update(struct particle_emiter *pe, struct frameinfo *fi, double delta)
{
    const hmm_vec3 gravity = HMM_Vec3(0.0f, -9.81f, 0.0f);

    make_new_particle(pe);
    if (pe->base.fps)
        update_animation(pe, delta);

    pe->countalive = pe->count;
    for (int i = 0; i < pe->count; ++i) {
        struct particle *p = &pe->particles[i];
        p->life -= delta;
        if (p->life <= 0.0f) {
            p->camdist = -1.0f;
            pe->countalive--;
            continue;
        }

        p->speed = HMM_AddVec3(p->speed, HMM_MultiplyVec3f(gravity, delta));
        p->pos = HMM_AddVec3(p->pos, HMM_MultiplyVec3f(p->speed, delta));
        p->camdist = HMM_LengthVec3(HMM_SubtractVec3(p->pos, fi->cam.pos));
    }

    sort_particles(pe);
    write_to_buffer(pe, fi);
}

void particle_emiter_render(struct particle_emiter *pe)
{
    sg_bindings bind = {
        .vertex_buffers[0] = pe->base.atlas->vbuf,
        .vertex_buffers[1] = pe->gpubuf,
        .vertex_buffer_offsets[1] = pe->instaoffset,
        .fs_images[SLOT_imgdiff] = pe->base.atlas->img,
    };
    sg_apply_bindings(&bind);
    int voffset = pe->base.atlas->sprites[pe->currentframe].voffset;
    sg_draw(voffset, 4, pe->countalive);
}

