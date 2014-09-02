/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
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

#include <rest/oauth-proxy.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;
  char pin[256];

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  if (argc != 2) {
    g_printerr ("$ post-twitter \"message\"\n");
    return 1;
  }

  /* Create the proxy */
  proxy = oauth_proxy_new ("UfXFxDbUjk41scg0kmkFwA",
                           "pYQlfI2ZQ1zVK0f01dnfhFTWzizBGDnhNJIw6xwto",
                           "https://api.twitter.com/", FALSE);

  /* First stage authentication, this gets a request token */
  if (!oauth_proxy_request_token (OAUTH_PROXY (proxy), "oauth/request_token", "oob", &error))
    g_error ("Cannot get request token: %s", error->message);

  /* From the token construct a URL for the user to visit */
  g_print ("Go to http://twitter.com/oauth/authorize?oauth_token=%s then enter the PIN\n",
           oauth_proxy_get_token (OAUTH_PROXY (proxy)));

  fgets (pin, sizeof (pin), stdin);
  g_strchomp (pin);

  /* Second stage authentication, this gets an access token */
  if (!oauth_proxy_access_token (OAUTH_PROXY (proxy), "oauth/access_token", pin, &error))
    g_error ("Cannot get access token: %s", error->message);

  /* We're now authenticated */

  /* Post the status message */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "1/statuses/update.xml");
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_add_param (call, "status", argv[1]);

  if (!rest_proxy_call_sync (call, &error))
    g_error ("Cannot make call: %s", error->message);

  /* TODO: parse the XML and print something useful */
  g_print ("%s\n", rest_proxy_call_get_payload (call));

  g_object_unref (call);
  g_object_unref (proxy);

  return 0;
}
