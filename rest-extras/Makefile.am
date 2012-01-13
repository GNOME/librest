CLEANFILES =

lib_sources = \
		     flickr-proxy.c	\
		     flickr-proxy-call.c	\
		     flickr-proxy-private.h \
		     lastfm-proxy.c	\
		     lastfm-proxy-call.c	\
		     lastfm-proxy-private.h \
		     youtube-proxy.c \
		     youtube-proxy-private.h \
		     rest-goa.c

lib_headers = \
		     flickr-proxy.h	\
		     flickr-proxy-call.h \
		     lastfm-proxy.h	\
		     lastfm-proxy-call.h \
		     youtube-proxy.h \
		     rest-goa.h


lib_LTLIBRARIES = librest-extras-@API_VERSION@.la
librest_extras_@API_VERSION@_la_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
		    $(SOUP_CFLAGS) $(SOUP_GNOME_CFLAGS) $(GOA_CFLAGS) \
		    $(XML_CFLAGS) $(GCOV_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_extras_@API_VERSION@_la_LIBADD = $(GLIB_LIBS) $(GTHREAD_LIBS) \
                    $(SOUP_LIBS) $(SOUP_GNOME_LIBS) $(XML_LIBS) $(GOA_LIBS) \
		    $(GCOV_LDFLAGS) \
                    $(top_builddir)/rest/librest-@API_VERSION@.la
librest_extras_@API_VERSION@_la_LDFLAGS = -no-undefined
librest_extras_@API_VERSION@_la_SOURCES = $(lib_sources) $(lib_headers)
librest_extras_@API_VERSION@_la_HEADERS = $(lib_headers)
librest_extras_@API_VERSION@_ladir = $(includedir)/rest-@API_VERSION@/rest-extras


# Test suite
TESTS = test-runner
check_PROGRAMS = test-runner

test_runner_SOURCES = test-runner.c $(lib_sources) $(lib_headers)
test_runner_CFLAGS = -DBUILD_TESTS $(librest_extras_@API_VERSION@_la_CFLAGS) $(GCOV_CFLAGS)
test_runner_LDFLAGS = $(librest_extras_@API_VERSION@_la_LIBADD) $(GCOV_LIBS)

# TODO: use gtester

# intospection
-include $(INTROSPECTION_MAKEFILE)

if HAVE_INTROSPECTION
INTROSPECTION_GIRS = RestExtras-@API_VERSION@.gir

# need to include ../rest when scanning/compiling
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_builddir)/rest --identifier-prefix=
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_builddir)/rest

RestExtras-@API_VERSION@.gir: librest-extras-@API_VERSION@.la Makefile

RestExtras_@API_VERSION_AM@_gir_NAMESPACE = RestExtras
RestExtras_@API_VERSION_AM@_gir_VERSION = @API_VERSION@
RestExtras_@API_VERSION_AM@_gir_LIBS = librest-extras-@API_VERSION@.la
RestExtras_@API_VERSION_AM@_gir_FILES = $(addprefix $(top_srcdir)/rest-extras/, $(lib_headers))
RestExtras_@API_VERSION_AM@_gir_CFLAGS = -I$(top_srcdir)
RestExtras_@API_VERSION_AM@_gir_INCLUDES = GObject-2.0 libxml2-2.0
RestExtras_@API_VERSION_AM@_gir_PACKAGES = gobject-2.0 libsoup-2.4 libxml-2.0 goa-1.0
RestExtras_@API_VERSION_AM@_gir_SCANNERFLAGS = --include-uninstalled=$(top_builddir)/rest/Rest-@API_VERSION@.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION
