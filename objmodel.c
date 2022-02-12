#include <stdio.h>
#include <string.h>
#include "objmodel.h"
#include "fileops.h"
#include "texloader.h"
#include "extrahmm.h"
#include "genshd_combo.h"

#define OBJ_MAGIC (0x1234567)

struct vertex {
    hmm_vec3 pos;
    hmm_vec3 nor;
    hmm_vec2  uv;
    hmm_vec3 tan;
};

struct invec {
    hmm_vec3 pos;
    hmm_vec3 nor;
    hmm_vec2 uv;
};

inline static uintptr_t align8(uintptr_t addr)
{
    int rest = addr & 7;
    if (rest)
        rest = 8 - rest;
    return rest;
}

inline static uintptr_t align4(uintptr_t addr)
{
    int rest = addr & 3;
    if (rest)
        rest = 4 - rest;
    return rest;
}

static void calc_tangents(struct vertex *vbuf, uint16_t *ibuf, int indecount)
{
    for (int i = 0; i < indecount; i +=3) {
        uint16_t idx[3] = {ibuf[i+0], ibuf[i+1], ibuf[i+2]};
        hmm_vec3 v0 = vbuf[idx[0]].pos;
        hmm_vec3 v1 = vbuf[idx[1]].pos;
        hmm_vec3 v2 = vbuf[idx[2]].pos;

        hmm_vec2 uv0 = vbuf[idx[0]].uv;
        hmm_vec2 uv1 = vbuf[idx[1]].uv;
        hmm_vec2 uv2 = vbuf[idx[2]].uv;

        hmm_vec3 t0 = HMM_NormalizeVec3(
                get_tangent(&v0, &v1, &v2, &uv0, &uv1, &uv2));
        vbuf[idx[0]].tan = t0;
        vbuf[idx[1]].tan = t0;
        vbuf[idx[2]].tan = t0;
    }
}

struct objheader {
    uint32_t magic;
    uint32_t vertcount;
    uint32_t indecount;
    uint32_t matlen;
};

int objmodel_open(const char *fn, struct obj_model *mdl)
{
    struct file f;
    if (openfile(&f, fn))
        return -1;

    struct objheader header;
    if (f.usize < sizeof(header)) {
        printf("%s obj model corrupted\n", fn);
        closefile(&f);
        return -1;
    }

    memcpy(&header, f.udata, sizeof(header));
    header.matlen++;
    int expectedsize = sizeof(header) +
        (header.matlen + align4(header.matlen) +
        (sizeof(struct invec) * header.vertcount) +
        (sizeof(uint16_t) * header.indecount));

    if (header.magic != OBJ_MAGIC || f.usize != expectedsize) {
        printf("%s obj model corrupted\n", fn);
        closefile(&f);
        return -1;
    }

    char *matname = &f.udata[sizeof(header)];
    matname[header.matlen] = 0;

    char tmp[0x1000];
    snprintf(tmp, 0x1000, "%s/diff", matname);
    mdl->imgdiff = texloader_find(tmp);
    snprintf(tmp, 0x1000, "%s/spec", matname);
    mdl->imgspec = texloader_find(tmp);
    snprintf(tmp, 0x1000, "%s/norm", matname);
    mdl->imgnorm = texloader_find(tmp);

    const int vbufsize = sizeof(struct vertex) * header.vertcount;
    const int ibufsize = sizeof(uint16_t) * header.indecount;
    struct vertex *vbuf = malloc(vbufsize);
    uint16_t *ibuf = malloc(ibufsize);

    const int voff = sizeof(header) + (header.matlen + align4(header.matlen));
    const int ioff = voff + (sizeof(struct invec) * header.vertcount);

    struct invec *vsrc = (struct invec *)&f.udata[voff];
    uint16_t *isrc = (uint16_t *)&f.udata[ioff];

    //TODO: right now im copying straight data to another buffer
    //not sure if necessary. Might be good idea if I do multithread.
    memcpy(ibuf, isrc, ibufsize);
    for (int i = 0; i < header.vertcount; ++i) {
        struct invec *vi = &vsrc[i];
        struct vertex *vo = &vbuf[i];
        memcpy(vo, vi, sizeof(*vi));
    }

    closefile(&f);
    calc_tangents(vbuf, ibuf, header.indecount);

    mdl->vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = vbufsize,
        .data.ptr = vbuf,
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
    });

    mdl->ibuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = ibufsize,
        .data.ptr = ibuf,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });

    mdl->vcount = header.vertcount;
    mdl->icount = header.indecount;

    free(vbuf);
    free(ibuf);

    return 0;
}

