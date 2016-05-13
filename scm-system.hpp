// Copyright (C) 2011-2014 Robert Kooima
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

#ifndef SCM_SYSTEM_HPP
#define SCM_SYSTEM_HPP

#include <map>
#include <set>

#include <SDL.h>
#include <SDL_thread.h>

#include "scm-file.hpp"
#include "scm-state.hpp"
#include "scm-path.hpp"

/** @mainpage Spherical Cube Map Library

LibSCM renders high-res spherical images. Specifically, LibSCM is a C++ class
library that provides a heterogeneous data representation and rendering engine
for the interactive display of spherical data sets at scales of hundreds of
gigapixels and beyond. Applications include panoramic image display and
planetary rendering. The SCM data representation enables out-of-core data access
at real-time rates. The spherical geometry tessellator supports displacement
mapping and enables the display of planetary terrain data of arbitrary
resolution.

As an example, here is a screenshot of a interactive rendering of the Moon
synthesized from 8 gigapixels of Lunar Reconnaissance Orbiter terrain data,
textured using another 8 gigapixels of LRO surface imagery.

@image html moon.jpg

The structure of a running SCM renderer is as shown in this image. An
application generally instantiates a single scm_system object, and that object
manages the rest of the structure.

@image html scm.svg

In this figure, a box reperesents an object and a stack of boxes reperesents a
collection of objects. A solid arrow is a strong (owning) reference, and a
dotted arrow is a weak (non-owning) reference.

- Most fundamentally, an scm_system maintains a collection of scm_file objects,
  each of which provides access to an SCM TIFF image file. The scm_system also
  maintains a collection of scm_cache files that maintain the OpenGL texture
  state needed to render these images. There is exactly one scm_cache for each
  image format, e.g. 8-bit RGB or 16-bit mono. Each scm_file carries a weak
  reference to the scm_cache that handles its data.

- The scm_system also maintains a collection of scm_scene objects which allow
  visualizations to be defined. Each scm_scene consists of a GLSL shader plus a
  collection of scm_image objects that feed it, where an scm_image associates an
  scm_file with normalization parameters and GLSL shader binding information. An
  scm_scene also includes an scm_label object that represents textual
  annotations and map icons.

- The scm_system's collection of scm_state objects defines a list of points of
  interest in the defined set of scenes. Each scm_state includes a foreground
  scene reference, a background scene reference, and a camera configuration
  giving an interesting view upon these scenes. This sequence of steps allows an
  application to provide a tour of the scm_scene definitions.

- Finally, the scm_system contains two functional objects. First, the scm_render
  object manages the production of SCM renderings with dissolve transitions
  and motion blur, with the help of one or more scm_frame off-screen render
  targents. Second, the scm_sphere object which manages the adaptive generation
  of the spherical geometry to which SCM image data is applied.

The scm_cache, scm_image, and scm_scene objects each maintain a weak reference
to the scm_system that created them. By this reference they access system state,
most notably scm_file data.

*/

//------------------------------------------------------------------------------

class scm_scene;
class scm_cache;
class scm_sphere;
class scm_render;

typedef std::vector<scm_scene *>           scm_scene_v;
typedef std::vector<scm_scene *>::iterator scm_scene_i;

//------------------------------------------------------------------------------
/// @cond INTERNAL

/// An active_pair structure represents the association of an scm_file object
/// with the scm_cache that chaches its data.

struct active_pair
{
    active_pair()                          : file(0), cache(0) { }
    active_pair(scm_file *f, scm_cache *c) : file(f), cache(c) { }

    scm_file  *file;
    scm_cache *cache;
};

typedef std::map<int, active_pair> active_pair_m;

/// An active_file structure represents a reference-counted scm_file object.

struct active_file
{
    active_file() : file(0), uses(0), index(-1) { }

    scm_file  *file;
    int        uses;
    int        index;
};

typedef std::map<std::string, active_file> active_file_m;

