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

#include <vector>

#include "util3d/glsl.h"
#include "scm-image.hpp"

//------------------------------------------------------------------------------

class scm_scene
{
public:

    scm_scene(scm_system *);
   ~scm_scene();

    // External Interface

    const std::string& get_name() const { return name; }
    const std::string& get_vert() const { return vert; }
    const std::string& get_frag() const { return frag; }

    void               set_name(const std::string &s) { name = s; }
    void               set_vert(const std::string &s);
    void               set_frag(const std::string &s);

    int                add_image(int);
    void               del_image(int);
    scm_image         *get_image(int);
    int                get_image_count() const { return int(images.size()); }

    // Internal Interface

    void   init_uniforms();

    GLuint bind(int) const;
    void unbind(int) const;

    GLuint bind_page(int, int, int, long long) const;
    void unbind_page(int, int)                 const;
    void  touch_page(int,      int, long long) const;

    float   get_minimum_height()               const;
    float   get_current_height(const double *) const;

    void    get_page_bounds(int, long long, float&, float &) const;
    bool    get_page_status(int, long long)                  const;


private:

    scm_system *sys;

    std::string name;
    std::string vert;
    std::string frag;

    scm_image_v images;
    glsl        render;
};

typedef std::vector<scm_scene *>           scm_scene_v;
typedef std::vector<scm_scene *>::iterator scm_scene_i;

//------------------------------------------------------------------------------

#endif
