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

#include <libsoup/soup.h>

#include "rest-marshal.h"
#include "rest-proxy-auth-private.h"
#include "rest-proxy.h"
#include "rest-private.h"


typedef struct _RestProxyPrivate RestProxyPrivate;

struct _RestProxyPrivate {
  gchar *url_format;
  gchar *url;
  gchar *user_agent;
  gchar *username;
  gchar *password;
  gboolean binding_required;
  SoupSession *session;
  gboolean disable_cookies;
  char *ssl_ca_file;
#ifndef WITH_SOUP_2
  gboolean ssl_strict;
#endif
};


G_DEFINE_TYPE_WITH_PRIVATE (RestProxy, rest_proxy, G_TYPE_OBJECT)

enum
{
  PROP0 = 0,
  PROP_URL_FORMAT,
  PROP_BINDING_REQUIRED,
  PROP_USER_AGENT,
  PROP_DISABLE_COOKIES,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_SSL_STRICT,
  PROP_SSL_CA_FILE
};

static gboolean       _rest_proxy_simple_run_valist (RestProxy  *proxy,
                                                     char      **payload,
                                                     goffset    *len,
                                                     GError    **error,
                                                     va_list     params);
static RestProxyCall *_rest_proxy_new_call          (RestProxy  *proxy);
static gboolean       _rest_proxy_bind_valist       (RestProxy  *proxy,
                                                     va_list     params);

/**
 * rest_proxy_error_quark:
 *
 * Registers an error quark for #RestProxy errors.
 *
 * Returns: the error quark
 **/
G_DEFINE_QUARK (rest-proxy-error-quark, rest_proxy_error)

