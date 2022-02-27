#ifndef __json_helpers_h
#define __json_helpers_h

#include "cJSON.h"
#include "fileops.h"
int json_start(const char *fn, cJSON **json, cJSON **root, struct file *jf);

inline static int json_get_array_count(cJSON *obj)
{
    int count = 0;
    cJSON *tmp;
    cJSON_ArrayForEach(tmp, obj)
        count++;

    return count;
}
#endif

