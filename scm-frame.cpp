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

#include "scm-frame.hpp"
#include "scm-log.hpp"

#include <stdlib.h>

//------------------------------------------------------------------------------

scm_frame::scm_frame(GLsizei w, GLsizei h) : width(w), height(h)
{
    glGenFramebuffers(1, &frame);
    glGenTextures    (1, &color);
    glGenTextures    (1, &depth);

    init_color();
    init_depth();
    init_frame();
}

scm_frame::~scm_frame()
{
    if (color) glDeleteTextures    (1, &color);
    if (depth) glDeleteTextures    (1, &depth);
    if (frame) glDeleteFramebuffers(1, &frame);
}

void scm_frame::bind_frame() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame);
}

void scm_frame::bind_color() const
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);
}

void scm_frame::bind_depth() const
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);
}

//------------------------------------------------------------------------------

// Initialize the storage and parameters of an off-screen color buffer.

void scm_frame::init_color()
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Initialize the storage and parameters of an off-screen depth buffer.

void scm_frame::init_depth()
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT24, width, height,
                 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Generate and initialize a framebuffer object with color and depth buffers.

void scm_frame::init_frame()
{
    GLint previous;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);

    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, frame);
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_RECTANGLE, color, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_RECTANGLE, depth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            scm_log("* init_framebuffer incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, previous);
}

//------------------------------------------------------------------------------
