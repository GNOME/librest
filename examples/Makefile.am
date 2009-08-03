noinst_PROGRAMS = test-raw test-xml dump-xml test-flickr test-facebook get-fireeagle-location post-twitter

AM_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(GLIB_LIBS) $(SOUP_LIBS) ../rest/librest.la

test_raw_SOURCES = test-raw.c
test_xml_SOURCES = test-xml.c
get_fireeagle_location_SOURCES = get-fireeagle-location.c
dump_xml_SOURCES = dump-xml.c
test_flickr_SOURCES = test-flickr.c
test_facebook_SOURCES = test-facebook.c
post_twitter_SOURCES = post-twitter.c
