CLEANFILES =

lib_sources = \
		     flickr-proxy.c	\
		     flickr-proxy-call.c	\
		     flickr-proxy-private.h
lib_headers = \
		     flickr-proxy.h	\
		     flickr-proxy-call.h


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

if HAVE_INTROSPECTION
BUILT_GIRSOURCES =

RestExtras-@API_VERSION@.gir: $(INTROSPECTION_SCANNER) librest-extras-@API_VERSION@.la
	$(AM_V_GEN)$(INTROSPECTION_SCANNER) -v \
		--namespace RestExtras --nsversion=@API_VERSION@ \
		--add-include-path=$(top_builddir)/rest \
		--include=GObject-2.0 \
		--include=Soup-2.4 \
		--include=libxml2-2.0 \
		--include=Rest-@API_VERSION@ \
		--library=librest-extras-@API_VERSION@.la \
		--libtool="$(top_builddir)/libtool" \
		--pkg gobject-2.0 \
		--pkg libsoup-2.4 \
		--pkg libxml-2.0 \
		--output $@ \
		-I$(top_srcdir) \
		$(addprefix $(top_srcdir)/rest-extras/, $(lib_headers)) \
		$(addprefix $(top_srcdir)/rest-extras/, $(lib_sources))

BUILT_GIRSOURCES += RestExtras-@API_VERSION@.gir

# INTROSPECTION_GIRDIR/INTROSPECTION_TYPELIBDIR aren't the right place to
# install anything - we need to install inside our prefix.
girdir = $(datadir)/gir-1.0
gir_DATA = $(BUILT_GIRSOURCES)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(BUILT_GIRSOURCES:.gir=.typelib)

%.typelib: %.gir $(INTROSPECTION_COMPILER)
	$(AM_V_GEN) \
	LD_LIBRARY_PATH=.libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
	$(INTROSPECTION_COMPILER) \
		--includedir=$(srcdir) \
		--includedir=. \
		--includedir=$(top_builddir)/rest \
		$(INTROSPECTION_COMPILER_OPTS) $< -o $(@F)

CLEANFILES += $(BUILT_GIRSOURCES) $(typelibs_DATA)

endif #HAVE_INTROSPECTION
