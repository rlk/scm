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

/// An scm_atmo stores the parameters of a planetary atmosphere definition.

struct scm_atmo
{
    scm_atmo()
    {
        c[0] = 1.0f;
        c[1] = 1.0f;
        c[2] = 1.0f;
        P    = 0.0f;
        H    = 0.0f;
    }

    GLfloat c[3];
    GLfloat P;
    GLfloat H;
};

//------------------------------------------------------------------------------

/// An scm_scene encapsulates the definition of a sphere and its parameters
///
/// This definition consists primarily of a set of scm_image objects plus the
/// vertex and fragment shaders that reference and render them. In addition,
/// an scm_label gives annotations and a name string allows a scene to be
/// requested by name.

class scm_scene
{
public:

    scm_scene(scm_system *);
   ~scm_scene();

    /// @name Image collection handlers
    /// @{

    int                add_image(int i);
    void               del_image(int i);
    scm_image         *get_image(int i);
    int                get_image_count() const;

    /// @}
    /// @name Configuration
    /// @{

    void               set_color(GLuint c);
    void               set_clear(GLuint c);
    void               set_name (const std::string &s);
    void               set_label(const std::string &s);
    void               set_vert (const std::string &s);
    void               set_frag (const std::string &s);
    void               set_atmo (const scm_atmo&);


    /// @}
    /// @name Query
    /// @{

    GLuint             get_color() const { return color;      }
    GLuint             get_clear() const { return clear;      }
    const std::string& get_name () const { return name;       }
    const std::string& get_label() const { return label_file; }
    const std::string& get_vert () const { return  vert_file; }
    const std::string& get_frag () const { return  frag_file; }
    const scm_atmo&    get_atmo () const { return atmo;       }

    /// @}
    /// @name Internal Interface
    /// @{

    void   init_uniforms();
    void   draw_label();

    void   bind(int) const;
    void unbind(int) const;

    void   bind_page(int, int, int, long long) const;
    void unbind_page(int, int)                 const;
    void  touch_page(int,      int, long long) const;

    float   get_minimum_ground()               const;
    float   get_current_ground(const double *) const;

    void    get_page_bounds(int, long long, float&, float &) const;
    bool    get_page_status(int, long long)                  const;

    /// @}

private:

    scm_system *sys;

    std::string name;
    std::string label_file;
    std::string  vert_file;
    std::string  frag_file;

    scm_atmo    atmo;
    scm_label  *label;
    scm_image_v images;
    glsl        render;
    GLuint      color;
    GLuint      clear;

    // Uniform locations must be visible to the scm_sphere.

    friend class scm_sphere;

    GLint uA[16];
    GLint uB[16];
    GLint uM;
    GLint uzoomv;
    GLint uzoomk;
};

//------------------------------------------------------------------------------

#endif
