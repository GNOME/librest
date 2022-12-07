/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, 2011 Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *          Tomas Frydrych <tf@linux.intel.com>
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

#pragma once

#include <glib-object.h>
#include <rest/rest-exports.h>

G_BEGIN_DECLS

#define REST_TYPE_XML_NODE rest_xml_node_get_type ()

/**
 * RestXmlNode:
 * @name: the name of the element
 * @content: the textual content of the element
 * @children: a #GHashTable of string name to #RestXmlNode for the children of
 * the element.
 * @attrs: a #GHashTable of string name to string values for the attributes of
 * the element.
 * @next: the sibling #RestXmlNode with the same name
 *
 * The #RestXmlNode contains a parsed XmlNode for easy consumption
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

REST_EXPORT
GType rest_xml_node_get_type (void);

REST_EXPORT
RestXmlNode *rest_xml_node_ref         (RestXmlNode *node);
REST_EXPORT
void         rest_xml_node_unref       (RestXmlNode *node);
REST_EXPORT
const gchar *rest_xml_node_get_attr    (RestXmlNode *node,
                                        const gchar *attr_name);
REST_EXPORT
RestXmlNode *rest_xml_node_find        (RestXmlNode *start,
                                        const gchar *tag);
REST_EXPORT
RestXmlNode *rest_xml_node_add_child   (RestXmlNode *parent,
                                        const char  *tag);
REST_EXPORT
char        *rest_xml_node_print       (RestXmlNode *node);
REST_EXPORT
void         rest_xml_node_add_attr    (RestXmlNode *node,
                                        const char  *attribute,
                                        const char  *value);
REST_EXPORT
void         rest_xml_node_set_content (RestXmlNode *node,
                                        const char  *value);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RestXmlNode, rest_xml_node_unref)

G_END_DECLS
