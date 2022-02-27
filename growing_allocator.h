#ifndef __growing_allocator_h
#define __growing_allocator_h

#include <string.h>

struct growing_alloc {
    char **pools;
    int *free;
    int poolsize;
    int poolscount;
};

int growing_alloc_init(struct growing_alloc *ga, int size, int count);
void growing_alloc_kill(struct growing_alloc *ga);
char *growing_alloc_get(struct growing_alloc *ga, int size);

inline static char *addname(struct growing_alloc *alloc, const char *name)
{
    int namelen = strlen(name);
    char *newname = growing_alloc_get(alloc, namelen+1);
    memcpy(newname, name, namelen);
    newname[namelen] = 0;
    return newname;
}


#endif

