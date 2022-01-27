#include "staticmapobj.h"
#include "fileops.h"
#include "extrahmm.h"
#include "cJSON/cJSON.h"
#include "kvec.h"

#include <stdio.h>

#define SWOS_DEF_COUNT 10
static struct swos {
    kvec_t(struct staticmapobj) objs;
} swos;

static int append_object(const char *model, hmm_vec3 pos,
                                hmm_vec3 scl, hmm_vec4 rot)
{
    struct staticmapobj obj;
    obj.om = objloader_find(model);
    if (!obj.om) {
        printf("model %s not found\n", model);
        return -1;
    }
    obj.matrix = calc_matrix(pos, rot, scl);
    kv_push(struct staticmapobj, swos.objs, obj);
    return 0;
}

static int parse_json()
{
    const char *fn = "world_static_objects.json";
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "world_static_objects");
    if (!root || root->type != cJSON_Array) {
        printf("%s: no world_static_objects array found\n");
        goto freejson;
    }

    cJSON *obj = 0;
    int objcount = 0;
    cJSON_ArrayForEach(obj, root) {
        objcount++;

        cJSON *jmodel = cJSON_GetObjectItem(obj, "model");
        cJSON *jpos = cJSON_GetObjectItem(obj, "pos");
        cJSON *jrot = cJSON_GetObjectItem(obj, "rot");
        cJSON *jscl = cJSON_GetObjectItem(obj, "scl");

        if (!jmodel || !jpos || jpos->type != cJSON_Array) {
            printf("%s: node %d is bad\n", fn, objcount);
            goto freejson;
        }

        char *model = jmodel->valuestring;
        hmm_vec3 pos;
        hmm_vec3 scl = HMM_Vec3(1.0f, 1.0f, 1.0f);
        hmm_vec4 rot = HMM_Vec4(0.0f, 0.0f, 0.0f, 0.0f);

        cJSON *tmp = 0;
        int cc = 0;
        cJSON_ArrayForEach(tmp, jpos) {
            pos.Elements[cc++] = tmp->valuedouble;
            if (cc>=3)
                break;
        }
        cc = 0;

        if (jscl && jscl->type == cJSON_Array) {
            cJSON_ArrayForEach(tmp, jscl) {
                scl.Elements[cc++] = tmp->valuedouble;
                if (cc>=3)
                    break;
            }
            cc = 0;
        }
        if (jrot && jrot->type == cJSON_Array) {
            cJSON_ArrayForEach(tmp, jrot) {
                rot.Elements[cc++] = tmp->valuedouble;
                if (cc>=4)
                    break;
            }
        }

        if (append_object(model, pos, scl, rot))
            goto freejson;
    }

    ret = 0;
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

void staticmapobj_setpos(struct staticmapobj *obj, hmm_vec3 pos)
{
    obj->matrix.Elements[3][0] = pos.X;
    obj->matrix.Elements[3][0] = pos.Y;
    obj->matrix.Elements[3][0] = pos.Z;
}

int staticmapobj_mngr_init()
{
    const int swosize = sizeof(struct staticmapobj);
    kv_init(swos.objs);
    kv_resize(struct staticmapobj, swos.objs, SWOS_DEF_COUNT);

    return parse_json();
}

void staticmapobj_mngr_kill()
{
    kv_destroy(swos.objs);
}

int staticmapobj_mngr_end()
{
    return kv_size(swos.objs);
}

struct staticmapobj *staticmapobj_get(int idx)
{
    return &kv_A(swos.objs, idx);
}
