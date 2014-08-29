Power Station Industrializer
-----------------------------

Copyright (c) 2000 David A. Bartold
Copyright (c) 2014 Wladimir J. van der Laan

This program generates synthesized percussion sounds using physical modelling.
The range of sounds possible include but is not limited to cymbol sounds,
metallic noises, bubbly sounds, and chimes.  After a sound is rendered, it
can be played and then saved to a .WAV file.

Requires:
  * Gnome desktop environment + devel files
  * Glib 1.2.7+ (earlier versions have threading bugs)
  * GtkGLArea widget (libgtkgl) + Mesa 3D or OpenGL
    GtkGLArea available at: [http://www.student.oulu.fi/~jlof/gtkglarea/]

Recommended:
  * K6 200mhz or faster

How to compile and install
---------------------------

Note: You *must* run "make install" for the About window to work.

```bash
./configure --prefix=/usr
make
make install
```

Origin
-------
The original site for this project is [on sourceforge](https://sourceforge.net/projects/industrializer/), but
it looks no longer maintained there.

License
--------

All code except that in industrial.[ch] is licensed under the GNU General
Public License version 2, or at your option, any later version.

The industrial.[ch] files are licensed under the GNU Library General
Public License version 2, or at your option, any later version.

