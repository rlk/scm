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

#ifndef SCM_SET_HPP
#define SCM_SET_HPP

#include <map>

#include "scm-item.hpp"

//------------------------------------------------------------------------------

struct scm_page : public scm_item
{
    scm_page()                                 : scm_item(    ), l(-1), t(0) { }
    scm_page(int f, long long i)               : scm_item(f, i), l(-1), t(0) { }
    scm_page(int f, long long i, int l)        : scm_item(f, i), l( l), t(0) { }
    scm_page(int f, long long i, int l, int t) : scm_item(f, i), l( l), t(t) { }

    int l;
    int t;
};

//------------------------------------------------------------------------------

class scm_set
{
public:

    // int  count() const { return int(m.size()); }
    // bool full()  const { return (count() >= size); }
    bool empty() const { return m.empty(); }

    scm_page search(scm_page, int);
    void     insert(scm_page, int);
    void     remove(scm_page);

    scm_page eject(int, long long);
    void     draw();

private:

    std::map<scm_page, int> m;
};

//------------------------------------------------------------------------------

#endif
