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

/*
 * TODO:
 *
 * Convenience API for authentication so that the user doesn't have to parse the
 * XML themselves.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "flickr-proxy.h"
#include "flickr-proxy-call.h"

typedef struct {
  char *api_key;
  char *shared_secret;
  char *token;
} FlickrProxyPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FlickrProxy, flickr_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_API_KEY,
  PROP_SHARED_SECRET,
  PROP_TOKEN,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

G_DEFINE_QUARK (rest-flickr-proxy-error-quark, flickr_proxy_error)

static RestProxyCall *
_new_call (RestProxy *self)
{
  RestProxyCall *call;

  call = g_object_new (FLICKR_TYPE_PROXY_CALL,
                       "proxy", self,
                       NULL);

  return call;
}

static void
flickr_proxy_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  FlickrProxy *self = FLICKR_PROXY (object);
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_API_KEY:
    g_value_set_string (value, priv->api_key);
    break;
  case PROP_SHARED_SECRET:
    g_value_set_string (value, priv->shared_secret);
    break;
  case PROP_TOKEN:
    g_value_set_string (value, priv->token);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
flickr_proxy_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  FlickrProxy *self = FLICKR_PROXY (object);
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_API_KEY:
    if (priv->api_key)
      g_free (priv->api_key);
    priv->api_key = g_value_dup_string (value);
    break;
  case PROP_SHARED_SECRET:
    if (priv->shared_secret)
      g_free (priv->shared_secret);
    priv->shared_secret = g_value_dup_string (value);
    break;
  case PROP_TOKEN:
    if (priv->token)
      g_free (priv->token);
    priv->token = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
flickr_proxy_finalize (GObject *object)
{
  FlickrProxy *self = FLICKR_PROXY (object);
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  g_clear_pointer (&priv->api_key, g_free);
  g_clear_pointer (&priv->shared_secret, g_free);
  g_clear_pointer (&priv->token, g_free);

  G_OBJECT_CLASS (flickr_proxy_parent_class)->finalize (object);
}

static void
flickr_proxy_class_init (FlickrProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);

  object_class->get_property = flickr_proxy_get_property;
  object_class->set_property = flickr_proxy_set_property;
  object_class->finalize = flickr_proxy_finalize;

  proxy_class->new_call = _new_call;

  properties [PROP_API_KEY] =
    g_param_spec_string ("api-key",
                         "api-key",
                         "The API key",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_SHARED_SECRET] =
    g_param_spec_string ("shared-secret",
                         "shared-secret",
                         "The shared secret",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_TOKEN] =
    g_param_spec_string ("token",
                         "token",
                         "The request or access token",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
flickr_proxy_init (FlickrProxy *self)
{
}

RestProxy *
flickr_proxy_new (const char *api_key,
                  const char *shared_secret)
{
  return flickr_proxy_new_with_token (api_key,
                                      shared_secret,
                                      NULL);
}

RestProxy *
flickr_proxy_new_with_token (const char *api_key,
                             const char *shared_secret,
                             const char *token)
{
  return g_object_new (FLICKR_TYPE_PROXY, 
                       "api-key", api_key,
                       "shared-secret", shared_secret,
                       "token", token,
                       "url-format", "https://%s.flickr.com/services/%s/",
                       "binding-required", TRUE,
                       NULL);
}

/**
 * flickr_proxy_get_api_key:
 * @proxy: an #FlickrProxy
 *
 * Get the API key.
 *
 * Returns: the API key. This string is owned by #FlickrProxy and should not be
 * freed.
 */
const char *
flickr_proxy_get_api_key (FlickrProxy *self)
{
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);

  return priv->api_key;
}

/**
 * flickr_proxy_get_shared_secret:
 * @proxy: an #FlickrProxy
 *
 * Get the shared secret for authentication.
 *
 * Returns: the shared secret. This string is owned by #FlickrProxy and should not be
 * freed.
 */
const char *
flickr_proxy_get_shared_secret (FlickrProxy *self)
{
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);

  return priv->shared_secret;
}

