//  Copyright (C) 2005-2011 Robert Kooima
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
            TIFFClose(T);
        }
    }
}

scm_file::~scm_file()
{
    free(zv);
    free(av);
    free(ov);
    free(xv);
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

uint64 scm_file::index(uint64 i) const
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

// Determine whether page i is given by this file.

bool scm_file::status(uint64 i) const
{
    if (index(i) < xc)
        return true;
    else
        return false;
}

// Seek page i in the page catalog and return its file offset.

uint64 scm_file::offset(uint64 i) const
{
    uint64 oj;

    if ((oj = index(i)) < oc)
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

    r0 = 1.0;
    r1 = 1.0;

    while (aj >= ac || zj >= zc)
    {
        uint64 j = index(i);

        if (aj >= ac) aj = j;
        if (zj >= zc) zj = j;

        if (i < 6)
            break;
        else
            i = scm_page_parent(i);
    }

    if (b == 8)
    {
        if (g == 2)
        {
            if (aj < ac) r0 = ((char *) av)[aj * c] / 127.f;
            if (zj < zc) r1 = ((char *) zv)[zj * c] / 127.f;
        }
        else
        {
            if (aj < ac) r0 = ((unsigned char *) av)[aj * c] / 255.f;
            if (zj < zc) r1 = ((unsigned char *) zv)[zj * c] / 255.f;
        }
    }
    else if (b == 16)
    {
        if (g == 2)
        {
            if (aj < ac) r0 = ((short *) av)[aj * c] / 32767.f;
            if (zj < zc) r1 = ((short *) zv)[zj * c] / 32767.f;
        }
        else
        {
            if (aj < ac) r0 = ((unsigned short *) av)[aj * c] / 65535.f;
            if (zj < zc) r1 = ((unsigned short *) zv)[zj * c] / 65535.f;
        }
    }
    else if (b == 32)
    {
        if (aj < ac) r0 = ((float *) av)[aj * c];
        if (zj < zc) r1 = ((float *) zv)[zj * c];
    }
}

// Return the buffer length for a page of this file.  24-bit is padded to 32.

size_t scm_file::length() const
{
    if (c == 3 && b == 8)
        return w * h * 4 * b / 8;
    else
        return w * h * c * b / 8;
}

//------------------------------------------------------------------------------
