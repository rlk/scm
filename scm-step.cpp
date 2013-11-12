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

#include "scm-step.hpp"

//------------------------------------------------------------------------------

static double hermite(double a, double b,
                      double c, double d,
                      double t, double tension, double bias)
{
    double e = (b - a) * (1.0 + bias) * (1.0 - tension) / 2.0
             + (c - b) * (1.0 - bias) * (1.0 - tension) / 2.0;
    double f = (c - b) * (1.0 + bias) * (1.0 - tension) / 2.0
             + (d - c) * (1.0 - bias) * (1.0 - tension) / 2.0;

    double t2 = t * t;
    double t3 = t * t2;

    double x0 =  2.0 * t3 - 3.0 * t2 + 1.0;
    double x1 =        t3 - 2.0 * t2 + t;
    double x2 =        t3 -       t2;
    double x3 = -2.0 * t3 + 3.0 * t2;

    return x0 * b + x1 * e + x2 * f + x3 * c;
}

//------------------------------------------------------------------------------

// Initialize a new SCM viewer state using default values.

scm_step::scm_step()
{
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

    speed          = 1.0;
    distance       = 0.0;
    tension        = 0.0;
    bias           = 0.0;
    zoom           = 1.0;

    vnormalize(light, light);
}

// Initialize a new SCM viewer step as a copy of the given step.

scm_step::scm_step(const scm_step *a)
{
    qcpy(orientation, a->orientation);
    vcpy(position,    a->position);
    vcpy(light,       a->light);

    name       = a->name;
    foreground = a->foreground;
    background = a->background;
    speed      = a->speed;
    distance   = a->distance;
    tension    = a->tension;
    bias       = a->bias;
    zoom       = a->zoom;
}

// Initialize a new SCM viewer step using linear interpolation of given steps.

scm_step::scm_step(const scm_step *a, const scm_step *b, double t)
{
    assert(a);
    assert(b);

    qslerp(orientation, a->orientation, b->orientation, t);
    vslerp(position,    a->position,    b->position,    t);
    vslerp(light,       a->light,       b->light,       t);

    speed        = lerp(a->speed,          b->speed,          t);
    distance     = lerp(a->distance,       b->distance,       t);
    tension      = lerp(a->tension,        b->tension,        t);
    bias         = lerp(a->bias,           b->bias,           t);
    zoom         = lerp(a->zoom,           b->zoom,           t);

    qnormalize(orientation, orientation);
    vnormalize(position,    position);
    vnormalize(light,       light);
}

// Initialize a new SCM viewer step using cubic interpolation of given steps.

scm_step::scm_step(const scm_step *a,
                   const scm_step *b,
                   const scm_step *c,
                   const scm_step *d, double t)
{
    assert(a);
    assert(b);
    assert(c);
    assert(d);

    double A[4];
    double B[4];
    double C[4];
    double D[4];

    qcpy (A,    a->orientation);
    qsign(B, A, b->orientation);
    qsign(C, B, c->orientation);
    qsign(D, C, d->orientation);

    orientation[0] = hermite(A[0], B[0], C[0], D[0], t, b->tension, b->bias);
    orientation[1] = hermite(A[1], B[1], C[1], D[1], t, b->tension, b->bias);
    orientation[2] = hermite(A[2], B[2], C[2], D[2], t, b->tension, b->bias);
    orientation[3] = hermite(A[3], B[3], C[3], D[3], t, b->tension, b->bias);

    position[0]    = hermite(a->position[0],
                             b->position[0],
                             c->position[0],
                             d->position[0], t, b->tension, b->bias);
    position[1]    = hermite(a->position[1],
                             b->position[1],
                             c->position[1],
                             d->position[1], t, b->tension, b->bias);
    position[2]    = hermite(a->position[2],
                             b->position[2],
                             c->position[2],
                             d->position[2], t, b->tension, b->bias);

    light[0]       = hermite(a->light[0],
                             b->light[0],
                             c->light[0],
                             d->light[0], t, b->tension, b->bias);
    light[1]       = hermite(a->light[1],
                             b->light[1],
                             c->light[1],
                             d->light[1], t, b->tension, b->bias);
    light[2]       = hermite(a->light[2],
                             b->light[2],
                             c->light[2],
                             d->light[2], t, b->tension, b->bias);

    distance       = hermite(a->distance,
                             b->distance,
                             c->distance,
                             d->distance, t, b->tension, b->bias);

    speed          = lerp(b->speed,   c->speed,   t);
    tension        = lerp(b->tension, c->tension, t);
    bias           = lerp(b->bias,    c->bias,    t);
    zoom           = lerp(b->zoom,    c->zoom,    t);

    qnormalize(orientation, orientation);
    vnormalize(position,    position);
    vnormalize(light,       light);
}

