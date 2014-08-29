## Process this file with automake to produce Makefile.in

datadir = @datadir@/@PACKAGE@

INCLUDES = \
	-I$(top_srcdir)/intl \
	-DPACKAGE_LOCALE_DIR=\"@datadir@/locale\" \
	$(AUDIOFILE_CFLAGS)

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

if DRIVER_PULSE
    psindustrializer_SOURCES += pulse.c pulse.h
endif

AM_CFLAGS = -DPSI_DATADIR=\"$(datadir)\" -I.. -I../psphymod

psindustrializer_LDADD = $(AUDIOFILE_LIBS) $(top_builddir)/psphymod/libpsphymod.la