/**
 * flickr_proxy_get_token:
 * @proxy: an #FlickrProxy
 *
 * Get the current token.
 *
 * Returns: (nullable): the token, or %NULL if there is no token yet.
 * This string is owned by #FlickrProxy and should not be freed.
 */
const char *
flickr_proxy_get_token (FlickrProxy *self)
{
  FlickrProxyPrivate *priv = flickr_proxy_get_instance_private (self);

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);

  return priv->token;
}

/**
 * flickr_proxy_set_token:
 * @proxy: an #FlickrProxy
 * @token: the access token
 *
 * Set the token.
 */
void
flickr_proxy_set_token (FlickrProxy *self,
                        const char  *token)
{
  FlickrProxyPrivate *priv;

  g_return_if_fail (FLICKR_IS_PROXY (self));

  priv = flickr_proxy_get_instance_private (self);

  if (g_strcmp0 (priv->token, token) != 0)
    {
      g_clear_pointer (&priv->token, g_free);
      priv->token = g_strdup (token);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TOKEN]);
    }
}

/**
 * flickr_proxy_sign:
 * @proxy: an #FlickrProxy
 * @params: (element-type utf8 utf8): the request parameters
 *
 * Get the md5 checksum of the request.
 *
 * Returns: The md5 checksum of the request
 */
char *
flickr_proxy_sign (FlickrProxy *self,
                   GHashTable  *params)
{
  FlickrProxyPrivate *priv;
  GList *keys;
  char *md5;
  GChecksum *checksum;

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);
  g_return_val_if_fail (params, NULL);

  priv = flickr_proxy_get_instance_private (self);

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (guchar *)priv->shared_secret, -1);

  keys = g_hash_table_get_keys (params);
  keys = g_list_sort (keys, (GCompareFunc)strcmp);

  while (keys) {
    const char *key, *value;

    key = keys->data;
    value = g_hash_table_lookup (params, key);

    g_checksum_update (checksum, (guchar *)key, -1);
    g_checksum_update (checksum, (guchar *)value, -1);

    keys = g_list_delete_link (keys, keys);
  }

  md5 = g_strdup (g_checksum_get_string (checksum));
  g_checksum_free (checksum);

  return md5;
}

char *
flickr_proxy_build_login_url (FlickrProxy *self,
                              const char  *frob,
                              const char  *perms)
{
  FlickrProxyPrivate *priv;
  GUri *uri;
  GHashTable *params;
  char *sig, *s;
  char *query;

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);

  priv = flickr_proxy_get_instance_private (self);

  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "api_key", priv->api_key);
  g_hash_table_insert (params, "perms", (gpointer)perms);

  if (frob)
    g_hash_table_insert (params, "frob", (gpointer)frob);

  sig = flickr_proxy_sign (self, params);
  g_hash_table_insert (params, "api_sig", sig);
  query = soup_form_encode_hash (params);

  uri = g_uri_build (G_URI_FLAGS_ENCODED,
                     "http",
                     NULL,
                     "flickr.com",
                     -1,
                     "/services/auth/",
                     query,
                     NULL);

  s = g_uri_to_string (uri);

  g_free (query);
  g_free (sig);
  g_hash_table_destroy (params);
  g_uri_unref (uri);

  return s;
}

/**
 * flickr_proxy_is_successful:
 * @root: The root node of a parsed Flickr response
 * @error: #GError to set if the response was an error
 *
 * Examines the Flickr response and if it not a successful reply, set @error and
 * return FALSE.
 *
 * Returns: %TRUE if this response is successful, %FALSE otherwise.
 */
gboolean
flickr_proxy_is_successful (RestXmlNode  *root,
                            GError      **error)
{
  RestXmlNode *node;

  g_return_val_if_fail (root, FALSE);

  if (strcmp (root->name, "rsp") != 0) {
    g_set_error (error, FLICKR_PROXY_ERROR, 0,
                 "Unexpected response from Flickr (root node %s)",
                 root->name);
    return FALSE;
  }

  if (strcmp (rest_xml_node_get_attr (root, "stat"), "ok") != 0) {
    node = rest_xml_node_find (root, "err");
    g_set_error_literal (error,FLICKR_PROXY_ERROR,
                         atoi (rest_xml_node_get_attr (node, "code")),
                         rest_xml_node_get_attr (node, "msg"));
    return FALSE;
  }

  return TRUE;
}

