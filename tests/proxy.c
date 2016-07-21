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

/*
 * TODO:
 * - decide if status 3xx is success or failure
 * - test query params
 * - test request headers
 * - test response headers
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy.h>

#if SOUP_CHECK_VERSION (2, 28, 0)
/* Avoid deprecation warning with newer libsoup */
#define soup_message_headers_get soup_message_headers_get_one
#endif

#define PORT 8080

typedef struct {
  SoupServer *server;
  GMainLoop  *main_loop;
  char *url;
  GThread *thread;
} TestServer;

static void
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

static void *
test_server_thread_func (gpointer data)
{
  TestServer *server = data;

  g_main_context_push_thread_default (g_main_loop_get_context (server->main_loop));
  g_main_loop_run (server->main_loop);
  g_main_context_pop_thread_default (g_main_loop_get_context (server->main_loop));

  return NULL;
}

static void
test_server_run (TestServer *server)
{
  server->thread = g_thread_new ("Server Thread", test_server_thread_func, server);
}

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  if (g_str_equal (path, "/ping")) {
    soup_message_set_status (msg, SOUP_STATUS_OK);
  }
  else if (g_str_equal (path, "/echo")) {
    const char *value;

    value = g_hash_table_lookup (query, "value");
    soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
                               value, strlen (value));
    soup_message_set_status (msg, SOUP_STATUS_OK);
  }
  else if (g_str_equal (path, "/reverse")) {
    char *value;

    value = g_strdup (g_hash_table_lookup (query, "value"));
    g_strreverse (value);

    soup_message_set_response (msg, "text/plain", SOUP_MEMORY_TAKE,
                               value, strlen (value));
    soup_message_set_status (msg, SOUP_STATUS_OK);
  }
  else if (g_str_equal (path, "/status")) {
    const char *value;
    int status;

    value = g_hash_table_lookup (query, "status");
    if (value) {
      status = atoi (value);
      soup_message_set_status (msg, status ?: SOUP_STATUS_INTERNAL_SERVER_ERROR);
    } else {
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    }
  }
  else if (g_str_equal (path, "/useragent/none")) {
    if (soup_message_headers_get (msg->request_headers, "User-Agent") == NULL) {
      soup_message_set_status (msg, SOUP_STATUS_OK);
    } else {
      soup_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED);
    }
  }
  else if (g_str_equal (path, "/useragent/testsuite")) {
    const char *value;
    value = soup_message_headers_get (msg->request_headers, "User-Agent");
    if (g_strcmp0 (value, "TestSuite-1.0") == 0) {
      soup_message_set_status (msg, SOUP_STATUS_OK);
    } else {
      soup_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED);
    }
  }
}

static TestServer *
create_server ()
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

  soup_server_add_handler (server, "/", server_callback,
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

static void
ping ()
{
  TestServer *server = create_server ();
  RestProxy *proxy = rest_proxy_new (server->url, FALSE);
  RestProxyCall *call = rest_proxy_new_call (proxy);
  GError *error = NULL;

  test_server_run (server);

  rest_proxy_call_set_function (call, "ping");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 0);

  g_object_unref (call);
  g_object_unref (proxy);

  test_server_stop (server);
}

static void
echo ()
{
  TestServer *server = create_server ();
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;

  test_server_run (server);

  proxy = rest_proxy_new (server->url, FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "value", "echome");

  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 6);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "echome");

  test_server_stop (server);
  g_object_unref (proxy);
  g_object_unref (call);
}

static void
reverse ()
{
  TestServer *server = create_server ();
  RestProxy *proxy = rest_proxy_new (server->url, FALSE);
  RestProxyCall *call;
  GError *error = NULL;

  test_server_run (server);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "reverse");
  rest_proxy_call_add_param (call, "value", "reverseme");

  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 9);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "emesrever");

  test_server_stop (server);
  g_object_unref (proxy);
  g_object_unref (call);
}

static void
params ()
{
  TestServer *server = create_server ();
  RestProxy *proxy = rest_proxy_new (server->url, FALSE);
  RestProxyCall *call;
  GError *error = NULL;
  char *status_str;
  int status = SOUP_STATUS_OK;

  test_server_run (server);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  g_free (status_str);

  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, status);
  g_object_unref (call);


  status = SOUP_STATUS_NO_CONTENT;
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  g_free (status_str);

  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, status);


  g_object_unref (proxy);
  g_object_unref (call);
  test_server_stop (server);
}

static void
fail ()
{
  TestServer *server = create_server ();
  RestProxy *proxy = rest_proxy_new (server->url, FALSE);
  RestProxyCall *call;
  GError *error = NULL;
  int status = SOUP_STATUS_BAD_REQUEST;
  char *status_str;

  test_server_run (server);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  g_free (status_str);

  g_assert (!rest_proxy_call_sync (call, &error));
  g_assert_nonnull (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, status);

  g_error_free (error);
  g_object_unref (proxy);
  g_object_unref (call);
  test_server_stop (server);
}

static void
useragent ()
{
  TestServer *server = create_server ();
  RestProxy *proxy = rest_proxy_new (server->url, FALSE);
  RestProxyCall *call;
  GError *error = NULL;
  gboolean result;

  test_server_run (server);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "useragent/none");

  result = rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert (result);

  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_object_unref (call);


  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "useragent/testsuite");
  rest_proxy_set_user_agent (proxy, "TestSuite-1.0");

  result = rest_proxy_call_sync (call, &error);
  g_assert (result);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);

  g_object_unref (proxy);
  g_object_unref (call);
  test_server_stop (server);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/proxy/ping", ping);
  g_test_add_func ("/proxy/echo", echo);
  g_test_add_func ("/proxy/reverse", reverse);
  g_test_add_func ("/proxy/params", params);
  g_test_add_func ("/proxy/fail", fail);
  g_test_add_func ("/proxy/useragent", useragent);

  return g_test_run ();
}
