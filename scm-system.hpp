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

#include <SDL.h>
#include <SDL_thread.h>

#include "scm-scene.hpp"
#include "scm-cache.hpp"
#include "scm-model.hpp"
#include "scm-file.hpp"

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
    cache_param(scm_file *file) : n(file->get_w() - 2), c(file->get_c()),
                                                        b(file->get_b()) { }
    int n;
    int c;
    int b;
    bool operator<(const cache_param& that) const {
        if      (n < that.n) return true;
        else if (c < that.c) return true;
        else if (b < that.b) return true;
        else                 return false;
    }
};

typedef std::map<cache_param, active_cache>           active_cache_m;
typedef std::map<cache_param, active_cache>::iterator active_cache_i;

//------------------------------------------------------------------------------
#if 0
    int        add_step(int);
    void       del_step(int);
    scm_step  *get_step(int);
    int        get_step_count() const;
#endif

class scm_system
{
public:

    // External Interface

    scm_system();
   ~scm_system();

    void update(bool);
    void render_model(const double *, int, int, int);
    void render_cache();

    int        add_scene(int);
    void       del_scene(int);
    scm_scene *get_scene(int);
    int        get_scene_count() const { return int(scenes.size()); }

    float      get_current_bottom() const;
    float      get_current_height() const;
    scm_scene *get_current_scene()  const;

    // Internal Interface

    int acquire_scm(const std::string&);
    int release_scm(const std::string&);

    scm_cache *get_cache(int);
    scm_file  *get_file (int);
    TIFF      *get_tiff (int);

    float get_page_sample(int, const double *v);
    void  get_page_bounds(int, long long, float&, float&);
    bool  get_page_status(int, long long);

private:

    int _acquire_scm(const std::string&);
    int _release_scm(const std::string&);

    scm_scene_v scenes;
    scm_model  *model;

    active_file_m  files;
    active_cache_m caches;
    active_pair_m  pairs;

    SDL_mutex *mutex;

    int    serial;
    double timer;

#if 0
    scm_path_p  path;
    scm_step    here;
#endif
};

//------------------------------------------------------------------------------

#endif
