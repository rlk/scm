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

#ifndef SCM_FRAME_HPP
#define SCM_FRAME_HPP

#include <vector>

#include "scm-image.hpp"

//------------------------------------------------------------------------------

class scm_frame
{
public:

    scm_frame();

    void add_image(scm_image *);

    void bind(GLuint) const;
    void free()       const;

    void set_texture(GLuint, int, int, long long) const;
    void clr_texture(GLuint, int)                 const;

    void   page_bounds(long long, float&, float &) const;
    bool   page_status(long long)                  const;
    void   page_touch (long long, int);

    float get_r0() const;
    float get_r1() const;

    void set_channel(int c) { channel = c; }

private:

    scm_image_v images;
    scm_image  *height;

    int channel;
};

typedef std::vector<scm_frame *>           scm_frame_v;
typedef std::vector<scm_frame *>::iterator scm_frame_i;

//------------------------------------------------------------------------------

#endif
