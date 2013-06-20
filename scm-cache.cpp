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

#include <GL/glew.h>

#include <cstdlib>
#include <cassert>
#include <limits>

#include "scm-cache.hpp"
#include "scm-system.hpp"
#include "scm-index.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

// Static cache configuration parameters and their defaults.

int scm_cache::cache_size      = 16;
int scm_cache::cache_threads   =  2;
int scm_cache::need_queue_size = 32;
int scm_cache::load_queue_size =  8;
int scm_cache::loads_per_cycle =  2;

//------------------------------------------------------------------------------

scm_cache::scm_cache(scm_system *sys, int n, int c, int b) :
    sys(sys),
    pages(),
    waits(),
    loads(load_queue_size),
    texture(0),
    s(cache_size),
    l(1),
    n(n),
    c(c),
    b(b)
{
    // Generate pixel buffer objects.

    for (int i = 0; i < 2 * need_queue_size; ++i)
    {
        GLuint o;
        glGenBuffers(1, &o);
        pbos.push_back(o);
    }

    // Generate the array texture object.

    GLenum i = scm_internal_form(c, b);
    GLenum e = scm_external_form(c, b);
    GLenum y = scm_external_type(c, b);

    glGenTextures  (1, &texture);
    glBindTexture  (GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
//  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

    // Initialize it with a buffer of zeros.

    const int m = s * (n + 2);

    if (GLubyte *p = (GLubyte *) calloc(m * m, scm_pixel_size(c, b)))
    {
        glTexImage2D(GL_TEXTURE_2D, 0, i, m, m, 0, e, y, p);
        free(p);
    }

    scm_log("scm_cache constructor %d %d %d", n, c, b);
}

scm_cache::~scm_cache()
{
    scm_log("scm_cache destructor %d %d %d", n, c, b);

    // Drain any completed loads to ensure that the loaders aren't blocked.

    update(0, true);

    // Release the pixel buffer objects.

    while (!pbos.empty())
    {
        glDeleteBuffers(1, &pbos.back());
        pbos.pop_back();
    }

    // Release the texture.

    glDeleteTextures(1, &texture);
}

void scm_cache::add_load(scm_task& task)
{
    loads.insert(task);
}

//------------------------------------------------------------------------------

// Return the index for the requested page. Request the page if necessary.

int scm_cache::get_page(int f, long long i, int t, int& u)
{
    if (scm_file *file = sys->get_file(f))
    {
        // If this page does not exist, return the filler.

        uint64 o = file->get_page_offset(i);

        if (o == 0)
            return 0;

        // If this page is waiting, return the filler.

        scm_page wait = waits.search(scm_page(f, i), t);

        if (wait.is_valid())
        {
            u    = wait.t;
            return wait.l;
        }

        // If this page is loaded, return the index.

        scm_page page = pages.search(scm_page(f, i), t);

        if (page.is_valid())
        {
            u    = page.t;
            return page.l;
        }

        // Otherwise request the page and add it to the waiting set.

        if (!pbos.empty())
        {
            scm_task task(f, i, o, n, c, b, pbos.deq(), this);
            scm_page page(f, i, 0);

            if (file->add_need(task))
                waits.insert(page, t);
            else
            {
                task.dump_page();
                pbos.enq(task.u);
            }
        }
    }

    // If all else fails, punt and let the app request again.

    u = t;
    return 0;
}

// Find a slot for an incoming page. Either take the next unused slot or eject
// a page to make room. Return 0 on failure.

int scm_cache::get_slot(int t, long long i)
{
    if (l < s * s)
        return l++;
    else
    {
        scm_page victim = pages.eject(t, i);

        if (victim.is_valid())
            return victim.l;
        else
            return 0;
    }
}

//------------------------------------------------------------------------------

// Handle incoming textures on the loads queue. t gives the current frame
// count.

void scm_cache::update(int t, bool b)
{
    int c;

    scm_task task;

    glBindTexture(GL_TEXTURE_2D, texture);

    for (c = 0; (b || c < loads_per_cycle) && loads.try_remove(task); ++c)
    {
        if (task.d)
        {
            scm_page page(task.f, task.i);

            waits.remove(page);

            if (int l = get_slot(t, page.i))
            {
                page.l = l;
                page.t = t;
                pages.insert(page, t);
                task.make_page((l % s) * (n + 2),
                               (l / s) * (n + 2));
            }
            else task.dump_page();
        }
        else task.dump_page();

        pbos.enq(task.u);
    }
}

void scm_cache::render(int ii, int nn)
{
    glPushAttrib(GL_ENABLE_BIT);
    {
        GLint v[4];

        glGetIntegerv(GL_VIEWPORT, v);

        const GLdouble a = GLdouble(v[3]) / GLdouble(v[2]);

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, nn, 0, nn * a, -1, +1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glColor4f(1.f, 1.f, 1.f, 1.f);

        glBegin(GL_QUADS);
        {
            GLfloat a = 0.05f;
            GLfloat b = 0.95f;

            glTexCoord2i(0, 1); glVertex2f(ii + a, a);
            glTexCoord2i(1, 1); glVertex2f(ii + b, a);
            glTexCoord2i(1, 0); glVertex2f(ii + b, b);
            glTexCoord2i(0, 0); glVertex2f(ii + a, b);
        }
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    glPopAttrib();
}

void scm_cache::flush()
{
    while (!pages.empty())
        pages.eject(0, -1);

    l = 1;
}

//------------------------------------------------------------------------------
