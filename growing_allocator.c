#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "growing_allocator.h"

#define DEFAULT_POOL_SIZE 4096
#define DEFAULT_POOL_COUNT 10

int growing_alloc_init(struct growing_alloc *ga, int size, int count)
{
    if (!size)
        size = DEFAULT_POOL_SIZE;
    if (!count)
        count = DEFAULT_POOL_COUNT;

    ga->pools = calloc(count, sizeof(char *));
    ga->free = calloc(count, sizeof(int));
    ga->poolscount = count;
    ga->poolsize = size;
    for (int i = 0; i < count; ++i) {
        ga->pools[i] = malloc(size);
        memset(ga->pools[i], 0, size);
        ga->free[i] = size;
    }

    return 0;
}

void growing_alloc_kill(struct growing_alloc *ga)
{
    for (int i = 0; i < ga->poolscount; ++i) {
        free(ga->pools[i]);
    }

    free(ga->pools);
    free(ga->free);
}

inline static char *get_from_pool(struct growing_alloc *ga, int size, int idx)
{
    char *ret = 0;
    if (ga->free[idx] >= size) {
        ret = &ga->pools[idx][ga->poolsize - ga->free[idx]];
        ga->free[idx] -= size;
    }

    return ret;
}

char *growing_alloc_get(struct growing_alloc *ga, int size)
{
    char *ret = 0;
    if (size > ga->poolsize)
        return ret;

    for (int i = 0; i < ga->poolscount; ++i) {
        ret = get_from_pool(ga, size, i);
        if (ret)
            return ret;
    }

    ga->poolscount++;
    ga->free = realloc(ga->free, sizeof(int)*ga->poolscount);
    ga->pools = realloc(ga->pools, sizeof(char *)*ga->poolscount);
    ga->free[ga->poolscount-1] = ga->poolsize;
    char *pool = malloc(ga->poolsize);
    memset(pool, 0, ga->poolsize);
    ga->pools[ga->poolscount-1] = pool;

    ret = get_from_pool(ga, size, ga->poolscount-1);

    return ret;
}

