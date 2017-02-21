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
#include "test-server.h"

#define NUM_CHUNKS 20
#define SIZE_CHUNK 4
#define UPLOAD_SUCCESSFUL "Upload successful"

static guint8 server_count = 0;
static guint8 client_count = 0;

typedef struct {
  TestServer  *test_server;
  SoupMessage *msg;
} ServerData;

static gboolean
send_chunks (gpointer user_data)
{
  ServerData *server_data = user_data;
  guint i;
  guint8 data[SIZE_CHUNK];

  for (i = 0; i < SIZE_CHUNK; i++)
  {
    data[i] = server_count;
    server_count++;
  }

  soup_message_body_append (server_data->msg->response_body, SOUP_MEMORY_COPY, data, SIZE_CHUNK);
  soup_server_unpause_message (server_data->test_server->server, server_data->msg);

  if (server_count == NUM_CHUNKS * SIZE_CHUNK) {
    soup_message_body_complete (server_data->msg->response_body);
    return G_SOURCE_REMOVE;
  } else {
    return G_SOURCE_CONTINUE;
  }
}

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  TestServer *test_server = user_data;
  GSource *source;
  ServerData *server_data;

  g_message ("Path: %s", path);

  if (strcmp (path, "/stream") == 0)
    {
      g_assert_cmpstr (path, ==, "/stream");
      soup_message_set_status (msg, SOUP_STATUS_OK);
      soup_message_headers_set_encoding (msg->response_headers,
                                         SOUP_ENCODING_CHUNKED);
      soup_server_pause_message (server, msg);

      server_data = g_malloc (sizeof (ServerData));
      server_data->test_server = test_server;
      server_data->msg = msg;
      /* The server is running in its own thread with its own GMainContext,
       * so schedule the sending there */
      source = g_idle_source_new ();
      g_source_set_callback (source, send_chunks, server_data, g_free);
      g_source_attach (source, g_main_loop_get_context (test_server->main_loop));
      g_source_unref (source);
    }
  else if (strcmp (path, "/upload") == 0)
    {
      soup_message_set_status (msg, SOUP_STATUS_OK);
      soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
                                 UPLOAD_SUCCESSFUL, strlen (UPLOAD_SUCCESSFUL));
    }
  else
    {
      soup_message_set_status (msg, SOUP_STATUS_MALFORMED);
    }

}

static void
_call_continuous_cb (RestProxyCall *call,
                     const gchar   *buf,
                     gsize          len,
                     const GError  *error,
                     gpointer       userdata)
{
  guint i;

  g_assert_no_error (error);

  if (buf == NULL && len == 0)
    g_assert (client_count == NUM_CHUNKS * SIZE_CHUNK);

  for (i = 0; i < len; i++)
  {
    g_assert_cmpint (buf[i], ==, client_count);
    client_count++;
  }
}

static void
call_done_cb (GObject      *source_object,
              GAsyncResult *result,
              gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  GMainLoop *loop = user_data;
  GError *error = NULL;

  rest_proxy_call_continuous_finish (call, result, &error);
  g_assert_no_error (error);

  g_assert (client_count == NUM_CHUNKS * SIZE_CHUNK);

  g_main_loop_quit (loop);
}

static void
continuous ()
{
  TestServer *test_server = test_server_create (server_callback);
  RestProxy *proxy;
  RestProxyCall *call;
  GMainLoop *loop;

  test_server_run (test_server);

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new (test_server->url, FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "stream");
  rest_proxy_call_continuous (call,
                              _call_continuous_cb,
                              NULL,
                              call_done_cb,
                              loop);
  g_main_loop_run (loop);

  test_server_stop (test_server);
  g_main_loop_unref (loop);
  g_object_unref (call);
  g_object_unref (proxy);
}

static void
upload_callback (RestProxyCall *call,
                 gsize          total,
                 gsize          uploaded,
                 const GError  *error,
                 gpointer       user_data)
{
  g_assert (REST_IS_PROXY_CALL (call));
  g_assert (total < uploaded);
  g_assert_no_error (error);
}

static void
upload_done_cb (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  GMainLoop *loop = user_data;
  GError *error = NULL;

  rest_proxy_call_upload_finish (call, result, &error);
  g_assert_no_error (error);

  g_assert_nonnull (rest_proxy_call_get_payload (call));
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, strlen (UPLOAD_SUCCESSFUL));

  g_main_loop_quit (loop);
}

static void
upload ()
{
  TestServer *test_server = test_server_create (server_callback);
  RestProxy *proxy;
  RestProxyCall *call;
  GMainLoop *loop;

  test_server_run (test_server);

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new (test_server->url, FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "upload");
  rest_proxy_call_upload (call,
                          upload_callback,
                          NULL,
                          upload_done_cb,
                          loop);

  g_main_loop_run (loop);

  test_server_stop (test_server);
  g_main_loop_unref (loop);
  g_object_unref (call);
  g_object_unref (proxy);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/proxy/continuous", continuous);
  g_test_add_func ("/proxy/upload", upload);

  return g_test_run ();
}
