//  Copyright (C) 2005-2012 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#include <cstdlib>
#include <GL/glew.h>
#include <tiffio.h>

#include "scm-task.hpp"

//------------------------------------------------------------------------------

// Construct a load task. Map the PBO to provide a destination for the loader.

scm_task::scm_task(int f, long long i, uint64 o, GLuint u, GLsizei s)
    : scm_item(f, i), o(o), u(u), d(false)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        glBufferData(GL_PIXEL_UNPACK_BUFFER, s, 0, GL_STREAM_DRAW);
        p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// Upload the pixel buffer to the OpenGL texture object.

void scm_task::make_page(GLint l, uint32 w, uint32 h,
                                  uint16 c, uint16 b, uint16 g)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u);
    {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        // glTexSubImage3D(GL_TEXTURE_TARGET, 0, -1, -1, l, w, h, 1,
        //                                 scm_external_form(c, b, g),
        //                                 scm_external_type(c, b, g), 0);
        glTexSubImage3D(GL_TEXTURE_TARGET, 0, 0, 0, l, w, h, 1,
                                       scm_external_form(c, b, g),
                                       scm_external_type(c, b, g), 0);
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

// Load the current TIFF directory image data into the mapped pixel buffer.
// This executes in a loader thread.

void scm_task::load_page(TIFF *T, uint32 w, uint32 h,
                                  uint16 c, uint16 b, uint16 g)
{
    // Confirm the page format.

    uint32 W, H;
    uint16 C, B, G;

    TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &W);
    TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &H);
    TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &B);
    TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &C);
    TIFFGetField(T, TIFFTAG_SAMPLEFORMAT,    &G);

    if (W == w && H == h && B == b && C == c)
    {
        // Pad a 24-bit image to 32-bit BGRA. TODO: eliminate this malloc.

        if (c == 3 && b == 8)
        {
            if (void *q = malloc(TIFFScanlineSize(T)))
            {
                const uint32 S = w * 4 * b / 8;

                for (uint32 r = 0; r < h; ++r)
                {
                    TIFFReadScanline(T, q, r, 0);

                    for (int j = w - 1; j >= 0; --j)
                    {
                        uint8 *s = (uint8 *) q         + j * c * b / 8;
                        uint8 *d = (uint8 *) p + r * S + j * 4 * b / 8;

                        d[0] = s[2];
                        d[1] = s[1];
                        d[2] = s[0];
                        d[3] = 0xFF;
                    }
                }
                free(q);
            }
        }

        // Load a non-24-bit image normally.

        else
        {
            const uint32 S = (uint32) TIFFScanlineSize(T);

            for (uint32 r = 0; r < h; ++r)
                TIFFReadScanline(T, (uint8 *) p + r * S, r, 0);
        }
    }
}

//------------------------------------------------------------------------------

// Select an OpenGL internal texture format for an image with c channels and
// b bits per channel.

GLenum scm_internal_form(uint16 c, uint16 b, uint16 g)
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
    else // (b == 8)
        switch (c)
        {
        case  1: return GL_LUMINANCE8;
        case  2: return GL_LUMINANCE_ALPHA;
        case  3: return GL_RGBA8; // *
        default: return GL_RGBA8;
        }
}

// Select an OpenGL external texture format for an image with c channels.

GLenum scm_external_form(uint16 c, uint16 b, uint16 g)
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

GLenum scm_external_type(uint16 c, uint16 b, uint16 g)
{
    if      (b ==  8 && c == 3) return GL_UNSIGNED_INT_8_8_8_8_REV; // *
    else if (b ==  8 && c == 4) return GL_UNSIGNED_INT_8_8_8_8_REV;
#if 0
    else if (b ==  8) return (g == 2) ? GL_BYTE  : GL_UNSIGNED_BYTE;
    else if (b == 16) return (g == 2) ? GL_SHORT : GL_UNSIGNED_SHORT;
#endif
    else if (b == 32) return GL_FLOAT;
    else if (b == 16) return GL_UNSIGNED_SHORT;
    else              return GL_UNSIGNED_BYTE;
}

// * 24-bit images are always padded to 32 bits.  BGRA order.

//------------------------------------------------------------------------------
