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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <GL/glew.h>

#include "util3d/math3d.h"
#include "util3d/type.h"
#include "util3d/glsl.h"

#include "scm-label.hpp"
#include "scm-path.hpp"
#include "scm-log.hpp"

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
        GLubyte      a = 255;

        if (c[0] == 'A' && c[1] == 'A') a /= 1;
        if (c[0] == 'S' && c[1] == 'F') a /= 2;

        p[0].v[0] = M.M[0] * (-s) + M.M[4] * (-s) + M.M[12];
        p[0].v[1] = M.M[1] * (-s) + M.M[5] * (-s) + M.M[13];
        p[0].v[2] = M.M[2] * (-s) + M.M[6] * (-s) + M.M[14];
        p[0].t[0] = 0;
        p[0].t[1] = 0;
        p[0].c[0] = 255;
        p[0].c[1] = 255;
        p[0].c[2] = 255;
        p[0].c[3] = a;

        p[1].v[0] = M.M[0] * ( s) + M.M[4] * (-s) + M.M[12];
        p[1].v[1] = M.M[1] * ( s) + M.M[5] * (-s) + M.M[13];
        p[1].v[2] = M.M[2] * ( s) + M.M[6] * (-s) + M.M[14];
        p[1].t[0] = 0;
        p[1].t[1] = 1;
        p[1].c[0] = 255;
        p[1].c[1] = 255;
        p[1].c[2] = 255;
        p[1].c[3] = a;

        p[2].v[0] = M.M[0] * ( s) + M.M[4] * ( s) + M.M[12];
        p[2].v[1] = M.M[1] * ( s) + M.M[5] * ( s) + M.M[13];
        p[2].v[2] = M.M[2] * ( s) + M.M[6] * ( s) + M.M[14];
        p[2].t[0] = 1;
        p[2].t[1] = 1;
        p[2].c[0] = 255;
        p[2].c[1] = 255;
        p[2].c[2] = 255;
        p[2].c[3] = a;

        p[3].v[0] = M.M[0] * (-s) + M.M[4] * ( s) + M.M[12];
        p[3].v[1] = M.M[1] * (-s) + M.M[5] * ( s) + M.M[13];
        p[3].v[2] = M.M[2] * (-s) + M.M[6] * ( s) + M.M[14];
        p[3].t[0] = 1;
        p[3].t[1] = 0;
        p[3].c[0] = 255;
        p[3].c[1] = 255;
        p[3].c[2] = 255;
        p[3].c[3] = a;
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
        p.c[0] = 255;
        p.c[1] = 255;
        p.c[2] = 255;
        p.c[3] = 255;
    }
};

struct latlon
{
    point p[360];

    latlon(float lat, float lon, float rad)
    {
        float P = 0.0f;
        float T = 0.0f;

        for (int i = 0; i < 360; ++i)
        {
            if (lon)
            {
                P = radians(i);
                T = radians(lon);
            }
            else
            {
                P = radians(lat);
                T = radians(i);
            }

            p[i].v[0] = rad * sinf(T) * cosf(P);
            p[i].v[1] = rad *           sinf(P);
            p[i].v[2] = rad * cosf(T) * cosf(P);
            p[i].t[0] =   0;
            p[i].t[1] =   0;
            p[i].c[0] = 127;
            p[i].c[1] = 127;
            p[i].c[2] = 127;
            p[i].c[3] = 127;
        }
    }
};

//------------------------------------------------------------------------------

int scm_label::scan(FILE *fp, label& L)
{
    int n;

    if (fscanf(fp, "\"%63[^\"]\",%f,%f,%f,%f,%c%c\n%n",
        L.str, &L.lat, &L.lon, &L.dia, &L.rad, &L.typ[0], &L.typ[1], &n) > 5)
        return n;

    if (fscanf(fp,      "%63[^,],%f,%f,%f,%f,%c%c\n%n",
        L.str, &L.lat, &L.lon, &L.dia, &L.rad, &L.typ[0], &L.typ[1], &n) > 5)
        return n;

    return 0;
}

// Parse the label definition file.

void scm_label::parse(const std::string& file)
{
    std::string path = scm_path_search(file);

    if (!path.empty())
    {
        FILE *fp;
        label L;

        if ((fp = fopen(path.c_str(), "r")))
        {
            while (scan(fp, L))
                labels.push_back(L);

            fclose(fp);
        }
    }
}

//------------------------------------------------------------------------------

