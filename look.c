//------------------------------------------------------------------------------
//  super fucking amazing openGL3.3 engine
//  super quallity shit code by Patryk Seregiet (c) 2021-2022
//------------------------------------------------------------------------------
#include "sokolgl.h"
#include <SDL2/SDL.h>
#include "hmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sdl2_stuff.h"
#include "frameinfo.h"
#include "event.h"
#include "md5loader.h"
#include "objloader.h"
#include "terrain.h"
#include "mouse2world.h"
#include "shadow.h"
#include "hitbox.h"
#include "animodel_mngr.h"
#include "objloader.h"
#include "animatmapobj.h"
#include "staticmapobj.h"
#include "texloader.h"
#include "genshd_combo.h"
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

struct frameinfo fi = {
    .cam = (struct camera) {
        .yaw = -90.f,
        .pitch = 0,
        .pos = {0.0f, 0.0f, 3.0f},
        .front = { 0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .dir = {0.0f, 0.0f, 0.0f},
    },
    .dlight_dir =  (hmm_vec3){0.5f, 1.0f, 0.3f},
    .dlight_diff = (hmm_vec3){0.4f, 0.4f, 0.4f},
    .dlight_ambi = (hmm_vec3){0.05f, 0.05f, 0.05f},
    .dlight_spec = (hmm_vec3){0.5f, 0.5f, 0.5f},
};


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

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        fatalerror("gladLoadGLLoader failed\n");
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

static void imgui_static_objs()
{
    igBegin("Static objects", NULL, 0);
    static int new_selected = 0;
    if (igBeginListBox("Select model", (ImVec2){0.0f,0.0f})) {
        const int beg = objloader_beg();
        const int end = objloader_end();
        for (int i = beg; i < end; ++i) {
            const char *name;
            const struct obj_model *model;
            if (objloader_get(i, &name, &model))
                continue;

            const bool isselected = (new_selected == i);
            if (igSelectable_Bool(name, isselected, ImGuiSelectableFlags_None, (ImVec2){0.0f,0.0f})) {
                new_selected = i;
                m2.obj.om = model;
            }
        }
        igEndListBox();
    }
/*
    static int placed_selected = 0;
    if (igBeginListBox("Placed objects", (ImVec2){0.0f,0.0f})) {
        for (int i = 0; i < static_objs.count; ++i) {
            const bool isselected = (placed_selected == i);
            const struct model *model = static_objs.data[i].model;
            char name[0x20];
            sprintf(name, "some model %d", i);

            if (igSelectable_Bool(name, isselected, ImGuiSelectableFlags_None, (ImVec2){0.0f,0.0f}))
                placed_selected = i;
        }
        igEndListBox();
    }
*/  
    igEnd();
}

static void do_imgui_frame(int w, int h, double delta)
{
    simgui_new_frame(w, h, delta);
    static float f = 0.0f;

    igBegin("Hello world! window", NULL, 0);

    igText("Camera pos: %f, %f, %f", fi.cam.pos.X, fi.cam.pos.Y, fi.cam.pos.Z);
    igText("Camera dir: %f, %f, %f", fi.cam.front.X, fi.cam.front.Y, fi.cam.front.Z);
    igText("Light dir: %f, %f, %f", fi.dlight_dir.X, fi.dlight_dir.Y, fi.dlight_dir.Z);
    igColorEdit3("directional light diffuse", fi.dlight_diff.Elements, ImGuiColorEditFlags_DefaultOptions_);
    igColorEdit3("directional light ambient", fi.dlight_ambi.Elements, ImGuiColorEditFlags_DefaultOptions_);

    igCheckbox("Light enable1", &fi.lightsenable[0]);
    igCheckbox("Light enable2", &fi.lightsenable[1]);
    igCheckbox("Light enable3", &fi.lightsenable[2]);
    igCheckbox("Light enable4", &fi.lightsenable[3]);

    if (igButton("Set lightdir", (ImVec2) {100.0f, 100.0f})) {
        fi.dlight_dir = fi.cam.front;
    }

    m2.cam = fi.cam.pos;
    m2.map = &fi.map;
    hmm_vec3 mray = mouse2ray(&m2);
    staticmapobj_setpos(&m2.obj, mray);
    igText("Picked position: %f, %f, %f", mray.X, mray.Y, mray.Z);

    igText("App average %.3f ms/frame (%.1f FPS)", delta, 1000.0f / delta);
    igEnd();

    imgui_static_objs();
}

static void do_update_animated(double delta)
{
    int end = animatmapobj_mngr_end();
    struct animodel *mdls[end];

    //here we would do sorting, culling etc.
    for (int i = 0; i < end; ++i) {
        struct animatmapobj *obj = animatmapobj_mngr_get(i);
        mdls[i] = &obj->am;
    }
    animodel_mngr_calc_boneuv(mdls, end);

    for (int i = 0; i < end; ++i) {
        animodel_play(mdls[i], delta);
        animodel_plain(mdls[i]);
        //animodel_interpolate(mdls[i]);
        animodel_joint2matrix(mdls[i]);
    }
}

static void do_update(double delta)
{
    static float rottt = 0.0f;
    do_update_animated(delta / 1000.f);
    //rotate static objects...very fucking static, lol
    /*
    for (int i = 0; i < static_objs.count; ++i) {
        static_objs.data[i].matrix = HMM_MultiplyMat4(static_objs.data[i].matrix,
                (HMM_Rotate(HMM_ToRadians(rottt), HMM_Vec3(1.0f, 1.0f, 1.0f))));
        rottt += 0.01;
    }
    if (rottt >= 360.f)
        rottt=0.f;
    */
}

static void do_render_static_objs(struct frameinfo *fi, double delta, vs_params_t *munis)
{
    sg_apply_pipeline(fi->pipes.objmodel);
    //fs uniforms
    fs_params_t fs_params = {
        .viewpos = fi->cam.pos,
        .matshine = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &SG_RANGE(fs_params));

    fs_dir_light_t fs_dir_light = {
        .direction = fi->dlight_dir,
        .ambient = fi->dlight_ambi,
        .diffuse = fi->dlight_diff,
        .specular = fi->dlight_spec,
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
        .position = fi->cam.pos,
        .direction = fi->cam.front,
        .cutoff = HMM_COSF(HMM_ToRadians(12.5f)),
        .outcutoff = HMM_COSF(HMM_ToRadians(15.0f)),
        .attenuation = HMM_Vec3(1.0f, 0.09f, 0.032f),
        .ambient = HMM_Vec3(0.0f, 0.0f, 0.0f),
        .diffuse = HMM_Vec3(1.0f, 1.0f, 1.0f),
        .specular = HMM_Vec3(1.0f, 1.0f, 1.0f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_spot_light, &SG_RANGE(fs_spot_light));

    const int end = staticmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct staticmapobj *obj = staticmapobj_get(i);
        const struct obj_model *mdl = obj->om;
        sg_bindings bind = {
            .vertex_buffers[0] = mdl->vbuf,
            .fs_images[SLOT_imgdiff] = mdl->imgdiff,
            .fs_images[SLOT_imgspec] = mdl->imgspec,
            //.fs_images[SLOT_imgnorm] = mdl->imgnorm,
        };
        sg_apply_bindings(&bind);
        
        munis->model = obj->matrix;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(*munis));
        sg_draw(0, mdl->vcount, 1);
    }

