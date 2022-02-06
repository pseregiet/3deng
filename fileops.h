#ifndef __fileops_h
#define __fileops_h
#include <stdint.h>

struct file {
    char *udata;
    char *pdata;
    int usize;
    int psize;
    int pos;
};

int openfile(struct file *file, const char *fp);
void closefile(struct file *f);

#endif
