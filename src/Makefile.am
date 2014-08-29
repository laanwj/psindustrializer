## Process this file with automake to produce Makefile.in

datadir = @datadir@/@PACKAGE@

INCLUDES = \
	-I$(top_srcdir)/intl \
	-DPACKAGE_LOCALE_DIR=\"@datadir@/locale\"

#	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = psindustrializer

psindustrializer_SOURCES = \
	main.c main.h\
	interface.c interface.h \
	callbacks.c callbacks.h \
	api-wrapper.c api-wrapper.h\
	xml-parser.c xml-parser.h

if DRIVER_ALSA
    psindustrializer_SOURCES += alsa.c alsa.h
endif

if DRIVER_ESD
    psindustrializer_SOURCES += esnd.c esnd.h
endif

AM_CFLAGS = -DPSI_DATADIR=\"$(datadir)\" -I.. -I../psphymod

#psindustrializer_LDADD = -lgthread $(top_builddir)/psphymod/libpsphymod.so
psindustrializer_LDADD = $(top_builddir)/psphymod/libpsphymod.la