void objmodel_kill(struct obj_model *mdl)
{
    sg_destroy_buffer(mdl->vbuf);
    sg_destroy_buffer(mdl->ibuf);
}

void objmodel_pipeline(struct pipelines *pipes)
{
    pipes->objmodel_shd = sg_make_shader(comboshader_shader_desc(SG_BACKEND_GLCORE33));
    
    pipes->objmodel = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->objmodel_shd,
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
            .buffers = {
                [ATTR_vs_position] = {.stride = sizeof(float) * 11},
                [ATTR_vs_normal] = {.stride = sizeof(float) * 11},
                [ATTR_vs_texcoord] = {.stride = sizeof(float) * 11},
            },

            .attrs = {
                [ATTR_vs_position] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_normal] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_texcoord] = {.format = SG_VERTEXFORMAT_FLOAT2},
                //[ATTR_vs_tang] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_FRONT,
    });
}

static inline float bits2float(const bool *bits)
{
    uint32_t in = 0;

    for (int i = 0; i < 32; ++i)
        if (bits[i])
            in |= (1 << i);

    return (float)in;
}

void objmodel_fraguniforms(struct frameinfo *fi)
{
    fs_params_t fs_params = {
        .viewpos = fi->cam.pos,
        .matshine = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &SG_RANGE(fs_params));

    fs_dir_light_t fs_dir_light = {
        .direction =    fi->dirlight.dir,
        .ambient =      fi->dirlight.ambi,
        .diffuse =      fi->dirlight.diff,
        .specular =     fi->dirlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_dir_light, &SG_RANGE(fs_dir_light));

    fs_point_lights_t fs_point_lights;
    fs_point_lights.enabled = bits2float((bool *)&fi->lightsenable);
    for (int i = 0; i < 4; ++i) {
        fs_point_lights.position[i] =       HMM_Vec4v(fi->pointlight[i].pos, 0.0f);
        fs_point_lights.ambient[i] =        HMM_Vec4v(fi->pointlight[i].ambi, 0.0f);
        fs_point_lights.diffuse[i] =        HMM_Vec4v(fi->pointlight[i].diff, 0.0f);
        fs_point_lights.specular[i] =       HMM_Vec4v(fi->pointlight[i].spec, 0.0f);
        fs_point_lights.attenuation[i] =    HMM_Vec4v(fi->pointlight[i].atte, 0.0f);
    }
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_point_lights, &SG_RANGE(fs_point_lights));

    fs_spot_light_t fs_spot_light = {
        .position =     fi->cam.pos,
        .direction =    fi->cam.front,
        .cutoff =       fi->spotlight.cutoff, //HMM_COSF(HMM_ToRadians(12.5f)),
        .outcutoff =    fi->spotlight.outcutoff, //HMM_COSF(HMM_ToRadians(15.0f)),
        .attenuation =  fi->spotlight.atte,
        .ambient =      fi->spotlight.ambi,
        .diffuse =      fi->spotlight.diff,
        .specular =     fi->spotlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_spot_light, &SG_RANGE(fs_spot_light));
}

void objmodel_render(const struct obj_model *mdl, struct frameinfo *fi, hmm_mat4 model)
{
    sg_bindings bind = {
        .vertex_buffers[0] = mdl->vbuf,
        .index_buffer = mdl->ibuf,
        .fs_images[SLOT_imgdiff] = mdl->imgdiff,
        .fs_images[SLOT_imgspec] = mdl->imgspec,
        //.fs_images[SLOT_imgnorm] = mdl->imgnorm,
    };
    sg_apply_bindings(&bind);

    vs_params_t vsuni = {
        .vp = fi->vp,
        .model = model
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(vsuni));
    sg_draw(0, mdl->icount, 1);
}
