#include <libsoup/soup.h>
#include <string.h>

#include "rest-proxy.h"
#include "rest-private.h"

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
      priv->url = NULL;
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_dispose (GObject *object)
{
  RestProxyPrivate *priv = GET_PRIVATE (object);

  if (G_OBJECT_CLASS (rest_proxy_parent_class)->dispose)
    G_OBJECT_CLASS (rest_proxy_parent_class)->dispose (object);

  if (priv->session)
  {
    g_object_unref (priv->session);
    priv->session = NULL;
  }
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
  g_return_val_if_fail (priv->binding_required == TRUE, FALSE);

  g_free (priv->url);
  va_start (params, proxy);
  priv->url = g_strdup_vprintf (priv->url_format, params);
  va_end (params);

  return TRUE;
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

static void
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

  /* Convert the soup headers in to hash */
  /* FIXME: Eeek..are you allowed duplicate headers? ... */
  soup_message_headers_foreach (message->response_headers,
      (SoupMessageHeadersForeachFunc)_populate_headers_hash_table,
      headers);

  closure->callback (closure->proxy,  /* proxy */
      message->status_code,           /* status code */
      message->reason_phrase,         /* status message */
      headers,                        /* hash of headers */
      message->response_body->data,   /* payload */
      message->response_body->length, /* payload length */
      closure->weak_object,
      closure->userdata);

  /* Finished with the headers. */
  g_hash_table_unref (headers);

  /* Success. We don't need the weak reference any more */
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

  /* Remove the "finished" signal handler on the message */
  g_signal_handlers_disconnect_by_func (closure->message,
      _call_raw_async_finished_cb,
      closure);

  g_object_unref (closure->proxy);
  g_free (closure);
}

gboolean
rest_proxy_call_raw_async_valist (RestProxy *proxy,
                                  const gchar *function,
                                  const gchar *method,
                                  RestProxyCallRawCallback callback,
                                  GObject *weak_object,
                                  gpointer userdata,
                                  GError **error,
                                  const gchar *first_field_name,
                                  va_list params)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);
  SoupMessage *message = NULL;
  gchar *url = NULL;
  RestProxyCallRawAsyncClosure *closure;
  SoupURI *uri;
  gchar *formdata;

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

  /* FIXME: Perhaps excessive memory duplication */
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
    uri = soup_uri_new (url);

    if (!uri)
    {
      g_warning (G_STRLOC ": Unable to parse URI: %s", url);
      return FALSE;
    }

    formdata = soup_form_encode_valist (first_field_name, params);

    /* In the GET case we must encode into the URI */
    if (g_str_equal (method, "GET"))
    {
      soup_uri_set_query (uri, formdata);
      g_free (formdata);
      formdata = NULL;
    }

    message = soup_message_new_from_uri (method, uri);
    soup_uri_free (uri);

    /* In the POST case we must encode into the request */
    if (g_str_equal (method, "POST"))
    {
      /* This function takes over the memory so no need to free */
      soup_message_set_request (message,
          "application/x-www-form-urlencoded",
          SOUP_MEMORY_TAKE,
          formdata,
          strlen (formdata));
      formdata = NULL;
    }

    if (formdata)
    {
      g_warning (G_STRLOC ": Unexpected method: %s", method);
      g_free (formdata);
      return FALSE;
    }
  } else {
    message = soup_message_new (method, url);
  }

  /* Set up the closure. */
  /* FIXME: For cancellation perhaps we should return an opaque like dbus */
  closure = g_new0 (RestProxyCallRawAsyncClosure, 1);
  closure->proxy = g_object_ref (proxy);
  closure->callback = callback;
  closure->weak_object = weak_object;
  closure->message = message;
  closure->userdata = userdata;

  /* Weakly reference this object. We remove our callback if it goes away. */
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

gboolean
rest_proxy_call_raw_async (RestProxy *proxy,
                           const gchar *function,
                           const gchar *method,
                           RestProxyCallRawCallback callback,
                           GObject *weak_object,
                           gpointer userdata,
                           GError **error,
                           const gchar *first_field_name,
                           ...)

{
  gboolean res;
  va_list params;

  va_start (params, first_field_name);
  res = rest_proxy_call_raw_async_valist (proxy,
      function,
      method,
      callback,
      weak_object,
      userdata,
      error,
      first_field_name,
      params);
  va_end (params);

  return res;
}

typedef struct 
{
  GMainLoop *loop;
  guint *status_code;
  gchar **response_message;
  GHashTable **headers;
  gchar **payload;
  goffset *len;
} RestProxyRunRawClosure;

