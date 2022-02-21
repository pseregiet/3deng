#include "shadow.h"
#include "objloader.h"
#include "staticmapobj.h"
#include "frameinfo.h"
#include "event.h"
#include "animodel.h"
#include "animatmapobj.h"
#include "genshd_terrain.h"

extern struct frameinfo fi;

static void calc_lightmatrix()
{
    const float near = 0.01f;
    const float far = 1200.0f;
    const float boxsize = 500.0f;
    const float dist = -250.0f;
    hmm_mat4 lightproject = HMM_Orthographic(-boxsize, boxsize, -boxsize, boxsize, near, far);
    hmm_vec3 shadowstart = HMM_MultiplyVec3f(fi.dirlight.dir, dist);
    shadowstart = HMM_AddVec3(shadowstart, fi.cam.pos);
    hmm_mat4 lightview = HMM_LookAt(shadowstart, HMM_AddVec3(shadowstart, fi.dirlight.dir), HMM_Vec3(0.0f, 1.0f, 0.0f));
    fi.shadow.lightspace = HMM_MultiplyMat4(lightproject, lightview);
}

static sg_pipeline shadow_create_pipeline(int stride, bool cullfront, bool ibuf)
{
    int cull = cullfront ? SG_CULLMODE_FRONT : SG_CULLMODE_BACK;
    int imode = ibuf ? SG_INDEXTYPE_UINT16 : SG_INDEXTYPE_NONE;

    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = fi.shadow.shd,
        .layout = {
            .buffers[ATTR_vs_depth_apos] = {.stride = stride }, //8 * sizeof(float) },
            .attrs[ATTR_vs_depth_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = cull,
        .index_type = imode,
    });
}

int shadowmap_init()
{
    sg_image_desc imgdesc = {
        .render_target = true,
        .width = 2048,
        .height = 2048,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
        .border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
        .sample_count = 1,
    };

    fi.shadow.colormap = sg_make_image(&imgdesc);    
    imgdesc.pixel_format = SG_PIXELFORMAT_DEPTH;
    fi.shadow.depthmap = sg_make_image(&imgdesc);


    fi.shadow.shd = sg_make_shader(shddepth_shader_desc(SG_BACKEND_GLCORE33));
    fi.shadow.pip = shadow_create_pipeline((11 * sizeof(float)), false, true);
    fi.shadow.tpip = shadow_create_pipeline((11 * sizeof(float)), true, true);
    fi.shadow.act = (sg_pass_action){
        0
        //.colors[0] = { .action = SG_ACTION_CLEAR, .value = {1.0f, 1.0f, 1.0f, 1.0f } },
    };
    fi.shadow.pass = sg_make_pass(&(sg_pass_desc) {
        .color_attachments[0].image = fi.shadow.colormap,
        .depth_stencil_attachment.image = fi.shadow.depthmap,
    });

    return 0;
}

void shadowmap_kill()
{
    sg_destroy_pipeline(fi.shadow.pip);
    sg_destroy_pipeline(fi.shadow.tpip);
    sg_destroy_shader(fi.shadow.shd);
    sg_destroy_image(fi.shadow.colormap);
    sg_destroy_image(fi.shadow.depthmap);
    sg_destroy_pass(fi.shadow.pass);
}


void shadowmap_draw()
{
    calc_lightmatrix();
    sg_begin_pass(fi.shadow.pass, &fi.shadow.act);
    sg_apply_pipeline(fi.shadow.tpip);
    
    vs_depthparams_t unis;

    int i = 0;
    for (int y = 0; y < fi.map.h; ++y) {
        for (int x = 0; x < fi.map.w; ++x) {
            fi.shadow.tbind.vertex_buffers[0] = fi.map.vbuffers[i];
            fi.shadow.tbind.index_buffer = fi.map.ibuffers[i++];
            
            unis.model = HMM_Translate(HMM_Vec3(fi.map.scale * x, 0.0f, fi.map.scale * y));
            unis.model = HMM_MultiplyMat4(fi.shadow.lightspace, unis.model);
            
            sg_apply_bindings(&fi.shadow.tbind);
            
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_depthparams, &SG_RANGE(unis));
            sg_draw(0, 128*128*6, 1);
        }
    }

    sg_apply_pipeline(fi.shadow.pip);
    {
    const int end = staticmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct staticmapobj *obj = staticmapobj_get(i);
        const struct obj_model *mdl = obj->om;
        fi.shadow.mbind.vertex_buffers[0] = mdl->vbuf;
        fi.shadow.mbind.index_buffer = mdl->ibuf;
        sg_apply_bindings(&fi.shadow.mbind);
        
        unis.model = HMM_MultiplyMat4(fi.shadow.lightspace, obj->matrix);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(unis));
        sg_draw(0, mdl->vcount, 1);
    }
    }

    sg_apply_pipeline(fi.pipes.animodel_shadow);
    {
    const int end = animatmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct animatmapobj *obj = animatmapobj_mngr_get(i);
        struct animodel *am = &obj->am;
        animodel_shadow_render(am, &fi, obj->matrix);
    }
    }

    sg_end_pass();
}

