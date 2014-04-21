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

#ifndef SCM_LABEL_HPP
#define SCM_LABEL_HPP

#include <string>
#include <vector>

#include <GL/glew.h>

#include "util3d/type.h"
#include "util3d/glsl.h"

//-----------------------------------------------------------------------------

/// An scm_label renders annotations on the sphere
///
/// The label renderer reads data from a CSV file giving a list of
/// surface features, each with the following elements:
///
///    Key       | Type
///    --------- | --------
///    Name      | a string
///    Latitude  | in degrees
///    Longitude | in degrees
///    Diameter  | in meters
///    Radius    | distance from the center of the globe, in meters
///    Feature   | a two-letter code
///
/// The name string is rendered in text scaled to fit the given diameter.
/// The currently-defined two-letter codes are:
///
///    Code  | Feature          | Representation
///    ----  | ---------------- | --------------
///    AA    | Crater           | Circle of given diameter
///    SF    | Secondary Crater | Circle of given diameter with half opacity
///    MO    | Mountain         | Triangle icon
///    LF    | Landing Site     | Flag icon
///    \@*   | Star             | Star icon
///    \@C   | Circle           | Circle icon

class scm_label
{
public:

    scm_label(const std::string&, int);
   ~scm_label();

    void draw(GLubyte r, GLubyte g, GLubyte b, GLubyte a);

private:

    static const int strmax = 64;

    struct label
    {
        char  str[strmax];
        float lat;
        float lon;
        float dia;
        float rad;
        char  typ[2];

        bool circle() const {
            return ((typ[0] == 'A' && typ[1] == 'A')
                 || (typ[0] == 'S' && typ[1] == 'F'));
        }
        bool sprite() const {
            return ((typ[0] == 'L' && typ[1] == 'F')
                 || (typ[0] == 'M' && typ[1] == 'O')
                 || (typ[0] == '@' && typ[1] == '*')
                 || (typ[0] == '@' && typ[1] == 'C'));
        }
        bool latlon() const {
            return ((typ[0] == '@' && typ[1] == '#'));
        }
    };

    int  scan (FILE *, label&);
    void parse(const std::string&);
    void apply(label *);

    font *label_font;
    line *label_line;

    int    num_circles;
    int    num_sprites;
    int    num_latlons;
    int    sprite_size;

    glsl   circle_glsl;
    glsl   sprite_glsl;

    GLuint circle_vbo;
    GLuint sprite_vbo;
    GLuint latlon_vbo;
    GLuint sprite_tex;

    std::vector<label> labels;
};

//-----------------------------------------------------------------------------

#endif
