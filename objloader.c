#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "objloader.h"
#include "texloader.h"
#include "fast_obj.h"
#include "hmm.h"
#include "genshader.h"
#include "khash.h"

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)
extern sg_image imgdummy;
static float *vbuf = 0;
static int vbufsize = 0;

KHASH_MAP_INIT_STR(vmodelmap, struct model *)
static khash_t(vmodelmap) vmodel;
static khash_t(vmodelmap) *vmodelptr;

static struct vmodel_data {
    struct model *data;
    char *names;
    int datasize;
    int namessize;
    int datacap;
    int namescap;
} vmodel_data;

void vmodel_init()
{
    //TODO: get a real number of models or something
    int datacap = 10 * sizeof(struct model *);
    int namescap = 0x1000;
    vmodel_data.data = malloc(datacap);
    vmodel_data.names = malloc(namescap);
    vmodel_data.datacap = datacap;
    vmodel_data.namescap = namescap;
    vmodel_data.datasize = 0;
    vmodel_data.namessize = 0;

    vmodelptr = &vmodel;
}

struct model *vmodel_find(const char *key)
{
    struct model *model = 0;
    khint_t idx = kh_get(vmodelmap, vmodelptr, key);
    if (idx != kh_end(vmodelptr))
        model = kh_val(vmodelptr, idx);

    return model;
}

inline static void vmodel_resize_data(int size)
{
    if (!size)
        size = vmodel_data.datacap * 2;
    else
        size += vmodel_data.datacap;

    vmodel_data.data = realloc(vmodel_data.data, size * sizeof(struct model *));
    assert(vmodel_data.data);
    vmodel_data.datacap = size;
}

inline static void vmodel_resize_names(int size)
{
    if (!size)
        size = vmodel_data.namescap * 2;
    else
        size += vmodel_data.namescap;

    vmodel_data.names = realloc(vmodel_data.names, size);
    assert(vmodel_data.names);
    vmodel_data.namescap = size;
}

inline static void vmodel_append(const char *key, struct model *model)
{
    int namelen = strlen(key);
    int namecap = vmodel_data.namessize;
    if (namecap + namelen + 2 >= vmodel_data.namescap)
        vmodel_resize_names(0);

    strcpy(&vmodel_data.names[namecap], key);
    namelen = namecap + namelen + 1;
    vmodel_data.names[namelen] = 0;
    vmodel_data.namessize = namelen + 1;

    int datacap = vmodel_data.datasize;
    if (datacap + 1 >= vmodel_data.datacap)
        vmodel_resize_data(0);

    vmodel_data.data[datacap] = *model;
    vmodel_data.datasize++;

    int ret;
    khint_t idx = kh_put(vmodelmap, vmodelptr, &vmodel_data.names[namecap], &ret);
    kh_value(vmodelptr, idx) = model;
}

/*
    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = sizeof(indices),
        .data.ptr = indices,
        .type = SG_BUFFERTYPE_INDEXBUFFER,
    });
*/

static int parse_obj_vertex(struct model *model, fastObjMesh *mesh)
{
    int facecount = mesh->face_count;
    printf("%d\n", facecount*3);
    int datasize = facecount * 3 * 8 * sizeof(float);
    if (datasize > vbufsize) {
        vbuf = (float *)realloc((void *)vbuf, datasize);
        if (!vbuf) {
            fatalerror("parse_obj out of memory\n");
            return -1;
        }
        vbufsize = datasize;
    }

    printf("facecount: %d, x3: %d\n", mesh->face_count, mesh->face_count*3);

    if (mesh->position_count != mesh->texcoord_count ||
            mesh->position_count != mesh->normal_count ||
            mesh->texcoord_count != mesh->normal_count)
    {
        printf("Non equal amount of vertex atributes: %d, %d, %d\n",
                mesh->position_count, mesh->normal_count, mesh->texcoord_count);
    }

    for (int i = 0; i < facecount *3; ++i) {
        fastObjIndex vertex = mesh->indices[i];

        int pos = i * 8;
        int vpos = vertex.p * 3;
        int npos = vertex.n * 3;
        int tpos = vertex.t * 2;

        memcpy(&vbuf[pos + 0], mesh->positions + vpos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 3], mesh->normals + npos, 3 * sizeof(float));
        memcpy(&vbuf[pos + 6], mesh->texcoords + tpos, 2 * sizeof(float));
    }
    
    model->buffer = sg_make_buffer(&(sg_buffer_desc) {
        .data.size = datasize,
        .data.ptr = (const void *)vbuf,
    });

    model->vcount = mesh->face_count * 3;

    return 0;
}

static int get_material(sg_image *img, const char *fn)
{
    if (!fn)
        return -1;

    int ret = load_sg_image(fn, img);
    return (ret != 0);
}

static void parse_obj_material(struct model *model, fastObjMesh *mesh)
{
    sg_image imgdiff = imgdummy;
    sg_image imgspec = imgdummy;
    sg_image imgbump = imgdummy;

    get_material(&imgdiff, mesh->materials[0].map_Kd.path);
    get_material(&imgspec, mesh->materials[0].map_Ks.path);
    //get_material(&imgbump, mesh->materials[0].map_bump.name);
    
    model->imgdiff = imgdiff;
    model->imgspec = imgspec;
    //model->imgbump = imgbump;
}

int obj_load(struct model *model, const char *fn)
{
    fastObjMesh *mesh = fast_obj_read(fn);
    if (!mesh) {
        fatalerror("model %s couldn't be loaded\n", fn);
        return -1;
    }

    if (parse_obj_vertex(model, mesh))
        return -1;

    parse_obj_material(model, mesh);
    vmodel_append(fn, model);

    return 0;
}

void obj_bind(struct model *model, sg_bindings *bind)
{
    bind->vertex_buffers[0] = model->buffer;
    bind->fs_images[SLOT_imgdiff] = model->imgdiff;
    bind->fs_images[SLOT_imgspec] = model->imgspec;
    //bind->fs_images[SLOT_imgbump] = model->imgbump;

}

