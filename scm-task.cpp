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

#include <cstdlib>
#include <GL/glew.h>
#include <tiffio.h>

#include "scm-task.hpp"
#include "scm-file.hpp"

//------------------------------------------------------------------------------

scm_task::scm_task()
    : scm_item()
{
}

scm_task::scm_task(int f, long long i)
    : scm_item(f, i), o(0), n(0), c(0), b(0), u(0), d(false)
{
}

// Construct a load task. Map the PBO to provide a destination for the loader.

scm_task::scm_task(int f, long long i, uint64 o, int n, int c, int b, GLuint u)
    : scm_item(f, i), o(o), n(n), c(c), b(b), u(u), d(false)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        const size_t s = size_t(n + 2) * size_t(n + 2) * c * b / 8;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, s, 0, GL_STREAM_DRAW);
        p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// Upload the pixel buffer to the OpenGL texture object.

void scm_task::make_page(int x, int y)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, n + 2, n + 2,
                                     scm_external_form(c, b),
                                     scm_external_type(c, b), 0);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// A page was loaded but is no longer necessary. Discard the pixel buffer.

void scm_task::dump_page()
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// Load the page at offset o of TIFF T. Store it in pixel buffer p. On success,
// mark the buffer as dirty.

void scm_task::load_page(TIFF *T, void *q)
{
    d = scm_load_page(T, o, n + 2, n + 2, c, b, p, q);
}

//------------------------------------------------------------------------------

// Select an OpenGL internal texture format for an image with c channels and
// b bits per channel.

GLenum scm_internal_form(uint16 c, uint16 b)
{
    if      (b == 32)
        switch (c)
        {
        case  1: return GL_LUMINANCE32F_ARB;
        case  2: return GL_LUMINANCE_ALPHA32F_ARB;
        case  3: return GL_RGB32F_ARB;
        default: return GL_RGBA32F_ARB;
        }
    else if (b == 16)
        switch (c)
        {
        case  1: return GL_LUMINANCE16;
        case  2: return GL_LUMINANCE16_ALPHA16;
        case  3: return GL_RGB16;
        default: return GL_RGBA16;
        }
    else
        switch (c)
        {
        case  1: return GL_LUMINANCE;
        case  2: return GL_LUMINANCE_ALPHA;
        case  3: return GL_RGBA; // *
        default: return GL_RGBA;
        }
}

// Select an OpenGL external texture format for an image with c channels.

GLenum scm_external_form(uint16 c, uint16 b)
{
    if (b == 8)
        switch (c)
        {
        case  1: return GL_LUMINANCE;
        case  2: return GL_LUMINANCE_ALPHA;
        case  3: return GL_BGRA; // *
        default: return GL_BGRA; // *
        }
    else
        switch (c)
        {
        case  1: return GL_LUMINANCE;
        case  2: return GL_LUMINANCE_ALPHA;
        case  3: return GL_RGB;
        default: return GL_RGBA;
        }
}

// Select an OpenGL data type for an image with c channels of b bits.

GLenum scm_external_type(uint16 c, uint16 b)
{
    if      (b ==  8 && c == 3) return GL_UNSIGNED_INT_8_8_8_8_REV; // *
    else if (b ==  8 && c == 4) return GL_UNSIGNED_INT_8_8_8_8_REV;
    else if (b == 32)           return GL_FLOAT;
    else if (b == 16)           return GL_UNSIGNED_SHORT;
    else                        return GL_UNSIGNED_BYTE;
}

// * BGRA order. 24-bit images are always padded to 32 bits.

//------------------------------------------------------------------------------
