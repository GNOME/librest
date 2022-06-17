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

static GMainLoop *loop = NULL;

#define NUM_CHUNKS 20
#define SIZE_CHUNK 4
static guint8 server_count = 0;
static guint8 client_count = 0;
static SoupServer *server;

static gboolean
send_chunks (gpointer user_data)
{
  guint i;
  guint8 data[SIZE_CHUNK];
#ifdef WITH_SOUP_2
  SoupMessage *msg = SOUP_MESSAGE (user_data);
  SoupMessageBody *response_body = msg->response_body;
#else
  SoupServerMessage *msg = SOUP_SERVER_MESSAGE (user_data);
  SoupMessageBody *response_body = soup_server_message_get_response_body (msg);
#endif

  for (i = 0; i < SIZE_CHUNK; i++)
  {
    data[i] = server_count;
    server_count++;
  }

  soup_message_body_append (response_body, SOUP_MEMORY_COPY, data, SIZE_CHUNK);
  soup_server_unpause_message (server, msg);

  if (server_count == NUM_CHUNKS * SIZE_CHUNK)
  {
    soup_message_body_complete (response_body);
    return FALSE;
  } else {
    return TRUE;
  }
}

static void
#ifdef WITH_SOUP_2
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
#else
server_callback (SoupServer *server, SoupServerMessage *msg,
                 const char *path, GHashTable *query, gpointer user_data)
#endif
{
#ifdef WITH_SOUP_2
  SoupMessageHeaders *response_headers = msg->response_headers;
#else
  SoupMessageHeaders *response_headers = soup_server_message_get_response_headers (msg);
#endif

  g_assert_cmpstr (path, ==, "/stream");
#ifdef WITH_SOUP_2
  soup_message_set_status (msg, SOUP_STATUS_OK);
#else
  soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
#endif
  soup_message_headers_set_encoding (response_headers,
                                     SOUP_ENCODING_CHUNKED);
  soup_server_pause_message (server, msg);

  g_idle_add (send_chunks, msg);
}

static void
_call_continuous_cb (RestProxyCall *call,
                     const gchar   *buf,
                     gsize          len,
                     const GError  *error,
                     GObject       *weak_object,
                     gpointer       userdata)
{
  guint i;

  g_assert_no_error (error);
  g_assert (REST_IS_PROXY (weak_object));

  if (buf == NULL || len == 0)
    g_assert (client_count == NUM_CHUNKS * SIZE_CHUNK);

  for (i = 0; i < len; i++)
  {
    g_assert_cmpint (buf[i], ==, client_count);
    client_count++;
  }


  if (client_count == NUM_CHUNKS * SIZE_CHUNK)
    g_main_loop_quit (loop);
}

static void
stream_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error = NULL;
  gboolean result;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "stream");

  result = rest_proxy_call_continuous (call,
                                       _call_continuous_cb,
                                       (GObject *)proxy,
                                       NULL,
                                       &error);

  g_assert_no_error (error);
  g_assert (result);

  g_object_unref (call);
}

static void
continuous ()
{
  char *url;
  RestProxy *proxy;
  GError *error = NULL;
  GSList *uris;


  server = soup_server_new (NULL, NULL);
  soup_server_listen_local (server, 0, SOUP_SERVER_LISTEN_IPV4_ONLY, &error);
  g_assert_no_error (error);

  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);

  uris = soup_server_get_uris (server);
  g_assert (g_slist_length (uris) > 0);

#ifdef WITH_SOUP_2
  url = soup_uri_to_string (uris->data, FALSE);
#else
  url = g_uri_to_string (uris->data);
#endif

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new (url, FALSE);
  stream_test (proxy);
#ifdef WITH_SOUP_2
  g_slist_free_full (uris, (GDestroyNotify)soup_uri_free);
#else
  g_slist_free_full (uris, (GDestroyNotify)g_uri_unref);
#endif

  g_main_loop_run (loop);
  g_free (url);
  g_main_loop_unref (loop);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/proxy/continuous", continuous);

  return g_test_run ();
}