/**
 * flickr_proxy_new_upload:
 * @proxy: a valid #FlickrProxy
 *
 * Create a new #RestProxyCall that can be used for uploading.
 *
 * See http://www.flickr.com/services/api/upload.api.html for details on
 * uploading to Flickr.
 *
 * Returns: (type FlickrProxyCall) (transfer full): a new #FlickrProxyCall
 */
RestProxyCall *
flickr_proxy_new_upload (FlickrProxy *self)
{
  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);

  return g_object_new (FLICKR_TYPE_PROXY_CALL,
                       "proxy", self,
                       "upload", TRUE,
                       NULL);
}

/**
 * flickr_proxy_new_upload_for_file:
 * @proxy: a valid #FlickrProxy
 * @filename: the file to upload
 * @error: #GError to set on error

 * Create a new #RestProxyCall that can be used for uploading.  @filename will
 * be set as the "photo" parameter for you, avoiding you from having to open the
 * file and determine the MIME type.
 *
 * Note that this function can in theory block.
 *
 * See http://www.flickr.com/services/api/upload.api.html for details on
 * uploading to Flickr.
 *
 * Returns: (type FlickrProxyCall) (transfer full): a new #FlickrProxyCall
 */
RestProxyCall *
flickr_proxy_new_upload_for_file (FlickrProxy  *self,
                                  const char   *filename,
                                  GError      **error)
{
  GMappedFile *map;
  GError *err = NULL;
  char *basename = NULL, *content_type = NULL;
  RestParam *param;
  RestProxyCall *call = NULL;

  g_return_val_if_fail (FLICKR_IS_PROXY (self), NULL);
  g_return_val_if_fail (filename, NULL);

  /* Open the file */
  map = g_mapped_file_new (filename, FALSE, &err);
  if (err) {
    g_propagate_error (error, err);
    return NULL;
  }

  /* Get the file information */
  basename = g_path_get_basename (filename);
  content_type = g_content_type_guess (filename,
                                       (const guchar*) g_mapped_file_get_contents (map),
                                       g_mapped_file_get_length (map),
                                       NULL);

  /* Make the call */
  call = flickr_proxy_new_upload (self);
  param = rest_param_new_with_owner ("photo",
                                     g_mapped_file_get_contents (map),
                                     g_mapped_file_get_length (map),
                                     content_type,
                                     basename,
                                     map,
                                     (GDestroyNotify)g_mapped_file_unref);
  rest_proxy_call_add_param_full (call, param);

  g_free (basename);
  g_free (content_type);

  return call;
}

#if BUILD_TESTS
void
test_flickr_error (void)
{
  RestXmlParser *parser;
  RestXmlNode *root;
  GError *error;
  const char test_1[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<rsp stat=\"ok\"><auth></auth></rsp>";
  const char test_2[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<foobar/>";
  const char test_3[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<rsp stat=\"fail\"><err code=\"108\" msg=\"Invalid frob\" /></rsp>";

  parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser, test_1, sizeof (test_1) - 1);
  error = NULL;
  flickr_proxy_is_successful (root, &error);
  g_assert_no_error (error);
  rest_xml_node_unref (root);

  error = NULL;
  root = rest_xml_parser_parse_from_data (parser, test_2, sizeof (test_2) - 1);
  flickr_proxy_is_successful (root, &error);
  g_assert_error (error, FLICKR_PROXY_ERROR, 0);
  g_error_free (error);
  rest_xml_node_unref (root);

  error = NULL;
  root = rest_xml_parser_parse_from_data (parser, test_3, sizeof (test_3) - 1);
  flickr_proxy_is_successful (root, &error);
  g_assert_error (error, FLICKR_PROXY_ERROR, 108);
  g_error_free (error);
  rest_xml_node_unref (root);
}
#endif
