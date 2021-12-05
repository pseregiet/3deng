#include "terrain.h"
#include "texloader.h"
#include "genshd_terrain.h"
#include "hmm.h"
#include <stdio.h>
#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

extern struct frameinfo fi;

static hmm_vec3 computeTangent(hmm_vec3 pos0, hmm_vec3 pos1, hmm_vec3 pos2, hmm_vec2 uv0, hmm_vec2 uv1, hmm_vec2 uv2) {
    hmm_vec3 edge0 = HMM_SubtractVec3(pos1, pos0);
    hmm_vec3 edge1 = HMM_SubtractVec3(pos2, pos0);
    hmm_vec2 delta_uv0 = HMM_SubtractVec2(uv1, uv0);
    hmm_vec2 delta_uv1 = HMM_SubtractVec2(uv2, uv0);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);

    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

int init_terrain()
{
    const char *filesd[] = {
        "terraintex/grass_autumn_d.jpg",
        "terraintex/mntn_dark_d.jpg",
        "terraintex/snow_mud_d.jpg",
    };
    const char *filess[] = {
        "terraintex/grass_autumn_s.jpg",
        "terraintex/mntn_dark_s.jpg",
        "terraintex/snow_mud_s.jpg",
    };
    const char *filesn[] = {
        "terraintex/grass_autumn_n.jpg",
        "terraintex/mntn_dark_n.jpg",
        "terraintex/snow_mud_n.jpg",
    };
    sg_image imgd;
    sg_image imgs;
    sg_image imgn;
    sg_image imgb;

    if (load_sg_image_array(filesd, &imgd, 3)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (load_sg_image_array(filess, &imgs, 3)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (load_sg_image_array(filesn, &imgn, 3)) {
        fatalerror("can't load terrain textures\n");
        return -1;
    }

    if (load_sg_image("terraintex/blend.png", &imgb)) {
        fatalerror("can't load terrain blend\n");
        return -1;
    }

    /* positions */
    hmm_vec3 pos0 = HMM_Vec3(-25.f,  25.f, 0.f);
    hmm_vec3 pos1 = HMM_Vec3(-25.f, -25.f, 0.f);
    hmm_vec3 pos2 = HMM_Vec3( 25.f, -25.f, 0.f);
    hmm_vec3 pos3 = HMM_Vec3( 25.f,  25.f, 0.f);
    /* texture coordinates */
    hmm_vec2 uv0 = HMM_Vec2(0.f, 1.f);
    hmm_vec2 uv1 = HMM_Vec2(0.f, 0.f);
    hmm_vec2 uv2 = HMM_Vec2(1.f, 0.f);  
    hmm_vec2 uv3 = HMM_Vec2(1.f, 1.f);
    /* normal vector */
    hmm_vec3 nm = HMM_Vec3(0.f, 0.f, 1.f);
    /* tangents */
    hmm_vec3 ta0 = computeTangent(pos0, pos1, pos2, uv0, uv1, uv2);
    hmm_vec3 ta1 = computeTangent(pos0, pos2, pos3, uv0, uv2, uv3);

    /* we can compute the bitangent in the vertex shader by taking 
       the cross product of the normal and tangent */
    float quad_vertices[] = {
        // positions            // normal         // texcoords  // tangent           
        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta0.X, ta0.Y, ta0.Z,
        pos1.X, pos1.Y, pos1.Z, nm.X, nm.Y, nm.Z, uv1.X, uv1.Y, ta0.X, ta0.Y, ta0.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta0.X, ta0.Y, ta0.Z,

        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta1.X, ta1.Y, ta1.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta1.X, ta1.Y, ta1.Z,
        pos3.X, pos3.Y, pos3.Z, nm.X, nm.Y, nm.Z, uv3.X, uv3.Y, ta1.X, ta1.Y, ta1.Z
    };
    
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(quad_vertices),
        .data.ptr = quad_vertices,
    });

    fi.terrainbind = (sg_bindings){
        .vertex_buffers[0] = vbuf,
        .fs_images[SLOT_imgdiff] = imgd,
        .fs_images[SLOT_imgspec] = imgs,
        .fs_images[SLOT_imgnorm] = imgn,
        .fs_images[SLOT_imgblend] = imgb,
    };
    
    sg_shader terrainshd = sg_make_shader(terrainshd_shader_desc(SG_BACKEND_GLCORE33));

    fi.terrainpip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = terrainshd,
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
        //.cull_mode = SG_CULLMODE_FRONT,
    });

    return 0;
}

extern hmm_vec3 dirlight_ambi;
extern hmm_vec3 dirlight_diff;

void draw_terrain(struct frameinfo *fi, hmm_mat4 vp, hmm_mat4 model,
        hmm_vec3 lightpos, hmm_vec3 viewpos)
{
    sg_apply_pipeline(fi->terrainpip);
    sg_apply_bindings(&fi->terrainbind);

    hmm_mat4 rotat =  HMM_Rotate(90, HMM_Vec3(1.0f, 0.0f, 0.0f));
    model = HMM_MultiplyMat4(model, rotat);
    
    vs_params_t munis = {
        .vp = vp,
        .model = model,
        .lightpos = lightpos,
        .viewpos = viewpos,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(munis));
    
    fs_params_t fsparm = {
        .uambi = dirlight_ambi,
        .udiff = dirlight_diff,
        .uspec = HMM_Vec3(0.5f, 0.5f, 0.5f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &SG_RANGE(fsparm));
    sg_draw(0, 6, 1);
}
