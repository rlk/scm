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

#ifndef SCM_ITEM_HPP
#define SCM_ITEM_HPP

//------------------------------------------------------------------------------

struct scm_item
{
    scm_item()                   : f(-1), i(-1) { }
    scm_item(int f, long long i) : f( f), i( i) { }

    int       f;
    long long i;

    bool valid() const { return (f >= 0 && i >= 0); }

    bool operator<(const scm_item& that) const {
        if (i == that.i)
            return f < that.f;
        else
            return i < that.i;
    }
};

//------------------------------------------------------------------------------

#endif
