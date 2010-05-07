TESTS = proxy threaded oauth oauth-async oauth2 flickr lastfm

AM_CPPFLAGS = $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(SOUP_LIBS) ../rest/librest-@API_VERSION@.la ../rest-extras/librest-extras-@API_VERSION@.la

check_PROGRAMS = $(TESTS)

proxy_SOURCES = proxy.c
threaded_SOURCES = threaded.c
oauth_SOURCES = oauth.c
oauth_async_SOURCES = oauth-async.c
oauth2_SOURCES = oauth2.c
flickr_SOURCES = flickr.c
lastfm_SOURCES = lastfm.c
