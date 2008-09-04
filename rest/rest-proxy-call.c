#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <libsoup/soup.h>

#include "rest-private.h"

G_DEFINE_TYPE (RestProxyCall, rest_proxy_call, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_PROXY_CALL, RestProxyCallPrivate))

typedef struct _RestProxyCallPrivate RestProxyCallPrivate;

struct _RestProxyCallPrivate {
  gchar *method;
  gchar *function;
  GHashTable *headers;
  GHashTable *params;

  GHashTable *response_headers;
  goffset length;
  gchar *payload;
  guint status_code;
  gchar *response_message;

  RestProxy *proxy;
};

static void
rest_proxy_call_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_call_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
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
  g_free (priv->response_message);

  if (G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize)
    G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize (object);
}

static void
rest_proxy_call_class_init (RestProxyCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RestProxyCallPrivate));

  object_class->get_property = rest_proxy_call_get_property;
  object_class->set_property = rest_proxy_call_set_property;
  object_class->dispose = rest_proxy_call_dispose;
  object_class->finalize = rest_proxy_call_finalize;
}

static void
rest_proxy_call_init (RestProxyCall *self)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (self);

  priv->method = g_strdup ("GET");

  priv->params = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        g_free);
  priv->headers = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);
}

RestProxyCall*
rest_proxy_call_new (void)
{
  return g_object_new (REST_TYPE_PROXY_CALL, NULL);
}

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

void
rest_proxy_call_set_function (RestProxyCall *call,
                              const gchar   *function)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_free (priv->function);

  priv->function = g_strdup (function);
}

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

void
rest_proxy_call_add_headers (RestProxyCall *call,
                             const char *first_header_name,
                             ...)
{
  va_list headers;

  va_start (headers, first_header_name);
  rest_proxy_call_add_headers_from_valist (call, headers);
  va_end (headers);
}

void 
rest_proxy_call_add_headers_from_valist (RestProxyCall *call,
                                         va_list headers)
{
  const gchar *header = NULL;
  const gchar *value = NULL;

  while ((header = va_arg (headers, const gchar *)) != NULL)
  {
    value = va_arg (headers, const gchar *);
    rest_proxy_call_add_header (call, header, value);
  }
}

const gchar *
rest_proxy_call_lookup_header (RestProxyCall *call,
                               const gchar   *header)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return g_hash_table_lookup (priv->headers, header);
}

void 
rest_proxy_call_remove_header (RestProxyCall *call,
                               const gchar   *header)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_hash_table_remove (priv->headers, header);
}

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

void
rest_proxy_call_add_params (RestProxyCall *call,
                            const char *first_param_name,
                            ...)
{
  va_list params;

  va_start (params, first_param_name);
  rest_proxy_call_add_params_from_valist (call, params);
  va_end (params);
}

void 
rest_proxy_call_add_params_from_valist (RestProxyCall *call,
                                        va_list params)
{
  const gchar *param = NULL;
  const gchar *value = NULL;

  while ((param = va_arg (params, const gchar *)) != NULL)
  {
    value = va_arg (params, const gchar *);
    rest_proxy_call_add_param (call, param, value);
  }
}

const gchar *
rest_proxy_call_lookup_param (RestProxyCall *call,
                              const gchar   *param)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  return g_hash_table_lookup (priv->params, param);
}

void 
rest_proxy_call_remove_param (RestProxyCall *call,
                              const gchar   *param)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);

  g_hash_table_remove (priv->params, param);
}

gboolean
rest_proxy_call_run (RestProxyCall *call,
                     GMainLoop    **loop,
                     GError       **error)
{
  return FALSE;
}

typedef struct {
  RestProxyCall *call;
  RestProxyCallAsyncCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
} RestProxyCallAsyncClosure;

static void _call_async_weak_notify_cb (gpointer *data, 
                                        GObject  *dead_object);

static void _call_async_finished_cb (SoupMessage *message,
                                     gpointer     userdata);

static void
_populate_headers_hash_table (const gchar *name,
                              const gchar *value,
                              gpointer userdata)
{
  GHashTable *headers = (GHashTable *)userdata;

  g_hash_table_insert (headers, g_strdup (name), g_strdup (value));
}

static void
_call_async_finished_cb (SoupMessage *message,
                         gpointer     userdata)
{
  RestProxyCallAsyncClosure *closure;
  RestProxyCall *call;
  RestProxyCallPrivate *priv;

  closure = (RestProxyCallAsyncClosure *)userdata;
  call = closure->call;
  priv = GET_PRIVATE (call);

  /* Convert the soup headers in to hash */
  /* FIXME: Eeek..are you allowed duplicate headers? ... */
  g_hash_table_remove_all (priv->response_headers);
  soup_message_headers_foreach (message->response_headers,
      (SoupMessageHeadersForeachFunc)_populate_headers_hash_table,
      priv->response_headers);

  closure->callback (closure->call,
      closure->weak_object,
      closure->userdata);

  /* Success. We don't need the weak reference any more */
  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object, 
        (GWeakNotify)_call_async_weak_notify_cb,
        closure);
  }

  g_object_unref (closure->call);
  g_free (closure);
}

static void 
_call_async_weak_notify_cb (gpointer *data,
                            GObject  *dead_object)
{
  RestProxyCallAsyncClosure *closure;

  closure = (RestProxyCallAsyncClosure *)data;

  /* Remove the "finished" signal handler on the message */
  g_signal_handlers_disconnect_by_func (closure->message,
      _call_async_finished_cb,
      closure);

  g_object_unref (closure->call);
  g_free (closure);
}

gboolean
rest_proxy_call_async (RestProxyCall                *call,
                       RestProxyCallAsyncCallback    callback,
                       GObject                      *weak_object,
                       gpointer                      userdata,
                       GError                      **error)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  const gchar *bound_url;
  gchar *url = NULL;
  SoupMessage *message;
  RestProxyCallAsyncClosure *closure;

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
      url = g_strdup_printf ("%s%s", bound_url, priv->function);
    } else {
      url = g_strdup_printf ("%s/%s", bound_url, priv->function);
    }
  } else {
    url = g_strdup (bound_url);
  }

  message = soup_form_request_new_from_hash (priv->method,
                                             url,
                                             priv->params);

  closure = g_slice_new0 (RestProxyCallAsyncClosure);
  closure->call = g_object_ref (call);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;
  closure->userdata = userdata;

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
  g_free (url);
  return TRUE;
}

void
_rest_proxy_call_set_proxy (RestProxyCall *call, 
                            RestProxy     *proxy)
{
  RestProxyCallPrivate *priv = GET_PRIVATE (call);
  priv->proxy = g_object_ref (proxy);
}
