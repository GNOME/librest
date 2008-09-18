noinst_PROGRAMS = test-raw test-xml test-oauth

AM_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(GLIB_LIBS) $(SOUP_LIBS) ../rest/librest.la

test_raw_SOURCES = test-raw.c
test_xml_SOURCES = test-xml.c
test_oauth_SOURCES = test-oauth.c
