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

static void fillscreen(int, int);

static void wire_on();
static void wire_off();

static double fardistance(const double *);

//------------------------------------------------------------------------------

/// Create a new render manager. Initialize the necessary OpenGL state
/// framebuffer object state.
///
/// Motion blur is disabled (set to zero) by default.
///
/// @param w Width of the off-screen render targets (in pixels).
/// @param h Height of the off-screen render targets (in pixels).

scm_render::scm_render(int w, int h) :
    width(w), height(h), blur(0), wire(false), frame0(0), frame1(0)
{
    init_ogl();
    init_matrices();

    for (int i = 0; i < 16; i++)
        midentity(previous_T[i]);

    // Test configuration for atmosphere shader

    atmo_r[0] = 3373043.f;
    atmo_r[1] = 3573043.f;
    atmo_c[0] = 0.6f;
    atmo_c[1] = 0.4f;
    atmo_c[2] = 0.3f;
}

/// Finalize all OpenGL state.

scm_render::~scm_render()
{
    free_ogl();
}

//------------------------------------------------------------------------------

/// Set the size of the off-screen render targets. This entails the destruction
/// and recreation of OpenGL framebuffer objects, so it should *not* be called
/// every frame.

void scm_render::set_size(int w, int h)
{
    free_ogl();
    width  = w;
    height = h;
    init_ogl();
    init_matrices();
}

/// Set the motion blur degree. Higher degrees incur greater rendering loads.
/// 8 is an effective value. Set 0 to disable motion blur completely.

void scm_render::set_blur(int b)
{
    blur = b;
}

/// Set the wireframe option.

void scm_render::set_wire(bool w)
{
    wire = w;
}

/// Set the atmosphere parameters.

void scm_render::set_atmo(double r0, double r1, double r, double g, double b)
{
    atmo_r[0] = GLfloat(r0);
    atmo_r[1] = GLfloat(r1);

    atmo_c[0] = GLfloat(r);
    atmo_c[1] = GLfloat(g);
    atmo_c[2] = GLfloat(b);
}

//------------------------------------------------------------------------------

/// Render the foreground and background with optional blur and dissolve.
///
/// @param sphere  Sphere geometry manager to perform the rendering
/// @param fore0   Foreground scene at the beginning of a dissolve
/// @param fore1   Foreground scene at the end of a dissolve
/// @param back0   Background scene at the beginning of a dissolve
/// @param back1   Background scene at the end of a dissolve
/// @param P       Projection matrix in OpenGL column-major order
/// @param M       Model-view matrix in OpenGL column-major order
/// @param channel Channel index
/// @param frame   Frame number
/// @param t       Dissolve time between 0 and 1

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *fore0,
                        scm_scene  *fore1,
                        scm_scene  *back0,
                        scm_scene  *back1,
                      const double *P,
                      const double *M, int channel, int frame, double t)
{
    GLfloat blur_T[16];
    GLfloat atmo_T[16];
    GLfloat atmo_p[ 4];

    const bool do_fade = check_fade(fore0, fore1, back0, back1, t);
    const bool do_blur = check_blur(P, M, blur_T, previous_T[channel]);
    const bool do_atmo = check_atmo(P, M, atmo_T, atmo_p);

    if (!do_fade && !do_blur && !do_atmo)
        render(sphere, fore0, back0, P, M, channel, frame);

    else
    {
        GLint framebuffer;

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);

        // Render the scene(s) to the offscreen framebuffers.

        glPushAttrib(GL_VIEWPORT_BIT | GL_SCISSOR_BIT);
        {
            glViewport(0, 0, width, height);
            glScissor (0, 0, width, height);
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

        // Bind the resulting textures.

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
            glUniform1i       (uniform_both_n,       blur);
            glUniformMatrix4fv(uniform_both_T, 1, 0, blur_T);
        }
        else if (do_fade && !do_blur)
        {
            glUseProgram(render_fade.program);
            glUniform1f       (uniform_fade_t,       t);
        }
        else if (!do_fade && do_blur)
        {
            glUseProgram(render_blur.program);
            glUniform1i       (uniform_blur_n,       blur);
            glUniformMatrix4fv(uniform_blur_T, 1, 0, blur_T);
        }
        else
        {
            glUseProgram(render_atmo.program);
            glUniform3fv      (uniform_atmo_c, 1,    atmo_c);
            glUniform2fv      (uniform_atmo_r, 1,    atmo_r);
            glUniform3fv      (uniform_atmo_p, 1,    atmo_p);
            glUniformMatrix4fv(uniform_atmo_T, 1, 0, atmo_T);
        }

        // Render the blur / fade to the framebuffer.

        fillscreen(width, height);
        glUseProgram(0);
    }
}

