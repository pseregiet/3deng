#include "objloader.h"
#include "growing_allocator.h"
#include "khash.h"
#include "kvec.h"
#include "json_helpers.h"
#include <assert.h>
#include <stdio.h>

KHASH_MAP_INIT_STR(modelmap, struct obj_model)

static struct models {
    struct growing_alloc names;
    khash_t(modelmap) map;
} models;

#define DEFAULT_OBJ_COUNT 16

static int append_model(const char *fp, const char *name)
{
    struct obj_model mdl = {0};
    if (objmodel_open(fp, &mdl)) {
        return -1;
    }

    char *newname = addname(&models.names, name);
    int ret;
    khint_t idx = kh_put(modelmap, &models.map, newname, &ret);
    kh_value(&models.map, idx) = mdl;

    return 0;
}

static int load_model(const char *model)
{
    if (!model)
        return -1;

    char fp[0x1000];
    snprintf(fp, 0x1000, "data/models/obj/%s.3do", model);
    khint_t idx = kh_get(modelmap, &models.map, model);

    if (idx != kh_end(&models.map)) {
        printf("Model %s already exists, skip\n", model);
        return 0;
    }

    return append_model(fp, model);
}

static int objloader_json()
{
    const char *fn = "objmodels";
    struct file jf;
    cJSON *json;
    cJSON *root;
    int ret = -1;

    if (json_start(fn, &json, &root, &jf))
        return -1;

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
    closefile(&jf);
    return ret;
}

struct obj_model *objloader_find(const char *name)
{
    khint_t idx = kh_get(modelmap, &models.map, name);
    if (idx == kh_end(&models.map))
        return 0;

    return &kh_val(&models.map, idx);
}

int objloader_init()
{
    assert(!growing_alloc_init(&models.names, 0, 1));

    memset(&models.map, 0, sizeof(models.map));
    kh_resize(modelmap, &models.map, DEFAULT_OBJ_COUNT);
    return objloader_json();
}

void objloader_kill()
{
    khint_t i;
    for (i = kh_begin(&models.map); i != kh_end(&models.map); ++i) {
        if (!kh_exist(&models.map, i))
            continue;

        objmodel_kill(&kh_val(&models.map, i));
    }

    kh_destroy(modelmap, &models.map);
    growing_alloc_kill(&models.names);
}

int objloader_get(int idx, const char **name, const struct obj_model **mdl)
{
    khint_t i = idx;
    if (!kh_exist(&models.map, i))
        return -1;

    *mdl = &kh_val(&models.map, i);
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
