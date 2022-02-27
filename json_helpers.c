#include "json_helpers.h"
#include <stdio.h>

int json_start(const char *fn, cJSON **json, cJSON **root, struct file *jf)
{
    char tmp[0x1000];
    snprintf(tmp, 0x1000, "data/%s.json", fn);
    if (openfile(jf, tmp))
        return -1;

    *json = cJSON_ParseWithLength(jf->udata, jf->usize);
    if (!*json) {
        printf("cJSON_Parse(%s) failed\n", tmp);
        closefile(jf);
        return -1;
    }

    *root = cJSON_GetObjectItem(*json, fn);
    if (!*root || (*root)->type != cJSON_Array) {
        printf("no %s array found\n", fn);
        cJSON_Delete(*json);
        closefile(jf);
        return -1;
    }

    return 0;
}

