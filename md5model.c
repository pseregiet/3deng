#include <stdio.h>
#include <string.h>
#include "md5model.h"
#include "extrahmm.h"
#include "texloader.h"

struct temp_md5_vert {
    hmm_vec2 uv;
    int wstart;
    int wcount;
};

struct temp_md5_trig {
    uint16_t index[3];
};

struct temp_md5_mesh {
    char shader[256];

    struct temp_md5_vert *vbuf;
    struct temp_md5_trig *tbuf;
    struct md5_weight *wbuf;

    int vcount;
    int tcount;
    int wcount;
};

struct temp_md5_model {
    struct md5_basejoint *base_skel;
    struct temp_md5_mesh *meshes;

    int jcount;
    int mcount;
};

static void md5model_mesh(FILE *f, char *line, struct temp_md5_model *model, int meshidx)
{
    struct temp_md5_mesh *mesh = &model->meshes[meshidx];
    int vidx = 0;
    int tidx = 0;
    int widx = 0;
    int idata[3];
    float fdata[4];

    while ((line[0] != '}') && !feof(f)) {
        fgets(line, 0x200, f);
        if (strstr(line, "shader ")) {
            int quote = 0;
            int j = 0;

            for (int i = 0; i < 0x200 && (quote < 2); ++i) {
                if (line[i] == '\"')
                    quote++;

                if ((quote == 1) && line[i] != '\"') {
                    mesh->shader[j++] = line[i];
                }
            }
        }
        else if (sscanf(line, " numverts %d", &mesh->vcount) == 1) {
            if (mesh->vcount > 0) {
                mesh->vbuf = (struct temp_md5_vert *)
                    calloc(mesh->vcount, sizeof(*mesh->vbuf));
            }
        }
        else if (sscanf(line, " numtris %d", &mesh->tcount) == 1) {
            if (mesh->vcount > 0) {
                mesh->tbuf = (struct temp_md5_trig *)
                    calloc(mesh->tcount, sizeof(*mesh->tbuf));
            }
        }
        else if (sscanf(line, " numweights %d", &mesh->wcount) == 1) {
            if (mesh->wcount > 0) {
                mesh->wbuf = (struct md5_weight *)
                    calloc(mesh->wcount, sizeof(*mesh->wbuf));
            }
        }
        else if (sscanf(line, " vert %d ( %f %f ) %d %d", &vidx,
                    &fdata[0], &fdata[1], &idata[0], &idata[1]) == 5) {
            mesh->vbuf[vidx].uv.U = fdata[0];
            mesh->vbuf[vidx].uv.V = fdata[1];
            mesh->vbuf[vidx].wstart = idata[0];
            mesh->vbuf[vidx].wcount = idata[1];
        }
        else if (sscanf(line, " tri %d %d %d %d", &tidx,
                    &idata[0], &idata[1], &idata[2]) == 4) {
            mesh->tbuf[tidx].index[0] = idata[0];
            mesh->tbuf[tidx].index[1] = idata[1];
            mesh->tbuf[tidx].index[2] = idata[2];
        }
        else if (sscanf(line, " weight %d %d %f ( %f %f %f )",
                    &widx, &idata[0], &fdata[3], &fdata[0],
                    &fdata[1], &fdata[2]) == 6) {
            mesh->wbuf[widx].joint = idata[0];
            mesh->wbuf[widx].bias = fdata[3];
            mesh->wbuf[widx].pos.X = fdata[0];
            mesh->wbuf[widx].pos.Y = fdata[1];
            mesh->wbuf[widx].pos.Z = fdata[2];
        }
    }
}

static int temp_md5model_verify(const struct temp_md5_model *model)
{
    if (model->jcount < 1 || model->mcount < 1)
        return -1;

    for (int i = 0; i < model->mcount; ++i) {
        struct temp_md5_mesh *mesh = &model->meshes[i];
        if (mesh->vcount < 1 || mesh->tcount < 1 || mesh->wcount < 1)
            return -1;
    }

    return 0;
}

struct md5vertex {
    hmm_vec3 pos;
    hmm_vec3 norm;
    hmm_vec3 tang;
    hmm_vec2 uv;
    //start and count
    int16_t weight[2];
};

