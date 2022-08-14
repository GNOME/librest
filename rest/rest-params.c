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

#include <config.h>
#include <glib-object.h>
#include "rest-params.h"

/**
 * SECTION:rest-params
 * @short_description: Container for call parameters
 * @see_also: #RestParam, #RestProxyCall.
 */

G_DEFINE_BOXED_TYPE (RestParams, rest_params, rest_params_ref, rest_params_unref)

/**
 * rest_params_new:
 *
 * Create a new #RestParams.
 *
 * Returns: A empty #RestParams.
 **/
RestParams *
rest_params_new (void)
{
  RestParams *self;

  self = g_slice_new0 (RestParams);
  self->ref_count = 1;
  self->params = NULL;

  return self;
}

/**
 * rest_params_free:
 * @params: a valid #RestParams
 *
 * Destroy the #RestParams and the #RestParam objects that it contains.
 **/
void
rest_params_free (RestParams *self)
{
  g_assert (self);
  g_assert_cmpint (self->ref_count, ==, 0);

  g_list_free_full (g_steal_pointer (&self->params), (GDestroyNotify) rest_param_unref);

  g_slice_free (RestParams, self);
}

/**
 * rest_params_copy:
 * @self: a #RestParams
 *
 * Makes a deep copy of a #RestParams.
 *
 * Returns: (transfer full): A newly created #RestParams with the same
 *   contents as @self
 */
RestParams *
rest_params_copy (RestParams *self)
{
  RestParams *copy;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  copy = rest_params_new ();
  copy->params = g_list_copy_deep (self->params, (GCopyFunc) rest_param_ref, NULL);

  return copy;
}

/**
 * rest_params_ref:
 * @self: A #RestParams
 *
 * Increments the reference count of @self by one.
 *
 * Returns: (transfer full): @self
 */
RestParams *
rest_params_ref (RestParams *self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

/**
 * rest_params_unref:
 * @self: A #RestParams
 *
 * Decrements the reference count of @self by one, freeing the structure when
 * the reference count reaches zero.
 */
void
rest_params_unref (RestParams *self)
{
  g_return_if_fail (self);
  g_return_if_fail (self->ref_count);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    rest_params_free (self);
}

/**
 * rest_params_add:
 * @params: a valid #RestParams
 * @param: (transfer full): a valid #RestParam
 *
 * Add @param to @params.
 **/
void
rest_params_add (RestParams *self,
                 RestParam  *param)
{
  g_return_if_fail (self);
  g_return_if_fail (param);

  self->params = g_list_append (self->params, param);
}

static gint
rest_params_find (gconstpointer self,
                  gconstpointer name)
{
  const RestParam *e = self;
  const char *n = name;
  const char *n2 = rest_param_get_name ((RestParam *)e);

  if (g_strcmp0 (n2, n) == 0) return 0;
  return -1;
}

/**
 * rest_params_get:
 * @params: a valid #RestParams
 * @name: a parameter name
 *
 * Return the #RestParam called @name, or %NULL if it doesn't exist.
 *
 * Returns: (transfer none) (nullable): a #RestParam or %NULL if the name
 * doesn't exist
 **/
RestParam *
rest_params_get (RestParams *self,
                 const char *name)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (name, NULL);

  return g_list_find_custom (self->params, name, rest_params_find)->data;
}

/**
 * rest_params_remove:
 * @params: a valid #RestParams
 * @name: a parameter name
 *
 * Remove the #RestParam called @name.
 **/
void
rest_params_remove (RestParams *self,
                    const char *name)
{
  GList *elem = NULL;

  g_return_if_fail (self);
  g_return_if_fail (name);

  elem = g_list_find_custom (self->params, name, rest_params_find);
  self->params = g_list_remove (self->params, elem->data);
}

/**
 * rest_params_are_strings:
 * @params: a valid #RestParams
 *
 * Checks if the parameters are all simple strings (have a content type of
 * "text/plain").
 *
 * Returns: %TRUE if all of the parameters are simple strings, %FALSE otherwise.
 **/
gboolean
rest_params_are_strings (RestParams *self)
{
  g_return_val_if_fail (self, FALSE);

  for (GList *cur = self->params; cur; cur = g_list_next (cur))
    {
      if (!rest_param_is_string (cur->data))
        return FALSE;
    }

  return TRUE;
}

/**
 * rest_params_as_string_hash_table:
 * @self: a valid #RestParams
 *
 * Create a new #GHashTable which contains the name and value of all string
 * (content type of text/plain) parameters.
 *
 * The values are owned by the #RestParams, so don't destroy the #RestParams
 * before the hash table.
 *
 * Returns: (element-type utf8 Rest.Param) (transfer container): a new #GHashTable.
 **/
GHashTable *
rest_params_as_string_hash_table (RestParams *self)
{
  GHashTable *strings;

  g_return_val_if_fail (self, NULL);

  strings = g_hash_table_new (g_str_hash, g_str_equal);

  for (GList *cur = self->params; cur; cur = g_list_next (cur))
    {
      if (rest_param_is_string (cur->data))
        g_hash_table_insert (strings, (gpointer)rest_param_get_name (cur->data), (gpointer)rest_param_get_content (cur->data));
    }

  return strings;
}

/**
 * rest_params_iter_init:
 * @iter: an uninitialized #RestParamsIter
 * @params: a valid #RestParams
 *
 * Initialize a parameter iterator over @params. Modifying @params after calling
 * this function invalidates the returned iterator.
 * |[
 * RestParamsIter iter;
 * const char *name;
 * RestParam *param;
 *
 * rest_params_iter_init (&iter, params);
 * while (rest_params_iter_next (&iter, &name, &param)) {
 *   /&ast; do something with name and param &ast;/
 * }
 * ]|
 **/
void
rest_params_iter_init (RestParamsIter *iter,
                       RestParams     *params)
{
  g_return_if_fail (iter);
  g_return_if_fail (params);

  iter->params = params;
  iter->position = -1;
}

/**
 * rest_params_iter_next:
 * @iter: an initialized #RestParamsIter
 * @name: (out) (optional) (nullable) (transfer none): a location to store the name,
 * or %NULL
 * @param: (out) (optional) (nullable) (transfer none): a location to store the
 * #RestParam, or %NULL
 *
 * Advances @iter and retrieves the name and/or parameter that are now pointed
 * at as a result of this advancement.  If %FALSE is returned, @name and @param
 * are set to %NULL and the iterator becomes invalid.
 *
 * Returns: %FALSE if the end of the #RestParams has been reached, %TRUE otherwise.
 **/
gboolean
rest_params_iter_next (RestParamsIter  *iter,
                       const char     **name,
                       RestParam      **param)
{
  GList *cur = NULL;

  g_return_val_if_fail (iter, FALSE);

  iter->position++;
  cur = g_list_nth (iter->params->params, iter->position);

  if (cur == NULL)
    {
      if (param)
        *param = NULL;
      if (name)
        *name = NULL;
      return FALSE;
    }

  if (param)
    *param = cur->data;
  if (name)
    *name = rest_param_get_name (*param);

  return TRUE;
}

