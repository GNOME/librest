#ifndef _REST_PROXY
#define _REST_PROXY

#include <glib-object.h>
#include <rest/rest-proxy-call.h>

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
  gboolean (*bind_valist)(RestProxy *proxy, va_list params);
  RestProxyCall *(*new_call)(RestProxy *proxy);
  gboolean (*simple_run_valist)(RestProxy *proxy, gchar **payload, 
      goffset *len, GError **error, va_list params);
} RestProxyClass;

typedef void (*RestProxyCallRawCallback)(RestProxy   *proxy, 
                                         guint        status_code, 
                                         const gchar *response_message,
                                         GHashTable  *headers,
                                         const gchar *payload,
                                         goffset      len,
                                         GObject     *weak_object,
                                         gpointer     userdata);

GType rest_proxy_get_type (void);

RestProxy *rest_proxy_new (const gchar *url_format, 
                           gboolean     binding_required);

gboolean rest_proxy_bind (RestProxy *proxy,
                          ...);

gboolean rest_proxy_bind_valist (RestProxy *proxy,
                                 va_list    params);
G_GNUC_DEPRECATED
gboolean rest_proxy_call_raw_async (RestProxy               *proxy,
                                    const gchar             *function,
                                    const gchar             *method,
                                    RestProxyCallRawCallback callback,
                                    GObject                 *weak_object,
                                    gpointer                 userdata,
                                    GError                 **error,
                                    const gchar             *first_field_name,
                                    ...);

G_GNUC_DEPRECATED
gboolean rest_proxy_call_raw_async_valist (RestProxy               *proxy,
                                           const gchar             *function,
                                           const gchar             *method,
                                           RestProxyCallRawCallback callback,
                                           GObject                 *weak_object,
                                           gpointer                 userdata,
                                           GError                 **error,
                                           const gchar             *first_field_name,
                                           va_list                  params);

G_GNUC_DEPRECATED
gboolean rest_proxy_run_raw (RestProxy   *proxy,
                             const gchar *function,
                             const gchar *method,
                             guint       *status_code,
                             gchar      **response_message,
                             GHashTable **headers,
                             gchar      **payload,
                             goffset     *len,
                             GError     **error,
                             const gchar *first_field_name,
                             ...);

RestProxyCall *rest_proxy_new_call (RestProxy *proxy);

G_GNUC_NULL_TERMINATED
gboolean rest_proxy_simple_run (RestProxy *proxy, 
                                gchar    **payload, 
                                goffset   *len,
                                GError   **error,
                                ...);
gboolean rest_proxy_simple_run_valist (RestProxy *proxy, 
                                       gchar    **payload, 
                                       goffset   *len,
                                       GError   **error,
                                       va_list    params);
G_END_DECLS

#endif /* _REST_PROXY */

