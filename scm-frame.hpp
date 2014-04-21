// Copyright (C) 2011-2013 Robert Kooima
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

#ifndef SCM_FRAME_HPP
#define SCM_FRAME_HPP

#include <GL/glew.h>

//------------------------------------------------------------------------------

/// An scm_frame abstracts the OpenGL framebuffer object.

class scm_frame
{
public:

    scm_frame(GLsizei, GLsizei);
   ~scm_frame();

    void bind_frame() const;
    void bind_color() const;
    void bind_depth() const;

private:

    GLuint  frame;
    GLuint  color;
    GLuint  depth;
    GLsizei width;
    GLsizei height;

    void init_color();
    void init_depth();
    void init_frame();
};

//------------------------------------------------------------------------------

#endif
