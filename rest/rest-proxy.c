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
#if WITH_GNOME
#include <libsoup/soup-gnome.h>
#endif

#include "rest-proxy.h"
#include "rest-private.h"

G_DEFINE_TYPE (RestProxy, rest_proxy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_PROXY, RestProxyPrivate))

typedef struct _RestProxyPrivate RestProxyPrivate;

struct _RestProxyPrivate {
  gchar *url_format;
  gchar *url;
  gchar *user_agent;
  gboolean binding_required;
  SoupSession *session;
  SoupSession *session_sync;
};

enum
{
  PROP0 = 0,
  PROP_URL_FORMAT,
  PROP_BINDING_REQUIRED,
  PROP_USER_AGENT
};

static gboolean _rest_proxy_simple_run_valist (RestProxy *proxy, 
                                               char     **payload, 
                                               goffset   *len,
                                               GError   **error,
                                               va_list    params);

static RestProxyCall *_rest_proxy_new_call (RestProxy *proxy);

static gboolean _rest_proxy_bind_valist (RestProxy *proxy,
                                         va_list    params);

GQuark
rest_proxy_error_quark (void)
{
  return g_quark_from_static_string ("rest-proxy-error-quark");
}

static void
rest_proxy_get_property (GObject   *object,
                         guint      property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  RestProxyPrivate *priv = GET_PRIVATE (object);

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
  RestProxyPrivate *priv = GET_PRIVATE (object);

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
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_dispose (GObject *object)
{
  RestProxyPrivate *priv = GET_PRIVATE (object);

  if (priv->session)
  {
    g_object_unref (priv->session);
    priv->session = NULL;
  }

  if (priv->session_sync)
  {
    g_object_unref (priv->session_sync);
    priv->session_sync = NULL;
  }

  G_OBJECT_CLASS (rest_proxy_parent_class)->dispose (object);
}

static void
rest_proxy_finalize (GObject *object)
{
  RestProxyPrivate *priv = GET_PRIVATE (object);

  g_free (priv->url);
  g_free (priv->url_format);
  g_free (priv->user_agent);

  G_OBJECT_CLASS (rest_proxy_parent_class)->finalize (object);
}

static void
rest_proxy_class_init (RestProxyClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);

  _rest_setup_debugging ();

  g_type_class_add_private (klass, sizeof (RestProxyPrivate));

  object_class->get_property = rest_proxy_get_property;
  object_class->set_property = rest_proxy_set_property;
  object_class->dispose = rest_proxy_dispose;
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
}

static void
rest_proxy_init (RestProxy *self)
{
  RestProxyPrivate *priv = GET_PRIVATE (self);

  priv->session = soup_session_async_new ();
  priv->session_sync = soup_session_sync_new ();
#if WITH_GNOME
  soup_session_add_feature_by_type (priv->session,
                                    SOUP_TYPE_PROXY_RESOLVER_GNOME);
  soup_session_add_feature_by_type (priv->session_sync,
                                    SOUP_TYPE_PROXY_RESOLVER_GNOME);
#endif

  if (REST_DEBUG_ENABLED(PROXY)) {
    soup_session_add_feature
      (priv->session, (SoupSessionFeature*)soup_logger_new (SOUP_LOGGER_LOG_BODY, 0));
    soup_session_add_feature
      (priv->session_sync, (SoupSessionFeature*)soup_logger_new (SOUP_LOGGER_LOG_BODY, 0));
  }
}

/**
 * rest_proxy_new:
 * @url_format: the endpoint URL
 * @binding_required: whether the URL needs to be bound before calling
 *
 * Create a new #RestProxy for the specified endpoint @url_format, using the
 * %GET method.
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
  return g_object_new (REST_TYPE_PROXY, 
                       "url-format", url_format,
                       "binding-required", binding_required,
                       NULL);
}

static gboolean
_rest_proxy_bind_valist (RestProxy *proxy,
                         va_list    params)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);

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
  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);

  gboolean res;
  va_list params;

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

  g_object_set (proxy, "user-agent", user_agent, NULL);
}

const gchar *
rest_proxy_get_user_agent (RestProxy *proxy)
{
  RestProxyPrivate *priv;

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);

  priv = GET_PRIVATE (proxy);

  return priv->user_agent;
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

RestProxyCall *
rest_proxy_new_call (RestProxy *proxy)
{
  RestProxyClass *proxy_class = REST_PROXY_GET_CLASS (proxy);
  return proxy_class->new_call (proxy);
}

gboolean
_rest_proxy_get_binding_required (RestProxy *proxy)
{
  RestProxyPrivate *priv;

  g_return_val_if_fail (REST_IS_PROXY (proxy), FALSE);

  priv = GET_PRIVATE (proxy);

  return priv->binding_required;
}

const gchar *
_rest_proxy_get_bound_url (RestProxy *proxy)
{
  RestProxyPrivate *priv;

  g_return_val_if_fail (REST_IS_PROXY (proxy), NULL);

  priv = GET_PRIVATE (proxy);

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

  ret = rest_proxy_call_run (call, NULL, error);
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

void
_rest_proxy_queue_message (RestProxy   *proxy,
                           SoupMessage *message,
                           SoupSessionCallback callback,
                           gpointer user_data)
{
  RestProxyPrivate *priv;

  g_return_if_fail (REST_IS_PROXY (proxy));
  g_return_if_fail (SOUP_IS_MESSAGE (message));

  priv = GET_PRIVATE (proxy);

  soup_session_queue_message (priv->session,
                              message,
                              callback,
                              user_data);
}

void
_rest_proxy_cancel_message (RestProxy   *proxy,
                            SoupMessage *message)
{
  RestProxyPrivate *priv;

  g_return_if_fail (REST_IS_PROXY (proxy));
  g_return_if_fail (SOUP_IS_MESSAGE (message));

  priv = GET_PRIVATE (proxy);
  soup_session_cancel_message (priv->session,
                               message,
                               SOUP_STATUS_CANCELLED);
}

guint
_rest_proxy_send_message (RestProxy   *proxy,
                          SoupMessage *message)
{
  RestProxyPrivate *priv;

  g_return_val_if_fail (REST_IS_PROXY (proxy), 0);
  g_return_val_if_fail (SOUP_IS_MESSAGE (message), 0);

  priv = GET_PRIVATE (proxy);

  return soup_session_send_message (priv->session_sync, message);
}