    if (m2.obj.om) {
        struct staticmapobj *obj = &m2.obj;
        const struct obj_model *mdl = obj->om;
        sg_bindings bind = {
            .vertex_buffers[0] = mdl->vbuf,
            .fs_images[SLOT_imgdiff] = mdl->imgdiff,
            .fs_images[SLOT_imgspec] = mdl->imgspec,
            //.fs_images[SLOT_imgnorm] = mdl->imgnorm,
        };
        sg_apply_bindings(&bind);
        
        munis->model = obj->matrix;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(*munis));
        sg_draw(0, mdl->vcount, 1);
    }

    simpleobj_hitbox_render(fi, munis->vp);
}

static void do_render_lightcubes(struct frameinfo *fi, double delta, vs_params_t *munis)
{
    sg_apply_pipeline(fi->pipes.lightcube);
    sg_apply_bindings(&fi->lightbind);
    hmm_mat4 scale = HMM_Scale(HMM_Vec3(0.1, 0.1, 0.1));
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
        model = HMM_MultiplyMat4(munis->vp, model);
        vs_paramsl_t lvs = {model};

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(lvs));
        sg_draw(0, 36, 1);
    }
}

static void do_render_animat_objs(struct frameinfo *fi, double delta)
{
    animodel_mngr_upload_bones();
    sg_apply_pipeline(fi->pipes.animodel);

    int end = animatmapobj_mngr_end();
    for (int i = 0; i < end; ++i) {
        struct animatmapobj *obj = animatmapobj_mngr_get(i);
        animodel_render(&obj->am, fi, obj->matrix);
    }
}

