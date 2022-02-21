#include "animodel.h"
#include "genshd_md5.h"
#include <stdio.h>
#include <string.h>

static int lmao = 0;
int animodel_init(struct animodel *am, const char *modelname)
{
    const struct md5_model *mdl = md5loader_find(modelname);
    if (!mdl) {
        printf("mdl %s not found\n", modelname);
        return -1;
    }

    am->model = mdl;
    am->interp = (struct md5_joint *)calloc(mdl->jcount, sizeof(*am->interp));
    am->curranim = 0;
    am->currframe = lmao++;
    am->nextframe = 0;
    am->pause = false;
    am->lasttime = 0.0f;
    am->bonebuf = 0;

    return 0;
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

static inline float bits2float(const bool *bits)
{
    uint32_t in = 0;

    for (int i = 0; i < 32; ++i)
        if (bits[i])
            in |= (1 << i);

    return (float)in;
}

void animodel_fraguniforms_slow(struct frameinfo *fi)
{
    fs_md5_dirlight_t fs_dirlight = {
        .ambi    =   fi->dirlight.ambi,
        .diff    =   fi->dirlight.diff,
        .spec    =   fi->dirlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_md5_dirlight, &SG_RANGE(fs_dirlight));

    fs_md5_pointlights_t fs_pointlights;
    fs_pointlights.enabled = bits2float((bool *)&fi->lightsenable);
    for (int i = 0; i < 4; ++i) {
        fs_pointlights.ambi[i] =    HMM_Vec4v(fi->pointlight[i].ambi, 0.0f);
        fs_pointlights.diff[i] =    HMM_Vec4v(fi->pointlight[i].diff, 0.0f);
        fs_pointlights.spec[i] =    HMM_Vec4v(fi->pointlight[i].spec, 0.0f);
        fs_pointlights.atte[i] =    HMM_Vec4v(fi->pointlight[i].atte, 0.0f);
    }
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_md5_pointlights, &SG_RANGE(fs_pointlights));

    fs_md5_spotlight_t fs_spotlight = {
        .cutoff =       fi->spotlight.cutoff, //HMM_COSF(HMM_ToRadians(12.5f)),
        .outcutoff =    fi->spotlight.outcutoff, //HMM_COSF(HMM_ToRadians(15.0f)),
        .atte =         fi->spotlight.atte,
        .ambi =         fi->spotlight.ambi,
        .diff =         fi->spotlight.diff,
        .spec =         fi->spotlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_md5_spotlight, &SG_RANGE(fs_spotlight));
}

void animodel_vertuniforms_slow(struct frameinfo *fi)
{
    vs_md5_slow_t vs = {
        .uvp           = fi->vp,
        .uviewpos      = fi->cam.pos,
        .usptlight_pos = fi->cam.pos,
        .usptlight_dir = fi->cam.front,
        .udirlight_dir = fi->dirlight.dir,
    };
    for (int i = 0; i < 4; ++i)
        vs.upntlight_pos[i] = HMM_Vec4v(fi->pointlight[i].pos, 0.0f);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_md5_slow, &SG_RANGE(vs));
}

inline static void animodel_vertuniforms_fast(const hmm_mat4 model, const int *boneuv)
{
    vs_md5_fast_t vs = {
        .umodel = model,
        .uboneuv = {
            boneuv[0],
            boneuv[1],
            boneuv[2],
            boneuv[3]
        },
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_md5_fast, &SG_RANGE(vs));
}

inline static void animodel_fraguniforms_fast(const struct material *mat)
{
    fs_md5_fast_t fs = {
        .umatshine = mat->shine.value,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_md5_fast, &SG_RANGE(fs));
}

void animodel_render(struct animodel *am, struct frameinfo *fi, hmm_mat4 model)
{
    const struct md5_model *mdl = am->model;
    for (int i = 0; i < mdl->mcount; ++i) {
        const struct md5_mesh *mesh = &mdl->meshes[i];

        sg_bindings bind = {
            .vertex_buffers[0] = mesh->vbuf,
            .index_buffer = mesh->ibuf,
            .vs_images[SLOT_weightmap] = mdl->weightmap,
            .vs_images[SLOT_bonemap] = am->bonemap,
            .fs_images[SLOT_imgdiff] = mesh->material.imgs[MAT_DIFF],
            .fs_images[SLOT_imgspec] = mesh->material.imgs[MAT_SPEC],
            .fs_images[SLOT_imgnorm] = mesh->material.imgs[MAT_NORM],
        };
        sg_apply_bindings(&bind);

        const int boneuv[4] = {
            am->boneuv[0],
            am->boneuv[1],
            mesh->woffset,
            mdl->weightw
        };
        animodel_vertuniforms_fast(model, boneuv);
        animodel_fraguniforms_fast(&mesh->material);
        sg_draw(mesh->ioffset, mesh->icount, 1);
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
                .enabled = false,
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

void animodel_shadow_pipeline(struct pipelines *pipes)
{
    pipes->animodel_shadow_shd = sg_make_shader(shdmd5_depth_shader_desc(SG_BACKEND_GLCORE33));

    const int stride = (11 * sizeof(float) + (2 * sizeof(uint16_t)));
    const int offset = (11 * sizeof(float));
    sg_pipeline_desc desc = {
        .shader = pipes->animodel_shadow_shd,
        .layout = {
            .attrs = {
                [ATTR_vs_md5_depth_apos] = {
                    .format = SG_VERTEXFORMAT_FLOAT3,
                    .offset = 0
                },
                [ATTR_vs_md5_depth_aweight] = {
                    .format = SG_VERTEXFORMAT_SHORT2,
                    .offset = offset
                },
            },
            .buffers = {
                [ATTR_vs_md5_depth_apos] = {.stride = stride},
                [ATTR_vs_md5_depth_aweight] = {.stride = stride},
            },
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_FRONT,
        .index_type = SG_INDEXTYPE_UINT16,
    };

    pipes->animodel_shadow = sg_make_pipeline(&desc);
}

void animodel_shadow_render(struct animodel *am, struct frameinfo *fi, hmm_mat4 model)
{
    const struct md5_model *mdl = am->model;
    for (int i = 0; i < mdl->mcount; ++i) {
        const struct md5_mesh *mesh = &mdl->meshes[i];

        sg_bindings bind = {
            .vertex_buffers[0] = mesh->vbuf,
            .index_buffer = mesh->ibuf,
            .vs_images[SLOT_weightmap] = mdl->weightmap,
            .vs_images[SLOT_bonemap] = am->bonemap,
        };
        sg_apply_bindings(&bind);

        vs_md5_depth_t univs = {
            .umodel = HMM_MultiplyMat4(fi->shadow.lightspace, model),
            .uboneuv = {
                am->boneuv[0],
                am->boneuv[1],
                mesh->woffset,
                mdl->weightw
            },
        };
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_md5_depth, &SG_RANGE(univs));
        sg_draw(mesh->ioffset, mesh->icount, 1);
    }
}
