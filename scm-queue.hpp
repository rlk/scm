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

#ifndef SCM_QUEUE_HPP
#define SCM_QUEUE_HPP

#include <SDL.h>
#include <SDL_thread.h>

#include <set>

//------------------------------------------------------------------------------

/// An scm_queue implements a templated producer-consumer priority queue.
///
/// Priority is given by the partial ordering on the templated type.
///
/// A "needs" queue is used by the render thread to delegate work to a set of
/// loader threads. A "loads" queue is used by the loader threads to return
/// their results to the render thread. In all cases, the loader threads perform
/// blocking operations while the render thread uses non-blocking operations to
/// ensure that frames are not dropped due to data latency.
///
/// @see scm_file
/// @see scm_cache

template <typename T> class scm_queue
{
public:

    scm_queue(int n);
   ~scm_queue();

    bool try_insert(T&);
    bool try_remove(T&);

    void insert(T);
    T    remove( );

private:

    SDL_sem   *full_slots;
    SDL_sem   *free_slots;
    SDL_mutex *data_mutex;

    std::set<T> S;
};

//------------------------------------------------------------------------------

/// Create a new queue with n slots. Initialize counting semaphores for full
/// slots and empty slots, plus a mutex to protect the data.

template <typename T> scm_queue<T>::scm_queue(int n)
{
    full_slots = SDL_CreateSemaphore(0);
    free_slots = SDL_CreateSemaphore(n);
    data_mutex = SDL_CreateMutex();
}

/// Finalize a queue and release its mutex and semaphores.

template <typename T> scm_queue<T>::~scm_queue()
{
    SDL_DestroyMutex    (data_mutex);
    SDL_DestroySemaphore(free_slots);
    SDL_DestroySemaphore(full_slots);
}

//------------------------------------------------------------------------------

/// Non-blocking enqueue for use by the render thread.

template <typename T> bool scm_queue<T>::try_insert(T& d)
{
    if (SDL_SemTryWait(free_slots) == 0)
    {
        SDL_LockMutex(data_mutex);
        {
            S.insert(d);
        }
        SDL_UnlockMutex(data_mutex);
        SDL_SemPost(full_slots);
        return true;
    }
    return false;
}

/// Non-blocking dequeue for use by the render thread.

template <typename T> bool scm_queue<T>::try_remove(T& d)
{
    if (SDL_SemTryWait(full_slots) == 0)
    {
        SDL_LockMutex(data_mutex);
        {
            d   = *(S.begin());
            S.erase(S.begin());
        }
        SDL_UnlockMutex(data_mutex);
        SDL_SemPost(free_slots);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------

/// Blocking enqueue for use by the loader threads.

template <typename T> void scm_queue<T>::insert(T d)
{
    SDL_SemWait(free_slots);
    SDL_LockMutex(data_mutex);
    {
        S.insert(d);
    }
    SDL_UnlockMutex(data_mutex);
    SDL_SemPost(full_slots);
}

/// Blocking dequeue for use by the loader threads.

template <typename T> T scm_queue<T>::remove()
{
    T d;

    SDL_SemWait(full_slots);
    SDL_LockMutex(data_mutex);
    {
        d   = *(S.begin());
        S.erase(S.begin());
    }
    SDL_UnlockMutex(data_mutex);
    SDL_SemPost(free_slots);

    return d;
}

//------------------------------------------------------------------------------

#endif
