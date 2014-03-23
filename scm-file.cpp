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
#include <cstring>
#include <cmath>

#ifdef _WIN32
#define snprintf _snprintf_s
#endif

#include "util3d/math3d.h"
#include "scm-index.hpp"
#include "scm-cache.hpp"
#include "scm-file.hpp"
#include "scm-path.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

// Construct a file table entry. Open the TIFF briefly to determine its format
// and cache its meta-data.

scm_file::scm_file(const std::string& tiff) :
    name(tiff),
    needs(32),
    active(true),
    sampler(0),
    w(256), h(256), c(1), b(8),
    xv(0), xc(0),
    ov(0), oc(0),
    av(0), ac(0),
    zv(0), zc(0)
{
    // Attempt to find and load the located TIFF.

    path = scm_path_search(tiff);

    if (!path.empty())
    {
        if (TIFF *T = TIFFOpen(path.c_str(), "r"))
        {
            uint64 n = 0;
            void  *p = 0;

            // Cache the image parameters.

            TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &w);
            TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &h);
            TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &b);
            TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &c);

            // Preload all metadata.

            if (TIFFGetField(T, 0xFFB1, &n, &p))
            {
                if ((xv = (uint64 *) malloc(n * sizeof (uint64))))
                {
                    memcpy(xv, p, n * sizeof (uint64));
                    xc = n;
                }
            }
            if (TIFFGetField(T, 0xFFB2, &n, &p))
            {
                if ((ov = (uint64 *) malloc(n * sizeof (uint64))))
                {
                    memcpy(ov, p, n * sizeof (uint64));
                    oc = n;
                }
            }
            if (TIFFGetField(T, 0xFFB3, &n, &p))
            {
                if ((av = malloc(n * b / 8)))
                {
                    memcpy(av, p, n * b / 8);
                    ac = n;
                }
            }
            if (TIFFGetField(T, 0xFFB4, &n, &p))
            {
                if ((zv = malloc(n * b / 8)))
                {
                    memcpy(zv, p, n * b / 8);
                    zc = n;
                }
            }
            TIFFClose(T);
        }
    }
    else path = name;

    scm_log("scm_file constructor %s", path.c_str());
}

scm_file::~scm_file()
{
    scm_log("scm_file destructor %s", path.c_str());

    // If we are not already exiting, signal the intention to do so.

    if (is_active()) deactivate();

    // Await the exit of each loader.

    int s = 0;

    for (thread_i i = threads.begin(); i != threads.end(); ++i)
        SDL_WaitThread(*i, &s);

    // Release all resources.

    if (sampler) delete sampler;

    free(zv);
    free(av);
    free(ov);
    free(xv);
}

//------------------------------------------------------------------------------

void scm_file::activate(scm_cache *cache)
{
    this->cache = cache;

    // Launch the loader threads.

    int loader(void *);

    for (int i = 0; i < 2; ++i)
        threads.push_back(SDL_CreateThread(loader, "scm-loader", this));
}

void scm_file::deactivate()
{
    // Notify the loaders that they may disregard their tasks.

    active.set(false);

    // A non-empty queue ensures that each loader unblocks.

    int t = 1;

    for (thread_i i = threads.begin(); i != threads.end(); ++i, ++t)
    {
        scm_task junk(-t, -t);
        needs.try_insert(junk);
    }
}

bool scm_file::add_need(scm_task& task)
{
    return needs.try_insert(task);
}

//------------------------------------------------------------------------------

// Determine whether page i is given by this file. If no catalog exists then
// return true to indicate that data will be synthesized.

bool scm_file::get_page_status(uint64 i) const
{
    if (xc)
    {
        if (toindex(i) < xc)
            return true;
        else
            return false;
    }
    return true;
}

// Seek page i in the page catalog and return its file offset. Return zero
// on failure. If this file is synthesizing data, return a non-zero value.

uint64 scm_file::get_page_offset(uint64 i) const
{
    if (oc)
    {
        uint64 oj;

        if ((oj = toindex(i)) < oc)
        {
            return ov[oj];
        }
        return 0;
    }
    return (uint64) (-1);
}

// Determine the min and max values of page i. Seek it in the page catalog and
// reference the corresponding page in the min and max caches. If page i is not
// represented, assume its parent provides a useful bound and iterate up.

