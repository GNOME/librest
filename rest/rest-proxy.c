#include <libsoup/soup.h>

#include "rest-proxy.h"

G_DEFINE_TYPE (RestProxy, rest_proxy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_PROXY, RestProxyPrivate))

typedef struct _RestProxyPrivate RestProxyPrivate;

struct _RestProxyPrivate {
  gchar *url_format;
  gchar *url;
  gboolean binding_required;
  SoupSession *session;
};

enum
{
  PROP0 = 0,
  PROP_URL_FORMAT,
  PROP_BINDING_REQUIRED
};

static void
rest_proxy_get_property (GObject *object,
                         guint property_id,
                         GValue *value,
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
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
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
      priv->url;
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (rest_proxy_parent_class)->dispose)
    G_OBJECT_CLASS (rest_proxy_parent_class)->dispose (object);
}

static void
rest_proxy_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (rest_proxy_parent_class)->finalize)
    G_OBJECT_CLASS (rest_proxy_parent_class)->finalize (object);
}

static void
rest_proxy_class_init (RestProxyClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RestProxyPrivate));

  object_class->get_property = rest_proxy_get_property;
  object_class->set_property = rest_proxy_set_property;
  object_class->dispose = rest_proxy_dispose;
  object_class->finalize = rest_proxy_finalize;

  pspec = g_param_spec_string ("url-format", 
      "url-format",
      "Format string for the RESTful url for this proxy",
      NULL,
      G_PARAM_READWRITE);
  g_object_class_install_property (object_class, 
      PROP_URL_FORMAT,
      pspec);

  pspec = g_param_spec_boolean ("binding-required",
      "binding-required",
      "Whether the URL format string requires binding",
      FALSE,
      G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
      PROP_BINDING_REQUIRED,
      pspec);
}

static void
rest_proxy_init (RestProxy *self)
{
  RestProxyPrivate *priv = GET_PRIVATE (self);

  /* TODO: Use proxy settings, etc etc... */
  priv->session = soup_session_async_new ();
}

RestProxy *
rest_proxy_new (const gchar *url_format, 
                gboolean binding_required)
{
  return g_object_new (REST_TYPE_PROXY, 
      "url-format", url_format,
      "binding-required", binding_required,
      NULL);
}

gboolean
rest_proxy_bind (RestProxy *proxy, ...)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);
  va_list params;

  g_return_val_if_fail (proxy != NULL, FALSE);
  g_return_val_if_fail (priv->url_format != NULL, FALSE);
  g_return_val_if_fail (priv->binding_required != TRUE, FALSE);

  g_free (priv->url);
  priv->url = g_strdup_vprintf (priv->url_format, params);
}

typedef struct {
  RestProxy *proxy;
  RestProxyCallRawCallback callback;
  GObject *weak_object;
  gpointer userdata;
  SoupMessage *message;
} RestProxyCallRawAsyncClosure;

static void _call_raw_async_weak_notify_cb (gpointer *data, 
                                            GObject *dead_object);
static void _call_raw_async_finished_cb (SoupMessage *message,
                                         gpointer userdata);

static GHashTable *
_populate_headers_hash_table (const gchar *name,
                              const gchar *value,
                              gpointer userdata)
{
  GHashTable *headers = (GHashTable *)userdata;

  g_hash_table_insert (headers, g_strdup (name), g_strdup (value));
}

static void
_call_raw_async_finished_cb (SoupMessage *message,
                             gpointer userdata)
{
  RestProxyCallRawAsyncClosure *closure;
  GHashTable *headers;

  closure = (RestProxyCallRawAsyncClosure *)userdata;

  headers = g_hash_table_new_full (g_str_hash, 
      g_str_equal,
      g_free,
      g_free);

  soup_message_headers_foreach (message->response_headers,
      (SoupMessageHeadersForeachFunc)_populate_headers_hash_table,
      headers);

  closure->callback (closure->proxy,
      message->status_code,
      message->reason_phrase,
      headers,
      message->response_body->data,
      message->response_body->length,
      closure->weak_object,
      closure->userdata);

  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object, 
        (GWeakNotify)_call_raw_async_weak_notify_cb,
        closure);
  }

  g_object_unref (closure->proxy);
  g_free (closure);
}

static void 
_call_raw_async_weak_notify_cb (gpointer *data,
                                GObject *dead_object)
{
  RestProxyCallRawAsyncClosure *closure;

  closure = (RestProxyCallRawAsyncClosure *)data;

  g_signal_handlers_disconnect_by_func (closure->message,
      _call_raw_async_finished_cb,
      closure);
  g_object_unref (closure->proxy);
  g_free (closure);
}

gboolean
rest_proxy_call_raw_async (RestProxy *proxy,
                           const gchar *function,
                           const gchar *method,
                           RestProxyCallRawCallback callback,
                           GObject *weak_object,
                           gpointer userdata,
                           GError *error,
                           const gchar *first_field_name,
                           ...)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);
  va_list params;
  SoupMessage *message = NULL;
  gchar *url = NULL;
  RestProxyCallRawAsyncClosure *closure;

  if (priv->binding_required && !priv->url)
  {
    g_critical (G_STRLOC ": URL requires binding and is unbound");
    /* FIXME: Return a GError */
    return FALSE;
  }

  if (!priv->binding_required)
  {
    priv->url = g_strdup (priv->url_format);
  }

  if (function)
  {
    if (g_str_has_suffix (priv->url, "/"))
    {
      url = g_strdup_printf ("%s%s", priv->url, function);
    } else {
      url = g_strdup_printf ("%s/%s", priv->url, function);
    }
  } else {
    url = g_strdup (priv->url);
  }

  if (first_field_name)
  {
    message = soup_form_request_new (method,
        url,
        first_field_name,
        params);
  } else {
    message = soup_message_new (method, url);
  }

  closure = g_new0 (RestProxyCallRawAsyncClosure, 1);
  closure->proxy = g_object_ref (proxy);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;

  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object, 
        (GWeakNotify)_call_raw_async_weak_notify_cb, 
        closure);
  }

  g_signal_connect (message,
      "finished", 
      (GCallback)_call_raw_async_finished_cb,
      closure);

  /* TODO: Should we do something in this callback ? */
  soup_session_queue_message (priv->session,
      message,
      NULL,
      NULL);

  return TRUE;
}
