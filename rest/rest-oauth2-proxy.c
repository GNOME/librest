/* rest-oauth2-proxy.c
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

#include "rest-oauth2-proxy.h"
#include "rest-oauth2-proxy-call.h"
#include "rest-utils.h"
#include "rest-private.h"
#include <json-glib/json-glib.h>

typedef struct
{
  gchar *authurl;
  gchar *tokenurl;
  gchar *redirect_uri;
  gchar *client_id;
  gchar *client_secret;

  gchar *access_token;
  gchar *refresh_token;

  GDateTime *expiration_date;
} RestOAuth2ProxyPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (RestOAuth2Proxy, rest_oauth2_proxy, REST_TYPE_PROXY)

G_DEFINE_QUARK (rest-oauth2-error-quark, rest_oauth2_error)

enum {
  PROP_0,
  PROP_AUTH_URL,
  PROP_TOKEN_URL,
  PROP_REDIRECT_URI,
  PROP_CLIENT_ID,
  PROP_CLIENT_SECRET,
  PROP_ACCESS_TOKEN,
  PROP_REFRESH_TOKEN,
  PROP_EXPIRATION_DATE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
rest_oauth2_proxy_parse_access_token (RestOAuth2Proxy *self,
                                      GBytes          *payload,
                                      GTask           *task)
{
  g_autoptr(JsonParser) parser = NULL;
  g_autoptr(GError) error = NULL;
  JsonNode *root;
  JsonObject *root_object;
  const gchar *data;
  gsize size;
  gint expires_in;
  gint created_at;

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));
  if (!payload)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Empty payload");
      return;
    }

  data = g_bytes_get_data (payload, &size);

  parser = json_parser_new ();
  json_parser_load_from_data (parser, data, size, &error);
  if (error != NULL)
    {
      g_task_return_error (task, error);
      return;
    }

  root = json_parser_get_root (parser);
  root_object = json_node_get_object (root);

  if (json_object_has_member (root_object, "access_token"))
    rest_oauth2_proxy_set_access_token (self, json_object_get_string_member (root_object, "access_token"));
  if (json_object_has_member (root_object, "refresh_token"))
    rest_oauth2_proxy_set_refresh_token (self, json_object_get_string_member (root_object, "refresh_token"));

  if (json_object_has_member (root_object, "expires_in") && json_object_has_member (root_object, "created_at"))
    {
      expires_in = json_object_get_int_member (root_object, "expires_in");
      created_at = json_object_get_int_member (root_object, "created_at");

      rest_oauth2_proxy_set_expiration_date (self, g_date_time_new_from_unix_local (created_at+expires_in));
    }
  else if (json_object_has_member (root_object, "expires_in"))
    {
      g_autoptr(GDateTime) now = g_date_time_new_now_utc ();
      expires_in = json_object_get_int_member (root_object, "expires_in");
      rest_oauth2_proxy_set_expiration_date (self, g_date_time_add_seconds (now, expires_in));
    }

  g_task_return_boolean (task, TRUE);
}

RestProxyCall *
rest_oauth2_proxy_new_call (RestProxy *proxy)
{
  RestOAuth2Proxy *self = (RestOAuth2Proxy *)proxy;
  RestProxyCall *call;
  g_autofree gchar *auth = NULL;

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  auth = g_strdup_printf ("Bearer %s", rest_oauth2_proxy_get_access_token (self));

  call = g_object_new (REST_TYPE_OAUTH2_PROXY_CALL, "proxy", proxy, NULL);
  rest_proxy_call_add_header (call, "Authorization", auth);

  return call;
}

/**
 * rest_oauth2_proxy_new:
 *
 * Create a new #RestOAuth2Proxy.
 *
 * Returns: (transfer full): a newly created #RestOAuth2Proxy
 */
RestOAuth2Proxy *
rest_oauth2_proxy_new (const gchar *authurl,
                       const gchar *tokenurl,
                       const gchar *redirecturl,
                       const gchar *client_id,
                       const gchar *client_secret,
                       const gchar *baseurl)
{
  return g_object_new (REST_TYPE_OAUTH2_PROXY,
                       "url-format", baseurl,
                       "auth-url", authurl,
                       "token-url", tokenurl,
                       "redirect-uri", redirecturl,
                       "client-id", client_id,
                       "client-secret", client_secret,
                       NULL);
}

