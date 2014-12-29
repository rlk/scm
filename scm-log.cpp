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

#include <cstdarg>
#include <cstdio>

//------------------------------------------------------------------------------

#ifdef WIN32
#include <Windows.h>

void scm_log(const char *fmt, ...)
{
    char err[1024];
    char str[1024];

    va_list  ap;
    va_start(ap, fmt);
    vsnprintf(err, 1024, fmt, ap);
    va_end  (ap);

    sprintf(str, "(SCM) %s\n", err);

    OutputDebugStringA(str);
}

#else

void scm_log(const char *fmt, ...)
{
    flockfile(stderr);
    {
        va_list  ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
         fprintf(stderr, "\n");
        va_end  (ap);
    }
    funlockfile(stderr);
}

#endif

//------------------------------------------------------------------------------

void tiff_error(const char *module, const char *fmt, va_list args)
{
    char err[1024];

    sprintf(err, "%s Error: %s", module, fmt);
    scm_log(err, args);
}

void tiff_warning(const char *module, const char *fmt, va_list args)
{
    char err[1024];

    sprintf(err, "%s Warning: %s", module, fmt);
    scm_log(err, args);
}

//------------------------------------------------------------------------------
