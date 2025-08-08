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
#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <rest/rest-params.h>
#include <libsoup/soup.h>

#include "rest-private.h"
#include "rest-proxy-auth-private.h"
#include "rest-proxy-call-private.h"


struct _RestProxyCallAsyncClosure {
  RestProxyCall *call;
  RestProxyCallAsyncCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
};
typedef struct _RestProxyCallAsyncClosure RestProxyCallAsyncClosure;

#define READ_BUFFER_SIZE 8192

struct _RestProxyCallContinuousClosure {
  RestProxyCall *call;
  RestProxyCallContinuousCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
  guchar buffer[READ_BUFFER_SIZE];
};
typedef struct _RestProxyCallContinuousClosure RestProxyCallContinuousClosure;

struct _RestProxyCallUploadClosure {
  RestProxyCall *call;
  RestProxyCallUploadCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
  gsize uploaded;
};
typedef struct _RestProxyCallUploadClosure RestProxyCallUploadClosure;



#define GET_PRIVATE(o) ((RestProxyCallPrivate*)(rest_proxy_call_get_instance_private (REST_PROXY_CALL(o))))

struct _RestProxyCallPrivate {
  gchar *method;
  gchar *function;
  GHashTable *headers;
  RestParams *params;
  /* The real URL we're about to invoke */
  gchar *url;

  GHashTable *response_headers;
  GBytes *payload;
  guint status_code;
  gchar *status_message;

  GCancellable *cancellable;
  gulong cancel_sig;

  RestProxy *proxy;

  RestProxyCallAsyncClosure *cur_call_closure;
};
typedef struct _RestProxyCallPrivate RestProxyCallPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (RestProxyCall, rest_proxy_call, G_TYPE_OBJECT)


enum
{
  PROP_0 = 0,
  PROP_PROXY
};

/**
 * rest_proxy_call_error_quark:
 *
 * Registers an error quark for #RestProxyCall errors.
 *
 * Returns: the error quark
 **/
G_DEFINE_QUARK (rest-proxy-call-error-quark, rest_proxy_call_error)

static void
rest_proxy_call_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PROXY:
      g_value_set_object (value, priv->proxy);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_call_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PROXY:
      priv->proxy = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_call_dispose (GObject *object)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (object);

  if (priv->cancellable)
    {
      g_signal_handler_disconnect (priv->cancellable, priv->cancel_sig);
      g_clear_object (&priv->cancellable);
    }

  g_clear_pointer (&priv->params, rest_params_unref);
  g_clear_pointer (&priv->headers, g_hash_table_unref);
  g_clear_pointer (&priv->response_headers, g_hash_table_unref);
  g_clear_object (&priv->proxy);

  G_OBJECT_CLASS (rest_proxy_call_parent_class)->dispose (object);
}

static void
rest_proxy_call_finalize (GObject *object)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (object);

  g_free (priv->method);
  g_free (priv->function);

  g_clear_pointer (&priv->payload, g_bytes_unref);
  g_free (priv->status_message);

  g_free (priv->url);

  G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize (object);
}

static void
rest_proxy_call_class_init (RestProxyCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  object_class->get_property = rest_proxy_call_get_property;
  object_class->set_property = rest_proxy_call_set_property;
  object_class->dispose = rest_proxy_call_dispose;
  object_class->finalize = rest_proxy_call_finalize;

  pspec = g_param_spec_object ("proxy",
                               "proxy",
                               "Proxy for this call",
                               REST_TYPE_PROXY,
                               G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PROXY, pspec);
}

static void
rest_proxy_call_init (RestProxyCall *self)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (self);

  priv->method = g_strdup ("GET");

  priv->params = rest_params_new ();

  priv->headers = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);
  priv->response_headers = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  g_free);
}

/**
 * rest_proxy_call_set_method:
 * @call: The #RestProxyCall
 * @method: The HTTP method to use
 *
 * Set the HTTP method to use when making the call, for example GET or POST.
 */
void
rest_proxy_call_set_method (RestProxyCall *call,
                            const gchar   *method)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  g_free (priv->method);

  if (method)
    priv->method = g_strdup (method);
  else
    priv->method = g_strdup ("GET");
}

/**
 * rest_proxy_call_get_method:
 * @call: The #RestProxyCall
 *
 * Get the HTTP method to use when making the call, for example GET or POST.
 *
 * Returns: (transfer none): the HTTP method
 */
const char *
rest_proxy_call_get_method (RestProxyCall *call)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return GET_PRIVATE (call)->method;
}

/**
 * rest_proxy_call_set_function:
 * @call: The #RestProxyCall
 * @function: The function to call
 *
 * Set the REST "function" to call on the proxy.  This is appended to the URL,
 * so that for example a proxy with the URL
 * <literal>http://www.example.com/</literal> and the function
 * <literal>test</literal> would actually access the URL
 * <literal>http://www.example.com/test</literal>
 */