static void
rest_oauth2_proxy_finalize (GObject *object)
{
  RestOAuth2Proxy *self = (RestOAuth2Proxy *)object;
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_clear_pointer (&priv->authurl, g_free);
  g_clear_pointer (&priv->tokenurl, g_free);
  g_clear_pointer (&priv->redirect_uri, g_free);
  g_clear_pointer (&priv->client_id, g_free);
  g_clear_pointer (&priv->client_secret, g_free);
  g_clear_pointer (&priv->access_token, g_free);
  g_clear_pointer (&priv->refresh_token, g_free);
  g_clear_pointer (&priv->expiration_date, g_date_time_unref);

  G_OBJECT_CLASS (rest_oauth2_proxy_parent_class)->finalize (object);
}

static void
rest_oauth2_proxy_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  RestOAuth2Proxy *self = REST_OAUTH2_PROXY (object);

  switch (prop_id)
    {
    case PROP_AUTH_URL:
      g_value_set_string (value, rest_oauth2_proxy_get_auth_url (self));
      break;
    case PROP_TOKEN_URL:
      g_value_set_string (value, rest_oauth2_proxy_get_token_url (self));
      break;
    case PROP_REDIRECT_URI:
      g_value_set_string (value, rest_oauth2_proxy_get_redirect_uri (self));
      break;
    case PROP_CLIENT_ID:
      g_value_set_string (value, rest_oauth2_proxy_get_client_id (self));
      break;
    case PROP_CLIENT_SECRET:
      g_value_set_string (value, rest_oauth2_proxy_get_client_secret (self));
      break;
    case PROP_ACCESS_TOKEN:
      g_value_set_string (value, rest_oauth2_proxy_get_access_token (self));
      break;
    case PROP_REFRESH_TOKEN:
      g_value_set_string (value, rest_oauth2_proxy_get_refresh_token (self));
      break;
    case PROP_EXPIRATION_DATE:
      g_value_set_boxed (value, rest_oauth2_proxy_get_expiration_date (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
rest_oauth2_proxy_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  RestOAuth2Proxy *self = REST_OAUTH2_PROXY (object);

  switch (prop_id)
    {
    case PROP_AUTH_URL:
      rest_oauth2_proxy_set_auth_url (self, g_value_get_string (value));
      break;
    case PROP_TOKEN_URL:
      rest_oauth2_proxy_set_token_url (self, g_value_get_string (value));
      break;
    case PROP_REDIRECT_URI:
      rest_oauth2_proxy_set_redirect_uri (self, g_value_get_string (value));
      break;
    case PROP_CLIENT_ID:
      rest_oauth2_proxy_set_client_id (self, g_value_get_string (value));
      break;
    case PROP_CLIENT_SECRET:
      rest_oauth2_proxy_set_client_secret (self, g_value_get_string (value));
      break;
    case PROP_ACCESS_TOKEN:
      rest_oauth2_proxy_set_access_token (self, g_value_get_string (value));
      break;
    case PROP_REFRESH_TOKEN:
      rest_oauth2_proxy_set_refresh_token (self, g_value_get_string (value));
      break;
    case PROP_EXPIRATION_DATE:
      rest_oauth2_proxy_set_expiration_date (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
rest_oauth2_proxy_class_init (RestOAuth2ProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestOAuth2ProxyClass *oauth2_class = REST_OAUTH2_PROXY_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);

  object_class->finalize = rest_oauth2_proxy_finalize;
  object_class->get_property = rest_oauth2_proxy_get_property;
  object_class->set_property = rest_oauth2_proxy_set_property;
  oauth2_class->parse_access_token = rest_oauth2_proxy_parse_access_token;
  proxy_class->new_call = rest_oauth2_proxy_new_call;

  properties [PROP_AUTH_URL] =
    g_param_spec_string ("auth-url",
                         "AuthUrl",
                         "AuthUrl",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_TOKEN_URL] =
    g_param_spec_string ("token-url",
                         "TokenUrl",
                         "TokenUrl",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_REDIRECT_URI] =
    g_param_spec_string ("redirect-uri",
                         "RedirectUri",
                         "RedirectUri",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_CLIENT_ID] =
    g_param_spec_string ("client-id",
                         "ClientId",
                         "ClientId",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_CLIENT_SECRET] =
    g_param_spec_string ("client-secret",
                         "ClientSecret",
                         "ClientSecret",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_ACCESS_TOKEN] =
    g_param_spec_string ("access-token",
                         "AccessToken",
                         "AccessToken",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_REFRESH_TOKEN] =
    g_param_spec_string ("refresh-token",
                         "RefreshToken",
                         "RefreshToken",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_EXPIRATION_DATE] =
    g_param_spec_boxed ("expiration-date",
                        "ExpirationDate",
                        "ExpirationDate",
                        G_TYPE_DATE_TIME,
                        (G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
rest_oauth2_proxy_init (RestOAuth2Proxy *self)
{
}

/**
 * rest_oauth2_proxy_build_authorization_url:
 * @self: a #RestOAuth2Proxy
 * @code_challenge: the code challenge (see #RestPkceCodeChallenge)
 * @scope: (nullable): the requesting scope of the resource
 * @state: (out): a CRSF token which should be verified from the redirect_uri
 *
 *
 * Returns: (transfer full): the authorization url which should be shown in a WebView in order to accept/decline the request
 * to authorize the application
 *
 * Since: 0.8
 */
gchar *
rest_oauth2_proxy_build_authorization_url (RestOAuth2Proxy  *self,
                                           const gchar      *code_challenge,
                                           const gchar      *scope,
                                           gchar           **state)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);
  g_autoptr(GHashTable) params = NULL;
  g_autoptr(GUri) auth = NULL;
  g_autoptr(GUri) authorization_url = NULL;
  g_autofree gchar *params_string = NULL;

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  if (state != NULL)
    *state = random_string (10);
  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "response_type", "code");
  g_hash_table_insert (params, "client_id", priv->client_id);
  g_hash_table_insert (params, "redirect_uri", priv->redirect_uri);
  if (state != NULL)
    g_hash_table_insert (params, "state", *state);
  g_hash_table_insert (params, "code_challenge", (gchar *)code_challenge);
  g_hash_table_insert (params, "code_challenge_method", "S256");
  if (scope)
    g_hash_table_insert (params, "scope", (gchar *)scope);

  params_string = soup_form_encode_hash (params);
  auth = g_uri_parse (priv->authurl, G_URI_FLAGS_NONE, NULL);
  authorization_url = g_uri_build (G_URI_FLAGS_ENCODED,
                                   g_uri_get_scheme (auth),
                                   NULL,
                                   g_uri_get_host (auth),
                                   g_uri_get_port (auth),
                                   g_uri_get_path (auth),
                                   params_string,
                                   NULL);
  return g_uri_to_string (authorization_url);
}

static void
rest_oauth2_proxy_fetch_access_token_cb (SoupMessage *msg,
                                         GBytes      *body,
                                         GError      *error,
                                         gpointer     user_data)
{
  g_autoptr(GTask) task = user_data;
  RestOAuth2Proxy *self;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  REST_OAUTH2_PROXY_GET_CLASS (self)->parse_access_token (self, body, g_steal_pointer (&task));
}

void
rest_oauth2_proxy_fetch_access_token_async (RestOAuth2Proxy     *self,
                                            const gchar         *authorization_code,
                                            const gchar         *code_verifier,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);
  g_autoptr(SoupMessage) msg = NULL;
  g_autoptr(GTask) task = NULL;
  g_autoptr(GHashTable) params = NULL;

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));
  g_return_if_fail (authorization_code != NULL);

  task = g_task_new (self, cancellable, callback, user_data);
  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "client_id", priv->client_id);
  g_hash_table_insert (params, "client_secret", priv->client_secret);
  g_hash_table_insert (params, "grant_type", "authorization_code");
  g_hash_table_insert (params, "code", (gchar *)authorization_code);
  g_hash_table_insert (params, "redirect_uri", priv->redirect_uri);
  g_hash_table_insert (params, "code_verifier", (gchar *)code_verifier);

#if WITH_SOUP_2
  msg = soup_form_request_new_from_hash (SOUP_METHOD_POST, priv->tokenurl, params);
#else
  msg = soup_message_new_from_encoded_form (SOUP_METHOD_POST, priv->tokenurl, soup_form_encode_hash (params));
#endif

  _rest_proxy_queue_message (REST_PROXY (self),
#if WITH_SOUP_2
                             g_steal_pointer (&msg),
#else
                             msg,
#endif
                             cancellable, rest_oauth2_proxy_fetch_access_token_cb, g_steal_pointer (&task));

}

