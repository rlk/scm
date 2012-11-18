//  Copyright (C) 2005-2012 Robert Kooima
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

#ifndef SCM_LABEL_HPP
#define SCM_LABEL_HPP

#include <string>
#include <vector>

#include <GL/glew.h>

#include "util3d/type.h"
#include "util3d/glsl.h"

//-----------------------------------------------------------------------------

// IAU Type codes:
//
//     AL ... Albedo Feature
//     AR ... Arcus
//     CA ... Catena
//     CB ... Cavus
//     CH ... Chaos
//     CM ... Chasma
//     CO ... Collis
//     CR ... Corona
//     AA ... Crater
//     DO ... Dorsum
//     ER ... Eruptive center
//     FA ... Facula
//     FR ... Farrum
//     FE ... Flexus
//     FL ... Fluctus
//     FM ... Flumen
//     FO ... Fossa
//     IN ... Insula
//     LA ... Labes
//     LB ... Labyrinthus
//     LC ... Lacus
//     LF ... Landing site name
//     LG ... Large ringed feature
//     LI ... Linea
//     LN ... Lingula
//     MA ... Macula
//     ME ... Mare
//     MN ... Mensa
//     MO ... Mons
//     OC ... Oceanus
//     PA ... Palus
//     PE ... Patera
//     PL ... Planitia
//     PM ... Planum
//     PU ... Plume
//     PR ... Promontorium
//     RE ... Regio
//     RI ... Rima
//     RU ... Rupes
//     SF ... Satellite Feature
//     SC ... Scopulus
//     SI ... Sinus
//     SU ... Sulcus
//     TA ... Terra
//     TE ... Tessera
//     TH ... Tholus
//     UN ... Unda
//     VA ... Vallis
//     VS ... Vastitas
//     VI ... Virga

//-----------------------------------------------------------------------------

class scm_label
{
public:

    scm_label(const void *, size_t,
              const void *, size_t, double, int);
   ~scm_label();

    void draw();

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
    };

    int  scan (const char *, label&);
    void parse(const void *, size_t, double);
    void apply(label *);

    font *label_font;
    line *label_line;

    int    num_circles;
    int    num_sprites;
    int    sprite_size;

    glsl   circle_glsl;
    glsl   sprite_glsl;

    GLuint circle_vbo;
    GLuint sprite_vbo;
    GLuint sprite_tex;

    std::vector<label> labels;
};

//-----------------------------------------------------------------------------

#endif