void
rest_proxy_call_set_function (RestProxyCall *call,
                              const gchar   *function)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  g_free (priv->function);
  priv->function = g_strdup (function);
}

/**
 * rest_proxy_call_get_function:
 * @call: The #RestProxyCall
 *
 * Get the REST function that is going to be called on the proxy.
 *
 * Returns: The REST "function" for the current call, see also
 * rest_proxy_call_set_function(). This string is owned by the #RestProxyCall
 * and should not be freed.
 * Since: 0.7.92
 */
const char *
rest_proxy_call_get_function (RestProxyCall *call)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return GET_PRIVATE (call)->function;
}


/**
 * rest_proxy_call_add_header:
 * @call: The #RestProxyCall
 * @header: The name of the header to set
 * @value: The value of the header
 *
 * Add a header called @header with the value @value to the call.  If a
 * header with this name already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_header (RestProxyCall *call,
                            const gchar   *header,
                            const gchar   *value)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  g_hash_table_insert (priv->headers,
                       g_strdup (header),
                       g_strdup (value));

}

/**
 * rest_proxy_call_add_headers:
 * @call: The #RestProxyCall
 * @...: Header name and value pairs, followed by %NULL.
 *
 * Add the specified header name and value pairs to the call.  If a header
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_headers (RestProxyCall *call,
                             ...)
{
  va_list headers;

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  va_start (headers, call);
  rest_proxy_call_add_headers_from_valist (call, headers);
  va_end (headers);
}

/**
 * rest_proxy_call_add_headers_from_valist:
 * @call: The #RestProxyCall
 * @headers: Header name and value pairs, followed by %NULL.
 *
 * Add the specified header name and value pairs to the call.  If a header
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_headers_from_valist (RestProxyCall *call,
                                         va_list        headers)
{
  const gchar *header = NULL;
  const gchar *value = NULL;

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  while ((header = va_arg (headers, const gchar *)) != NULL)
  {
    value = va_arg (headers, const gchar *);
    rest_proxy_call_add_header (call, header, value);
  }
}

/**
 * rest_proxy_call_lookup_header:
 * @call: The #RestProxyCall
 * @header: The header name
 *
 * Get the value of the header called @header.
 *
 * Returns: The header value, or %NULL if it does not exist. This string is
 * owned by the #RestProxyCall and should not be freed.
 */
const gchar *
rest_proxy_call_lookup_header (RestProxyCall *call,
                               const gchar   *header)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return g_hash_table_lookup (GET_PRIVATE (call)->headers, header);
}

/**
 * rest_proxy_call_remove_header:
 * @call: The #RestProxyCall
 * @header: The header name
 *
 * Remove the header named @header from the call.
 */
void
rest_proxy_call_remove_header (RestProxyCall *call,
                               const gchar   *header)
{
  g_return_if_fail (REST_IS_PROXY_CALL (call));

  g_hash_table_remove (GET_PRIVATE (call)->headers, header);
}

/**
 * rest_proxy_call_add_param:
 * @call: The #RestProxyCall
 * @name: The name of the parameter to set
 * @value: The value of the parameter
 *
 * Add a query parameter called @param with the string value @value to the call.
 * If a parameter with this name already exists, the new value will replace the
 * old.
 */
void
rest_proxy_call_add_param (RestProxyCall *call,
                           const gchar   *name,
                           const gchar   *value)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  RestParam *param;

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  param = rest_param_new_string (name, REST_MEMORY_COPY, value);
  rest_params_add (priv->params, param);
}

/**
 * rest_proxy_call_add_param_full:
 * @call: The #RestProxyCall
 * @param: (transfer full): A #RestParam
 *
 * Add a query parameter @param to the call.
 * If a parameter with this name already exists, the new value will replace the
 * old.
 */
void
rest_proxy_call_add_param_full (RestProxyCall *call, RestParam *param)
{
  g_return_if_fail (REST_IS_PROXY_CALL (call));
  g_return_if_fail (param);

  rest_params_add (GET_PRIVATE (call)->params, param);
}

/**
 * rest_proxy_call_add_params:
 * @call: The #RestProxyCall
 * @...: Parameter name and value pairs, followed by %NULL.
 *
 * Add the specified parameter name and value pairs to the call.  If a parameter
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_params (RestProxyCall *call,
                            ...)
{
  va_list params;

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  va_start (params, call);
  rest_proxy_call_add_params_from_valist (call, params);
  va_end (params);
}

/**
 * rest_proxy_call_add_params_from_valist:
 * @call: The #RestProxyCall
 * @params: Parameter name and value pairs, followed by %NULL.
 *
 * Add the specified parameter name and value pairs to the call.  If a parameter
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_params_from_valist (RestProxyCall *call,
                                        va_list        params)
{
  const gchar *param = NULL;
  const gchar *value = NULL;

  g_return_if_fail (REST_IS_PROXY_CALL (call));

  while ((param = va_arg (params, const gchar *)) != NULL)
  {
    value = va_arg (params, const gchar *);
    rest_proxy_call_add_param (call, param, value);
  }
}

/**
 * rest_proxy_call_lookup_param:
 * @call: The #RestProxyCall
 * @name: The paramter name
 *
 * Get the value of the parameter called @name.
 *
 * Returns: (transfer none) (nullable): The parameter value, or %NULL if it does
 * not exist. This string is owned by the #RestProxyCall and should not be
 * freed.
 */
