## Process this file with automake to produce Makefile.in

datadir = @datadir@/@PACKAGE@

INCLUDES = \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

noinst_LIBRARIES = libpsphymod.a

psphymod_public_h_sources = $(strip \
	psmetalobj.h \
	psphymod.h \
)

psphymod_c_sources = $(strip \
	psmetalobj.c \
)

noinst_HEADERS = $(psphymod_public_h_sources)
libpsphymod_a_SOURCES = $(psphymod_c_sources)
