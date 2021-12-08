#include "shadow.h"
#include "objloader.h"
#include "frameinfo.h"
#include "event.h"
#include "genshd_terrain.h"

struct frameshadow shadow;
extern struct camera cam;
extern struct frameinfo fi;
extern hmm_vec3 ldir;
extern hmm_mat4 model_matrix[10];

hmm_vec3 lipos;
void calc_lightmatrix()
{
    float near = -100.0f;
    float far = 100.0f;
    hmm_vec3 lightpos = cam.pos;
    hmm_mat4 lightproject = HMM_Orthographic(-50.f, 50.f, -50.f, 50.0f, near, far);
    hmm_mat4 lightview = HMM_LookAt(lightpos, cam.dir, HMM_Vec3(0.0f, 1.0f, 0.0f));
    shadow.lightspace = HMM_MultiplyMat4(lightproject, lightview);
    lipos = lightpos;
}

void init_shadow()
{
    calc_lightmatrix();
    sg_image_desc imgdesc = {
        .render_target = true,
        .width = 1024,
        .height = 1024,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
        .sample_count = 1,
    };

    shadow.colormap = sg_make_image(&imgdesc);    
    imgdesc.pixel_format = SG_PIXELFORMAT_DEPTH;
    shadow.depthmap = sg_make_image(&imgdesc);

    shadow.tbind.vertex_buffers[0] = fi.terrainvbuf;
    shadow.mbind.vertex_buffers[0] = fi.modelvbuf;
    //shadow.tbind.fs_images[SLOT_shadowmap] = shadow.colormap;
    //shadow.mbind.fs_images[SLOT_shadowmap] = shadow.colormap;

    sg_shader shd_depth = sg_make_shader(shddepth_shader_desc(SG_BACKEND_GLCORE33));

    shadow.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_depth,
        .layout = {
            .buffers[ATTR_vs_depth_apos] = {.stride = 8 * sizeof(float) },
            .attrs[ATTR_vs_depth_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .color_count = 1,
        .colors[0] = {
            //.write_mask = SG_COLORMASK_RGB
        },
        .cull_mode = SG_CULLMODE_BACK,
    });
    
    shadow.tpip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_depth,
        .layout = {
            .buffers[ATTR_vs_depth_apos] = {.stride = 11 * sizeof(float) },
            .attrs[ATTR_vs_depth_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .color_count = 1,
        .colors[0] = {
            //.write_mask = SG_COLORMASK_RGB
        },
        .cull_mode = SG_CULLMODE_FRONT,
    });

    shadow.act = (sg_pass_action){
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = {1.0f, 1.0f, 1.0f, 1.0f } },
    };

    shadow.pass = sg_make_pass(&(sg_pass_desc) {
        .color_attachments[0].image = shadow.colormap,
        .depth_stencil_attachment.image = shadow.depthmap,
    });

    //fi.mainbind.fs_images[SLOT_shadowmap] = shadow.colormap;
}

void shadow_draw()
{
    sg_begin_pass(shadow.pass, &shadow.act);
    sg_apply_pipeline(shadow.tpip);
    sg_apply_bindings(&shadow.tbind);
    
    hmm_mat4 rotat =  HMM_Rotate(90, HMM_Vec3(1.0f, 0.0f, 0.0f));
    hmm_mat4 model = HMM_MultiplyMat4(model, rotat);
    
    vs_depthparams_t unis = {
        .lightmat = shadow.lightspace,
        .model = model,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_depthparams, &SG_RANGE(unis));
    sg_draw(0, 6, 1);

    sg_apply_pipeline(shadow.pip);
    sg_apply_bindings(&shadow.mbind);
    for (int i = 0; i < 9; ++i) {
        unis.model = model_matrix[i];
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_depthparams, &SG_RANGE(unis));
        sg_draw(0, fi.cat.vcount, 1);
    }
}