static void
rest_proxy_get_property (GObject   *object,
                         guint      property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  RestProxy *self = REST_PROXY (object);
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  switch (property_id) {
    case PROP_URL_FORMAT:
      g_value_set_string (value, priv->url_format);
      break;
    case PROP_BINDING_REQUIRED:
      g_value_set_boolean (value, priv->binding_required);
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, priv->user_agent);
      break;
    case PROP_DISABLE_COOKIES:
      g_value_set_boolean (value, priv->disable_cookies);
      break;
    case PROP_USERNAME:
      g_value_set_string (value, priv->username);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_SSL_STRICT: {
#ifdef WITH_SOUP_2
      gboolean ssl_strict;
      g_object_get (G_OBJECT(priv->session),
                    "ssl-strict", &ssl_strict,
                    NULL);
      g_value_set_boolean (value, ssl_strict);
#else
      g_value_set_boolean (value, priv->ssl_strict);
#endif
      break;
    }
    case PROP_SSL_CA_FILE:
      g_value_set_string (value, priv->ssl_ca_file);
      break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  RestProxy *self = REST_PROXY (object);
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  switch (property_id) {
    case PROP_URL_FORMAT:
      g_free (priv->url_format);
      priv->url_format = g_value_dup_string (value);

      /* Clear the cached url */
      g_free (priv->url);
      priv->url = NULL;
      break;
    case PROP_BINDING_REQUIRED:
      priv->binding_required = g_value_get_boolean (value);

      /* Clear cached url */
      g_free (priv->url);
      priv->url = NULL;
      break;
    case PROP_USER_AGENT:
      g_free (priv->user_agent);
      priv->user_agent = g_value_dup_string (value);
      break;
    case PROP_DISABLE_COOKIES:
      priv->disable_cookies = g_value_get_boolean (value);
      break;
    case PROP_USERNAME:
      g_free (priv->username);
      priv->username = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_SSL_STRICT:
#ifdef WITH_SOUP_2
      g_object_set (G_OBJECT(priv->session),
                    "ssl-strict", g_value_get_boolean (value),
                    NULL);
#else
      priv->ssl_strict = g_value_get_boolean (value);
#endif
      break;
    case PROP_SSL_CA_FILE:
      g_free(priv->ssl_ca_file);
      priv->ssl_ca_file = g_value_dup_string (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_dispose (GObject *object)
{
  RestProxy *self = REST_PROXY (object);
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  g_clear_object (&priv->session);

  G_OBJECT_CLASS (rest_proxy_parent_class)->dispose (object);
}

#ifdef WITH_SOUP_2
/* Note: authentication on Session level got removed from libsoup3. This is
 * contained in the #RestCall now
 */
static void
authenticate (RestProxy   *self,
              SoupMessage *msg,
              SoupAuth    *soup_auth,
              gboolean     retrying,
              SoupSession *session)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  g_assert (REST_IS_PROXY (self));

  if (retrying)
    return;

  soup_auth_authenticate (soup_auth, priv->username, priv->password);
}
#endif

static void
rest_proxy_constructed (GObject *object)
{
  RestProxy *self = REST_PROXY (object);
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  if (!priv->disable_cookies) {
    SoupSessionFeature *cookie_jar =
      (SoupSessionFeature *)soup_cookie_jar_new ();
    soup_session_add_feature (priv->session, cookie_jar);
    g_object_unref (cookie_jar);
  }

  if (REST_DEBUG_ENABLED(PROXY)) {
#ifdef WITH_SOUP_2
    SoupSessionFeature *logger = (SoupSessionFeature*)soup_logger_new (SOUP_LOGGER_LOG_BODY, 0);
#else
    SoupSessionFeature *logger = (SoupSessionFeature*)soup_logger_new (SOUP_LOGGER_LOG_HEADERS);
#endif
    soup_session_add_feature (priv->session, logger);
    g_object_unref (logger);
  }

#ifdef WITH_SOUP_2
  /* session lifetime is same as self, no need to keep signalid */
  g_signal_connect_swapped (priv->session, "authenticate",
                            G_CALLBACK(authenticate), object);
#endif
}

static void
rest_proxy_finalize (GObject *object)
{
  RestProxy *self = REST_PROXY (object);
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);

  g_free (priv->url);
  g_free (priv->url_format);
  g_free (priv->user_agent);
  g_free (priv->username);
  g_free (priv->password);
  g_free (priv->ssl_ca_file);

  G_OBJECT_CLASS (rest_proxy_parent_class)->finalize (object);
}

static void
rest_proxy_class_init (RestProxyClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);

  _rest_setup_debugging ();

  object_class->get_property = rest_proxy_get_property;
  object_class->set_property = rest_proxy_set_property;
  object_class->dispose = rest_proxy_dispose;
  object_class->constructed = rest_proxy_constructed;
  object_class->finalize = rest_proxy_finalize;

  proxy_class->simple_run_valist = _rest_proxy_simple_run_valist;
  proxy_class->new_call = _rest_proxy_new_call;
  proxy_class->bind_valist = _rest_proxy_bind_valist;

  pspec = g_param_spec_string ("url-format", 
                               "url-format",
                               "Format string for the RESTful url",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, 
                                   PROP_URL_FORMAT,
                                   pspec);

  pspec = g_param_spec_boolean ("binding-required",
                                "binding-required",
                                "Whether the URL format requires binding",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_BINDING_REQUIRED,
                                   pspec);

  pspec = g_param_spec_string ("user-agent",
                               "user-agent",
                               "The User-Agent of the client",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_USER_AGENT,
                                   pspec);

  pspec = g_param_spec_boolean ("disable-cookies",
                                "disable-cookies",
                                "Whether to disable cookie support",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class,
                                   PROP_DISABLE_COOKIES,
                                   pspec);

  pspec = g_param_spec_string ("username",
                               "username",
                               "The username for authentication",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_USERNAME,
                                   pspec);

  pspec = g_param_spec_string ("password",
                               "password",
                               "The password for authentication",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_PASSWORD,
                                   pspec);

  pspec = g_param_spec_boolean ("ssl-strict",
                                "Strictly validate SSL certificates",
                                "Whether certificate errors should be considered a connection error",
                                TRUE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_SSL_STRICT,
                                   pspec);

  pspec = g_param_spec_string ("ssl-ca-file",
                               "SSL CA file",
                               "File containing SSL CA certificates.",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_SSL_CA_FILE,
                                   pspec);
}

static gboolean
transform_ssl_ca_file_to_tls_database (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data)
{
  g_value_take_object (to_value,
                       g_tls_file_database_new (g_value_get_string (from_value), NULL));
  return TRUE;
}

