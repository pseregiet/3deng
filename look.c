//------------------------------------------------------------------------------
//  1-9-3-look
//------------------------------------------------------------------------------
#define SOKOL_NO_SOKOL_APP
#include "../sokol/sokol_gfx.h"

#include <GL/gl.h>
#include <SDL2/SDL.h>
#include "hmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdl2_stuff.h"
#include "frameinfo.h"
#include "event.h"
#include "objloader.h"
#include "static_object.h"
#include "terrain.h"
#include "mouse2world.h"
#include "shadow.h"
#include "genshader.h"
#include "sdl2_imgui.h"

#define u16 uint16_t

#define WW 1280
#define WH 720

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

extern struct frameshadow shadow;

sg_image imgdummy = {0};
struct sdlobjs sdl = {0};

bool gquit = 0;
struct matrix_unis {
    hmm_mat4 model;
    hmm_mat4 view;
    hmm_mat4 projection;
};

struct camera cam = {
    .yaw = -90.f,
    .pitch = 0,
    .pos = {0.0f, 0.0f, 3.0f},
    .front = { 0.0f, 0.0f, -1.0f},
    .up = {0.0f, 1.0f, 0.0f},
    .dir = {0.0f, 0.0f, 0.0f},
};

struct frameinfo fi = {0};

hmm_vec3 dirlight_diff = {0.4f, 0.4f, 0.4f};
hmm_vec3 dirlight_ambi = {0.05f, 0.05f, 0.05f};
hmm_vec3 ldir = {0.5f, 1.0f, 0.3f};

struct m2world m2 = {0};

hmm_vec4 lightspos[4] = {
    {0.7f, 0.2f, 2.0f, 1.0f},
    {2.3f, -3.3f, -4.0f, 1.0f},
    {-4.0f, 2.0f, 12.0f, 1.0f},
    {0.0f, 0.0f, -3.0f, 1.0f}
};

