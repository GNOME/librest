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

#include <rest/rest-proxy-call.h>
#include "oauth-proxy-call.h"
#include "oauth-proxy-private.h"

G_DEFINE_TYPE (OAuthProxyCall, oauth_proxy_call, REST_TYPE_PROXY_CALL)

static char *
sign_plaintext (OAuthProxyPrivate *priv)
{
  return g_strdup_printf ("%s&%s", priv->consumer_secret, priv->token_secret ?: "");
}

static gboolean
_prepare (RestProxyCall *call, GError **error)
{
  OAuthProxy *proxy = NULL;
  OAuthProxyPrivate *priv;
  char *s;
  
  g_object_get (call, "proxy", &proxy, NULL);
  priv = PROXY_GET_PRIVATE (proxy);

  rest_proxy_call_add_param (call, "oauth_version", "1.0");

  s = g_strdup_printf ("%lli", (long long int) time (NULL));
  rest_proxy_call_add_param (call, "oauth_timestamp", s);
  g_free (s);

  s = g_strdup_printf ("%u", g_random_int ());
  rest_proxy_call_add_param (call, "oauth_nonce", s);
  g_free (s);

  rest_proxy_call_add_param (call, "oauth_consumer_key", priv->consumer_key);

  if (priv->token)
    rest_proxy_call_add_param (call, "oauth_token", priv->token);

  rest_proxy_call_add_param (call, "oauth_signature_method", "PLAINTEXT");
  
  s = sign_plaintext (priv);
  rest_proxy_call_add_param (call, "oauth_signature", s);
  g_free (s);

  g_object_unref (proxy);

  return TRUE;
}

static void
oauth_proxy_call_class_init (OAuthProxyCallClass *klass)
{
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = _prepare;
}

static void
oauth_proxy_call_init (OAuthProxyCall *self)
{
}
