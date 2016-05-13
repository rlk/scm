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

#include <cmath>
#include <cassert>
#include <cstdlib>

#include "GL/glew.h"
#include "util3d/math3d.h"

#include "scm-state.hpp"
#include "scm-scene.hpp"

//------------------------------------------------------------------------------

/// Initialize a new SCM viewer state using default values.

scm_state::scm_state()
{
    foreground0    = 0;
    foreground1    = 0;
    background0    = 0;
    background1    = 0;

    orientation[0] = 0.0;
    orientation[1] = 0.0;
    orientation[2] = 0.0;
    orientation[3] = 1.0;

    position[0]    = 0.0;
    position[1]    = 0.0;
    position[2]    = 1.0;

    light[0]       = 0.0;
    light[1]       = 2.0;
    light[2]       = 1.0;

    distance       = 0.0;
    zoom           = 1.0;
    fade           = 0.0;

    vnormalize(light, light);
}

/// Initialize a new SCM viewer state as a copy of the given state.

scm_state::scm_state(const scm_state *a)
{
    name        = a->name;
    foreground0 = a->foreground0;
    foreground1 = a->foreground1;
    background0 = a->background0;
    background1 = a->background1;
    distance    = a->distance;
    zoom        = a->zoom;
    fade        = a->fade;

    qcpy(orientation, a->orientation);
    vcpy(position,    a->position);
    vcpy(light,       a->light);
}

/// Initialize a new SCM viewer state using linear interpolation of given states.

scm_state::scm_state(const scm_state *a, const scm_state *b, double t)
{
    assert(a);
    assert(b);

    foreground0 = a->foreground0;
    foreground1 = b->foreground0;
    background0 = a->background0;
    background1 = b->background0;

    distance    = lerp(a->distance, b->distance, t);
    zoom        = lerp(a->zoom,     b->zoom,     t);
    fade        = lerp(a->fade,     b->fade,     t);

    qslerp(orientation, a->orientation, b->orientation, t);
    vslerp(position,    a->position,    b->position,    t);
    vslerp(light,       a->light,       b->light,       t);

    qnormalize(orientation, orientation);
    vnormalize(position,    position);
    vnormalize(light,       light);
}

/// Initialize a new SCM viewer state using the given camera configuration. position, camera
/// orientation, and lightsource orientation.
///
/// @param t Camera position (3D vector)
/// @param r Camera orientation (Euler angles)
/// @param l Light direction (Euler angles)

scm_state::scm_state(const double *t, const double *r, const double *l)
{
    assert(t);
    assert(r);
    assert(l);

    double M[16];

    foreground0 = 0;
    foreground1 = 0;
    background0 = 0;
    background1 = 0;

    qeuler(orientation, r);
    meuler(M,           l);

    vnormalize(light, M + 8);
    vnormalize(position, t);

    distance = vlen(t);
    zoom     = 1.0;
    fade     = 0.0;
}


//------------------------------------------------------------------------------

/// Return the orientation quaternion.

void scm_state::get_orientation(double *q) const
{
    qcpy(q, orientation);
}

/// Return the position vector.

void scm_state::get_position(double *v) const
{
    vcpy(v, position);
}

/// Return the light direction vector.

void scm_state::get_light(double *v) const
{
    vcpy(v, light);
}

//------------------------------------------------------------------------------

/// Set the name of the state.

void scm_state::set_name(const std::string& s)
{
    name = s;
}

/// Set the starting foreground scene.

void scm_state::set_foreground0(scm_scene *s)
{
    foreground0 = s;
}

/// Set the ending foreground scene.

void scm_state::set_foreground1(scm_scene *s)
{
    foreground1 = s;
}

/// Set the starting background scene.

void scm_state::set_background0(scm_scene *s)
{
    background0 = s;
}

/// Set the ending background scene.

void scm_state::set_background1(scm_scene *s)
{
    background1 = s;
}

/// Set the orientation quaternion.

void scm_state::set_orientation(const double *q)
{
    qnormalize(orientation, q);
}

/// Set the position vector.

void scm_state::set_position(const double *v)
{
    vnormalize(position, v);
}

/// Set the light direction vector.

void scm_state::set_light(const double *v)
{
    vnormalize(light, v);
}

/// Set the distance of the camera from the center of the sphere.

void scm_state::set_distance(double d)
{
    distance = d;
}

/// Set the camera zoom.

void scm_state::set_zoom(double z)
{
    zoom = z;
}

