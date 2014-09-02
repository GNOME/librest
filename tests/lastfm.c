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

#include <rest-extras/lastfm-proxy.h>
#include <rest/rest-xml-parser.h>
#include <stdio.h>

#define API_KEY "aa581f6505fd3ea79073ddcc2215cbc7"
#define SECRET "7db227a36b3154e3a3306a23754de1d7"
#define USERNAME "rossburton"

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;
  RestXmlParser *parser;
  RestXmlNode *root, *u_node, *node;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  /* Create the proxy */
  proxy = lastfm_proxy_new (API_KEY, SECRET);

  g_assert_cmpstr (lastfm_proxy_get_api_key (LASTFM_PROXY (proxy)),
                   ==, API_KEY);
  g_assert_cmpstr (lastfm_proxy_get_secret (LASTFM_PROXY (proxy)),
                   ==, SECRET);

  /*
   * Sadly can't unit test authentication.  Need an interactive mode.
   */

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "user.getInfo");
  rest_proxy_call_add_param (call, "user", USERNAME);
  if (!rest_proxy_call_sync (call, &error))
    g_error ("Cannot make call: %s", error->message);

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));
  g_assert (root);
  g_assert_cmpstr (root->name, ==, "lfm");
  g_assert_cmpstr (rest_xml_node_get_attr (root, "status"), ==, "ok");

  u_node = rest_xml_node_find (root, "user");
  g_assert (u_node);

  node = rest_xml_node_find (u_node, "id");
  g_assert (node);
  g_assert_cmpstr (node->content, ==, "17038");

  node = rest_xml_node_find (u_node, "name");
  g_assert (node);
  g_assert_cmpstr (node->content, ==, USERNAME);

  rest_xml_node_unref (root);
  g_object_unref (parser);
  g_object_unref (call);

  g_object_unref (proxy);
  return 0;
}
