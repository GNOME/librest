/* rest.h
 *
 * Copyright 2021-2022 Günther Wagner <info@gunibert.de>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define REST_INSIDE
# include <rest/rest-enum-types.h>
# include <rest/rest-oauth2-proxy.h>
# include <rest/rest-oauth2-proxy-call.h>
# include <rest/rest-param.h>
# include <rest/rest-params.h>
# include <rest/rest-pkce-code-challenge.h>
# include <rest/rest-proxy.h>
# include <rest/rest-proxy-auth.h>
# include <rest/rest-proxy-call.h>
# include <rest/rest-utils.h>
# include <rest/rest-xml-node.h>
# include <rest/rest-xml-parser.h>
#undef REST_INSIDE

G_END_DECLS
