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

#include <rest/rest-proxy.h>
#include <rest/rest-xml-parser.h>
#include <unistd.h>

/* These debugging functions *leak* */
static gchar *
_generate_attrs_output (GHashTable *attrs)
{
  gchar *res;
  gchar *res_old = NULL;
  GList *keys, *values, *l, *ll;

  res = g_strdup ("{ ");

  keys = g_hash_table_get_keys (attrs);
  values = g_hash_table_get_values (attrs);

  for (l = keys, ll = values; l; l = l->next, ll = ll->next)
  {
    res_old = res;
    res = g_strconcat (res, l->data, ":", ll->data, " ", NULL);
    g_free (res_old);
  }

  g_list_free (keys);
  g_list_free (values);

  res_old = res;
  res = g_strconcat (res, "}", NULL);
  g_free (res_old);

  return res;
}

static void
_rest_xml_node_output (RestXmlNode *node, gint depth)
{
  RestXmlNode *child;
  GList *values;
  GList *l;
  gchar *attrs_output = NULL;

  do {
    attrs_output = _generate_attrs_output (node->attrs);
    g_print ("%*s[%s, %s, %s]\n",
             depth,
             "",
             node->name,
             node->content,
             attrs_output);
    g_free (attrs_output);
    values = g_hash_table_get_values (node->children);
    for (l = values; l; l = l->next)
    {
      child = (RestXmlNode *)l->data;
      g_print ("%*s%s - >\n", depth, "", child->name);
      _rest_xml_node_output (child, depth + 4);
    }
    g_list_free (values);
  } while ((node = node->next) != NULL);
}

static void
proxy_call_raw_async_cb (GObject      *source_object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  RestProxyCall *call = REST_PROXY_CALL (source_object);
  RestXmlParser *parser;
  RestXmlNode *node;
  const gchar *payload;
  goffset len;

  parser = rest_xml_parser_new ();

  payload = rest_proxy_call_get_payload (call);
  len = rest_proxy_call_get_payload_length (call);
  write (1, payload, len);
  node = rest_xml_parser_parse_from_data (parser, payload, len);

  _rest_xml_node_output (node, 0);
  rest_xml_node_unref (node);
  g_object_unref (parser);
  g_main_loop_quit ((GMainLoop *)user_data);
}

gint
main (gint argc, gchar **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new ("https://www.flickr.com/services/rest/", FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "GET");
  rest_proxy_call_add_params (call,
                              "method", "flickr.photos.getInfo",
                              "api_key", "314691be2e63a4d58994b2be01faacfb",
                              "photo_id", "2658808091",
                              NULL);
  rest_proxy_call_invoke_async (call,
                                NULL,
                                proxy_call_raw_async_cb,
                                loop);

  g_main_loop_run (loop);
  g_object_unref (call);

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "GET");
  rest_proxy_call_add_params (call,
                              "method", "flickr.people.getPublicPhotos",
                              "api_key", "314691be2e63a4d58994b2be01faacfb",
                              "user_id","66598853@N00",
                              NULL);
  rest_proxy_call_invoke_async (call,
                                NULL,
                                proxy_call_raw_async_cb,
                                loop);

  g_main_loop_run (loop);
  g_object_unref (call);
  g_object_unref (proxy);

  g_main_loop_unref (loop);
}
