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

#ifndef SCM_SYSTEM_HPP
#define SCM_SYSTEM_HPP

//------------------------------------------------------------------------------

class scm_system
{
public:

    scm_system();
   ~scm_system();

private:

    void       del_file (const std::string&);
    void       del_scene(const std::string&);

    scm_file  *get_file (const std::string&);
    scm_scene *get_scene(const std::string&);
    scm_model *get_model();
    scm_cache *get_cache(int, int, int);

    scm_model_p model;
    scm_scene_l scenes;
    scm_cache_s caches;
    scm_file_s  files;
    scm_path_p  path;
    scm_step    here;
};

//------------------------------------------------------------------------------

#endif
