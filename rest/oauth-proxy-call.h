#ifndef _OAUTH_PROXY_CALL
#define _OAUTH_PROXY_CALL

#include <rest/rest-proxy-call.h>

G_BEGIN_DECLS

#define OAUTH_TYPE_PROXY_CALL oauth_proxy_call_get_type()

#define OAUTH_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OAUTH_TYPE_PROXY_CALL, OAuthProxyCall))

#define OAUTH_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), OAUTH_TYPE_PROXY_CALL, OAuthProxyCallClass))

#define OAUTH_IS_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OAUTH_TYPE_PROXY_CALL))

#define OAUTH_IS_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), OAUTH_TYPE_PROXY_CALL))

#define OAUTH_PROXY_CALL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OAUTH_TYPE_PROXY_CALL, OAuthProxyCallClass))

typedef struct {
  RestProxyCall parent;
} OAuthProxyCall;

typedef struct {
  RestProxyCallClass parent_class;
} OAuthProxyCallClass;

GType oauth_proxy_call_get_type (void);

G_END_DECLS

#endif /* _OAUTH_PROXY_CALL */

