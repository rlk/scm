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

#ifndef SCM_ITEM_HPP
#define SCM_ITEM_HPP

//------------------------------------------------------------------------------

/// An scm_item is a reference to a specific page in a specific SCM file
///
/// An scm_item is valid if both the file and page indices are non-negative.
/// A partial ordering is defined that enables O(log n) searching of objects
/// derived from scm_item.

struct scm_item
{
    int       f;  ///< File index
    long long i;  ///< Page index

    /// Initialize an invalid SCM page reference.

    scm_item()                   : f(-1), i(-1) { }

    /// Initialize a valid SCM page reference.

    scm_item(int f, long long i) : f( f), i( i) { }

    /// Return true if this is a valid SCM page reference.

    bool is_valid() const { return (f >= 0 && i >= 0); }

    /// Determine the order of two SCM page references.

    bool operator<(const scm_item& that) const {
        if     (i < that.i) return true;
        if     (i > that.i) return false;
        return (f < that.f);
    }
};

//------------------------------------------------------------------------------

#endif
