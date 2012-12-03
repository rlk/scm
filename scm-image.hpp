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
#include <set>

//------------------------------------------------------------------------------

class scm_system;
class scm_cache;

//------------------------------------------------------------------------------

class scm_image
{
public:

    // External Interface

    const std::string& get_scm()        const { return scm;     }
    const std::string& get_name()       const { return name;    }
    bool               get_height()     const { return height;  }
    int                get_channel()    const { return channel; }
    float              get_normal_min() const { return k0;      }
    float              get_normal_max() const { return k1;      }

    void    set_scm       (const std::string& s);
    void    set_name      (const std::string& s) { name = s; }
    void    set_height    (bool  h)              { height  = h; }
    void    set_channel   (int   c)              { channel = c; }
    void    set_normal_min(float k)              { k0      = k; }
    void    set_normal_max(float k)              { k1      = k; }

    // Internal Interface

    scm_image(scm_system *);

    void   bind(GLuint, GLuint) const;
    void unbind(GLuint)         const;

    void   bind_page(GLuint, int, int, long long) const;
    void unbind_page(GLuint, int)                 const;
    void  touch_page(long long, int)              const;

    float   get_page_sample(const double *)              const;
    void    get_page_bounds(long long, float &, float &) const;
    bool    get_page_status(long long)                   const;

private:

    scm_system *sys;
    std::string scm;
    std::string name;
    int         channel;
    bool        height;
    float       k0;
    float       k1;

    scm_cache  *cache;
    int         index;
};

typedef std::set<scm_image *>           scm_image_s;
typedef std::set<scm_image *>::iterator scm_image_i;

//------------------------------------------------------------------------------

#endif
