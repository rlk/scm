
# This Makefile expects a CFLAGS export or set in the environment.

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
	scm-label-circle-frag.h \
	scm-label-circle-vert.h \
	scm-label-sprite-frag.h \
	scm-label-sprite-vert.h
RENDER_GLSL= \
	scm-render-blur-frag.h \
	scm-render-blur-vert.h \
	scm-render-both-frag.h \
	scm-render-both-vert.h \
	scm-render-atmo-frag.h \
	scm-render-atmo-vert.h \
	scm-render-fade-frag.h \
	scm-render-fade-vert.h

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

TARG= libscm.a

#------------------------------------------------------------------------------

$(TARG) : $(OBJS)
	ar -r $(TARG) $(OBJS)

clean:
	$(RM) $(TARG) $(GLSL) $(OBJS) $(DEPS)

#------------------------------------------------------------------------------

%.o : %.cpp
	$(CXX) $(CFLAGS) $(CONF) -c $< -o $@

%.o : %.c
	$(CC)  $(CFLAGS) $(CONF) -c $< -o $@

#------------------------------------------------------------------------------
# Embed shaders within C headers for direct inclusion.

%-vert.h : %.vert
	xxd -i $^ > $@

%-frag.h : %.frag
	xxd -i $^ > $@

#------------------------------------------------------------------------------

scm-render.o : $(RENDER_GLSL)
scm-label.o  : $(LABEL_GLSL)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