static gboolean
transform_tls_database_to_ssl_ca_file (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data)
{
  GTlsDatabase *tls_database;
  char *path = NULL;

  tls_database = g_value_get_object (from_value);
  if (tls_database)
    g_object_get (tls_database, "anchors", &path, NULL);
  g_value_take_string (to_value, path);
  return TRUE;
}

static void
rest_proxy_init (RestProxy *self)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (self);
#ifdef REST_SYSTEM_CA_FILE
  GTlsDatabase *tls_database;
#endif

#ifndef WITH_SOUP_2
  priv->ssl_strict = TRUE;
#endif

  priv->session = soup_session_new ();

#ifdef REST_SYSTEM_CA_FILE
  /* with ssl-strict (defaults TRUE) setting ssl-ca-file forces all
   * certificates to be trusted */
  tls_database = g_tls_file_database_new (REST_SYSTEM_CA_FILE, NULL);
  if (tls_database) {
          g_object_set (priv->session,
                        "tls-database", tls_database,
                        NULL);
          g_object_unref (tls_database);
  }
#endif
  g_object_bind_property_full (self, "ssl-ca-file",
                               priv->session, "tls-database",
                               G_BINDING_BIDIRECTIONAL,
                               transform_ssl_ca_file_to_tls_database,
                               transform_tls_database_to_ssl_ca_file,
                               NULL, NULL);
}

/**
 * rest_proxy_new:
 * @url_format: the endpoint URL
 * @binding_required: whether the URL needs to be bound before calling
 *
 * Create a new #RestProxy for the specified endpoint @url_format, using the
 * "GET" method.
 *
 * Set @binding_required to %TRUE if the URL contains string formatting
 * operations (for example "http://foo.com/%<!-- -->s".  These must be expanded
 * using rest_proxy_bind() before invoking the proxy.
 *
 * Returns: A new #RestProxy.
 */
RestProxy *
rest_proxy_new (const gchar *url_format,
                gboolean     binding_required)
{
  g_return_val_if_fail (url_format != NULL, NULL);

  return g_object_new (REST_TYPE_PROXY,
                       "url-format", url_format,
                       "binding-required", binding_required,
                       NULL);
}

/**
 * rest_proxy_new_with_authentication:
 * @url_format: the endpoint URL
 * @binding_required: whether the URL needs to be bound before calling
 * @username: the username provided by the user or client
 * @password: the password provided by the user or client
 *
 * Create a new #RestProxy for the specified endpoint @url_format, using the
 * "GET" method.
 *
 * Set @binding_required to %TRUE if the URL contains string formatting
 * operations (for example "http://foo.com/%<!-- -->s".  These must be expanded
 * using rest_proxy_bind() before invoking the proxy.
 *
 * Returns: A new #RestProxy.
 */
RestProxy *
rest_proxy_new_with_authentication (const gchar *url_format,
                                    gboolean     binding_required,
                                    const gchar *username,
                                    const gchar *password)
{
  g_return_val_if_fail (url_format != NULL, NULL);
  g_return_val_if_fail (username != NULL, NULL);
  g_return_val_if_fail (password != NULL, NULL);

  return g_object_new (REST_TYPE_PROXY,
                       "url-format", url_format,
                       "binding-required", binding_required,
                       "username", username,
                       "password", password,
                       NULL);
}

static gboolean
_rest_proxy_bind_valist (RestProxy *proxy,
                         va_list    params)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_val_if_fail (proxy != NULL, FALSE);
  g_return_val_if_fail (priv->url_format != NULL, FALSE);
  g_return_val_if_fail (priv->binding_required == TRUE, FALSE);

  g_free (priv->url);

  priv->url = g_strdup_vprintf (priv->url_format, params);

  return TRUE;
}


gboolean
rest_proxy_bind_valist (RestProxy *proxy,
                        va_list    params)
{
  RestProxyClass *proxy_class = REST_PROXY_GET_CLASS (proxy);

  return proxy_class->bind_valist (proxy, params);
}

