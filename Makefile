OUTD = builddir
GITH = \"$(shell git rev-parse HEAD)\"

OPTS = \
    -I./3rdparty/cJSON \
    -I./3rdparty/hmm \
    -I./3rdparty/klib \
    -I./3rdparty/sokol \
    -I./3rdparty/qoi \
    -I./3rdparty/glad \
    -I./3rdparty/sort.h \
    -I./$(OUTD) \
    -DGIT_HASH=$(GITH) \
    -Wno-missing-braces \
    -Wall \
    -g

LIBS = \
    -lcimgui \
    -L../cimgui/ \
    -lSDL2 \
    -lm \
    -ldl \

FILE = \
    3rdparty/glad/glad.c \
    3rdparty/cJSON/cJSON.c \
    rand.c \
    qoi.c \
    animatmapobj.c \
    animodel.c \
    animodel_mngr.c \
    camera.c \
    event.c \
    extrahmm.c \
    fileops.c \
    growing_allocator.c \
    heightmap.c \
    wirebox.c \
    lightcube.c \
    look.c \
    md5anim.c \
    md5loader.c \
    md5model.c \
    mouse2world.c \
    worldedit.c \
    objloader.c \
    objmodel.c \
    pipelines.c \
    sdl2_imgui.c \
    shadow.c \
    sokol_gfx.c \
    sokol_imgui.c \
    staticmapobj.c \
    stb_image.c \
    terrain.c \
    material.c \
    atlas2d.c \
    json_helpers.c \
    particle_mngr.c \
    particle_emiter.c \
    particle.c \

GLSL = \
	shd_combo.glsl \
	shd_terrain.glsl \
	shd_wirebox.glsl \
	shd_md5.glsl \
	shd_particle.glsl \

SHAD = $(patsubst %.glsl, $(OUTD)/gen%.h, $(GLSL))
OBJS = $(patsubst %.c, $(OUTD)/%.o, $(FILE))
$(info $$SHAD is [${SHAD}])
$(info $$OBJS is [${OBJS}])

engine : $(SHAD) $(OBJS)
	$(CC) $(OPTS) $(OBJS) $(LIBS) -o engine

$(OUTD)/gen%.h: shaders/%.glsl
	@mkdir -p $(@D)
	sokol-shdc --input $< --output $@ --slang glsl330

#shaders: $(SHAD)

$(OUTD)/%.o : %.c
	@mkdir -p $(@D)
	$(CC) -c $(OPTS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(OUTD) engine