/// An active_cache structure represents a reference-counted scm_cache object.

struct active_cache
{
    active_cache() : cache(0), uses(0) { }

    scm_cache *cache;
    int        uses;
};

/// A cache_param structure represents the format of the image data that a
/// cache contains.

struct cache_param
{
    cache_param(scm_file *file) : n(int(file->get_w()) - 2),
                                  c(int(file->get_c())),
                                  b(int(file->get_b())) { }

    int n;  // Page size
    int c;  // Channels per pixel
    int b;  // Bits per channel

    bool operator<(const cache_param& that) const {
        if      (n < that.n) return true;
        else if (n > that.n) return false;
        else if (c < that.c) return true;
        else if (c > that.c) return false;
        else if (b < that.b) return true;
        else                 return false;
    }
};

typedef std::map<cache_param, active_cache>           active_cache_m;
typedef std::map<cache_param, active_cache>::iterator active_cache_i;

/// @endcond
//------------------------------------------------------------------------------

/// An scm_system encapsulates all of the state of an SCM renderer. Its
/// interface is the primary API of the SCM rendering library.
///
/// The SCM system maintains the list of scenes and steps currently held open
/// by an application, all of the image that these scenes and steps refer to,
/// all of the caches that store the data of these images, the sphere manager
/// used to render it, and the render handler that manages this rendering.
/// A queue of steps enables the recording and playback of camera motion.

class scm_system
{
public:

    scm_system(int w, int h, int d, int l);
   ~scm_system();

    void     render_sphere(const double *, const double *, int) const;

    /// @name System queries
    /// @{

    scm_sphere *get_sphere() const;
    scm_render *get_render() const;
    scm_scene  *get_fore()   const;
    scm_scene  *get_back()   const;

    /// @}
    /// @name Scene collection handlers
    /// @{

    int         add_scene(int i);
    void        del_scene(int i);
    scm_scene  *get_scene(int i);

    int         get_scene_count() const;
    double      set_scene_blend(double);

    /// @}
    /// @name Step collection handlers
    /// @{

    int         add_step(int i);
    void        del_step(int i);
    scm_state   *get_step(int i);

    int         get_step_count()       const;
    scm_state    get_step_blend(double) const;

    /// @}
    /// @name Step queue handlers
    /// @{

    void     import_queue(const std::string&);
    void     export_queue(      std::string&);
    void     append_queue(scm_state *);
    void      flush_queue();

    /// @}
    /// @name Cache handlers
    /// @{

    void     update_cache();
    void     render_cache();
    void      flush_cache();

    void        set_synchronous(bool);
    bool        get_synchronous() const;

    /// @}
    /// @name Data queries
    /// @{

    float       get_current_ground(const double *) const;
    float       get_minimum_ground()               const;

    /// @}
    /// @name Data path handlers
    /// @{

    std::string search_path(const std::string&) const;
    void          push_path(const std::string&);
    void           pop_path();

    /// @}
    /// @name Internal Interface
    /// @{

    int     acquire_scm(const std::string&);
    int     release_scm(const std::string&);

    scm_scene *find_scene(const std::string&) const;
    scm_cache  *get_cache(int);
    scm_file   *get_file (int);

    float       get_page_sample(int f, const double *v);
    bool        get_page_status(int f, long long i);
    void        get_page_bounds(int f, long long i, float& r0, float& r1);

    /// @}

private:

    SDL_mutex     *mutex;

    scm_state_v     steps;
    scm_state_v     queue;
    scm_scene_v    scenes;

    scm_render    *render;
    scm_sphere    *sphere;
    scm_path      *path;
    scm_scene     *fore0;
    scm_scene     *fore1;
    scm_scene     *back0;
    scm_scene     *back1;

    active_file_m  files;
    active_cache_m caches;
    active_pair_m  pairs;

    int            serial;
    int            frame;
    bool           sync;
    double         fade;
};

//------------------------------------------------------------------------------

#endif
