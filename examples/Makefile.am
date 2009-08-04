noinst_PROGRAMS = test-raw test-xml dump-xml test-facebook get-fireeagle-location post-twitter get-flickr-favorites

AM_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(GLIB_LIBS) $(SOUP_LIBS) ../rest/librest.la

test_raw_SOURCES = test-raw.c
test_xml_SOURCES = test-xml.c
get_fireeagle_location_SOURCES = get-fireeagle-location.c
dump_xml_SOURCES = dump-xml.c
test_facebook_SOURCES = test-facebook.c
post_twitter_SOURCES = post-twitter.c
get_flickr_favorites_SOURCES = get-flickr-favorites.c
