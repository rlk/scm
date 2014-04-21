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

/// An scm_step defines a view configuration
///
/// A view configuration consists of the name of an scm_scene to be applied to
/// the foreground sphere, the name of an scm_scene to be applied to the back-
/// ground sphere, and the position and orientation of the viewer.
///
/// This mechanism is intended to permit the creation of scripted paths among
/// points of interest through a series of visualizations. As such, the scm_step
/// object includes parameters that ture a Hermitian interpolation of view
/// configurations, which is where the name "step" comes from.

class scm_step
{
public:

    /// @name Constructors
    /// @{

    scm_step();
    scm_step(const scm_step *a);
    scm_step(const scm_step *a,
             const scm_step *b, double t);
    scm_step(const scm_step *a,
             const scm_step *b,
             const scm_step *c,
             const scm_step *d, double t);
    scm_step(const double *t,
             const double *r,
             const double *l);

    /// @}
    /// @name Basic mutators
    /// @{

    void   set_name       (const std::string& s);
    void   set_foreground (const std::string& s);
    void   set_background (const std::string& s);

    void   set_orientation(const double *);
    void   set_position   (const double *);
    void   set_light      (const double *);

    void   set_speed      (double s);
    void   set_distance   (double r);
    void   set_tension    (double t);
    void   set_bias       (double b);
    void   set_zoom       (double z);

    /// @}
    /// @name Basic accessors
    /// @{

    const std::string& get_name()       const { return name;       }
    const std::string& get_foreground() const { return foreground; }
    const std::string& get_background() const { return background; }

    void   get_orientation(double *) const;
    void   get_position   (double *) const;
    void   get_light      (double *) const;

    double get_speed()    const { return speed;    }
    double get_distance() const { return distance; }
    double get_tension()  const { return tension;  }
    double get_bias()     const { return bias;     }
    double get_zoom()     const { return zoom;     }

    /// @}
    /// @name Derived methods
    /// @{

    void   get_matrix     (double *) const;
    void   get_up         (double *) const;
    void   get_right      (double *) const;
    void   get_forward    (double *) const;

    void   set_pitch(double);
    void   set_matrix(const double *);

    void   transform_orientation(const double *);
    void   transform_position   (const double *);
    void   transform_light      (const double *);

    /// @}

    friend double operator-(const scm_step&, const scm_step&);

private:

    std::string name;        /// Step name
    std::string foreground;  /// Foreground scene name
    std::string background;  /// Background scene name

    double orientation[4];   /// View orientation
    double position[3];      /// View point location
    double light[3];         /// Light location
    double speed;            /// Camera speed
    double distance;         /// View point distance
    double tension;          /// Hermite interpolation tension
    double bias;             /// Hermite interpolation bias
    double zoom;             /// Magnification
};

typedef std::vector<scm_step *>                 scm_step_v;
typedef std::vector<scm_step *>::iterator       scm_step_i;
typedef std::vector<scm_step *>::const_iterator scm_step_c;

//------------------------------------------------------------------------------

#endif
