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

#include <string.h>
#include <libsoup/soup.h>
#include <rest/rest-private.h>
#include <rest/rest-proxy-call.h>
#include "lastfm-proxy-call.h"
#include "lastfm-proxy.h"
#include "rest/sha1.h"

G_DEFINE_TYPE (LastfmProxyCall, lastfm_proxy_call, REST_TYPE_PROXY_CALL)

static gboolean
_prepare (RestProxyCall *call, GError **error)
{
  LastfmProxy *proxy = NULL;
  GHashTable *params;
  const gchar *session_key;
  char *s;

  g_object_get (call, "proxy", &proxy, NULL);

  rest_proxy_call_add_params (call,
                              "method", rest_proxy_call_get_function (call),
                              "api_key", lastfm_proxy_get_api_key (proxy),
                              NULL);

  /* Reset function because Lastfm puts the function in the parameters */
  rest_proxy_call_set_function (call, NULL);

  session_key = lastfm_proxy_get_session_key (proxy);
  if (session_key)
    rest_proxy_call_add_param (call, "sk", session_key);

  params = rest_params_as_string_hash_table (rest_proxy_call_get_params (call));
  s = lastfm_proxy_sign (proxy, params);
  g_hash_table_unref (params);
  rest_proxy_call_add_param (call, "api_sig", s);
  g_free (s);

  g_object_unref (proxy);

  return TRUE;
}

static void
lastfm_proxy_call_class_init (LastfmProxyCallClass *klass)
{
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = _prepare;
}

static void
lastfm_proxy_call_init (LastfmProxyCall *self)
{
}
