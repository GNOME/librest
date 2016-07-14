/*
 * librest - RESTful web services access
 * Copyright (c) 2010 Intel Corporation.
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rest-extras/lastfm-proxy.h>
#include <rest/rest-xml-parser.h>

/*
 * Parse the payload and either return a RestXmlNode, or error. As a side-effect
 * the call is unreffed.
 */
static RestXmlNode *
get_xml (RestProxyCall *call)
{
  RestXmlParser *parser;
  RestXmlNode *root;
  GError *error = NULL;

  parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));

  if (!lastfm_proxy_is_successful (root, &error))
    g_error ("%s", error->message);

  g_object_unref (call);
  g_object_unref (parser);

  return root;
}

static void
shout (RestProxy *proxy, const char *username, const char *message)
{
  RestProxyCall *call;
  GError *error = NULL;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_set_function (call, "user.shout");
  rest_proxy_call_add_param (call, "user", username);
  rest_proxy_call_add_param (call, "message", message);

  if (!rest_proxy_call_sync (call, &error))
    g_error ("Cannot shout: %s", error->message);

  rest_xml_node_unref (get_xml (call));
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  RestProxy *proxy;
  RestProxyCall *call;
  RestXmlNode *root;
  char *token, *url, *userid = NULL, *session = NULL;
  GOptionEntry entries[] = {
    { "session", 's', 0, G_OPTION_ARG_STRING, &session, "Session key (optional)", "KEY" },
    { "user", 'u', 0, G_OPTION_ARG_STRING, &userid, "User to send a message to", "USERNAME" },
    { NULL }
  };

  context = g_option_context_new ("- send a shout to a Last.fm user");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_print ("Option parsing failed: %s\n", error->message);
    return 1;
  }

  if (userid == NULL) {
    g_print ("Need a user ID to send a shout out to\n\n");
    g_print ("%s", g_option_context_get_help (context, TRUE, NULL));
    return 1;
  }

  proxy = lastfm_proxy_new ("aa581f6505fd3ea79073ddcc2215cbc7",
                            "7db227a36b3154e3a3306a23754de1d7");

  if (session) {
    lastfm_proxy_set_session_key (LASTFM_PROXY (proxy), argv[1]);
  } else {
    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "auth.getToken");

    if (!rest_proxy_call_sync (call, NULL))
      g_error ("Cannot get token");

    root = get_xml (call);
    token = g_strdup (rest_xml_node_find (root, "token")->content);
    g_debug ("got token %s", token);
    rest_xml_node_unref (root);

    url = lastfm_proxy_build_login_url (LASTFM_PROXY (proxy), token);
    g_print ("Go to %s to authorise me and then press any key.\n", url);
    getchar ();

    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "auth.getSession");
    rest_proxy_call_add_param (call, "token", token);
    if (!rest_proxy_call_sync (call, NULL))
      g_error ("Cannot get session");

    root = get_xml (call);
    session = rest_xml_node_find (root, "key")->content;
    g_print ("Got session key %s\n", session);
    g_print ("Got username %s\n", rest_xml_node_find (root, "name")->content);
    lastfm_proxy_set_session_key (LASTFM_PROXY (proxy), session);
    rest_xml_node_unref (root);
  }

  shout (proxy, userid, "Hello from librest!");

  g_object_unref (proxy);

  return 0;
}
