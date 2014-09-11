dnl Process this file with autoconf to produce a configure script.
AC_INIT([psindustrializer],[0.2.4])

AC_CANONICAL_HOST
    AC_CANONICAL_TARGET
       case "$target" in
           alpha*-*-linux*)
               CFLAGS="$CFLAGS -mcpu=ev4 -Wa,-mall"
               ;;
       esac

AM_INIT_AUTOMAKE([foreign])
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)

if test $GCC != yes; then
    AC_MSG_ERROR(Fatal error: Need gcc)
fi

CFLAGS="$CFLAGS -Wall"

AC_CHECK_LIB([m],[log10])

dnl test for GTK+
PSI_MODULES="gtk+-2.0 >= 2.4"
PKG_CHECK_MODULES(GTK, $PSI_MODULES, [], AC_MSG_ERROR(Fatal error: Need gtk+-2.0 >= 2.4.0))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl test for libxml2
AM_PATH_XML2(2.6.0, [], AC_MSG_ERROR(Fatal error: Need libxml2 >= 2.6.0))

LIBS="$LIBS $GTK_LIBS $GTHREAD_LIBS $XML_LIBS"
CFLAGS="$CFLAGS $GTK_CFLAGS $GTHREAD_CFLAGS $XML_CPPFLAGS"

AC_ARG_ENABLE(openGL,   [ --disable-openGL          don't use openGL [default=no]],
                have_opengl="no", have_opengl="yes")

if test x$have_opengl != xno; then
    PSI_MODULES="gtkglext-1.0 >= 1.0"
    PKG_CHECK_MODULES(GTKGL, $PSI_MODULES,
	[AC_DEFINE(HAVE_OPENGL, 1, [Set if OpenGL wanted])
	AC_SUBST(GTKGL_CFLAGS)
	AC_SUBST(GTKGL_LIBS)
	CFLAGS="$CFLAGS $GTKGL_CFLAGS"
	LIBS="$LIBS $GTKGL_LIBS"])
fi

PKG_CHECK_MODULES([AUDIOFILE], [audiofile], [], [AC_MSG_ERROR(* No sample I/O library found, fatal!)])

AC_ARG_ENABLE(pulse,
[  --disable-pulse          Disable Pulseaudio driver (default = try)],
pulse_support=$enableval)

AC_ARG_ENABLE(alsa,
[  --disable-alsa           Disable ALSA driver (default = try)],
alsa_support=$enableval)

AC_ARG_ENABLE(jack,
[  --disable-jack           Disable JACK driver (default = try)],
jack_support=$enableval)

if test x$alsa_support != xno; then
  AM_PATH_ALSA(0.9.0, have_alsa=yes, have_alsa=no)
  AC_CHECK_FUNCS(snd_pcm_plug_open)
  if test x$have_alsa = xyes; then
    AC_DEFINE([DRIVER_ALSA], 1, [Set if ALSA driver wanted])
    CFLAGS="$CFLAGS $ALSA_CFLAGS"
    LIBS="$LIBS $ALSA_LIBS"
  fi
fi

AM_CONDITIONAL(DRIVER_ALSA, test x$have_alsa = xyes)

if test x$pulse_support != xno; then
  PKG_CHECK_MODULES([PULSE], [libpulse-simple],
    [have_pulse=yes
    AC_DEFINE([DRIVER_PULSE], 1, [Set if PULSE driver wanted])
    CFLAGS="$CFLAGS $PULSE_CFLAGS"
    LIBS="$LIBS $PULSE_LIBS" ], [ have_pulse=no ])
fi

AM_CONDITIONAL(DRIVER_PULSE, test x$have_pulse = xyes)

if test x$jack_support != xno; then
  PKG_CHECK_MODULES([JACK], [jack],
    [have_jack=yes
    AC_DEFINE([DRIVER_JACK], 1, [Set if JACK driver wanted])
    CFLAGS="$CFLAGS $JACK_CFLAGS"
    LIBS="$LIBS $JACK_LIBS" ], [ have_jack=no ])
fi

AM_CONDITIONAL(DRIVER_JACK, test x$have_jack = xyes)

echo -n "Audio drivers enabled: "
if test "x$have_alsa" = "xyes"; then
    echo -n "ALSA "
fi
if test "x$have_pulse" = "xyes"; then
    echo -n "Pulseaudio "
fi
if test "x$have_jack" = "xyes"; then
    echo -n "JACK "
fi
echo

if test "x$have_alsa" != "xyes" && test "x$have_pulse" != "xyes" && test "x$have_jack" != "xyes"; then
    echo "*** No suitable sound output driver found on your system! ***"
    echo "*** You will be unable to preview the generated samples.  ***"
fi

AC_DEFINE([GETTEXT_PACKAGE],[],[Power Station Industrializer])
GETTEXT_PACKAGE=psindustrializer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE")

dnl Add the languages which your application supports here.
ALL_LINGUAS="ru"
AM_GLIB_GNU_GETTEXT

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PREFIX, "${ac_default_prefix}", [Kein Mensch weiss, warum])
else
  AC_DEFINE_UNQUOTED(PREFIX, "${prefix}", [Kein Mensch weiss, warum])
fi

AC_OUTPUT([
Makefile
psphymod/Makefile
src/Makefile
po/Makefile.in
])
