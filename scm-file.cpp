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
#include <cmath>

#include "util3d/math3d.h"
#include "scm-index.hpp"
#include "scm-cache.hpp"
#include "scm-file.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define PATH_LIST_SEP ';'
#else
#define PATH_LIST_SEP ':'
#endif

// Does the given path name an existing regular file?

static bool exists(const std::string& path)
{
    struct stat info;

    if (stat(path.c_str(), &info) == 0)
        return ((info.st_mode & S_IFMT) == S_IFREG);
    else
        return false;
}

//------------------------------------------------------------------------------

// Construct a file table entry. Open the TIFF briefly to determine its format
// and cache its meta-data.

scm_file::scm_file(const std::string& tiff) :
    needs(32),
    active(true),
    name(tiff),
    w(0), h(0), c(0), b(0),
    xv(0), xc(0),
    ov(0), oc(0),
    av(0), ac(0),
    zv(0), zc(0)
{
    cache_v[0] = 0;
    cache_v[1] = 0;
    cache_v[2] = 0;
    cache_k    = 0;
    cache_p    = 0;
    cache_T    = 0;
    cache_o    = 0;
    cache_i0   = (tsize_t) (-1);
    cache_i1   = (tsize_t) (-1);

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

    // Attempt to load the located TIFF.

    if (!path.empty())
    {
        if ((cache_T = TIFFOpen(path.c_str(), "r")))
        {
            uint64 n = 0;
            void  *p = 0;

            // Cache the image parameters.

            TIFFGetField(cache_T, TIFFTAG_IMAGEWIDTH,      &w);
            TIFFGetField(cache_T, TIFFTAG_IMAGELENGTH,     &h);
            TIFFGetField(cache_T, TIFFTAG_BITSPERSAMPLE,   &b);
            TIFFGetField(cache_T, TIFFTAG_SAMPLESPERPIXEL, &c);

            // Preload all metadata.

            if (TIFFGetField(cache_T, 0xFFB1, &n, &p))
            {
                if ((xv = (uint64 *) malloc(n * sizeof (uint64))))
                {
                    memcpy(xv, p, n * sizeof (uint64));
                    xc = n;
                }
            }
            if (TIFFGetField(cache_T, 0xFFB2, &n, &p))
            {
                if ((ov = (uint64 *) malloc(n * sizeof (uint64))))
                {
                    memcpy(ov, p, n * sizeof (uint64));
                    oc = n;
                }
            }
            if (TIFFGetField(cache_T, 0xFFB3, &n, &p))
            {
                if ((av = malloc(n * c * b / 8)))
                {
                    memcpy(av, p, n * c * b / 8);
                    ac = n;
                }
            }
            if (TIFFGetField(cache_T, 0xFFB4, &n, &p))
            {
                if ((zv = malloc(n * c * b / 8)))
                {
                    memcpy(zv, p, n * c * b / 8);
                    zc = n;
                }
            }

            // Allocate a sample cache.

            cache_p = (uint8 *) malloc(TIFFScanlineSize(cache_T) * h);
        }
    }
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

    if (cache_T) TIFFClose(cache_T);

    free(cache_p);
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
        threads.push_back(SDL_CreateThread(loader, this));
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

// Determine whether page i is given by this file.

bool scm_file::get_page_status(uint64 i) const
{
    if (toindex(i) < xc)
        return true;
    else
        return false;
}

// Seek page i in the page catalog and return its file offset.

uint64 scm_file::get_page_offset(uint64 i) const
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

void scm_file::get_page_bounds(uint64 i, float& r0, float& r1) const
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

// Sample this file along vector v using linear filtering.

float scm_file::get_page_sample(const double *v)
{
    if (v[0] != cache_v[0] || v[1] != cache_v[1] || v[2] != cache_v[2])
    {
        // Locate the face and coordinates of vector v.

        long long a;
        double    y;
        double    x;

        scm_locate(&a, &y, &x, v);

        // Assume for now that height height maps are for planets, thus inside-out.

        x = 1 - x;

        // Seek the deepest page at this location.

        long long n = 1;
        long long l = 1;
        uint64    i = a;
        uint64    j = 0;
        uint64    o = ov[a];

        while ((j = toindex(scm_page_index(a, l, int(2 * n * y),
                                                 int(2 * n * x)))) < oc)
            if (ov[j])
            {
                i = xv[j];
                o = ov[j];
                l = l + 1;
                n = n * 2;
            }
            else break;

        // Convert the root face coordinate to a local face coordinate.

        x = (x * n) - floor(x * n);
        y = (y * n) - floor(y * n);

        double r = y * (h - 2.0) + 0.5;
        double c = x * (w - 2.0) + 0.5;

        int r0 = int(floor(r)), r1 = r0 + 1;
        int c0 = int(floor(c)), c1 = c0 + 1;

        tsize_t i0 = TIFFComputeStrip(cache_T, r0, 0);
        tsize_t i1 = TIFFComputeStrip(cache_T, r1, 0);

        // If the required page is not current, set it.

        if (cache_o != o)
        {
            if (TIFFSetSubDirectory(cache_T, o))
            {
                cache_i0 = (tsize_t) -1;
                cache_i1 = (tsize_t) -1;
                cache_o  = o;
            }
        }

        // If the required data is not cached, load it.

        if (cache_o == o)
        {
            if (cache_i0 != i0 || cache_i1 != i1)
            {
                tsize_t S = TIFFStripSize(cache_T);

                if (i0 == i1)
                    TIFFReadEncodedStrip(cache_T, i0, cache_p + i0 * S, S);
                else
                {
                    TIFFReadEncodedStrip(cache_T, i0, cache_p + i0 * S, S);
                    TIFFReadEncodedStrip(cache_T, i1, cache_p + i1 * S, S);
                }
                cache_i0 = i0;
                cache_i1 = i1;
            }

            if (cache_i0 == i0 && cache_i1 == i1)
            {
                double rr = r - floor(r);
                double cc = c - floor(c);

                // Sample the cache with linear filtering.

                float s00 = tofloat(cache_p, (this->w * r0 + c0) * this->c);
                float s01 = tofloat(cache_p, (this->w * r0 + c1) * this->c);
                float s10 = tofloat(cache_p, (this->w * r1 + c0) * this->c);
                float s11 = tofloat(cache_p, (this->w * r1 + c1) * this->c);

                // Cache the request and its result.

                cache_v[0] = v[0];
                cache_v[1] = v[1];
                cache_v[2] = v[2];
                cache_k    = lerp(lerp(s00, s01, cc), lerp(s10, s11, cc), rr);
            }
        }
    }
    return cache_k;
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
    if      (b ==  8) return ((unsigned  char *) v)[i] / 255.f;
    else if (b == 16) return ((unsigned short *) v)[i] / 65535.f;
    else if (b == 32) return ((         float *) v)[i];
    else return 0.f;
}

//------------------------------------------------------------------------------

// Load the page at offset o of TIFF T. Confirm the image parameters and return
// success.

bool scm_load_page(TIFF *T, uint64 o, int w, int h, int c, int b, void *p)
{
    int i = 0;

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

            for (i = 0; i < N; ++i)
                TIFFReadEncodedStrip(T, i, (uint8 *) p + i * S, -1);
        }
    }
    return (i > 0);
}

int loader(void *data)
{
    scm_file *file = (scm_file *) data;
    scm_task  task;

    scm_log("loader thread begin %s", file->path.c_str());
    {
        TIFF *tiff = TIFFOpen(file->path.c_str(), "r");

        while ((task = file->needs.remove()).f >= 0)

            if (file->is_active())
            {
                task.load_page(tiff);
                file->cache->add_load(task);
            }
            else break;

        TIFFClose(tiff);
    }
    scm_log("loader thread end %s", file->path.c_str());
    return 0;
}

//------------------------------------------------------------------------------