gboolean
rest_proxy_bind (RestProxy *proxy, ...)
{
  gboolean res;
  va_list params;

  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);

  va_start (params, proxy);
  res = rest_proxy_bind_valist (proxy, params);
  va_end (params);

  return res;
}

void
rest_proxy_set_user_agent (RestProxy  *proxy,
                           const char *user_agent)
{
  g_return_if_fail (REST_IS_PROXY (proxy));
  g_return_if_fail (user_agent != NULL);

  g_object_set (proxy, "user-agent", user_agent, NULL);
}

const gchar *
rest_proxy_get_user_agent (RestProxy *proxy)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);

  return priv->user_agent;
}

/**
 * rest_proxy_add_soup_feature:
 * @proxy: The #RestProxy
 * @feature: A #SoupSessionFeature
 *
 * This method can be used to add specific features to the #SoupSession objects
 * that are used by librest for its HTTP connections. For example, if one needs
 * extensive control over the cookies which are used for the REST HTTP
 * communication, it's possible to get full access to libsoup cookie API by
 * using
 *
 *   <programlisting>
 *   RestProxy *proxy = g_object_new(REST_TYPE_PROXY,
 *                                   "url-format", url,
 *                                   "disable-cookies", TRUE,
 *                                   NULL);
 *   SoupSessionFeature *cookie_jar = SOUP_SESSION_FEATURE(soup_cookie_jar_new ());
 *   rest_proxy_add_soup_feature(proxy, cookie_jar);
 *   </programlisting>
 *
 * Since: 0.7.92
 */
void
rest_proxy_add_soup_feature (RestProxy          *proxy,
                             SoupSessionFeature *feature)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_if_fail (REST_IS_PROXY(proxy));
  g_return_if_fail (feature != NULL);
  g_return_if_fail (priv->session != NULL);

  soup_session_add_feature (priv->session, feature);
}

static RestProxyCall *
_rest_proxy_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (REST_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);

  return call;
}

/**
 * rest_proxy_new_call:
 * @proxy: the #RestProxy
 *
 * Create a new #RestProxyCall for making a call to the web service.  This call
 * is one-shot and should not be re-used for making multiple calls.
 *
 * Returns: (transfer full): a new #RestProxyCall.
 */
RestProxyCall *
rest_proxy_new_call (RestProxy *proxy)
{
  RestProxyClass *proxy_class;

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);

  proxy_class = REST_PROXY_GET_CLASS (proxy);
  return proxy_class->new_call (proxy);
}

gboolean
_rest_proxy_get_binding_required (RestProxy *proxy)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);

  return priv->binding_required;
}

const gchar *
_rest_proxy_get_bound_url (RestProxy *proxy)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);

  if (!priv->url && !priv->binding_required)
    {
      priv->url = g_strdup (priv->url_format);
    }

  return priv->url;
}

static gboolean
_rest_proxy_simple_run_valist (RestProxy *proxy, 
                               gchar     **payload, 
                               goffset   *len,
                               GError   **error,
                               va_list    params)
{
  RestProxyCall *call;
  gboolean ret;

  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);
  g_return_val_if_fail (payload, FALSE);

  call = rest_proxy_new_call (proxy);

  rest_proxy_call_add_params_from_valist (call, params);

  ret = rest_proxy_call_sync (call, error);
  if (ret) {
    *payload = g_strdup (rest_proxy_call_get_payload (call));
    if (len) *len = rest_proxy_call_get_payload_length (call);
  } else {
    *payload = NULL;
    if (len) *len = 0;
  }
 
  g_object_unref (call);

  return ret;
}

gboolean
rest_proxy_simple_run_valist (RestProxy *proxy, 
                              char     **payload, 
                              goffset   *len,
                              GError   **error,
                              va_list    params)
{
  RestProxyClass *proxy_class = REST_PROXY_GET_CLASS (proxy);
  return proxy_class->simple_run_valist (proxy, payload, len, error, params);
}

