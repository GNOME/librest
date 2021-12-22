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

#pragma once

#include <glib-object.h>
#include <rest/rest-xml-node.h>

G_BEGIN_DECLS

#define REST_TYPE_XML_PARSER rest_xml_parser_get_type()
G_DECLARE_DERIVABLE_TYPE (RestXmlParser, rest_xml_parser, REST, XML_PARSER, GObject)

/**
 * RestXmlParser:
 *
 * A Xml Parser for Rest Responses
 */

struct _RestXmlParserClass {
  GObjectClass parent_class;
};

RestXmlParser *rest_xml_parser_new             (void);
RestXmlNode   *rest_xml_parser_parse_from_data (RestXmlParser *parser,
                                                const gchar   *data,
                                                goffset        len);

G_END_DECLS
