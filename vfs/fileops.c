
struct pakfile {
    void *pdata;
    void *udata;
    int psize;
    int usize;
    int upos;
};

struct pakfile *pfopen(const char *fn);
void *pfgets(void *dst, int num, struct pakfile *f);
int pfread(void *dst, int size, int count, struct pakfile *f);
int pfeof(struct pakfile *f);

#define u8 uint8_t

struct pakfile *pfopen(const char *fn)
{
    struct pakfile *pf = malloc(sizeof(*pf));


}

int pfeof(struct pakfile *f)
{
    return (f->upos >= f->usize)
}

void *pfgets(void *dst, int num, struct pakfile *f)
{
    void *out = dst;
    void *src = &f->udata[upos];
    int bleft = f->usize - f->upos;
    if (!bleft)
        return out;

    if (num > bleft)
        num = bleft;

    for (int i = 0; i < num-1; ++i) {
        u8 by = src[i];
        *out++ = by;
        if (by == 0 || by == '\n')
            break;
    }

    *out++ = 0;
    return out;
}

int pfread(void *dst, int size, int count, struct pakfile *f)
{
    int tsize = size * count;
    int bleft = f->usize - f->upos;
    if (tsize > bleft)
        tsize = bleft;

    memcpy(dst, f->udata, tsize);
    return tsize;
}


FILE * fopen ( const char * filename, const char * mode );
char * fgets ( char * str, int num, FILE * stream );
size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
int feof ( FILE * stream );
