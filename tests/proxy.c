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

#if SOUP_CHECK_VERSION (2, 28, 0)
/* Avoid deprecation warning with newer libsoup */
#define soup_message_headers_get soup_message_headers_get_one
#endif

static int errors = 0;

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

static void
ping_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }
  g_assert(error == NULL);

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code\n");
    errors++;
    return;
  }

  if (rest_proxy_call_get_payload_length (call) != 0) {
    g_printerr ("wrong length returned\n");
    errors++;
    return;
  }

  g_object_unref (call);
}

static void
echo_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "value", "echome");

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }
  g_assert(error == NULL);

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code\n");
    errors++;
    g_object_unref (call);
    return;
  }
  if (rest_proxy_call_get_payload_length (call) != 6) {
    g_printerr ("wrong length returned\n");
    errors++;
    g_object_unref (call);
    return;
  }
  if (g_strcmp0 ("echome", rest_proxy_call_get_payload (call)) != 0) {
    g_printerr ("wrong string returned\n");
    errors++;
    g_object_unref (call);
    return;
  }
  g_object_unref (call);
}

static void
reverse_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "reverse");
  rest_proxy_call_add_param (call, "value", "reverseme");

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }
  g_assert(error == NULL);

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code\n");
    errors++;
    g_object_unref (call);
    return;
  }
  if (rest_proxy_call_get_payload_length (call) != 9) {
    g_printerr ("wrong length returned\n");
    errors++;
    g_object_unref (call);
    return;
  }
  if (g_strcmp0 ("emesrever", rest_proxy_call_get_payload (call)) != 0) {
    g_printerr ("wrong string returned\n");
    errors++;
    g_object_unref (call);
    return;
  }
  g_object_unref (call);
}

static void
status_ok_test (RestProxy *proxy, int status)
{
  RestProxyCall *call;
  GError *error = NULL;
  char *status_str;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  g_free (status_str);

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }
  g_assert(error == NULL);

  if (rest_proxy_call_get_status_code (call) != status) {
    g_printerr ("wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    errors++;
    return;
  }

  g_object_unref (call);
}

static void
status_error_test (RestProxy *proxy, int status)
{
  RestProxyCall *call;
  GError *error = NULL;
  char *status_str;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  status_str = g_strdup_printf ("%d", status);
  rest_proxy_call_add_param (call, "status", status_str);
  g_free (status_str);

  if (rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call succeeded should have failed");
    errors++;
    g_object_unref (call);
    return;
  }
  g_error_free (error);

  if (rest_proxy_call_get_status_code (call) != status) {
    g_printerr ("wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    errors++;
    return;
  }

  g_object_unref (call);
}

static void
test_status_ok (RestProxy *proxy, const char *function)
{
  RestProxyCall *call;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, function);

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("%s call failed: %s\n", function, error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }
  g_assert(error == NULL);

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    errors++;
    return;
  }

  g_object_unref (call);
}

int
main (int argc, char **argv)
{
  SoupServer *server;
  char *url;
  RestProxy *proxy;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  server = soup_server_new (NULL);
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  soup_server_run_async (server);

  url = g_strdup_printf ("http://127.0.0.1:%d/", soup_server_get_port (server));
  proxy = rest_proxy_new (url, FALSE);
  g_free (url);

  ping_test (proxy);
  echo_test (proxy);
  reverse_test (proxy);
  status_ok_test (proxy, SOUP_STATUS_OK);
  status_ok_test (proxy, SOUP_STATUS_NO_CONTENT);
  /* status_ok_test (proxy, SOUP_STATUS_MULTIPLE_CHOICES); */
  status_error_test (proxy, SOUP_STATUS_BAD_REQUEST);
  status_error_test (proxy, SOUP_STATUS_NOT_IMPLEMENTED);

  test_status_ok (proxy, "useragent/none");
  rest_proxy_set_user_agent (proxy, "TestSuite-1.0");
  test_status_ok (proxy, "useragent/testsuite");

  return errors != 0;
}
