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
#include "scm-frame.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_render::scm_render(int w, int h) :
    width(w), height(h), blur(0), wire(false), frame0(0), frame1(0)
{
    init_ogl();
    init_matrices();

    for (int i = 0; i < 16; i++)
        midentity(previous_T[i]);
}

scm_render::~scm_render()
{
    free_ogl();
}

//------------------------------------------------------------------------------

// Initialize the uniforms of the given GLSL program object.

void scm_render::init_uniforms(GLuint program)
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

//------------------------------------------------------------------------------

#include <scm-render-fade-vert.h>
#include <scm-render-fade-frag.h>
#include <scm-render-blur-vert.h>
#include <scm-render-blur-frag.h>
#include <scm-render-both-vert.h>
#include <scm-render-both-frag.h>

void scm_render::init_ogl()
{
    glsl_source(&render_fade, (const char *) scm_render_fade_vert,
                                             scm_render_fade_vert_len,
                              (const char *) scm_render_fade_frag,
                                             scm_render_fade_frag_len);
    glsl_source(&render_blur, (const char *) scm_render_blur_vert,
                                             scm_render_blur_vert_len,
                              (const char *) scm_render_blur_frag,
                                             scm_render_blur_frag_len);
    glsl_source(&render_both, (const char *) scm_render_both_vert,
                                             scm_render_both_vert_len,
                              (const char *) scm_render_both_frag,
                                             scm_render_both_frag_len);

    init_uniforms(render_fade.program);
    init_uniforms(render_blur.program);
    init_uniforms(render_both.program);

    glUseProgram(render_fade.program);
    uniform_fade_t = glsl_uniform(render_fade.program, "t");

    glUseProgram(render_blur.program);
    uniform_blur_n = glsl_uniform(render_blur.program, "n");
    uniform_blur_T = glsl_uniform(render_blur.program, "T");

    glUseProgram(render_both.program);
    uniform_both_t = glsl_uniform(render_both.program, "t");
    uniform_both_n = glsl_uniform(render_both.program, "n");
    uniform_both_T = glsl_uniform(render_both.program, "T");

    glUseProgram(0);

    frame0 = new scm_frame(width, height);
    frame1 = new scm_frame(width, height);

    scm_log("scm_render init_ogl %d %d", width, height);
}

void scm_render::free_ogl()
{
    scm_log("scm_render free_ogl %d %d", width, height);

    delete frame0;
    delete frame1;

    frame0 = frame1 = 0;

    glsl_delete(&render_fade);
    glsl_delete(&render_blur);
    glsl_delete(&render_both);
}

//------------------------------------------------------------------------------

// Draw a screen-filling rectangle.

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

// Set the OpenGL state for wireframe rendering.

static void wire_on()
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.0);
}

// Unset the OpenGL state for wireframe rendering.

static void wire_off()
{
    glPopAttrib();
}

//------------------------------------------------------------------------------

// Determine whether fading is necessary.

bool scm_render::check_fade(scm_scene *fore0, scm_scene *fore1,
                            scm_scene *back0, scm_scene *back1, double t)
{
    if (t < 1.0 / 255)  return false;
    if (fore0 != fore1) return true;
    if (back0 != back1) return true;
    return false;
}

// Determine whether blurring is necessary and compute its transform.

bool scm_render::check_blur(const double *P,
                            const double *M,
                                  double *S, GLfloat *U)
{
    if (blur)
    {
        double T[16];
        double I[16];
        double N[16];

        // S is the previous transform. T is the current one. I is its inverse.

        mmultiply(T, P, M);

        if (T[ 0] != S[ 0] || T[ 1] != S[ 1] ||
            T[ 2] != S[ 2] || T[ 3] != S[ 3] ||
            T[ 4] != S[ 4] || T[ 5] != S[ 5] ||
            T[ 6] != S[ 6] || T[ 7] != S[ 7] ||
            T[ 8] != S[ 8] || T[ 9] != S[ 9] ||
            T[10] != S[10] || T[11] != S[11] ||
            T[12] != S[12] || T[13] != S[13] ||
            T[14] != S[14] || T[15] != S[15])
        {
            // Compose a transform taking current screen coords to previous.

            minvert (I, T);
            mcpy    (N, D);
            mcompose(N, C);
            mcompose(N, S);
            mcompose(N, I);
            mcompose(N, B);
            mcompose(N, A);
            mcpy    (S, T);

            // Return this transform for use as an OpenGL uniform.

            for (int i = 0; i < 16; i++)
                U[i] = GLfloat(N[i]);

            return true;
        }
    }
    return false;
}

