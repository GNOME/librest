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

static const gboolean verbose = TRUE;
static int errors = 0;

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  if (verbose) g_message (path);

  if (g_str_equal (path, "/ping")) {
    soup_message_set_status (msg, SOUP_STATUS_OK);
  }
  else if (g_str_equal (path, "/slowping")) {
    g_usleep (G_USEC_PER_SEC * 2);
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

  if (verbose) g_message (__FUNCTION__);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");

  if (!rest_proxy_call_invoke (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }

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

  if (verbose) g_message (__FUNCTION__);

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

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code\n");
    errors++;
    return;
  }
  if (rest_proxy_call_get_payload_length (call) != 6) {
    g_printerr ("wrong length returned\n");
    errors++;
    return;
  }
  if (g_strcmp0 ("echome", rest_proxy_call_get_payload (call)) != 0) {
    g_printerr ("wrong string returned\n");
    errors++;
    return;
  }
}

static void
reverse_test (RestProxy *proxy)
{
  RestProxyCall *call;
  GError *error = NULL;

  if (verbose) g_message (__FUNCTION__);

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

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code\n");
    errors++;
    return;
  }
  if (rest_proxy_call_get_payload_length (call) != 9) {
    g_printerr ("wrong length returned\n");
    errors++;
    return;
  }
  if (g_strcmp0 ("emesrever", rest_proxy_call_get_payload (call)) != 0) {
    g_printerr ("wrong string returned\n");
    errors++;
    return;
  }
}

static void
status_ok_test (RestProxy *proxy, int status)
{
  RestProxyCall *call;
  GError *error = NULL;

  if (verbose) g_message (__FUNCTION__);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  rest_proxy_call_add_param (call, "status", g_strdup_printf ("%d", status));

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }

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

  if (verbose) g_message (__FUNCTION__);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  rest_proxy_call_add_param (call, "status", g_strdup_printf ("%d", status));

  if (rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("Call succeeded should have failed");
    errors++;
    g_object_unref (call);
    return;
  }

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

  if (verbose) g_message (__FUNCTION__);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, function);

  if (!rest_proxy_call_run (call, NULL, &error)) {
    g_printerr ("%s call failed: %s\n", function, error->message);
    g_error_free (error);
    errors++;
    g_object_unref (call);
    return;
  }

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    errors++;
    return;
  }

  g_object_unref (call);
}

static void
status_error_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (object);
  GError *error = NULL;
  int status = GPOINTER_TO_INT (user_data);

  if (call == NULL) {
    errors++;
    return;
  }

  if (rest_proxy_call_invoke_finish (call, result, &error)) {
    g_printerr ("Call succeeded incorrectly\n");
    errors++;
    return;
  }

  /* TODO: check error code/domain */
}

static void
test_async_cancelled (RestProxy *proxy)
{
  RestProxyCall *call;
  GCancellable *cancel;

  if (verbose) g_message (__FUNCTION__);

  cancel = g_cancellable_new ();
  g_cancellable_cancel (cancel);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");

  rest_proxy_call_invoke_async (call, cancel, NULL, status_error_cb, NULL);

  g_object_unref (cancel);
}

static void
test_sync_cancelled (RestProxy *proxy)
{
  RestProxyCall *call;
  GCancellable *cancel;

  if (verbose) g_message (__FUNCTION__);

  cancel = g_cancellable_new ();
  g_cancellable_cancel (cancel);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "ping");
  if (rest_proxy_call_invoke (call, cancel, NULL)) {
    g_printerr ("Call succeeded incorrectly\n");
    errors++;
  }

  g_object_unref (cancel);
}

static void
status_ok_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (object);
  GError *error = NULL;
  int status = GPOINTER_TO_INT (user_data);

  if (call == NULL) {
    errors++;
    return;
  }

  if (!rest_proxy_call_invoke_finish (call, result, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    errors++;
  }

  if (rest_proxy_call_get_status_code (call) != status) {
    g_printerr ("wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    errors++;
    return;
  }

  /* TODO: arrange it so that call is unreffed after this callback? */
  g_object_unref (call);
}

static void
status_ok_test_async (RestProxy *proxy, int status)
{
  RestProxyCall *call;

  if (verbose) g_message (__FUNCTION__);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status");
  rest_proxy_call_add_param (call, "status", g_strdup_printf ("%d", status));

  rest_proxy_call_invoke_async (call, NULL, NULL, status_ok_cb, GINT_TO_POINTER (status));
}

int
main (int argc, char **argv)
{
  SoupSession *session;
  SoupServer *server;
  char *url;
  RestProxy *proxy;

  g_thread_init (NULL);
  g_type_init ();

  session = soup_session_sync_new ();

  server = soup_server_new (NULL);
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  g_thread_create ((GThreadFunc)soup_server_run, server, FALSE, NULL);

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

  status_ok_test_async (proxy, SOUP_STATUS_OK);
  status_ok_test_async (proxy, SOUP_STATUS_NO_CONTENT);

  test_async_cancelled (proxy);
  test_sync_cancelled (proxy);

  test_status_ok (proxy, "useragent/none");
  rest_proxy_set_user_agent (proxy, "TestSuite-1.0");
  test_status_ok (proxy, "useragent/testsuite");

  return errors != 0;
}
