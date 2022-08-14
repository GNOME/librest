/* rest-oauth2-proxy-call.h
 *
 * Copyright 2021-2022 Günther Wagner <info@gunibert.de>
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
 */

#pragma once

#include <glib-object.h>
#include <rest/rest-proxy-call.h>

G_BEGIN_DECLS

#define REST_TYPE_OAUTH2_PROXY_CALL (rest_oauth2_proxy_call_get_type())

G_DECLARE_DERIVABLE_TYPE (RestOAuth2ProxyCall, rest_oauth2_proxy_call, REST, OAUTH2_PROXY_CALL, RestProxyCall)

struct _RestOAuth2ProxyCallClass {
  RestProxyCallClass parent_class;
};

G_END_DECLS
