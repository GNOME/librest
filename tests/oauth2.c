/* oauth2.c
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

#include <glib.h>
#include "rest/rest.h"
#include "helper/test-server.h"

static void
server_callback (SoupServer        *server,
                 SoupMessage       *msg,
                 const gchar       *path,
                 GHashTable        *query,
                 SoupClientContext *client,
                 gpointer           user_data)
{
  g_print ("%s\n", path);
  if (g_strcmp0 (path, "/token") == 0)
    {
      g_print ("%s\n", msg->request_body->data);
      gchar *json = "{"
           "\"access_token\":\"2YotnFZFEjr1zCsicMWpAA\","
           "\"token_type\":\"example\","
           "\"expires_in\":3600,"
           "\"refresh_token\":\"tGzv3JOkF0XG5Qx2TlKWIA\","
           "\"example_parameter\":\"example_value\""
         "}";
      soup_message_set_status (msg, SOUP_STATUS_OK);
      soup_message_set_response (msg, "application/json", SOUP_MEMORY_COPY, json, strlen(json));
      return;
    }
  else if (g_strcmp0 (path, "/api/bearer") == 0)
    {
      const gchar *authorization = soup_message_headers_get_one (msg->request_headers, "Authorization");
      g_print ("%s\n", authorization);
      soup_message_set_status (msg, SOUP_STATUS_OK);
      soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY, authorization, strlen(authorization));
      return;
    }
}

static void
test_authorization_url (gconstpointer url)
{
  g_autoptr(RestProxy) proxy = REST_PROXY (rest_oauth2_proxy_new ("http://www.example.com/auth",
                                                                  "http://www.example.com/token",
                                                                  "http://www.example.com",
                                                                  "client-id",
                                                                  "client-secret",
                                                                  "http://www.example.com/api"));
  g_autoptr(RestPkceCodeChallenge) pkce = rest_pkce_code_challenge_new_random ();

  gchar *authorization_url = rest_oauth2_proxy_build_authorization_url (REST_OAUTH2_PROXY (proxy),
                                             rest_pkce_code_challenge_get_challenge (pkce),
                                             NULL,
                                             NULL);
  g_autofree gchar *expected = g_strdup_printf ("http://www.example.com/auth?code_challenge_method=S256&redirect_uri=http%%3A%%2F%%2Fwww.example.com&client_id=client-id&code_challenge=%s&response_type=code", rest_pkce_code_challenge_get_challenge (pkce));
  g_assert_cmpstr (authorization_url, ==, expected);
}

static void
test_fetch_access_token_finished (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  g_autoptr(GError) error = NULL;
  gboolean *finished = user_data;

  g_assert (G_IS_OBJECT (object));
  g_assert (G_IS_ASYNC_RESULT (result));

  rest_oauth2_proxy_fetch_access_token_finish (REST_OAUTH2_PROXY (object), result, &error);
  g_assert_no_error (error);

  *finished = TRUE;
}

static void
test_fetch_access_token (gconstpointer url)
{
  GMainContext *async_context = g_main_context_ref_thread_default ();
  g_autoptr(GError) error = NULL;

  g_autofree gchar *tokenurl = g_strdup_printf ("%stoken", (gchar *)url);
  g_autofree gchar *baseurl = g_strdup_printf ("%sapi", (gchar *)url);

  gboolean finished = FALSE;
  g_autoptr(RestProxy) proxy = REST_PROXY (rest_oauth2_proxy_new ("http://www.example.com/auth",
                                                                  tokenurl,
                                                                  "http://www.example.com",
                                                                  "client-id",
                                                                  "client-secret",
                                                                  baseurl));
  rest_oauth2_proxy_fetch_access_token_async (REST_OAUTH2_PROXY (proxy), "1234567890", "code_verifier", NULL, test_fetch_access_token_finished, &finished);
  while (!finished) {
    g_main_context_iteration (async_context, TRUE);
  }

  g_assert_cmpstr ("2YotnFZFEjr1zCsicMWpAA", ==, rest_oauth2_proxy_get_access_token (REST_OAUTH2_PROXY (proxy)));
  g_assert_cmpstr ("tGzv3JOkF0XG5Qx2TlKWIA", ==, rest_oauth2_proxy_get_refresh_token (REST_OAUTH2_PROXY (proxy)));

  g_autoptr(RestProxyCall) call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "GET");
  rest_proxy_call_set_function (call, "bearer");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_autofree gchar *payload = g_strndup (rest_proxy_call_get_payload (call), rest_proxy_call_get_payload_length (call));
  g_assert_cmpstr ("Bearer 2YotnFZFEjr1zCsicMWpAA", ==, payload);

  g_main_context_unref (async_context);
}

static void
test_refresh_access_token_finished_error (GObject      *object,
                                          GAsyncResult *result,
                                          gpointer      user_data)
{
  g_autoptr(GError) error = NULL;
  gboolean *finished = user_data;

  g_assert (G_IS_OBJECT (object));
  g_assert (G_IS_ASYNC_RESULT (result));

  rest_oauth2_proxy_refresh_access_token_finish (REST_OAUTH2_PROXY (object), result, &error);
  g_assert_error (error, REST_OAUTH2_PROXY_ERROR, REST_OAUTH2_PROXY_NO_REFRESH_TOKEN);

  *finished = TRUE;
}

static void
test_refresh_access_token_finished (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  g_autoptr(GError) error = NULL;
  gboolean *finished = user_data;

  g_assert (G_IS_OBJECT (object));
  g_assert (G_IS_ASYNC_RESULT (result));

  rest_oauth2_proxy_refresh_access_token_finish (REST_OAUTH2_PROXY (object), result, &error);
  g_assert_no_error (error);

  *finished = TRUE;
}

static void
test_refresh_access_token (gconstpointer url)
{
  GMainContext *async_context = g_main_context_ref_thread_default ();
  g_autoptr(GError) error = NULL;

  g_autofree gchar *tokenurl = g_strdup_printf ("%stoken", (gchar *)url);
  g_autofree gchar *baseurl = g_strdup_printf ("%sapi", (gchar *)url);

  gboolean finished = FALSE;
  g_autoptr(RestProxy) proxy = REST_PROXY (rest_oauth2_proxy_new ("http://www.example.com/auth",
                                                                  tokenurl,
                                                                  "http://www.example.com",
                                                                  "client-id",
                                                                  "client-secret",
                                                                  baseurl));
  rest_oauth2_proxy_refresh_access_token_async (REST_OAUTH2_PROXY (proxy), NULL, test_refresh_access_token_finished_error, &finished);

  while (!finished) {
    g_main_context_iteration (async_context, TRUE);
  }

  rest_oauth2_proxy_set_refresh_token (REST_OAUTH2_PROXY (proxy), "refresh_token");
  rest_oauth2_proxy_refresh_access_token_async (REST_OAUTH2_PROXY (proxy), NULL, test_refresh_access_token_finished, &finished);

  while (!finished) {
    g_main_context_iteration (async_context, TRUE);
  }

  g_assert_cmpstr ("2YotnFZFEjr1zCsicMWpAA", ==, rest_oauth2_proxy_get_access_token (REST_OAUTH2_PROXY (proxy)));
  g_assert_cmpstr ("tGzv3JOkF0XG5Qx2TlKWIA", ==, rest_oauth2_proxy_get_refresh_token (REST_OAUTH2_PROXY (proxy)));
}

gint
main (gint   argc,
      gchar *argv[])
{
  SoupServer *server;
  g_autofree gchar *url;

  g_test_init (&argc, &argv, NULL);

  server = test_server_new ();
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  test_server_run_in_thread (server);
  url = test_server_get_uri (server, "http", NULL);

  g_test_add_data_func ("/oauth2/authorization_url", url, test_authorization_url);
  g_test_add_data_func ("/oauth2/fetch_access_token", url, test_fetch_access_token);
  g_test_add_data_func ("/oauth2/refresh_access_token", url, test_refresh_access_token);

  return g_test_run ();
}
