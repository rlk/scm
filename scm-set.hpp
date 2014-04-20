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

#ifndef SCM_SET_HPP
#define SCM_SET_HPP

#include <map>

#include "scm-item.hpp"

//------------------------------------------------------------------------------

/// An scm_page structure represents an active image page, either currently in
/// a cache or awaiting loading.

struct scm_page : public scm_item
{
    scm_page()                                 : scm_item(    ), l(-1), t(0) { }
    scm_page(int f, long long i)               : scm_item(f, i), l(-1), t(0) { }
    scm_page(int f, long long i, int l)        : scm_item(f, i), l( l), t(0) { }
    scm_page(int f, long long i, int l, int t) : scm_item(f, i), l( l), t(t) { }

    int l;  /// Cache line index
    int t;  /// Cache add time
};

//------------------------------------------------------------------------------

/// An scm_set represents an a set of active pages, either currently in
/// a cache or awaiting loading, with associated insertion time.

class scm_set
{
public:

    bool empty() const { return m.empty(); }

    scm_page search(scm_page, int);
    void     insert(scm_page, int);
    void     remove(scm_page);

    scm_page eject(int, long long);

private:

    std::map<scm_page, int> m;
};

//------------------------------------------------------------------------------

#endif
