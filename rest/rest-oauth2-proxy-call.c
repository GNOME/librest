/* rest-oauth2-proxy-call.c
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "rest-oauth2-proxy-call.h"

G_DEFINE_TYPE (RestOAuth2ProxyCall, rest_oauth2_proxy_call, REST_TYPE_PROXY_CALL)

G_DEFINE_QUARK (rest-oauth2-error-quark, rest_oauth2_error)

static gboolean
rest_oauth2_proxy_call_prepare (RestProxyCall  *call,
                                GError        **error)
{
  RestOAuth2ProxyCall *self = (RestOAuth2ProxyCall *)call;
  RestOAuth2Proxy *proxy = NULL;
  g_autoptr(GDateTime) now = NULL;
  GDateTime *expiration_date = NULL;

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY_CALL (call), FALSE);

  g_object_get (call, "proxy", &proxy, NULL);

  now = g_date_time_new_now_local ();
  expiration_date = rest_oauth2_proxy_get_expiration_date (proxy);

  // access token expired -> refresh
  if (g_date_time_compare (now, expiration_date) > 0)
    {
      g_set_error (error,
                   REST_OAUTH2_ERROR,
                   REST_OAUTH2_PROXY_ERROR_ACCESS_TOKEN_EXPIRED,
                   "Access token is expired");
      return FALSE;
    }

  return TRUE;
}

static void
rest_oauth2_proxy_call_class_init (RestOAuth2ProxyCallClass *klass)
{
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = rest_oauth2_proxy_call_prepare;
}

static void
rest_oauth2_proxy_call_init (RestOAuth2ProxyCall *self)
{
}
