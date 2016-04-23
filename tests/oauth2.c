/*
 * librest - RESTful web services access
 * Copyright (c) 2010 Intel Corporation.
 *
 * Authors: Jonathon Jongsma <jonathon.jongsma@collabora.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <rest/oauth2-proxy.h>
#include <string.h>

static void
test_url_no_fragment ()
{
  char *token = oauth2_proxy_extract_access_token ("http://example.com");

  g_assert_null (token);
}

static void
test_url_fragment_no_access_token ()
{
  char *token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#bar");

  g_assert_null (token);
}

static void
test_access_token_simple ()
{
  char *token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#access_token=1234567890_12.34561abcdefg&bar");

  g_assert_cmpstr (token, ==, "1234567890_12.34561abcdefg");
}

static void test_url_encoding_access_token ()
{
  char *token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#access_token=1234567890%5F12%2E34561abcdefg&bar");

  g_assert_cmpstr (token, ==, "1234567890_12.34561abcdefg");
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/oauth2/url-no-fragment", test_url_no_fragment);
  g_test_add_func ("/oauth2/url-fragment-no-access-token", test_url_fragment_no_access_token);
  g_test_add_func ("/oauth2/access-token-simple", test_access_token_simple);
  g_test_add_func ("/oauth2/access-token-url-encoding", test_url_encoding_access_token);

  return g_test_run ();
}
