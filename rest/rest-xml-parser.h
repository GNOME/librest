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

#ifndef _REST_XML_PARSER
#define _REST_XML_PARSER

#include <glib-object.h>

G_BEGIN_DECLS

#define REST_TYPE_XML_PARSER rest_xml_parser_get_type()

#define REST_XML_PARSER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REST_TYPE_XML_PARSER, RestXmlParser))

#define REST_XML_PARSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), REST_TYPE_XML_PARSER, RestXmlParserClass))

#define REST_IS_XML_PARSER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REST_TYPE_XML_PARSER))

#define REST_IS_XML_PARSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), REST_TYPE_XML_PARSER))

#define REST_XML_PARSER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), REST_TYPE_XML_PARSER, RestXmlParserClass))

#define REST_TYPE_XML_NODE rest_xml_node_get_type ()

typedef struct {
  GObject parent;
} RestXmlParser;

typedef struct {
  GObjectClass parent_class;
} RestXmlParserClass;

/**
 * RestXmlNode:
 * @name: the name of the element
 * @content: the textual content of the element
 * @children: a #GHashTable of string name to #RestXmlNode for the children of
 * the element.
 * @attrs: a #GHashTable of string name to string values for the attributes of
 * the element.
 * @next: the sibling #RestXmlNode with the same name
 */
typedef struct _RestXmlNode RestXmlNode;
struct _RestXmlNode {
  /*< private >*/
  volatile int ref_count;
  /*< public >*/
  gchar *name;
  gchar *content;
  GHashTable *children;
  GHashTable *attrs;
  RestXmlNode *next;
};

GType rest_xml_parser_get_type (void);

RestXmlParser *rest_xml_parser_new (void);
RestXmlNode *rest_xml_parser_parse_from_data (RestXmlParser *parser,
                                              const gchar   *data,
                                              goffset        len);

GType rest_xml_node_get_type (void);

RestXmlNode * rest_xml_node_ref (RestXmlNode *node);
void rest_xml_node_unref (RestXmlNode *node);
G_GNUC_DEPRECATED void rest_xml_node_free (RestXmlNode *node);

const gchar *rest_xml_node_get_attr (RestXmlNode *node,
                                     const gchar *attr_name);
RestXmlNode *rest_xml_node_find (RestXmlNode *start,
                                 const gchar *tag);

G_END_DECLS

#endif /* _REST_XML_PARSER */
