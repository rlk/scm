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
#include <string>

#include "util3d/glsl.h"

//------------------------------------------------------------------------------

class scm_system;
class scm_label;
class scm_image;

typedef std::vector<scm_image *>                 scm_image_v;
typedef std::vector<scm_image *>::iterator       scm_image_i;
typedef std::vector<scm_image *>::const_iterator scm_image_c;

//------------------------------------------------------------------------------

class scm_scene
{
public:

    scm_scene(scm_system *);
   ~scm_scene();

    // External Interface

    const std::string& get_name () const { return name;       }
    const std::string& get_label() const { return label_file; }
    const std::string& get_vert () const { return  vert_file; }
    const std::string& get_frag () const { return  frag_file; }

    void               set_name (const std::string &s) { name = s; }
    void               set_label(const std::string &s);
    void               set_vert (const std::string &s);
    void               set_frag (const std::string &s);

    int                add_image(int);
    void               del_image(int);
    scm_image         *get_image(int);
    int                get_image_count() const { return int(images.size()); }

    // Internal Interface

    void   init_uniforms();

    void   bind(int) const;
    void unbind(int) const;

    void   bind_page(int, int, int, long long) const;
    void unbind_page(int, int)                 const;
    void  touch_page(int,      int, long long) const;

    float   get_minimum_ground()               const;
    float   get_current_ground(const double *) const;

    void    get_page_bounds(int, long long, float&, float &) const;
    bool    get_page_status(int, long long)                  const;

    GLint   uA[16];
    GLint   uB[16];
    GLint   uM;

private:

    scm_system *sys;

    std::string name;
    std::string label_file;
    std::string  vert_file;
    std::string  frag_file;

    scm_label  *label;
    scm_image_v images;
    glsl        render;
};

//------------------------------------------------------------------------------

#endif