RestParam *
rest_proxy_call_lookup_param (RestProxyCall *call,
                              const gchar   *name)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return rest_params_get (GET_PRIVATE (call)->params, name);
}

/**
 * rest_proxy_call_remove_param:
 * @call: The #RestProxyCall
 * @name: The paramter name
 *
 * Remove the parameter named @name from the call.
 */
void
rest_proxy_call_remove_param (RestProxyCall *call,
                              const gchar   *name)
{
  g_return_if_fail (REST_IS_PROXY_CALL (call));

  rest_params_remove (GET_PRIVATE (call)->params, name);
}

/**
 * rest_proxy_call_get_params:
 * @call: The #RestProxyCall
 *
 * Get the parameters as a #RestParams of parameter names to values.  The
 * returned value is owned by the RestProxyCall instance and should not
 * be freed by the caller.
 *
 * Returns: (transfer none): A #RestParams.
 */
RestParams *
rest_proxy_call_get_params (RestProxyCall *call)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return GET_PRIVATE (call)->params;
}



static void _call_async_weak_notify_cb (gpointer *data,
                                        GObject  *dead_object);

static void
_populate_headers_hash_table (const gchar *name,
                              const gchar *value,
                              gpointer     userdata)
{
  GHashTable *headers = (GHashTable *)userdata;

  g_hash_table_insert (headers, g_strdup (name), g_strdup (value));
}

#ifdef WITH_SOUP_2
/* I apologise for this macro, but it saves typing ;-) */
#define error_helper(x) g_set_error_literal(error, REST_PROXY_ERROR, x, message->reason_phrase)
#endif
static gboolean
_handle_error_from_message (SoupMessage *message, GError **error)
{
  guint status_code;
  const char *reason_phrase;

#ifdef WITH_SOUP_2
  status_code = message->status_code;

  if (status_code < 100)
  {
    g_clear_error (error);
    switch (status_code)
    {
      case SOUP_STATUS_CANCELLED:
        error_helper (REST_PROXY_ERROR_CANCELLED);
        break;
      case SOUP_STATUS_CANT_RESOLVE:
      case SOUP_STATUS_CANT_RESOLVE_PROXY:
        error_helper (REST_PROXY_ERROR_RESOLUTION);
        break;
      case SOUP_STATUS_CANT_CONNECT:
      case SOUP_STATUS_CANT_CONNECT_PROXY:
        error_helper (REST_PROXY_ERROR_CONNECTION);
        break;
      case SOUP_STATUS_SSL_FAILED:
        error_helper (REST_PROXY_ERROR_SSL);
        break;
      case SOUP_STATUS_IO_ERROR:
        error_helper (REST_PROXY_ERROR_IO);
        break;
      case SOUP_STATUS_MALFORMED:
      case SOUP_STATUS_TRY_AGAIN:
      default:
        error_helper (REST_PROXY_ERROR_FAILED);
        break;
    }
    return FALSE;
  }
  reason_phrase = message->reason_phrase;
#else
  status_code = soup_message_get_status (message);
  reason_phrase = soup_message_get_reason_phrase (message);
#endif

  if (status_code >= 200 && status_code < 300)
  {
    return TRUE;
  }

  if (*error != NULL)
    return FALSE;

  /* If we are here we must be in some kind of HTTP error, lets try */
  g_set_error_literal (error,
                       REST_PROXY_ERROR,
                       status_code,
                       reason_phrase);
  return FALSE;
}

static gboolean
finish_call (RestProxyCall *call, SoupMessage *message, GBytes *payload, GError **error)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  SoupMessageHeaders *response_headers;

  g_assert (call);
  g_assert (message);
  g_assert (payload);

#ifdef WITH_SOUP_2
  response_headers = message->response_headers;
#else
  response_headers = soup_message_get_response_headers (message);
#endif

  /* Convert the soup headers in to hash */
  /* FIXME: Eeek..are you allowed duplicate headers? ... */
  g_hash_table_remove_all (priv->response_headers);
  soup_message_headers_foreach (response_headers,
      (SoupMessageHeadersForeachFunc)_populate_headers_hash_table,
      priv->response_headers);

  priv->payload = payload;

#ifdef WITH_SOUP_2
  priv->status_code = message->status_code;
  priv->status_message = g_strdup (message->reason_phrase);
#else
  priv->status_code = soup_message_get_status (message);
  priv->status_message = g_strdup (soup_message_get_reason_phrase (message));
