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

#ifndef SCM_STEP_HPP
#define SCM_STEP_HPP

#include <string>
#include <vector>

//------------------------------------------------------------------------------

class scm_step
{
public:

    scm_step();
    scm_step(const scm_step *,
             const scm_step *, double);
    scm_step(const scm_step *,
             const scm_step *,
             const scm_step *,
             const scm_step *, double);

    void draw_point();

    // Basic accessors

    const std::string& get_name()  const { return name;  }
    const std::string& get_scene() const { return scene; }

    void   get_orientation(double *) const;
    void   get_position   (double *) const;
    void   get_light      (double *) const;
    double get_speed      () const { return speed;    }
    double get_distance   () const { return distance; }
    double get_tension    () const { return tension;  }
    double get_bias       () const { return bias;     }
    double get_zoom       () const { return zoom;     }

    // Basic mutators

    void   set_name (const std::string& s) { name = s;  }
    void   set_scene(const std::string& s) { scene = s; }

    void   set_orientation(const double *);
    void   set_position   (const double *);
    void   set_light      (const double *);
    void   set_speed      (double s) { speed    = s; }
    void   set_distance   (double r) { distance = r; }
    void   set_tension    (double t) { tension  = t; }
    void   set_bias       (double b) { bias     = b; }
    void   set_zoom       (double z) { zoom     = z; }

    // Derived methods

    void   get_matrix     (double *) const;
    void   get_up         (double *) const;
    void   get_right      (double *) const;
    void   get_forward    (double *) const;

    void   set_pitch(double);

    void   transform_orientation(const double *);
    void   transform_position   (const double *);
    void   transform_light      (const double *);

private:

    std::string name;
    std::string scene;

    double orientation[4]; // View orientation
    double position[3];    // View point location
    double light[3];       // Light location
    double speed;          // Camera speed
    double distance;       // View point distance
    double tension;        // Hermite interpolation tension
    double bias;           // Hermite interpolation bias
    double zoom;           // Magnification
};

typedef std::vector<scm_step *>                 scm_step_v;
typedef std::vector<scm_step *>::iterator       scm_step_i;
typedef std::vector<scm_step *>::const_iterator scm_step_c;

//------------------------------------------------------------------------------

#endif
