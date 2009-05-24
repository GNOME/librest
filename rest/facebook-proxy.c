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
#include <string.h>
#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "facebook-proxy.h"
#include "facebook-proxy-private.h"
#include "facebook-proxy-call.h"

G_DEFINE_TYPE (FacebookProxy, facebook_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_API_KEY,
  PROP_APP_SECRET,
  PROP_SESSION_KEY,
};

static RestProxyCall *
_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (FACEBOOK_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);

  return call;
}

static void
facebook_proxy_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_API_KEY:
    g_value_set_string (value, priv->api_key);
    break;
  case PROP_APP_SECRET:
    g_value_set_string (value, priv->app_secret);
    break;
  case PROP_SESSION_KEY:
    g_value_set_string (value, priv->session_key);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
facebook_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_API_KEY:
    if (priv->api_key)
      g_free (priv->api_key);
    priv->api_key = g_value_dup_string (value);
    break;
  case PROP_APP_SECRET:
    if (priv->app_secret)
      g_free (priv->app_secret);
    priv->app_secret = g_value_dup_string (value);
    break;
  case PROP_SESSION_KEY:
    if (priv->session_key);
      g_free (priv->session_key);
    priv->session_key = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
facebook_proxy_finalize (GObject *object)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  g_free (priv->api_key);
  g_free (priv->app_secret);
  g_free (priv->session_key);

  G_OBJECT_CLASS (facebook_proxy_parent_class)->finalize (object);
}

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

static void
facebook_proxy_class_init (FacebookProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (FacebookProxyPrivate));

  object_class->get_property = facebook_proxy_get_property;
  object_class->set_property = facebook_proxy_set_property;
  object_class->finalize = facebook_proxy_finalize;

  proxy_class->new_call = _new_call;

  pspec = g_param_spec_string ("api-key",  "api-key",
                               "The API key", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_API_KEY,
                                   pspec);

  pspec = g_param_spec_string ("app-secret",  "app-secret",
                               "The application secret", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_APP_SECRET,
                                   pspec);

  pspec = g_param_spec_string ("session-key",  "session-key",
                               "The session key", NULL,
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_SESSION_KEY,
                                   pspec);
}

static void
facebook_proxy_init (FacebookProxy *self)
{
  self->priv = PROXY_GET_PRIVATE (self);
}

RestProxy *
facebook_proxy_new (const char *api_key,
                 const char *app_secret)
{
  return facebook_proxy_new_with_session (api_key,
                                          app_secret,
                                          NULL);
}

RestProxy *
facebook_proxy_new_with_session (const char *api_key,
                             const char *app_secret,
                             const char *session_key)
{
  return g_object_new (FACEBOOK_TYPE_PROXY,
                       "api-key", api_key,
                       "app-secret", app_secret,
                       "session-key", session_key,
                       "url-format", "http://api.facebook.com/restserver.php",
                       "binding-required", FALSE,
                       NULL);
}

/**
 * facebook_proxy_get_api_key:
 * @proxy: an #FacebookProxy
 *
 * Get the API key.
 *
 * Returns: the API key. This string is owned by #FacebookProxy and should not be
 * freed.
 */
const char *
facebook_proxy_get_api_key (FacebookProxy *proxy)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->api_key;
}

/**
 * facebook_proxy_get_app_secret:
 * @proxy: an #FacebookProxy
 *
 * Get the application secret for authentication.
 *
 * Returns: the application secret. This string is owned by #FacebookProxy and should not be
 * freed.
 */
const char *
facebook_proxy_get_app_secret (FacebookProxy *proxy)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->app_secret;
}

void
facebook_proxy_set_app_secret (FacebookProxy *proxy, const char *secret)
{
  FacebookProxyPrivate *priv;

  g_return_if_fail (FACEBOOK_IS_PROXY (proxy));
  priv = PROXY_GET_PRIVATE (proxy);

  if (priv->app_secret)
    g_free (priv->app_secret);

  priv->app_secret = g_strdup (secret);
}

/**
 * facebook_proxy_get_session_key:
 * @proxy: an #FacebookProxy
 *
 * Get the current session key
 *
 * Returns: the session key, or %NULL if there is no session yet.  This string is owned
 * by #FacebookProxy and should not be freed.
 */
const char *
facebook_proxy_get_session_key (FacebookProxy *proxy)
{
  FacebookProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->session_key;
}

/**
 * facebook_proxy_set_session_key:
 * @proxy: an #FacebookProxy
 * @session_key: the session key
 *
 * Set the token.
 */
void
facebook_proxy_set_session_key (FacebookProxy *proxy, const char *session_key)
{
  FacebookProxyPrivate *priv;

  g_return_if_fail (FACEBOOK_IS_PROXY (proxy));
  priv = PROXY_GET_PRIVATE (proxy);

  if (priv->session_key)
    g_free (priv->session_key);

  priv->session_key = g_strdup (session_key);
}

char *
facebook_proxy_sign (FacebookProxy *proxy, GHashTable *params)
{
  FacebookProxyPrivate *priv;
  GString *s;
  GList *keys;
  char *md5;

  g_return_val_if_fail (FACEBOOK_IS_PROXY (proxy), NULL);
  g_return_val_if_fail (params, NULL);

  priv = PROXY_GET_PRIVATE (proxy);

  s = g_string_new (NULL);

  keys = g_hash_table_get_keys (params);
  keys = g_list_sort (keys, (GCompareFunc)strcmp);

  while (keys) {
    const char *key;
    const char *value;

    key = keys->data;
    value = g_hash_table_lookup (params, key);

    g_string_append_printf (s, "%s=%s", key, value);

    keys = keys->next;
  }

  g_string_append (s, priv->app_secret);

  md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5, s->str, s->len);

  g_string_free (s, TRUE);

  return md5;
}

char *
facebook_proxy_build_login_url (FacebookProxy *proxy, const char *token)
{
  SoupURI *uri;
  GHashTable *params;
  char *s;

  g_return_val_if_fail (FACEBOOK_IS_PROXY (proxy), NULL);
  g_return_val_if_fail (token, NULL);

  uri = soup_uri_new ("http://facebook.com/login.php/");
  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "api_key", proxy->priv->api_key);
  g_hash_table_insert (params, "v", "1.0");
  g_hash_table_insert (params, "auth_token", (gpointer)token);

  soup_uri_set_query_from_form (uri, params);

  s = soup_uri_to_string (uri, FALSE);

  g_hash_table_unref (params);
  soup_uri_free (uri);
  return s;
}
