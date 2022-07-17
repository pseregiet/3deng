#include "shadow.h"
#include "objloader.h"
#include "staticmapobj.h"
#include "frameinfo.h"
#include "event.h"
#include "animodel.h"
#include "animatmapobj.h"
#include "terrain.h"

extern struct frameinfo fi;

static void calc_lightmatrix_dir()
{
    const float near = 0.01f;
    const float far = 1200.0f;
    const float boxsize = 500.0f;
    const float dist = -250.0f;
    const hmm_mat4 lightproject = HMM_Orthographic(-boxsize, boxsize, -boxsize, boxsize, near, far);
    hmm_vec3 shadowstart = HMM_MultiplyVec3f(fi.dirlight.dir, dist);
    shadowstart = HMM_AddVec3(shadowstart, fi.cam.pos);
    hmm_mat4 lightview = HMM_LookAt(shadowstart, HMM_AddVec3(shadowstart, fi.dirlight.dir), HMM_Vec3(0.0f, 1.0f, 0.0f));
    fi.shadowmap.lightspace_dir = HMM_MultiplyMat4(lightproject, lightview);
}

int shadowmap_init()
{
    sg_image_desc imgdesc = {
        .type = SG_IMAGETYPE_2D,
        .render_target = true,
        .width = fi.shadowmap_resolution,
        .height = fi.shadowmap_resolution,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
        /*.wrap_w = SG_WRAP_CLAMP_TO_EDGE,*/
        .border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
        .sample_count = 1,
    };

    //TODO? sokol limitation: shadow shader still needs a color attachment even if unused.
    //Create one image to be used for all shadow maps
    fi.shadowmap.colormap = sg_make_image(&imgdesc);

    // pointlights need a cubemap, color one too...
    imgdesc.type = SG_IMAGETYPE_CUBE;
    fi.shadowmap.colormap_cube = sg_make_image(&imgdesc);
    
    imgdesc.pixel_format = SG_PIXELFORMAT_DEPTH;
    for (int i = 0; i < DEFAULT_POINTLIGHT_COUNT; ++i)
        fi.shadowmap.depthmap_point[i] = sg_make_image(&imgdesc);

    imgdesc.type = SG_IMAGETYPE_2D;
    fi.shadowmap.depthmap_dir = sg_make_image(&imgdesc);
    fi.shadowmap.depthmap_spot = sg_make_image(&imgdesc);

    fi.shadowmap.pass_dir = sg_make_pass(&(sg_pass_desc) {
        .color_attachments[0].image = fi.shadowmap.colormap,
        .depth_stencil_attachment.image = fi.shadowmap.depthmap_dir,
    });

    fi.shadowmap.pass_spot = sg_make_pass(&(sg_pass_desc) {
        .color_attachments[0].image = fi.shadowmap.colormap,
        .depth_stencil_attachment.image = fi.shadowmap.depthmap_spot,
    });

    for (int l = 0; l < DEFAULT_POINTLIGHT_COUNT; ++l) {
        for (int i = 0; i < 6; ++i) {
            fi.shadowmap.pass_point[l * 6 + i] = sg_make_pass(&(sg_pass_desc) {
                .color_attachments[0] = {
                    .image = fi.shadowmap.colormap_cube,
                    .slice = i,
                },
                .depth_stencil_attachment = {
                    .image = fi.shadowmap.depthmap_point[l],
                    .slice = i,
                },
            });
        }
    }

    return 0;
}

void shadowmap_kill()
{
    sg_destroy_image(fi.shadowmap.colormap);
    sg_destroy_image(fi.shadowmap.colormap_cube);
    sg_destroy_image(fi.shadowmap.depthmap_dir);
    sg_destroy_image(fi.shadowmap.depthmap_spot);
    for (int i = 0; i < DEFAULT_POINTLIGHT_COUNT; ++i)
        sg_destroy_image(fi.shadowmap.depthmap_point[i]);

    sg_destroy_pass(fi.shadowmap.pass_dir);
    sg_destroy_pass(fi.shadowmap.pass_spot);
    for (int i = 0; i < DEFAULT_POINTLIGHT_COUNT * 6; ++i)
        sg_destroy_pass(fi.shadowmap.pass_point[i]);
}


void shadowmap_draw()
{
    calc_lightmatrix_dir();

    sg_pass_action action = {0};
    sg_begin_pass(fi.shadowmap.pass_dir, &action);

    // terrain shadows
    sg_apply_pipeline(fi.pipes.terrain_shadow);
    {
    terrain_shadow_render(&fi);
    }

    // objmodel shadows
    sg_apply_pipeline(fi.pipes.objmodel_shadow);
    {
    const int end = staticmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct staticmapobj *obj = staticmapobj_get(i);
        const struct obj_model *mdl = obj->om;
        objmodel_shadow_render(mdl, &fi, obj->matrix);
    }
    }

    // md5model shadows
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

