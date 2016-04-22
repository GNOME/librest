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

#include <rest/rest-xml-parser.h>

/* These debugging functions *leak* */
static gchar *
_generate_attrs_output (GHashTable *attrs)
{
  char *res;
  char *res_old = NULL;
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
  char *attrs_output = NULL;

  do {
    attrs_output = _generate_attrs_output (node->attrs);
    g_print ("%*s[%s, %s, %s]\n",
             depth,
             "",
             node->name,
             node->content ? node->content : "NULL",
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

int
main (int argc, char **argv)
{
  RestXmlParser *parser;
  RestXmlNode *node;
  GError *error = NULL;
  char *data = NULL;
  gsize length;

  if (argc != 2) {
    g_printerr ("$ dump-xml <FILENAME>\n");
    return 1;
  }

  if (!g_file_get_contents (argv[1], &data, &length, &error)) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return 1;
  }

  parser = rest_xml_parser_new ();
  node = rest_xml_parser_parse_from_data (parser, data, length);
  if (node) {
    _rest_xml_node_output (node, 0);
    rest_xml_node_unref (node);
  } else {
    g_print ("Cannot parse document\n");
  }
  g_object_unref (parser);

  return 0;
}
