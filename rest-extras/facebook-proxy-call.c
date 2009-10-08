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

#include <string.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy-call.h>
#include "facebook-proxy-call.h"
#include "facebook-proxy-private.h"
#include "rest/rest-proxy-call-private.h"
#include "rest/sha1.h"

G_DEFINE_TYPE (FacebookProxyCall, facebook_proxy_call, REST_TYPE_PROXY_CALL)

static gboolean
_prepare (RestProxyCall *call, GError **error)
{
  FacebookProxy *proxy = NULL;
  FacebookProxyPrivate *priv;
  RestProxyCallPrivate *call_priv;
  char *s;

  g_object_get (call, "proxy", &proxy, NULL);
  priv = PROXY_GET_PRIVATE (proxy);
  call_priv = call->priv;

  /* First reset the URL because Facebook puts the function in the parameters */
  g_object_get (proxy, "url-format", &call_priv->url, NULL);

  rest_proxy_call_add_params (call,
                              "method", call_priv->function,
                              "api_key", priv->api_key,
                              "v", "1.0",
                              NULL);

  if (priv->session_key) {
    GTimeVal time;
    g_get_current_time (&time);
    s = g_strdup_printf ("%ld.%ld", time.tv_sec, time.tv_usec);
    rest_proxy_call_add_param (call, "call_id", s);
    g_free (s);

    rest_proxy_call_add_param (call, "session_key", priv->session_key);
  }

  s = facebook_proxy_sign (proxy, call_priv->params);
  rest_proxy_call_add_param (call, "sig", s);
  g_free (s);

  g_object_unref (proxy);

  return TRUE;
}

static void
facebook_proxy_call_class_init (FacebookProxyCallClass *klass)
{
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = _prepare;
}

static void
facebook_proxy_call_init (FacebookProxyCall *self)
{
}

#if BUILD_TESTS
#warning TODO facebook signature test cases
#endif
