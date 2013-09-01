## Process this file with automake to produce Makefile.in
# Makefile.am for The Machine Emulator host/dfb:

AUTOMAKE_OPTIONS = 1.4 gnu

AM_CPPFLAGS = -I$(top_srcdir) -I$(srcdir) -I. -D_TME_IMPL
AM_CFLAGS = $(DIRECTFB_CFLAGS)

pkglib_LTLIBRARIES = tme_host_dfb.la
tme_host_dfb_la_SOURCES = dfb-mouse.c dfb-keyboard.c dfb-screen.c dfb-display.c dfb-display.h
tme_host_dfb_la_LDFLAGS = -module -version-info 0:0:0
tme_host_dfb_la_LIBADD = $(top_builddir)/libtme/libtme.la \
	$(top_builddir)/generic/libtme-generic.la

include $(top_srcdir)/modules
