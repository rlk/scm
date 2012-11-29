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

#ifndef SCM_IMAGE_HPP
#define SCM_IMAGE_HPP

#include <string>

#include "scm-cache.hpp"

//------------------------------------------------------------------------------

class scm_image
{
public:

    scm_image(const std::string&,
              const std::string&, scm_cache *, int, bool);

    void   bind(GLint, GLuint) const;
    void unbind(GLint)         const;

    void   bind_page(GLuint, int, int, long long) const;
    void unbind_page(GLuint, int)                 const;
    void  touch_page(long long, int)              const;

    float   get_page_sample(const double *)              const;
    void    get_page_bounds(long long, float &, float &) const;
    bool    get_page_status(long long)                   const;

    float   get_normal_min() const { return cache->get_r0(); }
    float   get_normal_max() const { return cache->get_r1(); }

    bool     is_channel(int c) const { return (chan == -1 || chan == c); }
    bool     is_height()       const { return height; }

private:

    std::string name;
    scm_cache  *cache;
    int         file;
    int         chan;
    bool        height;
};

typedef std::vector<scm_image *>                 scm_image_v;
typedef std::vector<scm_image *>::iterator       scm_image_i;
typedef std::vector<scm_image *>::const_iterator scm_image_c;

//------------------------------------------------------------------------------

#endif
