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

#ifndef scm_state_HPP
#define scm_state_HPP

#include <string>
#include <list>

//------------------------------------------------------------------------------

class scm_scene;

//------------------------------------------------------------------------------

/// An scm_state defines a view configuration
///
/// A view configuration consists of the viewer's position and orientation, the
/// light source's position, and the environment being viewed. This environment
/// might be in a state of transition between two visualizations, so it consists
/// of references to scm_scenes to be applied to the foreground sphere and
/// background sphere at both the beginning and the end of a transition.

class scm_state
{
public:

    /// @name Constructors
    /// @{

    scm_state();
    scm_state(const scm_state *a);
    scm_state(const scm_state *a,
              const scm_state *b, double t);
    scm_state(const scm_state *a,
              const scm_state *b,
              const scm_state *c,
              const scm_state *d, double t);
    scm_state(const double *t,
              const double *r,
              const double *l);

    /// @}
    /// @name Basic mutators
    /// @{

    void   set_name       (const std::string&);
    void   set_foreground0(scm_scene *);
    void   set_foreground1(scm_scene *);
    void   set_background0(scm_scene *);
    void   set_background1(scm_scene *);

    void   set_orientation(const double *);
    void   set_position   (const double *);
    void   set_light      (const double *);

    void   set_distance   (double);
    void   set_zoom       (double);
    void   set_fade       (double);

    /// @}
    /// @name Basic accessors
    /// @{

    const std::string& get_name()        const { return name;        }
    scm_scene         *get_foreground0() const { return foreground0; }
    scm_scene         *get_foreground1() const { return foreground1; }
    scm_scene         *get_background0() const { return background0; }
    scm_scene         *get_background1() const { return background1; }
    double             get_distance()    const { return distance;    }
    double             get_zoom()        const { return zoom;        }
    double             get_fade()        const { return fade;        }

    void   get_orientation(double *) const;
    void   get_position   (double *) const;
    void   get_light      (double *) const;

    bool renderable() const { return foreground0
                                  || foreground1
                                  || background0
                                  || background1; }

    /// @}
    /// @name Derived methods
    /// @{

    void   get_matrix (double *) const;
    void   get_up     (double *) const;
    void   get_right  (double *) const;
    void   get_forward(double *) const;

    float  get_current_ground() const;
    float  get_minimum_ground() const;

    void   set_pitch(double);
    void   set_matrix(const double *);

    void   transform_orientation(const double *);
    void   transform_position   (const double *);
    void   transform_light      (const double *);

    /// @}

    friend double operator-(const scm_state&, const scm_state&);

private:

    std::string name;        ///< Step name

    scm_scene *foreground0;  ///< Starting foreground scene
    scm_scene *foreground1;  ///< Ending foreground scene
    scm_scene *background0;  ///< Starting background scene
    scm_scene *background1;  ///< Ending background scene

    double orientation[4];   ///< Viewer orientation
    double position[3];      ///< Viewer position vector (normalized)
    double light[3];         ///< Light position vector (normalized)
    double distance;         ///< Viewer point distance
    double zoom;             ///< Magnification
    double fade;             ///< Transition progress
};

//------------------------------------------------------------------------------

#endif
