/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *          Günther Wagner <info@gunibert.de>
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

#define FLICKR_PROXY_ERROR flickr_proxy_error_quark()
#define FLICKR_TYPE_PROXY flickr_proxy_get_type()

G_DECLARE_DERIVABLE_TYPE (FlickrProxy, flickr_proxy, FLICKR, PROXY, RestProxy)

struct _FlickrProxyClass {
  RestProxyClass parent_class;

  /*< private >*/
  /* padding for future expansion */
  gpointer _padding_dummy[8];
};


RestProxy     *flickr_proxy_new                 (const char   *api_key,
                                                 const char   *shared_secret);
RestProxy     *flickr_proxy_new_with_token      (const char   *api_key,
                                                 const char   *shared_secret,
                                                 const char   *token);
const char    *flickr_proxy_get_api_key         (FlickrProxy  *proxy);
const char    *flickr_proxy_get_shared_secret   (FlickrProxy  *proxy);
const char    *flickr_proxy_get_token           (FlickrProxy  *proxy);
void           flickr_proxy_set_token           (FlickrProxy  *proxy,
                                                 const char   *token);
char          *flickr_proxy_sign                (FlickrProxy  *proxy,
                                                 GHashTable   *params);
char          *flickr_proxy_build_login_url     (FlickrProxy  *proxy,
                                                 const char   *frob,
                                                 const char   *perms);
gboolean       flickr_proxy_is_successful       (RestXmlNode  *root,
                                                 GError      **error);
RestProxyCall *flickr_proxy_new_upload          (FlickrProxy  *proxy);
RestProxyCall *flickr_proxy_new_upload_for_file (FlickrProxy  *proxy,
                                                 const char   *filename,
                                                 GError      **error);

G_END_DECLS