#include "scm-label-icons.h"
#include "scm-label-font.h"

#include "scm-label-circle-vert.h"
#include "scm-label-circle-frag.h"
#include "scm-label-sprite-vert.h"
#include "scm-label-sprite-frag.h"

scm_label::scm_label(const std::string& file, int size) :
    label_line(0),
    num_circles(0),
    num_sprites(0),
    num_latlons(0),
    sprite_size(size)
{
    // Initialize the font.

    label_font = font_create(Vera_ttf, Vera_ttf_len, 64, 1.0);

    // Initialize the shaders.

    memset(&circle_glsl, 0, sizeof (glsl));
    memset(&sprite_glsl, 0, sizeof (glsl));

    glsl_source(&circle_glsl, (const char *) scm_label_circle_vert,
                                             scm_label_circle_vert_len,
                              (const char *) scm_label_circle_frag,
                                             scm_label_circle_frag_len);
    glsl_source(&sprite_glsl, (const char *) scm_label_sprite_vert,
                                             scm_label_sprite_vert_len,
                              (const char *) scm_label_sprite_frag,
                                             scm_label_sprite_frag_len);

    glUseProgram(sprite_glsl.program);
    glUniform1i(glGetUniformLocation(sprite_glsl.program, "icons"), 0);

    // Parse the data file into labels.

    parse(file);

    // Generate an annotation for each label.

    std::vector<char *> string_v;
    std::vector<matrix> matrix_v;
    std::vector<circle> circle_v;
    std::vector<sprite> sprite_v;
    std::vector<latlon> latlon_v;

    for (int i = 0; i < int(labels.size()); ++i)
    {
        int w = line_length(labels[i].str, label_font);
        int h = font_height(               label_font);

        matrix M;
        double x = -w / 2.0;
        double y = 0.0;
        double z = 0.0;

        double d = labels[i].dia;
        double r = labels[i].rad;
        double k = d / r;

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

        // Create a line of latitude.

        if (labels[i].latlon())
        {
            latlon L(labels[i].lat, labels[i].lon, labels[i].rad);
            latlon_v.push_back(L);
        }

        // Add the string and matrix to the list.

        double e = 0.001 * clamp(k, 0.0, 0.5) / k;

        M.scale(e);
        M.translate(x, y, z);

        string_v.push_back(labels[i].str);
        matrix_v.push_back(M);
    }

    num_circles = circle_v.size();
    num_sprites = sprite_v.size();
    num_latlons = latlon_v.size();

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

    // Create a VBO for the latlons.

    glGenBuffers(1,              &latlon_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, latlon_vbo);
    glBufferData(GL_ARRAY_BUFFER, 360 * sz * latlon_v.size(),
                                            &latlon_v.front(), GL_STATIC_DRAW);

    // Create a texture for the sprites.

    glGenTextures(1,             &sprite_tex);
    glBindTexture(GL_TEXTURE_2D, sprite_tex);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, 128, 32, 0,
                  GL_LUMINANCE, GL_UNSIGNED_BYTE, scm_label_icons);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    scm_log("scm_label constructor %s", file.c_str());
}

scm_label::~scm_label()
{
    scm_log("scm_label destructor");

    glDeleteBuffers(1, &latlon_vbo);
    glDeleteBuffers(1, &sprite_vbo);
    glDeleteBuffers(1, &circle_vbo);

    line_delete(label_line);
    font_delete(label_font);

    glsl_delete(&sprite_glsl);
    glsl_delete(&circle_glsl);
}

//------------------------------------------------------------------------------

void scm_label::draw(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    size_t sz = sizeof (point);

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    {
        // Ensure we're in clockwise mode regardless of sphere winding.

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glColor4ub(r, g, b, a);
        glLineWidth(0.5);

        // Blend for antialiasing but preserve dest alpha for the blur shader.

        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);

            // Draw the latlons.

            // glEnable(GL_CLIP_PLANE0);

            glBindBuffer(GL_ARRAY_BUFFER, latlon_vbo);
            glVertexPointer  (3, GL_FLOAT,         sz, (GLvoid *)  0);
            glTexCoordPointer(2, GL_FLOAT,         sz, (GLvoid *) 12);
            glColorPointer   (4, GL_UNSIGNED_BYTE, sz, (GLvoid *) 20);

            glUseProgram(0);
            for (int i = 0; i < num_latlons; i++)
                glDrawArrays(GL_LINE_LOOP, i * 360, 360);

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
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);

        line_render(label_line);
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