static void md5model_normals(struct md5vertex *vbuf, struct temp_md5_trig *tbuf,
                            int vcount, int tcount)
{
    for (int i = 0; i < tcount; ++i) {
        hmm_vec3 v0 = vbuf[tbuf[i].index[0]].pos;
        hmm_vec3 v1 = vbuf[tbuf[i].index[1]].pos;
        hmm_vec3 v2 = vbuf[tbuf[i].index[2]].pos;

        hmm_vec2 uv0 = vbuf[tbuf[i].index[0]].uv;
        hmm_vec2 uv1 = vbuf[tbuf[i].index[1]].uv;
        hmm_vec2 uv2 = vbuf[tbuf[i].index[2]].uv;

        hmm_vec3 v20 = HMM_SubtractVec3(v2, v0);
        hmm_vec3 v10 = HMM_SubtractVec3(v1, v0);
        hmm_vec3 normal = HMM_Cross(v20, v10);

        hmm_vec3 n0 = HMM_AddVec3(vbuf[tbuf[i].index[0]].norm, normal);
        hmm_vec3 n1 = HMM_AddVec3(vbuf[tbuf[i].index[1]].norm, normal);
        hmm_vec3 n2 = HMM_AddVec3(vbuf[tbuf[i].index[2]].norm, normal);

        vbuf[tbuf[i].index[0]].norm = n0;
        vbuf[tbuf[i].index[1]].norm = n1;
        vbuf[tbuf[i].index[2]].norm = n2;

        hmm_vec3 t0 = HMM_NormalizeVec3(get_tangent(&v0, &v1, &v2, &uv0, &uv1, &uv2));
        vbuf[tbuf[i].index[0]].tang = t0;
        vbuf[tbuf[i].index[1]].tang = t0;
        vbuf[tbuf[i].index[2]].tang = t0;
    }

    for (int i = 0; i < vcount; ++i)
        vbuf[i].norm = HMM_NormalizeVec3(vbuf[i].norm);
}

static void calc_invmatrix(const struct temp_md5_model *model, struct md5_model *mdl)
{
    mdl->invmatrices = (hmm_mat4 *)calloc(model->jcount, sizeof(hmm_mat4));
    for (int i = 0; i < model->jcount; ++i) {
        struct md5_joint *j = &model->base_skel[i].joint;
        hmm_mat4 m = HMM_QuaternionToMat4(j->orient);
        m.Elements[3][0] = j->pos.X;
        m.Elements[3][1] = j->pos.Y;
        m.Elements[3][2] = j->pos.Z;
        mdl->invmatrices[i] = extrahmm_inverse_mat4(m);
    }
}

static void calc_weightmap(const struct temp_md5_model *model, struct md5_model *mdl)
{
    int totalwcount = 0;
    for (int i = 0; i < model->mcount; ++i) {
        totalwcount += model->meshes[i].wcount;
    }

    float *buf = (float *)malloc(totalwcount*2 * sizeof(float));
    float *out = buf;
    int woffset = 0;
    for (int m = 0; m < model->mcount; ++m) {
        struct temp_md5_mesh *mesh = &model->meshes[m];
        mdl->meshes[m].woffset = woffset;

        for (int i = 0; i < mesh->wcount; ++i) {
            float bias = mesh->wbuf[i].bias;
            float joint = (float)mesh->wbuf[i].joint;

            *out++ = bias;
            *out++ = joint;
            woffset++;
        }
    }
    make_sg_image_16f(buf, totalwcount, &mdl->weightw, &mdl->weighth, &mdl->weightmap);
    free(buf);
}

extern sg_image imgdummy;
static void make_shader(struct md5_mesh *mesh, char *shader)
{
    char fn[0x200];
    mesh->imgd = imgdummy;
    mesh->imgs = imgdummy;
    mesh->imgn = imgdummy;

    snprintf(fn, 0x200, "md5/%s_d.qoi", shader);
    load_sg_image(fn, &mesh->imgd);
    snprintf(fn, 0x200, "md5/%s_s.qoi", shader);
    load_sg_image(fn, &mesh->imgs);
    snprintf(fn, 0x200, "md5/%s_n.qoi", shader);
    load_sg_image(fn, &mesh->imgn);
}

