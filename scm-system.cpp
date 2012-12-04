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

#include "scm-system.hpp"

//------------------------------------------------------------------------------

scm_system::scm_system()
{
}

scm_system::~scm_system()
{
}

//------------------------------------------------------------------------------

// Allocate and insert a new scene before i. Return its index.

int scm_scene::add_scene(int i)
{
    if (scm_scene *scene = new scm_scene(sys))
        return int(scenes.insert(scenes.begin() + i, scene) - scenes.begin());
    else
        return -1;
}

// Delete the scene at i.

void scm_scene::del_scene(int i)
{
    delete scenes[i];
    scenes.erase(scenes.begin() + i);
}

// Return a pointer to the scene at i.

scm_scene *scm_scene::get_scene(int i)
{
    return scenes[i];
}

//------------------------------------------------------------------------------
