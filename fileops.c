#include "fileops.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int openfile(struct file *file, const char *fp)
{
    FILE *f = fopen(fp, "rb");
    if (!f) {
        printf("fileops %s: open failed\n", fp);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    rewind(f);

    file->udata = calloc(size + 1, sizeof(char));
    assert(file->udata);
    fread(file->udata, 1, size, f);
    file->udata[size] = 0;
    fclose(f);

    file->usize = size;
    file->pos = 0;
    file->pdata = 0;
    file->psize = 0;

    return 0;
}

void closefile(struct file *file)
{
    if (file) {
        if (file->udata)
            free(file->udata);
        if (file->pdata)
            free(file->pdata);
    }
}
