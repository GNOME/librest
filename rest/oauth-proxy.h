/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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

/**
 * OAuthProxy:
 *
 * #OAuthProxy has no publicly available members.
 */
typedef struct {
  RestProxy parent;
} OAuthProxy;

typedef struct {
  RestProxyClass parent_class;
} OAuthProxyClass;

typedef enum {
  PLAINTEXT,
  HMAC_SHA1
} OAuthSignatureMethod;

GType oauth_proxy_get_type (void);

RestProxy* oauth_proxy_new (const char *consumer_key,
                            const char *consumer_secret,
                            const gchar *url_format,
                            gboolean binding_required);

RestProxy* oauth_proxy_new_with_token (const char *consumer_key,
                                       const char *consumer_secret,
                                       const char *token,
                                       const char *token_secret,
                                       const gchar *url_format,
                                       gboolean binding_required);

typedef void (*OAuthProxyAuthCallback)(OAuthProxy *proxy,
                                           GError        *error,
                                           GObject       *weak_object,
                                           gpointer       userdata);

G_GNUC_DEPRECATED
gboolean oauth_proxy_auth_step (OAuthProxy *proxy,
                                const char *function,
                                GError    **error);

gboolean oauth_proxy_auth_step_async (OAuthProxy *proxy,
                             const char *function,
                             OAuthProxyAuthCallback callback,
                             GObject *weak_object,
                             gpointer user_data,
                             GError **error_out);


gboolean oauth_proxy_request_token (OAuthProxy *proxy,
                                    const char *function,
                                    const char *callback,
                                    GError    **error);


void oauth_proxy_set_verifier (OAuthProxy *proxy, const char *verifier);

gboolean oauth_proxy_access_token (OAuthProxy *proxy,
                                   const char *function,
                                   GError    **error);

/* TODO async forms of request and access token */

const char * oauth_proxy_get_token (OAuthProxy *proxy);

void oauth_proxy_set_token (OAuthProxy *proxy, const char *token);

const char * oauth_proxy_get_token_secret (OAuthProxy *proxy);

void oauth_proxy_set_token_secret (OAuthProxy *proxy, const char *token_secret);

G_END_DECLS

#endif /* _OAUTH_PROXY */
