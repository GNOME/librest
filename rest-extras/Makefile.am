lib_sources = \
		     flickr-proxy.c	\
		     flickr-proxy-call.c	\
		     flickr-proxy-private.h \
		     facebook-proxy.c	\
		     facebook-proxy-call.c	\
		     facebook-proxy-private.h
lib_headers = \
		     flickr-proxy.h	\
		     flickr-proxy-call.h	\
		     facebook-proxy.h	\
		     facebook-proxy-call.h


lib_LTLIBRARIES = librest-extras-@API_VERSION@.la
librest_extras_@API_VERSION@_la_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
		    $(SOUP_CFLAGS) $(SOUP_GNOME_CFLAGS) \
		    $(XML_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_extras_@API_VERSION@_la_LIBADD = $(GLIB_LIBS) $(GTHREAD_LIBS) \
                    $(SOUP_LIBS) $(SOUP_GNOME_LIBS) $(XML_LIBS) \
                    $(top_builddir)/rest/librest-@API_VERSION@.la
librest_extras_@API_VERSION@_la_SOURCES = $(lib_sources) $(lib_headers)
librest_extras_@API_VERSION@_la_HEADERS = $(lib_headers)
librest_extras_@API_VERSION@_ladir = $(includedir)/rest-@API_VERSION@/rest-extras


# Test suite
TESTS = test-runner
check_PROGRAMS = test-runner

test_runner_SOURCES = test-runner.c $(lib_sources) $(lib_headers)
test_runner_CFLAGS = -DBUILD_TESTS $(librest_extras_@API_VERSION@_la_CFLAGS)
test_runner_LDFLAGS = $(librest_extras_@API_VERSION@_la_LIBADD)

# TODO: use gtester
