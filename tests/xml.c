/*
 * librest - RESTful web services access
 * Copyright (c) 2011, Intel Corporation.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

#include <string.h>

#define TEST_XML "<node0 a00=\'v00\' a01=\'v01\'><node1 a10=\'v10\'></node1><node1 a10=\'v10\'></node1>Cont0</node0>"

int
main (int argc, char **argv)
{
  RestXmlParser *parser;
  RestXmlNode *root, *node;
  char *xml;

  parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser, "", -1);
  g_assert (root == NULL);

  root = rest_xml_parser_parse_from_data (parser, "<invalid", -1);
  g_assert (root == NULL);

  root = rest_xml_parser_parse_from_data (parser, TEST_XML, strlen (TEST_XML));
  g_assert (root);

  xml = rest_xml_node_print (root);

  g_assert (xml);

  if (strcmp (TEST_XML, xml))
    {
      g_error ("Generated output for parsed XML does not match:\n"
               "in:  %s\n"
               "out: %s\n",
               TEST_XML, xml);
    }

  g_free (xml);
  rest_xml_node_unref (root);

  root = rest_xml_node_add_child (NULL, "node0");
  rest_xml_node_add_attr (root, "a00", "v00");
  rest_xml_node_add_attr (root, "a01", "v01");

  node = rest_xml_node_add_child (root, "node1");
  rest_xml_node_add_attr (node, "a10", "v10");

  node = rest_xml_node_add_child (root, "node1");
  rest_xml_node_add_attr (node, "a10", "v10");

  rest_xml_node_set_content (root, "Cont0");

  xml = rest_xml_node_print (root);

  g_assert (xml);

  if (strcmp (TEST_XML, xml))
    {
      g_error ("Generated output for constructed XML does not match:\n"
               "in:  %s\n"
               "out: %s\n",
               TEST_XML, xml);
    }

  g_free (xml);
  rest_xml_node_unref (root);
  g_object_unref (parser);

  return 0;
}
