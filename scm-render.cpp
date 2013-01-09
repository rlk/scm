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
#include <cstring>

#include "util3d/math3d.h"

#include "scm-render.hpp"
#include "scm-sphere.hpp"
#include "scm-scene.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_render::scm_render(int w, int h) :
    width(w), height(h), blur(16), wire(false)
{
    init_ogl();
    init_matrices();

    for (int i = 0; i < 16; i++)
        midentity(L[i]);
}

scm_render::~scm_render()
{
    free_ogl();
}

//------------------------------------------------------------------------------

// Initialize the uniforms of the given GLSL program object.

static void init_uniforms(GLuint program, int w, int h)
{
    glUseProgram(program);
    {
        glUniform1i(glGetUniformLocation(program, "color0"), 0);
        glUniform1i(glGetUniformLocation(program, "color1"), 1);
        glUniform1i(glGetUniformLocation(program, "depth0"), 2);
        glUniform1i(glGetUniformLocation(program, "depth1"), 3);
    }
    glUseProgram(0);
}

// Initialize the storage and parameters of an off-screen color buffer.

static void init_color(GLuint color, int w, int h)
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

// Initialize the storage and parameters of an off-screen depth buffer.

static void init_depth(GLuint depth, int w, int h)
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

// Generate and initialize a framebuffer object with color and depth buffers.

static void init_fbo(GLuint& color,
                     GLuint& depth,
                     GLuint& framebuffer, int w, int h)
{
    GLint previous;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);

    glGenFramebuffers(1, &framebuffer);
    glGenTextures    (1, &color);
    glGenTextures    (1, &depth);

    init_color(color, w, h);
    init_depth(depth, w, h);

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_RECTANGLE, color, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_RECTANGLE, depth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            scm_log("* init_fbo incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, previous);
}

//------------------------------------------------------------------------------

void scm_render::init_matrices()
{
    double w = double(width);
    double h = double(height);

    A[0] = 1/w; A[4] = 0.0; A[ 8] = 0.0; A[12] =  0.0;
    A[1] = 0.0; A[5] = 1/h; A[ 9] = 0.0; A[13] =  0.0;
    A[2] = 0.0; A[6] = 0.0; A[10] = 1.0; A[14] =  0.0;
    A[3] = 0.0; A[7] = 0.0; A[11] = 0.0; A[15] =  1.0;

    B[0] = 2.0; B[4] = 0.0; B[ 8] = 0.0; B[12] = -1.0;
    B[1] = 0.0; B[5] = 2.0; B[ 9] = 0.0; B[13] = -1.0;
    B[2] = 0.0; B[6] = 0.0; B[10] = 2.0; B[14] = -1.0;
    B[3] = 0.0; B[7] = 0.0; B[11] = 0.0; B[15] =  1.0;

    C[0] = 0.5; C[4] = 0.0; C[ 8] = 0.0; C[12] =  0.5;
    C[1] = 0.0; C[5] = 0.5; C[ 9] = 0.0; C[13] =  0.5;
    C[2] = 0.0; C[6] = 0.0; C[10] = 0.5; C[14] =  0.5;
    C[3] = 0.0; C[7] = 0.0; C[11] = 0.0; C[15] =  1.0;

    D[0] =   w; D[4] = 0.0; D[ 8] = 0.0; D[12] =  0.0;
    D[1] = 0.0; D[5] =   h; D[ 9] = 0.0; D[13] =  0.0;
    D[2] = 0.0; D[6] = 0.0; D[10] = 1.0; D[14] =  0.0;
    D[3] = 0.0; D[7] = 0.0; D[11] = 0.0; D[15] =  1.0;
}

#include <scm-render-fade-vert.h>
#include <scm-render-fade-frag.h>
#include <scm-render-blur-vert.h>
#include <scm-render-blur-frag.h>
#include <scm-render-both-vert.h>
#include <scm-render-both-frag.h>

void scm_render::init_ogl()
{
    glsl_source(&render_fade, (const char *) scm_render_fade_vert,
                              (const char *) scm_render_fade_frag);
    glsl_source(&render_blur, (const char *) scm_render_blur_vert,
                              (const char *) scm_render_blur_frag);
    glsl_source(&render_both, (const char *) scm_render_both_vert,
                              (const char *) scm_render_both_frag);

    init_uniforms(render_fade.program, width, height);
    init_uniforms(render_blur.program, width, height);
    init_uniforms(render_both.program, width, height);

    uniform_fade_t = glsl_uniform(render_fade.program, "t");
    uniform_both_t = glsl_uniform(render_both.program, "t");
    uniform_blur_n = glsl_uniform(render_blur.program, "n");
    uniform_both_n = glsl_uniform(render_both.program, "n");
    uniform_blur_T = glsl_uniform(render_blur.program, "T");
    uniform_both_T = glsl_uniform(render_both.program, "T");

    init_fbo(color0, depth0, framebuffer0, width, height);
    init_fbo(color1, depth1, framebuffer1, width, height);

    scm_log("scm_render init_ogl %d %d", width, height);
}

