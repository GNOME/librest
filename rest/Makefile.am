lib_LTLIBRARIES = librest.la

librest_la_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) $(XML_CFLAGS) \
		    -I$(top_srcdir) -Wall -DG_LOG_DOMAIN=\"Rest\"
librest_la_LIBADD = $(GLIB_LIBS) $(SOUP_LIBS) $(XML_LIBS)
librest_la_SOURCES = rest-proxy.c 	\
		     rest-proxy-call.c	\
		     rest-xml-parser.c	\
		     rest-main.c	\
		     rest-private.h \
		     oauth-proxy.c \
		     oauth-proxy-call.c \
		     oauth-proxy-private.h \
		     $(librest_la_HEADERS)
librest_la_HEADERS = rest-proxy.h 	\
		     rest-proxy-call.h	\
		     oauth-proxy.h \
		     oauth-proxy-call.h \
		     rest-xml-parser.h
librest_ladir = $(includedir)/rest/rest
