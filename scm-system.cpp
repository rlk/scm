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

//------------------------------------------------------------------------------

scm_system::scm_system() : serial(1)
{
    mutex = SDL_CreateMutex();
}

scm_system::~scm_system()
{
    SDL_DestroyMutex(mutex);
}

//------------------------------------------------------------------------------

// Allocate and insert a new scene before i. Return its index.

int scm_system::add_scene(int i)
{
    if (scm_scene *scene = new scm_scene(this))
        return int(scenes.insert(scenes.begin() + i, scene) - scenes.begin());
    else
        return -1;
}

// Delete the scene at i.

void scm_system::del_scene(int i)
{
    delete scenes[i];
    scenes.erase(scenes.begin() + i);
}

// Return a pointer to the scene at i.

scm_scene *scm_system::get_scene(int i)
{
    return scenes[i];
}

//------------------------------------------------------------------------------

int scm_system::_acquire_scm(const std::string& name)
{
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

            pairs[index] = active_pair(files[name].file, caches[cp].cache);
        }
        else
        {
            delete file;
            return -1;
        }
    }
    return files[name].index;
}

int scm_system::_release_scm(const std::string& name)
{
    // Release the named file and delete it if no uses remain.

    if (--files[name].uses == 0)
    {
        // Remove the index from the reverse look-up.

        pairs.erase(files[name].index);

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

int scm_system::acquire_scm(const std::string& name)
{
    int index = -1;

    SDL_mutexP(mutex);
    index = _acquire_scm(name);
    SDL_mutexV(mutex);

    return index;
}

int scm_system::release_scm(const std::string& name)
{
    int index = -1;

    SDL_mutexP(mutex);
    index = _release_scm(name);
    SDL_mutexV(mutex);

    return index;
}

scm_cache *scm_system::get_cache(int index)
{
    scm_cache *cache = 0;

    SDL_mutexP(mutex);
    {
        if (pairs.find(index) != pairs.end())
            cache = pairs[index].cache;
    }
    SDL_mutexV(mutex);

    return cache;
}

scm_file *scm_system::get_file(int index)
{
    scm_file *file = 0;

    SDL_mutexP(mutex);
    {
        if (pairs.find(index) != pairs.end())
            file = pairs[index].file;
    }
    SDL_mutexV(mutex);

    return file;
}

//------------------------------------------------------------------------------
