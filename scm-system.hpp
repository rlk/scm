// Copyright (C) 2011-2014 Robert Kooima
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

#ifndef SCM_SYSTEM_HPP
#define SCM_SYSTEM_HPP

#include <map>
#include <set>

#include <SDL.h>
#include <SDL_thread.h>

#include "scm-file.hpp"
#include "scm-step.hpp"

//------------------------------------------------------------------------------

class scm_scene;
class scm_cache;
class scm_sphere;
class scm_render;

typedef std::vector<scm_step *>           scm_step_v;
typedef std::vector<scm_step *>::iterator scm_step_i;

typedef std::vector<scm_scene *>           scm_scene_v;
typedef std::vector<scm_scene *>::iterator scm_scene_i;

//------------------------------------------------------------------------------

/// An active_pair structure allows an scm_system to associate an scm_file
/// object with the scm_cache that caches its data.

struct active_pair
{
    active_pair()                          : file(0), cache(0) { }
    active_pair(scm_file *f, scm_cache *c) : file(f), cache(c) { }

    scm_file  *file;
    scm_cache *cache;
};

typedef std::map<int, active_pair> active_pair_m;

//------------------------------------------------------------------------------

/// An active_file structure allows an scm_system to perform reference-counted
/// management of open scm_file objects.

struct active_file
{
    active_file() : file(0), uses(0), index(-1) { }

    scm_file  *file;
    int        uses;
    int        index;
};

typedef std::map<std::string, active_file> active_file_m;

//------------------------------------------------------------------------------

/// An active_cache structure allows an scm_system to perform reference-counted
/// management of functioning scm_cache objects.

struct active_cache
{
    active_cache() : cache(0), uses(0) { }

    scm_cache *cache;
    int        uses;
};

/// A cache_param structure represents the format of the image data that a
/// cache contains.
///
/// n is size, c is channel count, and b is byte count. Partial ordering is
/// defined, enabling log n search. This lets an scm_system combine similar
/// images in a single cache.

struct cache_param
{
    cache_param(scm_file *file) : n(int(file->get_w()) - 2),
                                  c(int(file->get_c())),
                                  b(int(file->get_b())) { }
    int n;
    int c;
    int b;
    bool operator<(const cache_param& that) const {
        if      (n < that.n) return true;
        else if (n > that.n) return false;
        else if (c < that.c) return true;
        else if (c > that.c) return false;
        else if (b < that.b) return true;
        else                 return false;
    }
};

typedef std::map<cache_param, active_cache>           active_cache_m;
typedef std::map<cache_param, active_cache>::iterator active_cache_i;

//------------------------------------------------------------------------------

/// An scm_system encapsulates all of the state of an SCM renderer. Its
/// interface is the primary API of the SCM rendering library.
///
/// The SCM system maintains the list of scenes and steps currently held open
/// by an application, all of the image that these scenes and steps refer to,
/// all of the caches that store the data of these images, the sphere manager
/// used to render it, and the render handler that manages this rendering.
/// A queue of steps enables the recording and playback of camera motion.

class scm_system
{
public:

    scm_system(int w, int h, int d, int l);
   ~scm_system();

    void     render_sphere(const double *, const double *, int) const;

    /// @name System queries
    /// @{

    scm_sphere *get_sphere() const;
    scm_render *get_render() const;
    scm_scene  *get_fore()   const;
    scm_scene  *get_back()   const;

    /// @}
    /// @name Scene collection handlers
    /// @{

    int         add_scene(int i);
    void        del_scene(int i);
    scm_scene  *get_scene(int i);

    int         get_scene_count() const;
    double      set_scene_blend(double);

    /// @}
    /// @name Step collection handlers
    /// @{

    int         add_step(int i);
    void        del_step(int i);
    scm_step   *get_step(int i);

    int         get_step_count()       const;
    scm_step    get_step_blend(double) const;

    /// @}
    /// @name Step queue handlers
    /// @{

    void     import_queue(const std::string&);
    void     export_queue(      std::string&);
    void     append_queue(scm_step *);
    void      flush_queue();

    /// @}
    /// @name Cache handlers
    /// @{

    void     update_cache();
    void     render_cache();
    void      flush_cache();

    void        set_synchronous(bool);
    bool        get_synchronous() const;

    /// @}
    /// @name Data queries
    /// @{

    float       get_current_ground(const double *) const;
    float       get_minimum_ground()               const;

    /// @}
    /// @name Internal Interface
    /// @{

    int     acquire_scm(const std::string&);
    int     release_scm(const std::string&);

    scm_scene *find_scene(const std::string&) const;
    scm_cache  *get_cache(int);
    scm_file   *get_file (int);

    float       get_page_sample(int f, const double *v);
    bool        get_page_status(int f, long long i);
    void        get_page_bounds(int f, long long i, float& r0, float& r1);

    /// @}

private:

    SDL_mutex     *mutex;

    scm_step_v     steps;
    scm_step_v     queue;
    scm_scene_v    scenes;

    scm_render    *render;
    scm_sphere    *sphere;
    scm_scene     *fore0;
    scm_scene     *fore1;
    scm_scene     *back0;
    scm_scene     *back1;

    active_file_m  files;
    active_cache_m caches;
    active_pair_m  pairs;

    int            serial;
    int            frame;
    bool           sync;
    double         fade;
};

//------------------------------------------------------------------------------

#endif
