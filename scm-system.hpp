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

#ifndef SCM_SYSTEM_HPP
#define SCM_SYSTEM_HPP

#include <map>
#include <set>

/* Modified by Kevin */
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
/*********************/

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

struct active_pair
{
    active_pair()                          : file(0), cache(0) { }
    active_pair(scm_file *f, scm_cache *c) : file(f), cache(c) { }

    scm_file  *file;
    scm_cache *cache;
};

typedef std::map<int, active_pair> active_pair_m;

//------------------------------------------------------------------------------

struct active_file
{
    active_file() : file(0), uses(0), index(-1) { }

    scm_file  *file;
    int        uses;
    int        index;
};

typedef std::map<std::string, active_file> active_file_m;

//------------------------------------------------------------------------------

struct active_cache
{
    active_cache() : cache(0), uses(0) { }

    scm_cache *cache;
    int        uses;
};

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

class scm_system
{
public:

    // External Interface

    scm_system(int, int, int, int);
   ~scm_system();

    void     render_sphere(const double *, const double *, int) const;

    void      flush_cache();
    void     render_cache();
    void     update_cache();

    void      flush_queue();
    void     import_queue(const std::string&);
    void     export_queue(      std::string&);
    void     append_queue(scm_step *);
    void     render_queue();

    int         add_scene(int);
    void        del_scene(int);
    scm_scene  *get_scene(int);

    int         add_step(int);
    void        del_step(int);
    scm_step   *get_step(int);

    int         get_scene_count()      const { return int(scenes.size()); }
    double      set_scene_blend(double);

    int         get_step_count()       const { return int( steps.size()); }
    scm_step    get_step_blend(double) const;

    bool        get_synchronous() const { return sync; }
    void        set_synchronous(bool b) { sync = b;    }

    float       get_current_ground(const double *) const;
    float       get_minimum_ground()               const;

    scm_sphere *get_sphere() const { return sphere; }
    scm_render *get_render() const { return render; }

    scm_scene  *get_fore() const { return fore0; }
    scm_scene  *get_back() const { return back0; }

    // Internal Interface

    int     acquire_scm(const std::string&);
    int     release_scm(const std::string&);

    scm_scene *find_scene(const std::string&) const;
    scm_cache  *get_cache(int);
    scm_file   *get_file (int);

    float       get_page_sample(int, const double *v);
    bool        get_page_status(int, long long);
    void        get_page_bounds(int, long long, float&, float&);

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

    int    serial;
    int    frame;
    bool   sync;
    double fade;
};

//------------------------------------------------------------------------------

#endif