// Initialize a new SCM viewer step using the given camera position, camera
// orientation, and lightsource orientation.

scm_step::scm_step(const double *t, const double *r, const double *l)
{
    double M[16];

    qeuler(orientation, r);
    meuler(M,           l);

    vnormalize(light, M + 8);
    vnormalize(position, t);

    distance = vlen(t);
    speed    = 1.0;
    tension  = 0.0;
    bias     = 0.0;
    zoom     = 1.0;
}

//------------------------------------------------------------------------------

void scm_step::draw()
{
    double v[3];

    get_position(v);

    v[0] *= distance;
    v[1] *= distance;
    v[2] *= distance;

    glVertex3dv(v);
}

//------------------------------------------------------------------------------

// Return the view transformation matrix.

void scm_step::get_matrix(double *M) const
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

// Return the Y axis of the matrix form of the orientation quaternion, thus
// giving the view up vector.

void scm_step::get_up(double *v) const
{
    vquaterniony(v, orientation);
}

// Return the X axis of the matrix form of the orientation quaternion, thus
// giving the view right vector.

void scm_step::get_right(double *v) const
{
    vquaternionx(v, orientation);
}

// Return the negated Z axis of the matrix form of the orientation quaternion,
// thus giving the view forward vector.

void scm_step::get_forward(double *v) const
{
    vquaternionz(v, orientation);
    vneg(v, v);
}

//------------------------------------------------------------------------------

// Return the orientation quaternion.

void scm_step::get_orientation(double *q) const
{
    qcpy(q, orientation);
}

// Return the position vector.

void scm_step::get_position(double *v) const
{
    vcpy(v, position);
}

// Return the light direction vector.

void scm_step::get_light(double *v) const
{
    vcpy(v, light);
}

//------------------------------------------------------------------------------

// Set the orientation quaternion.

void scm_step::set_orientation(const double *q)
{
    qnormalize(orientation, q);
}

// Set the position vector.

void scm_step::set_position(const double *v)
{
    vnormalize(position, v);
}

// Set the light direction vector.

void scm_step::set_light(const double *v)
{
    vnormalize(light, v);
}

//------------------------------------------------------------------------------

void scm_step::set_pitch(double a)
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

void scm_step::set_matrix(const double *M)
{
    qmatrix(orientation, M);
    vnormalize(position, M + 12);
}

//------------------------------------------------------------------------------

void scm_step::transform_orientation(const double *M)
{
    double A[16];
    double B[16];

    mquaternion(A, orientation);
    mmultiply(B, M, A);
    qmatrix(orientation, B);
    qnormalize(orientation, orientation);
}

void scm_step::transform_position(const double *M)
{
    double v[3];

    vtransform(v, M, position);
    vnormalize(position, v);
}

void scm_step::transform_light(const double *M)
{
    double v[3];

    vtransform(v, M, light);
    vnormalize(light, v);
}

//------------------------------------------------------------------------------

// The subtraction operator returns the linear distance between steps.

double operator-(const scm_step& a, const scm_step& b)
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