void scm_file::get_page_bounds(uint64 i, float& r0, float& r1) const
{
    if (ac && zc)
    {
        uint64 aj = (uint64) (-1);
        uint64 zj = (uint64) (-1);

        while (aj >= ac || zj >= zc)
        {
            uint64 j = toindex(i);

            if (aj >= ac) aj = j;
            if (zj >= zc) zj = j;

            if (i < 6)
                break;
            else
                i = scm_page_parent(i);
        }

        r0 = (aj < ac) ? tofloat(av, aj * c) : 1.f;
        r1 = (zj < zc) ? tofloat(zv, zj * c) : 1.f;
    }
    else
    {
        r0 = 0.5f;
        r1 = 0.5f;
    }
}

// Sample this file along vector v using linear filtering.

float scm_file::get_page_sample(const double *v)
{
    if (xc)
    {
        if (sampler == 0)
            sampler = new scm_sample(this);

        return sampler ? sampler->get(v) : 1.f;
    }
    return 0.5f;
}

//------------------------------------------------------------------------------

// Compare two uint64s, for use by bsearch and qsort.

static int xcmp(const void *p, const void *q)
{
    const uint64 *a = (const uint64 *) p;
    const uint64 *b = (const uint64 *) q;

    if      (a[0] < b[0]) return -1;
    else if (a[0] > b[0]) return +1;
    else                  return  0;
}

// Determine where SCM index i appears in the sorted index list xv. This will
// indicate where the file offset and extrema appear in ov, av, and zv.

uint64 scm_file::toindex(uint64 i) const
{
    void *p;

    if (xc)
    {
        if ((p = bsearch(&i, xv, xc, sizeof (uint64), xcmp)))
        {
            return (uint64) ((uint64 *) p - xv);
        }
    }
    return (uint64) (-1);
}

// Return sample i of the given buffer as a float.

float scm_file::tofloat(const void *v, uint64 i) const
{
    switch (b)
    {
    case  8: return ((unsigned  char *) v)[i] /   255.f;
    case 16: return ((unsigned short *) v)[i] / 65535.f;
    case 32: return ((         float *) v)[i];
    default: return 0.f;
    }
}

// Set sample i of the given buffer to a float.

void scm_file::fromfloat(const void *v, uint64 i, float f) const
{
    switch (b)
    {
    case  8: ((unsigned  char *) v)[i] = (unsigned char)  (f *   255.f); break;
    case 16: ((unsigned short *) v)[i] = (unsigned short) (f * 65535.f); break;
    case 32: ((         float *) v)[i] = (         float) (f          ); break;
    }
}

//------------------------------------------------------------------------------

// Seek the deepest page at this location (x, y) of root page a. Return the
// file offset of this page and convert (x, y) to local coordinates there.

uint64 scm_file::find_page(long long a, double& y, double& x) const
{
    long long n = 1;
    long long l = 1;
    uint64    j = 0;
    uint64    o = ov[a];

    while ((j = toindex(scm_page_index(a, l, int(2 * n * y),
                                             int(2 * n * x)))) < oc)
        if (ov[j])
        {
            o = ov[j];
            l = l + 1;
            n = n * 2;
        }
        else break;

    x = (x * n) - floor(x * n);
    y = (y * n) - floor(y * n);

    return o;
}

//------------------------------------------------------------------------------

// This defines an 8x8 bitmap font used to write text directly to images.

