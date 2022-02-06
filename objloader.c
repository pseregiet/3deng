#include "objloader.h"
#include "growing_allocator.h"
#include "fileops.h"
#include "khash.h"
#include "kvec.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>

KHASH_MAP_INIT_STR(modelmap, struct obj_model *)

static struct models {
    struct growing_alloc names;
    kvec_t(struct obj_model) models;
    khash_t(modelmap) map;
} models;

#define MODELS_DEF_COUNT 10

inline static char *addname(struct growing_alloc *alloc, const char *name)
{
    int namelen = strlen(name);
    char *newname = growing_alloc_get(alloc, namelen+1);
    assert(newname);
    memcpy(newname, name, namelen);
    newname[namelen] = 0;
    return newname;
}

static int append_model(const char *fp, const char *name)
{
    struct obj_model mdl;
    if (objmodel_open(fp, &mdl)) {
        return -1;
    }

    kv_push(struct obj_model, models.models, mdl);

    char *newname = addname(&models.names, name);
    int ret;
    khint_t idx = kh_put(modelmap, &models.map, newname, &ret);
    kh_value(&models.map, idx) = &kv_A(models.models, kv_size(models.models)-1);

    return 0;
}

static int load_model(const char *model)
{
    if (!model)
        return -1;

    char fp[0x1000];
    snprintf(fp, 0x1000, "data/models/obj/%s/mesh.obj", model);
    khint_t idx = kh_get(modelmap, &models.map, model);

    if (idx != kh_end(&models.map)) {
        printf("Model %s already exists, skip\n", model);
        return 0;
    }

    return append_model(fp, model);
}

static int parse_json()
{
    const char *fn = "data/objmodels.json";
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "objmodels");
    if (!root || root->type != cJSON_Array) {
        printf("no objmodels array found\n");
        goto freejson;
    }

    cJSON *mdl = 0;
    int mdlcount = 0;
    cJSON_ArrayForEach(mdl, root) {
        mdlcount++;
        cJSON *jmodel = cJSON_GetObjectItem(mdl, "model");
        if (!jmodel) {
            printf("%s: node %d is bad\n", fn, mdlcount);
            goto freejson;
        }

        if (load_model(jmodel->valuestring))
            goto freejson;
    }

    ret = 0;
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

struct obj_model *objloader_find(const char *name)
{
    khint_t idx = kh_get(modelmap, &models.map, name);
    if (idx == kh_end(&models.map))
        return 0;

    return kh_val(&models.map, idx);
}

int objloader_init()
{
    assert(!growing_alloc_init(&models.names, 0, 1));
    kv_init(models.models);
    kv_resize(struct obj_model, models.models, MODELS_DEF_COUNT);
    cube_index_buffer_init();
    return parse_json();
}

void objloader_kill()
{
    growing_alloc_kill(&models.names);
    for (int i = 0; i < kv_size(models.models); ++i) {
        struct obj_model *obj = &kv_A(models.models, i);
        if (obj)
            objmodel_kill(obj);
    }
    kv_destroy(models.models);
    cube_index_buffer_kill();
}

int objloader_get(int idx, const char **name, const struct obj_model **mdl)
{
    khint_t i = idx;
    if (!kh_exist(&models.map, i))
        return -1;

    *mdl = kh_val(&models.map, i);
    *name = kh_key(&models.map, i);

    return 0;
}

int objloader_beg()
{
    return (int)kh_begin(&models.map);
}

int objloader_end()
{
    return (int)kh_end(&models.map);
}
