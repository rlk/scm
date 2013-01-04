// Copyright (C) 2011-2013 Robert Kooima
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

std::string scm_path_search(const std::string& file)
{
    // If the given file name is absolute, use it.

    if (exists(file))
        return file;

    // Otherwise, search the SCM path for the file.

    if (char *val = getenv("SCMPATH"))
    {
        std::stringstream list(val);
        std::string       path;
        std::string       temp;

        while (std::getline(list, path, PATH_LIST_SEP))
        {
            temp = path + "/" + file;

            if (exists(temp))
            {
                scm_log("scm_path %s found in %s", file.c_str(), path.c_str());
                return temp;
            }
        }
    }

    // Return the empty string upon failure.

    scm_log("* scm_path %s not found", file.c_str());
    return std::string();
}

//------------------------------------------------------------------------------