static void do_render(struct frameinfo *fi, double delta)
{
    hmm_mat4 projection = HMM_Perspective(75.0f, (float)WW/(float)WH, 0.1f, 2000.0f);

    int w, h;
    SDL_GL_GetDrawableSize(sdl.win, &w, &h);
   
    shadowmap_draw();

    sg_begin_default_pass(&fi->pass_action, w, h);
    hmm_mat4 view = HMM_LookAt(fi->cam.pos, HMM_AddVec3(fi->cam.pos, fi->cam.front), fi->cam.up);
    hmm_mat4 vp = HMM_MultiplyMat4(projection, view);
    fi->vp = vp;
    vs_params_t munis = { .vp = vp };
    draw_terrain(fi, vp, fi->dlight_dir, fi->cam.pos, fi->shadow.lightspace);
 
    do_render_static_objs(fi, delta, &munis);
    do_render_lightcubes(fi, delta, &munis);
    do_render_animat_objs(fi, delta);

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

static int imgdummy_init()
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

    return 0;
}
static void imgdummy_kill()
{
    sg_destroy_image(imgdummy);
}

int main(int argc, char **argv)
{
    if (sdl_init()) {
        fatalerror("sdl_init() failed\n");
        return -1;
    }

    uint32_t init_start = SDL_GetTicks();
    assert(!imgdummy_init());
    assert(!texloader_init());
    assert(!pipelines_init(&fi.pipes));
    assert(!init_terrain());
    assert(!shadowmap_init());
    assert(!md5loader_init());
    assert(!objloader_init());
    assert(!animodel_mngr_init());
    assert(!animatmapobj_mngr_init());
    assert(!staticmapobj_mngr_init());

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
    
    fi.lightbind = (sg_bindings){
        .vertex_buffers[0] = lvbuf,
    };

    fi.pass_action = (sg_pass_action){
        .colors[0] = {.action = SG_ACTION_CLEAR, .value = {0.5, 0.5, 0.5, 1.0 }},
    };

    terrain_set_shadowmap(fi.shadow.depthmap);
    m2.obj.matrix = calc_matrix(
            HMM_Vec3(0.0f, 0.0f, 0.0f),
            HMM_Vec4(0.0f, 0.0f, 0.0f, 0.0f),
            HMM_Vec3(10.0f, 10.0f, 10.0f));

    double init_done = (double)(SDL_GetTicks() - init_start);
    printf("%.3fs init\n", init_done / 1000.0f);

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0;
    double delta = 0;
    while (!gquit) {
        last = now;
        now = SDL_GetPerformanceCounter();
        delta = ((now - last)*1000 / (double)SDL_GetPerformanceFrequency());

        do_input(delta);
        do_update(delta);
        do_render(&fi, delta);

    }

    staticmapobj_mngr_kill();
    animatmapobj_mngr_kill();
    animodel_mngr_kill();
    objloader_kill();
    md5loader_kill();
    shadowmap_kill();
    //kill_terrain();
    pipelines_kill(&fi.pipes);
    texloader_kill();
    imgdummy_kill();
    

    igsdl2_Shutdown();
    sg_shutdown();
    SDL_GL_DeleteContext(sdl.ctx);
    SDL_DestroyWindow(sdl.win);
    SDL_Quit();

    return 0;
}

