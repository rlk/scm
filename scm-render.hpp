// Copyright (C) 2011-2012 Robert Kooima
//
// LIBSCM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

#ifndef SCM_RENDER_HPP
#define SCM_RENDER_HPP

#include <GL/glew.h>

#include "util3d/glsl.h"

//------------------------------------------------------------------------------

class scm_sphere;
class scm_scene;

//------------------------------------------------------------------------------

class scm_render
{
public:

    scm_render(int, int);
   ~scm_render();

    void render(scm_sphere *,
                scm_scene  *,
                scm_scene  *, const double *, double, int, int);

    void set_size(int, int);
    void set_blur(int);

private:

    void init_ogl();
    void free_ogl();

    int    width;
    int    height;
    int    motion;

    GLuint color0;
    GLuint depth0;
    GLuint framebuffer0;

    GLuint color1;
    GLuint depth1;
    GLuint framebuffer1;

    double L[16];

    glsl   fade;
    glsl   blur;
    glsl   both;
};


//------------------------------------------------------------------------------

#endif
