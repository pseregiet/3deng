#include "particle_mngr.h"
#include "hmm.h"
#include "genshd_particle.h"
#include "sokolgl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PARTICLE_VBUF_COUNT 10
#define PARTICLE_COUNT_PER_VBUF 4096
#define PARTICLE_SIZE_B (sizeof(struct particle_gpu))
#define PARTICLE_SIZE_VBUF_B (PARTICLE_SIZE_B * PARTICLE_COUNT_PER_VBUF)
static struct particle_gpu {
    sg_buffer gpubufs[PARTICLE_VBUF_COUNT];
    struct particle_vbuf *cpubufs[PARTICLE_VBUF_COUNT];
    struct particle_vbuf *bigcpubuf;
    int bufoffset[PARTICLE_VBUF_COUNT];
    char touched[PARTICLE_VBUF_COUNT];
} pgpu = {0};

int particle_mngr_init()
{
    pgpu.bigcpubuf = malloc(
            PARTICLE_SIZE_B * PARTICLE_COUNT_PER_VBUF * PARTICLE_VBUF_COUNT);
    sg_buffer_desc desc = {
        .size = (PARTICLE_SIZE_B * PARTICLE_COUNT_PER_VBUF),
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_STREAM,
        .data.ptr = 0,
        .data.size = 0,
    };

    for (int i = 0; i < PARTICLE_VBUF_COUNT; ++i) {
        pgpu.gpubufs[i] = sg_make_buffer(&desc);
        pgpu.cpubufs[i] = &pgpu.bigcpubuf[PARTICLE_COUNT_PER_VBUF * i];
        pgpu.bufoffset[i] = 0;
        pgpu.touched[i] = 0;
    }

    return 0;
}

void particle_mngr_kill()
{
    if (pgpu.bigcpubuf)
        free(pgpu.bigcpubuf);

    for (int i = 0; i < PARTICLE_VBUF_COUNT; ++i)
        sg_destroy_buffer(pgpu.gpubufs[i]);

    memset(&pgpu, 0, sizeof(pgpu));
}

void particle_mngr_upload_vbuf()
{
    for (int i = 0; i < PARTICLE_VBUF_COUNT; ++i) {
        if (!pgpu.touched[i])
            continue;

        sg_range range = {
            .ptr = pgpu.cpubufs[i],
            .size = PARTICLE_SIZE_VBUF_B,
        };
        sg_update_buffer(pgpu.gpubufs[i], &range);
    }
}

void particle_mngr_calc_buffers(struct particle_emiter **pes, int count)
{
    for (int i = 0; i < PARTICLE_VBUF_COUNT; ++i) {
        pgpu.bufoffset[i] = 0;
        pgpu.touched[i] = 0;
    }

    for (int p = 0; p < count; ++p) {
        struct particle_emiter *pe = pes[p];
        for (int i = 0; i < PARTICLE_VBUF_COUNT; ++i) {
            const int curoffset = pgpu.bufoffset[i];
            const int newoffset = curoffset + pe->count;

            if (newoffset < PARTICLE_COUNT_PER_VBUF) {
                pe->cpubuf = &pgpu.cpubufs[i][curoffset];
                pe->gpubuf = pgpu.gpubufs[i];

                pgpu.bufoffset[i] = newoffset;
                pgpu.touched[i] = 1;
                break;
            }
        }

        if (!pe->cpubuf)
            printf("Particle emiter doesn't have cpubuf :(\n");
    }
}

void particle_pipeline(struct pipelines *pipes)
{
    pipes->particle_shd = sg_make_shader(
            shdparticle_shader_desc(SG_BACKEND_GLCORE33));

    pipes->particle = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->particle_shd,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGBA,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .layout = {
            .attrs = {
                [ATTR_vs_particle_apos]    = {
                    .format = SG_VERTEXFORMAT_USHORT2N,
                    .buffer_index = 0,
                },
                [ATTR_vs_particle_auv]     = {
                    .format = SG_VERTEXFORMAT_USHORT2N,
                    .buffer_index = 0,
                },
                [ATTR_vs_particle_amodel1] = {
                    .format = SG_VERTEXFORMAT_FLOAT4,
                    .buffer_index = 1,
                }, 
                [ATTR_vs_particle_amodel2] = {
                    .format = SG_VERTEXFORMAT_FLOAT4,
                    .buffer_index = 1,
                }, 
                [ATTR_vs_particle_amodel3] = {
                    .format = SG_VERTEXFORMAT_FLOAT4,
                    .buffer_index = 1,
                }, 
                [ATTR_vs_particle_amodel4] = {
                    .format = SG_VERTEXFORMAT_FLOAT4,
                    .buffer_index = 1,
                }, 
            },
            .buffers = {
                [0]    = {.step_func = SG_VERTEXSTEP_PER_VERTEX},
                [1] = {.step_func = SG_VERTEXSTEP_PER_INSTANCE},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = false,
        },
        .cull_mode = SG_CULLMODE_NONE,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    });
}
