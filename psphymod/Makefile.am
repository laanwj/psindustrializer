## Process this file with automake to produce Makefile.in

datadir = @datadir@/@PACKAGE@

INCLUDES = \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

lib_LTLIBRARIES = libpsphymod.la

psphymod_public_h_sources = $(strip \
	psmetalobj.h \
	psphymod.h \
)

psphymod_c_sources = $(strip \
	psmetalobj.c \
)

libpsphymodincludedir = $(includedir)/psphymod
libpsphymodinclude_HEADERS = $(psphymod_public_h_sources)
libpsphymod_la_SOURCES = $(psphymod_c_sources)
#libpsphymod_la_LIBADD = -lglib -lm
libpsphymod_la_LIBADD = -lm
