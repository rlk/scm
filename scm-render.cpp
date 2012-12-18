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
    width(w), height(h), motion(8)
{
    init_ogl();
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
        glUniform2f(glGetUniformLocation(program, "size"), w, h);
    }
    glUseProgram(0);
}

// Initialize the storage and parameters of an off-screen color buffer.

static void init_color(GLuint color, int w, int h)
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Initialize the storage and parameters of an off-screen depth buffer.

static void init_depth(GLuint depth, int w, int h)
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Generate and initialize a framebuffer object with color and depth buffers.

static void init_fbo(GLuint& color,
                     GLuint& depth,
                     GLuint& framebuffer, int w, int h)
{
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
            scm_log("init_fbo incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//------------------------------------------------------------------------------

void scm_render::init_ogl()
{
    glsl_create(&fade, "scm/scm-render-fade.vert", "scm/scm-render-fade.frag");
    glsl_create(&blur, "scm/scm-render-blur.vert", "scm/scm-render-blur.frag");
    glsl_create(&both, "scm/scm-render-both.vert", "scm/scm-render-both.frag");

    init_uniforms(fade.program, width, height);
    init_uniforms(blur.program, width, height);
    init_uniforms(both.program, width, height);

    fade_ut = glsl_uniform(fade.program, "t");
    both_ut = glsl_uniform(both.program, "t");
    blur_un = glsl_uniform(blur.program, "n");
    both_un = glsl_uniform(both.program, "n");
    blur_uI = glsl_uniform(blur.program, "I");
    both_uI = glsl_uniform(both.program, "I");
    blur_uL = glsl_uniform(blur.program, "L");
    both_uL = glsl_uniform(both.program, "L");

    init_fbo(color0, depth0, framebuffer0, width, height);
    init_fbo(color1, depth1, framebuffer1, width, height);

    scm_log("scm_render init_ogl %d %d", width, height);
}

void scm_render::free_ogl()
{
    scm_log("scm_render free_ogl %d %d", width, height);

    glsl_delete(&fade);
    glsl_delete(&blur);
    glsl_delete(&both);

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

static void fillscreen()
{
    glPushAttrib(GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    {
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

void scm_render::render0(scm_sphere *sphere,
                         scm_scene  *scene,
                         const double *M, int channel, int frame)
{
    glPushAttrib(GL_VIEWPORT_BIT);
    {
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer0);
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            sphere->draw(scene, M, width, height, channel, frame);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glPopAttrib();
}

void scm_render::render1(scm_sphere *sphere,
                         scm_scene  *scene,
                         const double *M, int channel, int frame)
{
    glPushAttrib(GL_VIEWPORT_BIT);
    {
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            sphere->draw(scene, M, width, height, channel, frame);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glPopAttrib();
}

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *scene0,
                        scm_scene  *scene1,
                        const double *M, double t, int channel, int frame)
{
    const bool mixing = (t >= 1.0 / 256.0);

    if (!mixing && !motion)
    {
        sphere->draw(scene0, M, width, height, channel, frame);
    }
    else
    {
        double  T[16];
        GLfloat I[16];
        minvert(T, M);

        for (int i = 0; i < 16; i++)
            I[i] = GLfloat(T[i]);

        // Render the scene to the offscreen framebuffers.

        if (mixing)
        {
            render1(sphere, scene1, M, channel, frame);
            render0(sphere, scene0, M, channel, frame);
        }
        else
            render0(sphere, scene0, M, channel, frame);

        // Bind the resurting textures.

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_RECTANGLE, depth1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_RECTANGLE, depth0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_RECTANGLE, color1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_RECTANGLE, color0);

        // Bind the right shader and set the necessary uniforms.

        if      (mixing && motion)
        {
            glUseProgram      (both.program);
            glUniform1f       (both_ut,       t);
            glUniform1i       (both_un,  motion);
            glUniformMatrix4fv(both_uL, 1, 0, L);
            glUniformMatrix4fv(both_uI, 1, 0, I);
        }
        else if (mixing && !motion)
        {
            glUseProgram      (fade.program);
            glUniform1f       (fade_ut,       t);
        }
        else if (!mixing && motion)
        {
            glUseProgram      (blur.program);
            glUniform1i       (blur_un,  motion);
            glUniformMatrix4fv(blur_uL, 1, 0, L);
            glUniformMatrix4fv(blur_uI, 1, 0, I);
        }

        fillscreen();
        glUseProgram(0);

        for (int i = 0; i < 16; i++)
            L[i] = I[i];
    }
}

//------------------------------------------------------------------------------

void scm_render::set_size(int w, int h)
{
    free_ogl();
    width  = w;
    height = h;
    init_ogl();
}

void scm_render::set_blur(int m)
{
    motion = m;
}

//------------------------------------------------------------------------------
