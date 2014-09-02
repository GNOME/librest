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

#include <rest-extras/flickr-proxy.h>
#include <rest/rest-xml-parser.h>
#include <stdio.h>

#define API_KEY "cf4e02fc57240a9b07346ad26e291080"
#define SHARED_SECRET "cdfa2329cb206e50"
#define ROSS_ID "35468147630@N01"
int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;
  RestXmlParser *parser;
  RestXmlNode *root, *node;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  /* Create the proxy */
  proxy = flickr_proxy_new (API_KEY, SHARED_SECRET);

  g_assert_cmpstr (flickr_proxy_get_api_key (FLICKR_PROXY (proxy)),
                   ==, API_KEY);
  g_assert_cmpstr (flickr_proxy_get_shared_secret (FLICKR_PROXY (proxy)),
                   ==, SHARED_SECRET);

  /*
   * Sadly can't unit test authentication.
   */

  /*
   * Test a call which just requires an API key but no signature.
   */

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "flickr.people.getInfo");
  rest_proxy_call_add_param (call, "user_id", ROSS_ID);
  if (!rest_proxy_call_run (call, NULL, &error))
    g_error ("Cannot make call: %s", error->message);

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));
  g_assert (root);
  g_assert_cmpstr (root->name, ==, "rsp");
  g_assert_cmpstr (rest_xml_node_get_attr (root, "stat"), ==, "ok");

  node = rest_xml_node_find (root, "person");
  g_assert (node);
  g_assert_cmpstr (rest_xml_node_get_attr (node, "nsid"), ==, ROSS_ID);

  node = rest_xml_node_find (node, "username");
  g_assert (node);
  g_assert_cmpstr (node->content, ==, "Ross Burton");

  rest_xml_node_unref (root);
  g_object_unref (call);

  /*
   * Test a call which requires a signature.
   */

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "flickr.auth.getFrob");
  if (!rest_proxy_call_run (call, NULL, &error))
    g_error ("Cannot make call: %s", error->message);

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));
  g_assert (root);
  g_assert_cmpstr (root->name, ==, "rsp");
  g_assert_cmpstr (rest_xml_node_get_attr (root, "stat"), ==, "ok");

  node = rest_xml_node_find (root, "frob");
  g_assert (node);
  g_assert (node->content);
  g_assert_cmpstr (node->content, !=, "");

  g_object_unref (proxy);
  return 0;
}
