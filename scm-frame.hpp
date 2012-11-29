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

    void   bind(int, GLuint) const;
    void unbind(int)         const;

    void   bind_page(GLuint, int, int, int, long long) const;
    void unbind_page(GLuint, int, int)                 const;
    void  touch_page(int, int, long long);

    void    get_page_bounds(int, long long, float&, float &) const;
    bool    get_page_status(int, long long)                  const;

    double get_height(const double *) const;
    double min_height()               const;

private:

    scm_image_v images;
    scm_image  *height;
};

typedef std::vector<scm_frame *>           scm_frame_v;
typedef std::vector<scm_frame *>::iterator scm_frame_i;

//------------------------------------------------------------------------------

#endif