gboolean
rest_proxy_simple_run (RestProxy *proxy, 
                       gchar    **payload,
                       goffset   *len,
                       GError   **error,
                       ...)
{
  va_list params;
  gboolean ret;

  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);
  g_return_val_if_fail (payload, FALSE);

  va_start (params, error);
  ret = rest_proxy_simple_run_valist (proxy,
                                      payload,
                                      len,
                                      error,
                                      params);
  va_end (params);

  return ret;
}

typedef struct {
  RestMessageFinishedCallback callback;
  gpointer user_data;
} RestMessageQueueData;

#ifdef WITH_SOUP_2
static void
message_finished_cb (SoupSession *session,
                     SoupMessage *message,
                     gpointer     user_data)
{
  RestMessageQueueData *data = user_data;
  GBytes *body;
  GError *error = NULL;

  body = g_bytes_new (message->response_body->data,
                      message->response_body->length);
  data->callback (message, body, error, data->user_data);
  g_free (data);
}
#else
static void
message_send_and_read_ready_cb (GObject      *source,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  SoupSession *session = SOUP_SESSION (source);
  RestMessageQueueData *data = user_data;
  GBytes *body;
  GError *error = NULL;

  body = soup_session_send_and_read_finish (session, result, &error);
  data->callback (soup_session_get_async_result_message (session, result), body, error, data->user_data);
  g_free (data);
}
#endif

void
_rest_proxy_queue_message (RestProxy                  *proxy,
                           SoupMessage                *message,
                           GCancellable               *cancellable,
                           RestMessageFinishedCallback callback,
                           gpointer                    user_data)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);
  RestMessageQueueData *data;

  g_return_if_fail (REST_IS_PROXY (proxy));
  g_return_if_fail (SOUP_IS_MESSAGE (message));

  data = g_new0 (RestMessageQueueData, 1);
  data->callback = callback;
  data->user_data = user_data;

#ifdef WITH_SOUP_2
  soup_session_queue_message (priv->session,
                              message,
                              message_finished_cb,
                              data);
#else
  soup_session_send_and_read_async (priv->session,
                                    message,
                                    G_PRIORITY_DEFAULT,
                                    cancellable,
                                    message_send_and_read_ready_cb,
                                    data);
#endif
}

static void
message_send_ready_cb (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  SoupSession *session = SOUP_SESSION (source);
  GTask *task = user_data;
  GInputStream *stream;
  GError *error = NULL;

  stream = soup_session_send_finish (session, result, &error);
  if (stream)
    g_task_return_pointer (task, stream, g_object_unref);
  else
    g_task_return_error (task, error);
  g_object_unref (task);
}

void
_rest_proxy_send_message_async (RestProxy          *proxy,
                                SoupMessage        *message,
                                GCancellable       *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer            user_data)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);
  GTask *task;

  task = g_task_new (proxy, cancellable, callback, user_data);
  soup_session_send_async (priv->session,
                           message,
#ifndef WITH_SOUP_2
                           G_PRIORITY_DEFAULT,
#endif
                           cancellable,
                           message_send_ready_cb,
                           task);
}

GInputStream *
_rest_proxy_send_message_finish (RestProxy    *proxy,
                                 GAsyncResult *result,
                                 GError      **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

void
_rest_proxy_cancel_message (RestProxy   *proxy,
                            SoupMessage *message)
{
#ifdef WITH_SOUP_2
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);

  g_return_if_fail (REST_IS_PROXY (proxy));
  g_return_if_fail (SOUP_IS_MESSAGE (message));

  soup_session_cancel_message (priv->session,
                               message,
                               SOUP_STATUS_CANCELLED);
#endif
}

GBytes *
_rest_proxy_send_message (RestProxy    *proxy,
                          SoupMessage  *message,
                          GCancellable *cancellable,
                          GError      **error)
{
  RestProxyPrivate *priv = rest_proxy_get_instance_private (proxy);
  GBytes *body;

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);
  g_return_val_if_fail (SOUP_IS_MESSAGE (message), NULL);

#ifdef WITH_SOUP_2
  soup_session_send_message (priv->session, message);
  body = g_bytes_new (message->response_body->data,
                      message->response_body->length);
#else
  body = soup_session_send_and_read (priv->session,
                                     message,
                                     cancellable,
                                     error);
#endif

  return body;
}
