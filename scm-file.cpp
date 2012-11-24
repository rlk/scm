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
#include <sstream>

#include "scm-index.hpp"
#include "scm-file.hpp"

//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>

static bool exists(const std::string& path)
{
    struct stat info;

    if (stat(path.c_str(), &info) == 0)
        return ((info.st_mode & S_IFMT) == S_IFREG);
    else
        return false;
}

//------------------------------------------------------------------------------

#ifdef _WIN32
#define PATH_LIST_SEP ';'
#else
#define PATH_LIST_SEP ':'
#endif

// Construct a file table entry. Open the TIFF briefly to determine its format.

scm_file::scm_file(const std::string& tiff) :
    name(tiff),
    xv(0), xc(0),
    ov(0), oc(0),
    av(0), ac(0),
    zv(0), zc(0)
{
    // If the given file name is absolute, use it.

    if (exists(tiff))
        path = tiff;

    // Otherwise, search the SCM path for the file.

    else if (char *val = getenv("SCMPATH"))
    {
        std::stringstream list(val);
        std::string       dir;
        std::string       temp;

        while (std::getline(list, dir, PATH_LIST_SEP))
        {
            temp = dir + "/" + tiff;

            if (exists(temp))
            {
                path = temp;
                break;
            }
        }
    }

    if (!path.empty())
    {
        if (TIFF *T = TIFFOpen(path.c_str(), "r"))
        {
            uint64 n = 0;
            void  *p = 0;

            TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &w);
            TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &h);
            TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &b);
            TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &c);
            TIFFGetField(T, TIFFTAG_SAMPLEFORMAT,    &g);

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
                if ((av = malloc(n * c * b / 8)))
                {
                    memcpy(av, p, n * c * b / 8);
                    ac = n;
                }
            }
            if (TIFFGetField(T, 0xFFB4, &n, &p))
            {
                if ((zv = malloc(n * c * b / 8)))
                {
                    memcpy(zv, p, n * c * b / 8);
                    zc = n;
                }
            }

            tmp = malloc(TIFFScanlineSize(T));

            TIFFClose(T);
        }
    }
}

scm_file::~scm_file()
{
    free(tmp);
    free(zv);
    free(av);
    free(ov);
    free(xv);
}

//------------------------------------------------------------------------------

bool scm_file::load_page(void *p, uint64 o)
{
    uint32 r = 0;

    if (TIFF *T = TIFFOpen(path.c_str(), "r"))
    {
        if (TIFFSetSubDirectory(T, o))
        {
            uint32 W, H;
            uint16 C, B;

            TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &W);
            TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &H);
            TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &B);
            TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &C);

            if (W == w && H == h && B == b && C == c)
            {
                if (c == 3 && b == 8)
                {
                    const uint32 S = w * 4 * b / 8;

                    for (r = 0; r < h; ++r)
                    {
                        TIFFReadScanline(T, tmp, r, 0);

                        for (int j = w - 1; j >= 0; --j)
                        {
                            uint8 *s = (uint8 *) tmp       + j * c * b / 8;
                            uint8 *d = (uint8 *) p + r * S + j * 4 * b / 8;

                            d[0] = s[2];
                            d[1] = s[1];
                            d[2] = s[0];
                            d[3] = 0xFF;
                        }
                    }
                }
                else
                {
                    const uint32 S = (uint32) TIFFScanlineSize(T);

                    for (r = 0; r < h; ++r)
                        TIFFReadScanline(T, (uint8 *) p + r * S, r, 0);
                }
            }
        }
        TIFFClose(T);
    }
    return (r > 0);
}

//------------------------------------------------------------------------------

// Determine whether page i is given by this file.

bool scm_file::status(uint64 i) const
{
    if (toindex(i) < xc)
        return true;
    else
        return false;
}

// Seek page i in the page catalog and return its file offset.

uint64 scm_file::offset(uint64 i) const
{
    uint64 oj;

    if ((oj = toindex(i)) < oc)
    {
        return ov[oj];
    }
    return 0;
}

// Determine the min and max values of page i. Seek it in the page catalog and
// reference the corresponding page in the min and max caches. If page i is not
// represented, assume its parent provides a useful bound and iterate up.

void scm_file::bounds(uint64 i, float& r0, float& r1) const
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

// Return the buffer length for a page of this file. 24-bit is padded to 32.

size_t scm_file::length() const
{
    if (c == 3 && b == 8)
        return w * h * 4 * b / 8;
    else
        return w * h * c * b / 8;
}

//------------------------------------------------------------------------------

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
    if (b == 8)
    {
        if (g == 2)
            return ((          char *) v)[i] / 127.f;
        else
            return ((unsigned  char *) v)[i] / 255.f;
    }
    else if (b == 16)
    {
        if (g == 2)
            return ((         short *) v)[i] / 32767.f;
        else
            return ((unsigned short *) v)[i] / 65535.f;
    }
    else if (b == 32)
        return ((float *) v)[i];

    return 0.f;
}

//------------------------------------------------------------------------------
