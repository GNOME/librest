/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *          Günther Wagner <info@gunibert.de>
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

#include <string.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy-call.h>
#include "flickr-proxy-call.h"
#include "flickr-proxy.h"
#include "rest/sha1.h"

typedef struct {
  gboolean upload;
} FlickrProxyCallPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FlickrProxyCall, flickr_proxy_call, REST_TYPE_PROXY_CALL)

enum {
  PROP_0,
  PROP_UPLOAD,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
flickr_proxy_call_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  FlickrProxyCall *self = FLICKR_PROXY_CALL (object);
  FlickrProxyCallPrivate *priv = flickr_proxy_call_get_instance_private (self);

  switch (property_id)
    {
    case PROP_UPLOAD:
      priv->upload = g_value_get_boolean (value);
      break;
   default:
     G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static gboolean
_prepare (RestProxyCall  *call,
          GError        **error)
{
  FlickrProxyCall *self = (FlickrProxyCall *)call;
  FlickrProxyCallPrivate *priv = flickr_proxy_call_get_instance_private (self);

  FlickrProxy *proxy = NULL;
  const gchar *token = NULL;
  GHashTable *params;
  char *s;

  g_object_get (self, "proxy", &proxy, NULL);

  if (priv->upload) {
    rest_proxy_bind (REST_PROXY(proxy), "up", "upload");
    rest_proxy_call_set_function (call, NULL);
  } else {
    rest_proxy_bind (REST_PROXY(proxy), "api", "rest");
    rest_proxy_call_add_param (call, "method",
                               rest_proxy_call_get_function (call));
  /* We need to reset the function because Flickr puts the function in the
   * parameters, not in the base URL */
    rest_proxy_call_set_function (call, NULL);
  }

  rest_proxy_call_add_param (call, "api_key", flickr_proxy_get_api_key (proxy));
  token = flickr_proxy_get_token (proxy);

  if (token)
    rest_proxy_call_add_param (call, "auth_token", token);

  /* Get the string params as a hash for signing */
  params = rest_params_as_string_hash_table (rest_proxy_call_get_params (call));
  s = flickr_proxy_sign (proxy, params);
  g_hash_table_unref (params);

  rest_proxy_call_add_param (call, "api_sig", s);
  g_free (s);

  g_object_unref (proxy);

  return TRUE;
}

static void
flickr_proxy_call_class_init (FlickrProxyCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = _prepare;
  object_class->set_property = flickr_proxy_call_set_property;

  /**
   * FlickrProxyCall:upload:
   *
   * Set if the call should be sent to the photo upload endpoint and not the
   * general-purpose endpoint.
   */
  properties [PROP_UPLOAD] =
    g_param_spec_boolean ("upload",
                          "upload",
                          "upload",
                          FALSE,
                          (G_PARAM_WRITABLE |
                           G_PARAM_CONSTRUCT_ONLY |
                           G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
flickr_proxy_call_init (FlickrProxyCall *self)
{
}