// Render the foreground and background scenes. We render the foreground
// first to allow the depth test to eliminate background texture cache traffic.

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *fore,
                        scm_scene  *back,
                      const double *P,
                      const double *M, int channel, int frame)
{
    double T[16];

    if (wire)
        wire_on();

    // Background

    if (back)
    {
        // Extract the far clipping plane distance from the projection.

        double I[16];
        double ne[4], nc[4] = { 0, 0, -1, 1 };
        double fe[4], fc[4] = { 0, 0, +1, 1 };

        minvert   (I, P);
        wtransform(ne, I, nc);
        wtransform(fe, I, fc);

        double n = vlen(ne) / ne[3];
        double f = vlen(fe) / fe[3];
        double k = lerp(n, f, 0.5);

        // Center the sphere at the origin and scale it to the far plane.

        double N[16];

        N[ 0] = M[ 0] * k;
        N[ 1] = M[ 1] * k;
        N[ 2] = M[ 2] * k;
        N[ 3] = M[ 3] * k;
        N[ 4] = M[ 4] * k;
        N[ 5] = M[ 5] * k;
        N[ 6] = M[ 6] * k;
        N[ 7] = M[ 7] * k;
        N[ 8] = M[ 8] * k;
        N[ 9] = M[ 9] * k;
        N[10] = M[10] * k;
        N[11] = M[11] * k;
        N[12] = 0;
        N[13] = 0;
        N[14] = 0;
        N[15] = M[15];

        // Apply the transform.

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixd(P);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(N);

        mmultiply(T, P, N);

        // Render the inside of the sphere.

        glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
        {
            glDepthMask(GL_FALSE);
            glFrontFace(GL_CCW);
            sphere->draw(back, T, width, height, channel, frame);
            back->draw_label();
        }
        glPopAttrib();
    }

    // Foreground

    if (fore)
    {
        // Apply the transform.

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixd(P);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(M);

        mmultiply(T, P, M);

        // Render the outside of the sphere.

        glPushAttrib(GL_POLYGON_BIT);
        {
            glFrontFace(GL_CW);
            sphere->draw(fore, T, width, height, channel, frame);
            fore->draw_label();
        }
        glPopAttrib();
    }

    if (wire)
        wire_off();
}

// Render, blur, and blend the given scenes.

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *fore0,
                        scm_scene  *fore1,
                        scm_scene  *back0,
                        scm_scene  *back1,
                      const double *P,
                      const double *M, int channel, int frame, double t)
{
    GLfloat T[16];

    const bool do_fade = check_fade(fore0, fore1, back0, back1, t);
    const bool do_blur = check_blur(P, M, previous_T[channel], T);

    if (!do_fade && !do_blur)
        render(sphere, fore0, back0, P, M, channel, frame);

    else
    {
        GLint framebuffer;

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);

        // Render the scene(s) to the offscreen framebuffers.

        glPushAttrib(GL_VIEWPORT_BIT);
        {
            glViewport(0, 0, width, height);
            glClearColor(0.f, 0.f, 0.f, 0.f);

            frame0->bind_frame();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render(sphere, fore0, back0, P, M, channel, frame);

            if (do_fade)
            {
                frame1->bind_frame();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                render(sphere, fore1, back1, P, M, channel, frame);
            }
        }
        glPopAttrib();

        // Bind the resurting textures.

        glActiveTexture(GL_TEXTURE3);
        frame1->bind_depth();
        glActiveTexture(GL_TEXTURE2);
        frame0->bind_depth();
        glActiveTexture(GL_TEXTURE1);
        frame1->bind_color();
        glActiveTexture(GL_TEXTURE0);
        frame0->bind_color();
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Bind the necessary shader and set its uniforms.

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

        // Render the blur / fade to the framebuffer.

        fillscreen();
        glUseProgram(0);
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
