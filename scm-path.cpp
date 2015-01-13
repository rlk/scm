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
#include <cassert>
#include <sstream>

#include "scm-log.hpp"
#include "scm-path.hpp"

//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_LIST_SEPARATOR ';'
#else
#define PATH_SEPARATOR '/'
#define PATH_LIST_SEPARATOR ':'
#endif

// Does the given path name an existing regular file?

static bool exists(const std::string& path)
{
#ifdef _WIN32
    struct __stat64 info;

    if (_stat64(path.c_str(), &info) == 0)
        return ((info.st_mode & S_IFMT) == S_IFREG);
    else
        return false;
#else
    struct stat info;

    if (stat(path.c_str(), &info) == 0)
        return ((info.st_mode & S_IFMT) == S_IFREG);
    else
        return false;
#endif
}

//------------------------------------------------------------------------------

/// Initialize an SCM path search manager

scm_path::scm_path()
{
    if (char *val = getenv("SCMPATH"))
    {
        std::stringstream list(val);
        std::string       directory;

        while (std::getline(list, directory, PATH_LIST_SEPARATOR))
            directories.push_back(directory);
    }
}

std::string scm_path::search(const std::string& file)
{
    std::list<std::string>::iterator i;

    // If the given file name is absolute, use it.

    if (exists(file))
        return file;

    // Otherwise, search the SCM path for the file.

    for (i = directories.begin(); i != directories.end(); ++i)
    {
        std::string path = (*i) + PATH_SEPARATOR + file;

        if (exists(path))
        {
            scm_log("scm_path %s found at %s", file.c_str(), path.c_str());
            return path;
        }
    }

    // Return the empty string upon failure.

    scm_log("* scm_path \"%s\" not found", file.c_str());
    return std::string();
}

void scm_path::push(const std::string& dir)
{
    directories.push_front(dir);
    scm_log("scm_path push %s", directories.front().c_str());
}

void scm_path::pop()
{
    assert(!directories.empty());

    scm_log("scm_path pop %s", directories.front().c_str());
    directories.pop_front();
}

//------------------------------------------------------------------------------
