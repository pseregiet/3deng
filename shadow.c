#include "shadow.h"
#include "objloader.h"
#include "static_object.h"
#include "frameinfo.h"
#include "event.h"
#include "genshd_terrain.h"

struct frameshadow shadow;
extern struct frameinfo fi;

static void calc_lightmatrix()
{
    float near = 0.01f;
    float far = 1200.0f;
    hmm_mat4 lightproject = HMM_Orthographic(-600.f, 600.f, -600.f, 600.0f, near, far);
    hmm_vec3 shadowstart = HMM_MultiplyVec3f(fi.dlight_dir, -250.f);
    shadowstart = HMM_AddVec3(shadowstart, fi.cam.pos);
    hmm_mat4 lightview = HMM_LookAt(shadowstart, HMM_AddVec3(shadowstart, fi.dlight_dir), HMM_Vec3(0.0f, 1.0f, 0.0f));
    shadow.lightspace = HMM_MultiplyMat4(lightproject, lightview);
}

sg_pipeline shadow_create_pipeline(int stride, bool cullfront, bool ibuf)
{
    int cull = cullfront ? SG_CULLMODE_FRONT : SG_CULLMODE_BACK;
    int imode = ibuf ? SG_INDEXTYPE_UINT16 : SG_INDEXTYPE_NONE;

    sg_pipeline pipe = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shadow.shd,
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

    shadow.colormap = sg_make_image(&imgdesc);    
    imgdesc.pixel_format = SG_PIXELFORMAT_DEPTH;
    shadow.depthmap = sg_make_image(&imgdesc);


    shadow.shd = sg_make_shader(shddepth_shader_desc(SG_BACKEND_GLCORE33));
    shadow.pip = shadow_create_pipeline((8 * sizeof(float)), false, false);
    shadow.tpip = shadow_create_pipeline((11 * sizeof(float)), true, true);
    shadow.act = (sg_pass_action){
        //.colors[0] = { .action = SG_ACTION_CLEAR, .value = {1.0f, 1.0f, 1.0f, 1.0f } },
    };
    shadow.pass = sg_make_pass(&(sg_pass_desc) {
        .color_attachments[0].image = shadow.colormap,
        .depth_stencil_attachment.image = shadow.depthmap,
    });

    return 0;
}

void shadowmap_kill()
{
    sg_destroy_pipeline(shadow.pip);
    sg_destroy_pipeline(shadow.tpip);
    sg_destroy_shader(shadow.shd);
    sg_destroy_image(shadow.colormap);
    sg_destroy_image(shadow.depthmap);
    sg_destroy_pass(shadow.pass);
}


void shadowmap_draw()
{
    calc_lightmatrix();
    sg_begin_pass(shadow.pass, &shadow.act);
    sg_apply_pipeline(shadow.tpip);
    
    vs_depthparams_t unis;

    int i = 0;
    for (int y = 0; y < fi.map.h; ++y) {
        for (int x = 0; x < fi.map.w; ++x) {
            shadow.tbind.vertex_buffers[0] = fi.map.vbuffers[i];
            shadow.tbind.index_buffer = fi.map.ibuffers[i++];
            
            unis.model = HMM_Translate(HMM_Vec3(fi.map.scale * x, 0.0f, fi.map.scale * y));
            unis.model = HMM_MultiplyMat4(shadow.lightspace, unis.model);
            
            sg_apply_bindings(&shadow.tbind);
            
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_depthparams, &SG_RANGE(unis));
            sg_draw(0, 128*128*6, 1);
        }
    }

    sg_apply_pipeline(shadow.pip);

    for (int i = 0; i < static_objs.count; ++i) {
        shadow.mbind.vertex_buffers[0] = static_objs.data[i].model->buffer;
        sg_apply_bindings(&shadow.mbind);
        
        unis.model = HMM_MultiplyMat4(shadow.lightspace, static_objs.data[i].matrix);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(unis));
        sg_draw(0, static_objs.data[i].model->vcount, 1);
    }

    sg_end_pass();
}

