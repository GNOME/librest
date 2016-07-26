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
#include "test-server.h"

const int N_THREADS = 10;

static volatile int threads_done = 0;
static const gboolean verbose = TRUE;

GMainLoop *main_loop;
SoupServer *server;

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  g_assert_cmpstr (path, ==, "/ping");

  soup_message_set_status (msg, SOUP_STATUS_OK);
  g_atomic_int_add (&threads_done, 1);

  if (threads_done == N_THREADS) {
    g_main_loop_quit (main_loop);
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

  g_assert (rest_proxy_call_sync (call, &error));
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);

  if (verbose)
    g_print ("Thread %p done\n", g_thread_self ());

  g_object_unref (call);
  g_object_unref (proxy);

  return NULL;
}


static void ping ()
{
  TestServer *server = test_server_create (server_callback);
  GThread *threads[N_THREADS];
  int i;

  main_loop = g_main_loop_new (NULL, TRUE);

  test_server_run (server);

  for (i = 0; i < N_THREADS; i++) {
    threads[i] = g_thread_new ("client thread", func, server->url);
    if (verbose)
      g_print ("Starting thread %p\n", threads[i]);
  }

  g_main_loop_run (main_loop);

  test_server_stop (server);
  g_main_loop_unref (main_loop);
}

int
main (int argc, char **argv)
{

  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/threaded/ping", ping);

  return g_test_run ();
}
