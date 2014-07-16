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
class scm_frame;

//------------------------------------------------------------------------------

/// An scm_render manages the rendering of background and foreground spheres.
///
/// In the simplest case, this entails merely invoking the scm_sphere's render
/// function to draw geometry to the screen. However, the render manager also
/// supports motion blur and dissolve transitions. These features require the
/// sphere to be rendered first to an off-screen buffer which is then drawn to
/// the screen as a single rectangle with the appropriate shader enabled.
///
/// The render manager also maintains the wireframe debug option, which would
/// otherwise conflict with more sophisticated capabilities.

class scm_render
{
public:

    scm_render(int, int);
   ~scm_render();

    void set_size(int, int);
    void set_blur(int);
    void set_wire(bool);
    void set_atmo(double, double, double, double, double, double, double);

    int  get_blur() const { return blur; }
    bool get_wire() const { return wire; }

    void render(scm_sphere *,
                scm_scene  *,
                scm_scene  *,
                scm_scene  *,
                scm_scene  *,
              const double *,
              const double *, int, int, double);
    void render(scm_sphere *,
                scm_scene  *,
                scm_scene  *,
              const double *,
              const double *, int, int);

private:

    bool check_fade(scm_scene *, scm_scene *, scm_scene *, scm_scene *, double);
    bool check_blur(const double *, const double *, GLfloat *, double *);
    bool check_atmo(const double *, const double *, GLfloat *, GLfloat *);

    void init_uniforms(GLuint);
    void init_matrices();
    void init_ogl();
    void free_ogl();

    int    width;
    int    height;
    int    blur;
    bool   wire;

    GLfloat atmo_c[3];
    GLfloat atmo_r[2];
    GLfloat atmo_H;
    GLfloat atmo_P;

    scm_frame *frame0;
    scm_frame *frame1;

    double A[16];
    double B[16];
    double C[16];
    double D[16];

    double previous_T[16][16];

    glsl   render_fade;
    glsl   render_blur;
    glsl   render_both;
    glsl   render_atmo;

    GLint  uniform_fade_t;
    GLint  uniform_both_t;
    GLint  uniform_blur_T;
    GLint  uniform_both_T;
    GLint  uniform_blur_n;
    GLint  uniform_both_n;

    GLint  uniform_atmo_c;
    GLint  uniform_atmo_r;
    GLint  uniform_atmo_T;
    GLint  uniform_atmo_p;
    GLint  uniform_atmo_P;
    GLint  uniform_atmo_H;
};


//------------------------------------------------------------------------------

#endif
