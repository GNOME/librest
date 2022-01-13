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

#pragma once

#include <rest/rest-proxy.h>

G_BEGIN_DECLS

#define YOUTUBE_TYPE_PROXY youtube_proxy_get_type()

G_DECLARE_DERIVABLE_TYPE (YoutubeProxy, youtube_proxy, YOUTUBE, PROXY, RestProxy)

struct _YoutubeProxyClass {
  RestProxyClass parent_class;

  /*< private >*/
  /* padding for future expansion */
  gpointer _padding_dummy[8];
};

#define YOUTUBE_PROXY_ERROR youtube_proxy_error_quark()

typedef void (*YoutubeProxyUploadCallback)(YoutubeProxy  *proxy,
                                           const gchar   *payload,
                                           gsize          total,
                                           gsize          uploaded,
                                           const GError  *error,
                                           GObject       *weak_object,
                                           gpointer       user_data);

RestProxy *youtube_proxy_new           (const gchar                 *developer_key);
RestProxy *youtube_proxy_new_with_auth (const gchar                 *developer_key,
                                        const gchar                 *user_auth);
void       youtube_proxy_set_user_auth (YoutubeProxy                *proxy,
                                        const gchar                 *user_auth);
gboolean   youtube_proxy_upload_async  (YoutubeProxy                *self,
                                        const gchar                 *filename,
                                        GHashTable                  *fields,
                                        gboolean                     incomplete,
                                        YoutubeProxyUploadCallback   callback,
                                        GObject                     *weak_object,
                                        gpointer                     user_data,
                                        GError                     **error);

G_END_DECLS
