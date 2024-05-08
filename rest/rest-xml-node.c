/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, 2011, Intel Corporation.
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

#include "rest-xml-node.h"

#define G(x) (gchar *)x

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

void
_rest_xml_node_reverse_children_siblings (RestXmlNode *node)
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

/*
 * _rest_xml_node_prepend:
 * @cur_node: a sibling #RestXmlNode to prepend the new before
 * @new_node: new #RestXmlNode to prepend to the siblings list
 *
 * Prepends new_node to the list of siblings starting at cur_node.
 *
 * Return value: (transfer none): returns new start of the sibling list
 */
RestXmlNode *
_rest_xml_node_prepend (RestXmlNode *cur_node, RestXmlNode *new_node)
{
  g_assert (new_node->next == NULL);
  new_node->next = cur_node;

  return new_node;
}

/*
 * rest_xml_node_append_end:
 * @cur_node: a member of the sibling #RestXmlNode list
 * @new_node: new #RestXmlNode to append to the siblings list
 *
 * Appends new_node to end of the list of siblings containing cur_node.
 */
static void
rest_xml_node_append_end (RestXmlNode *cur_node, RestXmlNode *new_node)
{
  RestXmlNode *tmp = cur_node;

  g_return_if_fail (cur_node);

  while (tmp->next)
    tmp = tmp->next;

  tmp->next = new_node;
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

/*
 * _rest_xml_node_new:
 *
 * Creates a new instance of #RestXmlNode.
 *
 * Return value: (transfer full): newly allocated #RestXmlNode.
 */
RestXmlNode *
_rest_xml_node_new ()
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
 * rest_xml_node_ref: (skip)
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
 * rest_xml_node_unref: (skip)
 * @node: a #RestXmlNode
 *
 * Decreases the reference count of @node. When its reference count drops to 0,
 * the node is finalized (i.e. its memory is freed).
 */
void
rest_xml_node_unref (RestXmlNode *node)
{
  GList *l;
  RestXmlNode *next = NULL;
  g_return_if_fail (node);
  g_return_if_fail (node->ref_count > 0);

  /* Try and unref the chain, this is equivalent to being tail recursively
   * unreffing the next pointer
   */
  while (node && g_atomic_int_dec_and_test (&node->ref_count))
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
  g_return_val_if_fail (attr_name != NULL, NULL);

  return g_hash_table_lookup (node->attrs, attr_name);
}

/**
 * rest_xml_node_find:
 * @start: a #RestXmlNode
 * @tag: the name of a node
 *
 * Searches for the first child node of @start named @tag.
 *
 * Returns: (transfer none): the first child node, or %NULL if it doesn't exist.
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
  g_return_val_if_fail (tag != NULL, NULL);
  g_return_val_if_fail (start->ref_count > 0, NULL);

  tag_interned = g_intern_string (tag);

  g_queue_push_head (&stack, start);

  while ((node = g_queue_pop_head (&stack)) != NULL)
  {
    if ((tmp = g_hash_table_lookup (node->children, tag_interned)) != NULL)
    {
      g_queue_clear (&stack);
      return tmp;
    }

    children = g_hash_table_get_values (node->children);
    for (l = children; l; l = l->next)
    {
      g_queue_push_head (&stack, l->data);
    }
    g_list_free (children);
  }

  g_queue_clear (&stack);
  return NULL;
}

/**
 * rest_xml_node_print:
 * @node: #RestXmlNode
 *
 * Recursively outputs given node and it's children.
 *
 * Return value: (transfer full): xml string representing the node.
 */
char *
rest_xml_node_print (RestXmlNode *node)
{
  GHashTableIter iter;
  gpointer       key, value;
  GList          *attrs = NULL;
  GList          *children = NULL;
  GList          *l;
  GString        *xml = g_string_new (NULL);
  RestXmlNode   *n;

  g_string_append (xml, "<");
  g_string_append (xml, node->name);

  g_hash_table_iter_init (&iter, node->attrs);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      char *attr = g_strdup_printf ("%s=\'%s\'", (char *)key, (char *)value);
      attrs = g_list_prepend (attrs, attr);
    }

  attrs = g_list_sort (attrs, (GCompareFunc) g_strcmp0);
  for (l = attrs; l; l = l->next)
    {
      const char *attr = (const char *) l->data;
      g_string_append_printf (xml, " %s", attr);
    }

  g_string_append (xml, ">");

  g_hash_table_iter_init (&iter, node->children);
  while (g_hash_table_iter_next (&iter, &key, &value))
    children = g_list_prepend (children, key);

  children = g_list_sort (children, (GCompareFunc) g_strcmp0);
  for (l = children; l; l = l->next)
    {
      const char *name = (const char *) l->data;
      RestXmlNode *value = (RestXmlNode *) g_hash_table_lookup (node->children, name);
      char *child = rest_xml_node_print ((RestXmlNode *) value);

      g_string_append (xml, child);
      g_free (child);
    }

  if (node->content)
    g_string_append (xml, node->content);

  g_string_append_printf (xml, "</%s>", node->name);

  for (n = node->next; n; n = n->next)
    {
      char *sibling = rest_xml_node_print (n);

      g_string_append (xml, sibling);
      g_free (sibling);
    }

  g_list_free_full (attrs, g_free);
  g_list_free (children);
  return g_string_free (xml, FALSE);
}

/**
 * rest_xml_node_add_child:
 * @parent: parent #RestXmlNode, or %NULL for the top-level node
 * @tag: name of the child node
 *
 * Adds a new node to the given parent node; to create the top-level node,
 * parent should be %NULL.
 *
 * Return value: (transfer none): the newly added #RestXmlNode; the node object
 * is owned by, and valid for the life time of, the #RestXmlCreator.
 */
RestXmlNode *
rest_xml_node_add_child (RestXmlNode *parent, const char *tag)
{
  RestXmlNode *node;
  char        *escaped;

  g_return_val_if_fail (tag && *tag, NULL);

  escaped = g_markup_escape_text (tag, -1);

  node = _rest_xml_node_new ();
  node->name = (char *) g_intern_string (escaped);

  if (parent)
    {
      RestXmlNode *tmp_node;

      tmp_node = g_hash_table_lookup (parent->children, node->name);

      if (tmp_node)
        {
          rest_xml_node_append_end (tmp_node, node);
        }
      else
        {
          g_hash_table_insert (parent->children, G(node->name), node);
        }
    }

  g_free (escaped);

  return node;
}

/**
 * rest_xml_node_add_attr:
 * @node: #RestXmlNode to add attribute to
 * @attribute: name of the attribute
 * @value: value to set attribute to
 *
 * Adds attribute to the given node.
 */
void
rest_xml_node_add_attr (RestXmlNode *node,
                        const char  *attribute,
                        const char  *value)
{
  g_return_if_fail (node);
  g_return_if_fail (attribute);
  g_return_if_fail (*attribute);
  g_return_if_fail (value);
  g_return_if_fail (*value);

  g_hash_table_insert (node->attrs,
                       g_markup_escape_text (attribute, -1),
                       g_markup_escape_text (value, -1));
}

/**
 * rest_xml_node_set_content:
 * @node: #RestXmlNode to set content
 * @value: the content
 *
 * Sets content for the given node.
 */
void
rest_xml_node_set_content (RestXmlNode *node, const char *value)
{
  g_return_if_fail (node);
  g_return_if_fail (value);
  g_return_if_fail (*value);

  g_free (node->content);
  node->content = g_markup_escape_text (value, -1);
}
