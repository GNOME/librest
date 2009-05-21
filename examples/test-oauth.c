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

  g_thread_init (NULL);
  g_type_init ();

  /* Create the proxy */
  proxy = oauth_proxy_new (
                           /* Consumer Key */
                           "NmUm6hxQ9a4u",
                           /* Consumer Secret */
                           "t4FM7LiUeD4RBwKSPa6ichKPDh5Jx4kt",
                           /* FireEagle endpoint */
                           "https://fireeagle.yahooapis.com/", FALSE);

  /* First stage authentication, this gets a request token */
  if (!oauth_proxy_auth_step (OAUTH_PROXY (proxy), "oauth/request_token", &error))
    g_error ("Cannot request token: %s", error->message);

  /* From the token construct a URL for the user to visit */
  g_print ("Go to https://fireeagle.yahoo.net/oauth/authorize?oauth_token=%s then hit any key\n",
           oauth_proxy_get_token (OAUTH_PROXY (proxy)));
  getchar ();

  /* Second stage authentication, this gets an access token */
  if (!oauth_proxy_auth_step (OAUTH_PROXY (proxy), "oauth/access_token", &error))
    g_error ("Cannot request token: %s", error->message);

  /* We're now authenticated */
  g_print ("Got access token\n");

  /* Get the user's current location */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "api/0.1/user");

  if (!rest_proxy_call_run (call, NULL, &error))
    g_error ("Cannot make call: %s", error->message);

  g_print ("%s\n", rest_proxy_call_get_payload (call));

  g_object_unref (call);

  g_object_unref (proxy);

  return 0;
}
