#include <stdio.h>
#include <string.h>
#include "md5model.h"
#include "extrahmm.h"
#include "fileops.h"

struct jointinfo {
    char name[64];
    int parent;
    int flags;
    int startidx;
};

struct baseframe {
    hmm_vec3 pos;
    hmm_quat orient;
};

static void buildframe(const struct jointinfo *jinfo, const struct baseframe *base,
                      const float *fdata, struct md5_joint *frame, int jcount) {

    for (int i = 0; i < jcount; ++i) {
        const struct baseframe *b = &base[i];
        hmm_vec3 apos = b->pos;
        hmm_quat aorient = b->orient;
        int j = 0;

        int flags = jinfo[i].flags;
        int idx = jinfo[i].startidx;

        if (flags & 1) // Tx
            apos.X = fdata[idx + j++];
        if (flags & 2) // Ty
            apos.Y = fdata[idx + j++];
        if (flags & 4) // Tz
            apos.Z = fdata[idx + j++];
        if (flags & 8) // Qx
            aorient.X = fdata[idx + j++];
        if (flags & 16) // Qy
            aorient.Y = fdata[idx + j++];
        if (flags & 32) // Qz
            aorient.Z = fdata[idx + j++];

        aorient = quat_calcw(aorient);

        struct md5_joint *thisj = &frame[i];
        int parent = jinfo[i].parent;
        thisj->parent = parent;
        //strcpy(thisj->name, jinfo[i].name);

        if (thisj->parent < 0) {
            thisj->pos = apos;
            thisj->orient = aorient;
            continue;
        }

        struct md5_joint *parentj = &frame[parent];
        hmm_vec3 rpos = quat_rotat(parentj->orient, apos);
        thisj->pos = HMM_AddVec3(rpos, parentj->pos);
        thisj->orient = HMM_MultiplyQuaternion(parentj->orient, aorient);
        thisj->orient = HMM_NormalizeQuaternion(thisj->orient);
    }
}

inline static int checkanim(const struct md5_anim *anim)
{
    if (anim->jcount < 1 || anim->fcount < 1 || anim->fps < 1)
        return -1;

    return 0;
}

void md5anim_kill(struct md5_anim *anim)
{
    if (anim->frames)
        free(anim->frames);

    if (anim->bboxes)
        free(anim->bboxes);
}

inline static int getnextline(char **line, char **nextline)
{
    *nextline = strchr(*line, '\n');
    if (!*nextline || !(*nextline)[1])
        return -1;

    (*nextline)[0] = 0;
    return 0;
}

int md5anim_open(const char *fn, struct md5_anim *anim)
{
    struct jointinfo *jinfo = 0;
    struct baseframe *base = 0;
    float *fdata = 0;
    char *line;
    char *nextline;
    struct file f;
    int version;
    int animccount;
    int findex;
    int ret = -1;

    if (openfile(&f, fn))
        return -1;

    line = f.udata;
    while (1) {
        if (getnextline(&line, &nextline))
            break;

        if (sscanf(line, " MD5Version %d", &version) == 1) {
            if (version != 10) {
                printf("md5anim_open: bad version %d %s\n", version, fn);
                goto closefile;
            }
        }
        else if (sscanf(line, " numFrames %d", &anim->fcount) == 1) {
            if (anim->fcount > 0) {
                anim->bboxes = (struct md5_bbox *)
                    calloc(anim->fcount, sizeof(*anim->bboxes));
            }
        }
        else if (sscanf(line, " numJoints %d", &anim->jcount) == 1) {
            if (anim->jcount > 0) {
                const int count = anim->fcount * anim->jcount;
                const int size = sizeof(*anim->frames);
                anim->frames = (struct md5_joint *)calloc(count, size);

                jinfo = (struct jointinfo *)calloc(anim->jcount, sizeof(*jinfo));
                base = (struct baseframe *)calloc(anim->jcount, sizeof(*base));
            }
        }
        else if (sscanf(line, "frameRate %d", &anim->fps) == 1) {
            // nothing
        }
        else if (sscanf(line, "numAnimatedComponents %d", &animccount) == 1) {
            if (animccount) {
                fdata = (float *)calloc(animccount, sizeof(float));
            }
        }
        else if (strncmp(line, "hierarchy {", 11) == 0) {
            for (int i = 0; i < anim->jcount; ++i) {
                line = nextline+1;
                if (getnextline(&line, &nextline))
                    break;

                struct jointinfo *j = &jinfo[i];
                sscanf(line, " %64s %d %d %d", 
                        j->name, &j->parent, &j->flags, &j->startidx);
            }
        }
        else if (strncmp(line, "bounds {", 8) == 0) {
            for (int i = 0; i < anim->fcount; ++i) {
                line = nextline+1;
                if (getnextline(&line, &nextline))
                    break;

                struct md5_bbox *b = &anim->bboxes[i];
                sscanf(line, " ( %f %f %f ) ( %f %f %f )",
                            &b->min.X, &b->min.Y, &b->min.Z,
                            &b->max.X, &b->max.Y, &b->max.Z);
            }
        }
        else if (strncmp(line, "baseframe {", 10) == 0) {
            for (int i = 0; i < anim->jcount; ++i) {
                line = nextline+1;
                if (getnextline(&line, &nextline))
                    break;
                struct baseframe *b = &base[i];
                if (sscanf(line, " ( %f %f %f ) ( %f %f %f )",
                            &b->pos.X, &b->pos.Y, &b->pos.Z,
                            &b->orient.X, &b->orient.Y, &b->orient.Z) == 6) {

                    //b->orient = quat_calcw(b->orient);                   
                }
            }
        }
        else if (sscanf(line, " frame %d", &findex) == 1) {
            line = nextline + 1;
            for (int i = 0; i < animccount; ++i) {
                int n;
                sscanf(line, "%f%n", &fdata[i], &n);
                line = &line[n];
            }
                
            int jc = anim->jcount;
            buildframe(jinfo, base, fdata, &anim->frames[findex * jc], jc);
            continue;
        }

        line = nextline+1;
    }

    ret = checkanim(anim);
    if (ret)
        md5anim_kill(anim);

    if (jinfo)
        free(jinfo);
    if (base)
        free(base);
    if (fdata)
        free(fdata);

closefile:
    closefile(&f);
    return ret;
}

void md5anim_interp(const struct md5_anim *anim, struct md5_joint *out, 
                    int fa, int fb, float interp)
{
    const struct md5_joint *framea = &anim->frames[fa * anim->jcount];
    const struct md5_joint *frameb = &anim->frames[fb * anim->jcount];
    for (int i = 0; i < anim->jcount; ++i) {

        out[i].parent = framea[i].parent;
        out[i].pos.X = framea[i].pos.X + (frameb[i].pos.X - framea[i].pos.X) * interp;
        out[i].pos.Y = framea[i].pos.Y + (frameb[i].pos.Y - framea[i].pos.Y) * interp;
        out[i].pos.Z = framea[i].pos.Z + (frameb[i].pos.Z - framea[i].pos.Z) * interp;
        out[i].orient = HMM_Slerp(framea[i].orient, interp, frameb[i].orient);
    }
}

void md5anim_plain(const struct md5_anim *anim, struct md5_joint *out, int fa)
{
    const struct md5_joint *framea = &anim->frames[fa * anim->jcount];
    memcpy(out, framea, sizeof(struct md5_joint) * anim->jcount);
}
