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
 * - port to gtest
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
#include "helper/test-server.h"

#if SOUP_CHECK_VERSION (2, 28, 0)
/* Avoid deprecation warning with newer libsoup */
#define soup_message_headers_get soup_message_headers_get_one
#endif

#ifdef WITH_SOUP_2
static void
server_callback (SoupServer        *server,
                 SoupMessage       *msg,
                 const gchar       *path,
                 GHashTable        *query,
                 SoupClientContext *client,
                 gpointer           user_data)
{
  if (g_str_equal (path, "/ping") && g_strcmp0 ("GET", msg->method) == 0) {
    soup_message_set_status (msg, SOUP_STATUS_OK);
  }
  else if (g_str_equal (path, "/ping") && g_strcmp0 ("POST", msg->method) == 0) {
    soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
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
    SoupMessageHeaders *request_headers = msg->request_headers;

    if (soup_message_headers_get (request_headers, "User-Agent") == NULL) {
      soup_message_set_status (msg, SOUP_STATUS_OK);
    } else {
      soup_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED);
    }
  }
  else if (g_str_equal (path, "/useragent/testsuite")) {
    SoupMessageHeaders *request_headers = msg->request_headers;
    const char *value;
    value = soup_message_headers_get (request_headers, "User-Agent");
    if (g_strcmp0 (value, "TestSuite-1.0") == 0) {
      soup_message_set_status (msg, SOUP_STATUS_OK);
    } else {
      soup_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED);
    }
  }
}
#else
static void
server_callback (SoupServer        *server,
                 SoupServerMessage *msg,
                 const char        *path,
                 GHashTable        *query,
                 gpointer           user_data)
{
  if (g_str_equal (path, "/ping") && g_strcmp0 ("GET", soup_server_message_get_method (msg)) == 0) {
    soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
  }
  else if (g_str_equal (path, "/ping") && g_strcmp0 ("POST", soup_server_message_get_method (msg)) == 0) {
    soup_server_message_set_status (msg, SOUP_STATUS_NOT_FOUND, NULL);
  }
  else if (g_str_equal (path, "/echo")) {
    const char *value;

    value = g_hash_table_lookup (query, "value");
    soup_server_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
                                      value, strlen (value));
    soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
  }
  else if (g_str_equal (path, "/reverse")) {
    char *value;

    value = g_strdup (g_hash_table_lookup (query, "value"));
    g_strreverse (value);

    soup_server_message_set_response (msg, "text/plain", SOUP_MEMORY_TAKE,
                                       value, strlen (value));
    soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
  }
  else if (g_str_equal (path, "/status")) {
    const char *value;
    int status;

    value = g_hash_table_lookup (query, "status");
    if (value) {
      status = atoi (value);
      soup_server_message_set_status (msg, status ?: SOUP_STATUS_INTERNAL_SERVER_ERROR, NULL);
    } else {
      soup_server_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR, NULL);
    }
  }
  else if (g_str_equal (path, "/useragent/none")) {
    SoupMessageHeaders *request_headers = soup_server_message_get_request_headers (msg);

    if (soup_message_headers_get (request_headers, "User-Agent") == NULL) {
      soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
    } else {
      soup_server_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED, NULL);
    }
  }
  else if (g_str_equal (path, "/useragent/testsuite")) {
    SoupMessageHeaders *request_headers = soup_server_message_get_request_headers (msg);
    const char *value;
    value = soup_message_headers_get (request_headers, "User-Agent");
    if (g_strcmp0 (value, "TestSuite-1.0") == 0) {
      soup_server_message_set_status (msg, SOUP_STATUS_OK, NULL);
    } else {
      soup_server_message_set_status (msg, SOUP_STATUS_EXPECTATION_FAILED, NULL);
    }
  }
}
#endif


static void
ping_test (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  g_autoptr(RestProxyCall) call = NULL;
  g_autoptr(GError) error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");
  rest_proxy_call_set_method (call, "GET");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 0);

  g_clear_object (&call);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_sync (call, &error);
  g_assert_error (error, REST_PROXY_ERROR, 404);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_NOT_FOUND);
}

static void
echo_test (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  g_autoptr(RestProxyCall) call = NULL;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "value", "echome");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 6);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "echome");
}

static void
reverse_test (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  g_autoptr(RestProxyCall) call = NULL;
  g_autoptr(GError) error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "reverse");
  rest_proxy_call_add_param (call, "value", "reverseme");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
  g_assert_cmpint (rest_proxy_call_get_payload_length (call), ==, 9);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "emesrever");
}

static void
status_ok_test (RestProxy *proxy, guint status)
{
  g_autoptr(RestProxyCall) call = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *status_str = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, status);
}

static void
status_test (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  status_ok_test (proxy, SOUP_STATUS_OK);
  status_ok_test (proxy, SOUP_STATUS_NO_CONTENT);
}

static void
status_error_test (RestProxy *proxy, guint status)
{
  g_autoptr(RestProxyCall) call = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *status_str = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  rest_proxy_call_sync (call, &error);
  g_assert_error (error, REST_PROXY_ERROR, status);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, status);
}

static void
status_test_error (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  status_error_test (proxy, SOUP_STATUS_BAD_REQUEST);
  status_error_test (proxy, SOUP_STATUS_NOT_IMPLEMENTED);
}

static void
test_status_ok (RestProxy *proxy, const char *function)
{
  g_autoptr(RestProxyCall) call = NULL;
  g_autoptr(GError) error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, function);
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpint (rest_proxy_call_get_status_code (call), ==, SOUP_STATUS_OK);
}

static void
test_user_agent (gconstpointer data)
{
  RestProxy *proxy = (RestProxy *)data;
  test_status_ok (proxy, "useragent/none");
  rest_proxy_set_user_agent (proxy, "TestSuite-1.0");
  test_status_ok (proxy, "useragent/testsuite");
}

int
main (int     argc,
      gchar **argv)
{
  RestProxy *proxy;
  SoupServer *server;
  gint ret;
  gchar *uri;

  g_test_init (&argc, &argv, NULL);

  server = test_server_new ();
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  test_server_run_in_thread (server);
  uri = test_server_get_uri (server, "http", NULL);

  proxy = rest_proxy_new (uri, FALSE);

  g_test_add_data_func ("/proxy/ping", proxy, ping_test);
  g_test_add_data_func ("/proxy/echo", proxy, echo_test);
  g_test_add_data_func ("/proxy/reverse", proxy, reverse_test);
  g_test_add_data_func ("/proxy/status_ok_test", proxy, status_test);
  g_test_add_data_func ("/proxy/status_error_test", proxy, status_test_error);
  g_test_add_data_func ("/proxy/user_agent", proxy, test_user_agent);

  ret = g_test_run ();

	g_main_context_unref (g_main_context_default ());

  return ret;
}
