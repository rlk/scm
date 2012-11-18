//  Copyright (C) 2005-2012 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation; either version 2 of the License, or (at your option) any later
//  version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.

#ifndef SCM_INDEX_HPP
#define SCM_INDEX_HPP

// The following functions compute SCM page index relationships. There is quite
// a bit of redundant computation in their implementation, and it is expected
// that the compiler will aggressively inline and reduce common sub-expressions.

// These calculations are performed using 64-bit signed indices. They're 64-bit
// because 31-bit indices have already been found in the wild, and increasing
// data set sizes are expected. The sign is useful for exception signaling.

// Calculate the integer binary log of n. --------------------------------------

static inline long long log2(long long n)
{
    unsigned long long v = (unsigned long long) n;
    unsigned long long r;
    unsigned long long s;

    r = (v > 0xFFFFFFFFULL) << 5; v >>= r;
    s = (v > 0xFFFFULL    ) << 4; v >>= s; r |= s;
    s = (v > 0xFFULL      ) << 3; v >>= s; r |= s;
    s = (v > 0xFULL       ) << 2; v >>= s; r |= s;
    s = (v > 0x3ULL       ) << 1; v >>= s; r |= s;

    return (long long) (r | (v >> 1));
}

// Calculate the number of pages in an SCM of depth d. -------------------------

static inline long long scm_page_count(long long d)
{
    return (1LL << (2 * d + 3)) - 2;
}

// Calculate the subdivision level at which page i appears. --------------------

static inline long long scm_page_level(long long i)
{
    return (log2(i + 2) - 1) / 2;
}

// Calculate the root page in the ancestry of page i. --------------------------

static inline long long scm_page_root(long long i)
{
    long long n = 1LL << (2 * scm_page_level(i));
    return (i - 2 * (n - 1)) / n;
}

// Calculate the tile number (face index) of page i. ---------------------------

static inline long long scm_page_tile(long long i)
{
    long long n = 1LL << (2 * scm_page_level(i));
    return (i - 2 * (n - 1)) % n;
}

// Calculate the tile row of page i. -------------------------------------------

static inline long long scm_page_row(long long i)
{
    return scm_page_tile(i) / (1LL << scm_page_level(i));
}

// Calculate the tile column of page i. ----------------------------------------

static inline long long scm_page_col(long long i)
{
    return scm_page_tile(i) % (1LL << scm_page_level(i));
}

// Calculate the index of the page on root a at level l, row r, column c. -----

static inline long long scm_page_index(long long a, long long l,
                                       long long r, long long c)
{
    return scm_page_count(l - 1) + (a << (2 * l)) + (r << l) + c;
}

// Calculate the parent page of page i. ----------------------------------------

static inline long long scm_page_parent(long long i)
{
    return scm_page_index(scm_page_root(i), scm_page_level(i) - 1,
                                            scm_page_row  (i) / 2,
                                            scm_page_col  (i) / 2);
}

// Calculate child page k of page i. -------------------------------------------

static inline long long scm_page_child(long long i, long long k)
{
    return scm_page_index(scm_page_root(i), scm_page_level(i) + 1,
                                            scm_page_row  (i) * 2 + k / 2,
                                            scm_page_col  (i) * 2 + k % 2);
}

// Calculate the order (child index) of page i. --------------------------------

static inline long long scm_page_order(long long i)
{
    return 2 * (scm_page_row(i) % 2)
             + (scm_page_col(i) % 2);
}

//------------------------------------------------------------------------------

void scm_vector(long long, double, double, double *);

long long scm_page_north(long long);
long long scm_page_south(long long);
long long scm_page_west (long long);
long long scm_page_east (long long);

void scm_page_corners(long long, double *);

//------------------------------------------------------------------------------

#endif
