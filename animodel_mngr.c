#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"
#include "animodel_mngr.h"
#include "hmm.h"
#include <stdio.h>

#define BONE_TEXTURE_COUNT (10)
#define BONES_PER_LINE (256)
#define BONES_LINES (1)
#define BONES_PER_TEXT (BONES_PER_LINE * BONES_LINES)
#define BONE_SIZE_B (sizeof(hmm_mat4))
#define BONE_SIZE_F (BONE_SIZE_B / sizeof(float))
#define TEXTURE_W (BONES_PER_LINE * 4)
#define TEXTURE_H (BONES_LINES)
#define TEXTURE_SIZE_B (TEXTURE_W * TEXTURE_H * sizeof(hmm_vec4))
#define TEXTURE_SIZE_F (TEXTURE_SIZE_B / sizeof(float))

static sg_image boneimgs[BONE_TEXTURE_COUNT] = {0};
static float *bonebuf[BONE_TEXTURE_COUNT] = {0};
static float *bigbonebuf = 0;
static int texoffset[BONE_TEXTURE_COUNT] = {0};
static char touched[BONE_TEXTURE_COUNT] = {0};

int animodel_mngr_init()
{
    bigbonebuf = malloc(TEXTURE_SIZE_B * BONE_TEXTURE_COUNT);
    sg_image_desc desc = {
        .width = TEXTURE_W,
        .height = TEXTURE_H,
        .num_mipmaps = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA32F,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };

    for (int i = 0; i < BONE_TEXTURE_COUNT; ++i) {
        boneimgs[i] = sg_make_image(&desc);
        bonebuf[i] = &bigbonebuf[TEXTURE_SIZE_F * i];
        texoffset[i] = 0;
    }

    return 0;
}

void animodel_mngr_kill()
{
    sg_image zero = {.id = 0};
    if (bigbonebuf)
        free(bigbonebuf);

    for (int i = 0; i < 10; ++i) {
        sg_destroy_image(boneimgs[i]);
        boneimgs[i] = zero;
        bonebuf[i] = 0;
    }
}
void animodel_mngr_calc_boneuv(struct animodel **ams, int count)
{
    for (int i = 0; i < BONE_TEXTURE_COUNT; ++i) {
        touched[i] = 0;
        texoffset[i] = 0;
    }

    for (int a = 0; a < count; ++a) {
        struct animodel *am = ams[a];
        for (int i = 0; i < BONE_TEXTURE_COUNT; ++i) {
            const int curoffset = texoffset[i];
            const int newoffset = curoffset + am->model->jcount;

            if (newoffset < BONES_PER_TEXT) {
                const int offsetuv = (curoffset * 4);
                am->boneuv[0] = (offsetuv) % TEXTURE_W,
                am->boneuv[1] = (offsetuv) / TEXTURE_W,
                    //TODO: These are const, so shouldn't be part of uniform
                am->boneuv[2] = TEXTURE_W,
                am->boneuv[3] = TEXTURE_H,
                am->bonemap = boneimgs[i];
                am->bonebuf = &bonebuf[i][curoffset * BONE_SIZE_F];
                //assert(am->bonebuf == bonebuf[i]);
                texoffset[i] = newoffset;
                touched[i] = 1;
                break;
            }
        }
        if (!am->bonebuf)
            printf("model doesn't have bonebuf :(\n");
    }
}

void animodel_mngr_upload_bones()
{
    for (int i = 0; i < BONE_TEXTURE_COUNT; ++i) {
        if (!touched[i])
            continue;

        sg_image_data imgdata = {
            .subimage[0][0] = {.ptr = bonebuf[i], .size = TEXTURE_SIZE_B},
        };
        sg_update_image(boneimgs[i], &imgdata);
    }
}

