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

#include "scm-scene.hpp"

//------------------------------------------------------------------------------

scm_scene::scm_scene() : height(0)
{
}

void scm_scene::add_image(scm_image *p)
{
    images.push_back(p);
    if (p->is_height())
        height = p;
}

#define FOR_ALL_OF_CHANNEL(it, c) \
     for (scm_image_c it = images.begin(); it != images.end(); ++it) \
        if ((*it)->is_channel(c))

//------------------------------------------------------------------------------

void scm_scene::bind(int channel, GLuint program) const
{
    GLenum unit = 0;

    glUseProgram(program);

    FOR_ALL_OF_CHANNEL(it, channel)
        (*it)->bind(unit++, program);

    glActiveTexture(GL_TEXTURE0);
}

void scm_scene::unbind(int channel) const
{
    GLenum unit = 0;

    glUseProgram(0);

    FOR_ALL_OF_CHANNEL(it, channel)
        (*it)->unbind(unit++);

    glActiveTexture(GL_TEXTURE0);
}

//------------------------------------------------------------------------------

void scm_scene::bind_page(GLuint program,
                             int channel,
                             int depth,
                             int frame, long long i) const
{
    FOR_ALL_OF_CHANNEL(it, channel)
        (*it)->bind_page(program, depth, frame, i);
}

void scm_scene::unbind_page(GLuint program, int channel, int depth) const
{
    FOR_ALL_OF_CHANNEL(it, channel)
        (*it)->unbind_page(program, depth);
}

void scm_scene::touch_page(int channel, int frame, long long i)
{
    FOR_ALL_OF_CHANNEL(it, channel)
        (*it)->touch_page(i, frame);
}

//------------------------------------------------------------------------------

// Return the range of any height image in this frame.

void scm_scene::get_page_bounds(int channel, long long i, float& r0, float &r1) const
{
    if (height)
        height->bounds(i, r0, r1);
    else
    {
        r0 = 1.0;
        r1 = 1.0;
    }
}

// Return true if any one of the images has page i in cache.

bool scm_scene::get_page_status(int channel, long long i) const
{
    FOR_ALL_OF_CHANNEL(it, channel)
        if ((*it)->status(i))
            return true;

    return false;
}

double scm_scene::get_height(const double *v) const
{
    if (height)
        return height->sample(v);
    else
        return 1.0;
}

double scm_scene::min_height() const
{
    if (height)
        return height->minimum();
    else
        return 1.0;
}

//------------------------------------------------------------------------------
