#include "simple_list.h"

#include <string.h>
#include <stdlib.h>

#define SLIST_DEFAULT_SIZE 1
#define SLIST_DEFAULT_COUNT 100

int simple_list_init(struct simple_list *sl, int size, int count)
{
    if (!size)
        size = SLIST_DEFAULT_SIZE;
    if (!count)
        count = SLIST_DEFAULT_COUNT;

    sl->data = (char *)calloc(count, size);
    sl->count = 0;
    sl->cap = count;
    sl->elemsize = size;
    return 0;
}

int simple_list_kill(struct simple_list *sl)
{
    if (sl->data) {
        free(sl->data);
        sl->data = 0;
    }
}

char *simple_list_getfree(struct simple_list *sl, int count)
{
    assert(sl->data);
    int left = sl->cap - count;
    if (left < count) {
        char *newptr = realloc(sl->data, sl->cap*2*sl->elemsize);
        assert(newptr);
        sl->data = newptr;
        sl->cap *= 2;
    }

    char *ret = &sl->data[(sl->count + count -1) * sl->elemsize];
    sl->count += count;
    return ret;
}

void simple_list_delrange(struct simple_list *sl, int start, int count)
{
    assert(start < sl->count && (start + count) < sl->count);
    int mvsize = count > sl->count - (start + count) ? 
        sl->count - (start + count) : count;

    char *dst = &sl->data[start * sl->elemsize];
    char *src = &sl->data[(start + count) * sl->elemsize];
    memmove(dst, src, mvsize * sl->elemsize); 
}
