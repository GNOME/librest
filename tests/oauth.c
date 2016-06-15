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
#include <rest/oauth-proxy-private.h>
#include <stdio.h>

static void
test_auth ()
{
  RestProxy *proxy;
  OAuthProxy *oproxy;
  GError *error = NULL;

  /* Create the proxy */
  proxy = oauth_proxy_new ("key", "secret",
                           "http://oauthbin.com/v1/",
                           FALSE);
  oproxy = OAUTH_PROXY (proxy);
  g_assert (oproxy);

  /* First stage authentication, this gets a request token */
  oauth_proxy_request_token (oproxy, "request-token", NULL, &error);
  if (error != NULL && g_error_matches (error, REST_PROXY_ERROR, REST_PROXY_ERROR_CONNECTION))
    return;

  g_assert_no_error (error);
  g_assert_cmpstr (oauth_proxy_get_token (oproxy), ==, "requestkey");
  g_assert_cmpstr (oauth_proxy_get_token_secret (oproxy), ==, "requestsecret");

  /* Second stage authentication, this gets an access token */
  oauth_proxy_access_token (OAUTH_PROXY (proxy), "access-token", NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (oauth_proxy_get_token (oproxy), ==, "accesskey");
  g_assert_cmpstr (oauth_proxy_get_token_secret (oproxy), ==, "accesssecret");

  g_object_unref (proxy);
}

static void
test_calls ()
{
  RestProxy *proxy;
  RestProxyCall *call;
  OAuthProxy *oproxy;
  GError *error = NULL;


  /* Create the proxy */
  proxy = oauth_proxy_new ("key", "secret",
                           "http://oauthbin.com/v1/",
                           FALSE);
  oproxy = OAUTH_PROXY (proxy);
  g_assert (oproxy);

  /* First stage authentication, this gets a request token */
  oauth_proxy_request_token (oproxy, "request-token", NULL, &error);
  if (error != NULL && g_error_matches (error, REST_PROXY_ERROR, REST_PROXY_ERROR_CONNECTION))
    return;

  g_assert_no_error (error);
  g_assert_cmpstr (oauth_proxy_get_token (oproxy), ==, "requestkey");
  g_assert_cmpstr (oauth_proxy_get_token_secret (oproxy), ==, "requestsecret");

  /* Second stage authentication, this gets an access token */
  oauth_proxy_access_token (OAUTH_PROXY (proxy), "access-token", NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpstr (oauth_proxy_get_token (oproxy), ==, "accesskey");
  g_assert_cmpstr (oauth_proxy_get_token_secret (oproxy), ==, "accesssecret");

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
  rest_proxy_call_add_param (call, "escape", "!+£$%^&*()");
  rest_proxy_call_sync (call, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (rest_proxy_call_get_payload (call), ==, "escape=%21%2B%C2%A3%24%25%5E%26%2A%28%29");
  g_object_unref (call);


  g_object_unref (proxy);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/oauth/first-stage-auth", test_auth);
  g_test_add_func ("/oauth/test-calls", test_calls);

  return g_test_run ();
}
