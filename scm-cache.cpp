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
#include "scm-index.hpp"

//------------------------------------------------------------------------------

scm_cache::scm_cache(int s, int n, int c, int b, int t, float r0, float r1) :
    pages(),
    waits(),
    needs(need_queue_size),
    loads(load_queue_size),
    texture(0),
    s(s),
    l(1),
    n(n),
    c(c),
    b(b),
    r0(r0),
    r1(r1)
{
    // Launch the image loader threads.

    int loader(void *data);

    for (int i = 0; i < t; ++i)
        threads.push_back(SDL_CreateThread(loader, this));

    // Generate pixel buffer objects.

    for (int i = 0; i < 2 * need_queue_size; ++i)
    {
        GLuint b;
        glGenBuffers(1, &b);
        pbos.push_back(b);
    }

    // Generate the array texture object.

    GLenum i = scm_internal_form(c, b, 0);
    GLenum e = scm_external_form(c, b, 0);
    GLenum y = scm_external_type(c, b, 0);

    glGenTextures  (1, &texture);
    glBindTexture  (GL_TEXTURE_RECTANGLE, texture);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Initialize it with a buffer of zeros.

    const int m = s * (n + 2);

    if (GLubyte *p = (GLubyte *) calloc(m * m, c * b))
    {
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, i, m, m, 0, e, y, p);
        free(p);
    }
}

scm_cache::~scm_cache()
{
    std::vector<SDL_Thread *>::iterator t;

#if 0
    int c = 1;
    int s = 0;

    // Continue servicing the loads queue until the needs queue is emptied.

    sync(0);

    // Enqueue an exit command for each loader thread.

    for (t = threads.begin(); t != threads.end(); ++t, ++c)
        needs.insert(scm_task(-c, -c));

    // Await their exit.

    for (t = threads.begin(); t != threads.end(); ++t)
        SDL_WaitThread(*t, &s);
#endif
#if 0
    for (t = threads.begin(); t != threads.end(); ++t)
        SDL_KillThread(*t);
#endif

    // Release the pixel buffer objects.
#if 0
    while (!pbos.empty())
    {
        glDeleteBuffers(1, &pbos.back());
        pbos.pop_back();
    }
#endif
    // Release the texture.

    glDeleteTextures(1, &texture);
}

//------------------------------------------------------------------------------

int scm_cache::add_file(const std::string& name)
{
    int f;

    // Scan to determine whether the named file is already loaded.

    for (f = 0; f < int(files.size()); ++f)
        if (name.compare(files[f]->get_name()) == 0)
            return f;

    // Otherwise try to load it.

    if (scm_file *n = new scm_file(name))
    {
        // If succesful, add it to the collection.

        f = int(files.size());
        files.push_back(n);

        return f;
    }
    return -1;
}

// Return the layer for the requested page. Request the page if necessary.

int scm_cache::get_page(int f, long long i, int t, int& n)
{
    // If this page does not exist, return the filler.

    uint64 o = files[f]->offset(i);

    if (o == 0)
        return 0;

    // If this page is waiting, return the filler.

    scm_page wait = waits.search(scm_page(f, i), t);

    if (wait.valid())
    {
        n    = wait.t;
        return wait.l;
    }

    // If this page is loaded, return the layer.

    scm_page page = pages.search(scm_page(f, i), t);

    if (page.valid())
    {
        n    = page.t;
        return page.l;
    }

    // Otherwise request the page and add it to the waiting set.

    if (!pbos.empty())
    {
        scm_task task(f, i, o, pbos.deq(), files[f]->length());
        scm_page page(f, i, 0);

        if (needs.try_insert(task))
            waits.insert(page, t);
        else
        {
            task.dump_page();
            pbos.enq(task.u);
        }
    }

    // if (!needs.full() && !pbos.empty())
    // {
    //     needs.insert(scm_task(f, i, o, pbos.deq(), files[f]->length()));
    //     waits.insert(scm_page(f, i, 0), t);
    // }

    // If there is no PBO available, punt and let the app request again.

    n = t;
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

        if (victim.valid())
            return victim.l;
        else
            return 0;
    }
}

// Handle incoming textures on the loads queue.

void scm_cache::update(int t)
{
    scm_task task;

    glBindTexture(GL_TEXTURE_RECTANGLE, texture);

    for (int c = 0; c < max_loads_per_update && loads.try_remove(task); ++c)
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
                               (l / s) * (n + 2),
                               files[task.f]->get_w(),
                               files[task.f]->get_h(),
                               files[task.f]->get_c(),
                               files[task.f]->get_b(),
                               files[task.f]->get_g());
            }
            else task.dump_page();
        }
        else task.dump_page();

        pbos.enq(task.u);
    }
}

void scm_cache::flush()
{
    while (!pages.empty())
        pages.eject(0, -1);

    l = 1;
}

void scm_cache::sync(int t)
{
    // while (!needs.empty())
    //     update(t);
}

void scm_cache::draw(int ii, int nn)
{
    glPushAttrib(GL_ENABLE_BIT);
    {
        GLint v[4];

        glGetIntegerv(GL_VIEWPORT, v);

        const GLdouble a = GLdouble(v[2]) / GLdouble(v[3]);
        const GLdouble b = 0.5;

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_RECTANGLE);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-a, +a, -1, +1, -1, +1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(2.1 * b * (ii - 0.5 * (nn - 1)), 0.f, 0.f);

        glUseProgram(0);
        glBindTexture(GL_TEXTURE_RECTANGLE, texture);
        glColor4f(1.f, 1.f, 1.f, 1.f);

        glBegin(GL_QUADS);
        {
            const GLint m = s * (n + 2);

            glTexCoord2i(0, 0); glVertex2f(-b, -b);
            glTexCoord2i(m, 0); glVertex2f( b, -b);
            glTexCoord2i(m, m); glVertex2f( b,  b);
            glTexCoord2i(0, m); glVertex2f(-b,  b);
        }
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------

void scm_cache::get_page_bounds(int f, long long i, float& t0, float& t1)
{
    files[f]->bounds(uint64(i), t0, t1);

    t0 = t0 * (r1 - r0) + r0;
    t1 = t1 * (r1 - r0) + r0;
}

bool scm_cache::get_page_status(int f, long long i)
{
    return files[f]->status(uint64(i));
}

//------------------------------------------------------------------------------

// Load textures. Remove a task from the cache's needed queue, open and read
// the TIFF image file, and insert the task in the cache's loaded queue. Exit
// when given a negative file index.

int loader(void *data)
{
    scm_cache *cache = (scm_cache *) data;
    scm_task   task;

    while ((task = cache->needs.remove()).f >= 0)
    {
        assert(task.valid());

        if (TIFF *T = TIFFOpen(cache->files[task.f]->get_path(), "r"))
        {
            if (TIFFSetSubDirectory(T, task.o))
            {
                uint32 w = cache->files[task.f]->get_w();
                uint32 h = cache->files[task.f]->get_h();
                uint16 c = cache->files[task.f]->get_c();
                uint16 b = cache->files[task.f]->get_b();
                uint16 g = cache->files[task.f]->get_g();

                task.load_page(T, w, h, c, b, g);
                task.d = true;
            }
            TIFFClose(T);
        }
        cache->loads.insert(task);
    }
    return 0;
}

//------------------------------------------------------------------------------

