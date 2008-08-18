#ifndef _REST_PROXY
#define _REST_PROXY

#include <glib-object.h>

G_BEGIN_DECLS

#define REST_TYPE_PROXY rest_proxy_get_type()

#define REST_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REST_TYPE_PROXY, RestProxy))

#define REST_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), REST_TYPE_PROXY, RestProxyClass))

#define REST_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REST_TYPE_PROXY))

#define REST_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), REST_TYPE_PROXY))

#define REST_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), REST_TYPE_PROXY, RestProxyClass))

typedef struct {
  GObject parent;
} RestProxy;

typedef struct {
  GObjectClass parent_class;
} RestProxyClass;

typedef void (*RestProxyCallRawCallback)(RestProxy *proxy, 
    guint status_code, 
    const gchar *response_message,
    GHashTable *headers,
    const gchar *payload,
    gssize len,
    GObject *weak_object,
    gpointer userdata);

GType rest_proxy_get_type (void);

RestProxy *rest_proxy_new (const gchar *url_format, 
    gboolean binding_required);

gboolean rest_proxy_call_raw_async (RestProxy *proxy,
    const gchar *function,
    const gchar *method,
    RestProxyCallRawCallback callback,
    GObject *weak_object,
    gpointer userdata,
    GError *error,
    const gchar *first_field_name,
    ...);

G_END_DECLS

#endif /* _REST_PROXY */

