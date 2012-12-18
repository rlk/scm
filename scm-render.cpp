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
    midentity(L);
    init_ogl();
}

scm_render::~scm_render()
{
    free_ogl();
}

//------------------------------------------------------------------------------

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
    init_fbo(framebuffer0, color0, depth0, width, height);
    init_fbo(framebuffer1, color1, depth1, width, height);

    glsl_create(&fade, "scm/scm-render-fade.vert", "scm/scm-render-fade.frag");
    glsl_create(&blur, "scm/scm-render-blur.vert", "scm/scm-render-blur.frag");
    glsl_create(&both, "scm/scm-render-both.vert", "scm/scm-render-both.frag");

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

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *scene0,
                        scm_scene  *scene1,
                        const double *M, double t, int channel, int frame)
{
    sphere->draw(scene0, M, width, height, channel, frame);

    memcpy(L, M, 16 * sizeof (double));
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
