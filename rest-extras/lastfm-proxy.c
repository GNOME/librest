/*
 * librest - RESTful web services access
 * Copyright (c) 2010 Intel Corporation.
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
#include <stdlib.h>
#include <string.h>
#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "lastfm-proxy.h"
#include "lastfm-proxy-call.h"

typedef struct {
  char *api_key;
  char *secret;
  char *session_key;
} LastfmProxyPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (LastfmProxy, lastfm_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_API_KEY,
  PROP_SECRET,
  PROP_SESSION_KEY,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

G_DEFINE_QUARK (rest-lastfm-proxy-error-quark, lastfm_proxy_error)

static RestProxyCall *
_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (LASTFM_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);

  return call;
}

static void
lastfm_proxy_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LastfmProxy *self = LASTFM_PROXY (object);
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_API_KEY:
    g_value_set_string (value, priv->api_key);
    break;
  case PROP_SECRET:
    g_value_set_string (value, priv->secret);
    break;
  case PROP_SESSION_KEY:
    g_value_set_string (value, priv->session_key);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
lastfm_proxy_set_property (GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
  LastfmProxy *self = LASTFM_PROXY (object);
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_API_KEY:
    if (priv->api_key)
      g_free (priv->api_key);
    priv->api_key = g_value_dup_string (value);
    break;
  case PROP_SECRET:
    if (priv->secret)
      g_free (priv->secret);
    priv->secret = g_value_dup_string (value);
    break;
  case PROP_SESSION_KEY:
    if (priv->session_key)
      g_free (priv->session_key);
    priv->session_key = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
lastfm_proxy_finalize (GObject *object)
{
  LastfmProxy *self = LASTFM_PROXY (object);
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  g_free (priv->api_key);
  g_free (priv->secret);
  g_free (priv->session_key);

  G_OBJECT_CLASS (lastfm_proxy_parent_class)->finalize (object);
}

static void
lastfm_proxy_class_init (LastfmProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);

  object_class->get_property = lastfm_proxy_get_property;
  object_class->set_property = lastfm_proxy_set_property;
  object_class->finalize = lastfm_proxy_finalize;

  proxy_class->new_call = _new_call;

  properties [PROP_API_KEY] =
    g_param_spec_string ("api-key",
                         "api-key",
                         "The API key",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_SECRET] =
    g_param_spec_string ("secret",
                         "secret",
                         "The API key secret",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_SESSION_KEY] =
    g_param_spec_string ("session-key",
                         "session-key",
                         "The session key",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
lastfm_proxy_init (LastfmProxy *self)
{
}

RestProxy *
lastfm_proxy_new (const char *api_key,
                  const char *secret)
{
  return lastfm_proxy_new_with_session (api_key,
                                        secret,
                                        NULL);
}

RestProxy *
lastfm_proxy_new_with_session (const char *api_key,
                               const char *secret,
                               const char *session_key)
{
  return g_object_new (LASTFM_TYPE_PROXY,
                       "api-key", api_key,
                       "secret", secret,
                       "session-key", session_key,
                       "url-format", "http://ws.audioscrobbler.com/2.0/",
                       "binding-required", FALSE,
                       NULL);
}

/**
 * lastfm_proxy_get_api_key:
 * @proxy: an #LastfmProxy
 *
 * Get the API key.
 *
 * Returns: the API key. This string is owned by #LastfmProxy and should not be
 * freed.
 */
const char *
lastfm_proxy_get_api_key (LastfmProxy *self)
{
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  g_return_val_if_fail (LASTFM_IS_PROXY (self), NULL);

  return priv->api_key;
}

/**
 * lastfm_proxy_get_secret:
 * @proxy: an #LastfmProxy
 *
 * Get the secret for authentication.
 *
 * Returns: the secret. This string is owned by #LastfmProxy and should not be
 * freed.
 */
const char *
lastfm_proxy_get_secret (LastfmProxy *self)
{
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  g_return_val_if_fail (LASTFM_IS_PROXY (self), NULL);

  return priv->secret;
}

/**
 * lastfm_proxy_get_session_key:
 * @proxy: an #LastfmProxy
 *
 * Get the current session key.
 *
 * Returns: (nullable): the session key, or %NULL if there is no session key yet.
 * This string is owned by #LastfmProxy and should not be freed.
 */
const char *
lastfm_proxy_get_session_key (LastfmProxy *self)
{
  LastfmProxyPrivate *priv = lastfm_proxy_get_instance_private (self);

  g_return_val_if_fail (LASTFM_IS_PROXY (self), NULL);

  return priv->session_key;
}

