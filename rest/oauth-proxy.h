#ifndef _OAUTH_PROXY
#define _OAUTH_PROXY

#include <rest/rest-proxy.h>

G_BEGIN_DECLS

#define OAUTH_TYPE_PROXY oauth_proxy_get_type()

#define OAUTH_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_PROXY, OAuthProxy))

#define OAUTH_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_PROXY, OAuthProxyClass))

#define OAUTH_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_PROXY))

#define OAUTH_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_PROXY))

#define OAUTH_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_PROXY, OAuthProxyClass))

typedef struct {
  RestProxy parent;
} OAuthProxy;

typedef struct {
  RestProxyClass parent_class;
} OAuthProxyClass;

GType oauth_proxy_get_type (void);

RestProxy* oauth_proxy_new (const char *consumer_key,
                            const char *consumer_secret,
                            const gchar *url_format,
                            gboolean binding_required);

typedef void (*OAuthProxyAuthCallback)(OAuthProxy *proxy,
                                           GError        *error,
                                           GObject       *weak_object,
                                           gpointer       userdata);

void oauth_proxy_auth_step (OAuthProxy *proxy, const char *fragment);

gboolean oauth_proxy_auth_step_async (OAuthProxy *proxy,
                             const char *fragment,
                             OAuthProxyAuthCallback callback,
                             GObject *weak_object,
                             gpointer user_data,
                             GError **error_out);

const char * oauth_proxy_get_token (OAuthProxy *proxy);

G_END_DECLS

#endif /* _OAUTH_PROXY */
