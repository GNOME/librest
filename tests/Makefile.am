TESTS = proxy threaded oauth oauth-async flickr

AM_CPPFLAGS = $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(SOUP_LIBS) ../rest/librest.la

check_PROGRAMS = $(TESTS)

proxy_SOURCES = proxy.c
threaded_SOURCES = threaded.c
oauth_SOURCES = oauth.c
oauth_async_SOURCES = oauth-async.c
flickr_SOURCES = flickr.c
