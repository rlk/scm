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

#include <cstring>

#include "scm-scene.hpp"
#include "scm-image.hpp"
#include "scm-label.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

/// Create a new SCM scene for use in the given SCM system.

scm_scene::scm_scene(scm_system *sys) : sys(sys), label(0), color(0xFFBF00FF)
{
    memset(&render, 0, sizeof (glsl));

    atmo.c[0] = 1;
    atmo.c[1] = 1;
    atmo.c[2] = 1;
    atmo.H    = 0;
    atmo.P    = 0;

    scm_log("scm_scene constructor");
}

/// Finalize all SCM scene state.

scm_scene::~scm_scene()
{
    scm_log("scm_scene destructor");

    while (get_image_count())
        del_image(0);

    if (label)
        delete label;
}

//------------------------------------------------------------------------------

/// Allocate and insert a new image before index i. Return its index.

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

/// Delete the image at index i.

void scm_scene::del_image(int i)
{
    scm_log("scm_scene del_image %d", i);

    delete images[i];
    images.erase(images.begin() + i);
}

/// Return a pointer to the image at index i.

scm_image *scm_scene::get_image(int i)
{
    return images[i];
}

/// Return the number of images in the collection.

int scm_scene::get_image_count() const
{
    return int(images.size());
}

//------------------------------------------------------------------------------

/// Set the label color
///
/// @param c Color in 32-bit RGBA form

void scm_scene::set_color(GLuint c)
{
    color = c;
}

/// Set the scene name
///
/// @param s Scene's new name, silly.

void scm_scene::set_name(const std::string &s)
{
    name = s;
}

/// Set the label text
///
/// @param s CSV string giving a table of annotations

void scm_scene::set_label(const std::string &s)
{
    if (label)
        delete label;

    if (s.empty())
        label = 0;
    else
        label = new scm_label(s, 16);

    label_file = s;
}

/// Set the vertex shader and reinitialize the uniforms
///
/// @param s GLSL vertex shader source (*not* file name)

void scm_scene::set_vert(const std::string &s)
{
    vert_file = s;

    glsl_delete(&render);

    if (!vert_file.empty() && !frag_file.empty())
        glsl_source(&render, vert_file.c_str(), -1, frag_file.c_str(), -1);

    init_uniforms();
}

/// Set the fragment shader and reinitialize the uniforms
///
/// @param s GLSL fragment shader source (*not* file name)

void scm_scene::set_frag(const std::string &s)
{
    frag_file = s;

    glsl_delete(&render);

    if (!vert_file.empty() && !frag_file.empty())
        glsl_source(&render, vert_file.c_str(), -1, frag_file.c_str(), -1);

    init_uniforms();
}

/// Set the atmospheric parameters
///
/// @param A Atmospheric parameter structure

void scm_scene::set_atmo(const scm_atmo& A)
{
    atmo = A;
}

//------------------------------------------------------------------------------

/// Request and store the uniform locations for the current program.

void scm_scene::init_uniforms()
{
    if (render.program)
    {
        for (int j = 0; j < get_image_count(); ++j)
            images[j]->init_uniforms(render.program);

        for (int d = 0; d < 16; d++)
        {
            uA[d] = glsl_uniform(render.program, "A[%d]", d);
            uB[d] = glsl_uniform(render.program, "B[%d]", d);
        }
        uM     = glsl_uniform(render.program, "M");
        uzoomv = glsl_uniform(render.program, "zoomv");
        uzoomk = glsl_uniform(render.program, "zoomk");
    }
}

/// Render the labels for this scene, if any.

void scm_scene::draw_label()
{
    if (label)
    {
        GLubyte r = (color & 0xFF000000) >> 24;
        GLubyte g = (color & 0x00FF0000) >> 16;
        GLubyte b = (color & 0x0000FF00) >>  8;
        GLubyte a = (color & 0x000000FF) >>  0;

        label->draw(r, g, b, a);
    }
}

/// Bind the program and all image textures matching the given channel.
/// @see scm_image::bind

void scm_scene::bind(int channel) const
{
    GLenum unit = 0;

    glUseProgram(render.program);

    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel))
            images[j]->bind(unit++, render.program);

    glActiveTexture(GL_TEXTURE0);
}

/// Unbind the program and all image textures matching the given channel.
/// @see scm_image::unbind

void scm_scene::unbind(int channel) const
{
    GLenum unit = 0;

    glUseProgram(0);

    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel))
            images[j]->unbind(unit++);

    glActiveTexture(GL_TEXTURE0);
}

//------------------------------------------------------------------------------

/// Bind a page in each image matching a channel. @see scm_image::bind_page

void scm_scene::bind_page(int channel, int depth, int frame, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel))
            images[j]->bind_page(render.program, depth, frame, i);
}

/// Unbind a page in each image matching a channel. @see scm_image::unbind_page

void scm_scene::unbind_page(int channel, int depth) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel))
            images[j]->unbind_page(render.program, depth);
}

/// Touch a page in each image matching a channel. @see scm_image::touch_page

void scm_scene::touch_page(int channel, int frame, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel))
            images[j]->touch_page(frame, i);
}

//------------------------------------------------------------------------------

/// Sample the height image at the given location.
/// @see scm_system::get_current_ground
///
/// @param v Vector from the center of the planet to the query position.

float scm_scene::get_current_ground(const double *v) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_height())
            return images[j]->get_page_sample(v);

    return 1.f;
}

/// Return the smallest value in the height image.
/// @see scm_system::get_minimum_ground

float scm_scene::get_minimum_ground() const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_height())
            return images[j]->get_normal_min();

    return 1.f;
}

/// Determine the minimum and maximum values of one page of the height image
/// @see scm_system::get_page_bounds
/// @see scm_image::get_page_bounds

void scm_scene::get_page_bounds(int channel, long long i, float& r0, float &r1) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_height())
        {
            images[j]->get_page_bounds(i, r0, r1);
            return;
        }

    r0 = 1.f;
    r1 = 1.f;
}

/// Return true if any one of the images has page i in cache.
/// @see scm_system::get_page_status
/// @see scm_image::get_page_status

bool scm_scene::get_page_status(int channel, long long i) const
{
    for (int j = 0; j < get_image_count(); ++j)
        if (images[j]->is_channel(channel) &&
            images[j]->get_page_status(i))
            return true;

    return false;
}

//------------------------------------------------------------------------------