static int sdl_init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        fatalerror("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdl.win = SDL_CreateWindow("SDL2 x sokol_gfx", 500, 500, WW, WH, SDL_WINDOW_OPENGL);
    if (!sdl.win) {
        fatalerror("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    sdl.ctx = SDL_GL_CreateContext(sdl.win);
    if (!sdl.ctx) {
        fatalerror("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return -1;
    }

    sg_setup(&(sg_desc) {0});
    if (!sg_isvalid()) {
        fatalerror("sg_setup failed\n");
        return -1;
    }

    //SDL_GL_SetSwapInterval(0);

    igsdl2_Init();

    gquit = 0;

    return 0;
}

static inline float bits2float(const bool *bits)
{
    uint32_t in = 0;

    for (int i = 0; i < 32; ++i)
        if (bits[i])
            in |= (1 << i);

    return (float)in;
}

#define MAX_RAY 600

static void imgui_static_objs()
{
    igBegin("Static objects", NULL, 0);
    static int selected = 0;
    if (igBeginListBox("Select model", (ImVec2){0.0f,0.0f})) {
        for (int i = 1; i < 3; ++i) {
            const bool isselected = (selected == i);
            const char *name = 0;
            const struct model *model = 0;
            vmodel_get_key_value(i, &model, &name);

            if (igSelectable_Bool(name, isselected, ImGuiSelectableFlags_None, (ImVec2){0.0f,0.0f}))
                selected = i;
        }
        igEndListBox();
    }
    
    igText("selected: %d", selected);
    igEnd();
}

static void do_imgui_frame(int w, int h, double delta)
{
    simgui_new_frame(w, h, delta);
    static float f = 0.0f;

    igBegin("Hello world! window", NULL, 0);

    igText("Camera pos: %f, %f, %f", cam.pos.X, cam.pos.Y, cam.pos.Z);
    igText("Camera dir: %f, %f, %f", cam.front.X, cam.front.Y, cam.front.Z);
    igText("Light dir: %f, %f, %f", ldir.X, ldir.Y, ldir.Z);
    igColorEdit3("directional light diffuse", dirlight_diff.Elements, ImGuiColorEditFlags_DefaultOptions_);
    igColorEdit3("directional light ambient", dirlight_ambi.Elements, ImGuiColorEditFlags_DefaultOptions_);

    igCheckbox("Light enable1", &fi.lightsenable[0]);
    igCheckbox("Light enable2", &fi.lightsenable[1]);
    igCheckbox("Light enable3", &fi.lightsenable[2]);
    igCheckbox("Light enable4", &fi.lightsenable[3]);

    if (igButton("Set lightdir", (ImVec2) {100.0f, 100.0f})) {
        ldir = cam.front;
    }

    m2.cam = cam.pos;
    m2.map = &fi.map;
    hmm_vec3 mray = mouse2ray(&m2);
    static_obj_set_position(9, mray);
    igText("Picked position: %f, %f, %f", mray.X, mray.Y, mray.Z);

    igText("App average %.3f ms/frame (%.1f FPS)", delta, 1000.0f / delta);
    igEnd();

    imgui_static_objs();
}

float rottt = 0.0f;
static void do_frame(struct frameinfo *fi, double delta)
{
    //rotate static objects...very fucking static, lol
    for (int i = 0; i < static_objs.count - 1; ++i) {
        static_objs.data[i].matrix = HMM_MultiplyMat4(static_objs.data[i].matrix,
                (HMM_Rotate(HMM_ToRadians(rottt), HMM_Vec3(1.0f, 1.0f, 1.0f))));
        rottt += 0.01;
    }
    if (rottt >= 360.f)
        rottt=0.f;

    hmm_mat4 projection = HMM_Perspective(75.0f, (float)WW/(float)WH, 0.1f, 2000.0f);

    int w, h;
    SDL_GL_GetDrawableSize(sdl.win, &w, &h);
   
    shadow_draw();
    sg_end_pass();

    sg_begin_default_pass(&fi->pass_action, w, h);
    
    hmm_mat4 view = HMM_LookAt(cam.pos, HMM_AddVec3(cam.pos, cam.front), cam.up);
    hmm_mat4 vp = HMM_MultiplyMat4(projection, view);

    vs_params_t munis = {
        .vp = vp,
    };

    draw_terrain(fi, vp, ldir, cam.pos, shadow.lightspace);
    sg_apply_pipeline(fi->mainpip);

    //fs uniforms
    fs_params_t fs_params = {
        .viewpos = cam.pos,
        .matshine = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &SG_RANGE(fs_params));

    fs_dir_light_t fs_dir_light = {
        .direction = ldir,
        .ambient = dirlight_ambi,
        .diffuse = dirlight_diff,
        .specular = HMM_Vec3(0.5f, 0.5f, 0.5f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_dir_light, &SG_RANGE(fs_dir_light));

    fs_point_lights_t fs_point_lights = {
        .position[0]    = lightspos[0],
        .ambient[0]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[0]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[0]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[0] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[1]    = lightspos[1],
        .ambient[1]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[1]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[1]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[1] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[2]    = lightspos[2],
        .ambient[2]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[2]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[2]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[2] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .position[3]    = lightspos[3],
        .ambient[3]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
        .diffuse[3]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
        .specular[3]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
        .attenuation[3] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
        .enabled = bits2float((bool *)&fi->lightsenable),
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_point_lights, &SG_RANGE(fs_point_lights));

    fs_spot_light_t fs_spot_light = {
        .position = cam.pos,
        .direction = cam.front,//HMM_AddVec3(cam.pos, cam.front),
        .cutoff = HMM_COSF(HMM_ToRadians(12.5f)),//cosf(12.5f),
        .outcutoff = HMM_COSF(HMM_ToRadians(15.0f)),//cosf(15.0f),
        .attenuation = HMM_Vec3(1.0f, 0.09f, 0.032f),
        .ambient = HMM_Vec3(0.0f, 0.0f, 0.0f),
        .diffuse = HMM_Vec3(1.0f, 1.0f, 1.0f),
        .specular = HMM_Vec3(1.0f, 1.0f, 1.0f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_spot_light, &SG_RANGE(fs_spot_light));


    hmm_mat4 scale;

    for (int i = 0; i < static_objs.count; ++i) {
        obj_bind(static_objs.data[i].model, &fi->mainbind);
        sg_apply_bindings(&fi->mainbind);
        
        munis.model = static_objs.data[i].matrix;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(munis));
        sg_draw(0, static_objs.data[i].model->vcount, 1);
    }

    sg_apply_pipeline(fi->lightpip);
    sg_apply_bindings(&fi->lightbind);
    scale = HMM_Scale(HMM_Vec3(0.1, 0.1, 0.1));
    for (int i = 0; i < 4; ++i) {

        if (!fi->lightsenable[i])
            continue;

        hmm_vec3 pos = {
            lightspos[i].X,
            lightspos[i].Y,
            lightspos[i].Z
        };
        hmm_mat4 model = HMM_Translate(pos);
        model = HMM_MultiplyMat4(model, scale);
        model = HMM_MultiplyMat4(vp, model);
        vs_paramsl_t lvs = {model};

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(lvs));
        sg_draw(0, 36, 1);
    }

    SDL_GetMouseState(&m2.mx, &m2.my);
    m2.ww = WW;
    m2.wh = WH;
    m2.projection = projection;
    m2.view = view;

    if (sdl.imguifocus) {
        do_imgui_frame(w, h, delta);
        simgui_render();
    }

    sg_end_pass();
    sg_commit();
    SDL_GL_SwapWindow(sdl.win);
}

static sg_image init_imgdummy()
{
    char data[4] = {0,0,0,0};

    imgdummy = sg_make_image(&(sg_image_desc) {
        .width = 1,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = data,
            .size = 4,
        },
    });
}

int main(int argc, char **argv)
{
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    init_imgdummy();
    vmodel_init();

    if (init_static_objects()) {
        fatalerror("couldn't load the fucking static objects\n");
        return -1;
    }

    sg_shader shd = sg_make_shader(comboshader_shader_desc(SG_BACKEND_GLCORE33));
    sg_shader lightshd = sg_make_shader(light_cube_shader_desc(SG_BACKEND_GLCORE33));

    fi.mainpip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = shd,
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
                [ATTR_vs_position] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_normal] = {.format = SG_VERTEXFORMAT_FLOAT3},
                [ATTR_vs_texcoord] = {.format = SG_VERTEXFORMAT_FLOAT2},
            },
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_FRONT,
    });


    float vertices[36*3] =
    {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, 
        0.5f,  0.5f, -0.5f, 
        0.5f,  0.5f, -0.5f, 
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f, -0.5f, 
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, 
        0.5f, -0.5f,  0.5f, 
        0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f, 
        0.5f,  0.5f,  0.5f, 
        0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,

    };

    sg_buffer lvbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(vertices),
        .data.ptr = vertices,
    });
    
    fi.lightpip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = lightshd,
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

    fi.lightbind = (sg_bindings){
        .vertex_buffers[0] = lvbuf,
    };

    fi.pass_action = (sg_pass_action){
        .colors[0] = {.action = SG_ACTION_CLEAR, .value = {0.0, 0.0, 0.0, 1.0 }},
    };

    init_terrain();
    init_shadow();
    terrain_set_shadowmap(shadow.depthmap);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0;
    double delta = 0;
    while (!gquit) {
        last = now;
        now = SDL_GetPerformanceCounter();
        delta = ((now - last)*1000 / (double)SDL_GetPerformanceFrequency());

        do_input(delta);
        do_frame(&fi, delta);

    }

    igsdl2_Shutdown();
    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;

}
