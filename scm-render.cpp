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

#include <cmath>

#include "util3d/math3d.h"

#include "scm-render.hpp"
#include "scm-sphere.hpp"
#include "scm-scene.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

scm_render::scm_render(int w, int h) :
    width(w), height(h), do_blur(true), do_fade(true)
{
}

scm_render::~scm_render()
{
}

//------------------------------------------------------------------------------

void scm_render::render(scm_sphere *sphere,
                        scm_scene  *scene0,
                        scm_scene  *scene1,
                        const double *M, double t, int channel, int frame)
{
    sphere->draw(scene0, M, width, height, channel, frame);
}

//------------------------------------------------------------------------------
