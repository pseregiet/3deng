#include <assert.h>
#include "static_object.h"

struct vector_static_object static_objs;

//TODO: put these in a file
static hmm_vec3 staticpos[10] = {
    {.X= 409.0f,  .Y=350.0f, .Z= 580.f},
    {.X= 419.0f,  .Y=355.0f, .Z= 590.0f},
    {.X= 429.5f,  .Y=352.2f, .Z= 600.f},
    {.X= 439.8f,  .Y=352.0f, .Z= 610.3f},
    {.X= 449.4f,  .Y=350.4f, .Z= 620.f},
    {.X= 459.7f,  .Y=353.0f, .Z= 630.f},
    {.X= 469.3f,  .Y=352.0f, .Z= 640.f},
    {.X= 479.5f,  .Y=352.0f, .Z= 650.f},
    {.X= 489.5f,  .Y=350.2f, .Z= 660.f},
    {.X= 499.3f,  .Y=351.0f, .Z= 670.f},
};

static hmm_vec3 staticscale[10] = {
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
    {.X= 10.0f,    .Y= 10.0f,    .Z= 10.0f},
};

static hmm_vec4 staticrot[10] = {
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
    {.X= 1.0f,    .Y= 1.0f,    .Z= 1.0f,    .W= 90.0f},
};

inline static void vector_init(int size)
{
    static_objs.data = malloc(size * sizeof(struct static_object));
    assert(static_objs.data);
    static_objs.cap = size;
    static_objs.count = 0;
}

inline static void vector_resize(int size)
{
    if (!size)
        size = static_objs.cap * 2;
    else
        size += static_objs.cap;

    size *= sizeof(struct static_object *);
    static_objs.data = realloc(static_objs.data, size);
    assert(static_objs.data);
    static_objs.cap = size;
}

inline static void vector_append(struct static_object *obj)
{
    if (static_objs.count >= static_objs.cap)
        vector_resize(0);

    static_objs.data[static_objs.count++] = *obj;
}

inline static hmm_mat4 obj_calc_matrix(hmm_vec3 pos, hmm_vec4 rotation, hmm_vec3 scale)
{
    hmm_mat4 t = HMM_Translate(pos);
    hmm_mat4 r = HMM_Rotate(rotation.W, rotation.XYZ);
    hmm_mat4 s = HMM_Scale(scale);

    hmm_mat4 tmp = HMM_MultiplyMat4(t, r);
    return HMM_MultiplyMat4(tmp, s);
}

int init_static_objects()
{
    vector_init(10);
    //TODO, reading from a file or something
    for (int i = 0; i < 10; ++i) {
        struct static_object obj = {
            .model = vmodel_find("trump/untitled-scene.obj"),
            .matrix = obj_calc_matrix(staticpos[i], staticrot[i], staticscale[i]),
        };
        vector_append(&obj);
    }

    return 0;
}

