noinst_PROGRAMS = test-raw test-json

AM_CFLAGS = $(GLIB_CFLAGS) $(SOUP_CFLAGS) $(JSON_CFLAGS) -I..
AM_LDFLAGS = $(GLIB_LIBS) $(SOUP_LIBS) $(JSON_LIBS) ../rest/librest.la

test_raw_SOURCES = test-raw.c
test_json_SOURCES = test-json.c

