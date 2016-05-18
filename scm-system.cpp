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
#include <sstream>
#include <iomanip>
#include <cassert>
#include <limits>
#include <cmath>
#include "util3d/math3d.h"

#include <tiffio.h>

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "scm-state.hpp"
#include "scm-scene.hpp"
#include "scm-cache.hpp"
#include "scm-sphere.hpp"
#include "scm-render.hpp"
#include "scm-system.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

/// Create a new empty SCM system. Instantiate a render handler and a sphere
/// handler.
///
/// @see scm_render::scm_render
/// @see scm_sphere::scm_sphere
///
/// @param w  Width of the off-screen render target (in pixels)
/// @param h  Height of the off-screen render target (in pixels)
/// @param d  Detail with which sphere pages are drawn (in vertices)
/// @param l  Limit at which sphere pages are subdivided (in pixels)

scm_system::scm_system(int w, int h, int d, int l) :
    serial(1), frame(0), sync(false)
{
    TIFFSetWarningHandler(0);
    TIFFSetErrorHandler  (0);

    scm_log("scm_system working directory is %s", getcwd(0, 0));

    mutex  = SDL_CreateMutex();
    render = new scm_render(w, h);
    sphere = new scm_sphere(d, l);
    path   = new scm_path();
}

/// Finalize all SCM system state.

scm_system::~scm_system()
{
    while (get_scene_count())
        del_scene(0);

    delete path;
    delete sphere;
    delete render;

    SDL_DestroyMutex(mutex);
}

//------------------------------------------------------------------------------

/// Render the sphere. This is among the most significant entry points of the
/// SCM API as it is the simplest function that accomplishes the goal. It should
/// be called once per frame.
///
/// The request is forwarded directly to the render handler, augmented with the
/// given state information.
///
/// @see scm_render::render
///
/// @param state    Viewer and environment state
/// @param P        Projection matrix in column-major OpenGL form
/// @param M        Model-view matrix in column-major OpenGL form
/// @param channel  Channel index (e.g. 0 for left eye, 1 for right eye)

