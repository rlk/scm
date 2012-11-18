//  Copyright (C) 2005-2012 Robert Kooima
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

#include "util3d/glsl.h"

#include "scm-image.hpp"
#include "scm-index.hpp"

//------------------------------------------------------------------------------

scm_image::scm_image(const std::string& name,
					 const std::string& scm, scm_cache *cache, int c, bool h) :
	name(name),
	cache(cache),
	chan(c),
    height(h)
{
	file = cache->add_file(scm);
}

//------------------------------------------------------------------------------

void scm_image::bind(GLint unit, GLuint program) const
{
    if (height)
    {
        GLuint r0 = glsl_uniform(program, "r0");
        GLuint r1 = glsl_uniform(program, "r1");

        glUniform1f(r0, cache->get_r0());
        glUniform1f(r1, cache->get_r1());
    }

    GLuint img = glsl_uniform(program, "%s.img", name.c_str());
    GLuint siz = glsl_uniform(program, "%s.siz", name.c_str());

    glUniform1i(img, unit);
    glUniform2f(siz, GLfloat(cache->get_n()),
                     GLfloat(cache->get_n()));

    glActiveTexture(GL_TEXTURE0 + unit);
    cache->bind();
}

void scm_image::free(GLint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_TARGET, 0);
}

//------------------------------------------------------------------------------

void scm_image::bounds(long long i, float& r0, float& r1) const
{
    return cache->get_page_bounds(file, i, r0, r1);
}

bool scm_image::status(long long i) const
{
    return cache->get_page_status(file, i);
}

void scm_image::touch(long long i, int t) const
{
    int u;
    cache->get_page(file, i, t, u);
}

//------------------------------------------------------------------------------

void scm_image::set_texture(GLuint program, int d, int t, long long i) const
{
    GLint idx = glsl_uniform(program, "%s.idx[%d]", name.c_str(), d);
    GLint age = glsl_uniform(program, "%s.age[%d]", name.c_str(), d);

    int u, n = cache->get_page(file, i, t, u);

    double a = (t - u) / 60.0;

    if      (n ==  0) a = 0.0;
    else if (a > 1.0) a = 1.0;
    else if (a < 0.0) a = 0.0;

    glUniform1f(idx, GLfloat(n + 0.5) / cache->get_size());
//  glUniform1f(idx, GLfloat(n));
    glUniform1f(age, GLfloat(a));
}

void scm_image::clr_texture(GLuint program, int d) const
{
    GLint idx = glsl_uniform(program, "%s.idx[%d]", name.c_str(), d);
    GLint age = glsl_uniform(program, "%s.age[%d]", name.c_str(), d);

    glUniform1f(idx, 0.0);
    glUniform1f(age, 0.0);
}

//------------------------------------------------------------------------------
