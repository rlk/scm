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

#ifndef SCM_GUARD_HPP
#define SCM_GUARD_HPP

#include <SDL.h>
#include <SDL_thread.h>

//------------------------------------------------------------------------------

/// The scm_guard template enforces mutual exclusion on a single value of its
/// templated type.

template <typename T> class scm_guard
{
    SDL_mutex *data_mutex;
    T          data;

public:

    /// Create the mutex for this guard

    scm_guard(T d) : data(d)
    {
        data_mutex = SDL_CreateMutex();
    }

    /// Destroy this guard's mutex

    ~scm_guard()
    {
        SDL_DestroyMutex(data_mutex);
    }

    /// Atomically set the guarded value.

    void set(T d)
    {
        SDL_mutexP(data_mutex);
        data = d;
        SDL_mutexV(data_mutex);
    }

    /// Atomically get the guarded value.

    T get() const
    {
        T d;
        SDL_mutexP(data_mutex);
        d = data;
        SDL_mutexV(data_mutex);
        return d;
    }
};

//------------------------------------------------------------------------------

#endif
