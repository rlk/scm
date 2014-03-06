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

void scm_log(const char *fmt, ...)
{
#ifndef WIN32
    flockfile(stderr);
#endif
    {
        va_list  ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
         fprintf(stderr, "\n");
        va_end  (ap);
    }
#ifndef WIN32
    funlockfile(stderr);
#endif
}

//------------------------------------------------------------------------------
