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

#ifndef SCM_FILE_HPP
#define SCM_FILE_HPP

#include <string>
#include <tiffio.h>

//------------------------------------------------------------------------------

struct scm_file
{
public:

    scm_file(const std::string& name);
   ~scm_file();

    size_t get_scan_length()                       const;
    size_t get_page_length()                       const;
    bool   get_page_status(uint64)                 const;
    uint64 get_page_offset(uint64)                 const;
    void   get_page_bounds(uint64, float&, float&) const;
    float  get_page_sample(const double *);

    uint32 get_w() const { return w; }
    uint32 get_h() const { return h; }
    uint16 get_c() const { return c; }
    uint16 get_b() const { return b; }
    uint16 get_g() const { return g; }

    const char *get_path() const { return path.c_str(); }
    const char *get_name() const { return name.c_str(); }

    bool load_page(void *, uint64, void *);

private:

    std::string path;
    std::string name;

    uint32 w;   // Page width
    uint32 h;   // Page height
    uint16 c;   // Sample count
    uint16 b;   // Sample depth
    uint16 g;   // Sample format

    uint64 *xv; // Page indices
    uint64  xc;

    uint64 *ov; // Page offsets
    uint64  oc;

    void   *av; // Page minima
    uint64  ac;

    void   *zv; // Page maxima
    uint64  zc;

    double cache_v[3];  // Sample cache last vector
    float  cache_k;     // Sample cache last value
    void  *cache_p;     // Sample cache last page buffer
    uint64 cache_i;     // Sample cache last page index

    float  tofloat(const void *, uint64) const;
    uint64 toindex(uint64)               const;
};

//------------------------------------------------------------------------------

#endif
