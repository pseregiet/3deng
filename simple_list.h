#ifndef __simple_list_h
#define __simple_list_h
#include <assert.h>

struct simple_list {
    char *data;
    int count;
    int cap;
    int elemsize;
};

int simple_list_init(struct simple_list *sl, int size, int count);
int simple_list_kill(struct simple_list *sl);
char *simple_list_getfree(struct simple_list *sl, int count);
void simple_list_delrange(struct simple_list *sl, int start, int count);

inline static void simple_list_delall(struct simple_list *sl)
{
    sl->count = 0;
}

inline static int simple_list_count(struct simple_list *sl)
{
    return sl->count;
}

inline static char *simple_list_get(struct simple_list *sl, int index)
{
    assert(sl->data);
    assert(index < sl->count);
    return &sl->data[index * sl->elemsize];
}

#endif