void scm_render::free_ogl()
{
    scm_log("scm_render free_ogl %d %d", width, height);

    glsl_delete(&render_fade);
    glsl_delete(&render_blur);
    glsl_delete(&render_both);

    if (color0)       glDeleteTextures    (1, &color0);
    if (depth0)       glDeleteTextures    (1, &depth0);
    if (framebuffer0) glDeleteFramebuffers(1, &framebuffer0);

    if (color1)       glDeleteTextures    (1, &color1);
    if (depth1)       glDeleteTextures    (1, &depth1);
    if (framebuffer1) glDeleteFramebuffers(1, &framebuffer1);

    color0 = depth0 = framebuffer0 = 0;
    color1 = depth1 = framebuffer1 = 0;
}

//------------------------------------------------------------------------------

static bool is_same_transform(const double *A, const double *B)
{
    return (A[ 0] == B[ 0] && A[ 1] == B[ 1]
         && A[ 2] == B[ 2] && A[ 3] == A[ 3]
         && A[ 4] == B[ 4] && A[ 5] == B[ 5]
         && A[ 6] == B[ 6] && A[ 7] == A[ 7]
         && A[ 8] == B[ 8] && A[ 9] == B[ 9]
         && A[10] == B[10] && A[11] == A[11]
         && A[12] == B[12] && A[13] == B[13]
         && A[14] == B[14] && A[15] == A[15]);
}

static void fillscreen()
{
    glPushAttrib(GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    {
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glFrontFace(GL_CCW);
        glDepthMask(GL_FALSE);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        {
            glVertex2f(-1.0f, -1.0f);
            glVertex2f(+1.0f, -1.0f);
            glVertex2f(+1.0f, +1.0f);
            glVertex2f(-1.0f, +1.0f);
        }
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    glPopAttrib();
}

static void wire_on()
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.0);
}

static void wire_off()
{
    glPopAttrib();
}

//------------------------------------------------------------------------------

void scm_render::render0(scm_sphere *sphere,
                         scm_scene  *scene0,
                         const double *M, int channel, int frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sphere->draw(scene0, M, width, height, channel, frame);
    // scene0->draw_label();
}

void scm_render::render1(scm_sphere *sphere,
                         scm_scene  *scene1,
                         const double *M, int channel, int frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sphere->draw(scene1, M, width, height, channel, frame);
    // scene1->draw_label();
}

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *scene0,
                        scm_scene  *scene1,
                        const double *M, double t, int channel, int frame)
{
    const bool do_fade = !(scene0 == scene1 || t < 1.0 / 256.0);
    const bool do_blur = !(blur == 0 || is_same_transform(M, L[channel]));

    // Compose the transform taking current screen coordinates to previous.

    double  I[16];
    double  U[16];
    GLfloat T[16];

    minvert (I, M);

    mcpy    (U, D);
    mcompose(U, C);
    mcompose(U, L[channel]);
    mcompose(U, I);
    mcompose(U, B);
    mcompose(U, A);

    mcpy(L[channel], M);

    for (int i = 0; i < 16; i++)
        T[i] = GLfloat(U[i]);

    if (!do_fade && !do_blur)
    {
        if (wire) wire_on();
        sphere->draw(scene0, M, width, height, channel, frame);
        scene0->draw_label();
        if (wire) wire_off();
    }
    else
    {
        GLint framebuffer;

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);

        // Render the scene to the offscreen framebuffers.

        glPushAttrib(GL_VIEWPORT_BIT);
        {
            glViewport(0, 0, width, height);
            glClearColor(0.f, 0.f, 0.f, 0.f);

            if (wire) wire_on();

            if (do_fade)
            {
                render0(sphere, scene0, M, channel, frame);
                render1(sphere, scene1, M, channel, frame);
            }
            else
                render0(sphere, scene0, M, channel, frame);

            if (wire) wire_off();
        }
        glPopAttrib();

        // Bind the resurting textures.

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // This texture push should NOT be necessary.

        glPushAttrib(GL_TEXTURE_BIT);
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_RECTANGLE, depth1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_RECTANGLE, depth0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_RECTANGLE, color1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_RECTANGLE, color0);

            // Bind the right shader and set the necessary uniforms.

            if      (do_fade && do_blur)
            {
                glUseProgram(render_both.program);
                glUniform1f       (uniform_both_t,       t);
                glUniform1i       (uniform_both_n,    blur);
                glUniformMatrix4fv(uniform_both_T, 1, 0, T);
            }
            else if (do_fade && !do_blur)
            {
                glUseProgram(render_fade.program);
                glUniform1f       (uniform_fade_t,       t);
            }
            else if (!do_fade && do_blur)
            {
                glUseProgram(render_blur.program);
                glUniform1i       (uniform_blur_n,    blur);
                glUniformMatrix4fv(uniform_blur_T, 1, 0, T);
            }

            fillscreen();

            glUseProgram(0);
        }
    glPopAttrib();
    }
}

//------------------------------------------------------------------------------

void scm_render::set_size(int w, int h)
{
    free_ogl();
    width  = w;
    height = h;
    init_ogl();
    init_matrices();
}

void scm_render::set_blur(int b)
{
    blur = b;
}

void scm_render::set_wire(bool w)
{
    wire = w;
}

//------------------------------------------------------------------------------
