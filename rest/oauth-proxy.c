#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "oauth-proxy.h"
#include "oauth-proxy-private.h"
#include "oauth-proxy-call.h"

G_DEFINE_TYPE (OAuthProxy, oauth_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_CONSUMER_KEY,
  PROP_CONSUMER_SECRET
};

static RestProxyCall *
_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (OAUTH_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);
  
  return call;
}

static void
oauth_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (object);
  
  switch (property_id) {
  case PROP_CONSUMER_KEY:
    if (priv->consumer_key)
      g_free (priv->consumer_key);
    priv->consumer_key = g_value_dup_string (value);
    break;
  case PROP_CONSUMER_SECRET:
    if (priv->consumer_secret)
      g_free (priv->consumer_secret);
    priv->consumer_secret = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
oauth_proxy_finalize (GObject *object)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (object);
  
  g_free (priv->consumer_key);
  g_free (priv->consumer_secret);
  g_free (priv->token);
  g_free (priv->token_secret);
  
  if (G_OBJECT_CLASS (oauth_proxy_parent_class)->finalize)
    G_OBJECT_CLASS (oauth_proxy_parent_class)->finalize (object);
}

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

static void
oauth_proxy_class_init (OAuthProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (OAuthProxyPrivate));

  object_class->set_property = oauth_proxy_set_property;
  object_class->finalize = oauth_proxy_finalize;

  proxy_class->new_call = _new_call;

  pspec = g_param_spec_string ("consumer-key",  "consumer-key",
                               "The consumer key", NULL,
                               G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, 
                                   PROP_CONSUMER_KEY,
                                   pspec);

  pspec = g_param_spec_string ("consumer-secret",  "consumer-secret",
                               "The consumer secret", NULL,
                               G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, 
                                   PROP_CONSUMER_SECRET,
                                   pspec);
}

static void
oauth_proxy_init (OAuthProxy *self)
{
}

RestProxy *
oauth_proxy_new (const char *consumer_key,
                 const char *consumer_secret,
                 const gchar *url_format,
                 gboolean binding_required)
{
  return g_object_new (OAUTH_TYPE_PROXY, 
                       "consumer-key", consumer_key,
                       "consumer-secret", consumer_secret,
                       "url-format", url_format,
                       "binding-required", binding_required,
                       NULL);
}

typedef struct {
  OAuthProxyAuthCallback callback;
  gpointer user_data;
} AuthData;

static void
auth_callback (RestProxyCall *call,
               GError        *error,
               GObject       *weak_object,
               gpointer       user_data)
{
  AuthData *data = user_data;
  OAuthProxy *proxy = NULL;
  OAuthProxyPrivate *priv;
  GHashTable *form;

  if (error)
    /* TODO: something useful! */
    g_error (error->message);
  
  g_object_get (call, "proxy", &proxy, NULL);
  priv = PROXY_GET_PRIVATE (proxy);

  form = soup_form_decode (rest_proxy_call_get_payload (call));
  priv->token = g_strdup (g_hash_table_lookup (form, "oauth_token"));
  priv->token_secret = g_strdup (g_hash_table_lookup (form, "oauth_token_secret"));
  g_hash_table_destroy (form);

  data->callback (proxy, error, weak_object, data->user_data);
  
  g_slice_free (AuthData, data);
  g_object_unref (call);
  g_object_unref (proxy);
}

gboolean
oauth_proxy_auth_step_async (OAuthProxy *proxy,
                             const char *fragment,
                             OAuthProxyAuthCallback callback,
                             GObject *weak_object,
                             gpointer user_data,
                             GError **error_out)
{
  RestProxyCall *call;
  AuthData *data;

  rest_proxy_bind (REST_PROXY (proxy), fragment);
  call = rest_proxy_new_call (REST_PROXY (proxy));

  data = g_slice_new0 (AuthData);
  data->callback = callback;
  data->user_data = user_data;

  return rest_proxy_call_async (call, auth_callback, weak_object, data, error_out);
  /* TODO: if call_async fails, the call is leaked */
}

void
oauth_proxy_auth_step (OAuthProxy *proxy, const char *fragment)
{
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  GError *error = NULL;
  RestProxyCall *call;
  GHashTable *form;

  rest_proxy_bind (REST_PROXY (proxy), fragment);
  call = rest_proxy_new_call (REST_PROXY (proxy));

  if (!rest_proxy_call_run (call, NULL, &error))
    g_error ("Cannot make call: %s", error->message);

  form = soup_form_decode (rest_proxy_call_get_payload (call));
  priv->token = g_strdup (g_hash_table_lookup (form, "oauth_token"));
  priv->token_secret = g_strdup (g_hash_table_lookup (form, "oauth_token_secret"));
  g_hash_table_destroy (form);

  g_object_unref (call);
}

/**
 * oauth_proxy_get_token:
 * @proxy:
 *
 * Get the current request or access token.
 *
 * Returns: the token, or %NULL if there is no token yet.  This string is owned
 * by #OAuthProxy and should not be freed.
 */
const char *
oauth_proxy_get_token (OAuthProxy *proxy)
{  
  OAuthProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->token;
}
