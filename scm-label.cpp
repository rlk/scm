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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GL/glew.h>

#include "util3d/math3d.h"
#include "util3d/type.h"
#include "util3d/glsl.h"

#include "scm-label.hpp"

#define LABEL_R 0xFF
#define LABEL_G 0x80
#define LABEL_B 0x00
#define LABEL_A 0xFF

//------------------------------------------------------------------------------

static double clamp(double k, double a, double b)
{
    if      (k < a) return a;
    else if (k > b) return b;
    else            return k;
}

struct point
{
    float         v[3];
    float         t[2];
    unsigned char c[4];
};

struct matrix
{
    double M[16];

    matrix()
    {
        midentity(M);
    }

    void rotatey(double a)
    {
        double A[16], B[16];
        mrotatey(A, a);
        mmultiply(B, M, A);
        mcpy(M, B);
    }

    void rotatex(double a)
    {
        double A[16], B[16];
        mrotatex(A, a);
        mmultiply(B, M, A);
        mcpy(M, B);
    }

    void rotatez(double a)
    {
        double A[16], B[16];
        mrotatez(A, a);
        mmultiply(B, M, A);
        mcpy(M, B);
    }

    void translate(double x, double y, double z)
    {
        double A[16], B[16], v[3] = { x, y, z };
        mtranslate(A, v);
        mmultiply(B, M, A);
        mcpy(M, B);
    }

    void scale(double k)
    {
        double A[16], B[16], v[3] = { k, k, k };
        mscale(A, v);
        mmultiply(B, M, A);
        mcpy(M, B);
    }
};

struct circle
{
    point p[4];

    circle(matrix& M, const char *c)
    {
        const double s = 0.5;

        GLfloat R = LABEL_R;
        GLfloat G = LABEL_G;
        GLfloat B = LABEL_B;
        GLfloat A = LABEL_A;

        if (c[0] == 'A' && c[1] == 'A') A *= 1.0;
        if (c[0] == 'S' && c[1] == 'F') A *= 0.5;

        p[0].v[0] = M.M[0] * (-s) + M.M[4] * (-s) + M.M[12];
        p[0].v[1] = M.M[1] * (-s) + M.M[5] * (-s) + M.M[13];
        p[0].v[2] = M.M[2] * (-s) + M.M[6] * (-s) + M.M[14];
        p[0].t[0] = 0;
        p[0].t[1] = 0;
        p[0].c[0] = R;
        p[0].c[1] = G;
        p[0].c[2] = B;
        p[0].c[3] = A;

        p[1].v[0] = M.M[0] * ( s) + M.M[4] * (-s) + M.M[12];
        p[1].v[1] = M.M[1] * ( s) + M.M[5] * (-s) + M.M[13];
        p[1].v[2] = M.M[2] * ( s) + M.M[6] * (-s) + M.M[14];
        p[1].t[0] = 0;
        p[1].t[1] = 1;
        p[1].c[0] = R;
        p[1].c[1] = G;
        p[1].c[2] = B;
        p[1].c[3] = A;

        p[2].v[0] = M.M[0] * ( s) + M.M[4] * ( s) + M.M[12];
        p[2].v[1] = M.M[1] * ( s) + M.M[5] * ( s) + M.M[13];
        p[2].v[2] = M.M[2] * ( s) + M.M[6] * ( s) + M.M[14];
        p[2].t[0] = 1;
        p[2].t[1] = 1;
        p[2].c[0] = R;
        p[2].c[1] = G;
        p[2].c[2] = B;
        p[2].c[3] = A;

        p[3].v[0] = M.M[0] * (-s) + M.M[4] * ( s) + M.M[12];
        p[3].v[1] = M.M[1] * (-s) + M.M[5] * ( s) + M.M[13];
        p[3].v[2] = M.M[2] * (-s) + M.M[6] * ( s) + M.M[14];
        p[3].t[0] = 1;
        p[3].t[1] = 0;
        p[3].c[0] = R;
        p[3].c[1] = G;
        p[3].c[2] = B;
        p[3].c[3] = A;
    }
};

struct sprite
{
    point p;

    sprite(matrix& M, const char *c)
    {
        GLfloat x = 0.0;
        GLfloat y = 0.0;

        if (c[0] == 'M' && c[1] == '0') x = 0.0;
        if (c[0] == 'L' && c[1] == 'F') x = 1.0;
        if (c[0] == '@' && c[1] == '*') x = 2.0;
        if (c[0] == '@' && c[1] == 'C') x = 3.0;

        p.v[0] = M.M[12];
        p.v[1] = M.M[13];
        p.v[2] = M.M[14];
        p.t[0] = x;
        p.t[1] = y;
        p.c[0] = LABEL_R;
        p.c[1] = LABEL_G;
        p.c[2] = LABEL_B;
        p.c[3] = LABEL_A;
    }
};

//------------------------------------------------------------------------------

int scm_label::scan(const char *dat, label& L)
{
    int n;

    if (sscanf(dat, "\"%63[^\"]\",%f,%f,%f,%f,%c%c\n%n",
        L.str, &L.lat, &L.lon, &L.dia, &L.rad, &L.typ[0], &L.typ[1], &n) > 5)
        return n;

    if (sscanf(dat,      "%63[^,],%f,%f,%f,%f,%c%c\n%n",
        L.str, &L.lat, &L.lon, &L.dia, &L.rad, &L.typ[0], &L.typ[1], &n) > 5)
        return n;

    return 0;
}

// Parse the label definition file.

