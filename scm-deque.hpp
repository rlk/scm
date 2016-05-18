// Copyright (C) 2011-2016 Robert Kooima
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

#ifndef SCM_DEQUE_HPP
#define SCM_DEQUE_HPP

#include <string>
#include <list>

#include "scm-state.hpp"

//------------------------------------------------------------------------------

/// An scm_deque defines a sequence of scm_state objects

class scm_deque
{
public:

    /// @name Constructors
    /// @{

    scm_deque();

    /// @}
    /// @name Basic accessors
    /// @{

          scm_state& front()       { return sequence.front(); }
          scm_state&  back()       { return sequence.back();  }
    const scm_state& front() const { return sequence.front(); }
    const scm_state&  back() const { return sequence.back();  }
          bool       empty() const { return sequence.empty(); }

    /// @}
    /// @name Basic mutators
    /// @{

    void push_front(const scm_state& s) { sequence.push_front(s); }
    void push_back (const scm_state& s) { sequence.push_back (s); }

    void pop_front() { sequence.pop_front(); }
    void pop_back () { sequence.pop_back (); }

    void clear() { sequence.clear(); }

private:

    std::list<scm_state> sequence; ///< Sequence of states
};

//------------------------------------------------------------------------------

#endif
