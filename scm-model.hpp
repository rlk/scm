//  Copyright (C) 2005-2011 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#ifndef SCM_MODEL_HPP
#define SCM_MODEL_HPP

#include <GL/glew.h>
#include <vector>
#include <set>

#include "scm-frame.hpp"

//------------------------------------------------------------------------------

class scm_model
{
public:

    scm_model(const char *, const char *, int, int);
   ~scm_model();

    int  tick() { return time++; }

    void prep(scm_frame *, const double *, const double *, int, int);
    void draw(scm_frame *, const double *, const double *, int, int);

    void set_debug(bool b) { debug = b; }
    void set_fade(double k);
    void set_zoom(double x, double y, double z, double k)
    {
        zoomv[0] = x;
        zoomv[1] = y;
        zoomv[2] = z;
        zoomk    = k;
    }

private:

    int    time;
    int    size;
    bool   debug;

    GLfloat age(int);

    // Zooming state.

    double zoomv[3];
    double zoomk;

    void zoom(double *, const double *);

    // Data structures and algorithms for handling face adaptive subdivision.

    std::set<long long> pages;

    bool  is_set (long long i) const { return (pages.find(i) != pages.end()); }
    void set_page(long long i);

    void    add_page(const double *, int, int, double, double, long long);
    double view_page(const double *, int, int, double, double, long long);
    void  debug_page(const double *,           double, double, long long);

    bool prep_page(scm_frame *, const double *, int, int, long long);
    void draw_page(scm_frame *,                      int, long long);

    // OpenGL programmable processing state

    void init_program(const char *, const char *);
    void free_program();

    GLuint program;
    GLuint vert_shader;
    GLuint frag_shader;

    GLuint u_fader;
    GLuint u_zoomk;
    GLuint u_zoomv;

    // OpenGL geometry state.

    void init_arrays(int);
    void free_arrays();

    GLsizei count;
    GLuint  vertices;
    GLuint  elements[16];
};

//------------------------------------------------------------------------------

#endif
