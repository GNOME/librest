/*
 * librest - RESTful web services access
 * Copyright (c) 2009 Intel Corporation.
 *
 * Authors: Timm Bäder <mail@baedert.org>
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
#include "test-server.h"

TestServer *
test_server_create (SoupServerCallback server_cb)
{
  TestServer *test_server;
  GMainLoop *main_loop;
  GMainContext *context;
  SoupServer *server;
  GError *error = NULL;
  GSList *uris;
  char *url;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);
  main_loop = g_main_loop_new (context, FALSE);
  server = soup_server_new (NULL);

  soup_server_listen_local (server, 0, 0, &error);
  g_assert_no_error (error);

  soup_server_add_handler (server, "/", server_cb,
                           NULL, NULL);

  uris = soup_server_get_uris (server);
  g_assert (g_slist_length (uris) > 0);
  url = soup_uri_to_string (uris->data, FALSE);

  test_server = g_slice_alloc (sizeof (TestServer));
  test_server->server = server;
  test_server->main_loop = main_loop;
  test_server->url = url;
  test_server->thread = NULL;

  g_slist_free_full (uris, (GDestroyNotify)soup_uri_free);

  g_main_context_pop_thread_default (context);

  return test_server;
}

static void *
test_server_thread_func (gpointer data)
{
  TestServer *server = data;

  g_main_context_push_thread_default (g_main_loop_get_context (server->main_loop));
  g_main_loop_run (server->main_loop);
  g_main_context_pop_thread_default (g_main_loop_get_context (server->main_loop));

  return NULL;
}

void
test_server_run (TestServer *server)
{
  server->thread = g_thread_new ("Server Thread", test_server_thread_func, server);
}

void
test_server_stop (TestServer *server)
{
  g_assert (server->thread);
  g_main_loop_quit (server->main_loop);
  g_thread_join (server->thread);
  g_object_unref (server->server);
  g_main_loop_unref (server->main_loop);
  g_free (server->url);
  g_slice_free (TestServer, server);
}
