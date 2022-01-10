#include "pipelines.h"
#include "objloader.h"
#include "terrain.h"
#include "hitbox.h"
#include "lightcube.h"

int pipelines_init(struct pipelines *pipes)
{
    simpleobj_pipeline(pipes);
    simpleobj_hitbox_pipeline(pipes);
    terrain_pipeline(pipes);
    lightcube_pipeline(pipes);

    return 0;
}

void pipelines_kill(struct pipelines *pipes)
{
    sg_destroy_pipeline(pipes->simpleobj);
    sg_destroy_pipeline(pipes->simpleobj_hitbox);
    sg_destroy_pipeline(pipes->terrain);
    sg_destroy_pipeline(pipes->lightcube);

    sg_destroy_shader(pipes->simpleobj_shd);
    sg_destroy_shader(pipes->simpleobj_hitbox_shd);
    sg_destroy_shader(pipes->terrain_shd);
    sg_destroy_shader(pipes->lightcube_shd);
}

