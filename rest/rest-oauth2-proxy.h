/* rest-oauth2-proxy.h
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
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

#pragma once

#include <rest/rest-proxy.h>

G_BEGIN_DECLS

#define REST_TYPE_OAUTH2_PROXY (rest_oauth2_proxy_get_type())

G_DECLARE_DERIVABLE_TYPE (RestOAuth2Proxy, rest_oauth2_proxy, REST, OAUTH2_PROXY, RestProxy)

struct _RestOAuth2ProxyClass
{
  RestProxyClass parent_class;

  void (*parse_access_token) (RestOAuth2Proxy *self,
                              GBytes          *payload,
                              GTask           *task);

  gpointer padding[8];
};

typedef enum {
  REST_OAUTH2_ERROR_NO_REFRESH_TOKEN,
  REST_OAUTH2_ERROR_ACCESS_TOKEN_EXPIRED,
} RestOAuth2Error;

#define REST_OAUTH2_ERROR (rest_oauth2_error_quark())
GQuark rest_oauth2_error_quark (void);

RestOAuth2Proxy *rest_oauth2_proxy_new                         (const gchar          *authurl,
                                                                const gchar          *tokenurl,
                                                                const gchar          *redirecturl,
                                                                const gchar          *client_id,
                                                                const gchar          *client_secret,
                                                                const gchar          *baseurl);
gchar           *rest_oauth2_proxy_build_authorization_url     (RestOAuth2Proxy      *self,
                                                                const gchar          *code_challenge,
                                                                const gchar          *scope,
                                                                gchar               **state);
void             rest_oauth2_proxy_fetch_access_token_async    (RestOAuth2Proxy      *self,
                                                                const gchar          *authorization_code,
                                                                const gchar          *code_verifier,
                                                                GCancellable         *cancellable,
                                                                GAsyncReadyCallback   callback,
                                                                gpointer              user_data);
gboolean         rest_oauth2_proxy_fetch_access_token_finish   (RestOAuth2Proxy      *self,
                                                                GAsyncResult         *result,
                                                                GError              **error);
gboolean         rest_oauth2_proxy_refresh_access_token        (RestOAuth2Proxy      *self,
                                                                GError              **error);
void             rest_oauth2_proxy_refresh_access_token_async  (RestOAuth2Proxy      *self,
                                                                GCancellable         *cancellable,
                                                                GAsyncReadyCallback   callback,
                                                                gpointer              user_data);
gboolean         rest_oauth2_proxy_refresh_access_token_finish (RestOAuth2Proxy      *self,
                                                                GAsyncResult         *result,
                                                                GError              **error);
const gchar     *rest_oauth2_proxy_get_auth_url                (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_auth_url                (RestOAuth2Proxy      *self,
                                                                const gchar          *tokenurl);
const gchar     *rest_oauth2_proxy_get_token_url               (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_token_url               (RestOAuth2Proxy      *self,
                                                                const gchar          *tokenurl);
const gchar     *rest_oauth2_proxy_get_redirect_uri            (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_redirect_uri            (RestOAuth2Proxy      *self,
                                                                const gchar          *redirect_uri);
const gchar     *rest_oauth2_proxy_get_client_id               (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_client_id               (RestOAuth2Proxy      *self,
                                                                const gchar          *client_id);
const gchar     *rest_oauth2_proxy_get_client_secret           (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_client_secret           (RestOAuth2Proxy      *self,
                                                                const gchar          *client_secret);
const gchar     *rest_oauth2_proxy_get_access_token            (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_access_token            (RestOAuth2Proxy      *self,
                                                                const gchar          *access_token);
const gchar     *rest_oauth2_proxy_get_refresh_token           (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_refresh_token           (RestOAuth2Proxy      *self,
                                                                const gchar          *refresh_token);
GDateTime       *rest_oauth2_proxy_get_expiration_date         (RestOAuth2Proxy      *self);
void             rest_oauth2_proxy_set_expiration_date         (RestOAuth2Proxy      *self,
                                                                GDateTime            *expiration_date);

G_END_DECLS
