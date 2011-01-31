CLEANFILES =

# For some reason I can't use $(librest_@API_VERSION@_la_SOURCES) in
# test_runner_SOURCES, so we have to do this
lib_sources = \
	rest-param.c			\
	rest-params.c			\
	rest-proxy.c 			\
	rest-proxy-call.c		\
	rest-proxy-call-private.h	\
	rest-xml-node.c			\
	rest-xml-parser.c		\
	rest-main.c			\
	rest-private.h			\
	oauth-proxy.c			\
	oauth-proxy-call.c		\
	oauth-proxy-private.h 		\
	oauth2-proxy.c			\
	oauth2-proxy-call.c		\
	oauth2-proxy-private.h 		\
	sha1.c				\
	sha1.h
lib_headers = \
	rest-param.h		\
	rest-params.h		\
	rest-proxy.h		\
	rest-proxy-call.h	\
	oauth-proxy.h		\
	oauth-proxy-call.h	\
	oauth2-proxy.h		\
	oauth2-proxy-call.h	\
	rest-xml-node.h		\
	rest-xml-parser.h

lib_LTLIBRARIES = librest-@API_VERSION@.la
librest_@API_VERSION@_la_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
		    $(SOUP_CFLAGS) $(SOUP_GNOME_CFLAGS) \
		    $(XML_CFLAGS) $(GCOV_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_@API_VERSION@_la_LIBADD = $(GLIB_LIBS) $(GTHREAD_LIBS) \
                    $(SOUP_LIBS) $(SOUP_GNOME_LIBS) $(XML_LIBS) \
		    $(GCOV_LDFLAGS)
librest_@API_VERSION@_la_SOURCES = $(lib_sources) $(lib_headers)
librest_@API_VERSION@_la_HEADERS = $(lib_headers)
librest_@API_VERSION@_ladir = $(includedir)/rest-@API_VERSION@/rest


# Test suite
TESTS = test-runner
check_PROGRAMS = test-runner

test_runner_SOURCES = test-runner.c $(lib_sources) $(lib_headers)
test_runner_CFLAGS = -DBUILD_TESTS $(librest_@API_VERSION@_la_CFLAGS) $(GCOV_CFLAGS)
test_runner_LDFLAGS = $(librest_@API_VERSION@_la_LIBADD) $(GCOV_LDFLAGS)

# TODO: use gtester

# intospection
-include $(INTROSPECTION_MAKEFILE)

if HAVE_INTROSPECTION
INTROSPECTION_GIRS = Rest-@API_VERSION@.gir

Rest-@API_VERSION@.gir: librest-@API_VERSION@.la Makefile

Rest_@API_VERSION_AM@_gir_NAMESPACE = Rest
Rest_@API_VERSION_AM@_gir_VERSION = @API_VERSION@
Rest_@API_VERSION_AM@_gir_LIBS = librest-@API_VERSION@.la
Rest_@API_VERSION_AM@_gir_FILES = $(addprefix $(top_srcdir)/rest/, $(lib_headers))
Rest_@API_VERSION_AM@_gir_CFLAGS = -I$(top_srcdir)
Rest_@API_VERSION_AM@_gir_INCLUDES = GObject-2.0
Rest_@API_VERSION_AM@_gir_PACKAGES = gobject-2.0 libsoup-2.4 libxml-2.0

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION
