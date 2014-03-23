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

#include <GL/glew.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <limits>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#include "util3d/math3d.h"
#include "util3d/glsl.h"

#include "scm-sphere.hpp"
#include "scm-index.hpp"
#include "scm-log.hpp"

//------------------------------------------------------------------------------

#if 1
typedef GLushort         GLindex;
#define GL_ELEMENT_INDEX GL_UNSIGNED_SHORT
#else
typedef GLuint           GLindex;
#define GL_ELEMENT_INDEX GL_UNSIGNED_INT
#endif

//------------------------------------------------------------------------------

static inline void mid4(double *v, const double *a,
                                   const double *b,
                                   const double *c,
                                   const double *d)
{
    v[0] = a[0] + b[0] + c[0] + d[0];
    v[1] = a[1] + b[1] + c[1] + d[1];
    v[2] = a[2] + b[2] + c[2] + d[2];

    vnormalize(v, v);
}

static inline double scale(double k, double t)
{
    if (k < 1.0)
        return std::min(t / k, 1.0 - (1.0 - t) * k);
    else
        return std::max(t / k, 1.0 - (1.0 - t) * k);
}

void scm_sphere::zoom(double *w, const double *v)
{
    double d = vdot(v, zoomv);

    if (-1 < d && d < 1)
    {
        double b = scale(zoomk, acos(d) / M_PI) * M_PI;

        double x[3];

        vmad(x, v, zoomv, -d);
        vnormalize(x, x);

        vmul(w, zoomv, cos(b));
        vmad(w, w,  x, sin(b));
    }
    else vcpy(w, v);
}

//------------------------------------------------------------------------------

scm_sphere::scm_sphere(int d, int l) : detail(d), limit(l)
{
    init_arrays(d);

    zoomv[0] =  0;
    zoomv[1] =  0;
    zoomv[2] = -1;
    zoomk    =  1;
}

scm_sphere::~scm_sphere()
{
    free_arrays();
}

//------------------------------------------------------------------------------

void scm_sphere::set_detail(int d)
{
    if (0 < d && d < 256)
    {
        free_arrays( );
        detail = d;
        init_arrays(d);
    }
}

void scm_sphere::set_limit(int l)
{
    if (0 < l)
        limit = l;
}

//------------------------------------------------------------------------------

double determinant(const double *a, const double *b, const double *c)
{
    double t[3];

    vcrs(t, b, c);
    return vdot(a, t);
}

static inline double length(const double *a, const double *b, int w, int h)
{
    if (a[3] <= 0 && b[3] <= 0) return 0;
    if (a[3] <= 0)              return HUGE_VAL;
    if (b[3] <= 0)              return HUGE_VAL;

    double dx = (a[0] / a[3] - b[0] / b[3]) * w / 2;
    double dy = (a[1] / a[3] - b[1] / b[3]) * h / 2;

    return sqrt(dx * dx + dy * dy);
}

double scm_sphere::view_page(const double *M, int vw, int vh,
                             double r0, double r1, long long i, bool zoomb)
{
    double v[12];

    scm_page_corners(i, v);

    if (zoomb && zoomk != 1)
    {
        // Zoom, if necessary.

        zoom(v + 0, v + 0);
        zoom(v + 3, v + 3);
        zoom(v + 6, v + 6);
        zoom(v + 9, v + 9);

        // If zooming has stretched the page to obtuse, force a subdivision.

        if (vdot(v + 0, v + 3) < 0 ||
            vdot(v + 3, v + 9) < 0 ||
            vdot(v + 9, v + 6) < 0 ||
            vdot(v + 6, v + 0) < 0) return HUGE_VAL;

        // If zooming has popped the page inside-out, force a subdivision.

        if (determinant(v + 3, v + 0, v + 6) < 0 ||
            determinant(v + 3, v + 0, v + 9) < 0 ||

            determinant(v + 9, v + 3, v + 0) < 0 ||
            determinant(v + 9, v + 3, v + 6) < 0 ||

            determinant(v + 6, v + 9, v + 0) < 0 ||
            determinant(v + 6, v + 9, v + 3) < 0 ||

            determinant(v + 0, v + 6, v + 3) < 0 ||
            determinant(v + 0, v + 6, v + 9) < 0) return HUGE_VAL;
    }

    // Compute the maximum extent due to bulge.

    double u[3];

    u[0] = v[0] + v[3] + v[6] + v[ 9];
    u[1] = v[1] + v[4] + v[7] + v[10];
    u[2] = v[2] + v[5] + v[8] + v[11];

    double r2 = r1 * vlen(u) / vdot(v, u);

    // Apply the inner and outer radii to the bounding volume.

    double a[3], e[3], A[4], E[4];
    double b[3], f[3], B[4], F[4];
    double c[3], g[3], C[4], G[4];
    double d[3], h[3], D[4], H[4];

    vmul(a, v + 0, r0);
    vmul(b, v + 3, r0);
    vmul(c, v + 6, r0);
    vmul(d, v + 9, r0);

    vmul(e, v + 0, r2);
    vmul(f, v + 3, r2);
    vmul(g, v + 6, r2);
    vmul(h, v + 9, r2);

    // Compute W and reject any volume on the far side of the singularity.

    A[3] = M[ 3] * a[0] + M[ 7] * a[1] + M[11] * a[2] + M[15];
    B[3] = M[ 3] * b[0] + M[ 7] * b[1] + M[11] * b[2] + M[15];
    C[3] = M[ 3] * c[0] + M[ 7] * c[1] + M[11] * c[2] + M[15];
    D[3] = M[ 3] * d[0] + M[ 7] * d[1] + M[11] * d[2] + M[15];
    E[3] = M[ 3] * e[0] + M[ 7] * e[1] + M[11] * e[2] + M[15];
    F[3] = M[ 3] * f[0] + M[ 7] * f[1] + M[11] * f[2] + M[15];
    G[3] = M[ 3] * g[0] + M[ 7] * g[1] + M[11] * g[2] + M[15];
    H[3] = M[ 3] * h[0] + M[ 7] * h[1] + M[11] * h[2] + M[15];

    if (A[3] <= 0 && B[3] <= 0 && C[3] <= 0 && D[3] <= 0 &&
        E[3] <= 0 && F[3] <= 0 && G[3] <= 0 && H[3] <= 0)
        return 0;

    // Compute Z and reject using the near and far clipping planes.

    A[2] = M[ 2] * a[0] + M[ 6] * a[1] + M[10] * a[2] + M[14];
    B[2] = M[ 2] * b[0] + M[ 6] * b[1] + M[10] * b[2] + M[14];
    C[2] = M[ 2] * c[0] + M[ 6] * c[1] + M[10] * c[2] + M[14];
    D[2] = M[ 2] * d[0] + M[ 6] * d[1] + M[10] * d[2] + M[14];
    E[2] = M[ 2] * e[0] + M[ 6] * e[1] + M[10] * e[2] + M[14];
    F[2] = M[ 2] * f[0] + M[ 6] * f[1] + M[10] * f[2] + M[14];
    G[2] = M[ 2] * g[0] + M[ 6] * g[1] + M[10] * g[2] + M[14];
    H[2] = M[ 2] * h[0] + M[ 6] * h[1] + M[10] * h[2] + M[14];

    if (A[2] >  A[3] && B[2] >  B[3] && C[2] >  C[3] && D[2] >  D[3] &&
        E[2] >  E[3] && F[2] >  F[3] && G[2] >  G[3] && H[2] >  H[3])
        return 0;
    if (A[2] < -A[3] && B[2] < -B[3] && C[2] < -C[3] && D[2] < -D[3] &&
        E[2] < -E[3] && F[2] < -F[3] && G[2] < -G[3] && H[2] < -H[3])
        return 0;

    // Compute Y and reject using the bottom and top clipping planes.

    A[1] = M[ 1] * a[0] + M[ 5] * a[1] + M[ 9] * a[2] + M[13];
    B[1] = M[ 1] * b[0] + M[ 5] * b[1] + M[ 9] * b[2] + M[13];
    C[1] = M[ 1] * c[0] + M[ 5] * c[1] + M[ 9] * c[2] + M[13];
    D[1] = M[ 1] * d[0] + M[ 5] * d[1] + M[ 9] * d[2] + M[13];
    E[1] = M[ 1] * e[0] + M[ 5] * e[1] + M[ 9] * e[2] + M[13];
    F[1] = M[ 1] * f[0] + M[ 5] * f[1] + M[ 9] * f[2] + M[13];
    G[1] = M[ 1] * g[0] + M[ 5] * g[1] + M[ 9] * g[2] + M[13];
    H[1] = M[ 1] * h[0] + M[ 5] * h[1] + M[ 9] * h[2] + M[13];

    if (A[1] >  A[3] && B[1] >  B[3] && C[1] >  C[3] && D[1] >  D[3] &&
        E[1] >  E[3] && F[1] >  F[3] && G[1] >  G[3] && H[1] >  H[3])
        return 0;
    if (A[1] < -A[3] && B[1] < -B[3] && C[1] < -C[3] && D[1] < -D[3] &&
        E[1] < -E[3] && F[1] < -F[3] && G[1] < -G[3] && H[1] < -H[3])
        return 0;

    // Compute X and reject using the left and right clipping planes.

    A[0] = M[ 0] * a[0] + M[ 4] * a[1] + M[ 8] * a[2] + M[12];
    B[0] = M[ 0] * b[0] + M[ 4] * b[1] + M[ 8] * b[2] + M[12];
    C[0] = M[ 0] * c[0] + M[ 4] * c[1] + M[ 8] * c[2] + M[12];
    D[0] = M[ 0] * d[0] + M[ 4] * d[1] + M[ 8] * d[2] + M[12];
    E[0] = M[ 0] * e[0] + M[ 4] * e[1] + M[ 8] * e[2] + M[12];
    F[0] = M[ 0] * f[0] + M[ 4] * f[1] + M[ 8] * f[2] + M[12];
    G[0] = M[ 0] * g[0] + M[ 4] * g[1] + M[ 8] * g[2] + M[12];
    H[0] = M[ 0] * h[0] + M[ 4] * h[1] + M[ 8] * h[2] + M[12];

    if (A[0] >  A[3] && B[0] >  B[3] && C[0] >  C[3] && D[0] >  D[3] &&
        E[0] >  E[3] && F[0] >  F[3] && G[0] >  G[3] && H[0] >  H[3])
        return 0;
    if (A[0] < -A[3] && B[0] < -B[3] && C[0] < -C[3] && D[0] < -D[3] &&
        E[0] < -E[3] && F[0] < -F[3] && G[0] < -G[3] && H[0] < -H[3])
        return 0;

    // Compute the length of the longest visible edge, in pixels.

    return std::max(std::max(length(A, B, vw, vh),
                             length(C, D, vw, vh)),
                    std::max(length(A, C, vw, vh),
                             length(B, D, vw, vh)));
}

