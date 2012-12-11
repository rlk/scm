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

#ifndef SCM_CACHE_HPP
#define SCM_CACHE_HPP

#include <vector>
#include <string>

#include <GL/glew.h>

#include "scm-queue.hpp"
#include "scm-fifo.hpp"
#include "scm-task.hpp"
#include "scm-set.hpp"

//------------------------------------------------------------------------------

class scm_system;

//------------------------------------------------------------------------------

class scm_cache
{
public:

    scm_cache(scm_system *, int, int, int);
   ~scm_cache();

    void   add_load(scm_task&);
    int    get_page(int, long long, int, int&);

    GLuint get_texture()    const { return texture; }
    int    get_grid_size()  const { return s;       }
    int    get_page_size()  const { return n;       }

    void   update(int, bool);
    void   render(int, int);
    void   flush ();

private:

    static const int cache_size    = 12;
    static const int cache_threads =  2;

    static const int need_queue_size      = 32;   // 32
    static const int load_queue_size      =  8;   //  8
    static const int max_loads_per_update =  2;   //  2

    scm_system         *sys;
    scm_set             pages;  // Page set currently active
    scm_set             waits;  // Page set currently being loaded
    scm_queue<scm_task> loads;  // Page loader queue
    scm_fifo<GLuint>    pbos;   // Asynchronous upload ring

    GLuint  texture;            // Atlas texture object
    int     s;                  // Atlas width and height in pages
    int     l;                  // Atlas current page
    int     n;                  // Page width and height in pixels
    int     c;                  // Channels per pixel
    int     b;                  // Bits per channel

    int get_slot(int, long long);
};

typedef std::vector<scm_cache *>           scm_cache_v;
typedef std::vector<scm_cache *>::iterator scm_cache_i;

//------------------------------------------------------------------------------

#endif
