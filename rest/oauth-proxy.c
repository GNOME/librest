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

#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "oauth-proxy.h"
#include "oauth-proxy-private.h"
#include "oauth-proxy-call.h"

G_DEFINE_TYPE (OAuthProxy, oauth_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_CONSUMER_KEY,
  PROP_CONSUMER_SECRET,
  PROP_TOKEN,
  PROP_TOKEN_SECRET
};

static RestProxyCall *
_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (OAUTH_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);

  return call;
}

static void
oauth_proxy_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_CONSUMER_KEY:
    g_value_set_string (value, priv->consumer_key);
    break;
  case PROP_CONSUMER_SECRET:
    g_value_set_string (value, priv->consumer_secret);
    break;
  case PROP_TOKEN:
    g_value_set_string (value, priv->token);
    break;
  case PROP_TOKEN_SECRET:
    g_value_set_string (value, priv->token_secret);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
oauth_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_CONSUMER_KEY:
    if (priv->consumer_key)
      g_free (priv->consumer_key);
    priv->consumer_key = g_value_dup_string (value);
    break;
  case PROP_CONSUMER_SECRET:
    if (priv->consumer_secret)
      g_free (priv->consumer_secret);
    priv->consumer_secret = g_value_dup_string (value);
    break;
  case PROP_TOKEN:
    if (priv->token)
      g_free (priv->token);
    priv->token = g_value_dup_string (value);
    break;
  case PROP_TOKEN_SECRET:
    if (priv->token_secret)
      g_free (priv->token_secret);
    priv->token_secret = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
oauth_proxy_finalize (GObject *object)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  g_free (priv->consumer_key);
  g_free (priv->consumer_secret);
  g_free (priv->token);
  g_free (priv->token_secret);

  if (G_OBJECT_CLASS (oauth_proxy_parent_class)->finalize)
    G_OBJECT_CLASS (oauth_proxy_parent_class)->finalize (object);
}

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

static void
oauth_proxy_class_init (OAuthProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (OAuthProxyPrivate));

  object_class->get_property = oauth_proxy_get_property;
  object_class->set_property = oauth_proxy_set_property;
  object_class->finalize = oauth_proxy_finalize;

  proxy_class->new_call = _new_call;

  pspec = g_param_spec_string ("consumer-key",  "consumer-key",
                               "The consumer key", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_CONSUMER_KEY,
                                   pspec);

  pspec = g_param_spec_string ("consumer-secret",  "consumer-secret",
                               "The consumer secret", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_CONSUMER_SECRET,
                                   pspec);

  pspec = g_param_spec_string ("token",  "token",
                               "The request or access token", NULL,
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_TOKEN,
                                   pspec);

  pspec = g_param_spec_string ("token-secret",  "token-secret",
                               "The request or access token secret", NULL,
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_TOKEN_SECRET,
                                   pspec);

  /* TODO: add enum property for signature method */
}

static void
oauth_proxy_init (OAuthProxy *self)
{
  PROXY_GET_PRIVATE (self)->method = HMAC_SHA1;
}

RestProxy *
oauth_proxy_new (const char *consumer_key,
                 const char *consumer_secret,
                 const gchar *url_format,
                 gboolean binding_required)
{
  return g_object_new (OAUTH_TYPE_PROXY,
                       "consumer-key", consumer_key,
                       "consumer-secret", consumer_secret,
                       "url-format", url_format,
                       "binding-required", binding_required,
                       NULL);
}

RestProxy *
oauth_proxy_new_with_token (const char *consumer_key,
                 const char *consumer_secret,
                 const char *token,
                 const char *token_secret,
                 const gchar *url_format,
                 gboolean binding_required)
{
  return g_object_new (OAUTH_TYPE_PROXY,
                       "consumer-key", consumer_key,
                       "consumer-secret", consumer_secret,
                       "token", token,
                       "token-secret", token_secret,
                       "url-format", url_format,
                       "binding-required", binding_required,
                       NULL);
}

typedef struct {
  OAuthProxyAuthCallback callback;
  gpointer user_data;
} AuthData;

