#include <string.h>
#include <stdlib.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy.h>

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
  if (strcmp ("echome", rest_proxy_call_get_payload (call)) != 0) {
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
  if (strcmp ("emesrever", rest_proxy_call_get_payload (call)) != 0) {
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

int
main (int argc, char **argv)
{
  SoupSession *session;
  SoupServer *server;
  char *url;
  RestProxy *proxy;

  g_thread_init (NULL);
  g_type_init ();

  session = soup_session_async_new ();

  server = soup_server_new (NULL);
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  soup_server_run_async (server);

  url = g_strdup_printf ("http://localhost:%d/", soup_server_get_port (server));
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

  return errors != 0;
}
