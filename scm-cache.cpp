//  Copyright (C) 2005-2011 Robert Kooima
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
    next(1),
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

    // Limit the cache size to the maximum array texture depth.

    GLint max;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max);

    size = std::min(max, s);

    // Generate the array texture object.

    GLenum i = scm_internal_form(c, b, 0);
    GLenum e = scm_external_form(c, b, 0);
    GLenum y = scm_external_type(c, b, 0);

    glGenTextures  (1, &texture);
    glBindTexture  (GL_TEXTURE_TARGET,  texture);
    // glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_WRAP_S,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_WRAP_T,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_TARGET, GL_TEXTURE_WRAP_R,     GL_CLAMP);

    // Initialize it with a buffer of zeros.

    const int m = n + 2;
    GLubyte  *p;

    if ((p = (GLubyte *) malloc(size * m * m * c * b)))
    {
        memset(p, 0, size * m * m * c * b);
        // glTexImage3D(GL_TEXTURE_TARGET, 0, i, n, n, size, 1, e, y, p);
        glTexImage3D(GL_TEXTURE_TARGET, 0, i, m, m, size, 0, e, y, p);
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
#else
    for (t = threads.begin(); t != threads.end(); ++t)
        SDL_KillThread(*t);
#endif

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

    n = t;
    return 0;
}

// Handle incoming textures on the loads queue.

void scm_cache::update(int t)
{
    scm_task task;

    glBindTexture(GL_TEXTURE_TARGET, texture);

    for (int c = 0; c < max_loads_per_update && loads.try_remove(task); ++c)
    {
        scm_page page(task.f, task.i);

        waits.remove(page);

        if (task.d)
        {
            if (next < size)
                page.l = next++;
            else
            {
                scm_page victim = pages.eject(t, page.i);

                if (victim.valid())
                    page.l = victim.l;
            }

            if (page.l >= 0)
            {
                page.t = t;
                pages.insert(page, t);

                task.make_page(page.l, files[task.f]->get_w(),
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

    next = 1;
}

void scm_cache::draw()
{
    pages.draw();
}

void scm_cache::sync(int t)
{
    // while (!needs.empty())
    //     update(t);
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