static void
auth_callback (RestProxyCall *call,
               GError        *error,
               GObject       *weak_object,
               gpointer       user_data)
{
  AuthData *data = user_data;
  OAuthProxy *proxy = NULL;
  OAuthProxyPrivate *priv;
  GHashTable *form;

  g_object_get (call, "proxy", &proxy, NULL);
  priv = PROXY_GET_PRIVATE (proxy);

  if (!error) {
    /* TODO: sanity check response */
    form = soup_form_decode (rest_proxy_call_get_payload (call));
    priv->token = g_strdup (g_hash_table_lookup (form, "oauth_token"));
    priv->token_secret = g_strdup (g_hash_table_lookup (form, "oauth_token_secret"));
    g_hash_table_destroy (form);
  }

  data->callback (proxy, error, weak_object, data->user_data);

  g_slice_free (AuthData, data);
  g_object_unref (call);
  g_object_unref (proxy);
}

gboolean
oauth_proxy_auth_step_async (OAuthProxy *proxy,
                             const char *function,
                             OAuthProxyAuthCallback callback,
                             GObject *weak_object,
                             gpointer user_data,
                             GError **error_out)
{
  RestProxyCall *call;
  AuthData *data;

  call = rest_proxy_new_call (REST_PROXY (proxy));
  rest_proxy_call_set_function (call, function);

  data = g_slice_new0 (AuthData);
  data->callback = callback;
  data->user_data = user_data;

  return rest_proxy_call_async (call, auth_callback, weak_object, data, error_out);
  /* TODO: if call_async fails, the call is leaked */
}

/**
 * oauth_proxy_auth_step:
 * @proxy: an #OAuthProxy
 * @function: the function to invoke on the proxy
 *
 * Perform an OAuth authorisation step.  This calls @function and then updates
 * the token and token secret in the proxy.
 *
 * @proxy must not require binding, the function will be invoked using
 * rest_proxy_call_set_function().
 */
gboolean
oauth_proxy_auth_step (OAuthProxy *proxy, const char *function, GError **error)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  RestProxyCall *call;
  GHashTable *form;

  call = rest_proxy_new_call (REST_PROXY (proxy));
  rest_proxy_call_set_function (call, function);

  if (!rest_proxy_call_run (call, NULL, error)) {
    g_object_unref (call);
    return FALSE;
  }

  /* TODO: sanity check response */
  form = soup_form_decode (rest_proxy_call_get_payload (call));
  priv->token = g_strdup (g_hash_table_lookup (form, "oauth_token"));
  priv->token_secret = g_strdup (g_hash_table_lookup (form, "oauth_token_secret"));
  g_hash_table_destroy (form);

  g_object_unref (call);

  return TRUE;
}

/**
 * oauth_proxy_get_token:
 * @proxy: an #OAuthProxy
 *
 * Get the current request or access token.
 *
 * Returns: the token, or %NULL if there is no token yet.  This string is owned
 * by #OAuthProxy and should not be freed.
 */
const char *
oauth_proxy_get_token (OAuthProxy *proxy)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->token;
}

/**
 * oauth_proxy_set_token:
 * @proxy: an #OAuthProxy
 * @token: the access token
 *
 * Set the access token.
 */
void
oauth_proxy_set_token (OAuthProxy *proxy, const char *token)
{
  OAuthProxyPrivate *priv;

  g_return_if_fail (OAUTH_IS_PROXY (proxy));
  priv = PROXY_GET_PRIVATE (proxy);

  if (priv->token)
    g_free (priv->token);

  priv->token = g_strdup (token);
}

/**
 * oauth_proxy_get_token_secret:
 * @proxy: an #OAuthProxy
 *
 * Get the current request or access token secret.
 *
 * Returns: the token secret, or %NULL if there is no token secret yet.  This
 * string is owned by #OAuthProxy and should not be freed.
 */
const char *
oauth_proxy_get_token_secret (OAuthProxy *proxy)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->token_secret;
}

/**
 * oauth_proxy_set_token_secret:
 * @proxy: an #OAuthProxy
 * @token_secret: the access token secret
 *
 * Set the access token secret.
 */
void
oauth_proxy_set_token_secret (OAuthProxy *proxy, const char *token_secret)
{
  OAuthProxyPrivate *priv;

  g_return_if_fail (OAUTH_IS_PROXY (proxy));
  priv = PROXY_GET_PRIVATE (proxy);

  if (priv->token_secret)
    g_free (priv->token_secret);

  priv->token_secret = g_strdup (token_secret);
}
