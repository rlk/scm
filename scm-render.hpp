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
    void set_wire(bool);

    int  get_blur() const { return blur; }
    bool get_wire() const { return wire; }

private:

    void render0(scm_sphere *, scm_scene *, const double *, int, int);
    void render1(scm_sphere *, scm_scene *, const double *, int, int);

    void init_matrices();
    void init_ogl();
    void free_ogl();

    int    width;
    int    height;
    int    blur;
    bool   wire;

    GLuint color0;
    GLuint depth0;
    GLuint framebuffer0;

    GLuint color1;
    GLuint depth1;
    GLuint framebuffer1;

    double A[16];
    double B[16];
    double C[16];
    double D[16];
    double L[16][16];

    glsl   render_fade;
    glsl   render_blur;
    glsl   render_both;

    GLint  uniform_fade_t, uniform_both_t;
    GLint  uniform_blur_T, uniform_both_T;
    GLint  uniform_blur_n, uniform_both_n;
};


//------------------------------------------------------------------------------

#endif
