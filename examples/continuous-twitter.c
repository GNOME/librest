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

static void
_call_continous_cb (RestProxyCall *call,
                    const gchar   *buf,
                    gsize          len,
                    const GError  *error,
                    GObject       *weak_object,
                    gpointer       userdata)
{
  g_message ("%s", buf);
}

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;
  char pin[256];
  GMainLoop *loop;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  loop = g_main_loop_new (NULL, FALSE);

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
  g_object_set (proxy, "url-format", "http://stream.twitter.com/", NULL);
  rest_proxy_call_set_function (call, "1/statuses/filter.json");
  rest_proxy_call_set_method (call, "GET");
  rest_proxy_call_add_param (call, "track", "Cameron");
  rest_proxy_call_add_param (call, "delimited", "length");

  rest_proxy_call_continuous (call,
                              _call_continous_cb,
                              NULL,
                              NULL,
                              NULL);

  g_main_loop_run (loop);

  g_object_unref (call);
  g_object_unref (proxy);

  return 0;
}