void scm_system::render_sphere(const scm_state *state, const double *P,
                                                       const double *M,
                                                       int channel) const
{
    if (state->renderable())
        render->render(sphere, state, P, M, channel, frame);
    else
    {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

//------------------------------------------------------------------------------

/// Return a pointer to the sphere geometry handler.

scm_sphere *scm_system::get_sphere() const
{
    return sphere;
}

/// Return a pointer to the render manager.

scm_render *scm_system::get_render() const
{
    return render;
}

//------------------------------------------------------------------------------

/// Allocate and insert a new scene before index i. Return its index.

int scm_system::add_scene(int i)
{
    int j = -1;

    if (scm_scene *scene = new scm_scene(this))
    {
        scm_scene_i it = scenes.insert(scenes.begin() + std::max(i, 0), scene);
        j         = it - scenes.begin();
    }
    scm_log("scm_system add_scene %d = %d", i, j);

    return j;
}

/// Delete the scene at index i.

void scm_system::del_scene(int i)
{
    scm_log("scm_system del_scene %d", i);

    delete scenes[i];
    scenes.erase(scenes.begin() + i);
}

/// Return a pointer to the scene at index i, or 0 if i is out-of-range.

scm_scene *scm_system::get_scene(int i)
{
    if (0 <= i && i < int(scenes.size()))
        return scenes[i];
    else
        return 0;
}

/// Return the number of scenes in the collection.

int scm_system::get_scene_count() const
{
    return int(scenes.size());
}

//------------------------------------------------------------------------------

/// Update all image caches. This is among the most significant entry points of
/// the SCM API as it handles image input. It ensures that any page requests
/// being serviced in the background are properly transmitted to the OpenGL
/// context. It should be called once per frame. @see scm_cache::update

void scm_system::update_cache()
{
    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->update(frame, sync);
    frame++;
}

/// Render a 2D overlay of the contents of all caches. This can be a helpful
/// visual debugging tool as well as an effective demonstration of the inner
/// workings of the library. @see scm_cache::render

void scm_system::render_cache()
{
    int ii = 0, nn = caches.size();

    if (nn < 2)
        nn = 2;

    for (active_cache_i i = caches.begin(); i != caches.end(); ++i, ++ii)
        i->second.cache->render(ii, nn);
}

/// Flush all image caches. All pages are ejected from all caches.
/// @see scm_cache::flush

void scm_system::flush_cache()
{
    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->flush();
}

/// In synchronous mode, scm_cache::update will block until all background
/// input handling is complete. This ensures perfect data each frame, but
/// may delay frames. @see scm_cache::update

void scm_system::set_synchronous(bool b)
{
    sync = b;
}

/// Return the synchronous flag.

bool scm_system::get_synchronous() const
{
    return sync;
}

//------------------------------------------------------------------------------

/// Determine a fully-resolved path for the given file name
///
/// @param name File name

std::string scm_system::search_path(const std::string& name) const
{
    return path->search(name);
}

/// Push a directory onto the front of the path list.
///
/// @param directory directory name

void scm_system::push_path(const std::string& directory)
{
    path->push(directory);
}

/// Pop a directory off of the front of the path list.

void scm_system::pop_path()
{
    path->pop();
}

//------------------------------------------------------------------------------

/// Internal: Load the named SCM file, if not already loaded.
///
/// Add a new scm_file object to the collection and return its index. If needed,
/// create a new scm_cache object to manage this file's data. This will always
/// succeed as an scm_file object produces fallback data under error conditions,
/// such as an unfound SCM TIFF.

int scm_system::acquire_scm(const std::string& name)
{
    scm_log("acquire_scm %s", name.c_str());

    // If the file is loaded, note another usage.

    if (files[name].file)
        files[name].uses++;
    else
    {
        // Otherwise load the file.

        std::string pathname = path->search(name);

        if (!pathname.empty())
        {
            if (scm_file *file = new scm_file(name, pathname))
            {
                int index = serial++;

                files[name].file  = file;
                files[name].index = index;
                files[name].uses  = 1;

                // Make sure we have a compatible cache.

                cache_param cp(file);

                if (caches[cp].cache)
                    caches[cp].uses++;
                else
                {
                    caches[cp].cache = new scm_cache(this, cp.n, cp.c, cp.b);
                    caches[cp].uses  = 1;
                }

                // Associate the index, file, and cache in the reverse look-up.

                SDL_mutexP(mutex);
                pairs[index] = active_pair(files[name].file, caches[cp].cache);
                SDL_mutexV(mutex);

                file->activate(caches[cp].cache);
            }
        }
    }
    return files[name].index;
}

/// Release the named SCM file.
///
/// The file collection is reference-counted, and the scm_file object is only
/// deleted when all acquisitions are released. If a deleted file is the only
/// file handled by an scm_cache then delete that cache.

int scm_system::release_scm(const std::string& name)
{
    scm_log("release_scm %s", name.c_str());

    // Release the named file and delete it if no uses remain.

    if (--files[name].uses == 0)
    {
        // Remove the index from the reverse look-up.

        SDL_mutexP(mutex);
        pairs.erase(files[name].index);
        SDL_mutexV(mutex);

        // Signal the loaders to prepare to exit.

        files[name].file->deactivate();

        // Cycle the cache to ensure the loaders unblock.

        cache_param cp(files[name].file);
        caches[cp].cache->update(0, true);

        // Delete the file.

        delete files[name].file;
        files.erase(name);

        // Release the associated cache and delete it if no uses remain.

        if (--caches[cp].uses == 0)
        {
            delete caches[cp].cache;
            caches.erase(cp);
        }
    }
    return -1;
}

//------------------------------------------------------------------------------

/// Return the scene with the given name.

scm_scene *scm_system::find_scene(const std::string& name) const
{
    for (size_t i = 0; i < scenes.size(); i++)
        if (scenes[i]->get_name() == name)
            return scenes[i];

    return 0;
}

/// Return the cache associated with the given file index.

scm_cache *scm_system::get_cache(int i)
{
    if (pairs.find(i) == pairs.end())
        return 0;
    else
        return pairs[i].cache;
}

/// Return the file associated with the given file index.

scm_file *scm_system::get_file(int i)
{
    if (pairs.find(i) == pairs.end())
        return 0;
    else
        return pairs[i].file;
}

//------------------------------------------------------------------------------

/// Sample an SCM file at the given location. O(log n). This may incur data
/// access in the render thread. @see scm_file::get_page_sample
///
/// @param f File index
/// @param v Vector from the center of the planet to the query position.

float scm_system::get_page_sample(int f, const double *v)
{
    if (scm_file *file = get_file(f))
        return file->get_page_sample(v);
    else
        return 1.f;
}

/// Determine the minimum and maximum values of an SCM file page. O(log n).
/// @see scm_file::get_page_bounds
///
/// @param f  File index
/// @param i  Page index
/// @param r0 Minimum radius output
/// @param r1 Maximum radius output

void scm_system::get_page_bounds(int f, long long i, float& r0, float& r1)
{
    if (scm_file *file = get_file(f))
        file->get_page_bounds(uint64(i), r0, r1);
    else
    {
        r0 = 1.f;
        r1 = 1.f;
    }
}

/// Return true if a page is present in the SCM file. O(log n).
/// @see scm_file::get_page_status
///
/// @param f File index
/// @param i Page index

bool scm_system::get_page_status(int f, long long i)
{
    if (scm_file *file = get_file(f))
        return file->get_page_status(uint64(i));
    else
        return false;
}

//------------------------------------------------------------------------------
