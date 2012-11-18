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

    bool   status(uint64)                 const;
    uint64 offset(uint64)                 const;
    void   bounds(uint64, float&, float&) const;

    size_t length() const;

    uint32 get_w() const { return w; }
    uint32 get_h() const { return h; }
    uint16 get_c() const { return c; }
    uint16 get_b() const { return b; }
    uint16 get_g() const { return g; }

    const char *get_path() const { return path.c_str(); }
    const char *get_name() const { return name.c_str(); }

private:

    std::string path;
    std::string name;

    uint32 w;
    uint32 h;
    uint16 c;
    uint16 b;
    uint16 g;

    uint64 index(uint64) const;

    uint64 *xv;
    uint64  xc;

    uint64 *ov;
    uint64  oc;

    void   *av;
    uint64  ac;

    void   *zv;
    uint64  zc;
};

//------------------------------------------------------------------------------

#endif
