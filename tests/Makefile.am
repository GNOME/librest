TESTS = proxy oauth flickr

AM_CPPFLAGS = $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(SOUP_LIBS) ../rest/librest.la

check_PROGRAMS = $(TESTS)

proxy_SOURCES = proxy.c
oauth_SOURCES = oauth.c
flickr_SOURCES = flickr.c
