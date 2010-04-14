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

static RestXmlNode *
rest_xml_node_reverse_siblings (RestXmlNode *current)
{
  RestXmlNode *next;
  RestXmlNode *prev = NULL;

  while (current)
  {
    next = current->next;
    current->next = prev;

    prev = current;
    current = next;
  }

  return prev;
}

static void
rest_xml_node_reverse_children_siblings (RestXmlNode *node)
{
  GList *l, *children;
  RestXmlNode *new_node;

  children = g_hash_table_get_values (node->children);

  for (l = children; l; l = l->next)
  {
    new_node = rest_xml_node_reverse_siblings ((RestXmlNode *)l->data);
    g_hash_table_insert (node->children, new_node->name, new_node);
  }

  if (children)
    g_list_free (children);
}

static RestXmlNode *
rest_xml_node_prepend (RestXmlNode *cur_node, RestXmlNode *new_node)
{
  g_assert (new_node->next == NULL);
  new_node->next = cur_node;

  return new_node;
}

GType
rest_xml_node_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    type = g_boxed_type_register_static ("RestXmlNode",
                                         (GBoxedCopyFunc)rest_xml_node_ref,
                                         (GBoxedFreeFunc)rest_xml_node_unref);
  }
  return type;
}

static RestXmlNode *
rest_xml_node_new ()
{
  RestXmlNode *node;

  node = g_slice_new0 (RestXmlNode);
  node->ref_count = 1;
  node->children = g_hash_table_new (NULL, NULL);
  node->attrs = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_free);

  return node;
}

/**
 * rest_xml_node_ref:
 * @node: a #RestXmlNode
 *
 * Increases the reference count of @node.
 *
 * Returns: the same @node.
 */
RestXmlNode *
rest_xml_node_ref (RestXmlNode *node)
{
  g_return_val_if_fail (node, NULL);
  g_return_val_if_fail (node->ref_count > 0, NULL);

  g_atomic_int_inc (&node->ref_count);

  return node;
}

/**
 * rest_xml_node_unref:
 * @node: a #RestXmlNode
 *
 * Decreases the reference count of @node. When its reference count drops to 0,
 * the node is finalized (i.e. its memory is freed).
 */
void
rest_xml_node_unref (RestXmlNode *node)
{
  g_return_if_fail (node);
  g_return_if_fail (node->ref_count > 0);

  if (g_atomic_int_dec_and_test (&node->ref_count)) {
    GList *l;
    RestXmlNode *next = NULL;

    while (node)
    {
        /*
         * Save this pointer now since we are going to free the structure it
         * contains soon.
         */
      next = node->next;

      l = g_hash_table_get_values (node->children);
      while (l)
      {
        rest_xml_node_unref ((RestXmlNode *)l->data);
        l = g_list_delete_link (l, l);
      }

      g_hash_table_unref (node->children);
      g_hash_table_unref (node->attrs);
      g_free (node->content);
      g_slice_free (RestXmlNode, node);

      /*
       * Free the next in the chain by updating node. If we're at the end or
       * there are no siblings then the next = NULL definition deals with this
       * case
       */
      node = next;
    }
  }
}

G_GNUC_DEPRECATED void
rest_xml_node_free (RestXmlNode *node)
{
  rest_xml_node_unref (node);
}

/**
 * rest_xml_node_get_attr:
 * @node: a #RestXmlNode
 * @attr_name: the name of an attribute
 *
 * Get the value of the attribute named @attr_name, or %NULL if it doesn't
 * exist.
 *
 * Returns: the attribute value. This string is owned by #RestXmlNode and should
 * not be freed.
 */
const gchar *
rest_xml_node_get_attr (RestXmlNode *node, 
                        const gchar *attr_name)
{
  return g_hash_table_lookup (node->attrs, attr_name);
}

/**
 * rest_xml_node_find:
 * @start: a #RestXmlNode
 * @tag: the name of a node
 *
 * Searches for the first child node of @start named @tag.
 *
 * Returns: the first child node, or %NULL if it doesn't exist.
 */
RestXmlNode *
rest_xml_node_find (RestXmlNode *start,
                    const gchar *tag)
{
  RestXmlNode *node;
  RestXmlNode *tmp;
  GQueue stack = G_QUEUE_INIT;
  GList *children, *l;
  const char *tag_interned;

  g_return_val_if_fail (start, NULL);
  g_return_val_if_fail (start->ref_count > 0, NULL);

  tag_interned = g_intern_string (tag);

  g_queue_push_head (&stack, start);

  while ((node = g_queue_pop_head (&stack)) != NULL)
  {
    if ((tmp = g_hash_table_lookup (node->children, tag_interned)) != NULL)
    {
      return tmp;
    }

    children = g_hash_table_get_values (node->children);
    for (l = children; l; l = l->next)
    {
      g_queue_push_head (&stack, l->data);
    }
    g_list_free (children);
  }

  return NULL;
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
 * @len: the length of @data
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
  gint res = 0;

  g_return_val_if_fail (REST_IS_XML_PARSER (parser), NULL);

  _rest_setup_debugging ();

  reader = xmlReaderForMemory (data,
                               len,
                               NULL, /* URL? */
                               NULL, /* encoding */
                               XML_PARSE_RECOVER | XML_PARSE_NOCDATA);

  while ((res = xmlTextReaderRead (reader)) == 1)
  {
    switch (xmlTextReaderNodeType (reader))
    {
      case XML_READER_TYPE_ELEMENT:
        /* Lookup the "name" for the tag */
        name = G(xmlTextReaderConstName (reader));
        REST_DEBUG (XML_PARSER, "Opening tag: %s", name);

        /* Create our new node for this tag */

        new_node = rest_xml_node_new ();
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
                                 rest_xml_node_prepend (tmp_node, new_node));
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

          } while ((res = xmlTextReaderMoveToNextAttribute (reader)) == 1);
        }

        break;
      case XML_READER_TYPE_END_ELEMENT:
        REST_DEBUG (XML_PARSER, "Closing tag: %s",
                 xmlTextReaderConstLocalName (reader));

        REST_DEBUG (XML_PARSER, "Popping from stack and updating state.");

        /* For those children that have siblings, reverse the siblings */
        node = (RestXmlNode *)g_queue_pop_head (&nodes);
        rest_xml_node_reverse_children_siblings (node);

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
        cur_node->content = g_strdup (G(xmlTextReaderConstValue (reader)));
        REST_DEBUG (XML_PARSER, "Text content found: %s",
                 cur_node->content);
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