static const uint8 bitfont[96][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00 }, // !
    { 0x6c, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00 }, // "
    { 0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00 }, // #
    { 0x18, 0x7e, 0xc0, 0x7c, 0x06, 0xfc, 0x18, 0x00 }, // $
    { 0x00, 0xc6, 0xcc, 0x18, 0x30, 0x66, 0xc6, 0x00 }, // %
    { 0x38, 0x6c, 0x38, 0x76, 0xdc, 0xcc, 0x76, 0x00 }, // &
    { 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '
    { 0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00 }, // (
    { 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00 }, // )
    { 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00 }, // *
    { 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00 }, // +
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30 }, // ,
    { 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00 }, // -
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00 }, // .
    { 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x80, 0x00 }, // /
    { 0x7c, 0xce, 0xde, 0xf6, 0xe6, 0xc6, 0x7c, 0x00 }, // 0
    { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00 }, // 1
    { 0x7c, 0xc6, 0x06, 0x7c, 0xc0, 0xc0, 0xfe, 0x00 }, // 2
    { 0x7c, 0xc6, 0x06, 0x3c, 0x06, 0xc6, 0x7c, 0x00 }, // 3
    { 0x0c, 0xcc, 0xcc, 0xcc, 0xfe, 0x0c, 0x0c, 0x00 }, // 4
    { 0xfe, 0xc0, 0xfc, 0x06, 0x06, 0xc6, 0x7c, 0x00 }, // 5
    { 0x7c, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, // 6
    { 0xfe, 0x06, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x00 }, // 7
    { 0x7c, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0x7c, 0x00 }, // 8
    { 0x7c, 0xc6, 0xc6, 0x7e, 0x06, 0x06, 0x7c, 0x00 }, // 9
    { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00 }, // :
    { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30 }, // ;
    { 0x0c, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x00 }, // <
    { 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00 }, // =
    { 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x00 }, // >
    { 0x3c, 0x66, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x00 }, // ?
    { 0x7c, 0xc6, 0xde, 0xde, 0xde, 0xc0, 0x7e, 0x00 }, // @
    { 0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0x00 }, // A
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xfc, 0x00 }, // B
    { 0x7c, 0xc6, 0xc0, 0xc0, 0xc0, 0xc6, 0x7c, 0x00 }, // C
    { 0xf8, 0xcc, 0xc6, 0xc6, 0xc6, 0xcc, 0xf8, 0x00 }, // D
    { 0xfe, 0xc0, 0xc0, 0xf8, 0xc0, 0xc0, 0xfe, 0x00 }, // E
    { 0xfe, 0xc0, 0xc0, 0xf8, 0xc0, 0xc0, 0xc0, 0x00 }, // F
    { 0x7c, 0xc6, 0xc0, 0xce, 0xc6, 0xc6, 0x7c, 0x00 }, // G
    { 0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00 }, // H
    { 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00 }, // I
    { 0x06, 0x06, 0x06, 0x06, 0x06, 0xc6, 0x7c, 0x00 }, // J
    { 0xc6, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0xc6, 0x00 }, // K
    { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0x00 }, // L
    { 0xc6, 0xee, 0xfe, 0xfe, 0xd6, 0xc6, 0xc6, 0x00 }, // M
    { 0xc6, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0xc6, 0x00 }, // N
    { 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, // O
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0, 0xc0, 0x00 }, // P
    { 0x7c, 0xc6, 0xc6, 0xc6, 0xd6, 0xde, 0x7c, 0x06 }, // Q
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xd8, 0xcc, 0xc6, 0x00 }, // R
    { 0x7c, 0xc6, 0xc0, 0x7c, 0x06, 0xc6, 0x7c, 0x00 }, // S
    { 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 }, // T
    { 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, // U
    { 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00 }, // V
    { 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xfe, 0x6c, 0x00 }, // W
    { 0xc6, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0xc6, 0x00 }, // X
    { 0xc6, 0xc6, 0xc6, 0x7c, 0x18, 0x30, 0x60, 0x00 }, // Y
    { 0xfe, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0xfe, 0x00 }, // Z
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, // [
    { 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x02, 0x00 }, //
    { 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00 }, // ]
    { 0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00 }, // ^
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff }, // _
    { 0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 }, // `
    { 0x00, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00 }, // a
    { 0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xfc, 0x00 }, // b
    { 0x00, 0x00, 0x7c, 0xc6, 0xc0, 0xc6, 0x7c, 0x00 }, // c
    { 0x06, 0x06, 0x7e, 0xc6, 0xc6, 0xc6, 0x7e, 0x00 }, // d
    { 0x00, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00 }, // e
    { 0x1c, 0x36, 0x30, 0x78, 0x30, 0x30, 0x30, 0x00 }, // f
    { 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0x7e, 0x06, 0xfc }, // g
    { 0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00 }, // h
    { 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 }, // i
    { 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0xc6, 0x7c }, // j
    { 0xc0, 0xc0, 0xc6, 0xcc, 0xf8, 0xcc, 0xc6, 0x00 }, // k
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 }, // l
    { 0x00, 0x00, 0xcc, 0xfe, 0xd6, 0xd6, 0xd6, 0x00 }, // m
    { 0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00 }, // n
    { 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, // o
    { 0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0 }, // p
    { 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0x7e, 0x06, 0x06 }, // q
    { 0x00, 0x00, 0xfc, 0xc6, 0xc0, 0xc0, 0xc0, 0x00 }, // r
    { 0x00, 0x00, 0x7e, 0xc0, 0x7c, 0x06, 0xfc, 0x00 }, // s
    { 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00 }, // t
    { 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00 }, // u
    { 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x7c, 0x38, 0x00 }, // v
    { 0x00, 0x00, 0xc6, 0xc6, 0xd6, 0xfe, 0x6c, 0x00 }, // w
    { 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0x00 }, // x
    { 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0xfc }, // y
    { 0x00, 0x00, 0xfe, 0x0c, 0x38, 0x60, 0xfe, 0x00 }, // z
    { 0x0e, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0e, 0x00 }, // {
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // |
    { 0x70, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x70, 0x00 }, // }
    { 0x76, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ~
    { 0x72, 0x6c, 0x6b, 0x40, 0x4C, 0x53, 0x55, 0x00 },
};

//------------------------------------------------------------------------------

static void set_pixel(float v, int x, int y, int w, int c, int b, void *p)
{
    const int d = (y * w + x) * c;

    for (int k = 0; k < c; ++k)
        switch (b)
        {
        case  8: ((uint8  *) p)[d + k] = (uint8 ) (v *   255); break;
        case 16: ((uint16 *) p)[d + k] = (uint16) (v * 65535); break;
        case 32: ((float  *) p)[d + k] = (float ) (v *     1); break;
        }
}

static void set_text(const char *s, int x, int y,
                                    int w, int h, int c, int b, void *p)
{
    while (s[0])
    {
        for     (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)

                if (32 <= s[0] && s[0] < 127 && 0 <= x + j && x + j < w
                                             && 0 <= y + i && y + i < h)
                {
                    bool d = bitfont[s[0] - 32][i] & (1 << (7 - j));

                    set_pixel(d ? 1.f : 0.f, x + j,     y + i, w, c, b, p);
                    set_pixel(d ? 1.f : 0.f, x + j, h - y - i, w, c, b, p);
                }
        s += 1;
        x += 8;
    }
}

//------------------------------------------------------------------------------

// Write a message with diagnostics to the given image buffer.

void scm_page_text(const char *mesg,
                   const char *name,
                   long long i, int w, int h, int c, int b, void *p)
{
    char diag[256];

    // Fill the image with black.

    memset(p, 0, w * h * c * b / 8);

    // Fill the borders with white.

    for (int j = 0; j < h; ++j)
    {
        set_pixel(1.f,     0, j, w, c, b, p);
        set_pixel(1.f,     1, j, w, c, b, p);
        set_pixel(1.f, w - 1, j, w, c, b, p);
        set_pixel(1.f, w - 2, j, w, c, b, p);
        set_pixel(1.f, j,     0, w, c, b, p);
        set_pixel(1.f, j,     1, w, c, b, p);
        set_pixel(1.f, j, h - 1, w, c, b, p);
        set_pixel(1.f, j, h - 2, w, c, b, p);
    }

    // Draw the text.

    snprintf(diag, 256, "%d %d %d %d %d", int(i), w, h, c, b);

    int msz = strlen(mesg);
    int nsz = strlen(name);
    int dsz = strlen(diag);

    set_text(mesg, w / 2 - msz * 4, h / 3 - 14, w, h, c, b, p);
    set_text(name, w / 2 - nsz * 4, h / 3 -  4, w, h, c, b, p);
    set_text(diag, w / 2 - dsz * 4, h / 3 +  6, w, h, c, b, p);
}

// Load the page at offset o of TIFF T. Confirm the image parameters and return
// success.

bool scm_load_page(const char *name, long long i,
                         TIFF *T, uint64 o, int w, int h, int c, int b, void *p)
{
    if (T)
    {
        if (TIFFSetSubDirectory(T, o))
        {
            uint32 W, H;
            uint16 C, B;

            TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &W);
            TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &H);
            TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &B);
            TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &C);

            if (int(W) == w && int(H) == h && int(B) == b && int(C) == c)
            {
                tsize_t N = TIFFNumberOfStrips(T);
                tsize_t S = TIFFStripSize(T);

                for (int l = 0; l < N; ++l)
                    TIFFReadEncodedStrip(T, l, (uint8 *) p + l * S, -1);
            }
            else scm_page_text("Bad page format", name, i, W, H, C, B, p);
        }
        else scm_page_text("Page not found", name, i, w, h, c, b, p);
    }
    else scm_page_text("File not found", name, i, w, h, c, b, p);

    return true;
}

int loader(void *data)
{
    scm_file *file = (scm_file *) data;
    scm_task  task;

    scm_log("loader thread begin %s", file->path.c_str());
    {
        const char *name = file->path.c_str();
        TIFF       *tiff = TIFFOpen(name, "r");

        while ((task = file->needs.remove()).f >= 0)

            if (file->is_active())
            {
                task.load_page(name, tiff);
                file->cache->add_load(task);
            }
            else break;

        if (tiff) TIFFClose(tiff);
    }
    scm_log("loader thread end %s", file->path.c_str());
    return 0;
}
