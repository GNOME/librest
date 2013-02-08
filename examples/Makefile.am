noinst_PROGRAMS = test-raw test-xml dump-xml get-fireeagle-location post-twitter post-twitter-media get-flickr-favorites lastfm-shout continuous-twitter

AM_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(GLIB_LIBS) $(GTHREAD_LIBS) $(SOUP_LIBS) ../rest/librest-@API_VERSION@.la ../rest-extras/librest-extras-@API_VERSION@.la

test_raw_SOURCES = test-raw.c
test_xml_SOURCES = test-xml.c
get_fireeagle_location_SOURCES = get-fireeagle-location.c
dump_xml_SOURCES = dump-xml.c
post_twitter_SOURCES = post-twitter.c
post_twitter_media_SOURCES = post-twitter-media.c
get_flickr_favorites_SOURCES = get-flickr-favorites.c
lastfm_shout_SOURCES = lastfm-shout.c
continuous_twitter_SOURCES = continuous-twitter.c
