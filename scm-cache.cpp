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

/// The cache grid size. The texture atlas will have size N x N where
/// N = grid_size * page_size. Too large a value may exceed the maximum texture
/// size or available VRAM, resulting in peer performance. Too small a value may
/// result in poor image quality and unnecessary data traffic.

int scm_cache::cache_size      = 16;

/// The number of loader threads servicing page load requests for each cache.

int scm_cache::cache_threads   =  2;

/// The maximum number of page load requests allowed at any moment. (Requests
/// from the render thread to the loader threads.) If this limit is exceeded
/// the render thread will abandon the request and repeat it later.

int scm_cache::need_queue_size = 32;

/// The maximum number of page load results allowed at any moment. (Results
/// from the loader threads to the render thread.) If this limit is exceeded
/// the loader thread will block on load queue.

int scm_cache::load_queue_size =  8;

/// The maximum number of page load results that may be uploaded to the atlas
/// by the render thread each frame. A large value may impact frame rate. A
/// small value may increase frame latency and/or block the loader threads.

int scm_cache::loads_per_cycle =  2;

//------------------------------------------------------------------------------

/// Create a new page cache with a queue for making page requests
///
/// Initialize all OpenGL state including the texture atlas and a ring of
/// pixel buffer objects for use in asynchronous upload of page data.
///
/// @param sys SCM system
/// @param n   Page size in pixels
/// @param c   Channels per pixel
/// @param b   Bits per channel

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

/// Destroy a page cache and finalize all OpenGL state

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

/// Add a page request to the load queue

void scm_cache::add_load(scm_task& task)
{
    loads.insert(task);
}

//------------------------------------------------------------------------------

/// Return the OpenGL texture object representing the cache

GLuint scm_cache::get_texture() const
{
    return texture;
}

/// Return the cache line of a loaded page
///
/// Cache lines are indexed from left to right and top to bottom. Request the
/// page if necessary. Return 0 if the page is not available (line 0 is always
/// transparent blank and will thus appear invisible).
///
/// @param f File index
/// @param i Page index
/// @param t Current time
/// @param u Time at which the page was loaded.

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
            {
                waits.insert(page, t);
            }
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

/// Find a slot for an incoming page
///
/// Either take the next unused slot or eject a page to make room. Return 0
// on failure. @see scm_set::eject
///
/// @param t Current time
/// @param i Page index

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

/// Handle incoming textures on the loads queue, copying them to the atlas.
///
/// This should be called by the render thread every frame. If invoked with
/// the synchronous flag, loop until all page requests in the load queue are
/// handled.
///
/// @param t Current time
/// @param b Synchronous?

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

/// Render a 2D overlay of the contents of all caches.
///
/// The parameters are used to format an optimal on-screen array of caches.
///
/// @param ii Cache index
/// @param nn Cache count

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

/// Eject all pages
///
/// All page requests in the load queue remain, so a flush is unlikely to
/// be 100% effective unless performed directly after a synchronous update.

void scm_cache::flush()
{
    while (!pages.empty())
        pages.eject(0, -1);

    l = 1;
}

//------------------------------------------------------------------------------