#endif

  return _handle_error_from_message (message, error);
}

static void
_continuous_call_message_completed (SoupMessage *message,
                                    GError      *error,
                                    gpointer     userdata)
{
  RestProxyCallContinuousClosure *closure;
  RestProxyCall *call;
  RestProxyCallPrivate *priv;

  closure = (RestProxyCallContinuousClosure *)userdata;
  call = closure->call;
  priv = GET_PRIVATE (call);

#ifdef WITH_SOUP_2
  priv->status_code = message->status_code;
  priv->status_message = g_strdup (message->reason_phrase);
#else
  priv->status_code = soup_message_get_status (message);
  priv->status_message = g_strdup (soup_message_get_reason_phrase (message));
#endif

  _handle_error_from_message (message, &error);

  closure->callback (closure->call,
                     NULL,
                     0,
                     error,
                     closure->weak_object,
                     closure->userdata);

  g_clear_error (&error);

  /* Success. We don't need the weak reference any more */
  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object,
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  priv->cur_call_closure = NULL;
  g_object_unref (closure->call);
  g_object_unref (message);
  g_slice_free (RestProxyCallContinuousClosure, closure);
}


static void
_call_async_weak_notify_cb (gpointer *data,
                            GObject  *dead_object)
{
  RestProxyCallAsyncClosure *closure;

  closure = (RestProxyCallAsyncClosure *)data;

  /* Will end up freeing the closure */
  rest_proxy_call_cancel (closure->call);
}

static void
set_header (gpointer key, gpointer value, gpointer user_data)
{
  const char *name = key;
  SoupMessageHeaders *headers = user_data;

  soup_message_headers_replace (headers, name, value);
}

static gboolean
set_url (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  const gchar *bound_url;

  bound_url =_rest_proxy_get_bound_url (priv->proxy);

  if (_rest_proxy_get_binding_required (priv->proxy) && !bound_url)
  {
    g_critical (G_STRLOC ": URL requires binding and is unbound");
    return FALSE;
  }

  g_free (priv->url);

  /* FIXME: Perhaps excessive memory duplication */
  if (priv->function)
  {
    if (g_str_has_suffix (bound_url, "/")
          || g_str_has_prefix (priv->function, "/"))
    {
      priv->url = g_strdup_printf ("%s%s", bound_url, priv->function);
    } else {
      priv->url = g_strdup_printf ("%s/%s", bound_url, priv->function);
    }
  } else {
    priv->url = g_strdup (bound_url);
  }

  return TRUE;
}

#ifndef WITH_SOUP_2
static gboolean
authenticate (RestProxyCall *call,
              SoupAuth      *soup_auth,
              gboolean       retrying,
              SoupMessage   *message)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  g_autofree char *username;
  g_autofree char *password;

  if (retrying)
    return FALSE;

  g_object_get (priv->proxy, "username", &username, "password", &password, NULL);
  soup_auth_authenticate (soup_auth, username, password);

  return TRUE;
}

static gboolean
accept_certificate (RestProxyCall        *call,
                    GTlsCertificate      *tls_certificate,
                    GTlsCertificateFlags *tls_errors,
                    SoupMessage          *message)
{
        RestProxyCallPrivate *priv = GET_PRIVATE (call);
        gboolean ssl_strict;

        if (tls_errors == 0)
                return TRUE;

        g_object_get (priv->proxy, "ssl-strict", &ssl_strict, NULL);
        return !ssl_strict;
}
#endif

