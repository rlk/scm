# libscm -- Linux / Mac OSX Makefile

ifdef DEBUG
	CONFIG  = Debug
	CFLAGS += -g
else
	CONFIG  = Release
	CFLAGS += -O2 -DNDEBUG
endif

#------------------------------------------------------------------------------

OBJS= \
	util3d/glsl.o \
	util3d/math3d.o \
	util3d/type.o \
	scm-cache.o \
	scm-file.o \
	scm-frame.o \
	scm-image.o \
	scm-index.o \
	scm-label.o \
	scm-log.o \
	scm-path.o \
	scm-render.o \
	scm-sample.o \
	scm-scene.o \
	scm-set.o \
	scm-sphere.o \
	scm-step.o \
	scm-system.o \
	scm-task.o

DEPS= $(OBJS:.o=.d)

LABEL_GLSL= \
	scm_label_circle_frag.h \
	scm_label_circle_vert.h \
	scm_label_sprite_frag.h \
	scm_label_sprite_vert.h
RENDER_GLSL= \
	scm_render_blur_frag.h \
	scm_render_blur_vert.h \
	scm_render_both_frag.h \
	scm_render_both_vert.h \
	scm_render_atmo_frag.h \
	scm_render_atmo_vert.h \
	scm_render_fade_frag.h \
	scm_render_fade_vert.h

GLSL= $(LABEL_GLSL) $(RENDER_GLSL)

#------------------------------------------------------------------------------

SDLCONF = $(firstword $(wildcard /usr/local/bin/sdl2-config  \
	                         /opt/local/bin/sdl2-config  \
	                               /usr/bin/sdl2-config) \
	                                        sdl2-config)
FT2CONF = $(firstword $(wildcard /usr/local/bin/freetype-config  \
	                         /opt/local/bin/freetype-config  \
	                               /usr/bin/freetype-config) \
	                                        freetype-config)

CONF =	$(shell $(SDLCONF) --cflags) \
	$(shell $(FT2CONF) --cflags)

TARGDIR = $(CONFIG)
TARG    = libscm.a

#------------------------------------------------------------------------------

$(TARGDIR)/$(TARG) : $(TARGDIR) $(OBJS)
	ar -r $(TARGDIR)/$(TARG) $(OBJS)

$(TARGDIR) :
	mkdir -p $(TARGDIR)

clean:
	$(RM) $(TARGDIR)/$(TARG) $(GLSL) $(OBJS) $(DEPS)

#------------------------------------------------------------------------------
# The bin2c tool embeds binary data in C sources.

B2C = etc/bin2c

$(B2C) : etc/bin2c.c
	$(CC) -o $(B2C) etc/bin2c.c

#------------------------------------------------------------------------------

%.o : %.cpp
	$(CXX) $(CFLAGS) $(CONF) -c $< -o $@

%.o : %.c
	$(CC)  $(CFLAGS) $(CONF) -c $< -o $@

%.h : %.glsl $(B2C)
	$(B2C) $(basename $@) < $^ > $@

#------------------------------------------------------------------------------

scm-render.o : $(RENDER_GLSL)
scm-label.o  : $(LABEL_GLSL)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
