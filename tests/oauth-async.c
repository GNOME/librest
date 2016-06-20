/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
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

#include <rest/oauth-proxy.h>
#include <rest/oauth-proxy-call.h>
#include <rest/oauth-proxy-private.h>
#include <stdio.h>
#include <stdlib.h>

static void
make_calls (OAuthProxy *oproxy, GMainLoop *loop)
{
  RestProxy *proxy = REST_PROXY (oproxy);
  RestProxyCall *call;
  GError *error = NULL;

  /* Make some test calls */

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "foo", "bar");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "foo=bar");
  g_object_unref (call);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "numbers", "1234567890");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "numbers=1234567890");
  g_object_unref (call);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "echo");
  rest_proxy_call_add_param (call, "escape", "!£$%^&*()");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "escape=%21%C2%A3%24%25%5E%26%2A%28%29");
  g_object_unref (call);

  g_main_loop_quit (loop);
}

static void
access_token_cb2 (GObject      *source_object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  OAuthProxy *proxy = OAUTH_PROXY (source_object);
  const gchar *token, *token_secret;
  GError *error = NULL;
  GMainLoop *loop = user_data;

  oauth_proxy_access_token_finish (proxy, result, &error);
  g_assert_no_error (error);

  token = oauth_proxy_get_token (proxy);
  g_assert_cmpstr (token, ==, "accesskey");
  token_secret = oauth_proxy_get_token_secret (proxy);
  g_assert_cmpstr (token_secret, ==, "accesssecret");

  make_calls (proxy, loop);
}



static void
access_token_cb1 (GObject      *source_object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  OAuthProxy *proxy = OAUTH_PROXY (source_object);
  const gchar *token, *token_secret;
  GError *error = NULL;
  GMainLoop *loop = user_data;

  oauth_proxy_access_token_finish (proxy, result, &error);
  g_assert_no_error (error);

  token = oauth_proxy_get_token (proxy);
  g_assert_cmpstr (token, ==, "accesskey");
  token_secret = oauth_proxy_get_token_secret (proxy);
  g_assert_cmpstr (token_secret, ==, "accesssecret");

  g_main_loop_quit (loop);
}

static void
request_token_cb3 (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  OAuthProxy *proxy = OAUTH_PROXY (source_object);
  GMainLoop *loop = user_data;
  const char *token, *token_secret;
  GError *error = NULL;

  oauth_proxy_request_token_finish (proxy, result, &error);
  g_assert_no_error (error);

  token = oauth_proxy_get_token (proxy);
  g_assert_cmpstr (token, ==, "requestkey");
  token_secret = oauth_proxy_get_token_secret (proxy);
  g_assert_cmpstr (token_secret, ==, "requestsecret");

  /* Second stage authentication, this gets an access token */
  oauth_proxy_access_token_async (proxy, "access-token", NULL,
                                  NULL, access_token_cb2, loop);
}

static void
request_token_cb2 (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  OAuthProxy *proxy = OAUTH_PROXY (source_object);
  GMainLoop *loop = user_data;
  const char *token, *token_secret;
  GError *error = NULL;

  oauth_proxy_request_token_finish (proxy, result, &error);
  g_assert_no_error (error);

  token = oauth_proxy_get_token (proxy);
  g_assert_cmpstr (token, ==, "requestkey");
  token_secret = oauth_proxy_get_token_secret (proxy);
  g_assert_cmpstr (token_secret, ==, "requestsecret");

  /* Second stage authentication, this gets an access token */
  oauth_proxy_access_token_async (proxy, "access-token", NULL,
                                  NULL, access_token_cb1, loop);
}

static void
request_token_cb1 (GObject      *source_object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  OAuthProxy *proxy = OAUTH_PROXY (source_object);
  GMainLoop *loop = user_data;
  GError *error = NULL;

  oauth_proxy_request_token_finish (proxy, result, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (oauth_proxy_get_token (proxy), ==, "requestkey");
  g_assert_cmpstr (oauth_proxy_get_token_secret (proxy), ==, "requestsecret");

  g_main_loop_quit (loop);
}

static gboolean
on_timeout (gpointer data)
{
  exit (1);
  return FALSE;
}

static void
test_request_token ()
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);
  RestProxy *proxy;

  /* Install a timeout so that we don't hang or infinite loop */
  g_timeout_add_seconds (10, on_timeout, NULL);
  proxy = oauth_proxy_new ("key", "secret", "http://oauthbin.com/v1/", FALSE);

  oauth_proxy_request_token_async (OAUTH_PROXY (proxy), "request-token", NULL,
                                   NULL, request_token_cb1, loop);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

static void
test_access_token ()
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);
  RestProxy *proxy;

  /* Install a timeout so that we don't hang or infinite loop */
  g_timeout_add_seconds (10, on_timeout, NULL);
  proxy = oauth_proxy_new ("key", "secret", "http://oauthbin.com/v1/", FALSE);

  oauth_proxy_request_token_async (OAUTH_PROXY (proxy), "request-token", NULL,
                                   NULL, request_token_cb2, loop);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

static void
test_calls ()
{
  GMainLoop *loop = g_main_loop_new (NULL, TRUE);
  RestProxy *proxy;

  /* Install a timeout so that we don't hang or infinite loop */
  g_timeout_add_seconds (10, on_timeout, NULL);
  proxy = oauth_proxy_new ("key", "secret", "http://oauthbin.com/v1/", FALSE);

  oauth_proxy_request_token_async (OAUTH_PROXY (proxy), "request-token", NULL,
                                   NULL, request_token_cb3, loop);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/oauth-async/request-token", test_request_token);
  g_test_add_func ("/oauth-async/access-token", test_access_token);
  g_test_add_func ("/oauth-async/calls", test_calls);

  return g_test_run ();
}
