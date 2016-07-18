/*
 * librest - RESTful web services access
 * Copyright (c) 2009 Intel Corporation.
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
#include <libsoup/soup.h>
#include <rest/rest-proxy.h>

const int N_THREADS = 10;

static volatile int errors = 0;
static volatile int threads_done = 0;
static const gboolean verbose = FALSE;

GMainLoop *main_loop;
SoupServer *server;

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  if (g_str_equal (path, "/ping")) {
    soup_message_set_status (msg, SOUP_STATUS_OK);
    g_atomic_int_add (&threads_done, 1);

    if (threads_done == N_THREADS) {
      g_main_loop_quit (main_loop);
    }
  } else {
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
  }
}

static gpointer
func (gpointer data)
{
  RestProxy *proxy;
  RestProxyCall *call;
  const char *url = data;
  GError *error = NULL;


  proxy = rest_proxy_new (url, FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");

  if (!rest_proxy_call_sync (call, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    g_atomic_int_add (&errors, 1);
    goto done;
  }

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("Wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    g_atomic_int_add (&errors, 1);
    goto done;
  }

  if (verbose)
    g_print ("Thread %p done\n", g_thread_self ());

 done:
  g_object_unref (call);
  g_object_unref (proxy);
  return NULL;
}

int
main (int argc, char **argv)
{
  GThread *threads[N_THREADS];
  GError *error = NULL;
  char *url;
  int i;
  GSList *uris;
  GSocketAddress *address;

  server = soup_server_new (NULL);
  soup_server_listen_all (server, 0, 0, &error);

  if (error)
    {
      g_critical ("listen failed: %s", error->message);
      return -1;
    }

  soup_server_add_handler (server, "/ping", server_callback,
                           NULL, NULL);

  uris = soup_server_get_uris (server);
  g_assert (g_slist_length (uris) > 0);

  url = soup_uri_to_string (uris->data, FALSE);

  main_loop = g_main_loop_new (NULL, TRUE);

  for (i = 0; i < N_THREADS; i++) {
    threads[i] = g_thread_new ("zomg", func, url);
    if (verbose)
      g_print ("Starting thread %p\n", threads[i]);
  }

  g_main_loop_run (main_loop);

  g_free (url);
  g_slist_free_full (uris, (GDestroyNotify)soup_uri_free);
  g_object_unref (server);

  return errors != 0;
}
