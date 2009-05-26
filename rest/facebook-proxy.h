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

#ifndef _FACEBOOK_PROXY
#define _FACEBOOK_PROXY

#include <rest/rest-proxy.h>

G_BEGIN_DECLS

#define FACEBOOK_TYPE_PROXY facebook_proxy_get_type()

#define FACEBOOK_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FACEBOOK_TYPE_PROXY, FacebookProxy))

#define FACEBOOK_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FACEBOOK_TYPE_PROXY, FacebookProxyClass))

#define FACEBOOK_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FACEBOOK_TYPE_PROXY))

#define FACEBOOK_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FACEBOOK_TYPE_PROXY))

#define FACEBOOK_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FACEBOOK_TYPE_PROXY, FacebookProxyClass))

typedef struct _FacebookProxyPrivate FacebookProxyPrivate;

/**
 * FacebookProxy:
 *
 * #FacebookProxy has no publicly available members.
 */
typedef struct {
  RestProxy parent;
  FacebookProxyPrivate *priv;
} FacebookProxy;

typedef struct {
  RestProxyClass parent_class;
} FacebookProxyClass;

GType facebook_proxy_get_type (void);

RestProxy* facebook_proxy_new (const char *api_key,
                             const char *app_secret);

RestProxy* facebook_proxy_new_with_session (const char *api_key,
                                        const char *app_secret,
                                        const char *session_key);

const char * facebook_proxy_get_api_key (FacebookProxy *proxy);

void facebook_proxy_set_app_secret (FacebookProxy *proxy, const char *secret);

const char * facebook_proxy_get_app_secret (FacebookProxy *proxy);

const char * facebook_proxy_get_session_key (FacebookProxy *proxy);

void facebook_proxy_set_session_key (FacebookProxy *proxy, const char *session_key);

char * facebook_proxy_sign (FacebookProxy *proxy, GHashTable *params);

char * facebook_proxy_build_login_url (FacebookProxy *proxy, const char *frob);

char * facebook_proxy_build_permission_url (FacebookProxy *proxy, const char *perms);

G_END_DECLS

#endif /* _FACEBOOK_PROXY */
