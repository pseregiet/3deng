#include "terrain.h"
#include "heightmap.h"
#include "texloader.h"
#include "genshd_terrain.h"
#include "hmm.h"
#include "extrahmm.h"
#include <stdio.h>
#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

extern struct frameinfo fi;

void terrain_pipeline(struct pipelines *pipes)
{
    pipes->terrain_shd = sg_make_shader(terrainshd_shader_desc(SG_BACKEND_GLCORE33));

    pipes->terrain = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = pipes->terrain_shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .color_count = 1,
        .colors[0] = {
            .write_mask = SG_COLORMASK_RGB,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .layout = {
            .attrs = {
                [ATTR_vs_terrain_apos] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_terrain_anorm] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_terrain_auv] = {.format = SG_VERTEXFORMAT_FLOAT2},
                [ATTR_vs_terrain_atang] = {.format = SG_VERTEXFORMAT_FLOAT3},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_BACK,
    });
}

int init_terrain()
{
    const char *filesd[] = {
        "terraintex/grass_mix_d.qoi",
        "terraintex/desert_mud_d.qoi",
        "terraintex/snow_mud_d.qoi",
        "terraintex/ground_dry2_d.qoi",
        "terraintex/mntn_dark_d.qoi",
    };
    const char *filess[] = {
        "terraintex/grass_mix_s.qoi",
        "terraintex/desert_mud_s.qoi",
        "terraintex/snow_mud_s.qoi",
        "terraintex/mntn_dark_s.qoi",
        "terraintex/ground_dry2_s.qoi",
    };
    const char *filesn[] = {
        "terraintex/grass_mix_n.qoi",
        "terraintex/desert_mud_n.qoi",
        "terraintex/snow_mud_n.qoi",
        "terraintex/mntn_dark_n.qoi",
        "terraintex/ground_dry2_n.qoi",
    };
 /*   
    const char *filesd[] = {
        "terraintex/grass_02_01.png",
        "terraintex/tile01.png",
        "terraintex/grass_02_01.png",
        "terraintex/field_04.png",
        "terraintex/stone01_01.png"
    };
   */ 

    sg_image imgd;
    sg_image imgs;
    sg_image imgn;
    //sg_image imgb;

    if (load_sg_image_array(filesd, &imgd, 5)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (load_sg_image_array(filess, &imgs, 5)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (load_sg_image_array(filesn, &imgn, 5)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (worldmap_init(&fi.map, "metin2_map_battlefied"))
        return -1;

    for (int i = 0; i < 4; ++i) {
        fi.terrainbind[i] = (sg_bindings){
            .vertex_buffers[0] = fi.map.vbuffers[i],
            .index_buffer = fi.map.ibuffers[i],
            .fs_images[SLOT_imgdiff] = imgd,
            .fs_images[SLOT_imgspec] = imgs,
            .fs_images[SLOT_imgnorm] = imgn,
            .fs_images[SLOT_imgblend] = fi.map.blendmap,
            //.fs_images[SLOT_imgblend] = imgb,
        };
    }

    return 0;
}

void terrain_set_shadowmap(sg_image shadowmap)
{
    for (int i = 0; i < 4; ++i)
        fi.terrainbind[i].fs_images[SLOT_shadowmap] = shadowmap;
}

void draw_terrain(struct frameinfo *fi, hmm_mat4 vp,
        hmm_vec3 lightpos, hmm_vec3 viewpos, hmm_mat4 lightmatrix)
{
    sg_apply_pipeline(fi->pipes.terrain);
    
    vs_params_t munis = {
        .vp = vp,
        .lightpos = lightpos,
        .lightpos_s = fi->dlight_dir,
        .viewpos = viewpos,
        .lightspace_matrix = lightmatrix,
    };

    fs_params_t fsparm = {
        .uambi = fi->dlight_ambi,
        .udiff = fi->dlight_diff,
        .uspec = fi->dlight_spec,
        .shadowmap_size = HMM_Vec2(2048.0f, 2048.0f),
    };

    int idx = 0;
    float tx = 1.0f / (float)fi->map.w;
    float ty = 1.0f / (float)fi->map.h;
    for (int y = 0; y < fi->map.h; ++y) {
        for (int x = 0; x < fi->map.w; ++x) {
            sg_apply_bindings(&fi->terrainbind[idx++]);
        
            munis.model = HMM_Translate(HMM_Vec3(fi->map.scale * x, 0.0f, fi->map.scale * y));
            munis.unormalmat = extrahmm_transpose_inverse_mat3(munis.model);
            fsparm.blendoffset = HMM_Vec2(ty * y, tx * x);
        
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(munis));
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(fsparm));
            
            sg_draw(0, 128*128*6, 1);
        }
    }
}

