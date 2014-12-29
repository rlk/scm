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
#include <cmath>

#include "util3d/math3d.h"

#include "scm-sample.hpp"
#include "scm-index.hpp"
#include "scm-file.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

/// Create a new SCM TIFF file sampler
///
/// The given scm_file object includes the path and parameters of the TIFF
/// file. Open that TIFF and prepare to make cached access to it.

scm_sample::scm_sample(scm_file *file) : file(file)
{
    last_v[0] = 0;
    last_v[1] = 0;
    last_v[2] = 0;
    last_k    = 0;
    last_p    = 0;
    last_o    = 0;
    last_i0   = (tsize_t) (-1);
    last_i1   = (tsize_t) (-1);

    if ((tiff = TIFFOpen(file->get_path(), "r")))
    {
        tsize_t N = TIFFNumberOfStrips(tiff);
        tsize_t S = TIFFStripSize     (tiff);

        last_p = (uint8 *) malloc(S * N);

        scm_log("scm_sample constructor %s", file->get_path());
    }
}

// Release the TIFF

scm_sample::~scm_sample()
{
    scm_log("scm_sample destructor");

    free(last_p);

    if (tiff) TIFFClose(tiff);
}

//------------------------------------------------------------------------------

/// Decode the raw data in the current page cache, returning a pixel value.
///
/// @param y pixel row
/// @param x pixel column

float scm_sample::lookup(int y, int x) const
{
    int w = file->get_w();
    int c = file->get_c();

    switch (file->get_b())
    {
        case  8: return ((unsigned char  *) last_p)[(w * y + x) * c] /   255.f;
        case 16: return ((unsigned short *) last_p)[(w * y + x) * c] / 65535.f;
        case 32: return ((         float *) last_p)[(w * y + x) * c];
        default: return 1.f;
    }
}

/// Perform a sample along a vector
///
/// Seek the deepest page that contains the given vector and return a linearly-
/// filtered sample of it. In the interest of performance, cache the page and
/// cache the most recent result. In the interest of minimizing latency, read
/// only a single scanline at a time.
///
/// @param v Vector from the center of the sphere to the sample point

float scm_sample::get(const double *v)
{
    if (file && tiff)
    {
        if (v[0] != last_v[0] || v[1] != last_v[1] || v[2] != last_v[2])
        {
            // Locate the face and coordinates of vector v.

            long long a;
            double    y;
            double    x;

            scm_locate(&a, &y, &x, v);
            x = 1 - x;

            // Find the deepest page covering this location.

            uint64 o = file->find_page(a, y, x);

            // Convert the root face coordinate to a local face coordinate.

            double r = y * (file->get_h() - 2.0) + 0.5;
            double c = x * (file->get_w() - 2.0) + 0.5;

            int r0 = int(floor(r)), r1 = r0 + 1;
            int c0 = int(floor(c)), c1 = c0 + 1;

            tsize_t i0 = TIFFComputeStrip(tiff, r0, 0);
            tsize_t i1 = TIFFComputeStrip(tiff, r1, 0);

            // If the required page is not current, set it.

            if (last_o != o)
            {
                if (TIFFSetSubDirectory(tiff, o))
                {
                    last_i0 = (tsize_t) -1;
                    last_i1 = (tsize_t) -1;
                    last_o  = o;
                }
            }

            // If the required data is not cached, load it.

            if (last_o == o)
            {
                if (last_i0 != i0 || last_i1 != i1)
                {
                    tsize_t S = TIFFStripSize(tiff);

                    if (i0 == i1)
                        TIFFReadEncodedStrip(tiff, i0, last_p + i0 * S, S);
                    else
                    {
                        TIFFReadEncodedStrip(tiff, i0, last_p + i0 * S, S);
                        TIFFReadEncodedStrip(tiff, i1, last_p + i1 * S, S);
                    }
                    last_i0 = i0;
                    last_i1 = i1;
                }

                if (last_i0 == i0 && last_i1 == i1)
                {
                    // Sample the cache with linear filtering.

                    float s00 = lookup(r0, c0);
                    float s01 = lookup(r0, c1);
                    float s10 = lookup(r1, c0);
                    float s11 = lookup(r1, c1);

                    double rr = r - floor(r);
                    double cc = c - floor(c);

                    // Cache the request and its result.

                    last_v[0] = v[0];
                    last_v[1] = v[1];
                    last_v[2] = v[2];
                    last_k    = float(lerp(lerp(s00, s01, cc),
                                           lerp(s10, s11, cc), rr));
                }
            }
        }
    }
    return last_k;
}

//------------------------------------------------------------------------------

