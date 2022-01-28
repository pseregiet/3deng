#include "animodel.h"
#include "genshd_md5.h"
#include <string.h>

static int lmao = 0;
int animodel_init(struct animodel *am, const char *modelname)
{
    struct md5_model *mdl = md5loader_find(modelname);
    if (!mdl)
        return -1;

    am->model = mdl;
    am->interp = (struct md5_joint *)calloc(mdl->jcount, sizeof(*am->interp));
    am->curranim = 0;
    am->currframe = lmao++;
    am->nextframe = 0;
    am->pause = false;
    am->lasttime = 0.0f;
    am->bonebuf = 0;
}

void animodel_kill(struct animodel *am)
{
    if (am->interp)
        free(am->interp);
}

void animodel_play(struct animodel *am, double delta)
{
    if (am->curranim == -1 || am->pause == true)
        return;

    const struct md5_anim *curranim = am->model->anims[am->curranim];
    const int maxframes = curranim->fcount;
    am->lasttime += delta;

    const float maxtime = 1.0f / curranim->fps;
    if (am->lasttime >= maxtime) {
        am->currframe++;
        am->nextframe++;
        am->lasttime -= maxtime;

        if (am->currframe >= maxframes)
            am->currframe = 0;
        if (am->nextframe >= maxframes)
            am->nextframe = 0;
    }
}

void animodel_interpolate(struct animodel *am)
{
    struct md5_anim *anim = am->model->anims[am->curranim];
    float interptime = am->lasttime * anim->fps;
    md5anim_interp(anim, am->interp, am->currframe, am->nextframe, interptime);
}

void animodel_plain(struct animodel *am)
{
    struct md5_anim *anim = am->model->anims[am->curranim];
    md5anim_plain(anim, am->interp, am->currframe);
}

void animodel_joint2matrix(struct animodel *am)
{
    if (!am->bonebuf)
        return;

    float *outptr = am->bonebuf;
    for (int i = 0; i < am->model->jcount; ++i) {
        struct md5_joint *j = &am->interp[i];
        hmm_mat4 m = HMM_QuaternionToMat4(j->orient);
        m.Elements[3][0] = j->pos.X;
        m.Elements[3][1] = j->pos.Y;
        m.Elements[3][2] = j->pos.Z;
        m = HMM_MultiplyMat4(m, am->model->invmatrices[i]);

        memcpy(outptr, &m, sizeof(hmm_mat4));
        outptr = &outptr[16];
    }

    am->bonebuf = 0;
}

void animodel_render(struct animodel *am, struct frameinfo *fi, hmm_mat4 model)
{
    struct md5_model *mdl = am->model;
    for (int i = 0; i < mdl->mcount; ++i) {
        struct md5_mesh *mesh = &mdl->meshes[i];

        sg_bindings bind = {
            .vertex_buffers[0] = mesh->vbuf,
            .index_buffer = mesh->ibuf,
            .vs_images[SLOT_weightmap] = mdl->weightmap,
            .vs_images[SLOT_bonemap] = am->bonemap,
            .fs_images[SLOT_imgdiff] = mesh->imgd,
            .fs_images[SLOT_imgspec] = mesh->imgs,
            .fs_images[SLOT_imgnorm] = mesh->imgn,
        };
        sg_apply_bindings(&bind);

        vs_md5_t univs = {
            .umodel = model,
            .uvp = fi->vp,
            .uboneuv = {
                am->boneuv[0],
                am->boneuv[1],
                am->boneuv[2],
                am->boneuv[3],
            },
            .uweightuv = {
                mesh->woffset,
                0,
                mdl->weightw,
                mdl->weighth,
            },
            .ulightpos = fi->dlight_dir,
            .uviewpos = fi->cam.pos,
        };
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_md5, &SG_RANGE(univs));

        fs_md5_t unifs = {
            .uambi = fi->dlight_ambi,
            .udiff = fi->dlight_diff,
            .uspec = fi->dlight_spec,
        };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_vs_md5, &SG_RANGE(unifs));

        sg_draw(0, mesh->icount*3, 1);
    }
}

void animodel_pipeline(struct pipelines *pipes)
{
    pipes->animodel_shd = sg_make_shader(shdmd5_shader_desc(SG_BACKEND_GLCORE33));

    pipes->animodel = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->animodel_shd,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGB,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [ATTR_vs_md5_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_md5_anorm] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_md5_atang] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_md5_auv] = {.format = SG_VERTEXFORMAT_FLOAT2},
                [ATTR_vs_md5_aweight] = {.format = SG_VERTEXFORMAT_SHORT2},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_BACK,
    });
}

