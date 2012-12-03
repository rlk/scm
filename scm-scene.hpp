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

#ifndef SCM_SCENE_HPP
#define SCM_SCENE_HPP

#include <list>

#include "scm-image.hpp"

//------------------------------------------------------------------------------

class scm_scene
{
public:

    scm_scene();

    // External Interface

    void       add_image(int);
    scm_image *get_image(int);
    void       rem_image(int);

    // Internal Interface

//  void add_image(scm_image *);
//  void rem_image(scm_image *);

    void   bind(int, GLuint) const;
    void unbind(int)         const;

    void   bind_page(GLuint, int, int, int, long long) const;
    void unbind_page(GLuint, int, int)                 const;
    void  touch_page(int, int, long long);

    void    get_page_bounds(int, long long, float&, float &) const;
    bool    get_page_status(int, long long)                  const;

    float   get_height_sample(const double *) const;
    float   get_height_bottom()               const;

private:

    scm_image_s images;
    scm_image  *height;
};

typedef std::list<scm_scene *>           scm_scene_l;
typedef std::list<scm_scene *>::iterator scm_scene_i;

//------------------------------------------------------------------------------

#endif
