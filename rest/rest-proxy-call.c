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

#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <libsoup/soup.h>

#include "rest-private.h"
#include "rest-proxy-call-private.h"

G_DEFINE_TYPE (RestProxyCall, rest_proxy_call, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_PROXY_CALL, RestProxyCallPrivate))

struct _RestProxyCallAsyncClosure {
  RestProxyCall *call;
  RestProxyCallAsyncCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
};

enum
{
  PROP_0 = 0,
  PROP_PROXY
};

GQuark
rest_proxy_call_error_quark (void)
{
  return g_quark_from_static_string ("rest-proxy-call-error-quark");
}

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

  if (priv->params)
  {
    g_hash_table_unref (priv->params);
    priv->params = NULL;
  }

  if (priv->headers)
  {
    g_hash_table_unref (priv->headers);
    priv->headers = NULL;
  }

  if (priv->response_headers)
  {
    g_hash_table_unref (priv->response_headers);
    priv->response_headers = NULL;
  }

  if (priv->proxy)
  {
    g_object_unref (priv->proxy);
    priv->proxy = NULL;
  }

  if (G_OBJECT_CLASS (rest_proxy_call_parent_class)->dispose)
    G_OBJECT_CLASS (rest_proxy_call_parent_class)->dispose (object);
}

static void
rest_proxy_call_finalize (GObject *object)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (object);

  g_free (priv->method);
  g_free (priv->function);

  g_free (priv->payload);
  g_free (priv->status_message);

  if (G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize)
    G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize (object);
}

static void
rest_proxy_call_class_init (RestProxyCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (RestProxyCallPrivate));

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

  self->priv = priv;

  priv->method = g_strdup ("GET");

  priv->params = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        g_free);
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
 */
const char *
rest_proxy_call_get_method (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return priv->method;
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

  g_free (priv->function);

  priv->function = g_strdup (function);
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

  g_hash_table_insert (priv->headers,
                       g_strdup (header),
                       g_strdup (value));

}

/**
 * rest_proxy_call_add_headers:
 * @call: The #RestProxyCall
 * @Varargs: Header name and value pairs, followed by %NULL.
 *
 * Add the specified header name and value pairs to the call.  If a header
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_headers (RestProxyCall *call,
                             ...)
{
  va_list headers;

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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return g_hash_table_lookup (priv->headers, header);
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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_hash_table_remove (priv->headers, header);
}

/**
 * rest_proxy_call_add_param:
 * @call: The #RestProxyCall
 * @param: The name of the parameter to set
 * @value: The value of the parameter
 *
 * Add a query parameter called @param with the value @value to the call.  If a
 * parameter with this name already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_param (RestProxyCall *call,
                           const gchar   *param,
                           const gchar   *value)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_hash_table_insert (priv->params,
                       g_strdup (param),
                       g_strdup (value));

}

/**
 * rest_proxy_call_add_params:
 * @call: The #RestProxyCall
 * @Varargs: Parameter name and value pairs, followed by %NULL.
 *
 * Add the specified parameter name and value pairs to the call.  If a parameter
 * already exists, the new value will replace the old.
 */
void
rest_proxy_call_add_params (RestProxyCall *call,
                            ...)
{
  va_list params;

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

  while ((param = va_arg (params, const gchar *)) != NULL)
  {
    value = va_arg (params, const gchar *);
    rest_proxy_call_add_param (call, param, value);
  }
}

/**
 * rest_proxy_call_lookup_param:
 * @call: The #RestProxyCall
 * @param: The paramter name
 *
 * Get the value of the parameter called @name.
 *
 * Returns: The parameter value, or %NULL if it does not exist. This string is
 * owned by the #RestProxyCall and should not be freed.
 */
const gchar *
rest_proxy_call_lookup_param (RestProxyCall *call,
                              const gchar   *param)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return g_hash_table_lookup (priv->params, param);
}

/**
 * rest_proxy_call_remove_param:
 * @call: The #RestProxyCall
 * @param: The paramter name
 *
 * Remove the parameter named @param from the call.
 */
void
rest_proxy_call_remove_param (RestProxyCall *call,
                              const gchar   *param)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_hash_table_remove (priv->params, param);
}

/**
 * rest_proxy_call_get_params:
 * @call: The #RestProxyCall
 *
 * Get the parameters as a #GHashTable of parameter names to values.  The caller
 * should call g_hash_table_unref() when they have finished using it.
 *
 * Returns: A #GHashTable.
 */
GHashTable *
rest_proxy_call_get_params (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return g_hash_table_ref (priv->params);
}



static void _call_async_weak_notify_cb (gpointer *data,
                                        GObject  *dead_object);

static void _call_async_finished_cb (SoupMessage *message,
                                     gpointer     userdata);