static void md5model_make(const struct temp_md5_model *model, struct md5_model *mdl)
{
    const int vertsize = sizeof(struct md5vertex);
    const int indesize = sizeof(uint16_t);

    for (int i = 0; i < model->mcount; ++i) {
        struct temp_md5_mesh *mesh = &model->meshes[i];
        struct md5vertex *vbuf = (struct md5vertex *)calloc(mesh->vcount, vertsize);
        uint16_t *ibuf = (uint16_t *)mesh->tbuf;

        mdl->meshes[i].icount = mesh->tcount;
        mdl->meshes[i].wcount = mesh->wcount;
        mdl->meshes[i].vcount = mesh->vcount;

        for (int vi = 0; vi < mesh->vcount; ++vi) {
            for (int w = 0; w < mesh->vbuf[vi].wcount; ++w) {
                int wstart = mesh->vbuf[vi].wstart;
                struct md5_weight *weight = &mesh->wbuf[wstart + w];
                struct md5_joint *joint = &model->base_skel[weight->joint].joint;

                hmm_vec3 rot = quat_rotat(joint->orient, weight->pos);
                hmm_vec3 new = HMM_MultiplyVec3f(
                        HMM_AddVec3(joint->pos, rot), weight->bias);
                vbuf[vi].pos = HMM_AddVec3(vbuf[vi].pos, new);
            }
            vbuf[vi].uv = mesh->vbuf[vi].uv;
            vbuf[vi].weight[0] = mesh->vbuf[vi].wstart;
            vbuf[vi].weight[1] = mesh->vbuf[vi].wcount;
        }
        md5model_normals(vbuf, mesh->tbuf, mesh->vcount, mesh->tcount);

        mdl->meshes[i].vbuf = sg_make_buffer(&(sg_buffer_desc) {
            .data.size = mesh->vcount * vertsize,
            .data.ptr = vbuf,
        });

        mdl->meshes[i].ibuf = sg_make_buffer(&(sg_buffer_desc) {
            .data.size = mesh->tcount * 3 * indesize,
            .data.ptr = ibuf,
            .type = SG_BUFFERTYPE_INDEXBUFFER,
        });

        free(vbuf);

        make_shader(&mdl->meshes[i], mesh->shader);
    }

    calc_invmatrix(model, mdl);
    calc_weightmap(model, mdl);
    mdl->mcount = model->mcount;
    mdl->jcount = model->jcount;
}

static void temp_md5model_kill(const struct temp_md5_model *model)
{
    if (model->base_skel)
        free(model->base_skel);

    if (model->meshes) {
        for (int i = 0; i < model->mcount; ++i) {
            struct temp_md5_mesh *mesh = &model->meshes[i];
            if (mesh->vbuf)
                free(mesh->vbuf);
            if (mesh->wbuf)
                free(mesh->wbuf);
            if (mesh->tbuf)
                free(mesh->tbuf);
        }
        free(model->meshes);
    }
}

int md5model_open(const char *fn, struct md5_model *mdl)
{
    char line[0x200];
    FILE *f;
    int version;
    int meshidx = 0;
    int ret = - 1;
    struct temp_md5_model model = {0};

    f = fopen(fn, "rb");
    if (!f) {
        printf("md5model_open: Can't open file %s\n", fn);
        return ret;
    }

    while (!feof(f)) {
        fgets(line, sizeof(line), f);
        if (sscanf(line, " MD5Version %d", &version) == 1) {
            if (version != 10) {
                printf("md5model_open: Bad MD5Version %d %s\n", version, fn);
                goto closefile;
            }
        }
        else if (sscanf(line, " numJoints %d", &model.jcount) == 1) {
            if (model.jcount > 0) {
                model.base_skel = (struct md5_basejoint *)
                    calloc(model.jcount, sizeof(*model.base_skel));
            }
        }
        else if (sscanf(line, " numMeshes %d", &model.mcount) == 1) {
            if (model.mcount > 0) {
                model.meshes = (struct temp_md5_mesh *)
                    calloc(model.mcount, sizeof(*model.meshes));
            }
        }
        else if (strncmp(line, "joints {", 8) == 0) {
            for (int i = 0; i < model.jcount; ++i) {
                struct md5_basejoint *bjoint = &model.base_skel[i];
                struct md5_joint *joint = &bjoint->joint;
                fgets(line, sizeof(line), f);

                if (sscanf(line, "%64s %d ( %f %f %f ) ( %f %f %f )",
                            bjoint->name, &joint->parent, &joint->pos.X,
                            &joint->pos.Y, &joint->pos.Z, &joint->orient.X,
                            &joint->orient.Y, &joint->orient.Z) == 8) {

                    joint->orient = quat_calcw(joint->orient);
                }
            }
        }
        else if (strncmp(line, "mesh {", 6) == 0) {
            md5model_mesh(f, line, &model, meshidx++);
        }
    }

    if (!temp_md5model_verify(&model)) {
        md5model_make(&model, mdl);
        ret = 0;
    }

    temp_md5model_kill(&model);
closefile:
    fclose(f);
    return ret;
}

static void md5mesh_kill(struct md5_mesh *mesh)
{
    if (mesh->vbuf.id)
        sg_destroy_buffer(mesh->vbuf);
    if (mesh->ibuf.id)
        sg_destroy_buffer(mesh->vbuf);
    if (mesh->imgd.id)
        sg_destroy_image(mesh->imgd);
    if (mesh->imgs.id)
        sg_destroy_image(mesh->imgs);
    if (mesh->imgn.id)
        sg_destroy_image(mesh->imgn);
}

void md5model_kill(struct md5_model *model)
{
    for (int i = 0; i < model->mcount; ++i)
        md5mesh_kill(&model->meshes[i]);

    if (model->invmatrices)
        free(model->invmatrices);

    if (model->weightmap.id)
        sg_destroy_image(model->weightmap);
}
