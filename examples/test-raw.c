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

#include <rest/rest-proxy.h>
#include <unistd.h>

static void
proxy_call_async_cb (GObject      *source_object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  const gchar *payload;
  goffset len;

  payload = rest_proxy_call_get_payload (call);
  len = rest_proxy_call_get_payload_length (call);
  write (1, payload, len);
  g_main_loop_quit ((GMainLoop *)user_data);
}

gint
main (gint argc, gchar **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GMainLoop *loop;
  const gchar *payload;
  gssize len;

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new ("https://www.flickr.com/services/rest/", FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_add_params (call,
                              "method", "flickr.test.echo",
                              "api_key", "314691be2e63a4d58994b2be01faacfb",
                              "format", "json",
                              NULL);
  rest_proxy_call_invoke_async (call,
                                NULL,
                                proxy_call_async_cb,
                                loop);

  g_main_loop_run (loop);

  rest_proxy_call_sync (call, NULL);

  payload = rest_proxy_call_get_payload (call);
  len = rest_proxy_call_get_payload_length (call);
  write (1, payload, len);

  g_object_unref (call);
  g_object_unref (proxy);
}
