#ifndef _REST_PROXY_CALL
#define _REST_PROXY_CALL

#include <glib-object.h>

G_BEGIN_DECLS

#define REST_TYPE_PROXY_CALL rest_proxy_call_get_type()

#define REST_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REST_TYPE_PROXY_CALL, RestProxyCall))

#define REST_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), REST_TYPE_PROXY_CALL, RestProxyCallClass))

#define REST_IS_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REST_TYPE_PROXY_CALL))

#define REST_IS_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), REST_TYPE_PROXY_CALL))

#define REST_PROXY_CALL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), REST_TYPE_PROXY_CALL, RestProxyCallClass))

typedef struct {
  GObject parent;
} RestProxyCall;

typedef struct {
  GObjectClass parent_class;
} RestProxyCallClass;

GType rest_proxy_call_get_type (void);

/* Functions for dealing with request */
void rest_proxy_call_set_method (RestProxyCall *call,
                                 const gchar   *method);

void rest_proxy_call_add_header (RestProxyCall *call,
                                 const gchar   *header,
                                 const gchar   *value);

void rest_proxy_call_add_headers (RestProxyCall *call,
                                  const char *first_header_name,
                                  ...);

void rest_proxy_call_add_headers_from_valist (RestProxyCall *call,
                                              va_list headers);

void rest_proxy_call_add_headers_from_hash (RestProxyCall *call,
                                            GHashTable    *headers);

const gchar *rest_proxy_call_lookup_header (RestProxyCall *call,
                                            const gchar   *header);

void rest_proxy_call_remove_header (RestProxyCall *call,
                                    const gchar   *header);

void rest_proxy_call_add_param (RestProxyCall *call,
                                const gchar   *param,
                                const gchar   *value);

void rest_proxy_call_add_params (RestProxyCall *call,
                                 ...);

void rest_proxy_call_add_params_from_valist (RestProxyCall *call,
                                             va_list params);

void rest_proxy_call_add_params_from_hash (RestProxyCall *call,
                                           GHashTable    *params);

const gchar *rest_proxy_call_lookup_param (RestProxyCall *call,
                                           const gchar   *param);

void rest_proxy_call_remove_param (RestProxyCall *call,
                                   const gchar   *param);

gboolean rest_proxy_call_run (RestProxyCall *call,
                              GMainLoop **loop,
                              GError **error);

typedef void (*RestProxyCallAsyncCallback)(RestProxyCall *call,
                                           GObject       *weak_object,
                                           gpointer       userdata);

gboolean rest_proxy_call_async (RestProxyCall                *call,
                                RestProxyCallAsyncCallback    callback,
                                GObject                      *weak_object,
                                gpointer                      userdata,
                                GError                      **error);

/* Functions for dealing with responses */

const gchar *rest_proxy_call_lookup_response_header (RestProxyCall *call,
                                                     const gchar   *header);

GHashTable *rest_proxy_call_get_response_headers (RestProxyCall *call);

goffset rest_proxy_call_get_payload_length (RestProxyCall *call);
const gchar *rest_proxy_call_get_payload (RestProxyCall *call);
guint rest_proxy_call_get_status_code (RestProxyCall *call);
const gchar *rest_proxy_call_get_response_message (RestProxyCall *call);

G_END_DECLS

#endif /* _REST_PROXY_CALL */

