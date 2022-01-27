#ifndef __md5loader_h
#define __md5loader_h
#include "md5model.h"

int md5loader_init();
void md5loader_kill();
struct md5_model *md5loader_find(const char *name);

#endif
