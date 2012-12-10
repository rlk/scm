//  Copyright (C) 2005-2012 Robert Kooima
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

#ifndef VIEW_STEP_HPP
#define VIEW_STEP_HPP

#include <cstdio>
#include <string>

#include <app-file.hpp>

//------------------------------------------------------------------------------

class view_step
{
public:

    view_step();
    view_step(app::node);
    view_step(const view_step *,
              const view_step *, double);
    view_step(const view_step *,
              const view_step *,
              const view_step *,
              const view_step *, double);

    void draw() const;

    app::node serialize() const;

    void   transform_orientation(const double *);
    void   transform_position   (const double *);
    void   transform_light      (const double *);

    void   set_pitch   (double p);
    void   set_speed   (double s) { speed    = s; }
    void   set_distance(double r) { distance = r; }
    void   set_tension (double t) { tension  = t; }
    void   set_bias    (double b) { bias     = b; }
    void   set_zoom    (double z) { zoom     = z; }

    void   get_matrix  (double *, double) const;
    void   get_position(double *) const;
    void   get_up      (double *) const;
    void   get_right   (double *) const;
    void   get_forward (double *) const;
    void   get_light   (double *) const;

    double get_speed()             const { return speed;    }
    double get_distance()          const { return distance; }
    double get_tension()           const { return tension;  }
    double get_bias()              const { return bias;     }
    double get_zoom()              const { return zoom;     }
    int    get_scene()             const { return scene;    }

    const std::string& get_name()  const { return name;  }
    const std::string& get_label() const { return label; }

private:

    std::string name;
    std::string label;
    int         scene;

    double orientation[4]; // View orientation
    double position[3];    // View point location
    double light[3];       // Light location
    double speed;          // Camera speed
    double distance;       // View point distance
    double tension;        // Hermite interpolation tension
    double bias;           // Hermite interpolation bias
    double zoom;           // Magnification
};

//------------------------------------------------------------------------------

#endif