//------------------------------------------------------------------------------

// Add page i to the set of pages needed for this scene. Recursively traverse
// the neighborhood of this branch, adding pages to ensure that no two visibly
// adjacent pages differ by more than one level of detail.

void scm_sphere::add_page(const double *M,
                                   int width,
                                   int height,
                                double r0,
                                double r1, long long i, bool zoom)
{
    if (!is_set(i))
    {
        double k = view_page(M, width, height, r0, r1, i, zoom);

        if (k > 0)
        {
            pages.insert(i);

            if (i > 5)
            {
                long long p = scm_page_parent(i);

                add_page(M, width, height, r0, r1, p, zoom);

                switch (scm_page_order(i))
                {
                    case 0:
                        add_page(M, width, height, r0, r1, scm_page_north(p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_south(i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_east (i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_west (p), zoom);
                        break;
                    case 1:
                        add_page(M, width, height, r0, r1, scm_page_north(p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_south(i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_east (p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_west (i), zoom);
                        break;
                    case 2:
                        add_page(M, width, height, r0, r1, scm_page_north(i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_south(p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_east (i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_west (p), zoom);
                        break;
                    case 3:
                        add_page(M, width, height, r0, r1, scm_page_north(i), zoom);
                        add_page(M, width, height, r0, r1, scm_page_south(p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_east (p), zoom);
                        add_page(M, width, height, r0, r1, scm_page_west (i), zoom);
                        break;
                }
            }
        }
    }
}

bool scm_sphere::prep_page(scm_scene *scene,
                        const double *M,
                                  int width,
                                  int height,
                                  int channel, long long i, bool zoom)
{
    float t0;
    float t1;

    // If this page is missing from all data sets, skip it.

    if (scene->get_page_status(channel, i))
    {
        scene->get_page_bounds(channel, i, t0, t1);

        double r0 = double(t0);
        double r1 = double(t1);

        // Compute the on-screen pixel size of this page.

        double k = view_page(M, width, height, r0, r1, i, zoom);

        // Subdivide if too large, otherwise mark for drawing.

        if (k > 0)
        {
            if (k > limit)
            {
                long long i0 = scm_page_child(i, 0);
                long long i1 = scm_page_child(i, 1);
                long long i2 = scm_page_child(i, 2);
                long long i3 = scm_page_child(i, 3);

                bool b0 = prep_page(scene, M, width, height, channel, i0, zoom);
                bool b1 = prep_page(scene, M, width, height, channel, i1, zoom);
                bool b2 = prep_page(scene, M, width, height, channel, i2, zoom);
                bool b3 = prep_page(scene, M, width, height, channel, i3, zoom);

                if (b0 || b1 || b2 || b3)
                    return true;
            }
            add_page(M, width, height, r0, r1, i, zoom);

            return true;
        }
    }
    return false;
}

void scm_sphere::draw_page(scm_scene *scene,
                           int channel, int depth, int frame, long long i)
{
    scene->bind_page(channel, depth, frame, i);
    {
        long long i0 = scm_page_child(i, 0);
        long long i1 = scm_page_child(i, 1);
        long long i2 = scm_page_child(i, 2);
        long long i3 = scm_page_child(i, 3);

        bool b0 = is_set(i0);
        bool b1 = is_set(i1);
        bool b2 = is_set(i2);
        bool b3 = is_set(i3);

        if (b0 || b1 || b2 || b3)
        {
            // Draw any children marked for drawing.

            if (b0) draw_page(scene, channel, depth + 1, frame, i0);
            if (b1) draw_page(scene, channel, depth + 1, frame, i1);
            if (b2) draw_page(scene, channel, depth + 1, frame, i2);
            if (b3) draw_page(scene, channel, depth + 1, frame, i3);
        }
        else
        {
            // Compute the texture coordate transform for this page.

            long long r = scm_page_row(i);
            long long c = scm_page_col(i);
            long long R = r;
            long long C = c;

            for (int l = depth; l >= 0; --l)
            {
                GLfloat m = 1.0f / (1 << (depth - l));
                GLfloat x = m * c - C;
                GLfloat y = m * r - R;

                glUniform2f(scene->uA[l], m, m);
                glUniform2f(scene->uB[l], x, y);

                C /= 2;
                R /= 2;
            }

            // Select a mesh that matches up with the neighbors. Draw it.

            int j = (i < 6) ? 0 : (is_set(scm_page_north(i)) ? 0 : 1)
                                | (is_set(scm_page_south(i)) ? 0 : 2)
                                | (is_set(scm_page_west (i)) ? 0 : 4)
                                | (is_set(scm_page_east (i)) ? 0 : 8);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements[j]);
            glDrawElements(GL_QUADS, count, GL_ELEMENT_INDEX, 0);
        }
    }
    scene->unbind_page(channel, depth);
}

//------------------------------------------------------------------------------

void scm_sphere::prep(scm_scene *scene, const double *M,
                      int width, int height, int channel, bool zoom)
{
    pages.clear();

    prep_page(scene, M, width, height, channel, 0, zoom);
    prep_page(scene, M, width, height, channel, 1, zoom);
    prep_page(scene, M, width, height, channel, 2, zoom);
    prep_page(scene, M, width, height, channel, 3, zoom);
    prep_page(scene, M, width, height, channel, 4, zoom);
    prep_page(scene, M, width, height, channel, 5, zoom);
}

void scm_sphere::draw(scm_scene *scene, const double *M,
                     int width, int height, int channel, int frame)
{
    glEnable(GL_COLOR_MATERIAL);

    // Perform the visibility pre-pass.

    prep(scene, M, width, height, channel, scene->uzoomk >= 0);

    // Pre-cache all visible pages in breadth-first order.

    std::set<long long>::iterator i;

    for (i = pages.begin(); i != pages.end(); ++i)
        scene->touch_page(channel, frame, (*i));

    // Bind the vertex buffer.

    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, 0);

    // Configure the shaders and draw the six root pages.

    scene->bind(channel);
    {
        static const GLfloat M[6][9] = {
            {  0.f,  0.f,  1.f,  0.f,  1.f,  0.f, -1.f,  0.f,  0.f },
            {  0.f,  0.f, -1.f,  0.f,  1.f,  0.f,  1.f,  0.f,  0.f },
            {  1.f,  0.f,  0.f,  0.f,  0.f,  1.f,  0.f, -1.f,  0.f },
            {  1.f,  0.f,  0.f,  0.f,  0.f, -1.f,  0.f,  1.f,  0.f },
            {  1.f,  0.f,  0.f,  0.f,  1.f,  0.f,  0.f,  0.f,  1.f },
            { -1.f,  0.f,  0.f,  0.f,  1.f,  0.f,  0.f,  0.f, -1.f },
        };

        glUniform1f(scene->uzoomk, GLfloat(zoomk));
        glUniform3f(scene->uzoomv, GLfloat(zoomv[0]),
                                   GLfloat(zoomv[1]),
                                   GLfloat(zoomv[2]));

        if (is_set(0))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[0]);
            draw_page(scene, channel, 0, frame, 0);
        }
        if (is_set(1))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[1]);
            draw_page(scene, channel, 0, frame, 1);
        }
        if (is_set(2))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[2]);
            draw_page(scene, channel, 0, frame, 2);
        }
        if (is_set(3))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[3]);
            draw_page(scene, channel, 0, frame, 3);
        }
        if (is_set(4))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[4]);
            draw_page(scene, channel, 0, frame, 4);
        }
        if (is_set(5))
        {
            glUniformMatrix3fv(scene->uM, 1, GL_TRUE, M[5]);
            draw_page(scene, channel, 0, frame, 5);
        }
    }
    scene->unbind(channel);

    // Revert the local GL state.

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER,         0);
    glDisableClientState(GL_VERTEX_ARRAY);
}