static SoupMessage *
prepare_message (RestProxyCall *call, GError **error_out)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  RestProxyCallClass *call_class;
  const gchar *user_agent;
  SoupMessage *message;
  SoupMessageHeaders *request_headers;
  GError *error = NULL;

  call_class = REST_PROXY_CALL_GET_CLASS (call);

  /* Emit a warning if the caller is re-using RestProxyCall objects */
  if (priv->url)
  {
    g_warning (G_STRLOC ": re-use of RestProxyCall %p, don't do this", call);
  }

  /* Allow an overrideable prepare function that is called before every
   * invocation so subclasses can do magic
   */
  if (call_class->prepare)
  {
    if (!call_class->prepare (call, &error))
    {
      g_propagate_error (error_out, error);
      return NULL;
    }
  }

  if (call_class->serialize_params) {
    gchar *content;
    gchar *content_type;
    gsize content_len;
#ifndef WITH_SOUP_2
    GBytes *body;
#endif

    if (!call_class->serialize_params (call, &content_type,
                                       &content, &content_len, &error))
    {
      g_propagate_error (error_out, error);
      return NULL;
    }

    /* Reset priv->url as the serialize_params vcall may have called
     * rest_proxy_call_set_function()
     */
    if (!set_url (call))
    {
        g_free (content);
        g_free (content_type);
        g_set_error_literal (error_out,
                             REST_PROXY_ERROR,
                             REST_PROXY_ERROR_BINDING_REQUIRED,
                             "URL is unbound");
        return NULL;
    }

    message = soup_message_new (priv->method, priv->url);
    if (message == NULL) {
        g_free (content);
        g_free (content_type);
        g_set_error_literal (error_out,
                             REST_PROXY_ERROR,
                             REST_PROXY_ERROR_FAILED,
                             "Could not parse URI");
        return NULL;
    }
#ifdef WITH_SOUP_2
    soup_message_set_request (message, content_type,
                              SOUP_MEMORY_TAKE, content, content_len);
#else
    body = g_bytes_new_take (content, content_len);
    soup_message_set_request_body_from_bytes (message, content_type, body);
    g_bytes_unref (body);
#endif

    g_free (content_type);
  } else if (rest_params_are_strings (priv->params)) {
    GHashTable *hash;

    if (!set_url (call))
    {
        g_set_error_literal (error_out,
                             REST_PROXY_ERROR,
                             REST_PROXY_ERROR_BINDING_REQUIRED,
                             "URL is unbound");
        return NULL;
    }

    hash = rest_params_as_string_hash_table (priv->params);

#ifdef WITH_SOUP_2
    if (g_hash_table_size (hash) == 0)
      message = soup_message_new (priv->method, priv->url);
    else
      message = soup_form_request_new_from_hash (priv->method,
                                                 priv->url,
                                                 hash);
#else
    if (g_hash_table_size (hash) == 0)
      message = soup_message_new (priv->method, priv->url);
    else
      message = soup_message_new_from_encoded_form (priv->method,
                                                    priv->url,
                                                    soup_form_encode_hash (hash));
#endif

    g_hash_table_unref (hash);

    if (!message) {
        g_set_error (error_out,
                     REST_PROXY_ERROR,
                     REST_PROXY_ERROR_URL_INVALID,
                     "URL '%s' is not valid",
                     priv->url);
        return NULL;
    }

  } else {
    SoupMultipart *mp;
    RestParamsIter iter;
    const char *name;
    RestParam *param;

    mp = soup_multipart_new (SOUP_FORM_MIME_TYPE_MULTIPART);

    rest_params_iter_init (&iter, priv->params);

    while (rest_params_iter_next (&iter, &name, &param)) {
      if (rest_param_is_string (param)) {
        soup_multipart_append_form_string (mp, name, rest_param_get_content (param));
      } else {
#ifdef WITH_SOUP_2
        SoupBuffer *sb = soup_buffer_new_with_owner (rest_param_get_content (param),
                                                     rest_param_get_content_length (param),
                                                     rest_param_ref (param),
                                                     (GDestroyNotify)rest_param_unref);
#else
        GBytes *sb = g_bytes_new_with_free_func (rest_param_get_content (param),
                                                 rest_param_get_content_length (param),
                                                 (GDestroyNotify)rest_param_unref,
                                                 rest_param_ref (param));
#endif

        soup_multipart_append_form_file (mp, name,
                                         rest_param_get_file_name (param),
                                         rest_param_get_content_type (param),
                                         sb);

#ifdef WITH_SOUP_2
        soup_buffer_free (sb);
#else
        g_bytes_unref (sb);
#endif
      }
    }

    if (!set_url (call))
    {
        soup_multipart_free (mp);
        g_set_error_literal (error_out,
                             REST_PROXY_ERROR,
                             REST_PROXY_ERROR_BINDING_REQUIRED,
                             "URL is unbound");
        return NULL;
    }

#ifdef WITH_SOUP_2
    message = soup_form_request_new_from_multipart (priv->url, mp);
#else
    message = soup_message_new_from_multipart (priv->url, mp);
#endif

    soup_multipart_free (mp);
  }

#ifdef WITH_SOUP_2
  request_headers = message->request_headers;
#else
  request_headers = soup_message_get_request_headers (message);
  g_signal_connect_swapped (message, "authenticate",
                            G_CALLBACK (authenticate),
                            call);
  g_signal_connect_swapped (message, "accept-certificate",
                            G_CALLBACK (accept_certificate),
                            call);
#endif


  /* Set the user agent, if one was set in the proxy */
  user_agent = rest_proxy_get_user_agent (priv->proxy);
  if (user_agent) {
    soup_message_headers_append (request_headers, "User-Agent", user_agent);
  }

  /* Set the headers */
  g_hash_table_foreach (priv->headers, set_header, request_headers);

  return message;
}

static void
_call_message_call_cancelled_cb (GCancellable  *cancellable,
                                 RestProxyCall *call)
{
  rest_proxy_call_cancel (call);
}

