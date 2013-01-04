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

#include <cassert>
#include <cmath>
#include "util3d/math3d.h"

#include "scm-step.hpp"
#include "scm-scene.hpp"
#include "scm-cache.hpp"
#include "scm-sphere.hpp"
#include "scm-render.hpp"
#include "scm-system.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_system::scm_system(int w, int h, int d, int l) :
    serial(1), frame(0), time(0)
{
    mutex  = SDL_CreateMutex();
    sphere = new scm_sphere(d, l);
    render = new scm_render(w, h);
    scene0 = 0;
    scene1 = 0;
}

scm_system::~scm_system()
{
    while (get_scene_count())
        del_scene(0);

    delete sphere;
    delete render;

    SDL_DestroyMutex(mutex);
}

//------------------------------------------------------------------------------

void scm_system::render_sphere(const double *M, int channel) const
{
    const double t = time - floor(time);

    if (scene0 && scene1)
        render->render(sphere, scene0, scene1, M, t, channel, frame);
}

//------------------------------------------------------------------------------

void scm_system::flush_cache()
{
    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->flush();
}

void scm_system::render_cache()
{
    int ii = 0, nn = caches.size();

    for (active_cache_i i = caches.begin(); i != caches.end(); ++i, ++ii)
        i->second.cache->render(ii, nn);
}

void scm_system::update_cache(bool sync)
{
    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->update(frame, sync);
    frame++;
}

//------------------------------------------------------------------------------

// Flush the current step queue. The queue does NOT own the steps so it does not
// delete them. They are owned by the step vector or by the caller.

void scm_system::flush_queue()
{
    queue.clear();
}

// Append the given step to the current queue.

void scm_system::append_queue(scm_step *s)
{
    queue.push_back(s);
}

// Render the current queue as an interpolated path.

void scm_system::render_queue()
{
    if (size_t n = queue.size() - 1)
    {
        glPushAttrib(GL_ENABLE_BIT);
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Draw a line to indicate the path.

            glColor4f(0.0f, 1.0f, 0.0, 0.5f);
            glLineWidth(2.0);
            glBegin(GL_LINE_STRIP);
            {
                for (size_t i = 0; i <= n * 10; i++)
                    interpolate(double(i) / 10.0).draw();
            }
            glEnd();

            // Draw the steps with colors to indicate first and last.

            glPointSize(8.0);
            glBegin(GL_POINTS);
            {
                for (size_t i = 0; i <= n; ++i)
                {
                    if      (i == 0) glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
                    else if (i == n) glColor4f(1.0f, 0.0f, 0.0f, 0.8f);
                    else             glColor4f(1.0f, 0.5f, 0.0f, 0.6f);

                    queue[i]->draw();
                }
            }
            glEnd();
        }
        glPopAttrib();
    }
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

        scene0 = scene;
        scene1 = scene;
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

// Allocate and insert a new step before i. Return its index.

int scm_system::add_step(int i)
{
    int j = -1;

    if (scm_step *step = new scm_step())
    {
        scm_step_i it = steps.insert(steps.begin() + i, step);
        j        = it - steps.begin();
    }
    scm_log("scm_system add_step %d = %d", i, j);

    return j;
}

// Delete the step at i.

void scm_system::del_step(int i)
{
    scm_log("scm_system del_step %d", i);

    delete steps[i];
    steps.erase(steps.begin() + i);
}

// Return a pointer to the step at i.

scm_step *scm_system::get_step(int i)
{
    return steps[i];
}

//------------------------------------------------------------------------------
#if 0
void scm_system::set_current_step(int s)
{
    if (!steps.empty())
    {
        s = std::max(s, 0);
        s = std::min(s, int(steps.size() - 1));

        scm_step *step0 = steps[int(floor(s))];
        scm_step *step1 = steps[int( ceil(s))];

        for (size_t i = 0; i < scenes.size(); i++)
        {
            if (scenes[i]->get_name() == step0->get_scene()) scene0 = scenes[i];
            if (scenes[i]->get_name() == step1->get_scene()) scene1 = scenes[i];
        }
    }
}
#endif

void scm_system::set_current_time(double t)
{
    time = t;
    time = std::max(time, 0.0);
    time = std::min(time, double(queue.size() - 1));
}

void scm_system::set_current_scene(int s)
{
    if (!scenes.empty())
    {
        s = std::max(s, 0);
        s = std::min(s, int(scenes.size() - 1));

        scene0 = scenes[s];
        scene1 = scenes[s];
    }
}

float scm_system::get_current_ground(const double *v) const
{
    if (scene0 && scene1)
        return std::max(scene0->get_current_ground(v),
                        scene1->get_current_ground(v));
    else
        return 1.f;
}

float scm_system::get_minimum_ground() const
{
    if (scene0 && scene1)
        return std::min(scene0->get_minimum_ground(),
                        scene1->get_minimum_ground());
    else
        return 1.f;
}

scm_sphere *scm_system::get_sphere() const
{
    return sphere;
}

scm_render *scm_system::get_render() const
{
    return render;
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

            file->activate(caches[cp].cache);
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

//------------------------------------------------------------------------------

float scm_system::get_page_sample(int f, const double *v)
{
    if (scm_file *file = get_file(f))
        return file->get_page_sample(v);
    else
        return 1.f;
}

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

bool scm_system::get_page_status(int f, long long i)
{
    if (scm_file *file = get_file(f))
        return file->get_page_status(uint64(i));
    else
        return false;
}

//------------------------------------------------------------------------------

// Compute the interpolated values of the current step queue at the given time.
// Extrapolate the first and last steps to produce a clean start and stop.

scm_step scm_system::interpolate(double t) const
{
    int    i = int(floor(t));
    double k = t - floor(t);

    if (queue.size() == 0) return scm_step();
    if (queue.size() == 1) return *queue[0];
    if (t            == 0) return *queue[0];
    if (k            == 0) return *queue[i];

    int n = queue.size() - 2;
    int m = queue.size() - 1;

    scm_step a = (i > 0) ? *queue[i - 1] : scm_step(queue[0], queue[1], -1.0);
    scm_step b =           *queue[i    ];
    scm_step c =           *queue[i + 1];
    scm_step d = (i < n) ? *queue[i + 2] : scm_step(queue[n], queue[m], +2.0);

    return scm_step(&a, &b, &c, &d, k);
}

//------------------------------------------------------------------------------
