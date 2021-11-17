/* test-server.c
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test-server.h"

static GMutex server_start_mutex;
static GCond server_start_cond;

SoupServer *
test_server_new ()
{
  SoupServer *server = soup_server_new ("tls-certificate", NULL, NULL);
  return server;
}

gchar *
test_server_get_uri (SoupServer *server,
                     const char *scheme,
                     const char *host)
{
	GSList *uris, *u;
#ifdef WITH_SOUP_2
  SoupURI *uri, *ret_uri = NULL;
#else
	GUri *uri, *ret_uri = NULL;
#endif

	uris = soup_server_get_uris (server);
	for (u = uris; u; u = u->next) {
		uri = u->data;

#ifdef WITH_SOUP_2
		if (scheme && strcmp (soup_uri_get_scheme (uri), scheme) != 0)
			continue;
		if (host && strcmp (soup_uri_get_host (uri), host) != 0)
			continue;

		ret_uri = soup_uri_copy (uri);
#else
		if (scheme && strcmp (g_uri_get_scheme (uri), scheme) != 0)
			continue;
		if (host && strcmp (g_uri_get_host (uri), host) != 0)
			continue;

		ret_uri = g_uri_ref (uri);
#endif
		break;
	}

#ifdef WITH_SOUP_2
	g_slist_free_full (uris, (GDestroyNotify)soup_uri_free);
  return soup_uri_to_string (ret_uri, FALSE);
#else
	g_slist_free_full (uris, (GDestroyNotify)g_uri_unref);
  return g_uri_to_string (ret_uri);
#endif
}

static gpointer
run_server_thread (gpointer user_data)
{
	SoupServer *server = user_data;
	GMainContext *context;
	GMainLoop *loop;
  GError *error = NULL;

	context = g_main_context_new ();
	g_main_context_push_thread_default (context);
	loop = g_main_loop_new (context, FALSE);
	g_object_set_data (G_OBJECT (server), "GMainLoop", loop);

	// TODO: error handling
	soup_server_listen_local (server, 0, 0, &error);
  if (error != NULL)
    g_error ("%s", error->message);

	g_mutex_lock (&server_start_mutex);
	g_cond_signal (&server_start_cond);
	g_mutex_unlock (&server_start_mutex);

	g_main_loop_run (loop);
	g_main_loop_unref (loop);

  g_print("%s\n", "Shutting down server...");

	soup_server_disconnect (server);

	g_main_context_pop_thread_default (context);
	g_main_context_unref (context);

	return NULL;
}

void
test_server_run_in_thread (SoupServer *server)
{
	GThread *thread;

	g_mutex_lock (&server_start_mutex);

	thread = g_thread_new ("server_thread", run_server_thread, server);
	g_cond_wait (&server_start_cond, &server_start_mutex);
	g_mutex_unlock (&server_start_mutex);

	g_object_set_data (G_OBJECT (server), "thread", thread);
}
