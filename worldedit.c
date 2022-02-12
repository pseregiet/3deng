#include <SDL2/SDL.h>
#include "extrahmm.h"
#include "mouse2world.h"
#include "sdl2_imgui.h"
#include "frameinfo.h"
#include "objloader.h"
#include "staticmapobj.h"
#include "genshd_combo.h"

extern struct frameinfo fi;

enum editmode {
    EM_NONE,
    EM_PLACE_STATIC,
    EM_EDIT_STATIC,
    EM_PLACE_DYNAMIC,
    EM_EDIT_DYNAMIC
};

struct worldedit {
    const struct obj_model *model;
    hmm_vec3 pos;
    hmm_vec3 rot;
    hmm_vec3 scl;
    hmm_vec3 ray;
    hmm_vec3 ambi_flash;
    float flash_time;
    struct m2world m2;
    enum editmode mode;
} wedata = {0};

#define PRINTVEC3(vec) vec.X, vec.Y, vec.Z
static void gui_pick_staticobj()
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
                wedata.m2.obj.om = model;
                printf("I selected something\n");
                wedata.mode = EM_PLACE_STATIC;
            }
        }
        igEndListBox();
    }
    igText("Picked position: %f, %f, %f", PRINTVEC3(wedata.ray));
    igEnd();
}

static void update_pick_staticobj()
{
    const int WH = 720;
    const int WW = 1280;
    SDL_GetMouseState(&wedata.m2.mx, &wedata.m2.my);
    wedata.m2.ww = WW;
    wedata.m2.wh = WH;
    wedata.m2.projection = fi.projection;
    wedata.m2.view = fi.view;

    wedata.m2.cam = fi.cam.pos;
    wedata.m2.map = &fi.map;
    wedata.ray = mouse2ray(&wedata.m2);
    staticmapobj_setpos(&wedata.m2.obj, wedata.ray);
    
}

static void render_staticobj()
{
    hmm_vec3 ambi_backup = fi.dirlight.ambi;
    hmm_vec3 diff_backup = fi.dirlight.diff;
    fi.dirlight.ambi = wedata.ambi_flash;
    fi.dirlight.diff = wedata.ambi_flash;

    sg_apply_pipeline(fi.pipes.objmodel);
    objmodel_fraguniforms(&fi);
    const struct staticmapobj *obj = &wedata.m2.obj;
    objmodel_render(obj->om, &fi, obj->matrix);

    fi.dirlight.ambi = ambi_backup;
    fi.dirlight.diff = diff_backup;
}

void worldedit_imgui()
{
    gui_pick_staticobj();
}

void worldedit_render()
{
    if (wedata.mode == EM_PLACE_STATIC)
        render_staticobj();
}

void worldedit_update(double delta)
{
    wedata.flash_time += delta;
    if (wedata.flash_time >= 1.0f)
        wedata.flash_time = 0.0f;

    wedata.ambi_flash = HMM_MultiplyVec3f(fi.dirlight.ambi, wedata.flash_time);

    if (wedata.mode == EM_PLACE_STATIC)
        update_pick_staticobj();
    
}

static bool input_staticobj(SDL_Event *e)
{
    switch (e->type) {
        case SDL_MOUSEBUTTONDOWN: {
            staticmapobj_new(&wedata.m2.obj);
            wedata.mode = 0;
            return true;
            break;
        }
        default: {
            break;
        }
    }
    return false;
}

bool worldedit_input(SDL_Event *e)
{
    switch (wedata.mode) {
        case EM_PLACE_STATIC:
            return input_staticobj(e);
            break;
        default:
            break;
    }
    return false;
}

int worldedit_init()
{
    wedata.m2.obj.matrix = calc_matrix(
        HMM_Vec3(0.0f, 0.0f, 0.0f),
        HMM_Vec4(0.0f, 0.0f, 0.0f, 0.0f),
        HMM_Vec3(3.0f, 3.0f, 3.0f));
    return 0;
}

void worldedit_kill()
{
}
