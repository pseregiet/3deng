#include <stdio.h>
#include <stdlib.h>

#define fatalerror(...)\
    printf(__VA_ARGS__);\
    exit(-1)

char *load_shader(const char *fn, char *dst)
{
    FILE *f = fopen(fn, "r");
    if (!f) {
        fatalerror("cannot open %s shader\n", fn);
        return 0;
    }

    int size;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size >= 0x1000) {
        fatalerror("shader %s larger than expected %x > %x\n", fn, size, 0x1000);
        return 0;
    }
    
    fread(dst, 1, size, f);
    dst[size] = 0;
    
    return dst;
}