static void
_call_message_call_completed_cb (SoupMessage *message,
                                 GBytes      *payload,
                                 GError      *error,
                                 gpointer     user_data)
{
  g_autoptr(GTask) task = user_data;
  RestProxyCall *call;

  call = REST_PROXY_CALL (g_task_get_source_object (task));

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  finish_call (call, message, payload, &error);

  if (error != NULL)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * rest_proxy_call_invoke_async:
 * @call: a #RestProxyCall
 * @cancellable: (nullable): an optional #GCancellable that can be used to
 *   cancel the call, or %NULL
 * @callback: (scope async): callback to call when the async call is finished
 * @user_data: user data for the callback
 */
void
rest_proxy_call_invoke_async (RestProxyCall      *call,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  GTask *task;
  SoupMessage *message;
  GError *error = NULL;

  g_return_if_fail (REST_IS_PROXY_CALL (call));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_assert (priv->proxy);

  message = prepare_message (call, &error);
  task = g_task_new (call, cancellable, callback, user_data);
  if (message == NULL)
    {
      g_task_return_error (task, error);
      return;
    }

  if (cancellable != NULL)
    {
      priv->cancel_sig = g_signal_connect (cancellable, "cancelled",
          G_CALLBACK (_call_message_call_cancelled_cb), call);
      priv->cancellable = g_object_ref (cancellable);
    }

  _rest_proxy_queue_message (priv->proxy,
                             message,
                             priv->cancellable,
                             _call_message_call_completed_cb,
                             task);
}

/**
 * rest_proxy_call_invoke_finish:
 * @call: a #RestProxyCall
 * @result: the result from the #GAsyncReadyCallback
 * @error: optional #GError
 *
 * Returns: %TRUE on success
 */
gboolean
rest_proxy_call_invoke_finish (RestProxyCall  *call,
                               GAsyncResult   *result,
                               GError        **error)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, call), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
_continuous_call_read_cb (GObject      *source,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  GInputStream *stream = G_INPUT_STREAM (source);
  RestProxyCallContinuousClosure *closure = user_data;
  RestProxyCallPrivate *priv = GET_PRIVATE (closure->call);
  gssize bytes_read;
  GError *error = NULL;

  bytes_read = g_input_stream_read_finish (stream, result, &error);
  if (bytes_read <= 0)
    {
      _continuous_call_message_completed (closure->message, error, user_data);
      return;
    }

  closure->callback (closure->call,
                     (gconstpointer)closure->buffer,
                     bytes_read,
                     NULL,
                     closure->weak_object,
                     closure->userdata);

  g_input_stream_read_async (stream, closure->buffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT,
                             priv->cancellable, _continuous_call_read_cb, closure);
}

static void
_continuous_call_message_sent_cb (GObject      *source,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  RestProxy *proxy = REST_PROXY (source);
  RestProxyCallContinuousClosure *closure = user_data;
  RestProxyCallPrivate *priv = GET_PRIVATE (closure->call);
  GInputStream *stream;
  GError *error = NULL;

  stream = _rest_proxy_send_message_finish (proxy, result, &error);
  if (!stream)
    {
      _continuous_call_message_completed (closure->message, error, user_data);
      return;
    }

  g_input_stream_read_async (stream, closure->buffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT,
                             priv->cancellable, _continuous_call_read_cb, closure);
  g_object_unref (stream);
}


/**
 * rest_proxy_call_continuous: (skip)
 * @call: The #RestProxyCall
 * @callback: (closure userdata): a #RestProxyCallContinuousCallback to invoke when data is available
 * @weak_object: The #GObject to weakly reference and tie the lifecycle to
 * @userdata: data to pass to @callback
 * @error: (out) (allow-none): a #GError, or %NULL
 *
 * Asynchronously invoke @call but expect a continuous stream of content. This
 * means that the body data will not be accumulated and thus you cannot use
 * rest_proxy_call_get_payload()
 *
 * When there is data @callback will be called and when the connection is
 * closed or the stream ends @callback will also be called.
 *
 * If @weak_object is disposed during the call then this call will be
 * cancelled. If the call is cancelled then the callback will be invoked with
 * an error state.
 *
 * You may unref the call after calling this function since there is an
 * internal reference, or you may unref in the callback.
 *
 * Returns: %TRUE on success
 */
gboolean
rest_proxy_call_continuous (RestProxyCall                    *call,
                            RestProxyCallContinuousCallback   callback,
                            GObject                          *weak_object,
                            gpointer                          userdata,
                            GError                          **error)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  SoupMessage *message;
  RestProxyCallContinuousClosure *closure;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);
  g_assert (priv->proxy);

  if (priv->cur_call_closure)
  {
    g_warning (G_STRLOC ": re-use of RestProxyCall %p, don't do this", call);
    return FALSE;
  }

  message = prepare_message (call, error);
  if (message == NULL)
    return FALSE;

  closure = g_slice_new0 (RestProxyCallContinuousClosure);
  closure->call = g_object_ref (call);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;
  closure->userdata = userdata;

  priv->cur_call_closure = (RestProxyCallAsyncClosure *)closure;

  /* Weakly reference this object. We remove our callback if it goes away. */
  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object,
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  _rest_proxy_send_message_async (priv->proxy,
                                  message,
                                  priv->cancellable,
                                  _continuous_call_message_sent_cb,
                                  closure);
  return TRUE;
}

