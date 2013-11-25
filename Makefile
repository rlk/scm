
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

GLSL= \
	scm-label-circle-frag.h \
	scm-label-circle-vert.h \
	scm-label-sprite-frag.h \
	scm-label-sprite-vert.h \
	scm-render-blur-frag.h \
	scm-render-blur-vert.h \
	scm-render-both-frag.h \
	scm-render-both-vert.h \
	scm-render-fade-frag.h \
	scm-render-fade-vert.h

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

$(TARG) : $(GLSL) $(OBJS)
	ar -r $(TARG) $(OBJS)

clean:
	$(RM) $(TARG) $(GLSL) $(OBJS) $(DEPS)

#------------------------------------------------------------------------------

%.o : %.cpp
	$(CXX) $(CFLAGS) $(CONF) -c $< -o $@

%.o : %.c
	$(CC)  $(CFLAGS) $(CONF) -c $< -o $@

#------------------------------------------------------------------------------

%-vert.h : %.vert
	xxd -i $^ > $@

%-frag.h : %.frag
	xxd -i $^ > $@

#------------------------------------------------------------------------------

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