/**
 * rest_oauth2_proxy_fetch_access_token_finish:
 * @self: an #RestOauth2Proxy
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Returns:
 */
gboolean
rest_oauth2_proxy_fetch_access_token_finish (RestOAuth2Proxy  *self,
                                             GAsyncResult     *result,
                                             GError          **error)
{
  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

gboolean
rest_oauth2_proxy_refresh_access_token (RestOAuth2Proxy *self,
                                        GError         **error)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);
  g_autoptr(SoupMessage) msg = NULL;
  g_autoptr(GHashTable) params = NULL;
  g_autoptr(GTask) task = NULL;
  GBytes *payload;

  task = g_task_new (self, NULL, NULL, NULL);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), FALSE);

  if (priv->refresh_token == NULL)
    {
      *error = g_error_new (REST_OAUTH2_ERROR,
                            REST_OAUTH2_ERROR_NO_REFRESH_TOKEN,
                            "No refresh token available");
      return FALSE;
    }

  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "client_id", priv->client_id);
  g_hash_table_insert (params, "refresh_token", priv->refresh_token);
  g_hash_table_insert (params, "redirect_uri", priv->redirect_uri);
  g_hash_table_insert (params, "grant_type", "refresh_token");

#if WITH_SOUP_2
  msg = soup_form_request_new_from_hash (SOUP_METHOD_POST, priv->tokenurl, params);
