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
        const uint16_t idx[3] = {ibuf[i+0], ibuf[i+1], ibuf[i+2]};
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
    pipes->objmodel_shd = sg_make_shader(shdobj_shader_desc(SG_BACKEND_GLCORE33));
    
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
            /*
            .buffers = {
                [ATTR_vs_obj_apos] = {.stride = sizeof(float) * 11},
                [ATTR_vs_obj_anorm] = {.stride = sizeof(float) * 11},
                [ATTR_vs_obj_auv] = {.stride = sizeof(float) * 11},
            },
            */

            .attrs = {
                [ATTR_vs_obj_apos]  = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_obj_anorm] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_obj_auv]   = {.format = SG_VERTEXFORMAT_FLOAT2},
                [ATTR_vs_obj_atang] = {.format = SG_VERTEXFORMAT_FLOAT3},
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

inline static void objmodel_fraguniforms_fast(float shine)
{

    fs_obj_fast_t fs_fast = {
        .umatshine = shine
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_obj_fast, &SG_RANGE(fs_fast));

}

void objmodel_fraguniforms_slow(struct frameinfo *fi)
{
    fs_obj_dirlight_t fs_dirlight = {
        .ambi    =   fi->dirlight.ambi,
        .diff    =   fi->dirlight.diff,
        .spec    =   fi->dirlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_obj_dirlight, &SG_RANGE(fs_dirlight));

    fs_obj_pointlights_t fs_pointlights;
    fs_pointlights.enabled = bits2float((bool *)&fi->lightsenable);
    for (int i = 0; i < 4; ++i) {
        fs_pointlights.ambi[i] =    HMM_Vec4v(fi->pointlight[i].ambi, 0.0f);
        fs_pointlights.diff[i] =    HMM_Vec4v(fi->pointlight[i].diff, 0.0f);
        fs_pointlights.spec[i] =    HMM_Vec4v(fi->pointlight[i].spec, 0.0f);
        fs_pointlights.atte[i] =    HMM_Vec4v(fi->pointlight[i].atte, 0.0f);
    }
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_obj_pointlights, &SG_RANGE(fs_pointlights));

    fs_obj_spotlight_t fs_spotlight = {
        .cutoff =       fi->spotlight.cutoff, //HMM_COSF(HMM_ToRadians(12.5f)),
        .outcutoff =    fi->spotlight.outcutoff, //HMM_COSF(HMM_ToRadians(15.0f)),
        .atte =         fi->spotlight.atte,
        .ambi =         fi->spotlight.ambi,
        .diff =         fi->spotlight.diff,
        .spec =         fi->spotlight.spec,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_obj_spotlight, &SG_RANGE(fs_spotlight));
}

void objmodel_vertuniforms_slow(struct frameinfo *fi)
{
    vs_obj_slow_t vs = {
        .uvp           = fi->vp,
        .uviewpos      = fi->cam.pos,
        .usptlight_pos = fi->cam.pos,
        .usptlight_dir = fi->cam.front,
        .udirlight_dir = fi->dirlight.dir,
    };
    for (int i = 0; i < 4; ++i)
        vs.upntlight_pos[i] = HMM_Vec4v(fi->pointlight[i].pos, 0.0f);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_obj_slow, &SG_RANGE(vs));
}

inline static void objmodel_vertuniforms_fast(hmm_mat4 model)
{
    vs_obj_fast_t vs = {
        .umodel = model
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_obj_fast, &SG_RANGE(vs));
}

void objmodel_render(const struct obj_model *mdl, struct frameinfo *fi, hmm_mat4 model)
{
    objmodel_vertuniforms_fast(model);
    objmodel_fraguniforms_fast(32.0f);

    sg_bindings bind = {
        .vertex_buffers[0] = mdl->vbuf,
        .index_buffer = mdl->ibuf,
        .fs_images[SLOT_imgdiff] = mdl->imgdiff,
        .fs_images[SLOT_imgspec] = mdl->imgspec,
        .fs_images[SLOT_imgnorm] = mdl->imgnorm,
    };
    sg_apply_bindings(&bind);
    sg_draw(0, mdl->icount, 1);
}
