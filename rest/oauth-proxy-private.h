#include "oauth-proxy.h"

#define PROXY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), OAUTH_TYPE_PROXY, OAuthProxyPrivate))

typedef struct {
  char *consumer_key;
  char *consumer_secret;
  char *token;
  char *token_secret;
} OAuthProxyPrivate;
