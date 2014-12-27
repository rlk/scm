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

#include <algorithm>

#include "util3d/glsl.h"

#include "scm-system.hpp"
#include "scm-cache.hpp"
#include "scm-image.hpp"
#include "scm-index.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

/// Initialize a new empty image for use in the given SCM system.

scm_image::scm_image(scm_system *sys) :
    sys(sys),
    channel(-1),
    height(false),
    k0 ( 0),
    k1 ( 1),
    uS (-1),
    ur (-1),
    uk0(-1),
    uk1(-1),
    index(-1)
{
}

/// Finalize this image's SCM file.

scm_image::~scm_image()
{
    set_scm("");
}

//------------------------------------------------------------------------------

/// Configure this image to read data from the named SCM file.
///
/// This method can have ripple effects throughout the SCM system...
///
/// 1. If this image was previously configured to read from a different SCM,
/// then that SCM is released. This might trigger the destruction of an scm_file
/// if its reference count goes to zero, and might also trigger the destruction
/// of an scm_cache if the file count of that cache goes to zero. In the event
/// of an scm_cache destruction, all of that cache's loader threads are asked
/// to exit and waited upon. @see scm_system::release_scm
///
/// 2. The nemed SCM is aquired. If it is not already open, this will trigger
/// the construction of a new scm_file object, and possibly the construction of
/// an scm_cache. In the event of an scm_cache construction, loader threads for
/// that cache are launched. @see scm_system::acquire_scm
///
/// So, while the scm_system makes every effort to minimize the effort of SCM
/// data access, significant setup may be necessary, and it all starts here.

void scm_image::set_scm(const std::string& s)
{
    if (!scm.empty()) index = sys->release_scm(scm);
    scm = s;
    if (!scm.empty()) index = sys->acquire_scm(scm);

    cache = (index < 0) ? 0 : sys->get_cache(index);
}

/// Set the name by which GLSL sampler uniforms may access this image.
///
/// The name "height" indicates that this image gives height map data, and that
/// an subsequent ground-level queries may use this image.
///
/// @see scm_scene::get_minimum_ground
/// @see scm_scene::get_current_ground

void scm_image::set_name(const std::string& s)
{
    name = s;
    height = (name == "height");
}

/// Set the channel index for this image.
///
/// In this case "channel" indicates left-eye (0), right-eye (1), etc. rather
/// than red (0), green (1), or similar.

void scm_image::set_channel(int c)
{
    channel = c;
}

/// Set the input value to be mapped onto 0 in the output.

void scm_image::set_normal_min(float k)
{
    k0 = k;
}

/// Set the input value to be mapped onto 1 in the output.

void scm_image::set_normal_max(float k)
{
    k1 = k;
}

//------------------------------------------------------------------------------

/// Request and store GLSL uniform locations for this image's parameters.

void scm_image::init_uniforms(GLuint program)
{
    scm_log("scm_image init_uniforms %s %s %d", scm.c_str(),
                                               name.c_str(), program);

    if (program && !name.empty())
    {
        uS  = glsl_uniform(program, "%s_sampler", name.c_str());
        ur  = glsl_uniform(program, "%s.r",       name.c_str());
        uk0 = glsl_uniform(program, "%s.k0",      name.c_str());
        uk1 = glsl_uniform(program, "%s.k1",      name.c_str());

        for (int d = 0; d < 16; d++)
        {
            ua[d] = glsl_uniform(program, "%s.a[%d]", name.c_str(), d);
            ub[d] = glsl_uniform(program, "%s.b[%d]", name.c_str(), d);
        }
    }
}

/// Set all GLSL uniform values for this image and bind the cache's texture.

void scm_image::bind(GLuint unit, GLuint program) const
{
    glUniform1i(uS,  unit);
    glUniform1f(uk0, k0);
    glUniform1f(uk1, k1);

    if (cache)
    {
        const GLfloat r = GLfloat(cache->get_page_size())
                        / GLfloat(cache->get_page_size() + 2)
                        / GLfloat(cache->get_grid_size());

        glUniform2f(ur,  r, r);
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, cache->get_texture());
    }
}

/// Unbind the cache's texture by binding the texture unit to zero.

void scm_image::unbind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//------------------------------------------------------------------------------

/// Set the GLSL uniforms necessary to map a page of texture data.
///
/// @param program GLSL program object
/// @param d       SCM page depth.
/// @param t       Current time
/// @param i       SCM page index.

void scm_image::bind_page(GLuint program, int d, int t, long long i) const
{
    if (cache)
    {
        // Get the page index and the time of its loading.

        int u, l = cache->get_page(index, i, t, u);

        // Compute the page age.

        double a = l ? 1.0 : 0.0;

        if (l && sys->get_synchronous())
        {
            a = (t - u) / 60.0;
            a = std::min(a, 1.0);
            a = std::max(a, 0.0);
        }

        // Compute texture coordinate offsets and set the uniforms.

        const int s = cache->get_grid_size();
        const int n = cache->get_page_size();

        glUniform1f(ua[d], GLfloat(a));
        glUniform2f(ub[d], GLfloat((l % s) * (n + 2) + 1) / (s * (n + 2)),
                           GLfloat((l / s) * (n + 2) + 1) / (s * (n + 2)));
    }
}

/// Set the texture mapping uniforms to reference cache line zero (which is
/// always blank).

void scm_image::unbind_page(GLuint program, int d) const
{
    glUniform1f(ua[d], 0.f);
    glUniform2f(ub[d], 0.f, 0.f);
}

/// Set the last-used time of a page.

void scm_image::touch_page(int t, long long i) const
{
    if (cache)
    {
        int ignored;
        cache->get_page(index, i, t, ignored);
    }
}

//------------------------------------------------------------------------------

/// Sample this image at the given location, returning a normalized result.
/// @see scm_scene::get_page_sample

float scm_image::get_page_sample(const double *v) const
{
    if (index < 0)
        return k1;
    else
        return sys->get_page_sample(index, v) * (k1 - k0) + k0;
}

/// Determine the minimum and maximum values of one page, returning a
/// normalized result. @see scm_scene::get_page_bounds

void scm_image::get_page_bounds(long long i, float& r0, float& r1) const
{
    if (index < 0)
    {
        r0 = k0;
        r1 = k1;
    }
    else
    {
        sys->get_page_bounds(index, i, r0, r1);

        r0 = k0 + (k1 - k0) * r0;
        r1 = k0 + (k1 - k0) * r1;
    }
}

/// Return true if a page is present in this image.
/// @see scm_scene::get_page_status

bool scm_image::get_page_status(long long i) const
{
    if (index < 0)
        return false;
    else
        return sys->get_page_status(index, i);
}

//------------------------------------------------------------------------------
