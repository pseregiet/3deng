#include "animatmapobj.h"
#include "fileops.h"
#include "extrahmm.h"
#include "cJSON.h"
#include "kvec.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define AWOS_DEF_COUNT 10
static struct awos {
    kvec_t(struct animatmapobj) objs;
} awos;

static int append_object(const char *model, hmm_vec3 pos,
                                hmm_vec3 scl, hmm_vec4 rot)
{
    struct animatmapobj awo;
    if (animodel_init(&awo.am, model))
        return -1;

    awo.matrix = calc_matrix(pos, rot, scl);
    kv_push(struct animatmapobj, awos.objs, awo);
    return 0;
}

static int parse_json()
{
    const char *fn = "data/world_animated_objects.json";
    struct file jf;
    int ret = -1;
    if (openfile(&jf, fn))
        return -1;

    cJSON *json = cJSON_ParseWithLength(jf.udata, jf.usize);
    if (!json) {
        printf("cJSON_Parse(%s) failed\n", fn);
        goto freebuf;
    }

    cJSON *root = cJSON_GetObjectItem(json, "world_animated_objects");
    if (!root || root->type != cJSON_Array) {
        printf("no world_animated_objects array found\n");
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

        if (append_object(model, pos, scl, rot)) {
            printf("append_object %s failed\n", model);
            goto freejson;
        }
    }

    ret = 0;
freejson:
    cJSON_Delete(json);
freebuf:
    closefile(&jf);
    return ret;
}

int animatmapobj_mngr_init()
{
    kv_init(awos.objs);
    kv_resize(struct animatmapobj, awos.objs, AWOS_DEF_COUNT);

    return parse_json();
}

void animatmapobj_mngr_kill()
{
    kv_destroy(awos.objs);
}

int animatmapobj_mngr_end()
{
    return kv_size(awos.objs);
}

struct animatmapobj *animatmapobj_mngr_get(int idx)
{
    return &kv_A(awos.objs, idx);
}
