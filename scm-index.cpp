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

#include <cmath>

#include "scm-index.hpp"

// Calculate the vector toward (x, y) on root face a. --------------------------

void scm_vector(long long a, double y, double x, double *v)
{
    const double s = x * M_PI_2 - M_PI_4;
    const double t = y * M_PI_2 - M_PI_4;

    double u[3];

    u[0] =  sin(s) * cos(t);
    u[1] = -cos(s) * sin(t);
    u[2] =  cos(s) * cos(t);

    double k = 1.0 / sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);

    u[0] *= k;
    u[1] *= k;
    u[2] *= k;

    switch (a)
    {
        case 0: v[0] =  u[2]; v[1] =  u[1]; v[2] = -u[0]; break;
        case 1: v[0] = -u[2]; v[1] =  u[1]; v[2] =  u[0]; break;
        case 2: v[0] =  u[0]; v[1] =  u[2]; v[2] = -u[1]; break;
        case 3: v[0] =  u[0]; v[1] = -u[2]; v[2] =  u[1]; break;
        case 4: v[0] =  u[0]; v[1] =  u[1]; v[2] =  u[2]; break;
        case 5: v[0] = -u[0]; v[1] =  u[1]; v[2] = -u[2]; break;
    }
}

// Determine the page to the north of page i. ----------------------------------

long long scm_page_north(long long i)
{
    long long l = scm_page_level(i);
    long long a = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - c;

    if      (r >  0) {        r = r - 1;    }
    else if (a == 0) { a = 2; r = t; c = m; }
    else if (a == 1) { a = 2; r = c; c = 0; }
    else if (a == 2) { a = 5; r = 0; c = t; }
    else if (a == 3) { a = 4; r = m;        }
    else if (a == 4) { a = 2; r = m;        }
    else             { a = 2; r = 0; c = t; }

    return scm_page_index(a, l, r, c);
}

// Determine the page to the south of page i. ----------------------------------

long long scm_page_south(long long i)
{
    long long l = scm_page_level(i);
    long long a = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - c;

    if      (r <  m) {        r = r + 1;    }
    else if (a == 0) { a = 3; r = c; c = m; }
    else if (a == 1) { a = 3; r = t; c = 0; }
    else if (a == 2) { a = 4; r = 0;        }
    else if (a == 3) { a = 5; r = m; c = t; }
    else if (a == 4) { a = 3; r = 0;        }
    else             { a = 3; r = m; c = t; }

    return scm_page_index(a, l, r, c);
}

// Determine the page to the west of page i. -----------------------------------

long long scm_page_west(long long i)
{
    long long l = scm_page_level(i);
    long long a = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - r;

    if      (c >  0) {        c = c - 1;    }
    else if (a == 0) { a = 4; c = m;        }
    else if (a == 1) { a = 5; c = m;        }
    else if (a == 2) { a = 1; c = r; r = 0; }
    else if (a == 3) { a = 1; c = t; r = m; }
    else if (a == 4) { a = 1; c = m;        }
    else             { a = 0; c = m;        }

    return scm_page_index(a, l, r, c);
}

// Determine the page to the east of page i. -----------------------------------

long long scm_page_east(long long i)
{
    long long l = scm_page_level(i);
    long long a = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - r;

    if      (c <  m) {        c = c + 1;    }
    else if (a == 0) { a = 5; c = 0;        }
    else if (a == 1) { a = 4; c = 0;        }
    else if (a == 2) { a = 0; c = t; r = 0; }
    else if (a == 3) { a = 0; c = r; r = m; }
    else if (a == 4) { a = 0; c = 0;        }
    else             { a = 1; c = 0;        }

    return scm_page_index(a, l, r, c);
}

// Calculate the four corner vectors of page i. --------------------------------

void scm_page_corners(long long i, double *v)
{
    long long l = scm_page_level(i);
    long long a = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l;

    scm_vector(a, (double) (r + 0) / n, (double) (c + 0) / n, v + 0);
    scm_vector(a, (double) (r + 0) / n, (double) (c + 1) / n, v + 3);
    scm_vector(a, (double) (r + 1) / n, (double) (c + 0) / n, v + 6);
    scm_vector(a, (double) (r + 1) / n, (double) (c + 1) / n, v + 9);
}

//------------------------------------------------------------------------------
