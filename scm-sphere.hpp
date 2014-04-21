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

#ifndef SCM_MODEL_HPP
#define SCM_MODEL_HPP

#include <GL/glew.h>
#include <vector>
#include <set>

#include "scm-scene.hpp"

//------------------------------------------------------------------------------

/// An scm_sphere generates the adaptive rendered geometry of the 3D sphere.
///
/// The sphere performs all visibility testing and subdivision necessary to
/// optimally render a given scene. Detail and limit parameters tune this
/// facility. Optional zoom direction and degree are maintained if needed.

class scm_sphere
{
public:

    scm_sphere(int d, int l);
   ~scm_sphere();

    void set_detail(int d);
    void set_limit (int l);

    int  get_detail() const { return detail; }
    int  get_limit () const { return limit;  }

    void prep(scm_scene *, const double *, int, int, int, bool);
    void draw(scm_scene *, const double *, int, int, int, int);

    void set_zoom(double x, double y, double z, double k);

private:

    int detail;
    int limit;

    // Zooming state.

    double zoomv[3];
    double zoomk;

    void zoom(double *, const double *);

    // Data structures and algorithms for handling face adaptive subdivision.

    std::set<long long> pages;

    bool     is_set (long long i) const { return (pages.find(i) != pages.end()); }
    void    set_page(long long i);

    void    add_page(const double *, int, int, double, double, long long, bool);
    double view_page(const double *, int, int, double, double, long long, bool);
    void  debug_page(const double *,           double, double, long long);

    bool   prep_page(scm_scene *, const double *, int, int, int, long long, bool);
    void   draw_page(scm_scene *,                 int, int, int, long long);

    // OpenGL geometry state.

    void init_arrays(int);
    void free_arrays();

    GLsizei count;
    GLuint  vertices;
    GLuint  elements[16];
};

//------------------------------------------------------------------------------

#endif
