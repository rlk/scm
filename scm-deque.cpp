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

#include <cmath>
#include <sstream>
#include <iomanip>

#include "util3d/math3d.h"

#include "scm-deque.hpp"

//------------------------------------------------------------------------------

/// Create a new, empty state deque.

scm_deque::scm_deque()
{
}

//------------------------------------------------------------------------------

/// Parse the given string as a series of camera states. Enqueue each. This
/// function ingests Maya MOV exports.

void scm_deque::import_mov(const std::string& data)
{
    std::stringstream file(data);
    std::string       line;

    sequence.clear();

    while (std::getline(file, line))
    {
        std::stringstream in(line);

        double t[3] = { 0, 0, 0 };
        double r[3] = { 0, 0, 0 };
        double l[3] = { 0, 0, 0 };

        if (in) in >> t[0] >> t[1] >> t[2];
        if (in) in >> r[0] >> r[1] >> r[2];
        if (in) in >> l[0] >> l[1] >> l[2];

        r[0] = radians(r[0]);
        r[1] = radians(r[1]);
        r[2] = radians(r[2]);

        l[0] = radians(l[0]);
        l[1] = radians(l[1]);
        l[2] = radians(l[2]);

        scm_state s(t, r, l);

        push_back(s);
    }
}

/// Print all steps on the current queue to the given string using the same
/// format expected by import.

void scm_deque::export_mov(std::string& data)
{
	std::list<scm_state>::iterator it;

    std::stringstream file;

    file << std::setprecision(std::numeric_limits<long double>::digits10);

    for (it = sequence.begin(); it != sequence.end(); ++it)
    {
        double d = it->get_distance();
        double p[3];
        double q[4];
        double r[3];

        it->get_position(p);
        it->get_orientation(q);

        p[0] *= d;
        p[1] *= d;
        p[2] *= d;

        equaternion(r, q);

        file << p[0] << " "
             << p[1] << " "
             << p[2] << " "
             << degrees(r[0]) << " "
             << degrees(r[1]) << " "
             << degrees(r[2]) << " "
             << "0.0 0.0 0.0" << std::endl;
    }
    data = file.str();
}


//------------------------------------------------------------------------------