/**
 * lastfm_proxy_set_session_key:
 * @proxy: an #LastfmProxy
 * @session_key: the access session_key
 *
 * Set the session key.
 */
void
lastfm_proxy_set_session_key (LastfmProxy *self,
                              const char  *session_key)
{
  LastfmProxyPrivate *priv;

  g_return_if_fail (LASTFM_IS_PROXY (self));

  priv = lastfm_proxy_get_instance_private (self);

  if (priv->session_key)
    g_free (priv->session_key);

  priv->session_key = g_strdup (session_key);
}

/**
 * lastfm_proxy_sign:
 * @proxy: an #LastfmProxy
 * @params: (element-type utf8 utf8): the request parameters
 *
 * Get the md5 checksum of the request.
 *
 * Returns: The md5 checksum of the request
 */
char *
lastfm_proxy_sign (LastfmProxy *self,
                   GHashTable  *params)
{
  LastfmProxyPrivate *priv;
  GString *s;
  GList *keys;
  char *md5;

  g_return_val_if_fail (LASTFM_IS_PROXY (self), NULL);
  g_return_val_if_fail (params, NULL);

  priv = lastfm_proxy_get_instance_private (self);

  s = g_string_new (NULL);

  keys = g_hash_table_get_keys (params);
  keys = g_list_sort (keys, (GCompareFunc)strcmp);

  while (keys) {
    const char *key;
    const char *value;

    key = keys->data;
    value = g_hash_table_lookup (params, key);

    g_string_append_printf (s, "%s%s", key, value);

    keys = g_list_delete_link (keys, keys);
  }

  g_string_append (s, priv->secret);

  md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5, s->str, s->len);

  g_string_free (s, TRUE);

  return md5;
}

char *
lastfm_proxy_build_login_url (LastfmProxy *self,
                              const char  *token)
{
  LastfmProxyPrivate *priv;

  g_return_val_if_fail (LASTFM_IS_PROXY (self), NULL);
  g_return_val_if_fail (token, NULL);

  priv = lastfm_proxy_get_instance_private (self);

  return g_strdup_printf ("http://www.last.fm/api/auth/?api_key=%s&token=%s",
                          priv->api_key,
                          token);
}

/**
 * lastfm_proxy_is_successful:
 * @root: The root node of a parsed Lastfm response
 * @error: #GError to set if the response was an error
 *
 * Examines the Lastfm response and if it not a successful reply, set @error and
 * return FALSE.
 *
 * Returns: %TRUE if this response is successful, %FALSE otherwise.
 */
gboolean
lastfm_proxy_is_successful (RestXmlNode  *root,
                            GError      **error)
{
  RestXmlNode *node;

  g_return_val_if_fail (root, FALSE);

  if (strcmp (root->name, "lfm") != 0) {
    g_set_error (error, LASTFM_PROXY_ERROR, 0,
                 "Unexpected response from Lastfm (root node %s)",
                 root->name);
    return FALSE;
  }

  if (strcmp (rest_xml_node_get_attr (root, "status"), "ok") != 0) {
    node = rest_xml_node_find (root, "error");
    g_set_error_literal (error,LASTFM_PROXY_ERROR,
                         atoi (rest_xml_node_get_attr (node, "code")),
                         node->content);
    return FALSE;
  }

  return TRUE;
}

#if BUILD_TESTS
void
test_lastfm_error (void)
{
  RestXmlParser *parser;
  RestXmlNode *root;
  GError *error;
  const char test_1[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<foobar/>";
  const char test_2[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<lfm status=\"failed\"><error code=\"10\">Invalid API Key</error></lfm>";
  const char test_3[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<lfm status=\"ok\">some data</lfm>";

  parser = rest_xml_parser_new ();

  error = NULL;
  root = rest_xml_parser_parse_from_data (parser, test_1, sizeof (test_1) - 1);
  lastfm_proxy_is_successful (root, &error);
  g_assert_error (error, LASTFM_PROXY_ERROR, 0);
  g_error_free (error);
  rest_xml_node_unref (root);

  root = rest_xml_parser_parse_from_data (parser, test_2, sizeof (test_2) - 1);
  error = NULL;
  lastfm_proxy_is_successful (root, &error);
  g_assert_error (error, LASTFM_PROXY_ERROR, 10);
  rest_xml_node_unref (root);

  error = NULL;
  root = rest_xml_parser_parse_from_data (parser, test_3, sizeof (test_3) - 1);
  lastfm_proxy_is_successful (root, &error);
  g_assert_no_error (error);
  g_error_free (error);
  rest_xml_node_unref (root);
}
#endif