/// Render the foreground and background spheres without blur or dissolve.
///
/// This function is usually called by the previous function as needed to
/// produce the desired effects. Calling it directly is a legitimate means
/// of circumventing these options.
///
/// @param sphere  Sphere geometry manager to perform the rendering
/// @param fore    Foreground scene
/// @param back    Background scene
/// @param P       Projection matrix in OpenGL column-major order
/// @param M       Model-view matrix in OpenGL column-major order
/// @param channel Channel index
/// @param frame   Frame number

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
        // Center the sphere at the origin and scale it to the far plane.

        double N[16], k = fardistance(P);

        midentity(N);
        vmul(N + 0, M + 0, k / vlen(M + 0));
        vmul(N + 4, M + 4, k / vlen(M + 4));
        vmul(N + 8, M + 8, k / vlen(M + 8));

        // Apply the transform.

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixd(P);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(N);

        mmultiply(T, P, N);

        // Render the inside of the sphere.

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
        {
            glEnable(GL_DEPTH_CLAMP);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            glFrontFace(GL_CCW);
            sphere->draw(back, T, width, height, channel, frame);
            back->draw_label();
        }
        glPopAttrib();

        // Clear the alpha channel to distinguish background from foreground.

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColorMask(GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE);

        // TODO: Simplify the fardistance hack using the clear depth buffer?
        // This change does impact the blur shader.
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

//------------------------------------------------------------------------------

/// Initialize the uniforms of the given GLSL program object.

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

    // A transforms a fragment coordinate to a texture coordinate.

    A[0] = 1/w; A[4] = 0.0; A[ 8] = 0.0; A[12] =  0.0;
    A[1] = 0.0; A[5] = 1/h; A[ 9] = 0.0; A[13] =  0.0;
    A[2] = 0.0; A[6] = 0.0; A[10] = 1.0; A[14] =  0.0;
    A[3] = 0.0; A[7] = 0.0; A[11] = 0.0; A[15] =  1.0;

    // B transforms a texture coordinate to a normalized device coordinace.

    B[0] = 2.0; B[4] = 0.0; B[ 8] = 0.0; B[12] = -1.0;
    B[1] = 0.0; B[5] = 2.0; B[ 9] = 0.0; B[13] = -1.0;
    B[2] = 0.0; B[6] = 0.0; B[10] = 2.0; B[14] = -1.0;
    B[3] = 0.0; B[7] = 0.0; B[11] = 0.0; B[15] =  1.0;

    // C transforms a normalized device coordinate to a texture coordinate

    C[0] = 0.5; C[4] = 0.0; C[ 8] = 0.0; C[12] =  0.5;
    C[1] = 0.0; C[5] = 0.5; C[ 9] = 0.0; C[13] =  0.5;
    C[2] = 0.0; C[6] = 0.0; C[10] = 0.5; C[14] =  0.5;
    C[3] = 0.0; C[7] = 0.0; C[11] = 0.0; C[15] =  1.0;

    // D transforms a texture coordinate to a fragment coordinate.

    D[0] =   w; D[4] = 0.0; D[ 8] = 0.0; D[12] =  0.0;
    D[1] = 0.0; D[5] =   h; D[ 9] = 0.0; D[13] =  0.0;
    D[2] = 0.0; D[6] = 0.0; D[10] = 1.0; D[14] =  0.0;
    D[3] = 0.0; D[7] = 0.0; D[11] = 0.0; D[15] =  1.0;
}

//------------------------------------------------------------------------------

