CLEANFILES =

# For some reason I can't use $(librest_@API_VERSION@_la_SOURCES) in
# test_runner_SOURCES, so we have to do this
lib_sources = \
	glib-compat.h			\
	glib-compat.c			\
	rest-param.c			\
	rest-params.c			\
	rest-proxy.c 			\
	rest-proxy-auth.c		\
	rest-proxy-auth-private.h	\
	rest-proxy-call.c		\
	rest-proxy-call-private.h	\
	rest-xml-node.c			\
	rest-xml-parser.c		\
	rest-main.c			\
	rest-private.h			\
	rest-enum-types.c		\
	oauth-proxy.c			\
	oauth-proxy-call.c		\
	oauth-proxy-private.h 		\
	oauth2-proxy.c			\
	oauth2-proxy-call.c		\
	oauth2-proxy-private.h 		\
	sha1.c				\
	sha1.h
nodist_lib_sources = \
	rest-marshal.h		\
	rest-marshal.c
lib_headers = \
	rest-param.h		\
	rest-params.h		\
	rest-proxy.h		\
	rest-proxy-auth.h	\
	rest-proxy-call.h	\
	rest-enum-types.h	\
	oauth-proxy.h		\
	oauth-proxy-call.h	\
	oauth2-proxy.h		\
	oauth2-proxy-call.h	\
	rest-xml-node.h		\
	rest-xml-parser.h

EXTRA_DIST = \
	rest-marshal.txt

lib_LTLIBRARIES = librest-@API_VERSION@.la
librest_@API_VERSION@_la_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) \
		    $(SOUP_CFLAGS) $(SOUP_GNOME_CFLAGS) \
		    $(XML_CFLAGS) $(GCOV_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_@API_VERSION@_la_LDFLAGS = -no-undefined
librest_@API_VERSION@_la_LIBADD = $(GLIB_LIBS) $(GTHREAD_LIBS) \
                    $(SOUP_LIBS) $(SOUP_GNOME_LIBS) $(XML_LIBS) \
		    $(GCOV_LDFLAGS)
librest_@API_VERSION@_la_SOURCES = $(lib_sources) $(lib_headers)
nodist_librest_@API_VERSION@_la_SOURCES = $(nodist_lib_sources)
librest_@API_VERSION@_la_HEADERS = $(lib_headers)
librest_@API_VERSION@_ladir = $(includedir)/rest-@API_VERSION@/rest

rest-enum-types.h: stamp-rest-enum-types.h
	@true

stamp-rest-enum-types.h: rest-proxy.h rest-proxy-call.h Makefile
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __REST_ENUM_TYPES_H__\n#define __REST_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define REST_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __REST_ENUM_TYPES_H__ */" rest-proxy.h rest-proxy-call.h) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh rest-enum-types.h || cp xgen-gtbh rest-enum-types.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)

rest-enum-types.c: rest-proxy.h rest-proxy-call.h Makefile rest-enum-types.h
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"rest-proxy.h\"\n#include \"rest-proxy-call.h\"\n#include \"rest-enum-types.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		rest-proxy.h rest-proxy-call.h) > xgen-gtbc \
	&& cp xgen-gtbc rest-enum-types.c  \
	&& rm -f xgen-gtbc

BUILT_SOURCES = rest-marshal.c rest-marshal.h

rest-marshal.c: rest-marshal.txt
	$(AM_V_GEN)echo "#include \"rest-marshal.h\"" > $@ && \
		glib-genmarshal --body $< >> $@ || (rm -f $@ && exit 1)

rest-marshal.h: rest-marshal.txt
	$(AM_V_GEN)glib-genmarshal --header $< > $@ || (rm -f $@ && exit 1)


# Test suite
TESTS = test-runner
check_PROGRAMS = test-runner

test_runner_SOURCES = test-runner.c $(lib_sources) $(nodist_lib_sources) $(lib_headers)
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
Rest_@API_VERSION_AM@_gir_FILES = \
	$(lib_headers) \
	$(filter-out %private.h, $(lib_sources))
Rest_@API_VERSION_AM@_gir_CFLAGS = -I$(top_srcdir)
Rest_@API_VERSION_AM@_gir_INCLUDES = GObject-2.0 Gio-2.0 Soup-2.4
Rest_@API_VERSION_AM@_gir_PACKAGES = gobject-2.0 libsoup-2.4 libxml-2.0 gio-2.0
Rest_@API_VERSION_AM@_gir_SCANNERFLAGS = --accept-unprefixed --warn-all
Rest_@API_VERSION_AM@_gir_EXPORT_PACKAGES = rest-@API_VERSION@
REST_CINCLUDES=$(patsubst %,--c-include='rest/%',$(shell echo $(lib_headers)))
INTROSPECTION_SCANNER_ARGS = $(REST_CINCLUDES)

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) \
	$(typelibs_DATA) \
	rest-enum-types.h \
	rest-enum-types.c \
	stamp-rest-enum-types.h \
	$(NULL)

endif # HAVE_INTROSPECTION
