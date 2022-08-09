/* rest-oauth2-proxy-call.c
 *
 * Copyright 2021-2022 Günther Wagner <info@gunibert.de>
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
 */

#include "rest-oauth2-proxy-call.h"
#include "rest-oauth2-proxy.h"

G_DEFINE_TYPE (RestOAuth2ProxyCall, rest_oauth2_proxy_call, REST_TYPE_PROXY_CALL)

static gboolean
rest_oauth2_proxy_call_prepare (RestProxyCall  *call,
                                GError        **error)
{
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
                   REST_OAUTH2_ERROR_ACCESS_TOKEN_EXPIRED,
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
