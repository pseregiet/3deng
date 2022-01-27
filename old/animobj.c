#include "animobj.h"
#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "cJSON/cJSON.h"
#include "fileops.h"
#include "genshd_md5.h"
#include "md5loader.h"
#include "frameinfo.h"

/*
KHASH_MAP_INIT_STR(animobjmap, struct animobj *)

static struct animobjs {
    struct growing_alloc names;
    struct growing_alloc objs;
    khash_t(animobjmap) map;
};
*/

static struct animobj _obj;
static sg_image _boneimg;
static float *_bonebuf;

#define MAX_BONES 256
#define MAX_INSTA 10
#define INSTA_SIZE (sizeof(float)*4*4*MAX_BONES)


int animobj_init()
{
    struct animobj *obj = &_obj;

    obj->model = md5loader_find("archvile/archvile");
    obj->interp = (struct md5_joint *)calloc(obj->model->jcount, sizeof(*obj->interp));
    obj->curanim = 0;
    obj->curframe = 0;
    obj->nextframe = 1;
    obj->lasttime = 0.f;

    sg_image_desc imgdesc = {
        .width = MAX_BONES * 4,
        .height = MAX_INSTA,
        .num_mipmaps = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA32F,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    _boneimg = sg_make_image(&imgdesc);
    
    _bonebuf = (float *)calloc(sizeof(hmm_mat4), MAX_BONES);
    return 0;
}

void animobj_uploadbones(const float *data, int size)
{
    sg_image_data imgdata = {
        .subimage[0][0] = {.ptr = data, .size = size},
    };
    sg_update_image(_boneimg, &imgdata);
}

extern struct frameinfo fi;

void animobj_render(struct pipelines *pipes, hmm_mat4 vp)
{
    sg_apply_pipeline(pipes->animobj);

    struct md5_model *model = _obj.model;
    for (int i = 0; i < _obj.model->mcount; ++i) {
        sg_bindings bind = {
            .vertex_buffers[0] = model->meshes[i].vbuf,
            .index_buffer = model->meshes[i].ibuf,
            .vs_images[SLOT_weightmap] = model->weightmap,
            .vs_images[SLOT_bonemap] = _boneimg,
            .fs_images[SLOT_imgdiff] = model->meshes[i].imgd,
            .fs_images[SLOT_imgspec] = model->meshes[i].imgs,
            .fs_images[SLOT_imgnorm] = model->meshes[i].imgn,
        };
        sg_apply_bindings(&bind);

        vs_md5_t univs = {
            .umodel = HMM_MultiplyMat4(
                    HMM_Translate(HMM_Vec3(0.0f, 100.0f, 0.f)),
                    HMM_Rotate(-90.0f, HMM_Vec3(0.5f, 0.0f, 0.0f))),
            .uvp = vp,
            .uboneuv = {0, 0, MAX_BONES*4, MAX_INSTA},
            .uweightuv = {model->meshes[i].woffset, 0, model->weightw, 1},
            .ulightpos = fi.dlight_dir,
            .uviewpos = fi.cam.pos,
        };
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_md5, &SG_RANGE(univs));

        fs_md5_t unifs = {
            .uambi = fi.dlight_ambi,
            .udiff = fi.dlight_diff,
            .uspec = HMM_Vec3(0.5f, 0.5f, 0.5f),
        };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_md5, &SG_RANGE(unifs));

        sg_draw(0, model->meshes[i].icount*3, 1);
    }
}

void animobj_kill(struct animobj *obj)
{
    if (obj->interp)
        free(obj->interp);
}

void animobj_pipeline(struct pipelines *pipes)
{
    pipes->animobj_shd = sg_make_shader(shdmd5_shader_desc(SG_BACKEND_GLCORE33));

    pipes->animobj = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->animobj_shd,
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

static void animobj_bone2tex(const struct md5_model *mdl, const struct md5_joint *in, float *out, int jcount)
{
    for (int i = 0; i < jcount; ++i) {
        hmm_mat4 m = HMM_QuaternionToMat4(in[i].orient);
        m.Elements[3][0] = in[i].pos.X;
        m.Elements[3][1] = in[i].pos.Y;
        m.Elements[3][2] = in[i].pos.Z;
        m = HMM_MultiplyMat4(m, mdl->invmatrices[i]);
        int offset = 16 * i;
        memcpy(&out[offset], &m, sizeof(float) * 16);
    }
}

void animobj_play(double delta)
{
    struct animobj *obj = &_obj;
    if (obj->curanim == -1)
        return;

    delta /= 1000.0f;

    struct md5_anim *curanim = obj->model->anims[obj->curanim];
    int maxframes = curanim->fcount;
    obj->lasttime += delta;

    if (obj->lasttime >= (1.0f / curanim->fps)) {
        obj->curframe++;
        obj->nextframe++;
        obj->lasttime = 0.0f;

        if (obj->curframe >= maxframes)
            obj->curframe = 0;
        if (obj->nextframe >= maxframes)
            obj->nextframe = 0;
    }

    float interptime = obj->lasttime * curanim->fps;
    //md5anim_interp(curanim, obj->interp, obj->curframe, obj->nextframe, interptime);
    struct md5_joint *jf = &curanim->frames[curanim->jcount * obj->curframe];

    float *outbuf = &_bonebuf[0];
    animobj_bone2tex(obj->model, jf, outbuf, curanim->jcount);
}

void animobj_update_bonetex()
{
    animobj_uploadbones(_bonebuf, INSTA_SIZE * MAX_INSTA);
}
