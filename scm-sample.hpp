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

#ifndef SCM_SAMPLE_HPP
#define SCM_SAMPLE_HPP

#include <string>

#include <tiffio.h>

//------------------------------------------------------------------------------

class scm_file;

//------------------------------------------------------------------------------

class scm_sample
{
public:

    scm_sample(scm_file *);
   ~scm_sample();

    float get(const double *);

private:

    float lookup(int, int) const;

    TIFF     *tiff;
    scm_file *file;

    double  last_v[3];  // Sample cache last vector
    float   last_k;     // Sample cache last value
    uint64  last_o;     // Sample cache last page offset
    tsize_t last_i0;    // Sample cache last strip
    tsize_t last_i1;    // Sample cache last strip
    uint8  *last_p;     // Sample cache last page buffer
};

//------------------------------------------------------------------------------

#endif
