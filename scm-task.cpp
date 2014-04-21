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

/// Construct a load task
///
/// @param f File index
/// @param i Page index

scm_task::scm_task(int f, long long i)
    : scm_item(f, i), o(0), n(0), c(0), b(0), u(0), d(false)
{
}

/// Construct a load task. Map the PBO to provide a destination for the loader.
///
/// @param f File index
/// @param i Page index
/// @param o TIFF offset
/// @param n Page size in pixels
/// @param c Page channels per pixel
/// @param b Page bits per channel
/// @param u Pixel buffer object
/// @param C Destination cache

scm_task::scm_task(int f, long long i, uint64 o, int n, int c, int b, GLuint u, scm_cache *C)
    : scm_item(f, i), o(o), n(n), c(c), b(b), u(u), d(false), C(C)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        const size_t s = size_t(n + 2) * size_t(n + 2) * scm_pixel_size(c, b);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, s, 0, GL_STREAM_DRAW);
        p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/// Upload the pixel buffer to the OpenGL texture object.
///
/// @param x Location of upper-left pixel
/// @param y Location of upper-left pixel

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

/// Discard the pixel buffer
///
/// This used when a load task was created but its data should not be uploaded
/// to VRAM. This may be because the task could not be added to the load queue,
/// because the loader thread failed, or because the page was rejected for cache
/// inertion due priority.

void scm_task::dump_page()
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/// Load a page. On success, mark the buffer as dirty.
///
/// This method is called by a loader thread and exists solely to marshal
/// the entensive argument list of the global function scm_load_page.
///
/// @param name TIFF name (used for generating error pages)
/// @param T    TIFF pointer

bool scm_task::load_page(const char *name, TIFF *T)
{
    return (d = scm_load_page(name, i, T, o, n + 2, n + 2, c, b, p));
}

//------------------------------------------------------------------------------

/// Select an OpenGL internal texture format
///
/// @param c Channels per pixel
/// @param b Bits per channel

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
        case  3: return GL_RGB;
        default: return GL_BGRA;
        }
}

/// Select an OpenGL external texture format
///
/// @param c Channels per pixel
/// @param b Bits per channel

GLenum scm_external_form(uint16 c, uint16 b)
{
    if (b == 8)
        switch (c)
        {
        case  1: return GL_LUMINANCE;
        case  2: return GL_LUMINANCE_ALPHA;
        case  3: return GL_RGB;
        default: return GL_BGRA;
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

/// Select an OpenGL data type
///
/// @param c Channels per pixel
/// @param b Bits per channel

GLenum scm_external_type(uint16 c, uint16 b)
{
    if      (b == 32) return GL_FLOAT;
    else if (b == 16) return GL_UNSIGNED_SHORT;
    else if (c ==  4) return GL_UNSIGNED_INT_8_8_8_8_REV;
    else              return GL_UNSIGNED_BYTE;
}

/// Return the storage size for an OpenGL pixel
///
/// @param c Channels per pixel
/// @param b Bits per channel

GLsizei scm_pixel_size(uint16 c, uint16 b)
{
    return c * b / 8;
}

//------------------------------------------------------------------------------