#else
  msg = soup_message_new_from_encoded_form (SOUP_METHOD_POST, priv->tokenurl, soup_form_encode_hash (params));
#endif
  payload = _rest_proxy_send_message (REST_PROXY (self), msg, NULL, error);
  if (error && *error)
    {
      return FALSE;
    }

  REST_OAUTH2_PROXY_GET_CLASS (self)->parse_access_token (self, payload, g_steal_pointer (&task));
  return TRUE;
}

static void
rest_oauth2_proxy_refresh_access_token_cb (SoupMessage *msg,
                                           GBytes      *payload,
                                           GError      *error,
                                           gpointer     user_data)
{
  g_autoptr(GTask) task = user_data;
  RestOAuth2Proxy *self;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  REST_OAUTH2_PROXY_GET_CLASS (self)->parse_access_token (self, payload, g_steal_pointer (&task));
}

void
rest_oauth2_proxy_refresh_access_token_async (RestOAuth2Proxy     *self,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);
  g_autoptr(SoupMessage) msg = NULL;
  g_autoptr(GHashTable) params = NULL;
  g_autoptr(GTask) task = NULL;

  task = g_task_new (self, cancellable, callback, user_data);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (priv->refresh_token == NULL)
    {
      g_task_return_new_error (task,
                               REST_OAUTH2_ERROR,
                               REST_OAUTH2_ERROR_NO_REFRESH_TOKEN,
                               "No refresh token available");
      return;
    }

  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "client_id", priv->client_id);
  g_hash_table_insert (params, "refresh_token", priv->refresh_token);
  g_hash_table_insert (params, "redirect_uri", priv->redirect_uri);
  g_hash_table_insert (params, "grant_type", "refresh_token");

#if WITH_SOUP_2
  msg = soup_form_request_new_from_hash (SOUP_METHOD_POST, priv->tokenurl, params);
#else
  msg = soup_message_new_from_encoded_form (SOUP_METHOD_POST, priv->tokenurl, soup_form_encode_hash (params));
#endif
  _rest_proxy_queue_message (REST_PROXY (self),
#if WITH_SOUP_2
                             g_steal_pointer (&msg),
#else
                             msg,
#endif
                             cancellable,
                             rest_oauth2_proxy_refresh_access_token_cb,
                             g_steal_pointer (&task));
}

