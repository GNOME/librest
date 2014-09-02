/*
 * librest - RESTful web services access
 * Copyright (c) 2010 Intel Corporation.
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

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy.h>

static int errors = 0;
static GMainLoop *loop = NULL;

#define NUM_CHUNKS 20
static int server_count = 0;
static gint client_count = 0;
static SoupServer *server;

static gboolean
send_chunks (gpointer user_data)
{
  SoupMessage *msg = SOUP_MESSAGE (user_data);
  char *s;
  SoupBuffer *buf;

  s = g_strdup_printf ("%d %d %d %d\n",
                       server_count, server_count,
                       server_count, server_count);
  buf = soup_buffer_new (SOUP_MEMORY_TAKE, s, strlen (s));

  soup_message_body_append_buffer (msg->response_body, buf);
  soup_server_unpause_message (server, msg);

  if (++server_count == NUM_CHUNKS)
  {
    soup_message_body_complete (msg->response_body);
    return FALSE;
  } else {
    return TRUE;
  }
}

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  if (g_str_equal (path, "/stream")) {
    soup_message_set_status (msg, SOUP_STATUS_OK);
    soup_message_headers_set_encoding (msg->response_headers,
                                       SOUP_ENCODING_CHUNKED);
    soup_server_pause_message (server, msg);

    server_count = 1;
    g_idle_add (send_chunks, msg);
  }
}

static void
_call_continuous_cb (RestProxyCall *call,
                     const gchar   *buf,
                     gsize          len,
                     const GError  *error,
                     GObject       *weak_object,
                     gpointer       userdata)
{
  gint a = 0, b = 0, c = 0, d = 0;

  if (error)
  {
    g_printerr ("Error: %s\n", error->message);
    errors++;
    goto out;
  }

  if (!REST_IS_PROXY (weak_object))
  {
    g_printerr ("weak object not as expected\n");
    errors++;
    goto out;
  }

  if (buf == NULL && len == 0)
  {
    if (client_count != NUM_CHUNKS)
    {
      g_printerr ("stream ended prematurely\n");
      errors++;
    }
    goto out;
  }

  if (sscanf (buf, "%d %d %d %d\n", &a, &b, &c, &d) != 4)
  {
    g_printerr ("stream data not formatted as expected\n");
    errors++;
    goto out;
  }

  if (a != client_count ||
      b != client_count ||
      c != client_count ||
      d != client_count)
  {
    g_printerr ("stream data not as expected (got %d %d %d %d, expected %d)\n",
                a, b, c, d, client_count);
    errors++;
    goto out;
  }

  client_count++;
  return;
out:
  g_main_loop_quit (loop);
  return;
}

static void
stream_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error;

  client_count = 1;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "stream");

  if (!rest_proxy_call_continuous (call,
                                   _call_continuous_cb,
                                   (GObject *)proxy,
                                   NULL,
                                   &error))
  {
    g_printerr ("Making stream failed: %s", error->message);
    g_error_free (error);
    errors++;
    return;
  }

  g_object_unref (call);
}

int
main (int argc, char **argv)
{
  char *url;
  RestProxy *proxy;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif
  loop = g_main_loop_new (NULL, FALSE);

  server = soup_server_new (NULL);
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  soup_server_run_async (server);

  url = g_strdup_printf ("http://127.0.0.1:%d/", soup_server_get_port (server));
  proxy = rest_proxy_new (url, FALSE);
  g_free (url);

  stream_test (proxy);
  g_main_loop_run (loop);

  return errors != 0;
}
