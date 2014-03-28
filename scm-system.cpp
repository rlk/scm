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

#include <sstream>
#include <iomanip>
#include <cassert>
#include <limits>
#include <cmath>
#include "util3d/math3d.h"

#include "scm-step.hpp"
#include "scm-scene.hpp"
#include "scm-cache.hpp"
#include "scm-sphere.hpp"
#include "scm-render.hpp"
#include "scm-system.hpp"
#include "scm-log.hpp"

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

//------------------------------------------------------------------------------

scm_system::scm_system(int w, int h, int d, int l) :
    serial(1), frame(0), sync(false), fade(0)
{
    scm_log("scm_system working directory is %s", getcwd(0, 0));

    mutex  = SDL_CreateMutex();
    render = new scm_render(w, h);
    sphere = new scm_sphere(d, l);
    fore0 = 0;
    fore1 = 0;
    back0 = 0;
    back1 = 0;
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

void scm_system::render_sphere(const double *P,
                               const double *M, int channel) const
{
    render->render(sphere, fore0, fore1,
                           back0, back1, P, M, channel, frame, fade);
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

    if (nn < 2)
        nn = 2;

    for (active_cache_i i = caches.begin(); i != caches.end(); ++i, ++ii)
        i->second.cache->render(ii, nn);
}

void scm_system::update_cache()
{
    for (active_cache_i i = caches.begin(); i != caches.end(); ++i)
        i->second.cache->update(frame, sync);
    frame++;
}

//------------------------------------------------------------------------------

// Flush the current step queue and delete all steps.

void scm_system::flush_queue()
{
    for (size_t i = 0; i < queue.size(); i++)
        delete queue[i];

    queue.clear();
}

// Parse the given string as a series of camera states. Enqueue each.

void scm_system::import_queue(const std::string& data)
{
    std::stringstream file(data);
    std::string       line;

    queue.clear();

    int n = 0;

    while (std::getline(file, line))
    {
        std::stringstream in(line);

        double t[3] = { 0, 0, 0 };
        double r[3] = { 0, 0, 0 };
        double l[3] = { 0, 0, 0 };

        if (in) in >> t[0] >> t[1] >> t[2];
        if (in) in >> r[0] >> r[1] >> r[2];
        if (in) in >> l[0] >> l[1] >> l[2];

        r[0] = radians(r[0]);
        r[1] = radians(r[1]);
        r[2] = radians(r[2]);

        l[0] = radians(l[0]);
        l[1] = radians(l[1]);
        l[2] = radians(l[2]);

        scm_step *S = new scm_step(t, r, l);

        if (fore0) S->set_foreground(fore0->get_name());
        if (back0) S->set_background(back0->get_name());

        append_queue(S);

        n++;
    }
}

void scm_system::export_queue(std::string& data)
{
    std::stringstream file;

    for (size_t i = 0; i < queue.size(); ++i)
    {
        double d = queue[i]->get_distance();
        double p[3];
        double q[4];
        double r[3];

        queue[i]->get_position(p);
        queue[i]->get_orientation(q);

        p[0] *= d;
        p[1] *= d;
        p[2] *= d;

        equaternion(r, q);

        file << std::setprecision(std::numeric_limits<long double>::digits10)
             << p[0] << " "
             << p[1] << " "
             << p[2] << " "
             << degrees(r[0]) << " "
             << degrees(r[1]) << " "
             << degrees(r[2]) << " "
             << "0.0 0.0 0.0" << std::endl;
    }
    data = file.str();
}

// Take ownership of the given step and append it to the current queue.

void scm_system::append_queue(scm_step *s)
{
    queue.push_back(s);
}

// Render the current queue as an interpolated path.

void scm_system::render_queue()
{
    if (!queue.empty())
    {
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        {
            size_t n = queue.size() - 1;

            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_POINT_SMOOTH);
            glEnable(GL_DEPTH_CLAMP);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            // Draw a line of small points to indicate the path.

            glPointSize(2.0);
            glBegin(GL_POINTS);
            {
                glColor4f(1.0f, 1.0f, 0.0, 0.5f);
                for (size_t i = 1; i < n; i++)
                    queue[i]->draw();
            }
            glEnd();

            // Draw large points to indicate the first and last steps.

            glPointSize(6.0);
            glBegin(GL_POINTS);
            {
                glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
                queue[0]->draw();

                glColor4f(1.0f, 0.0f, 0.0f, 0.8f);
                queue[n]->draw();
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
        scm_scene_i it = scenes.insert(scenes.begin() + std::max(i, 0), scene);
        j         = it - scenes.begin();

        if (fore0 == 0) fore0 = scene;
        if (fore1 == 0) fore1 = scene;
    }
    scm_log("scm_system add_scene %d = %d", i, j);

    return j;
}

// Delete the scene at i.

void scm_system::del_scene(int i)
{
    scm_log("scm_system del_scene %d", i);

    if (scenes[i] == fore0) fore0 = 0;
    if (scenes[i] == back0) back0 = 0;
    if (scenes[i] == fore1) fore1 = 0;
    if (scenes[i] == back1) back1 = 0;

    delete scenes[i];
    scenes.erase(scenes.begin() + i);
}

// Return a pointer to the scene at i.

scm_scene *scm_system::get_scene(int i)
{
    if (0 <= i && i < int(scenes.size()))
        return scenes[i];
    else
        return 0;
}

//------------------------------------------------------------------------------

// Allocate and insert a new step before i. Return its index.

int scm_system::add_step(int i)
{
    int j = -1;

    if (scm_step *step = new scm_step())
    {
        scm_step_i it = steps.insert(steps.begin() + std::max(i, 0), step);
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
    if (0 <= i && i < int(steps.size()))
        return steps[i];
    else
        return 0;
}

//------------------------------------------------------------------------------

// Compute the interpolated values of the current step queue at the given time.
// Extrapolate the first and last steps to produce a clean start and stop.
#if 0
scm_step scm_system::get_step_blend(double t) const
{
    t = std::max(t, 0.0);
    t = std::min(t, double(queue.size() - 1));

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
#else
scm_step scm_system::get_step_blend(double t) const
{
    t = std::max(t, 0.0);
    t = std::min(t, double(queue.size() - 1));

    int i = int(floor(t));

    return queue[i];
}
#endif

// Set the scene caches and fade coefficient to give the rendering of the
// current step queue at the given time.

double scm_system::set_scene_blend(double t)
{
    if (!queue.empty())
    {
        t = std::max(t, 0.0);
        t = std::min(t, double(queue.size() - 1));

        scm_step *step0 = queue[int(floor(t))];
        scm_step *step1 = queue[int( ceil(t))];

#if 1
        fore0 = find_scene(step0->get_foreground());
        fore1 = find_scene(step1->get_foreground());
        back0 = find_scene(step0->get_background());
        back1 = find_scene(step1->get_background());
#else
        scm_scene *temp;

        if ((temp = find_scene(step0->get_foreground()))) fore0 = temp;
        if ((temp = find_scene(step1->get_foreground()))) fore1 = temp;
        if ((temp = find_scene(step0->get_background()))) back0 = temp;
        if ((temp = find_scene(step1->get_background()))) back1 = temp;
#endif

        fade = t - floor(t);
        return t;
    }
    else
    {
        fade = 0;
        return 0;
    }
}

// Return the ground level of current scene at the given location.

float scm_system::get_current_ground(const double *v) const
{
    if (fore0 && fore1)
        return std::max(fore0->get_current_ground(v),
                        fore1->get_current_ground(v));
    if (fore0)
        return fore0->get_current_ground(v);
    if (fore1)
        return fore1->get_current_ground(v);
    return 1.f;
}

// Return the minimum ground level of the current scene.

float scm_system::get_minimum_ground() const
{
    if (fore0 && fore1)
        return std::min(fore0->get_minimum_ground(),
                        fore1->get_minimum_ground());
    if (fore0)
        return fore0->get_minimum_ground();
    if (fore1)
        return fore1->get_minimum_ground();
    return 1.f;
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
        // Otherwise load the file.

        if (scm_file *file = new scm_file(name))
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

// Return the scene with the given name.

scm_scene *scm_system::find_scene(const std::string& name) const
{
    for (size_t i = 0; i < scenes.size(); i++)
        if (scenes[i]->get_name() == name)
            return scenes[i];

    return 0;
}

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