//------------------------------------------------------------------------------

static void init_vertices(int n)
{
    struct vertex
    {
        GLfloat x;
        GLfloat y;
    };

    const size_t s = (n + 1) * (n + 1) * sizeof (vertex);

    if (vertex *p = (vertex *) malloc(s))
    {
        vertex *v = p;

        // Compute the position of each vertex.

        for     (int r = 0; r <= n; ++r)
            for (int c = 0; c <= n; ++c, ++v)
            {
                v->x = GLfloat(c) / GLfloat(n);
                v->y = GLfloat(r) / GLfloat(n);
            }

        // Upload the vertices to the vertex buffer.

        glBufferData(GL_ARRAY_BUFFER, s, p, GL_STATIC_DRAW);
        free(p);
    }
}

static void init_elements(int n, int b)
{
    struct element
    {
        GLindex a;
        GLindex b;
        GLindex d;
        GLindex c;
    };

    const size_t s = n * n * sizeof (element);
    const int    d = n + 1;

    if (element *p = (element *) malloc(s))
    {
        element *e = p;

        // Compute the indices for each quad.

        for     (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c, ++e)
            {
                e->a = GLindex(d * (r    ) + (c    ));
                e->b = GLindex(d * (r    ) + (c + 1));
                e->c = GLindex(d * (r + 1) + (c    ));
                e->d = GLindex(d * (r + 1) + (c + 1));
            }

        // Rewind the indices to reduce edge resolution as necessary.

        element *N = p;
        element *W = p + (n - 1);
        element *E = p;
        element *S = p + (n - 1) * n;

        for (int i = 0; i < n; ++i, N += 1, S += 1, E += n, W += n)
        {
            if (b & 1) { if (i & 1) N->a -= 1; else N->b -= 1; }
            if (b & 2) { if (i & 1) S->c += 1; else S->d += 1; }
            if (b & 4) { if (i & 1) E->a += d; else E->c += d; }
            if (b & 8) { if (i & 1) W->b -= d; else W->d -= d; }
        }

        // Upload the indices to the element buffer.

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, s, p, GL_STATIC_DRAW);
        free(p);
    }
}

void scm_sphere::init_arrays(int n)
{
    glGenBuffers(1, &vertices);
    glGenBuffers(16, elements);

    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    init_vertices(n);

    for (int b = 0; b < 16; ++b)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements[b]);
        init_elements(n, b);
    }

    count = 4 * n * n;
}

void scm_sphere::free_arrays()
{
    glDeleteBuffers(16, elements);
    glDeleteBuffers(1, &vertices);
}

//------------------------------------------------------------------------------