void scm_label::parse(const void *data_ptr, size_t data_len, double radius)
{
    const char *dat = (const char *) data_ptr;
    label L;
    int   n;

    while ((n = scan(dat, L)))
    {
        labels.push_back(L);
        dat += n;
    }
}

//------------------------------------------------------------------------------

#include "scm-label-icons.h"

#include "scm-label-circle-vert.h"
#include "scm-label-circle-frag.h"
#include "scm-label-sprite-vert.h"
#include "scm-label-sprite-frag.h"

scm_label::scm_label(const void *data_ptr, size_t data_len,
                     const void *font_ptr, size_t font_len,
                     double radius, int size) :
    label_line(0),
    num_circles(0),
    num_sprites(0),
    sprite_size(size)
{
    // Initialize the font.

    label_font  = font_create(font_ptr, font_len, 64, 1.0);

    // Initialize the shaders.

    memset(&circle_glsl, 0, sizeof (glsl));
    memset(&sprite_glsl, 0, sizeof (glsl));

    glsl_source(&circle_glsl, (const char *) scm_label_circle_vert,
                              (const char *) scm_label_circle_frag);
    glsl_source(&sprite_glsl, (const char *) scm_label_sprite_vert,
                              (const char *) scm_label_sprite_frag);

    glUseProgram(sprite_glsl.program);
    glUniform1i(glGetUniformLocation(sprite_glsl.program, "icons"), 0);

    // Parse the data file into labels.

    parse(data_ptr, data_len, radius);

    // Generate an annotation for each label.

    std::vector<char *> string_v;
    std::vector<matrix> matrix_v;
    std::vector<circle> circle_v;
    std::vector<sprite> sprite_v;

    for (int i = 0; i < int(labels.size()); ++i)
    {
        int w = line_length(labels[i].str, label_font);
        int h = font_height(               label_font);

        matrix M;
        double x = -w / 2.0;
        double y = 0.0;
        double z = 0.0;

        // double r = sqrt(labels[i].rad / radius -
        //                 labels[i].dia * labels[i].dia / 4.0);

        double d = labels[i].dia / radius;
        double r = labels[i].rad / radius;

        // Transform it into position

        M.rotatey( radians(labels[i].lon));
        M.rotatex(-radians(labels[i].lat));
        M.translate(0, 0, r);
        M.scale(d);

        // Create a sprite.

        if (labels[i].sprite())
        {
            sprite S(M, labels[i].typ);
            sprite_v.push_back(S);
            y = +h / 3.0;
        }

        // Create a circle.

        if (labels[i].circle())
        {
            circle C(M, labels[i].typ);
            circle_v.push_back(C);
        }

        // Add the string and matrix to the list.

        double e = 0.001 * clamp(d, 0.0005, 0.5) / d;

        M.scale(e);
        M.translate(x, y, z);

        string_v.push_back(labels[i].str);
        matrix_v.push_back(M);
    }

    num_circles = circle_v.size();
    num_sprites = sprite_v.size();

    size_t sz = sizeof (point);

    // Typeset the labels.

    if (!string_v.empty())
        label_line = line_layout(string_v.size(), &string_v.front(), NULL,
                                                   matrix_v.front().M, label_font);

    // Create a VBO for the circles.

    glGenBuffers(1,              &circle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sz * circle_v.size(),
                                          &circle_v.front(), GL_STATIC_DRAW);

    // Create a VBO for the sprites.

    glGenBuffers(1,              &sprite_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    glBufferData(GL_ARRAY_BUFFER, 1 * sz * sprite_v.size(),
                                          &sprite_v.front(), GL_STATIC_DRAW);

    // Create a texture for the sprites.

    glGenTextures(1,             &sprite_tex);
    glBindTexture(GL_TEXTURE_2D, sprite_tex);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, 128, 32, 0,
                  GL_LUMINANCE, GL_UNSIGNED_BYTE, scm_label_icons);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

scm_label::~scm_label()
{
    glDeleteBuffers(1, &sprite_vbo);
    glDeleteBuffers(1, &circle_vbo);

    line_delete(label_line);
    font_delete(label_font);

    glsl_delete(&sprite_glsl);
    glsl_delete(&circle_glsl);
}

//------------------------------------------------------------------------------

void scm_label::draw()
{
    size_t sz = sizeof (point);

    glPushAttrib(GL_ENABLE_BIT);
    {
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4ub(LABEL_R, LABEL_G, LABEL_B, LABEL_A);

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            // Draw the circles.

            glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
            glVertexPointer  (3, GL_FLOAT,         sz, (GLvoid *)  0);
            glTexCoordPointer(2, GL_FLOAT,         sz, (GLvoid *) 12);
            glColorPointer   (4, GL_UNSIGNED_BYTE, sz, (GLvoid *) 20);

            glUseProgram(circle_glsl.program);
            glDrawArrays(GL_QUADS, 0, 4 * num_circles);

            // Draw the sprites.

            glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
            glVertexPointer  (3, GL_FLOAT,         sz, (GLvoid *)  0);
            glTexCoordPointer(2, GL_FLOAT,         sz, (GLvoid *) 12);
            glColorPointer   (4, GL_UNSIGNED_BYTE, sz, (GLvoid *) 20);

            glPointSize(sprite_size);
            glEnable(GL_POINT_SPRITE);
            glBindTexture(GL_TEXTURE_2D, sprite_tex);

            glUseProgram(sprite_glsl.program);
            glDrawArrays(GL_POINTS, 0, 1 * num_sprites);

            glDisable(GL_POINT_SPRITE);
        }
        glPopClientAttrib();

        // Draw the labels.

        glUseProgram(0);

        line_render(label_line);
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
