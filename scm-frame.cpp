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

#include "scm-frame.hpp"

//------------------------------------------------------------------------------

scm_frame::scm_frame() : height(0), channel(0)
{
}

void scm_frame::add_image(scm_image *p)
{
    images.push_back(p);
    if (p->is_height())
        height = p;
}

#define FOR_ALL_OF_CHANNEL(c) \
     for (scm_image_c c = images.begin(); c != images.end(); ++c) \
        if ((*c)->is_channel(channel))

//------------------------------------------------------------------------------

void scm_frame::bind(GLuint program) const
{
    GLenum unit = 0;

    glUseProgram(program);

    FOR_ALL_OF_CHANNEL(c)
        (*c)->bind(unit++, program);

    glActiveTexture(GL_TEXTURE0);
}

void scm_frame::unbind() const
{
    GLenum unit = 0;

    glUseProgram(0);

    FOR_ALL_OF_CHANNEL(c)
        (*c)->unbind(unit++);

    glActiveTexture(GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void scm_frame::bind_page(GLuint program, int d, int t, long long i) const
{
    FOR_ALL_OF_CHANNEL(c)
        (*c)->bind_page(program, d, t, i);
}

void scm_frame::unbind_page(GLuint program, int d) const
{
    FOR_ALL_OF_CHANNEL(c)
        (*c)->unbind_page(program, d);
}

// Touch the given page at the given time, "using" it in the LRU sense.

void scm_frame::touch_page(long long i, int time)
{
    FOR_ALL_OF_CHANNEL(c)
        (*c)->touch_page(i, time);
}

//------------------------------------------------------------------------------

// Return true if any one of the images has page i in cache.

bool scm_frame::get_page_status(long long i) const
{
    FOR_ALL_OF_CHANNEL(c)
        if ((*c)->status(i))
            return true;

    return false;
}

// Return the range of any height image in this frame.

void scm_frame::get_page_bounds(long long i, float& r0, float &r1) const
{
    if (height)
        height->bounds(i, r0, r1);
    else
    {
        r0 = 1.0;
        r1 = 1.0;
    }
}

double scm_frame::get_height(const double *v) const
{
    if (height)
        return height->sample(v);
    else
        return 1.0;
}

double scm_frame::min_height() const
{
    if (height)
        return height->minimum();
    else
        return 1.0;
}

//------------------------------------------------------------------------------
