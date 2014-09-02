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

void
test_extract_token ()
{
  char *token;

  /* test a url without a fragment */
  token = oauth2_proxy_extract_access_token ("http://example.com/");
  g_assert (token == NULL);

  /* test a url with a fragment but without an access_token parameter */
  token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#bar");
  g_assert (token == NULL);

  /* test simple access_token */
  token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#access_token=1234567890_12.34561abcdefg&bar");
  g_assert (strcmp(token, "1234567890_12.34561abcdefg") == 0);

  /* test access_token with url encoding */
  token = oauth2_proxy_extract_access_token ("http://example.com/foo?foo=1#access_token=1234567890%5F12%2E34561abcdefg&bar");
  g_assert (strcmp(token, "1234567890_12.34561abcdefg") == 0);
}

int
main (int argc, char **argv)
{
#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  test_extract_token ();

  return 0;
}