#include "scm-render-fade-vert.h"
#include "scm-render-fade-frag.h"
#include "scm-render-blur-vert.h"
#include "scm-render-blur-frag.h"
#include "scm-render-both-vert.h"
#include "scm-render-both-frag.h"
#include "scm-render-atmo-vert.h"
#include "scm-render-atmo-frag.h"

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
    glsl_source(&render_atmo, (const char *) scm_render_atmo_vert,
                                             scm_render_atmo_vert_len,
                              (const char *) scm_render_atmo_frag,
                                             scm_render_atmo_frag_len);

    init_uniforms(render_fade.program);
    init_uniforms(render_blur.program);
    init_uniforms(render_both.program);
    init_uniforms(render_atmo.program);

    glUseProgram(render_fade.program);
    uniform_fade_t = glsl_uniform(render_fade.program, "t");

    glUseProgram(render_blur.program);
    uniform_blur_n = glsl_uniform(render_blur.program, "n");
    uniform_blur_T = glsl_uniform(render_blur.program, "T");

    glUseProgram(render_both.program);
    uniform_both_t = glsl_uniform(render_both.program, "t");
    uniform_both_n = glsl_uniform(render_both.program, "n");
    uniform_both_T = glsl_uniform(render_both.program, "T");

    glUseProgram(render_atmo.program);
    uniform_atmo_p = glsl_uniform(render_atmo.program, "p");
    uniform_atmo_c = glsl_uniform(render_atmo.program, "atmo_c");
    uniform_atmo_r = glsl_uniform(render_atmo.program, "atmo_r");
    uniform_atmo_T = glsl_uniform(render_atmo.program, "atmo_T");

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
    glsl_delete(&render_atmo);
}

//------------------------------------------------------------------------------

/// Determine whether fading is necessary.

bool scm_render::check_fade(scm_scene *fore0, scm_scene *fore1,
                            scm_scene *back0, scm_scene *back1, double t)
{
    if (t < 1.0 / 255)  return false;
    if (fore0 != fore1) return true;
    if (back0 != back1) return true;
    return false;
}

/// Determine whether blurring is necessary and compute its transform.

bool scm_render::check_blur(const double *P,
                            const double *M, GLfloat *U, double *S)
{
    if (blur)
    {
        double T[16];
        double I[16];
        double N[16];

        // T is the current view-projection transform. S is the previous one.

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
            // Compose a transform taking current fragment coordinates to the
            // fragment coordinates of the previous frame.

            minvert (I, T);    // Inverse of the current view-projection.
            mcpy    (N, D);    // 6. Texture coordinate to fragment coordinate
            mcompose(N, C);    // 5. NDC to texture coordinate
            mcompose(N, S);    // 4. World coordinate to previous NDC
            mcompose(N, I);    // 3. NDC to current world coordinate
            mcompose(N, B);    // 2. Texture coordinate to NDC
            mcompose(N, A);    // 1. Fragment coordinate to texture coordinate
            mcpy    (S, T);    // Store the current transform til next frame

            // Return this matrix for use as an OpenGL uniform.

            for (int i = 0; i < 16; i++)
                U[i] = GLfloat(N[i]);

            return true;
        }
    }
    return false;
}

/// Determine whether atmosphere rendering is enabled and compute its tranform.

bool scm_render::check_atmo(const double *P,
                            const double *M, GLfloat *U, GLfloat *p)
{
    if (atmo_r[1] > 0)
    {
        double T[16];
        double I[16];
        double N[16];

        // Compose a transform taking fragment coordinates to world coordinates.

        mmultiply(T, P, M);  // Current view-projection transform
        minvert  (N, T);     // 3. NDC to current world coordinate
        mcompose (N, B);     // 2. Texture coordinate to NDC
        mcompose (N, A);     // 1. Fragment coordinate to texture coordinate

        // Return this matrix for use as an OpenGL uniform.

        for (int i = 0; i < 16; i++)
            U[i] = GLfloat(N[i]);

        // Return the view position for use as an OpenGL uniform.

        minvert(I, M);

        p[0] = GLfloat(I[12]);
        p[1] = GLfloat(I[13]);
        p[2] = GLfloat(I[14]);

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------

/// Draw a screen-filling rectangle.

static void fillscreen(int w, int h)
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
            glTexCoord2i(0, 0); glVertex2f(-1.0f, -1.0f);
            glTexCoord2i(w, 0); glVertex2f(+1.0f, -1.0f);
            glTexCoord2i(w, h); glVertex2f(+1.0f, +1.0f);
            glTexCoord2i(0, h); glVertex2f(-1.0f, +1.0f);
        }
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    glPopAttrib();
}

/// Set the OpenGL state for wireframe rendering.

static void wire_on()
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.0);
}

/// Unset the OpenGL state for wireframe rendering.

static void wire_off()
{
    glPopAttrib();
}

/// Calculate the distance to the far clipping plane of the give projection.

static double fardistance(const double *P)
{
    double c[4] = { 0.0, 0.0, 1.0, 1.0 };
    double e[4] = { 0.0, 0.0, 0.0, 0.0 };
    double I[16];

    minvert   (I, P);
    wtransform(e, I, c);

    return vlen(e) / e[3];
}

//------------------------------------------------------------------------------
