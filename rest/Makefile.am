CLEANFILES =

# For some reason I can't use $(librest_@API_VERSION@_la_SOURCES) in
# test_runner_SOURCES, so we have to do this
lib_sources = \
	rest-proxy.c 			\
	rest-proxy-call.c		\
	rest-proxy-call-private.h	\
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
lib_headers = rest-proxy.h 	\
	rest-proxy-call.h	\
	oauth-proxy.h		\
	oauth-proxy-call.h	\
	oauth2-proxy.h		\
	oauth2-proxy-call.h	\
	rest-xml-parser.h

lib_LTLIBRARIES = librest-@API_VERSION@.la
librest_@API_VERSION@_la_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
		    $(SOUP_CFLAGS) $(SOUP_GNOME_CFLAGS) \
		    $(XML_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_@API_VERSION@_la_LIBADD = $(GLIB_LIBS) $(GTHREAD_LIBS) \
                    $(SOUP_LIBS) $(SOUP_GNOME_LIBS) $(XML_LIBS)
librest_@API_VERSION@_la_SOURCES = $(lib_sources) $(lib_headers)
librest_@API_VERSION@_la_HEADERS = $(lib_headers)
librest_@API_VERSION@_ladir = $(includedir)/rest-@API_VERSION@/rest


# Test suite
TESTS = test-runner
check_PROGRAMS = test-runner

test_runner_SOURCES = test-runner.c $(lib_sources) $(lib_headers)
test_runner_CFLAGS = -DBUILD_TESTS $(librest_@API_VERSION@_la_CFLAGS)
test_runner_LDFLAGS = $(librest_@API_VERSION@_la_LIBADD)

# TODO: use gtester

if HAVE_INTROSPECTION
BUILT_GIRSOURCES =

Rest-@API_VERSION@.gir: $(INTROSPECTION_SCANNER) librest-@API_VERSION@.la
	$(AM_V_GEN)$(INTROSPECTION_SCANNER) -v \
		--namespace Rest --nsversion=@API_VERSION@ \
		--include=GObject-2.0 \
		--include=Soup-2.4 \
		--include=libxml2-2.0 \
		--library=librest-@API_VERSION@.la \
		--libtool="$(top_builddir)/libtool" \
		--pkg gobject-2.0 \
		--pkg libsoup-2.4 \
		--pkg libxml-2.0 \
		--output $@ \
		-I$(top_srcdir) \
		$(addprefix $(top_srcdir)/rest/, $(lib_headers)) \
		$(addprefix $(top_srcdir)/rest/, $(lib_sources))

BUILT_GIRSOURCES += Rest-@API_VERSION@.gir

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
		$(INTROSPECTION_COMPILER_OPTS) $< -o $(@F)

CLEANFILES += $(BUILT_GIRSOURCES) $(typelibs_DATA)

endif #HAVE_INTROSPECTION
