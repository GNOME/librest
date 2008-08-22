#include <libxml/xmlreader.h>
#include "rest-xml-parser.h"

G_DEFINE_TYPE (RestXmlParser, rest_xml_parser, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_XML_PARSER, RestXmlParserPrivate))

typedef struct _RestXmlParserPrivate RestXmlParserPrivate;

struct _RestXmlParserPrivate {
  xmlTextReaderPtr reader;
};

static void
rest_xml_parser_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_xml_parser_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_xml_parser_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (rest_xml_parser_parent_class)->dispose)
    G_OBJECT_CLASS (rest_xml_parser_parent_class)->dispose (object);
}

static void
rest_xml_parser_finalize (GObject *object)
{
  RestXmlParserPrivate *priv = GET_PRIVATE (object);

  xmlFreeTextReader (priv->reader);

  if (G_OBJECT_CLASS (rest_xml_parser_parent_class)->finalize)
    G_OBJECT_CLASS (rest_xml_parser_parent_class)->finalize (object);
}

static void
rest_xml_parser_class_init (RestXmlParserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RestXmlParserPrivate));

  object_class->get_property = rest_xml_parser_get_property;
  object_class->set_property = rest_xml_parser_set_property;
  object_class->dispose = rest_xml_parser_dispose;
  object_class->finalize = rest_xml_parser_finalize;
}

static void
rest_xml_parser_init (RestXmlParser *self)
{
}

static RestXmlNode *
rest_xml_node_prepend (RestXmlNode *cur_node, RestXmlNode *new_node)
{
  g_assert (new_node->next == NULL);
  new_node->next = cur_node;

  return new_node;
}

RestXmlNode *
rest_xml_node_new ()
{
  RestXmlNode *node;

  node = g_slice_new0 (RestXmlNode);
  node->children = g_hash_table_new (g_str_hash, g_str_equal);
  node->attrs = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_free);

  return node;
}

void
rest_xml_node_free (RestXmlNode *node)
{
  GList *l;

  for (l = g_hash_table_get_values (node->children); l; l = l->next)
  {
    rest_xml_node_free ((RestXmlNode *)l->data);
  }

  g_hash_table_unref (node->children);
  g_hash_table_unref (node->attrs);
  g_free (node->name);
  g_free (node->content);
  g_slice_free (RestXmlNode, node);
}

RestXmlParser *
rest_xml_parser_new (void)
{
  return g_object_new (REST_TYPE_XML_PARSER, NULL);
}

RestXmlNode *
rest_xml_parser_parse_from_data (RestXmlParser *parser, 
                                 const gchar   *data,
                                 gssize         len)
{
  RestXmlParserPrivate *priv = GET_PRIVATE (parser);
  RestXmlNode *cur_node = NULL;
  RestXmlNode *new_node = NULL;
  RestXmlNode *tmp_node = NULL;
  RestXmlNode *root_node = NULL;
  const gchar *name = NULL;
  const gchar *attr_name = NULL;
  const gchar *attr_value = NULL;
  GQueue *nodes = NULL;
  gint res = 0;

  priv->reader = xmlReaderForMemory (data,
                                     len,
                                     NULL, /* URL? */
                                     NULL, /* encoding */
                                     XML_PARSE_RECOVER | XML_PARSE_NOCDATA);
  nodes = g_queue_new ();

  while ((res = xmlTextReaderRead (priv->reader)) == 1)
  {
    switch (xmlTextReaderNodeType (priv->reader))
    {
      case XML_READER_TYPE_ELEMENT:
        /* Lookup the "name" for the tag */
        name = xmlTextReaderConstLocalName (priv->reader);
        g_debug (G_STRLOC ": Opening tag: %s", name);

        /* Create our new node for this tag */

        new_node = rest_xml_node_new ();
        new_node->name = g_strdup (name);

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
          tmp_node = g_hash_table_lookup (cur_node->children, name);

          if (tmp_node)
          {
            g_debug (G_STRLOC ": Existing node found for this name. "
                              "Prepending to the list.");
            g_hash_table_insert (cur_node->children, 
                                 (gchar *)new_node->name,
                                 rest_xml_node_prepend (tmp_node, new_node));
          } else {
            g_debug (G_STRLOC ": Unseen name. Adding to the children table.");
            g_hash_table_insert (cur_node->children,
                                 (gchar *)new_node->name,
                                 new_node);
          }
        }

        /* 
         * Check for empty element. If empty we needn't worry about children
         * or text and thus we don't need to update the stack or state
         */
        if (xmlTextReaderIsEmptyElement (priv->reader)) 
        {
          g_debug (G_STRLOC ": We have an empty element. No children or text.");
        } else {
          g_debug (G_STRLOC ": Non-empty element found."
                            "  Pushing to stack and updating current state.");
          g_queue_push_head (nodes, new_node);
          cur_node = new_node;
        }

        /* 
         * Check if we have attributes. These get stored in the node's attrs
         * hash table.
         */
        if (xmlTextReaderHasAttributes (priv->reader))
        {
          xmlTextReaderMoveToFirstAttribute (priv->reader);

          do {
            attr_name = xmlTextReaderConstLocalName (priv->reader);
            attr_value = xmlTextReaderConstValue (priv->reader);
            g_hash_table_insert (new_node->attrs,
                                 g_strdup (attr_name),
                                 g_strdup (attr_value));

            g_debug (G_STRLOC ": Attribute found: %s = %s",
                     attr_name, 
                     attr_value);

          } while ((res = xmlTextReaderMoveToNextAttribute (priv->reader)) == 1);
        }

        break;
      case XML_READER_TYPE_END_ELEMENT:
        g_debug (G_STRLOC ": Closing tag: %s", 
                 xmlTextReaderConstLocalName (priv->reader));

        g_debug (G_STRLOC ": Popping from stack and updating state.");
        g_queue_pop_head (nodes);
        cur_node = (RestXmlNode *)g_queue_peek_head (nodes);

        if (cur_node)
        {
          g_debug (G_STRLOC ": Head is now %s", cur_node->name);
        } else {
          g_debug (G_STRLOC ": At the top level");
        }
        break;
      case XML_READER_TYPE_TEXT:
        cur_node->content = g_strdup (xmlTextReaderConstValue (priv->reader));
        g_debug (G_STRLOC ": Text content found: %s",
                 cur_node->content);
      default:
        g_debug (G_STRLOC ": Found unknown content with type: 0x%x", 
                 xmlTextReaderNodeType (priv->reader));
        break;
    }
  }

  xmlTextReaderClose (priv->reader);
  return root_node;
}


