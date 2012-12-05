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

#include "scm-system.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_system::scm_system() : serial(1), timer(0)
{
    mutex = SDL_CreateMutex();
    model = new scm_model(32, 512);
}

scm_system::~scm_system()
{
    while (get_scene_count())
        del_scene(0);

    delete model;
    SDL_DestroyMutex(mutex);
}

//------------------------------------------------------------------------------

void scm_system::update(bool sync)
{
    int frame = model->tick();

    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->update(frame, sync);
}

void scm_system::render_model(const double *M, int width, int height, int channel)
{
    if (scm_scene *scene = get_current_scene())
        model->draw(scene, M, width, height, channel);
}

void scm_system::render_cache()
{
    int ii = 0, nn = caches.size();

    for (active_cache_i i = caches.begin(); i != caches.end(); ++i, ++ii)
        i->second.cache->render(ii, nn);
}

//------------------------------------------------------------------------------

// Allocate and insert a new scene before i. Return its index.

int scm_system::add_scene(int i)
{
    int j = -1;

    if (scm_scene *scene = new scm_scene(this))
    {
        scm_scene_i it = scenes.insert(scenes.begin() + i, scene);
        j         = it - scenes.begin();
    }
    scm_log("scm_system add_scene %d = %d", i, j);

    return j;
}

// Delete the scene at i.

void scm_system::del_scene(int i)
{
    scm_log("scm_system del_scene %d", i);

    delete scenes[i];
    scenes.erase(scenes.begin() + i);
}

// Return a pointer to the scene at i.

scm_scene *scm_system::get_scene(int i)
{
    return scenes[i];
}

//------------------------------------------------------------------------------

float scm_system::get_current_bottom() const
{
    if (scm_scene *scene = get_current_scene())
        return scene->get_height_bottom();
    else
        return 1.0;
}

float scm_system::get_current_height() const
{
#if 0
    double v[3];

    here.get_position(v);

    if (scm_scene *scene = get_current_scene())
        return scene->get_height_sample(v);
    else
#endif
        return 1.0;
}

scm_scene *scm_system::get_current_scene() const
{
    int s = int(scenes.size());
    int t = int(timer);

    if (s)
    {
        while (t <  0) t += s;
        while (t >= s) t -= s;

        return scenes[t];
    }
    return 0;
}

//------------------------------------------------------------------------------

float scm_system::get_page_sample(int f, const double *v)
{
    if (scm_file *file = get_file(f))
        return file->get_page_sample(v);
    else
        return 0.f;
}

void scm_system::get_page_bounds(int f, long long i, float& r0, float& r1)
{
    if (scm_file *file = get_file(f))
        file->get_page_bounds(uint64(i), r0, r1);
}

bool scm_system::get_page_status(int f, long long i)
{
    if (scm_file *file = get_file(f))
        return file->get_page_status(uint64(i));
    else
        return false;
}

//------------------------------------------------------------------------------

int scm_system::acquire_scm(const std::string& name)
{
    scm_log("acquire_scm %s", name.c_str());

    // If the file is loaded, note another usage.

    if (files[name].file)
        files[name].uses++;
    else
    {
        // Otherwise load the file and confirm its validity.

        scm_file *file = new scm_file(name);

        if (file->is_valid())
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
        }
        else
        {
            delete file;
            return -1;
        }
    }
    return files[name].index;
}

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

        // Release the associated cache and delete it if no uses remain.

        cache_param cp(files[name].file);

        if (--caches[cp].uses == 0)
        {
            delete caches[cp].cache;
            caches.erase(cp);
        }

        delete files[name].file;
        files.erase(name);
    }
    return -1;
}

//------------------------------------------------------------------------------

// Return the cache associated with the given file index.

scm_cache *scm_system::get_cache(int index)
{
    if (pairs.find(index) == pairs.end())
        return 0;
    else
        return pairs[index].cache;
}

// Return the file associated with the given file index.

scm_file *scm_system::get_file(int index)
{
    if (pairs.find(index) == pairs.end())
        return 0;
    else
        return pairs[index].file;
}

// Return an open TIFF pointer for the file with the given index. This function
// is called by the loader threads, so the index map must be mutually exclusive.

TIFF *scm_system::get_tiff(int index)
{
    TIFF *T = 0;

    SDL_mutexP(mutex);
    {
        if (pairs.find(index) != pairs.end())
            T = pairs[index].file->open();
    }
    SDL_mutexV(mutex);

    return T;
}

//------------------------------------------------------------------------------
