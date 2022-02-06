#include "sokolgl.h"
#include "hmm.h"
#include "objmodel.h"
#include "genshd_hitboxcube.h"
#include "hitbox.h"

void simpleobj_hitbox_pipeline(struct pipelines *pipes)
{
    pipes->simpleobj_hitbox_shd = sg_make_shader(hitboxcube_shader_desc(SG_BACKEND_GLCORE33));

    pipes->simpleobj_hitbox = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->simpleobj_hitbox_shd,
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
                [ATTR_hitboxcube_vs_pos] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .primitive_type = SG_PRIMITIVETYPE_LINES,
    });
}

void simpleobj_hitbox_render(struct frameinfo *fi, hmm_mat4 vp)
{
    /*
    vs_params_t uniform;
    sg_bindings bind = {0};

    sg_apply_pipeline(fi->pipes.simpleobj_hitbox);

    for (int i = 0; i < static_objs.count; ++i) {
        bind.vertex_buffers[0] = static_objs.data[i].model->hitbox_vbuf;
        bind.index_buffer = static_objs.data[i].model->hitbox_ibuf;
        sg_apply_bindings(&bind);
        uniform.mvp = HMM_MultiplyMat4(vp, static_objs.data[i].matrix);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(uniform));
        sg_draw(0, 36, 1);
    }
    */
}
