// Copyright () 2011-2012 Robert Kooima
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
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_scene::scm_scene(scm_system *sys) : sys(sys)
{
    memset(&render, 0, sizeof (glsl));
}

scm_scene::~scm_scene()
{
    while (get_image_count())
        del_image(0);
}

//------------------------------------------------------------------------------

void scm_scene::set_vert(const std::string &s)
{
    vert = s;

    glsl_delete(&render);

    if (!vert.empty() && !frag.empty())
        glsl_source(&render, vert.c_str(), frag.c_str());
}

void scm_scene::set_frag(const std::string &s)
{
    frag = s;

    glsl_delete(&render);

    if (!vert.empty() && !frag.empty())
        glsl_source(&render, vert.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------

// Allocate and insert a new image before i. Return its index.

int scm_scene::add_image(int i)
{
    int j = -1;

    if (scm_image *image = new scm_image(sys))
    {
        scm_image_i it = images.insert(images.begin() + i, image);
        j         = it - images.begin();
    }
    scm_log("scm_scene add_image %d = %d", i, j);

    return j;
}

// Delete the image at i.

void scm_scene::del_image(int i)
{
    scm_log("scm_scene del_image %d", i);

    delete images[i];
    images.erase(images.begin() + i);
}

// Return a pointer to the image at i.

scm_image *scm_scene::get_image(int i)
{
    return images[i];
}

//------------------------------------------------------------------------------

// Bind the program and all image textures matching channel.

GLuint scm_scene::bind(int channel) const
{
    GLenum unit = 0;

    glUseProgram(render.program);

    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel)
            images[j]->bind(unit++, render.program);

    glActiveTexture(GL_TEXTURE0);

    return render.program;
}

// Unbind the program and all image textures matching channel.

void scm_scene::unbind(int channel) const
{
    GLenum unit = 0;

    glUseProgram(0);

    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel)
            images[j]->unbind(unit++);

    glActiveTexture(GL_TEXTURE0);
}

//------------------------------------------------------------------------------

// For each image matching channel, bind page i. Return the program to allow the
// caller to set uniforms.

GLuint scm_scene::bind_page(int channel, int depth, int frame, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel)
            images[j]->bind_page(render.program, depth, frame, i);

    return render.program;
}

// For each image matching channel, unbind page i.

void scm_scene::unbind_page(int channel, int depth) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel)
            images[j]->unbind_page(render.program, depth);
}

// For each image maching channel, touch page i.

void scm_scene::touch_page(int channel, int frame, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel)
            images[j]->touch_page(frame, i);
}

//------------------------------------------------------------------------------

// Return the range of page i of the height image.

void scm_scene::get_page_bounds(int channel, long long i, float& r0, float &r1) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_height())
        {
            images[j]->get_page_bounds(i, r0, r1);
            return;
        }

    r0 = 1.f;
    r1 = 1.f;
}

// Return true if ANY one of the images has page i in cache.

bool scm_scene::get_page_status(int channel, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_channel() == channel &&
            images[j]->get_page_status(i))
            return true;

    return false;
}

// Sample the height image along vector v.

float scm_scene::get_height_sample(const double *v) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_height())
            return images[j]->get_page_sample(v);

    return 1.f;
}

// Return the smallest value in the height image.

float scm_scene::get_height_bottom() const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->get_height())
            return images[j]->get_normal_min();

    return 1.f;
}

//------------------------------------------------------------------------------