/**
 * rest_oauth2_proxy_refresh_access_token_finish:
 * @self: an #RestOauth2Proxy
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Returns:
 */
gboolean
rest_oauth2_proxy_refresh_access_token_finish (RestOAuth2Proxy  *self,
                                               GAsyncResult     *result,
                                               GError          **error)
{
  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

const gchar *
rest_oauth2_proxy_get_auth_url (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->authurl;
}

void
rest_oauth2_proxy_set_auth_url (RestOAuth2Proxy *self,
                                const gchar     *authurl)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->authurl, authurl) != 0)
    {
      g_clear_pointer (&priv->authurl, g_free);
      priv->authurl = g_strdup (authurl);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_AUTH_URL]);
    }
}

const gchar *
rest_oauth2_proxy_get_token_url (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->tokenurl;
}

void
rest_oauth2_proxy_set_token_url (RestOAuth2Proxy *self,
                                 const gchar     *tokenurl)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->tokenurl, tokenurl) != 0)
    {
      g_clear_pointer (&priv->tokenurl, g_free);
      priv->tokenurl = g_strdup (tokenurl);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TOKEN_URL]);
    }
}

const gchar *
rest_oauth2_proxy_get_redirect_uri (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->redirect_uri;
}

void
rest_oauth2_proxy_set_redirect_uri (RestOAuth2Proxy *self,
                                    const gchar     *redirect_uri)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->redirect_uri, redirect_uri) != 0)
    {
      g_clear_pointer (&priv->redirect_uri, g_free);
      priv->redirect_uri = g_strdup (redirect_uri);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_REDIRECT_URI]);
    }
}

const gchar *
rest_oauth2_proxy_get_client_id (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->client_id;
}

void
rest_oauth2_proxy_set_client_id (RestOAuth2Proxy *self,
                                 const gchar     *client_id)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->client_id, client_id) != 0)
    {
      g_clear_pointer (&priv->client_id, g_free);
      priv->client_id = g_strdup (client_id);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CLIENT_ID]);
    }
}

const gchar *
rest_oauth2_proxy_get_client_secret (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->client_secret;
}

void
rest_oauth2_proxy_set_client_secret (RestOAuth2Proxy *self,
                                     const gchar     *client_secret)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->client_secret, client_secret) != 0)
    {
      g_clear_pointer (&priv->client_secret, g_free);
      priv->client_secret = g_strdup (client_secret);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CLIENT_SECRET]);
    }
}

const gchar *
rest_oauth2_proxy_get_access_token (RestOAuth2Proxy *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->access_token;
}

void
rest_oauth2_proxy_set_access_token (RestOAuth2Proxy *self,
                                    const gchar     *access_token)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->access_token, access_token) != 0)
    {
      g_clear_pointer (&priv->access_token, g_free);
      priv->access_token = g_strdup (access_token);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCESS_TOKEN]);
    }
}

const gchar *
rest_oauth2_proxy_get_refresh_token (RestOAuth2Proxy  *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->refresh_token;
}

void
rest_oauth2_proxy_set_refresh_token (RestOAuth2Proxy *self,
                                     const gchar     *refresh_token)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  if (g_strcmp0 (priv->refresh_token, refresh_token) != 0)
    {
      g_clear_pointer (&priv->refresh_token, g_free);
      priv->refresh_token = g_strdup (refresh_token);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_REFRESH_TOKEN]);
    }
}

GDateTime *
rest_oauth2_proxy_get_expiration_date (RestOAuth2Proxy  *self)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);
  g_return_val_if_fail (REST_IS_OAUTH2_PROXY (self), NULL);

  return priv->expiration_date;
}

void
rest_oauth2_proxy_set_expiration_date (RestOAuth2Proxy *self,
                                       GDateTime       *expiration_date)
{
  RestOAuth2ProxyPrivate *priv = rest_oauth2_proxy_get_instance_private (self);

  g_return_if_fail (REST_IS_OAUTH2_PROXY (self));

  g_clear_pointer (&priv->expiration_date, g_date_time_unref);
  priv->expiration_date = g_date_time_ref (expiration_date);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPIRATION_DATE]);
}
