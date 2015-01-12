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

//------------------------------------------------------------------------------

/// Create a new OpenGL framebuffer object with color and depth textures
///
/// @param w width in pixels
/// @param h height in pixels

scm_frame::scm_frame(GLsizei w, GLsizei h) : width(w), height(h)
{
    glGenFramebuffers(1, &frame);
    glGenTextures    (1, &color);
    glGenTextures    (1, &depth);

    init_color();
    init_depth();
    init_frame();
}

/// Delete the framebuffer object and its textures

scm_frame::~scm_frame()
{
    if (color) glDeleteTextures    (1, &color);
    if (depth) glDeleteTextures    (1, &depth);
    if (frame) glDeleteFramebuffers(1, &frame);
}

/// Bind the framebuffer as render target and set the Viewport and Scissor

void scm_frame::bind_frame() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame);
    glViewport(0, 0, width, height);
    glScissor (0, 0, width, height);
}

/// Bind the color texture

void scm_frame::bind_color() const
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);
}

/// Bind the depth texture

void scm_frame::bind_depth() const
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);
}

//------------------------------------------------------------------------------

/// Initialize the storage and parameters of a color buffer.

void scm_frame::init_color()
{
    glBindTexture(GL_TEXTURE_RECTANGLE, color);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/// Initialize the storage and parameters of a depth buffer.

void scm_frame::init_depth()
{
    glBindTexture(GL_TEXTURE_RECTANGLE, depth);

    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT24, width, height,
                 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/// Generate and initialize a framebuffer object with color and depth buffers
/// and confirm its status.

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
