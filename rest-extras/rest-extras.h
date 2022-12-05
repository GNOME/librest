/* rest-extras.h
 *
 * Copyright 2022 Corentin Noël <corentin.noel@collabora.com>
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

#define REST_EXTRA_INSIDE
# include <rest-extras/flickr-proxy.h>
# include <rest-extras/flickr-proxy-call.h>
# include <rest-extras/lastfm-proxy.h>
# include <rest-extras/lastfm-proxy-call.h>
# include <rest-extras/youtube-proxy.h>
#undef REST_EXTRA_INSIDE

G_END_DECLS
