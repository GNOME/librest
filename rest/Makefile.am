lib_LTLIBRARIES = librest.la

librest_la_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) $(JSON_CFLAGS) $(XML_CFLAGS)
librest_la_LIBADD = $(GLIB_LIBS) $(SOUP_LIBS) $(JSON_CFLAGS) $(XML_LIBS)
librest_la_SOURCES = rest-proxy.c rest-xml-parser.c
librest_la_HEADERS = rest-proxy.h rest-xml-parser.h
librest_ladir = $(includedir)/rest/rest
