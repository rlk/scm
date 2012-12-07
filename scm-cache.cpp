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

scm_cache::scm_cache(scm_system *sys, int n, int c, int b) :
    sys(sys),
    pages(),
    waits(),
    needs(need_queue_size),
    loads(load_queue_size),
    run(true),
    texture(0),
    s(cache_size),
    l(1),
    n(n),
    c(c),
    b(b)
{
    // Launch the image loader threads.

    int loader(void *data);

    for (int i = 0; i < cache_threads; ++i)
        threads.push_back(SDL_CreateThread(loader, this));

    // Generate pixel buffer objects.

    for (int i = 0; i < 2 * need_queue_size; ++i)
    {
        GLuint b;
        glGenBuffers(1, &b);
        pbos.push_back(b);
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

    if (GLubyte *p = (GLubyte *) calloc(m * m, c * b))
    {
        glTexImage2D(GL_TEXTURE_2D, 0, i, m, m, 0, e, y, p);
        free(p);
    }

    scm_log("scm_cache constructor %d %d %d", n, c, b);
}

scm_cache::~scm_cache()
{
    scm_log("scm_cache destructor %d %d %d", n, c, b);

    // The following dance courts deadlock to terminate all synchronous IPC.

    std::vector<SDL_Thread *>::iterator i;
    int s = 0;
    int t = 1;

    // Notify the loaders that they need not complete their assigned tasks.

    run.set(false);

    // Drain any completed loads to ensure that the loaders aren't blocked.

    update(0, true);

    // Enqueue an exit command for each loader.

    for (i = threads.begin(); i != threads.end(); ++i, ++t)
        needs.insert(scm_task(-t, -t));

    // Await the exit of each loader.

    for (i = threads.begin(); i != threads.end(); ++i)
        SDL_WaitThread(*i, &s);

    // Release the pixel buffer objects.

    while (!pbos.empty())
    {
        glDeleteBuffers(1, &pbos.back());
        pbos.pop_back();
    }

    // Release the texture.

    glDeleteTextures(1, &texture);
}

//------------------------------------------------------------------------------

// Return the index for the requested page. Request the page if necessary.

int scm_cache::get_page(int f, long long i, int t, int& u)
{
    // TODO: sys->get_file is called every frame for every page. That's a lot.

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
            scm_task task(f, i, o, n, c, b, pbos.deq());
            scm_page page(f, i, 0);

            if (needs.try_insert(task))
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
// count and b request that the loads queue be drained completely.

void scm_cache::update(int t, bool b)
{
    int c;

    scm_task task;

    glBindTexture(GL_TEXTURE_2D, texture);

    for (c = 0; (b || c < max_loads_per_update) && loads.try_remove(task); ++c)
    {
        if (task.d && is_running())
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

        const GLdouble a = GLdouble(v[2]) / GLdouble(v[3]);

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-a, +a, -1, +1, -1, +1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(1.10f * ii - 0.55f * (nn - 1), 0.f, 0.f);

        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glColor4f(1.f, 1.f, 1.f, 1.f);

        glBegin(GL_QUADS);
        {
            glTexCoord2i(0, 1); glVertex2f(-0.5f, -0.5f);
            glTexCoord2i(1, 1); glVertex2f( 0.5f, -0.5f);
            glTexCoord2i(1, 0); glVertex2f( 0.5f,  0.5f);
            glTexCoord2i(0, 0); glVertex2f(-0.5f,  0.5f);
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

// Load textures. Remove a task from the cache's needed queue, load the texture
// to the task buffer, and insert the task in the cache's loaded queue. Ignore
// the task if the cache is quitting. Exit when given a negative file index.

int loader(void *data)
{
    scm_log("loader thread begin %p", data);
    {
        scm_cache *cache = (scm_cache *) data;
        scm_task   task;

        TIFF *tiff = 0;
        void *temp = 0;

        while ((task = cache->needs.remove()).f >= 0)
        {
            if (cache->is_running())
            {
                if ((tiff = cache->sys->get_tiff(task.f)))
                {
                    if ((temp) || (temp = malloc(TIFFScanlineSize(tiff))))
                    {
                        task.load_page(tiff, temp);
                        cache->loads.insert(task);
                    }
                }
            }
        }
        free(temp);
    }
    scm_log("loader thread end %p", data);
    return 0;
}

//------------------------------------------------------------------------------

