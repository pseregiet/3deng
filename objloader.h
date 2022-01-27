#ifndef __objloader_h
#define __objloader_h
#include "objmodel.h"

int objloader_init();
void objloader_kill();

struct obj_model *objloader_find(const char *name);
int objloader_get(int idx, const char **name, const struct obj_model **mdl);
int objloader_beg();
int objloader_end();

#endif
