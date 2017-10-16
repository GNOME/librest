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

#include <string.h>
#include <libxml/xmlreader.h>

#include "rest-private.h"
#include "rest-xml-parser.h"

G_DEFINE_TYPE (RestXmlParser, rest_xml_parser, G_TYPE_OBJECT)

#define G(x) (gchar *)x

static void
rest_xml_parser_class_init (RestXmlParserClass *klass)
{
}

static void
rest_xml_parser_init (RestXmlParser *self)
{
}

static void
rest_xml_parser_xml_reader_error (void *arg,
                                  const char *msg,
                                  xmlParserSeverities severity,
                                  xmlTextReaderLocatorPtr locator)
{
  REST_DEBUG(XML_PARSER, "%s", msg);
}

/**
 * rest_xml_parser_new:
 *
 * Create a new #RestXmlParser, for parsing XML documents.
 *
 * Returns: a new #RestXmlParser.
 */
RestXmlParser *
rest_xml_parser_new (void)
{
  return g_object_new (REST_TYPE_XML_PARSER, NULL);
}

/**
 * rest_xml_parser_parse_from_data:
 * @parser: a #RestXmlParser
 * @data: the XML content to parse
 * @len: the length of @data, or -1 if @data is a nul-terminated string
 *
 * Parse the XML in @data, and return a new #RestXmlNode.  If @data is invalid
 * XML, %NULL is returned.
 *
 * Returns: a new #RestXmlNode, or %NULL if the XML was invalid.
 */
RestXmlNode *
rest_xml_parser_parse_from_data (RestXmlParser *parser,
                                 const gchar   *data,
                                 goffset        len)
{
  xmlTextReaderPtr reader;
  RestXmlNode *cur_node = NULL;
  RestXmlNode *new_node = NULL;
  RestXmlNode *tmp_node = NULL;
  RestXmlNode *root_node = NULL;
  RestXmlNode *node = NULL;

  const gchar *name = NULL;
  const gchar *attr_name = NULL;
  const gchar *attr_value = NULL;
  GQueue nodes = G_QUEUE_INIT;

  g_return_val_if_fail (REST_IS_XML_PARSER (parser), NULL);
  g_return_val_if_fail (data != NULL, NULL);

  if (len == -1)
    len = strlen (data);

  _rest_setup_debugging ();

  reader = xmlReaderForMemory (data,
                               len,
                               NULL, /* URL? */
                               NULL, /* encoding */
                               XML_PARSE_RECOVER | XML_PARSE_NOCDATA);
  if (reader == NULL) {
      return NULL;
  }
  xmlTextReaderSetErrorHandler(reader, rest_xml_parser_xml_reader_error, NULL);

  while (xmlTextReaderRead (reader) == 1)
  {
    switch (xmlTextReaderNodeType (reader))
    {
      case XML_READER_TYPE_ELEMENT:
        /* Lookup the "name" for the tag */
        name = G(xmlTextReaderConstName (reader));
        REST_DEBUG (XML_PARSER, "Opening tag: %s", name);

        /* Create our new node for this tag */

        new_node = _rest_xml_node_new ();
        new_node->name = G (g_intern_string (name));

        if (!root_node)
        {
          root_node = new_node;
        }

        /* 
         * Check if we are not the root node because we need to update it's
         * children set to include the new node.
         */
        if (cur_node)
        {
          tmp_node = g_hash_table_lookup (cur_node->children, new_node->name);

          if (tmp_node)
          {
            REST_DEBUG (XML_PARSER, "Existing node found for this name. "
                              "Prepending to the list.");
            g_hash_table_insert (cur_node->children, 
                                 G(tmp_node->name),
                                 _rest_xml_node_prepend (tmp_node, new_node));
          } else {
            REST_DEBUG (XML_PARSER, "Unseen name. Adding to the children table.");
            g_hash_table_insert (cur_node->children,
                                 G(new_node->name),
                                 new_node);
          }
        }

        /* 
         * Check for empty element. If empty we needn't worry about children
         * or text and thus we don't need to update the stack or state
         */
        if (xmlTextReaderIsEmptyElement (reader)) 
        {
          REST_DEBUG (XML_PARSER, "We have an empty element. No children or text.");
        } else {
          REST_DEBUG (XML_PARSER, "Non-empty element found."
                            "  Pushing to stack and updating current state.");
          g_queue_push_head (&nodes, new_node);
          cur_node = new_node;
        }

        /* 
         * Check if we have attributes. These get stored in the node's attrs
         * hash table.
         */
        if (xmlTextReaderHasAttributes (reader))
        {
          xmlTextReaderMoveToFirstAttribute (reader);

          do {
            attr_name = G(xmlTextReaderConstLocalName (reader));
            attr_value = G(xmlTextReaderConstValue (reader));
            g_hash_table_insert (new_node->attrs,
                                 g_strdup (attr_name),
                                 g_strdup (attr_value));

            REST_DEBUG (XML_PARSER, "Attribute found: %s = %s",
                     attr_name, 
                     attr_value);

          } while (xmlTextReaderMoveToNextAttribute (reader) == 1);
        }

        break;
      case XML_READER_TYPE_END_ELEMENT:
        REST_DEBUG (XML_PARSER, "Closing tag: %s",
                 xmlTextReaderConstLocalName (reader));

        REST_DEBUG (XML_PARSER, "Popping from stack and updating state.");

        /* For those children that have siblings, reverse the siblings */
        node = (RestXmlNode *)g_queue_pop_head (&nodes);
        _rest_xml_node_reverse_children_siblings (node);

        /* Update the current node to the new top of the stack */
        cur_node = (RestXmlNode *)g_queue_peek_head (&nodes);

        if (cur_node)
        {
          REST_DEBUG (XML_PARSER, "Head is now %s", cur_node->name);
        } else {
          REST_DEBUG (XML_PARSER, "At the top level");
        }
        break;
      case XML_READER_TYPE_TEXT:
        if (cur_node)
        {
          cur_node->content = g_strdup (G(xmlTextReaderConstValue (reader)));
          REST_DEBUG (XML_PARSER, "Text content found: %s",
                   cur_node->content);
        } else {
          g_warning ("[XML_PARSER] " G_STRLOC ": "
                     "Text content ignored at top level.");
        }
        break;
      default:
        REST_DEBUG (XML_PARSER, "Found unknown content with type: 0x%x", 
                 xmlTextReaderNodeType (reader));
        break;
    }
  }

  xmlTextReaderClose (reader);
  xmlFreeTextReader (reader);
  return root_node;
}
