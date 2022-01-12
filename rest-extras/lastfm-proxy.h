/*
 * librest - RESTful web services access
 * Copyright (c) 2010 Intel Corporation.
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

#pragma once

#include <rest/rest-proxy.h>
#include <rest/rest-xml-parser.h>

G_BEGIN_DECLS

#define LASTFM_TYPE_PROXY lastfm_proxy_get_type()

G_DECLARE_DERIVABLE_TYPE (LastfmProxy, lastfm_proxy, LASTFM, PROXY, RestProxy)

struct _LastfmProxyClass {
  RestProxyClass parent_class;

  /*< private >*/
  /* padding for future expansion */
  gpointer _padding_dummy[8];
};

#define LASTFM_PROXY_ERROR lastfm_proxy_error_quark()

RestProxy  *lastfm_proxy_new              (const char   *api_key,
                                           const char   *secret);
RestProxy  *lastfm_proxy_new_with_session (const char   *api_key,
                                           const char   *secret,
                                           const char   *session_key);
const char *lastfm_proxy_get_api_key      (LastfmProxy  *proxy);
const char *lastfm_proxy_get_secret       (LastfmProxy  *proxy);
const char *lastfm_proxy_get_session_key  (LastfmProxy  *proxy);
void        lastfm_proxy_set_session_key  (LastfmProxy  *proxy,
                                           const char   *session_key);
char       *lastfm_proxy_sign             (LastfmProxy  *proxy,
                                           GHashTable   *params);
char       *lastfm_proxy_build_login_url  (LastfmProxy  *proxy,
                                           const char   *token);
gboolean    lastfm_proxy_is_successful    (RestXmlNode  *root,
                                           GError      **error);

G_END_DECLS
