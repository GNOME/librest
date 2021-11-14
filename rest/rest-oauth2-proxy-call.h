/* rest-oauth2-proxy-call.h
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <rest.h>

G_BEGIN_DECLS

#define REST_TYPE_OAUTH2_PROXY_CALL (rest_oauth2_proxy_call_get_type())

G_DECLARE_DERIVABLE_TYPE (RestOAuth2ProxyCall, rest_oauth2_proxy_call, REST, OAUTH2_PROXY_CALL, RestProxyCall)

struct _RestOAuth2ProxyCallClass {
  RestProxyCallClass parent_class;
};

enum {
  REST_OAUTH2_PROXY_ERROR_ACCESS_TOKEN_EXPIRED,
};

#define REST_OAUTH2_ERROR rest_oauth2_error_quark ()

G_END_DECLS
