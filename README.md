# Spherical Cube Map Rendering

Copyright &copy; 2011-2014 [Robert Kooima](http://kooima.net)

SCM is a C++ class library that implements a rendering engine and non-homogeneous data representation for the interactive display of spherical data sets at scales of hundreds of gigapixels and beyond. Applications include panoramic image display and planetary rendering. The SCM data representation enables out-of-core data access at real-time rates. The spherical geometry tessellator supports displacement mapping and enables the display of planetary terrain data of arbitrary resolution.

## Documentation

- [Complete Documentation](http://rlk.github.io/scm/)

Here are a few YouTube videos of this renderer in action:

- [Lunar rendering with discussion](http://www.youtube.com/watch?v=OPJDxEkmjJo)
- [Lunar rendering demonstration](http://www.youtube.com/watch?v=Km9_RMPdwR8)
- [Stereoscopic panorama rendering with discussion](http://www.youtube.com/watch?v=5dTpLCXRCfA)
- [Stereoscopic panoramic rendering in virtual reality](http://www.youtube.com/watch?v=0Gi2qZltdtc)

## Build

This module has a submodule that must be initialized after a new clone:

	git submodule update --init

### Linux and OS X

To build `Release/libscm.a` under Linux or OS X:

	make

To build `Debug/libscm.a`:

	make DEBUG=1

### Windows

To build `Release\scm.lib` under Windows, use the Visual Studio project or the included `Makefile.vc`:

	nmake /f Makefile.vc

To build `Debug\scm.lib`:

	nmake /f Makefile.vc DEBUG=1

## Dependencies

Dependencies for SCM include

- Freetype2
- SDL2

The SCM repo has a submodule ([util3d](https://github.com/rlk/util3d)) that must be explicitly added to a fresh clone:

    git submodule update --init