/// Set the transition progress.

void scm_state::set_fade(double f)
{
    fade = f;
}

//------------------------------------------------------------------------------

/// Return the view transformation matrix.

void scm_state::get_matrix(double *M) const
{
    vquaternionx(M +  0, orientation);
    vquaterniony(M +  4, orientation);
    vquaternionz(M +  8, orientation);

    vcpy(M + 12, position);

    M[13] *= distance;
    M[14] *= distance;
    M[12] *= distance;

    M[ 3] = 0.0;
    M[ 7] = 0.0;
    M[11] = 0.0;
    M[15] = 1.0;
}

/// Return the Y axis of the matrix form of the orientation quaternion, thus
/// giving the view up vector.

void scm_state::get_up(double *v) const
{
    vquaterniony(v, orientation);
}

/// Return the X axis of the matrix form of the orientation quaternion, thus
/// giving the view right vector.

void scm_state::get_right(double *v) const
{
    vquaternionx(v, orientation);
}

/// Return the negated Z axis of the matrix form of the orientation quaternion,
/// thus giving the view forward vector.

void scm_state::get_forward(double *v) const
{
    vquaternionz(v, orientation);
    vneg(v, v);
}

//------------------------------------------------------------------------------

/// Return the ground level of current scene at the given location. O(log n).
/// This may incur data access in the render thread.
///
/// @param v Vector from the center of the planet to the query position.

float scm_state::get_current_ground(const double *v) const
{
    if (foreground0 && foreground1)
        return std::max(foreground0->get_current_ground(v),
                        foreground1->get_current_ground(v));
    if (foreground0)
        return foreground0->get_current_ground(v);
    if (foreground1)
        return foreground1->get_current_ground(v);

    return 1.f;
}

/// Return the minimum ground level of the current scene, e.g. the radius of
/// the planet at the bottom of the deepest valley. O(1).

float scm_state::get_minimum_ground() const
{
    if (foreground0 && foreground1)
        return std::min(foreground0->get_minimum_ground(),
                        foreground1->get_minimum_ground());
    if (foreground0)
        return foreground0->get_minimum_ground();
    if (foreground1)
        return foreground1->get_minimum_ground();

    return 1.f;
}

//------------------------------------------------------------------------------

/// Reorient the view to the given pitch in radians

void scm_state::set_pitch(double a)
{
    double r[3];
    double p[3];
    double u[3];
    double b[3];
    double R[16];

    // Get the position and right vectors.

    vnormalize  (p, position);
    vquaternionx(r, orientation);

    // Make certain the right vector is perpendicular.

    vcrs(b, r, p);
    vnormalize(b, b);
    vcrs(r, p, b);

    // Pitch around the right vector and build a basis.

    mrotate   (R, r, a);
    vtransform(u, R, p);
    vnormalize(u, u);
    vcrs      (b, r, u);
    vnormalize(b, b);
    mbasis (R, r, u, b);

    // Convert the matrix to a new quaternion.

    qmatrix   (orientation, R);
    qnormalize(orientation, orientation);
}

/// Set the camera position and orientation using the given view matrix

void scm_state::set_matrix(const double *M)
{
    const double *p = M + 12;
    qmatrix(orientation, M);
    vnormalize(position, p);
    distance = vlen(p);
}

//------------------------------------------------------------------------------

/// Transform the current camera orientation
///
/// @param M Transformation matrix in OpenGL column-major order.

void scm_state::transform_orientation(const double *M)
{
    double A[16];
    double B[16];

    mquaternion(A, orientation);
    mmultiply(B, M, A);
    qmatrix(orientation, B);
    qnormalize(orientation, orientation);
}

/// Transform the current camera position
///
/// @param M Transformation matrix in OpenGL column-major order.

void scm_state::transform_position(const double *M)
{
    double v[3];

    vtransform(v, M, position);
    vnormalize(position, v);
}

/// Transform the current light direction
///
/// @param M Transformation matrix in OpenGL column-major order.

void scm_state::transform_light(const double *M)
{
    double v[3];

    vtransform(v, M, light);
    vnormalize(light, v);
}

//------------------------------------------------------------------------------

/// Return the linear distance between two states

double operator-(const scm_state& a, const scm_state& b)
{
    double u[3];
    double v[3];
    double w[3];

    vmul(u, a.position, a.distance);
    vmul(v, b.position, b.distance);
    vsub(w, u, v);

    return vlen(w);
}

//------------------------------------------------------------------------------
