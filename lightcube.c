#include "lightcube.h"
#include "pipelines.h"
#include "hmm.h"
#include "genshd_combo.h"

void lightcube_pipeline(struct pipelines *pipes)
{
    pipes->lightcube_shd = sg_make_shader(light_cube_shader_desc(SG_BACKEND_GLCORE33));

    pipes->lightcube = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->lightcube_shd,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGB,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        //.index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [ATTR_light_cube_vs_pos] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
    });
}
