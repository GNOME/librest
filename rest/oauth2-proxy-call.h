/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *          Jonathon Jongsma <jonathon.jongsma@collabora.co.uk>
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

#ifndef _OAUTH2_PROXY_CALL
#define _OAUTH2_PROXY_CALL

#include <rest/rest-proxy-call.h>

G_BEGIN_DECLS

#define OAUTH2_TYPE_PROXY_CALL oauth2_proxy_call_get_type()
G_DECLARE_DERIVABLE_TYPE (OAuth2ProxyCall, oauth2_proxy_call, OAUTH2, PROXY_CALL, RestProxyCall)

/**
 * OAuth2ProxyCall:
 *
 * #OAuth2ProxyCall has no publicly available members.
 */

struct _OAuth2ProxyCallClass {
  RestProxyCallClass parent_class;
  /*< private >*/
  /* padding for future expansion */
  gpointer _padding_dummy[8];
};

G_END_DECLS

#endif /* _OAUTH2_PROXY_CALL */