static void
_upload_call_message_completed_cb (SoupMessage *message,
                                   GBytes      *payload,
                                   GError      *error,
                                   gpointer     user_data)
{
  RestProxyCall *call;
  RestProxyCallPrivate *priv;
  RestProxyCallUploadClosure *closure;

  closure = (RestProxyCallUploadClosure *) user_data;
  call = closure->call;
  priv = GET_PRIVATE (call);

  finish_call (call, message, payload, &error);

  closure->callback (closure->call,
                     closure->uploaded,
                     closure->uploaded,
                     error,
                     closure->weak_object,
                     closure->userdata);

  g_clear_error (&error);

  /* Success. We don't need the weak reference any more */
  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object,
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  priv->cur_call_closure = NULL;
  g_object_unref (closure->call);
  g_slice_free (RestProxyCallUploadClosure, closure);
}

static void
_upload_call_message_wrote_data_cb (SoupMessage                *msg,
#ifdef WITH_SOUP_2
                                    SoupBuffer                 *chunk,
#else
                                    gsize                       chunk_size,
#endif
                                    RestProxyCallUploadClosure *closure)
{
#ifdef WITH_SOUP_2
  gsize chunk_size = chunk->length;
  goffset content_length = msg->request_body->length;
#else
  goffset content_length = soup_message_headers_get_content_length (soup_message_get_request_headers (msg));
#endif

  closure->uploaded = closure->uploaded + chunk_size;

  if (closure->uploaded < content_length)
    closure->callback (closure->call,
                       content_length,
                       closure->uploaded,
                       NULL,
                       closure->weak_object,
                       closure->userdata);
}

/**
 * rest_proxy_call_upload:
 * @call: The #RestProxyCall
 * @callback: (scope async): a #RestProxyCallUploadCallback to invoke when a chunk
 *   of data was uploaded
 * @weak_object: The #GObject to weakly reference and tie the lifecycle to
 * @userdata: data to pass to @callback
 * @error: a #GError, or %NULL
 *
 * Asynchronously invoke @call but expect to have the callback invoked every time a
 * chunk of our request's body is written.
 *
 * When the callback is invoked with the uploaded byte count equaling the message
 * byte count, the call has completed.
 *
 * If @weak_object is disposed during the call then this call will be
 * cancelled. If the call is cancelled then the callback will be invoked with
 * an error state.
 *
 * You may unref the call after calling this function since there is an
 * internal reference, or you may unref in the callback.
 */
gboolean
rest_proxy_call_upload (RestProxyCall                *call,
                        RestProxyCallUploadCallback   callback,
                        GObject                      *weak_object,
                        gpointer                      userdata,
                        GError                      **error)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  SoupMessage *message;
  RestProxyCallUploadClosure *closure;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);
  g_assert (priv->proxy);

  if (priv->cur_call_closure)
  {
    g_warning (G_STRLOC ": re-use of RestProxyCall %p, don't do this", call);
    return FALSE;
  }

  message = prepare_message (call, error);
  if (message == NULL)
    return FALSE;

  closure = g_slice_new0 (RestProxyCallUploadClosure);
  closure->call = g_object_ref (call);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;
  closure->userdata = userdata;
  closure->uploaded = 0;

  priv->cur_call_closure = (RestProxyCallAsyncClosure *)closure;

  /* Weakly reference this object. We remove our callback if it goes away. */
  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object,
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  g_signal_connect (message,
                    "wrote-body-data",
                    (GCallback) _upload_call_message_wrote_data_cb,
                    closure);

  _rest_proxy_queue_message (priv->proxy,
                             message,
                             priv->cancellable,
                             _upload_call_message_completed_cb,
                             closure);
  return TRUE;
}

/**
 * rest_proxy_call_cancel: (skip)
 * @call: The #RestProxyCall
 *
 * Cancel this call.  It may be too late to not actually send the message, but
 * the callback will not be invoked.
 *
 * N.B. this method should only be used with rest_proxy_call_async() and NOT
 * rest_proxy_call_invoke_async().
 */
gboolean
rest_proxy_call_cancel (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  RestProxyCallAsyncClosure *closure;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);

  closure = priv->cur_call_closure;

  if (priv->cancellable)
    {
      g_signal_handler_disconnect (priv->cancellable, priv->cancel_sig);
#ifndef WITH_SOUP_2
      if (!g_cancellable_is_cancelled (priv->cancellable))
              g_cancellable_cancel (priv->cancellable);
#endif
      g_clear_object (&priv->cancellable);
    }

  if (closure)
  {
    /* This will cause the _call_message_completed_cb to be fired which will
     * tidy up the closure and so forth */
    _rest_proxy_cancel_message (priv->proxy, closure->message);
  }

  return TRUE;
}