static void
_call_raw_async_for_run_raw_cb (RestProxy *proxy,
                                guint status_code,
                                const gchar *response_message,
                                GHashTable *headers,
                                const gchar *payload,
                                goffset len,
                                GObject *weak_object,
                                gpointer userdata)
{
  RestProxyRunRawClosure *closure;

  closure = (RestProxyRunRawClosure *)userdata;

  if (closure->status_code)
    *(closure->status_code) = status_code;

  if (closure->response_message)
    *(closure->response_message) = g_strdup (response_message);

  if (closure->headers)
    *(closure->headers) = g_hash_table_ref (headers);

  if (closure->payload)
    *(closure->payload) = g_memdup (payload, len);

  if (closure->len)
    *(closure->len) = len;

  g_main_loop_quit (closure->loop);
}

gboolean
rest_proxy_run_raw (RestProxy *proxy,
                    const gchar *function,
                    const gchar *method,
                    guint *status_code,
                    gchar **response_message,
                    GHashTable **headers,
                    gchar **payload,
                    goffset *len,
                    GError **error,
                    const gchar *first_field_name,
                    ...)
{
  RestProxyRunRawClosure *closure;
  va_list params;

  gboolean res = TRUE;

  closure = g_new0 (RestProxyRunRawClosure, 1);
  closure->loop = g_main_loop_new (NULL, FALSE);
  closure->payload = payload;
  closure->len = len;

  va_start (params, first_field_name);
  res = rest_proxy_call_raw_async_valist (proxy,
      function,
      method,
      _call_raw_async_for_run_raw_cb,
      NULL,
      closure,
      error,
      first_field_name,
      params);
  va_end (params);

  if (!res)
    goto error;

  g_main_loop_run (closure->loop);

error:
  g_main_loop_unref (closure->loop);
  g_free (closure);
  return res;
}

typedef struct
{
  gpointer userdata;
  GObject *weak_object;
  RestProxyCallJsonCallback callback;
} RestProxyCallJsonAsyncClosure;

static void
_call_raw_async_for_json_async_cb (RestProxy *proxy,
                                   guint status_code,
                                   const gchar *response_message,
                                   GHashTable *headers,
                                   const gchar *payload,
                                   goffset len,
                                   GObject *weak_object,
                                   gpointer userdata)
{
  GError *error = NULL;
  JsonParser *parser;
  JsonNode *root;
  RestProxyCallJsonAsyncClosure *closure;

  closure = (RestProxyCallJsonAsyncClosure *)userdata;

  parser = json_parser_new ();

  if (!json_parser_load_from_data (parser, payload, len, &error))
  {
    g_warning (G_STRLOC ": Error when parsing from JSON: %s", error->message);
    g_clear_error (&error);
    closure->callback (proxy,
        status_code,
        response_message,
        headers,
        NULL,
        weak_object,
        closure->userdata);
  } else {
    root = json_parser_get_root (parser);
    closure->callback (proxy,
        status_code,
        response_message,
        headers,
        root,
        weak_object,
        closure->userdata);
  }

  g_object_unref (parser);
  g_free (closure);
}

gboolean
rest_proxy_call_json_async_valist (RestProxy *proxy,
                                   const gchar *function,
                                   const gchar *method,
                                   RestProxyCallJsonCallback callback,
                                   GObject *weak_object,
                                   gpointer userdata,
                                   GError **error,
                                   const gchar *first_field_name,
                                   va_list params)
{
  RestProxyCallJsonAsyncClosure *closure;

  closure = g_new0 (RestProxyCallJsonAsyncClosure, 1);
  gboolean res;

  res = rest_proxy_call_raw_async_valist (proxy,
      function,
      method,
      _call_raw_async_for_json_async_cb,
      weak_object,
      closure,
      error,
      first_field_name,
      params);

  return res;
}

gboolean
rest_proxy_call_json_async (RestProxy *proxy,
                            const gchar *function,
                            const gchar *method,
                            RestProxyCallJsonCallback callback,
                            GObject *weak_object,
                            gpointer userdata,
                            GError **error,
                            const gchar *first_field_name,
                            ...)

{
  gboolean res;
  va_list params;

  va_start (params, first_field_name);
  res = rest_proxy_call_json_async_valist (proxy,
      function,
      method,
      callback,
      weak_object,
      userdata,
      error,
      first_field_name,
      params);
  va_end (params);

  return res;
}

RestProxyCall *
rest_proxy_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (REST_TYPE_PROXY_CALL, NULL);
  _rest_proxy_call_set_proxy (call, proxy);

  return call;
}


gboolean
_rest_proxy_get_binding_required (RestProxy *proxy)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);

  return priv->binding_required;
}

const gchar *
_rest_proxy_get_bound_url (RestProxy *proxy)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);

  if (!priv->url && !priv->binding_required)
  {
    priv->url = g_strdup (priv->url_format);
  }

  return priv->url;
}

void
_rest_proxy_queue_message (RestProxy   *proxy,
                           SoupMessage *message)
{
  RestProxyPrivate *priv = GET_PRIVATE (proxy);

  soup_session_queue_message (priv->session,
      message,
      NULL,
      NULL);
}