static void
_populate_headers_hash_table (const gchar *name,
                              const gchar *value,
                              gpointer     userdata)
{
  GHashTable *headers = (GHashTable *)userdata;

  g_hash_table_insert (headers, g_strdup (name), g_strdup (value));
}

/* I apologise for this macro, but it saves typing ;-) */
#define error_helper(x) g_set_error(error, REST_PROXY_ERROR, x, message->reason_phrase)
static gboolean
_handle_error_from_message (SoupMessage *message, GError **error)
{
  if (message->status_code < 100)
  {
    switch (message->status_code)
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

  if (message->status_code >= 200 && message->status_code < 300)
  {
    return TRUE;
  }

  /* If we are here we must be in some kind of HTTP error, lets try */
  g_set_error (error,
               REST_PROXY_ERROR,
               message->status_code,
               message->reason_phrase);
  return FALSE;
}

static gboolean
finish_call (RestProxyCall *call, SoupMessage *message, GError **error)
{
  RestProxyCallPrivate *priv;

  g_assert (call);
  g_assert (message);
  priv = GET_PRIVATE (call);

  /* Convert the soup headers in to hash */
  /* FIXME: Eeek..are you allowed duplicate headers? ... */
  g_hash_table_remove_all (priv->response_headers);
  soup_message_headers_foreach (message->response_headers,
      (SoupMessageHeadersForeachFunc)_populate_headers_hash_table,
      priv->response_headers);

  priv->payload = g_strdup (message->response_body->data);
  priv->length = message->response_body->length;

  priv->status_code = message->status_code;
  priv->status_message = g_strdup (message->reason_phrase);

  return _handle_error_from_message (message, error);
}

static void
_call_async_finished_cb (SoupMessage *message,
                         gpointer     userdata)
{
  RestProxyCallAsyncClosure *closure;
  RestProxyCall *call;
  RestProxyCallPrivate *priv;
  GError *error = NULL;

  closure = (RestProxyCallAsyncClosure *)userdata;
  call = closure->call;
  priv = GET_PRIVATE (call);

  finish_call (call, message, &error);

  closure->callback (closure->call,
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
  g_slice_free (RestProxyCallAsyncClosure, closure);
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

static SoupMessage *
prepare_message (RestProxyCall *call, GError **error_out)
{
  RestProxyCallPrivate *priv;
  RestProxyCallClass *call_class;
  const gchar *bound_url, *user_agent;
  SoupMessage *message;
  GError *error = NULL;

  priv = GET_PRIVATE (call);
  call_class = REST_PROXY_CALL_GET_CLASS (call);

  bound_url =_rest_proxy_get_bound_url (priv->proxy);

  if (_rest_proxy_get_binding_required (priv->proxy) && !bound_url)
  {
    g_critical (G_STRLOC ": URL requires binding and is unbound");
    return FALSE;
  }

  /* FIXME: Perhaps excessive memory duplication */
  if (priv->function)
  {
    if (g_str_has_suffix (bound_url, "/"))
    {
      priv->url = g_strdup_printf ("%s%s", bound_url, priv->function);
    } else {
      priv->url = g_strdup_printf ("%s/%s", bound_url, priv->function);
    }
  } else {
    priv->url = g_strdup (bound_url);
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

  message = soup_form_request_new_from_hash (priv->method,
                                             priv->url,
                                             priv->params);

  /* Set the user agent, if one was set in the proxy */
  user_agent = rest_proxy_get_user_agent (priv->proxy);
  if (user_agent) {
    soup_message_headers_append (message->request_headers, "User-Agent", user_agent);
  }

  /* Set the headers */
  g_hash_table_foreach (priv->headers, set_header, message->request_headers);

  return message;
}

/**
 * rest_proxy_call_async:
 * @call: The #RestProxyCall
 * @callback: a #RestProxyCallAsyncCallback to invoke on completion of the call
 * @weak_object: The #GObject to weakly reference and tie the lifecycle too
 * @userdata: data to pass to @callback
 * @error_out: a #GError, or %NULL
 *
 * Asynchronously invoke @call.
 *
 * When the call has finished, @callback will be called.  If @weak_object is
 * disposed during the call then this call will be cancelled.
 */
gboolean
rest_proxy_call_async (RestProxyCall                *call,
                       RestProxyCallAsyncCallback    callback,
                       GObject                      *weak_object,
                       gpointer                      userdata,
                       GError                      **error_out)
{
  RestProxyCallPrivate *priv;
  RestProxyCallClass *call_class;
  SoupMessage *message;
  RestProxyCallAsyncClosure *closure;

  g_return_val_if_fail (REST_IS_PROXY_CALL (call), FALSE);
  priv = GET_PRIVATE (call);
  g_assert (priv->proxy);
  call_class = REST_PROXY_CALL_GET_CLASS (call);

  if (priv->cur_call_closure)
  {
    /* FIXME: Use GError here */
    g_critical (G_STRLOC ": Call already in progress.");
    return FALSE;
  }

  message = prepare_message (call, error_out);
  if (message == NULL)
    goto error;

  closure = g_slice_new0 (RestProxyCallAsyncClosure);
  closure->call = g_object_ref (call);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;
  closure->userdata = userdata;

  priv->cur_call_closure = closure;

  /* Weakly reference this object. We remove our callback if it goes away. */
  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object,
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  g_signal_connect (message,
      "finished",
      (GCallback)_call_async_finished_cb,
      closure);

  _rest_proxy_queue_message (priv->proxy, message);
  g_free (priv->url);
  priv->url = NULL;
  return TRUE;

error:
  g_free (priv->url);
  priv->url = NULL;
  return FALSE;
}

/**
 * rest_proxy_call_cancel:
 * @call: The #RestProxyCall
 *
 * Cancel this call.  It may be too late to not actually send the message, but
 * the callback will not be invoked.
 */
gboolean
rest_proxy_call_cancel (RestProxyCall *call)
{
  RestProxyCallPrivate *priv;
  RestProxyCallAsyncClosure *closure;

  priv = GET_PRIVATE (call);
  closure = priv->cur_call_closure;

  if (closure)
  {
    /* Remove the "finished" signal handler on the message */
    g_signal_handlers_disconnect_by_func (closure->message,
        _call_async_finished_cb,
        closure);

    _rest_proxy_cancel_message (priv->proxy, closure->message);

    g_object_unref (closure->call);
    g_slice_free (RestProxyCallAsyncClosure, closure);
  }

  priv->cur_call_closure = NULL;
  return TRUE;
}

typedef struct
{
  GMainLoop *loop;
  GError *error;
} RestProxyCallRunClosure;

static void
_rest_proxy_call_async_cb (RestProxyCall *call,
                           const GError  *error,
                           GObject       *weak_object,
                           gpointer       userdata)
{
  RestProxyCallRunClosure *closure = (RestProxyCallRunClosure *)userdata;

  /* *duplicate* not propagate the error */
  if (error)
    closure->error = g_error_copy (error);

  g_main_loop_quit (closure->loop);
}

gboolean
rest_proxy_call_run (RestProxyCall *call,
                     GMainLoop    **loop_out,
                     GError       **error_out)
{
  gboolean res = TRUE;
  GError *error = NULL;
  RestProxyCallRunClosure closure = { NULL, NULL};

  closure.loop = g_main_loop_new (NULL, FALSE);

  if (loop_out)
    *loop_out = closure.loop;

  res = rest_proxy_call_async (call,
      _rest_proxy_call_async_cb,
      NULL,
      &closure,
      &error);

  if (!res)
  {
    g_propagate_error (error_out, error);
    goto error;
  }

  g_main_loop_run (closure.loop);

  if (closure.error)
  {
    /* If the caller has asked for the error then propagate else free it */
    if (error_out)
    {
      g_propagate_error (error_out, closure.error);
    } else {
      g_clear_error (&(closure.error));
    }
    res = FALSE;
  }

error:
  g_main_loop_unref (closure.loop);
  return res;
}

gboolean
rest_proxy_call_sync (RestProxyCall *call,
                      GError       **error_out)
{
  RestProxyCallPrivate *priv;
  SoupMessage *message;
  guint status;
  gboolean ret;

  priv = GET_PRIVATE (call);

  message = prepare_message (call, error_out);
  if (!message)
    return FALSE;

  status = _rest_proxy_send_message (priv->proxy, message);

  ret = finish_call (call, message, error_out);

  g_object_unref (message);

  return ret;
}

/**
 * rest_proxy_call_lookup_response_header
 *
 * @call: The #RestProxyCall
 * @header: The name of the header to lookup.
 *
 * Returns: The string value of the header @header or NULL if that header is
 * not present or there are no headers.
 */
const gchar *
rest_proxy_call_lookup_response_header (RestProxyCall *call,
                                        const gchar   *header)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  if (!priv->response_headers)
  {
    return NULL;
  }

  return g_hash_table_lookup (priv->response_headers, header);
}

/**
 * rest_proxy_call_get_response_headers
 *
 * @call: The #RestProxyCall
 *
 * Returns: A pointer to a hash table of headers. This hash table must not be
 * changed. You should call g_hash_table_unref() when you have finished with
 * it.
 */
GHashTable *
rest_proxy_call_get_response_headers (RestProxyCall *call)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  return priv->length;
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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  return priv->payload;
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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  return priv->status_code;
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
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  return priv->status_message;
}