/**
 * rest_proxy_call_sync:
 * @call: a #RestProxycall
 * @error_out: a #GError or %NULL
 *
 * Synchronously invokes @call. After this function has returned,
 * rest_proxy_call_get_payload() will return the result of this call.
 *
 * Note that this will block an undefined amount of time, so
 * rest_proxy_call_invoke_async() is generally recommended.
 *
 * Returns: %TRUE on success, %FALSE if a failure occurred,
 *   in which case @error_out will be set.
 */
gboolean
rest_proxy_call_sync (RestProxyCall *call,
                      GError       **error_out)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  SoupMessage *message;
  gboolean ret;
  GBytes *payload;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);

  message = prepare_message (call, error_out);
  if (!message)
    return FALSE;

  payload = _rest_proxy_send_message (priv->proxy, message, priv->cancellable, error_out);
  if (!payload)
  {
    g_object_unref (message);
    return FALSE;
  }

  ret = finish_call (call, message, payload, error_out);

  g_object_unref (message);

  return ret;
}

/**
 * rest_proxy_call_lookup_response_header:
 * @call: The #RestProxyCall
 * @header: The name of the header to lookup.
 *
 * Get the string value of the header @header or %NULL if that header is not
 * present or there are no headers.
 */
const gchar *
rest_proxy_call_lookup_response_header (RestProxyCall *call,
                                        const gchar   *header)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  if (!priv->response_headers)
  {
    return NULL;
  }

  return g_hash_table_lookup (priv->response_headers, header);
}

/**
 * rest_proxy_call_get_response_headers:
 * @call: The #RestProxyCall
 *
 * Returns: (transfer container) (element-type utf8 utf8): pointer to a hash
 * table of headers. This hash table must not be changed. You should call
 * g_hash_table_unref() when you have finished with it.
 */
GHashTable *
rest_proxy_call_get_response_headers (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  if (!priv->response_headers)
  {
    return NULL;
  }

  return g_hash_table_ref (priv->response_headers);
}

/**
 * rest_proxy_call_get_payload_length:
 * @call: The #RestProxyCall
 *
 * Get the length of the return payload.
 *
 * Returns: the length of the payload in bytes.
 */
goffset
rest_proxy_call_get_payload_length (RestProxyCall *call)
{
  GBytes *payload;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), 0);

  payload = GET_PRIVATE (call)->payload;
  return payload ? g_bytes_get_size (payload) : 0;
}

/**
 * rest_proxy_call_get_payload:
 * @call: The #RestProxyCall
 *
 * Get the return payload.
 *
 * Returns: A pointer to the payload. This is owned by #RestProxyCall and should
 * not be freed.
 */
const gchar *
rest_proxy_call_get_payload (RestProxyCall *call)
{
  GBytes *payload;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  payload = GET_PRIVATE (call)->payload;
  return payload ? g_bytes_get_data (payload, NULL) : NULL;
}

/**
 * rest_proxy_call_get_status_code:
 * @call: The #RestProxyCall
 *
 * Get the HTTP status code for the call.
 */
guint
rest_proxy_call_get_status_code (RestProxyCall *call)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), 0);

  return GET_PRIVATE (call)->status_code;
}

/**
 * rest_proxy_call_get_status_message:
 * @call: The #RestProxyCall
 *
 * Get the human-readable HTTP status message for the call.
 *
 * Returns: The status message. This string is owned by #RestProxyCall and
 * should not be freed.
 */
const gchar *
rest_proxy_call_get_status_message (RestProxyCall *call)
{
  g_return_val_if_fail (REST_IS_PROXY_CALL (call), NULL);

  return GET_PRIVATE (call)->status_message;
}

/**
 * rest_proxy_call_serialize_params:
 * @call: The #RestProxyCall
 * @content_type: (out): Content type of the payload
 * @content: (out): The payload
 * @content_len: (out): Length of the payload data
 * @error: a #GError, or %NULL
 *
 * Invoker for a virtual method to serialize the parameters for this
 * #RestProxyCall.
 *
 * Returns: TRUE if the serialization was successful, FALSE otherwise.
 */
gboolean
rest_proxy_call_serialize_params (RestProxyCall *call,
                                  gchar **content_type,
                                  gchar **content,
                                  gsize *content_len,
                                  GError **error)
{
  RestProxyCallClass *call_class;

  call_class = REST_PROXY_CALL_GET_CLASS (call);

  if (call_class->serialize_params)
  {
    return call_class->serialize_params (call, content_type,
                                         content, content_len, error);
  }

  return FALSE;
}

G_GNUC_INTERNAL const char *
rest_proxy_call_get_url (RestProxyCall *call)
{
  /* Ensure priv::url is current before returning it. This method is used
   * by OAuthProxyCall::_prepare which expects set_url() to have been called,
   * but this has been changed/broken by c66b6df
   */
  set_url(call);
  return GET_PRIVATE (call)->url;
}
