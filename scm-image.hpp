//  Copyright (C) 2005-2011 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

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

    void bind(GLint, GLuint) const;
    void free(GLint)         const;

    void set_texture(GLuint, int, int, long long) const;
    void clr_texture(GLuint, int)                 const;

    void bounds(long long, float &, float &) const;
    bool status(long long)                   const;
    void touch (long long, int)              const;

    bool is_channel(int c) const { return (chan == -1 || chan == c); }
    bool is_height()       const { return height; }

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
