#include "wirebox.h"
#include "sokolgl.h"
#include "hmm.h"
#include "genshd_wirebox.h"

sg_buffer wirebox_buf;

int wirebox_init()
{
    float vbuf[8*3*4] = {
        -0.5f, -0.5f, +0.5f,
        +0.5f, -0.5f, +0.5f,
        +0.5f, -0.5f, +0.5f,
        +0.5f, +0.5f, +0.5f,
        +0.5f, +0.5f, +0.5f,
        -0.5f, +0.5f, +0.5f,
        -0.5f, +0.5f, +0.5f,
        -0.5f, -0.5f, +0.5f,
        -0.5f, -0.5f, -0.5f,
        +0.5f, -0.5f, -0.5f,
        +0.5f, -0.5f, -0.5f,
        +0.5f, +0.5f, -0.5f,
        +0.5f, +0.5f, -0.5f,
        -0.5f, +0.5f, -0.5f,
        -0.5f, +0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, +0.5f,
        -0.5f, -0.5f, -0.5f,
        +0.5f, -0.5f, +0.5f,
        +0.5f, -0.5f, -0.5f,
        +0.5f, +0.5f, +0.5f,
        +0.5f, +0.5f, -0.5f,
        -0.5f, +0.5f, +0.5f,
        -0.5f, +0.5f, -0.5f,
    };

    wirebox_buf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(vbuf),
        .data.ptr = vbuf,
    });

    return 0;
}

void wirebox_kill()
{
    sg_destroy_buffer(wirebox_buf);
}

void wirebox_pipeline(struct pipelines *pipes)
{
    pipes->wirebox_shd = sg_make_shader(shdwirebox_shader_desc(SG_BACKEND_GLCORE33));

    pipes->wirebox = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->wirebox_shd,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGB,
            .blend = {
                .enabled = false,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .layout = {
            .attrs = {
                [ATTR_wirebox_vs_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = false,
        },
        .primitive_type = SG_PRIMITIVETYPE_LINES,
    });
}

inline static void wirebox_vertuniforms_slow(struct frameinfo *fi)
{
    vs_wirebox_slow_t vs = {
        .uvp = fi->vp,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_wirebox_slow, &SG_RANGE(vs));
}

inline static void wirebox_vertuniforms_fast(hmm_mat4 model)
{
    vs_wirebox_fast_t vs = {
        .umodel = model,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_wirebox_fast, &SG_RANGE(vs));
}

void wirebox_prepare_render(struct frameinfo *fi)
{
    sg_apply_pipeline(fi->pipes.wirebox);

    sg_bindings bind = { .vertex_buffers[0] = wirebox_buf };
    sg_apply_bindings(&bind);
    wirebox_vertuniforms_slow(fi);
}

void wirebox_render(struct frameinfo *fi, hmm_mat4 model)
{
    wirebox_vertuniforms_fast(model);
    sg_draw(0, 24, 1);
}
